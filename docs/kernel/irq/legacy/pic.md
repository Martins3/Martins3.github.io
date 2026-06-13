
## noapic

kernel cmdline 的文档:
```txt
 noapic          [SMP,APIC,EARLY] Tells the kernel to not make use of any
                        IOAPICs that may be present in the system.

```

在 kernel 中添加参数: noapic ，在虚拟机中可以观察到这个现象:

```txt
➜  ~ cat /proc/interrupts
           CPU0
  0:         25    XT-PIC      timer
  1:         10    XT-PIC      i8042
  2:          0    XT-PIC      cascade
  4:         21    XT-PIC      ttyS0
  6:          2    XT-PIC      floppy
  8:          1    XT-PIC      rtc0
  9:          0    XT-PIC      acpi
 11:         25    XT-PIC      virtio5
 12:        125    XT-PIC      i8042
 14:          0    XT-PIC      ata_piix
 15:          0    XT-PIC      ata_piix
 24:          0  PCI-MSIX-0000:00:11.0   0-edge      virtio10-config
 25:         28  PCI-MSIX-0000:00:11.0   1-edge      virtio10-virtqueues
 26:          0  PCI-MSIX-0000:00:0d.0   0-edge      virtio7-config
 27:         84  PCI-MSIX-0000:00:0d.0   1-edge      virtio7-req.0
 28:          0  PCI-MSIX-0000:00:0e.0   0-edge      virtio8-config
 29:         79  PCI-MSIX-0000:00:0e.0   1-edge      virtio8-req.0
 30:          0  PCI-MSIX-0000:00:03.0   0-edge      virtio0-config
 31:          0  PCI-MSIX-0000:00:03.0   1-edge      virtio0-control
 32:          0  PCI-MSIX-0000:00:03.0   2-edge      virtio0-event
 33:       7080  PCI-MSIX-0000:00:03.0   3-edge      virtio0-request
 34:          0  PCI-MSIX-0000:00:0f.0   0-edge      virtio9-config
 35:          0  PCI-MSIX-0000:00:0f.0   1-edge      virtio9-control
 36:          0  PCI-MSIX-0000:00:0f.0   2-edge      virtio9-event
 37:        554  PCI-MSIX-0000:00:0f.0   3-edge      virtio9-request
 38:          0  PCI-MSIX-0000:00:04.0   0-edge      virtio1-config
 39:        167  PCI-MSIX-0000:00:04.0   1-edge      virtio1-input.0
 40:        163  PCI-MSIX-0000:00:04.0   2-edge      virtio1-output.0
 41:          0  PCI-MSIX-0000:00:05.0   0-edge      virtio2-config
 42:         13  PCI-MSIX-0000:00:05.0   1-edge      virtio2-input.0
 43:          0  PCI-MSIX-0000:00:05.0   2-edge      virtio2-output.0
 44:          0  PCI-MSIX-0000:00:06.0   0-edge      virtio3-config
 45:         13  PCI-MSIX-0000:00:06.0   1-edge      virtio3-input.0
 46:          0  PCI-MSIX-0000:00:06.0   2-edge      virtio3-output.0
 47:         10  PCI-MSIX-0000:00:0c.0   0-edge      nvme2q0
 48:         10  PCI-MSIX-0000:00:0b.0   0-edge      nvme1q0
 49:         76  PCI-MSIX-0000:00:0c.0   1-edge      nvme2q1
 50:         73  PCI-MSIX-0000:00:0b.0   1-edge      nvme1q1
 51:         10  PCI-MSIX-0000:00:0a.0   0-edge      nvme0q0
 52:          0  PCI-MSIX-0000:00:0a.0   1-edge      nvme0q1
 53:         66  PCI-MSIX-0000:00:10.0   0-edge      xhci_hcd
 55:          0  PCI-MSIX-0000:00:09.0   0-edge      virtio6-config
 56:          5  PCI-MSIX-0000:00:09.0   1-edge      virtio6-requests
 57:          0  PCI-MSIX-0000:00:07.0   0-edge      virtio4-config
 58:          0  PCI-MSIX-0000:00:07.0   1-edge      virtio4-virtqueues
NMI:          0   Non-maskable interrupts
LOC:      33573   Local timer interrupts
SPU:          0   Spurious interrupts
PMI:          0   Performance monitoring interrupts
IWI:          0   IRQ work interrupts
RTR:          0   APIC ICR read retries
RES:          0   Rescheduling interrupts
CAL:          0   Function call interrupts
TLB:          0   TLB shootdowns
TRM:          0   Thermal event interrupts
THR:          0   Threshold APIC interrupts
DFR:          0   Deferred Error APIC interrupts
MCE:          0   Machine check exceptions
MCP:          1   Machine check polls
HYP:          1   Hypervisor callback interrupts
ERR:          0
MIS:          0
PIN:          0   Posted-interrupt notification event
NPI:          0   Nested posted-interrupt event
PIW:          0   Posted-interrupt wakeup event
```

