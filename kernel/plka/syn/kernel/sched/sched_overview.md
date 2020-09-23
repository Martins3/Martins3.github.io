# sched overview
> fair rt deadline 核心实现

1. 一共含有什么调度器
2. 调度器中间含有什么函数 : 循环的时候
3. 调度器的架构:
    1. cpu : rq 管理学 : 对于四个rq进行循环使用 next 找到
    2. wake_up 机制中间的内容是什么 ?
4. context switch 的过程
5. 优先级数值的计算的方法是什么 ?
6. 一直想要express的内容 : 线程其实和之间的是一个连续的变化过程，但是用户为什么看到的是进程的概念
https://stackoverflow.com/questions/9305992/if-threads-share-the-same-pid-how-can-they-be-identified
7. 实现迁移task 的方法
8. 当wakeup 一个线程的时候，如何确定其被加载哪一个rq 中间的 ?
9. 总结一下状态的变迁内容。
10. 一个进程如何确定自己的优先级，是不是rt的 ?

调度器的原则: 选择下一个ready 的进程执行以及决定执行的tick 数目 ?

1. 几个很怪异的地方: policy常量 和 (sched_class sched_entity rq) 的数量不是对应的啊!
2. pid 和 tgid 的各自使用的地方


| Filename            | blank | comment | code | explanation                                                                                                                                                                                            |
|---------------------|-------|---------|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| fair.c              | 1659  | 2975    | 5572 |                                                                                                                                                                                                        |
| core.c              | 1033  | 1986    | 4036 | 其实，我们没有core 这一个调度器 ，Core kernel scheduler code and related syscalls                                                                                                                      |
| rt.c                | 515   | 504     | 1712 |                                                                                                                                                                                                        |
| deadline.c          | 406   | 858     | 1493 |                                                                                                                                                                                                        |
| sched.h             | 365   | 445     | 1436 |                                                                                                                                                                                                        |
| topology.c          | 315   | 446     | 1164 | numa 系统                                                                                                                                                                                              |
| debug.c             | 166   | 36      | 804  | 看来，只要含有不确定性的东西(sched lock 之类的)，那么就必定含有debug的内容                                                                                                                             |
| cputime.c           | 142   | 229     | 524  | 利用kernel/timer 测量cpu 执行的时间                                                                                                                                                                    |
| cpufreq_schedutil.c | 156   | 206     | 523  |                                                                                                                                                                                                        |
| idle.c              | 78    | 134     | 270  | idle 进程的实现 ? 空转 ? 低功耗模式 ?                                                                                                                                                                  |
| cpuacct.c           | 65    | 41      | 269  | 统计cpu时间                                                                                                                                                                                            |
| wait.c              | 59    | 138     | 245  | wait queue 实现，被大量的驱动使用，应该不是workqueue 的实现基础                                                                                                                                        |
| clock.c             | 71    | 176     | 234  | @todo                                                                                                                                                                                                  |
| autogroup.c         | 51    | 40      | 179  | 没有配置                                                                                                                                                                                               |
| wait_bit.c          | 41    | 31      | 177  | 新的等待机制?                                                                                                                                                                                          |
| membarrier.c        | 29    | 114     | 175  | membarrier syscall 支持                                                                                                                                                                                |
| cpudeadline.c       | 42    | 67      | 167  | 似乎和 deadline.c 有关，但是 ...                                                                                                                                                                       |
| pelt.c              | 50    | 194     | 155  | Per Entity Load Tracking                                                                                                                                                                               |
| loadavg.c           | 48    | 202     | 150  | hhhh loadavg 找到 average 时间                                                                                                                                                                         |
| completion.c        | 28    | 157     | 144  | completion 机制                                                                                                                                                                                        |
| isolation.c         | 26    | 10      | 117  | 不知道为什么取名字叫做isolation 听到这个东西就烦! Housekeeping management. Manage the targets for routine code that can run on any CPU: unbound workqueues, timers, kthreads and any offloadable work. |
| stats.h             | 20    | 41      | 106  |                                                                                                                                                                                                        |
| stop_task.c         | 30    | 21      | 94   | Simple, special scheduling class for the per-CPU stop tasks 最简单的调度器了，是认识架构的好入口，@todo 所以，谁会使用这一个蛇皮。                                                                     |
| swait.c             | 21    | 18      | 93   | simple wait queue 又一个 wait 机制 ?                                                                                                                                                                   |
| cpupri.c            | 31    | 118     | 92   | numa 系统 ?，cpu                                                                                                                                                                                       |
| stats.c             | 19    | 19      | 90   | /proc/sys/kernel/sched_schedstats 相关的内容                                                                                                                                                           |
| pelt.h              | 14    | 9       | 49   | Per Entity Load Tracking 各种调度器的loadavg                                                                                                                                                           |
| autogroup.h         | 13    | 6       | 41   |                                                                                                                                                                                                        |
| features.h          | 14    | 54      | 24   |                                                                                                                                                                                                        |
| cpudeadline.h       | 4     | 1       | 21   |                                                                                                                                                                                                        |
| Makefile            | 4     | 8       | 19   |                                                                                                                                                                                                        |
| cpufreq.c           | 5     | 38      | 19   | Scheduler code and data structures related to cpufreq  @todo 并不是非常清楚cpufreq f                                                                                                                   |
| cpupri.h            | 5     | 2       | 18   |                                                                                                                                                                                                        |
| sched-pelt.h        | 2     | 2       | 10   |                                                                                                                                                                                                        |


