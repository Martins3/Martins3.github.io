https://lwn.net/Articles/44139/


## irqchip
[KVM_CREATE_IRQCHIP](https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-create-irqchip)
- kvm_vm_ioctl
    - [ ] kvm_vm_ioctl_irq_line

## split irqchip
[KVM_CAP_SPLIT_IRQCHIP](https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-cap-split-irqchip)

- [ ] kvm_vm_ioctl_enable_cap_generic

## bus
```c
enum kvm_bus {
	KVM_MMIO_BUS,
	KVM_PIO_BUS,
	KVM_VIRTIO_CCW_NOTIFY_BUS,
	KVM_FAST_MMIO_BUS,
	KVM_NR_BUSES
};

static const struct read_write_emulator_ops write_emultor = {
	.read_write_emulate = write_emulate,
	.read_write_mmio = write_mmio,
	.read_write_exit_mmio = write_exit_mmio,
	.write = true,
};
```
- [ ] 谁来使用这个东西
    - emulator_write_emulated
      - emulator_read_write 


CCW : 
1. https://www.kernel.org/doc/html/latest/s390/vfio-ccw.html
2. https://www.ibm.com/support/knowledgecenter/en/linuxonibm/com.ibm.linux.z.lkdd/lkdd_c_ccwdd.html


## irqfd
