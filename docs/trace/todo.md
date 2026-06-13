## bpftrace bcc 和 ftrace 实现多个 tracepooint 同时 trace 实现方法的差别

ftrace 是通过这两个方法吧:
kernel/trace/trace_events_trigger.c
kernel/trace/trace_events_filter.c
kernel/trace/trace_events_hist.c 如何使用
	- Documentation/trace/histogram.rst

这三个选择理解下:
CONFIG_FTRACE_SYSCALLS=y
CONFIG_TRACE_IRQFLAGS=y
CONFIG_TRACE_IRQFLAGS_NMI=y

## trace-irqoff
- 不知道为什么 mainline 已经存在了一个 trace-irqoff ，但是字节还是重新搞了一个
  - https://www.kawabangga.com/posts/6793 : 看看这个
  - https://github.com/bytedance/trace-irqoff
  - 两者有什么不同?
  - https://mp.weixin.qq.com/s/X1Lj0feVCqlQL7nHsq5kaQ : 又有一次提到这个问题

## TODO
这几个东西是一个东西吗?
- https://github.com/dendibakh/perf-ninja
- https://github.com/dendibakh/perf-book
- https://weedge.github.io/perf-book-cn/zh/chapters/4-Terminology-And-Metrics/4-3_CPI_and_IPC_cn.html

- https://googleprojectzero.blogspot.com/2022/08/the-quantum-state-of-linux-kernel.html

- https://www.brendangregg.com/perf.html : 这个还是需要浏览一遍

- 如何实现这个功能?
```txt
  PID [ %CPU] %SYS    P   Mcycle   Minstr   IPC  %MISS  %BMIS  %BUS COMMAND
 7140+   6.4   1.0   13        ?        ?     ?      ?      ?     ? wezterm-gui
 7972+   3.4   0.5    4        ?        ?     ?      ?      ?     ? nvim
 2216+   3.4   1.0   19   182.53 18446744073674.99  101061985570.06   0.00   0.00     ? .gnome-shell-wr
75782+   2.4   0.0    0        ?        ?     ?      ?      ?     ? chrome
 1932+   2.4   1.0   31   111.32     7.23  0.06  13.62   2.19     ? X
117676+   2.0   0.0    3        ?        ?     ?      ?      ?     ? chrome
75276+   2.0   0.0   27        ?        ?     ?      ?      ?     ? chrome
218245    1.5   1.0   24    28.73    45.82  1.59   0.16   0.13     ? tiptop
56757+   1.5   0.0   17        ?        ?     ?      ?      ?     ? chrome
 7460    1.0   0.5   30        ?        ?     ?      ?      ?     ? tmux:
84335    0.5   0.5   11        ?        ?     ?      ?      ?     ? sudo
10580+   0.5   0.0   24        ?        ?     ?      ?      ?     ? chrome
 8297+   0.5   0.0   26        ?        ?     ?      ?      ?     ? nvim
 7969+   0.5   0.0   30        ?        ?     ?      ?      ?     ? nvim
 6658+   0.5   0.0    7        ?        ?     ?      ?      ?     ? metrics-server
```
sudo perf stat -C8 --timeout 10000 比较接近，但是无法区分 process 的信息

sudo perf top -e r31 可以执行，但是不知道 r31 是什么东西

sudo perf record -e probe:follow_pte -g -aR -- sleep 1 中的 -R 是什么意思?

一直以为 sleep 1 只是定时器，但是发现其实只是记录 sleep 的:
```txt
perf record -- sleep 1
perf report
```

## TODO
最近的增加的新功能:
2024-04-08 : https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=1bbeaf83dd7b

https://github.com/RRZE-HPC/likwid


