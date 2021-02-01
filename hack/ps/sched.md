- loyenwang
- wowo
- 奔跑
- man sched(7) 也是很不错的
# linux kernel scheduler
1. cpu domain
2. nohz

cpu_attach_domain

- [ ] so many update load avg:
  - update_tg_load_avg
  - update_load_avg
  - rebalance_domains : 处理 idle , balance 的时机
    - load_balance

- [ ] sched_group_span
- [ ] load_balance_mask
```c
	struct cpumask *cpus = this_cpu_cpumask_var_ptr(load_balance_mask);
```

- [ ] 计算 sched_group 的时候的 busy 程度的时候，是使用什么来分析的。

- [ ] 存在多个调度类还是可以理解的，优先级高的队列首先清空，但是各种调度策略的影响如何在代码中间具体体现。
```c
/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
```


## smp balance

```c
__init void init_sched_fair_class(void)
{
#ifdef CONFIG_SMP
	open_softirq(SCHED_SOFTIRQ, run_rebalance_domains);

#ifdef CONFIG_NO_HZ_COMMON
	nohz.next_balance = jiffies;
	nohz.next_blocked = jiffies;
	zalloc_cpumask_var(&nohz.idle_cpus_mask, GFP_NOWAIT);
#endif
#endif /* SMP */

}
```
- [ ] 不知道 softirq 的实现，为什么不是直接调用，而是通过一次 softirq 的封装

```c
enum migration_type {
	migrate_load = 0,
	migrate_util,
	migrate_task,
	migrate_misfit
};

/*
 * 'group_type' describes the group of CPUs at the moment of load balancing.
 *
 * The enum is ordered by pulling priority, with the group with lowest priority
 * first so the group_type can simply be compared when selecting the busiest
 * group. See update_sd_pick_busiest().
 */
enum group_type {
	/* The group has spare capacity that can be used to run more tasks.  */
	group_has_spare = 0,
	/*
	 * The group is fully used and the tasks don't compete for more CPU
	 * cycles. Nevertheless, some tasks might wait before running.
	 */
	group_fully_busy,
	/*
	 * SD_ASYM_CPUCAPACITY only: One task doesn't fit with CPU's capacity
	 * and must be migrated to a more powerful CPU.
	 */
	group_misfit_task,
	/*
	 * SD_ASYM_PACKING only: One local CPU with higher capacity is available,
	 * and the task should be migrated to it instead of running on the
	 * current CPU.
	 */
	group_asym_packing,
	/*
	 * The tasks' affinity constraints previously prevented the scheduler
	 * from balancing the load across the system.
	 */
	group_imbalanced,
	/*
	 * The CPU is overloaded and can't provide expected CPU cycles to all
	 * tasks.
	 */
	group_overloaded
};
```

- [ ] migration 和 group_type 的关系是什么 ?


- run_rebalance_domains
  - * trigger_load_balance
  - * nohz_csd_func
  - nohz_csd_func
  - update_blocked_averages
  - rebalance_domains
    - should_we_balance
    - find_busiest_group
      - update_sd_lb_stats
        - update_group_capacity
        - update_sg_lb_stats
        - update_sd_pick_busiest
      - calculate_imbalance : 计算需要迁移多少负载量才能达到均衡
    - find_busiest_queue
    - stop_one_cpu_nowait
    - detach_tasks : 注意，我们只是迁移没有运行的 cpu
      - detach_task
        - deactivate_task
        - set_task_cpu


- [ ] there are **two** cpumask
  - 应该是一个描述这个 sched_group 包含那些 cpu，一个描述那些做 load balance 的 cpu
  - struct sched_group_capacity::cpumask
  - struct sched_group::cpumask

- select_task_rq
  - * sched_exec
  - * select_task_rq
    - * wake_up_new_task
    - * try_to_wake_up
  - select_task_rq_fair
    - wake_affine
      - wake_affine_idle
      - wake_affine_weight
    - select_idle_sibling : 快速路径，首先在附近进行查找
      - available_idle_cpu
      - sched_idle_cpu
      - select_idle_core
      - select_idle_cpu
      - select_idle_smt
    - find_idlest_cpu : 慢速路径，是在不行就在全局查找
      - find_idlest_group
      - find_idlest_group_cpu

