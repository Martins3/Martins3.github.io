## posted interrupt 在 vfio 的控制下是如何连接的
<!-- f2108957-8065-4553-81af-0568cbf66aa5 -->

### 1. 通过 eventfd 通知用户态
回到这个小 demo 中 : https://github.com/Martins3/vfio-host-test
```txt
ioctl(dev->device_fd, VFIO_DEVICE_GET_IRQ_INFO, irq);
int irqfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
ret = ioctl(device, VFIO_DEVICE_SET_IRQS, irq_set);
```
这里是用户态直接创建一个 eventfd ，然后让中断调用到 vfio_msihandler ，最后通知用户态的 vfio


### 2. 通过 eventfd 直接通知内核，无需经用用户态
核心代码:
drivers/vfio/pci/vfio_pci_intrs.c

而对于 qemu 而言，这个 eventfd 是通过 VM_IRQFD 获取的
在 kvm_irqfd_assign 中，调用 irq_bypass_register_consumer ，这个是无条件执行的，
对于所有的 irq 都会到这里，不过这不重要。

中断的注入路线为:
```txt
@[
        vmx_deliver_interrupt+5
        __apic_accept_irq+251
        kvm_irq_delivery_to_apic_fast+336
        kvm_arch_set_irq_inatomic+217
        irqfd_wakeup+275
        __wake_up_common+120
        eventfd_signal_mask+112
        vfio_msihandler+19
        __handle_irq_event_percpu+85
        handle_irq_event+56
        handle_edge_irq+199
        __common_interrupt+76
        common_interrupt+128
        asm_common_interrupt+38
        cpuidle_enter_state+211
        cpuidle_enter+45
        cpuidle_idle_call+241
        do_idle+119
        cpu_startup_entry+41
        start_secondary+296
        common_startup_64+318
]: 393269
```

### 3. 通过 vt-d 直接进行 posted interrupt 的投递
```c
#if IS_ENABLED(CONFIG_HAVE_KVM_IRQ_BYPASS)
	if (kvm_arch_has_irq_bypass()) {
		irqfd->consumer.add_producer = kvm_arch_irq_bypass_add_producer;
		irqfd->consumer.del_producer = kvm_arch_irq_bypass_del_producer;
		irqfd->consumer.stop = kvm_arch_irq_bypass_stop;
		irqfd->consumer.start = kvm_arch_irq_bypass_start;
		ret = irq_bypass_register_consumer(&irqfd->consumer, irqfd->eventfd);
		if (ret)
			pr_info("irq bypass consumer (eventfd %p) registration fails: %d\n",
				irqfd->eventfd, ret);
	}
#endif
```
这里的 irqfd->consumer 合理的，因为当前是 eventfd 是给 kvm 用来注入到 vCPU 的
如果有人来注入，那么就修改 itre 。

当通过 vfio 机制注册中断的时候，也就是使用 ioctl VFIO_DEVICE_SET_IRQS :

```txt
@[
        kvm_arch_irq_bypass_add_producer+5
        __connect+88
        irq_bypass_register_producer+181
        vfio_msi_set_vector_signal+427
        vfio_msi_set_block+89
        vfio_pci_core_ioctl+684
        vfio_device_fops_unl_ioctl+140
        __x64_sys_ioctl+150
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 6
```

也就是 kvm_arch_irq_bypass_add_producer 中调用 kvm_pi_update_irte ，这就是我们常说的 posted interrupt:

这个时候，当注入中断的时候，我们已经很难观察到中断了，从硬件中直接完成了。

## 通过 /proc/interrupts 来理解 vfio 中断
<!-- 85a7faad-a3cb-4d99-904b-3eaf89430a1b -->

1. PIN : Posted-interrupt notification event : vfio 和 virtio-blk 都可能
2. NPI : Nested posted-interrupt event : 相当容易触发，在虚拟机中正常的启动虚拟机就可以了，具体原理就复杂了
3. PIW : Posted-interrupt wakeup event : 如果 vfio 直通，但是 vCPU 没有正在运行，需要唤醒
4. PMN : Posted MSI notification event : 高级机制

就是会对应不同的 ipi 的:
```c
/* Vector for KVM to deliver posted interrupt IPI */
#define POSTED_INTR_VECTOR		0xf2
#define POSTED_INTR_WAKEUP_VECTOR	0xf1
#define POSTED_INTR_NESTED_VECTOR	0xf0
```


无论是否有 posted interrupt ，最后的结果都是一样的:
存在 posted interrupt 的结果:
```txt
 100:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:c8:00.0    0-edge      vfio-msix[0](0000:c8:00.0)
 101:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:c8:00.0    1-edge      vfio-msix[1](0000:c8:00.0)
 102:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:c8:00.0    2-edge      vfio-msix[2](0000:c8:00.0)
 103:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:c8:00.0    3-edge      vfio-msix[3](0000:c8:00.0)
 104:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:c8:00.0    4-edge      vfio-msix[4](0000:c8:00.0)
```

