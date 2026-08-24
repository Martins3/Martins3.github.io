# capability 的实现：auto-converge、background-snapshot 与新增功能模板

本文基于 QEMU `0d4709b8349a`。它讨论的不是 QMP 的表面用法，而是 capability 从
API 到 migration data path 的完整落点。

## 先看两种复杂度模型

| capability            | 类型                  | 改变了什么                                                                                     | 主要执行端                               |
| --------------------- | --------------------- | ---------------------------------------------------------------------------------------------- | ---------------------------------------- |
| `auto-converge`       | policy / 控制器       | 仍是普通 precopy stream；只在 dirty-rate 高时限制 source vCPU，促使剩余脏页收敛                | source                                   |
| `background-snapshot` | workflow + 一致性算法 | 放弃 dirty-log 的“多轮收敛到结束时刻”语义，改为 UFFD-WP 保存一个开始时刻的静态 snapshot stream | source；destination 只按普通 stream 载入 |

所以，前者是“在已有迁移循环中加一个策略分支”，后者是“替换迁移线程、内存写入跟踪
和 completion 语义”。新增 capability
之前，先把需求归到这两类之一；这会决定是否要 有对端握手、是否要改变
stream，以及测试范围。

## `auto-converge`：source 端的负反馈控制器

### 注册和配置层

`qapi/migration.json` 的 `MigrationCapability` 枚举定义 QMP 名
`auto-converge`；生成的 C 枚举为
`MIGRATION_CAPABILITY_AUTO_CONVERGE`。`migration/options.c` 中：

- `DEFINE_PROP_MIG_CAP("x-auto-converge", ...)` 提供 migration QOM property；
- `migrate_auto_converge()` 把 `MigrationState.capabilities[]` 的位封装为
  accessor；
- `migrate_caps_check()` 通过检查候选 bitmap，禁止它与 `dirty-limit` 同开；
- 五个 `MigrationParameters` 给控制器提供数值：
  `throttle-trigger-threshold`、`cpu-throttle-initial`、
  `cpu-throttle-increment`、`cpu-throttle-tailslow`、`max-cpu-throttle`。

它没有自己的 stream section，也没有 destination worker。它仍然是
capability，是因为 它会改变迁移时 guest 的性能语义（QEMU 可以主动暂停
vCPU），并与另一个 vCPU 控制器 `dirty-limit` 互斥；数值部分才是 parameter。

### 运行时闭环

```text
RAM dirty-bitmap sync（约每秒）
  │
  ├─ bytes_dirty_period = 本窗口新增脏页数 × TARGET_PAGE_SIZE
  ├─ bytes_xfer_period  = 本窗口实际迁移字节数
  └─ dirty > xfer × throttle-trigger-threshold / 100，连续两次？
          │ 是
          └─ auto-converge -> mig_throttle_guest_down()
                 ├─ 第一次：cpu_throttle_set(cpu-throttle-initial)
                 └─ 后续：增加 increment，最多 max-cpu-throttle
```

触发点在 `migration/ram.c:migration_bitmap_sync()` /
`migration_trigger_throttle()`。 连续两次高脏页率才动作，避免短暂的 burst
导致无谓降速。开启 capability 后， `migration_thread()` 还会启动一个每 5 秒的
dirty-bitmap sync timer，防止迁移循环没有 推进时缺少判断样本。

`cpu-throttle.c` 的实现不是降低 QEMU migration 发送限速，而是给每个 vCPU 注入
sleep：

- 一个工作 slice 是 10 ms；若 throttle 比例为 `p`，每运行 10 ms 后睡眠
  `p / (1 - p) × 10 ms`；完整周期中 sleep 的占比即为 `p`；
- timer 以 `10 / (1 - p)` ms 的周期为各 vCPU 调度该工作；
- `cpu-throttle-tailslow=true` 时，后续增量取“用户设定 increment”和按
  `dirty/xfer` 算出的理想增量中的较小值，减轻尾段过冲。

效果是降低 guest 写内存的执行时间，期望使
`dirty rate < migration throughput`，让 precopy 最终进入 `downtime-limit` 允许的
switchover。它不能保证收敛：若 dirty rate 对 CPU
时间不敏感，或可用带宽太低，即使 throttle 到 99% 也可能失败或不可接受。

### 生命周期和可观测性

- `query-migrate` 在 throttle 已生效时返回 `cpu-throttle-percentage`；
- 迁移结束、失败或取消后，`migration_iteration_finish()` 在 BQL 下调用
  `cpu_throttle_stop()`，同时撤销两个 timer，避免 guest 被永久限速；
- `tests/qtest/migration/precopy-tests.c:test_auto_converge()` 通过制造不收敛的
  workload，验证 initial throttle、增长和清理路径。

