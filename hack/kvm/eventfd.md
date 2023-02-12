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

## 简单分析下 eventfd 的机制

似乎 vfio 设置中断的过程:
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

[^1]: https://kernelgo.org/mmio.html
