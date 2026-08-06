# qs

## rcu 什么可以导致进入 quiescent state
<!-- 231d1717-a1ac-4645-ac24-58993fae3fa7 -->

可以的:
- 用户态执行
- idle 循环
- 从 RCU 读临界区退出

不可以的:
- 执行硬件中断/软中断（可能持有 RCU）
- 在 `rcu_read_lock()` / `rcu_read_unlock()` 之间


 结论：基本正确，但有三个地方需要精确化。逐条对照代码和文档（kernel/rcu/tree.c、kernel/rcu/tree_plugin.h、Documentation/RCU/Design/Requirements/Requirements.rst）：

 ✅ 「可以」的三条基本对

 1. 用户态执行 — 对。调度时钟中断从 usermode 打断时（rcu_sched_clock_irq(user=1)），直接记为 QS（tree.c:2711 的 if (user || rcu_is_cpu_rrupt_from_idle())）。
    NO_HZ_FULL 下纯用户态运行由 dyntick 检测跟踪。

 2. idle 循环 — 对。dyntick-idle 是 QS。有个重要的例外/细节：从 idle 被第一层（非嵌套）中断打断也算 QS —— 这正是 rcu_is_cpu_rrupt_from_idle() 存在的意义：
    ```c
      /* Non nested idle interrupt (interrupting section where RCU wasn't watching) */
      if (nmi_nesting == 1)
          return true;
    ```
    因为不能让 tick 打断 idle 后反而丢掉 QS。但 NMI 嵌套（nmi_nesting > 1）不算。

 3. 从 RCU 读临界区退出 — 只对可抢占 RCU（CONFIG_PREEMPTION=y）成立。退出最外层 rcu_read_unlock() 时若 need_qs/blocked 被置位，
 经 rcu_read_unlock_special() →
    rcu_preempt_deferred_qs() 报告 QS。对非抢占 RCU，rcu_read_lock/unlock 只是 preempt_disable/enable，退出本身不报告 QS。

 ❌ 「不可以」的两条基本对，但注意例外

 4. 中断/软中断 — 对，理由很明确（tree_plugin.h 的 rcu_flavor_sched_clock_irq）：
    ```c
      if (rcu_preempt_depth() > 0 ||
          (preempt_count() & (PREEMPT_MASK | SOFTIRQ_MASK))) {
          /* No QS, force context switch if deferred. */
    ```
    中断/软中断上下文 preempt_count 非零，且中断会延续被打断的 RCU 读临界区。

    例外：中断处理程序本身不产生 QS，但 tick 在中断里运行时看被打断的上下文——打断的是 usermode 或 idle 就仍记 QS（见第 1、2 条）。

 5. rcu_read_lock()/rcu_read_unlock() 之间 — 对，QS 的定义就是"不在读临界区中"。

 📌 你没提到的「可以」

 - 上下文切换/调度器： rcu_note_context_switch()（自愿调度、抢占都算）。
	1. 非抢占 RCU 几乎全靠这些特殊位置 + usermode + idle 推断 QS（文档原话："RCU infers quiescent states only at special locations, for example, within the scheduler"）。
	2. 可抢占 RCU 下若任务在临界区里被抢占，任务会被挂到 rnp->blkd_tasks 继续阻塞 grace period—— 此时是"CPU 记了 QS，但任务仍阻塞 GP"，两级概念要分开。
 - rcu_all_qs()：cond_resched() 等显式注册 QS。
 - 还有 get_user() 缺页、cond_resched 等会在内核中途制造 QS——这正是文档警告编译器不得把 get_user() 移进临界区的原因（Requirements.rst 2150 行附近的例子）。

 一句话总结：你的列表是对的，补充两点
 1. 从 idle 被非嵌套中断打断仍算 QS；
 2. "退出读临界区"这条只适用于可抢占 RCU，且别漏掉调度器/上下文切换这个最常见的 QS 来源。


- rcu_softirq_qs()：给长时间运行的 softirq 中途报 QS；
   - cond_resched() 本质就是自愿调度，归入第 1 类；
   - expedited grace period 会主动要求每个 CPU 报 QS（rcu_preempt_deferred_qs）。

还有一个细节：上下文切换这条路径是"被招募"的——jiffies_till_sched_qs 控制，GP 拖太久还没结束时，调度器才会通过
rcu_urgent_qs/rcu_momentary_eqs() 更积极地提供 QS 帮助；正常短 GP 主要靠 tick
推断和 idle。

所以准确说法是：普通 GP 靠 "tick 推断 user/idle/无锁现场 + dynticks EQS"，调度
点（含 cond_resched）和长 softirq 里的 rcu_softirq_qs() 兜底，而不是只靠"特殊位置"。




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
