配套的测试: /vn/code/src/m/ipi.c

## ipi
https://stackoverflow.com/questions/62068750/kinds-of-ipi-for-x86-architecture-in-linux

## 似乎 ipi 也是有 eoi

```txt
#0  apic_native_eoi () at ./arch/x86/include/asm/apic.h:420
#1  kvm_guest_apic_eoi_write () at arch/x86/kernel/kvm.c:348
#2  0xffffffff8264e92c in apic_eoi () at ./arch/x86/include/asm/apic.h:415
#3  __sysvec_reschedule_ipi (regs=0xffffc9001171ff58) at arch/x86/kernel/smp.c:250
#4  instr_sysvec_reschedule_ipi (regs=0xffffc9001171ff58) at arch/x86/kernel/smp.c:248
#5  sysvec_reschedule_ipi (regs=0xffffc9001171ff58) at arch/x86/kernel/smp.c:248
#6  0xffffffff8280150a in asm_sysvec_reschedule_ipi () at ./arch/x86/include/asm/idtentry.h:707
#7  0x0000000000000000 in ?? ()
```


为什么 hrtimer 也是有 eoi 的?
```txt
#0  apic_native_eoi () at ./arch/x86/include/asm/apic.h:420
#1  kvm_guest_apic_eoi_write () at arch/x86/kernel/kvm.c:348
#2  0xffffffff81139b01 in apic_eoi () at ./arch/x86/include/asm/apic.h:415
#3  __sysvec_apic_timer_interrupt (regs=0xffffc9001171fe88) at arch/x86/kernel/apic/apic.c:1047
#4  0xffffffff8264ebc1 in instr_sysvec_apic_timer_interrupt (regs=0xffffc9001171fe88) at arch/x86/kernel/apic/apic.c:1043
#5  sysvec_apic_timer_interrupt (regs=0xffffc9001171fe88) at arch/x86/kernel/apic/apic.c:1043
```

## ipi 在虚拟化有有特殊优化吧

## blk
```c
static void blk_mq_complete_send_ipi(struct request *rq)
{
	struct llist_head *list;
	unsigned int cpu;

	cpu = rq->mq_ctx->cpu;
	list = &per_cpu(blk_cpu_done, cpu);
	if (llist_add(&rq->ipi_list, list)) {
		INIT_CSD(&rq->csd, __blk_mq_complete_request_remote, rq);
		smp_call_function_single_async(cpu, &rq->csd);
	}
}
```

## 一共存在那些 ipi
<!-- 48172f34-688d-4edd-84cc-f66c9442b1f2 -->

### x86 下的
```c
static const char *ipi_types[MAX_IPI] __tracepoint_string = {
	[IPI_RESCHEDULE]	= "Rescheduling interrupts",
	[IPI_CALL_FUNC]		= "Function call interrupts",
	[IPI_CPU_STOP]		= "CPU stop interrupts",
	[IPI_CPU_STOP_NMI]	= "CPU stop NMIs",
	[IPI_TIMER]		= "Timer broadcast interrupts",
	[IPI_IRQ_WORK]		= "IRQ work interrupts",
	[IPI_CPU_BACKTRACE]	= "CPU backtrace interrupts",
	[IPI_KGDB_ROUNDUP]	= "KGDB roundup interrupts",
};
```
其实这个不是全部的，按道理还有 posted interrupt 那些，感觉本质都是一样的。

## 为什么 CPU 需要给自己发送 ipi
https://stackoverflow.com/questions/13416279/purpose-of-self-ipi-on-ia-32

也就是这个函数是否会被调用:
```sh
sudo bpftrace -e 'kprobe:x2apic_send_IPI_self { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
```
但是这个函数似乎的确是调用的不多啊

## ipi 有 linux irq ，irq doman irq_desc 之类的东西吗?

- /home/martins3/core/vn/code/src/m/ipi.c
- /home/martins3/core/vn/code/src/m/arch/x86_64/apic.c


## 使用 /home/martins3/core/vn/code/src/m/arch/x86_64/apic.c 调试

如果触发 SMI ，那么 firecracker 直接 crash

```txt
Error: RunWithApi(MicroVMStoppedWithError(GenericError))
```
似乎合理，现在无法通过 ioport 来拦截，所以有问题。

但是 firecracker 直接 crash 不是预期的吧。

