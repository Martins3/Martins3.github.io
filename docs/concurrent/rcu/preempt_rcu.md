# preempt rcu

## 内容整理
本文整合散落在各笔记中关于 preemptible RCU（PREEMPT_RCU）的分析，素材来源：
qs.md、overview.md、stall.md、stall-2.md、thread.md、boost.md、2-api.md、
tasks_rcu.md、bh.md、1-usage.md、doc.md。

### 定位：preemptible RCU 解决什么问题

非抢占 RCU 中，`rcu_read_lock()/rcu_read_unlock()` 基本只是
`preempt_disable()/preempt_enable()`（见 include/linux/rcupdate.h），
读临界区内禁止调度，因此一次 context switch 就能证明此前的临界区已经结束
——QS（quiescent state）是 CPU 级的，读者端极其便宜。

可抢占内核（CONFIG_PREEMPTION=y）打破了这个前提：task 可以在 RCU 读临界区
中间被抢占切走。此时"CPU 发生了调度"不再能证明"那个被切走的 reader 已经
离开临界区"。QS 的粒度必须从 CPU 级细化到 task 级，这是 preemptible RCU
的全部动机。

实现集中在 `kernel/rcu/tree_plugin.h`（tree.c include 的四个文件里专门处理
preemption 的那个，见 overview.md），配置开关是 `CONFIG_PREEMPT_RCU`
（doc.md 中 typical config 里它和 TREE_RCU 一起打开）。

关键变化（qs.md 中 ds 的解释）：

- 不再只看 CPU，而是维护 `task_struct::rcu_read_lock_nesting`
- `rcu_node` 中维护被抢占 task 的队列（`blkd_tasks`）
- 因此即使发生了 context switch，只要被切走的 task 仍在读临界区内，
  grace period 也不能结束

### 读者侧语义

来自 2-api.md 对 `rcu_read_lock()` 注释的摘录，三种实现下"临界区内能不能
堵"的规则不同：

- 非抢占 RCU（纯 TREE_RCU / TINY_RCU）：临界区内 block 非法
- PREEMPT_RCU + CONFIG_PREEMPTION：临界区**可以被抢占**，但显式 block 非法
- -rt patchset：可以被抢占，也可以 block——但仅限获取支持优先级继承的
  spinlock 时

另一个 v5.0 起的重要变化（2-api.md 中标注了 `<----- ?????` 的那条）：
`synchronize_rcu()`/`call_rcu()` 除了等显式 `rcu_read_lock()` 界定的临界区，
还要等 preempt-disable、irq-disable、softirq-disable 的代码段，包括硬中断、
softirq、NMI handler。也就是说这些上下文在语义上都被当作隐式 RCU reader。
这解释了 stall-2.md 里 tick 判断 QS 时为什么要看 `preempt_count()`。

实用规则：在 !PREEMPTION 内核里会 block 的东西，就不要放进
`rcu_read_lock()` 临界区——这一条在三种配置下都不会错。

### 核心机制：临界区内被抢占怎么办

这是 preemptible RCU 相对非抢占版本多出来的全部复杂度，证据链在
overview.md：

```
__schedule() -> rcu_note_context_switch() -> rcu_preempt_ctxt_queue()
```

PREEMPTION=y 的 `rcu_note_context_switch()` 比 PREEMPTION=n 版本复杂得多
（overview.md 贴了两个版本的对比）。其注释说明了机制：

> If this task is in an RCU read-side critical section, we will no longer
> be able to rely on the CPU to record that fact, so we enqueue the task
> on the blkd_tasks list. The task will dequeue itself when it exits the
> outermost enclosing RCU read-side critical section. Therefore, the
> current grace period cannot be permitted to complete until the
> blkd_tasks list entries predating the current grace period drain, in
> other words, until rnp->gp_tasks becomes NULL.

要点：

1. task 在读临界区内被切走时，挂到本 rcu_node 的 `blkd_tasks` 链表
   （stall-2.md 的 rcu_node 数据结构里也有这个字段，注释为"被抢占的 RCU
   读临界区任务（用于 PREEMPT_RCU）"）
2. GP 不能结束，直到早于当前 GP 的 blkd_tasks 条目全部 drain，
   即 `rnp->gp_tasks == NULL`
3. task 恢复执行、退出最外层 `rcu_read_unlock()` 时**自己出队**——
   出队动作在读者侧完成，不需要 GP 线程来扫描

### QS 报告路径汇总

qs.md 和 stall-2.md 合起来给出了 preemptible RCU 的 QS 全集：

task 级 QS（preemptible RCU 特有）：

- task 退出最外层 `rcu_read_unlock()`
- task 在临界区内被抢占后，最终退出临界区并从 blkd_tasks 出队
- task 结束 / 进入 idle / 迁移后标记完成

