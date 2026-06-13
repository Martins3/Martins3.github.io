# memory hotplug

ZONE_MOVABLE 是处理这个，具体需要参考其定义上的注释。

## [ ] 问题
- [ ] 为什么说是要处理 DIMMs 啊?

## 读读文档
- https://elixir.bootlin.com/qemu/latest/source/docs/memory-hotplug.txt

```txt
[   65.786899] acpi PNP0C80:00: Enumeration failure
```
是因为这个导致的:
```txt
CONFIG_ACPI_HOTPLUG_MEMORY=y
```

```txt
object_add memory-backend-ram,id=mem1,size=1G
device_add pc-dimm,id=dimm1,memdev=mem1
```

```txt
object_add memory-backend-ram,id=mem2,size=1G
device_add pc-dimm,id=dimm2,memdev=mem2
```

```txt
object_add memory-backend-ram,id=mem3,size=1G
device_add pc-dimm,id=dimm3,memdev=mem3
```

```txt
object_add memory-backend-ram,id=mem4,size=1G
device_add pc-dimm,id=dimm4,memdev=mem4
```

- [ ] 与之相关的 QEMU 的 acpi 代码是可以调查一下

- 其实，这个非常有意思，因为直接访问的内核:

## 简单分析一下 driver/base
- [ ] 我想知道，插上内存的时候，操作系统应该接受什么中断?
  - UEFI 中断 ?
  - ACPI 中断 ?


- [ ] memory group / memory block 是个什么概念

- memory_group_register_static : 从 acpi 中到来
- memory_block_online
- memory_notify

## hotplug_memory.c


```c
static struct acpi_scan_handler memory_device_handler = {
    .ids = memory_device_ids,
    .attach = acpi_memory_device_add,
    .detach = acpi_memory_device_remove,
    .hotplug = {
        .enabled = true,
    },
};
```

## [Memory Hot(Un)Plug](https://www.kernel.org/doc/html/latest/admin-guide/mm/memory-hotplug.html)

- Memory hot(un)plug in Linux uses the `SPARSEMEM` memory model, which divides the physical memory address space into chunks of the same size: memory sections. The size of a memory section is architecture dependent. For example, x86_64 uses 128 MiB and ppc64 uses 16 MiB.

- Memory sections are combined into chunks referred to as `memory blocks`. The size of a memory block is architecture dependent and corresponds to the smallest granularity that can be hot(un)plugged.

Memory hotplug consists of two phases:
- Adding the memory to Linux
- Onlining memory blocks

- In the first phase, metadata, such as the memory map (“memmap”) and page tables for the direct mapping, is allocated and initialized, and memory blocks are created; the latter also creates sysfs files for managing newly created memory blocks.
- In the second phase, added memory is exposed to the page allocator. After this phase, the memory is visible in memory statistics, such as free and total memory, of the system.

- [ ] 同样的，找到 total memory 统计的位置


- /sys/devices/system/memory/probe 可以手动检查，实际上，并没有看到:

在虚拟机中，只是检查到了:

- auto_online_blocks
- block_size_bytes
- uevent

大约一百多个如下的结构:
memory0
├── node0 -> ../../node/node0
├── online
├── phys_device
├── phys_index
├── power
│   ├── async
│   ├── autosuspend_delay_ms
│   ├── control
│   ├── runtime_active_kids
│   ├── runtime_active_time
│   ├── runtime_enabled
│   ├── runtime_status
│   ├── runtime_suspended_time
│   └── runtime_usage
├── removable
├── state
├── subsystem -> ../../../../bus/memory
├── uevent
└── valid_zones

- online 的时候，可以使用 online_policy，来决定 online 的 memory 是那一个 zone

- /sys/module/memory_hotplug/parameters : 也存在一些控制项目


下面的命令的确是可以的:
```sh
echo 0 > /sys/devices/system/memory/memory<n>/online
```

## 来点 backtrace

