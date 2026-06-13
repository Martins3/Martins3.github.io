# ftrace 实现
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
- kprobe / uprobe
- tracepoint
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


## kernel/trace/

```c
/*
 * Note, ftrace_ops can be referenced outside of RCU protection, unless
 * the RCU flag is set. If ftrace_ops is allocated and not part of kernel
 * core data, the unregistering of it will perform a scheduling on all CPUs
 * to make sure that there are no more users. Depending on the load of the
 * system that may take a bit of time.
 *
 * Any private data added must also take care not to be freed and if private
 * data is added to a ftrace_ops that is in core code, the user of the
 * ftrace_ops must perform a schedule_on_each_cpu() before freeing it.
 */
struct ftrace_ops {
	ftrace_func_t			func;
	struct ftrace_ops __rcu		*next;
	unsigned long			flags;
	void				*private;
	ftrace_func_t			saved_func;
#ifdef CONFIG_DYNAMIC_FTRACE
	struct ftrace_ops_hash		local_hash;
	struct ftrace_ops_hash		*func_hash;
	struct ftrace_ops_hash		old_hash;
	unsigned long			trampoline;
	unsigned long			trampoline_size;
	struct list_head		list;
	ftrace_ops_func_t		ops_func;
#ifdef CONFIG_DYNAMIC_FTRACE_WITH_DIRECT_CALLS
	unsigned long			direct_call;
#endif
#endif
};
```


### kernel/trace/bpf_trace.c

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


### [x] kernel/trace/trace_hwlat.c
- Documentation/trace/hwlat_detector.rst : 基本没有人用


### [ ] kernel/trace/trace_irqsoff.c

### [ ] kernel/trace/trace_stat.c


## [ ] tracepoint 的代码实现在什么地方


## [ ] 如何查找 backtrace

- Gregg's bcc book 分析过 backtrace 的麻烦之处
- https://github.com/bombela/backward-cpp

## [ ] raw_syscalls 和 syscalls 的差别是什么?
raw_syscalls 是什么意思?

/sys/kernel/tracing//events/raw_syscalls/sys_(enter|exit)

回答这个问题: https://unix.stackexchange.com/questions/324361/difference-between-syscalls-and-raw-syscalls-events

## 为什么这个函数无法 trace 啊 trace_kprobe
这是特地标注的，
```txt
[  323.274009] trace_kprobe: Could not probe notrace function sg_miter_stop
[  323.274155] trace_kprobe: Could not probe notrace function sg_miter_stop
[  361.337420] trace_kprobe: Could not probe notrace function sg_miter_stop
[  361.337512] trace_kprobe: Could not probe notrace function sg_miter_stop
[  379.112269] trace_kprobe: Could not probe notrace function sg_miter_skip
[  379.112361] trace_kprobe: Could not probe notrace function sg_miter_skip
```

## 到底是 int3 还是四个 nop 指令，感觉有好几种说法
```txt
crash> dis ktime_get
0xffffffff82105a30 <ktime_get>: nopl   0x0(%rax,%rax,1) [FTRACE NOP]
0xffffffff82105a35 <ktime_get+5>:       push   %rbp
0xffffffff82105a36 <ktime_get+6>:       mov    0xc54e68(%rip),%eax        # 0xffffffff82d5a8a4
0xffffffff82105a3c <ktime_get+12>:      mov    %rsp,%rbp
0xffffffff82105a3f <ktime_get+15>:      push   %r14
0xffffffff82105a41 <ktime_get+17>:      test   %eax,%eax
0xffffffff82105a43 <ktime_get+19>:      push   %r13
0xffffffff82105a45 <ktime_get+21>:      push   %r12
0xffffffff82105a47 <ktime_get+23>:      push   %rbx
```

