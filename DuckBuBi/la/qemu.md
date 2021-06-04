# qemu

- [ ] 找到串口的参数地址吗?
- [ ] 干他娘的，我需要搞到这个内核

# hw/loongson/mips_ls3a7a.c

- [ ] loaderparams 是怎么做的?
- [ ] cpu_reset 如何构建的, 实际上，这个 ROM 也不是第一个执行的代码

## kernel 虚拟地址
- [x] kernel 运行的物理地址是在哪里的呀
  - `reset_info[0]->vector` : 虽然这个 vector 实际上是一个虚拟地址的啊
在 main_cpu_reset 中间初始化，这个 pc 的设置就是 0x90000000000002000, 但是内核的具体物理地址是不知道的
如果做直接映射，那么物理地址是什么关系。
- `glue(load_elf, SZ)` 中间会调用参数 `cpu_mips_kseg0_to_phys` 来将虚拟地址映射为物理地址, 其实就是将 0x90000000000000000 的偏移去掉
- rom_add_elf_program 会将这个 section 正好放到物理地址上，所以，实际上，无论是 linker 的偏移在什么位置，都是可以的


- loaderparams 的参数是如何传递给内核的

## mmio
- store_helper
  - io_writex


## info qtree
```c
/*
(qemu) info qtree
bus: main-system-bus
  type System
  dev: hpet, id ""
    gpio-in "" 2
    gpio-out "" 1
    gpio-out "sysbus-irq" 32
    timers = 3 (0x3)
    msi = false
    hpet-intcap = 1 (0x1)
    hpet-offset-saved = true
    mmio 00000e0010001000/0000000000000400
  dev: loongson_i2c, id ""
    gpio-out "sysbus-irq" 1
    shift = 0 (0x0)
    mmio 0000000010090500/0000000000000008
    bus: i2c
      type i2c-bus
      dev: ds1338, id ""
        address = 104 (0x68)
  dev: loongson_i2c, id ""
    gpio-out "sysbus-irq" 1
    shift = 0 (0x0)
    mmio 0000000010090400/0000000000000008
    bus: i2c
      type i2c-bus
      dev: ds1338, id ""
        address = 104 (0x68)
  dev: loongson_i2c, id ""
    gpio-out "sysbus-irq" 1
    shift = 0 (0x0)
    mmio 0000000010090300/0000000000000008
    bus: i2c
      type i2c-bus
      dev: ds1338, id ""
        address = 104 (0x68)
  dev: loongson_i2c, id ""
    gpio-out "sysbus-irq" 1
    shift = 0 (0x0)
    mmio 0000000010090200/0000000000000008
    bus: i2c
      type i2c-bus
      dev: ds1338, id ""
        address = 104 (0x68)
  dev: loongson_i2c, id ""
    gpio-out "sysbus-irq" 1
    shift = 0 (0x0)
    mmio 0000000010090100/0000000000000008
    bus: i2c
      type i2c-bus
      dev: ds1338, id ""
        address = 104 (0x68)
  dev: loongson_i2c, id ""
    gpio-out "sysbus-irq" 1
    shift = 0 (0x0)
    mmio 0000000010090000/0000000000000008
    bus: i2c
      type i2c-bus
      dev: ds1338, id ""
        address = 104 (0x68)
  dev: loongson_rtc, id ""
    gpio-out "sysbus-irq" 3
    mmio 00000000100d0100/0000000000000100
  dev: loongson_acpi, id ""
    gpio-out "sysbus-irq" 1
    mmio 00000000100d0000/0000000000000100
  dev: ls3a7a-pcihost, id ""
    mmio 000000fe00004800/0000000000000100
    mmio 000000001a000000/0000000002000000
    mmio ffffffffffffffff/0000000000000100
    mmio ffffffffffffffff/0000000002000000
    bus: ls7a.0
      type PCIE
      dev: ls2k-ahci, id ""
        addr = 08.0
        romfile = ""
        rombar = 1 (0x1)
        multifunction = true
        command_serr_enable = true
        x-pcie-lnksta-dllla = true
        x-pcie-extcap-init = true
        failover_pair_id = ""
        class SATA controller, addr 00:08.0, pci id 0014:7a08 (sub 1af4:1100)
        bar 0: mem at 0xffffffffffffffff [0xffe]
        bus: ide.5
          type IDE
        bus: ide.4
          type IDE
        bus: ide.3
          type IDE
        bus: ide.2
          type IDE
          dev: ide-cd, id ""
            drive = "ide1-cd0"
            logical_block_size = 512 (0x200)
            physical_block_size = 512 (0x200)
            min_io_size = 0 (0x0)
            opt_io_size = 0 (0x0)
            discard_granularity = 512 (0x200)
            write-cache = "auto"
            share-rw = false
            rerror = "auto"
            werror = "auto"
            ver = "2.5+"
            wwn = 0 (0x0)
            serial = "QM00005"
            model = ""
            unit = 0 (0x0)
        bus: ide.1
          type IDE
        bus: ide.0
          type IDE
      dev: usb-ehci, id ""
        maxframes = 128 (0x80)
        addr = 04.1
        romfile = ""
        rombar = 1 (0x1)
        multifunction = true
        command_serr_enable = true
        x-pcie-lnksta-dllla = true
        x-pcie-extcap-init = true
        failover_pair_id = ""
        class USB controller, addr 00:04.1, pci id 0014:7a14 (sub 1af4:1100)
        bar 0: mem at 0xffffffffffffffff [0xffe]
        bus: usb-bus.1
          type usb-bus
      dev: pci-ohci, id ""
        masterbus = ""
        num-ports = 3 (0x3)
        firstport = 0 (0x0)
        addr = 04.0
        romfile = ""
        rombar = 1 (0x1)
        multifunction = true
        command_serr_enable = true
        x-pcie-lnksta-dllla = true
        x-pcie-extcap-init = true
        failover_pair_id = ""
        class USB controller, addr 00:04.0, pci id 0014:7a24 (sub 1af4:1100)
        bar 0: mem at 0xffffffffffffffff [0xfe]
        bus: usb-bus.0
          type usb-bus
      dev: pci-synopgmac, id ""
        mac = "52:54:00:12:34:57"
        netdev = ""
        buswidth = 128 (0x80)
        enh_desc = 1 (0x1)
        addr = 03.1
        romfile = ""
        rombar = 1 (0x1)
        multifunction = true
        command_serr_enable = true
        x-pcie-lnksta-dllla = true
        x-pcie-extcap-init = true
        failover_pair_id = ""
        class Ethernet controller, addr 00:03.1, pci id 0014:7a03 (sub 1af4:1100)
        bar 0: mem at 0xffffffffffffffff [0x1ffe]
      dev: pci-synopgmac, id ""
        mac = "52:54:00:12:34:56"
        netdev = "hub0port0"
        buswidth = 128 (0x80)
        enh_desc = 1 (0x1)
        addr = 03.0
        romfile = ""
        rombar = 1 (0x1)
        multifunction = true
        command_serr_enable = true
        x-pcie-lnksta-dllla = true
        x-pcie-extcap-init = true
        failover_pair_id = ""
        class Ethernet controller, addr 00:03.0, pci id 0014:7a03 (sub 1af4:1100)
        bar 0: mem at 0xffffffffffffffff [0x1ffe]
      dev: LS7A_Bonito, id ""
        addr = 0a.0
        romfile = ""
        rombar = 1 (0x1)
        multifunction = true
        command_serr_enable = true
        x-pcie-lnksta-dllla = true
        x-pcie-extcap-init = true
        failover_pair_id = ""
        class PCI bridge, addr 00:0a.0, pci id df53:00d5 (sub 0000:0000)
        bus: Advanced PCI Bus secondary bridge 1
          type PCI
      dev: LS7A_Bonito, id ""
        addr = 09.0
        romfile = ""
        rombar = 1 (0x1)
        multifunction = true
        command_serr_enable = true
        x-pcie-lnksta-dllla = true
        x-pcie-extcap-init = true
        failover_pair_id = ""
        class PCI bridge, addr 00:09.0, pci id df53:00d5 (sub 0000:0000)
        bus: Advanced PCI Bus secondary bridge 1
          type PCI
```

