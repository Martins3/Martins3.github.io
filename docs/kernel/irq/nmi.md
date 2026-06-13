# x86 nmi

## 通过 qemu 来进行测试

qemu 代码 arch/x86_64/x86_misc.c ，使用 hmp 的命令 nmi 来注入

通过 cat /proc/interrupts 看，似乎 nmi 没有进一步的分类了
```txt
NMI:          0          0   Non-maskable interrupts
```

kvm debugfs 有如下两个相关的东西:
```txt
nmi_injections
nmi_window_exits
```

```txt
        nmi_backtrace.backtrace_idle [KNL]
                        Dump stacks even of idle CPUs in response to an
                        NMI stack-backtrace request.

        nmi_debug=      [KNL,SH] Specify one or more actions to take
                        when a NMI is triggered.
                        Format: [state][,regs][,debounce][,die]

        nmi_watchdog=   [KNL,BUGS=X86] Debugging features for SMP kernels
                        Format: [panic,][nopanic,][rNNN,][num]
                        Valid num: 0 or 1
                        0 - turn hardlockup detector in nmi_watchdog off
                        1 - turn hardlockup detector in nmi_watchdog on
                        rNNN - configure the watchdog with raw perf event 0xNNN

                        When panic is specified, panic when an NMI watchdog
                        timeout occurs (or 'nopanic' to not panic on an NMI
                        watchdog, if CONFIG_BOOTPARAM_HARDLOCKUP_PANIC is set)
                        To disable both hard and soft lockup detectors,
                        please see 'nowatchdog'.
                        This is useful when you use a panic=... timeout and
                        need the box quickly up again.

                        These settings can be accessed at runtime via
                        the nmi_watchdog and hardlockup_panic sysctls.
```

## 内核处理 nmi 的流程
<!-- 48e8e52e-36eb-451c-a629-ef6bbf432d4b -->

nmi 中断注册到 idt 2 上，会调用所有的 nmi handler ，每一个 handler 都通过检查自己的
状态来判断到底是不是自己的 nmi 来了。例如 perf_event_nmi_handler 中:

```c
	/*
	 * All PMUs/events that share this PMI handler should make sure to
	 * increment active_events for their events.
	 */
	if (!atomic_read(&active_events))
		return NMI_DONE;
```

当所有注册的 handler 都说，不相干，就会走到 unknown nmi 中

nmi reason 从这里获取的
```c
static inline unsigned char default_get_nmi_reason(void)
{
	return inb(NMI_REASON_PORT);
}
```

```txt
[73436.632915] Uhhuh. NMI received for unknown reason 30 on CPU 0.
[73436.632930] Dazed and confused, but trying to continue
```

所以，一旦注册了一个 nmi handler ，这个 handler 总是返回 NMI_HANDLED ，那么就不会触发 unknown nmi

## kernel.unknown_nmi_panic=1 的影响

检查当前数值，结果为:
```sh
sysctl kernel.unknown_nmi_panic
```
默认为 0

配置
```sh
sysctl -w kernel.unknown_nmi_panic=1
```

通过 qmp 中注入 nmi ，这个时候虚拟机会 panic
```txt
Dazed and confused, but trying to continue
Uhhuh. NMI received for unknown reason 00 on CPU 0.
Kernel panic - not syncing: NMI: Not continuing
CPU: 0 UID: 0 PID: 0 Comm: swapper/0 Not tainted 6.17.7-martins3-00001-gfd23f075a322 #27 PREEMPT(full)
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
Call Trace:
 <NMI>
 dump_stack_lvl+0x6f/0xb0
 vpanic+0xca/0x2a0
 panic+0x6b/0x6b
 nmi_panic.cold+0xc/0xc
 unknown_nmi_error+0x77/0xa0
 exc_nmi+0xe3/0x110
 end_repeat_nmi+0xf/0x53
RIP: 0010:pv_native_safe_halt+0xf/0x20
Code: 4c 54 00 c3 cc cc cc cc 0f 1f 00 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 f3 0f 1e fa eb 07 0f 00 2d 15 16 14 00 fb f4 <c3> cc cc cc cc 66 2e 0f 1f 84 00 00 00 00 00 66 90 90 90 90 90 90
RSP: 0018:ffffffff82803e88 EFLAGS: 00000282
RAX: 000000000086d109 RBX: ffffffff828149c0 RCX: 0000000000000000
RDX: 0000000000000000 RSI: 0000000000000000 RDI: ffffffff81354cb0
RBP: 0000000000000000 R08: 0000000000000001 R09: 0000000000000000
R10: 0000000000000001 R11: 0000000000000000 R12: 0000000000000000
R13: 0000000000000000 R14: ffffffff82814050 R15: 0000000000014790
 ? do_idle+0x1e0/0x250
 ? pv_native_safe_halt+0xf/0x20
 ? pv_native_safe_halt+0xf/0x20
 </NMI>
 <TASK>
 default_idle+0x13/0x20
 default_idle_call+0x7a/0x1d0
 do_idle+0x1e0/0x250
 cpu_startup_entry+0x29/0x30
 rest_init+0x15c/0x160
 start_kernel+0x6c8/0x6d0
 x86_64_start_reservations+0x24/0x30
 x86_64_start_kernel+0xc4/0xd0
 common_startup_64+0x13e/0x148
 </TASK>
Kernel Offset: disabled
---[ end Kernel panic - not syncing: NMI: Not continuing ]---
```

