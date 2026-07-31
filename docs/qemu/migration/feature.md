## 3. 当前 capability 总览

| C 枚举                                         | QMP 名称                  | 主要作用                             | 最重要的依赖或冲突                            |
| ---------------------------------------------- | ------------------------- | ------------------------------------ | --------------------------------------------- |
| `MIGRATION_CAPABILITY_XBZRLE`                  | `xbzrle`                  | 对重复传输的脏页发送差分             | 冲突 `multifd`、RDMA、`mapped-ram`            |
| `MIGRATION_CAPABILITY_RDMA_PIN_ALL`            | `rdma-pin-all`            | RDMA 迁移时预先 pin 全部 RAM         | 只对 RDMA transport 有意义                    |
| `MIGRATION_CAPABILITY_AUTO_CONVERGE`           | `auto-converge`           | 自动限制 guest CPU 以促进收敛        | 冲突 `dirty-limit`                            |
| `MIGRATION_CAPABILITY_EVENTS`                  | `events`                  | 发出迁移状态和迭代事件               | 无数据面影响                                  |
| `MIGRATION_CAPABILITY_POSTCOPY_RAM`            | `postcopy-ram`            | 目的端提前运行，缺页时向源端取页     | 冲突 RDMA、`mapped-ram`、`x-ignore-shared`    |
| `MIGRATION_CAPABILITY_X_COLO`                  | `x-colo`                  | 连续 checkpoint 的 COLO 容错模式     | 要求 `return-path` 和 replication             |
| `MIGRATION_CAPABILITY_RELEASE_RAM`             | `release-ram`             | postcopy 中释放源端已迁 RAM          | 无法进行 postcopy recovery                    |
| `MIGRATION_CAPABILITY_RETURN_PATH`             | `return-path`             | precopy 也建立目的端到源端的返回通道 | COLO、`switchover-ack` 的前提                 |
| `MIGRATION_CAPABILITY_PAUSE_BEFORE_SWITCHOVER` | `pause-before-switchover` | switchover 前停在可控状态            | 需管理端执行 `migrate-continue`               |
| `MIGRATION_CAPABILITY_MULTIFD`                 | `multifd`                 | 用多个通道并行传 RAM                 | 冲突 XBZRLE、RDMA                             |
| `MIGRATION_CAPABILITY_DIRTY_BITMAPS`           | `dirty-bitmaps`           | 迁移 block node 的命名 dirty bitmap  | 两端 block node/bitmap 映射必须匹配           |
| `MIGRATION_CAPABILITY_POSTCOPY_BLOCKTIME`      | `postcopy-blocktime`      | 统计 postcopy 缺页造成的 vCPU 阻塞   | 只在 postcopy RAM 中有实际价值                |
| `MIGRATION_CAPABILITY_X_IGNORE_SHARED`         | `x-ignore-shared`         | 跳过两端共享的 file-backed RAM       | 实验性；冲突 postcopy；两端必须一致           |
| `MIGRATION_CAPABILITY_VALIDATE_UUID`           | `validate-uuid`           | 目的端核对源端 VM UUID               | 源端必须实际设置 UUID                         |
| `MIGRATION_CAPABILITY_BACKGROUND_SNAPSHOT`     | `background-snapshot`     | 生成迁移开始时刻的一致快照           | 依赖 Linux UFFD-WP，冲突项很多                |
| `MIGRATION_CAPABILITY_ZERO_COPY_SEND`          | `zero-copy-send`          | multifd 发送侧使用零拷贝             | Linux；要求无压缩、无 TLS 的 multifd          |
| `MIGRATION_CAPABILITY_POSTCOPY_PREEMPT`        | `postcopy-preempt`        | postcopy 紧急缺页请求抢占普通页流    | 要求 `postcopy-ram`                           |
| `MIGRATION_CAPABILITY_SWITCHOVER_ACK`          | `switchover-ack`          | 等目的端设备确认后才 switchover      | 要求 `return-path`                            |
| `MIGRATION_CAPABILITY_DIRTY_LIMIT`             | `dirty-limit`             | 按脏页速率配额限制各 vCPU            | 要求 KVM dirty ring；冲突 auto-converge       |
| `MIGRATION_CAPABILITY_MAPPED_RAM`              | `mapped-ram`              | RAM 页写入 migration file 的固定偏移 | 要求可 seek channel；冲突 postcopy、压缩、TLS |

