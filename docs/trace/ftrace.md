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

## 问题
- [ ] function and latency tracers : 为什么 ftrace 可以跟踪 latency tracer
- [ ] 为什么 ftrace 可以跟踪 kprobe 和 uprobe
- [ ] 无法理解 `CONFIG_FUNCTION_GRAPH_TRACER`
- [ ] set_ftrace_filter 到底可以设置什么内容？
  - 至少 kprobe 对于 inline 函数是非常敏感的
- [ ] function 和 function graph 有啥区别
- [ ] 这些 tracer 都是做啥的: blk function_graph wakeup_dl wakeup_rt wakeup function nop
- [ ] trace-cmd 的 event 不就是 tracepoint 机制吗？

## Kernel hacking -> Tracker

### Trace syscalls
- 不知道怎么用

## CONFIG_BOOTTIME_TRACING

设置如下等效于
```txt
   ftrace_notrace=rcu_read_lock,rcu_read_unlock,spin_lock,spin_unlock
   ftrace_filter=kfree,kmalloc,schedule,vmalloc_fault,spurious_fault
```
在开机的时候设置 ftrace_notrace 和 ftrace_filter 中的内容。

## [官方文档](https://www.kernel.org/doc/html/latest/trace/ftrace.html)

### dynamic ftrace

## [A look at ftrace](https://lwn.net/Articles/322666/)
The name ftrace comes from "function tracer", which was its original purpose, but it can do more than that.
Various additional tracers have been added to look at things like context switches, how long interrupts are disabled,
how long it takes for high-priority tasks to run after they have been woken up, and so on.
Its genesis in the realtime tree is evident in the tracers so far available, but ftrace also includes a plugin framework that allows new tracers to be added easily.
> 起源，ftrace 的功能后来逐渐增加，并且提供框架

- [ ] 还没看完

## [Secrets of the Ftrace function tracer](https://lwn.net/Articles/370423/)
```txt
echo set* > set_ftrace_filter # 可以 regex
echo ':mod:ext4' > set_ftrace_filter # 显示一个模块
echo '__bad_area_nosemaphore:traceoff' > set_ftrace_filter # 禁用
```

```txt
echo '!*lock*' >> set_ftrace_filter
```
不是存在 `set_ftrace_notrace`，为什么想要这个代码？

- [ ] ./code/ftrace-two.sh 中的代码无法产生任何结果

## [Debugging the kernel using Ftrace - part 1](https://lwn.net/Articles/365835/)
-[ ] trace_prink 没有看到输出

## [Debugging the kernel using Ftrace - part 2](https://lwn.net/Articles/366796/)
- tracing_off() 在内核中直接调控 tracing_on
- cat /proc/sys/kernel/ftrace_dump_on_oops
- You can also trigger a dump of the Ftrace buffer to the console with sysrq-z.

> To choose a particular location for the kernel dump, the kernel may call ftrace_dump() directly. Note, this may permanently disable Ftrace and a reboot may be necessary to enable it again. This is because ftrace_dump() reads the buffer. The buffer is made to be written to in all contexts (interrupt, NMI, scheduling) but the reading of the buffer requires locking. To be able to perform ftrace_dump() the locking is disabled and the buffer may end up being corrupted after the output.

这个什么意思？

- /sys/kernel/debug/tracing/stack_trace : 记录每次最大深度的 kernel stack 的

## /sys/kernel/debug/tracing/README

