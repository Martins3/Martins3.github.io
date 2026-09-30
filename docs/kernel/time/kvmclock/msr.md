# x86 KVM 时间相关寄存器
<!-- 09046074-6932-4ce8-a1a8-0904d09e3c8f -->

以普通 x86 KVM guest 为范围，涵盖 Intel VMX 和 AMD SVM；不展开
nested、机密虚拟机和 Hyper-V 兼容时钟。ARM64 的 Generic Timer 是另一套接口，参见
[timer.aarch64.md](../timer.aarch64.md)。

实现核对基于本机 `/home/martins3/data/kernel/linux`，HEAD
`eb5a10dc0e00`（`v7.2-1-geb5a10dc0e00`）。下面用源码文件和函数/宏定位；旧内核的分支与辅助函数可能不同。

### 1. 总览：计数、调整、通知和共享页

| guest 可见接口            | 地址         | 保存或返回什么                                        | 作用                                        |
| ------------------------- | ------------ | ----------------------------------------------------- | ------------------------------------------- |
| `MSR_IA32_TSC`            | `0x10`       | 当前 guest TSC，单位为 TSC tick                       | 读取计数；写入改变 guest TSC 起点           |
| `MSR_IA32_TSC_ADJUST`     | `0x3b`       | TSC 的累计软件调整量，单位为 TSC tick                 | 记录调整；写入时按新旧差值调整 TSC          |
| `MSR_TSC_AUX`             | `0xc0000103` | 软件设置的 32 位辅助标识                              | `RDTSCP`/`RDPID` 返回它，常用于识别 CPU     |
| `MSR_IA32_TSC_DEADLINE`   | `0x6e0`      | guest TSC 域中的绝对截止值                            | 为本 vCPU 的 LAPIC 安排一次定时器中断       |
| `MSR_KVM_SYSTEM_TIME_NEW` | `0x4b564d01` | 共享结构的 guest physical address（GPA）及 enable bit | 注册每 vCPU 的 TSC → ns 换算参数            |
| `MSR_KVM_WALL_CLOCK_NEW`  | `0x4b564d00` | 墙钟共享结构的 GPA                                    | 获取 KVM 时间轴对应的日历时间基准           |
| `MSR_KVM_STEAL_TIME`      | `0x4b564d03` | steal-time 共享结构的 GPA 及 enable bit               | 向 guest 报告 vCPU 被抢占而未运行的累计时间 |

硬件 MSR 编号见 `arch/x86/include/asm/msr-index.h`；KVM 自定义编号见
`arch/x86/include/uapi/asm/kvm_para.h`。这些接口是否可用，要看 guest 获得的
CPUID 能力，不能仅凭 host 有这个 MSR 就在 guest 使用。

TSC tick 不一定等于当前 CPU 核心周期：现代 invariant TSC
通常按固定参考频率前进。`TSC_ADJUST` 改起点，TSC scaling 改速率，`TSC_AUX`
只改标识，`TSC_DEADLINE` 只安排通知。

### 2. `MSR_IA32_TSC`：guest 看到的计数器

guest 读 TSC 有几个入口：

- `RDTSC`：通过 `EDX:EAX` 返回 64 位 TSC。
- `RDTSCP`：通过 `EDX:EAX` 返回 TSC，同时通过 `ECX` 返回 `TSC_AUX`。
- `RDMSR(MSR_IA32_TSC)`：特权方式读取 TSC；不要把它和普通应用使用的 `RDTSC`
  混为一条访问路径。

普通单层虚拟化、启用相应硬件控制时，可以用下面的模型理解：

```text
H = host TSC
G = guest TSC
R = TSC scaling ratio（定点数）
F = ratio 的小数位数
O = TSC offset（单位为 guest TSC tick）

G = ((H × R) >> F) + O       // 按 64 位计数语义处理
不缩放时 R = 1 << F，因而 G = H + O
```

