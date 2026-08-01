# memory hotplug

## 先用起来

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


如果没有自动探测的能力，那么可以执行如下脚本:
```sh
for d in /sys/devices/system/memory/*/online ; do
		[ -L "${d%/}" ] && continue
		echo "$d"
		echo 1 | sudo tee "$d"
done
```

## 来点 backtrace

- ??
  - ret_from_fork
    - kthread
      - worker_thread
        - process_one_work
          - acpi_hotplug_work_fn
            - acpi_device_hotplug
              - acpi_generic_hotplug_event
                - acpi_scan_device_check
                  - acpi_bus_scan
                    - acpi_bus_attach
                      - acpi_scan_attach_handler
                        - acpi_memory_device_add
                          - acpi_memory_enable_device
                            - memory_group_register_static

- 这个 backtrace 居然只有调用一次:
```txt
- ret_from_fork
  - kthread
    - worker_thread
      - process_one_work
        - acpi_hotplug_work_fn
          - acpi_device_hotplug
            - acpi_generic_hotplug_event
              - acpi_scan_device_check
                - acpi_bus_scan
                  - acpi_bus_attach
                    - acpi_scan_attach_handler
                      - acpi_memory_device_add
                        - acpi_memory_enable_device
                          - __add_memory
                            - add_memory_resource
                              - arch_add_memory
                                - add_pages
                                  - __add_pages
```

- 这个 backtrace 会出现 8 次，对应增加了 8 个 mmeory block 的:
```txt
- ??
  - ret_from_fork
    - kthread
      - worker_thread
        - process_one_work
          - acpi_hotplug_work_fn
            - acpi_device_hotplug
              - acpi_generic_hotplug_event
                - acpi_scan_device_check
                  - acpi_bus_scan
                    - acpi_bus_attach
                      - acpi_scan_attach_handler
                        - acpi_memory_device_add
                          - acpi_memory_enable_device
                            - __add_memory
                              - add_memory_resource
                                - walk_memory_blocks
                                  - device_online
                                    - memory_subsys_online
                                      - memory_block_change_state
                                        - memory_block_action
```

### 使用 echo 1 > memory33/online
```txt
- do_syscall_64
  - do_syscall_x64
    - ksys_write
      - vfs_write
        - new_sync_write
          - call_write_iter
            - kernfs_fop_write_iter
              - online_store
                - device_online
                  - memory_subsys_online
                    - memory_block_change_state
                      - memory_block_action
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

## 内容没看，但是图画的挺好的
- https://events.static.linuxfound.org/sites/events/files/lcjp13_chen.pdf

## [ ] ARM 环境中测试下

## 参考
https://www.qemu.org/docs/master/specs/acpi_mem_hotplug.html
https://liujunming.top/2022/01/07/The-usage-of-memory-hotplug-under-QEMU-KVM/

## 这个
https://github.com/kata-containers/kata-containers/blob/main/docs/how-to/how-to-hotplug-memory-arm64.md

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