```txt
tracing mini-HOWTO:

# echo 0 > tracing_on : quick way to disable tracing
# echo 1 > tracing_on : quick way to re-enable tracing

 Important files:
  trace                 - The static contents of the buffer
                          To clear the buffer write into this file: echo > trace
  trace_pipe            - A consuming read to see the contents of the buffer
  current_tracer        - function and latency tracers
  available_tracers     - list of configured tracers for current_tracer
  error_log     - error log for failed commands (that support it)
  buffer_size_kb        - view and modify size of per cpu buffer
  buffer_total_size_kb  - view total size of all cpu buffers

  trace_clock           - change the clock used to order events
       local:   Per cpu clock but may not be synced across CPUs
      global:   Synced across CPUs but slows tracing down.
     counter:   Not a clock, but just an increment
      uptime:   Jiffy counter from time of boot
        perf:   Same clock that perf events use
     x86-tsc:   TSC cycle counter

  timestamp_mode        - view the mode used to timestamp events
       delta:   Delta difference against a buffer-wide timestamp
    absolute:   Absolute (standalone) timestamp

  trace_marker          - Writes into this file writes into the kernel buffer

  trace_marker_raw              - Writes into this file writes binary data into the kernel buffer
  tracing_cpumask       - Limit which CPUs to trace
  instances             - Make sub-buffers with: mkdir instances/foo
                          Remove sub-buffer with rmdir
  trace_options         - Set format or modify how tracing happens
                          Disable an option by prefixing 'no' to the
                          option name
  saved_cmdlines_size   - echo command number in here to store comm-pid list

  available_filter_functions - list of functions that can be filtered on
  set_ftrace_filter     - echo function name in here to only trace these
                          functions
             accepts: func_full_name or glob-matching-pattern
             modules: Can select a group via module
              Format: :mod:<module-name>
             example: echo :mod:ext3 > set_ftrace_filter
            triggers: a command to perform when function is hit
              Format: <function>:<trigger>[:count]
             trigger: traceon, traceoff
                      enable_event:<system>:<event>
                      disable_event:<system>:<event>
                      stacktrace
                      snapshot
                      dump
                      cpudump
             example: echo do_fault:traceoff > set_ftrace_filter
                      echo do_trap:traceoff:3 > set_ftrace_filter
             The first one will disable tracing every time do_fault is hit
             The second will disable tracing at most 3 times when do_trap is hit
               The first time do trap is hit and it disables tracing, the
               counter will decrement to 2. If tracing is already disabled,
               the counter will not decrement. It only decrements when the
               trigger did work
             To remove trigger without count:
               echo '!<function>:<trigger> > set_ftrace_filter
             To remove trigger with a count:
               echo '!<function>:<trigger>:0 > set_ftrace_filter
  set_ftrace_notrace    - echo function name in here to never trace.
            accepts: func_full_name, *func_end, func_begin*, *func_middle*
            modules: Can select a group via module command :mod:
            Does not accept triggers
  set_ftrace_pid        - Write pid(s) to only function trace those pids
                    (function)
  set_ftrace_notrace_pid        - Write pid(s) to not function trace those pids
                    (function)
  set_graph_function    - Trace the nested calls of a function (function_graph)
  set_graph_notrace     - Do not trace the nested calls of a function (function_graph)
  max_graph_depth       - Trace a limited depth of nested calls (0 is unlimited)

  snapshot              - Like 'trace' but shows the content of the static
                          snapshot buffer. Read the contents for more
                          information
  stack_trace           - Shows the max stack trace when active
  stack_max_size        - Shows current max stack size that was traced
                          Write into this file to reset the max size (trigger a
                          new trace)
  stack_trace_filter    - Like set_ftrace_filter but limits what stack_trace
                          traces
  dynamic_events                - Create/append/remove/show the generic dynamic events
                          Write into this file to define/undefine new trace events.
  kprobe_events         - Create/append/remove/show the kernel dynamic events
                          Write into this file to define/undefine new trace events.
  uprobe_events         - Create/append/remove/show the userspace dynamic events
                          Write into this file to define/undefine new trace events.
          accepts: event-definitions (one definition per line)
           Format: p[:[<group>/][<event>]] <place> [<args>]
                   r[maxactive][:[<group>/][<event>]] <place> [<args>]
                   e[:[<group>/][<event>]] <attached-group>.<attached-event> [<args>]
                   -:[<group>/][<event>]
            place: [<module>:]<symbol>[+<offset>]|<memaddr>
place (kretprobe): [<module>:]<symbol>[+<offset>]%return|<memaddr>
   place (uprobe): <path>:<offset>[%return][(ref_ctr_offset)]
             args: <name>=fetcharg[:type]
         fetcharg: (%<register>|$<efield>), @<address>, @<symbol>[+|-<offset>],
                   $stack<index>, $stack, $retval, $comm, $arg<N>,
                   +|-[u]<offset>(<fetcharg>), \imm-value, \"imm-string"
             type: s8/16/32/64, u8/16/32/64, x8/16/32/64, string, symbol,
                   b<bit-width>@<bit-offset>/<container-size>, ustring,
                   symstr, <type>\[<array-size>\]
            efield: For event probes ('e' types), the field is on of the fields
                    of the <attached-group>/<attached-event>.
  events/               - Directory containing all trace event subsystems:
      enable            - Write 0/1 to enable/disable tracing of all events
  events/<system>/      - Directory containing all trace events for <system>:
      enable            - Write 0/1 to enable/disable tracing of all <system>
                          events
      filter            - If set, only events passing filter are traced
  events/<system>/<event>/      - Directory containing control files for
                          <event>:
      enable            - Write 0/1 to enable/disable tracing of <event>
      filter            - If set, only events passing filter are traced
      trigger           - If set, a command to perform when event is hit
            Format: <trigger>[:count][if <filter>]
           trigger: traceon, traceoff
                    enable_event:<system>:<event>
                    disable_event:<system>:<event>
                    stacktrace
                    snapshot
           example: echo traceoff > events/block/block_unplug/trigger
                    echo traceoff:3 > events/block/block_unplug/trigger
                    echo 'enable_event:kmem:kmalloc:3 if nr_rq > 1' > \
                          events/block/block_unplug/trigger
           The first disables tracing every time block_unplug is hit.
           The second disables tracing the first 3 times block_unplug is hit.
           The third enables the kmalloc event the first 3 times block_unplug
             is hit and has value of greater than 1 for the 'nr_rq' event field.
           Like function triggers, the counter is only decremented if it
            enabled or disabled tracing.
           To remove a trigger without a count:
             echo '!<trigger> > <system>/<event>/trigger
           To remove a trigger with a count:
             echo '!<trigger>:0 > <system>/<event>/trigger
           Filters can be ignored when removing a trigger.
```
- [ ] trace_marker

## [ftrace: trace your kernel functions!](https://jvns.ca/blog/2017/03/19/getting-started-with-ftrace/)

> ftrace 就像是打印函数调用路径，但是远远不止于此:

- [ ] 最后有一系列的附录

## kernelshark 使用介绍
https://elinux.org/images/6/64/Elc2011_rostedt.pdf


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


## ftrace 和 perf 怎么感觉关系存在耦合啊
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

## 代码分析
3. ftrace_graph_exit_task : ftrace_graph 是个啥

## 问题
- [ ] 感觉 perf 能做的事情，bpftrace 和 ebpf 都可以做，而且更加好
  - 例如 hardware monitor 在 bpftrace 中和 bcc 中 llcstat-bpfcc 都是可以做的

## [https://lwn.net/Articles/410200/](trace-cmd: A front-end for Ftrace)
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