对应 `arch/x86/kvm/x86.c` 的 `kvm_scale_tsc()` 和
`kvm_read_l1_tsc()`。正常情况下，硬件直接完成 `RDTSC` 的变换，不需要每次 VM
exit；是否拦截 `RDTSC` 与是否拦截 `RDMSR` 是不同控制。

guest 执行 `WRMSR(MSR_IA32_TSC, target)`，KVM 调整该 vCPU 的虚拟 TSC
offset，使读数达到目标值。它不会为此把 host 的物理 TSC 改成 guest 指定的值。

当前源码 `kvm_set_msr_common()` 的 `MSR_IA32_TSC` 分支区分两种写入来源：

- guest 执行 `WRMSR`：通过
  `kvm_compute_l1_tsc_offset()`、`adjust_tsc_offset_guest()` 更新
  offset，并累计对应的 `ia32_tsc_adjust_msr` 调整量。
- QEMU 等 userspace 通过 KVM API 设置状态：`host_initiated` 为真，进入
  `kvm_synchronize_tsc()`，还涉及多个 vCPU 的 TSC 同步和恢复语义。

因此，迁移时恢复 TSC 状态不能简单等同于 guest 自己写了一次 TSC。

### 3. `MSR_IA32_TSC_ADJUST`：累计调整量，不是 KVM 的全部 offset

对暴露 `TSC_ADJUST` 能力的 guest，用忽略操作耗时的模型表示联动关系：

```text
原来的 guest TSC = G，TSC_ADJUST = A

写 TSC = X：
    delta = X - G
    guest TSC 变为 X
    TSC_ADJUST 变为 A + delta

写 TSC_ADJUST = B：
    delta = B - A
    guest TSC 增加 delta
    TSC_ADJUST 变为 B
```

例如 `TSC_ADJUST` 从 100 写成 150，TSC 只增加 50 tick；随后把它写回
0，会撤销累计的 150 tick 调整。读取 `TSC_ADJUST` 不会得到当前 TSC。

KVM 在 `vcpu->arch.ia32_tsc_adjust_msr` 保存 guest 的值。`kvm_set_msr_common()`
的 `MSR_IA32_TSC_ADJUST` 分支使用新旧差值调用
`adjust_tsc_offset_guest()`，并请求 `KVM_REQ_CLOCK_UPDATE`，使 pvclock 的
`tsc_timestamp` 配合更新，避免锚点失配造成时间跳变。userspace 恢复该 MSR
时有单独处理，不再按 guest 写入语义重复调整 TSC。

**`TSC_ADJUST` 与 VMCS/VMCB 的 TSC offset 不是同一个状态。** 后者还承担 VM
初始时间轴、host 切换及迁移等映射工作；前者是 guest
架构可见的软件调整记录。不能通过读取 guest `TSC_ADJUST` 推导出完整的 host →
guest offset。

前文提到的两个 Linux 用途分别是：

- `arch/x86/kernel/tsc_sync.c` 的
  `tsc_store_and_check_tsc_adjust()`、`tsc_verify_tsc_adjust()`：检查不同
  CPU、启动/恢复等场景下的调整状态，发现或修正异常 TSC 调整。
- `arch/x86/kernel/tsc.c` 的 `detect_art()`：读取 `TSC_ADJUST` 作为 ART 与 TSC
  转换的 offset。当前实现发现 `X86_FEATURE_HYPERVISOR`
  就提前返回，因此这不是普通 KVM guest 建立 kvmclock 的路径。

### 4. `MSR_TSC_AUX`：读时间时附带的 CPU 标识

`TSC_AUX` 不参与 TSC 的加减、缩放或 ns 换算。它由操作系统写入：Linux 通常编码
CPU/node 信息，guest Linux 设置的也是自己的 vCPU/node 标识，见
`arch/x86/kernel/cpu/common.c` 的 `setup_getcpu()`。

`RDTSCP` 一次返回 TSC 和 AUX，便于把时间戳与执行 CPU 联系起来；`RDPID`
只取得这个标识，不读取时间。前后 AUX 不同可以提示测量期间发生过 CPU
迁移，但相同不能排除迁出后又迁回。

