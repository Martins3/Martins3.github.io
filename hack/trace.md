## trace

<!-- vim-markdown-toc GitLab -->

- [总结各种 tracer : overview](#总结各种-tracer-overview)
- [关键问题 : 到底实现什么功能，以及不可以做什么](#关键问题-到底实现什么功能以及不可以做什么)
- [关键问题 : 可以做的事情](#关键问题-可以做的事情)
- [(tmp)branch trace](#tmpbranch-trace)
- [任务](#任务)
    - [需要询问的问题](#需要询问的问题)
    - [计划](#计划)
- [实现](#实现)
- [垃圾堆](#垃圾堆)
- [question](#question)
- [perf](#perf)
- [flamegraph](#flamegraph)
- [kprobe](#kprobe)
- [uprobe](#uprobe)
- [dtrace](#dtrace)
- [ftrace](#ftrace)
    - [debug/tracing 文件夹的内容理解](#debugtracing-文件夹的内容理解)
    - [ftrace-cmd](#ftrace-cmd)
- [kallsyms](#kallsyms)
- [eBPF](#ebpf)
- [BPF Performance Tools](#bpf-performance-tools)
- [bpftrace](#bpftrace)
- [bcc](#bcc)
    - [tutorial](#tutorial)
    - [tutorial bcc python developers](#tutorial-bcc-python-developers)
    - [reference guide](#reference-guide)
- [valgrind](#valgrind)
- [SystemTap](#systemtap)
- [dtrace](#dtrace-1)
- [lttng](#lttng)
- [Gprof2dot](#gprof2dot)
- [FlameGraph](#flamegraph-1)

<!-- vim-markdown-toc -->

## 总结各种 tracer : overview
http://www.brendangregg.com/blog/2015-07-08/choosing-a-linux-tracer.html
https://jvns.ca/blog/2017/07/05/linux-tracing-systems/

## 关键问题 : 到底实现什么功能，以及不可以做什么
- [ ] 依赖 kprobe 可以为所欲为的效果 : 比如在 open 的位置插入 printk，或者插入函数直接返回错误，造成所有的对于 syscall open 函数失败. ?
- [x] bcc 能不能插入多个 kprobe 并且将所有的数据整合。(应该很容易，可以使用相同的 map 直接在内核层次使用，或者让 python 处理)
- [ ] bcc 
- [ ] file:///home/shen/Core/linux/Documentation/output/trace/ftrace.html
- [ ] available_filter_functions : dynamic ftrace 的含义

> 记录一个小问题 :
[shen-pc tracing]# cat ksys_read > set_ftrace_filter 
cat: ksys_read: No such file or directory
[shen-pc tracing]# cat ksys_read > s^C_ftrace_filter 
[shen-pc tracing]# trace 'ksys_read'^C

## 关键问题 : 可以做的事情
- [ ] 各种机制的原理
    - [ ] uprobe 真的利用了 kprobe 吗 ?
    - [ ] 
- 所有的工具的功能的整理 : dtrace SystemTap 等



下面两个链接写的非常好，可以深入理解
- https://alex.dzyoba.com/blog/kernel-profiling/ 
- https://elinux.org/images/d/d7/Launet-kernel_profiling_debugging_tools_0.pdf


## (tmp)branch trace
2. /sys/kernel/debug/tracing 可以对应 branch_print_header 的输出内容啊

```c
static void branch_print_header(struct seq_file *s)
{
	seq_puts(s, "#           TASK-PID    CPU#    TIMESTAMP  CORRECT"
		    "  FUNC:FILE:LINE\n"
		    "#              | |       |          |         |   "
		    "    |\n");
}

static struct tracer branch_trace __read_mostly =
{
	.name		= "branch",
	.init		= branch_trace_init,
	.reset		= branch_trace_reset,
#ifdef CONFIG_FTRACE_SELFTEST
	.selftest	= trace_selftest_startup_branch,
#endif /* CONFIG_FTRACE_SELFTEST */
	.print_header	= branch_print_header,
};
```



## 任务

#### 需要询问的问题
- 这个任务的指标是什么，是否存在 deadline ?
- 这个还有其他有人处理吗 ?
- eBPF 如此强力的工具，居然无法使用，为什么要坚持 3.10 的版本上，别人的代码又不会入侵我的代码，而且不用合并到主线中间，想使用那个版本就使用那个版本，为什么不使用更高的版本。
- file:///home/shen/Core/linux/Documentation/output/admin-guide/perf/index.html perf 中间需要硬件的支持吗 ?
- https://en.wikipedia.org/wiki/Hardware_performance_counter 我需要知道硬件计数器的文章在哪里 ?


可以完成的简单的事情:
- 需要确保，我们使用的是同一个 unixbench 的内容，阅读一下 unixbench 的内容，似乎 perl 需要阅读一下
- http://www.loongson.cn/index.html : 1课时的性能分析可以阅读一下
- 看来的确是存在性能计数器的: 用户手册 LPMP 

#### 计划

## 实现


## 垃圾堆
1. kernel/trace 中间的内容到底是干啥的 ?

[^9] 讲述一些基本的内容:
1. strace 实现的原理是什么 ?
2. sampling profiler 指的是 ?

分为三个部分: data source，mechanisms for collecting data for those sources，tracing frontends
// TODO ftrace 和 kprobe 的关系是什么 ?
// TODO 把那本书找到，好吧，存在两本书的内容


// TODO papi 是啥 ?
// TODO perf 是不是存在两个，一个测量操作系统，一个测量硬件的 ?

// 这个人写过一堆内容
https://github.com/NanXiao/perf-little-book

// 有一个 BPF，而且似乎写的还不错的东西
https://facebookmicrosites.github.io/bpf/


## question
1. 所以，kernel/events 和 kernel/trace 的关系是什么 ?


## perf 
TODO
http://www.brendangregg.com/blog/2015-02-27/linux-profiling-at-netflix.html
1. 为什么 perf 工具如何 flamegraph 配合使用的
2. perf 可以做到什么功能 ?

## flamegraph 
参考这个功能:
http://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html


## kprobe
// BCC 的 第一个例子 kprobe 可以检查整个内核中间的fork，为什么可以 ?

LWN 的这个文章还不错哦 [^4]。

KProbes is a debugging mechanism for the Linux kernel which can also be used for **monitoring events** inside a production system.

You can use it to **weed out performance bottlenecks**, **log specific events**, **trace problems** etc.

DProbes adds a number of features, including its own scripting language for the writing of probe handlers. However, only KProbes has been merged into the standard kernel.

KProbes heavily depends on processor architecture specific features and uses slightly different mechanisms depending on the architecture on which it's being executed.

A kernel probe is a set of handlers placed on a certain instruction address.
> 那么 kernel probe 是可以放到任何指令的位置吗 ?

A kernel probe is a set of handlers placed on a certain instruction address. There are two types of probes in the kernel as of now, called "KProbes" and "JProbes." A KProbe is defined by a pre-handler and a post-handler. When a KProbe is installed at a particular instruction and that instruction is executed, the pre-handler is executed just before the execution of the probed instruction. Similarly, the post-handler is executed just after the execution of the probed instruction. JProbes are used to get access to a kernel function's arguments at runtime. A JProbe is defined by a JProbe handler with the same prototype as that of the function whose arguments are to be accessed. When the probed function is executed the control is first transferred to the user-defined JProbe handler, followed by the transfer of execution to the original function. The KProbes package has been designed in such a way that tools for debugging, tracing and logging could be built by extending it.
> 两种类型 : KProbe 和 JProbe，KProbe 可以在指令的前后执行，而JProbe 可以获取内核参数，然后执行，最后回到原来的函数

Most of the handling of the probes is done in the context of the breakpoint and the debug exception handlers which make up the KProbes architecture dependent layer.
The KProbes architecture independent layer is the KProbes manager which is used to register and unregister probes. Users provide probe handlers in kernel modules which register probes through the KProbes manager.
> 划分为三个层次: 用户提供 handler，architecture independent layer 注册，architecture dependent 执行

Users can insert their own probe inside a running kernel by writing a kernel module which implements the pre-handler and the post-handler for the probe.

This spinlock is locked before a new probe is registered, an existing probe is unregistered or when a probe is hit.
This prevents these operations from executing simultaneously on a SMP machine.
Whenever a probe is hit, the probe handler is called with interrupts disabled. Interrupts are disabled because handling a probe is a **multiple step process** which involves breakpoint handling and single-step execution of the probed instruction. There is no easy way to save the state between these operations hence interrupts are kept disabled during probe handling.

关键的问题是 : 
1. 当执行到某一个指令的时候，我怎么知道我需要开始进行 kprobe 的部分了。
2. 按照 JProbe 的想法，我怎么知道，这个函数开始需要执行 JProbe。
3. 凭什么 eBPF 可以关联到 KProbe 上。


**A JProbe leverages the mechanism used by a KProbe.**
Instead of calling a user-defined pre-handler a JProbe specifies its own pre-handler called setjmp_pre_handler() and uses another handler called a break_handler. This is a three-step process.

Kprobe 的实现参考 register_kprobe 的内容，采用的方法应该将原有指令替换掉，然后写入 int3，然后让 int3 开始执行 handler 

> 至于 jprobe 在哪里，我们是不知道的


## uprobe
不知道为什么，uprobe.c 被放到 kernel/events/ 下面了。

- [ ] uprobe 利用的是 kprobe 的基础


## dtrace

## ftrace
总体的教程 : 直接在 debugfs 上的操作，然后 trace-cmd，最后图形化的 kernelshark
lwn 关于 ftrace 的介绍 [^10]
trace-cmd作为ftrace的前端，kernel shark作为trace-cmd的前端 [^7]


The name ftrace comes from "function tracer", which was its original purpose, but it can do more than that. Various additional tracers have been added to look at things like context switches, how long interrupts are disabled, how long it takes for high-priority tasks to run after they have been woken up, and so on. Its genesis in the realtime tree is evident in the tracers so far available, but ftrace also includes a plugin framework that allows new tracers to be added easily.
> 起源，ftrace 的功能后来逐渐增加，并且提供框架

// TODO 为什么感觉所有的内容都是可以挂载在 debug 下面的

观察一下 /sys/kernel/debug/tracing 的README

https://jvns.ca/blog/2017/03/19/getting-started-with-ftrace/

> ftrace 就像是打印函数调用路径，但是远远不止于此:

https://elinux.org/images/6/64/Elc2011_rostedt.pdf : kernelshark 使用介绍


对的，ftrace 下的内容就是:
https://www.kernel.org/doc/html/latest/trace/ftrace.html : 讲解了如何使用
2. Although ftrace is typically considered the function tracer, it is really a framework of several assorted tracing utilities.
1. One of the most common uses of ftrace is the event tracing. Throughout the kernel is hundreds of static event points that can be enabled via the tracefs file system to see what is going on in certain parts of the kernel.

After mounting tracefs you will have access to the control and output files of ftrace. Here is a list of some of the key files:
1. current_tracer available_tracers
```
[shen-pc tracing]# cat available_tracers
hwlat blk mmiotrace function_graph wakeup_dl wakeup_rt wakeup function nop
```
2. This tracer is similar to the function tracer except that it probes a function on its entry and its exit.
This is done by using a dynamically allocated stack of return addresses in each `task_struct`. On function entry the tracer overwrites the return address of each function traced to set a custom probe. 
Thus the original return address is stored on the stack of return address in the `task_struct`.
> 似乎合乎想象，在内核到处插入 checkpoint，然后从这些 checkpoint 找到需要知道
3. Note, the proc sysctl ftrace_enable is a big on/off switch for the function tracer. By default it is enabled (when function tracing is enabled in the kernel). If it is disabled, all function tracing is disabled. This includes not only the function tracers for ftrace, but also for any other uses (perf, kprobes, stack tracing, profiling, etc).


- [ ] event tracing 如何使用的 ?
- [ ] ftrace.c 和 trace.c 的各自的作用是什么?  ftrace 提供 function 相关的东西, available_filter_functions 之类的 ，trace 提供整个框架，其实 ftrace 变成了一个无所不包的东西了。
- [ ] kernel dynamic 和 kernel static tracing 指的是 ? 猜测，有的是动态插入，利用 kprobe 和 uprobe 的技术插入代码中间，而 static 指的是各种地方插入的 tracing 的内容。
- [ ] ftrace function (/sys/kernel/debug/tracing/available_filter_functions 采用的) 利用 mcount 实现的效果和 kprobe 有什么区别吗 ?


```c
static const struct file_operations ftrace_avail_fops = {
	.open = ftrace_avail_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release_private,
};

static const struct file_operations ftrace_enabled_fops = {
	.open = ftrace_enabled_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release_private,
};

static const struct file_operations ftrace_filter_fops = {
	.open = ftrace_filter_open,
	.read = seq_read,
	.write = ftrace_filter_write,
	.llseek = tracing_lseek,
	.release = ftrace_regex_release,
};

static const struct file_operations ftrace_notrace_fops = {
	.open = ftrace_notrace_open,
	.read = seq_read,
	.write = ftrace_notrace_write,
	.llseek = tracing_lseek,
	.release = ftrace_regex_release,
};

static __init int ftrace_init_dyn_tracefs(struct dentry *d_tracer)
{

	trace_create_file("available_filter_functions", 0444,
			d_tracer, NULL, &ftrace_avail_fops);

	trace_create_file("enabled_functions", 0444,
			d_tracer, NULL, &ftrace_enabled_fops);

	ftrace_create_filter_files(&global_ops, d_tracer);

#ifdef CONFIG_FUNCTION_GRAPH_TRACER
	trace_create_file("set_graph_function", 0644, d_tracer,
				    NULL,
				    &ftrace_graph_fops);
	trace_create_file("set_graph_notrace", 0644, d_tracer,
				    NULL,
				    &ftrace_graph_notrace_fops);
#endif /* CONFIG_FUNCTION_GRAPH_TRACER */

	return 0;
}
```

- https://lwn.net/Articles/370423/

#### debug/tracing 文件夹的内容理解
> 依靠那个文件实现 kprobe 和 uprobe 的功能 ?

- [ ] tracing_on
- [ ] available_tracers current_tracer: 1. 除了 function 和 function_graph 之外，其他的 tracer 的功能
- [ ] set_ftrace_filter set_ftrace_notrace  available_filter_functions : 确认一下前面两个就是为了给最后一个进行筛选的


#### ftrace-cmd

https://lwn.net/Articles/410200/ 的记录


- [ ] 两个用户层次的前端工具 : perf + trace-cmd 分别读取的内容是从哪里的呀 ?
- [ ] trace-cmd 

1. ftrace-cmd 的 record 原理: 为每一个 cpu 创建出来一个 process，读取 /sys/kernel/debug/tracing/per_cpu/cpu0，最后主函数将所有的 concatenate 起来
2. -e 可以选择一个 trace event 或者一个 subsystem 的 trace point。The -e option enables an event. The argument following the -e can be an event name, event subsystem name, or the special name all.
3. 四种功能 : Ftrace also has special plugin tracers that do not simply trace specific events. These tracers include the function, function graph, and latency tracers.
    1. trace function 指的是 ?
    2. current_tracer 可以放置的内容那么多，为什么只有这几个 plugin
    3. 当 latency tracer 和 perf 的关系是什么 ?
4. 解释下面的程序:
    1. -p 添加 function plugin
    2. -e 选择需要被 trace 的 events
    3. The `-l` option is the same as echoing its argument into `set_ftrace_filter`, and the `-n` option is the same as echoing its argument into `set_ftrace_notrace`.
```c
trace-cmd record -p function -l 'sched_*' -n 'sched_slice'
```
5. 解释下面的程序: TODO 应该是采用，
```
 trace-cmd record -p function_graph -l do_IRQ -e irq_handler_entry sleep 10
```


> TODO : 到底 dynamic ftrace 是什么 ?


> 让我们解决一个小问题，为什么没有办法写入到 set_ftrace_filter
https://superuser.com/questions/287371/obtain-kernel-config-from-currently-running-linux-system

> 引出了一个小问题:
tracing_on 和 /proc/sys/kernel/ftrace_enabled 分别表示什么 ?


## kallsyms
1. 如何构建表格的
2. 表格的格式是什么
3. 查询的方法
> 只有 700 行，你值得拥有

## eBPF

- [ ] [Dive into BPF: a list of reading material](https://qmonnet.github.io/whirl-offload/2016/09/01/dive-into-bpf/) : may be read all articles of the author

为什么使用eBPF ?[^1]
3. 为了向内核中间添加功能，如果修改kernel source code，需要等到用户更新内核。如果使用kernel module，每次内核升级，都需要发布对应的kernel module.
1. eBPF 是 100% modular and composable 的
2. eBPF 可以实现 hotpatching 

1. safety & security : verifier 保证内核中间发
2. continuous delivery : 程序可以动态修改
3. performance : JIT compiler ensures native execution performance (为什么JIT可以实现 native f)

bpf 可以加入的位置 : kprobe uprobe syscall fentry/fexit, some network related stuff

eBPF map 的作用，eBPF helper ，function calls 

eBPF 的升级内容:[^2]
1. 64bit 的寄存器
2. 寄存器数量从2个到10个
3. BPF_CALL : Plus, a new BPF_CALL instruction made it possible to call in-kernel functions cheaply.
4. The ease of mapping eBPF to native instructions lends itself to just-in-time compilation, yielding improved performance.

eBPF 的作用不仅仅限于 packet filter 的功能，其实可以动态的 debug 内核:
eBPF is also useful for debugging the kernel and carrying out performance analysis; 
programs can be attached to tracepoints, kprobes, and perf events.
Because eBPF programs can access kernel data structures, developers can write and test new debugging code without having to recompile the kernel. The implications are obvious for busy engineers debugging issues on live, running systems. It's even possible to use eBPF to debug user-space programs by using Userland Statically Defined Tracepoints.


eBPF verifier :
1. 对于CFG进行DFS，保证其中不会出现递归，死循环，以及不可执行的代码
2. 对于每条指令都进行模拟执行，保证程序的执行总是正常的
3. secure mode 下，不可以使用指针运算
4. Registers with uninitialized contents (those that have never been written to) cannot be read;
5. *The contents of registers R0-R5 are marked as unreadable across functions calls by storing a special value to catch any reads of an uninitialized register.* 什么叫做，R0 R5 之间
6. Similar checks are done for reading variables on the stack and to make sure that no instructions write to the read-only frame-pointer register.

Lastly, the verifier uses the eBPF program type (covered later) to restrict which kernel functions can be called from eBPF programs and which data structures can be accessed.


```c
int bpf(int cmd, union bpf_attr *attr, unsigned int size);
```
1. The `bpf_attr` union allows data to be passed between the kernel and user space;
2. the exact format depends on the `cmd` argument. 
3. The `size` argument gives the size of the bpf_attr union object in bytes.

cmd 类型包括:
1. 修改用于eBPF程序和kernel或者user space 沟通的 eBPF map
2. 将 eBPF 附着于特定的位置(socket file descriptor)

Though there appear to be many different commands, they can be broken down into three categories:
1. commands for working with eBPF programs,
2. working with eBPF maps, 
3. or commands for working with both programs and maps (collectively known as objects).

eBPF map : Each map is defined by four values: a type, a maximum number of elements, a value size in bytes, and a key size in bytes.

如何使用 BPF:
1. 使用 Clang -march=bpf 编译 或者手动写汇编代码
2. samples/bpf/ 提供了很多测试程序 
3. libpf 库 For example, the high-level flow of an eBPF program and user program using libbpf might go something like:
  - Read the eBPF bytecode into a buffer in your user application and pass it to bpf_load_program().
  - The eBPF program, when run by the kernel, will call bpf_map_lookup_elem() to find an element in a map and store a new value in it.
  - The user application calls bpf_map_lookup_elem() to read out the value stored by the eBPF program in the kernel.
这些测试程序的问题在于，需要使用 bpf 程序需要在 kernel source tree 中间编译，BCC 处理掉这个问题。

eBPF 的关键工具 : BCC 介绍了基本使用规则 [^3]
The project consists of a toolchain for writing, compiling, and loading eBPF programs, along with example programs and battle-hardened tools for debugging and diagnosing performance issues.

eBPF 更多的使用 : BCC 介绍处理用户层的代码 [^4]
// TODO 上面的代码操作一下

// TODO 关于 BPF 的问题在于，还是无法理解为什么实现监控

// TODO 一些高级话题 : [^6]

brendangregg 写的关于 eBPF 的内容: [^12]
1. eBPF 的消息来源:
kprobes: kernel dynamic tracing.
uprobes: user level dynamic tracing.
tracepoints: kernel static tracing.
perf_events: timed sampling and PMCs.

2. eBPF 将获取到的消息导出用户层的方法:
The BPF program has two ways to pass measured data back to user space: either per-event details, or via a BPF map. BPF maps can implement arrays, associative arrays, and histograms, and are suited for passing summary statistics.

3. 如何利用 bcc 进行编程:
// TODO 挺有意思的东西
http://www.brendangregg.com/ebpf.html#frontends

## BPF Performance Tools
也许分析其中的目录就是非常足够的吧，知道一共存在什么东西!
https://search.safaribooksonline.com/book/operating-systems-and-server-administration/linux/9780136588870

## bpftrace
建立在 bcc 上方的易用工具，手动编译真简单呀!
https://github.com/iovisor/bpftrace 

bcc 也提供了各种工具，包括 trace argdist 以及 funccount 等等

到底可以实现什么 ?


## bcc


#### tutorial

Before using bcc, you should start with the Linux basics. One reference is the [Linux Performance Analysis in 60s](http://techblog.netflix.com/2015/11/linux-performance-analysis-in-60s.html) post, which covers these commands:

1. uptime
1. dmesg | tail
1. vmstat 1
1. mpstat -P ALL 1
1. pidstat 1
1. iostat -xz 1
1. free -m
1. sar -n DEV 1
1. sar -n TCP,ETCP 1
1. top

> TODO 传统的分析工具都是从 /proc 中间读取数据的

分析各种 domain specific 工具:
1. runqlat
2. profile 暂时不知道如何使用

然后分析三个 generic 的工具:
argdist
trace : 
funccount

> TODO


#### tutorial bcc python developers
kprobe uprobe USDT SDT perf trace 等等，但是其实过于强调其中的

https://github.com/iovisor/bcc/blob/master/docs/tutorial_bcc_python_developer.md
      |
     \|/
记录一下问题:
1. 为什么 bpf 不能直接访问，而是需要这种封装函数
```c
    data.pid = bpf_get_current_pid_tgid();
    data.ts = bpf_ktime_get_ns();
    bpf_get_current_comm(&data.comm, sizeof(data.comm));
```
2. 这些 macro 的含义是什么，一共存在多少种这种东西。
```c
BPF_HASH(last);
BPF_PERF_OUTPUT(events); // 感觉非常神奇，似乎是 perf 提供特地的通道
```

3. uprobe 的例子非常玄乎，一个测试完全合乎例子，一个没有测试

#### reference guide
C 语言总是需要 lua, cpp 或者 python 的辅助，将 eBPF 程序插入到其中其中，划分为 BPF C 和 bcc python

1. 插入的位置
2. 输出数据
3. 从内核中间获取数据
4. MAPS 为什么发要定义这么多的类型

python : 各种 attach 函数， 分析 map 以及输出

## valgrind
https://stackoverflow.com/questions/5134891/how-do-i-use-valgrind-to-find-memory-leaks
> 1. valgrind 除了可以检查 mem leak，还可以做什么事情 ?
> 2. 实现的原理是什么

## SystemTap
> 文档清晰，但是安装并不简单呀
> 实现的方法不知道

https://www.jianshu.com/p/84b3885aa8cb

## dtrace


## lttng
https://lttng.org/docs/

## Gprof2dot
https://github.com/jrfonseca/gprof2dot
> 非常有意思，其可以 read output from 的项目，并不是每一个我都知道啊!

## FlameGraph
至今不知道如何使用，这是用户层 perf 的前端吗 ?


[^1]: [Outlook : future of eBPF](https://docs.google.com/presentation/d/1AcB4x7JCWET0ysDr0gsX-EIdQSTyBtmi6OAW7bE0jm0/preview?slide=id.g704abb5039_2_106)
[^2]: [A thorough introduction to eBPF](https://lwn.net/Articles/740157/)
[^3]: [An introduction to the BPF Compiler Collection](https://lwn.net/Articles/742082/)
[^4]: [An introduction to KProbes](https://lwn.net/Articles/132196/)
[^5]: [Using user-space tracepoints with BPF](https://lwn.net/Articles/753601/)
[^6]: [Some advanced BCC topics](https://lwn.net/Articles/747640/)
[^7]: [kernelshark](https://www.cnblogs.com/arnoldlu/p/9014365.html)
[^8]: [Linux Performance](http://www.brendangregg.com/linuxperf.html)
[^9]: [Linux tracing systems & how they fit together](https://jvns.ca/blog/2017/07/05/linux-tracing-systems/)
[^10]: [lwn : A look at ftrace](https://lwn.net/Articles/322666/)
[^11]: [perf tutorial](https://perf.wiki.kernel.org/index.php/Tutorial)
[^12]: [Linux Extended BPF (eBPF) Tracing Tools](http://www.brendangregg.com/ebpf.html)

