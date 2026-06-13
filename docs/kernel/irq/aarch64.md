## 对比整理一下 arm 和 x86_64 的 config 的差别是什么

ARM 环境中:
```txt
IPI0:      58589           Rescheduling interrupts
IPI1:  646472476           Function call interrupts
IPI2:          0           CPU stop interrupts
IPI3:          0           CPU stop (for crash dump) interrupts
IPI4:          0           Timer broadcast interrupts
IPI5:      11266           IRQ work interrupts
IPI6:          0           CPU wake-up interrupts
 Err:          0
```

x86 环境中:
```txt
 NMI:      41369      41342      41345      41358   Non-maskable interrupts
 LOC:   18823052   17609982   18857332   18760480   Local timer interrupts
 SPU:          0          0          0          0   Spurious interrupts
 PMI:      41369      41342      41345      41358   Performance monitoring interrupts
 IWI:      76415      79780      74243      75131   IRQ work interrupts
 RTR:          0          0          0          0   APIC ICR read retries
 RES:      12600      14018      13158      13312   Rescheduling interrupts
 CAL:     232469     234703     216998     200432   Function call interrupts
 TLB:       4708       4988       3985       4260   TLB shootdowns
 TRM:          0          0          0          0   Thermal event interrupts
 THR:          0          0          0          0   Threshold APIC interrupts
 DFR:          0          0          0          0   Deferred Error APIC interrupts
 MCE:          0          0          0          0   Machine check exceptions
 MCP:        952        953        953        953   Machine check polls
 ERR:          0
 MIS:          0
 PIN:          0          0          0          0   Posted-interrupt notification event
 NPI:          0          0          0          0   Nested posted-interrupt event
 PIW:          0          0          0          0   Posted-interrupt wakeup event
```

1. 如何理解 Timer broadcast interrupts

2. nmi :

在 aarch64 中有:
```c
static bool ipi_should_be_nmi(enum ipi_msg_type ipi)
{
	if (!system_uses_irq_prio_masking())
		return false;

	switch (ipi) {
	case IPI_CPU_STOP_NMI:
	case IPI_CPU_BACKTRACE:
	case IPI_KGDB_ROUNDUP:
		return true;
	default:
		return false;
	}
}
```

arm 中可以找到发出的原因
```txt
@[
    __traceiter_ipi_raise+76
    __traceiter_ipi_raise+76
    smp_cross_call+132
    arch_send_call_function_single_ipi+56
    __smp_call_single_queue+284
    ttwu_queue_wakelist+364
    try_to_wake_up+632
    wake_up_q+104
    futex_wake+336
    do_futex+212
    __arm64_sys_futex+128
    invoke_syscall+80
    el0_svc_common.constprop.0+200
    do_el0_svc+36
    el0_svc+68
    el0t_64_sync_handler+256
    el0t_64_sync+392
]: 103
```

按道理 x86_64 中根本没有使用，但是依旧可以 trace 到 trace_ipi_raise


只能说，调用链有点长了:
```txt
 __napi_poll+0x48/0x1c0
 net_rx_action+0x2bc/0x340
 __do_softirq+0x128/0x338
 irq_exit_rcu+0x11c/0x128
 irq_exit+0x18/0x28
 __handle_domain_irq+0x74/0xc8
 gic_handle_irq+0x1a8/0x2e4
 call_on_irq_stack+0x28/0x34
 do_interrupt_handler+0x48/0x68
 el0_interrupt+0x3c/0x90
 __el0_irq_handler_common+0x14/0x20
 el0t_64_irq_handler+0xc/0x18
 el0t_64_irq+0x148/0x14c
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