## 4. 逐项分析

### 4.1 `MIGRATION_CAPABILITY_XBZRLE`

XBZRLE 是 “Xor Based Zero Run Length Encoding”。precopy 第一轮仍发送完整
页面；从第二轮开始，源端从 cache 取该页的旧版本，与当前页 XOR，再对差分中的
零游程编码。它适合“同一页反复变脏，但每次只改少量字节”的 workload。

收益不是免费的：源端要维护 page cache、做查找和编码，目的端要解码；cache miss
或编码结果不够小时仍发送整页。可通过 `xbzrle-cache-size` 调 cache，并从
`query-migrate` 的 `xbzrle-cache` 统计判断 cache miss、overflow 和 encoding
rate。详细算法参见 [`xbzrle.rst`](xbzrle.rst)。

当前约束：

- 与 `multifd`、`mapped-ram` 不兼容。
- RDMA transport 拒绝 XBZRLE。
- `zero-copy-send` 拒绝 XBZRLE。
- `background-snapshot` 拒绝 XBZRLE。

主要消费点是 [`migration/ram.c`](../../../migration/ram.c) 的 dirty-page 发送和
XBZRLE cache 生命周期。

### 4.2 `MIGRATION_CAPABILITY_RDMA_PIN_ALL`

它只改变 RDMA migration 的内存注册策略，不会把普通 TCP/file migration 变成 RDMA
migration：

- 关闭时按需注册/pin RAM，启动成本和锁页峰值较低，但迁移过程中可能反复处理
  注册。
- 开启时一次性注册并锁住整个 VM RAM，迁移过程更可预测，但 setup 延迟、
  locked-memory 用量和资源失败风险都更大。

使用它时需正确配置 `memlock` 限额。RDMA transport 本身与 XBZRLE、multifd、
postcopy RAM 不兼容；`background-snapshot` 也拒绝此项。实现和部署要求参见
[`docs/rdma.txt`](../../rdma.txt) 与
[`migration/rdma.c`](../../../migration/rdma.c)。

### 4.3 `MIGRATION_CAPABILITY_AUTO_CONVERGE`

当某轮 guest 新产生的脏数据相对该轮已发送数据过高，并连续触发阈值时，QEMU
开始限制 guest CPU 时间；之后逐级增加 throttle，直到迁移收敛或达到
`max-cpu-throttle`。

相关参数包括：

- `throttle-trigger-threshold`
- `cpu-throttle-initial`
- `cpu-throttle-increment`
- `cpu-throttle-tailslow`
- `max-cpu-throttle`

它用牺牲 guest 计算性能换取收敛，适合脏页率长期接近或超过可用迁移带宽的
precopy。它与 `dirty-limit` 是两种不同的限速控制器，当前代码禁止同时启用；
`background-snapshot` 也不允许它。控制逻辑位于
[`migration/ram.c`](../../../migration/ram.c) 和
[`migration/cpu-throttle.c`](../../../migration/cpu-throttle.c)。

### 4.6 `MIGRATION_CAPABILITY_EVENTS`

启用后，每次 `migrate_set_state()` 成功完成原子状态切换时，QEMU 发出 `MIGRATION`
QMP event；每轮 dirty sync 完成后还会发出 `MIGRATION_PASS` event。

它只影响可观测性，不改变 migration stream，也不替代
`query-migrate`。管理端仍应在重连或怀疑丢 event 时查询当前状态。对象初始化时 对
`NONE` 的直接赋值不会经过 `migrate_set_state()`，因而不是一个状态事件。

