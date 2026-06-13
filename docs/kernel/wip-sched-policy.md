# 分析 sched policy 和 sched class

  感觉平时接触的更多的是 sched class

- 为什么会出现 policy 和 priority 之间的设置关系

## 两者之间的关系，这其实并不是的
```c
static inline int idle_policy(int policy)
{
	return policy == SCHED_IDLE;
}
static inline int fair_policy(int policy)
{
	return policy == SCHED_NORMAL || policy == SCHED_BATCH;
}

static inline int rt_policy(int policy)
{
	return policy == SCHED_FIFO || policy == SCHED_RR;
}

static inline int dl_policy(int policy)
{
	return policy == SCHED_DEADLINE;
}
static inline bool valid_policy(int policy)
{
	return idle_policy(policy) || fair_policy(policy) ||
		rt_policy(policy) || dl_policy(policy);
}
```

## nice 和 priority
```c
static inline int __normal_prio(int policy, int rt_prio, int nice)
{
	int prio;

	if (dl_policy(policy))
		prio = MAX_DL_PRIO - 1;
	else if (rt_policy(policy))
		prio = MAX_RT_PRIO - 1 - rt_prio;
	else
		prio = NICE_TO_PRIO(nice);

	return prio;
}
```

- sched_setscheduler(2)
- sched_setattr(2)

使用命令行工具 chrt

chrt -i 0 make -j32

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/monitoring_and_managing_system_status_and_performance/tuning-scheduling-policy_monitoring-and-managing-system-status-and-performance


## 分析 man sched(7)

- sched_setscheduler
  - find_process_by_pid
  - sched_setscheduler
    - _sched_setscheduler
      - 组装 sched_attr
      - __sched_setscheduler
        - 各种参数检查
        - user_check_sched_setscheduler : 还是参数检查

## task_struct::policy 是做啥用的


## SCHED_BATCH 只是出现在 yield_task_fair 中

> SCHED_BATCH：抢占不像普通任务那样频繁，因此允许任务运行更长时间，更好地利用缓存，
    不过要以交互性为代价。它很适合批处理工作。

yield_task_fair 中的代码继续分析下，不知道为什么这样就是更少抢占了。

- yield_to_task_fair

```c
DEFINE_SCHED_CLASS(fair) = {
  // ...
	.yield_task		= yield_task_fair,
	.yield_to_task		= yield_to_task_fair,
  // ...
}
```

通过 /proc/self/sched

## 确认一下 SCHED_IDLE 就是 idle class 中的

似乎并不是， kernel/sched/fair.c 中出现过很多次的
task_has_idle_policy 的，所以


## sched_yield 如何实现的
> sched_setattr(2) 中间的 attr 的作用 ?

```c
SYSCALL_DEFINE0(sched_yield)
{
	do_sched_yield();
	return 0;
}

/**
 * sys_sched_yield - yield the current processor to other threads.
 *
 * This function yields the current CPU to other tasks. If there are no
 * other threads running on this CPU then this function will return.
 *
 * Return: 0.
 */
static void do_sched_yield(void)
{
	struct rq_flags rf;
	struct rq *rq;

	local_irq_disable();
	rq = this_rq();
	rq_lock(rq, &rf); // rq 上锁的含义 TODO

	schedstat_inc(rq->yld_count); // 统计信息吗?
	current->sched_class->yield_task(rq);  // 做一下准备工作吗 ?

	/*
	 * Since we are going to call schedule() anyway, there's
	 * no need to preempt or enable interrupts:
	 */
	preempt_disable();
	rq_unlock(rq, &rf);
	sched_preempt_enable_no_resched();

	schedule(); // 真正导致当前被切换的原因
}

// 以fair为例
/*
 * sched_yield() is very simple
 *
 * The magic of dealing with the ->skip buddy is in pick_next_entity.
 */
static void yield_task_fair(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	struct cfs_rq *cfs_rq = task_cfs_rq(curr);
	struct sched_entity *se = &curr->se;

	/*
	 * Are we the only task in the tree?
	 */
	if (unlikely(rq->nr_running == 1))
		return;

	clear_buddies(cfs_rq, se);

  // TODO SCHED_BATCH ?
	if (curr->policy != SCHED_BATCH) {
		update_rq_clock(rq);
		/*
		 * Update run-time statistics of the 'current'.
		 */
		update_curr(cfs_rq);
		/*
		 * Tell update_rq_clock() that we've just updated,
		 * so we don't do microscopic update in schedule()
		 * and double the fastpath cost.
		 */
		rq_clock_skip_update(rq);
	}

  // 当 cfs_rq 中间的skip 的含义
  // TODO skip 的 ref 检查就可以知道吧!
	set_skip_buddy(se);
}

static void set_skip_buddy(struct sched_entity *se)
{
	for_each_sched_entity(se)
		cfs_rq_of(se)->skip = se;
}

/* runqueue on which this entity is (to be) queued */
static inline struct cfs_rq *cfs_rq_of(struct sched_entity *se)
{
	return se->cfs_rq;
}
```

## 总结
感觉，就是 sched_class 是内核概念, policy 是用户态的概念，而 SCHED_IDLE, SCHED_BATCH , SCHED_NORMAL 之类的都是一个在
一个 sched_class 存在细微的调整。
注意 SCHED_IDLE 和 idle scheduler 不是一个东西。