这就是新增“只影响 source 调度策略”的 capability 应有的最小闭环：启用点、量测点、
执行点、query 指标、所有 exit path 的回滚。

## `background-snapshot`：冻结旧值，而非重复发送脏页

### 为什么普通 precopy 不能生成开始时刻 snapshot

普通 precopy 先传页、之后不断重传被 guest 修改的页，最后停 VM 并传最后脏页。因此
它输出的是**结束/switchover 时刻**的一致状态。若源 VM 始终运行，要输出开始时刻的
内存，必须在每页第一次被改写前保存它的旧值，不能依赖“修改后再把新值标 dirty”。

`background-snapshot` 借 Linux `userfaultfd` 的
write-protect（UFFD-WP）实现这一点。 它不使用 KVM/QEMU migration dirty
log；写保护页上的第一次 guest write 触发 fault， migration thread
先把该页的旧内容写入 stream，再解除该范围的保护，写入才可继续。

### capability 启用时的拒绝条件

`migration/options.c:migrate_caps_check()` 在 QMP 设置 capability
时就做两类检查：

1. 调用 `ram_write_tracking_available()` 确认 host kernel 有
   `UFFD_FEATURE_PAGEFAULT_FLAG_WP`；再以临时 UFFD FD 逐个尝试注册实际
   RAMBlock，确认 `UFFDIO_REGISTER_MODE_WP` 和 `UFFDIO_WRITEPROTECT` 可用。
2. 检查一个集中维护的冲突集：postcopy、multifd、XBZRLE、RDMA、return path、
   auto-converge、dirty bitmaps、COLO、zero-copy 等都被拒绝；CPR mode
   在迁移开始前也 被拒绝。

冲突的共同原因是这些特性假定了普通 dirty bitmap、多通道/反向通道或最终
switchover 的生命周期；与“单次、按旧值保存、source
继续运行”的模型不兼容。把检查放在 `migrate_caps_check()` 而不是某一个 setter
中，能保证用户以任意顺序、甚至一个 QMP 请求同时设置多个 capability
时，得到同一结论。

### 线程和内存一致性时序

```text
bg_migration_thread（mig/snapshot）
  1. 写 migration header / do_setup
  2. 暂停 source VM，保存 non-RAM device/global state 到内存 buffer
  3. UFFD 注册每个可迁移 RAMBlock，并对它们施加 write-protect
  4. 通过 BH 恢复 source VM（此时 snapshot 时刻已固定）
  5. 扫描并把每个 RAM 页的旧值写到主 stream
     ├─ 正常扫描先写页，随后解除该页范围的保护
     └─ 若 guest 先写，UFFD fault 把该页插到优先队列：先写旧值，再解除保护
  6. 全部 RAM 已保存后，把步骤 2 缓存的 non-RAM bytes 追加到 stream，完成
  7. 无论成功、失败或取消，撤销 WP、注销 UFFD range、唤醒被 fault 阻塞的线程
```

关键落点：

- `migration_start_outgoing()` 选 `MIGRATION_THREAD_SNAPSHOT` /
  `bg_migration_thread()`， 不走普通 `migration_thread()`；
- `ram_write_tracking_prepare()` 在暂停前预触碰每页，避免 `UFFDIO_WRITEPROTECT`
  静默 跳过尚无 PTE 的页；`ram_write_tracking_start()` 再注册、持有 MemoryRegion
  引用并 protect；
- `poll_fault_page()` 从 UFFD 取 write fault；`get_queued_page()` 优先处理它；
  `ram_save_host_page()` 发送后调用 `ram_save_release_protection()`；
- `bg_migration_iteration_finish()` 始终执行 `ram_write_tracking_stop()`。这是
  correctness 条件，不是一般性的资源优化。

因此它并非“完全无停顿”：开始时必须暂停以抓取 device/global state 并建立写保护。
它避免的是与镜像大小成比例的长暂停和最终 switchover；迁移完成后 source VM 早已
恢复并继续运行，stream 是一个可载入的静态 snapshot，而不是 ownership 已转移的
live migration。

## 新增 capability 的实施清单

### 0. 先做设计判定

只有一个连续数值、阈值或速率时优先加 **parameter**；它选择一个可选算法、协议、
线程/通道拓扑、失败恢复模型或 guest-visible 行为时才加
**capability**。典型的设计是 `foo` capability 选择算法，`foo-*` parameters
调节算法。

还要在动代码前写清四件事：

1. source、destination 哪一端消费它，是否必须同值；
2. 是否改变 VMState stream 的格式/顺序，旧 QEMU 对端会怎样；
3. 它依赖和冲突哪些 cap、parameter、transport、host kernel、accelerator；
4. 失败/取消/重连/热拔设备时，应恢复哪些 RAM/CPU/FD/worker 状态。

