## fprobe 机制
<!-- b8501252-b989-45c9-b5ea-d2ddd38f1a6f -->

BPF syscall 处理逻辑 (kernel/bpf/syscall.c)

```txt
  case BPF_PROG_TYPE_KPROBE:
      if (attr->link_create.attach_type == BPF_PERF_EVENT)
          ret = bpf_perf_link_attach(attr, prog);           // ← 普通 kprobe
      else if (attr->link_create.attach_type == BPF_TRACE_KPROBE_MULTI ||
               attr->link_create.attach_type == BPF_TRACE_KPROBE_SESSION)
          ret = bpf_kprobe_multi_link_attach(attr, prog);   // ← fprobe
```

用户必须显式选择：
	- BPF_PERF_EVENT → 使用传统 kprobe（通过 perf_event 子系统）
	- BPF_TRACE_KPROBE_MULTI → 使用 fprobe



关键文档 Documentation/trace/fprobetrace.rst

### 从 bpftrace 到 kprobe 的记录

没想到把，调用的路线居然是: bpf -> kernel/events -> kernel/trace -> arch/x86/kernel/kprobes
```txt
#0  arch_remove_kprobe (p=0xffff8880144f3c18) at arch/x86/kernel/kprobes/core.c:834
#1  __unregister_trace_kprobe (tk=tk@entry=0xffff8880144f3c00) at kernel/trace/trace_kprobe.c:529
#2  destroy_local_trace_kprobe (event_call=<optimized out>) at kernel/trace/trace_kprobe.c:1958
#3  _free_event (event=event@entry=0xffff888008478530) at kernel/events/core.c:5357
#4  put_event (event=0xffff888008478530) at kernel/events/core.c:5454
#5  perf_event_release_kernel (event=0xffff888008478530) at kernel/events/core.c:5579
#6  0xffffffff812d54d2 in perf_release (inode=<optimized out>, file=<optimized out>) at kernel/events/core.c:5589
#7  0xffffffff81405e4c in __fput (file=0xffff88800e3a46c0) at fs/file_table.c:431
#8  0xffffffff814060c9 in __fput_sync (file=<optimized out>) at fs/file_table.c:516
#9  0xffffffff814003fc in __do_sys_close (fd=<optimized out>) at fs/open.c:1567
#10 __se_sys_close (fd=<optimized out>) at fs/open.c:1552
#11 __x64_sys_close (regs=<optimized out>) at fs/open.c:1552
#12 0xffffffff821068fc in do_syscall_x64 (nr=3, regs=0xffffc90000b7ff58) at arch/x86/entry/common.c:52
#13 do_syscall_64 (regs=0xffffc90000b7ff58, nr=3) at arch/x86/entry/common.c:83
#14 entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:121
```

## 源码分析
- kernel/trace/trace_fprobe.c
- kernel/trace/bpf_trace.c : 唯一调用 register_fprobe_ips 的位置
- kernel/trace/fprobe.c : 简单的封装
  - 其实也就是调用 ftrace_set_filter 而已


ftrace_ops 是一个通用的机制，例如:
```c
static void fprobe_init(struct fprobe *fp)
{
	fp->nmissed = 0;
	if (fprobe_shared_with_kprobes(fp))
		fp->ops.func = fprobe_kprobe_handler;
	else
		fp->ops.func = fprobe_handler;
	fp->ops.flags |= FTRACE_OPS_FL_SAVE_REGS;
}
```

### trace_fprobe.c 的作用
看上去就是围绕 /sys/kernel/debug/tracing/dynamic_events 来提供操作，其核心为
```c
static struct dyn_event_operations trace_fprobe_ops = {
	.create = trace_fprobe_create,
	.show = trace_fprobe_show,
	.is_busy = trace_fprobe_is_busy,
	.free = trace_fprobe_release,
	.match = trace_fprobe_match,
};
```

### 为什么 bpf 使用 fprobe

