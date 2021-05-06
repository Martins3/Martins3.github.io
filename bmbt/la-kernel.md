# Find out

1. kernel/
traps.c : 建立各种中断处理函数
setup.c : 内核参数 和 中断
perf_event.c                                 146             79            644
ptrace.c                                      96             51            600
signal.c                                     120             96            594
fpu.S                                         56            130            587


2. la64

3. mem

question:
- [ ] 内存的如何收集的
- [ ] dtb
- [ ] relocate.c 是怎么处理的?
- [ ] /sys/firmware
- [ ] https://wiki.gentoo.org/wiki/IOMMU_SWIOTLB
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/env.c : BIOS 参数解析
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/init.c : PROM ???
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/irq.c : 存在一些 vi 中断等东西
- [ ] MSI 是怎么处理的 ?

## 启动日志 
```
[    0.000000] ACPI: DSDT 0x00000000FD2EE000 001A9C (v02 LGSON  TP-R00   00000476 INTL 20160527)
[    0.000000] ACPI: FACS 0x00000000FD2F1000 000040
[    0.000000] ACPI: APIC 0x00000000FD2F2000 00005E (v01 LOONGS LOONGSON 00000001 LIUX 00000000)
[    0.000000] ACPI: SRAT 0x00000000FD2ED000 0000C0 (v02 LOONGS LOONGSON 00000002 LIUX 01000013)
[    0.000000] ACPI: SLIT 0x00000000FD2EC000 00002D (v01 LOONGS LOONGSON 00000002 LIUX 01000013)
[    0.000000] PCH_PIC[0]: pch_pic_id 0, version 0, address 0xe0010000000, IRQ 64-127
[    0.000000] SRAT: PXM 0 -> CPU 0x00 -> Node 0
[    0.000000] SRAT: PXM 0 -> CPU 0x01 -> Node 0
[    0.000000] SRAT: PXM 0 -> CPU 0x02 -> Node 0
[    0.000000] SRAT: PXM 0 -> CPU 0x03 -> Node 0
[    0.000000] SRAT: Node 0 PXM 0 [mem 0x00000000-0x0fffffff]
[    0.000000] SRAT: Node 0 PXM 0 [mem 0x90000000-0x47fffffff]
[    0.000000] Debug: node_id:0, mem_start:0x200000, mem_size:0xee00000 Bytes
[    0.000000]        start_pfn:0x80, end_pfn:0x3c00, num_physpages:0x3b80
[    0.000000] Debug: mem_type:2, mem_start:0xf000000, mem_size:0x1000000 Bytes
[    0.000000] Debug: mem_type:2, mem_start:0x90000000, mem_size:0x200000 Bytes
[    0.000000] Debug: node_id:0, mem_start:0x90200000, mem_size:0x2fe00000 Bytes
[    0.000000]        start_pfn:0x24080, end_pfn:0x30000, num_physpages:0xfb00
[    0.000000] Debug: node_id:0, mem_start:0xff000000, mem_size:0x381000000 Bytes
[    0.000000]        start_pfn:0x3fc00, end_pfn:0x120000, num_physpages:0xeff00
[    0.000000] Debug: node_id:0, mem_start:0xc0020000, mem_size:0x3d108000 Bytes
[    0.000000]        start_pfn:0x30008, end_pfn:0x3f44a, num_physpages:0xff342
[    0.000000] Debug: node_id:0, mem_start:0xfd134000, mem_size:0x1b8000 Bytes
[    0.000000]        start_pfn:0x3f44d, end_pfn:0x3f4bb, num_physpages:0xff3b0
[    0.000000] Debug: node_id:0, mem_start:0xfd388000, mem_size:0x1080000 Bytes
[    0.000000]        start_pfn:0x3f4e2, end_pfn:0x3f902, num_physpages:0xff7d0
[    0.000000] Debug: node_id:0, mem_start:0xfe460000, mem_size:0xba0000 Bytes
[    0.000000]        start_pfn:0x3f918, end_pfn:0x3fc00, num_physpages:0xffab8
[    0.000000] Debug: mem_type:2, mem_start:0xfd128000, mem_size:0xb000 Bytes
[    0.000000] Debug: mem_type:2, mem_start:0xfd2f5000, mem_size:0x93000 Bytes
[    0.000000] Debug: mem_type:2, mem_start:0xfe408000, mem_size:0x58000 Bytes
[    0.000000] Node0's addrspace_offset is 0x0
[    0.000000] Node0: start_pfn=0x80, end_pfn=0x120000
[    0.000000] SMBIOS 3.2.0 present.
[    0.000000] DMI: Loongson Loongson-LS3A5000-7A1000-1w-V0.1-CRB/Loongson-LS3A5000-7A1000-1w-EVB-V1.21, BIOS Loongson-UDK2018-V2.0.04073-beta2 03/
[    0.000000] CpuClock = 2300000000
[    0.000000] The BIOS Version: Loongson-UDK2018-V2.0.04073-beta2
[    0.000000] irq: no irq domain found for <no-node> !
[    0.000000] CPU0 revision is: 0014c010 (Loongson-64bit)
[    0.000000] FPU0 revision is: 00000000
[    0.000000] Initial ramdisk at: 0x90000000fbaa0000 (9866605 bytes)
[    0.000000] software IO TLB: mapped [mem 0x023f8000-0x063f8000] (64MB)
[    0.000000] PM: Registered nosave memory: [mem 0x01240000-0x01243fff]
[    0.000000] SMP: Allowing 4 CPUs, 0 hotplug CPUs
[    0.000000] Primary instruction cache 64kB, 4-way, VIPT, linesize 64 bytes.
[    0.000000] Primary data cache 64kB, 4-way, VIPT, no aliases, linesize 64 bytes
[    0.000000] Unified victim cache 256kB 16-way, linesize 64 bytes.
[    0.000000] Unified secondary cache 16384kB 16-way, linesize 64 bytes.
[    0.000000] Zone ranges:
[    0.000000]   DMA32    [mem 0x0000000000200000-0x00000000ffffffff]
[    0.000000]   Normal   [mem 0x0000000100000000-0x000000047fffffff]
[    0.000000] Movable zone start for each node
[    0.000000] Early memory node ranges
[    0.000000]   node   0: [mem 0x0000000000200000-0x000000000effffff]
[    0.000000]   node   0: [mem 0x0000000090200000-0x00000000bfffffff]
[    0.000000]   node   0: [mem 0x00000000c0020000-0x00000000fd127fff]
[    0.000000]   node   0: [mem 0x00000000fd134000-0x00000000fd2ebfff]
[    0.000000]   node   0: [mem 0x00000000fd388000-0x00000000fe407fff]
[    0.000000]   node   0: [mem 0x00000000fe460000-0x000000047fffffff]
[    0.000000] Zeroed struct page in unavailable ranges: 1352 pages
[    0.000000] Initmem setup node 0 [mem 0x0000000000200000-0x000000047fffffff]
[    0.000000] On node 0 totalpages: 1047224
[    0.000000]   DMA32 zone: 507 pages used for memmap
[    0.000000]   DMA32 zone: 0 pages reserved
[    0.000000]   DMA32 zone: 129720 pages, LIFO batch:15
[    0.000000]   Normal zone: 3584 pages used for memmap
[    0.000000]   Normal zone: 917504 pages, LIFO batch:15
[    0.000000] percpu: Embedded 5 pages/cpu s43856 r8192 d29872 u8388608
[    0.000000] pcpu-alloc: s43856 r8192 d29872 u8388608 alloc=1*33554432
[    0.000000] pcpu-alloc: [0] 0 1 2 3
[    0.000000] CPU0 __my_cpu_offset: 476cd0000
[    0.000000] Built 1 zonelists, mobility grouping on.  Total pages: 1043133
[    0.000000] Policy zone: Normal
[    0.000000] Kernel command line: root=UUID=e0289ba3-2285-418a-9068-e98d5800315f ro quiet splash resume=UUID=55c58fde-a5a4-4aee-8cdc-dfc69748ed03 console=ttyS0,115200 rd_start=0x90000000fbaa0000 rd_size=0x968d6d e1000e.InterruptThrottleRate=4,4,4,4
[    0.000000] Memory: 16546768K/16755584K available (13209K kernel code, 618K rwdata, 3388K rodata, 432K init, 17109K bss, 208816K reserved, 0K cma-reserved)
[    0.000000] SLUB: HWalign=64, Order=0-3, MinObjects=0, CPUs=4, Nodes=1
[    0.000000] rcu: Hierarchical RCU implementation.
[    0.000000] rcu:     RCU restricting CPUs from NR_CPUS=64 to nr_cpu_ids=4.
[    0.000000] rcu: Adjusting geometry for rcu_fanout_leaf=16, nr_cpu_ids=4
[    0.000000] NR_IRQS: 320, nr_irqs: 320, preallocated irqs: 16
[    0.000000] Support EXT interrupt.
[    0.000000] Setting up vectored interrupts
[    0.000000] stable clock event device register
[    0.000000] clocksource: stable counter: mask: 0xffffffffffffffff max_cycles: 0x171024e7e0, max_idle_ns: 440795205315 ns
[    0.000002] sched_clock: 64 bits at 100MHz, resolution 10ns, wraps every 4398046511100ns
[    0.000003] stable counter clock source device register
[    0.000059] Console: colour dummy device 80x25
[    0.000083] ACPI: Core revision 20180810
[    0.000767] Calibrating delay loop... 199.68 BogoMIPS (lpj=399360)
[    0.031986] pid_max: default: 32768 minimum: 301
[    0.032014] Security Framework initialized
[    0.032019] AppArmor: AppArmor disabled by boot time parameter
[    0.033172] Dentry cache hash table entries: 2097152 (order: 10, 16777216 bytes)
[    0.033773] Inode-cache hash table entries: 1048576 (order: 9, 8388608 bytes)
[    0.033806] Mount-cache hash table entries: 32768 (order: 4, 262144 bytes)
[    0.033826] Mountpoint-cache hash table entries: 32768 (order: 4, 262144 bytes)
[    0.034294] Performance counters: loongarch/loongson64 PMU enabled, 4 64-bit counters available to each CPU.
[    0.034324] rcu: Hierarchical SRCU implementation.
[    0.034444] smp: Bringing up secondary CPUs ...
[    0.034527] Booting CPU#1...
[    0.034656] Generic 64-bit Loongson Processor probed
[    0.034660] Primary instruction cache 64kB, 4-way, VIPT, linesize 64 bytes.
[    0.034662] Primary data cache 64kB, 4-way, VIPT, no aliases, linesize 64 bytes
[    0.034663] Unified victim cache 256kB 16-way, linesize 64 bytes.
[    0.034664] Unified secondary cache 16384kB 16-way, linesize 64 bytes.
[    0.034666] CPU1 __my_cpu_offset: 4774d0000
[    0.034705] CPU1 revision is: 0014c010 (Loongson-64bit)
[    0.034705] FPU1 revision is: 00000000
[    0.063997] CPU#1 finished
[    0.064110] Booting CPU#2...
[    0.064113] Generic 64-bit Loongson Processor probed
[    0.064117] Primary instruction cache 64kB, 4-way, VIPT, linesize 64 bytes.
[    0.064118] Primary data cache 64kB, 4-way, VIPT, no aliases, linesize 64 bytes
[    0.064119] Unified victim cache 256kB 16-way, linesize 64 bytes.
[    0.064120] Unified secondary cache 16384kB 16-way, linesize 64 bytes.
[    0.064122] CPU2 __my_cpu_offset: 477cd0000
[    0.064125] CPU2 revision is: 0014c010 (Loongson-64bit)
[    0.064126] FPU2 revision is: 00000000
[    0.095994] CPU#2 finished
[    0.096094] Booting CPU#3...
[    0.096097] Generic 64-bit Loongson Processor probed
[    0.096101] Primary instruction cache 64kB, 4-way, VIPT, linesize 64 bytes.
[    0.096102] Primary data cache 64kB, 4-way, VIPT, no aliases, linesize 64 bytes
[    0.096103] Unified victim cache 256kB 16-way, linesize 64 bytes.
[    0.096104] Unified secondary cache 16384kB 16-way, linesize 64 bytes.
[    0.096106] CPU3 __my_cpu_offset: 4784d0000
[    0.096108] CPU3 revision is: 0014c010 (Loongson-64bit)
[    0.096109] FPU3 revision is: 00000000
[    0.127995] CPU#3 finished
[    0.128020] smp: Brought up 1 node, 4 CPUs
[    0.128224] devtmpfs: initialized
[    0.129401] random: get_random_u32 called from bucket_table_alloc.isra.9+0x94/0x280 with crng_init=0
[    0.129572] clocksource: jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 7645041785100000 ns
[    0.129578] futex hash table entries: 1024 (order: 2, 65536 bytes)
[    0.129632] xor: measuring software checksum speed
[    0.167992]    8regs     : 11728.000 MB/sec
[    0.207992]    8regs_prefetch: 11700.000 MB/sec
[    0.247991]    32regs    : 11716.000 MB/sec
[    0.287992]    32regs_prefetch: 11692.000 MB/sec
[    0.287993] xor: using function: 8regs (11728.000 MB/sec)
[    0.288105] NET: Registered protocol family 16
[    0.288308] audit: initializing netlink subsys (disabled)
[    0.288381] audit: type=2000 audit(1619404245.292:1): state=initialized audit_enabled=0 res=1
[    0.288552] cpuidle: using governor menu
[    0.288637] SE-IRQ=20
[    0.288644] SE-IRQ-low=17
[    0.294049] ACPI: bus type PCI registered
[    0.297929] HugeTLB registered 32.0 MiB page size, pre-allocated 0 pages
[    0.363999] raid6: int64x1  gen()  2826 MB/s
[    0.376111] random: fast init done
[    0.432000] raid6: int64x1  xor()  1556 MB/s
[    0.500006] raid6: int64x2  gen()  3955 MB/s
[    0.568002] raid6: int64x2  xor()  2152 MB/s
[    0.636006] raid6: int64x4  gen()  3839 MB/s
[    0.704004] raid6: int64x4  xor()  2296 MB/s
[    0.772012] raid6: int64x8  gen()  2687 MB/s
[    0.840000] raid6: int64x8  xor()  2308 MB/s
[    0.840001] raid6: using algorithm int64x2 gen() 3955 MB/s
[    0.840002] raid6: .... xor() 2152 MB/s, rmw enabled
[    0.840003] raid6: using intx1 recovery algorithm
[    0.840162] ACPI: Added _OSI(Module Device)
[    0.840164] ACPI: Added _OSI(Processor Device)
[    0.840165] ACPI: Added _OSI(3.0 _SCP Extensions)
[    0.840167] ACPI: Added _OSI(Processor Aggregator Device)
[    0.840168] ACPI: Added _OSI(Linux-Dell-Video)
[    0.840170] ACPI: Added _OSI(Linux-Lenovo-NV-HDMI-Audio)
[    0.841599] ACPI: 1 ACPI AML tables successfully acquired and loaded
[    0.846937] ACPI Error: Could not enable GlobalLock event (20180810/evxfevnt-184)
[    0.854297] ACPI Warning: Could not enable fixed event - GlobalLock (1) (20180810/evxface-620)
[    0.854299] ACPI Error: No response from Global Lock hardware, disabling lock (20180810/evglock-60)
[    0.863649] ACPI: Interpreter enabled
[    0.863671] ACPI: (supports S0 S3 S4 S5)
[    0.863672] ACPI: Using IOAPIC for interrupt routing
[    0.863687] Failed to parse MCFG (-19)
[    0.867715] ACPI: Power Resource [PUBS] (on)
[    0.870071] ACPI: PCI Root Bridge [PCI0] (domain 0000 [bus 00-ff])
[    0.870077] acpi PNP0A08:00: _OSC: OS supports [ExtendedConfig ASPM ClockPM Segments MSI]
[    0.870083] acpi PNP0A08:00: _OSC failed (AE_NOT_FOUND); disabling ASPM
[    0.870129] PCI host bridge to bus 0000:00
[    0.870132] pci_bus 0000:00: root bus resource [io  0x4000-0xffff window]
[    0.870133] pci_bus 0000:00: root bus resource [mem 0x40000000-0x7fffffff window]
[    0.870136] pci_bus 0000:00: root bus resource [bus 00-ff]
[    0.870149] pci 0000:00:00.0: [0014:7a00] type 00 class 0x060000
[    0.870250] pci 0000:00:00.1: [0014:7a10] type 00 class 0x000000
[    0.870308] pci 0000:00:03.0: [0014:7a03] type 00 class 0x020000
[    0.870318] pci 0000:00:03.0: reg 0x10: [mem 0x47170000-0x47177fff 64bit]
[    0.870398] pci 0000:00:03.1: [0014:7a03] type 00 class 0x020000
[    0.870406] pci 0000:00:03.1: reg 0x10: [mem 0x47178000-0x4717ffff 64bit]
[    0.870481] pci 0000:00:04.0: [0014:7a24] type 00 class 0x0c0310
[    0.870490] pci 0000:00:04.0: reg 0x10: [mem 0x47180000-0x47187fff 64bit]
[    0.870559] pci 0000:00:04.1: [0014:7a14] type 00 class 0x0c0320
[    0.870567] pci 0000:00:04.1: reg 0x10: [mem 0x47188000-0x4718ffff 64bit]
[    0.870642] pci 0000:00:05.0: [0014:7a24] type 00 class 0x0c0310
[    0.870651] pci 0000:00:05.0: reg 0x10: [mem 0x47190000-0x47197fff 64bit]
[    0.870719] pci 0000:00:05.1: [0014:7a14] type 00 class 0x0c0320
[    0.870727] pci 0000:00:05.1: reg 0x10: [mem 0x47198000-0x4719ffff 64bit]
[    0.870803] pci 0000:00:06.0: [0014:7a15] type 00 class 0x040000
[    0.870811] pci 0000:00:06.0: reg 0x10: [mem 0x47100000-0x4713ffff 64bit]
[    0.870816] pci 0000:00:06.0: reg 0x18: [mem 0x40000000-0x43ffffff 64bit]
[    0.870822] pci 0000:00:06.0: reg 0x20: [mem 0x47140000-0x4714ffff 64bit]
[    0.870875] pci 0000:00:06.1: [0014:7a06] type 00 class 0x030000
[    0.870883] pci 0000:00:06.1: reg 0x10: [mem 0x47150000-0x4715ffff 64bit]
[    0.870950] pci 0000:00:07.0: [0014:7a07] type 00 class 0x040300
[    0.870958] pci 0000:00:07.0: reg 0x10: [mem 0x47160000-0x4716ffff 64bit]
[    0.871023] pci 0000:00:08.0: [0014:7a08] type 00 class 0x010601
[    0.871031] pci 0000:00:08.0: reg 0x10: [mem 0x471a0000-0x471a1fff 64bit]
[    0.871100] pci 0000:00:08.1: [0014:7a08] type 00 class 0x010601
[    0.871108] pci 0000:00:08.1: reg 0x10: [mem 0x471a2000-0x471a3fff 64bit]
[    0.871176] pci 0000:00:08.2: [0014:7a08] type 00 class 0x010601
[    0.871184] pci 0000:00:08.2: reg 0x10: [mem 0x471a4000-0x471a5fff 64bit]
[    0.871258] pci 0000:00:0a.0: [0014:7a09] type 01 class 0x060000
[    0.871269] pci 0000:00:0a.0: reg 0x10: [mem 0x471a6000-0x471a6fff 64bit]
[    0.871297] pci 0000:00:0a.0: supports D1 D2
[    0.871298] pci 0000:00:0a.0: PME# supported from D0 D1 D3hot D3cold
[    0.871377] pci 0000:00:16.0: [0014:7a0b] type 00 class 0x088000
[    0.871386] pci 0000:00:16.0: reg 0x10: [mem 0x471a7000-0x471a7fff 64bit]
[    0.871391] pci 0000:00:16.0: reg 0x18: [mem 0x46000000-0x46ffffff 64bit]
[    0.871450] pci 0000:00:17.0: [0014:7a0c] type 00 class 0x060100
[    0.871543] pci 0000:01:00.0: [1b4b:9215] type 00 class 0x010601
[    0.871562] pci 0000:01:00.0: reg 0x10: [io  0x40028-0x4002f]
[    0.871570] pci 0000:01:00.0: reg 0x14: [io  0x40034-0x40037]
[    0.871578] pci 0000:01:00.0: reg 0x18: [io  0x40020-0x40027]
[    0.871586] pci 0000:01:00.0: reg 0x1c: [io  0x40030-0x40033]
[    0.871595] pci 0000:01:00.0: reg 0x20: [io  0x40000-0x4001f]
[    0.871603] pci 0000:01:00.0: reg 0x24: [mem 0x47000000-0x470007ff]
[    0.871612] pci 0000:01:00.0: reg 0x30: [mem 0xffff0000-0xffffffff pref]
[    0.871653] pci 0000:01:00.0: PME# supported from D3hot
[    0.880010] pci_bus 0000:01: busn_res: [bus 01-ff] end is updated to 01
[    0.880031] pci 0000:00:06.0: BAR 2: assigned [mem 0x40000000-0x43ffffff 64bit]
[    0.880037] pci 0000:00:16.0: BAR 2: assigned [mem 0x44000000-0x44ffffff 64bit]
[    0.880042] pci 0000:00:0a.0: BAR 14: assigned [mem 0x45000000-0x450fffff]
[    0.880044] pci 0000:00:06.0: BAR 0: assigned [mem 0x45100000-0x4513ffff 64bit]
[    0.880048] pci 0000:00:06.0: BAR 4: assigned [mem 0x45140000-0x4514ffff 64bit]
[    0.880052] pci 0000:00:06.1: BAR 0: assigned [mem 0x45150000-0x4515ffff 64bit]
[    0.880056] pci 0000:00:07.0: BAR 0: assigned [mem 0x45160000-0x4516ffff 64bit]
[    0.880061] pci 0000:00:03.0: BAR 0: assigned [mem 0x45170000-0x45177fff 64bit]
[    0.880065] pci 0000:00:03.1: BAR 0: assigned [mem 0x45178000-0x4517ffff 64bit]
[    0.880069] pci 0000:00:04.0: BAR 0: assigned [mem 0x45180000-0x45187fff 64bit]
[    0.880074] pci 0000:00:04.1: BAR 0: assigned [mem 0x45188000-0x4518ffff 64bit]
[    0.880078] pci 0000:00:05.0: BAR 0: assigned [mem 0x45190000-0x45197fff 64bit]
[    0.880082] pci 0000:00:05.1: BAR 0: assigned [mem 0x45198000-0x4519ffff 64bit]
[    0.880087] pci 0000:00:08.0: BAR 0: assigned [mem 0x451a0000-0x451a1fff 64bit]
[    0.880091] pci 0000:00:08.1: BAR 0: assigned [mem 0x451a2000-0x451a3fff 64bit]
[    0.880095] pci 0000:00:08.2: BAR 0: assigned [mem 0x451a4000-0x451a5fff 64bit]
[    0.880100] pci 0000:00:0a.0: BAR 0: assigned [mem 0x451a6000-0x451a6fff 64bit]
[    0.880104] pci 0000:00:0a.0: BAR 13: assigned [io  0x4000-0x4fff]
[    0.880106] pci 0000:00:16.0: BAR 0: assigned [mem 0x451a7000-0x451a7fff 64bit]
[    0.880114] pci 0000:01:00.0: BAR 6: assigned [mem 0x45000000-0x4500ffff pref]
[    0.880115] pci 0000:01:00.0: BAR 5: assigned [mem 0x45010000-0x450107ff]
[    0.880118] pci 0000:01:00.0: BAR 4: assigned [io  0x4000-0x401f]
[    0.880121] pci 0000:01:00.0: BAR 0: assigned [io  0x4020-0x4027]
[    0.880124] pci 0000:01:00.0: BAR 2: assigned [io  0x4028-0x402f]
[    0.880127] pci 0000:01:00.0: BAR 1: assigned [io  0x4030-0x4033]
[    0.880130] pci 0000:01:00.0: BAR 3: assigned [io  0x4034-0x4037]
[    0.880134] pci 0000:00:0a.0: PCI bridge to [bus 01]
[    0.880136] pci 0000:00:0a.0:   bridge window [io  0x4000-0x4fff]
[    0.880139] pci 0000:00:0a.0:   bridge window [mem 0x45000000-0x450fffff]
[    0.880858] pci 0000:00:06.1: vgaarb: setting as boot VGA device
[    0.880861] pci 0000:00:06.1: vgaarb: VGA device added: decodes=io+mem,owns=io+mem,locks=none
[    0.880865] pci 0000:00:06.1: vgaarb: bridge control possible
[    0.880866] vgaarb: loaded
[    0.880933] SCSI subsystem initialized
[    0.880975] libata version 3.00 loaded.
[    0.881088] ls-spi-pci 0000:00:16.0: can't derive routing for PCI INT A
[    0.881090] ls-spi-pci 0000:00:16.0: PCI INT A: no GSI
[    0.881167] ls-spi ls-spi.0: controller is unqueued, this is deprecated
[    0.881248] ACPI: bus type USB registered
[    0.881267] usbcore: registered new interface driver usbfs
[    0.881277] usbcore: registered new interface driver hub
[    0.881286] usbcore: registered new device driver usb
[    0.881542] pps_core: LinuxPPS API ver. 1 registered
[    0.881543] pps_core: Software ver. 5.3.6 - Copyright 2005-2007 Rodolfo Giometti <giometti@linux.it>
[    0.881547] PTP clock support registered
[    0.881564] Registered efivars operations
[    0.881618] Advanced Linux Sound Architecture Driver Initialized.
[    0.881832] clocksource: Switched to clocksource stable counter
[    0.881867] VFS: Disk quotas dquot_6.6.0
[    0.881883] VFS: Dquot-cache hash table entries: 2048 (order 0, 16384 bytes)
[    0.881911] FS-Cache: Loaded
[    0.881947] pnp: PnP ACPI init
[    0.882057] pnp 00:00: Plug and Play ACPI device, IDs PNP0501 (active)
[    0.882087] pnp 00:01: Plug and Play ACPI device, IDs PNP0501 (active)
[    0.882110] pnp 00:02: Plug and Play ACPI device, IDs PNP0501 (active)
[    0.882131] pnp 00:03: Plug and Play ACPI device, IDs PNP0501 (active)
[    0.882154] pnp 00:04: Plug and Play ACPI device, IDs PNP0501 (active)
[    0.882172] pnp: PnP ACPI: found 5 devices
[    0.883556] NET: Registered protocol family 2
[    0.883707] tcp_listen_portaddr_hash hash table entries: 8192 (order: 3, 131072 bytes)
[    0.883739] TCP established hash table entries: 131072 (order: 6, 1048576 bytes)
[    0.883929] TCP bind hash table entries: 65536 (order: 6, 1048576 bytes)
[    0.884098] TCP: Hash tables configured (established 131072 bind 65536)
[    0.884133] UDP hash table entries: 8192 (order: 4, 262144 bytes)
[    0.884169] UDP-Lite hash table entries: 8192 (order: 4, 262144 bytes)
[    0.884242] NET: Registered protocol family 1
[    0.884342] RPC: Registered named UNIX socket transport module.
[    0.884343] RPC: Registered udp transport module.
[    0.884344] RPC: Registered tcp transport module.
[    0.884345] RPC: Registered tcp NFSv4.1 backchannel transport module.
[    0.884553] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884554] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884556] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884557] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884558] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884559] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884560] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884561] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884562] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884563] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884564] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884565] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884567] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884568] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884569] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884570] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884571] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884572] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884573] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884574] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884575] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884576] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884577] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884578] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884579] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884580] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884582] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884583] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884584] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884585] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884586] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884587] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884588] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884589] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884590] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884591] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884592] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884593] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884594] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884595] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884596] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884598] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884599] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884600] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884601] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884602] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884603] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884604] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884605] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884606] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884608] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884609] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884610] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884611] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884612] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884613] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884614] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884616] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884617] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884618] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884619] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884620] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884621] pci 0000:00:04.1: EHCI: unrecognized capability ff
[    0.884622] pci 0000:00:04.1: EHCI: capability loop?
[    0.884797] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884798] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884799] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884800] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884801] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884803] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884804] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884805] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884806] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884807] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884808] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884809] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884811] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884812] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884813] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884814] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884815] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884816] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884817] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884818] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884819] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884821] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884822] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884823] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884824] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884825] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884826] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884827] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884828] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884829] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884831] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884832] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884833] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884834] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884835] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884836] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884837] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884838] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884839] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884841] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884842] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884843] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884844] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884845] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884846] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884847] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884848] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884849] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884850] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884851] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884852] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884853] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884854] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884855] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884856] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884857] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884859] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884860] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884861] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884862] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884863] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884864] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884865] pci 0000:00:05.1: EHCI: unrecognized capability ff
[    0.884866] pci 0000:00:05.1: EHCI: capability loop?
[    0.884888] PCI: CLS 0 bytes, default 64
[    0.884943] Trying to unpack rootfs image as initramfs...
[    1.055171] Freeing initrd memory: 9632K
[    1.055276] Starting KVM with LOONGARCH VZ extensions
[    1.055291] Init KVM Timer with stable timer
[    1.055669] Initialise system trusted keyrings
[    1.055719] workingset: timestamp_bits=40 max_order=20 bucket_order=0
[    1.057089] squashfs: version 4.0 (2009/01/31) Phillip Lougher
[    1.057225] NFS: Registering the id_resolver key type
[    1.057232] Key type id_resolver registered
[    1.057234] Key type id_legacy registered
[    1.057237] nfs4filelayout_init: NFSv4 File Layout Driver Registering...
[    1.057242] Installing knfsd (copyright (C) 1996 okir@monad.swb.de).
[    1.057460] SGI XFS with security attributes, no debug enabled
[    1.057761] 9p: Installing v9fs 9p2000 file system support
[    1.347189] Key type asymmetric registered
[    1.347191] Asymmetric key parser 'x509' registered
[    1.347210] Block layer SCSI generic (bsg) driver version 0.4 loaded (major 250)
[    1.347242] io scheduler noop registered
[    1.347275] io scheduler cfq registered (default)
[    1.347277] io scheduler mq-deadline registered
[    1.347277] io scheduler kyber registered
[    1.347399] ls-pwm LOON0006:00: pwm->clock_frequency=50000000
[    1.347439] ls-pwm LOON0006:01: pwm->clock_frequency=50000000
[    1.347464] ls-pwm LOON0006:02: pwm->clock_frequency=50000000
[    1.347488] ls-pwm LOON0006:03: pwm->clock_frequency=50000000
[    1.347792] shpchp: Standard Hot Plug PCI Controller Driver version: 0.4
[    1.347937] input: Power Button as /devices/LNXSYSTM:00/LNXPWRBN:00/input/input0
[    1.347965] ACPI: Power Button [PWRF]
[    1.348847] Warning: Processor Platform Limit event detected, but not handled.
[    1.348848] Consider compiling CPUfreq support into your kernel.
[    1.349601] Serial: 8250/16550 driver, 16 ports, IRQ sharing enabled
[    1.349638] 00:00: ttyS0 at MMIO 0x1fe001e0 (irq = 27, base_baud = 6250000) is a ST16650
[    1.349654] console [ttyS0] enabled
[    1.349655] bootconsole [early0] disabled
[    1.369811] 00:01: ttyS1 at MMIO 0x10080000 (irq = 28, base_baud = 3125000) is a 16550A
[    1.389978] 00:02: ttyS2 at MMIO 0x10080100 (irq = 28, base_baud = 3125000) is a 16550A
[    1.410117] 00:03: ttyS3 at MMIO 0x10080200 (irq = 28, base_baud = 3125000) is a 16550A
[    1.430256] 00:04: ttyS4 at MMIO 0x10080300 (irq = 28, base_baud = 3125000) is a 16550A
[    1.430743] [drm] loongson kernel modesetting enabled.
[    1.430992] [drm] Get vbios from bios Success.
[    1.430994] [drm] Loongson vbios version 0.1
[    1.431013] [drm]    encoder0(BIOS) i2c:6
[    1.431014] [drm]    connector0:
[    1.431015] [drm]       No EDID
[    1.431016] [drm]      Detect:SHOW
[    1.431017] [drm]    encoder1(BIOS) i2c:7
[    1.431018] [drm]    connector1:
[    1.431018] [drm]       No EDID
[    1.431019] [drm]      Detect:SHOW
[    1.431027] [drm] io: 0x8000000010010000, mmio: 0xe0045150000, size: 0x10000
[    1.431078] [drm] Registered ls7a dc gpio driver
[    1.431118] [TTM] Zone  kernel: Available graphics memory: 8278200 kiB
[    1.431119] [TTM] Zone   dma32: Available graphics memory: 2097152 kiB
[    1.431120] [TTM] Initializing pool allocator
[    1.431126] [TTM] Initializing DMA pool allocator
[    1.431170] [drm] Created dc-i2c0, sda=0, scl=1
[    1.431201] [drm] Register i2c algo-bit adapter [ls_dc_i2c0]
[    1.431203] [drm] Created dc-i2c1, sda=2, scl=3
[    1.431220] [drm] Register i2c algo-bit adapter [ls_dc_i2c1]
[    1.431221] [drm] i2c_bus[2] not use
[    1.431237] [drm] No matching encoder chip 0x0, using default
[    1.431239] [drm] Parse resource:Encoder [0x00-unknown]:i2c6-0x00 irq(-1,-1)
[    1.431240] [drm] config type 2
[    1.431292] [drm] No matching encoder chip 0x0, using default
[    1.431293] [drm] Parse resource:Encoder [0x00-unknown]:i2c7-0x00 irq(-1,-1)
[    1.431294] [drm] config type 2
[    1.461613] i2c i2c-7: unable to read EDID block
[    1.756709] Console: switching to colour frame buffer device 128x48
[    1.833424] loongson-drm 0000:00:06.1: fb0: loongsondrmfb frame buffer device
[    1.833464] [drm] Supports vblank timestamp caching Rev 2 (21.10.2013).
[    1.833465] [drm] No driver support for vblank timestamp query.
[    1.833466] [drm] drm vblank init finished
[    1.833485] [drm] loongson irq initialized
[    1.833490] [drm] Initialized loongson-drm 0.4.0 20180328 for 0000:00:06.1 on minor 0
[    1.835408] brd: module loaded
[    1.836672] loop: module loaded
[    1.836741] megaraid cmm: 2.20.2.7 (Release Date: Sun Jul 16 00:01:03 EST 2006)
[    1.836763] megaraid: 2.20.5.1 (Release Date: Thu Nov 16 15:32:35 EST 2006)
[    1.836777] megasas: 07.706.03.00-rc1
[    1.836799] mpt3sas version 26.100.00.00 loaded
[    1.837104] ahci 0000:00:08.0: version 3.0
[    1.837227] ahci 0000:00:08.0: SSS flag set, parallel bus scan disabled
[    1.837241] ahci 0000:00:08.0: AHCI 0001.0300 32 slots 1 ports 3 Gbps 0x1 impl SATA mode
[    1.837244] ahci 0000:00:08.0: flags: 64bit ncq sntf stag pm led clo only pmp pio slum part ccc apst
[    1.837413] scsi host0: ahci
[    1.837499] ata1: SATA max UDMA/133 abar m8192@0xe00451a0000 port 0xe00451a0100 irq 35
[    1.837626] ahci 0000:00:08.1: SSS flag set, parallel bus scan disabled
[    1.837635] ahci 0000:00:08.1: AHCI 0001.0300 32 slots 1 ports 3 Gbps 0x1 impl SATA mode
[    1.837637] ahci 0000:00:08.1: flags: 64bit ncq sntf stag pm led clo only pmp pio slum part ccc apst
[    1.837776] scsi host1: ahci
[    1.837837] ata2: SATA max UDMA/133 abar m8192@0xe00451a2000 port 0xe00451a2100 irq 36
[    1.837968] ahci 0000:00:08.2: SSS flag set, parallel bus scan disabled
[    1.837977] ahci 0000:00:08.2: AHCI 0001.0300 32 slots 1 ports 3 Gbps 0x1 impl SATA mode
[    1.837979] ahci 0000:00:08.2: flags: 64bit ncq sntf stag pm led clo only pmp pio slum part ccc apst
[    1.838121] scsi host2: ahci
[    1.838173] ata3: SATA max UDMA/133 abar m8192@0xe00451a4000 port 0xe00451a4100 irq 37
[    1.848430] ahci 0000:01:00.0: AHCI 0001.0000 32 slots 4 ports 6 Gbps 0xf impl SATA mode
[    1.848432] ahci 0000:01:00.0: flags: 64bit ncq sntf led only pmp fbs pio slum part sxs
[    1.848814] scsi host3: ahci
[    1.848898] scsi host4: ahci
[    1.848979] scsi host5: ahci
[    1.849059] scsi host6: ahci
[    1.849110] ata4: SATA max UDMA/133 abar m2048@0xe0045010000 port 0xe0045010100 irq 38
[    1.849112] ata5: SATA max UDMA/133 abar m2048@0xe0045010000 port 0xe0045010180 irq 38
[    1.849114] ata6: SATA max UDMA/133 abar m2048@0xe0045010000 port 0xe0045010200 irq 38
[    1.849116] ata7: SATA max UDMA/133 abar m2048@0xe0045010000 port 0xe0045010280 irq 38
[    1.849360] libphy: Fixed MDIO Bus: probed
[    1.849381] e1000: Intel(R) PRO/1000 Network Driver - version 7.3.21-k8-NAPI
[    1.849382] e1000: Copyright (c) 1999-2006 Intel Corporation.
[    1.849398] e1000e: Intel(R) PRO/1000 Network Driver - 3.2.6-k
[    1.849399] e1000e: Copyright(c) 1999 - 2015 Intel Corporation.
[    1.849416] igb: Intel(R) Gigabit Ethernet Network Driver - version 5.4.0-k
[    1.849417] igb: Copyright (c) 2007-2014 Intel Corporation.
[    1.849432] ixgbe: Intel(R) 10 Gigabit PCI Express Network Driver - version 5.1.0-k
[    1.849433] ixgbe: Copyright (c) 1999-2016 Intel Corporation.
[    1.849486] ixgb: Intel(R) PRO/10GbE Network Driver - version 1.0.135-k2-NAPI
[    1.849487] ixgb: Copyright (c) 1999-2008 Intel Corporation.
[    1.849678] stmmaceth 0000:00:03.0: User ID: 0xd1, Synopsys ID: 0x37
[    1.849681] stmmaceth 0000:00:03.0:  DWMAC1000
[    1.849683] stmmaceth 0000:00:03.0: DMA HW capability register supported
[    1.849685] stmmaceth 0000:00:03.0: RX Checksum Offload Engine supported
[    1.849686] stmmaceth 0000:00:03.0: COE Type 2
[    1.849687] stmmaceth 0000:00:03.0: TX Checksum insertion supported
[    1.849689] stmmaceth 0000:00:03.0: Wake-Up On Lan supported
[    1.849690] stmmaceth 0000:00:03.0: Enhanced/Alternate descriptors
[    1.849691] stmmaceth 0000:00:03.0: Enabled extended descriptors
[    1.849692] stmmaceth 0000:00:03.0: Ring mode enabled
[    1.849694] stmmaceth 0000:00:03.0: Enable RX Mitigation via HW Watchdog Timer
[    1.849698] stmmaceth 0000:00:03.0 (unnamed net_device) (uninitialized): device MAC address 00:23:a0:00:29:38
[    1.857937] libphy: stmmac: probed
[    1.857940] mdio_bus stmmac-18:00: attached PHY driver [unbound] (mii_bus:phy_addr=stmmac-18:00, irq=POLL)
[    1.858189] stmmaceth 0000:00:03.1: User ID: 0xd1, Synopsys ID: 0x37
[    1.858191] stmmaceth 0000:00:03.1:  DWMAC1000
[    1.858193] stmmaceth 0000:00:03.1: DMA HW capability register supported
[    1.858194] stmmaceth 0000:00:03.1: RX Checksum Offload Engine supported
[    1.858195] stmmaceth 0000:00:03.1: COE Type 2
[    1.858196] stmmaceth 0000:00:03.1: TX Checksum insertion supported
[    1.858197] stmmaceth 0000:00:03.1: Wake-Up On Lan supported
[    1.858198] stmmaceth 0000:00:03.1: Enhanced/Alternate descriptors
[    1.858199] stmmaceth 0000:00:03.1: Enabled extended descriptors
[    1.858200] stmmaceth 0000:00:03.1: Ring mode enabled
[    1.858201] stmmaceth 0000:00:03.1: Enable RX Mitigation via HW Watchdog Timer
[    1.858204] stmmaceth 0000:00:03.1 (unnamed net_device) (uninitialized): device MAC address 00:23:a0:00:29:39
[    1.865834] libphy: stmmac: probed
[    1.865836] mdio_bus stmmac-19:00: attached PHY driver [unbound] (mii_bus:phy_addr=stmmac-19:00, irq=POLL)
[    1.866008] ehci_hcd: USB 2.0 'Enhanced' Host Controller (EHCI) Driver
[    1.866010] ehci-pci: EHCI PCI platform driver
[    1.866051] ehci-pci 0000:00:04.1: EHCI Host Controller
[    1.866086] ehci-pci 0000:00:04.1: new USB bus registered, assigned bus number 1
[    1.866110] ehci-pci 0000:00:04.1: cache line size of 64 is not supported
[    1.866122] ehci-pci 0000:00:04.1: irq 30, io mem 0xe0045188000
[    1.881842] ehci-pci 0000:00:04.1: USB f.f started, EHCI 1.00
[    1.882013] hub 1-0:1.0: USB hub found
[    1.882021] hub 1-0:1.0: 3 ports detected
[    1.882315] ehci-pci 0000:00:05.1: EHCI Host Controller
[    1.882343] ehci-pci 0000:00:05.1: new USB bus registered, assigned bus number 2
[    1.882359] ehci-pci 0000:00:05.1: cache line size of 64 is not supported
[    1.882370] ehci-pci 0000:00:05.1: irq 32, io mem 0xe0045198000
[    1.897833] ehci-pci 0000:00:05.1: USB f.f started, EHCI 1.00
[    1.897954] hub 2-0:1.0: USB hub found
[    1.897961] hub 2-0:1.0: 3 ports detected
[    1.898188] ehci-platform: EHCI generic platform driver
[    1.898201] ohci_hcd: USB 1.1 'Open' Host Controller (OHCI) Driver
[    1.898203] ohci-pci: OHCI PCI platform driver
[    1.898222] ohci-pci 0000:00:04.0: OHCI PCI host controller
[    1.898251] ohci-pci 0000:00:04.0: new USB bus registered, assigned bus number 3
[    1.898268] ohci-pci 0000:00:04.0: irq 29, io mem 0xe0045180000
[    1.961963] hub 3-0:1.0: USB hub found
[    1.961970] hub 3-0:1.0: 3 ports detected
[    1.962203] ohci-pci 0000:00:05.0: OHCI PCI host controller
[    1.962231] ohci-pci 0000:00:05.0: new USB bus registered, assigned bus number 4
[    1.962246] ohci-pci 0000:00:05.0: irq 31, io mem 0xe0045190000
[    2.025953] hub 4-0:1.0: USB hub found
[    2.025960] hub 4-0:1.0: 3 ports detected
[    2.026177] ohci-platform: OHCI generic platform driver
[    2.026229] i8042: PNP: No PS/2 controller found.
[    2.026280] mousedev: PS/2 mouse device common for all mice
[    2.028265] rtc rtc0: invalid alarm value: 1961-7-31 31:63:55
[    2.028301] ls2x-rtc LOON0001:00: rtc core: registered LOON0001:00 as rtc0
[    2.030052] rtc-efi rtc-efi: rtc core: registered rtc-efi as rtc1
[    2.030064] i2c /dev entries driver
[    2.030335] device-mapper: ioctl: 4.39.0-ioctl (2018-04-03) initialised: dm-devel@redhat.com
[    2.030368] hidraw: raw HID events driver (C) Jiri Kosina
[    2.030391] usbcore: registered new interface driver usbhid
[    2.030392] usbhid: USB HID core driver
[    2.030393] Loongson Hwmon Enter...
[    2.030394] (NULL device *): hwmon_device_register() is deprecated. Please convert the driver to use hwmon_device_register_with_info().
[    2.030412] Initial CPU temperature is 50000 (highest).
[    2.030562] loongson_generic_laptop: Not yet supported Loongson Generic Laptop/All-in-one detected!
[    2.039855] Initializing XFRM netlink socket
[    2.040031] NET: Registered protocol family 10
[    2.040288] Segment Routing with IPv6
[    2.040346] sit: IPv6, IPv4 and MPLS over IPv4 tunneling driver
[    2.040509] NET: Registered protocol family 17
[    2.040515] NET: Registered protocol family 15
[    2.040572] NET: Registered protocol family 21
[    2.040602] 9pnet: Installing 9P2000 support
[    2.040638] NET: Registered protocol family 37
[    2.040648] Key type dns_resolver registered
[    2.040846] Loading compiled-in X.509 certificates
[    2.041209] Btrfs loaded, crc32c=crc32c-generic
[    2.041861] ls2x-rtc LOON0001:00: setting system clock to 2021-04-26 02:30:47 UTC (1619404247)
[    2.151955] ata1: SATA link down (SStatus 0 SControl 300)
[    2.155951] ata3: SATA link down (SStatus 0 SControl 300)
[    2.163974] ata5: SATA link down (SStatus 0 SControl 300)
[    2.163995] ata4: SATA link down (SStatus 0 SControl 300)
[    2.164015] ata6: SATA link down (SStatus 0 SControl 300)
[    2.167965] ata7: SATA link down (SStatus 0 SControl 300)
[    2.313836] ata2: SATA link up 3.0 Gbps (SStatus 123 SControl 300)
[    2.313986] ata2.00: ACPI cmd ef/02:00:00:00:00:a0 (SET FEATURES) succeeded
[    2.313989] ata2.00: ACPI cmd ef/10:03:00:00:00:a0 (SET FEATURES) filtered out
[    2.314066] ata2.00: ATA-11: SATA SSD, 20120301, max UDMA/133
[    2.314068] ata2.00: 250069680 sectors, multi 16: LBA48 NCQ (depth 32), AA
[    2.314184] ata2.00: ACPI cmd ef/02:00:00:00:00:a0 (SET FEATURES) succeeded
[    2.314186] ata2.00: ACPI cmd ef/10:03:00:00:00:a0 (SET FEATURES) filtered out
[    2.314254] ata2.00: configured for UDMA/133
[    2.314387] scsi 1:0:0:0: Direct-Access     ATA      SATA SSD         0301 PQ: 0 ANSI: 5
[    2.314680] sd 1:0:0:0: [sda] 250069680 512-byte logical blocks: (128 GB/119 GiB)
[    2.314689] sd 1:0:0:0: [sda] Write Protect is off
[    2.314691] sd 1:0:0:0: [sda] Mode Sense: 00 3a 00 00
[    2.314700] sd 1:0:0:0: [sda] Write cache: enabled, read cache: enabled, doesn't support DPO or FUA
[    2.314702] sd 1:0:0:0: Attached scsi generic sg0 type 0
[    2.315106]  sda: sda1 sda2
[    2.315282] sd 1:0:0:0: [sda] Attached SCSI disk
[    2.315289] ALSA device list:
[    2.315290]   No soundcards found.
[    2.315411] Freeing unused kernel memory: 432K
[    2.315412] This architecture does not have kernel memory protection.
[    2.315415] Run /init as init process
[    2.327649] systemd[1]: systemd 241 running in system mode. (+PAM +AUDIT +SELINUX +IMA +APPARMOR +SMACK +SYSVINIT +UTMP +LIBCRYPTSETUP +GCRYPT +GNUTLS +ACL +XZ +LZ4 -SECCOMP +BLKID +ELFUTILS +KMOD -IDN2 +IDN -PCRE2 default-hierarchy=hybrid)
[    2.327742] systemd[1]: Detected architecture loongarch64.
[    2.327744] systemd[1]: Running in initial RAM disk.
[    2.328639] systemd[1]: Set hostname to <loongson-pc>.
[    2.364699] systemd[1]: File /lib/systemd/system/systemd-journald.service:12 configures an IP firewall (IPAddressDeny=any), but the local system does not support BPF/cgroup based firewalling.
[    2.364704] systemd[1]: Proceeding WITHOUT firewalling in effect! (This warning is only shown for the first loaded unit using IP firewalling.)
[    2.374319] random: systemd: uninitialized urandom read (16 bytes read)
[    2.374393] systemd[1]: Started Dispatch Password Requests to Console Directory Watch.
[    2.374427] random: systemd: uninitialized urandom read (16 bytes read)
[    2.374535] systemd[1]: Listening on Journal Socket (/dev/log).
[    2.374558] random: systemd: uninitialized urandom read (16 bytes read)
[    2.374615] systemd[1]: Listening on udev Kernel Socket.
[    2.374630] systemd[1]: Reached target Slices.
[    2.433844] usb 4-2: new low-speed USB device number 2 using ohci-pci
[    2.643363] stmmaceth 0000:00:03.0 enp0s3f0: renamed from eth0
[    2.668760] stmmaceth 0000:00:03.1 enp0s3f1: renamed from eth1
[    2.678293] input: Logitech USB Optical Mouse as /devices/pci0000:00/0000:00:05.0/usb4/4-2/4-2:1.0/0003:046D:C077.0001/input/input1
[    2.678632] hid-generic 0003:046D:C077.0001: input,hidraw0: USB HID v1.11 Mouse [Logitech USB Optical Mouse] on usb-0000:00:05.0-2/input0
[    3.197846] usb 4-3: new low-speed USB device number 3 using ohci-pci
[    3.433168] input: SONiX USB Keyboard as /devices/pci0000:00/0000:00:05.0/usb4/4-3/4-3:1.0/0003:0C45:760B.0002/input/input2
[    3.490199] hid-generic 0003:0C45:760B.0002: input,hidraw1: USB HID v1.11 Keyboard [SONiX USB Keyboard] on usb-0000:00:05.0-3/input0
[    3.494209] input: SONiX USB Keyboard Consumer Control as /devices/pci0000:00/0000:00:05.0/usb4/4-3/4-3:1.1/0003:0C45:760B.0003/input/input3
[    3.554032] input: SONiX USB Keyboard System Control as /devices/pci0000:00/0000:00:05.0/usb4/4-3/4-3:1.1/0003:0C45:760B.0003/input/input4
[    3.554105] hid-generic 0003:0C45:760B.0003: input,hidraw2: USB HID v1.11 Device [SONiX USB Keyboard] on usb-0000:00:05.0-3/input1
[    4.179865] EXT4-fs (sda2): mounted filesystem with ordered data mode. Opts: (null)
[    5.083761] systemd-journald[1306]: Received SIGTERM from PID 1 (systemd).
[    5.103915] systemd: 21 output lines suppressed due to ratelimiting
[    5.213136] systemd[1]: systemd 241 running in system mode. (+PAM +AUDIT +SELINUX +IMA +APPARMOR +SMACK +SYSVINIT +UTMP +LIBCRYPTSETUP +GCRYPT +GNUTLS +ACL +XZ +LZ4 -SECCOMP +BLKID +ELFUTILS +KMOD -IDN2 +IDN -PCRE2 default-hierarchy=hybrid)
[    5.213249] systemd[1]: Detected architecture loongarch64.
[    5.213738] systemd[1]: Set hostname to <loongson-pc>.
[    5.273717] random: crng init done
[    5.273722] random: 7 urandom warning(s) missed due to ratelimiting
[    5.299899] systemd[1]: File /lib/systemd/system/systemd-journald.service:12 configures an IP firewall (IPAddressDeny=any), but the local system does not support BPF/cgroup based firewalling.
[    5.299902] systemd[1]: Proceeding WITHOUT firewalling in effect! (This warning is only shown for the first loaded unit using IP firewalling.)
[    5.306662] systemd[1]: /lib/systemd/system/auditd.service:11: PIDFile= references path below legacy directory /var/run/, updating /var/run/auditd.pid → /run/auditd.pid; please update the unit file accordingly.
[    5.401209] systemd[1]: initrd-switch-root.service: Succeeded.
[    5.401632] systemd[1]: Stopped Switch Root.
[    5.418041] systemd[1]: systemd-journald.service: Service has no hold-off time (RestartSec=0), scheduling restart.
[    5.418115] systemd[1]: systemd-journald.service: Scheduled restart job, restart counter is at 1.
[    5.671194] EXT4-fs (sda2): re-mounted. Opts: discard
[    6.290061] systemd-journald[2498]: Received request to flush runtime journal from PID 1
[    6.383091] version 39.2
[    6.396357] ipmi device interface
[    6.416750] IPMI System Interface driver.
[    6.416807] ipmi_si IPI0001:00: ipmi_platform: probing via ACPI
[    6.416862] ipmi_si IPI0001:00: [io  0x0ca2] regsize 1 spacing 1 irq 0
[    6.416865] ipmi_si: Adding ACPI-specified kcs state machine
[    6.416951] ipmi_si: Trying ACPI-specified kcs state machine at i/o address 0xca2, slave address 0x0, irq 0
[    6.417012] ipmi_si IPI0001:00: There appears to be no BMC at this location
[    6.419428] EFI Variables Facility v0.08 2004-May-17
[    6.483101] snd_hda_codec_realtek hdaudioC0D0: autoconfig for ALC269VC: line_outs=1 (0x1b/0x0/0x0/0x0/0x0) type:line
[    6.483106] snd_hda_codec_realtek hdaudioC0D0:    speaker_outs=2 (0x14/0x17/0x0/0x0/0x0)
[    6.483108] snd_hda_codec_realtek hdaudioC0D0:    hp_outs=1 (0x15/0x0/0x0/0x0/0x0)
[    6.483109] snd_hda_codec_realtek hdaudioC0D0:    mono: mono_out=0x0
[    6.483111] snd_hda_codec_realtek hdaudioC0D0:    dig-out=0x1e/0x0
[    6.483112] snd_hda_codec_realtek hdaudioC0D0:    inputs:
[    6.483115] snd_hda_codec_realtek hdaudioC0D0:      Front Mic=0x19
[    6.483117] snd_hda_codec_realtek hdaudioC0D0:      Rear Mic=0x18
[    6.483118] snd_hda_codec_realtek hdaudioC0D0:      Line=0x1a
[    6.607888] input: HDA Digital PCBeep as /devices/pci0000:00/0000:00:07.0/sound/card0/input5
[    6.614956] input: HD-Audio Loongson Front Mic as /devices/pci0000:00/0000:00:07.0/sound/card0/input6
[    6.615016] input: HD-Audio Loongson Rear Mic as /devices/pci0000:00/0000:00:07.0/sound/card0/input7
[    6.615868] input: HD-Audio Loongson Line as /devices/pci0000:00/0000:00:07.0/sound/card0/input8
[    6.615923] input: HD-Audio Loongson Front Line Out as /devices/pci0000:00/0000:00:07.0/sound/card0/input9
[    6.615978] input: HD-Audio Loongson Headphone as /devices/pci0000:00/0000:00:07.0/sound/card0/input10
[    6.912726] Adding 17057776k swap on /dev/sda1.  Priority:-2 extents:1 across:17057776k SSDsc
[    7.521272] IPv6: ADDRCONF(NETDEV_UP): enp0s3f0: link is not ready
[    7.521846] Generic PHY stmmac-18:00: attached PHY driver [Generic PHY] (mii_bus:phy_addr=stmmac-18:00, irq=POLL)
[    7.529905] stmmaceth 0000:00:03.0 enp0s3f0: No Safety Features support found
[    7.529912] stmmaceth 0000:00:03.0 enp0s3f0: IEEE 1588-2008 Advanced Timestamp supported
[    7.530013] stmmaceth 0000:00:03.0 enp0s3f0: registered PTP clock
[    7.530128] IPv6: ADDRCONF(NETDEV_UP): enp0s3f0: link is not ready
[    7.541790] IPv6: ADDRCONF(NETDEV_UP): enp0s3f1: link is not ready
[    7.542303] Generic PHY stmmac-19:00: attached PHY driver [Generic PHY] (mii_bus:phy_addr=stmmac-19:00, irq=POLL)
[    7.542599] stmmaceth 0000:00:03.1 enp0s3f1: No Safety Features support found
[    7.542604] stmmaceth 0000:00:03.1 enp0s3f1: IEEE 1588-2008 Advanced Timestamp supported
[    7.542677] stmmaceth 0000:00:03.1 enp0s3f1: registered PTP clock
[    7.542756] IPv6: ADDRCONF(NETDEV_UP): enp0s3f1: link is not ready
[    8.546487] stmmaceth 0000:00:03.0 enp0s3f0: Link is Up - 100Mbps/Full - flow control rx/tx
[    8.577077] IPv6: ADDRCONF(NETDEV_CHANGE): enp0s3f0: link becomes ready
```
