# pci
https://wiki.osdev.org/PCI_Express : 通过 ACPI 的配置，可以让 PCI 配置空间的访问使用 mmio 的方式

- [ ] https://lwn.net/Articles/367630/ : how to write acpi driver

- [ ] 那些 probe 工作是怎么进行的

## Ntoes
- [ ] acpi_scan_add_handler
- [ ] acpi_get_table 可以直接获取 acpi table 出来，所以，这些 table 是什么时候构建的 ?

## really probe

#### acpi_init
1. acpi_init 位于 `subsys_initcall` 中间:

- [ ] 似乎再次之前，内存分配已经搞定了


```c
/*

[    1.846074] Call Trace:
[    1.846076] [<900000000020864c>] show_stack+0x2c/0x100
[    1.846079] [<9000000000ec3948>] dump_stack+0x90/0xc0
[    1.846081] [<900000000029f8e0>] irq_domain_set_mapping+0x90/0xb8
[    1.846084] [<90000000002a0698>] __irq_domain_alloc_irqs+0x200/0x2e0
[    1.846085] [<90000000002a2d08>] msi_domain_alloc_irqs+0x90/0x320
[    1.846088] [<900000000020306c>] arch_setup_msi_irqs+0x8c/0xc8
[    1.846090] [<900000000085cc64>] __pci_enable_msi_range+0x324/0x5e8
[    1.846092] [<900000000085d078>] pci_alloc_irq_vectors_affinity+0x120/0x158
[    1.846094] [<9000000000a926b8>] ahci_init_one+0xb28/0x1020
[    1.846097] [<900000000083c910>] local_pci_probe+0x48/0xe0
[    1.846099] [<900000000024cff4>] work_for_cpu_fn+0x1c/0x30
[    1.846100] [<9000000000250778>] process_one_work+0x210/0x418
[    1.846102] [<9000000000250cb8>] worker_thread+0x338/0x5e0
[    1.846104] [<90000000002578fc>] kthread+0x124/0x128
[    1.846106] [<9000000000203cc8>] ret_from_kernel_thread+0xc/0x10
[    1.846108] irq: irq_domain_set_mapping 64 38


[    0.888500] [<900000000020864c>] show_stack+0x2c/0x100
[    0.888502] [<9000000000ec3968>] dump_stack+0x90/0xc0
[    0.888505] [<90000000009a601c>] really_probe+0x20c/0x2b8
[    0.888507] [<90000000009a6274>] driver_probe_device+0x64/0x100
[    0.888509] [<90000000009a3dd8>] bus_for_each_drv+0x68/0xa8
[    0.888511] [<90000000009a5d8c>] __device_attach+0x124/0x1a0
[    0.888513] [<90000000009a500c>] bus_probe_device+0x9c/0xc0
[    0.888514] [<90000000009a1470>] device_add+0x350/0x608
[    0.888517] [<90000000009a7f80>] platform_device_add+0x128/0x288
[    0.888519] [<90000000009a8d90>] platform_device_register_full+0xb8/0x130
[    0.888521] [<90000000008a255c>] acpi_create_platform_device+0x28c/0x300
[    0.888523] [<9000000000898328>] acpi_default_enumeration+0x28/0x58
[    0.888524] [<9000000000898558>] acpi_bus_attach+0x1d0/0x210
[    0.888526] [<90000000008983e0>] acpi_bus_attach+0x58/0x210
[    0.888528] [<90000000008983e0>] acpi_bus_attach+0x58/0x210
[    0.888529] [<900000000089a52c>] acpi_bus_scan+0x34/0x78
[    0.888532] [<90000000012f8060>] acpi_scan_init+0x170/0x2b8
[    0.888534] [<90000000012f7c68>] acpi_init+0x2e4/0x338
[    0.888535] [<90000000002004fc>] do_one_initcall+0x6c/0x170
[    0.888537] [<90000000012d4ce0>] kernel_init_freeable+0x1f8/0x2b8
[    0.888540] [<9000000000eda774>] kernel_init+0x10/0xf4
```
