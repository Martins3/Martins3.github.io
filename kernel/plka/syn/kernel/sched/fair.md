# kernel/sched/fair.md
有点怀疑，cgroup 会将一组的thread的所有的资源统一管理的 ? 而不是存在一个cgroup for mem , cgroup for cpu 之类的 ?


## 整理一下kernel的doc
> A group’s unassigned quota is globally tracked, being refreshed back to cfs_quota units at each period boundary. As threads consume this bandwidth it is transferred to cpu-local “silos” on a demand basis. The amount transferred within each of these updates is tunable and described as the “slice”.

> For efficiency run-time is transferred between the global pool and CPU local “silos” in a batch fashion.


```c
static int cpu_stat_show(struct seq_file *seq, void *v)
{
	struct cgroup __maybe_unused *cgrp = seq_css(seq)->cgroup;
	int ret = 0;

	cgroup_base_stat_cputime_show(seq);
#ifdef CONFIG_CGROUP_SCHED
	ret = cgroup_extra_stat_show(seq, cgrp, cpu_cgrp_id);
#endif
	return ret;
}
```



## pick_next_task 分析
1. CONFIG_FAIR_GROUP_SCHED 如果不考虑 !

## sched_entity 是否对应的
看上去，sched_entity 和 rq 对应:
1. se 特指给 cfs_rq 使用 ?
2. 如果真的是仅仅作为rb tree 中间的一个node 显然没有必要高处三个来！

```c
  // 所以entity 其实应该
	struct sched_entity		se;
	struct sched_rt_entity		rt;
	struct sched_dl_entity		dl;
```

```c
struct sched_entity {
	/* For load-balancing: */
	struct load_weight		load;
	unsigned long			runnable_weight;　// 和 prio 的关系
	struct rb_node			run_node; // 本来以为的全部内容 !
	struct list_head		group_node;
	unsigned int			on_rq;

  // 时间统计
	u64				exec_start;
	u64				sum_exec_runtime;
	u64				vruntime;
	u64				prev_sum_exec_runtime;

	u64				nr_migrations;

	struct sched_statistics		statistics;

#ifdef CONFIG_FAIR_GROUP_SCHED
	int				depth;
	struct sched_entity		*parent;
	/* rq on which this entity is (to be) queued: */
	struct cfs_rq			*cfs_rq;
	/* rq "owned" by this entity/group: */
	struct cfs_rq			*my_q;
#endif

#ifdef CONFIG_SMP
	/*
	 * Per entity load average tracking.
	 *
	 * Put into separate cache line so it does not
	 * collide with read-mostly values above.
	 */
	struct sched_avg		avg;
#endif
};

// TODO 附加内容，还是说这就是全部的rt所需的
struct sched_rt_entity {
	struct list_head		run_list;
	unsigned long			timeout;
	unsigned long			watchdog_stamp;
	unsigned int			time_slice;
	unsigned short			on_rq;
	unsigned short			on_list;

	struct sched_rt_entity		*back;
  // TODO group 的含义不是给thread group
#ifdef CONFIG_RT_GROUP_SCHED
	struct sched_rt_entity		*parent;
	/* rq on which this entity is (to be) queued: */
	struct rt_rq			*rt_rq;
	/* rq "owned" by this entity/group: */
	struct rt_rq			*my_q;
#endif
} __randomize_layout;
```


## 为什么只有三个rq
> stop 和 idle 过于蛇皮，所以其实rq 和 sched_class 是对应的!

```c
	struct cfs_rq		cfs;
	struct rt_rq		rt;
	struct dl_rq		dl;

```


```c
static struct task_struct * pick_next_task_fair(struct rq *rq, struct task_struct *prev, struct rq_flags *rf) {
	struct cfs_rq *cfs_rq = &rq->cfs;

static struct task_struct * pick_next_task_dl(struct rq *rq, struct task_struct *prev, struct rq_flags *rf) {
	struct sched_dl_entity *dl_se;
	struct task_struct *p;
	struct dl_rq *dl_rq;

	dl_rq = &rq->dl;

// stop 利用 rq 中间的特定 task_struct 
static struct task_struct * pick_next_task_stop(struct rq *rq, struct task_struct *prev, struct rq_flags *rf) {
	struct task_struct *stop = rq->stop;

	if (!stop || !task_on_rq_queued(stop))
		return NULL;

	put_prev_task(rq, prev);

	stop->se.exec_start = rq_clock_task(rq);

	return stop;
}

// 连rq->idle 都没有使用 ?
static struct task_struct * pick_next_task_idle(struct rq *rq, struct task_struct *prev, struct rq_flags *rf) {
	put_prev_task(rq, prev);
	update_idle_core(rq);
	schedstat_inc(rq->sched_goidle);

	return rq->idle;
}
```



## 浏览一下所有的函数的作用
1. 为什么这些函数中间都需要持有的参数rq ? 难道不能通过 task_struct 找到 rq 吗 ?
2. 会不会一个rq 中间持有多个 cfs_rq

