## 关键参考 trace
- https://leezhenghui.github.io/linux/2019/03/05/exploring-usdt-on-linux.html
  - [ ] USDT 到底是如何工作的
- [ ] uprobe 是如何工作的

## perf
1. kernel/events/core.c 中间定义了 `perf_event_open`
2. 输出放到 buffer ring 中间

```c
/*
 * Callers need to ensure there can be no nesting of this function, otherwise
 * the seqlock logic goes bad. We can not serialize this because the arch
 * code calls this from NMI context.
 */
void perf_event_update_userpage(struct perf_event *event)
```

## 基本原理
参考 [^9]
- kprobe / uprobe
- tracepoint
  - [ ] tracepoint 相对 kprobe/uprobe 来说存在什么优点吗?
- MPC

导出的方法
- bpf
- ftrace
- `perf_event_open`

- 那些基于 stack 的操作是怎么搞出来的，例如 flamegraph 的
- [ ] 所以 bpf 是不是只是因为更加容易插入代码了而已
  - 应该也是更加安全吧
- [ ] uprobe 真的利用了 kprobe 吗 ?

- [ ] 将书上的内容整理一下

## 关键问题 : 到底实现什么功能，以及不可以做什么
- [ ] `available_filter_functions` : dynamic ftrace 的含义

> 记录一个小问题 :
[shen-pc tracing]# cat ksys_read > set_ftrace_filter
cat: ksys_read: No such file or directory
[shen-pc tracing]# cat ksys_read > s^C_ftrace_filter
[shen-pc tracing]# trace 'ksys_read'^C

## 关键问题 : 可以做的事情
- 所有的工具的功能的整理 : dtrace SystemTap 等

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

## 问题
1. kernel/trace 中间的内容到底是干啥的 ?
1. 所以，kernel/events 和 kernel/trace 的关系是什么 ?

## perf

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

Kprobe 的实现参考 `register_kprobe` 的内容，采用的方法应该将原有指令替换掉，然后写入 int3，然后让 int3 开始执行 handler

> 至于 jprobe 在哪里，我们是不知道的

## uprobe
不知道为什么，uprobe.c 被放到 kernel/events/ 下面了。

- [ ] uprobe 利用的是 kprobe 的基础

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

[^1] https://jvns.ca/blog/2016/03/12/how-does-perf-work-and-some-questions/