```Makefile
obj-y += core.o loadavg.o clock.o cputime.o
obj-y += idle.o fair.o rt.o deadline.o
obj-y += wait.o wait_bit.o swait.o completion.o

obj-$(CONFIG_SMP) += cpupri.o cpudeadline.o topology.o stop_task.o pelt.o
obj-$(CONFIG_SCHED_AUTOGROUP) += autogroup.o
obj-$(CONFIG_SCHEDSTATS) += stats.o
obj-$(CONFIG_SCHED_DEBUG) += debug.o
obj-$(CONFIG_CGROUP_CPUACCT) += cpuacct.o
obj-$(CONFIG_CPU_FREQ) += cpufreq.o
obj-$(CONFIG_CPU_FREQ_GOV_SCHEDUTIL) += cpufreq_schedutil.o
obj-$(CONFIG_MEMBARRIER) += membarrier.o
obj-$(CONFIG_CPU_ISOLATION) += isolation.o
```

task_struct 中间持有:
1. 调度策略
2. 各种

```c
// 存在 rq ，cfs_rq ，
// sched_entity 



struct task_struct {

	/* Scheduler bits, serialized by scheduler locks: */
	unsigned			sched_reset_on_fork:1;
	unsigned			sched_contributes_to_load:1;
	unsigned			sched_migrated:1;
	unsigned			sched_remote_wakeup:1;
	/* Force alignment to the next boundary: */
	unsigned			:0;


	int				on_rq; // 和迁移有关的

	int				prio;
	int				static_prio;
	int				normal_prio;
	unsigned int			rt_priority;

  // TODO 通过这一个sched_class 进行赋值
	const struct sched_class	*sched_class;

  // TODO 为什么只有这三个，或者说，为什么会多出来下面两个。
	struct sched_entity		se;
	struct sched_rt_entity		rt;
	struct sched_dl_entity		dl;
#ifdef CONFIG_CGROUP_SCHED
	struct task_group		*sched_task_group;
#endif

#ifdef CONFIG_PREEMPT_NOTIFIERS
	/* List of struct preempt_notifier: */
	struct hlist_head		preempt_notifiers;
#endif

#ifdef CONFIG_BLK_DEV_IO_TRACE
	unsigned int			btrace_seq;
#endif

	unsigned int			policy;
  // TODO policy 为什么不是和sched class 对应的，
  // sched_class 成员难道不能成为policy 吗 ?
  // policy 会形成什么影响
	int				nr_cpus_allowed;
	cpumask_t			cpus_allowed;

#ifdef CONFIG_PREEMPT_RCU
	int				rcu_read_lock_nesting;
	union rcu_special		rcu_read_unlock_special;
	struct list_head		rcu_node_entry;
	struct rcu_node			*rcu_blocked_node;
#endif /* #ifdef CONFIG_PREEMPT_RCU */

#ifdef CONFIG_TASKS_RCU
	unsigned long			rcu_tasks_nvcsw;
	u8				rcu_tasks_holdout;
	u8				rcu_tasks_idx;
	int				rcu_tasks_idle_cpu;
	struct list_head		rcu_tasks_holdout_list;
#endif /* #ifdef CONFIG_TASKS_RCU */
}




static inline int idle_policy(int policy) { return policy == SCHED_IDLE; }
static inline int fair_policy(int policy) { return policy == SCHED_NORMAL || policy == SCHED_BATCH; } // TODO 为什么使用
static inline int rt_policy(int policy) { return policy == SCHED_FIFO || policy == SCHED_RR; }
static inline int dl_policy(int policy) { return policy == SCHED_DEADLINE; }

static inline bool valid_policy(int policy) {
	return idle_policy(policy) || fair_policy(policy) ||
		rt_policy(policy) || dl_policy(policy);
}

static inline int task_has_rt_policy(struct task_struct *p) { return rt_policy(p->policy); }
static inline int task_has_dl_policy(struct task_struct *p) { return dl_policy(p->policy); }


extern const struct sched_class stop_sched_class;

extern const struct sched_class dl_sched_class;
extern const struct sched_class rt_sched_class;
extern const struct sched_class fair_sched_class;

extern const struct sched_class idle_sched_class;
```

