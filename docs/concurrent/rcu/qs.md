## 一共存在那些种类的 RCU
<!-- 374bb4cb-2d5b-4364-ad90-5700ee02ebdd -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

这个应该找一下文档看看对应的东西才对的

问了一下 qwen 的意思:

Main Types of RCU Quiescent States

1. RCU-sched quiescent states (classic RCU)
A CPU passes through a quiescent state when it:
 - Executes a voluntary context switch (calls schedule())
 - Enters user mode execution
 - Enters idle loop execution
 - Executes in interrupt or softirq context (since these are non-blocking)

2. RCU-preempt quiescent states (preemptible RCU)
A CPU passes through a quiescent state when:
 - A task exits an RCU read-side critical section via rcu_read_unlock()
 - A task blocks while in an RCU read-side critical section (the task is added to a blocked list)
 - A context switch occurs (voluntary or involuntary)

3. RCU Tasks quiescent states
For task-based RCU variants:
 - Voluntary context switches (schedule())
 - Execution of cond_resched_tasks_rcu_qs()
 - Entry into user space
 - CPU idle state
 - Task exit (via exit_tasks_rcu())

Specific Conditions That Trigger Quiescent States

Context Switches
When `rcu_note_context_switch()` is called during a context switch:
 - If the previous task was in an RCU read-side critical section, it's added to a blocked list
 - The CPU is marked as being in a quiescent state
 - This allows the grace period to progress

RCU Read-Side Critical Section Exit
When `rcu_read_unlock()` is called:
 - Decrements the nesting counter
 - If it reaches zero (outermost exit), reports a quiescent state
 - For preemptible RCU, this may trigger rcu_read_unlock_special() to handle special cases

Dyntick-idle States
For CPUs in dyntick-idle (NO_HZ_FULL):
 - CPU execution is considered a quiescent state when in user mode or idle
 - The `rcu_flavor_sched_clock_irq()` function checks for quiescent states when taking interrupts from user mode or idle

Task-Based RCU Variants
 - RCU Tasks: Quiescent states occur at voluntary context switches, `cond_resched_tasks_rcu_qs()`, user execution, and idle
 - RCU Tasks Trace: More fine-grained, with explicit `rcu_read_lock_trace()/rcu_read_unlock_trace()` primitives
 - RCU Tasks Rude: Forces context switches on all CPUs to ensure quiescent states

Key Implementation Details

The kernel tracks quiescent states through:
 - Per-CPU rcu_data structures with cpu_no_qs flags
 - rcu_qs() function to mark quiescent states
 - Task-level tracking via rcu_read_lock_nesting and rcu_blocked_node fields
 - Various state flags to track whether a CPU needs to report a quiescent state

The quiescent state mechanism is essential for RCU grace periods to complete, as RCU waits for all CPUs to pass through quiescent states before
allowing old data to be freed after an update operation.

关于 RCU-preempt ，ds 的解释:

关键变化：
不再只看 CPU
而是维护：
- task_struct::rcu_read_lock_nesting
- rcu_node 中的 task 队列
因此：
即使发生了 context switch，只要被切走的 task 仍在 RCU read-side，grace period 也不能结束。

在 preemptible RCU 中：
task 退出其最后一个 rcu_read_unlock()
或 task 结束 / 进入 idle
或 task 被迁移并标记完成 QS
这是 task 级 quiescent state。

关于

Classic / Preemptible RCU 有一个共同前提：
RCU 读侧临界区必须包含调度点，或能被调度感知
但某些代码段：
- 不能插入调度点
- 也不会调用 rcu_read_lock()

Tasks RCU 的基本规则：

一个 task 只有在“经过一次明确的调度安全点”后，才被认为进入 quiescent state
这些安全点包括：
- 上下文切换
- 系统调用边界
- 用户态执行
- 任务退出

Tasks RCU 实际上是一组 RCU：
| 类型              | 覆盖范围                  |
| ----------------- | ----------------          |
| `RCU Tasks`       | 调度点                    |
| `RCU Tasks Trace` | tracing / ftrace          |
| `RCU Tasks Rude`  | 极端路径（不可插入 hook） |


最强的可见性保证:
最慢的 grace period

主要用于：
- BPF
- kprobes
- tracing
- 安全模块

(似乎是那么回事，但是可以从一些实验入手
1. 如果关闭 CONFIG_PREEMPT ，那么 rcu 的代码是不是会很多都被关闭了，因为现在没有 preeempt rcu 了)
2. task rcu 测试
3. preempt 模式下，那岂不是 cpu 很难进入 quiescent ，如果在 rcu_read_lock 中 context switch 了，有什么特殊措施来保证吗?

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
