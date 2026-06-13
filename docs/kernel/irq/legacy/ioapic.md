
# ioapic

https://zhuanlan.zhihu.com/p/26464793

## ioapic
- [ ] io apic 寄存器 && Redirection Table

## 为什么 timer 的中断无法发送到虚拟机中
如果 disable msi 的时候，virtio 可以发过去，
当然 ttyS0 的也是可以发送过去的:

```txt
@[
    bpf_trace_run4+144
    vmx_deliver_interrupt+320
    __apic_accept_irq+244
    kvm_irq_delivery_to_apic_fast+320
    kvm_irq_delivery_to_apic+103
    ioapic_service+311
    ioapic_set_irq+308
    kvm_ioapic_set_irq+101
    kvm_set_irq+192
    kvm_vm_ioctl_irq_line+39
    kvm_vm_ioctl+719
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 3222
```

## arch/x86/kernel/apic/io_apic.c 中也会触发 SMI

的确这里也是会有 smi 中断的
```c
static void clear_IO_APIC_pin(unsigned int apic, unsigned int pin)
{
	struct IO_APIC_route_entry entry;

	/* Check delivery_mode to be sure we're not clearing an SMI pin */
	entry = ioapic_read_entry(apic, pin);
	if (entry.delivery_mode == APIC_DELIVERY_MODE_SMI)
		return;

```


### how ioapic got programmed
ioapic 的作用的输入是引脚编号，其最后会告知一个 lapic 的哪一个中断到了

简单来说，这个引脚编号，就是对应着 gsi, 在内核中，也称之为 linux irq
通过调用函数 `irq_to_desc` 可以索引到 `irq_desc`
```c
struct irq_desc *irq_to_desc(unsigned int irq)
{
  return radix_tree_lookup(&irq_desc_tree, irq);
}
```
使用 `cat /proc/interrupts` 看到的是
```txt
            CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7
   0:          8          0          0          0          0          0          0          0  IR-IO-APIC    2-edge      timer
   1:         53          0          0          0          0          0        150          0  IR-IO-APIC    1-edge      i8042
   8:          0          0          0          0          0          0          0          1  IR-IO-APIC    8-edge      rtc0
   9:         13         13          0          0          0          0          0          0  IR-IO-APIC    9-fasteoi   acpi
  12:         83          0          0          0          0        143          0          0  IR-IO-APIC   12-edge      i8042
  14:       1000          1          0          0          0          0          0          0  IR-IO-APIC   14-fasteoi   INT344B:00
  16:       7726          0          0        756          0          0          0          0  IR-IO-APIC   16-fasteoi   idma64.0, i801_smbus, i2c_designware.0
  17:          0          0          0          0          0          0          0          0  IR-IO-APIC   17-fasteoi   idma64.1, i2c_designware.1
 120:          0          0          0          0          0          0          0          0  DMAR-MSI    0-edge      dmar0
 121:          0          0          0          0          0          0          0          0  DMAR-MSI    1-edge      dmar1
```
最左侧的编号就是 gsi 了。

vector index 就是 common_interrupt 中的参数，用于索引 `vector_irq`
vector index 实际上是 lapic 发送给 CPU 的中断数值，导致第 vector index 的 idt 被执行。
```c
DEFINE_PER_CPU(vector_irq_t, vector_irq) = {
  [0 ... NR_VECTORS - 1] = VECTOR_UNUSED,
};
```

Understanding Linux Kernel 中的 Table 4-2. Interrupt vectors in Linux

