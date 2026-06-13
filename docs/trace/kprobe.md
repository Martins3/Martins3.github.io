# kprobe

- Documentation/trace/kprobes.rst
- Documentation/trace/kprobetrace.rst

register_kprobe 的调用者:
1. kernel/fail_function.c
2. kernel/trace/trace_kprobe.c


## [ ] /sys/kernel/debug/kprobes/blacklist 和 ftrace 中的 KPROBE_EVENTS_ON_NOTRACE 是一个东西吗

## ftrace 和 kprobe 是啥关系
<!-- 0c919019-b89f-40a8-8bcb-7f7fdbeb4089 -->

```c
/* Is this kprobe uses ftrace ? */
static inline bool kprobe_ftrace(struct kprobe *p)
{
	return p->flags & KPROBE_FLAG_FTRACE;
}
```
```txt
config KPROBES_ON_FTRACE
	def_bool y
	depends on KPROBES && HAVE_KPROBES_ON_FTRACE
	depends on DYNAMIC_FTRACE_WITH_REGS
	help
	  If function tracer is enabled and the arch supports full
	  passing of pt_regs to function tracing, then kprobes can
	  optimize on top of function tracing.
```
哦，我意识到了，ftrace 中的 function tracer 其实就是基本版本的 kprobe，
因为 ftrace 给函数都制作了 nop 的埋点了。 所以，kprobe 基于 ftrace 来的话，应该是有的事情比较好做的。

如果将 CONFIG_DYNAMIC_FTRACE 关闭之后，那么可以
1. sudo perf probe --add follow_pte 之后
```txt
[root@localhost debug]# cat kprobes/list
ffffffff8139ba04  k  follow_pte+0x4    [DISABLED]
```

- arch/loongarch/kernel/kprobes.c : 架构相关支持
- kernel/kprobes.c : 架构无关的实现
- kernel/trace/trace_probe.c : 支持 perf fail_function 以及 ftrace 的基础

## 实现相关的文件

- arch/loongarch/kernel/kprobes.c : 架构相关支持
- kernel/kprobes.c : 架构无关的实现
- kernel/trace/trace_probe.c : 支持 perf fail_function 以及 ftrace 的基础

- kernel/trace/trace_kprobe.c
- kernel/trace/trace_uprobe.c
- kernel/trace/trace_kprobe.c
- kernel/trace/trace_eprobe.c

(他们都是对应的 ?)

## 简单操作一下

sudo perf probe --add follow_pte
sudo perf probe --add do_fault

```txt
[root@nixos:/sys/kernel/debug/kprobes]# cat list
ffffffffa7119a04  k  follow_pte+0x4    [DISABLED][FTRACE]
ffffffffa711e1c0  k  do_fault+0x0    [DISABLED][FTRACE]
```

```sh
sudo bpftrace -e "kprobe:do_futex { @[kstack] = count(); }"
```

```txt
[root@nixos:/sys/kernel/debug/kprobes]# cat list
ffffffffa6f93f64  k  do_futex+0x4    [FTRACE]
ffffffffa7119a04  k  follow_pte+0x4    [DISABLED][FTRACE]
ffffffffa711e1c0  k  do_fault+0x0    [DISABLED][FTRACE]
```
输出含义参考 report_probe

## 再简单操作一下

使用如下命令，ftrace 的目录中并不会增加什么
```sh
sudo trace do_fault
sudo bpftrace -e "kprobe:do_fault { @[kstack] = count(); }"
```
走的路径都是这种的:
```txt
@[
    register_kprobe+5
    create_local_trace_kprobe+333
    perf_kprobe_init+49
    perf_kprobe_event_init+67
    perf_try_init_event+71
    perf_event_alloc+1623
    __do_sys_perf_event_open+453
    do_syscall_64+67
    entry_SYSCALL_64_after_hwframe+111
]: 1
```

使用
```sh
echo 'p:myprobe2 do_sys_openat2 dfd=%ax filename=%dx open_how=%cx usize=+4($stack)' >kprobe_events
```
增加的目录是:

```txt
[root@nixos:/sys/kernel/debug/tracing/events/kprobes]# ls
enable  filter  myprobe2
```

echo 'p:myprobe2 do_sys_openat2 dfd=%ax filename=%dx open_how=%cx usize=+4($stack)' >kprobe_events

echo 'p:myprobe2 follow_pte ' > dynamic_events

使用
```sh
sudo perf probe --add follow_pte
```

```txt
[root@nixos:/sys/kernel/debug/tracing/events]# ls probe
enable  filter  follow_pte
```

他们两个的代码都是长的一样的:
```txt
@[
    register_kprobe+5
    __trace_kprobe_create+2555
    trace_probe_create+120
    create_or_delete_trace_kprobe+21
    trace_parse_run_command+247
    vfs_write+239
    ksys_write+111
    do_syscall_64+67
    entry_SYSCALL_64_after_hwframe+111
]: 1
```

在 qemu 中观察， sudo perf probe --add follow_pte 的行为，使用的命令为: `p:probe/follow_pfn _text+3857104`
```txt
#0  trace_probe_create (raw_command=0xffff888105f91000 "p:probe/follow_pfn _text+3857104", createfn=createfn@entry=0xffffffff812c6fe0 <__trace_kprobe_create>) at kernel/trace/trace_probe.c:1943
#1  0xffffffff812c5055 in trace_kprobe_create (raw_command=<optimized out>) at kernel/trace/trace_kprobe.c:985
#2  create_or_delete_trace_kprobe (raw_command=<optimized out>) at kernel/trace/trace_kprobe.c:995
#3  0xffffffff8129bbda in trace_parse_run_command (file=<optimized out>, buffer=0x55d8c631f520 "p:probe/follow_pfn _text+3857104", count=32, ppos=<optimized out>, createfn=0xffffffff812c5040 <create_or_delete_trace_kprobe>) at kernel/trace/trace.c:10477
#4  0xffffffff814685fb in vfs_write (file=file@entry=0xffff888106416f00, buf=buf@entry=0x55d8c631f520 "p:probe/follow_pfn _text+3857104", count=count@entry=32, pos=pos@entry=0xffffc90001e6fef8) at fs/read_write.c:588
#5  0xffffffff81468d2f in ksys_write (fd=<optimized out>, buf=0x55d8c631f520 "p:probe/follow_pfn _text+3857104", count=32) at fs/read_write.c:643
#6  0xffffffff8240a8aa in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001e6ff58) at arch/x86/entry/common.c:52
#7  do_syscall_64 (regs=0xffffc90001e6ff58, nr=<optimized out>) at arch/x86/entry/common.c:83
#8  0xffffffff826000eb in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
直接在命令行执行 `p:probe/follow_pfn _text+3857104 > kprobe_events` 之后，可以得到的完全相同的结果，其中的其中 probe 可以修改的

再看 Documentation/trace/kprobetrace.rst 中的
> GRP		: Group name. If omitted, use "kprobes" for it.

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