但是 qemu
使用 pc + seabios + smm=on 最后可以得到，就是可以正常触发。

## icr 模拟的基本路径
```sh
sudo bpftrace -e "kfunc:__apic_accept_irq { if (args->delivery_mode == 0x200 ) { @[kstack]=count(); } }"
```

### 虚拟机使用 arch/x86/kernel/apic/apic_flat_64.c
```txt
@[
    bpf_prog_815d6551cd4d7b0b_sd_fw_ingress+2080
    bpf_prog_815d6551cd4d7b0b_sd_fw_ingress+2080
    bpf_trampoline_1294932758007+87
    __apic_accept_irq+5
    kvm_irq_delivery_to_apic_fast+332
    kvm_irq_delivery_to_apic+103
    kvm_apic_send_ipi+175
    kvm_lapic_reg_write+1574
    handle_apic_write+96
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

### 虚拟机使用 arch/x86/kernel/apic/x2apic_phys.c

也就是 CONFIG_X86_X2APIC=y
```txt
@[
    bpf_prog_7dc8126e8768ea37_sd_fw_ingress+408
    bpf_prog_7dc8126e8768ea37_sd_fw_ingress+408
    bpf_trampoline_1294932758007+87
    __apic_accept_irq+5
    kvm_irq_delivery_to_apic_fast+332
    kvm_irq_delivery_to_apic+103
    kvm_apic_send_ipi+175
    kvm_x2apic_icr_write.part.0+39
    vmx_set_msr+3170
    kvm_set_msr_ignored_check+156
    kvm_emulate_wrmsr+78
    vmx_handle_exit+1898
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

##  doc

仔细检查 v3 30.5 VIRTUALIZING MSR-BASED APIC ACCESSES

13900k 不支持的，可以看 cat /sys/module/kvm_intel/parameters/enable_ipiv
```c
bool __read_mostly enable_ipiv = true;
module_param(enable_ipiv, bool, 0444);
```

但是这个实际上本来就有一个 APIC_SELF_IPI 的 MSR 啊


只有部分中断才可以被 ipi 虚拟化支持:
```txt
@[
    kvm_x2apic_icr_write+5
    handle_fastpath_set_msr_irqoff+203
    vcpu_enter_guest.constprop.0+1193
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 1727
```


## 如果虚拟机通过 __apic_send_IPI 发送 ipi ，但是
vCPU 没有运行，中断还可以如何转发。
既然 vapic 可以不用退出，如何判断目标 vCPU 哪一个物理 CPU 并且是否在运行。

### 这么简单的内容一直都搞不懂 grok 说
```txt
在虚拟化技术中，特别是在使用 vAPIC（虚拟 APIC，Virtual Advanced Programmable Interrupt Controller）的场景下，IPI（Inter-Processor Interrupt，处理器间中断）的虚拟化是一个关键过程。vAPIC 是 Intel VT-x 和 AMD-V 等硬件虚拟化技术中的一部分，旨在通过减少虚拟机退出（VM-exit）来提高性能。以下是 vAPIC 中 IPI 虚拟化的基本原理和实现方式：

### 1. **背景：IPI 和 vAPIC 的作用**
IPI 是多核处理器系统中核间通信的重要机制，用于一个 CPU 向另一个 CPU 发送中断信号。在物理环境中，IPI 通常通过写入 Local APIC 的 ICR（Interrupt Command Register， 中断命令寄存器）来触发。然而，在虚拟化环境中，如果每次 Guest OS 发起 IPI 都导致 VM-exit（虚拟机退出到 VMM，虚拟机监视器），性能会受到显著影响。vAPIC 的目标是通过硬件支持，让 IPI 的发送和接收尽量在 Guest 模式（non-root mode）下完成，避免不必要的 VM-exit。

### 2. **vAPIC 的核心机制**
vAPIC 通过虚拟化 Local APIC 的寄存器（如 ICR）以及相关的中断处理逻辑来实现 IPI 的虚拟化。主要依赖以下硬件特性：
- **Virtual-APIC Page**：每个虚拟 CPU（vCPU）都有一个对应的 virtual-APIC page，这是一个 4KB 的内存页面，用于模拟物理 Local APIC 的寄存器状态。VMM（虚拟机监视器）通过 VMCS（Virtual Machine Control Structure）中的字段指定其物理地址。
- **VMCS 控制位**：VMCS 中定义了与 APIC 虚拟化相关的控制位，例如：
  - “Virtualize APIC accesses”：控制对 APIC 寄存器的直接访问。
  - “Virtual-interrupt delivery”：启用虚拟中断的评估和投递。
  - “IPI Virtualization”（在较新的 VT-x 中）：专门优化单播 IPI 的虚拟化。

### 3. **IPI 虚拟化的流程**
在 vAPIC 环境中，IPI 的虚拟化分为发送端和接收端两个部分：

#### **发送端（Source vCPU）**
- **写入 ICR 的虚拟化**：
  - 当 Guest OS 尝试向目标 vCPU 发送 IPI 时，它会写入虚拟 Local APIC 的 ICR 寄存器（通常是 MMIO 地址 0xFEE0_0300 或 MSR 方式在 x2APIC 模式下）。
  - 如果启用了 “Virtualize APIC accesses” 或 “IPI Virtualization”，处理器会拦截这一操作，并将其重定向到 virtual-APIC page 的对应字段（如 VICR，Virtual Interrupt Command Register），而不是直接触发 VM-exit。
  - 硬件根据 ICR 的内容提取关键信息：中断向量（Vector）和目标 APIC ID（Destination）。
- **硬件处理**：
  - 对于单播 IPI（即明确指定单一目标 vCPU 的情况），如果启用了 “IPI Virtualization”，处理器会直接使用 PID-pointer Table（Posted Interrupt Descriptor 指针表，VT-x 新特性）查找目标 vCPU 的 Posted Interrupt Descriptor（PID）。
  - 硬件将中断向量记录到目标 vCPU 的 PID 中的 PIR（Posted Interrupt Request）位图中，无需 VMM 干预。

#### **接收端（Target vCPU）**
- **中断记录与通知**：
  - 如果目标 vCPU 当前正在运行（处于 non-root mode），硬件会检查其 virtual-APIC page 的状态（如 VTPR，Virtual Task Priority Register），并在适当条件下直接投递虚拟中断（通过 “Virtual-interrupt delivery” 机制）。
  - 如果目标 vCPU 未运行（被调度出去），中断请求会保留在 PID 的 PIR 中，等待下次 VM-entry 时处理。
- **无需 VM-exit 的投递**：
  - 当 “Virtual-interrupt delivery” 启用时，处理器会自动评估 virtual-APIC page 中的中断状态（如 VIRR，Virtual Interrupt Request Register），并将最高优先级的中断直接注入到 Guest 中，更新 VISR（Virtual Interrupt Service Register），无需退出到 VMM。
- **Self-IPI 的特殊处理**：
  - 如果是自我 IPI（Self-IPI），处理器直接更新当前 vCPU 的 virtual-APIC page 状态，并触发中断投递，同样避免 VM-exit。

### 4. **关键优化：减少 VM-exit**
- **传统方式的开销**：在没有 vAPIC 的情况下，Guest 写入 ICR 会触发 VM-exit，VMM 模拟中断逻辑并注入中断到目标 vCPU，这涉及到多次上下文切换。
- **vAPIC 的优势**：通过硬件支持（如 virtual-APIC page 和 PID），发送端和接收端的 IPI 处理都在 non-root 模式下完成，只有在特殊情况（如多播 IPI 或硬件不支持）才会退回到 VMM 处理，从而显著降低性能开销。

### 5. **限制与条件**
- **单播 vs 多播**：当前 “IPI Virtualization” 主要优化单播 IPI（unicast，物理寻址）。对于多播 IPI（multicast）或逻辑寻址模式，可能仍会触发 VM-exit。
- **硬件支持**：需要处理器支持 APICv（Intel）或类似功能（如 AMD 的 AVIC），并且 Guest OS 使用 xAPIC 或 x2APIC 模式。
- **VMCS 配置**：VMM 必须正确配置 VMCS 中的控制位和数据结构（如 PID-pointer Table）。

### 6. **总结**
在 vAPIC 中，IPI 的虚拟化通过硬件辅助实现，主要依赖 virtual-APIC page 和 VMCS 的控制机制。发送端 vCPU 写入 ICR 时，硬件将其转化为对目标 vCPU 的虚拟中断记录；接收端 vCPU 则通过 “Virtual-interrupt delivery” 直接处理这些中断。这种方式最大限度地减少了 VM-exit，提高了虚拟化环境下的 IPI 性能，特别适用于多核虚拟机的高频核间通信场景。

如果需要更深入的技术细节（例如具体的 VMCS 字段或 PID 数据结构），可以进一步探讨！
```

这个东西找一个证据:
> 处理器会直接使用 PID-pointer Table（Posted Interrupt Descriptor 指针表，VT-x 新特性）查找目标 vCPU 的 Posted Interrupt Descriptor（PID）。
>   - 硬件将中断向量记录到目标 vCPU 的 PID 中的 PIR（Posted Interrupt Request）位图中，无需 VMM 干预。

## /proc/interrupts 中的 Rescheduling interrupts 是做什么的
<!-- 02dbf3cd-2caa-4f7e-8e5a-31b5d8891eb6 -->

### x86 的实现
arch/x86/kernel/irq.c:arch_show_interrupts
```c
	seq_puts(p, "  Rescheduling interrupts\n");
```

很容易跟踪到:
```c
/*
 * Reschedule call back. KVM uses this interrupt to force a cpu out of
 * guest mode.
 */
DEFINE_IDTENTRY_SYSVEC_SIMPLE(sysvec_reschedule_ipi)
{
	apic_eoi();
	trace_reschedule_entry(RESCHEDULE_VECTOR);
	inc_irq_stat(irq_resched_count);
	scheduler_ipi();
	trace_reschedule_exit(RESCHEDULE_VECTOR);
}
```

虚拟机中观测:
```txt
@[
        bpf_prog_6deef7357e7b4530_sd_fw_ingress+351
        bpf_prog_6deef7357e7b4530_sd_fw_ingress+351
        bpf_trace_run1+214
        sysvec_reschedule_ipi+233
        asm_sysvec_reschedule_ipi+26
        pv_native_safe_halt+15
        default_idle+19
        default_idle_call+122
        do_idle+480
        cpu_startup_entry+41
        start_secondary+281
        common_startup_64+318
]: 3
```

### 为什么有的机器的 scheduler ipi 太多了
3 秒的统计:

内部一个
```txt
  b'update_blocked_averages'
  b'update_nohz_stats'
  b'_nohz_idle_balance'
  b'__softirqentry_text_start'
  b'irq_exit'
  b'reschedule_interrupt'
    17784
```
客户那里的是 640000 次，

scheduler_ipi 次数为 81 万次
```txt
[root@hygon4-86 18:23:49 tools]$ ./funccount scheduler_ipi -d 3
Tracing 1 functions for "b'scheduler_ipi'"... Hit Ctrl-C to end.

FUNC                                    COUNT
b'scheduler_ipi'                         7321
Detaching...
```

19.16 中也是很少的:
```txt
[root@dogfood-idc-elf-19-16 18:26:52 tools]$ ./funccount scheduler_ipi -d 3
Tracing 1 functions for "scheduler_ipi"... Hit Ctrl-C to end.

FUNC                                    COUNT
scheduler_ipi                            3647
Detaching...
```

```txt
[root@dogfood-idc-elf-19-21-NVME-M2 18:31:59 tools]$ ./funccount scheduler_ipi -d 3
Tracing 1 functions for "scheduler_ipi"... Hit Ctrl-C to end.

FUNC                                    COUNT
scheduler_ipi                           16323
Detaching...
```

触发的原因:
```c
/*
 * Reschedule call back. KVM uses this interrupt to force a cpu out of
 * guest mode
 */
__visible void __irq_entry smp_reschedule_interrupt(struct pt_regs *regs)
{
	ack_APIC_irq();
	inc_irq_stat(irq_resched_count);
	kvm_set_cpu_l1tf_flush_l1d();

	if (trace_resched_ipi_enabled()) {
		/*
		 * scheduler_ipi() might call irq_enter() as well, but
		 * nested calls are fine.
		 */
		irq_enter();
		trace_reschedule_entry(RESCHEDULE_VECTOR);
		scheduler_ipi();
		trace_reschedule_exit(RESCHEDULE_VECTOR);
		irq_exit();
		return;
	}
	scheduler_ipi();
}
```
现在的版本：
```c
/*
 * Reschedule call back. KVM uses this interrupt to force a cpu out of
 * guest mode.
 */
DEFINE_IDTENTRY_SYSVEC_SIMPLE(sysvec_reschedule_ipi)
{
	apic_eoi();
	trace_reschedule_entry(RESCHEDULE_VECTOR);
	inc_irq_stat(irq_resched_count);
	scheduler_ipi();
	trace_reschedule_exit(RESCHEDULE_VECTOR);
}
```

但是没有启动虚拟机似乎也会有中断啊，应该只是简单的，一个 core 告诉另外一个 core 有活了 ?

参考一下这个:
https://stackoverflow.com/questions/18396980/what-is-rescheduling-interrupts-res-what-causes-it-how-is-it-handled-in-linu

### 触发的方法

```c
static inline void arch_smp_send_reschedule(int cpu)
{
	smp_ops.smp_send_reschedule(cpu);
}
```
使用 native_smp_send_reschedule

例如:
```txt
@[
    native_smp_send_reschedule+1
    add_timer_on+220
    clocksource_watchdog+1040
    call_timer_fn+36
    expire_timers+167
    run_timer_softirq+192
    __do_softirq+200
    irq_exit_rcu+156
    sysvec_apic_timer_interrupt+110
    asm_sysvec_apic_timer_interrupt+22
    default_idle+11
    default_idle_call+40
    cpuidle_idle_call+288
    do_idle+123
    cpu_startup_entry+38
    start_secondary+277
    secondary_startup_64_no_verify+399
]: 3
```

再例如:
```txt
  native_smp_send_reschedule
  try_to_wake_up
  swake_up_locked.part.3
  swake_up_one
  kvm_vcpu_kick
  __apic_accept_irq
  kvm_irq_delivery_to_apic_fast
  kvm_arch_set_irq_inatomic
  irqfd_wakeup
  __wake_up_common
  eventfd_signal
  handle_rx
  vhost_worker
  kthread
  ret_from_fork
    6119

  native_smp_send_reschedule
  yield_to
  kvm_vcpu_yield_to
  kvm_vcpu_on_spin
  handle_pause
  vcpu_enter_guest
  kvm_arch_vcpu_ioctl_run
  kvm_vcpu_ioctl
  do_vfs_ioctl
  ksys_ioctl
  __x64_sys_ioctl
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    7570
```

主线内核中，触发的路径完全不同:
```txt
@[
    native_smp_send_reschedule+5
    wakeup_preempt+105
    ttwu_do_activate+132
    try_to_wake_up+713
    rcuwait_wake_up+39
    kvm_vcpu_wake_up+22
    vmx_deliver_interrupt+80
    __apic_accept_irq+248
    kvm_irq_delivery_to_apic_fast+332
    kvm_irq_delivery_to_apic+103
    kvm_apic_send_ipi+175
    kvm_x2apic_icr_write+69
    handle_fastpath_set_msr_irqoff+203
    vcpu_enter_guest.constprop.0+1193
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 449
```

### 也是通过 Reschedule ipi 来实现 kick vcpu 的
kvm_vcpu_kick

## /proc/interrupts 中的 TLB flush 为什么没有 callback

在这个文件中没有找到 tlb flush 的 callback ，似乎硬件直接优化了:
arch/x86/kernel/smp.c

arch_show_interrupts
```txt
	for_each_online_cpu(j)
		seq_printf(p, "%10u ", irq_stats(j)->irq_tlb_count);
	seq_puts(p, "  TLB shootdowns\n");
```
找这个 irq_tlb_count 的调用，发现其果然不同

```txt
@[
    flush_tlb_func+1
    smp_call_function_many_cond+307
    on_each_cpu_cond_mask+25
    flush_tlb_mm_range+241
    tlb_flush_mmu+109
    tlb_finish_mmu+61
    unmap_region.constprop.0+323
    do_vmi_align_munmap+919
    do_vmi_munmap+216
    __vm_munmap+168
    __x64_sys_munmap+23
    do_syscall_64+85
    entry_SYSCALL_64_after_hwframe+120
]: 30
@[
    flush_tlb_func+1
    flush_tlb_mm_range+285
    ptep_clear_flush+86
    wp_page_copy+618
    __handle_mm_fault+1410
    handle_mm_fault+430
    exc_page_fault+492
    asm_exc_page_fault+34
]: 136
```

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