没有 posted interrupt
```txt
 137:          0          0          0          0          0          0          0          0          0          0          0          0          0         23          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0        132          0          0 IR-PCI-MSIX-0000:03:00.0    0-edge      vfio-msix[0](0000:03:00.0)
 138:          0          0          0          0          0          0          0          0          0          0          0          0          0        190          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:03:00.0    1-edge      vfio-msix[1](0000:03:00.0)
 140:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0         62          0          0         78          0 IR-PCI-MSIX-0000:03:00.0    2-edge      vfio-msix[2](0000:03:00.0)
 141:          0          0          0          0          0          0          0          0          0          0          0          0         19          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:03:00.0    3-edge      vfio-msix[3](0000:03:00.0)
 142:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0         32          0          0 IR-PCI-MSIX-0000:03:00.0    4-edge      vfio-msix[4](0000:03:00.0)
 143:          0          0          0          0          0          0          0          0          0          0          0          0          0         12          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:03:00.0    5-edge      vfio-msix[5](0000:03:00.0)
 144:          0          0          0          0          0          0          0          0         37          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0        386          0 IR-PCI-MSIX-0000:03:00.0    6-edge      vfio-msix[6](0000:03:00.0)
 145:          0          0          0          0          0          0          0          0          0         27          0          0         42          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0 IR-PCI-MSIX-0000:03:00.0    7-edge      vfio-msix[7](0000:03:00.0)
 146:          0          0          0          0          0          0          0          0          0          0          0          0          0        148          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          2          0 IR-PCI-MSIX-0000:03:00.0    8-edge      vfio-msix[8](0000:03:00.0)
```

### 导致 PIN : Posted-interrupt notification event 的来源
<!-- b6e9c2e6-22ed-440f-a4c4-968d09c82fa6 -->

1. vfio posted inter : 这是最常见的
2. 虚拟机设备的中断注入，例如 virtio ，也就是从 vmx_deliver_posted_interrupt

为什么会统计到这些内容:
```diff
History:        #0
Commit:         d78f2664832f8d70e36422af9a10e44276dced48
Author:         Yang Zhang <yang.z.zhang@Intel.com>
Committer:      Marcelo Tosatti <mtosatti@redhat.com>
Author Date:    Thu 11 Apr 2013 07:25:11 PM CST
Committer Date: Wed 17 Apr 2013 03:32:39 AM CST

KVM: VMX: Register a new IPI for posted interrupt

Posted Interrupt feature requires a special IPI to deliver posted interrupt
to guest. And it should has a high priority so the interrupt will not be
blocked by others.
Normally, the posted interrupt will be consumed by vcpu if target vcpu is
running and transparent to OS. But in some cases, the interrupt will arrive
when target vcpu is scheduled out. And host will see it. So we need to
register a dump handler to handle it.

Signed-off-by: Yang Zhang <yang.z.zhang@Intel.com>
Acked-by: Ingo Molnar <mingo@kernel.org>
Reviewed-by: Gleb Natapov <gleb@redhat.com>
Signed-off-by: Marcelo Tosatti <mtosatti@redhat.com>
```
PIN 的数量和接受到中断的数量不是一致的，虚拟机中 iops 283k ，观察 PIN 只有几千个，
所以这个 commit msg 就说的非常清楚了，只有 vCPU 不在执行的时候，被 Host 观察到才会出现。
sysvec_kvm_posted_intr_ipi 是一个纯 booking 的作用

```c
/*
 * Handler for POSTED_INTERRUPT_VECTOR.
 */
DEFINE_IDTENTRY_SYSVEC_SIMPLE(sysvec_kvm_posted_intr_ipi)
{
	apic_eoi();
	inc_irq_stat(kvm_posted_intr_ipis);
}

/*
 * Handler for POSTED_INTERRUPT_WAKEUP_VECTOR.
 */
DEFINE_IDTENTRY_SYSVEC(sysvec_kvm_posted_intr_wakeup_ipi)
{
	apic_eoi();
	inc_irq_stat(kvm_posted_intr_wakeup_ipis);
	kvm_posted_intr_wakeup_handler();
}

/*
 * Handler for POSTED_INTERRUPT_NESTED_VECTOR.
 */
DEFINE_IDTENTRY_SYSVEC_SIMPLE(sysvec_kvm_posted_intr_nested_ipi)
{
	apic_eoi();
	inc_irq_stat(kvm_posted_intr_nested_ipis);
}
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