实际上，我现在可以看到:
```txt
$ disass ktime_get
Dump of assembler code for function ktime_get:
   0xffffffff81202e40 <+0>:     endbr64
   0xffffffff81202e44 <+4>:     call   0xffffffff811116b0 <__fentry__>
   0xffffffff81202e49 <+9>:     mov    0x1f2aabd(%rip),%eax        # 0xffffffff8312d90c <timekeeping_suspended>
   0xffffffff81202e4f <+15>:    push   %r12
   0xffffffff81202e51 <+17>:    push   %rbp
   0xffffffff81202e52 <+18>:    push   %rbx
   0xffffffff81202e53 <+19>:    test   %eax,%eax
   0xffffffff81202e55 <+21>:    jne    0xffffffff81202ed3 <ktime_get+147>
   0xffffffff81202e57 <+23>:    xor    %ebp,%ebp
   0xffffffff81202e59 <+25>:    mov    0x2a99320(%rip),%r12d        # 0xffffffff83c9c180 <tk_core>
   0xffffffff81202e60 <+32>:    test   $0x1,%r12b
   0xffffffff81202e64 <+36>:    jne    0xffffffff81202ecf <ktime_get+143>
   0xffffffff81202e66 <+38>:    mov    0x2a9931b(%rip),%rdi        # 0xffffffff83c9c188 <tk_core+8>
   0xffffffff81202e6d <+45>:    mov    0x2a9933c(%rip),%rbx        # 0xffffffff83c9c1b0 <tk_core+48>
   0xffffffff81202e74 <+52>:    mov    (%rdi),%rax
   0xffffffff81202e77 <+55>:    call   0xffffffff82282ce0 <__x86_indirect_thunk_array>
   0xffffffff81202e7c <+60>:    mov    0x2a9930d(%rip),%rdx        # 0xffffffff83c9c190 <tk_core+16>
   0xffffffff81202e83 <+67>:    sub    0x2a9930e(%rip),%rax        # 0xffffffff83c9c198 <tk_core+24>
   0xffffffff81202e8a <+74>:    mov    0x2a99317(%rip),%rdi        # 0xffffffff83c9c1a8 <tk_core+40>
   0xffffffff81202e91 <+81>:    mov    0x2a9930d(%rip),%ecx        # 0xffffffff83c9c1a4 <tk_core+36>
   0xffffffff81202e97 <+87>:    and    %rdx,%rax
   0xffffffff81202e9a <+90>:    shr    %rdx
   0xffffffff81202e9d <+93>:    not    %rdx
   0xffffffff81202ea0 <+96>:    test   %rax,%rdx
   0xffffffff81202ea3 <+99>:    mov    0x2a992f7(%rip),%edx        # 0xffffffff83c9c1a0 <tk_core+32>
   0xffffffff81202ea9 <+105>:   cmovne %rbp,%rax
   0xffffffff81202ead <+109>:   mov    0x2a992cd(%rip),%esi        # 0xffffffff83c9c180 <tk_core>
   0xffffffff81202eb3 <+115>:   cmp    %esi,%r12d
   0xffffffff81202eb6 <+118>:   jne    0xffffffff81202e59 <ktime_get+25>
   0xffffffff81202eb8 <+120>:   imul   %rax,%rdx
   0xffffffff81202ebc <+124>:   lea    (%rdx,%rdi,1),%rax
   0xffffffff81202ec0 <+128>:   shr    %cl,%rax
   0xffffffff81202ec3 <+131>:   add    %rbx,%rax
   0xffffffff81202ec6 <+134>:   pop    %rbx
   0xffffffff81202ec7 <+135>:   pop    %rbp
   0xffffffff81202ec8 <+136>:   pop    %r12
   0xffffffff81202eca <+138>:   jmp    0xffffffff82283344 <__x86_return_thunk>
   0xffffffff81202ecf <+143>:   pause
   0xffffffff81202ed1 <+145>:   jmp    0xffffffff81202e59 <ktime_get+25>
   0xffffffff81202ed3 <+147>:   ud2
   0xffffffff81202ed5 <+149>:   jmp    0xffffffff81202e57 <ktime_get+23>
End of assembler dump.
```
ftrace 和 tracepoint 应该是分别都插入点的，ftrace 要求所有的函数头都插入点，而 tracepoint 在函数中间。

## 代码分析
- kernel/trace/ftrace.c
```c
static __init int ftrace_init_dyn_tracefs(struct dentry *d_tracer)
```

## [x] function_graph 如何实现的

参考 Documentation/trace/ftrace.rst

> This is done by using a dynamically allocated stack of return addresses in each `task_struct`.
On function entry the tracer overwrites the return address of each function traced to set a custom probe.
Thus the original return address is stored on the stack of return address in the `task_struct`.


实现的代码 : kernel/trace/fgraph.c

- https://mp.weixin.qq.com/s/Nr_UY-_T9usHltug1v00xw
- https://stackoverflow.com/questions/7290131/how-does-gccs-pg-flag-work-in-relation-to-profilers
  - 基本分析其原理
- https://stackoverflow.com/questions/72445665/linux-ftrace-why-code-profiling-is-achieved-through-mcount-function-gcc-pg
  - 现代的操作系统中，并不是使用的 mcount 来实现的

```c
struct ftrace_ret_stack *task_struct::ret_stack;
```

使用 ftrace 来实现分析，操作方法
```sh
	echo chrdev_show >set_graph_function
	echo 1 >tracing_on
	echo function_graph >current_tracer
  cat /proc/devices
	cat trace
```

```txt
#0  ftrace_graph_ignore_func (trace=0xffffc900001e4de4) at kernel/trace/trace.h:994
#1  trace_graph_entry (trace=0xffffc900001e4de4) at kernel/trace/trace_functions_graph.c:164
#2  0xffffffff812af11d in function_graph_enter (ret=18446744071580395770, func=func@entry=18446744071579168804, frame_pointer=frame_pointer@entry=0, retp=retp@entry=0xffffc900001e4f08) at kernel/trace/fgraph.c:146
#3  0xffffffff81113153 in prepare_ftrace_return (ip=18446744071579168804, parent=0xffffc900001e4f08, frame_pointer=0) at arch/x86/kernel/ftrace.c:649
#4  0xffffffffc000209b in ?? ()
#5  0xffff88810aa46180 in ?? ()
#6  0xffffffff8119968e in update_cfs_rq_load_avg (cfs_rq=0x0 <fixed_percpu_data>, now=0) at kernel/sched/fair.c:4628
#7  update_load_avg (cfs_rq=0x0 <fixed_percpu_data>, se=0xffffffff8322fc50 <pvclock_gtod_notifier>, flags=-2079740128) at kernel/sched/fair.c:4741
#8  0x00000000ffffffff in ?? ()
#9  0xffffffff8409af20 in ?? ()
#10 0x0000000000000000 in ?? ()
```
正常的环境中，没有任何程序会打开 cat /proc/devices 的，但是使用 gdb 还是可以观测到上述的 backtrace ，可见，
实际上是将所有的函数的入口全部打开了，只是在 function_graph_enter 中会进行判断时不时应该进行跟踪。