```txt
➜  irqs cat 4
handler:  handle_level_irq
device:   (null)
status:   0x00000500
            _IRQ_NOPROBE
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x00402200
            IRQD_LEVEL
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
node:     0
affinity: 0
effectiv:
pending:
domain:
 hwirq:   0x0
 chip:    XT-PIC
  flags:   0x0


➜  irqs cat 45
handler:  handle_edge_irq
device:   0000:00:06.0
status:   0x00000000
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x1d401200
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_SINGLE_TARGET
            IRQD_AFFINITY_SET
            IRQD_AFFINITY_ON_ACTIVATE
            IRQD_CAN_RESERVE
            IRQD_HANDLE_ENFORCE_IRQCTX
node:     -1
affinity: 0
effectiv: 0
pending:
domain:  PCI-MSIX-0000:00:06.0-12
 hwirq:   0x1
 chip:    PCI-MSIX-0000:00:06.0
  flags:   0x430
             IRQCHIP_SKIP_SET_WAKE
             IRQCHIP_ONESHOT_SAFE
 parent:
    domain:  VECTOR
     hwirq:   0x2d
     chip:    APIC
      flags:   0x0
     Vector:    69
     Target:     0
     move_in_progress: 0
     is_managed:       0
     can_reserve:      1
     has_reserved:     0
     cleanup_pending:  0
```

- [ ] 这里 APIC 还是在的吧，PIC 的中断是通过 lapic 的 LINT0 / LINT1 发生过来的吗?

比较还是通过 APIC 的，但是

## noapic pci=nomsi
```txt
➜  ~ cat /proc/interrupts
           CPU0
  0:         27    XT-PIC      timer
  1:         10    XT-PIC      i8042
  2:          0    XT-PIC      cascade
  4:         21    XT-PIC      ttyS0
  6:          2    XT-PIC      floppy
  8:          1    XT-PIC      rtc0
  9:          0    XT-PIC      acpi
 10:        164    XT-PIC      virtio10, virtio7, virtio8, virtio2, virtio3, nvme2q0, nvme2q1, virtio6
 11:       6759    XT-PIC      virtio5, virtio0, virtio9, virtio1, nvme0q0, nvme0q1, nvme1q0, nvme1q1, xhci-hcd:usb1, virtio4
 12:        125    XT-PIC      i8042
 14:          0    XT-PIC      ata_piix
 15:          0    XT-PIC      ata_piix
NMI:          0   Non-maskable interrupts
LOC:      34888   Local timer interrupts
SPU:          0   Spurious interrupts
PMI:          0   Performance monitoring interrupts
IWI:          0   IRQ work interrupts
RTR:          0   APIC ICR read retries
RES:          0   Rescheduling interrupts
CAL:          0   Function call interrupts
TLB:          0   TLB shootdowns
TRM:          0   Thermal event interrupts
THR:          0   Threshold APIC interrupts
DFR:          0   Deferred Error APIC interrupts
MCE:          0   Machine check exceptions
MCP:          1   Machine check polls
HYP:          1   Hypervisor callback interrupts
ERR:          0
MIS:          0
PIN:          0   Posted-interrupt notification event
NPI:          0   Nested posted-interrupt event
PIW:          0   Posted-interrupt wakeup event
➜  ~ cd /sys/kernel/debug/irq/irqs
➜  irqs ls
0  1  10  11  12  13  14  15  2  3  4  5  6  7  8  9
➜  irqs cat 4
handler:  handle_level_irq
device:   (null)
status:   0x00000500
            _IRQ_NOPROBE
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x00402200
            IRQD_LEVEL
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
node:     0
affinity: 0
effectiv:
pending:
domain:
 hwirq:   0x0
 chip:    XT-PIC
  flags:   0x0
```

## 在物理机上做测试

所以，pic 没有被淘汰，而是就是真的存在的。

### noapic

