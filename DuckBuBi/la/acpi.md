# acpi
感觉 ramooflax 的处理是相当的简单，其实他甚至可以在 acpi 解析之前就可以来初始化 e1000 

ramooflax 不需要模拟 acpi 设备空间，因为设备本身就是相同的

- [ ] 是不是， acpi 探测出来了 pcie，那么剩下的事情应该 pcie 自己做
- [ ] 探测，到底指的是什么
```c
/*
[    0.000000] efi:  ACPI 2.0=0xfd2f4000  SMBIOS=0x900000000fffe000  SMBIOS 3.0=0x90000000fd132000 
[    0.000000] ACPI: Early table checksum verification disabled
[    0.000000] ACPI: RSDP 0x00000000FD2F4000 000024 (v02 LOONGS)
[    0.000000] ACPI: XSDT 0x00000000FD2F3000 000044 (v01 LOONGS TP-R00   00000004      01000013)
[    0.000000] ACPI: FACP 0x00000000FD2F0000 0000F4 (v03 LOONGS TP-R00   00000004 PTEC 20160527)
[    0.000000] ACPI: DSDT 0x00000000FD2EE000 001A9C (v02 LGSON  TP-R00   00000476 INTL 20160527)
[    0.000000] ACPI: FACS 0x00000000FD2F1000 000040
[    0.000000] ACPI: APIC 0x00000000FD2F2000 00005E (v01 LOONGS LOONGSON 00000001 LIUX 00000000)
[    0.000000] ACPI: SRAT 0x00000000FD2ED000 0000C0 (v02 LOONGS LOONGSON 00000002 LIUX 01000013)
[    0.000000] ACPI: SLIT 0x00000000FD2EC000 00002D (v01 LOONGS LOONGSON 00000002 LIUX 01000013)
```

```c
/*
[    0.888502] [<900000000020864c>] show_stack+0x2c/0x100
[    0.888504] [<9000000000ec3988>] dump_stack+0x90/0xc0
[    0.888507] [<90000000009a605c>] really_probe+0x20c/0x2b8
[    0.888509] [<90000000009a62b4>] driver_probe_device+0x64/0x100
[    0.888511] [<90000000009a3e18>] bus_for_each_drv+0x68/0xa8
[    0.888513] [<90000000009a5dcc>] __device_attach+0x124/0x1a0
[    0.888515] [<90000000009a504c>] bus_probe_device+0x9c/0xc0
[    0.888517] [<90000000009a14b0>] device_add+0x350/0x608
[    0.888519] [<90000000009a7fc0>] platform_device_add+0x128/0x288
[    0.888521] [<90000000009a8dd0>] platform_device_register_full+0xb8/0x130
[    0.888523] [<90000000008a259c>] acpi_create_platform_device+0x28c/0x300
[    0.888525] [<9000000000898368>] acpi_default_enumeration+0x28/0x58
[    0.888526] [<9000000000898598>] acpi_bus_attach+0x1d0/0x210
[    0.888528] [<9000000000898420>] acpi_bus_attach+0x58/0x210
[    0.888530] [<9000000000898420>] acpi_bus_attach+0x58/0x210
[    0.888532] [<900000000089a56c>] acpi_bus_scan+0x34/0x78
[    0.888534] [<90000000012f8060>] acpi_scan_init+0x170/0x2b8
[    0.888536] [<90000000012f7c68>] acpi_init+0x2e4/0x338
[    0.888537] [<90000000002004fc>] do_one_initcall+0x6c/0x170
[    0.888540] [<90000000012d4ce0>] kernel_init_freeable+0x1f8/0x2b8
[    0.888542] [<9000000000eda794>] kernel_init+0x10/0xf4
[    0.888544] [<9000000000203cc8>] ret_from_kernel_thread+0xc/0x10
```
- [ ] 那么 e1000 需要加入到 acpi 的空间中吗 ?


- acpi_init
  - [ ] init_acpi_device_notify
  - acpi_bus_init
    - acpi_bus_init_irq
    - bus_register(&acpi_bus_type);
  - [ ] pci_mmcfg_late_init : 龙芯自定义的一些的东西
  - acpi_iort_init
  - acpi_scan_init
    - acpi_bus_scan : 进一步调用 acpi_bus_check_add 来探测设备，将探测到的设备初始化为 acpi_device
      - device_attach

  - [ ] acpi_bus_init
  - [ ] acpi_ec_init
