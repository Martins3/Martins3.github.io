## CONFIG_TASKS_RCU

function tracer 为什么会打开 CONFIG_TASKS_RUDE_RCU

```txt
config FUNCTION_TRACER
	bool "Kernel Function Tracer"
	depends on HAVE_FUNCTION_TRACER
	select KALLSYMS
	select GENERIC_TRACER
	select CONTEXT_SWITCH_TRACER
	select GLOB
	select TASKS_RCU if PREEMPTION
	select TASKS_RUDE_RCU
	help
	  Enable the kernel to trace every kernel function. This is done
	  by using a compiler feature to insert a small, 5-byte No-Operation
	  instruction at the beginning of every kernel function, which NOP
	  sequence is then dynamically patched into a tracer call when
	  tracing is enabled by the administrator. If it's runtime disabled
	  (the bootup default), then the overhead of the instructions is very
	  small and not measurable even in micro-benchmarks (at least on
	  x86, but may have impact on other architectures).
```

不能说是简单易懂，也算是真的牛逼

https://docs.kernel.org/RCU/Design/Requirements/Requirements.html#tasks-rcu

居然考虑到 trace 机制和 preemption 通知打开的时候，tracepoint 里面的 trampolines 的内存的释放。

所以，我们观察到，这一组函数的调用者都是 call_rcu_tasks() 之类的

## task rcu 做什么的
<!-- 9f7e1524-16f2-48ca-90f0-57f3bfd280b9 -->

它也是一种 RCU，但它等待的不是普通 RCU 的 quiescent state，而是“task 级别”的执行点”。

最常见的普通 RCU 里，你关心的是：

- 某个 CPU 上是否还在 old reader 的 rcu_read_lock() 临界区里
- 只要每个 CPU 都经历一次 quiescent state，比如用户态、idle、context switch，旧 reader 就算过去了

但有些场景里，这个定义不够用。因为你想等的并不是“CPU 上的 RCU read-side critical section 结束”，而是：

- 所有可能还在执行某段代码的 task
- 都至少经过一次“我已经不在那段代码里了”的时刻

也就是说，RCU Tasks 不太关心你有没有 rcu_read_lock()，而更关心：

- 某个 task 现在是不是还可能卡在那段旧代码路径上
- 只有等这个 task 发生一次合适的切换点，才能认为它已经“走过去了”

所以它常用于这类需求：

- 等待所有 task 都离开某段内核代码
- trampoline / tracing / BPF / 静态调用 / 文本补丁这类，要确认没有 task 还在执行旧指令
- 不能单靠普通 RCU reader 模型描述的场景

为什么叫 Tasks

因为 grace period 的完成条件，和 task 的状态转换强相关，而不是单纯 CPU 是否经历一次 quiescent state。

普通 RCU 的 reader：

- 常见是 rcu_read_lock() / rcu_read_unlock()
- reader 很轻
- grace period 主要按 CPU quiescent state 推进

RCU Tasks：

- 不依赖普通 rcu_read_lock()
- 它默认把“task 正在内核里跑某些代码”本身当成可能的 reader
- grace period 往往要等 task 发生 context switch、回到用户态、idle，或者别的能证明“它不在旧执行点了”的事件


## codex 关于 rcu 变体的两个问题
<!-- 54a59be0-2f28-4552-8733-faa6f23d1352 -->

1. RCU Tasks、RCU Tasks Trace、RCU Tasks Rude 三者分别是什么
2. 为什么 eBPF/tracing/text patching 更适合用它们而不是普通 RCU


可以先按“读侧临界区是怎么定义的”来记：

- RCU Tasks
  读侧没有显式 rcu_read_lock()。
  它把“一个 task 从当前执行点走到一次自愿调度、用户态、idle 等安全点之前”视为隐式读侧。代码注释在 kernel/rcu/tasks.h:1178 说得很直接：它假定读侧在 schedule()、
  cond_resched_tasks_rcu_qs()、进 idle、回用户态时结束。
- RCU Tasks Rude
  也是无显式 reader 标记，但更强硬。它要覆盖“RCU 没在 watching 的地方”以及 preempt-disabled 区域，所以通过更粗暴的跨 CPU 同步推进 GP。文档在 Documentation/RCU/
  Design/Requirements/Requirements.rst:2759 明说它会打扰实时负载和 idle CPU。
- RCU Tasks Trace
  这个有显式 reader：rcu_read_lock_trace() / rcu_read_unlock_trace()，见 kernel/rcu/tasks.h:2003。它是给 tracing 场景准备的，特点是 reader 可以睡眠，而且比 SRCU 更
  想压低读侧开销。文档说明在 Documentation/RCU/Design/Requirements/Requirements.rst:2779。

一句话压缩：

- Tasks：等 task 走过一个“自然安全点”
- Tasks Rude：强制逼所有 CPU/task 过安全点
- Tasks Trace：给 tracing/sleepable reader 的显式 task-RCU

为什么普通 RCU 不够

普通 synchronize_rcu() 适合“数据结构 reader 在普通 RCU read-side critical section 里”的模型。
但 eBPF、ftrace、text patching 这类问题常常不是“某个指针还被读着”，而是：

- 可能还有 task 停在旧 trampoline / 旧指令流里
- task 可能被 preempt 在 trampoline 中间
- 有的路径发生在 RCU 不 watching 的时刻
- 有的 reader 甚至允许睡眠

这时你要等的是“没有 task 还在执行旧代码”，不是“没有 CPU 还在普通 RCU 读侧”。

文档在 Documentation/RCU/Design/Requirements/Requirements.rst:2730 对 Tasks RCU 的动机讲得很准：trampoline 可能在真正执行很久之前就布置好了，普通 RCU 的 CPU 视角不
够表达“task 还在那段旧代码里”。

