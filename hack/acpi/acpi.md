# acpi 

## [核心文档](https://acpica.org/sites/acpica/files/ACPI-Introduction.pdf)
Fundamentally, ACPI defines two types of data structures which are shared between the
system firmware and the OS: **data tables** and **definition blocks**. These data structures are the
primary communication mechanism between the firmware and the OS. Data tables store raw
data and are consumed by device drivers. Definition blocks consist of byte code that is
executable by an interpreter.
> data tables 是提供的表单数据，definition blocks 是

This definition block byte code is compiled from the ACPI Source Language (ASL) code. ASL
is the language used to define ACPI objects and to write control methods.
An ASL compiler translates ASL into ACPI Machine Language (AML) byte code. AML is the language
processed by the ACPI AML interpreter.

The system bus is the root of enumeration for these ACPI devices. 

Devices that are enumerable on other
buses, like PCI or USB devices, are usually not enumerated in the namespace. *Instead, their
own buses enumerate the devices and load their drivers.* However, all enumerable buses
have an encoding technique that allows ACPI to encode the bus­specific addresses of the
devices so they can be found in ACPI, even though ACPI usually does not load drivers for
these devices.

As an example of this, PCI does not support native hotplug. However, PCI can use ACPI to
evaluate objects and define methods that allow ACPI to fill in the functions necessary to
perform hotplug on PCI.

After the system is up and running, ACPI works with the OS to handle any ACPI interrupt
events that occur via the *ACPI system control interrupt (SCI) handler*. This interrupt invokes
ACPI events in one of two general ways: fixed events and general purpose events (GPEs).

> 这个也是介绍的相当的清楚了: https://wiki.osdev.org/ACPI
> 那么 AML (definition blocks) 和 namespace 是什么关系?

Upon initialization, the AML interpreter extracts the byte code in the definition blocks as enumerable objects, 

This collection of enumerable forms the OS construct called the ACPI namespace.

Objects can either have a directly defined value or must be evaluated and interpreted by the AML interpreter.

The system bus is the root of enumeration for these ACPI devices.

## [ACPI considerations for PCI host bridges](https://www.kernel.org/doc/html/latest/PCI/acpi-info.html)
For example, there’s no standard hardware mechanism for enumerating PCI host bridges, so the ACPI namespace must describe each host bridge, the method for accessing PCI config space below it, the address space windows the host bridge forwards to PCI (using _CRS), and the routing of legacy INTx interrupts (using _PRT).

However, ACPI may describe PCI devices if it provides power management or hotplug functionality for them or if the device has INTx interrupts connected by platform interrupt controllers and a `_PRT` is needed to describe those connections.

If the OS is expected to manage a non-discoverable device described via ACPI, that device will have a specific `_HID`/`_CID` that tells the OS what driver to bind to it, and the `_CRS` tells the OS and the driver where the device’s registers are.

## Official Documents

### [ ](https://uefi.org/specs/ACPI/6.4/06_Device_Configuration/Device_Configuration.html)



https://lwn.net/Articles/367630/

