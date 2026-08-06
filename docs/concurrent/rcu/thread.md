# kthread && softirq

似乎总是存在 softirq 和 kthread 总是成对出现的:
```c
/*
 * Wake up this CPU's rcuc kthread to do RCU core processing.
 */
static void invoke_rcu_core(void)
{
	if (!cpu_online(smp_processor_id()))
		return;
	if (use_softirq)
		raise_softirq(RCU_SOFTIRQ);
	else
		invoke_rcu_core_kthread();
}
```


```txt
invoke_rcu_core+1
rcu_sched_clock_irq+497
update_process_times+147
tick_sched_handle+34
tick_sched_timer+109
__hrtimer_run_queues+298
hrtimer_interrupt+262
__sysvec_apic_timer_interrupt+127
sysvec_apic_timer_interrupt+157
asm_sysvec_apic_timer_interrupt+18
native_safe_halt+11
default_idle+10
default_idle_call+50
do_idle+478
cpu_startup_entry+25
start_secondary+278
secondary_startup_64_no_verify+213
```

```txt
rcu_core_si+1
__softirqentry_text_start+238
__irq_exit_rcu+181
sysvec_apic_timer_interrupt+162
asm_sysvec_apic_timer_interrupt+18
native_safe_halt+11
default_idle+10
default_idle_call+50
do_idle+478
cpu_startup_entry+25
start_secondary+278
secondary_startup_64_no_verify+213
```

## ps -elf | grep rcu 逐个分析

```txt
1 I root           4       2  0  60 -20 -     0 -      Jul29 ?        00:00:00 [kworker/R-rcu_gp]
1 I root           6       2  0  60 -20 -     0 -      Jul29 ?        00:00:00 [kworker/R-kvfree_rcu_reclaim]
1 I root          16       2  0  80   0 -     0 -      Jul29 ?        00:02:22 [rcu_preempt]
1 S root          17       2  0  80   0 -     0 -      Jul29 ?        00:00:05 [rcu_exp_par_gp_kthread_worker/1]
1 S root          18       2  0  80   0 -     0 -      Jul29 ?        00:00:10 [rcu_exp_gp_kthread_worker]
1 S root          71       2  0  80   0 -     0 -      Jul29 ?        00:00:00 [rcu_exp_par_gp_kthread_worker/2]
1 I root         215       2  0  80   0 -     0 -      Jul29 ?        00:00:00 [rcu_tasks_kthread]
1 I root         216       2  0  80   0 -     0 -      Jul29 ?        00:00:00 [rcu_tasks_rude_kthread]
```

这台机器开了 `PREEMPT_RCU`、`TASKS_RCU`、`TASKS_RUDE_RCU`，没开 `TASKS_TRACE_RCU`，
所以没有 `rcu_tasks_trace_kthread`。逐条分析:

### [kworker/R-rcu_gp]

这个 workqueue(`kernel/rcu/tree.c` 中 `alloc_workqueue("rcu_gp", WQ_MEM_RECLAIM | WQ_PERCPU, 0)`)
承担 force-quiescent-state、expedited GP 轮询、SRCU 状态机推进等工作，
这些工作是 synchronize_rcu() 能返回的前提，所以不能因内存压力卡住，
于是带 `WQ_MEM_RECLAIM` 并配了一个 rescuer 兜底。

这是 RCU 的 rcu_gp 工作队列的 rescuer 线程，属于正常存在的内核线程，不用担心。拆开解释：

名字解析
• R- 前缀表示 rescuer 线程。只有带 WQ_MEM_RECLAIM 标志的 workqueue 才有 rescuer。

它对应的 workqueue

在 kernel/rcu/tree.c:4901：

```c
  rcu_gp_wq = alloc_workqueue("rcu_gp", WQ_MEM_RECLAIM | WQ_PERCPU, 0);
```

它是干什么的

这个工作队列承担的是 RCU grace period 相关的关键工作，典型任务包括：

- 强制静默期（force quiescent state）相关的 strict_work（kernel/rcu/tree.c:2881）——当某个 CPU 迟迟不报 quiescent state 时，RCU 在这个 wq 上排
  队工作去推进 grace period；
- 加速型（expedited）grace period 的轮询工作 exp_poll_wq（kernel/rcu/tree_exp.h:1038）；
- SRCU 的状态机推进工作（kernel/rcu/srcutree.c:866）。

这些工作都有一个共同点：完成它们是内存回收（以及很多其他子系统）能继续推进的前提。

为什么需要 rescuer（R- 前缀）

普通 kworker 在系统内存极度紧张、无法分配新 worker 时可能跑不起来。但如果 RCU 的 grace period 无法推进，synchronize_rcu() 类调用会卡住，进而把
内存回收等路径一起卡死，形成死锁。所以 rcu_gp 工作队列带 WQ_MEM_RECLAIM，内核为它预先创建了一个 rescuer 线程：内存紧张导致无法 fork 新 kworker
时，就由这个 rescuer 串行执行排队的 work，保证 RCU 一定能推进。

关于你看到的这一行

- 优先级 60 -20：rescuer 以高实时优先级（SCHED_FIFO 类）运行，确保内存压力下也能被调度；
- 00:00:00 累计 CPU 时间为零：绝大多数时间它都在睡眠，只有极端内存压力下普通 worker 起不来时才会真正干活。