```diff
History:        #0
Commit:         0dcac272540613d41c05e89679e4ddb978b612f1
Author:         Jiri Olsa <jolsa@kernel.org>
Committer:      Alexei Starovoitov <ast@kernel.org>
Author Date:    Wed 16 Mar 2022 08:24:09 PM CST
Committer Date: Fri 18 Mar 2022 11:17:18 AM CST

bpf: Add multi kprobe link

Adding new link type BPF_LINK_TYPE_KPROBE_MULTI that attaches kprobe
program through fprobe API.

The fprobe API allows to attach probe on multiple functions at once
very fast, because it works on top of ftrace. On the other hand this
limits the probe point to the function entry or return.

The kprobe program gets the same pt_regs input ctx as when it's attached
through the perf API.

Adding new attach type BPF_TRACE_KPROBE_MULTI that allows attachment
kprobe to multiple function with new link.

User provides array of addresses or symbols with count to attach the
kprobe program to. The new link_create uapi interface looks like:

  struct {
          __u32           flags;
          __u32           cnt;
          __aligned_u64   syms;
          __aligned_u64   addrs;
  } kprobe_multi;

The flags field allows single BPF_TRACE_KPROBE_MULTI bit to create
return multi kprobe.

Signed-off-by: Masami Hiramatsu <mhiramat@kernel.org>
Signed-off-by: Jiri Olsa <jolsa@kernel.org>
Signed-off-by: Alexei Starovoitov <ast@kernel.org>
Acked-by: Andrii Nakryiko <andrii@kernel.org>
Link: https://lore.kernel.org/bpf/20220316122419.933957-4-jolsa@kernel.org
```

和想象的很不一样，这个函数调用很容易，甚至是
```sh
sudo bpftrace -e 'kprobe:do_sys_openat2 { print("hit kprobe:do_sys_openat2") }'
```

```txt
      - entry_SYSCALL_64
        - do_syscall_64
          - do_syscall_x64
            - __x64_sys_bpf
              - __se_sys_bpf
                - __do_sys_bpf
                  - __sys_bpf
                    - link_create
                      - bpf_kprobe_multi_link_attach
                        - register_fprobe_ips
```


## fprobe 的定位 : ftrace 的封装

### 从 trace_probe_add_file 的调用位置

- kernel/trace/trace_eprobe.c
  - commit 7491e2c44278 ("tracing: Add a probe that attaches to trace events")
- kernel/trace/trace_fprobe.c
  - commit 334e5519c375 ("tracing/probes: Add fprobe events for tracing function entry and exit.")
  - Documentation/trace/fprobe.rst
  - Documentation/trace/fprobetrace.rst
- kernel/trace/trace_kprobe.c
- kernel/trace/trace_uprobe.c

### 第一个 commit 的说明

```txt
commit cad9931f64dc7f5dbdec12cae9f30063360f9855
Author: Masami Hiramatsu <orgakes filtering patterns of the functin names.
     - register_fprobe_ips() takes an array of ftrace-location addresses.
     - register_fprobe_syms() takes an array of function names.

    The registered fprobes can be unregistered with unregister_fprobe().
    e.g.

    struct fprobe fp = { .entry_handler = user_handler };
    const char *targets[] = { "func1", "func2", "func3"};
    ...

    ret = register_fprobe_syms(&fp, targets, ARRAY_SIZE(targets));

    ...

    unregister_fprobe(&fp);

    Signed-off-by: Masami Hiramatsu <mhiramat@kernel.org>
    Signed-off-by: Steven Rostedt (Google) <rostedt@goodmis.org>
    Tested-by: Steven Rostedt (Google) <rostedt@goodmis.org>
    Signed-off-by: Alexei Starovoitov <ast@kernel.org>
    Link: https://lore.kernel.org/bpf/164735283857.1084943.1154436951479395551.stgit@devnote2
>
Date:   Tue Mar 15 23:00:38 2022 +0900

    fprobe: Add ftrace based probe APIs

    The fprobe is a wrapper API for ftrace function tracer.
    Unlike kprobes, this probes only supports the function entry, but this
    can probe multiple functions by one fprobe. The usage is similar, user
    will set their callback to fprobe::entry_handler and call
    register_fprobe*() with probed functions.
    There are 3 registration interfaces,

     - register_fprobe() tmhiramat@kernel.
```

### Documentation/trace/fprobe.rst
https://www.kernel.org/doc/html/latest/trace/fprobe.html

> Fprobe is a function entry/exit probe mechanism based on ftrace.
> Instead of using ftrace full feature, if you only want to attach callbacks on function entry and exit,
> similar to the kprobes and kretprobes, you can use fprobe.
> Compared with kprobes and kretprobes, fprobe gives faster instrumentation for multiple functions with single handler.
> This document describes how to use fprobe.

从 samples/fprobe/fprobe_example.c 看，就是用于注册 hook

