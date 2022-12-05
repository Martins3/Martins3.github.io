# intel 中断虚拟化，基于 狮子书

## 中断虚拟化
中断虚拟化的关键在于对中断控制器的模拟，我们知道x86上中断控制器主要有旧的中断控制器PIC(intel 8259a)和适应于SMP框架的IOAPIC/LAPIC两种。

https://luohao-brian.gitbooks.io/interrupt-virtualization/content/qemu-kvm-zhong-duan-xu-ni-hua-kuang-jia-fen-679028-4e2d29.html

查询 GSI 号上对应的所有的中断号:

从 ioctl 到下层，kvm_vm_ioctl 注入的中断，最后更改了 kvm_kipc_state:irr

kvm_kipc_state 的信息如何告知 CPU ? 通过 kvm_pic_read_irq


# 8254 / 8259
KVM_CREATE_IRQCHIP :

https://en.wikipedia.org/wiki/Intel_8253

# 之前记录的笔记
https://lwn.net/Articles/44139/

- [ ] What's irr(interrupt request register), e.g kvm_lapic_find_highest_irr
- [ ] kvm_lapic::regs : What's APIC register page


- [ ] currently, we know three way to info guest about the interrupt
  - kick guest
  - host get the irq and send to guest without vmexit
  - device to guest directly

