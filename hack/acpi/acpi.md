# acpi

## TODO
- [ ] 和 UEFI 的关系
- [ ] 和 PCIe 的关系
- [ ] ACPI 实现电源管理是通过什么硬件做到的，硬件需要何种适配

## 基本内容
- https://en.wikipedia.org/wiki/Advanced_Configuration_and_Power_Interface

ACPI 的一个关键功能就是处理电源管理，为此分别定义了 Global State，Device State, C State 和 P state

> Internally, ACPI advertises the available components and their functions to the operating system kernel using instruction lists ("methods") provided through the system firmware (UEFI or BIOS), which the kernel parses. ACPI then executes the desired operations written in ACPI Machine Language (such as the initialization of hardware components) using an embedded minimal virtual machine.

操作系统通过 UEFI 来控制 ACPI，然后 ACPI 利用其虚拟机来执行 AML 语言。

> To make use of the ACPI tables, the operating system must have an interpreter for the AML bytecode. A reference AML interpreter implementation is provided by the ACPI Component Architecture (ACPICA).

但是这里又说，操作系统通过持有 ACPICA 来解释 AML

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

## [ACPI: Design Principles and Concerns](https://www.ssi.gouv.fr/uploads/IMG/pdf/article_acpi.pdf)
> a userland service called acpid (ACPI daemon) that is functionally part of
the OSPM. acpid is configured through a set of configuration files stored
in the /etc/acpi directory, each of which specifying the expected system
behavior when an ACPI “Notify” event for a particular device is received.
For instance, the /etc/acpi/power file can be used to configure acpid so
that whenever a power button event is received, the shutdown command is
executed.

## Official Documents
### [ ](https://uefi.org/specs/ACPI/6.4/06_Device_Configuration/Device_Configuration.html)

https://lwn.net/Articles/367630/