## 记录一个问题，kunpeng 中 perf 中为什么只能得到这么粗略的结果
```txt
+    3.47%     0.00%  qemu-system-aar  libc.so.6                [.] start_thread
+    3.47%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] qemu_thread_start
+    3.47%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] kvm_vcpu_thread_fn
+    3.47%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] kvm_cpu_exec
+    3.46%     0.00%  qemu-system-aar  libc.so.6                [.] __GI___ioctl
+    3.46%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] kvm_vcpu_ioctl
     0.34%     0.00%  qemu-system-aar  libc.so.6                [.] __libc_start_main@@GLIBC_2.34
     0.34%     0.00%  qemu-system-aar  libc.so.6                [.] __libc_start_call_main
     0.34%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] main
     0.34%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] qemu_default_main
     0.34%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] qemu_main_loop
     0.34%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] main_loop_wait
     0.34%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] os_host_main_loop_wait (inlined)
     0.25%     0.00%  qemu-system-aar  qemu-system-aarch64      [.] glib_pollfds_poll (inlined)
     0.24%     0.00%  qemu-system-aar  libglib-2.0.so.0.8200.1  [.] g_main_context_dispatch
```

## 为什么 perf 的时候 的输出不是 100%

例如使用 qemu 的脚本:
```txt
+   90.65%     0.00%  qemu-system-x86  libc.so.6                [.] __GI___clone3
+   90.65%     0.00%  qemu-system-x86  libc.so.6                [.] start_thread
+   90.56%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] qemu_thread_start
+   90.53%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] kvm_vcpu_thread_fn
+   90.53%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] kvm_cpu_exec
+   90.53%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] kvm_vcpu_ioctl
+   90.53%     0.00%  qemu-system-x86  libc.so.6                [.] __GI___ioctl
+   89.98%     0.00%  qemu-system-x86  [kernel.kallsyms]        [k] entry_SYSCALL_64_after_hwframe
+   89.98%     0.00%  qemu-system-x86  [kernel.kallsyms]        [k] do_syscall_64
+   83.74%     0.00%  qemu-system-x86  [kernel.kallsyms]        [k] __x64_sys_ioctl
+   83.74%     0.00%  qemu-system-x86  [kernel.kallsyms]        [k] kvm_vcpu_ioctl
+   83.74%     1.23%  qemu-system-x86  [kernel.kallsyms]        [k] kvm_arch_vcpu_ioctl_run
+   37.99%     2.00%  qemu-system-x86  [kernel.kallsyms]        [k] vcpu_enter_guest.constprop.0
+   31.33%     0.36%  qemu-system-x86  [kernel.kallsyms]        [k] kvm_vcpu_halt
+   30.66%     0.34%  qemu-system-x86  [kernel.kallsyms]        [k] kvm_vcpu_block
+   27.59%     0.31%  qemu-system-x86  [kernel.kallsyms]        [k] schedule
+   26.99%     0.40%  qemu-system-x86  [kernel.kallsyms]        [k] __schedule
+   22.44%     2.46%  qemu-system-x86  [kernel.kallsyms]        [k] vmx_vcpu_run
+   15.28%    15.28%  qemu-system-x86  [kernel.kallsyms]        [k] native_write_msr
+   10.85%     0.98%  qemu-system-x86  [kernel.kallsyms]        [k] finish_task_switch.isra.0
+    9.04%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] _start
+    9.04%     0.00%  qemu-system-x86  libc.so.6                [.] __libc_start_main@@GLIBC_2.34
+    9.04%     0.00%  qemu-system-x86  libc.so.6                [.] __libc_start_call_main
+    9.04%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] main
+    9.04%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] qemu_default_main
+    9.04%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] qemu_main_loop
+    9.04%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] main_loop_wait
+    8.77%     0.00%  qemu-system-x86  qemu-system-x86_64       [.] os_host_main_loop_wait (inlined)
+    8.42%     0.17%  qemu-system-x86  [kernel.kallsyms]        [k] kvm_sched_in
+    8.31%     0.63%  qemu-system-x86  [kernel.kallsyms]        [k] kvm_arch_vcpu_load
+    7.78%     0.10%  qemu-system-x86  [kernel.kallsyms]        [k] dequeue_task_fair
+    7.75%     0.28%  qemu-system-x86  [kernel.kallsyms]        [k] kvm_load_guest_xsave_state
+    7.67%     0.58%  qemu-system-x86  [kernel.kallsyms]        [k] dequeue_entities
```

### 96.45% 的比例下面直接就是 36.36% 了啊