实现递归的 ftrace 的方法就很粗暴，设置上 TRACE_GRAPH_NOTRACE_BIT 的标记之后，之后就不会跟踪，
没有标记之后，之后的函数都会跟踪。

```c
int trace_graph_entry(struct ftrace_graph_ent *trace)
{
	struct trace_array *tr = graph_array;
	struct trace_array_cpu *data;
	unsigned long flags;
	unsigned int trace_ctx;
	long disabled;
	int ret;
	int cpu;

	if (trace_recursion_test(TRACE_GRAPH_NOTRACE_BIT))
		return 0;

	/*
	 * Do not trace a function if it's filtered by set_graph_notrace.
	 * Make the index of ret stack negative to indicate that it should
	 * ignore further functions.  But it needs its own ret stack entry
	 * to recover the original index in order to continue tracing after
	 * returning from the function.
	 */
	if (ftrace_graph_notrace_addr(trace->func)) {
		trace_recursion_set(TRACE_GRAPH_NOTRACE_BIT);
		/*
		 * Need to return 1 to have the return called
		 * that will clear the NOTRACE bit.
		 */
		return 1;
	}
```

顺带说下 kernel/trace/trace_functions_graph.c 中的内容，大多数都是如何解析法
```txt
#0  print_graph_duration (flags=268437142, s=0xffff888105bb2098, duration=0,
    tr=<optimized out>) at kernel/trace/trace_functions_graph.c:601
#1  print_graph_entry_nested (cpu=<optimized out>, flags=1686,
    s=0xffff888105bb2098, entry=0xffffc9000225bcc4, iter=0xffff888105bb0000)
    at kernel/trace/trace_functions_graph.c:762
#2  print_graph_entry (field=field@entry=0xffffc9000225bcc4,
    s=s@entry=0xffff888105bb2098, iter=iter@entry=0xffff888105bb0000,
    flags=flags@entry=1686) at kernel/trace/trace_functions_graph.c:945
#3  0xffffffff812ab1b1 in print_graph_function_flags (iter=0xffff888105bb0000,
    flags=1686) at kernel/trace/trace_functions_graph.c:1151
#4  0xffffffff8129851a in print_trace_line (iter=0xffff888105bb0000)
    at kernel/trace/trace.c:4590
#5  0xffffffff812994ad in s_show (m=0xffff8881119df000, v=0xffff888105bb0000)
    at kernel/trace/trace.c:4747
#6  0xffffffff8149e0c8 in seq_read_iter (iocb=iocb@entry=0xffffc9000225be18,
    iter=iter@entry=0xffffc9000225bdf0) at fs/seq_file.c:272
#7  0xffffffff8149e3af in seq_read (file=<optimized out>, buf=<optimized out>,
    size=<optimized out>, ppos=0xffffc9000225bef8) at fs/seq_file.c:162
#8  0xffffffff81466e5c in vfs_read (file=file@entry=0xffff8881070fd800,
    buf=buf@entry=0x7efc756db000 <error: Cannot access memory at address 0x7efc756db000>, count=18446744071616008224, count@entry=131072,
    pos=pos@entry=0xffffc9000225bef8) at fs/read_write.c:474
#9  0xffffffff81467f2f in ksys_read (fd=<optimized out>,
    buf=0x7efc756db000 <error: Cannot access memory at address 0x7efc756db000>,
    count=131072) at fs/read_write.c:619
#10 0xffffffff824088aa in do_syscall_x64 (nr=<optimized out>,
    regs=0xffffc9000225bf58) at arch/x86/entry/common.c:52
#11 do_syscall_64 (regs=0xffffc9000225bf58, nr=<optimized out>)
    at arch/x86/entry/common.c:83
#12 0xffffffff826000eb in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```


## [x] function profile 是如何实现的

是的，当理解了 function graph 的原理，只是将所有的函数的执行都记录一下耗时而已。


## [x] 所有的 tracer 存在一个共同的模板，用于抽象输出，配置 current_tracer 的行为之类的


```c
static struct tracer graph_trace __tracer_data = {
	.name		= "function_graph",
	.update_thresh	= graph_trace_update_thresh,
	.open		= graph_trace_open,
	.pipe_open	= graph_trace_open,
	.close		= graph_trace_close,
	.pipe_close	= graph_trace_close,
	.init		= graph_trace_init,
	.reset		= graph_trace_reset,
	.print_line	= print_graph_function,
	.print_header	= print_graph_headers,
	.flags		= &tracer_flags,
	.set_flag	= func_graph_set_flag,
#ifdef CONFIG_FTRACE_SELFTEST
	.selftest	= trace_selftest_startup_function_graph,
#endif
};
```