n100 上的测试，才发现，CPU 只有一个了:
```txt
🧀  cat /proc/interrupts
           CPU0
  2:          0    XT-PIC      cascade
  8:          0    XT-PIC      rtc0
  9:          0    XT-PIC      acpi
 11:          0    XT-PIC      i801_smbus
 14:          0    XT-PIC      INTC1057:00
 16:          0  PCI-MSI-0000:00:1c.0   0-edge      PCIe PME, aerdrv
 17:          0  PCI-MSI-0000:00:1d.0   0-edge      PCIe PME, aerdrv
 18:          0  PCI-MSI-0000:00:1d.3   0-edge      PCIe PME, aerdrv
 19:          0  PCI-MSI-0000:00:17.0   0-edge      ahci[0000:00:17.0]
 20:       6374  PCI-MSI-0000:00:14.0   0-edge      xhci_hcd
 21:         11  PCI-MSIX-0000:02:00.0   0-edge      nvme0q0
 22:       6772  PCI-MSIX-0000:02:00.0   1-edge      nvme0q1
 23:         24  PCI-MSI-0000:00:02.0   0-edge      i915
 24:         54  PCI-MSI-0000:00:16.0   0-edge      mei_me
 25:          1  PCI-MSIX-0000:01:00.0   0-edge      enp1s0
 26:        326  PCI-MSIX-0000:01:00.0   1-edge      enp1s0-rx-0
 27:        552  PCI-MSIX-0000:01:00.0   2-edge      enp1s0-tx-0
 28:          0  PCI-MSIX-0000:03:00.0   0-edge      enp3s0
 29:         54  PCI-MSIX-0000:03:00.0   1-edge      enp3s0-rx-0
 30:         54  PCI-MSIX-0000:03:00.0   2-edge      enp3s0-tx-0
 31:        504  PCI-MSI-0000:00:1f.3   0-edge      snd_hda_intel:card0
NMI:          4   Non-maskable interrupts
LOC:      23664   Local timer interrupts
SPU:          0   Spurious interrupts
PMI:          4   Performance monitoring interrupts
IWI:         17   IRQ work interrupts
RTR:          0   APIC ICR read retries
RES:          0   Rescheduling interrupts
CAL:          0   Function call interrupts
TLB:          0   TLB shootdowns
TRM:          0   Thermal event interrupts
THR:          0   Threshold APIC interrupts
DFR:          0   Deferred Error APIC interrupts
MCE:          0   Machine check exceptions
MCP:          1   Machine check polls
ERR:          0
MIS:          0
PIN:          0   Posted-interrupt notification event
NPI:          0   Nested posted-interrupt event
PIW:          0   Posted-interrupt wakeup event
```

```txt
  2:          0    XT-PIC      cascade
```
这个是虚拟机中没有看到的，有趣，而且对比 /proc/iomem 也是可以看到差别。

### nomsi

不错，是可以的，也是符合预期的

```txt
🧀  cat /proc/interrupts
            CPU0       CPU1       CPU2       CPU3
   8:          0          0          0          0  IR-IO-APIC    8-edge      rtc0
   9:          0          0          0          0  IR-IO-APIC    9-fasteoi   acpi
  14:          0          0          0          0  IR-IO-APIC   14-fasteoi   INTC1057:00
  16:          0       1115      10797          0  IR-IO-APIC   16-fasteoi   ahci[0000:00:17.0], xhci-hcd:usb1, nvme0q0, nvme0q1, i915, mei_me, i801_smbus
  18:       2462          0          0          0  IR-IO-APIC   18-fasteoi   enp1s0
  19:          0          0          0        534  IR-IO-APIC   19-fasteoi   snd_hda_intel:card0, enp3s0
  26:          0          0        283          0  IR-IO-APIC   26-fasteoi   intel_ish_ipc
 120:          0          0          0          0  DMAR-MSI    0-edge      dmar0
 121:          0          0          0          0  DMAR-MSI    1-edge      dmar1
 NMI:          1          1          1          1   Non-maskable interrupts
 LOC:       9048      12266       9755       8658   Local timer interrupts
 SPU:          0          0          0          0   Spurious interrupts
 PMI:          1          1          1          1   Performance monitoring interrupts
 IWI:          9          0          4          4   IRQ work interrupts
 RTR:          0          0          0          0   APIC ICR read retries
 RES:        939        951        857       1120   Rescheduling interrupts
 CAL:      19055       3969      17072      16580   Function call interrupts
 TLB:       1214        181        167        163   TLB shootdowns
 TRM:          0          0          0          0   Thermal event interrupts
 THR:          0          0          0          0   Threshold APIC interrupts
 DFR:          0          0          0          0   Deferred Error APIC interrupts
 MCE:          0          0          0          0   Machine check exceptions
 MCP:          1          2          2          2   Machine check polls
 ERR:          0
 MIS:          0
 PIN:          0          0          0          0   Posted-interrupt notification event
 NPI:          0          0          0          0   Nested posted-interrupt event
 PIW:          0          0          0          0   Posted-interrupt wakeup event
```
但是虚拟机

