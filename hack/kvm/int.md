https://lwn.net/Articles/44139/

# What happened to vmx
```c
static struct kvm_x86_ops vmx_x86_ops __initdata = {

  // WOW, a huge mount of function for interrupt
	.run = vmx_vcpu_run,
	.handle_exit = vmx_handle_exit,
	.skip_emulated_instruction = vmx_skip_emulated_instruction,
	.update_emulated_instruction = vmx_update_emulated_instruction,
	.set_interrupt_shadow = vmx_set_interrupt_shadow,
	.get_interrupt_shadow = vmx_get_interrupt_shadow,
	.patch_hypercall = vmx_patch_hypercall,
	.set_irq = vmx_inject_irq,
	.set_nmi = vmx_inject_nmi,
	.queue_exception = vmx_queue_exception,
	.cancel_injection = vmx_cancel_injection,
	.interrupt_allowed = vmx_interrupt_allowed,
	.nmi_allowed = vmx_nmi_allowed,
	.get_nmi_mask = vmx_get_nmi_mask,
	.set_nmi_mask = vmx_set_nmi_mask,
	.enable_nmi_window = enable_nmi_window,
	.enable_irq_window = enable_irq_window,
	.update_cr8_intercept = update_cr8_intercept,
	.set_virtual_apic_mode = vmx_set_virtual_apic_mode,
	.set_apic_access_page_addr = vmx_set_apic_access_page_addr,
	.refresh_apicv_exec_ctrl = vmx_refresh_apicv_exec_ctrl,
	.load_eoi_exitmap = vmx_load_eoi_exitmap,
	.apicv_post_state_restore = vmx_apicv_post_state_restore,
	.check_apicv_inhibit_reasons = vmx_check_apicv_inhibit_reasons,
	.hwapic_irr_update = vmx_hwapic_irr_update,
	.hwapic_isr_update = vmx_hwapic_isr_update,
	.guest_apic_has_interrupt = vmx_guest_apic_has_interrupt,
	.sync_pir_to_irr = vmx_sync_pir_to_irr,
	.deliver_posted_interrupt = vmx_deliver_posted_interrupt,
	.dy_apicv_has_pending_interrupt = pi_has_pending_interrupt,
```

## let's read posted_ipi.c


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


## ioapic
*主要分析 ioapic.c 下面的代码*
```c
static const struct kvm_io_device_ops ioapic_mmio_ops = {
	.read     = ioapic_mmio_read,
	.write    = ioapic_mmio_write, // 用于模拟 .write 的功能
};
```

Trace 一下 vcpu_mmio_read 的结果 :


路线 1 : emulator_read_write_onepage => vcpu_mmio_read

```c
static const struct read_write_emulator_ops read_emultor = {
	.read_write_prepare = read_prepare,
	.read_write_emulate = read_emulate,
	.read_write_mmio = vcpu_mmio_read,
	.read_write_exit_mmio = read_exit_mmio,
};

static const struct read_write_emulator_ops write_emultor = {
	.read_write_emulate = write_emulate,
	.read_write_mmio = write_mmio,
	.read_write_exit_mmio = write_exit_mmio,
	.write = true,
};
```
- [ ] emulator_read_write_onepage 是靠什么判断是普通页面，从而调用 read_write_emulate, 还是 mmio 页面，从而调用 read_write_mmio


路线 2：
存在两条路线，第一条是 
```c
static const struct x86_emulate_ops emulate_ops = {
	.pio_in_emulated     = emulator_pio_in_emulated,
	.pio_out_emulated    = emulator_pio_out_emulated,
```
第二个是:
```c
static int (*kvm_vmx_exit_handlers[])(struct kvm_vcpu *vcpu) = {
	[EXIT_REASON_IO_INSTRUCTION]          = handle_io,
```
emulator_pio_in_out => kernel_pio

- [ ] 所以这两种路径下触发的 io 存在什么区别啊!

- [ ] iodev.h 向其中注册 kvm_io_device_ops::read / write 的用户三个，lapic, ioapic, i8259 和 eventfd，其他的设备如果没有 vfio virtio 之类的辅助，都是需要退出到用户态的

- [ ] 所以，这个模拟就是向 `ioapic->redirtbl` 写入数值，最后的作用是什么 ?
  - 最终到达 : kvm_vcpu_kick，在 vm entry 的时候进行检查
  - kvm_vm_ioctl -> kvm_vm_ioctl_irq_line -> kvm_set_irq => kvm_set_ioapic_irq(使用其中一个作为例子) => kvm_ioapic_set_irq => ioapic_set_irq => ioapic_service => kvm_irq_delivery_to_apic => kvm_apic_set_irq => `__apic_accept_irq` => kvm_vcpu_kick

## irq routing
```c
struct kvm_irq_routing_table {
	int chip[KVM_NR_IRQCHIPS][KVM_IRQCHIP_NUM_PINS];
	u32 nr_rt_entries;
	/*
	 * Array indexed by gsi. Each entry contains list of irq chips
	 * the gsi is connected to.
	 */
	struct hlist_head map[];
};
```
gsi，通过 routing table 的 map 可以得到是哪一个芯片的 map

kvm_set_irq_routing => setup_routing_entry

- [ ] 有点复杂
  - [ ] gsi 是什么
  - [ ] ioapic 不就是 routing 的

## interrupt injection
加快中断的响应:
1. cpu 在 guest mode : kvm_vcpu_kick 使用
2. vcpu 所在的线程在睡眠