### 4.7 `MIGRATION_CAPABILITY_POSTCOPY_RAM`

precopy 先传一部分 RAM；切入 postcopy 后，目的端开始运行 guest。目的端访问
尚未到达的页面时由 userfaultfd 捕获 fault，经 return path 向源端请求该页；
源端同时继续后台推送剩余页面。

优点是可以给高脏页率 workload 一个有界的 switchover 点，缺点是切换之后源端
不再拥有完整、可安全恢复的 guest 状态。新版代码用 `POSTCOPY_DEVICE` 把“设备
状态尚未在目的端加载完、目的端尚未运行”的短窗口单独表示；进入 `POSTCOPY_ACTIVE`
后不能执行普通 cancel。

重要约束：

- 源端和目的端必须都正确配置，目的端还必须通过 host 的 postcopy 支持检查。
- 与 RDMA、`mapped-ram`、`x-ignore-shared`、`background-snapshot` 不兼容。
- CPR modes 不允许 postcopy。
- 实际切换还需在迁移开始后执行 `migrate-start-postcopy`。
- postcopy 活跃期的网络 I/O 失败进入 `POSTCOPY_PAUSED`，可在双方重连后恢复； 但
  `release-ram` 会破坏这种恢复能力。

详细流程参见 [`postcopy.rst`](postcopy.rst)、
[`migration/postcopy-ram.c`](../../../migration/postcopy-ram.c) 和
[`migration/migration.c`](../../../migration/migration.c)。

### 4.8 `MIGRATION_CAPABILITY_X_COLO`

COLO（COarse-Grain LOck Stepping）不是“迁移完成后退出”，而是在 primary 与
secondary 之间持续做 checkpoint，为故障切换维持冗余实例。因此状态会进入
`MIGRATION_STATUS_COLO`，直到 failover 或终止。

当前要求：

- 构建时启用 replication module。
- 同时启用 `return-path`。
- 不与 `background-snapshot` 或 CPR mode 组合。
- COLO 路径不支持 postcopy，实际部署不应把两者组合。
- `x-` 名称和 QAPI 的 `unstable` 标记表示接口仍属实验性。

代码入口位于 [`migration/colo.c`](../../../migration/colo.c)，系统说明位于
[`docs/system/qemu-colo.rst`](../../system/qemu-colo.rst)。

### 4.9 `MIGRATION_CAPABILITY_RELEASE_RAM`

postcopy 中，源端确认页面已进入发送路径后可丢弃相应 RAM，从而降低同机迁移等
场景下源、目的实例同时存在时的内存峰值。

代价是源端不再保有那些页面。一旦网络中断，已经从源端释放但尚未可靠到达目的
端的页面可能永久丢失，所以 `migrate` 的 postcopy resume 路径明确拒绝
`release-ram`。它只应在故障域和传输可靠性都经过评估的 postcopy 场景使用； 在普通
precopy 中没有实际收益。`background-snapshot` 也拒绝它。

### 4.11 `MIGRATION_CAPABILITY_RETURN_PATH`

migration 主数据通道是源端到目的端；return path 是目的端到源端的反向控制
通道。postcopy RAM 会自动需要它，而本 capability 的含义是让普通 precopy 也
建立并使用 return path。

它是以下功能的基础：

- COLO 的双向协调。
- `switchover-ack` 的目的端确认。
- 一些 ping、错误报告和接收 bitmap 等反向消息。

它与 `background-snapshot` 不兼容。transport 还必须真的支持
`qemu_file_get_return_path()`；不能仅因 capability 被接受就假定所有 URI 都有
可用反向通道。

### 4.13 `MIGRATION_CAPABILITY_MULTIFD`

multifd 建立多个数据通道，把 RAM page batch 并行发送，从而利用多核和多队列
网络。`multifd-channels` 控制通道数，`multifd-compression` 选择压缩方法。
当前默认通道数为 2、默认不压缩。