```txt
#0  memory_group_register_static (nid=0, max_pages=max_pages@entry=262144) at drivers/base/memory.c:1062
#1  0xffffffff81711e5f in acpi_memory_enable_device (mem_device=0xffff8880074aa8a0) at drivers/acpi/acpi_memhotplug.c:195
#2  acpi_memory_device_add (device=0xffff888003b63800, not_used=<optimized out>) at drivers/acpi/acpi_memhotplug.c:319
#3  0xffffffff816d7505 in acpi_scan_attach_handler (device=0xffff888003b63800) at drivers/acpi/scan.c:2163
#4  acpi_bus_attach (device=0xffff888003b63800, first_pass=first_pass@entry=0x1 <fixed_percpu_data+1>) at drivers/acpi/scan.c:2210
#5  0xffffffff816d986f in acpi_bus_scan (handle=0xffff888003af0120) at drivers/acpi/scan.c:2421
#6  0xffffffff816d9c72 in acpi_scan_device_check (adev=0xffff888003b63800) at drivers/acpi/scan.c:322
#7  acpi_generic_hotplug_event (type=1, adev=0xffff888003b63800) at drivers/acpi/scan.c:364
#8  acpi_device_hotplug (adev=0xffff888003b63800, src=1) at drivers/acpi/scan.c:397
#9  0xffffffff816cf925 in acpi_hotplug_work_fn (work=0xffff88800850d280) at drivers/acpi/osl.c:1162
#10 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff88800384e9c0, work=0xffff88800850d280) at kernel/workqueue.c:2289
#11 0xffffffff811232c8 in worker_thread (__worker=0xffff88800384e9c0) at kernel/workqueue.c:2436
#12 0xffffffff81129c73 in kthread (_create=0xffff888003947040) at kernel/kthread.c:376
#13 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#14 0x0000000000000000 in ?? ()
```