```txt
-   97.83%     0.00%  .qemu-system-x8  libc.so.6                    [.] __clone3 (inlined)
     __clone3 (inlined)
     start_thread
     qemu_thread_start
     kvm_vcpu_thread_fn
   - kvm_cpu_exec
      - 96.51% kvm_vcpu_ioctl
         - 96.45% __GI___ioctl
            - 36.36% entry_SYSCALL_64_after_hwframe
               - do_syscall_64
                  - 36.17% __x64_sys_ioctl
                     - 36.08% kvm_vcpu_ioctl
                        - 36.05% kvm_arch_vcpu_ioctl_run
                           - 25.98% vmx_vcpu_run
                              - 6.27% delay_halt
                                   delay_halt_tpause
                              - 5.48% kvm_load_host_xsave_state
                                   native_write_msr
                                3.45% native_write_msr
                              - 3.08% kvm_load_guest_xsave_state
                                   native_write_msr
                              - 1.57% kvm_lapic_expired_hv_timer
                                 - 1.21% apic_timer_expired
                                    - 1.07% kvm_apic_local_deliver
                                         __apic_accept_irq
                                1.48% vmx_vcpu_enter_exit
                                0.59% add_atomic_switch_msr.constprop.0
                                0.56% __vmx_vcpu_run_flags
                           - 3.11% handle_fastpath_set_msr_irqoff
                              - 0.77% restart_apic_timer
                                   start_hv_timer
                              - 0.77% kvm_skip_emulated_instruction
                                   0.55% skip_emulated_instruction
                           - 2.16% vmx_sync_pir_to_irr
                              - 0.70% kvm_apic_update_irr
                                   __kvm_apic_update_irr
                           - 0.83% xfer_to_guest_mode_handle_work
                              - 0.58% schedule
                                   __schedule
              9.82% vmx_vmexit
      - 0.97% address_space_rw
         - 0.71% flatview_write
            - flatview_write_continue
               - memory_region_dispatch_write
                 access_with_adjusted_size
                 memory_region_write_accessor
               - gsi_handler
                  - kvm_pic_set_irq
                  - kvm_set_irq
                     - kvm_vm_ioctl
                       __GI___ioctl
                       entry_SYSCALL_64_after_hwframe
```

## 应该可以对于 trace handler 继续注册 handler 吧

## kernel/trace/rv/


## 到时候整理一下这个
https://stackoverflow.com/questions/375913/how-do-i-profile-c-code-running-on-linux

## Use ftrace_regs for function tracing
- https://lpc.events/event/17/contributions/1588/attachments/1169/2481/LPC23_%20Use%20ftrace_regs%20for%20tracers_1.pdf

介绍了 ftrace 的基础


## 参考这个来看 trace 中的
https://github.com/kernel-cyrus/perf-case/blob/master/perf_case.c

## 等待
https://stratoshark.org/

## 为什么 kernel 中，在时钟相关的函数上不可以 trace 的?
```c
/*
 * Similar to cpu_clock(), but requires local IRQs to be disabled.
 *
 * See cpu_clock().
 */
notrace u64 sched_clock_cpu(int cpu)
```

## 两个问题

```txt
+ sudo bpftrace -e 'kprobe:io_schedule { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[
    io_schedule+5
    blk_mq_get_tag+348
    __blk_mq_alloc_requests+437
    blk_mq_submit_bio+424
    __submit_bio+149
    submit_bio_noacct_nocheck+666
    blkdev_direct_IO.part.0+586
    blkdev_read_iter+184
    aio_read+312
    io_submit_one+392
    __x64_sys_io_submit+173
    do_syscall_64+57
    entry_SYSCALL_64_after_hwframe+120
]: 710
```
但是如果去测试 io_schedule_prepare 就完全没有

## 即便是 trace 的点有一个锁，也是可能让输出为乱序的

```sh
./trace -I linux/timekeeper_internal.h  'update_vsyscall(struct timekeeper *tk) "%lx %lx", tk->tkr_mono.xtime_nsec , tk->xtime_sec'
```