```c
struct sched_class {
	const struct sched_class *next;

	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);

  // 支持yield的内容
	void (*yield_task)   (struct rq *rq);
	bool (*yield_to_task)(struct rq *rq, struct task_struct *p, bool preempt);

	void (*check_preempt_curr)(struct rq *rq, struct task_struct *p, int flags);

	/*
	 * It is the responsibility of the pick_next_task() method that will
	 * return the next task to call put_prev_task() on the @prev task or
	 * something equivalent.
	 *
	 * May return RETRY_TASK when it finds a higher prio class has runnable
	 * tasks.
	 */
	struct task_struct * (*pick_next_task)(struct rq *rq,
					       struct task_struct *prev,
					       struct rq_flags *rf);
	void (*put_prev_task)(struct rq *rq, struct task_struct *p);

#ifdef CONFIG_SMP
	int  (*select_task_rq)(struct task_struct *p, int task_cpu, int sd_flag, int flags);
	void (*migrate_task_rq)(struct task_struct *p, int new_cpu);

	void (*task_woken)(struct rq *this_rq, struct task_struct *task);

	void (*set_cpus_allowed)(struct task_struct *p,
				 const struct cpumask *newmask);

  // 神奇的函数
	void (*rq_online)(struct rq *rq);
	void (*rq_offline)(struct rq *rq);
#endif

	void (*set_curr_task)(struct rq *rq);
	void (*task_tick)(struct rq *rq, struct task_struct *p, int queued);
	void (*task_fork)(struct task_struct *p);
	void (*task_dead)(struct task_struct *p);

	/*
	 * The switched_from() call is allowed to drop rq->lock, therefore we
	 * cannot assume the switched_from/switched_to pair is serliazed by
	 * rq->lock. They are however serialized by p->pi_lock.
	 */
	void (*switched_from)(struct rq *this_rq, struct task_struct *task);
	void (*switched_to)  (struct rq *this_rq, struct task_struct *task);
	void (*prio_changed) (struct rq *this_rq, struct task_struct *task,
			      int oldprio);

	unsigned int (*get_rr_interval)(struct rq *rq,
					struct task_struct *task);

	void (*update_curr)(struct rq *rq);

#define TASK_SET_GROUP		0
#define TASK_MOVE_GROUP		1

#ifdef CONFIG_FAIR_GROUP_SCHED
	void (*task_change_group)(struct task_struct *p, int type);
#endif
};
```

```c
// dequeue_task 整个指针仅仅使用在一个位置:
// 但是 : dequeue_task 被众多内容使用 !
// TODO 检查一下到底那些蛇皮在调用这一个东西 ?

static inline void dequeue_task(struct rq *rq, struct task_struct *p, int flags)
{
	if (!(flags & DEQUEUE_NOCLOCK))
		update_rq_clock(rq);

	if (!(flags & DEQUEUE_SAVE))
		sched_info_dequeued(rq, p);

	p->sched_class->dequeue_task(rq, p, flags);
}

// dequeue_task 的实现有点麻烦啊!

		cfs_rq->h_nr_running--; // 两次计数 ?
		if (cfs_rq_throttled(cfs_rq)) // 什么机制 ?
		update_load_avg(cfs_rq, se, UPDATE_TG); // 为什么支持load avg 但是load avg 自己是什么 ?
		update_cfs_group(se); // group 机制
    util_est_dequeue(&rq->cfs, p, task_sleep); // 什么神仙东西 ?
    hrtick_update(rq); // 高精度时钟 ?
```

```c

```

## 更新时钟机制

```c
/*
 * Update the current task's runtime statistics.
 */
static void update_curr(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;
	u64 now = rq_clock_task(rq_of(cfs_rq));
	u64 delta_exec;

  // 如果curr的cfs的sched_class 根本不是fair_sched_class呢?
  // 那么就会给cfs 注册上 !
	if (unlikely(!curr))
		return;

	delta_exec = now - curr->exec_start;
	if (unlikely((s64)delta_exec <= 0))
		return;

	curr->exec_start = now;

	schedstat_set(curr->statistics.exec_max,
		      max(delta_exec, curr->statistics.exec_max));

	curr->sum_exec_runtime += delta_exec;
	schedstat_add(cfs_rq->exec_clock, delta_exec);

	curr->vruntime += calc_delta_fair(delta_exec, curr); // 核心
	update_min_vruntime(cfs_rq); // 调整rbtree

  // TODO 不知道own 是什么含义 ?
  // 又是和cgroup 相关的内容 ?
	if (entity_is_task(curr)) {
		struct task_struct *curtask = task_of(curr);

		trace_sched_stat_runtime(curtask, delta_exec, curr->vruntime);
		cgroup_account_cputime(curtask, delta_exec);
		account_group_exec_runtime(curtask, delta_exec);
	}

	account_cfs_rq_runtime(cfs_rq, delta_exec);
}

static void update_curr_fair(struct rq *rq)
{
	update_curr(cfs_rq_of(&rq->curr->se));
}

// 应该处理了一些数值溢出之类的问题
/*
 * delta /= w
 */
static inline u64 calc_delta_fair(u64 delta, struct sched_entity *se)
{
  // NICE_0_LOAD TODO 获取nice的含义 !
	if (unlikely(se->load.weight != NICE_0_LOAD))
		delta = __calc_delta(delta, NICE_0_LOAD, &se->load);

	return delta;
}
```