## 常见的 NMI

NMI 和 PMI (Performance monitoring interrupts) 的数量完全相同:

一般来说，都是 perf 导致的
```txt
cat /proc/interrupts | grep -E "NMI|PMI"

 NMI:        595        290        733        277        751        283        763        299       1979        622       2013        666       1025        270        882        270        499        472        412        450        350        339        332        322        331        330        320        319        325        321        318        313   Non-maskable interrupts
 PMI:        595        290        733        277        751        283        763        299       1979        622       2013        666       1025        270        882        270        499        472        412        450        350        339        332        322        331        330        320        319        325        321        318        313   Performance monitoring interrupts
VPMI:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0  Perf Guest Mediated PMI
```
VPMI 显然和 kvm 有关，这里不深究了。

## kvm 注入

### vnmi

kimi 的解答，结合代码看，完全正确:
```txt
背景：物理 CPU 的 NMI 行为

在 x86 物理 CPU 上，NMI 有一个关键语义：当一个 NMI 被响应后，CPU 会自动阻塞后续 NMI，直到当前 NMI handler 执行 IRET 返回。这是硬件保证的，防止 NMI
无限嵌套导致栈溢出或系统崩溃。

没有 vNMI 时的问题

在虚拟化环境中，KVM 需要向 guest 注入虚拟 NMI（例如来自虚拟 watchdog、qemu-monitor 注入的 NMI、或 guest 间的中断）。如果没有硬件辅助的 vNMI，KVM 只能靠软件模拟
NMI 阻塞行为：

1. IRET 跟踪困难：KVM 必须拦截 guest 的 IRET 指令来判断 NMI handler 是否返回，但 guest 可能在任何上下文执行 IRET，很难准确区分。
2. 竞态与丢中断：VMM 和 guest 之间对 NMI 阻塞状态的同步容易出现竞态，可能导致 NMI 丢失或错误重入。
3. 性能开销：为跟踪 NMI 状态需要额外的 VM exit，增加开销。

如果模拟不正确，guest OS 可能在 NMI handler 内收到新的 NMI，破坏其关键状态，导致 guest panic 或挂起。

vNMI 如何解决

Intel VT-x 的 "Virtual NMIs" 特性将 NMI 的阻塞/解除逻辑卸载到硬件：

• 自动阻塞：向 guest 注入 NMI 时，硬件自动在 guest_interruptibility-state 中设置 NMI blocking 位。
• 自动解除：当 guest 执行 IRET 时，硬件自动清除阻塞状态，无需 VMM 拦截。
• VM exit 语义：如果 IRET 导致 VM exit，硬件会设置 NMI unblocking due to IRET 标志，帮助 KVM 正确处理边界情况。
• 可靠投递：硬件保证 NMI 按顺序投递，不会嵌套，完全模拟物理 x86 语义。

总结

┌──────────────┬──────────────────┬───────────────────────────────────────────┐
│ 场景         │ 没有 vNMI        │ 有 vNMI                                   │
├──────────────┼──────────────────┼───────────────────────────────────────────┤
│ NMI 阻塞跟踪 │ 软件模拟，易出错 │ 硬件自动处理                              │
├──────────────┼──────────────────┼───────────────────────────────────────────┤
│ IRET 拦截    │ 可能需要         │ 不需要                                    │
├──────────────┼──────────────────┼───────────────────────────────────────────┤
│ 可靠性       │ 竞态、可能丢 NMI │ 精确模拟物理 CPU                          │
├──────────────┼──────────────────┼───────────────────────────────────────────┤
│ 典型用途     │ —                │ 虚拟 watchdog、KVM 调试、guest panic 处理 │
└──────────────┴──────────────────┴───────────────────────────────────────────┘
```

```c
static bool __read_mostly enable_vnmi = 1;
module_param_named(vnmi, enable_vnmi, bool, 0444);
```

这的确是硬件能力:
```c
static inline bool cpu_has_virtual_nmis(void)
{
	return vmcs_config.pin_based_exec_ctrl & PIN_BASED_VIRTUAL_NMIS &&
	       vmcs_config.cpu_based_exec_ctrl & CPU_BASED_NMI_WINDOW_EXITING;
}
```