## kprobe 是可以在任何地方打点的

这是 fprobe 的语法:
```txt
  f[:[GRP1/][EVENT1]] SYM [FETCHARGS]                       : Probe on function entry
```

这是 kprobetrace 的语法
```txt
  p[:[GRP/][EVENT]] [MOD:]SYM[+offs]|MEMADDR [FETCHARGS]	: Set a probe
```

测试参考 docs/trace/code/ftrace-example.sh 中的 kprobe_in_function_body
kprobe_in_function_body

在这里介绍了分别使用 trace-cmd 和 perf 来实现在任何地方打点:
https://walac.github.io/kernel-tracing/

但是，知道有这个效果就可以了，还需要使用 vmlinux ，看着就脑袋疼。


## 需要验证这个东西是什么

那么，也就是说， kprobe 可以通过三个方法获取:
1. perf_event_open
2. bpf
3. 内核模块

> [!NOTE]
> 参考神奇海螺的意见，有待验证

### 方式1: 通过 perf_event 系统调用

```c
// 用户态代码
struct perf_event_attr attr = {
    .type = PERF_TYPE_PROBE,            // PMU 类型
    .kprobe_func = (u64)"do_nanosleep", // 探测函数
    .probe_offset = 0,
};
syscall(__NR_perf_event_open, &attr, ...);

// 内核: perf_event → perf_kprobe → kprobe
```

### 方式2: 直接 kprobe API

```c
// 内核模块代码
struct kprobe kp = {
    .symbol_name = "do_nanosleep",
    .pre_handler = handler,
};
register_kprobe(&kp);

// 内核: 直接调用 kprobe 子系统
```

### 方式3: eBPF 中的选择

```c
// eBPF 程序中通过 attach_type 选择底层机制

// 使用传统 kprobe（通过 perf_event）
BPF_PROG_TYPE_KPROBE + BPF_PERF_EVENT
    → bpf_perf_link_attach()
    → perf_event → kprobe

// 使用 fprobe（批量探测）
BPF_PROG_TYPE_KPROBE + BPF_TRACE_KPROBE_MULTI
    → bpf_kprobe_multi_link_attach()
    → fprobe → ftrace
```

## BPF attach_type 路径总结

| attach_type | 路径 | 适用场景 |
|-------------|------|----------|
| `BPF_PERF_EVENT` | BPF → perf_event → **kprobe** | 单个函数探测，传统方式 |
| `BPF_TRACE_KPROBE_MULTI` | BPF → **fprobe** | 批量函数探测，性能更好 |
| `BPF_TRACE_KPROBE_SESSION` | BPF → **fprobe** | entry/exit 成对探测 |

## kprobe 和 perf 子系统的关系

也就是这个?
```c
static struct pmu perf_kprobe = {
    .task_ctx_nr  = perf_sw_context,
    .event_init   = perf_kprobe_event_init,    // 初始化 kprobe
    .add          = perf_trace_add,             // 添加事件
    .del          = perf_trace_del,             // 删除事件
    .start        = perf_swevent_start,
    .stop         = perf_swevent_stop,
    .read         = perf_swevent_read,
    .attr_groups  = kprobe_attr_groups,
};
```
那么为什么没有定义一个 fprobe 类似的结构?