* ***Latency Tracking***

> 似乎latency 描述 : 在特定的时间之类的所有active 的process 必须处理一下。
> 的确是 preempt 机制的目的相同


## group 机制
1. 所有的好像就是这些吧!
CONFIG_FAIR_GROUP_SCHED
CONFIG_RT_GROUP_SCHED

2. 采用的原因: cpu bandwidth control ?

> The bandwidth allowed for a group is specified using a quota and period. Within
> each given "period" (microseconds), a group is allowed to consume only up to
> "quota" microseconds of CPU time.  When the CPU bandwidth consumption of a
> group exceeds this limit (for that period), the tasks belonging to its
> hierarchy will be throttled and are not allowed to run again until the next
> period.

```c
// 为什么感觉进程是共同管理的原因 : 整个 tg 的，
static int cpu_cfs_quota_write_s64(struct cgroup_subsys_state *css, struct cftype *cftype, s64 cfs_quota_us) { return tg_set_cfs_quota(css_tg(css), cfs_quota_us); }
static u64 cpu_cfs_period_read_u64(struct cgroup_subsys_state *css, struct cftype *cft) { return tg_get_cfs_period(css_tg(css)); }


int tg_set_cfs_quota(struct task_group *tg, long cfs_quota_us)
{
	u64 quota, period;

	period = ktime_to_ns(tg->cfs_bandwidth.period);
	if (cfs_quota_us < 0)
		quota = RUNTIME_INF;
	else
		quota = (u64)cfs_quota_us * NSEC_PER_USEC;

	return tg_set_cfs_bandwidth(tg, period, quota);
}

// TODO 所以，哪里分析过 task_group ?
// TODO us ? 我们 ?
long tg_get_cfs_period(struct task_group *tg)
{
	u64 cfs_period_us;

	cfs_period_us = ktime_to_ns(tg->cfs_bandwidth.period);
	do_div(cfs_period_us, NSEC_PER_USEC);

	return cfs_period_us;
}
```

#### 理解 task_group
> For example, it may be desirable to first provide fair CPU time to each user on the system and then to each task belonging to a user.

1. 如何确定将哪一个 thread 加入到哪一个 group ?
2. 创建 thread group 的创建的时机是什么 ?
3. thread group 让整个 reb tree 如何构建 ?
4. 一个 thread group 会不会对于另一个 thread group 含有优先级 ?
5. 是不是一旦配置了tg那么就所有的thread 都必须属于某一个group 中间 ?

```c
/* Task group related information */
struct task_group {
	struct cgroup_subsys_state css;
  // cgroup 如影随形，cgroup 和 rlimit 是相同的机制吗 ?
  // 使用在什么位置了 : mem cpu io and ?

#ifdef CONFIG_FAIR_GROUP_SCHED
  // 居然thread group 中间的thread 可以出现在不同的CPU上
	/* schedulable entities of this group on each CPU */
	struct sched_entity	**se;
  // TODO cpu 不是和rq 一一对应吗? rq 不是和 cfs_rq 意义对应吗 ?
	/* runqueue "owned" by this group on each CPU */
	struct cfs_rq		**cfs_rq;
	unsigned long		shares;

#ifdef	CONFIG_SMP
	/*
	 * load_avg can be heavily contended at clock tick time, so put
	 * it in its own cacheline separated from the fields above which
	 * will also be accessed at each tick.
	 */
	atomic_long_t		load_avg ____cacheline_aligned;
#endif
#endif

#ifdef CONFIG_RT_GROUP_SCHED
	struct sched_rt_entity	**rt_se;
	struct rt_rq		**rt_rq;

	struct rt_bandwidth	rt_bandwidth;
#endif

	struct rcu_head		rcu;
	struct list_head	list;

	struct task_group	*parent;
	struct list_head	siblings;
	struct list_head	children;

#ifdef CONFIG_SCHED_AUTOGROUP
	struct autogroup	*autogroup;
#endif

	struct cfs_bandwidth	cfs_bandwidth;
};

struct cfs_bandwidth {
#ifdef CONFIG_CFS_BANDWIDTH
	raw_spinlock_t		lock;
	ktime_t			period;
	u64			quota;
	u64			runtime;
	s64			hierarchical_quota;
	u64			runtime_expires;
	int			expires_seq;

	short			idle;
	short			period_active;
	struct hrtimer		period_timer;
	struct hrtimer		slack_timer;
	struct list_head	throttled_cfs_rq;

	/* Statistics: */
	int			nr_periods;
	int			nr_throttled;
	u64			throttled_time;

	bool                    distribute_running;
#endif
};
```
> Documentation/admin-guide/cgroup-v1/cgroups.rst


