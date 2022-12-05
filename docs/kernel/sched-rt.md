# rt.c 分析


## sched_attr
> TODO

## 各种 prio 的计算方法
> nice
> 算了，看书吧!
> 始终搞不清楚啊 !

```c
#define MAX_NICE	19
#define MIN_NICE	-20
#define NICE_WIDTH	(MAX_NICE - MIN_NICE + 1)

/*
 * Priority of a process goes from 0..MAX_PRIO-1, valid RT
 * priority is 0..MAX_RT_PRIO-1, and SCHED_NORMAL/SCHED_BATCH
 * tasks are in the range MAX_RT_PRIO..MAX_PRIO-1. Priority
 * values are inverted: lower p->prio value means higher priority.
 *
 * The MAX_USER_RT_PRIO value allows the actual maximum
 * RT priority to be separate from the value exported to
 * user-space.  This allows kernel threads to set their
 * priority to a value higher than any user task.
 *
 * Note: * MAX_RT_PRIO must not be smaller than MAX_USER_RT_PRIO.
 */

// 其实两者没有拆分的必要 !
#define MAX_USER_RT_PRIO	100
#define MAX_RT_PRIO		MAX_USER_RT_PRIO

// 100 到 140 之间
// default : 120
#define MAX_PRIO		(MAX_RT_PRIO + NICE_WIDTH)
#define DEFAULT_PRIO		(MAX_RT_PRIO + NICE_WIDTH / 2)

// TODO 所以，通过设置nice user 永远都不会成为一个RT的吗 ?
/*
 * Convert user-nice values [ -20 ... 0 ... 19 ]
 * to static priority [ MAX_RT_PRIO..MAX_PRIO-1 ],
 * and back.
 */
#define NICE_TO_PRIO(nice)	((nice) + DEFAULT_PRIO)
#define PRIO_TO_NICE(prio)	((prio) - DEFAULT_PRIO)

// 将USER_PRIO设置为 : 0 ~ 40 之间
/*
 * 'User priority' is the nice value converted to something we
 * can work with better when scaling various scheduler parameters,
 * it's a [ 0 ... 39 ] range.
 */
#define USER_PRIO(p)		((p)-MAX_RT_PRIO)
#define TASK_USER_PRIO(p)	USER_PRIO((p)->static_prio)
#define MAX_USER_PRIO		(USER_PRIO(MAX_PRIO))
```


```c
// 一共存在四个prio
	int				prio; // prio 是可以动态变化的 ?
	int				static_prio;
	int				normal_prio; // 为normal policy 设置的 ?
	unsigned int			rt_priority; // TODO 唯独这一个是unsigned ?



/*
 * __normal_prio - return the priority that is based on the static prio
 */
static inline int __normal_prio(struct task_struct *p)
{
	return p->static_prio;
}

// 另一个使用位置 : fork 的时候设置默认值的时候
/* p->prio = p->normal_prio = __normal_prio(p); */



/*
 * Calculate the expected normal priority: i.e. priority
 * without taking RT-inheritance into account. Might be
 * boosted by interactivity modifiers. Changes upon fork,
 * setprio syscalls, and whenever the interactivity
 * estimator recalculates.
 */
static inline int normal_prio(struct task_struct *p)
{
	int prio;

	if (task_has_dl_policy(p))
		prio = MAX_DL_PRIO-1;
	else if (task_has_rt_policy(p))
		prio = MAX_RT_PRIO-1 - p->rt_priority;
	else
		prio = __normal_prio(p);
	return prio;
}

// set_uesr_nice 唯一调用
/*
 * Calculate the current priority, i.e. the priority
 * taken into account by the scheduler. This value might
 * be boosted by RT tasks, or might be boosted by
 * interactivity modifiers. Will be RT if the task got
 * RT-boosted. If not then it returns p->normal_prio.
 */
static int effective_prio(struct task_struct *p)
{
	p->normal_prio = normal_prio(p);
	/*
	 * If we are RT tasks or we were boosted to RT priority,
	 * keep the priority unchanged. Otherwise, update priority
	 * to the normal priority:
	 */
	if (!rt_prio(p->prio))
		return p->normal_prio;
	return p->prio;
}
```


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



## policy 的含义

```c
/*
 * Scheduling policies
 */
 // stop_sched_class 不和任何具体的policy 对应，因为根本policy 是给用户使用的
 // 猜测如下对应: TODO 但是不知道fair 和 rt 如何进行拆分!
#define SCHED_FIFO		1
#define SCHED_RR		2

#define SCHED_NORMAL		0
#define SCHED_BATCH		3

#define SCHED_IDLE		5

/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_DEADLINE		6

// core.c
/**
 * sys_sched_get_priority_min - return minimum RT priority.
 * @policy: scheduling class.
 *
 * Return: On success, this syscall returns the minimum
 * rt_priority that can be used by a given scheduling class.
 * On failure, a negative error code is returned.
 */
SYSCALL_DEFINE1(sched_get_priority_min, int, policy)
{
	int ret = -EINVAL;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		ret = 1;
		break;
	case SCHED_DEADLINE:
	case SCHED_NORMAL:
	case SCHED_BATCH:
	case SCHED_IDLE:
		ret = 0;
	}
	return ret;
}
```

> Man sched

```c
sched_getscheduler // return task_struct->policy


```



## rt 如何区分 fifo rr

`rt_sched_class`就对应 RR 算法或者 FIFO 算法的调度策略，具体调度策略由进程的`task_struct->policy`指定；

SCHED_FIFO

SCHED_RR

https://mp.weixin.qq.com/s/EqKU052H5LxRjE68jeiaDg

> /proc/sys/kernel/sched_rt_runtime_us ,sched_rt_runtime_us 的默认值为 950000，代表 RT 进程可以占用 95% 的 CPU 时间片，剩余 5% 用于响应其他请求

这种现象的出现其实涉及到 Linux 的系统调度，它主要分为两大类：实时调度类和非实时调度类。
- 实时调度类：Linux 上实时调度类主要有 SCHED_RR、SCHED_FIFO 两种，采用 RT 调度算法。调度策略 SCHED_FIFO 和 SCHED_RR 是实时策略，数值越大优先级越高，另外实时调度策略的线程总是比前面三种通常的调度策略优先级更高。通常，调度器会为每个可能的调度优先级(sched_priority value)维护一个可运行的线程列表，并且是以最高静态优先级列表头部的线程作为下次调度的线程。所有的调度都是抢占式的：如果一个具有更高静态优先级的线程转换为可以运行了，那么当前运行的线程会被强制进入其等待的队列中。
- 非实时调度类：非实时调度类就是完全公平调度 CFS(Completely Fair Scheduler)，在内核定义中是 SCHED_OTHER 或者 SCHED_NORMAL，**这两个代表同样的含义**。

SCHED_FIFO 是一种先入先出的调度策略（First In First Out）。该策略简单来说就是一旦进程占用 CPU 则一直运行，直到有更高优先级的任务达到或者自己放弃运行，并且运行期间不响应软中断。
