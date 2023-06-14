## 问题
- [ ] eventfd 是不是和 fast mmio 有关
  - [x] 为什么，fast mmio 凭什么是 fast 的
      - kvm_assign_ioeventfd_idx 中间，在于从 KVM_IOEVENTFD 中间的时候，如果 kvm_ioeventfd::len == 0 那么会被设置为 FAST
      - 因为 len == 0 的不存在 datamatch 的判断

## 分析
```c
static const struct kvm_io_device_ops ioeventfd_ops = {
	.write      = ioeventfd_write,
	.destructor = ioeventfd_destructor,
};
```


看看内核那边的实现:
```c
struct kvm_ioeventfd {
	__u64 datamatch;
	__u64 addr;        /* legal pio/mmio address */
	__u32 len;         /* 1, 2, 4, or 8 bytes; or 0 to ignore length */
	__s32 fd;
	__u32 flags;
	__u8  pad[36];
};

/*
 * --------------------------------------------------------------------
 * ioeventfd: translate a PIO/MMIO memory write to an eventfd signal.
 *
 * userspace can register a PIO/MMIO address with an eventfd for receiving
 * notification when the memory has been touched.
 * --------------------------------------------------------------------
 */

struct _ioeventfd {
	struct list_head     list;
	u64                  addr;
	int                  length;
	struct eventfd_ctx  *eventfd;
	u64                  datamatch;
	struct kvm_io_device dev;
	u8                   bus_idx;
	bool                 wildcard;
};

struct kvm_io_device {
	const struct kvm_io_device_ops *ops;
};

static const struct kvm_io_device_ops ioeventfd_ops = {
	.write      = ioeventfd_write,
	.destructor = ioeventfd_destructor,
};
```

- kvm_ioeventfd
  - kvm_assign_ioeventfd
    - kvm_assign_ioeventfd_idx : idx 指明到底是那种 bus 总线类型
      - kvm_io_bus_register_dev : 构建 `_ioeventfd`, 其成员 struct kvm_io_device 的 初始化为 ioeventfd_ops
        - kvm_io_bus_register_dev

在内核现在只是剩下最后一个问题了, 靠什么通知 ?

1. 首先，在通用的内核中间，命中 mmio / pio 最后会逐步到达的这个位置
2. 然后调用 ioeventfd_write, 进而调用 eventfd_signal, 那么在用户态的另一个线程将会检测到这个 event
```c
static int __kvm_io_bus_write(struct kvm_vcpu *vcpu, struct kvm_io_bus *bus,
			      struct kvm_io_range *range, const void *val)
{
	int idx;

	idx = kvm_io_bus_get_first_dev(bus, range->addr, range->len);
	if (idx < 0)
		return -EOPNOTSUPP;

	while (idx < bus->dev_count &&
		kvm_io_bus_cmp(range, &bus->range[idx]) == 0) {
		if (!kvm_iodevice_write(vcpu, bus->range[idx].dev, range->addr,
					range->len, val))
			return idx;
		idx++;
	}

	return -EOPNOTSUPP;
}
```

## 注册 producer 的过程

QEMU 中，注册 msix 中断:
```txt
#0  vfio_enable_vectors (vdev=vdev@entry=0x5555577bab70, msix=msix@entry=true) at ../hw/vfio/pci.c:378
#1  0x0000555555af99ce in vfio_msix_vector_do_use (pdev=pdev@entry=0x5555577bab70, nr=nr@entry=0, msg=msg@entry=0x0, handler=handler@entry=0x0) at ../hw/vfio/pci.c:523
#2  0x0000555555af9bb4 in vfio_msix_enable (vdev=vdev@entry=0x5555577bab70) at ../hw/vfio/pci.c:653
#3  0x0000555555afa0a0 in vfio_pci_write_config (pdev=0x5555577bab70, addr=178, val=49160, len=2) at ../hw/vfio/pci.c:1232
#4  0x0000555555b399f0 in memory_region_write_accessor (mr=mr@entry=0x555556936460, addr=2, value=value@entry=0x7ffc65ffa518, size=size@entry=2, shift=<optimized out>, mask=mask@entry=65535, attrs=...) at ../softmmu/memory.c:493
#5  0x0000555555b371d6 in access_with_adjusted_size (addr=addr@entry=2, value=value@entry=0x7ffc65ffa518, size=size@entry=2, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0x555555b39970 <memory_region_write_accessor>, mr=0x555556936460, attrs=...) at ../softmmu/memory.c:555
#6  0x0000555555b3b49a in memory_region_dispatch_write (mr=mr@entry=0x555556936460, addr=2, data=<optimized out>, op=<optimized out>, attrs=attrs@entry=...) at ../softmmu/memory.c:1522
```

```c
    ret = ioctl(vdev->vbasedev.fd, VFIO_DEVICE_SET_IRQS, irq_set);
```

```txt
vmx_pi_update_irte+1
kvm_arch_irq_bypass_add_producer+67
__connect+136
irq_bypass_register_producer+192
vfio_msi_set_vector_signal+469
vfio_msi_set_block+107
vfio_pci_set_msi_trigger+505
vfio_pci_core_ioctl+2085
vfio_device_fops_unl_ioctl+126
__x64_sys_ioctl+135
do_syscall_64+56
entry_SYSCALL_64_after_hwframe+99
```
在 vfio_msi_set_vector_signal 中:
- ret = request_irq(irq, vfio_msihandler, 0, vdev->ctx[vector].name, trigger);
- irq_bypass_register_producer

