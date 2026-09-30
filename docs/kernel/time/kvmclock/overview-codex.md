# 从 Linux timekeeping 到 KVM clock：沿着一次读时间理解

> [!NOTE]
> 参考神奇海螺的意见，有待验证

本文作为已有 time/kvmclock 笔记的阅读入口。先建立数据流，再回到 masterclock、NTP 和迁移；旧笔记中的实验记录和历史补丁保留原样。

源码基准：本机 `/home/martins3/data/kernel/linux`，HEAD `eb5a10dc0e00`（`git describe`: `v7.2-1-geb5a10dc0e00`）；补充 QEMU `/home/martins3/data/qemu`，HEAD `0d4709b8349a`。下文讨论普通 x86-64 KVM guest，默认稳定 TSC，不包含 nested、Xen/Hyper-V 兼容分支；旧内核的字段、锁和补偿实现可能不同。函数名用于定位，代码片段注明是否为简化模型。

最重要的关系：**kvmclock 用 guest TSC 和 KVM 提供的参数生成纳秒计数；guest Linux 的 timekeeping 再把这个计数变成应用所读的各种时钟。**

## 1. 先把四件不同的事拆开

| 问题 | Linux 机制 | 典型对象 |
| --- | --- | --- |
| 从上次采样到现在，计数增加多少？ | clocksource | TSC、HPET counter、kvm-clock |
| 到指定时刻请通知 CPU | clockevent | LAPIC timer、TSC-deadline |
| 如何从计数生成系统时间？ | timekeeping | `timekeeper`、`tk_read_base` |
| 现在对应公历哪一天，走时准不准？ | 初始墙上时间、校时 | RTC/pvclock wallclock、NTP/chrony |

`hrtimer` 管理软件定时任务，最终依靠 clockevent 安排中断；`jiffies` 是 tick 相关的软件计数。不能把所有时间理解成“每来一次中断就增加一点”。现代读时间路径可以在两次 tick 之间读取硬件计数并插值。

`sched_clock()` 主要服务调度统计和相关时间戳，有自己的实现选择和约束；它可能复用 kvmclock，但不等于 `CLOCK_MONOTONIC` 的 API 路径。

例如 guest 完全可以同时使用 `kvm-clock` 作为 clocksource、LAPIC TSC-deadline 作为 clockevent。`arch/x86/kernel/apic/apic.c` 的 `lapic_next_deadline()` 写入 `MSR_IA32_TSC_DEADLINE`，属于安排通知；它没有读取 pvti 来返回当前系统时间。

## 2. 普通 Linux 怎么从计数得到时间

看 `kernel/time/timekeeping.c` 的 `ktime_get()`：

```c
base = tk->tkr_mono.base;
nsecs = timekeeping_get_ns(&tk->tkr_mono);
return ktime_add_ns(base, nsecs);
```

这里省略了 seqcount 重试。`timekeeping_get_ns()` 通过 `tk_clock_read()` 读 clocksource，再由 `timekeeping_cycles_to_ns()` 换算。正常分支的简化公式是：

```text
delta = (当前 clocksource 读数 - cycle_last) & mask
now = base + ((delta × mult + xtime_nsec) >> shift)
```

`base`、`cycle_last` 和 `xtime_nsec` 描述上次维护的基准；现在的读数用于补上此后的增量。定期维护基准也承担累计时间、处理计数范围和校时等工作，不是专为 NTP 才存在。

`mult/shift` 是定点数换算，避免热路径做一般除法。读数的单位由 clocksource 定义：TSC 返回 cycle，kvm-clock 返回的已经是纳秒。

| 应用时钟 | timekeeping 的解释 |
| --- | --- |
| `CLOCK_MONOTONIC` | 基于 `tkr_mono`；受频率校正影响，不随手工设置日历时间跳变；不计入 Linux suspend |
| `CLOCK_MONOTONIC_RAW` | 基于 `tkr_raw`；不应用本机 NTP 频率校正；不计入 Linux suspend |
| `CLOCK_REALTIME` | 单调时间加 `offs_real`；表示日历时间，可以被设置而跳变 |
| `CLOCK_BOOTTIME` | 单调时间加 `offs_boot`，包含 Linux suspend 的时间 |