关键约束：

- 必须在目的端 incoming 启动前设置。
- 与 XBZRLE 和 RDMA transport 不兼容。
- 与 `background-snapshot` 不兼容。
- `zero-copy-send` 反过来要求 multifd。
- 可与 `mapped-ram` 组合；此时不同 channel 按文件固定偏移并行写入。
- multifd 改变双方的 channel 建立和 RAM wire path，管理端应在两端一致启用。

实现位于 [`migration/multifd.c`](../../../migration/multifd.c) 及各
`multifd-*.c` backend。

### 4.14 `MIGRATION_CAPABILITY_DIRTY_BITMAPS`

它迁移的是 block layer 的命名 dirty bitmap，常用于增量备份链；不是 RAM 的
迁移脏页 bitmap。

源端把 bitmap metadata 和数据放入 migration stream，目的端按 node/bitmap
名称或显式 alias 恢复。管理端必须保证两端 block graph、node name、bitmap
粒度与持久性语义匹配。失败时可能出现 VM state 已迁完但部分 bitmap 未迁完的
边界情况，因此需要检查最终 migration error 和目的端 bitmap 状态。

它与 `background-snapshot` 不兼容。内部 `migrate_postcopy()` 把 `dirty-bitmaps`
也视为需要 postcopy command 基础设施的功能，但这不等于启用 了 postcopy
RAM。实现位于
[`migration/block-dirty-bitmap.c`](../../../migration/block-dirty-bitmap.c)。

### 4.15 `MIGRATION_CAPABILITY_POSTCOPY_BLOCKTIME`

启用后，目的端记录 vCPU 因 postcopy 缺页而阻塞的时间，并通过 `query-migrate`
暴露总 blocktime、每 vCPU blocktime 及窗口统计。这是性能观测 能力，不会减少
fault 或改变传输策略。

它依赖 postcopy RAM 才有有意义的数据，但 capability 集中校验没有强制两者
绑定。测量本身有 bookkeeping 成本；只在需要诊断 postcopy pause/fault 延迟时
开启。它与 `background-snapshot` 不兼容。

### 4.16 `MIGRATION_CAPABILITY_LATE_BLOCK_ACTIVATE`

历史语义是：目的端加载完成时先不激活 block devices，推迟到真正启动 VM 时再
获取锁，以降低源、目的端短暂争抢存储锁的风险。

当前 `process_incoming_migration_bh()` 已无条件延迟 block activation，并明确
说明旧 capability 的行为现在总是执行。当前树中 `migrate_late_block_activate()`
除 getter 外没有消费点。因此：

- QMP 枚举仍保留它以维持接口兼容。
- 在当前版本打开或关闭都不改变运行行为。
- 新管理软件不应依赖它来判断目的端何时拿 block lock。

### 4.17 `MIGRATION_CAPABILITY_X_IGNORE_SHARED`

它跳过满足以下条件的 RAMBlock：shared、named file-backed，并且被认为在目的端
可访问同一份内容。常见目标是同机更新/CPR 场景，避免复制本来就共享的巨大 内存。

风险在于 QEMU 只知道映射属性，无法证明两端文件内容和语义确实相同。错误使用
会造成静默 guest 内存损坏。因此：

- 它是实验性 capability。
- 与 postcopy RAM 不兼容。
- 源端和目的端状态会写入 stream 并被强制核对，必须一致。
- 管理端必须自行保证 shared backing file 的身份、offset、size、生命周期和
  内容一致。

主要判定在 [`migration/ram.c`](../../../migration/ram.c) 的
`migrate_ram_is_ignored()`。

### 4.18 `MIGRATION_CAPABILITY_VALIDATE_UUID`

启用后，源端在 configuration section 中携带 VM UUID，目的端将其与本地 QEMU
配置的 UUID 比较；不一致则拒绝加载。这可防止编排错误地把 migration stream
接到另一台逻辑 VM。

