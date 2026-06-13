## arm timer 模拟
<!-- 9a94bc99-6181-4114-b170-4425d4fc6e88 -->
(这个问题有趣的)

arm 的 host 中， cat /proc/interrupts 可以看到:
```txt
  10:          0          0          GICv3   30 Level     kvm guest ptimer
  11:      64526      53692          GICv3   27 Level     kvm guest vtimer
```

结合 arch/arm64/kvm/arch_timer.c 分析其大致 arm 架构下是如何实现 timer 的模拟的


真的会注册一个:
```txt
	  err = request_percpu_irq(host_vtimer_irq, kvm_arch_timer_handler,
				 "kvm guest vtimer", kvm_get_running_vcpus());

		err = request_percpu_irq(host_ptimer_irq, kvm_arch_timer_handler,
					 "kvm guest ptimer", kvm_get_running_vcpus());
```

也就是专门有硬件支持，不过不知道为什么 kvm_arch_timer_handler 的 backtrace 找不到，
相同的环境测试普通的中断是没有问题的:
```txt
@[
        nvme_irq+0
        handle_irq_event+76
        handle_fasteoi_irq+180
        handle_irq_desc+60
        generic_handle_domain_irq+36
        __gic_handle_irq_from_irqson.isra.0+340
        gic_handle_irq+40
        call_on_irq_stack+48
        do_interrupt_handler+136
        el1_interrupt+68
        el1h_64_irq_handler+24
        el1h_64_irq+128
        default_idle_call+56
        cpuidle_idle_call+380
        do_idle+244
        cpu_startup_entry+64
        secondary_start_kernel+224
        __secondary_switched+192
]: 29
```

arm 也可以使用 linux 的 hrtimer 来模拟 guest os 中的时钟:
```c
struct arch_timer_context {
	struct kvm_vcpu			*vcpu;

	/* Emulated Timer (may be unused) */
	struct hrtimer			hrtimer;
```

## timer_chip 中为什么会注册了 irq_set_vcpu_affinity ，难道这个东西不是用于透传设备的 posted interrupt 吗?

那么，也就说，中断也是可以做 posted 注入的?
```txt
static struct irq_chip timer_chip = {
	.name			= "KVM",
	.irq_ack		= timer_irq_ack,
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_eoi		= timer_irq_eoi,
	.irq_set_type		= irq_chip_set_type_parent,
	.irq_set_vcpu_affinity	= timer_irq_set_vcpu_affinity,
	.irq_set_irqchip_state	= timer_irq_set_irqchip_state,
};
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