We have three device for interrupt
1. i8259.c
```c
static const struct kvm_io_device_ops picdev_master_ops = {
	.read     = picdev_master_read,
	.write    = picdev_master_write,
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

## What happened to vmx
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

- [ ] shadow interrupt
- [ ] irq window

```c
struct kvm_lapic {
	unsigned long base_address;
	struct kvm_io_device dev;
	struct kvm_timer lapic_timer;
	u32 divide_count;
	struct kvm_vcpu *vcpu;
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
};
```

## posted_ipi.c
pi_update_irte
  - kvm_arch_has_assigned_device : kvm_arch_irq_bypass_add_producer :
  - irq_set_vcpu_affinity : kernel/irq/manage.c
    - `chip->irq_set_vcpu_affinity`
      - intel_ir_set_vcpu_affinity
        - modify_irte
      - amd_ir_set_vcpu_affinity


## irq_comm.c
- kvm_set_routing_entry : seems used for choosing the correct irq chip.

- [ ] kvm_set_irq
  - kvm_set_ioapic_irq
    - kvm_ioapic_set_irq
      - ioapic_set_irq
        - ioapic_service
          - kvm_irq_delivery_to_apic
            - kvm_irq_delivery_to_apic_fast
              - kvm_apic_set_irq
                - `__apic_accept_irq`
                  - kvm_x86_ops.deliver_posted_interrupt(vcpu, vector)
  - kvm_set_pic_irq
    - kvm_pic_set_irq : it's clear pic use a vmexit way to deliver interrupt
  - kvm_set_msi
    - kvm_irq_delivery_to_apic
      - kvm_irq_delivery_to_apic_fast


## i8259.c

## lapic.c
timer

```c
static void restart_apic_timer(struct kvm_lapic *apic)
{
	preempt_disable();

	if (!apic_lvtt_period(apic) && atomic_read(&apic->lapic_timer.pending))
		goto out;

	if (!start_hv_timer(apic))
		start_sw_timer(apic);
out:
	preempt_enable();
}
```
hv timer and sw timer :

- kvm_lapic_reg_write : MSR access exit, and apic access exit handler 's code flow leads to here

```c
static int (*kvm_vmx_exit_handlers[])(struct kvm_vcpu *vcpu) = {
  // ...
	[EXIT_REASON_MSR_READ]                = kvm_emulate_rdmsr,
	[EXIT_REASON_MSR_WRITE]               = kvm_emulate_wrmsr,
  // ...
	[EXIT_REASON_APIC_ACCESS]             = handle_apic_access,
	[EXIT_REASON_APIC_WRITE]              = handle_apic_write,
	[EXIT_REASON_EOI_INDUCED]             = handle_apic_eoi_induced,
  // ...
```

- apic_mmio_write
  - kvm_lapic_reg_write
    - kvm_apic_send_ipi
      - kvm_irq_delivery_to_apic

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

## interrupt injection
加快中断的响应:
1. cpu 在 guest mode : kvm_vcpu_kick 使用
2. vcpu 所在的线程在睡眠

# 狮子书笔记

### 3.3.23 IRQ routing
在加入 APIC 虚拟化后，当外设发送中断请求后，那么 KVM 模块究竟是通过 8259A 还是通过 APIC 向 Guest 注入中断?

这里的 routing 和 ioapic 将中断信息 routing 到 CPU 上其实不是一个事情，这里的 routing 是为了解决中断控制器的选择

gsi 可以理解为中断控制器的引脚线，当 gsi 小于 16 的时候， gsi 可能是 8259A 和 APIC 两个中断控制器的中断，所以需要对于这两个控制器都进行调用。

- [ ] 一个在 Host userspace 态的只是知道发送中断，其实无法确定中断到底发送给谁, 比如在 kvmtool 中间 kvmtool/hw/i8042.c:kbd_update_irq 调用的 `kvm__irq_line(state.kvm, KBD_IRQ, klevel);`

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

```c
static const struct kvm_irq_routing_entry default_routing[] = {
	ROUTING_ENTRY2(0), ROUTING_ENTRY2(1),
	ROUTING_ENTRY2(2), ROUTING_ENTRY2(3),
	ROUTING_ENTRY2(4), ROUTING_ENTRY2(5),
	ROUTING_ENTRY2(6), ROUTING_ENTRY2(7),
	ROUTING_ENTRY2(8), ROUTING_ENTRY2(9),
	ROUTING_ENTRY2(10), ROUTING_ENTRY2(11),
	ROUTING_ENTRY2(12), ROUTING_ENTRY2(13),
	ROUTING_ENTRY2(14), ROUTING_ENTRY2(15),
	ROUTING_ENTRY1(16), ROUTING_ENTRY1(17),
	ROUTING_ENTRY1(18), ROUTING_ENTRY1(19),
	ROUTING_ENTRY1(20), ROUTING_ENTRY1(21),
	ROUTING_ENTRY1(22), ROUTING_ENTRY1(23),
};

int kvm_setup_default_irq_routing(struct kvm *kvm)
{
	return kvm_set_irq_routing(kvm, default_routing,
				   ARRAY_SIZE(default_routing), 0);
}
```
- 0-15 gsi, set ROUTING_ENTRY2, higher gsi set ROUTING_ENTRY1


```c
struct kvm_irq_routing_entry {
	__u32 gsi;
	__u32 type;
	__u32 flags;
	__u32 pad;
	union {
		struct kvm_irq_routing_irqchip irqchip;
		struct kvm_irq_routing_msi msi;
		struct kvm_irq_routing_s390_adapter adapter;
		struct kvm_irq_routing_hv_sint hv_sint;
		__u32 pad[8];
	} u;
};

struct kvm_kernel_irq_routing_entry {
	u32 gsi;
	u32 type;
	int (*set)(struct kvm_kernel_irq_routing_entry *e,
		   struct kvm *kvm, int irq_source_id, int level,
		   bool line_status);
	union {
		struct {
			unsigned irqchip;
			unsigned pin;
		} irqchip;
		struct {
			u32 address_lo;
			u32 address_hi;
			u32 data;
			u32 flags;
			u32 devid;
		} msi;
		struct kvm_s390_adapter_int adapter;
		struct kvm_hv_sint hv_sint;
	};
	struct hlist_node link;
};
```
- [ ] How we setup kvm_kernel_irq_routing_entry ?
  - kvm_set_irq_routing
    * kvm_setup_default_irq_routing
      * KVM_CREATE_IRQCHIP
    * kvm_setup_empty_irq_routing
      * KVM_CAP_SPLIT_IRQCHIP
    * KVM_SET_GSI_ROUTING(Sets the GSI routing table entries, overwriting any previously set entries.)
    - setup_routing_entry
      - kvm_set_routing_entry

- [ ] What's relation between `kvm_kernel_irq_routing_entry` and `kvm_irq_routing_entry` ?
  - [ ] I don't know, but just some software trick to make code more beautiful
  - IRQ routing here is used for call multiple kvm_kernel_irq_routing_entry::set with the specific gsi

```c
/*
 * Return value:
 *  < 0   Interrupt was ignored (masked or not delivered for other reasons)
 *  = 0   Interrupt was coalesced (previous irq is still pending)
 *  > 0   Number of CPUs interrupt was delivered to
 */
int kvm_set_irq(struct kvm *kvm, int irq_source_id, u32 irq, int level,
		bool line_status)
{
	struct kvm_kernel_irq_routing_entry irq_set[KVM_NR_IRQCHIPS];
	int ret = -1, i, idx;

	trace_kvm_set_irq(irq, level, irq_source_id);

	/* Not possible to detect if the guest uses the PIC or the
	 * IOAPIC.  So set the bit in both. The guest will ignore
	 * writes to the unused one.
	 */
	idx = srcu_read_lock(&kvm->irq_srcu);
	i = kvm_irq_map_gsi(kvm, irq_set, irq); // kvm_irq_routing_table::map 会记录 irq 数量
	srcu_read_unlock(&kvm->irq_srcu, idx);

	while (i--) {
		int r;
		r = irq_set[i].set(&irq_set[i], kvm, irq_source_id, level,
				   line_status);
		if (r < 0)
			continue;

		ret = r + ((ret < 0) ? 0 : ret);
	}

	return ret;
}
```
## 3.4 MSI(X) 虚拟化
MSI 让中断不在受管脚约束，MSI 能够支持的中断数大大增加。支持 MSI 的设备绕过 I/O APIC，直接和 LAPIC 通过系统总线相连。

I/O APIC 从中断重定向表提取中断信息，而 MSI-X 是从 MSI-X Capability 提取信息，找到目标CPU，可以，MSI-X 就是将 I/O APIC的功能下沉到外设中间。

- [x] I/O APIC 的转发在什么位置?
  - arch/x86/kvm/ioapic.c:ioapic_set_irq 可以看到 `entry = ioapic->redirtbl[irq];`

## 3.5 硬件虚拟化支持
硬件支持的目的是为了减少 vmexit 啊

- virtual-APIC page : 在 Guest 中间拷贝一份 APIC page
- Guest 模式下的中断评估逻辑 :
  - Interrupt-window exiting : 当 退出到 host 的时候，如果 guest 此时在屏蔽中断，这样就只能等下一次 vmexit，但是中断不能等待太久，为了让中断快速响应
  - 通过 vmcs_write16(GUEST_INTR_STATUS, status) 直接写入，在 guest 中间就可以处理了
- posted-interrupt processing :

- [ ] 看不出来 posted interrupt processing 和 vmcs_write16(GUEST_INTR_STATUS, status) 有什么区别，不都是在让目标 CPU 在 guest 态中间不用退出

- [ ] 而且之前分析的 pic apic 之类的控制器的虚拟化和这里的技术没有冲突，但是书上给我的感觉都是非要 kick 一下 cpu 才可以

- [ ] 分析 IOMMU 的时候，都是假设收到中断的 CPU 和目标 CPU 不是一个 CPU, 但是如果中断从设备直接到达目标 CPU, 需要退出吗 ?

## 3.5.2
```c
static struct kvm_x86_ops vmx_x86_ops __initdata = {

	.hwapic_irr_update = vmx_hwapic_irr_update,
	.hwapic_isr_update = vmx_hwapic_isr_update,
```

- [ ] `.enable_irq_window = enable_irq_window,` if guest mode interrupt evaluation is enabled, how kvm take advantage of irq window ?


```c
static inline void apic_clear_irr(int vec, struct kvm_lapic *apic)
{
	struct kvm_vcpu *vcpu;

	vcpu = apic->vcpu;

	if (unlikely(vcpu->arch.apicv_active)) {
		/* need to update RVI */
		kvm_lapic_clear_vector(vec, apic->regs + APIC_IRR);
		kvm_x86_ops.hwapic_irr_update(vcpu,
				apic_find_highest_irr(apic));
	} else {
		apic->irr_pending = false;
		kvm_lapic_clear_vector(vec, apic->regs + APIC_IRR);
		if (apic_search_irr(apic) != -1)
			apic->irr_pending = true;
	}
}
```


[^3]: Inside the Linux Virtualization : Principle and Implementation
