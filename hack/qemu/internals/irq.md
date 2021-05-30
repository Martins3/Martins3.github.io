# irq

- [ ] 再次反思一下，为什么 x86 kvm 需要奇怪的中断路由机制?
  - 以及为此制作出来了 gsi 的概念
  - [ ] 原则上，CPU 只能知道哪一个中断到来了才对, vmcs 记录一下，这就是跳转的位置了
  - [ ] 为什么会出现 kvm 不知道 os 使用的是哪一个中断控制器?

这个书的太痛苦了，还是使用一下其他的制作方法吧!

coreboot 三部曲 : https://habr.com/en/post/501912/ : 永远的神

## [ ]  qemu 参数 kernel-kernel 的影响是什么 ?


## qemu_set_irq 到底做了什么?
gsi 似乎是总体的入口:

1. 创建
```c
GSIState *pc_gsi_create(qemu_irq **irqs, bool pci_enabled)
{
    GSIState *s;

    s = g_new0(GSIState, 1);
    if (kvm_ioapic_in_kernel()) {
        kvm_pc_setup_irq_routing(pci_enabled);
    }
    *irqs = qemu_allocate_irqs(gsi_handler, s, GSI_NUM_PINS);

    return s;
}
```

2. 使用
```c
void qemu_set_irq(qemu_irq irq, int level)
{
    if (!irq)
        return;

    // 这个 handler 总是 gsi_handler
    irq->handler(irq->opaque, irq->n, level);
}
```

3. 分发
```c
void gsi_handler(void *opaque, int n, int level)
{
    GSIState *s = opaque;

    trace_x86_gsi_interrupt(n, level);
    switch (n) {
    case 0 ... ISA_NUM_IRQS - 1:
        if (s->i8259_irq[n]) {
            /* Under KVM, Kernel will forward to both PIC and IOAPIC */
            qemu_set_irq(s->i8259_irq[n], level);
        }
        /* fall through */
    case ISA_NUM_IRQS ... IOAPIC_NUM_PINS - 1:
        qemu_set_irq(s->ioapic_irq[n], level);
        break;
    case IO_APIC_SECONDARY_IRQBASE
        ... IO_APIC_SECONDARY_IRQBASE + IOAPIC_NUM_PINS - 1:
        qemu_set_irq(s->ioapic2_irq[n - IO_APIC_SECONDARY_IRQBASE], level);
        break;
    }
}
```

```c
typedef struct GSIState {
    qemu_irq i8259_irq[ISA_NUM_IRQS];
    qemu_irq ioapic_irq[IOAPIC_NUM_PINS];
    qemu_irq ioapic2_irq[IOAPIC_NUM_PINS];
} GSIState;
```

- pc_i8259_create
  - kvm_i8259_init
    - i8259_init_chip : 创建主从两个设备，映射地址空间, 在 `info mtree` 中可以看到 `kvm-pic` 和 `kvm-elcr`
      - `qemu_allocate_irqs(kvm_pic_set_irq, NULL, ISA_NUM_IRQS);`
        - 因此注册的 handler 就是 kvm_pic_set_irq 了, 其会调用 kvm 的接口
- ioapic_init_gsi : 这个地方是唯一对于 GSIState 进行赋值的位置了
  - 中间利用 gpio 的框架，最后注册的 handler 就是 kvm_ioapic_set_irq 了

## [x] 键盘的中断是如何注入的
参考 ./hack/qemu/internals/i8042.md