- [ ] SD_WAKE_AFFINE 标志位 : 表示运行唤醒进程的CPU可以运行这个被唤醒的进程。

## load
```c
static unsigned long cpu_load(struct rq *rq)
{
	return cfs_rq_load_avg(&rq->cfs);
}
```

```c
/*
 * The load/runnable/util_avg accumulates an infinite geometric series
 * (see __update_load_avg_cfs_rq() in kernel/sched/pelt.c).
 *
 * [load_avg definition]
 *
 *   load_avg = runnable% * scale_load_down(load)
 *
 * [runnable_avg definition]
 *
 *   runnable_avg = runnable% * SCHED_CAPACITY_SCALE
 *
 * [util_avg definition]
 *
 *   util_avg = running% * SCHED_CAPACITY_SCALE
 *
 * where runnable% is the time ratio that a sched_entity is runnable and
 * running% the time ratio that a sched_entity is running.
 *
 * For cfs_rq, they are the aggregated values of all runnable and blocked
 * sched_entities.
 *
 * The load/runnable/util_avg doesn't directly factor frequency scaling and CPU
 * capacity scaling. The scaling is done through the rq_clock_pelt that is used
 * for computing those signals (see update_rq_clock_pelt())
 *
 * N.B., the above ratios (runnable% and running%) themselves are in the
 * range of [0, 1]. To do fixed point arithmetics, we therefore scale them
 * to as large a range as necessary. This is for example reflected by
 * util_avg's SCHED_CAPACITY_SCALE.
 *
 * [Overflow issue]
 *
 * The 64-bit load_sum can have 4353082796 (=2^64/47742/88761) entities
 * with the highest load (=88761), always runnable on a single cfs_rq,
 * and should not overflow as the number already hits PID_MAX_LIMIT.
 *
 * For all other cases (including 32-bit kernels), struct load_weight's
 * weight will overflow first before we do, because:
 *
 *    Max(load_avg) <= Max(load.weight)
 *
 * Then it is the load_weight's responsibility to consider overflow
 * issues.
 */
struct sched_avg {
	u64				last_update_time;
	u64				load_sum;
	u64				runnable_sum;
	u32				util_sum;
	u32				period_contrib;
	unsigned long			load_avg;
	unsigned long			runnable_avg;
	unsigned long			util_avg;
	struct util_est			util_est;
} ____cacheline_aligned;
```

首先，一共三个数值，load, runnable, util, 这三个都是无穷级数
- 表示可运行状态，在就绪队列可运行状态，和正在执行
- [x] 所以 可运行状态 和 在就绪队列 真的区分了吗 ?
  - 确实如此表述的

其次，两个对象，sched_entities 和 rq

- [x] load_sum 对于 entity 只是衰减时间，而对于 rq 是 时间 乘以 权重
   - 分析 `__update_load_avg_se` 和 `__update_load_avg_cfs_rq`，就 load，前者是 1 ，而后者是 `scale_load_down(cfs_rq->load.weight)`

- 分析 `___update_load_avg`, 其实 avg 的作用对于 cfs_rq 就是除以 `get_pelt_divider`
  - 但是对于 se 来说，load 参数为其 weight

```c
static __always_inline void
___update_load_avg(struct sched_avg *sa, unsigned long load)
{
	u32 divider = get_pelt_divider(sa);

	/*
	 * Step 2: update *_avg.
	 */
	sa->load_avg = div_u64(load * sa->load_sum, divider);
	sa->runnable_avg = div_u64(sa->runnable_sum, divider);
	WRITE_ONCE(sa->util_avg, sa->util_sum / divider);
}
```


