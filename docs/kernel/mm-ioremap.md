# ioremap 和 resource 机制

因为内核也是运行在虚拟地址空间上的，而访问设备是需要物理地址，为了将访问设备的物理地址映射到虚拟地址空间中，所以需要 ioremap，当然 pci 访问带来的各种 cache coherency 问题也是需要尽量考虑的:

## ioremap 的调用时间
```txt
#0  __ioremap_caller (phys_addr=4263518208, size=4096, pcm=pcm@entry=_PAGE_CACHE_MODE_UC_MINUS, caller=0xffffffff8174efd4 <vp_modern_map_capability+308>, encrypted=<optimized out>) at arch/x86/mm/ioremap.c:179
#1  0xffffffff810f485e in ioremap (phys_addr=<optimized out>, size=<optimized out>) at arch/x86/mm/ioremap.c:350
#2  0xffffffff816889b7 in pci_iomap_range (dev=dev@entry=0xffff888100c70000, bar=<optimized out>, offset=<optimized out>, maxlen=<optimized out>) at lib/pci_iomap.c:46
#3  0xffffffff8174efd4 in vp_modern_map_capability (mdev=mdev@entry=0xffff888101003b48, off=off@entry=96, minlen=minlen@entry=0, align=align@entry=4, start=start@entry=0, size=size@entry=4096, len=0xffff888101003b80, pa=0x0 <fixed_percpu_data>) at drivers/virtio/virtio_pci_modern_dev.c:94
#4  0xffffffff8174f96a in vp_modern_probe (mdev=mdev@entry=0xffff888101003b48) at drivers/virtio/virtio_pci_modern_dev.c:331
#5  0xffffffff8175057a in virtio_pci_modern_probe (vp_dev=vp_dev@entry=0xffff888101003800) at drivers/virtio/virtio_pci_modern.c:533
#6  0xffffffff817509f5 in virtio_pci_probe (pci_dev=0xffff888100c70000, id=<optimized out>) at drivers/virtio/virtio_pci_common.c:551
#7  0xffffffff816c94aa in local_pci_probe (_ddi=_ddi@entry=0xffffc9000005fd60) at drivers/pci/pci-driver.c:324
```

使用 x86 为例子：
- ioremap : 将
  -  `__ioremap_caller`
    - `__ioremap_check_mem` : 检测需要映射物理地址，通过 resource 机制
    - get_vm_area_caller : 在虚拟地址空间分配一块位置
    - ioremap_page_range : 将虚拟地址和物理地址链接起来


## `__iomem` 是做什么的
- 通过编译器来检查
  - https://stackoverflow.com/questions/40987780/where-can-i-find-the-reference-of-gcc-extended-attribute