| Vector range        | Use                                                                                                         |
|---------------------|-------------------------------------------------------------------------------------------------------------|
| 0–19 (0x0-0x13)     | Nonmaskable interrupts and exceptions                                                                       |
| 20–31 (0x14-0x1f)   | Intel-reserved                                                                                              |
| 32–127 (0x20-0x7f)  | External interrupts (IRQs)                                                                                  |
| 128 (0x80)          | Programmed exception for system calls (see Chapter 10)                                                      |
| 129–238 (0x81-0xee) | External interrupts (IRQs)                                                                                  |
| 239 (0xef)          | Local APIC timer interrupt (see Chapter 6)                                                                  |
| 240 (0xf0)          | Local APIC thermal interrupt (introduced in the Pentium 4 models)                                           |
| 241–250 (0xf1-0xfa) | Reserved by Linux for future use                                                                            |
| 251–253 (0xfb-0xfd) | Interprocessor interrupts (see the section "Interprocessor Interrupt Handling" later in this chapter)       |
| 254 (0xfe)          | Local APIC error interrupt (generated when the local APIC detects an erroneous condition)                   |
| 255 (0xff)          | Local APIC spurious interrupt (generated if the CPU masks an interrupt while the hardware device raises it) |

其中用于 External interrupts 的那些 vector 数值就是 vector index 的可以取值。

而 ioapic 中映射表其实存储了 gsi 到 vector index 的。


下面分析一个经典例子:
```txt
/*
#0  apic_update_irq_cfg (irqd=irqd@entry=0xffff88810004e840, vector=33, cpu=1) at arch/x86/kernel/apic/vector.c:120
#1  0xffffffff810bd9a8 in assign_vector_locked (irqd=irqd@entry=0xffff88810004e840, dest=dest@entry=0xffffffff82efdb60 <vector_searchmask>) at arch/x86/kernel/apic/vector.c:253
#2  0xffffffff810bdb9b in assign_irq_vector_any_locked (irqd=0xffff88810004e840) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff810bdfa4 in activate_reserved (irqd=0xffff88810004e840) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004e840, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff81136d4e in __irq_domain_activate_irq (irqd=0xffff88810004e840, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1761
#6  0xffffffff81136d2d in __irq_domain_activate_irq (irqd=irqd@entry=0xffff888100100a28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1758
#7  0xffffffff811387f0 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff888100100a28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1784
#8  0xffffffff81135bfb in irq_activate (desc=desc@entry=0xffff888100100a00) at kernel/irq/chip.c:291
#9  0xffffffff81133515 in __setup_irq (irq=irq@entry=9, desc=desc@entry=0xffff888100100a00, new=new@entry=0xffff888100325a00) at kernel/irq/manage.c:1709
#10 0xffffffff81133a57 in request_threaded_irq (irq=9, handler=handler@entry=0xffffffff814e36f0 <acpi_irq>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, irqflags=irqflags@entry=128, devname=devname@entry=0xffffffff8243c085 "acpi", dev_id=dev_id@entry=0xffffffff814e36f0 <acpi_irq>) at kernel/irq/manage.c:2173
#11 0xffffffff814e3af7 in request_irq (dev=0xffffffff814e36f0 <acpi_irq>, name=0xffffffff8243c085 "acpi", flags=128, handler=0xffffffff814e36f0 <acpi_irq>, irq=<optimized out>) at ./include/linux/interrupt.h:167
#12 acpi_os_install_interrupt_handler (gsi=9, handler=handler@entry=0xffffffff814ffd00 <acpi_ev_sci_xrupt_handler>, context=0xffff888100309ae0) at drivers/acpi/osl.c:586
#13 0xffffffff814ffd47 in acpi_ev_install_sci_handler () at drivers/acpi/acpica/evsci.c:156
#14 0xffffffff814fd4c3 in acpi_ev_install_xrupt_handlers () at drivers/acpi/acpica/evevent.c:94
#15 0xffffffff82ddf987 in acpi_enable_subsystem (flags=flags@entry=2) at drivers/acpi/acpica/utxfinit.c:184
#16 0xffffffff82dddca0 in acpi_bus_init () at drivers/acpi/bus.c:1230
#17 acpi_init () at drivers/acpi/bus.c:1323
#18 0xffffffff81000def in do_one_initcall (fn=0xffffffff82dddc1c <acpi_init>) at init/main.c:1278
#19 0xffffffff82daa3d3 in do_initcall_level (command_line=0xffff8881001277c0 "root", level=4) at ./include/linux/compiler.h:250
#20 do_initcalls () at init/main.c:1367
#21 do_basic_setup () at init/main.c:1387
#22 kernel_init_freeable () at init/main.c:1589
#23 0xffffffff81c38121 in kernel_init (unused=<optimized out>) at init/main.c:1481
#24 0xffffffff81001992 in ret_from_fork () at arch/x86/entry/entry_64.S:295
#25 0x0000000000000000 in ?? ()
```