`ktime_get_with_offset()` 可以直接看到后三种 offset 的选择，其中还包括 TAI。`tkr_mono` 和 `tkr_raw` 使用同一个选中的 clocksource，但使用不同的换算/累计状态。**RAW 不表示绕过 kvmclock 直接执行 RDTSC，也不保证物理绝对准确。**

注意：这里的 suspend 是 Linux 自身的挂起/恢复。host 没调度某个 vCPU、QEMU stop、热迁移，不应直接套用这组 suspend 语义。

## 3. kvmclock 的读路径：TSC 仍然在最底下

先看 guest 侧 `arch/x86/kernel/kvmclock.c` 的 `kvm_clock`：

```c
.name = "kvm-clock",
.read = kvm_clock_get_cycles,
```

`kvmclock_init()` 用 `clocksource_register_hz(&kvm_clock, NSEC_PER_SEC)` 注册它。这里的 1 GHz 表示“一秒增加十亿个返回单位”，即返回单位是纳秒；不是说 CPU 的 TSC 是 1 GHz。

当 guest 当前 clocksource 是 kvm-clock，内核读单调时间的路径为：

```text
guest: kernel/time/timekeeping.c
  ktime_get()
    timekeeping_get_ns()
      tk_clock_read() → 当前 clocksource 的 read 回调
guest: arch/x86/kernel/kvmclock.c
  kvm_clock_get_cycles()
    kvm_clock_read()
guest: arch/x86/kernel/pvclock.c
  pvclock_clocksource_read_nowd()
    __pvclock_clocksource_read()
      rdtsc_ordered() + __pvclock_read_cycles()
```

核心计算在 `arch/x86/include/asm/pvclock.h` 的 `__pvclock_read_cycles()`：

```c
u64 delta = tsc - src->tsc_timestamp;
u64 offset = pvclock_scale_delta(delta, src->tsc_to_system_mul,
                                src->tsc_shift);
return src->system_time + offset;
```

把它写成更容易记的模型：

```text
K(G) = S₀ + P(G - G₀)

G  = 当前 guest RDTSC 读数
G₀ = pvti.tsc_timestamp，guest TSC 域中的锚点
S₀ = pvti.system_time，锚点对应的 KVM 纳秒计数
P  = 由 tsc_shift 和 tsc_to_system_mul 定义的定点换算
```

`pvclock_scale_delta()` 先按 `tsc_shift` 正负左右移，再乘 `tsc_to_system_mul` 并右移 32 位。不要漏掉减去 `G₀`，也不要把 `tsc_shift` 总写成右移。

例如忽略定点误差，guest TSC 是 2 GHz，锚点为 `(G₀=100 亿 cycles, S₀=30 秒)`。下一次读到 100.02 亿 cycles，结果就是 `30 秒 + 1 毫秒`。共享页即使没有更新，读数仍然随 TSC 前进。

**正常读时间不需要每次 VM exit，也不需要向 QEMU 请求。** guest 执行 RDTSC、读取共享内存并计算即可；RDTSC 是否被配置成陷出是另一个问题。应用的 `clock_gettime()` 还可能使用 vDSO，连 guest 系统调用都省掉。

`arch/x86/include/asm/vdso/gettimeofday.h` 的 `vread_pvclock()` 展示了 pvclock 的 vDSO 路径及 stable 条件。不要把上面的内核调用链当成每次用户读时间的必经路径。

## 4. 谁提供 pvti？两个 MSR 各自做什么？

guest 的 `kvm_register_clock()` 做的是：

```c
pa = slow_virt_to_phys(&src->pvti) | 0x01ULL;
wrmsrq(msr_kvm_system_time, pa);
```

guest 为 vCPU 准备共享结构，把 **guest physical address 加 enable bit** 写进 MSR。host KVM 随后向这块 guest RAM 写参数。

| 接口 | 注册的内存 | 用途 |
| --- | --- | --- |
| `MSR_KVM_SYSTEM_TIME_NEW` | `pvclock_vcpu_time_info`，简称 pvti | TSC → KVM 纳秒计数的锚点、比例、版本和标志 |
| `MSR_KVM_WALL_CLOCK_NEW` | `pvclock_wall_clock` | 给 KVM 计数配上日历时间的 epoch |