CPU 级 QS（与非抢占 RCU 共用）：

- 上下文切换（对不在读临界区的 task 而言）
- 进 user mode / idle（dynticks EQS，GP 线程可远程确认）
- scheduler tick 观察：tick 打断的内核代码若 `preempt_count == 0`
  （CONFIG_PREEMPT_COUNT=y），证明当时不在 RCU/atomic 区，记 QS
  （kernel/rcu/tree_plugin.h:1062 附近）
- `cond_resched()` 等显式安全点：长时间不调度不进 user/idle 的 CPU，
  RCU 紧急需要 QS 时通过 `rcu_all_qs()` / `rcu_momentary_eqs()` 构造
  零时长 dyntick-idle

慢路径 `rcu_read_unlock_special()`（qs.md）：最外层 unlock 时若
need_qs / blocked 等标志被置位，走慢路径处理，经
`rcu_preempt_deferred_qs()` 报告 QS。非抢占 RCU 没有这条路径——
退出 unlock 本身不报告 QS，这正是非抢占读者端便宜的原因。

两个容易记错的例外（stall-2.md）：

- 普通 IRQ/NMI 的进入退出本身不算 QS，因为它可能打断一个尚未结束的
  reader；tick 在中断里看的是被打断的上下文——打断的是 usermode 或
  idle 才记 QS。`rcu_flavor_sched_clock_irq()` 里的判断：
  `rcu_preempt_depth() > 0 || (preempt_count() & (PREEMPT_MASK|SOFTIRQ_MASK))`
  时不记 QS。
- "退出读临界区时报告 QS"这一条只对 preemptible RCU 成立。

另外 stall-2.md 指出上下文切换这条 QS 路径是"被招募"的：
`jiffies_till_sched_qs` 控制，GP 拖太久时调度器才通过
`rcu_urgent_qs`/`rcu_momentary_eqs()` 更积极地提供 QS 帮助；正常短 GP
主要靠 tick 推断和 idle。

### GP 推进线程 rcu_preempt

thread.md 对 `ps` 输出的分析：`rcu_preempt` 是普通 Tree RCU 的主 GP
kthread，跑 `rcu_gp_kthread()` 循环，是整个 RCU 子系统最常干活的线程：

- `rcu_gp_init` 启动新 GP
- `rcu_gp_fqs_loop` 等待所有 CPU 报 QS，周期做 force-quiescent-state
  检查，包括 stall 检测
- `rcu_gp_cleanup` 收尾，唤醒 callback 处理

名字叫 `rcu_preempt` 只是因为现代发行版内核都开 PREEMPT_RCU，
它就是"普通 RCU 的 GP 线程"，不要和"处理抢占"的职能混淆。

### stall 检测与 preemptible RCU

stall.md / stall-2.md 收集的报错清一色是 `rcu_preempt detected stalls` /
`rcu_preempt self-detected stall`，两种形态：

- **CPU stall**：某 CPU 长时间不报 QS，如 hrtimer 死循环、
  IPI 等不到返回、soft lockup 场景。报错带 NMI backtrace。
- **GP kthread starvation**：`rcu_preempt kthread starved for N jiffies`，
  GP 线程自己拿不到 CPU（stall.md 里 QEMU guest 卡 idle 的案例），
  伴随 `timer wakeup didn't happen` 的 timer 子系统嫌疑提示。

stall-2.md 列的常见 stall 原因里有一条悬而未决的疑问值得记住：
"`preempt_disable()` 区间执行复杂操作"这条，在 preempt kernel 下行为有
很大变化——preempt_disable 区间在 PREEMPTION=y 下同样禁止调度，但 RCU
语义上它已被并入隐式 reader 集合（v5.0 起的规则），stall 表现会不同。

另一个关联：expedited GP 处理被抢占 reader 时，如果在临界区内就置
urgent 标志让外层 unlock 报 QS；如果 PREEMPT_RCU 且任务已被抢占，
则 `set_tsk_need_resched` 请求调度器帮助（stall-2.md 的 expedited 流程图）。

### 与 expedited GP、RCU boost 的关系

boost.md 的总结，三者不是互相替代而是协同：

```
expedited GP
    ├─ 催促 CPU 尽快进入/报告 quiescent state
    └─ 如果仍被 preempted RCU reader 阻塞
           └─ RCU boost 提高该 reader 优先级，使其尽快退出临界区
```

- `synchronize_rcu_expedited()` 加速"寻找 QS"
- `CONFIG_RCU_BOOST`（depends on PREEMPT_RCU，PREEMPT_RT 默认开）加速
  "让阻塞中的 reader 完成"，解决低优先级 reader 被抢占后长期阻塞 GP 的
  优先级反转