## tracing 的 trigger 的实现

```c
static struct ftrace_func_command ftrace_traceon_cmd = {
	.name			= "traceon",
	.func			= ftrace_trace_onoff_callback,
};

static struct ftrace_func_command ftrace_traceoff_cmd = {
	.name			= "traceoff",
	.func			= ftrace_trace_onoff_callback,
};

static struct ftrace_func_command ftrace_stacktrace_cmd = {
	.name			= "stacktrace",
	.func			= ftrace_stacktrace_callback,
};

static struct ftrace_func_command ftrace_dump_cmd = {
	.name			= "dump",
	.func			= ftrace_dump_callback,
};

static struct ftrace_func_command ftrace_cpudump_cmd = {
	.name			= "cpudump",
	.func			= ftrace_cpudump_callback,
};
```
## CONFIG_DYNAMIC_FTRACE

当将这个选项关闭之后:

```diff
🧀  diff .config .config.old
515a516
> # CONFIG_LIVEPATCH is not set
743a745
> CONFIG_KPROBES_ON_FTRACE=y
3670a3673
> # CONFIG_HID_BPF is not set
5383a5387,5388
> CONFIG_HAVE_FUNCTION_GRAPH_TRACER=y
> CONFIG_HAVE_FUNCTION_GRAPH_RETVAL=y
5395a5401
> CONFIG_BUILDTIME_MCOUNT_SORT=y
5409c5415,5421
< # CONFIG_DYNAMIC_FTRACE is not set
---
> CONFIG_FUNCTION_GRAPH_TRACER=y
> # CONFIG_FUNCTION_GRAPH_RETVAL is not set
> CONFIG_DYNAMIC_FTRACE=y
> CONFIG_DYNAMIC_FTRACE_WITH_REGS=y
> CONFIG_DYNAMIC_FTRACE_WITH_DIRECT_CALLS=y
> CONFIG_DYNAMIC_FTRACE_WITH_ARGS=y
> CONFIG_FPROBE=y
5426a5439
> CONFIG_FPROBE_EVENTS=y
5427a5441
> # CONFIG_KPROBE_EVENTS_ON_NOTRACE is not set
5432a5447,5448
> CONFIG_FTRACE_MCOUNT_RECORD=y
> CONFIG_FTRACE_MCOUNT_USE_CC=y
5441a5458
> # CONFIG_FTRACE_SORT_STARTUP_TEST is not set
```
这三个功能都消失了:
1. CONFIG_FPROBE=y
2. CONFIG_KPROBES_ON_FTRACE
3. CONFIG_HAVE_FUNCTION_GRAPH_TRACER

```txt
config DYNAMIC_FTRACE
	bool "enable/disable function tracing dynamically"
	depends on FUNCTION_TRACER
	depends on HAVE_DYNAMIC_FTRACE
	default y
	help
	  This option will modify all the calls to function tracing
	  dynamically (will patch them out of the binary image and
	  replace them with a No-Op instruction) on boot up. During
	  compile time, a table is made of all the locations that ftrace
	  can function trace, and this table is linked into the kernel
	  image. When this is enabled, functions can be individually
	  enabled, and the functions not enabled will not affect
	  performance of the system.

	  See the files in /sys/kernel/tracing:
	    available_filter_functions
	    set_ftrace_filter
	    set_ftrace_notrace

	  This way a CONFIG_FUNCTION_TRACER kernel is slightly larger, but
	  otherwise has native performance as long as no tracing is active.
```
的确， available_filter_functions set_ftrace_filter set_ftrace_notrace 文件都消失了，
可以使用 ftrace ，但是只能全部打开，或者全部关闭。

## ftrace.c 和 trace.c 各自的作用的是什么?
- [ ] ftrace.c 和 trace.c 的各自的作用是什么?  ftrace 提供 function 相关的东西, available_filter_functions 之类的 ，trace 提供整个框架，其实 ftrace 变成了一个无所不包的东西了。

## tracepoint 的实现简单分析

其中 xchg   %ax,%ax 就是 nop 指令:

```c
void this(void){
	trace_me_silly(0, 0);
}

int x;
int own(void){
	int i = 0;
	i += x;
	trace_me_silly(0, 0);
	i += x;
	return i;
}
```
当打开 tracepoint 的时候，是无需使用 int3 的，比较怀疑是直接的跳转就可以了。

