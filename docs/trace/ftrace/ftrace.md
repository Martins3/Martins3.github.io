## ftrace 输出的格式
https://www.kernel.org/doc/html/latest/trace/ftrace.html#output-format

## ftrace 是如何防止自己 trace 自己的

linux/kernel/trace/ 下的函数都没有办法 trace，

sudo perf probe --add register_ftrace_command 可以观察到

```txt
[29020.876213] trace_kprobe: Could not probe notrace function _text
[29288.128622] trace_kprobe: Could not probe notrace function _text
```

```txt
#0  __register_trace_kprobe (tk=0xffff888128bd6700) at kernel/trace/trace_kprobe.c:482
#1  0xffffffff812c7379 in register_trace_kprobe (tk=0xffff888128bd6700) at kernel/trace/trace_kprobe.c:657
#2  __trace_kprobe_create (argc=<optimized out>, argv=<optimized out>) at kernel/trace/trace_kprobe.c:957
#3  0xffffffff812cf6b8 in trace_probe_create (raw_command=<optimized out>, createfn=createfn@entry=0xffffffff812c69d0 <__trace_kprobe_create>) at kernel/trace/trace_probe.c:1948
#4  0xffffffff812c4a45 in trace_kprobe_create (raw_command=<optimized out>) at kernel/trace/trace_kprobe.c:985
#5  create_or_delete_trace_kprobe (raw_command=<optimized out>) at kernel/trace/trace_kprobe.c:995
#6  0xffffffff8129b5ca in trace_parse_run_command (file=<optimized out>, buffer=0x555d3cf78520 "p:probe/__register_trace_kprobe _text+2902656", count=45, ppos=<optimized out>, createfn=0xffffffff812c4a30 <create_or_delete_trace_kprobe>) at kernel/trace/trace.c:10478
#7  0xffffffff8146795b in vfs_write (file=file@entry=0xffff8881355ae200, buf=buf@entry=0x555d3cf78520 "p:probe/__register_trace_kprobe _text+2902656", count=count@entry=45, pos=pos@entry=0xffffc900023ebef8) at fs/read_write.c:588
#8  0xffffffff8146808f in ksys_write (fd=<optimized out>, buf=0x555d3cf78520 "p:probe/__register_trace_kprobe _text+2902656", count=45) at fs/read_write.c:643
#9  0xffffffff824088aa in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900023ebf58) at arch/x86/entry/common.c:52
#10 do_syscall_64 (regs=0xffffc900023ebf58, nr=<optimized out>) at arch/x86/entry/common.c:83
#11 0xffffffff826000eb in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

`__register_trace_kprobe` 中检查跟踪的符号的位置:

```c
/* Internal register function - just handle k*probes and flags */
static int __register_trace_kprobe(struct trace_kprobe *tk)
{
	int i, ret;

	ret = security_locked_down(LOCKDOWN_KPROBES);
	if (ret)
		return ret;

	if (trace_kprobe_is_registered(tk))
		return -EINVAL;

	if (within_notrace_func(tk)) {
		pr_warn("Could not probe notrace function %ps\n",
			(void *)trace_kprobe_address(tk));
		return -EINVAL;
	}
```

kernel/trace/Kconfig
```txt
config KPROBE_EVENTS_ON_NOTRACE
	bool "Do NOT protect notrace function from kprobe events"
	depends on KPROBE_EVENTS
	depends on DYNAMIC_FTRACE
	default n
	help
	  This is only for the developers who want to debug ftrace itself
	  using kprobe events.

	  If kprobes can use ftrace instead of breakpoint, ftrace related
	  functions are protected from kprobe-events to prevent an infinite
	  recursion or any unexpected execution path which leads to a kernel
	  crash.

	  This option disables such protection and allows you to put kprobe
	  events on ftrace functions for debugging ftrace by itself.
	  Note that this might let you shoot yourself in the foot.

	  If unsure, say N.
```

确定一个符号是否是 notrace 的比想象的要复杂
- within_notrace_func
  - `__within_notrace_func`
    - ftrace_location_range
      - lookup_rec : 在这里，利用 `ftrace_pages_start` 记录的数据来查询，这和我想象的不一样，本来以为链接信息的

## tracer

- timerlat
- osnoise
- blk
- mmiotrace
- function_graph
- wakeup_dl
- wakeup_rt
- wakeup
- preemptirqsoff
- preemptoff
- irqsoff
- function
- nop

### [x] timerlat
Documentation/trace/timerlat-tracer.rst

和 osnoise 很像

### [x] mmiotrace
Documentation/trace/mmiotrace.rst

感觉长期没有人维护了

1. 自己构建内核，使用之后 guest kernel 直接 crash ，应该是存在什么依赖的选项没有打上，应该是和 cpuoffline 相关的
```txt
echo 0 > options/function-trace
echo preemptoff > current_tracer
echo 1 > tracing_on
echo 0 > tracing_max_latency
```
2. 打开浏览器，可以观察到，但是看上去 header 和输出不是匹配的
```txt
# tracer: mmiotrace
#
# entries-in-buffer/entries-written: 15/15   #P:1
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
W 4 411.390479 4 0x3ffa01c90048 0xa1690a20 0x0 0
W 4 411.390483 4 0x3ffa01c9004c 0x17b4dbba 0x0 0
W 4 411.390484 4 0x3ffa01c90040 0xbbbbbbbb 0x0 0
W 4 411.407167 4 0x3ffa01c90048 0xa267c040 0x0 0
W 4 411.407168 4 0x3ffa01c9004c 0x17b4dbba 0x0 0
W 4 411.407169 4 0x3ffa01c90040 0xbbbbbbbb 0x0 0
W 4 411.423834 4 0x3ffa01c90048 0xa3661bc0 0x0 0
W 4 411.423835 4 0x3ffa01c9004c 0x17b4dbba 0x0 0
W 4 411.423836 4 0x3ffa01c90040 0xbbbbbbbb 0x0 0
W 4 411.440515 4 0x3ffa01c90048 0xa464b900 0x0 0
W 4 411.440516 4 0x3ffa01c9004c 0x17b4dbba 0x0 0
W 4 411.440517 4 0x3ffa01c90040 0xbbbbbbbb 0x0 0
W 4 411.457201 4 0x3ffa01c90048 0xa5634d20 0x0 0
W 4 411.457203 4 0x3ffa01c9004c 0x17b4dbba 0x0 0
W 4 411.457204 4 0x3ffa01c90040 0xbbbbbbbb 0x0 0
```

打开 mmiotrace 之后，除了 CPU 0 之外，其他的 CPU 都会被 offline 掉的

### [x] preemptirqsoff preemptoff irqsoff
Documentation/trace/ftrace.rst

### wakeup wakeup_dl wakeup_rt


Documentation/trace/ftrace.rst

实现的代码: kernel/trace/trace_sched_wakeup.c

当使用 fio 测试的时候

echo wakeup > current_tracer
cat trace
```txt
     fio-1745     11d..5. 160174482us :     1745:120:R   + [011]     379:100:R kworker/11:1H
     fio-1745     11d..5. 160174484us : <stack trace>
 => __ftrace_trace_stack
 => probe_wakeup
 => ttwu_do_activate
 => try_to_wake_up
 => kick_pool
 => __queue_work
 => mod_delayed_work_on
 => kblockd_mod_delayed_work_on
 => blk_mq_submit_bio
 => submit_bio_noacct_nocheck
 => blkdev_direct_IO.part.0
 => blkdev_read_iter
 => aio_read
 => io_submit_one
 => __x64_sys_io_submit
 => do_syscall_64
 => entry_SYSCALL_64_after_hwframe
     fio-1745     11d..5. 160174484us : 0
     fio-1745     11d..3. 160174692us : __schedule
     fio-1745     11d..3. 160174693us :     1745:120:S ==> [011]     379:100:R kworker/11:1H
     fio-1745     11d..3. 160174694us : <stack trace>
 => __ftrace_trace_stack
 => probe_wakeup_sched_switch
 => __schedule
 => schedule
 => read_events
 => do_io_getevents
 => __x64_sys_io_getevents
 => do_syscall_64
 => entry_SYSCALL_64_after_hwframe
```

其实这个 tracer 主要是 realtime 的时候有用，用于观察，从进入就绪到真的可以执行的时候一共花费了多长时间。

#### 其他阅读材料
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux_for_real_time/7/html/tuning_guide/using_the_ftrace_utility_for_tracing_latencies
- https://static.lwn.net/images/conf/rtlws11/papers/proc/p02.pdf
- https://www.kernel.org/doc/Documentation/trace/ftrace.txt


### sched_switch 和 sched_wakeup
其实并不存在这个 tracer ，而是存在对应这个 event
```txt
config SCHED_TRACER
	bool "Scheduling Latency Tracer"
	select GENERIC_TRACER
	select CONTEXT_SWITCH_TRACER
	select TRACER_MAX_TRACE
	select TRACER_SNAPSHOT
	help
	  This tracer tracks the latency of the highest priority task
	  to be scheduled in, starting from the point it has woken up.
```

- kernel/trace/trace_sched_switch.c


echo 1 > /sys/kernel/tracing/events/sched/sched_switch/enable 可以得到这个 backtrace 出来:

```txt
#0  tracing_start_cmdline_record () at kernel/trace/trace_sched_switch.c:133
#1  0xffffffff812b32c2 in __ftrace_event_enable_disable (file=file@entry=0xffff888100815420, enable=<optimized out>, soft_disable=soft_disable@entry=0) at kernel/trace/trace_events.c:690
#2  0xffffffff812b37f7 in ftrace_event_enable_disable (enable=<optimized out>, file=0xffff888100815420) at kernel/trace/trace_events.c:730
#3  event_enable_write (filp=<optimized out>, ubuf=<optimized out>, cnt=2, ppos=0xffffc90002157ef8) at kernel/trace/trace_events.c:1434
#4  0xffffffff8146867b in vfs_write (file=file@entry=0xffff8881064e4b00, buf=buf@entry=0x55aafd83f980 "1\n1;echo\a1 > enable\asys/kernel/tracing/events/sched/sched_switch \a", count=count@entry=2, pos=pos@entry=0xffffc90002157ef8) at fs/read_write.c:588
#5  0xffffffff81468daf in ksys_write (fd=<optimized out>, buf=0x55aafd83f980 "1\n1;echo\a1 > enable\asys/kernel/tracing/events/sched/sched_switch\a", count=2) at fs/read_write.c:643
#6  0xffffffff8240a8ca in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90002157f58) at arch/x86/entry/common.c:52
#7  do_syscall_64 (regs=0xffffc90002157f58, nr=<optimized out>) at arch/x86/entry/common.c:83
```

但是谁来调用 tracing_start_tgid_record

### [x] branch
Documentation/trace/ftrace.rst

- kernel/trace/trace_branch.c : 实现的文件

三种方式来实现，实现的方式令人震惊
```txt
CONFIG_BRANCH_PROFILE_NONE=y
CONFIG_PROFILE_ANNOTATED_BRANCHES
CONFIG_PROFILE_ALL_BRANCHES
```

include/linux/compiler.h

```txt
#ifdef CONFIG_PROFILE_ALL_BRANCHES
/*
 * "Define 'is'", Bill Clinton
 * "Define 'if'", Steven Rostedt
 */
#define if(cond, ...) if ( __trace_if_var( !!(cond , ## __VA_ARGS__) ) )

#define __trace_if_var(cond) (__builtin_constant_p(cond) ? (cond) : __trace_if_value(cond))

#define __trace_if_value(cond) ({			\
	static struct ftrace_branch_data		\
		__aligned(4)				\
		__section("_ftrace_branch")		\
		__if_trace = {				\
			.func = __func__,		\
			.file = __FILE__,		\
			.line = __LINE__,		\
		};					\
	(cond) ?					\
		(__if_trace.miss_hit[1]++,1) :		\
		(__if_trace.miss_hit[0]++,0);		\
})