真的是这个结构吗? 似乎是的，但是细节需要确认
```txt
┌─────────────────────────────────────────────────────────────────────┐
│                        用户空间                                      │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐                      │
│  │ perf_event()│ │  bpf()   │    │ftrace cmd│                      │
│  └────┬─────┘    └────┬─────┘    └────┬─────┘                      │
└───────┼───────────────┼───────────────┼──────────────────────────────┘
        │               │               │
        ▼               ▼               ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        内核层                                        │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │                    perf_event 子系统                          │   │
│  │  ┌──────────────────────────────────────────────────────┐   │   │
│  │  │  PMU (Performance Monitoring Unit) 抽象层            │   │   │
│  │  │                                                      │   │   │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐     │   │   │
│  │  │  │ perf_hw    │  │ perf_sw    │  │ perf_kprobe│     │   │   │
│  │  │  │ (硬件PMU)  │  │ (软件事件)  │  │ (kprobe)   │     │   │   │
│  │  │  └────────────┘  └────────────┘  └─────┬──────┘     │   │   │
│  │  └─────────────────────────────────────────┼────────────┘   │   │
│  └────────────────────────────────────────────┼─────────────────┘   │
│                                               │                      │
│                                               ▼                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │         - kprobe 子系统 (kernel/kprobes.c)                 │   │
│  │         - 注册/注销 kprobe                                     │   │
│  │         - 断点处理 (int3/ebreak)                              │   │
│  │         - 调用用户回调                                         │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │         - fprobe 子系统 (kernel/trace/fprobe.c)            │   │
│  │         - 基于 ftrace 的批量函数探测                           │   │
│  │         - entry/exit 统一回调                                  │   │
│  │         - 通过 function-graph 实现返回探测                     │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## fprobe 是完全绕过 kprobe 的
<!-- 604bb17a-2ab2-48b2-90ea-9c73e6043368 -->

fprobe 不直接使用 kernel/kprobes.c，它是基于 fgraph（function graph tracer） 和 ftrace 实现的。

但 fprobe 有一个可选功能可以与 kprobes 共享回调：

// include/linux/fprobe.h
#define FPROBE_FL_KPROBE_SHARED 2  // 与 kprobes 共享 handler

static inline bool fprobe_shared_with_kprobes(struct fprobe *fp)
{
    return (fp) ? fp->flags & FPROBE_FL_KPROBE_SHARED : false;
}

fprobe 的真实实现层次

┌─────────────────────────────────────────────────────────────────┐
│  fprobe 用户层 (kernel/trace/trace_fprobe.c)                     │
│  • 通过 tracefs 暴露 fprobe_events 接口                          │
│  • 类似 trace_kprobe.c，但基于 fprobe 而非 kprobes               │
├─────────────────────────────────────────────────────────────────┤
│  fprobe 核心层 (kernel/trace/fprobe.c)                           │
│  • 基于 fgraph_ops 和 ftrace_ops 实现                            │
│  • 【不依赖 kprobes.c】                                          │
├─────────────────────────────────────────────────────────────────┤
│  fgraph/ftrace 层 (kernel/trace/fgraph.c, kernel/trace/ftrace.c) │
│  • 函数图跟踪基础设施                                             │
│  • 使用 ftrace 的动态代码修改能力                                 │
└─────────────────────────────────────────────────────────────────┘

fprobe 与 kprobes 的关系

 特性       fprobe                 kprobe
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 基础机制   fgraph + ftrace        断点指令 (break)
 单步执行   无需（利用函数返回）   需要（指令模拟/单步）
 性能       更高（无断点开销）     较低（断点 + 单步）
 适用场景   函数入口/出口探测      任意指令位置探测

fprobe 使用 fgraph 的证据

// kernel/trace/fprobe.c
static struct fgraph_ops fprobe_graph_ops = {
    .entryfunc = fprobe_fgraph_entry,   // 函数入口回调
    .exitfunc  = fprobe_return,          // 函数返回回调
};

// 注册时使用 fgraph 接口
fgraph_get_fgraph_ops(&fprobe_graph_ops);  // 获取 fgraph 操作符

为什么有 kprobes 相关代码？

fprobe 代码中的 kprobes 引用是为了防止与 kprobes 冲突：

// kernel/trace/fprobe.c
static inline int __fprobe_kprobe_handler(...)
{
    // 如果已经有 kprobe 在运行，跳过避免递归
    if (unlikely(kprobe_running())) {
        fp->nmissed++;
        return 0;
    }

    kprobe_busy_begin();
    ret = __fprobe_handler(ip, parent_ip, fp, fregs, data);
    kprobe_busy_end();
    return ret;
}

这只是互斥标记，不是功能依赖。

对比：trace_kprobe vs trace_fprobe

 层次       trace_kprobe              trace_fprobe
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 上层       trace_kprobe.c            trace_fprobe.c
 公共代码   trace_probe.c             trace_probe.c
 核心机制   kernel/kprobes.c          kernel/trace/fprobe.c
 底层基础   arch/*/kernel/kprobes.c   kernel/trace/fgraph.c + kernel/trace/ftrace.c

所以 fprobe 完全绕过了 kprobes 子系统，是一个独立的轻量级探测机制。

## fprobetrace.rst 中介绍了 fprobe 可以使用 BTF 的，那么 kprobetrace 也可以吗?

还是 kprobetrace 只是可以在函数入口可以的?

还是

## 无论如何，fprobe 是通过 fgraph 实现，都是很奇怪的

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