```txt
Dump of assembler code for function this:
   0xffffffff81d7a870 <+0>:     endbr64
   0xffffffff81d7a874 <+4>:     xchg   %ax,%ax
   0xffffffff81d7a876 <+6>:     jmp    0xffffffff821f3544 <__x86_return_thunk>
   0xffffffff81d7a87b <+11>:    mov    %gs:0x7e2b198a(%rip),%eax        # 0x2c20c <pcpu_hot+12>
   0xffffffff81d7a882 <+18>:    mov    %eax,%eax
   0xffffffff81d7a884 <+20>:    bt     %rax,0x1199ac4(%rip)        # 0xffffffff82f14350 <__cpu_online_mask>
   0xffffffff81d7a88c <+28>:    jae    0xffffffff81d7a8b7 <this+71>
   0xffffffff81d7a88e <+30>:    incl   %gs:0x7e2b1973(%rip)        # 0x2c208 <pcpu_hot+8>
   0xffffffff81d7a895 <+37>:    mov    0x11786e4(%rip),%rax        # 0xffffffff82ef2f80 <__tracepoint_me_silly+64>
   0xffffffff81d7a89c <+44>:    test   %rax,%rax
   0xffffffff81d7a89f <+47>:    je     0xffffffff81d7a8ae <this+62>
   0xffffffff81d7a8a1 <+49>:    mov    0x8(%rax),%rdi
   0xffffffff81d7a8a5 <+53>:    xor    %edx,%edx
   0xffffffff81d7a8a7 <+55>:    xor    %esi,%esi
   0xffffffff81d7a8a9 <+57>:    call   0xffffffff821f6dc8 <__SCT__tp_func_me_silly>
   0xffffffff81d7a8ae <+62>:    decl   %gs:0x7e2b1953(%rip)        # 0x2c208 <pcpu_hot+8>
   0xffffffff81d7a8b5 <+69>:    je     0xffffffff81d7a8bc <this+76>
   0xffffffff81d7a8b7 <+71>:    jmp    0xffffffff821f3544 <__x86_return_thunk>
   0xffffffff81d7a8bc <+76>:    call   0xffffffff821f41f0 <__SCT__preempt_schedule_notrace>
   0xffffffff81d7a8c1 <+81>:    jmp    0xffffffff821f3544 <__x86_return_thunk>
```

```txt
di$ disass own
Dump of assembler code for function own:
   0xffffffff81d7a8e0 <+0>:     endbr64
   0xffffffff81d7a8e4 <+4>:     push   %rbx
   0xffffffff81d7a8e5 <+5>:     mov    0x1b9a2bd(%rip),%ebx        # 0xffffffff83914ba8 <x>
   0xffffffff81d7a8eb <+11>:    xchg   %ax,%ax
   0xffffffff81d7a8ed <+13>:    mov    %ebx,%eax
   0xffffffff81d7a8ef <+15>:    add    %ebx,%eax
   0xffffffff81d7a8f1 <+17>:    pop    %rbx
   0xffffffff81d7a8f2 <+18>:    jmp    0xffffffff821f3544 <__x86_return_thunk>
   0xffffffff81d7a8f7 <+23>:    mov    %gs:0x7e2b190e(%rip),%eax        # 0x2c20c <pcpu_hot+12>
   0xffffffff81d7a8fe <+30>:    mov    %eax,%eax
   0xffffffff81d7a900 <+32>:    bt     %rax,0x1199a48(%rip)        # 0xffffffff82f14350 <__cpu_online_mask>
   0xffffffff81d7a908 <+40>:    jae    0xffffffff81d7a933 <own+83>
   0xffffffff81d7a90a <+42>:    incl   %gs:0x7e2b18f7(%rip)        # 0x2c208 <pcpu_hot+8>
   0xffffffff81d7a911 <+49>:    mov    0x1178668(%rip),%rax        # 0xffffffff82ef2f80 <__tracepoint_me_silly+64>
   0xffffffff81d7a918 <+56>:    test   %rax,%rax
   0xffffffff81d7a91b <+59>:    je     0xffffffff81d7a92a <own+74>
   0xffffffff81d7a91d <+61>:    mov    0x8(%rax),%rdi
   0xffffffff81d7a921 <+65>:    xor    %edx,%edx
   0xffffffff81d7a923 <+67>:    xor    %esi,%esi
   0xffffffff81d7a925 <+69>:    call   0xffffffff821f6dc8 <__SCT__tp_func_me_silly>
   0xffffffff81d7a92a <+74>:    decl   %gs:0x7e2b18d7(%rip)        # 0x2c208 <pcpu_hot+8>
   0xffffffff81d7a931 <+81>:    je     0xffffffff81d7a941 <own+97>
   0xffffffff81d7a933 <+83>:    mov    0x1b9a26f(%rip),%eax        # 0xffffffff83914ba8 <x>
   0xffffffff81d7a939 <+89>:    add    %ebx,%eax
   0xffffffff81d7a93b <+91>:    pop    %rbx
   0xffffffff81d7a93c <+92>:    jmp    0xffffffff821f3544 <__x86_return_thunk>
   0xffffffff81d7a941 <+97>:    call   0xffffffff821f41f0 <__SCT__preempt_schedule_notrace>
   0xffffffff81d7a946 <+102>:   jmp    0xffffffff81d7a933 <own+83>
End of assembler dump.
```

## fork 之后为什么要初始化这个
perf_event_init_task

## 将 ftrace 的各种 sample

- samples/trace_printk/ : 就是 trace_printk 如何使用
- samples/trace_events/ : 介绍如何创建出来 tracepoint 来
- samples/ftrace/
  - samples/ftrace/ftrace-ops.c : 如何在注册 struct ftrace_ops
  - samples/ftrace/ftrace-direct.c : 一个函数如何直接调用 ftrace 注册 hook ，测试 register_ftrace_direct 的使用
  - samples/ftrace/ftrace-direct-multi.c : 这个 ftrace_ops 如何同时监控多个
  - samples/ftrace/sample-trace-array.c : 有趣的