- [MADT](https://wiki.osdev.org/MADT)
- [HPET](https://wiki.osdev.org/HPET) : 
- [RSDT](https://wiki.osdev.org/RSDT) : Root System Description Table
- [RSDP](https://wiki.osdev.org/RSDP) : Root System Description Pointer
- [XSDT](https://wiki.osdev.org/XSDT) : eXtended System Descriptor Table (XSDT) - the 64-bit version of the ACPI RSDT
- [DMAR](https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/08/10/iommu-driver-analysis) : DMA Remapping Reporting
- [FADT](https://wiki.osdev.org/FADT) : fixed ACPI description table, This table contains information about fixed register blocks pertaining to power management.
- [SSDT](https://wiki.osdev.org/SSDT) : Secondary System Descriptor Table
- [DSDT](https://wiki.osdev.org/DSDT) : DSDT stands for Differentiated System Description Table. It Is a major ACPI table and is used to describe what peripherals the machine has. 

- [GPE](https://askubuntu.com/questions/148726/what-is-an-acpi-gpe-storm)



- https://github.com/rust-osdev/about : 这个组织提供一堆可以用于 os dev 的工具，包括 uefi bootloader acpi
- https://github.com/acpica/acpica : acpi 框架的源代码 

## TODO

- [ ] acpi 是如何提供给 os 的
- [ ] acpi 表是如何构建起来的

- [ ] /home/maritns3/core/kvmqemu/hw/i386/pc.c
- [ ] /home/maritns3/core/kvmqemu/hw/acpi/cpu.c


- [ ] 让人奇怪的地方在于，acpi 的只是解析出来了 IOAPIC 和 HPET 来, pcie 是怎么被探索出来的，
  - [ ] pcie 在 acpi 中对应的 table 是什么 ？

- [ ] 至少，需要截获所有的 acpi 的访问，才可以正确模拟
  - [ ] acpi 表是从什么地方读去的, 固定的地址，还是进一步使用其他的机制通知的

- [ ] 其实，还需要创建一个虚假的 acpi 给 x86

- [ ] 除了 acpi 之外，还有什么是 firmware 传递给操作系统的?
  - [ ] efi 吗?

- [ ] mmio 空间分配是怎么实现的？

- [ ] 也许阅读一下

- [什么是 AML](https://stackoverflow.com/questions/43088172/why-do-we-need-aml-acpi-machine-language)

- [ ] apci 实际上还提供了关机选项啊

acpi 的解析[^2]

原来kernel中最终是通过acpi_evaluate_object 来调用bios中在asl中定义好的函数啊 [^1]

uefi 提供了 ACPI 的文档

acpi 是内核上的设计


在 acpi_pci_root_add 中，调用 pci_acpi_scan_root
```
#0  acpi_pci_root_add (device=0xffff888100217000, not_used=0xffffffff82063c60 <root_device_ids>) at drivers/acpi/pci_root.c:517
#1  0xffffffff8146a1c3 in acpi_scan_attach_handler (device=0xffff888100217000) at drivers/acpi/scan.c:2038
#2  acpi_bus_attach (device=device@entry=0xffff888100217000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2086
#3  0xffffffff8146a110 in acpi_bus_attach (device=device@entry=0xffff888100216800, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#4  0xffffffff8146a110 in acpi_bus_attach (device=0xffff888100216000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#5  0xffffffff8146bf1f in acpi_bus_scan (handle=handle@entry=0xffffffffffffffff) at drivers/acpi/scan.c:2167
#6  0xffffffff82b699a2 in acpi_scan_init () at drivers/acpi/scan.c:2342
#7  0xffffffff82b696d3 in acpi_init () at drivers/acpi/bus.c:1341
#8  0xffffffff81000def in do_one_initcall (fn=0xffffffff82b692e1 <acpi_init>) at init/main.c:1249
#9  0xffffffff82b3726b in do_initcall_level (command_line=0xffff888100122400 "root", level=4) at ./include/linux/compiler.h:234
#10 do_initcalls () at init/main.c:1338
#11 do_basic_setup () at init/main.c:1358
#12 kernel_init_freeable () at init/main.c:1560
#13 0xffffffff81b6f3b9 in kernel_init (unused=<optimized out>) at init/main.c:1447
#14 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#15 0x0000000000000000 in ?? ()
```

pcibios_add_bus 就是 acpi_pci_add_bus，最后调用到 bios 的处理函数，利用 bios 的功能实现 pci host bridge 的查找:
```
#0  acpi_evaluate_object (handle=handle@entry=0xffff8881000efde0, pathname=pathname@entry=0xffffffff822a918e "_DSM", external_params=external_params@entry=0xffffc900000 13a80, return_buffer=return_buffer@entry=0xffffc90000013a70) at drivers/acpi/acpica/nsxfeval.c:167
#1  0xffffffff81464717 in acpi_evaluate_dsm (handle=handle@entry=0xffff8881000efde0, guid=guid@entry=0xffffffff8205f340 <pci_acpi_dsm_guid>, rev=rev@entry=3, func=func@ entry=8, argv4=argv4@entry=0x0 <fixed_percpu_data>) at drivers/acpi/utils.c:664
#2  0xffffffff81448751 in acpi_pci_add_bus (bus=0xffff888100259c00) at ./include/linux/acpi.h:40
#3  0xffffffff81430ee2 in pci_register_host_bridge (bridge=bridge@entry=0xffff888100259800) at drivers/pci/probe.c:944
#4  0xffffffff81431014 in pci_create_root_bus (parent=parent@entry=0x0 <fixed_percpu_data>, bus=bus@entry=0, ops=0xffffffff8263f640 <pci_root_ops>, sysdata=sysdata@entr y=0xffff888100254338, resources=resources@entry=0xffff888100254318) at drivers/pci/probe.c:2979
#5  0xffffffff8147151e in acpi_pci_root_create (root=root@entry=0xffff88810021fa00, ops=ops@entry=0xffffffff8263f580 <acpi_pci_root_ops>, info=info@entry=0xffff88810025 4300, sysdata=sysdata@entry=0xffff888100254338) at drivers/acpi/pci_root.c:895
#6  0xffffffff81b179cd in pci_acpi_scan_root (root=root@entry=0xffff88810021fa00) at arch/x86/pci/acpi.c:368
#7  0xffffffff81b3542c in acpi_pci_root_add (device=0xffff888100217000, not_used=<optimized out>) at drivers/acpi/pci_root.c:597
#8  0xffffffff8146a1c3 in acpi_scan_attach_handler (device=0xffff888100217000) at drivers/acpi/scan.c:2038 #9  acpi_bus_attach (device=device@entry=0xffff888100217000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2086
#10 0xffffffff8146a110 in acpi_bus_attach (device=device@entry=0xffff888100216800, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#11 0xffffffff8146a110 in acpi_bus_attach (device=0xffff888100216000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#12 0xffffffff8146bf1f in acpi_bus_scan (handle=handle@entry=0xffffffffffffffff) at drivers/acpi/scan.c:2167
#13 0xffffffff82b699a2 in acpi_scan_init () at drivers/acpi/scan.c:2342
#14 0xffffffff82b696d3 in acpi_init () at drivers/acpi/bus.c:1341
#15 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b692e1 <acpi_init>) at init/main.c:1249
#16 0xffffffff82b3726b in do_initcall_level (command_line=0xffff888100122400 "root", level=4) at ./include/linux/compiler.h:234
#17 do_initcalls () at init/main.c:1338
#18 do_basic_setup () at init/main.c:1358
#19 kernel_init_freeable () at init/main.c:1560
#20 0xffffffff81b6f3b9 in kernel_init (unused=<optimized out>) at init/main.c:1447
#21 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#22 0x0000000000000000 in ?? ()
```


## really_probe
来看看第一个设备，acpi_button_driver_init 其最后调用 really_probe

- [ ] 似乎这些设备的 probe 可能是 acpi 之类的推动起来的，也可能是 driver 初始化的时候推动起来的

```c
/*
#0  acpi_button_add (device=0xffff8881002d5000) at drivers/acpi/button.c:474
#1  0xffffffff8146861c in acpi_device_probe (dev=0xffff8881002d5288) at drivers/acpi/bus.c:1003
#2  0xffffffff8167fd76 in really_probe (dev=dev@entry=0xffff8881002d5288, drv=drv@entry=0xffffffff825b37e8 <acpi_button_driver+200>) at drivers/base/dd.c:576
#3  0xffffffff8167ffb1 in driver_probe_device (drv=drv@entry=0xffffffff825b37e8 <acpi_button_driver+200>, dev=dev@entry=0xffff8881002d5288) at drivers/base/dd.c:763
#4  0xffffffff8168027e in device_driver_attach (drv=drv@entry=0xffffffff825b37e8 <acpi_button_driver+200>, dev=dev@entry=0xffff8881002d5288) at drivers/base/dd.c:1039
#5  0xffffffff816802e0 in __driver_attach (dev=0xffff8881002d5288, data=0xffffffff825b37e8 <acpi_button_driver+200>) at drivers/base/dd.c:1117
#6  0xffffffff8167e103 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff825b37e8 <acpi_button_driver+200>,
 fn=fn@entry=0xffffffff81680290 <__driver_attach>) at drivers/base/bus.c:305
#7  0xffffffff8167f765 in driver_attach (drv=drv@entry=0xffffffff825b37e8 <acpi_button_driver+200>) at drivers/base/dd.c:1133
#8  0xffffffff81b4f09a in bus_add_driver (drv=drv@entry=0xffffffff825b37e8 <acpi_button_driver+200>) at drivers/base/bus.c:622
#9  0xffffffff81680b77 in driver_register (drv=0xffffffff825b37e8 <acpi_button_driver+200>) at drivers/base/driver.c:171
#10 0xffffffff81000e1d in do_one_initcall (fn=0xffffffff82b70f39 <acpi_button_driver_init>) at init/main.c:1250
#11 0xffffffff82b3d26b in do_initcall_level (command_line=0xffff888100123400 "root", level=6) at ./include/linux/compiler.h:234
#12 do_initcalls () at init/main.c:1339
#13 do_basic_setup () at init/main.c:1359
#14 kernel_init_freeable () at init/main.c:1561
#15 0xffffffff81b7c549 in kernel_init (unused=<optimized out>) at init/main.c:1448
#16 0xffffffff810018d2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#17 0x0000000000000000 in ?? ()
```
所有的 driver 在注册的时候，都会试图和对应的 device match 一下，如果在该总线下，找不到这个 device，
那么就只好让这个总线 probe (acpi_device_probe，总线 probe 进一步调用 driver 提供的具体的 probe 函数(acpi_button_add

- [ ] 从 acpi_button_add 完全看不出来是怎么使用的


- [ ] 思考一下 e1000 是如何探测的?
  - 最开始的时候，e1000 设备就被探测了, 那么是如何匹配上的?

```c
/*
[    1.278648] e1000 0000:00:03.0 eth0: (PCI:33MHz:32-bit) 52:54:00:12:34:56
[    1.279551] e1000 0000:00:03.0 eth0: Intel(R) PRO/1000 Network Connection
[    1.280417] huxueshi:pci_device_probe e1000
[    1.280965] CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.12.0+ #19
[    1.281410] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-0-g155821a1990b-prebuilt.qemu.org 04/01/2014
[    1.281410] Call Trace:
[    1.281410]  dump_stack+0x64/0x7c
[    1.281410]  pci_device_probe+0x115/0x160
[    1.281410]  really_probe+0xd6/0x2c0
[    1.281410]  driver_probe_device+0x51/0xb0
[    1.281410]  device_driver_attach+0x4e/0x60
[    1.281410]  __driver_attach+0x50/0xc0
[    1.281410]  ? device_driver_attach+0x60/0x60
[    1.281410]  bus_for_each_dev+0x73/0xb0
[    1.281410]  bus_add_driver.cold+0xe3/0x1a5
[    1.281410]  driver_register+0x67/0xb0
[    1.281410]  ? e100_init_module+0x4c/0x4c
[    1.281410]  e1000_init_module+0x3d/0x72
[    1.281410]  do_one_initcall+0x3f/0x1b0
[    1.281410]  kernel_init_freeable+0x19e/0x1e6
[    1.281410]  ? rest_init+0xa4/0xa4
[    1.281410]  kernel_init+0x5/0xfc
[    1.281410]  ret_from_fork+0x22/0x30
[    1.292657] e1000e: Intel(R) PRO/1000 Network Driver
[    1.293310] e1000e: Copyright(c) 1999 - 2015 Intel Corporation.

[    0.893101] huxueshi:pci_device_probe ata_piix
[    0.893830] CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.12.0+ #19
[    0.894625] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-0-g155821a1990b-prebuilt.qemu.org 04/01/2014
[    0.894826] Call Trace:
[    0.894826]  dump_stack+0x64/0x7c
[    0.894826]  pci_device_probe+0x115/0x160
[    0.894826]  really_probe+0xd6/0x2c0
[    0.894826]  driver_probe_device+0x51/0xb0
[    0.894826]  device_driver_attach+0x4e/0x60
[    0.894826]  __driver_attach+0x50/0xc0
[    0.894826]  ? device_driver_attach+0x60/0x60
[    0.894826]  bus_for_each_dev+0x73/0xb0
[    0.894826]  bus_add_driver.cold+0xe3/0x1a5
[    0.894826]  driver_register+0x67/0xb0
[    0.894826]  ? ahci_pci_driver_init+0x15/0x15
[    0.894826]  piix_init+0x15/0x24
[    0.894826]  do_one_initcall+0x3f/0x1b0
[    0.894826]  kernel_init_freeable+0x19e/0x1e6
[    0.894826]  ? rest_init+0xa4/0xa4
[    0.894826]  kernel_init+0x5/0xfc
[    0.894826]  ret_from_fork+0x22/0x30
[    0.906406] bus: 'pci': add driver pata_amd
[    0.907253] bus: 'pci': add driver pata_oldpiix
[    0.908120] bus: 'pci': add driver pata_sch
```

## 从 acpi 到 pcie
所有的 acpi 都是从这里开始的:

```c
/*
#0  huxueshi () at drivers/base/bus.c:457
#1  bus_add_device (dev=dev@entry=0xffff8881002d90c0) at drivers/base/bus.c:457
#2  0xffffffff8167cb4d in device_add (dev=dev@entry=0xffff8881002d90c0) at drivers/base/core.c:3273
#3  0xffffffff814301d3 in pci_device_add (dev=dev@entry=0xffff8881002d9000, bus=bus@entry=0xffff888100259c00) at drivers/pci/probe.c:2498
#4  0xffffffff814305ff in pci_scan_single_device (devfn=0, bus=0xffff888100259c00) at drivers/pci/probe.c:2516 #5  pci_scan_single_device (bus=0xffff888100259c00, devfn=0) at drivers/pci/probe.c:2502
#6  0xffffffff8143066d in pci_scan_slot (bus=bus@entry=0xffff888100259c00, devfn=devfn@entry=0) at drivers/pci/probe.c:2591
#7  0xffffffff81431790 in pci_scan_child_bus_extend (bus=bus@entry=0xffff888100259c00, available_buses=available_buses@entry=0) at drivers/pci/probe.c:2808
#8  0xffffffff81431977 in pci_scan_child_bus (bus=bus@entry=0xffff888100259c00) at drivers/pci/probe.c:2938
#9  0xffffffff81471561 in acpi_pci_root_create (root=root@entry=0xffff888100222a00, ops=ops@entry=0xffffffff826411c0 <acpi_pci_root_ops>, info=info@entry=0xffff88810025 4360, sysdata=sysdata@entry=0xffff888100254398) at drivers/acpi/pci_root.c:925
#10 0xffffffff81b237bd in pci_acpi_scan_root (root=root@entry=0xffff888100222a00) at arch/x86/pci/acpi.c:368
#11 0xffffffff81b41564 in acpi_pci_root_add (device=0xffff888100217000, not_used=<optimized out>) at drivers/acpi/pci_root.c:597
#12 0xffffffff8146a143 in acpi_scan_attach_handler (device=0xffff888100217000) at drivers/acpi/scan.c:2038
#13 acpi_bus_attach (device=device@entry=0xffff888100217000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2086
#14 0xffffffff8146a090 in acpi_bus_attach (device=device@entry=0xffff888100216800, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#15 0xffffffff8146a090 in acpi_bus_attach (device=0xffff888100216000, first_pass=first_pass@entry=true) at drivers/acpi/scan.c:2107
#16 0xffffffff8146be9f in acpi_bus_scan (handle=handle@entry=0xffffffffffffffff) at drivers/acpi/scan.c:2167
#17 0xffffffff82b6f9a2 in acpi_scan_init () at drivers/acpi/scan.c:2342
#18 0xffffffff82b6f6d3 in acpi_init () at drivers/acpi/bus.c:1341
#19 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b6f2e1 <acpi_init>) at init/main.c:1249
#20 0xffffffff82b3d26b in do_initcall_level (command_line=0xffff888100123400 "root", level=4) at ./include/linux/compiler.h:234
#21 do_initcalls () at init/main.c:1338
#22 do_basic_setup () at init/main.c:1358
#23 kernel_init_freeable () at init/main.c:1560
#24 0xffffffff81b7c629 in kernel_init (unused=<optimized out>) at init/main.c:1447
#25 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#26 0x0000000000000000 in ?? ()
*/
```
其中 acpi_bus_attach 连续递归三次调用，那么说明 acpi 中的设备也是递归创建的。

- [ ] e1000 之类的东西，需要放入到 acpi 中吗 ?
- [ ] 那么至少 e1000 是需要放入的


## 剩余的设备都是从 pci 向下探测的吗 ?

## [ ] 中断路由是什么个东西
```c
[    0.535591] ACPI: PCI: Interrupt link LNKA configured for IRQ 10
[    0.535956] ACPI: PCI: Interrupt link LNKB configured for IRQ 10
[    0.536718] ACPI: PCI: Interrupt link LNKC configured for IRQ 11
[    0.536945] ACPI: PCI: Interrupt link LNKD configured for IRQ 11
[    0.537683] ACPI: PCI: Interrupt link LNKS configured for IRQ 9
```

```c
[    0.764758] ACPI: \_SB_.LNKB: Enabled at IRQ 10
```

中断到底如何路由的?

## [ ] hotplug 这的设备
在 device_add 中可以检测到:

```
[    0.464903] huxueshi device: 'device:02': device_add
[    0.465557] huxueshi device: 'device:03': device_add
[    0.465896] huxueshi device: 'device:04': device_add
[    0.466476] huxueshi device: 'device:05': device_add
[    0.466896] huxueshi device: 'device:06': device_add
[    0.467550] huxueshi device: 'device:07': device_add
[    0.467903] huxueshi device: 'device:08': device_add
[    0.468899] huxueshi device: 'device:09': device_add
[    0.469488] huxueshi device: 'device:0a': device_add
[    0.469896] huxueshi device: 'device:0b': device_add
[    0.470479] huxueshi device: 'device:0c': device_add
[    0.470896] huxueshi device: 'device:0d': device_add
[    0.471478] huxueshi device: 'device:0e': device_add
[    0.471917] huxueshi device: 'device:0f': device_add
[    0.472602] huxueshi device: 'device:10': device_add
[    0.472897] huxueshi device: 'device:11': device_add
[    0.473480] huxueshi device: 'device:12': device_add
[    0.473895] huxueshi device: 'device:13': device_add
[    0.474464] huxueshi device: 'device:14': device_add
[    0.474894] huxueshi device: 'device:15': device_add
[    0.475509] huxueshi device: 'device:16': device_add
[    0.475894] huxueshi device: 'device:17': device_add
[    0.476463] huxueshi device: 'device:18': device_add
[    0.476895] huxueshi device: 'device:19': device_add
[    0.477463] huxueshi device: 'device:1a': device_add
[    0.477895] huxueshi device: 'device:1b': device_add
[    0.478507] huxueshi device: 'device:1c': device_add
[    0.478897] huxueshi device: 'device:1d': device_add
[    0.479536] huxueshi device: 'device:1e': device_add
[    0.479895] huxueshi device: 'device:1f': device_add
[    0.480464] huxueshi device: 'device:20': device_add
```


[^1]: https://blog.csdn.net/tiantao2012/article/details/73775993
[^2]: https://blog.csdn.net/woai110120130/article/details/93318611
[^3]: https://uefi.org/sites/default/files/resources/ACPI%206_2_A_Sept29.pdf
[^4]: https://www.kernel.org/doc/ols/2005/ols2005v1-pages-59-76.pdf