只有源端实际设置了 UUID 时校验才生效；没有 UUID 时打开 capability 不能提供
身份保证。它验证的是 VM identity，不验证 machine type、CPU compatibility、
磁盘内容或 RAM backing file。它与 `background-snapshot` 不兼容。实现入口在
[`migration/savevm.c`](../../../migration/savevm.c)。

### 4.19 `MIGRATION_CAPABILITY_BACKGROUND_SNAPSHOT`

该模式在开始时短暂停 VM、保存 device state，并利用 Linux userfaultfd
write-protect 跟踪后续写入；RAM 则在 VM 继续运行时保存，从而得到“迁移开始
时刻”的一致快照，而不是普通 precopy 接近结束时刻的状态。

启用时会检查 host kernel 的 write tracking 支持和当前 guest memory layout。
当前集中校验明确拒绝它与以下 capability 同时开启：

`postcopy-ram`、`dirty-bitmaps`、`postcopy-blocktime`、
`late-block-activate`、`return-path`、`multifd`、
`pause-before-switchover`、`auto-converge`、`release-ram`、
`rdma-pin-all`、`xbzrle`、`x-colo`、`validate-uuid`、`zero-copy-send`。

它更接近“生成一致 snapshot stream”，不是普通 live migration 的加速开关。
实现核心在 [`migration/ram.c`](../../../migration/ram.c) 的 write tracking
代码和 [`migration/migration.c`](../../../migration/migration.c) 的
`bg_migration_thread()`。

### 4.20 `MIGRATION_CAPABILITY_ZERO_COPY_SEND`

在 multifd 无压缩发送路径中，Linux 可通过零拷贝 socket 机制减少用户态 copy 和
CPU 消耗。发送完成仍要处理 completion notification，且 guest RAM 需满足
锁页要求，所以它不是“完全没有 CPU 成本”。

当前硬性条件：

- 仅 Linux。
- 必须启用 `multifd`。
- 不能启用 XBZRLE。
- `multifd-compression` 必须是 `none`。
- 不能使用 migration TLS。
- QEMU 必须有足够 locked-memory 权限。
- 与 `background-snapshot` 不兼容。

它主要改善大页吞吐路径；小消息、控制面和设备状态仍会复制。实现位于
[`migration/multifd-nocomp.c`](../../../migration/multifd-nocomp.c)。

### 4.21 `MIGRATION_CAPABILITY_POSTCOPY_PREEMPT`

这是当前版本相对问题枚举新增的 capability。普通 postcopy 中，目的端急需的 fault
page 可能排在后台 page stream 后面；preempt 模式为紧急请求建立快速 通道，使
fault page 抢占普通迁移流，降低 guest fault stall。

约束：

- 必须同时启用 `postcopy-ram`。
- 必须在 incoming migration 启动前设置。
- 会增加 channel 建立、恢复和兼容旧版本 peer 的复杂度。
- 它是性能优化，不改变 postcopy 的一致性模型。

主要实现位于 [`migration/postcopy-ram.c`](../../../migration/postcopy-ram.c) 和
[`migration/channel.c`](../../../migration/channel.c)。


### 4.23 `MIGRATION_CAPABILITY_DIRTY_LIMIT`

这是当前版本相对问题枚举新增的 capability。它基于 KVM dirty ring 统计各 vCPU
脏页速率，并用 `vcpu-dirty-limit`（MB/s）为 vCPU 设置配额。与按 CPU 时间比例逐步
throttle 的 auto-converge 相比，它直接控制脏页生成目标，通常 更适合大 VM
的稳定延迟需求。

硬性约束：

- 必须使用 KVM。
- accelerator 必须配置非零 `dirty-ring-size`。
- 与 `auto-converge` 冲突。
- `vcpu-dirty-limit` 必须至少为 1 MB/s；采样周期由 `x-vcpu-dirty-limit-period`
  控制。

