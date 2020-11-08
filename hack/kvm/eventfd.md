

```c
static inline int kvm_iodevice_read(struct kvm_vcpu *vcpu,
				    struct kvm_io_device *dev, gpa_t addr,
				    int l, void *v)
{
	return dev->ops->read ? dev->ops->read(vcpu, dev, addr, l, v)
				: -EOPNOTSUPP;
}

static inline int kvm_iodevice_write(struct kvm_vcpu *vcpu,
				     struct kvm_io_device *dev, gpa_t addr,
				     int l, const void *v)
{
	return dev->ops->write ? dev->ops->write(vcpu, dev, addr, l, v)
				 : -EOPNOTSUPP;
}
```
- [ ] eventfd 是不是和 fast mmio 有关
  - [x] 为什么，fast mmio 凭什么是 fast 的
      - kvm_assign_ioeventfd_idx 中间，在于从 KVM_IOEVENTFD 中间的时候，如果 kvm_ioeventfd::len == 0 那么会被设置为 FAST 
      - 因为 len == 0 的不存在 datamatch 的判断

```c
static const struct kvm_io_device_ops ioeventfd_ops = {
	.write      = ioeventfd_write,
	.destructor = ioeventfd_destructor,
};
```


## API

```c
int
kvm_ioeventfd(struct kvm *kvm, struct kvm_ioeventfd *args)
{
	if (args->flags & KVM_IOEVENTFD_FLAG_DEASSIGN)
		return kvm_deassign_ioeventfd(kvm, args);

	return kvm_assign_ioeventfd(kvm, args);
}
```
- kvm_assign_ioeventfd
  - kvm_assign_ioeventfd_idx


[^1]: https://kernelgo.org/mmio.html