1. stop_sched_class优先级最高的任务会使用这种策略，会中断所有其他线程，且不会被其他任务打断； // TODO 有这个

1. dl_sched_class就对应上面的deadline调度策略；
1. rt_sched_class就对应RR算法或者FIFO算法的调度策略，具体调度策略由进程的`task_struct->policy`指定；

1. fair_sched_class就是普通进程的调度策略；
1. idle_sched_class就是空闲进程的调度策略。

对于调度策略，其中SCHED_FIFO、SCHED_RR、SCHED_DEADLINE是实时进程的调度策略。

SCHED_NORMAL、SCHED_BATCH、SCHED_IDLE。

> 调度策略和调度常量不一致

两个关键函数 :
> scheduele 进入到ready 的状态，被 semaphore 捕获，成为不是ready 的状态。


# http://www.ece.ubc.ca/~sasha/papers/eurosys16-final29.pdf
Our recent experience with the Linux scheduler revealed
that the pressure to work around the challenging properties
of modern hardware, such as non-uniform memory access
latencies (NUMA), high costs of cache coherency and synchronization, and diverging CPU and memory latencies, resulted in a scheduler with an incredibly complex implementation. As a result, the very basic function of the scheduler,
which is to make sure that runnable threads use idle cores,
fell through the cracks.

These bugs have different root causes, but a common
symptom. The scheduler unintentionally and for a long
time leaves cores idle while there are runnable threads
waiting in runqueues.


The rest of the paper is organized as follows.
- Section 2 describes the architecture of the Linux scheduler.
- Section 3 introduces the bugs we discovered, analyzes their root causes
and reports their effect on performance.
- Section 4 presents the tools. 
- In Section 5 we reflect on the lessons learned as a result of this study and identify open research problems.
- Section 6 discusses related work.
- Section 7 summarizesour finding.

The key decisions made in the scheduler are:
how to determine a thread’s timeslice? and how to pick the next thread to run ?

The scheduler defines a fixed time interval during which
each thread in the system must run at least once. The interval
is divided among threads proportionally to their weights.
*The resulting interval (after division) is what we call the
timeslice*. A thread’s weight is essentially its priority, or
niceness in UNIX parlance.
> 随着进程数量的增加，timeslice 是不是越来越小。会不会进而导致timeslice 小到连context switch 的时间都没，如果没有设置限制的话!
> 不会吧 ! 至少也会运行一个时钟周期。

Context switches are on a critical path, so they
must be fast. Accessing only a core-local queue prevents the
scheduler from making potentially expensive synchronized
accesses, which would be required if it accessed a globally
shared runqueue.
> 由于context switch 导致需要在local queue 中间获取 !

However, in order for the scheduling algorithm to still
work correctly and efficiently in the presence of per-core
runqueues, the runqueues must be kept balanced.
> 周期性的balance 算法，但是问题是: 确定balance的标准是什么，balance 的过程是什么 ?

Load balancing is an
expensive procedure on today’s systems, both computationwise, because it requires iterating over dozens of runqueues,
and communication-wise, because it involves modifying remotely cached data structures, causing extremely expensive
cache misses and synchronization. As a result, the scheduler
goes to great lengths to avoid executing the load-balancing
procedure often.

So in addition to **periodic** loadbalancing, the scheduler also invokes “emergency” load balancing when a core becomes idle, and implements some
load-balancing logic upon placement of *newly created* or
newly awoken threads. 

A basic load balancing
algorithm would compare the load of all cores and then
transfer tasks from the most loaded core to the least loaded
core. Unfortunately this would result in threads being migrated across the machine without considering cache locality or NUMA. Instead, the load balancer uses a hierarchical
strategy.
> @todo load : how to calculate it ?

The load balancing algorithm is summarized in Algorithm 1. Load balancing is run for each scheduling domain,
starting from the bottom to the top.
> 自底向上计算 load 来实现balance


Algorithm 1:
> scheduling group && sched domain ?

# https://en.wikipedia.org/wiki/Completely_Fair_Scheduler
> @todo read the doc

