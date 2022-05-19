## trace

<!-- vim-markdown-toc GitLab -->

* [[ ] libbpf](#-libbpf)
* [总结各种 tracer : overview](#总结各种-tracer-overview)
* [关键问题 : 到底实现什么功能，以及不可以做什么](#关键问题-到底实现什么功能以及不可以做什么)
* [关键问题 : 可以做的事情](#关键问题-可以做的事情)
* [(tmp)branch trace](#tmpbranch-trace)
* [垃圾堆](#垃圾堆)
* [question](#question)
* [perf](#perf)
* [flamegraph](#flamegraph)
* [kprobe](#kprobe)
* [uprobe](#uprobe)
* [dtrace](#dtrace)
* [ftrace](#ftrace)
    * [debug/tracing 文件夹的内容理解](#debugtracing-文件夹的内容理解)
    * [ftrace-cmd](#ftrace-cmd)
* [valgrind](#valgrind)
* [gprof2dot](#gprof2dot)
* [SystemTap](#systemtap)
* [dtrace](#dtrace-1)
* [ltrace](#ltrace)
* [[ ] uftrace](#-uftrace)
* [lttng](#lttng)
* [[ ] sysdig](#-sysdig)
* [usdt](#usdt)
* [kdump](#kdump)
* [[ ] QEMU 中的 trace](#-qemu-中的-trace)
  * [log](#log)

<!-- vim-markdown-toc -->


- [ ] https://oprofile.sourceforge.io/about/
- [ ] https://github.com/jrfonseca/gprof2dot
  - 这个工具是被我们使用上了，但是本身是一个将各种 perf 结果生成新的结果的工具，可以看看原来的结果的位置
- https://github.com/Netflix/flamescope : FlameScope is a visualization tool for exploring different time ranges as Flame Graphs
- https://en.algorithmica.org/ : 基于现代硬件的算法，对于 cache miss branch predictor 都是有考虑的

- https://github.com/opcm/pcm

- [ ] bpftrace 和 bcc 的关系是什么?
- [ ] ftrace-cmd 的功能都可以使用 bcc 替代吗?
- https://leezhenghui.github.io/linux/2019/03/05/exploring-usdt-on-linux.html
  - 总结的很全面
- [ ] BCC 可以取代 ftrace 吗?

## [ ] libbpf
https://pingcap.com/blog/why-we-switched-from-bcc-to-libbpf-for-linux-bpf-performance-analysis

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
  .name   = "branch",
  .init   = branch_trace_init,
  .reset    = branch_trace_reset,
#ifdef CONFIG_FTRACE_SELFTEST
  .selftest = trace_selftest_startup_branch,
#endif /* CONFIG_FTRACE_SELFTEST */
  .print_header = branch_print_header,
};
```

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
// BCC 的 第一个例子 kprobe 可以检查整个内核中间的 fork，为什么可以 ?

LWN 的这个文章还不错哦 [^4]。

KProbes is a debugging mechanism for the Linux kernel which can also be used for **monitoring events** inside a production system.

You can use it to **weed out performance bottlenecks**, **log specific events**, **trace problems** etc.

DProbes adds a number of features, including its own scripting language for the writing of probe handlers. However, only KProbes has been merged into the standard kernel.

KProbes heavily depends on processor architecture specific features and uses slightly different mechanisms depending on the architecture on which it's being executed.

A kernel probe is a set of handlers placed on a certain instruction address.
> 那么 kernel probe 是可以放到任何指令的位置吗 ?

A kernel probe is a set of handlers placed on a certain instruction address. There are two types of probes in the kernel as of now, called "KProbes" and "JProbes." A KProbe is defined by a pre-handler and a post-handler. When a KProbe is installed at a particular instruction and that instruction is executed, the pre-handler is executed just before the execution of the probed instruction. Similarly, the post-handler is executed just after the execution of the probed instruction. JProbes are used to get access to a kernel function's arguments at runtime. A JProbe is defined by a JProbe handler with the same prototype as that of the function whose arguments are to be accessed. When the probed function is executed the control is first transferred to the user-defined JProbe handler, followed by the transfer of execution to the original function. The KProbes package has been designed in such a way that tools for debugging, tracing and logging could be built by extending it.
> 两种类型 : KProbe 和 JProbe，KProbe 可以在指令的前后执行，而 JProbe 可以获取内核参数，然后执行，最后回到原来的函数

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
trace-cmd 作为 ftrace 的前端，kernel shark 作为 trace-cmd 的前端 [^7]


The name ftrace comes from "function tracer", which was its original purpose, but it can do more than that. Various additional tracers have been added to look at things like context switches, how long interrupts are disabled, how long it takes for high-priority tasks to run after they have been woken up, and so on. Its genesis in the realtime tree is evident in the tracers so far available, but ftrace also includes a plugin framework that allows new tracers to be added easily.
> 起源，ftrace 的功能后来逐渐增加，并且提供框架

// TODO 为什么感觉所有的内容都是可以挂载在 debug 下面的

观察一下 /sys/kernel/debug/tracing 的 README

https://jvns.ca/blog/2017/03/19/getting-started-with-ftrace/

> ftrace 就像是打印函数调用路径，但是远远不止于此:

https://elinux.org/images/6/64/Elc2011_rostedt.pdf : kernelshark 使用介绍


对的，ftrace 下的内容就是:
https://www.kernel.org/doc/html/latest/trace/ftrace.html : 讲解了如何使用
2. Although ftrace is typically considered the function tracer, it is really a framework of several assorted tracing utilities.
1. One of the most common uses of ftrace is the event tracing. Throughout the kernel is hundreds of static event points that can be enabled via the tracefs file system to see what is going on in certain parts of the kernel.

After mounting tracefs you will have access to the control and output files of ftrace. Here is a list of some of the key files:
1. current_tracer available_tracers
```plain
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
```plain
 trace-cmd record -p function_graph -l do_IRQ -e irq_handler_entry sleep 10
```

> TODO : 到底 dynamic ftrace 是什么 ?


> 让我们解决一个小问题，为什么没有办法写入到 set_ftrace_filter
https://superuser.com/questions/287371/obtain-kernel-config-from-currently-running-linux-system

> 引出了一个小问题:
tracing_on 和 /proc/sys/kernel/ftrace_enabled 分别表示什么 ?


## [valgrind](http://valgrind.org/)
使用上很简单:
https://stackoverflow.com/questions/5134891/how-do-i-use-valgrind-to-find-memory-leaks

## [gprof2dot](https://github.com/jrfonseca/gprof2dot)

## SystemTap
> 文档清晰，但是安装并不简单呀
> 实现的方法不知道

https://www.jianshu.com/p/84b3885aa8cb

## dtrace

## ltrace
library call trace

## [ ] uftrace
https://github.com/namhyung/uftrace

## lttng
https://lttng.org/docs/

https://lttng.org/

yaourt -s lttng-tools lttng-ust lttng-modules

第三个的安装似乎并不简单，出现了大量的错误，于是采用手动安装，在第二步骤会出现错误，类似于
https://github.com/umlaeute/v4l2loopback/issues/139，**虽然完全不知道为什么**
但是这个东西大概是可以使用的。

其分析居然还要使用一个 : https://babeltrace.org/

## [ ] sysdig

https://github.com/draios/sysdig

```txt
$ sysdig -l
```

1. fd :
2. process : 不仅仅可以从进程的描述符，程序名称等，还可以使用
3. user group
4. evt :
5. syslog
6. container k8s mesos
7. evtin
8. span

## usdt
sudo apt install systemtap-sdt-dev

## kdump
when linux crashed, kexec and kdump will dump kernel memory to vmcore
https://github.com/crash-utility/crash

## [ ] QEMU 中的 trace
- [ ] 测试一下, 添加新的 Trace 的方法

- [ ]  为什么会支持各种 backend, 甚至有的 dtrace 的内容?

- [ ] 这里显示的 trace 都是做什么的 ?
```plain
➜  vn git:(master) ✗ qemu-system-x86_64 -trace help
```

- [ ] 例如这些文件:
/home/maritns3/core/qemu/hw/i386/trace-events

### log
util/log.c 定义所有的 log, 其实整个机制是很容易的

- asm_in : accel/tcg/translator.c::translator_loop
- asm_out : tcg/translate-all.c::tb_gen_code

[^4]: [An introduction to KProbes](https://lwn.net/Articles/132196/)
[^5]: [Using user-space tracepoints with BPF](https://lwn.net/Articles/753601/)
[^7]: [kernelshark](https://www.cnblogs.com/arnoldlu/p/9014365.html)
[^8]: [Linux Performance](http://www.brendangregg.com/linuxperf.html)
[^9]: [Linux tracing systems & how they fit together](https://jvns.ca/blog/2017/07/05/linux-tracing-systems/)
[^10]: [lwn : A look at ftrace](https://lwn.net/Articles/322666/)
[^11]: [perf tutorial](https://perf.wiki.kernel.org/index.php/Tutorial)
