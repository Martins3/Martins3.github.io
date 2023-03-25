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

#### debug/tracing 文件夹的内容理解
> 依靠那个文件实现 kprobe 和 uprobe 的功能 ?

- [ ] tracing_on
- [ ] available_tracers current_tracer: 1. 除了 function 和 function_graph 之外，其他的 tracer 的功能
- [ ] set_ftrace_filter set_ftrace_notrace  available_filter_functions : 确认一下前面两个就是为了给最后一个进行筛选的

[^1] https://jvns.ca/blog/2016/03/12/how-does-perf-work-and-some-questions/



### 这种操作的基础是什么
https://lore.kernel.org/lkml/YmU2izhF0HDlgbrW@casper.infradead.org/T/
```c
root@pepe-kvm:~# mkfs.xfs /dev/sdb
root@pepe-kvm:~# mount /dev/sdb /mnt/
root@pepe-kvm:~# truncate -s 10G /mnt/bigfile
root@pepe-kvm:~# echo 1 >/sys/kernel/tracing/events/filemap/mm_filemap_add_to_page_cache/enable
root@pepe-kvm:~# dd if=/mnt/bigfile of=/dev/null bs=100K count=4
root@pepe-kvm:~# cat /sys/kernel/tracing/trace
```

### 简单看看 kernel/trace/bpf_trace.c 的实现

```txt
#0  kprobe_prog_func_proto (func_id=BPF_FUNC_get_current_pid_tgid, prog=0xffffc90001563000) at kernel/trace/bpf_trace.c:1525
#1  0xffffffff8124b6fb in check_helper_call (env=env@entry=0xffff888219a68000, insn=insn@entry=0xffffc90001563060, insn_idx_p=insn_idx_p@entry=0xffff888219a68000) at kernel/bpf/verifier.c:7724
#2  0xffffffff8124fd21 in do_check (env=0x3 <fixed_percpu_data+3>) at kernel/bpf/verifier.c:13970
#3  do_check_common (env=env@entry=0xffff888219a68000, subprog=subprog@entry=0) at kernel/bpf/verifier.c:16289
#4  0xffffffff8125470e in do_check_main (env=0xffff888219a68000) at kernel/bpf/verifier.c:16352
#5  bpf_check (prog=prog@entry=0xffffc90001e17d50, attr=attr@entry=0xffffc90001e17e58, uattr=...) at kernel/bpf/verifier.c:16936
#6  0xffffffff81235397 in bpf_prog_load (attr=attr@entry=0xffffc90001e17e58, uattr=...) at kernel/bpf/syscall.c:2619
#7  0xffffffff81236dfb in __sys_bpf (cmd=5, uattr=..., size=120) at kernel/bpf/syscall.c:4979
#8  0xffffffff81238c65 in __do_sys_bpf (size=<optimized out>, uattr=<optimized out>, cmd=<optimized out>) at kernel/bpf/syscall.c:5083
#9  __se_sys_bpf (size=<optimized out>, uattr=<optimized out>, cmd=<optimized out>) at kernel/bpf/syscall.c:5081
#10 __x64_sys_bpf (regs=<optimized out>) at kernel/bpf/syscall.c:5081
#11 0xffffffff81fc8d08 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001e17f58) at arch/x86/entry/common.c:50
#12 do_syscall_64 (regs=0xffffc90001e17f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#13 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
- kprobe_prog_func_proto
  - bpf_tracing_func_proto
    - bpf_base_func_proto : 这里在 kernel/bpf/helper 的位置了