- [MADT](https://wiki.osdev.org/MADT) : (Multiple APIC Description Tableacpi_ev_install_xrupt_handlers
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
- [ ] qemu/hw/i386/pc.c
- [ ] qemu/hw/acpi/cpu.c
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
- [ ] acpi_scan_add_handler
- [ ] acpi_get_table 可以直接获取 acpi table 出来，所以，这些 table 是什么时候构建的 ?

acpi 的解析[^2]

原来kernel中最终是通过acpi_evaluate_object 来调用bios中在asl中定义好的函数啊 [^1]

## 在 Guest 中间 disassembly acpi
参考 : https://01.org/linux-acpi/utilities

```
$ acpidump > acpidump.out
$ acpixtract -a acpidump.out
$ iasl -d TABLE.dat
```

#### [ ] qemu 是如何组装 fadt 的
- acpi_build
  - acpi_get_pm_info : 系统初始化的时候获取的信息
  - build_dsdt
  - build_fadt

acpi_build 会进行两次配置

- [ ] seabios 什么时候触发第二次 acpi_build 的
- [ ] 真实的机器上怎么可能发生动态分配 fadt 的事情啊

- [ ] 既然 i440fx 已经是一个 pcie 设备了，为什么需要将自己的端口通过 acpi 提供出去

#### i440fx-pm 配置 acpi-evt 的端口
```
0000-0cf7 : PCI Bus 0000:00
  0000-001f : dma1
  0020-0021 : pic1
  0040-0043 : timer0
  0050-0053 : timer1
  0060-0060 : keyboard
  0064-0064 : keyboard
  0070-0077 : rtc0
  0080-008f : dma page reg
  00a0-00a1 : pic2
  00c0-00df : dma2
  00f0-00ff : fpu
  0170-0177 : 0000:00:01.1
    0170-0177 : ata_piix
  01f0-01f7 : 0000:00:01.1
    01f0-01f7 : ata_piix
  0376-0376 : 0000:00:01.1
    0376-0376 : ata_piix
  03c0-03df : vga+
  03f2-03f2 : floppy
  03f4-03f5 : floppy
  03f6-03f6 : 0000:00:01.1
    03f6-03f6 : ata_piix
  03f7-03f7 : floppy
  03f8-03ff : serial
  0510-051b : QEMU0002:00
  0600-063f : 0000:00:01.3
    0600-0603 : ACPI PM1a_EVT_BLK
    0604-0605 : ACPI PM1a_CNT_BLK
    0608-060b : ACPI PM_TMR
  0700-070f : 0000:00:01.3
0cf8-0cff : PCI conf1
0d00-ffff : PCI Bus 0000:00
  afe0-afe3 : ACPI GPE0_BLK
  c000-c03f : 0000:00:03.0
    c000-c03f : e1000
  c040-c05f : 0000:00:05.0
  c060-c06f : 0000:00:01.1
    c060-c06f : ata_piix
```

从 pci -vvv 中间部分截取的信息:
```
00:01.3 Bridge: Intel Corporation 82371AB/EB/MB PIIX4 ACPI (rev 03)
    Subsystem: Red Hat, Inc. Qemu virtual machine
    Control: I/O+ Mem+ BusMaster- SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR+ FastB2B- DisINTx-
    Status: Cap- 66MHz- UDF- FastB2B+ ParErr- DEVSEL=medium >TAbort- <TAbort- <MAbort- >SERR- <PERR- INTx-
    Interrupt: pin A routed to IRQ 9
```
注意其实这个是  apci 普通的设备.

- [ ] 从 acpi 到 pci 的配置空间 ?


```txt
>>> bt
#0  pm_io_space_update (s=0x40) at ../hw/acpi/piix4.c:129
#1  0x0000555555af6801 in pm_write_config (d=0x555557e57210, address=64, val=1537, len=4) at ../hw/acpi/piix4.c:161
#2  0x0000555555932c1c in pci_host_config_write_common (pci_dev=0x555557e57210, addr=64, limit=256, val=1537, len=4) at ../hw/pci/pci_host.c:83
#3  0x0000555555932d8b in pci_data_write (s=0x555556d53260, addr=2147486528, val=1537, len=4) at ../hw/pci/pci_host.c:120
#4  0x0000555555932ec1 in pci_host_data_write (opaque=0x555556c3f960, addr=0, val=1537, len=4) at ../hw/pci/pci_host.c:167
#5  0x0000555555c938c8 in memory_region_write_accessor (mr=0x555556c3fd70, addr=0, value=0x7fffe890ffe8, size=4, shift=0, mask=4294967295, attrs=...) at ../softmmu/memo
```


- 这里是 bios 的 log
```txt
=== PCI new allocation pass #2 ===
PCI: IO: c000 - c04f
PCI: 32: 00000000c0000000 - 00000000fec00000                                               ---> pci_bios_map_devices
PCI: map device bdf=00:03.0  bar 1, addr 0000c000, size 00000040 [io]
PCI: map device bdf=00:01.1  bar 4, addr 0000c040, size 00000010 [io]
PCI: map device bdf=00:03.0  bar 6, addr feb80000, size 00040000 [mem]
PCI: map device bdf=00:03.0  bar 0, addr febc0000, size 00020000 [mem]
PCI: map device bdf=00:02.0  bar 6, addr febe0000, size 00010000 [mem]
PCI: map device bdf=00:04.0  bar 0, addr febf0000, size 00004000 [mem]
PCI: map device bdf=00:02.0  bar 4, addr febf4000, size 00001000 [mem]
PCI: map device bdf=00:02.0  bar 0, addr fe000000, size 00800000 [prefmem]
PCI: map device bdf=00:02.0  bar 2, addr fe800000, size 00004000 [prefmem]
PCI: init bdf=00:00.0 id=8086:1237
PCI: init bdf=00:01.0 id=8086:7000
PIIX3/PIIX4 init: elcr=00 0c
PCI: init bdf=00:01.1 id=8086:7010
PCI: init bdf=00:01.3 id=8086:7113                                                        -----> pci_bios_init_device -> pci_init_device -> ids->func
```
在 seabios 中，硬编码了一些
```c
static const struct pci_device_id pci_device_tbl[] = {
    /* PIIX4 Power Management device (for ACPI) */
    PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82371AB_3,
               piix4_pm_setup),
}

/* PIIX4 Power Management device (for ACPI) */
static void piix4_pm_setup(struct pci_device *pci, void *arg)
{
    PiixPmBDF = pci->bdf;
    piix4_pm_config_setup(pci->bdf);

    acpi_pm1a_cnt = acpi_pm_base + 0x04;
    pmtimer_setup(acpi_pm_base + 0x08);
}

static void piix4_pm_config_setup(u16 bdf)
{
    // acpi sci is hardwired to 9
    pci_config_writeb(bdf, PCI_INTERRUPT_LINE, 9);

    // 此时 acpi_pm_base = 0x600
    pci_config_writel(bdf, PIIX_PMBASE, acpi_pm_base | 1);                            ------> 40
    pci_config_writeb(bdf, PIIX_PMREGMISC, 0x01); /* enable PM io space */            ------> 80
    pci_config_writel(bdf, PIIX_SMBHSTBASE, (acpi_pm_base + 0x100) | 1);
    pci_config_writeb(bdf, PIIX_SMBHSTCFG, 0x09); /* enable SMBus io space */         ------> 0xd2
}
```

- [x] acpi_pm_base = 0x600 是怎么得到的?
  - seabios 日志 : Moving pm_base to 0x600
- [ ] piix4 的 pci 配置空间 : 0x40 / 0x80 都是个啥，为什么可以启动这个东西

从内核中间找到的:
```c
struct acpi_table_fadt {
    struct acpi_table_header header;    /* Common ACPI table header */
    u32 facs;       /* 32-bit physical address of FACS */
    u32 dsdt;       /* 32-bit physical address of DSDT */
    u8 model;       /* System Interrupt Model (ACPI 1.0) - not used in ACPI 2.0+ */
    u8 preferred_profile;   /* Conveys preferred power management profile to OSPM. */
    u16 sci_interrupt;  /* System vector of SCI interrupt */
    u32 smi_command;    /* 32-bit Port address of SMI command port */
    u8 acpi_enable;     /* Value to write to SMI_CMD to enable ACPI */
    u8 acpi_disable;    /* Value to write to SMI_CMD to disable ACPI */
    u8 s4_bios_request; /* Value to write to SMI_CMD to enter S4BIOS state */
    u8 pstate_control;  /* Processor performance state control */
    u32 pm1a_event_block;   /* 32-bit port address of Power Mgt 1a Event Reg Blk */
    u32 pm1b_event_block;   /* 32-bit port address of Power Mgt 1b Event Reg Blk */
    u32 pm1a_control_block; /* 32-bit port address of Power Mgt 1a Control Reg Blk */
    u32 pm1b_control_block; /* 32-bit port address of Power Mgt 1b Control Reg Blk */
    u32 pm2_control_block;  /* 32-bit port address of Power Mgt 2 Control Reg Blk */
    u32 pm_timer_block; /* 32-bit port address of Power Mgt Timer Ctrl Reg Blk */
    u32 gpe0_block;     /* 32-bit port address of General Purpose Event 0 Reg Blk */
    u32 gpe1_block;     /* 32-bit port address of General Purpose Event 1 Reg Blk */
    u8 pm1_event_length;    /* Byte Length of ports at pm1x_event_block */
    u8 pm1_control_length;  /* Byte Length of ports at pm1x_control_block */
    u8 pm2_control_length;  /* Byte Length of ports at pm2_control_block */
    u8 pm_timer_length; /* Byte Length of ports at pm_timer_block */
    u8 gpe0_block_length;   /* Byte Length of ports at gpe0_block */
    u8 gpe1_block_length;   /* Byte Length of ports at gpe1_block */
    u8 gpe1_base;       /* Offset in GPE number space where GPE1 events start */
    u8 cst_control;     /* Support for the _CST object and C-States change notification */
    u16 c2_latency;     /* Worst case HW latency to enter/exit C2 state */
    u16 c3_latency;     /* Worst case HW latency to enter/exit C3 state */
    u16 flush_size;     /* Processor memory cache line width, in bytes */
    u16 flush_stride;   /* Number of flush strides that need to be read */
    u8 duty_offset;     /* Processor duty cycle index in processor P_CNT reg */
    u8 duty_width;      /* Processor duty cycle value bit width in P_CNT register */
    u8 day_alarm;       /* Index to day-of-month alarm in RTC CMOS RAM */
    u8 month_alarm;     /* Index to month-of-year alarm in RTC CMOS RAM */
    u8 century;     /* Index to century in RTC CMOS RAM */
    u16 boot_flags;     /* IA-PC Boot Architecture Flags (see below for individual flags) */
    u8 reserved;        /* Reserved, must be zero */
    u32 flags;      /* Miscellaneous flag bits (see below for individual flags) */
    struct acpi_generic_address reset_register; /* 64-bit address of the Reset register */
    u8 reset_value;     /* Value to write to the reset_register port to reset the system */
    u16 arm_boot_flags; /* ARM-Specific Boot Flags (see below for individual flags) (ACPI 5.1) */
    u8 minor_revision;  /* FADT Minor Revision (ACPI 5.1) */
    u64 Xfacs;      /* 64-bit physical address of FACS */
    u64 Xdsdt;      /* 64-bit physical address of DSDT */
    struct acpi_generic_address xpm1a_event_block;  /* 64-bit Extended Power Mgt 1a Event Reg Blk address */
    struct acpi_generic_address xpm1b_event_block;  /* 64-bit Extended Power Mgt 1b Event Reg Blk address */
    struct acpi_generic_address xpm1a_control_block;    /* 64-bit Extended Power Mgt 1a Control Reg Blk address */
    struct acpi_generic_address xpm1b_control_block;    /* 64-bit Extended Power Mgt 1b Control Reg Blk address */
    struct acpi_generic_address xpm2_control_block; /* 64-bit Extended Power Mgt 2 Control Reg Blk address */
    struct acpi_generic_address xpm_timer_block;    /* 64-bit Extended Power Mgt Timer Ctrl Reg Blk address */
    struct acpi_generic_address xgpe0_block;    /* 64-bit Extended General Purpose Event 0 Reg Blk address */
    struct acpi_generic_address xgpe1_block;    /* 64-bit Extended General Purpose Event 1 Reg Blk address */
    struct acpi_generic_address sleep_control;  /* 64-bit Sleep Control register (ACPI 5.0) */
    struct acpi_generic_address sleep_status;   /* 64-bit Sleep Status register (ACPI 5.0) */
    u64 hypervisor_id;  /* Hypervisor Vendor ID (ACPI 6.0) */
};
```

```txt
>>> p acpi_gbl_FADT
$3 = {
  header = {
    signature = "FACP",
    length = 276,
    revision = 1 '\001',
    checksum = 129 '\201',
    oem_id = "BOCHS ",
    oem_table_id = "BXPC    ",
    oem_revision = 1,
    asl_compiler_id = "BXPC",
    asl_compiler_revision = 1
  },
  facs = 3221094400,
  dsdt = 3221094464,
  model = 1 '\001',
  preferred_profile = 0 '\000',
  sci_interrupt = 9,
  smi_command = 178,
  acpi_enable = 241 '\361',
  acpi_disable = 240 '\360',
  s4_bios_request = 0 '\000',
  pstate_control = 0 '\000',
  pm1a_event_block = 1536,
  pm1b_event_block = 0,
  pm1a_control_block = 1540,
  pm1b_control_block = 0,
  pm2_control_block = 0,
  pm_timer_block = 1544,
  gpe0_block = 45024,
  gpe1_block = 0,
  pm1_event_length = 4 '\004',
  pm1_control_length = 2 '\002',
  pm2_control_length = 0 '\000',
  pm_timer_length = 4 '\004',
  gpe0_block_length = 4 '\004',
  gpe1_block_length = 0 '\000',
  gpe1_base = 0 '\000',
  cst_control = 0 '\000',
  c2_latency = 4095,
  c3_latency = 4095,
  flush_size = 0,
  flush_stride = 0,
  duty_offset = 0 '\000',
  duty_width = 0 '\000',
  day_alarm = 0 '\000',
  month_alarm = 0 '\000',
  century = 50 '2',
  boot_flags = 0,
  reserved = 0 '\000',
  flags = 32933,
  reset_register = {
    space_id = 0 '\000',
    bit_width = 0 '\000',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 0
  },
  reset_value = 0 '\000',
  arm_boot_flags = 0,
  minor_revision = 0 '\000',
  Xfacs = 0,
  Xdsdt = 3221094464,
  xpm1a_event_block = {
    space_id = 1 '\001',
    bit_width = 32 ' ',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 1536 // 0x600
  },
  xpm1b_event_block = {
    space_id = 0 '\000',
    bit_width = 0 '\000',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 0
  },
  xpm1a_control_block = {
    space_id = 1 '\001',
    bit_width = 16 '\020',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 1540 // 0x604
  },
  xpm1b_control_block = {
    space_id = 0 '\000',
    bit_width = 0 '\000',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 0
  },
  xpm2_control_block = {
    space_id = 0 '\000',
    bit_width = 0 '\000',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 0
  },
  xpm_timer_block = {
    space_id = 1 '\001',
    bit_width = 32 ' ',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 1544
  },
  xgpe0_block = {
    space_id = 1 '\001',
    bit_width = 32 ' ',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 45024
  },
  xgpe1_block = {
    space_id = 0 '\000',
    bit_width = 0 '\000',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 0
  },
  sleep_control = {
    space_id = 0 '\000',
    bit_width = 0 '\000',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 0
  },
  sleep_status = {
    space_id = 0 '\000',
    bit_width = 0 '\000',
    bit_offset = 0 '\000',
    access_width = 0 '\000',
    address = 0
  },
  hypervisor_id = 0
}
>>>
```




## acpi in address space
```txt
    0000000000000600-000000000000063f (prio 0, i/o): piix4-pm
      0000000000000600-0000000000000603 (prio 0, i/o): acpi-evt
      0000000000000604-0000000000000605 (prio 0, i/o): acpi-cnt
      0000000000000608-000000000000060b (prio 0, i/o): acpi-tmr
    0000000000000700-000000000000073f (prio 0, i/o): pm-smbus
    0000000000000cf8-0000000000000cfb (prio 0, i/o): pci-conf-idx
    0000000000000cf9-0000000000000cf9 (prio 1, i/o): piix3-reset-control
    0000000000000cfc-0000000000000cff (prio 0, i/o): pci-conf-data
    0000000000005658-0000000000005658 (prio 0, i/o): vmport
    000000000000ae00-000000000000ae17 (prio 0, i/o): acpi-pci-hotplug
    000000000000af00-000000000000af0b (prio 0, i/o): acpi-cpu-hotplug
    000000000000afe0-000000000000afe3 (prio 0, i/o): acpi-gpe0
```
下面三个设备在 acpi 中间存在对应的配置的

seabios.md 中的部分内容
```txt
    PHPR, hid, sta (0x8), crs
        i/o 0xae00 -> 0xae17  // acpi-pci-hotplug
    GPE0, hid, sta (0x8), crs // acpi-gpe0
        i/o 0xafe0 -> 0xafe3
    \_SB.CPUS, hid
    \_SB.PCI0.PRES, hid, crs // acpi-cpu-hotplug
        i/o 0xaf00 -> 0xaf0b
```

- [x] piix4-pm 的子空间什么时候定义的
  - piix4_pm_realize : 以及几个子空间

## [ ] Notify

## [ ] 从 acpi 的 scan 到 pci 的 scan
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

```txt
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

```txt
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

```txt
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

## 中断路由(routing) 是什么个东西
在这个文档中间搜索 PIRQx ROUTE CONTROL REGISTERS,
[PIRQx ROUTE CONTROL REGISTERS](https://composter.com.ua/documents/Intel_82371FB_%2082371SB.pdf)
> These registers control the routing of the PIRQ[A:D]# signals to the IRQ inputs of the interrupt controller.

- [ ] 似乎是 piix3 控制着从 PIR 到达中断控制器之间的联系
  - **https://habr.com/en/post/501912/** : 好了，还是 coreboot 三部曲

piix3 的 pci 配置空间 PIIX_PIRQCA 是这个设备的扩展功能:
```c
/* PIRQRC[A:D]: PIRQx Route Control Registers */
#define PIIX_PIRQCA 0x60
#define PIIX_PIRQCB 0x61
#define PIIX_PIRQCC 0x62
#define PIIX_PIRQCD 0x63
```

内核 acpi_pci_link_get_current 函数通过执行 `_CRS` 获取 link

```txt
[    0.535591] ACPI: PCI: Interrupt link LNKA configured for IRQ 10
[    0.535956] ACPI: PCI: Interrupt link LNKB configured for IRQ 10
[    0.536718] ACPI: PCI: Interrupt link LNKC configured for IRQ 11
[    0.536945] ACPI: PCI: Interrupt link LNKD configured for IRQ 11
[    0.537683] ACPI: PCI: Interrupt link LNKS configured for IRQ 9
```

```c
[    0.764758] ACPI: \_SB_.LNKB: Enabled at IRQ 10
```
上面的 Interrupt link LNKA 的 10 10 11 11 这四个数组就是完全受 seabios 的 pci_irqs 控制的
```c
/* host irqs corresponding to PCI irqs A-D */
const u8 pci_irqs[4] = {
    11, 11, 10, 10
};
```
应该是处理 ISA Bridge 的


这个人分析了类似的问题:
https://gist.github.com/mcastelino/4acda7c2407f1c51e68f3f994d8ffc98

## 网卡是一个什么东西

- [x] 找到 qemu 中生成 link 的方法
  - 在 acpi_build.c::build_link_dev

即使是修改了 dsdt 的内容，那么

当然现在还存在一些小问题:
- [ ] dmesg 是 LNKA / LNKB / LNKC 之类的，但是设备都是 pin A routed to
- [ ] acpi 是静态的配置的，设备是动态添加的啊

参考资料:
https://unix.stackexchange.com/questions/368926/what-does-this-mean-interrupt-pin-a-routed-to-irq-17

- [ ] device 的 pci 的配置空间哪里记录了 lspci -vvv 中的 `Interrupt: pin A routed to IRQ 9`


在 seabios 中初始化 pci_bios_init_device :
```c
static void pci_bios_init_device(struct pci_device *pci)
{
    dprintf(1, "PCI: init bdf=%pP id=%04x:%04x\n"
            , pci, pci->vendor, pci->device);

    /* map the interrupt */
    u16 bdf = pci->bdf;
    int pin = pci_config_readb(bdf, PCI_INTERRUPT_PIN);
    if (pin != 0)
        pci_config_writeb(bdf, PCI_INTERRUPT_LINE, pci_slot_get_irq(pci, pin));
```
- pin 等于 0, 比如 ISA bridge / IDE bridge
- 存在 pin 的其余全部需要初始化

- [ ] 应该修改了硬件才对，至少，从 qemu 的角度看，qemu 需要知道是那么中断才可以发送给 guest
  - [ ] 比如说，nvme 驱动模拟传递完成数据之后，应该就是需要注入中断，让 guest 知道输入传输完成了, 怎么知道注入是 10, 11 之后的
- [x] 当使用 nomsi 模式，这些 pci 设备都是使用 irq 10 和 11 应该一定是存在一个机制来确定到底是哪一个设备吧
  - 应该是的，毕竟还可以从 pcie 设备中间读取自己是否触发中断啊

存在 interrupt routing 的配置
```c
/* PIRQRC[A:D]: PIRQx Route Control Registers */
#define PIIX_PIRQCA 0x60
#define PIIX_PIRQCB 0x61
#define PIIX_PIRQCC 0x62
#define PIIX_PIRQCD 0x63
```


```c
/* host irqs corresponding to PCI irqs A-D */
const u8 pci_irqs[4] = {
    10, 10, 11, 11
};

// Return the global irq number corresponding to a host bus device irq pin.
static int piix_pci_slot_get_irq(struct pci_device *pci, int pin)
{
    int slot_addend = 0;

    while (pci->parent != NULL) {
        slot_addend += pci_bdf_to_dev(pci->bdf);
        pci = pci->parent;
    }
    slot_addend += pci_bdf_to_dev(pci->bdf) - 1;
    return pci_irqs[(pin - 1 + slot_addend) & 3];
}
```
piix_pci_slot_get_irq 感觉这就是纯粹的物理编码规则而已啊，而 pci_irqs 是主板决定的。


PCI_INTERRUPT_LINE : 发出去中断号
PCI_INTERRUPT_PIN : 表示存在中断引脚可以发送中断

但是 pcie 的东西是在其他的位置探测的:


## [ ] hotplug 这的设备
在 device_add 中可以检测到:

```txt
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

## fadt : acpi event interrupt number

acpi_table_fadt::sci_interrupt  提供了
```txt
[    0.846127] Call Trace:
[    0.846130] [<900000000020864c>] show_stack+0x2c/0x100
[    0.846133] [<9000000000ec3968>] dump_stack+0x90/0xc0
[    0.846136] [<900000000029f8e0>] irq_domain_set_mapping+0x90/0xb8
[    0.846138] [<90000000002a0698>] __irq_domain_alloc_irqs+0x200/0x2e0
[    0.846141] [<90000000002a0bf0>] irq_create_fwspec_mapping+0x258/0x368
[    0.846143] [<900000000020f3d0>] acpi_register_gsi+0x70/0x100
[    0.846145] [<900000000020f750>] acpi_gsi_to_irq+0x28/0x48
[    0.846148] [<9000000000890044>] acpi_os_install_interrupt_handler+0x84/0x148
[    0.846151] [<90000000008ac830>] acpi_ev_install_xrupt_handlers+0x24/0x98
[    0.846155] [<90000000012f7a14>] acpi_init+0xc0/0x338
[    0.846157] [<90000000002004fc>] do_one_initcall+0x6c/0x170
[    0.846159] [<90000000012d4ce0>] kernel_init_freeable+0x1f8/0x2b8
[    0.846162] [<9000000000eda774>] kernel_init+0x10/0xf4
[    0.846164] [<9000000000203cc8>] ret_from_kernel_thread+0xc/0x10
```

[^1]: https://blog.csdn.net/tiantao2012/article/details/73775993
[^2]: https://blog.csdn.net/woai110120130/article/details/93318611
[^3]: https://uefi.org/sites/default/files/resources/ACPI%206_2_A_Sept29.pdf
[^4]: https://www.kernel.org/doc/ols/2005/ols2005v1-pages-59-76.pdf
