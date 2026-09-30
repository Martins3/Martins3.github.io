# QEMU/KVM 的 kvmclock 热迁移：保存时间值，在目标重新建立映射

本文对应本地源码：QEMU `/home/martins3/data/qemu` 的 `0d4709b8349a`；
Linux `/home/martins3/data/kernel/linux` 的 `eb5a10dc0e00`。
讨论普通 x86 KVM、precopy 热迁移、guest 使用 kvm-clock 的路径，先假定 host TSC 稳定、masterclock 可用。
不是一次实际迁移实验；不展开 postcopy、nested、机密 VM、CPR 或 Xen/Hyper-V 兼容路径。

核心结论：**源端保存 guest TSC 状态和 kvmclock 纳秒值；目标端恢复 guest TSC，再以迁来的纳秒值为目标重建 kvmclock_offset、masterclock 和 pvti。源 host 的 uptime 与物理 TSC 起点不需要和目标相同。**

## 1. 先明确需要延续的两条时间轴

简化模型（省略定点舍入）：

```text
G(H) = scale(H) + O_tsc
K(G) = S₀ + P(G - G₀)

H：host TSC
G：guest RDTSC 的返回值
O_tsc：l1_tsc_offset，单位 cycles
G₀：pvti.tsc_timestamp，guest TSC 域
S₀：pvti.system_time，单位 ns
P：pvti 的 mult/shift 换算
```

host 生成 S₀ 时又有：

```text
S₀ = B₀ + O_clock
B₀：host 提供的参考纳秒值
O_clock：kvm->arch.kvmclock_offset，单位 ns
```

`KVM_SET_MSRS(MSR_IA32_TSC)` 主要恢复 G 的映射，`KVM_SET_CLOCK` 恢复 K 的基准。**只恢复 G 不会告诉目标 KVM：这个 TSC 计数对应源 VM 时间轴上的多少纳秒。**

guest RAM 中虽然包含旧 pvti，但目标 KVM 还必须建立自己的时钟状态，以便后续更新 pvti；不能永久沿用源 host 的参数。

## 2. 源端：先停 vCPU，再保存 TSC 和 kvmclock

QEMU `migration/migration.c`：

```text
migration_completion_precopy()
  migration_stop_vm(..., RUN_STATE_FINISH_MIGRATE)
    vm_stop_force_state()
  qemu_savevm_state_complete_precopy()
```

`system/runstate.c` 的 `do_vm_stop()` 先 `pause_all_vcpus()`，再 `vm_state_notify(false, state)`。kvmclock 设备在 `hw/i386/kvm/clock.c` 的 `kvmclock_realize()` 注册了状态通知，于是进入：

```text
kvmclock_vm_state_change(running=false)
  kvm_synchronize_all_tsc()
  kvm_update_clock()
  s->clock_valid = true
```

`target/i386/kvm/kvm.c` 中的 `kvm_synchronize_all_tsc()` 在各 vCPU 线程上调用 `kvm_get_tsc()`，读取 `MSR_IA32_TSC` 存入 `env->tsc`。停止状态下 `env->tsc_valid` 可避免重复采样覆盖这个值。

这里各 vCPU 不是在物理上完全同一时刻取 TSC；这也是恢复端需要 TSC 同步逻辑，而不只是独立写几个寄存器的原因。

`kvm_update_clock()` 则调用 VM ioctl：

```c
ret = kvm_vm_ioctl(kvm_state, KVM_GET_CLOCK, &data);
s->clock = data.clock;
s->clock_is_reliable = kvm_has_adjust_clock_stable();
```

它取得的是 **kvmclock 当前纳秒计数**，不是 guest `CLOCK_REALTIME`，也不是直接读取 guest `timekeeper`。

## 3. 源内核的 GET_CLOCK 为什么能在 guest 停止后继续取时间？

Linux `arch/x86/kvm/x86.c`：

```text
kvm_vm_ioctl_get_clock()
  get_kvmclock()
    __get_kvmclock()
```

masterclock 路径中，`__get_kvmclock()` 构造一份临时 pvclock 参数：

```c
hv_clock.tsc_timestamp = ka->master_cycle_now;
hv_clock.system_time = ka->master_kernel_ns + ka->kvmclock_offset;
/* 换算比例由 host TSC 频率生成 */
data->clock = __pvclock_read_cycles(&hv_clock, data->host_tsc);
```

此处临时结构的 TSC 域是 **host TSC**；发布给 guest 的 pvti 使用 **guest TSC**。二者都用同一类插值公式，但不要混用字段的坐标域。

KVM 用当前 host TSC 从共同基准外推，guest 不执行也能算。源 QEMU 停 vCPU 没有让 host TSC 停止，也没有自动冻结内核内部的 kvmclock。