## [x] qemu 如何知道 e1000 的中断号
首先，可以清楚的知道 e1000 使用的就是 10 号作为 IRQ
```c
/*
─── Variables ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
arg opaque = 0x555556bc5080, irq = 10, level = 1
loc s = 0x0: Cannot access memory at address 0x0, common = 0x1ffffcdee: Cannot access memory at address 0x1ffffcdee, delivered = 0
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
>>> bt
#0  kvm_ioapic_set_irq (opaque=0x555556bc5080, irq=10, level=1) at ../hw/i386/kvm/ioapic.c:114
#1  0x0000555555e8c04a in qemu_set_irq (irq=0x555556c176e0, level=1) at ../hw/core/irq.c:46
#2  0x0000555555b7ca51 in gsi_handler (opaque=0x555556b87600, n=10, level=1) at ../hw/i386/x86.c:600
#3  0x0000555555e8c04a in qemu_set_irq (irq=0x555556b8e1c0, level=1) at ../hw/core/irq.c:46
#4  0x0000555555999b69 in piix3_set_irq_pic (piix3=0x555556c6f720, pic_irq=10) at ../hw/isa/piix3.c:44
#5  0x0000555555999c72 in piix3_set_irq_level (piix3=0x555556c6f720, pirq=2, level=1) at ../hw/isa/piix3.c:76
#6  0x0000555555999ca9 in piix3_set_irq (opaque=0x555556c6f720, pirq=2, level=1) at ../hw/isa/piix3.c:82
#7  0x000055555585f50c in pci_bus_change_irq_level (bus=0x5555570c7800, irq_num=2, change=1) at ../hw/pci/pci.c:266 // 这里还是北桥控制的
#8  0x000055555585f587 in pci_change_irq_level (pci_dev=0x555557c7d680, irq_num=2, change=1) at ../hw/pci/pci.c:279
#9  0x0000555555862725 in pci_irq_handler (opaque=0x555557c7d680, irq_num=0, level=1) at ../hw/pci/pci.c:1463
#10 0x00005555558627f0 in pci_set_irq (pci_dev=0x555557c7d680, level=1) at ../hw/pci/pci.c:1482
#11 0x0000555555a0ff93 in set_interrupt_cause (s=0x555557c7d680, index=0, val=16) at ../hw/net/e1000.c:333
#12 0x0000555555a10023 in set_ics (s=0x555557c7d680, index=50, val=16) at ../hw/net/e1000.c:351
#13 0x0000555555a126aa in e1000_mmio_write (opaque=0x555557c7d680, addr=200, val=16, size=4) at ../hw/net/e1000.c:1309
#14 0x0000555555c93868 in memory_region_write_accessor (mr=0x555557c7ffa0, addr=200, value=0x7fffd9ff9028, size=4, shift=0, mask=4294967295, attrs=...) at ../softmmu/me
mory.c:491
```
下面分析 IRQ = 10 是怎么得到的?

- [ ] 无法理解 : 为什么 e1000 的中断需要 piix3 参与?
  - piix3 作为一个南桥芯片
  - 从 `info qtree` 中间，e1000 和 piix3 是平级的才对啊

```c
/*
>>> bt
#0  pci_bus_irqs (bus=0x555556e19800, set_irq=0x555556b533c0, map_irq=0x555555999b02 <PIIX3_PCI_DEVICE+50>, irq_opaque=0x7fffffffd360, nirq=21845) at ../hw/pci/pci.c:48
#1  0x000055555599a4d2 in piix3_create (pci_bus=0x5555570d1e00, isa_bus=0x7fffffffd3d0) at ../hw/isa/piix3.c:389
#2  0x0000555555b56298 in pc_init1 (machine=0x5555569ba000, host_type=0x55555609066a "i440FX-pcihost", pci_type=0x555556090663 "i440FX") at ../hw/i386/pc_piix.c:211
#3  0x0000555555b56bea in pc_init_v6_0 (machine=0x5555569ba000) at ../hw/i386/pc_piix.c:427
#4  0x0000555555b3840f in machine_run_board_init (machine=0x5555569ba000) at ../hw/core/machine.c:1232
#5  0x0000555555d2fd9a in qemu_init_board () at ../softmmu/vl.c:2514
#6  0x0000555555d2ff79 in qmp_x_exit_preconfig (errp=0x5555567a0d50 <error_fatal>) at ../softmmu/vl.c:2588
#7  0x0000555555d326c9 in qemu_init (argc=28, argv=0x7fffffffd7d8, envp=0x7fffffffd8c0) at ../softmmu/vl.c:3611
#8  0x000055555582f3a5 in main (argc=28, argv=0x7fffffffd7d8, envp=0x7fffffffd8c0) at ../softmmu/main.c:49
```

将 pci_bus_irqs 的参数 bus 还是 i440FX 也就是北桥。

- [ ] irq_opaque 是做啥的
```c
void pci_bus_irqs(PCIBus *bus, pci_set_irq_fn set_irq, pci_map_irq_fn map_irq,
                  void *irq_opaque, int nirq)
{
    bus->set_irq = set_irq;
    bus->map_irq = map_irq;
    bus->irq_opaque = irq_opaque;
    bus->nirq = nirq;
    bus->irq_count = g_malloc0(nirq * sizeof(bus->irq_count[0]));
}
```


## [ ] 没有看到 msi 的模拟啊 ?

- [ ] 从 pci_default_write_config 的位置分析