- samples/fprobe/fprobe_example.c
- samples/kprobes
  - samples/kprobes/kprobe_example.c
  - samples/kprobes/kretprobe_example.c

sample-trace-array.c 的测试中 需要打开这个 tracepoint ，然后可以看到，这里的 array 是什么意思?

```txt
 sample-instance-3463    [001] .....  6088.851942: sample_event: count value=369 at jiffies=4300755968
```

fprobe_example.c
```txt
[ 6535.873251] sample_entry_handler: Enter <kernel_clone+0x4/0x430> ip = 0x00000000e9b74bc7
[ 6535.874026 +0x4/0x430> ip = 0x00000000e9b74bc7                         fprobe_handler+0x113/0x220
[ 6535.874545]                          0xffffffffc07660de
[ 6535.875057]                          kernel_clone+0x9/0x430
[ 6535.875550]                          __do_sys_clone+0x66/0x90
[ 6535.876103]                          do_syscall_64+0xbc/0x210
[ 6535.876612]                          entry_SYSCALL_64_after_hwframe+0x77/0x7f
[ 6535.877340] sample_exit_handler: Return from <kernel_clone+0x4/0x430> ip = 0x00000000e9b74bc7 to rip = 0x00000000e75778bc (__do_sys_clone+0x66/0x90)
[ 6535.880113]                          rethook_trampoline_handler+0x8b/0x120
[ 6535.880568] sample_entry_handler: Enter <kernel_clone]
```

现在更加迷茫了，那么 samples/kprobes/kprobe_example.c 和 ftrace_ops ，和 fprobe 有什么区别吗?

register_kprobe 和 register_fprobe_syms 似乎一个级别的，而 ftrace_ops 在他们的上面的

## 如何实现过滤的

kernel/trace/trace_events_trigger.c


## 调查一下这个: kernel/trace/trace_events_user.c

user 这个功能如何实现的?

## 这个对应什么功能来着?
kernel/trace/trace_dynevent.c


## 原来 ring buffer 也是一个重要的功能哦
Documentation/trace/ring-buffer-design.rst
Documentation/trace/ring-buffer-map.rst

在 6.16 中提到了用户态的 ringbuffer ，不知道支
https://lwn.net/Articles/1023075/

# Linux 内核 Trace 子系统文件组织分析
<!-- 7260a4b3-884a-4f66-9bd2-045a9eaf10e1 -->

> 分析对象：`kernel/trace/` 目录下的源代码文件
> 内核版本：基于 Linux 6.x 源码分析

## 为什么 ftrace 让人感觉复杂

ftrace 复杂性源于其**多层架构交叉混合**的设计。它不是一个单一工具，而是一个**庞大的 tracing 生态系统**。

### 1. 多层架构

| 层次 | 功能 | 对应文件 |
|------|------|----------|
| 基础设施层 | Ring buffer、时间戳、时钟 | `ring_buffer.c`, `trace_clock.c` |
| 底层机制层 | ftrace hook、function tracer | `ftrace.c`, `fgraph.c`, `rethook.c` |
| Probe 抽象层 | 统一 probe 接口 | `trace_probe.c` |
| 具体 Probe 实现 | kprobe/uprobe/eprobe/fprobe | `trace_kprobe.c`, `trace_uprobe.c`, `trace_eprobe.c`, `trace_fprobe.c` |
| Event 系统层 | Trace event 注册、过滤、触发器 | `trace_events.c`, `trace_events_filter.c`, `trace_events_trigger.c` |
| Tracer 实现层 | 各种 tracer（function、irqsoff、graph 等） | `trace_functions.c`, `trace_functions_graph.c`, `trace_irqsoff.c` |
| 用户接口层 | tracefs、输出格式化 | `trace.c`, `trace_output.c`, `trace_seq.c` |

### 2. 术语混淆

| 术语 | 实际含义 |
|------|----------|
| **ftrace** | 狭义指函数跟踪基础设施（`ftrace.c`）；广义指整个 tracing 系统 |
| **trace event** | 基于 tracepoint 的静态事件系统 |
| **probe** | 动态插入的探测点（kprobe/uprobe/fprobe） |
| **tracer** | 实现特定跟踪功能的模块（如 function_graph、irqsoff） |

---

## 文件功能划分详解

### 第一层：核心基础设施

| 文件             | 功能说明                                                  | 代码规模  |
|------------------|-----------------------------------------------------------|-----------|
| `ring_buffer.c`  | 通用的环形缓冲区实现（非 trace 专用，可被其他子系统使用） | ~7,787 行 |
| `trace_clock.c`  | 各种时钟源（local/global/x86-tsc 等）                     | ~4,320 行 |
| `trace_seq.c`    | trace 输出序列化工具                                      | ~1,529 行 |
| `trace_output.c` | 各种 entry 类型的输出格式化                               | ~1,896 行 |

### 第二层：ftrace 底层机制（Function Tracing 核心）