是的，可以非常容易的发现:
```txt
0       0       swapper/41      update_vsyscall  4389942710058787 1740553824
9493    21294   spdk-main       update_vsyscall  4306058445639321 1740553824
```
这里的 tk->tkr_mono.xtime_nsec 在 tk->xtime_sec 没有变化的情况下，
发生了回退，其实是错误的。

配套工具:
```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

tmp=/tmp/tsc.txt
awk '{print $NF}' "$1" > $tmp

prev_val=$(awk 'BEGIN{print -2^31}')

while IFS= read -r line; do
    [ -z "$line" ] && continue

    if ! [[ "$line" =~ ^[0-9]+$ ]]; then
        echo "发现非数字值: $line"
        continue
    fi

    curr_val=$line

    if [ "$curr_val" -lt "$prev_val" ]; then
        echo "非递增值: $curr_val (前一个值: $prev_val)"
    fi

    prev_val=$curr_val

done < $tmp
```
## 似乎有的函数无法被 trace 出来

ttwu_do_activate 中，如果去测试 activate_task ，
发现其调用者很少，但是测试 trace_sched_wakeup ，发现调用很多

```c
	activate_task(rq, p, en_flags);
	wakeup_preempt(rq, p, wake_flags);

	ttwu_do_wakeup(p);
```

```c
/*
 * Mark the task runnable.
 */
static inline void ttwu_do_wakeup(struct task_struct *p)
{
	WRITE_ONCE(p->__state, TASK_RUNNING);
	trace_sched_wakeup(p);
}
```
```txt
@[
    activate_task+5
    attach_task+52
    active_load_balance_cpu_stop+678
    cpu_stopper_thread+151
    smpboot_thread_fn+375
    kthread+220
    ret_from_fork+49
    ret_from_fork_asm+26
]: 12
```

```txt
@[
    bpf_prog_7dc8126e8768ea37_sd_fw_ingress+462
    bpf_prog_7dc8126e8768ea37_sd_fw_ingress+462
    bpf_trace_run1+129
    ttwu_do_activate+278
    try_to_wake_up+713
    hrtimer_wakeup+34
    __hrtimer_run_queues+332
    hrtimer_interrupt+255
    __sysvec_apic_timer_interrupt+82
    sysvec_apic_timer_interrupt+110
    asm_sysvec_apic_timer_interrupt+26
    cpuidle_enter_state+220
    cpuidle_enter+45
    do_idle+459
    cpu_startup_entry+41
    start_secondary+291
    common_startup_64+318
]: 2689
```
看上去，自动的把 ttwu_do_activate 中调用 activate_task 的 backtrace 忽视掉了

## 如果两个模块中有相同的名称，trace 机制如何选择?

## 为什么自己构建的内核模块有这个问题
仅仅在 nixos 中:
```txt
sudo bpftrace -e "kfunc:__apic_accept_irq { if (args->delivery_mode == 0x200 ) { @[kstack]=count(); } }"
[sudo] password for martins3:
stdin:1:1-24: ERROR: kfunc:__apic_accept_irq: no BTF data for __apic_accept_irq
kfunc:__apic_accept_irq { if (args->delivery_mode == 0x200 ) { @[kstack]=count(); } }

```

## 从 docs/kernel/mm/mm-slub.sh 可以知道
如果机器中携带了 debuginfo ，那么 perf 可以
进行过滤，当然，没有 debuginfo 就难办了

```txt
🧀  sudo perf probe kmem_cache_alloc_noprof 's->name:string'
Failed to find the path for the kernel: No such file or directory
  Error: Failed to add events.
```

## 剥离启动的 trace 技术
https://clickhouse.com/blog/a-case-of-the-vanishing-cpu-a-linux-kernel-debugging-story

## perf-prof
https://github.com/OpenCloudOS/perf-prof

## 如果在同时 trace ，可以用这个效果
```txt
  b'enqueue_to_backlog'
  b'netif_rx_internal'
  b'internal_dev_recv'
  b'do_execute_actions'
  b'ovs_execute_actions'
  b'ovs_dp_process_packet'
  b'kretprobe_trampoline'
```

## 有办法知道用户态的 backtrace 吗?

例如现在调用了 kvm_arch_vcpu_ioctl ，那么现在如果我想知道，qemu 是如何调用到这里的，
该如何实现。

