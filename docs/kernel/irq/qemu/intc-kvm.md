# 内核中模拟 intc
## lapic 的模拟

```c
struct kvm_lapic {
	unsigned long base_address;
	struct kvm_io_device dev;
	struct kvm_timer lapic_timer;
	u32 divide_count;
	struct kvm_vcpu *vcpu;
	bool apicv_active;
	bool sw_enabled;
	bool irr_pending;
	bool lvt0_in_nmi_mode;
	/* Number of bits set in ISR. */
	s16 isr_count;
	/* The highest vector set in ISR; if -1 - invalid, must scan ISR. */
	int highest_isr_cache;
	/**
	 * APIC register page.  The layout matches the register layout seen by
	 * the guest 1:1, because it is accessed by the vmx microcode.
	 * Note: Only one register, the TPR, is used by the microcode.
	 */
	void *regs;
	gpa_t vapic_addr;
	struct gfn_to_hva_cache vapic_cache;
	unsigned long pending_events;
	unsigned int sipi_vector;
	int nr_lvt_entries;
};
```
- kvm_lapic::regs : 对应物理的模拟

## 通过设备的方式定义的
1. i8259.c
```c
static const struct kvm_io_device_ops picdev_master_ops = {
	.read     = picdev_master_read,
	.write    = picdev_master_write,
};

static const struct kvm_io_device_ops picdev_slave_ops = {
	.read     = picdev_slave_read,
	.write    = picdev_slave_write,
};

static const struct kvm_io_device_ops picdev_elcr_ops = {
	.read     = picdev_elcr_read,
	.write    = picdev_elcr_write,
};
```
2. lapic.c
```c
static const struct kvm_io_device_ops apic_mmio_ops = {
	.read     = apic_mmio_read,
	.write    = apic_mmio_write,
};
```
3. ioapic.c
```c
static const struct kvm_io_device_ops ioapic_mmio_ops = {
	.read     = ioapic_mmio_read,
	.write    = ioapic_mmio_write,
};
```

## 梳理一下 qemu kvm 支持的中断控制器可以同时在内核或者用户态的模拟

之前 alpine.sh 中测试发现:

```txt
想不到吧，原来 split 参数才是正确的，on 导致不断的中断
arg_machine+=",kernel-irqchip=split"
```

但是这个和 man qemu(2) 的说法不一致:
```txt
              kernel-irqchip=on|off|split
                     Controls  KVM  in-kernel  irqchip support. The default is full acceleration of the interrupt controllers. On x86,
                     split irqchip reduces the kernel attack surface, at a performance cost  for  non-MSI  interrupts.  Disabling  the
                     in-kernel irqchip completely is not recommended except for debugging purposes.
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