KVM 为每个 vCPU 保存该状态，并通过 MSR 切换或硬件虚拟化支持使 guest
读到自己的值。因此 vCPU 换了 host pCPU 后，AUX 不应自动变成 host pCPU 编号。

源码入口是 `arch/x86/kvm/x86.c` 的 `kvm_set_msr()` / `kvm_get_msr()` 中
`MSR_TSC_AUX` 分支，以及 `arch/x86/kvm/vmx/vmx.c` 的 `vmx_setup_uret_msr()`
调用、`arch/x86/kvm/svm/svm.c` 的 `svm_set_msr()` /
`svm_get_msr()`。普通路径还会借助 user-return MSR 机制恢复 host
的值，不宜概括成每次 VM exit 都立即恢复全部 MSR。

### 5. `MSR_IA32_TSC_DEADLINE`：指定何时通知，不返回当前时间

使用该 MSR 需要 guest 获得 TSC-deadline 能力，并把 LAPIC LVT timer 配置成
deadline 模式。

```text
当前 guest TSC = G
guest TSC 频率 = 2 GHz
希望 1 ms 后触发：写 TSC_DEADLINE = G + 2,000,000
```

写入值是 **guest TSC 的绝对截止值**，不是纳秒，也不是从零开始的倒计数。写 0
取消；触发后需要重新写入才能安排下一次事件。实际中断处理还受屏蔽、vCPU
是否运行等影响，不能把 deadline 到达等同于 guest 回调立即执行。

guest Linux 的入口是 `arch/x86/kernel/apic/apic.c` 的
`lapic_next_deadline()`。host KVM 在 `arch/x86/kvm/lapic.c` 的
`kvm_set_lapic_tscdeadline_msr()` 保存 `lapic_timer.tscdeadline`，然后启动虚拟
LAPIC timer：

- 软件路径 `start_sw_tscdeadline()`：读当前 guest TSC，把剩余 tick 按
  `virtual_tsc_khz` 换算成 ns，用 host hrtimer 等待到期。
- Intel 硬件辅助路径 `arch/x86/kvm/vmx/vmx.c` 的 `vmx_set_hv_timer()`：把 guest
  TSC 差值转换到 host TSC 域，用 VMX preemption timer
  辅助实现。不能使用时回退到软件路径。

所以 guest 写这个 MSR，并不等于把同一个数值原样写入 host 的物理 TSC-deadline
MSR。guest 可以同时用 `kvm-clock` 读时间、用 TSC-deadline 安排中断，两者分属
clocksource 和 clockevent。

LAPIC 还存在传统的计数式 timer 寄存器，定义见 `arch/x86/include/asm/apicdef.h`：

| 寄存器       | xAPIC MMIO 偏移 | x2APIC MSR | 含义                            |
| ------------ | --------------- | ---------- | ------------------------------- |
| `APIC_LVTT`  | `0x320`         | `0x832`    | 中断 vector、mask 和 timer 模式 |
| `APIC_TMICT` | `0x380`         | `0x838`    | initial count，设置初始倒计数   |
| `APIC_TMCCT` | `0x390`         | `0x839`    | current count，读取当前倒计数   |
| `APIC_TDCR`  | `0x3e0`         | `0x83e`    | divide configuration，配置分频  |

x2APIC 编号按 `0x800 + (MMIO offset >> 4)` 映射。这些 count/divider 主要服务
one-shot/periodic 模式；deadline 模式使用 TSC 截止值，不能套用传统倒计数公式。

### 6. kvmclock MSR：登记共享内存，随后读内存与 TSC