为什么 eBPF / tracing / text patching 分别会选它们

- ftrace / function tracing / 某些文本 patching 会用 RCU Tasks 或 RCU Tasks Rude

  原因是 task 可能被 preempt 在 ftrace trampoline 上。普通 RCU 看到一次 CPU quiescent state 也不能证明这个 task 已经离开 trampoline。ftrace 自己的注释在 kernel/
  trace/ftrace.c:3210 已经把这两层都写出来了：
    - synchronize_rcu_tasks_rude()：因为有些 tracer 运行点 “RCU is not watching”
    - synchronize_rcu_tasks()：因为 preemptive kernel 下 task 可能卡在 trampoline 上，仅仅“某 CPU 调度过了”不够
- BPF trampoline 会同时用 RCU Tasks Trace 和 RCU Tasks

  kernel/bpf/trampoline.c:330 的注释很典型：
    - sleepable BPF programs 用 rcu_read_lock_trace 保护，所以释放时先 call_rcu_tasks_trace()
    - 普通 BPF programs 和 trampoline 剩余那几条 asm 指令，再用 call_rcu_tasks()

  这说明 BPF 的需求是分层的：
    - sleepable reader：普通 RCU 不行，得 Tasks Trace
    - 非 sleepable 但可能停在 trampoline 指令里：得 Tasks
- tracepoint / uprobe 这类 faultable tracing 点偏向 RCU Tasks Trace

  include/linux/tracepoint.h:104 明确写了：
    - 非 faultable tracepoint：call_rcu()
    - faultable tracepoint：call_rcu_tasks_trace()

  因为 faultable probe 可能睡眠，普通 RCU reader 模型不成立。
  uprobes 也类似，释放路径里串了 call_rcu_tasks_trace()，见 kernel/events/uprobes.c:667。

怎么选

工程上可以这样判断：

- 只是普通 RCU 指针发布/替换，reader 不睡眠，也不关心旧代码执行点：RCU
- 要确认 task 已经离开旧 trampoline / 旧函数前导 / 旧 patch 点：RCU Tasks
- 连 RCU-not-watching、preempt-disabled、idle 这些更“野”的执行点也要覆盖：RCU Tasks Rude
- tracing reader 可能睡眠，或 faultable/sleepable probe 需要生命周期保护：RCU Tasks Trace

一个容易记错的点

RCU Tasks 和 RCU Tasks Rude 都没有显式 rcu_read_lock*() reader 标记。
RCU Tasks Trace 则有，而且这是它和前两者最本质的 API 区别。这个对照表在 Documentation/RCU/whatisRCU.rst:1170 很清楚。

如果你愿意，我下一条可以继续讲一个更细的问题：
“为什么 Tasks Trace 不直接用 SRCU，而是单独搞一个 flavor？”

## `ps -elf | grep rcu` 里这些线程都是什么
<!-- 10957550-fe1a-45df-b9a5-5b853351edf4 -->

这些线程可以粗分成四类:

- 普通 Tree RCU 的宽限期线程
- expedited RCU 的加急宽限期线程
- Tasks RCU 各个 flavor 的宽限期线程
- RCU 相关 workqueue 的 rescuer kworker

在一台开启 `PREEMPT_RCU`、`TASKS_RCU`、`TASKS_TRACE_RCU` 之类配置的机器上，常见线程名可以这样理解:

- `rcu_preempt`
  普通 Tree RCU 的主 grace-period kthread。它负责推进普通的 RCU 宽限期，确认旧 reader 都过去之后，相关 callback 才能安全执行。
- `rcu_exp_gp_kthread_worker`
  expedited RCU 的工作线程。有人走 `synchronize_rcu_expedited()` 这类“加急”等待路径时，它负责推进更激进的 expedited grace period。
- `rcu_exp_par_gp_kthread_worker/N`
  expedited RCU 的并行辅助线程。大机器上，为了并行做 expedited GP 的一部分扫描和等待工作，会看到多个 `/N` worker。
- `rcu_tasks_kthread`
  普通 Tasks RCU 的 grace-period 线程。它等待的是 task 级别的安全点，而不是普通 RCU 的 CPU quiescent state。
- `rcu_tasks_rude_kthread`
  Tasks Rude RCU 的 grace-period 线程。这个 flavor 更粗暴，会通过更强硬的跨 CPU 同步方式逼系统尽快过安全点，代价也更大。
- `rcu_tasks_trace_kthread`
  Tasks Trace RCU 的 grace-period 线程。它主要服务 tracing/BPF/faultable probe 这类场景，保护的是“task 是否还可能停在旧 tracing 路径里”。
- `kworker/R-rcu_gp`
  这个不是独立 flavor 的主线程，而是 `rcu_gp` workqueue 的 rescuer kworker。这里的 `R-` 表示 rescuer，通常出现在 `WQ_MEM_RECLAIM` workqueue 上，用来在内存压力或普通 worker 不方便运行时兜底执行 `rcu_gp` 队列里的工作。

可以用名字快速猜它的职责:

- 带 `exp` 的，通常和 expedited grace period 有关
- 带 `tasks` 的，通常和 Tasks RCU 有关
- 带 `trace` 的，通常和 tracing/BPF 的 task-level reader 有关
- `kworker/R-...` 通常不是某个子系统自己专门造的新主线程，而是某个 workqueue 的 rescuer

一个容易混淆的点:

`ps` 里看到“很多 rcu 线程”，并不意味着每个线程都在做同一件事。它们分别服务不同的 RCU flavor 和不同的推进策略。真正最常见、最基础的仍然是普通 Tree RCU 的 `rcu_preempt`；而 `rcu_exp_*`、`rcu_tasks_*` 更多是为了加急路径或特殊 reader 模型存在。

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