使用 bcc 的实现效果相当之差:
```txt
/trace kvm_arch_vcpu_ioctl -U
PID     TID     COMM            FUNC
35638   35741   CPU 0/KVM       kvm_arch_vcpu_ioctl
        ioctl+0x7 [libc-2.17.so]
        g_free+0x0 [libglib-2.0.so.0.5000.3]

35638   35741   CPU 0/KVM       kvm_arch_vcpu_ioctl
        ioctl+0x7 [libc-2.17.so]

35638   35741   CPU 0/KVM       kvm_arch_vcpu_ioctl
        ioctl+0x7 [libc-2.17.so]
```


## 似乎对于 tracepoint 的理解还是有问题

```txt
sudo trace-cmd record -e block:block_rq_insert ./your_program
sudo trace-cmd report
```

可以获取到这个效果:
```txt
fio-40164 [001] d..1.  7848.448709: block_rq_insert:      8,16 RS 4096 () 53758080 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.448839: block_rq_insert:      8,16 RS 4096 () 53316024 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.448966: block_rq_insert:      8,16 RS 4096 () 54891400 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.449101: block_rq_insert:      8,16 RS 4096 () 53451544 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.449266: block_rq_insert:      8,16 RS 4096 () 54570384 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.449479: block_rq_insert:      8,16 RS 4096 () 53232448 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.449623: block_rq_insert:      8,16 RS 4096 () 54284896 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.449764: block_rq_insert:      8,16 RS 4096 () 55154952 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.449904: block_rq_insert:      8,16 RS 4096 () 53796472 + 8 0x2,0,4 [fio]
fio-40164 [001] d..1.  7848.450043: block_rq_insert:      8,16 RS 4096 () 53870632 + 8 0x2,0,4 [fio]
```

但是如果是:

sudo perf trace -e block:block_rq_insert
```txt
  2126.806 :41409/41409 block:block_rq_insert(dev: 8388624, sector: 19663072, nr_sector: 56, bytes: 28672, ioprio: 16388, rwbs: "W", comm: "sqlx-sqlite-wor", cmd: "")
  2127.024 :41409/41409 block:block_rq_insert(dev: 8388624, sector: 20986272, nr_sector: 8, bytes: 4096, ioprio: 16388, rwbs: "W", comm: "sqlx-sqlite-wor", cmd: "")
```

问题是 include/trace/events/block.h 中定义的是这样的
```c
/**
 * block_rq_insert - insert block operation request into queue
 * @rq: block IO operation request
 *
 * Called immediately before block operation request @rq is inserted
 * into queue @q.  The fields in the operation request @rq struct can
 * be examined to determine which device and sectors the pending
 * operation would access.
 */
DEFINE_EVENT(block_rq, block_rq_insert,

	TP_PROTO(struct request *rq),

	TP_ARGS(rq)
);
```

为什么最后就可以自动选择数值输出了，他的参数只是 request 啊

通过这个东西可以清楚的看到，看来 tracepoint 写了一个什么 macro 的:
```txt
🧀  sudo bpftrace -lv t:block:block_rq_insert
tracepoint:block:block_rq_insert
    dev_t dev
    sector_t sector
    unsigned int nr_sector
    unsigned int bytes
    unsigned short ioprio
    char rwbs[10]
    char comm[16]
    __data_loc char[] cmd
```

## 看看这个
https://github.com/SeeFlowerX/stackplz 仅适用于Android平台

## 看看他的意见
https://blog.vmsplice.net/2025/06/profiling-tools-i-use-for-qemu-storage.html

## 有一个 profile 工具?
https://github.com/rogercoll/eprofiler-tui


## 看看这个
https://github.com/bytedance/kvm-utils

## 可以对于一个函数开始 perf 吗?

## trace 工具细节
> [!NOTE]
> 参考神奇海螺的意见，有待验证

AI 生成的，不过挺好的

