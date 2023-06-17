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