迁移取消或结束时会撤销相应 limit。详见 [`dirty-limit.rst`](dirty-limit.rst) 和
[`system/dirtylimit.c`](../../../system/dirtylimit.c)。

### 4.24 `MIGRATION_CAPABILITY_MAPPED_RAM`

这是当前版本相对问题枚举新增的 capability。传统 migration stream 中 RAM page
是顺序记录；mapped-ram 先为每个 RAMBlock 分配固定文件区域，再按 page offset
写入，因此适合 file migration、并行 I/O、直接 I/O 和后续按位置 访问。

约束：

- channel 必须支持 seek，典型 URI 是 file。
- 与 XBZRLE、postcopy RAM 不兼容。
- migration 启动检查还会拒绝 TLS 和任何 multifd compression。
- 可与无压缩 multifd 组合。
- 源端和目的端 capability 状态会写入 stream 并被强制核对。

固定布局会改变 stream 格式和 channel 操作方式，不能只在单端启用。详见
[`mapped-ram.rst`](mapped-ram.rst)、
[`migration/file.c`](../../../migration/file.c) 和
[`migration/ram.c`](../../../migration/ram.c)。

### 4.22 `MIGRATION_CAPABILITY_SWITCHOVER_ACK`

支持它的目的端 device 可以先加载一部分数据并发送 ACK；
源端只有在待确认计数归零后，才停止仍在运行的源 VM 并进入最终 switchover。
这样可把部分 device load 移出 downtime 窗口。

它要求 `return-path`。当前 VFIO migration 是主要使用者之一。它与
`pause-before-switchover` 不同：

- `switchover-ack` 等待目的端设备的程序化确认。
- `pause-before-switchover` 等待管理端显式 `migrate-continue`。

### 4.12 `MIGRATION_CAPABILITY_PAUSE_BEFORE_SWITCHOVER`

启用后，源端在停止 VM、序列化最终设备状态和切换 block ownership 之前进入
`PRE_SWITCHOVER`，等待管理端：

```json
{
  "execute": "migrate-continue",
  "arguments": { "state": "pre-switchover" }
}
```

这给外部编排系统一个明确的最后协调点。不开启时，状态从 `ACTIVE` 直接进入
`DEVICE`；开启时为 `ACTIVE -> PRE_SWITCHOVER -> DEVICE`。等待期间仍可取消。
管理端漏发 continue 会使迁移一直停在该状态。它与 `background-snapshot` 不兼容。


## 5. 组合选择建议

常见组合可以按目标来选：

| 目标                            | 建议组合                                | 说明                                  |
| ------------------------------- | --------------------------------------- | ------------------------------------- |
| 高带宽、多核 TCP live migration | `multifd`                               | 再按 CPU/带宽选择 multifd compression |
| 降低发送端 CPU copy             | `multifd` + `zero-copy-send`            | Linux、无压缩、无 TLS                 |
| 高脏页率、希望继续 precopy      | `auto-converge` 或 `dirty-limit`        | 二选一；前者控 CPU 时间，后者控脏页率 |
| 高脏页率、接受 postcopy 风险    | `postcopy-ram`，可加 `postcopy-preempt` | 必须规划网络中断与 recovery           |
| file snapshot/restore 吞吐      | `mapped-ram`，可加无压缩 `multifd`      | channel 必须可 seek                   |
| 精确抓取开始时刻快照            | `background-snapshot`                   | 不是普通迁移加速项，兼容组合很少      |
| 编排 switchover 检查点          | `pause-before-switchover`               | 管理端负责 continue/cancel            |
| 降低 device load downtime       | `switchover-ack` + `return-path`        | 需要设备实现配合                      |

最终仍应以 `migrate-set-capabilities` 的实时校验结果、两端 QEMU 版本、构建配置和
transport 能力为准。

## 问题

1. auto-converge 和 dirty-limit 是什么关系，似乎都是做一个事情了

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