`MSR_KVM_SYSTEM_TIME_NEW` 注册每 vCPU 的 `struct pvclock_vcpu_time_info`：guest
写入 4 字节对齐的 GPA，并设置 bit 0 启用；写入 bit 0 为 0
的值停用更新。`MSR_KVM_WALL_CLOCK_NEW` 则接收 `struct pvclock_wall_clock` 的 4
字节对齐 GPA，没有相同的 enable-bit 协议。接口布局和更新约定见
[KVM-specific MSRs](https://docs.kernel.org/virt/kvm/x86/msr.html)。

`SYSTEM_TIME_NEW` 对应的关键共享字段是：

| 字段                             | 单位/用途                                        |
| -------------------------------- | ------------------------------------------------ |
| `tsc_timestamp`                  | guest TSC tick，采样锚点                         |
| `system_time`                    | ns，该锚点对应的 KVM 时间                        |
| `tsc_to_system_mul`、`tsc_shift` | 将 TSC 增量转换为 ns 的定点参数                  |
| `version`                        | 读取前后必须一致且为偶数，防止读到更新一半的数据 |
| `flags`                          | 包括跨 vCPU 稳定性和 guest 暂停提示              |

guest 正常读时间执行下面的计算，而不是反复 `RDMSR(SYSTEM_TIME_NEW)`：

```text
delta = 当前 guest TSC - tsc_timestamp
shift >= 0 时 delta 左移 shift，否则右移 -shift
kvmclock_ns = system_time + ((delta × tsc_to_system_mul) >> 32)
```

源码路径：guest `arch/x86/kernel/kvmclock.c` 的 `kvm_register_clock()`
注册内存，`arch/x86/include/asm/pvclock.h` 的 `__pvclock_read_cycles()` /
`pvclock_scale_delta()` 完成计算；host `arch/x86/kvm/x86.c` 的
`kvm_write_system_time()` 接收注册，`kvm_guest_time_update()` /
`kvm_setup_guest_pvclock()` 更新数据。

`WALL_CLOCK_NEW` 提供的是日历时间的 epoch，作用于整个 VM。需要将 epoch 与当前
pvclock 读数相加才能获得当前墙钟样本；guest 的 `pvclock_read_wallclock()`
完成这一步。接口只保证在写 MSR 时刷新墙钟结构，再次查询需要再次写入。旧编号
`MSR_KVM_WALL_CLOCK = 0x11`、`MSR_KVM_SYSTEM_TIME = 0x12` 已弃用；旧接口由
`KVM_FEATURE_CLOCKSOURCE` 宣告支持，带 `_NEW` 的接口由
`KVM_FEATURE_CLOCKSOURCE2` 宣告支持。参见
[KVM MSR ABI 的 WALL_CLOCK 和 SYSTEM_TIME 条目](https://docs.kernel.org/virt/kvm/x86/msr.html)。

这组关系可以串成：

```text
host TSC
  → 硬件 TSC scaling + offset
guest TSC
  → pvclock 锚点 + 增量换算
kvmclock 纳秒计数
  → guest Linux timekeeping
CLOCK_MONOTONIC / CLOCK_REALTIME 等应用时钟
```

`system_time` 字段、`MSR_KVM_SYSTEM_TIME_NEW` 和应用的 `CLOCK_REALTIME`
是三个不同对象。更完整的读路径见 [overview-codex.md](overview-codex.md)。

### 7. `MSR_KVM_STEAL_TIME`：调度统计接口

该 MSR 为每个 vCPU 注册 64 字节对齐的 `struct kvm_steal_time` GPA，bit 0 为
enable。真正的时间数据在共享内存的 `steal` 字段中，单位 ns，反映 vCPU
被抢占而未运行的累计时间，不把正常 idle 时间算进去；需要按 `version`
协议读取。见
[KVM-specific MSRs 的 STEAL_TIME 条目](https://docs.kernel.org/virt/kvm/x86/msr.html)。

host 更新入口是 `arch/x86/kvm/x86.c` 的 `record_steal_time()`。它用于 guest
调度和 CPU 使用统计，不是新的 wallclock，也不应作为补偿值直接加进或减出
kvmclock。稳定 TSC 的常规读路径本来就会反映 vCPU 未被调度期间经过的时间。

### 8. host 控制寄存器/字段：guest TSC 是怎样生成的

这些是 hypervisor 配置的硬件控制，普通 guest 不通过前面的 kvmclock MSR
访问它们：

| 硬件接口                         | 所在位置                 | 单位/作用                                            |
| -------------------------------- | ------------------------ | ---------------------------------------------------- |
| VMX `TSC_OFFSET`                 | VMCS 字段，编码 `0x2010` | guest TSC tick，缩放后的加法偏移                     |
| VMX `TSC_MULTIPLIER`             | VMCS 字段，编码 `0x2032` | 低 48 位为小数部分的缩放倍率                         |
| SVM `tsc_offset`                 | VMCB control 字段        | guest TSC tick，加法偏移                             |
| SVM `MSR_AMD64_TSC_RATIO`        | host MSR `0xc0000104`    | 低 32 位为小数部分的缩放倍率                         |
| VMX `VMX_PREEMPTION_TIMER_VALUE` | VMCS 字段，编码 `0x482e` | 32 位倒计数，耗尽引起 VM exit                        |
| `MSR_IA32_VMX_MISC`              | host MSR `0x485`         | 低 5 位给出 preemption timer 相对 TSC 的计数速率参数 |

VMCS 字段编码不是 MSR 编号，不能拿 `0x2010` 当成 `rdmsr` 的索引来读取 TSC
offset。

VMX 的 `vmx_write_tsc_offset()` / `vmx_write_tsc_multiplier()` 写入 VMCS；SVM 的
`svm_write_tsc_offset()` 写入 VMCB，`__svm_write_tsc_multiplier()` 写
`MSR_AMD64_TSC_RATIO`。这些函数分别位于 `arch/x86/kvm/vmx/vmx.c` 和
`arch/x86/kvm/svm/svm.c`。scaling 需要相应硬件能力，不代表每台机器都支持任意
guest TSC 频率。

VMX preemption timer 按 host 硬件时间基准在 VMX non-root
执行期间倒计数，速率参数为 N 时，一个计数单位对应 `2^N` 个 TSC
tick。它耗尽产生的是 VM exit，KVM 再处理虚拟 LAPIC
的到期事件；它本身不是直接送给 guest 的 LAPIC 中断，也不会因为 guest TSC scaling
自动改用 guest tick。vCPU 不运行时需要其他等待/唤醒机制配合，见
[kvm-preemption-timer.md](../kvm-preemption-timer.md)。

### 9. 看源码或排查时先确定是哪一层

| 观察/问题                            | 优先检查                                        |
| ------------------------------------ | ----------------------------------------------- |
| guest `RDTSC` 数值或频率不对         | TSC offset、scaling、`MSR_IA32_TSC` 写入与恢复  |
| guest TSC 突然被软件移动             | `MSR_IA32_TSC_ADJUST` 及 TSC 写入路径           |
| `RDTSCP` 附带的 CPU 编号不对         | `MSR_TSC_AUX` 和 vCPU 状态切换                  |
| kvmclock 纳秒跳变，但 guest TSC 正常 | pvclock 的锚点、比例、版本及 host 更新路径      |
| 定时器晚到，但读时间正常             | LAPIC deadline、host timer、vCPU 调度与中断投递 |
| 日历时间不对，但单调时间正常         | wallclock epoch、guest timekeeping 和校时服务   |

QEMU 的 `KVM_GET_CLOCK` / `KVM_SET_CLOCK` 是 VM 级 ioctl，`KVM_GET_TSC_KHZ` /
`KVM_SET_TSC_KHZ` 是 TSC 频率相关 ioctl，都不是 guest
MSR。`vcpu->arch.l1_tsc_offset` 的单位是 tick，`kvm->arch.kvmclock_offset`
的单位是 ns，也不能互换。迁移相关的恢复顺序和补偿另见
[migration-codex.md](migration-codex.md)。

## human

#### MSR_IA32_TSC_ADJUST

1. tsc_sync.c
2. 函数 detect_art ?

#### MSR_IA32_TSC

似乎的确是如此的:

MSR_IA32_TSC 返回的是 guest 看到的“虚拟 TSC 计数器值”；kvmclock
返回的是“时间”，通常是纳秒基准。两者的关系是：

#### [ ] MSR_TSC_AUX

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