1. ftrace 系列（内核内置跟踪框架）

 工具                 用途                 说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 ftrace               内核函数跟踪         通过 /sys/kernel/tracing 接口，支持多种 tracer
 function tracer      函数级跟踪           跟踪内核函数调用
 function_graph       带调用图的函数跟踪   显示函数调用关系和耗时
 blk tracer           块设备I/O跟踪        类似 blktrace，内置于 ftrace
 irqsoff/preemptoff   关中断/抢占延迟      检测最长关中断/抢占时间
 wakeup/wakeup_rt     调度延迟             测量实时任务唤醒延迟
 mmiotrace            MMIO 跟踪            跟踪内存映射 I/O 操作
 timerlat/osnoise     定时器/系统噪声      RT 内核延迟分析

2. perf 系列（性能剖析）

 工具                 用途              说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 perf record/report   性能采样          热点函数分析
 perf top             实时性能监控      类似 top，但看内核符号
 perf trace           系统调用跟踪      类似 strace，但性能更好
 perf sched           调度分析          perf sched latency/map/timehist
 perf kvm             KVM 分析          类似 kvm_stat，可分析 guest
 perf probe           动态探针          kprobe 前端，动态添加跟踪点
 perf ftrace          ftrace 封装       perf ftrace -G vfs_open
 perf lock            锁竞争分析        分析内核锁性能
 perf mem/c2c         内存/cache 分析   内存访问模式分析

3. eBPF 工具（现代动态跟踪）

 工具       用途           说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 bpftrace   高级跟踪脚本   类 D 语言语法，如 bpftrace -e 'kprobe:do_nanosleep { printf("sleep\n"); }'
 BCC        BPF 编译集合   Python/C 编写 eBPF 程序
 bpftool    BPF 程序管理   查看、加载、调试 eBPF 程序

4. sysstat 系列（系统监控）

 工具      用途           说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 sar       系统活动报告   sar -n DEV 1（网络）、sar -d 1（磁盘）
 iostat    I/O 统计       块设备吞吐和延迟
 pidstat   进程统计       CPU、内存、I/O 按进程统计
 mpstat    CPU 统计       多核 CPU 使用率
 vmstat    内存统计       虚拟内存、进程、CPU 活动

5. 其他专用工具

 工具        用途            说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 strace      系统调用跟踪    用户态程序 syscall 跟踪
 iotop       I/O 实时视图    按进程显示 I/O 使用
 slabtop     Slab 缓存监控   内核对象分配情况
 pstack      进程堆栈        查看进程在内核的栈
 lsof        打开文件        查看进程打开的文件
 pmap        进程内存映射    解析 /proc/pid/maps
 numastat    NUMA 统计       NUMA 节点内存访问
 turbostat   CPU 电源状态    频率、C-state、P-state

6. 调试分析工具

 工具                  用途           说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 crash/drgn            内核崩溃转储   分析 vmcore
 kdump                 崩溃收集       配置收集崩溃信息
 /proc/sysrq-trigger   系统调试       echo m > sysrq-trigger（内存信息）
 dynamic_debug         动态打印       控制 pr_debug 输出

## eprobe 按道理不能使用 kprobe ?
<!-- df8d0ba4-bc56-4b12-98a3-14a27b6931f4 -->

从这里看跳过的:
```c
static struct trace_event_call *
find_and_get_event(const char *system, const char *event_name)
{
	struct trace_event_call *tp_event;
	const char *name;

	list_for_each_entry(tp_event, &ftrace_events, list) {
		/* Skip other probes and ftrace events */
		if (tp_event->flags &
		    (TRACE_EVENT_FL_IGNORE_ENABLE |
		     TRACE_EVENT_FL_KPROBE |
		     TRACE_EVENT_FL_UPROBE |
		     TRACE_EVENT_FL_EPROBE))
			continue;
		if (!tp_event->class->system ||
		    strcmp(system, tp_event->class->system))
			continue;
		name = trace_event_name(tp_event);
		if (!name || strcmp(event_name, name))
			continue;
		if (!trace_event_try_get_ref(tp_event))
			return NULL;
		return tp_event;
	}
	return NULL;
}
```

但是 Documentation/trace/eprobetrace.rst 这里看，kprobe 是可以使用的

如何理解 Documentation/trace/eprobetrace.rst 中的 Synthetic Events

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