- 普通 GP 会等一段 boost 延迟再 boost；阻塞 expedited GP 的 reader 通常
  立即被 boost
- 没有 RCU_BOOST 时 expedited GP 仍可能被一个长期得不到调度的
  preempted reader 拖住

### 与 Tasks RCU 的分界

tasks_rcu.md 把边界说得很清楚：preemptible RCU 已经能跟踪"被抢占在临界区
里的显式 reader"，但**没有显式 rcu_read_lock() 标记的执行点**它管不了。
典型场景是 task 被 preempt 在 ftrace/BPF trampoline 中间——普通 RCU 看到
一次 CPU QS 不能证明这个 task 已离开旧代码。这时需要 Tasks RCU（隐式
reader，等 task 走到自愿调度点）甚至 Tasks Rude（连 RCU-not-watching、
preempt-disabled、idle 里的执行点也覆盖）。

选择的心智模型：普通 preemptible RCU 等"旧 reader 消失"，Tasks RCU 等
"旧 task 不可能还停在某段旧代码里"。

### 调试手段

1-usage.md 收集的相关 tracepoint：

- `rcu/rcu_preempt_task`：task 在读临界区被抢占、挂入 blkd_tasks
- `rcu/rcu_unlock_preempted_task`：被抢占的 task 退出临界区、出队
- `rcu/rcu_quiescent_state_report`、`rcu/rcu_fqs`、`rcu/rcu_stall_warning`

这一对 tracepoint 正好覆盖 blkd_tasks 的入队和出队，观察
"哪些 task 拖住了 GP" 最直接。

### 遗留问题 / todo

来自 qs.md 和 stall-2.md 中记下的、还没动手的点：

1. 如果关闭 CONFIG_PREEMPT，RCU 代码有多少随之关掉？（tree_plugin.h
   的大半逻辑应该都被编译掉，可以 diff 一下 .config 和 vmlinux 符号验证）
2. preempt 模式下 CPU 是不是很难进入 QS？——读完本文"QS 报告路径"一节
   可知答案是否定的：task 级 QS（unlock 出队）+ tick 推断 + idle/EQS
   三条腿走路，真正拖住 GP 的只有"被抢占后长期得不到调度"的 reader，
   那正是 RCU boost 要解决的问题
3. preempt kernel 下 `preempt_disable()` 区间的 stall 行为变化
   （stall-2.md 末尾的疑问）
4. Tasks RCU 的实验（qs.md）

## 一句话概括：

> PREEMPT_RCU 允许任务在 rcu_read_lock() 临界区内被抢占；当任务被切走时，RCU 把它从“CPU 上的读者”转换成“显式挂在 rcu_node 上的任务读者”，宽限期必须等这个任务最终执行外
> 层 rcu_read_unlock()。

### 1. 配置关系

```txt
配置入口在 kernel/rcu/Kconfig:19：

PREEMPT / PREEMPT_RT / PREEMPT_DYNAMIC
                    │
                    ▼
             CONFIG_PREEMPT_RCU
                    │
                    ▼
              CONFIG_TREE_RCU
```

这里容易混淆：

- CONFIG_PREEMPTION：调度器是否允许任务被抢占。
- CONFIG_PREEMPT_RCU：RCU 是否允许读侧临界区被抢占。
- PREEMPT_DYNAMIC 即使运行时使用 preempt=none，编译进去的仍然是 PREEMPT_RCU 实现。
- PREEMPT_RCU 不是一种独立于 TREE_RCU 的实现；它是在 TREE_RCU 上增加任务级读者跟踪。

### 2. 读侧快速路径

核心代码在 kernel/rcu/tree_plugin.h:412：

```txt
void __rcu_read_lock(void)
{
	rcu_preempt_read_enter();
	barrier();
}
```

本质是增加：

current->rcu_read_lock_nesting++;

与非抢占 RCU 不同，它不会直接执行 preempt_disable()。所以快速路径很轻，但每个任务需要在 task_struct 保存状态：

- rcu_read_lock_nesting：嵌套深度
- rcu_read_unlock_special：是否被抢占、是否需要报告 QS 等
- rcu_node_entry：挂入阻塞任务链表
- rcu_blocked_node：记录挂在哪个 rcu_node

字段位于 include/linux/sched.h:931，特殊状态定义在 include/linux/sched.h:786。

正常、没有发生抢占的路径只是：

```txt
rcu_read_lock()
  nesting: 0 → 1
```

读取 RCU 对象

```txt
rcu_read_unlock()
  nesting: 1 → 0
```

只有外层 unlock 且存在特殊状态，才进入慢路径 kernel/rcu/tree_plugin.h:430。

### 3. 临界区内被抢占时发生什么

调度器每次切换任务前都会调用：

kernel/sched/core.c:7044

