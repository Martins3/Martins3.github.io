## gdb kernel 的常用命令
<!-- 2480d50a-ceb3-4329-9142-69e015e2b070 -->

仔细看看这个，写的相当好了，不用自己瞎尝试了:
https://docs.kernel.org/process/debugging/gdb-kernel-debugging.html

```txt
$ apropos lx
function lx_clk_core_lookup -- Find struct clk_core by name
function lx_current -- Return current task.
function lx_dentry_name -- Return string of the full path of a dentry.
function lx_device_find_by_bus_name -- Find struct device by bus and name (both strings)
function lx_device_find_by_class_name -- Find struct device by class and name (both strings)
function lx_i_dentry -- Return dentry pointer for inode.
function lx_module -- Find module by name and return the module variable.
function lx_per_cpu -- Return per-cpu variable.
function lx_per_cpu_ptr -- Return per-cpu pointer.
function lx_radix_tree_lookup --  Lookup and return a node from a RadixTree.
function lx_rb_first -- Lookup and return a node from an RBTree
function lx_rb_last -- Lookup and return a node from an RBTree.
function lx_rb_next -- Lookup and return a node from an RBTree.
function lx_rb_prev -- Lookup and return a node from an RBTree.
function lx_task_by_pid -- Find Linux task by PID and return the task_struct variable.
function lx_thread_info -- Calculate Linux thread_info from task variable.
function lx_thread_info_by_pid -- Calculate Linux thread_info from task variable found by pid
lx-clk-summary -- Print clk tree summary
lx-cmdline -- Report the Linux Commandline used in the current kernel.
lx-configdump -- Output kernel config to the filename specified as the command
lx-cpus -- List CPU status arrays
lx-device-list-bus -- Print devices on a bus (or all buses if not specified)
lx-device-list-class -- Print devices in a class (or all classes if not specified)
lx-device-list-tree -- Print a device and its children recursively
lx-dmesg -- Print Linux kernel log buffer.
lx-dump-page-owner -- Dump page owner
lx-fdtdump -- Output Flattened Device Tree header and dump FDT blob to the filename
lx-genpd-summary -- Print genpd summary
lx-getmod-by-textaddr -- Look up loaded kernel module by text address.
lx-interruptlist -- Print /proc/interrupts
lx-iomem -- Identify the IO memory resource locations defined by the kernel
lx-ioports -- Identify the IO port resource locations defined by the kernel
lx-list-check -- Verify a list consistency
lx-lsmod -- List currently loaded modules.
lx-mounts -- Report the VFS mounts of the current process namespace.
lx-page_address -- struct page to linear mapping address
lx-page_to_pfn -- struct page to PFN
lx-page_to_phys -- struct page to physical address
lx-pfn_to_kaddr -- PFN to kernel address
lx-pfn_to_page -- PFN to struct page
lx-ps -- Dump Linux tasks.
lx-slabinfo -- Show slabinfo
lx-slabtrace -- Show specific cache slabtrace
lx-stack_depot_lookup -- Search backtrace by handle
lx-sym_to_pfn -- symbol address to PFN
lx-symbols -- (Re-)load symbols of Linux kernel and currently loaded modules.
lx-timerlist -- Print /proc/timer_list
lx-version -- Report the Linux Version of the current kernel.
lx-virt_to_page -- virtual address to struct page
lx-virt_to_phys -- virtual address to physical address
lx-vmallocinfo -- Show vmallocinfo
```