没有 masterclock 时，此源码退回 `get_kvmclock_base_ns() + kvmclock_offset`。在当前 x86-64 实现中，该 base 是 RAW 加 suspend 偏移；QEMU 旧注释/API 中的 `CLOCK_MONOTONIC` 描述不要机械当成当前实现。

## 4. pre_save 再取一次：计入停机后的一部分时间

QEMU `migration/savevm.c` 的 `qemu_savevm_state_complete_precopy()` 先完成可迭代部分，再保存非迭代设备状态。kvmclock 的 VMState 设置了 `kvmclock_pre_save()`：

```c
if (!s->runstate_paused) {
    kvm_update_clock(s);
}
```

对从 running 状态进入普通热迁移的 VM，再取一次更晚的 clock 值。源端停止到 pre_save 之间，最后一批 RAM 等数据的处理花费了一段时间；这段时间通过更晚的 GET_CLOCK 计入迁移值。

`clock_valid` 避免普通停机回调反复抓取，不会阻止这个 pre_save 显式刷新。若迁移的是原本就由用户暂停的 VM，`runstate_paused` 使它保留先前冻结的值。

因此，“pre_save 让时间向前跳一大步”要加上上下文：guest 没运行期间现实时间确实经过了；这里在保存值中计入这段流逝，不等于错误地额外加时间。

## 5. 迁移流里到底包含什么？

| 数据 | QEMU 保存位置 | 用途 |
| --- | --- | --- |
| `env.tsc` | `target/i386/machine.c` 的 `vmstate_x86_cpu` | 各 vCPU 的 guest TSC 状态 |
| `env.tsc_khz` | `vmstate_tsc_khz` 子节，非零时存在 | 请求目标延续 guest TSC 频率 |
| `env.system_time_msr`、`env.wall_clock_msr` | CPU VMState | pvti/墙钟结构所在 guest physical address 与控制位 |
| `KVMClockState.clock` | `kvmclock_vmsd` | VM 级的 kvmclock 纳秒目标值 |
| `clock_is_reliable` | `kvmclock/clock_is_reliable` 子节 | 兼容旧源 KVM 的恢复策略 |
| pvti 及 guest timekeeper 等 | guest RAM | guest 内存状态与旧参数，供恢复及旧路径使用 |

这个 kvmclock VMState 没有直接搬运源 KVM 的 `master_kernel_ns`、`master_cycle_now`、`kvmclock_offset`，也没有保存完整 `kvm_clock_data` 的 `realtime/host_tsc`。

目标 host 的物理 TSC、uptime 是另一套坐标；重新算偏移比照搬源端 host 内部字段更合理。

## 6. 目标：恢复 CPU 状态，先处理 TSC 频率和计数

QEMU `migration/savevm.c` 的 `qemu_loadvm_state()` 加载状态后调用 `cpu_synchronize_all_post_init()`。KVM 后端最终走：

```text
accel/kvm/kvm-all.c:
  do_kvm_cpu_synchronize_post_init()
    kvm_cpu_synchronize_put(..., KVM_PUT_FULL_STATE, ...)
target/i386/kvm/kvm.c:
  kvm_arch_put_registers()
    kvm_arch_set_tsc_khz()
    ...
    kvm_put_msrs()
```

`kvm_arch_set_tsc_khz()` 尝试 `KVM_SET_TSC_KHZ`，允许硬件通过 TSC scaling 提供迁来的频率。随后 `kvm_put_msrs()` 将迁来的 `MSR_IA32_TSC`、system-time MSR 和 wall-clock MSR 等写入 KVM。

需要避免一个过强结论：这份 QEMU 的 full-state 路径**不检查 `kvm_arch_set_tsc_khz()` 的返回值**；失败可能只警告，并不保证所有频率不匹配都会终止迁移。显式用户频率等约束另有检查，不能把“尝试保频”写成无条件成功。

Linux 对 host 发起的 `MSR_IA32_TSC` 写入走：

```text
kvm_set_msr_common()
  kvm_synchronize_tsc()
    kvm_compute_l1_tsc_offset()
    __kvm_synchronize_tsc()
      kvm_vcpu_write_tsc_offset()
      kvm_track_tsc_matching()
```

概念上：`O_tsc = 希望恢复的 guest TSC - scale(当前 host TSC)`。实际还包含多 vCPU 的同步、generation 和补偿。

`kvm_synchronize_tsc()` 对相近的 userspace TSC 写入有历史启发式：在相同频率等条件下，将约一秒范围内的差异视作旧 API 采样/恢复竞争所致，并匹配 offset。因此不应描述成“每个 vCPU 的 TSC 都精确回到自己的快照值，且同时起跑”。

恢复 system-time MSR 时，`kvm_write_system_time()` 重新关联目标 KVM 对 pvti 的 guest-memory 映射，并请求时钟更新。寄存器里的地址与 RAM 迁移都重要，仅复制 RAM 不会让目标 KVM 自动拥有这个关联。