在 acpi_ev_install_sci_handler 中，传递给 acpi_os_install_interrupt_handler 的参数 gsi 的数值是从 acpi_gbl_FADT.sci_interrupt 中获取的。
这非常符合逻辑，因为中断接入到哪一个引脚上的信息就是从 acpi 上获取的，acpi 的信息需要和主板上是对应的。
而 vector index 则是动态生成的。

在 irq_domain_activate_irq 会递归的调用 irq_domain_ops::activate 的，从 parent 开始，也即是 lapic 然后是 ioapic 的
在 lapic 的 irq_domain_ops::activate 调用中，分配出来 vector index, 而在 ioapic 的 irq_domain_ops::activate 中，
建立 vector index 和 gsi（也即是 ioapic 的引脚）的联系，这是靠写 ioapic 的地址空间实现的。

- x86_vector_activate
  - activate_reserved
    - assign_irq_vector_any_locked
      - assign_vector_locked
        - irq_matrix_alloc : irq_matrix::alloc_start 和 irq_matrix::alloc_end 决定了分配的起始位置
        - apic_update_vector :  通过 gsi 用于索引出来一个 irq_desc 之后，该 desc 最后赋值给 vector_irq 上
        - apic_update_irq_cfg
          - apicd->hw_irq_cfg.vector = vector; // 将分配出来的 vector 存储到此处, 给接下来 ioapic 更新路由表使用

- mp_irqdomain_activate
  - ioapic_configure_entry : 看 `__add_pin_to_irq_node` 的注释: The common case is 1:1 `IRQ<->pin` mappings.
      - ioapic_setup_msg_from_msi : 构造 msi
          - irq_chip_compose_msi_msg :  构建 struct msi_msg
              - irq_chip::irq_compose_msi_msg
                - x86_vector_msi_compose_msg  : 这就是注册的 hook 函数
                  - __x86_vector_msi_compose_msg
                    - msg->arch_data.vector = cfg->vector; // 使用 x86_vector_activate 初始化出来的 vector
          - 使用 struct msi_msg 来填充 struct IO_APIC_route_entry
      - `__ioapic_write_entry` : 将 msi 信息写入到 ioapic 的重定向表格

现在可以回答刚才的问题:
> 在 common_interrupt 的参数 vector = 37 和 nvme_irq 的参数 irq=24 分别值得是什么?

vector = 37 就是 vector index, 而  irq = 24 是 gsi 也即是 linux irq.

## firecracker 的 virtio 设备中断 走的真的是 ioapic 吗?

```txt
  cat /proc/interrupts
           CPU0       CPU1       CPU2       CPU3
 24:          0          0          0          0  IO-APIC   9-edge      ACPI:Ged
 25:          0          0          0       7662  IO-APIC   5-edge      virtio0
 26:          0        132          0          0  IO-APIC   6-edge      virtio1
 27:        293          0          0          0  IO-APIC   7-edge      virtio2
 29:          0          0        613          0  IO-APIC   4-edge      ttyS0
 31:          0          8          0          0  IO-APIC   1-edge      i8042
NMI:          0          0          0          0   Non-maskable interrupts
LOC:       4835       4682      12680      31736   Local timer interrupts
SPU:          0          0          0          0   Spurious interrupts
PMI:          0          0          0          0   Performance monitoring interrupts
IWI:          0          0          0          0   IRQ work interrupts
RTR:          0          0          0          0   APIC ICR read retries
RES:       1780       1101       1165        309   Rescheduling interrupts
CAL:       9617       5745      11371       9395   Function call interrupts
TLB:        136        136        124         45   TLB shootdowns
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
似乎中断的处理过程没什么区别?
```txt
-   31.03%     0.01%  fc_vcpu 0    [kernel.kallsyms]                        [k] __x64_sys_ioctl
   - 31.02% __x64_sys_ioctl
      - 30.95% kvm_vcpu_ioctl
         - 30.90% kvm_arch_vcpu_ioctl_run
            - 27.86% kvm_vcpu_halt
               - 13.26% kvm_vcpu_check_block
                  - 9.87% kvm_arch_vcpu_runnable
                     - kvm_vcpu_has_events
                        - 7.02% kvm_cpu_has_interrupt
                           - 3.53% apic_has_interrupt_for_ppr
                              - vmx_sync_pir_to_irr
                                   1.48% kvm_lapic_find_highest_irr
                                   0.71% vmx_set_rvi
                           - 2.78% kvm_apic_has_interrupt
                                __apic_update_ppr
                        - 1.44% vmx_interrupt_allowed
                             __vmx_interrupt_blocked
                    1.80% __srcu_read_lock
                    0.96% __srcu_read_unlock
               - 5.08% ktime_get
                    2.97% read_tsc
            - 1.14% vmx_handle_exit
               - 1.11% kvm_mmu_page_fault
                  - x86_emulate_instruction
                     - 0.58% x86_decode_emulated_instruction
                          0.55% x86_decode_insn
            - 1.08% vcpu_enter_guest.constprop.0