- 这个 backtrace 居然只有调用一次:
```txt
#0  __add_pages (nid=nid@entry=0, pfn=pfn@entry=1048576, nr_pages=nr_pages@entry=262144, params=params@entry=0xffffc9000004bd00) at mm/memory_hotplug.c:305
#1  0xffffffff810f193d in add_pages (nid=nid@entry=0, start_pfn=start_pfn@entry=1048576, nr_pages=nr_pages@entry=262144, params=params@entry=0xffffc9000004bd00) at arch/x86/mm/init_64.c:954
#2  0xffffffff810f19d1 in arch_add_memory (nid=nid@entry=0, start=start@entry=4294967296, size=size@entry=1073741824, params=params@entry=0xffffc9000004bd00) at arch/x86/mm/init_64.c:972
#3  0xffffffff81efe6fc in add_memory_resource (nid=nid@entry=0, res=res@entry=0xffff88800850df40, mhp_flags=mhp_flags@entry=4) at mm/memory_hotplug.c:1378
#4  0xffffffff81efe876 in __add_memory (nid=nid@entry=0, start=<optimized out>, size=<optimized out>, mhp_flags=mhp_flags@entry=4) at mm/memory_hotplug.c:1442
#5  0xffffffff81711ebe in acpi_memory_enable_device (mem_device=0xffff8880074aa8a0) at drivers/acpi/acpi_memhotplug.c:216
#6  acpi_memory_device_add (device=0xffff888003b63800, not_used=<optimized out>) at drivers/acpi/acpi_memhotplug.c:319
#7  0xffffffff816d7505 in acpi_scan_attach_handler (device=0xffff888003b63800) at drivers/acpi/scan.c:2163
#8  acpi_bus_attach (device=0xffff888003b63800, first_pass=first_pass@entry=0x1 <fixed_percpu_data+1>) at drivers/acpi/scan.c:2210
#9  0xffffffff816d986f in acpi_bus_scan (handle=0xffff888003af0120) at drivers/acpi/scan.c:2421
#10 0xffffffff816d9c72 in acpi_scan_device_check (adev=0xffff888003b63800) at drivers/acpi/scan.c:322
#11 acpi_generic_hotplug_event (type=1, adev=0xffff888003b63800) at drivers/acpi/scan.c:364
#12 acpi_device_hotplug (adev=0xffff888003b63800, src=1) at drivers/acpi/scan.c:397
#13 0xffffffff816cf925 in acpi_hotplug_work_fn (work=0xffff88800850d280) at drivers/acpi/osl.c:1162
#14 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff88800384e9c0, work=0xffff88800850d280) at kernel/workqueue.c:2289
#15 0xffffffff811232c8 in worker_thread (__worker=0xffff88800384e9c0) at kernel/workqueue.c:2436
#16 0xffffffff81129c73 in kthread (_create=0xffff888003947040) at kernel/kthread.c:376
#17 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

- 这个 backtrace 会出现 8 次，对应增加了 8 个 mmeory block 的:
```txt
#0  memory_block_action (action=1, mem=0xffff88800922bc00) at drivers/base/memory.c:268
#1  memory_block_change_state (from_state_req=4, to_state=1, mem=0xffff88800922bc00) at drivers/base/memory.c:293
#2  memory_subsys_online (dev=0xffff88800922bc20) at drivers/base/memory.c:315
#3  0xffffffff8194cc0d in device_online (dev=0xffff88800922bc20) at drivers/base/core.c:4048
#4  0xffffffff81968a5d in walk_memory_blocks (start=start@entry=5368709120, size=size@entry=1073741824, arg=arg@entry=0x0 <fixed_percpu_data>, func=func@entry=0xffffffff812e93b0 <online_memory_block>) at drivers/base/memory.c:969
#5  0xffffffff81efe7bd in add_memory_resource (nid=0, nid@entry=1, res=res@entry=0xffff8880091b0180, mhp_flags=mhp_flags@entry=4) at mm/memory_hotplug.c:1421
#6  0xffffffff81efe876 in __add_memory (nid=nid@entry=1, start=<optimized out>, size=<optimized out>, mhp_flags=mhp_flags@entry=4) at mm/memory_hotplug.c:1442
#7  0xffffffff81711f9e in acpi_memory_enable_device (mem_device=0xffff888007fcf500) at drivers/acpi/acpi_memhotplug.c:216
#8  acpi_memory_device_add (device=0xffff888003b64000, not_used=<optimized out>) at drivers/acpi/acpi_memhotplug.c:319
#9  0xffffffff816d75e5 in acpi_scan_attach_handler (device=0xffff888003b64000) at drivers/acpi/scan.c:2163
#10 acpi_bus_attach (device=0xffff888003b64000, first_pass=first_pass@entry=0x1 <fixed_percpu_data+1>) at drivers/acpi/scan.c:2210
#11 0xffffffff816d994f in acpi_bus_scan (handle=0xffff888003af02a0) at drivers/acpi/scan.c:2421
#12 0xffffffff816d9d52 in acpi_scan_device_check (adev=0xffff888003b64000) at drivers/acpi/scan.c:322
#13 acpi_generic_hotplug_event (type=1, adev=0xffff888003b64000) at drivers/acpi/scan.c:364
#14 acpi_device_hotplug (adev=0xffff888003b64000, src=1) at drivers/acpi/scan.c:397
#15 0xffffffff816cfa05 in acpi_hotplug_work_fn (work=0xffff8880093142c0) at drivers/acpi/osl.c:1162
#16 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff888003ab83c0, work=0xffff8880093142c0) at kernel/workqueue.c:2289
#17 0xffffffff811232c8 in worker_thread (__worker=0xffff888003ab83c0) at kernel/workqueue.c:2436
#18 0xffffffff81129c73 in kthread (_create=0xffff888003aaa400) at kernel/kthread.c:376
#19 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#20 0x0000000000000000 in ?? ()
```

### 使用 echo 1 > memory33/online
```txt
#0  memory_block_action (action=1, mem=0xffff888007764800) at drivers/base/memory.c:268
#1  memory_block_change_state (from_state_req=4, to_state=1, mem=0xffff888007764800) at drivers/base/memory.c:293
#2  memory_subsys_online (dev=0xffff888007764820) at drivers/base/memory.c:315
#3  0xffffffff8194cc0d in device_online (dev=dev@entry=0xffff888007764820) at drivers/base/core.c:4048
#4  0xffffffff8194ccba in online_store (dev=0xffff888007764820, attr=<optimized out>, buf=<optimized out>, count=2) at drivers/base/core.c:2545
#5  0xffffffff813e3a0e in kernfs_fop_write_iter (iocb=0xffffc900008abea0, iter=<optimized out>) at fs/kernfs/file.c:354
#6  0xffffffff8134b12c in call_write_iter (iter=0xffffc900008abe78, kio=0xffffc900008abea0, file=0xffff888008090e00) at include/linux/fs.h:2187
#7  new_sync_write (ppos=0xffffc900008abf08, len=2, buf=0x55c5d96924d0 "1\n", filp=0xffff888008090e00) at fs/read_write.c:491
#8  vfs_write (file=file@entry=0xffff888008090e00, buf=buf@entry=0x55c5d96924d0 "1\n", count=count@entry=2, pos=pos@entry=0xffffc900008abf08) at fs/read_write.c:578
#9  0xffffffff8134b4fa in ksys_write (fd=<optimized out>, buf=0x55c5d96924d0 "1\n", count=2) at fs/read_write.c:631
#10 0xffffffff81ef7bdb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900008abf58) at arch/x86/entry/common.c:50
#11 do_syscall_64 (regs=0xffffc900008abf58, nr=<optimized out>) at arch/x86/entry/common.c:80
```
- memory_block_online
- memory_block_offline

真正的工作在 online_pages / offline_pages 中，在其中处理统计相关

## 分析 memory_block_online

- memory_block_online
  - mhp_init_memmap_on_memory ：处理 vmemmap
  - online_pages
    - memory_notify : 通知
    - online_pages_range
    - adjust_present_page_count : 调整 zone 中间的统计数据
    - undo_isolate_page_range ：将新增加的页面释放到 buddy 中
    - init_per_zone_wmark_min ：重新设置 watermark
    - kswapd_run / kcompactd_run : 如果新增加了 node ，那么需要启动对应的 kswapd 和 kcompactd
  - adjust_present_page_count ：处理 vmemmap

- register_memory_notifier 的调用者
```txt
#0  register_memory_notifier (nb=0xffffffff82b74e00 <slab_memory_callback_nb>) at drivers/base/memory.c:95
#1  0xffffffff83321b5a in kmem_cache_init () at mm/slub.c:4830
#2  0xffffffff832f3ff1 in mm_init () at init/main.c:842
#3  start_kernel () at init/main.c:98