无论 vnmi 是否打开，都是通过这个路线注入的，也就是说注入还是需要 kick 出来的，
这很合逻辑:
```txt
@[
    vmx_inject_nmi+5
    kvm_check_and_inject_events+1160
    vcpu_enter_guest.constprop.0+188
    kvm_arch_vcpu_ioctl_run+855
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

### nmi window
```c
static int handle_nmi_window(struct kvm_vcpu *vcpu)
{
	if (KVM_BUG_ON(!enable_vnmi, vcpu->kvm))
		return -EIO;

	exec_controls_clearbit(to_vmx(vcpu), CPU_BASED_NMI_WINDOW_EXITING);
	++vcpu->stat.nmi_window_exits;
	kvm_make_request(KVM_REQ_EVENT, vcpu);

	return 1;
}
```

```txt
  KVM 想注入 NMI
      ↓
  guest 当前 NMI 被阻塞（如正在处理前一个 NMI）
      ↓
  vmx_enable_nmi_window() 设置 CPU_BASED_NMI_WINDOW_EXITING
      ↓
  guest 继续运行... 执行 IRET，硬件自动解除 NMI 阻塞
      ↓
  NMI Window 打开 → VM exit
      ↓
  handle_nmi_window() 被调用
      ↓
  清除 CPU_BASED_NMI_WINDOW_EXITING
  标记 KVM_REQ_EVENT（下次 vmentry 时注入 NMI）
```

- PIN_BASED_VIRTUAL_NMIS 负责 NMI 语义正确（阻塞/解除）。
- NMI_WINDOW_EXITING + handle_nmi_window 负责 注入时机正确（等 guest 准备好再投）。

两者配合，才构成完整的 vNMI 硬件辅助方案。如果 enable_vnmi=0，KVM 会退回到软件模拟路径（soft_vnmi_blocked），根本不会走到 handle_nmi_window。

## 通过 nmi 展示 backtrace

经常观察到 RCU stall 的时候，通过 nmi 给其他的 CPU 发消息的

接收端: lib/nmi_backtrace.c:nmi_cpu_backtrace
发送端:
```c
static void nmi_raise_cpu_backtrace(cpumask_t *mask)
{
	__apic_send_IPI_mask(mask, NMI_VECTOR);
}
```
也就是 nmi 可以作为一个 ipi 被触发。

## ARM 环境中的 nmi 叫什么?

> [!NOTE]
> 参考 Deepseeek ，有待验证

总体是比较合理的:
```txt
在 ARM 环境中，类似 x86 架构中 NMI（Non-Maskable Interrupt，非可屏蔽中断）的机制通常被称为 **FIQ（Fast Interrupt Request）**。

### 详细说明：
- **FIQ** 是 ARM 架构中的一种高优先级中断，设计目的是为了处理需要快速响应的关键事件。它与 NMI 的功能类似，都用于处理无法被屏蔽或延迟的紧急中断。
- 与普通中断（IRQ，Interrupt Request）相比，FIQ 具有以下特点：
  - **更高的优先级**：FIQ 的优先级高于 IRQ，通常用于需要极低延迟的场景，如实时系统中的关键任务。
  - **专属寄存器**：ARM 处理器为 FIQ 提供了专用的寄存器（如 R8-R12 在某些模式下），以减少上下文切换的开销，从而加速中断处理。
  - **不可屏蔽**：FIQ 通常不能被其他中断或处理器状态屏蔽（除非明确配置），类似于 NMI 的行为。

### 与 NMI 的区别：
- NMI 是 x86 架构中的术语，专指不可屏蔽的中断，通常由硬件错误（如内存错误或电源故障）触发。
- FIQ 是 ARM 架构中的术语，更多用于高优先级的快速中断处理，虽然功能上类似，但 FIQ 的使用场景更灵活，可以由软件或硬件触发，具体取决于系统设计。

### 其他相关机制：
在 ARM 架构的某些高级版本（如 ARMv7-A、ARMv8-A）中，可能还有其他机制与 NMI 的功能有部分重叠：
- **SError（System Error）**：在 ARMv8-A 中，系统错误异常（System Error Exception）可以用来处理严重的硬件错误，类似于 NMI 的某些用途。
- **GIC（Generic Interrupt Controller）**：ARM 系统通常使用 GIC 来管理中断，FIQ 和 IRQ 都通过 GIC 分发。GIC 可以配置某些中断为高优先级，接近 NMI 的效果。

### 总结：
在 ARM 环境中，**FIQ** 是最接近 x86 NMI 的机制，用于高优先级、快速响应的中断处理。如果涉及具体场景（如硬件错误处理），可能需要结合 SError 或 GIC 的配置来实现类似 NMI 的功能。
```

## TODO
1. 为什么有时候，在物理机上触发 nmi ，是没有办法触发 vmcore 的
2. https://docs.hyperdbg.org/design/features/vmm-module/control-over-nmis

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