因此，旧笔记 [yes-we-know.md](../yes-we-know.md) 中“`MSR_KVM_WALL_CLOCK_NEW` 存储 pvti 地址”的说法应改为 `MSR_KVM_SYSTEM_TIME_NEW`。MSR 也不是存着当前纳秒时间等 guest 每次读取。

host 的 `arch/x86/kvm/x86.c` 中，`kvm_guest_time_update()` 准备参数，`kvm_setup_guest_pvclock()` 发布到共享内存。发布时版本变奇数，写完变偶数；guest 前后检查版本，避免拿到新锚点配旧比例。这个版本协议保证快照一致性，不负责日历校准。

官方 ABI 也明确区分这两个结构以及更新规则：[KVM-specific MSRs](https://docs.kernel.org/virt/kvm/x86/msr.html)。

## 5. 再拆开三个容易混淆的换算

```text
host TSC H
  │ KVM 的 TSC scaling + offset
  ▼
guest TSC G
  │ pvclock：S₀ + P(G - G₀)
  ▼
kvm-clock 返回的纳秒 K
  │ guest timekeeping：基准、增量、频率校正、各类 offset
  ▼
guest CLOCK_MONOTONIC / RAW / REALTIME / BOOTTIME
```

第一层见 host `kvm_read_l1_tsc()`：

```c
return vcpu->arch.l1_tsc_offset +
       kvm_scale_tsc(host_tsc, vcpu->arch.l1_tsc_scaling_ratio);
```

这层决定 guest 执行 RDTSC 看见什么，也服务直接选用 TSC 的 guest。不能把 `kvm_synchronize_tsc()` 理解成只有 kvmclock 才需要的机制。

第二层由 host `kvm_guest_time_update()` 生成：

```c
tsc_timestamp = kvm_read_l1_tsc(v, host_tsc);
hv_clock.tsc_timestamp = tsc_timestamp;
hv_clock.system_time = kernel_ns + v->kvm->arch.kvmclock_offset;
```

特别注意两个不同单位的 offset：

- `l1_tsc_offset`：guest TSC 的计数偏移，单位 cycles。
- `kvmclock_offset`：KVM 纳秒时间轴相对于 host 基准的偏移，单位 ns。

第三层才包含 guest NTP 校正。kvmclock 的纳秒计数不是 guest `CLOCK_REALTIME`，也不应直接等同于 guest `CLOCK_MONOTONIC`。

## 6. masterclock 是为所有 vCPU 选一对共同锚点

host 的 `pvclock_update_vm_gtod_copy()` 前面有一整段跨 CPU 单调性的推导：即便 TSC 同步，各 vCPU 独立采样 host 时间和 TSC，采样关系及换算误差也可能使两个换算公式不完全一致。任务从 vCPU0 换到 vCPU1 后，就可能看到较小的读数。

masterclock 让同一 VM 使用共同的 host 快照：

```text
kvm->arch.master_cycle_now  = H₀
kvm->arch.master_kernel_ns  = B₀

各 vCPU：
  G₀ = 对 H₀ 应用自己的 guest TSC 变换
  S₀ = B₀ + kvmclock_offset
```

它是 **每 VM 一组协调更新的参考值**。guest 仍有各 vCPU 的 pvti；不是让 guest 每次回 host 读一个全局变量。

需要分清三种作用域：

| 状态 | 作用域 | 含义 |
| --- | --- | --- |
| `pvclock_gtod_data` | host 全局 | host timekeeping 状态的镜像，供 KVM 换算/成对采样 |
| `master_kernel_ns/master_cycle_now` | 每 VM | 该 VM 的共同参考快照 |
| pvti | 每 vCPU | 发布给 guest 的换算参数 |

当前源码中，masterclock 的启用检查不仅看 host TSC：还看 vCPU TSC 是否匹配、是否观察到 TSC 倒退，以及旧 pvclock 接口的兼容条件。`kvm_track_tsc_matching()` 中的 generation 用于跟踪 vCPU 属于哪一组同步后的 TSC 状态；不是 pvti 的发布版本号。

`kvm_update_masterclock()` 通过 `kvm_start_pvclock_update()` / `kvm_end_pvclock_update()` 协调更新和 vCPU 重新进入 guest，并请求各 vCPU 刷新 pvti。读取端的版本一致性和跨 vCPU 时间一致性是两层不同问题。

使用 masterclock 时，host 在 pvti 中设置 `PVCLOCK_TSC_STABLE_BIT`。guest 的 `__pvclock_clocksource_read()` 在认可该标志时直接返回计算值；否则借助全局 `last_value` 和原子操作防止返回值倒退。这个兜底无法把错误频率变准确，也不能修复任意大幅跳变。

## 7. host NTP 为什么不等于 guest 自动校时？

当前 x86-64 源码 `get_kvmclock_base_ns()` 的基准是：

```c
return ktime_to_ns(ktime_add(ktime_get_raw(), pvclock_gtod_data.offs_boot));
```

即 **host RAW 加 Linux suspend 累计偏移**；不要把它写成受 NTP 调频的 `CLOCK_BOOTTIME`。`do_kvmclock_base()` 从 host timekeeping 镜像计算同类时间，同时取得对应的 TSC，供 masterclock 使用。

因此有两个分开的校时域：host 的 NTP 调 host timekeeping；guest 的 NTP 调 guest timekeeping。稳定 TSC 常规路径中，host 设置日历时间不会作为“每次 pvti 刷新时的时间差”自动灌进 guest 的运行中 `CLOCK_REALTIME`。

墙上时间是另一条路径：host `kvm_get_wall_clock_epoch()` 概念上计算 `host realtime - 当前 KVM 纳秒计数`。guest `kvm_get_wallclock()` 注册墙上时间结构，再由 `pvclock_read_wallclock()` 将 epoch 与当前 pvclock 读数相加，获得日历时间样本，供初始化等平台读墙钟场景使用。

它不是一个持续运行的 guest NTP 客户端。重新读墙上时间接口、guest agent 设置时间、迁移恢复、host suspend 等路径需要分别分析。旧笔记“host NTP 调整后，直到同步的时候才传过去”过于含糊，必须指出是什么同步接口。

`kvm_get_wall_clock_epoch()` 的源码注释还解释了为什么 epoch 要在查询时计算：host realtime 受 NTP 和闰秒影响，而 KVM 的 TSC 换算时间轴没有相同的调整，两者的差并非永远固定。

## 8. 为什么刷新 masterclock 反而可能造成小跳变？

旧笔记 [mono-raw.md](mono-raw.md) 中引用的提交 `c52ffadc65e2`，主题是避免 vCPU 热插拔触发不必要的 masterclock 更新。这个问题不要求 NTP 出错。

简化地设旧锚点为 `(H₀, B₀)`，在 H₁ 时刷新：

```text
刷新前：K_old = B₀ + offset + P(H₁ - H₀)
刷新后：K_new = B₁ + offset
若 host 的增量换算为 Q，则 B₁ ≈ B₀ + Q(H₁ - H₀)

跳变量 ≈ Q(H₁ - H₀) - P(H₁ - H₀)
```

这里为说明舍入问题暂时忽略 guest TSC scaling/offset。host timekeeping 与 pvclock 的定点表示和舍入不同，P、Q 并非严格相等；更新锚点时就可能暴露积累的差。代码提交描述明确指出 pvclock 换算有精度损失。

所以 masterclock 可以同时解决跨 vCPU 一致性问题，又存在重设基准的误差问题。也不能笼统写成“host uptime 越长必然误差越大”：要看锚点年龄、换算系数和具体更新路径。

## 9. 暂停、迁移与不调度，需要分别判断

**vCPU 暂时没被调度：**在稳定 TSC、没有显式改写时间基准的常规路径中，host TSC 继续走，guest 下次读时间会包含这段间隔。时间已到不代表定时器回调已执行；vCPU 需要重新运行才能处理通知。

**QEMU stop/cont：**不能仅凭“TSC 在走”推断 guest kvmclock 包含全部暂停时间。本机 QEMU `hw/i386/kvm/clock.c` 的 `kvmclock_vm_state_change()` 停止时保存时钟，恢复时用 `KVM_SET_CLOCK` 写回保存值；恢复结构的 flags 为零。显式暂停期间的全部 host 墙钟间隔并未通过这个调用自动补入。

**热迁移：**源和目标 host 的 TSC 起点、host uptime 乃至 TSC 频率都可能不同。必须让目标建立与源 guest 时间衔接的映射，不能直接采用目标 host 的 uptime。

host `kvm_vm_ioctl_set_clock()` 的关键是：

```c
ka->kvmclock_offset = data.clock - now_raw_ns;
```

目标 host 用自己的基准减出一个新 offset，使 KVM 时间达到 userspace 指定的目标值。TSC 状态也需要恢复/适配，不能只设置这个 ns offset 就认为任意 guest TSC 使用都安全。

`KVM_SET_CLOCK` 支持 `KVM_CLOCK_REALTIME`，可根据提供的 realtime 样本补偿正的墙钟间隔，但是否使用由调用方决定。不能把 API 支持误写成 QEMU 总会这样调用。

本机 QEMU 的 `kvmclock_pre_save()` 对正在迁移的运行中 VM 再取一次时间，把停止到 pre-save 之间的一部分间隔计入；它和已处于 paused 状态的 VM 区别处理。迁移停顿是否全部反映到 guest 时间，需要检查具体路径，不能仅从 kvmclock ABI 下结论。

`PVCLOCK_GUEST_STOPPED` 则用于帮助 guest 处理长暂停后的 watchdog 判断。`pvclock_touch_watchdogs()` 能看到实际动作；它不负责补回时间、调整比例或修复倒退。

所以 [kvmclock.md](kvmclock.md) 中“迁移后完全按照新机器时间算，只要新机器没问题就没问题”需要纠正：**目标的基准必须经过状态恢复和 offset 转换来延续源 VM 的时间轴。**

## 10. 回到源码时的阅读顺序

1. guest `kernel/time/timekeeping.c`：`ktime_get()`、`timekeeping_get_ns()`、`timekeeping_cycles_to_ns()`、`ktime_get_with_offset()`。先回答应用时钟如何从 clocksource 得来。
2. guest `arch/x86/kernel/kvmclock.c`：`kvm_clock`、`kvm_clock_read()`、`kvm_register_clock()`、`kvmclock_init()`。区分注册、读取和平台初始化钩子。
3. guest `arch/x86/include/asm/pvclock.h`：`__pvclock_read_cycles()`、`pvclock_scale_delta()`。把公式写出来。
4. guest `arch/x86/kernel/pvclock.c`：`__pvclock_clocksource_read()`、`pvclock_read_wallclock()`。区分快照一致性、单调性兜底和 epoch。
5. host `arch/x86/kvm/x86.c`：`kvm_guest_time_update()`、`kvm_setup_guest_pvclock()`。反推公式里的每个字段由谁生成。
6. 同文件：`get_kvmclock_base_ns()`、`do_kvmclock_base()`、`pvclock_update_vm_gtod_copy()`、`kvm_update_masterclock()`。再理解共同锚点及更新协议。
7. 同文件：`kvm_read_l1_tsc()`、`kvm_track_tsc_matching()`、`kvm_vm_ioctl_set_clock()`、`kvm_get_wall_clock_epoch()`。最后区分 TSC 同步、迁移和墙上时间。
8. QEMU `hw/i386/kvm/clock.c`：`kvmclock_vm_state_change()`、`kvmclock_pre_save()`、`kvm_update_clock()`。验证具体 userspace 生命周期策略。

已有 [kvmclock3.md](kvmclock3.md) 和 [master-clock.md](master-clock.md) 留作历史补丁与深挖材料，不必从所有 generation/last/cur 字段开始阅读。

另一个已有观察是对的：`clocksource=tsc` 不等于禁用整个 kvmclock 初始化。`kvmclock_init()` 还设置 TSC 频率校准、墙上时间和 sched_clock 等钩子；当前 clocksource 选择与是否使用这些辅助能力是不同问题。

以后遇到“时间不准”，先写下观测的是 guest RDTSC、kvm-clock 返回值、guest RAW、MONOTONIC 还是 REALTIME；再找变的是 TSC 映射、pvti 锚点/比例，还是 guest timekeeping。这样才能让函数、日志和实际现象落到同一层。

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