实际上，这是
```txt
vfio_msihandler
__handle_irq_event_percpu
handle_irq_event
handle_edge_irq
__common_interrupt
common_interrupt
asm_common_interrupt
```

- vfio_msihandler 就是 producer 的

## 注册 consumer 的

在用户态知道自己注册的 eventfd 和需要注入的中断，在 QEMU 中
```txt
#0  kvm_irqchip_assign_irqfd (s=0x5555568214b0, event=event@entry=0x5555577bbb84, resample=resample@entry=0x5555577bbb90, virq=0, assign=assign@entry=true) at ../accel/kvm/kvm-all.c:2084
#1  0x0000555555b6079b in kvm_irqchip_add_irqfd_notifier_gsi (s=<optimized out>, n=n@entry=0x5555577bbb84, rn=rn@entry=0x5555577bbb90, virq=<optimized out>) at ../accel/kvm/kvm-all.c:2233
#2  0x0000555555af7081 in vfio_intx_enable_kvm (vdev=vdev@entry=0x5555577bb0f0, errp=errp@entry=0x7fffffff0fe8) at ../hw/vfio/pci.c:140
#3  0x0000555555af8a98 in vfio_intx_enable (vdev=vdev@entry=0x5555577bb0f0, errp=errp@entry=0x7fffffff2210) at ../hw/vfio/pci.c:303
#4  0x0000555555afcf8e in vfio_realize (pdev=<optimized out>, errp=<optimized out>) at ../hw/vfio/pci.c:3106
```

在内核中:
- kvm_vm_ioctl
  - kvm_irqfd
    - kvm_irqfd_assign

其中有关键的代码:
```c
#ifdef CONFIG_HAVE_KVM_IRQ_BYPASS
	if (kvm_arch_has_irq_bypass()) {
		irqfd->consumer.token = (void *)irqfd->eventfd;
		irqfd->consumer.add_producer = kvm_arch_irq_bypass_add_producer;
		irqfd->consumer.del_producer = kvm_arch_irq_bypass_del_producer;
		irqfd->consumer.stop = kvm_arch_irq_bypass_stop;
		irqfd->consumer.start = kvm_arch_irq_bypass_start;
		ret = irq_bypass_register_consumer(&irqfd->consumer);
		if (ret)
			pr_info("irq bypass consumer (token %p) registration fails: %d\n",
				irqfd->consumer.token, ret);
	}
#endif
```

```c
/*
 * vmx_pi_update_irte - set IRTE for Posted-Interrupts
 *
 * @kvm: kvm
 * @host_irq: host irq of the interrupt
 * @guest_irq: gsi of the interrupt
 * @set: set or unset PI
 * returns 0 on success, < 0 on failure
 */
int vmx_pi_update_irte(struct kvm *kvm, unsigned int host_irq,
		       uint32_t guest_irq, bool set)
```

原来 irqfd 的机制是做这个用途的:
```txt
@[
    irqfd_wakeup+1
    __wake_up_common+125
    eventfd_signal_mask+127
    vfio_msihandler+18
    __handle_irq_event_percpu+70
    handle_irq_event+58
    handle_edge_irq+177
    __common_interrupt+105
    common_interrupt+179
    asm_common_interrupt+34
    cpuidle_enter_state+222
    cpuidle_enter+41
    do_idle+492
    cpu_startup_entry+25
    start_secondary+271
    secondary_startup_64_no_verify+224
]: 40
```


```txt
4.75 KVM_IRQFD
--------------

:Capability: KVM_CAP_IRQFD
:Architectures: x86 s390 arm64
:Type: vm ioctl
:Parameters: struct kvm_irqfd (in)
:Returns: 0 on success, -1 on error

Allows setting an eventfd to directly trigger a guest interrupt.
kvm_irqfd.fd specifies the file descriptor to use as the eventfd and
kvm_irqfd.gsi specifies the irqchip pin toggled by this event.  When
an event is triggered on the eventfd, an interrupt is injected into
the guest using the specified gsi pin.  The irqfd is removed using
the KVM_IRQFD_FLAG_DEASSIGN flag, specifying both kvm_irqfd.fd
and kvm_irqfd.gsi.

With KVM_CAP_IRQFD_RESAMPLE, KVM_IRQFD supports a de-assert and notify
mechanism allowing emulation of level-triggered, irqfd-based
interrupts.  When KVM_IRQFD_FLAG_RESAMPLE is set the user must pass an
additional eventfd in the kvm_irqfd.resamplefd field.  When operating
in resample mode, posting of an interrupt through kvm_irq.fd asserts
the specified gsi in the irqchip.  When the irqchip is resampled, such
as from an EOI, the gsi is de-asserted and the user is notified via
kvm_irqfd.resamplefd.  It is the user's responsibility to re-queue
the interrupt if the device making use of it still requires service.
Note that closing the resamplefd is not sufficient to disable the
irqfd.  The KVM_IRQFD_FLAG_RESAMPLE is only necessary on assignment
and need not be specified with KVM_IRQFD_FLAG_DEASSIGN.

On arm64, gsi routing being supported, the following can happen:

- in case no routing entry is associated to this gsi, injection fails
- in case the gsi is associated to an irqchip routing entry,
  irqchip.pin + 32 corresponds to the injected SPI ID.
- in case the gsi is associated to an MSI routing entry, the MSI
  message and device ID are translated into an LPI (support restricted
  to GICv3 ITS in-kernel emulation).
```

## 回答这个问题
- https://stackoverflow.com/questions/66937301/what-is-irq-bypass-and-how-to-use-it-in-linux

[^1]: https://kernel.love/mmio.html