| 文件        | 功能说明                                                                                                                    | 代码规模  |
|-------------|-----------------------------------------------------------------------------------------------------------------------------|-----------|
| `ftrace.c`  | **核心**：gcc `-pg` 插桩基础设施<br>• mcount 处理<br>• ftrace_ops 管理<br>• 代码动态修改（nop → call）<br>• 过滤、hash 管理 | ~9,033 行 |
| `fgraph.c`  | Function graph tracer 核心逻辑<br>• 函数调用图（entry/return）<br>• shadow stack 管理                                       | ~4,146 行 |
| `rethook.c` | 通用 return hook 机制                                                                                                       | ~1,563 行 |
| `fprobe.c`  | fprobe 底层库（基于 fgraph 的简化 probe 接口）                                                                              | ~2,492 行 |

### 第三层：Probe 抽象与实现

#### 3.1 Probe 公共代码

| 文件 | 功能说明 | 代码规模 |
|------|----------|----------|
| `trace_probe.c` | **抽象层**：probe 公共代码<br>• 参数解析、字段定义<br>• 数据定位（data_loc）<br>• BTF 参数解析支持 | ~2,279 行 |

#### 3.2 具体 Probe 实现

| 文件             | 功能说明                                          | 代码规模  |
|------------------|---------------------------------------------------|-----------|
| `trace_kprobe.c` | kprobe-based 动态事件（内核函数 probe）           | ~2,198 行 |
| `trace_uprobe.c` | uprobe-based 动态事件（用户态函数 probe）         | ~1,710 行 |
| `trace_eprobe.c` | event probe（基于其他 event 触发的 probe）        | ~2,317 行 |
| `trace_fprobe.c` | fprobe-based 动态事件（新，更简单的函数级 probe） | ~1,589 行 |

### 第四层：Event 系统（静态 Tracepoints）

| 文件                     | 功能说明                                                                                  | 代码规模  |
|--------------------------|-------------------------------------------------------------------------------------------|-----------|
| `trace_events.c`         | **核心**：Event 注册/管理<br>• 定义 `TRACE_EVENT()` 宏基础设施<br>• event 与 tracefs 集成 | ~4,902 行 |
| `trace_events_filter.c`  | Event 过滤表达式（如 `pid==1234`）                                                        | ~2,924 行 |
| `trace_events_trigger.c` | Event 触发器（stacktrace, enable 等）                                                     | ~1,941 行 |
| `trace_events_hist.c`    | Histogram 触发器（直方图统计）                                                            | ~7,027 行 |
| `trace_events_synth.c`   | 合成事件（synthetic events）                                                              | ~2,363 行 |
| `trace_events_user.c`    | 用户态定义事件（user events）                                                             | ~2,933 行 |
| `trace_event_perf.c`     | Event 与 perf 集成                                                                        | ~1,259 行 |

### 第五层：各种 Tracer 实现

| 文件                      | 功能说明                                       |
|---------------------------|------------------------------------------------|
| `trace_functions.c`       | function tracer（最基础的函数跟踪）            |
| `trace_functions_graph.c` | function graph tracer（带耗时信息的调用图）    |
| `trace_irqsoff.c`         | irqsoff/preemptoff tracer（中断/抢占关闭时间） |
| `trace_sched_switch.c`    | 调度切换 tracer                                |
| `trace_sched_wakeup.c`    | 唤醒延迟 tracer                                |
| `trace_hwlat.c`           | 硬件延迟 tracer                                |
| `trace_osnoise.c`         | OS 噪声 tracer                                 |
| `trace_stack.c`           | 栈使用 tracer                                  |
| `trace_mmiotrace.c`       | MMIO tracer                                    |
| `trace_branch.c`          | 分支预测 tracer                                |
| `trace_syscalls.c`        | 系统调用 tracer                                |

### 第六层：用户接口与控制

| 文件               | 功能说明                                                                                                               | 代码规模   |
|--------------------|------------------------------------------------------------------------------------------------------------------------|------------|
| `trace.c`          | **主控制**：tracefs 主要接口<br>• `/sys/kernel/tracing/` 创建<br>• `tracing_on`、`buffer_size` 等控制<br>• tracer 切换 | ~11,787 行 |
| `trace_dynevent.c` | dynamic_events 接口<br>• `kprobe_events`/`uprobe_events` 统一入口                                                      | ~1,369 行  |
| `trace_stat.c`     | trace 统计信息                                                                                                         | ~761 行    |
| `trace_printk.c`   | `trace_printk()` 实现                                                                                                  | ~910 行    |

### 其他辅助文件

| 文件                       | 功能说明            |
|----------------------------|---------------------|
| `blktrace.c`               | 块设备 IO trace     |
| `bpf_trace.c`              | BPF 与 trace 集成   |
| `trace_boot.c`             | 启动时 tracing 配置 |
| `pid_list.c`               | PID 过滤列表管理    |
| `trace_recursion_record.c` | 递归调用记录        |

---

## 关键头文件说明

| 文件                | 功能说明                                |
|---------------------|-----------------------------------------|
| `trace.h`           | 核心数据结构定义（~74KB，最大的头文件） |
| `trace_probe.h`     | probe 公共数据结构（~20KB）             |
| `trace_dynevent.h`  | 动态事件接口定义                        |
| `trace_output.h`    | 输出格式化接口                          |
| `ftrace_internal.h` | ftrace 内部接口                         |
| `trace_entries.h`   | 各种 trace entry 类型定义               |
| `tracing_map.h`     | histogram 使用的哈希映射                |