- [ioremap() and memremap()](https://lwn.net/Articles/653585/)

## resource 的初始化
初始化时间
- setup_arch
  - e820__reserve_resources : 内存，ACPI 相关的 resource 初始化

ioapic 的初始化:
```txt
#0  insert_resource (parent=0xffffffff82a61160 <iomem_resource>, new=new@entry=0xffff88823fff1880) at kernel/resource.c:857
#1  0xffffffff833525a8 in ioapic_insert_resources () at arch/x86/kernel/apic/io_apic.c:2751
#2  0xffffffff83392b6c in pcibios_resource_survey () at arch/x86/pci/i386.c:408
#3  0xffffffff833944ec in pcibios_init () at arch/x86/pci/common.c:509
#4  0xffffffff83393929 in pci_subsys_init () at arch/x86/pci/legacy.c:73
#5  0xffffffff81000e7c in do_one_initcall (fn=0xffffffff833938cf <pci_subsys_init>) at init/main.c:1303
#6  0xffffffff8333b4c7 in do_initcall_level (command_line=0xffff888004438900 "root", level=4) at init/main.c:1376
#7  do_initcalls () at init/main.c:1392
#8  do_basic_setup () at init/main.c:1411
#9  kernel_init_freeable () at init/main.c:1631
#10 0xffffffff81faff01 in kernel_init (unused=<optimized out>) at init/main.c:1519
#11 0xffffffff81001a6f in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

但是顶层的 PCI 设备什么时候注册的，我没有找到:
```txt
c0000000-febfffff : PCI Bus 0000:00
```

## resource 注册申请

```txt
#0  __request_region (parent=0xffffffff82a611a0 <ioport_resource>, start=744, n=8, name=name@entry=0xffffffff82832361 "serial", flags=flags@entry=0) at kernel/resource.c:1220
#1  0xffffffff8177da7f in serial8250_request_std_resource (up=up@entry=0xffffffff835502a0 <serial8250_ports+2304>) at drivers/tty/serial/8250/8250_port.c:2997
#2  0xffffffff8177f608 in serial8250_config_port (port=0xffffffff835502a0 <serial8250_ports+2304>, flags=3) at drivers/tty/serial/8250/8250_port.c:3193
#3  0xffffffff8177cadf in univ8250_config_port (port=0xffffffff835502a0 <serial8250_ports+2304>, flags=3) at drivers/tty/serial/8250/8250_core.c:444
#4  0xffffffff8177b519 in uart_configure_port (drv=0xffffffff82c10b80 <serial8250_reg>, port=0xffffffff835502a0 <serial8250_ports+2304>, state=0xffff8880055f0528) at drivers/tty/serial/serial_core.c:2547
#5  uart_add_one_port (drv=drv@entry=0xffffffff82c10b80 <serial8250_reg>, uport=uport@entry=0xffffffff835502a0 <serial8250_ports+2304>) at drivers/tty/serial/serial_core.c:3096
#6  0xffffffff8337b0ba in serial8250_register_ports (drv=0xffffffff82c10b80 <serial8250_reg>, dev=0xffff888004545810) at drivers/tty/serial/8250/8250_core.c:572
#7  serial8250_init () at drivers/tty/serial/8250/8250_core.c:1205
#8  0xffffffff81000e7c in do_one_initcall (fn=0xffffffff8337af91 <serial8250_init>) at init/main.c:1303
#9  0xffffffff8333b4c7 in do_initcall_level (command_line=0xffff88800440b900 "root", level=6) at init/main.c:1376
#10 do_initcalls () at init/main.c:1392
#11 do_basic_setup () at init/main.c:1411
#12 kernel_init_freeable () at init/main.c:1631
#13 0xffffffff81faff01 in kernel_init (unused=<optimized out>) at init/main.c:1519
#14 0xffffffff81001a6f in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

其中 PCI 设备 pci_request_selected_regions --->  __request_region_locked

## /proc/iomap 的实现
- 在 kernel/resource.c 中，对于整个 tree resource 中进行 walk

```txt
sudo cat /proc/iomem
[sudo] password for martins3:
00000000-00000fff : Reserved
00001000-0009dfff : System RAM
0009e000-0009efff : Reserved
0009f000-0009ffff : System RAM
000a0000-000fffff : Reserved
  000a0000-000bffff : PCI Bus 0000:00
  00000000-00000000 : PCI Bus 0000:00
  00000000-00000000 : PCI Bus 0000:00
  00000000-00000000 : PCI Bus 0000:00
  000e0000-000effff : PCI Bus 0000:00
  000f0000-000fffff : System ROM
00100000-5ed4b017 : System RAM
  4e000000-55ffffff : Crash kernel
5ed4b018-5ed5a057 : System RAM
5ed5a058-63e5dfff : System RAM
63e5e000-63e5efff : Reserved
63e5f000-63e9bfff : System RAM
63e9c000-63e9cfff : Reserved
63e9d000-6b60bfff : System RAM
6b60c000-6b9d8fff : Reserved
6b9d9000-6dc60fff : System RAM
6dc61000-6dc61fff : Reserved
6dc62000-70560fff : System RAM
70561000-74260fff : Reserved
74261000-744b0fff : ACPI Tables
744b1000-7471efff : ACPI Non-volatile Storage
7471f000-75ffefff : Reserved
75fff000-75ffffff : System RAM
76000000-79ffffff : Reserved
7a400000-7a7fffff : Reserved
7b000000-807fffff : Reserved
  7c800000-807fffff : Graphics Stolen Memory
80800000-bfffffff : PCI Bus 0000:00
  80800000-80800fff : 0000:00:1f.5
    80800000-80800fff : 0000:00:1f.5 0000:00:1f.5
  82000000-83ffffff : 0000:00:0e.0
    82000000-83ffffff : VMD MEMBAR1
  84000000-849fffff : PCI Bus 0000:03
  84a00000-84bfffff : PCI Bus 0000:04
    84a00000-84afffff : 0000:04:00.0
      84a00000-84afffff : igc
    84b00000-84b03fff : 0000:04:00.0
      84b00000-84b03fff : igc
  84c00000-84cfffff : PCI Bus 0000:02
    84c00000-84c03fff : 0000:02:00.0
      84c00000-84c03fff : nvme
    84c04000-84c0ffff : 0000:02:00.0
  84d00000-84dfffff : PCI Bus 0000:01
    84d00000-84d1ffff : 0000:01:00.0
    84d20000-84d2ffff : 0000:01:00.0
      84d20000-84d2ffff : nvme
    84d30000-84d33fff : 0000:01:00.0
      84d30000-84d33fff : nvme
    84d34000-84db3fff : 0000:01:00.0
  84e00000-84e01fff : 0000:00:17.0
    84e00000-84e01fff : ahci
  84e02000-84e027ff : 0000:00:17.0
    84e02000-84e027ff : ahci
  84e03000-84e030ff : 0000:00:17.0
    84e03000-84e030ff : ahci
c0000000-cfffffff : PCI MMCONFIG 0000 [bus 00-ff]
  c0000000-cfffffff : Reserved
e0690000-e069ffff : INTC1056:00
  e0690000-e069ffff : INTC1056:00 INTC1056:00
e06a0000-e06affff : INTC1056:00
  e06a0000-e06affff : INTC1056:00 INTC1056:00
e06b0000-e06bffff : INTC1056:00
  e06b0000-e06bffff : INTC1056:00 INTC1056:00
e06d0000-e06dffff : INTC1056:00
  e06d0000-e06dffff : INTC1056:00 INTC1056:00
e06e0000-e06effff : INTC1056:00
  e06e0000-e06effff : INTC1056:00 INTC1056:00
fe000000-fe010fff : Reserved
fec00000-fec00fff : Reserved
  fec00000-fec003ff : IOAPIC 0
fed00000-fed00fff : Reserved
  fed00000-fed003ff : HPET 0
    fed00000-fed003ff : PNP0103:00
fed20000-fed7ffff : Reserved
  fed40000-fed44fff : MSFT0101:00
    fed40000-fed44fff : MSFT0101:00
fed90000-fed90fff : dmar0
fed91000-fed91fff : dmar1
feda0000-feda0fff : pnp 00:04
feda1000-feda1fff : pnp 00:04
fedc0000-fedc7fff : pnp 00:04
fee00000-fee00fff : Local APIC
  fee00000-fee00fff : Reserved
ff000000-ffffffff : Reserved
100000000-107f7fffff : System RAM
  d3f000000-d3fc01a87 : Kernel code
  d3fe00000-d40587fff : Kernel rodata
  d40600000-d4083607f : Kernel data
  d40da4000-d40ffffff : Kernel bss
107f800000-107fffffff : RAM buffer
4000000000-7fffffffff : PCI Bus 0000:00
  4000000000-400fffffff : 0000:00:02.0
  4010000000-4016ffffff : 0000:00:02.0
  4017000000-40171fffff : PCI Bus 0000:03
  4017200000-4017200fff : 0000:00:15.0
    4017200000-40172001ff : lpss_dev
      4017200000-40172001ff : i2c_designware.0 lpss_dev
    4017200200-40172002ff : lpss_priv
    4017200800-4017200fff : idma64.0
      4017200800-4017200fff : idma64.0 idma64.0
  4017201000-4017201fff : 0000:00:15.1
    4017201000-40172011ff : lpss_dev
      4017201000-40172011ff : i2c_designware.1 lpss_dev
    4017201200-40172012ff : lpss_priv
    4017201800-4017201fff : idma64.1
      4017201800-4017201fff : idma64.1 idma64.1
  4017202000-4017202fff : 0000:00:15.2
    4017202000-40172021ff : lpss_dev
      4017202000-40172021ff : i2c_designware.2 lpss_dev
    4017202200-40172022ff : lpss_priv
    4017202800-4017202fff : idma64.2
      4017202800-4017202fff : idma64.2 idma64.2
  4020000000-40ffffffff : 0000:00:02.0
  6000000000-6001ffffff : 0000:00:0e.0
  6002000000-6002ffffff : 0000:00:02.0
  6003000000-60030fffff : 0000:00:1f.3
  6003100000-60031fffff : 0000:00:0e.0
    6003102000-60031fffff : VMD MEMBAR2
  6003200000-600320ffff : 0000:00:14.0
    6003200000-600320ffff : xhci-hcd
  6003210000-6003217fff : 0000:00:0a.0
    6003214000-6003214d9f : telem0
    6003214da0-6003214edf : telem1
    60032164d8-60032164e7 : intel_vsec.telemetry.0
    60032164e8-60032164f7 : intel_vsec.telemetry.0
    60032164f8-6003216507 : intel_vsec.telemetry.0
    6003216508-6003216517 : intel_vsec.telemetry.0
  6003218000-600321bfff : 0000:00:1f.3
    6003218000-600321bfff : ICH HD audio
  600321c000-600321ffff : 0000:00:14.3
    600321c000-600321ffff : iwlwifi
  6003220000-6003223fff : 0000:00:14.2
  6003224000-60032240ff : 0000:00:1f.4
  6003225000-6003225fff : 0000:00:16.0
    6003225000-6003225fff : mei_me
  6003229000-6003229fff : 0000:00:14.2
```

## 补充一下，QEMU 中观测到的 info mtree 和 guest 中的 /proc/mtree
