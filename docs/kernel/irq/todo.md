## 好家伙，现在完全可以搞一个中断的彻底分析

- tty 设备直接链接到物理机机器的
- 键盘鼠标直接链接到物理机机器的接到 usb 后面的
- 时钟中断
- msi 中断
- 中断虚拟化
- qemu 的 chip 模式
- iommu 使用 ir
- 虚拟机中使用 ir
- 嵌套虚拟化使用 ir


## 到底是什么时候打开中断来着
handle_fasteoi_irq 中出现了 mask_irq ，也就是执行到 handle_fasteoi_irq 的时候，中断已经打开了?

```c
	/*
	 * If its disabled or no action available
	 * then mask it and get out of here:
	 */
	if (unlikely(!desc->action || irqd_irq_disabled(&desc->irq_data))) {
		desc->istate |= IRQS_PENDING;
		mask_irq(desc);
		goto out;
	}
```

## 这里下这个 backtrace

```txt
@[
    exit_to_user_mode_prepare+1
    irqentry_exit_to_user_mode+5
    asm_exc_page_fault+30
]: 3
@[
    exit_to_user_mode_prepare+1
    irqentry_exit_to_user_mode+5
    asm_sysvec_apic_timer_interrupt+18
]: 4
@[
    exit_to_user_mode_prepare+1
    syscall_exit_to_user_mode+34
    do_syscall_64+72
    entry_SYSCALL_64_after_hwframe+68
]: 225
```
## 最后一个问题，看看 hwirq 是如何映射的
其余的都还好

## 为什么需要使用 apic 来提供 cpu 的标志

qemu 的确是如此模拟的


## 为什么需要中断路由来着，一个中断发送给好几个 CPU

## what's percpu's meaning in handle_irq_event_percpu?

## 先把 apic 一共有哪些寄存器放到一个醒目的位置吧

## 总结下
如下似乎场景的，那些共同点，基本都做什么:
- ret_from_exception()
- ret_from_intr()
- ret_from_sys_call()
- ret_from_fork()

## TODO
- 分析中断和各种锁的影响
- 分析中断和调度的影响 : preemption
- 在虚拟化中的中断

## Question
- [ ] 总结 从 ics 的中断 和 ucore 的中断的实现，然后再去分析
- [ ] fwnode 是做什么的 ?
- [ ] handle level 和 handle edge 都是在搞什么?