## Advanced topic

### how kernel switch from pic to apic
从 gsi 到 apic 的基本流程:

和 kvm 非常类似，在系统启动之后，抛弃使用 pic,
具体表现为 apic_accept_pic_intr 的这个判断失败
通过分析 QEMU 的源代码，可以知道
在 apic_mem_write 中，对于 lvt 被 masked 了

在  apic_mem_write 中添加如下的代码:
```c
X86CPU *cpu = X86_CPU(current_cpu);

if(index == 0x32 + APIC_LVT_LINT0)
  printf("->:%s %lx\n", __FUNCTION__, cpu->env.eip);
```

输出发现 guest 的地址为 : 0xffffffff810c14d0

使用 gdb 找到这一行:
```gdb
>>> info line *0xffffffff810c14d0
Line 33 of "./include/asm-generic/fixmap.h" starts at address 0xffffffff810c14d0 <native_apic_mem_write> and ends at 0xffffffff810c14d2 <native_apic_mem_write+2>.
```

```txt
#0  native_apic_mem_write (reg=848, v=1792) at ./include/asm-generic/fixmap.h:33
#1  0xffffffff810bc7eb in apic_write (val=1792, reg=848) at ./arch/x86/include/asm/apic.h:394
#2  setup_local_APIC () at arch/x86/kernel/apic/apic.c:1698
#3  0xffffffff82dbd0ad in apic_bsp_setup (upmode=<optimized out>) at arch/x86/kernel/apic/apic.c:2601
#4  apic_intr_mode_init () at arch/x86/kernel/apic/apic.c:1444
#5  0xffffffff82db1cd8 in x86_late_time_init () at arch/x86/kernel/time.c:100
#6  0xffffffff82daa109 in start_kernel () at init/main.c:1080
#7  0xffffffff81000107 in secondary_startup_64 () at arch/x86/kernel/head_64.S:283
```

进而可以找到 在 setup_local_APIC 中存在 Set up LVT0, LVT1 相关的代码, 这个会进行屏蔽

使用 tcg 的时候(否则是 kvm 模拟了)，在 QEMU 初始化会调用一次 apic_mem_write
在内核启动之前会调用一次, 之后 seabios 会调用数次
```txt
(qemu) ->:apic_mem_write addr=0 // qemu 初始化 hpet 的时候代码自动触发的
->:apic_mem_write addr=f0 // 都是 kernel 启动之前搞定的
->:apic_mem_write addr=350
->:apic_mem_write eip=ec676 // 暂时没有方法通过地址找 seabios 的源代码
->:apic_mem_write val=8700
->:apic_mem_write addr=360
->:apic_mem_write addr=300
->:apic_mem_write addr=300
```
因为 seabios 的代码很简单，其实可以很容易的 seabios 操作 apic 的位置在 smp_scan 中


### switch intc
我们知道最开始的时候，guest 会使用 pic ，之后切换为 apic 的，现在我试图找到在 linux kernel 的源码中定位具体发生修改的位置。

在系统启动之后，抛弃使用 pic 的具体表现为 apic_accept_pic_intr 的这个判断失败
也就是 APICCommonState::lvt[APIC_LVT_NB] 被 mask 掉了，而这个操作是在
apic_mem_write 中进行的。

在 apic_mem_write 中添加如下的代码:
```c
X86CPU *cpu = X86_CPU(current_cpu);

if(index == 0x32 + APIC_LVT_LINT0)
  printf("%lx\n", cpu->env.eip);
```

输出发现 guest 的地址为 : 0xffffffff810c14d0

使用 gdb 找到这一行:
```txt
>>> info line *0xffffffff810c14d0
Line 33 of "./include/asm-generic/fixmap.h" starts at address 0xffffffff810c14d0 <native_apic_mem_write> and ends at 0xffffffff810c14d2 <native_apic_mem_write+2>.
```

```txt
/*
#0  native_apic_mem_write (reg=848, v=1792) at ./include/asm-generic/fixmap.h:33
#1  0xffffffff810bc7eb in apic_write (val=1792, reg=848) at ./arch/x86/include/asm/apic.h:394
#2  setup_local_APIC () at arch/x86/kernel/apic/apic.c:1698
#3  0xffffffff82dbd0ad in apic_bsp_setup (upmode=<optimized out>) at arch/x86/kernel/apic/apic.c:2601
#4  apic_intr_mode_init () at arch/x86/kernel/apic/apic.c:1444
#5  0xffffffff82db1cd8 in x86_late_time_init () at arch/x86/kernel/time.c:100
#6  0xffffffff82daa109 in start_kernel () at init/main.c:1080
#7  0xffffffff81000107 in secondary_startup_64 () at arch/x86/kernel/head_64.S:283
```