```

## arch/x86/kernel/apic/io_apic.c 中定义的 lapic_chip 是做什么的
```c
static struct irq_chip lapic_chip __read_mostly = {
	.name		= "local-APIC",
	.irq_mask	= mask_lapic_irq,
	.irq_unmask	= unmask_lapic_irq,
	.irq_ack	= ack_lapic_irq,
};
```


# 分析 x86 中断控制器的驱动

现在的问题的转机在于可以阅读下 AMD 的手册，然后从虚拟化的角度分析。


主要是分析 arch/x86/kernel/apic 和

| File             | blank | comment | code | explanation |
|------------------|-------|---------|------|-------------|
| io_apic.c        | 449   | 560     | 2063 |             |
| apic.c           | 413   | 758     | 1711 |             |
| x2apic_uv_x.c    | 274   | 107     | 1193 |             |
| vector.c         | 189   | 218     | 848  |             |
| msi.c            | 75    | 79      | 361  |             |
| apic_numachip.c  | 70    | 18      | 250  |             |
| ipi.c            | 55    | 68      | 208  |             |
| x2apic_cluster.c | 44    | 9       | 173  |             |
| apic_flat_64.c   | 48    | 35      | 168  |             |
| probe_32.c       | 38    | 25      | 152  |             |
| x2apic_phys.c    | 41    | 2       | 147  |             |
| bigsmp_32.c      | 41    | 13      | 138  |             |
| apic_noop.c      | 28    | 22      | 95   |             |
| local.h          | 12    | 16      | 41   |             |
| hw_nmi.c         | 7     | 11      | 41   |             |
| probe_64.c       | 7     | 13      | 35   |             |
| apic_common.c    | 7     | 5       | 34   |             |
| Makefile         | 6     | 9       | 15   |             |

## 实现分析

arch/x86/kvm/lapic.c

tokei -f lapic.c ioapic.c i8254.c i8259.c

| Language | Files | Lines | Code | Comments | Blanks | explanation |
|----------|-------|-------|------|----------|--------|-------------|
| i8259.c  |       | 660   | 503  | 79       | 78     | 控制器      |
| i8254.c  |       | 751   | 557  | 84       | 110    | 时钟        |
| ioapic.c |       | 776   | 554  | 112      | 110    |             |
| lapic.c  |       | 3319  | 2342 | 418      | 559    |             |

## pic
Interrupt	sequence:
– Interrupt	controller	raises	INT	line
– 80386	core	pulses	INTA	line	low,	allowing	INT	to	go	low
– 80386	core	pulses	INTA	line	low	again,	signaling	controller	to put	interrupt	number	on	data	bus

> 一共三根线 : interrupt line, interrupt acknowledge line, Data bus 参与

## [ ] 为什么 io_apic.c 作为 driver 代码那么多，但是模拟要少那么多?

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
