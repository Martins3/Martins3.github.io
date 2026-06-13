# Linux 调度器学习问题整理
<!-- 6012962f-ab12-4a6f-9864-fd5b195f076b -->

> 本文档整理学习 Linux 调度器过程中遇到的所有问题，按照主题分类组织。
> 对应回答文档: [sched-answers.md](./sched-answers.md)

---

## 1. 基础概念问题

### 1.1 调度器核心概念
1. **load、priority、weight、share 之间的关系是什么？**
   - 这些概念各自代表什么？如何相互转换？

2. **sched_class 和 policy 的关系是什么？**
   - 为什么需要两个层次的概念？
   - SCHED_NORMAL、SCHED_BATCH、SCHED_IDLE 都属于 fair_sched_class，如何区分？

3. **task_struct 中的四个 priority 字段有什么区别？**
   - prio / static_prio / normal_prio / rt_priority
   - 为什么需要这么多不同的优先级表示？

4. **什么是 MLFQ（多级反馈队列）？**
   - Linux 调度器是否实现了 MLFQ？

### 1.2 CFS 核心机制
5. **vruntime 的作用是什么？**
   - 新加入的进程 vruntime 最小，会不会一直占用 CPU？
   - min_vruntime 如何处理数值溢出？

6. **time_slice 和 vruntime 的关系是什么？**
   - 两者如何共同决定进程执行？
   - time_slice 和 time_period 的区别？

7. **如何实现在 sysctl_sched_latency 时间内所有程序都可以运行？**
   - 当进程数量超过 sched_nr_latency 时如何处理？

### 1.3 调度实体
8. **sched_entity 中的 load 和 runnable_weight 的关系是什么？**
   - 为什么需要两个权重字段？

9. **se->on_rq 和 task_struct->on_rq 的区别是什么？**
   - 为什么 sched_entity 中也有 on_rq 字段？

10. **group_node 的作用是什么？**
    - 用于 task group 的实现？

---

## 2. PELT (Per-Entity Load Tracking) 问题

### 2.1 PELT 基础
11. **PELT 解决了什么问题？**
    - 不使用 PELT 的问题是什么？
    - 替代方案是什么？

12. **weight 计算、share 计算、load avg 计算是三件不同的事情吗？**
    - 各自的计算时机和用途？

13. **sched_avg 结构体中的字段含义是什么？**
    - load_sum / load_avg / runnable_load_sum / runnable_load_avg / util_sum / util_avg
    - 为什么需要 sum 和 avg 两个版本？

### 2.2 PELT 计算细节
14. **load_sum 和 load_avg 在 sched_entity 和 cfs_rq 上的区别？**
    - se 的 load_sum 没有权重，而 cfs_rq 的 load_sum 有权重？

15. **update_load_avg 的调用位置非常多，各自的作用是什么？**
    - enqueue/dequeue/periodic tick 等不同场景

16. **propagate_entity_load_avg 的作用是什么？**
    - 为什么需要向上传播 load？

17. **CPU 的 utilization 是如何获取的？**
    - util_avg 的计算方式？

---

## 3. Task Group / Cgroup 问题

### 3.1 Task Group 机制
18. **task_group::shares 和 task_group::load_avg 的区别？**
    - shares 是静态配置，load_avg 是动态统计？

19. **calc_group_shares() 的复杂计算公式是什么意思？**
    - 公式 (1) 到 (6) 的推导逻辑？
    - 为什么需要近似计算？

20. **update_cfs_group 的作用是什么？**
    - 什么时候需要重新计算 group entity 的权重？

21. **grq (group runqueue) 是什么？**
    - 和 cfs_rq 的关系？

### 3.2 层级调度
22. **group 之间如何实现负载均衡？**
    - 不同 cgroup 的进程如何分配 CPU？

23. **se_runnable() 和 se_weight() 对于 task 和 group 的区别？**
    - 为什么会有不同的计算方式？

24. **tg->load_avg := sum tg->cfs_rq[]->avg.load_avg 是如何保证的？**
    - update_tg_load_avg 的实现机制？

---

## 4. SMP 负载均衡问题

### 4.1 负载均衡基础
25. **负载均衡的触发时机有哪些？**
    - idle_balance / rebalance_domains / nohz_idle_balance 的区别？

26. **find_busiest_group 和 find_busiest_queue 的区别？**
    - 如何确定最繁忙的调度组和最繁忙的队列？