进而可以找到 在 **setup_local_APIC** 中存在 Set up LVT0, LVT1 相关的代码了。

### legacy PCI interrupt
legacy PCI interrupt 指的是 PCI 提供四根 interrupt line 链接到 intc 上，所有的 pci 设备需要共享这些 interrupt line

在 guest 中运行 lspci 来查看所有的 PCI 设备:

```txt
00:00.0 Host bridge: Intel Corporation 440FX - 82441FX PMC [Natoma] (rev 02)
00:01.0 ISA bridge: Intel Corporation 82371SB PIIX3 ISA [Natoma/Triton II]
00:01.1 IDE interface: Intel Corporation 82371SB PIIX3 IDE [Natoma/Triton II]
00:01.3 Bridge: Intel Corporation 82371AB/EB/MB PIIX4 ACPI (rev 03)
00:02.0 VGA compatible controller: Red Hat, Inc. Virtio GPU (rev 01)
00:03.0 Ethernet controller: Intel Corporation 82540EM Gigabit Ethernet Controller (rev 03)
00:04.0 Non-Volatile memory controller: Red Hat, Inc. Device 0010 (rev 02)
00:05.0 Non-Volatile memory controller: Red Hat, Inc. Device 0010 (rev 02)
00:06.0 Unclassified device [0002]: Red Hat, Inc. Virtio filesystem
```

虽然 piix3 是一个 ISA bridge, 但是实际上在 QEMU 中 piix3 和 ISA 没有啥关系，
piix3 负责控制 pci interrupt line 具体路由到那个中断上, 在 seabios 配置 piix3 的配置空间就可以了:

```c
/* PIIX3/PIIX4 PCI to ISA bridge */
static void piix_isa_bridge_setup(struct pci_device *pci, void *arg)
{
    int i, irq;
    u8 elcr[2];

    elcr[0] = 0x00;
    elcr[1] = 0x00;
    for (i = 0; i < 4; i++) {
        irq = pci_irqs[i];
        /* set to trigger level */
        elcr[irq >> 3] |= (1 << (irq & 7));
        /* activate irq remapping in PIIX */
        pci_config_writeb(pci->bdf, 0x60 + i, irq);
    }
    outb(elcr[0], PIIX_PORT_ELCR1);
    outb(elcr[1], PIIX_PORT_ELCR2);
    dprintf(1, "PIIX3/PIIX4 init: elcr=%02x %02x\n", elcr[0], elcr[1]);
}
```
其中的 pci_config_writeb 最后会调用到 QEMU 中的  piix3_write_config

- nvme_irq_assert
  - nvme_irq_check
    - pci_irq_assert
      - pci_set_irq
        - int intx = pci_intx(pci_dev) : 从 PCI 配置空间读去这个设备的 PCI_INTERRUPT_PIN, PCI_INTERRUPT_PIN 表示 PCI 设备接入到 PCI interrupt line 的输入端
        - pci_irq_handler
          - pci_change_irq_level
            - PCIBus::map_irq : 也即是 pci_slot_get_pirq 根据 dev 的位置计算出来在 PCI interrupt line 的输出位置。
            - pci_bus_change_irq_level : 调用 PCIBus::map_irq，这个 hook 是在 piix3_create 中初始化为 piix3_set_irq
              - piix3_set_irq
                - piix3_set_irq_level
                  - pic_irq = piix3->dev.config[PIIX_PIRQCA + pirq] : 读去配置从而知道中断是路由到哪里去的

但是，一个设备是如何知道自己发出的中断是发送到哪一个 pci interrupt line 上的，这就靠 PCIBus::map_irq 了
具体而言，这个 hook 注册的函数是:

```c
/*
 * Return the global irq number corresponding to a given device irq
 * pin. We could also use the bus number to have a more precise mapping.
 */
static int pci_slot_get_pirq(PCIDevice *pci_dev, int pci_intx)
{
    int slot_addend;
    slot_addend = PCI_SLOT(pci_dev->devfn) - 1;
    return (pci_intx + slot_addend) & 3;
}
```
## 似乎可以从这里入手看看的

init_ISA_irqs

## 从这个注释中再次说明什么是 PIC 和 APIC
```c
/*
 * Read pending interrupt vector and intack.
 */
int kvm_cpu_get_interrupt(struct kvm_vcpu *v)
{
	int vector = kvm_cpu_get_extint(v);
	if (vector != -1)
		return vector;			/* PIC */

	return kvm_get_apic_interrupt(v);	/* APIC */
}
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