#endif /* CONFIG_PROFILE_ALL_BRANCHES */

#else
# define likely(x)	__builtin_expect(!!(x), 1)
# define unlikely(x)	__builtin_expect(!!(x), 0)
# define likely_notrace(x)	likely(x)
# define unlikely_notrace(x)	unlikely(x)
#endif
```

### [ ] `OSNOISE_TRACER` 和 `HWLAT_TRACER` 的关系是什么?

### boottime tracing
检查内核选项: CONFIG_BOOTTIME_TRACING

设置如下等效于
```txt
   ftrace_notrace=rcu_read_lock,rcu_read_unlock,spin_lock,spin_unlock
   ftrace_filter=kfree,kmalloc,schedule,vmalloc_fault,spurious_fault
```
在开机的时候设置 ftrace_notrace 和 ftrace_filter 中的内容。

- https://docs.kernel.org/trace/boottime-trace.html : 几乎什么都可以做到的
- https://lpc.events/event/4/contributions/445/attachments/289/487/LPC_2019_-_boottime_tracing.pdf

- 使用 "kprobe_event=" 来实现 kprobes

### 似乎不容易使用啊

ftrace=function ftrace_filter=request_firmware

```txt
🧀  iperf3 -c 10.0.0.2
Connecting to host 10.0.0.2, port 5201
[  5] local 10.0.0.2 port 46030 connected to 10.0.0.2 port 5201
[ ID] Interval           Transfer     Bitrate         Retr  Cwnd
[  5]   0.00-1.00   sec  2.52 GBytes  21.6 Gbits/sec    0   1.06 MBytes
[  5]   1.00-2.00   sec  2.56 GBytes  22.0 Gbits/sec    0   1.25 MBytes
[  5]   2.00-3.00   sec  2.48 GBytes  21.3 Gbits/sec    0   1.25 MBytes
[  5]   3.00-4.00   sec  2.28 GBytes  19.6 Gbits/sec    0   1.25 MBytes
[  5]   4.00-5.00   sec  2.42 GBytes  20.8 Gbits/sec    0   1.25 MBytes
[  5]   5.00-6.00   sec  2.38 GBytes  20.5 Gbits/sec    0   1.25 MBytes
[  5]   6.00-7.00   sec  2.48 GBytes  21.3 Gbits/sec    0   1.25 MBytes
[  5]   7.00-8.00   sec  2.55 GBytes  21.9 Gbits/sec    0   3.06 MBytes
[  5]   8.00-9.00   sec  2.50 GBytes  21.4 Gbits/sec    0   3.06 MBytes
[  5]   9.00-10.00  sec  2.50 GBytes  21.5 Gbits/sec    0   3.06 MBytes
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-10.00  sec  25.4 GBytes  21.8 Gbits/sec    0             sender
[  5]   0.00-10.00  sec  25.4 GBytes  21.8 Gbits/sec                  receiver

