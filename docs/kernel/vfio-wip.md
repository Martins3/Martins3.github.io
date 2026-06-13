
## 什么是 virt/kvm/vfio.c

只有 kvm_vfio_ops 被 kvm_device_ioctl 使用过:
```c
static struct kvm_device_ops kvm_vfio_ops = {
	.name = "kvm-vfio",
	.create = kvm_vfio_create,
	.destroy = kvm_vfio_destroy,
	.set_attr = kvm_vfio_set_attr,
	.has_attr = kvm_vfio_has_attr,
};
```


- vfio_kvm_device_add_group
  - kvm_vm_ioctl(kvm_state, KVM_CREATE_DEVICE, &cd) : 创建的 kvm_device
  - ioctl(vfio_kvm_device_fd, KVM_SET_DEVICE_ATTR, &attr) : 告诉 kvm_device, 内容是 attr 决定的

attr 一共三种行为:
```c
#define   KVM_DEV_VFIO_GROUP_ADD			1
#define   KVM_DEV_VFIO_GROUP_DEL			2
#define   KVM_DEV_VFIO_GROUP_SET_SPAPR_TCE		3
```
主要就是 GROUP_ADD 和 GROUP_DEL 了:

- kvm_vfio_set_attr
  - kvm_vfio_set_group
    - kvm_vfio_group_add
    - kvm_vfio_group_del

感觉最后就是: kvm_vfio_update_coherency