## info mtree
```c
/*
(qemu) info mtree
address-space: memory
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-000000000fffffff (prio 0, i/o): alias lowmem @mips_r4k.ram0 0000000000000000-000000000fffffff
    000000000ff00000-000000000fffffff (prio 1, i/o): ddr
    0000000010000000-0000000011ffffff (prio 0, i/o): alias pcisubmem @system 0000000010000000-0000000011ffffff
    0000000010000000-00000000100003ff (prio 0, i/o): ls7a_int
    0000000010002000-0000000010002013 (prio 1, i/o): 0x10002000
    000000001001041c-000000001001041f (prio 1, i/o): 0x1001041c
    0000000010080000-0000000010080007 (prio 0, i/o): serial
    0000000010080100-0000000010080107 (prio 0, i/o): serial
    0000000010080200-0000000010080207 (prio 0, i/o): serial
    0000000010080300-0000000010080307 (prio 0, i/o): serial
    0000000010090000-0000000010090007 (prio 0, i/o): loongson i2c
    0000000010090100-0000000010090107 (prio 0, i/o): loongson i2c
    0000000010090200-0000000010090207 (prio 0, i/o): loongson i2c
    0000000010090300-0000000010090307 (prio 0, i/o): loongson i2c
    0000000010090400-0000000010090407 (prio 0, i/o): loongson i2c
    0000000010090500-0000000010090507 (prio 0, i/o): loongson i2c
    00000000100d0000-00000000100d00ff (prio 0, i/o): loongson_acpi
    00000000100d0100-00000000100d01ff (prio 0, i/o): loongson_rtc
    000000001a000000-000000001bffffff (prio 0, i/o): south-bridge-pci-config
    000000001fc00000-000000001fffffff (prio 0, rom): mips_r4k.bios
    000000001fe00100-000000001fe00103 (prio 1, i/o): ((hwaddr)0x1fe00100 | off)
    000000001fe00128-000000001fe0012b (prio 1, i/o): ((hwaddr)0x1fe00128 | off)
    000000001fe00134-000000001fe00137 (prio 1, i/o): ((hwaddr)0x1fe00134 | off)
    000000001fe00180-000000001fe00187 (prio 0, i/o): 0x1fe00180
    000000001fe0019c-000000001fe0019f (prio 1, i/o): ((hwaddr)0x1fe0019c | off)
    000000001fe001d0-000000001fe001d3 (prio 1, i/o): ((hwaddr)0x1fe001d0 | off)
    000000001fe001e0-000000001fe001e7 (prio 0, i/o): serial
    000000001fe001e8-000000001fe001ef (prio 0, i/o): serial
    000000003ff00008-000000003ff0000f (prio 1, i/o): ((hwaddr)0x3ff00008 | off)
    000000003ff00400-000000003ff00407 (prio 1, i/o): ((hwaddr)0x3ff00400 | off)
    000000003ff00408-000000003ff0040f (prio 1, i/o): ((hwaddr)0x3ff00408 | off)
    000000003ff00420-000000003ff00427 (prio 1, i/o): ((hwaddr)0x3ff00420 | off)
    000000003ff01000-000000003ff010ff (prio 0, i/o): gipi0
    000000003ff01400-000000003ff01cff (prio 0, i/o): ls3a_intctl
    0000000040000000-000000007fffffff (prio 0, i/o): alias pcisubmem @system 0000000040000000-000000007fffffff
    0000000080000000-00000000bfffffff (prio 0, ram): mips_r4k.ram0
    000000fe00004800-000000fe000048ff (prio 0, i/o): north-bridge-pci-config
    00000e000f000000-00000e000f00ffff (prio 0, i/o): alias lowmem @mips_r4k.ram0 000000000f000000-000000000f00ffff
    00000e0010001000-00000e00100013ff (prio 0, i/o): hpet
    00000e001001041c-00000e001001041f (prio 1, i/o): 0xe001001041c
    00000e0010010424-00000e0010010427 (prio 1, i/o): 0xe0010010424
    00000e0010010480-00000e00100104cf (prio 1, i/o): 0xe0010010480
    00000e0010010594-00000e0010010597 (prio 1, i/o): 0xe0010010594
    00000e00100105b4-00000e00100105b7 (prio 1, i/o): 0xe00100105b4
    00000e00100105d0-00000e00100105df (prio 1, i/o): 0xe00100105d0
    00000e00100105f0-00000e00100105ff (prio 1, i/o): 0xe00100105f0
    00000e0010010610-00000e001001061f (prio 1, i/o): 0xe0010010610
    00000e0010010694-00000e0010010697 (prio 1, i/o): 0xe0010010694
    00000e00100106b4-00000e00100106b7 (prio 1, i/o): 0xe00100106b4
    00000e00100106d0-00000e00100106df (prio 1, i/o): 0xe00100106d0
    00000e00100106f0-00000e00100106ff (prio 1, i/o): 0xe00100106f0
    00000e0010010710-00000e001001071f (prio 1, i/o): 0xe0010010710
    00000e0010010794-00000e0010010797 (prio 1, i/o): 0xe0010010794
    00000e00100107b4-00000e00100107b7 (prio 1, i/o): 0xe00100107b4
    00000e00100107d0-00000e00100107df (prio 1, i/o): 0xe00100107d0
    00000e00100107f0-00000e00100107ff (prio 1, i/o): 0xe00100107f0
    00000e0010010810-00000e001001081f (prio 1, i/o): 0xe0010010810
    00000e0010010894-00000e0010010897 (prio 1, i/o): 0xe0010010894
    00000e00100108b4-00000e00100108b7 (prio 1, i/o): 0xe00100108b4
    00000e00100108d0-00000e00100108df (prio 1, i/o): 0xe00100108d0
    00000e00100108f0-00000e00100108ff (prio 1, i/o): 0xe00100108f0
    00000e0010010910-00000e001001091f (prio 1, i/o): 0xe0010010910
    00000e0010013ff8-00000e0010013ffb (prio 1, i/o): 0xe0010013ff8
    00000e0040000000-00000e007fffffff (prio 0, i/o): alias pcisubmem1 @system 0000000040000000-000000007fffffff
    00000e0040000160-00000e0040000167 (prio 1, i/o): 0xe0040000160
    00000efdfb000000-00000efdfb0000ff (prio 0, i/o): ls3a_intctl
    00000efdfb000044-00000efdfb000047 (prio 1, i/o): 0xefdfb000044
    00000efdfb000178-00000efdfb00017b (prio 1, i/o): 0xefdfb000178
    00000efdfc000000-00000efdfc00ffff (prio 0, i/o): system
      00000efdfc000000-00000efdfc000000 (prio 1, i/o): alias pci_bridge_io @pci_bridge_io 0000000000000000-0000000000000000
      00000efdfc000000-00000efdfc000000 (prio 1, i/o): alias pci_bridge_io @pci_bridge_io 0000000000000000-0000000000000000
    00000efdfe0001f4-00000efdfe0001f7 (prio 1, i/o): 0xefdfe0001f4

address-space: I/O
  0000000000000000-000000000000ffff (prio 0, i/o): io

address-space: cpu-memory-0
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-000000000fffffff (prio 0, i/o): alias lowmem @mips_r4k.ram0 0000000000000000-000000000fffffff
    000000000ff00000-000000000fffffff (prio 1, i/o): ddr
    0000000010000000-0000000011ffffff (prio 0, i/o): alias pcisubmem @system 0000000010000000-0000000011ffffff
    0000000010000000-00000000100003ff (prio 0, i/o): ls7a_int
    0000000010002000-0000000010002013 (prio 1, i/o): 0x10002000
    000000001001041c-000000001001041f (prio 1, i/o): 0x1001041c
    0000000010080000-0000000010080007 (prio 0, i/o): serial
    0000000010080100-0000000010080107 (prio 0, i/o): serial
    0000000010080200-0000000010080207 (prio 0, i/o): serial
    0000000010080300-0000000010080307 (prio 0, i/o): serial
    0000000010090000-0000000010090007 (prio 0, i/o): loongson i2c
    0000000010090100-0000000010090107 (prio 0, i/o): loongson i2c
    0000000010090200-0000000010090207 (prio 0, i/o): loongson i2c
    0000000010090300-0000000010090307 (prio 0, i/o): loongson i2c
    0000000010090400-0000000010090407 (prio 0, i/o): loongson i2c
    0000000010090500-0000000010090507 (prio 0, i/o): loongson i2c
    00000000100d0000-00000000100d00ff (prio 0, i/o): loongson_acpi
    00000000100d0100-00000000100d01ff (prio 0, i/o): loongson_rtc
    000000001a000000-000000001bffffff (prio 0, i/o): south-bridge-pci-config
    000000001fc00000-000000001fffffff (prio 0, rom): mips_r4k.bios
    000000001fe00100-000000001fe00103 (prio 1, i/o): ((hwaddr)0x1fe00100 | off)
    000000001fe00128-000000001fe0012b (prio 1, i/o): ((hwaddr)0x1fe00128 | off)
    000000001fe00134-000000001fe00137 (prio 1, i/o): ((hwaddr)0x1fe00134 | off)
    000000001fe00180-000000001fe00187 (prio 0, i/o): 0x1fe00180
    000000001fe0019c-000000001fe0019f (prio 1, i/o): ((hwaddr)0x1fe0019c | off)
    000000001fe001d0-000000001fe001d3 (prio 1, i/o): ((hwaddr)0x1fe001d0 | off)
    000000001fe001e0-000000001fe001e7 (prio 0, i/o): serial
    000000001fe001e8-000000001fe001ef (prio 0, i/o): serial
    000000003ff00008-000000003ff0000f (prio 1, i/o): ((hwaddr)0x3ff00008 | off)
    000000003ff00400-000000003ff00407 (prio 1, i/o): ((hwaddr)0x3ff00400 | off)
    000000003ff00408-000000003ff0040f (prio 1, i/o): ((hwaddr)0x3ff00408 | off)
    000000003ff00420-000000003ff00427 (prio 1, i/o): ((hwaddr)0x3ff00420 | off)
    000000003ff01000-000000003ff010ff (prio 0, i/o): gipi0
    000000003ff01400-000000003ff01cff (prio 0, i/o): ls3a_intctl
    0000000040000000-000000007fffffff (prio 0, i/o): alias pcisubmem @system 0000000040000000-000000007fffffff
    0000000080000000-00000000bfffffff (prio 0, ram): mips_r4k.ram0
    000000fe00004800-000000fe000048ff (prio 0, i/o): north-bridge-pci-config
    00000e000f000000-00000e000f00ffff (prio 0, i/o): alias lowmem @mips_r4k.ram0 000000000f000000-000000000f00ffff
    00000e0010001000-00000e00100013ff (prio 0, i/o): hpet
    00000e001001041c-00000e001001041f (prio 1, i/o): 0xe001001041c
    00000e0010010424-00000e0010010427 (prio 1, i/o): 0xe0010010424
    00000e0010010480-00000e00100104cf (prio 1, i/o): 0xe0010010480
    00000e0010010594-00000e0010010597 (prio 1, i/o): 0xe0010010594
    00000e00100105b4-00000e00100105b7 (prio 1, i/o): 0xe00100105b4
    00000e00100105d0-00000e00100105df (prio 1, i/o): 0xe00100105d0
    00000e00100105f0-00000e00100105ff (prio 1, i/o): 0xe00100105f0
    00000e0010010610-00000e001001061f (prio 1, i/o): 0xe0010010610
    00000e0010010694-00000e0010010697 (prio 1, i/o): 0xe0010010694
    00000e00100106b4-00000e00100106b7 (prio 1, i/o): 0xe00100106b4
    00000e00100106d0-00000e00100106df (prio 1, i/o): 0xe00100106d0
    00000e00100106f0-00000e00100106ff (prio 1, i/o): 0xe00100106f0
    00000e0010010710-00000e001001071f (prio 1, i/o): 0xe0010010710
    00000e0010010794-00000e0010010797 (prio 1, i/o): 0xe0010010794
    00000e00100107b4-00000e00100107b7 (prio 1, i/o): 0xe00100107b4
    00000e00100107d0-00000e00100107df (prio 1, i/o): 0xe00100107d0
    00000e00100107f0-00000e00100107ff (prio 1, i/o): 0xe00100107f0
    00000e0010010810-00000e001001081f (prio 1, i/o): 0xe0010010810
    00000e0010010894-00000e0010010897 (prio 1, i/o): 0xe0010010894
    00000e00100108b4-00000e00100108b7 (prio 1, i/o): 0xe00100108b4
    00000e00100108d0-00000e00100108df (prio 1, i/o): 0xe00100108d0
    00000e00100108f0-00000e00100108ff (prio 1, i/o): 0xe00100108f0
    00000e0010010910-00000e001001091f (prio 1, i/o): 0xe0010010910
    00000e0010013ff8-00000e0010013ffb (prio 1, i/o): 0xe0010013ff8
    00000e0040000000-00000e007fffffff (prio 0, i/o): alias pcisubmem1 @system 0000000040000000-000000007fffffff
    00000e0040000160-00000e0040000167 (prio 1, i/o): 0xe0040000160
    00000efdfb000000-00000efdfb0000ff (prio 0, i/o): ls3a_intctl
    00000efdfb000044-00000efdfb000047 (prio 1, i/o): 0xefdfb000044
    00000efdfb000178-00000efdfb00017b (prio 1, i/o): 0xefdfb000178
    00000efdfc000000-00000efdfc00ffff (prio 0, i/o): system
      00000efdfc000000-00000efdfc000000 (prio 1, i/o): alias pci_bridge_io @pci_bridge_io 0000000000000000-0000000000000000
      00000efdfc000000-00000efdfc000000 (prio 1, i/o): alias pci_bridge_io @pci_bridge_io 0000000000000000-0000000000000000
    00000efdfe0001f4-00000efdfe0001f7 (prio 1, i/o): 0xefdfe0001f4

address-space: ls3a7a axi memory
  0000000000000000-ffffffffffffffff (prio 0, i/o): ls3a7a axi
    0000000000000000-000000000fffffff (prio 0, i/o): alias dmalowmem @mips_r4k.ram0 0000000000000000-000000000fffffff
    0000000080000000-00000000bfffffff (prio 0, i/o): alias dmamem @mips_r4k.ram0 0000000000000000-000000003fffffff

address-space: pcie memory
  0000000000000000-7ffffffffffffffe (prio 0, i/o): system
    0000000000000000-0000000000000000 (prio 1, i/o): alias pci_bridge_mem @pci_bridge_pci 0000000000000000-0000000000000000
    0000000000000000-0000000000000000 (prio 1, i/o): alias pci_bridge_pref_mem @pci_bridge_pci 0000000000000000-0000000000000000
    0000000000000000-0000000000000000 (prio 1, i/o): alias pci_bridge_mem @pci_bridge_pci 0000000000000000-0000000000000000
    0000000000000000-0000000000000000 (prio 1, i/o): alias pci_bridge_pref_mem @pci_bridge_pci 0000000000000000-0000000000000000
    0000000000000000-000000000fffffff (prio 0, i/o): alias ddrlowmem @mips_r4k.ram0 0000000000000000-000000000fffffff
    0000000080000000-00000000bfffffff (prio 0, i/o): alias ddrmem @mips_r4k.ram0 0000000000000000-000000003fffffff

address-space: pcie io
  00000efdfc000000-00000efdfc00ffff (prio 0, i/o): system
    00000efdfc000000-00000efdfc000000 (prio 1, i/o): alias pci_bridge_io @pci_bridge_io 0000000000000000-0000000000000000
    00000efdfc000000-00000efdfc000000 (prio 1, i/o): alias pci_bridge_io @pci_bridge_io 0000000000000000-0000000000000000

address-space: LS7A_Bonito
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-7ffffffffffffffe (prio 0, i/o): alias bus master @system 0000000000000000-7ffffffffffffffe [disabled]

address-space: LS7A_Bonito
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-7ffffffffffffffe (prio 0, i/o): alias bus master @system 0000000000000000-7ffffffffffffffe [disabled]

address-space: pci-synopgmac
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-7ffffffffffffffe (prio 0, i/o): alias bus master @system 0000000000000000-7ffffffffffffffe [disabled]

address-space: pci-synopgmac
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-7ffffffffffffffe (prio 0, i/o): alias bus master @system 0000000000000000-7ffffffffffffffe [disabled]

address-space: pci-ohci
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-7ffffffffffffffe (prio 0, i/o): alias bus master @system 0000000000000000-7ffffffffffffffe [disabled]

address-space: usb-ehci
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-7ffffffffffffffe (prio 0, i/o): alias bus master @system 0000000000000000-7ffffffffffffffe [disabled]

address-space: ls2k-ahci
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-7ffffffffffffffe (prio 0, i/o): alias bus master @system 0000000000000000-7ffffffffffffffe [disabled]

memory-region: mips_r4k.ram0
  0000000080000000-00000000bfffffff (prio 0, ram): mips_r4k.ram0

memory-region: system
  0000000000000000-7ffffffffffffffe (prio 0, i/o): system
    0000000000000000-0000000000000000 (prio 1, i/o): alias pci_bridge_mem @pci_bridge_pci 0000000000000000-0000000000000000
    0000000000000000-0000000000000000 (prio 1, i/o): alias pci_bridge_pref_mem @pci_bridge_pci 0000000000000000-0000000000000000
    0000000000000000-0000000000000000 (prio 1, i/o): alias pci_bridge_mem @pci_bridge_pci 0000000000000000-0000000000000000
    0000000000000000-0000000000000000 (prio 1, i/o): alias pci_bridge_pref_mem @pci_bridge_pci 0000000000000000-0000000000000000
    0000000000000000-000000000fffffff (prio 0, i/o): alias ddrlowmem @mips_r4k.ram0 0000000000000000-000000000fffffff
    0000000080000000-00000000bfffffff (prio 0, i/o): alias ddrmem @mips_r4k.ram0 0000000000000000-000000003fffffff

memory-region: pci_bridge_io
  0000000000000000-00000000fffffffe (prio 0, i/o): pci_bridge_io

memory-region: pci_bridge_io
  0000000000000000-00000000fffffffe (prio 0, i/o): pci_bridge_io

memory-region: pci_bridge_pci
  0000000000000000-ffffffffffffffff (prio 0, i/o): pci_bridge_pci

memory-region: pci_bridge_pci
  0000000000000000-ffffffffffffffff (prio 0, i/o): pci_bridge_pci

```