## TODO
- [answer this question](https://unix.stackexchange.com/questions/491437/how-does-linux-kernel-switches-from-kernel-stack-to-interrupt-stack?rq=1)
- [ ] http://wiki.0xffffff.org/posts/hurlex-8.html : used for understand 8259APIC
- [ ] 总结从 idt 分别到 interrupt 和 exception 的过程
- [ ] 所以 CPU 为什么需要 debug 的 exception 啊 ?
- [ ] https://os.phil-opp.com/cpu-exceptions/#the-interrupt-stack-frame 和 insides 处理各种细节应该就可以了吧 !

## gpio
https://github.com/Manawyrm/pata-gpio

### Exception Handling
[^6]
- Most	error	excepWons	—	divide by zero,	invalid operation,	illegal	memory	reference,	etc.
—	translate directly	into	signals: `force_sig(sig_number,	current);`
- An exception	can	(infrequently)	happen	in	the	kernel : `die()`;	//	kernel	oops

- [ ] 之前疑惑于为什么 debug 需要 exception 中间的 debug handler, 比如 software breakpint 采用将目标指令替换为 `int 3`
  - https://stackoverflow.com/questions/14031930/how-to-break-on-instruction-with-a-specific-opcode-in-gdb/31249378#31249378

### Interrupt Handling
asm_common_interrupt => handle_irq => run_irq_on_irqstack_cond

```c
static __always_inline void
run_irq_on_irqstack_cond(void (*func)(struct irq_desc *desc), struct irq_desc *desc,
			 struct pt_regs *regs)
{
	lockdep_assert_irqs_disabled();

	if (irq_needs_irq_stack(regs))
		__run_irq_on_irqstack(func, desc);
	else
		func(desc);
}
```

- [ ] irq_desc::handler
    - [ ] `__irq_do_set_handler` : currently this is only function where irq_desc::handler is set


```c
const struct irq_domain_ops mp_ioapic_irqdomain_ops = {
	.alloc		= mp_irqdomain_alloc,
	.free		= mp_irqdomain_free,
	.activate	= mp_irqdomain_activate,
	.deactivate	= mp_irqdomain_deactivate,
};
```
mp_irqdomain_alloc ==> mp_register_handler ==> `__irq_set_handler` ==> `__irq_do_set_handler`

`__irq_domain_alloc_irqs` ==> irq_domain_alloc_irqs_hierarchy


## isa and pci
[^1]:
ISA and PCI handle interrupts very differently. ISA expansion cards are configured manually for IRQ, usually by setting a jumper, but sometimes by running a setup program. All ISA slots have all IRQ lines present, so it doesn’t matter which card is placed in which slot. ISA cards use edge-sensitive interrupts, which means that an ISA device asserts a voltage on one of the interrupt lines to generate an interrupt. That in turn means that ISA devices cannot share interrupts because when the processor senses voltage on a particular interrupt line, it has no way to determine which of multiple devices might be asserting that interrupt. For ISA slots and devices, the rule is simple: two devices cannot share an IRQ if there is any possibility that those two devices may be used simultaneously. In practice that means that you cannot assign the same IRQ to more than one ISA device.

PCI cards use level-sensitive interrupts, which means that different PCI devices can assert different voltages on the same physical interrupt line, allowing the processor to determine which device generated the interrupt. PCI cards and slots manage interrupts internally. A PCI bus normally supports a maximum of four PCI slots, numbered 1 through 4. Each PCI slot can access four interrupts, labeled INT#1 through INT#4 (or INT#A through INT#D). Ordinarily, INT#1/A is used by PCI Slot 1, INT#2/B by Slot 2, and so on.



## fasteoi 是什么含义?
```txt
           CPU0       CPU1
  0:         49          0   IO-APIC   2-edge      timer
  1:          9          0   IO-APIC   1-edge      i8042
  4:         12          0   IO-APIC   4-edge      ttyS0
  6:          2          0   IO-APIC   6-edge      floppy
  8:          1          0   IO-APIC   8-edge      rtc0
  9:          0          0   IO-APIC   9-fasteoi   acpi
```
https://stackoverflow.com/questions/7005331/difference-between-io-apic-fasteoi-and-io-apic-edge

## 将 sysfs 中内容都看看
docs/kernel/sysfs-pci.md

### pci 下的 irq 是什么？

## 问题

现在会想起来这里是有问题的，如果 pci line 的中断是通过的
pci_read_irq config 获取的

```c
/*
 * Read interrupt line and base address registers.
 * The architecture-dependent code can tweak these, of course.
 */
static void pci_read_irq(struct pci_dev *dev)
{
	unsigned char irq;

	/* VFs are not allowed to use INTx, so skip the config reads */
	if (dev->is_virtfn) {
		dev->pin = 0;
		dev->irq = 0;
		return;
	}

	pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq);
	dev->pin = irq;
	if (irq)
		pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irq);
	dev->irq = irq;
}
```

但是这个 irq 又是配置作为参数传递给 request_irq ，实际上是 linux irq。
例如 igc_request_irq

但是 linux irq 不是动态分配的吗?

以及这里
```c
	error = request_irq(I8042_AUX_IRQ, i8042_interrupt, IRQF_SHARED,
			    "i8042", i8042_platform_device);
```

## 看看 x86 和 arm 的 init_IRQ 的差别

arm 环境中的:
```c
void __init init_IRQ(void)
{
	init_irq_stacks();
	init_irq_scs();
	irqchip_init();

	if (system_uses_irq_prio_masking()) {
		/*
		 * Now that we have a stack for our IRQ handler, set
		 * the PMR/PSR pair to a consistent state.
		 */
		WARN_ON(read_sysreg(daif) & PSR_A_BIT);
		local_daif_restore(DAIF_PROCCTX_NOIRQ);
	}
}
```
这是一个有趣的对比


## 通过 /proc/irq/*/spurious 的实现在
虚拟机也可以观测到
```txt
🧀  cat /proc/irq/*/spurious
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 24
unhandled 24
last_unhandled 4294668877 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 15350
unhandled 1
last_unhandled 26441813 ms
count 0
unhandled 0
last_unhandled 0 ms
count 6702
unhandled 1
last_unhandled 26428009 ms
count 0
unhandled 0
last_unhandled 0 ms
count 22144
unhandled 1
last_unhandled 33266301 ms
count 21762
unhandled 1
last_unhandled 37540110 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 2856
unhandled 1
last_unhandled 26272333 ms
count 0
unhandled 0
last_unhandled 0 ms
count 2656
unhandled 1
last_unhandled 33596328 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 691
unhandled 1
last_unhandled 4294678091 ms
count 0kvmvapic
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
count 0
unhandled 0
last_unhandled 0 ms
```

实现的原理是，如果有这么多没有实现，那么就报错:
```c
	desc->irq_count++;
	if (likely(desc->irq_count < 100000))
		return;

	desc->irq_count = 0;
	if (unlikely(desc->irqs_unhandled > 99900)) {
```

实现在 note_interrupt  ，对于 softirq 中处理就可以了，这个

如果返回 action_ret == IRQ_NONE ，。

## 为什么需要 iommu 来转发中断?

### 不考虑中断虚拟化，既然 iommu 可以调整中断，msi 也可以
当需要 set irq affnitity 的时候，要去配置哪一个，为什么？

- apic_set_affinity : 决定 idt 是可以发送到哪一个 CPU
- intel_ir_set_affinity : 更新 irte 中的内容，表示其指向哪一个 CPU
- msi_domain_set_affinity : 更新 msi data ，表示指向哪一个 itre

仔细观察这三个函数，发现，都是会判断 parent 是否会成功，如果成功了就不会去尝试搞。

## TODO
```txt
#0  nvme_irq (irq=24, data=0xffff888101150e00) at drivers/nvme/host/pci.c:1066
#1  0xffffffff810bb448 in __handle_irq_event_percpu (desc=desc@entry=0xffff888101163200, flags=flags@entry=0xffffc90000003f84) at kernel/irq/handle.c:156
#2  0xffffffff810bb58c in handle_irq_event_percpu (desc=desc@entry=0xffff888101163200) at kernel/irq/handle.c:196
#3  0xffffffff810bb603 in handle_irq_event (desc=desc@entry=0xffff888101163200) at kernel/irq/handle.c:213
#4  0xffffffff810bf4c9 in handle_edge_irq (desc=0xffff888101163200) at kernel/irq/chip.c:819
#5  0xffffffff81021c19 in generic_handle_irq_desc (desc=0xffff888101163200) at ./include/linux/irqdesc.h:158
#6  handle_irq (regs=<optimized out>, desc=0xffff888101163200) at arch/x86/kernel/irq.c:231
#7  __common_interrupt (regs=<optimized out>, vector=37) at arch/x86/kernel/irq.c:250
#8  0xffffffff81b94c46 in common_interrupt (regs=0xffffc90000013868, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
这里留下一个问题，在 `common_interrupt` 的参数 vector = 37 和 `nvme_irq` 的参数 irq=24 分别值得是什么?

## 解决 kvm 中哪些乱七八糟的 pic ioapic gsi 整理问题

### 看一眼 io-apic 的模拟

kvm_ioapic_set_irq 调用非常频繁

```txt
@[
    kvm_ioapic_set_irq+5
    kvm_set_irq+192
    kvm_vm_ioctl_irq_line+39
    kvm_vm_ioctl+719
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 882
```

在 ioapic_set_irq 中
```txt
  99.98%  pin 2 dst 0 vec 48 (Fixed|physical|edge)
   0.01%  pin 10 dst 3 vec 39 (Fixed|physical|level)
   0.00%  pin 8 dst 4 vec 36 (Fixed|physical|edge)
```

在虚拟机这，也就这个最像了，因为这里的 vector 也是 48 :
```txt
➜  irqs cat 0
handler:  handle_edge_irq
device:   (null)
status:   0x00002000
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x19400600
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_NO_BALANCING
            IRQD_SINGLE_TARGET
            IRQD_AFFINITY_ON_ACTIVATE
            IRQD_HANDLE_ENFORCE_IRQCTX
node:     0
affinity: 0-63
effectiv: 0
pending:
domain:  IO-APIC-0
 hwirq:   0x2
 chip:    IO-APIC
  flags:   0x410
             IRQCHIP_SKIP_SET_WAKE
 parent:
    domain:  VECTOR
     hwirq:   0x0
     chip:    APIC
      flags:   0x0
     Vector:    48
     Target:     0
     move_in_progress: 0
     is_managed:       0
     can_reserve:      0
     has_reserved:     0
     cleanup_pending:  0
```


似乎是 rtc 导致的，qemu 的这个开销不小啊

但是奇怪的是中断没有接受多少。
```txt
           CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7
  0:         55          0          0          0          0          0          0          0   IO-APIC   2-edge      timer
  1:          0          0          0          9          0          0          0          0   IO-APIC   1-edge      i8042
  4:          9          0          0          0          0          0          0          0   IO-APIC   4-edge      ttyS0
  6:          0          0          0          2          0          0          0          0   IO-APIC   6-edge      floppy
  8:          0          0          0          0          1          0          0          0   IO-APIC   8-edge      rtc0
  9:          0          0          0          0          0          0          0          0   IO-APIC   9-fasteoi   acpi
 10:          0          0          0         62          0          0          0          0   IO-APIC  10-fasteoi   virtio6
 11:          0          0          1          0          0          0          0          0   IO-APIC  11-fasteoi   virtio0
 12:          0          0        125          0          0          0          0          0   IO-APIC  12-edge      i8042
 14:          0          0          0          0          0          0          0          0   IO-APIC  14-edge      ata_piix
 15:          0          0          0          0          0          0          0          0   IO-APIC  15-edge      ata_piix
```

启动之后，host 中完全观测不到:
```txt
sudo perf trace -e kvm:kvm_inj_virq
```

这是因为 ioapic_set_irq 中基本走到了:
```c
	if (!irq_level) {
		ioapic->irr &= ~mask;
		ret = 1;
		goto out;
	}
```
根本调用不到 ioapic_service 中，这里可以深入调查下，至少可以优化下。

### [ ] 如果使用 qemu  hw/intc/ioapic.c 之后，中断是如何注入的 ?

### [ ] kvm 模式下，lapic 可以用 qemu 模拟吗?

看上去是不行的哈
```c
static void apic_set_irq(APICCommonState *s, int vector_num, int trigger_mode)
{
    kvm_report_irq_delivered(!apic_get_bit(s->irr, vector_num));

    apic_set_bit(s->irr, vector_num);
    if (trigger_mode)
        apic_set_bit(s->tmr, vector_num);
    else
        apic_reset_bit(s->tmr, vector_num);
    if (s->vapic_paddr) {
        apic_sync_vapic(s, SYNC_ISR_IRR_TO_VAPIC);
        /*
         * The vcpu thread needs to see the new IRR before we pull its current
         * TPR value. That way, if we miss a lowering of the TRP, the guest
         * has the chance to notice the new IRR and poll for IRQs on its own.
         */
        smp_wmb();
        apic_sync_vapic(s, SYNC_FROM_VAPIC);
    }
    apic_update_irq(s);
}
```

## 测试一下，中断在内核和在 qemu 的差别吧

## interrupt shadow 是做什么的

```c
	.set_interrupt_shadow = vmx_set_interrupt_shadow,
	.get_interrupt_shadow = vmx_get_interrupt_shadow,
```

|-----------------------------|--------|
| GUEST_INTERRUPTIBILITY_INFO | 27.7.1 |

除了 smm ，似乎主要的位置是:

x86_emulate_instruction 中的:

```c
	if (writeback) {
		unsigned long rflags = static_call(kvm_x86_get_rflags)(vcpu);
		toggle_interruptibility(vcpu, ctxt->interruptibility);
		vcpu->arch.emulate_regs_need_sync_to_vcpu = false;
```

而且这里的 KVM_X86_SHADOW_INT_MOV_SS 如何理解?


## 这些奇葩的中断分析，在 arm 中也走一遍吧


## amd64/intel-sdm/v3-ch30.md

30.2.2 中，四个不可以注入的条件
什么  "interrupt-window exiting" VM-execution control 是 0 的时候，不可以进行


## firecracker 中 /proc/interrupts 中结果

```txt
🧀  cat /proc/interrupts
           CPU0       CPU1       CPU2       CPU3
 24:          0          0          0          0  IO-APIC   9-edge      ACPI:Ged
 25:          0          0          0       7532  IO-APIC   5-edge      virtio0
 26:        108          0          0          0  IO-APIC   6-edge      virtio1
 27:          0        293          0          0  IO-APIC   7-edge      virtio2
 29:          0          0        607          0  IO-APIC   4-edge      ttyS0
 31:          0          8          0          0  IO-APIC   1-edge      i8042
NMI:          0          0          0          0   Non-maskable interrupts
LOC:      11475      15120       8680      18404   Local timer interrupts
SPU:          0          0          0          0   Spurious interrupts
PMI:          0          0          0          0   Performance monitoring interrupts
IWI:          0          0          0          0   IRQ work interrupts
RTR:          0          0          0          0   APIC ICR read retries
RES:       1105       1002       1294        625   Rescheduling interrupts
CAL:       6206      10230      11669       7806   Function call interrupts
TLB:         88        165         80        156   TLB shootdowns
TRM:          0          0          0          0   Thermal event interrupts
THR:          0          0          0          0   Threshold APIC interrupts
DFR:          0          0          0          0   Deferred Error APIC interrupts
MCE:          0          0          0          0   Machine check exceptions
MCP:          1          1          1          1   Machine check polls
HYP:          1          1          1          1   Hypervisor callback interrupts
ERR:          0
MIS:          0
PIN:          0          0          0          0   Posted-interrupt notification event
NPI:          0          0          0          0   Nested posted-interrupt event
PIW:          0          0          0          0   Posted-interrupt wakeup event
```
1. ACPI:Ged 是如何知道注册上的
2. 可以结合 debugfs 看看他们用的都是哪一个 pin
3. virito 设备的中断如何配置的，现在只能给一个 cpu 发中断，难顶
4. i8042 是 keyboard ，有意义吗?
   - 都是通过 serial 连接设备了

# 附录
什么叫做 interrupt line 和 irq line 是一个东西吗 ?
https://reverseengineering.stackexchange.com/questions/16975/whats-the-difference-between-an-interrupt-line-and-the-interrupt-number

# QEMU tcg 的中断模拟

## 问题
- [ ] make a table explaining every field of APIC and IOAPIC struct in QEMU

- [ ] 似乎没有整明白 : gsi_handler 和 qemu_set_irq 的关系
  - [ ] qdev_connect_gpio_out 等价的效果到底是什么
  - [ ] 比如 pic_realize 中的注册

- apic timer : 总体来说，timer 是比较容易处理的
  - apic_timer 被周期性的触发
    - [ ] 思考一下如何获取 clock time, 实际上，guest 操作系统可以主动校准实践
    - QEMU_CLOCK_VIRTUAL : 当虚拟机停下来的时候，时钟中断需要停止下来
  - 考虑一个小问题，所有的 vCPU 都是需要接受 local timer 的时钟的，难道为此需要创建出来多个 timer 吗 ?
    - 是的, 而且 timer 这个线程是在 main_loop_wait => qemu_clock_run_all_timers 中使用一个新的线程来进行的

- apic_update_irq 的分析
  - apic_poll_irq : 如果中断是来自于其他的 thread，那么就采用这种方式，比如时钟中断
    - 因为时钟是在另一个线程处理的，所以需要实现
  - 如果不是来自于 pic 的中断，那就清理掉这个中断

## isa
- [ ] 实际上，ISABus 中的只有成员 irq 有用，而且是通过 isa_bus_irqs 赋值的，实际上，没有啥作用的
  - 实际上，我猜测 ISABus 主要是为了 keyboard 之类的设备容易模拟吧

将 `x86ms->gsi` 赋值给 ISABus::irqs

```c
void isa_bus_irqs(ISABus *bus, qemu_irq *irqs)
{
    bus->irqs = irqs;
}
```

#### EOI
- [ ] apic_eoi : 和 10.8.5 中描述的一致，当 apic 接受到一个 EOIUpon receiving an EOI, the APIC clears the highest priority bit in the ISR and dispatches the next highest priority
interrupt to the processor.
  - [ ] 10.8.5 : 手册中间分析的 ioapic 的 broadcast 是什么意思
  - [x] apic_sync_vapic : 这个是处理 kvm 的，暂时不分析

*If the terminated interrupt was a level-triggered interrupt, the local APIC Also sends an
end-of-interrupt message to all I/O APICs.* (**无法理解为什么 level-triggered 的就需要向 io apic 发送**)

System software may prefer to direct EOIs to specific I/O APICs rather than having the local APIC send end-of-interrupt messages to all I/O APICs.

Software can inhibit the broadcast of EOI message by setting bit 12 of the *Spurious Interrupt Vector Register* (see
Section 10.9). If this bit is set, a broadcast EOI is not generated on an EOI cycle even if the associated *TMR* bit indicates that the current interrupt was level-triggered.
The default value for the bit is 0, indicating that EOI broadcasts are performed.

Bit 12 of the Spurious Interrupt Vector Register is reserved to 0 if the processor does not support suppression of
EOI broadcasts. Support for EOI-broadcast suppression is reported in bit 24 in the Local APIC Version Register (see
Section 10.4.8); the feature is supported if that bit is set to 1. When supported, the feature is available in both
xAPIC mode and x2APIC mode.

System software desiring to perform directed EOIs for level-triggered interrupts should set bit 12 of the *Spurious Interrupt Vector Register* and follow each the EOI to the local xAPIC for a level triggered interrupt with a directed
EOI to the I/O APIC generating the interrupt (this is done by writing to the I/O APIC’s EOI register).
System software performing directed EOIs must retain a mapping associating level-triggered interrupts with the I/O APICs in the system. (**并没有看懂这个英语，是如何实现 dedicated 的 EOI 的**)

- [ ] 实际上，ioapic 也是存在 eoi 的, 而且还在两个调用位置, 放到 tcg ioapic 中间分析吧

实际上，这个在 kvm 中也是很重要的:


## 这个文档也看看吧
Documentation/core-api/irq

## request irq
- [ ] 通过 irq_domain_alloc_descs 可以分配 irq_desc ，系统初始化的时候分配的

## https://lwn.net/Articles/302043/


## kernel/plka/1/plka-chapter-14.md


https://mp.weixin.qq.com/s/5GNMsotWZ05EjAGROoV60Q

## 这两个东西收藏很久了，需要处理一下

https://www.cnblogs.com/LoyenWang/p/13052677.html
https://zhuanlan.zhihu.com/p/26464793

## 这个 config 做什么的?
CONFIG_IRQ_POLL=y

打开 CONFIG_MEGARAID_SAS=m 的时候发现这个被打开

## 散落到其他位置的东西
docs/virtio/interrupt.md

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