27. **balance 的衡量标准是什么？**
    - 如何定义"负载不均衡"？

28. **软中断的 SCHED 为什么集中在 CPU 0 执行？**
    - 是否有规律可循？

### 4.2 调度域 (Scheduling Domain)
29. **sched_domain 和 sched_group 的关系是什么？**
    - 为什么需要两个层次的抽象？

30. **domain 的概念是什么？**
    - SD_BALANCE_WAKE / SD_BALANCE_FORK / SD_BALANCE_EXEC 的区别？

31. **大核小核 (big.LITTLE) 的调度代码在哪里？**
    - 如何实现异构 CPU 的负载均衡？

32. **taskset 的效果是如何实现的？**
    - cpus_allowed 如何限制进程运行在指定 CPU？

### 4.3 NUMA 相关
33. **NUMA balancing 的原理是什么？**
    - task_numa_fault 的调用时机？

34. **load balancing 是否能跨 NUMA 节点？**
    - 如果 load balancing 只处理同一个 die 内的，是否会导致某些线程永远无法迁移？

---

## 5. 抢占 (Preemption) 问题

### 5.1 抢占基础
35. **抢占实现的原理是什么？**
    - TIF_NEED_RESCHED 标志的作用？

36. **schedule() 中为什么要关闭抢占？**
    - preempt_disable 保护的是什么？

37. **为什么 cond_resched 不会导致内核崩溃？**
    - 如果当前持有锁，cond_resched 会怎样？

### 5.2 抢占模式
38. **CONFIG_PREEMPT_NONE / VOLUNTARY / FULL 的区别？**
    - might_sleep / might_resched / cond_resched 的关系？

39. **CONFIG_PREEMPT_DYNAMIC 如何动态控制抢占？**
    - preempt=none/voluntary/full/lazy 参数的效果？

40. **preempt_disable 只是增加 preempt_count，在哪里检查？**
    - raw_irqentry_exit_cond_resched 的检查逻辑？

### 5.3 抢占问题
41. **为什么关闭中断不是安全的抢占保护方式？**
    - cond_resched 可能在关闭中断时触发调度？

42. **nr_involuntary_switches 和 nr_voluntary_switches 的区别？**
    - 什么情况下会被强制切换？

43. **migrate_disable 和 preempt_disable 的区别？**
    - migrate_disable 不会关闭抢占，但保证进程不离开当前 CPU？

---

## 6. 实时调度 (RT) 问题

### 6.1 RT 基础
44. **SCHED_FIFO 和 SCHED_RR 的区别？**
    - 两者都属于 rt_sched_class，如何区分实现？

45. **RT throttling 是什么？**
    - /proc/sys/kernel/sched_rt_runtime_us 的作用？
    - 为什么会出现 "sched: RT throttling activated"？

46. **RT 进程的选核流程？**
    - 与 CFS 的选核有何不同？

47. **RT 进程是否会导致用户态 spinlock 出现问题？**
    - 优先级反转问题？

### 6.2 RT 优先级
48. **实时优先级的范围是多少？**
    - 1-99 的 rt_priority 如何映射到内核 prio？
    - 为什么数值越大优先级越高？

49. **RT 进程的 nice 值是否有效？**
    - "It is important to note that for real time processes, the nice value is not used."

---

## 7. EEVDF 相关问题

### 7.1 EEVDF 基础
50. **EEVDF 解决了 CFS 的什么问题？**
    - "延迟-带宽"耦合是什么意思？

51. **EEVDF 的三大数学基石是什么？**
    - Virtual Runtime / Lag / Virtual Deadline 的含义？

52. **EEVDF 的调度决策流程是什么？**
    - Eligibility Check 和 Deadline Sorting 两步筛选？

53. **EEVDF 真的把 SCHED_BATCH 干掉了吗？**
    - 文档中的这个说法是什么意思？

### 7.2 EEVDF vs CFS
54. **EEVDF 和 CFS 的核心数据结构区别？**
    - WAVL 树和红黑树的区别？

55. **latency_nice 参数如何工作？**
    - 如何通过 sched_setattr 显式声明所需时间片？

---

## 8. 进程管理相关问题

### 8.1 进程关系
56. **pid、ppid、sid、pgid 的区别？**
    - session leader / process group leader / thread group leader 的关系？

57. **real_parent 和 parent 的区别？**
    - 为什么需要两个 parent 字段？

58. **exit_signal 的作用是什么？**
    - thread_group_leader 的判断依据？