```c
/*
 * sched_entity:
 *
 *   task:
 *     se_weight()   = se->load.weight
 *     se_runnable() = !!on_rq
 *
 *   group: [ see update_cfs_group() ]
 *     se_weight()   = tg->weight * grq->load_avg / tg->load_avg
 *     se_runnable() = grq->h_nr_running
 *
 *   runnable_sum = se_runnable() * runnable = grq->runnable_sum
 *   runnable_avg = runnable_sum
 *
 *   load_sum := runnable
 *   load_avg = se_weight(se) * load_sum
 *
 * cfq_rq:
 *
 *   runnable_sum = \Sum se->avg.runnable_sum
 *   runnable_avg = \Sum se->avg.runnable_avg
 *
 *   load_sum = \Sum se_weight(se) * se->avg.load_sum
 *   load_avg = \Sum se->avg.load_avg
 */
int __update_load_avg_se(u64 now, struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (___update_load_sum(now, &se->avg, !!se->on_rq, se_runnable(se),
				cfs_rq->curr == se)) {

		___update_load_avg(&se->avg, se_weight(se));
		cfs_se_util_change(&se->avg);
		trace_pelt_se_tp(se);
		return 1;
	}

	return 0;
}

int __update_load_avg_cfs_rq(u64 now, struct cfs_rq *cfs_rq)
{
	if (___update_load_sum(now, &cfs_rq->avg,
				scale_load_down(cfs_rq->load.weight),
				cfs_rq->h_nr_running,
				cfs_rq->curr != NULL)) {

		___update_load_avg(&cfs_rq->avg, 1);
		trace_pelt_cfs_tp(cfs_rq);
		return 1;
	}

	return 0;
}
```


关于 load_avg 和 load_sum 在 rq 和 se 上的区别：
```c
static inline void
enqueue_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	cfs_rq->avg.load_avg += se->avg.load_avg;
	cfs_rq->avg.load_sum += se_weight(se) * se->avg.load_sum;
}

static inline void
dequeue_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	sub_positive(&cfs_rq->avg.load_avg, se->avg.load_avg);
	sub_positive(&cfs_rq->avg.load_sum, se_weight(se) * se->avg.load_sum);
}
```
- [x] 如果 load_avg 和 load_sum 靠这个来维持，那么为什么还存在 `__update_load_avg_cfs_rq`
  - 因为这两个函数是 task 加入到 cfs_rq 中的时候调用
- [x] 上面两个函数还印证的想法 : se 的 sum 都是没有加入权重的，而 load_avg 是增加了权重的

![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200216135939689-531768656.png)

- [x] why has split time into period(1024ms)
  - kernel can't handle float
- [x] why decay, how to decay ?
  - maybe the reason is same with /proc/loadavg

- [x] how to count the time is blocked , or runable ?
  - 状态更新的时候，这些统计函数都会被使用