iperf Done.
```

调整之后:
```txt
🧀  iperf3 -s
-----------------------------------------------------------
Server listening on 5201 (test #1)
-----------------------------------------------------------
Accepted connection from 10.0.0.2, port 36052
[  5] local 10.0.0.2 port 5201 connected to 10.0.0.2 port 36056
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-1.00   sec  14.7 GBytes   126 Gbits/sec
[  5]   1.00-2.00   sec  14.1 GBytes   121 Gbits/sec
[  5]   2.00-3.00   sec  14.5 GBytes   124 Gbits/sec
[  5]   3.00-4.00   sec  14.6 GBytes   126 Gbits/sec
[  5]   4.00-5.00   sec  14.6 GBytes   125 Gbits/sec
[  5]   4.00-5.00   sec  14.6 GBytes   125 Gbits/sec
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-5.00   sec  80.8 GBytes   139 Gbits/sec                  receiver
```

## CONFIG_FUNCTION_PROFILER

echo 1 > /sys/kernel/debug/tracing/function_profile_enabled

cd /sys/kernel/debug/tracing/trace_stat

其中每一个文件对应一个 CPU 的函数统计:

trace/ftrace.rst 中 function_profile_enabled 这个章节:

## 文摘
- [A look at ftrace](https://lwn.net/Articles/322666/)
  - 价值不大
- [Secrets of the Ftrace function tracer](https://lwn.net/Articles/370423/)
  - **基本的，最核心的使用，很有价值，常看常新**
- [Debugging the kernel using Ftrace - part 1](https://lwn.net/articles/365835/)
  - 介绍了 trace_prink
- [Debugging the kernel using Ftrace - part 2](https://lwn.net/Articles/366796/)
  - 介绍了 stack tracking 的技术
  - tracing_off() 在内核中直接调控 tracing_on
  - 通过设置 /proc/sys/kernel/ftrace_dump_on_oops : 可以实现在内核挂掉的时候，让 ftrace buffer 的内容输出到 printk 中
  - You can also trigger a dump of the Ftrace buffer to the console with sysrq-z.

## /sys/kernel/debug/tracing/README
- trace_marker : 给 trace 增加标记的
- set_ftrace_filter 和 events/<system>/<event>/trigger 的差别
  - [ ] 其中的 stacktrace snapshot dump cpudump 的操作效果检查下

## Documentation/trace/ftrace.rst
1. 参考 [`The File System`section](https://www.kernel.org/doc/html/latest/trace/ftrace.html#the-file-system) 和 `cat /sys/kernel/tracing/README` 来看看 options 都是做什么的
  - buffer_size_kb : 每一个 CPU 的 buffer size
  - buffer_total_size_kb : readonly 的，展示所有 CPU 的 buffer 的和
  - buffer_subbuf_size_kb : 和超大 log entry 有关，系统底层了，不关心了.
2. 一共到底存在那些 tracer
3. 讲解 [filter-commands](https://www.kernel.org/doc/html/latest/trace/ftrace.html#filter-commands) 的各种技巧

## Documentation/trace/kprobetrace.rst

> You can also use /sys/kernel/tracing/dynamic_events instead of kprobe_events.
That interface will provide unified access to other dynamic events too.

```plain
[root@nixos:/sys/kernel/debug/tracing]# cat dynamic_events
p:kprobes/myprobe2 do_sys_openat2 dfd=%ax filename=%dx open_how=%cx usize=+4($stack)
```


这样可以:
```sh
echo 'f:myprobe vfs_read' > dynamic_events
```
然后 events 目录中会增加 fprobes


## Documentation/trace/kprobes.rst
分析 kprobe 的实现原理

## Documentation/trace/kprobetrace.rst
分析 kprobe 在 ftrace 接口中如何使用

## 材料

- https://stackoverflow.com/questions/58080787/ftrace-function-enabling-fails-with-resource-busy

- 通过 dyn_ftrace_total_info 可以知道有多少函数其实含有 nop 指令的

```txt
64566 pages:472 groups: 273
```

### brendangregg/perf-tools

https://github.com/brendangregg/perf-tools : 被废弃的项目，基本说明了 ftrace 是怎么操作的
作为 trace 技术的考古还是非常有意思的。


## 其他特殊功能
- instances 目录 : 在这个目录中操作会记录到单独的 trace buffer
- snapshot : 对于 trace buffer 做 snapshot

- `tracing_on` 和 `/proc/sys/kernel/ftrace_enabled` 的含义: 没太搞懂差别，但是一般只是使用 tracing_on 就够了

## options
记录下让人好奇的 trace options

- display-graph

	When set, the latency tracers (irqsoff, wakeup, etc) will
	use function graph tracing instead of function tracing.

- function-trace

	The latency tracers will enable function tracing
	if this option is enabled (default it is). When
	it is disabled, the latency tracers do not trace
	functions. This keeps the overhead of the tracer down
	when performing latency tests.


- funcgraph-irqs

	When disabled, functions that happen inside an
	interrupt will not be traced.

- funcgraph-abstime

	When set, the timestamp is displayed at each line.

- funcgraph-proc

	Unlike other tracers, the process' command line
	is not displayed by default, but instead only
	when a task is traced in and out during a context
	switch. Enabling this options has the command
	of each process displayed at every line.

## trace_printk
关于 trace_printk 的内容，参考 https://lwn.net/articles/365835/

## 这这个图是怎么做的?
https://mp.weixin.qq.com/s/-qSXrEa1NsUxjtqxF8BvpA

## 我的记忆是有 ftrace 本来就有 backtrace 的，一个 option ?
man trace-cmd-record

```txt
       --func-stack

           Enable a stack trace on all functions. Note this is only applicable for the "function" plugin tracer, and will only take
           effect if the -l option is used and succeeds in limiting functions. If the function tracer is not filtered, and the
           stack trace is enabled, you can live lock the machine.
```

## 这里有一堆的 function-graph 的 option ，也许可以让问题更加简单
```txt
[root@nixos:/sys/kernel/debug/tracing]# ls options/
annotate      disable_on_free     funcgraph-irqs      function-fork   markers          record-tgid      test_nop_refuse
bin           display-graph       funcgraph-overhead  function-trace  overwrite        sleep-time       trace_printk
blk_cgname    event-fork          funcgraph-overrun   graph-time      pause-on-trace   stacktrace       userstacktrace
blk_cgroup    fields              funcgraph-proc      hash-ptr        printk-msg-only  sym-addr         verbose
blk_classic   funcgraph-abstime   funcgraph-tail      hex             print-parent     sym-offset
block         funcgraph-cpu       func-no-repeats     irq-info        raw              sym-userobj
context-info  funcgraph-duration  func_stack_trace    latency-format  record-cmd       test_nop_accept
```

## TODO

Documentation/trace/ftrace.rst 中分析了几个非常关键的技术

```txt
preemptirqsoff
--------------

Knowing the locations that have interrupts disabled or
preemption disabled for the longest times is helpful. But
sometimes we would like to know when either preemption and/or
interrupts are disabled.
```
利用这个好好看看 ftrace 来如何分析内核，尤其是 preemption

什么时候才会产生 stacktrace ，之前还一直以为内核是不会产生 backtrace 的:
```txt
➜  tracing cat trace

# tracer: preemptirqsoff
#
# preemptirqsoff latency trace v1.1.5 on 6.7.0-rc8-dirty
# --------------------------------------------------------------------
# latency: 102 us, #4/4, CPU#28 | (M:desktop VP:0, KP:0, SP:0 HP:0 #P:32)
#    -----------------
#    | task: irqbalance-963 (uid:0 nice:0 policy:0 rt_prio:0)
#    -----------------
#  => started at: mnt_want_write
#  => ended at:   mnt_want_write
#
#
#                    _------=> CPU#
#                   / _-----=> irqs-off/BH-disabled
#                  | / _----=> need-resched
#                  || / _---=> hardirq/softirq
#                  ||| / _--=> preempt-depth
#                  |||| / _-=> migrate-disable
#                  ||||| /     delay
#  cmd     pid     |||||| time  |   caller
#     \   /        ||||||  \    |    /
irqbalan-963      28...1.    0us : mnt_want_write <-mnt_want_write
irqbalan-963      28...1.    0us!: mnt_want_write <-mnt_want_write
irqbalan-963      28...1.  103us+: tracer_preempt_on <-mnt_want_write
irqbalan-963      28...1.  117us : <stack trace>
 => path_openat
 => do_filp_open
 => do_sys_openat2
 => __x64_sys_openat
 => do_syscall_64
 => entry_SYSCALL_64_after_hwframe
```

https://hn.algolia.com/?dateRange=all&page=0&prefix=false&query=nvme&sort=byPopularity&type=story


这个案例不错
https://stackoverflow.com/questions/72476553/how-to-monitor-the-io-queue-depth


## 实现相关的文件，都是有  perf_event_open 对应的实现

- kernel/trace/trace_kprobe.c
- kernel/trace/trace_uprobe.c
- kernel/trace/trace_kprobe.c
- kernel/trace/trace_eprobe.c

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