---

## Probe 机制的分层结构

```
┌────────────────────────────────────────────────────────────┐
│  用户接口层：trace_dynevent.c (dynamic_events 文件)         │
│  • 统一解析用户输入的命令                                    │
│  • 分发到具体的 probe 类型                                   │
├────────────────────────────────────────────────────────────┤
│  具体 probe 实现：                                           │
│    • trace_kprobe.c (内核函数 probe)                        │
│    • trace_uprobe.c (用户态函数 probe)                      │
│    • trace_eprobe.c (基于现有 event 的 probe)               │
│    • trace_fprobe.c (基于 fprobe 的 probe)                  │
├────────────────────────────────────────────────────────────┤
│  公共代码：trace_probe.c                                     │
│  • 参数解析、字段定义、数据定位                               │
├────────────────────────────────────────────────────────────┤
│  底层机制：                                                  │
│    • kprobes (arch/*/kernel/kprobes.c + kernel/kprobes.c)   │
│    • uprobes (kernel/events/uprobes.c)                      │
│    • fprobe (kernel/trace/fprobe.c)                         │
└────────────────────────────────────────────────────────────┘
```

## 与 perf 的关系
<!-- 7bc21e10-96fe-4d4e-a995-67ed3754be31 -->

`trace_kprobe.c` 等文件**同时支持**两种使用方式：

1. **ftrace 路径**：通过 tracefs 的 `kprobe_events` 文件
2. **perf 路径**：通过 `perf_event_open()` 系统调用

```c
// trace_kprobe.c 中的关键判断
if (tp->flags & TP_FLAG_TRACE)      // ftrace 模式
    trace_probe_trace_enter(...);
if (tp->flags & TP_FLAG_PROFILE)    // perf 模式
    perf_trace_buf_submit(...);
```

这里有一个典型函数:
```c
static int kprobe_dispatcher(struct kprobe *kp, struct pt_regs *regs)
{
	struct trace_kprobe *tk = container_of(kp, struct trace_kprobe, rp.kp);
	unsigned int flags = trace_probe_load_flag(&tk->tp);
	int ret = 0;

	raw_cpu_inc(*tk->nhit);

	if (flags & TP_FLAG_TRACE)
		kprobe_trace_func(tk, regs);
#ifdef CONFIG_PERF_EVENTS
	if (flags & TP_FLAG_PROFILE)
		ret = kprobe_perf_func(tk, regs);
#endif
	return ret;
}
```

## 简化理解框架

```
┌─────────────────────────────────────────────────────────────┐
│                     用户访问接口                            │
│         tracefs (/sys/kernel/tracing/)                      │
│         perf_event_open()                                   │
├─────────────────────────────────────────────────────────────┤
│                      控制层                                 │
│    trace.c (全局控制)   trace_events.c (event 管理)         │
│    trace_dynevent.c (动态事件)                              │
├─────────────────────────────────────────────────────────────┤
│                      功能层                                 │
│  ┌──────────────┬──────────────┬─────────────────────────┐  │
│  │   Tracers    │    Events    │       Probes            │  │
│  │  (各种       │  (trace_     │  (kprobe/uprobe/        │  │
│  │   tracer)    │   events*.c) │   eprobe/fprobe)        │  │
│  └──────────────┴──────────────┴─────────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                      基础设施                               │
│    ftrace.c (函数hook)    ring_buffer.c (缓冲区)            │
│    fgraph.c (图跟踪)       trace_probe.c (probe抽象)        │
└─────────────────────────────────────────────────────────────┘
```

---

## 常见误区澄清

### 误区 1：ftrace.c 是 tracefs 接口

**纠正**：`ftrace.c` 是**底层函数插桩基础设施**，不是 tracefs 接口：
- 处理 `mcount` 调用
- 管理 ftrace_ops（注册/注销/过滤）
- 运行时修改代码（nop → call → nop）

tracefs 接口主要在 `trace.c` 和 `trace_events.c` 中。

### 误区 2：probe 和 tracer 是同一层次的概念

**纠正**：
- **Probe** 是**动态探测机制**（在运行时在任意位置插入探测点）
- **Tracer** 是**功能实现**（利用 probe 或其他机制实现特定跟踪功能）

### 误区 3：所有 trace 文件都是 ftrace 的一部分

**纠正**：
- `ring_buffer.c` 是通用基础设施，不只服务于 trace
- `trace_events.c` 主要是静态 tracepoint 系统
- 只有 `ftrace.c`、`fgraph.c` 等才是真正的 "ftrace" 核心

## 总结

ftrace 的复杂性来源于其功能的丰富性：
- 同时支持**静态 tracepoints** 和**动态 probes**
- 同时支持**函数级跟踪**和**调用图跟踪**
- 同时支持**事件过滤**和**直方图统计**
- 同时支持**tracefs 接口**和**perf 接口**
- 同时支持**内核态**和**用户态**跟踪

理解这些文件的分层关系，有助于在使用时快速定位问题所在的层次。

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