### cpu load
I think cpu load is used for [load balancing](#load-balancing)

#### global load
- [ ] 忽然发现 /proc/loadavg 的计算不是利用上面的体系的

exported by `/proc/loadavg`

[^9]: The load number is calculated by counting the number of running (currently running or waiting to run) and uninterruptible processes (waiting for disk or network activity). So it's simply a number of processes.

![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200216135559205-958783412.png)


1. scheduler_tick :
    - `curr->sched_class->task_tick(rq, curr, 0);`
    - calc_global_load_tick : update `calc_load_tasks`
    - perf_event_task_tick
2. do_timer : do the calculation

- [ ] `curr->sched_class->task_tick` : what are we doing in it ?
- [ ] what's relation `do_timer` and `scheduler_tick` ?

loadavg decay at rate `1/e` at 1min, 5min, 15min.

## task_group
- [x] struct task_group::load_avg
- [x] struct cfs_rq::tg_load_avg_contrib;
```c
/**
 * update_tg_load_avg - update the tg's load avg
 * @cfs_rq: the cfs_rq whose avg changed
 * @force: update regardless of how small the difference
 *
 * This function 'ensures': tg->load_avg := \Sum tg->cfs_rq[]->avg.load.
 * However, because tg->load_avg is a global value there are performance
 * considerations.
 *
 * In order to avoid having to look at the other cfs_rq's, we use a
 * differential update where we store the last value we propagated. This in
 * turn allows skipping updates if the differential is 'small'.
 *
 * Updating tg's load_avg is necessary before update_cfs_share().
 */
static inline void update_tg_load_avg(struct cfs_rq *cfs_rq, int force)


/* Task group related information */
struct task_group {
	/*
	 * load_avg can be heavily contended at clock tick time, so put
	 * it in its own cacheline separated from the fields above which
	 * will also be accessed at each tick.
	 */
	atomic_long_t		load_avg ____cacheline_aligned;
```
As two comments suggests, `tg->load_avg := \Sum tg->cfs_rq[]->avg.load_avg.`, tg_load_avg_contrib is used for lock efficiency.

- cfs_rq 上挂在的 node 可能是 se, 可能是 tg
  - task_group会为每个CPU再维护一个cfs_rq，这个cfs_rq用于组织挂在这个任务组上的任务以及子任务组

- 一个 task_group 并不会限制其 task 都是运行在哪一个 cpu 上
  - 一个 task_group 的 load_avg 就是其所在 cpu 的 load_avg 的总和
  - 需要 per_cpu 的 cfs_rq 是因为 cfs_rq 是用于管理的
  - 需要 per_cpu 的 se 是因为 se 作为挂载使用的


- [x] struct task_group::shares
  - `share` is exclusive concept used for task_group, describing share of a task_group in all task_groups


- [ ] calc_group_shares()
  - `reweight_entity` : 作用就是将 重新计算一下 load_avg, 因为该 tg 的 weight 发生了变化
第一件事情，当我们试图调整 share 的时候，是不是最终会影响到每一个所有人 task 的 weight ?
  - 这样是不是太慢了，甚至那些没有处于就绪队列的 task 的 weight 都需要重新计算
  - 但是 vruntime 的计算是靠 除以 weight 来的
  - 实际上的策略是，将整个 group 当做一asdfa 个 se, 当 share 调整之后，只是需要重新调整这个 group 的 weight 的
  - 其实所有的 process 都是放到 group 中间的
    - weight 不是直接算到每一个 se 上的
    - 感觉 share 就是 weight 啊
  - group 可能分布于所有的 cpu 上，但是，calc_group_shares 的参数只是一个 se
  - 而 cfs_rq 中间选择其最佳的 se 的时候，显然是从当前的 se 中间选择的
    - 如果 cpu A, B 中间都有进程，并且 A 中间有 tg 的 weight 为 1000, B 中间为 10, 那么 tg 在 A, B 对应的 se 的 weight 调整显然不同

```
                 tg->weight * grq->load.weight
e->load.weight = -----------------------------               (1)
	  \Sum grq->load.weight
```
    - 恐怖注释中间的，`tg->weight` 实际上是 `tg->shares`, 而 `tg->shares` 的数值就是普通的 weight

## bandwidth
利用接口
- /sys/fs/cgroup/cpu/cpu.cfs_quota_us
- /sys/fs/cgroup/cpu/cpu.cfs_period_us

`period`表示周期，`quota`表示限额，也就是在period期间内，用户组的CPU限额为quota值，当超过这个值的时候，用户组将会被限制运行（throttle），等到下一个周期开始被解除限制（unthrottle）；
- [ ] 还是一个 task_group 作为对象来限制吗 ?
  - [ ] 是一个 cpu 还是总的 cpu ?

- [ ] 那么我之前一致说，保证至少运行一点时间的机制在哪里啊 ?

```c
struct cfs_bandwidth {
#ifdef CONFIG_CFS_BANDWIDTH
	raw_spinlock_t		lock;
	ktime_t			period;
	u64			quota;
	u64			runtime; // 记录限额剩余时间，会使用quota值来周期性赋值；
	s64			hierarchical_quota;

	u8			idle;
	u8			period_active; // 周期性计时已经启动；
	u8			slack_started;
	struct hrtimer		period_timer;
	struct hrtimer		slack_timer; // 延迟定时器，在任务出列时，将剩余的运行时间返回到全局池里；
	struct list_head	throttled_cfs_rq;

	/* Statistics: */
	int			nr_periods;
	int			nr_throttled;
	u64			throttled_time;
#endif
};


/* CFS-related fields in a runqueue */
struct cfs_rq {
// ...
#ifdef CONFIG_CFS_BANDWIDTH
	int			runtime_enabled;
	s64			runtime_remaining; // 剩余的运行时间；

	u64			throttled_clock;
	u64			throttled_clock_task;
	u64			throttled_clock_task_time;
	int			throttled;
	int			throttle_count;
	struct list_head	throttled_list;
#endif /* CONFIG_CFS_BANDWIDTH */
```
- [ ] cfs_rq::runtime_remaining 和 cfs_bandwidth::runtime 描述感觉是同一个东西啊
- [x] cfs_bandwidth 会被多个 cfs_rq，是的，注意 bandwidth 的概念一致都是作用于 task_group 的，而不是 se 的
  - [x] 所以，cfs_bandwidth 就是一个全局概念

- `tg_set_cfs_bandwidth` 会从 `root_task_group` 根节点开始，遍历组调度树，并逐个设置限额比率 ；
- 由于 task_group 是层级的，如果顶层的被限制，下面的所有节点都是需要被限制，所以 quota 需要需要累计所有的子节点

注入时间, 或者称之为 runtime_remaining++ :
1. update_curr
2. check_enqueue_throttle :
3. set_next_task_fair : This routine is mostly called to set `cfs_rq->curr` field when a task migrates between groups/classes.

- slack_timer定时器，slack_period周期默认为5ms，在该定时器函数中也会调用distribute_cfs_runtime从全局运行时间中分配runtime；
- slack_timer : 一个用于将未用完的时间再返回到时间池中


A group’s unassigned quota is globally tracked, being refreshed back to cfs_quota units at each period boundary.

```c
/*
 * Amount of runtime to allocate from global (tg) to local (per-cfs_rq) pool
 * each time a cfs_rq requests quota.
 *
 * Note: in the case that the slice exceeds the runtime remaining (either due
 * to consumption or the quota being specified to be smaller than the slice)
 * we will always only issue the remaining available time.
 *
 * (default: 5 msec, units: microseconds)
 */
unsigned int sysctl_sched_cfs_bandwidth_slice		= 5000UL;
```

- bandwidth_slice : 时间是 cpu 时间，而不是 wall clock

- throttle 的两条路径:
  - check_enqueue_throttle : 如果从 runtime pool 中间都借不到资源，那么就只能 throttle
    - account_cfs_rq_runtime
      - `__account_cfs_rq_runtime` : 如果 `cfs_rq->runtime_remaining > 0`，那么就不需要继续了，有钱还借钱，贱不贱啊!
        - assign_cfs_rq_runtime : 借钱开始
          - `__assign_cfs_rq_runtime` : 从 runtime pool 中间尽量取出来给其
            - start_cfs_bandwidth : 如果 period 过期了，那么顺便将 period timer 移动一下
  - check_cfs_rq_runtime
    - throttle_cfs_rq

![](https://img2020.cnblogs.com/blog/1771657/202003/1771657-20200310214423221-158953219.png)


- [x] 可是，我还是无法理解 slack_timer
  - slack_timer：延迟定时器，在任务出列时，将**剩余的运行时间**返回到全局池里；
  - slack_timer定时器，slack_period周期默认为5ms，在该定时器函数中也会调用distribute_cfs_runtime从全局运行时间中分配runtime；
  - 好吧，还是理解的，当存在 task 将自己的时间返回给 runtime pool 的时候，不要立刻进行 distribute, 因为还有可能其他的 task 也在返回，所以等

- dequeue_entity 
  - return_cfs_rq_runtime : return excess runtime on last dequeue
    - `__return_cfs_rq_runtime` :  we know any runtime found here is valid as update_curr() precedes return
      - 将自己持有的时间 `cfs_rq->runtime_remaining` 返回给 runtime pool `cfs_b->runtime`，如果此时有人被 unthrottle, 那么 `start_cfs_slack_bandwidth`
      - runtime_refresh_within : Are we near the end of the current quota period?

```c
/* a cfs_rq won't donate quota below this amount */
static const u64 min_cfs_rq_runtime = 1 * NSEC_PER_MSEC;
/* minimum remaining period time to redistribute slack quota */
static const u64 min_bandwidth_expiration = 2 * NSEC_PER_MSEC;
/* how long we wait to gather additional slack before distributing */
static const u64 cfs_bandwidth_slack_period = 5 * NSEC_PER_MSEC;
```
- unthrottle_cfs_rq 的时候，似乎操作就是 enqueue_task 就可以了，再次之前，runtime pool 的数值必然得到补充了

- [ ] update_curr

# numa balancing
- https://www.linux-kvm.org/images/7/75/01x07b-NumaAutobalancing.pdf : 算是说的很清楚了吧！

- task_numa_work

- [ ] 忽然想到，奔跑上说的内容，是不是只对于 die 级别的，根本没有考虑 numa 层次
  - [ ] 如果 load balancing 只是能处理同一个 die 级别的，这不是会导致有的线程永远无法迁移其他的可用的 core 上去 ?

- update_numa_stats : Gather all necessary information to make NUMA balancing placement decisions that are compatible with standard load balancer. This borrows code and logic from `update_sg_lb_stats` but sharing a common implementation is impractical.

```c
struct task_numa_env {
	struct task_struct *p;

	int src_cpu, src_nid;
	int dst_cpu, dst_nid;

	struct numa_stats src_stats, dst_stats;

	int imbalance_pct;
	int dist;

	struct task_struct *best_task;
	long best_imp;
	int best_cpu;
};
```

- task_numa_fault : 调用者来自于 remote process 的位置，其中
  - numa_migrate_preferred
    - task_numa_migrate :
      - task_numa_find_cpu
        - task_numa_compare : 如果 task 迁移到新的 CPU 上去会更加好 ?

## auto group
man sched(7)
和 setsid 的关系

因为 task_group 是 cgroup 机制下，所以将 task_group 加入到机制中间:
- sched_online_group
- sched_offline_group

## preemption
- [ ] config PREEMPT_VOLUNTARY ?

- [x] So what's the difference between preempt and disable interrupt ?

this [ans](https://stackoverflow.com/questions/9473301/are-there-any-difference-between-kernel-preemption-and-interrupt)
summaries incisively, and we can check the comment of `__schedule`
> When a process is interrupted, the kernel runs some code, which may not be related to what the process does.
> When this is done, two things can happen:
> 1. The same process will get the CPU again.
> 2. A different process will get the CPU. The current process was preempted.
> 
> So preemption will only happen after an interrupt, but an interrupt doesn't always cause preemption.


[^7]
However, in nonpreemptive kernels, the current process cannot be replaced unless it is about to switch
to User Mode.

Therefore, the main characteristic of a preemptive kernel is that a process running in
Kernel Mode can be replaced by another process while in the middle of a kernel
function.

- *一句话总结 : 如果没有打开，那么想要切换，只有在返回到用户态的时候才可以，打开之后，在 interrupt handle 的时候都会进行检查*
  - 当内核不能抢占，一个 user process 阻塞到内核态中间，其无法陷入休眠，并且总是占有 CPU (qemu 重甲测试一下 ?)
  - 一旦 disable 掉 preempt，可以保证接下来执行的代码都是在同一个 CPU 上的

[^7]:
it is greater than zero when any of the following cases occurs:
1. The kernel is executing an interrupt service routine.
2. The deferrable functions are disabled (always true when the kernel is executing a
softirq or tasklet).
3. The kernel preemption has been explicitly disabled by setting the preemption
counter to a positive value.


- [x] I can't find anything about CONFIG_PREEMPT_NONE when tracing `scheduler_tick` ?

task_tick_fair => entity_tick => check_preempt_tick

As `__schedule`'s comment says, scheduler_tick only set TIF_NEED_RESCHED flags on the thread.

- [x] check_preempt_tick and check_preempt_wakeup

former used by timer, latter by kernel code like ttwu
```c
const struct sched_class fair_sched_class = {
	.check_preempt_curr	= check_preempt_wakeup,
```

在 64bit 中间 kmap 和 kmap_atomic 的区别:

1. 为什么这保证了 atomic ?
2. 什么情况需要 atomic ?
```c
static inline void *kmap_atomic(struct page *page)
{
	preempt_disable();
	pagefault_disable();
	return page_address(page);
}
```

- [ ] In fact there are three types of preemption !

detail `kernel/Kconfig.preempt`
- No Forced Preemption (Server)
- Voluntary Kernel Preemption (Desktop) `CONFIG_PREEMPT_VOLUNTARY`
- Preemptible Kernel (Low-Latency Desktop) `CONFIG_PREEMPT`

This's only place that `CONFIG_PREEMPT_VOLUNTARY` used !
```c
#ifdef CONFIG_PREEMPT_VOLUNTARY
extern int _cond_resched(void);
# define might_resched() _cond_resched()
#else
# define might_resched() do { } while (0)
#endif
```

#### preempt count
`CONFIG_PREEMPT` will select `CONFIG_PREEMPT_COUNT`

https://lwn.net/Articles/831678/ : valuable 

- [ ] why preempt need a counter ?

- [ ] I need a more clear and strong reason : why if the process disasble interrupt, it shouldn't be preempted ?


- [ ] preempt.h : where preempt meets need_resched

- [ ] how `__preempt_count` enable ?

- [ ] how to explain `preemptible()` ?

```c
#define preemptible()	(preempt_count() == 0 && !irqs_disabled())
```

- [ ] set_preempt_need_resched
```c
/*
 * We fold the NEED_RESCHED bit into the preempt count such that
 * preempt_enable() can decrement and test for needing to reschedule with a
 * single instruction.
 *
 * We invert the actual bit, so that when the decrement hits 0 we know we both
 * need to resched (the bit is cleared) and can resched (no preempt count).
 */

static __always_inline void set_preempt_need_resched(void)
{
	raw_cpu_and_4(__preempt_count, ~PREEMPT_NEED_RESCHED);
}
```

- [ ] when will preempt happens ?
```c
asmlinkage __visible void __sched notrace preempt_schedule(void)
```


#### preempt locking
https://www.kernel.org/doc/html/latest/locking/preempt-locking.html

https://stackoverflow.com/questions/18254713/why-linux-disables-kernel-preemption-after-the-kernel-code-holds-a-spinlock




#### preempt notes

https://stackoverflow.com/questions/5283501/what-does-it-mean-to-say-linux-kernel-is-preemptive
> If the system allows that task to be preempted while it is running kernel code, then we have what is called a "preemptive kernel." 

https://stackoverflow.com/questions/49414559/linux-kernel-why-preemption-is-disabled-when-use-per-cpu-variable
> There are preemption points throughout the kernel (might_sleep). Forcible preemption = each interrupt (including timer interrupts when time slices run out) is a possible preemption point. 
> 
> *when you finish your business and about to go back to user space your time slice is checked and if its done you go to another process instead of back to user space, so Im trying to understand how this dynamic work now*

really interesting : if one process is about to switch to user space and it's time slice is used up, just switch to another process.


#### preempt notifier
- One process can register multiple notifier
- context_switch
  - prepare_task_switch
    - fire_sched_out_preempt_notifiers
      - for each registered funcion
```c
struct task_struct {
// ...
#ifdef CONFIG_PREEMPT_NOTIFIERS
	/* List of struct preempt_notifier: */
	struct hlist_head		preempt_notifiers;
#endif

/**
 * preempt_notifier_register - tell me when current is being preempted & rescheduled
 * @notifier: notifier struct to register
 */
void preempt_notifier_register(struct preempt_notifier *notifier)
{
	if (!static_branch_unlikely(&preempt_notifier_key))
		WARN(1, "registering preempt_notifier while notifiers disabled\n");

	hlist_add_head(&notifier->link, &current->preempt_notifiers);
}

static void __fire_sched_in_preempt_notifiers(struct task_struct *curr)
{
	struct preempt_notifier *notifier;

	hlist_for_each_entry(notifier, &curr->preempt_notifiers, link)
		notifier->ops->sched_in(notifier, raw_smp_processor_id());
}
```

# uclamp
clamped utilization