结论：这是内核 RCU 子系统的保底执行线程，常驻、空闲是正常状态，不需要任何处理。

### [kworker/R-kvfree_rcu_reclaim]

`kvfree_rcu_reclaim` workqueue 的 rescuer kworker，同样 `R-` + `60 -20` 的组合
说明它是某个 `WQ_MEM_RECLAIM` workqueue 的保底执行线程。

这个 workqueue 服务于 `kfree_rcu()` / `kvfree_rcu()` 路径：调用者把要释放的内存
指针交给 RCU，等 grace period 结束、确认没有 reader 还持有旧指针后，
由排在这个队列上的 work 批量执行 `kfree_bulk()` / `kvfree()` 真正把内存还给伙伴系统。

为什么也需要 rescuer: 释放内存本身就是内存回收能推进的前提之一。
如果系统内存紧张到连普通 kworker 都 fork 不出来，而 kfree_rcu 的 reclaim
工作又跑不动，已经"过了宽限期"的内存就一直回不来，加剧内存紧张甚至形成环路等待。
所以内核给这个队列也加了 `WQ_MEM_RECLAIM`，常驻一个高优先级 rescuer。
`00:00:00` 的 CPU 时间同样说明它平时只是待命。

### [rcu_preempt]

普通 Tree RCU 的主 grace-period kthread，整个 RCU 子系统最核心的线程，
`rcu_gp_kthread()` 循环跑在里面。职责:

- 启动新的 grace period(`rcu_gp_init`)
- 等待所有 CPU 报告 quiescent state，中间周期性地做 force-quiescent-state
  检查(`rcu_gp_fqs_loop`)，包括 RCU stall 检测 —— stall.md 里那个
  `task:rcu_preempt state:R` 的 dump 就是它卡在 `rcu_gp_fqs_loop` 里的样子
- GP 结束后做收尾(`rcu_gp_cleanup`)，唤醒排队的 callback 处理

CPU 时间 `00:02:22` 是这几条里最多的，符合预期：所有 `synchronize_rcu()`、
`call_rcu()` 的宽限期推进都由它驱动，它是真正常年干活的那个。

### [rcu_exp_gp_kthread_worker]

expedited RCU 的主 worker，对应 `synchronize_rcu_expedited()` 路径。
普通 GP 是"等所有 CPU 自然经过 quiescent state"，可能拖几十毫秒;
expedited GP 则主动给相关 CPU 发 IPI 把 reader 逼出来，追求微秒到毫秒级完成，
代价是打扰其他 CPU。这个 kthread 就是 expedited GP 状态机
(`kernel/rcu/tree_exp.h`, `synchronize_sched_expedited_wait` 那条链)的执行体。

### [rcu_exp_par_gp_kthread_worker/1, /2]

expedited RCU 的并行辅助 worker，每个 rcu_node 层级一个(名字里的 /N 是 node 编号，
这台机器有 2 个 rcu_node 分组所以有 /1 和 /2)。
expedited GP 需要沿 rcu_node 树逐层 funnel-lock 选 CPU、发 IPI、等结果，
大机器上串行做太慢，于是每 node 一个 worker 并行推进，
最后由 `rcu_exp_gp_kthread_worker` 汇总。`00:00:00` 的 /2 只是还没轮到它干活。

### [rcu_tasks_kthread]

RCU Tasks flavor 的 grace-period 线程(`kernel/rcu/tasks.h`)。
普通 RCU 等的是 CPU 级 quiescent state; Tasks RCU 等的是 task 级安全点 —
每个 task 自愿调度点(voluntary context switch)或 usermode/idle 出口。
它服务的场景是：睡眠时间极长、永远不经过普通 QS 的 reader,
比如某些 tracing 回调。没有显式 `rcu_read_lock()`，靠遍历 task list 确认。

### [rcu_tasks_rude_kthread]

RCU Tasks Rude flavor 的 GP 线程。比 Tasks RCU 更粗暴：它不管 task 在哪，
连 RCU-not-watching、preempt-disabled、idle 里的执行点也当作 reader 来等，
用跨 CPU 的强制同步(对每个 CPU 做 work on each cpu 式的检查)逼系统过安全点。
API 用户极少，主要给那些无法定义边界、只能"等一切停下来"的场景用。

## 快速归类

| 名字特征 | 身份 | 活跃度 |
|---|---|---|
| `kworker/R-*` | workqueue 的 rescuer，内存压力下兜底 | 平时 idle |
| `rcu_preempt` | 普通 Tree RCU 主 GP 线程 | 常年干活 |
| `rcu_exp_*` | expedited GP 主线程 + 每 node 并行 worker | 有人调 expedited 才忙 |
| `rcu_tasks*` | Tasks / Tasks Rude flavor 的 GP 线程 | 有对应 API 使用者才忙 |

状态列也一致：rescuer 和 tasks 线程是 `I`(idle,几乎从不被唤醒),
`rcu_exp_*` 是 `S`(可中断睡眠,等待 expedited 请求),`rcu_preempt` 经常处于 `R`。

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