#0  register_memory_notifier (nb=0xffffffff82cb9fb0 <migrate_on_reclaim_callback_mem_nb>) at drivers/base/memory.c:95
#1  0xffffffff83321cae in migrate_on_reclaim_init () at mm/migrate.c:2553
#2  0xffffffff8331b232 in init_mm_internals () at mm/vmstat.c:2128
#3  0xffffffff832f43b4 in kernel_init_freeable () at init/main.c:1609
#4  0xffffffff81efca31 in kernel_init (unused=<optimized out>) at init/main.c:1512
#5  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306

#0  register_memory_notifier (nb=0xffffffff82b56a50 <cpuset_track_online_nodes_nb>) at drivers/base/memory.c:95
#1  0xffffffff83318633 in cpuset_init_smp () at kernel/cgroup/cpuset.c:3402
#2  0xffffffff832f43f3 in do_basic_setup () at init/main.c:1400
#3  kernel_init_freeable () at init/main.c:1623
#4  0xffffffff81efca31 in kernel_init (unused=<optimized out>) at init/main.c:1512
#5  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306

#0  register_memory_notifier (nb=0xffffffff82c184a0 <node_memory_callback_nb>) at drivers/base/memory.c:95
#1  0xffffffff83339e9f in node_dev_init () at drivers/base/node.c:1082
#2  0xffffffff8333983c in driver_init () at drivers/base/init.c:40
#3  0xffffffff832f43f8 in do_basic_setup () at init/main.c:1401
#4  kernel_init_freeable () at init/main.c:1623
#5  0xffffffff81efca31 in kernel_init (unused=<optimized out>) at init/main.c:1512
#6  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306

#0  register_memory_notifier (nb=0xffffffff82b72820 <reserve_mem_nb>) at drivers/base/memory.c:95
#1  0xffffffff81f01be5 in init_reserve_notifier () at mm/mmap.c:3742
#2  0xffffffff81000e7f in do_one_initcall (fn=0xffffffff81f01bd9 <init_reserve_notifier>) at init/main.c:1296
#3  0xffffffff832f44b8 in do_initcall_level (command_line=0xffff888003947040 "root", level=4) at init/main.c:1369
#4  do_initcalls () at init/main.c:1385
#5  do_basic_setup () at init/main.c:1404
#6  kernel_init_freeable () at init/main.c:1623
#7  0xffffffff81efca31 in kernel_init (unused=<optimized out>) at init/main.c:1512
#8  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306

#0  register_memory_notifier (nb=0xffffffff82cb9f90 <ksm_memory_callback_mem_nb>) at drivers/base/memory.c:95
#1  0xffffffff8332167d in ksm_init () at mm/ksm.c:3209
#2  0xffffffff81000e7f in do_one_initcall (fn=0xffffffff8332152a <ksm_init>) at init/main.c:1296
#3  0xffffffff832f44b8 in do_initcall_level (command_line=0xffff888003947040 "root", level=4) at init/main.c:1369
#4  do_initcalls () at init/main.c:1385
#5  do_basic_setup () at init/main.c:1404
#6  kernel_init_freeable () at init/main.c:1623
#7  0xffffffff81efca31 in kernel_init (unused=<optimized out>) at init/main.c:1512
#8  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

## movable zone

主要是 movable zone 导致的:

- https://www.cnblogs.com/aspirs/p/12781693.html

## 理解 QEMU 的 maxmem 是做什么的
-m 4G,slots=32,maxmem=32G
