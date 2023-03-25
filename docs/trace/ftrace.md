## 资料
```txt
[root@nixos:/sys/kernel/debug/tracing]# cat available_tracers
blk function_graph wakeup_dl wakeup_rt wakeup function nop
```
- 可以勉强读读的内容:
  - https://static.lwn.net/images/conf/rtlws11/papers/proc/p02.pdf

这个是存在具体代码的跟踪的:
- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/08/05/tracing-basic

总体的教程 : 直接在 debugfs 上的操作，然后 trace-cmd，最后图形化的 kernelshark

## 分析下这个
- CONFIG_BOOTTIME_TRACING=y

## 实际上，当 make menuconfig 的时候， CONFIG_BOOTTIME_TRACING 同级下面存在很多内容

## ftrace
lwn 关于 ftrace 的介绍 [^10]

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


### ftrace 对于编译的时候有要求
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

## TODO
3. ftrace_graph_exit_task : ftrace_graph 是个啥

- [ ] ftrace 和 perf 是什么关系呀 ? 至少应该是功能不同的东西吧，如果 perf 采用 sampling 的技术，而 ftrace 可以知道其中

也可以作为 ftrace 使用:
perf ftrace is a simple *wrapper* for kernel's ftrace functionality, and only supports single thread tracing now.
```plain
perf ftrace -T __kmalloc ./add_vec
perf ftrace ./add_vec
```

## kernel/trace/ftrace.c
```c

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

## 想不到这里也和 kernel
- delete_module :
  - ftrace_release_mod(mod);
- load_module
```c
    /* Ftrace init must be called in the MODULE_STATE_UNFORMED state */
    ftrace_module_init(mod);
```

## 分析下这个
[^10]: [lwn : A look at ftrace](https://lwn.net/Articles/322666/)

## 问题
- [ ] 感觉 perf 能做的事情，bpftrace 和 ebpf 都可以做，而且更加好
  - 例如 hardware monitor 在 bpftrace 中和 bcc 中 llcstat-bpfcc 都是可以做的

## ftrace-cmd
https://lwn.net/Articles/410200/ 的记录

- [ ] 两个用户层次的前端工具 : perf + trace-cmd 分别读取的内容是从哪里的呀 ?
- [ ] trace-cmd

1. ftrace-cmd 的 record 原理: 为每一个 cpu 创建出来一个 process，读取 /sys/kernel/debug/tracing/per_cpu/cpu0，最后主函数将所有的 concatenate 起来
2. -e 可以选择一个 trace event 或者一个 subsystem 的 trace point。The -e option enables an event. The argument following the -e can be an event name, event subsystem name, or the special name all.
3. 四种功能 : Ftrace also has special plugin tracers that do not simply trace specific events. These tracers include the function, function graph, and latency tracers.
    1. trace function 指的是 ?
    2. `current_tracer` 可以放置的内容那么多，为什么只有这几个 plugin
    3. 当 latency tracer 和 perf 的关系是什么 ?
4. 解释下面的程序:
    1. -p 添加 function plugin
    2. -e 选择需要被 trace 的 events
    3. The `-l` option is the same as echoing its argument into `set_ftrace_filter`, and the `-n` option is the same as echoing its argument into `set_ftrace_notrace`.
```sh
trace-cmd record -p function -l 'sched_*' -n 'sched_slice'
```
5. 解释下面的程序: TODO 应该是采用，
```txt
trace-cmd record -p function_graph -l do_IRQ -e irq_handler_entry sleep 10
```

> TODO : 到底 dynamic ftrace 是什么 ?


> 让我们解决一个小问题，为什么没有办法写入到 `set_ftrace_filter`
https://superuser.com/questions/287371/obtain-kernel-config-from-currently-running-linux-system

> 引出了一个小问题:
`tracing_on` 和 `/proc/sys/kernel/ftrace_enabled` 分别表示什么 ?