59. **父子树状关系的维护目的是什么？**
    - exit 回收资源 / 信号机制？

### 8.2 进程状态
60. **TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 的区别？**
    - 为什么需要两种等待状态？

61. **on_rq 的含义是什么？**
    - 和进程状态的关系？

---

## 9. 内核机制问题

### 9.1 时钟和 tick
62. **scheduler_tick 的作用是什么？**
    - 更新哪些统计信息？

63. **hrtick 和 schedule_tick 的关系？**
    - 高精度定时器的作用？

64. **nohz (tickless) 对调度器的影响？**
    - nohz_idle_balance 的作用？

### 9.2 同步机制
65. **schedule() 中的同步机制？**
    - rq->lock 的获取和释放？

66. **ttwu (try_to_wake_up) 的同步保证？**
    - migration 和 blocking 的内存序保证？

67. **smp_wmb() 在调度器中的作用？**

### 9.3 调试和统计
68. **taskstats.c 和 delayacct.c 如何使用？**
    - getdelays 工具的输出含义？

69. **latencytop.c 是做什么的？**

70. **schedstat 可以做什么？**
    - cpu ready 和 steal time 的关系？

---

## 10. 特定场景问题

### 10.1 错误和异常
71. **update_blocked_averages 的 WARNING 是什么意思？**
    ```
    cfs_rq->avg.load_avg || cfs_rq->avg.util_avg || cfs_rq->avg.runnable_avg
    WARNING at kernel/sched/fair.c:3307
    ```

72. **如何主动触发 RT throttling？**

### 10.2 性能问题
73. **为什么 CPU core 总是在到处跑？**
    - 上下文切换不是需要时间吗？
    - make -j4 时负载为什么不集中在 4 个 core？

74. **context switch 的代价是什么？**
    - 如何测量上下文切换开销？

### 10.3 特殊配置
75. **isolcpus 和 nohz_full 的关系？**
    - isolcpus 是否已经包含 nohz_full 的功能？

76. **cgroup cpu.weight 和 cpu.shares 的关系？**
    - cpu.weight.nice 的转换？

77. **autogroup 是如何工作的？**
    - /proc/pid/autogroup 的效果？

---

## 11. 代码实现问题

### 11.1 关键函数
78. **select_task_rq 和 set_task_cpu 的区别？**
    - 两者都调用 sched_class->select_task_rq？

79. **update_curr 和 update_cfs_group 的区别？**
    - 分别更新什么统计信息？

80. **attach_entity_cfs_rq 和 attach_tasks 的区别？**

### 11.2 数据结构
81. **rq 中内嵌 cfs_rq 的地位是什么？**
    - 如何通过 rq->cfs 找到 root_task_group？

82. **为什么 stop 和 idle sched_class 没有对应的 rq 结构体？**

83. **sched_class 的优先级如何通过链接脚本确定？**
    - __sched_class_highest / __sched_class_lowest？

---

## 12. 扩展和学习问题

### 12.1 新特性
84. **sched_ext (BPF scheduler) 如何工作？**
    - CONFIG_SCHED_CLASS_EXT 的影响？

85. **CONFIG_SCHED_PROXY_EXEC 是什么？**

86. **CONFIG_PREEMPT_LAZY 是什么？**
    - preempt=lazy 的工作机制？

### 12.2 参考资料
87. **Documentation/scheduler/ 目录下的文档如何阅读？**

88. **如何理解 LoyenWang / wowotech / 奔跑吧 Linux 内核 中的调度器文章？**

89. **EEVDF 的论文和内核文档如何结合理解？**

---

## 问题统计

| 类别 | 问题数量 |
|------|----------|
| 基础概念 | 10 |
| PELT | 7 |
| Task Group/Cgroup | 7 |
| SMP 负载均衡 | 11 |
| 抢占 | 9 |
| 实时调度 | 6 |
| EEVDF | 6 |
| 进程管理 | 6 |
| 内核机制 | 8 |
| 特定场景 | 7 |
| 代码实现 | 6 |
| 扩展学习 | 6 |
| **总计** | **89** |

---

## 学习建议

1. **先理解使用，再看代码** - 通过工具观察调度器行为
2. **结合实际测试** - 使用 `chrt`, `taskset`, `cgroup` 等工具
3. **关注设计思想** - 不要陷入函数调用链细节
4. **理解 Trade-offs** - 每个设计决策都有代价

---

*最后更新: 2026-03-04*

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