### 1. 暴露 QMP API

1. 在 `qapi/migration.json:MigrationCapability` **末尾追加** `foo`，写清语义、
   Since、依赖和可能的风险。尚未稳定的管理接口用 `x-foo` 加 `unstable` feature。
   QAPI 会生成 `MIGRATION_CAPABILITY_FOO`、string lookup 和 QMP schema；不要手写
   enum。
2. `migrate-set-capabilities` 和 `query-migrate-capabilities` 已经按
   `MIGRATION_CAPABILITY__MAX` 通用遍历，通常不需要为新名字再写 command
   handler。
3. 若需要 QOM/命令行 property，在 `migration/options.c:migration_properties[]`
   加 `DEFINE_PROP_MIG_CAP("x-foo", MIGRATION_CAPABILITY_FOO)`；这不是 QMP
   的必要条件， 但可让对象初始配置走同一校验。
4. 在 `migration/options.c` 加 `migrate_foo()` accessor，并在 `options.h`
   声明；不要把 `capabilities[]` 的数组下标散落到 RAM、transport、device
   代码中。

### 2. 对完整候选集合做校验

`qmp_migrate_set_capabilities()` 会先把请求合并到 `new_caps[]`，再调用
`migrate_caps_check(old_caps, new_caps)`，成功后才实际写位。新功能的检查应加在这里，
并以 `new_caps` 判断：

```c
if (new_caps[MIGRATION_CAPABILITY_FOO]) {
    if (!host_supports_foo()) {
        error_setg(errp, "foo requires ...");
        return false;
    }
    if (new_caps[MIGRATION_CAPABILITY_BAR]) {
        error_setg(errp, "foo is incompatible with bar");
        return false;
    }
}
```

这使 `foo` 后设或 `bar` 后设都被拒绝。若需求是“destination 的 incoming
启动前必须 设置”，像 multifd / postcopy-preempt 一样检查 incoming state；若是
source host feature， 像 background-snapshot 一样在 source 检查。QOM
初始化也会调用同一 `migrate_caps_check()`，所以检查不得依赖尚未创建的 migration
thread。

### 3. 实现数据面和状态机

按复杂度分三档：

| 新能力类型                                                     | 最小落点                                                                                                                                      |
| -------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| 仅 source policy（类似 auto-converge）                         | 在已有迁移循环的测量/决策点加 `migrate_foo()` 分支；为资源持有者加 setup、成功、失败、取消 cleanup；必要时扩展 `MigrationInfo`。              |
| 双端拓扑或 codec（类似 multifd）                               | source 建通道/发送、destination 在 incoming 前建 worker/decoder、双方一致的 parameter；中途修改必须拒绝或明确无效。                           |
| 改变 snapshot/stream 语义（类似 background-snapshot/postcopy） | 明确 stream 顺序和 receiver parser；在 `migration_start_outgoing()` 选择线程或状态机分支；设计 error/recovery、所有 RAM 保护/页所有权的回滚。 |

若 capability 改变 receiver 对 stream 的解释，不能只加一个 source 的
if：必须设计两端 协商。可在 `migration/savevm.c:should_validate_capability()`
中把它放入 configuration section 的严格 source/target 一致性核对，或仿照
postcopy 发送显式的早期 advise/ack。 当前只有 `x-ignore-shared` 和 `mapped-ram`
使用前一种严格核对；其余 capability 并不会 自动协商。新 feature
必须自己决定并测试旧/新版本组合的行为。

### 4. 测试、文档与兼容性

- QMP：query 中出现新 capability、on/off round trip、非法
  host/parameter/依赖/冲突的 错误；`tests/qtest/migration/misc-tests.c` 已有
  capability-pair 验证模板。
- 功能：一次成功迁移/快照、cancel、I/O error；若创建线程、FD、write protection
  或 vCPU throttle，还要验证它们全部回收。`test_auto_converge()` 是 policy
  feature 的模板。
- 两端：source/destination 一致与不一致、incoming 已启动后设置、旧版本
  peer（若承诺 兼容）以及 transport 组合。
- 观测：若影响收敛、带宽、stall、downtime 或资源占用，扩展 `query-migrate` /
  tracepoint， 否则运维无法判断 capability 实际是否生效。
- 文档：QAPI 注释、`docs/` 的行为/限制、管理端配置顺序。稳定前最好使用 `x-`
  名称， 避免把未经验证的跨版本和恢复语义固化成长期 API。

最容易遗漏的不是“在 QAPI 加一个
enum”，而是对端协议兼容和异常清理。`auto-converge` 证明一个 capability
可以很轻；`background-snapshot` 证明一旦其含义改变“某页到底是
哪个时刻的值”，它就必须有完整的资源生命周期和状态机设计。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