rcu_note_context_switch(preempt);

PREEMPT_RCU 的处理位于 kernel/rcu/tree_plugin.h:324。

如果当前任务满足：

current->rcu_read_lock_nesting > 0

RCU 会：

1. 设置 current->rcu_read_unlock_special.b.blocked。
2. 记录 current->rcu_blocked_node。
3. 把 current->rcu_node_entry 加入叶子 rcu_node->blkd_tasks。
4. 将该 CPU 报告为 quiescent state。
5. 但是如果该任务属于旧读者，宽限期会继续被 gp_tasks 阻塞。

因此它实现的是一次责任转移：

切换前：CPU qsmask 位代表这个旧读者
                  │
                  ▼ context switch
切换后：task 挂入 blkd_tasks，gp_tasks 指向它
        CPU 的 qsmask 位可以安全清除

这也是 PREEMPT_RCU 最核心的设计。

### 4. blkd_tasks 和 gp_tasks

每个叶子 rcu_node 包含 kernel/rcu/tree.h:41：

- blkd_tasks：所有在 RCU 临界区中被切走的任务。
- gp_tasks：第一个阻塞当前普通宽限期的任务。
- exp_tasks：第一个阻塞 expedited GP 的任务。
- boost_tasks：需要优先级提升的任务。

并不是 blkd_tasks 中所有任务都会阻塞当前 GP。

例如：

T1: rcu_read_lock()
GP: 开始
T1: 被抢占

T1 是 GP 开始前存在的旧读者，所以必须阻塞这个 GP。

反过来：

CPU: 已经为当前 GP 报告 QS
T2: rcu_read_lock()
T2: 被抢占

T2 是 QS 之后开始的新读者，不需要阻塞当前 GP，但仍可能进入 blkd_tasks，为后续 GP 保留跟踪状态。

具体插入位置由 kernel/rcu/tree_plugin.h:162 的状态表决定。其核心目标是用 gp_tasks/exp_tasks 把链表划分成“阻塞当前 GP”和“不阻塞当前 GP”的部分。

### 5. 宽限期什么时候结束

GP 初始化时，RCU 做两件事：

- 用 qsmask 记录还没报告 QS 的 CPU/子节点。
- 将 GP 开始前已经在 blkd_tasks 中的任务纳入 gp_tasks。

入口在 kernel/rcu/tree.c:1948。

所以叶子节点完成当前 GP 的条件是：

rnp->qsmask == 0 && rnp->gp_tasks == NULL

根节点的最终检查位于 kernel/rcu/tree.c:2005。

这比非抢占 TREE_RCU 多了第二个维度：

非抢占 RCU：等待所有相关 CPU 报告 QS
PREEMPT_RCU：等待 CPU 报告 QS
             + 等待被切走的旧读者退出

### 6. 被抢占任务恢复并 unlock

任务可以在另一颗 CPU 上恢复，因为归属信息保存在任务自身：

current->rcu_blocked_node

当它执行最外层 rcu_read_unlock() 时：

```txt
nesting 1 → 0
        │
        ▼
rcu_read_unlock_special()
        │
        ├─ 从 blkd_tasks 删除
        ├─ 推进 gp_tasks / exp_tasks
        └─ 若最后一个阻塞者消失，向 rcu_node 树上层报告
```

相关实现位于：

- kernel/rcu/tree_plugin.h:725
- kernel/rcu/tree_plugin.h:478

如果 unlock 时 IRQ、BH 或 preemption 仍然关闭，RCU 不一定能立刻拿锁和完成报告，因此会通过 softirq、need_resched 或 irq_work 延迟处理。这部分是为了避免在调度器锁、IRQ-
disabled 等敏感环境中形成死锁。

### 7. “允许抢占”不等于“允许睡眠”

普通 PREEMPT_RCU 中：

- 可以被调度器非自愿抢占。
- 不能主动调用可能睡眠的函数。
- 主动 schedule 会触发：

Voluntary context switch within RCU read-side critical section!

检查就在 kernel/rcu/tree_plugin.h:332。

PREEMPT_RT 有一个特殊例外：读侧临界区内可以阻塞在支持优先级继承的 RT spinlock 上。除此之外，仍然不能把普通 RCU 当成 SRCU 使用。

### 结论

PREEMPT_RCU 的主要取舍是：

- 优点：rcu_read_lock() 不需要禁止抢占，降低调度延迟，适合桌面、低延迟和 RT 场景。
- 代价：增加 task_struct 状态、上下文切换检查，以及被抢占读者的链表管理。
- 最重要的不变量：一个旧读者在任何时刻都必须由“CPU 的 qsmask 位”或“任务的 gp_tasks 状态”至少一种方式代表，不能出现跟踪空窗。

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
