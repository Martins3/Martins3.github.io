# intel 中断虚拟化，基于 狮子书

## 中断虚拟化
中断虚拟化的关键在于对中断控制器的模拟，我们知道 x86 上中断控制器主要有旧的中断控制器 PIC(intel 8259a)和适应于 SMP 框架的 IOAPIC/LAPIC 两种。

https://luohao-brian.gitbooks.io/interrupt-virtualization/content/qemu-kvm-zhong-duan-xu-ni-hua-kuang-jia-fen-679028-4e2d29.html

查询 GSI 号上对应的所有的中断号:

从 ioctl 到下层，kvm_vm_ioctl 注入的中断，最后更改了 kvm_kipc_state:irr

kvm_kipc_state 的信息如何告知 CPU ? 通过 kvm_pic_read_irq


# 8254 / 8259
KVM_CREATE_IRQCHIP :

https://en.wikipedia.org/wiki/Intel_8253

## irqchip
[KVM_CREATE_IRQCHIP](https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-create-irqchip)
- kvm_vm_ioctl
    - [ ] kvm_vm_ioctl_irq_line

## split irqchip
[KVM_CAP_SPLIT_IRQCHIP](https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-cap-split-irqchip)

- [ ] kvm_vm_ioctl_enable_cap_generic


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

# 狮子书笔记

### 3.3.23 IRQ routing
在加入 APIC 虚拟化后，当外设发送中断请求后，那么 KVM 模块究竟是通过 8259A 还是通过 APIC 向 Guest 注入中断?

这里的 routing 和 ioapic 将中断信息 routing 到 CPU 上其实不是一个事情，这里的 routing 是为了解决中断控制器的选择

gsi 可以理解为中断控制器的引脚线，当 gsi 小于 16 的时候，gsi 可能是 8259A 和 APIC 两个中断控制器的中断，所以需要对于这两个控制器都进行调用。

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

I/O APIC 从中断重定向表提取中断信息，而 MSI-X 是从 MSI-X Capability 提取信息，找到目标 CPU，可以，MSI-X 就是将 I/O APIC 的功能下沉到外设中间。

- [x] I/O APIC 的转发在什么位置?
  - arch/x86/kvm/ioapic.c:ioapic_set_irq 可以看到 `entry = ioapic->redirtbl[irq];`

## 3.5 硬件虚拟化支持
硬件支持的目的是为了减少 vmexit 啊

- virtual-APIC page : 在 Guest 中间拷贝一份 APIC page
- Guest 模式下的中断评估逻辑 :
  - Interrupt-window exiting : 当 退出到 host 的时候，如果 guest 此时在屏蔽中断，这样就只能等下一次 vmexit，但是中断不能等待太久，为了让中断快速响应
  - 通过 vmcs_write16(GUEST_INTR_STATUS, status) 直接写入，在 guest 中间就可以处理了
- posted-interrupt processing

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

## 时钟中断是如何注入的?
<!-- 9e5aa464-e384-48fc-9a23-68aebe6948d3 -->

| 场景 | 调用栈 | 说明 |
|------|--------|------|
| Guest 运行中 | `kvm_lapic_expired_hv_timer` → `apic_timer_expired` → `kvm_apic_local_deliver` | hv_timer 在 vmexit 时检查 |
| Guest 无负载 | `restart_apic_timer` → `start_hv_timer` → ... | 直接注入，无需 vmexit |

**关键机制:**
1. **Posted Interrupt**: APICv 特性允许直接注入，无需 VM-Exit
2. **vAPIC**: 虚拟 APIC，支持 trap-like 处理 (完成指令后再 exit)
3. **Vector 236 (0xec)**: 本地定时器向量 `LOCAL_TIMER_VECTOR`

**为什么 `trace_kvm_inj_virq` 看不到:**
- 该 tracepoint 只在非 vAPIC 模式下触发，那是 legacy 的路线
- 现代 KVM 默认启用 APICv，走 `vmx_deliver_interrupt` 路径

验证:
```sh
sudo perf top -e kvm:kvm_apicv_accept_irq
```
看到 vector 57 (virtio) 和 236 (timer)

感觉哪里怪怪的，首先我的确是记得时钟中断的

(这些时钟中断的注入可以做做实验，似乎时钟中断的注入需要特别多的 CPU)

### 之前的分析
似乎是在这里:

guset 负载高的时候:
```txt
@[
    kvm_apic_local_deliver+5
    apic_timer_expired+164
    kvm_lapic_expired_hv_timer+52
    vmx_vcpu_run+1865
    kvm_arch_vcpu_ioctl_run+1376
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 835
```

guest 无负载:
```txt
@[
    kvm_apic_local_deliver+5
    apic_timer_expired+164
    start_hv_timer+298
    restart_apic_timer+40
    handle_fastpath_set_msr_irqoff+343
    kvm_arch_vcpu_ioctl_run+1376
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    kvm_apic_local_deliver+5
    apic_timer_expired+164
    kvm_lapic_expired_hv_timer+52
    vmx_vcpu_run+1865
    kvm_arch_vcpu_ioctl_run+1376
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 20
@[
    kvm_apic_local_deliver+5
    kvm_inject_apic_timer_irqs+43
    kvm_arch_vcpu_ioctl_run+2186
    kvm_vcpu_ioctl+399
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 293
```

kvm_apic_local_deliver 可以看到 virtio 中断也是这里注入的。

使用了 iperf 之后，立刻可以看到:
```txt
Samples: 922K of event 'kvm:kvm_apic_accept_irq', 1 Hz, Event count (app Overhead  Trace output
  99.63%  apicid 0 vec 57 (Fixed|edge)
   0.36%  apicid 0 vec 236 (Fixed|edge)
   0.01%  apicid 0 vec 58 (Fixed|edge)
   0.01%  apicid 0 vec 44 (Fixed|edge)
```

vector 57 可以在 `grep "Vector.*57" /sys/kernel/debug/irq/irqs/*` 中验证。

vector 236 猜测是 timer 。这是由于:

```c
#define LOCAL_TIMER_VECTOR		0xec
```

### 为什么 trace_kvm_inj_virq 完全没有用

从 25.8.3 VM-Entry Controls for Event Injection 中来看

函数 kvm_check_and_inject_events -> vmx_inject_irq 操作了 `VM_ENTRY_INTR_INFO_FIELD` ，的确是中断注入的位置，但是
是 vapic 没有打开的时候没用。

但是 vmx_deliver_interrupt 是 vapic 打开的时候使用，而

## 这个文章写的好，可以仔细读读
https://compas.cs.stonybrook.edu/~mferdman/downloads.php/VEE15_Comprehensive_Implementation_and_Evaluation_of_Direct_Interrupt_Delivery.pdf

> APICv emulates APIC-access so that APIC-read requests no longer cause exits and APIC
write requests are transformed from fault-like VM exits into
trap-like VM exits, meaning that the instruction completes
before the VM exit and that processor state is updated by
the instruction.

关于 trap-like 和 fault-like 的含义，参考这里:
https://stackoverflow.com/questions/3149175/what-is-the-difference-between-trap-and-interrupt

*traps increment the instruction pointer, faults do not*

所以 trap-like 就是把事情干完了，平时的都是 fault like 了吧!

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