## 7. vm_start 通知：恢复 KVM 纳秒时间轴

目标 incoming 流程在允许自动启动时进入 `process_incoming_migration_bh()` → `vm_start()`。`system/runstate.c` 的顺序是：

```text
vm_start()
  vm_prepare_start()
    vm_state_notify(true, ...)
      kvmclock_vm_state_change(running=true)
  resume_all_vcpus()
```

也就是 kvmclock 恢复在真正重新运行 vCPU 之前。

先忽略旧 host fallback，这个回调的关键代码是：

```c
struct kvm_clock_data data = {};
data.clock = s->clock;
ret = kvm_vm_ioctl(kvm_state, KVM_SET_CLOCK, &data);
```

Linux `kvm_vm_ioctl_set_clock()`：

```c
kvm_start_pvclock_update(kvm);
pvclock_update_vm_gtod_copy(kvm);
/* 此 QEMU 调用 flags=0，跳过 KVM_CLOCK_REALTIME 补偿分支 */
if (ka->use_master_clock)
    now_raw_ns = ka->master_kernel_ns;
else
    now_raw_ns = get_kvmclock_base_ns();
ka->kvmclock_offset = data.clock - now_raw_ns;
kvm_end_pvclock_update(kvm);
```

设迁来的 clock 是 C，目标当前基准是 B，则新 offset 为 `C-B`。目标新 pvti 的参考纳秒自然是：

```text
system_time = B + (C - B) = C
```

例如源 VM 时间为 104 秒，目标 host 基准为 500 秒，就取 offset −396 秒。源 host 启动多久与此无关。这个 offset 的意义始终是“VM 纳秒域与当前 host 基准之间的差”，迁移后数值改变并没有破坏语义。

masterclock 在目标按目标 host 状态和 vCPU 匹配条件重新选取。后续刷新时，若两套插值完全一致，更新锚点不改变当前时间；实际可能有之前讨论过的定点舍入差，不能承诺数学上的完全无误差连续。

## 8. 最后一次进入 guest 前，重写 pvti

`kvm_end_pvclock_update()` 向所有 vCPU 设置 `KVM_REQ_CLOCK_UPDATE`。Linux `vcpu_enter_guest()` 在硬件进入 guest 前处理 masterclock/global clock/clock update 请求。

`kvm_guest_time_update()` 基于目标状态生成：

```c
tsc_timestamp = kvm_read_l1_tsc(v, host_tsc);
hv_clock.tsc_timestamp = tsc_timestamp;
hv_clock.system_time = kernel_ns + v->kvm->arch.kvmclock_offset;
/* 另设置适合当前 guest TSC 速率的 mul/shift 和 flags */
kvm_setup_guest_pvclock(&hv_clock, v, &vcpu->pv_time, 0);
```

`kvm_setup_guest_pvclock()` 按 version 奇数→写字段→偶数的协议发布。guest 再执行 `__pvclock_read_cycles()` 时，读到的是目标 host 重新建立的参数，而不是永久保留的源 pvti。

guest 自己的 timekeeper 随 RAM 迁来，继续消费新的 clocksource 读数。恢复过程不需要重新启动 guest，也没有通过 `settimeofday()` 把应用日历时间设置成源 host 或目标 host 的当前时间。

## 9. 停顿到底补了多少？

```text
t0 源 guest 停止，kvmclock=100 s，保存 TSC
   | 最后一批 RAM/设备保存前准备，约 4 s
t1 kvmclock_pre_save：GET_CLOCK 得到 104 s
   | 剩余传输、目标加载、恢复准备，约 0.1 s
t2 目标 SET_CLOCK(104 s)，flags=0
   | 从新参考点继续走时
t3 第一次重新运行 guest
```

在正常 masterclock 路径、忽略其他误差时，t0→t1 被计入；t1→t2 没有通过这个 SET_CLOCK 自动补偿；t2→t3 会通过新基准下的 TSC 外推继续计入。guest 从上次约 100 秒读到约 104 秒，并不是无缘无故跳了 4 秒。

`KVM_SET_CLOCK` 的 API 确实支持 `KVM_CLOCK_REALTIME`，当前内核会在 host realtime 晚于传入样本时，将正的差加到 `data.clock`。但本 QEMU kvmclock 恢复路径只传 `clock`、flags 为零，因此**支持这个 API 不等于热迁移路径使用了它**。也不能推论跨 host 墙钟误差会通过该分支自动传入这里。

若目标用 `-S`/禁用 autostart 等方式长期暂停，真正启动时才执行这次恢复，不能期待等待期间全部自动计入。旧 source fallback 也可能保留更早的停机时刻，见下一节。

## 10. clock_is_reliable：区分能力与本次状态