### info threads
检查每一个 vCPU 的 backtrace
```txt
  Id   Target Id                      Frame
  1    Thread 1.1 (CPU#0 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  2    Thread 1.2 (CPU#1 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  3    Thread 1.3 (CPU#2 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  4    Thread 1.4 (CPU#3 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  5    Thread 1.5 (CPU#4 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  6    Thread 1.6 (CPU#5 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  7    Thread 1.7 (CPU#6 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  8    Thread 1.8 (CPU#7 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  9    Thread 1.9 (CPU#8 [halted ])   0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  10   Thread 1.10 (CPU#9 [halted ])  0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  11   Thread 1.11 (CPU#10 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  12   Thread 1.12 (CPU#11 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  13   Thread 1.13 (CPU#12 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  14   Thread 1.14 (CPU#13 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  15   Thread 1.15 (CPU#14 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  16   Thread 1.16 (CPU#15 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  17   Thread 1.17 (CPU#16 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  18   Thread 1.18 (CPU#17 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  19   Thread 1.19 (CPU#18 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  20   Thread 1.20 (CPU#19 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  21   Thread 1.21 (CPU#20 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  22   Thread 1.22 (CPU#21 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  23   Thread 1.23 (CPU#22 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  24   Thread 1.24 (CPU#23 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  25   Thread 1.25 (CPU#24 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  26   Thread 1.26 (CPU#25 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  27   Thread 1.27 (CPU#26 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  28   Thread 1.28 (CPU#27 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  29   Thread 1.29 (CPU#28 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  30   Thread 1.30 (CPU#29 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
  31   Thread 1.31 (CPU#30 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
* 32   Thread 1.32 (CPU#31 [halted ]) 0xffffffff81cae88f in pv_native_safe_halt () at arch/x86/kernel/paravirt.c:82
```

### 加载模块的符号
错误的办法:
```txt
add-symbol-file drivers/block/virtio_blk.ko
add symbol table from file "drivers/block/virtio_blk.ko"
(y or n) y
Reading symbols from drivers/block/virtio_blk.ko...

$  info variables ^virtio
All variables matching regular expression "^virtio":

File drivers/block/virtio_blk.c:
1663:   static struct virtio_driver virtio_blk;
1226:   static const struct blk_mq_ops virtio_mq_ops;

File drivers/virtio/virtio.c:
438:    static const struct bus_type virtio_bus;
64:     static struct attribute *virtio_dev_attrs[6];
72:     static const struct attribute_group virtio_dev_group;
72:     static const struct attribute_group *virtio_dev_groups[2];
12:     static struct ida virtio_index_ida;

File drivers/virtio/virtio_anchor.c:
16:     bool (*virtio_check_mem_acc_cb)(struct virtio_device *);

File drivers/virtio/virtio_mmio.c:
793:    static const struct acpi_device_id virtio_mmio_acpi_match[2];
522:    static const struct virtio_config_ops virtio_mmio_config_ops;
800:    static struct platform_driver virtio_mmio_driver;
786:    static const struct of_device_id virtio_mmio_match[2];
556:    static const struct dev_pm_ops virtio_mmio_pm_ops;
```
正确的办法，是使用 lx-symbols ，然后就可以观察到这个了:

- ret_from_fork_asm
  - ret_from_fork
    - kthread
      - worker_thread
        - process_scheduled_works
          - process_one_work
            - blk_mq_requeue_work
              - blk_mq_run_hw_queues
                - blk_mq_run_hw_queue
                  - blk_mq_sched_dispatch_requests
                    - __blk_mq_sched_dispatch_requests
                      - blk_mq_dispatch_rq_list
                        - virtio_queue_rq

### current
```txt
$ p $lx_current().pid
$1 = 292
$ p $lx_current().comm
$2 = "kworker/21:1H\000\000"
```

### 打印结构体
```txt
$ set $leftmost = $lx_per_cpu(hrtimer_bases).clock_base[0].active.rb_root.rb_leftmost
$ p *$container_of($leftmost, "struct hrtimer", "node")
$3 = {
  node = {
    node = {
      __rb_parent_color = 18446612685234034192,
      rb_right = 0x0 <__pfx_nft_chain_nat_init>,
      rb_left = 0x0 <__pfx_nft_chain_nat_init>
    },
    expires = 166862000000
  },
  _softexpires = 166862000000,
  function = 0xffffffff813fdae0 <tick_nohz_handler>,
  base = 0xffff8880bc95c5c0,
  state = 1 '\001',
  is_rel = 0 '\000',
  is_soft = 0 '\000',
  is_hard = 1 '\001'
}
```

## 勉强能看的
https://duasynt.com/pub/gdb.pdf

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