三个不同的检查：

| 名称/位置 | 含义 |
| --- | --- |
| `KVM_CHECK_EXTENSION(KVM_CAP_ADJUST_CLOCK)` 的 `KVM_CLOCK_TSC_STABLE` | KVM 是否具备返回这种精确时钟状态的能力 |
| `KVM_GET_CLOCK` 的 `data.flags & KVM_CLOCK_TSC_STABLE` | 本次返回是否走稳定、共同时间轴的语义 |
| guest pvti 的 `PVCLOCK_TSC_STABLE_BIT` | 给 guest 的跨 vCPU 时间一致性承诺，影响读端兜底 |

QEMU 的 `kvm_has_adjust_clock_stable()` 检查第一个；`s->clock_is_reliable` 保存的也是这项能力，不等于“源 VM 此刻一定启用了 masterclock”。当前 Linux 对能力查询返回 `KVM_CLOCK_VALID_FLAGS`，是能力集合，不是每 VM 状态。

QEMU 为什么仍敢这么写？`kvm_update_clock()` 注释解释了它依赖的分支：新 KVM 在 masterclock 可用时能返回对应时间；不用 masterclock 时，Linux guest 有 `last_value` 原子累积等防倒退兜底。这里是在旧、新 KVM 行为之间选择恢复方法，不是对所有 guest、所有异常做无条件准确性保证。

旧源 KVM 没有该能力时，目标 `kvmclock_vm_state_change()` 调 `kvmclock_current_nsec()`：

```text
迁来的 env->tsc
  + 迁来的 RAM 中旧 pvti
  → 按 guest pvclock 算法计算一个 ns 值
  → 替代 s->clock，交给 KVM_SET_CLOCK
```

QEMU 用 `migration_tsc = env->tsc` 代入旧公式，而不是目标当前 RDTSC。这避免用目标域计数解释源域锚点，也避免旧 GET_CLOCK 的 host-base 时间与 guest 实际插值值不一致。

因为这个 TSC 是停止阶段保留的快照，fallback 不一定计入 pre_save 延后抓取所包含的那段时间。这解释了为什么较新的 GET_CLOCK 能力有助于减少迁移后时间落后。它不是简单读取旧 `system_time`，而是用保存的 TSC 继续做了一次插值。

## 11. KVM_KVMCLOCK_CTRL 的作用

目标 QEMU 在 `SET_CLOCK` 后、vCPU 恢复前，对各 vCPU 发 `KVM_KVMCLOCK_CTRL`。

Linux `kvm_set_guest_paused()` 设置 `pvclock_set_guest_stopped_request` 并请求更新。`kvm_guest_time_update()` 将其发布为 `PVCLOCK_GUEST_STOPPED`。guest pvclock/watchdog 路径消费该标志，帮助避免长暂停被误判为 lockup。

它不是第三种校时算法：TSC 恢复控制 cycles 域，SET_CLOCK 控制 ns 域，KVMCLOCK_CTRL 传递“被 host 暂停过”的信息。

## 12. 回答旧笔记里的几个疑问

- **保存 TSC 为什么不够？** 因为 cycles 和 kvmclock ns 的基准独立，目标下一次重写 pvti 需要自己的 VM 级 ns 状态。
- **直接复制/修改 pvti 是否更简单？** 只能改变 guest-memory 快照，不能替代 KVM 的 offset、频率与多 vCPU 更新状态，下一次更新还会覆盖它。
- **GET/SET_CLOCK 是否保证任意情况下绝不倒退？** 它们提供恢复单调性的机制，需配合一致的保存值、TSC 恢复、pvti 更新与相应 guest 支持；不等于任意传入旧时间也会被自动纠正。
- **为什么需要多 CPU 协调？** 每个 vCPU 的 TSC 是分别保存和写入的；KVM 用同步/generation 和共同 masterclock 把它们协调到同一时间轴，guest 的 version 协议则保证字段快照完整。这是不同层的保证。
- **masterclock 的意义是什么？** 它让各 vCPU 用协调的参考快照产生一致读数，也让 GET_CLOCK 可以在 guest 未运行时计算同一 VM 时间轴；它不是迁移源 host 的一块硬件必须被原样搬走。
- **TSC 连续是否等于 kvmclock 连续？** 不等于。正常新路径中，TSC 快照可能取自 t0，纳秒快照取自 t1；目标会通过新的 pvti 显式重建两者关系。

API 原始语义：[KVM_GET_CLOCK](https://docs.kernel.org/virt/kvm/api.html#kvm-get-clock)、[KVM_SET_CLOCK](https://docs.kernel.org/virt/kvm/api.html#kvm-set-clock)。旧疑问保留在 [kvmclock.qemu.md](kvmclock.qemu.md)，本文按上述固定版本的实际源码作答。

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
