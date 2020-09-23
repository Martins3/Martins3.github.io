# Reading notes from : http://www.wowotech.net/

## CFS调度器（3）-组调度 : http://www.wowotech.net/process_management/449.html

> task_group.shares : 调度实体有权重的概念，以权重的比例分配CPU时间。用户组同样有权重的概念，share就是task_group的权重。
> task_group.load_avg : 整个用户组的负载贡献总和。


![](http://www-x-wowotech-x-net.img.abc188.com/content/uploadfile/201811/1f8a1541854243.png)

在结合 sched_init 中间的代码，可以完全的确定:
```c
	for_each_possible_cpu(i) {
		struct rq *rq;

		rq = cpu_rq(i);
		raw_spin_lock_init(&rq->lock);
		rq->nr_running = 0;
		rq->calc_load_active = 0;
		rq->calc_load_update = jiffies + LOAD_FREQ;
		init_cfs_rq(&rq->cfs);
		init_rt_rq(&rq->rt);
		init_dl_rq(&rq->dl);
#ifdef CONFIG_FAIR_GROUP_SCHED
		root_task_group.shares = ROOT_TASK_GROUP_LOAD;
		INIT_LIST_HEAD(&rq->leaf_cfs_rq_list);
		rq->tmp_alone_branch = &rq->leaf_cfs_rq_list;
		/*
		 * How much CPU bandwidth does root_task_group get?
		 *
		 * In case of task-groups formed thr' the cgroup filesystem, it
		 * gets 100% of the CPU resources in the system. This overall
		 * system CPU resource is divided among the tasks of
		 * root_task_group and its child task-groups in a fair manner,
		 * based on each entity's (task or task-group's) weight
		 * (se->load.weight).
		 *
		 * In other words, if root_task_group has 10 tasks of weight
		 * 1024) and two child groups A0 and A1 (of weight 1024 each),
		 * then A0's share of the CPU resource is:
		 *
		 *	A0's bandwidth = 1024 / (10*1024 + 1024 + 1024) = 8.33%
		 *
		 * We achieve this by letting root_task_group's tasks sit
		 * directly in rq->cfs (i.e root_task_group->se[] = NULL).
		 */
		init_cfs_bandwidth(&root_task_group.cfs_bandwidth);
		init_tg_cfs_entry(&root_task_group, &rq->cfs, NULL, i, NULL);
#endif /* CONFIG_FAIR_GROUP_SCHED */
```

1. root_task_group 中间就是和 `rq->cfs_rq` 相对应的
2. `rq->cfs_rq` 也可以放 普通的 entities
3. `void init_tg_cfs_entry(struct task_group *tg, struct cfs_rq *cfs_rq, struct sched_entity *se, int cpu, struct sched_entity *parent)` 两个使用的位置是很清晰的。



o
```c
/*
 * scheduler tick hitting a task of our scheduling class.
 *
 * NOTE: This function can be called remotely by the tick offload that
 * goes along full dynticks. Therefore no local assumption can be made
 * and everything must be accessed through the @rq and @curr passed in
 * parameters.
 */
static void task_tick_fair(struct rq *rq, struct task_struct *curr, int queued)
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &curr->se;

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);
		entity_tick(cfs_rq, se, queued);
	}

	if (static_branch_unlikely(&sched_numa_balancing))
		task_tick_numa(rq, curr);
}

static void
entity_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr, int queued)
{
	/*
	 * Update run-time statistics of the 'current'.
	 */
	update_curr(cfs_rq);

	/*
	 * Ensure that runnable average is periodically updated.
	 */
	update_load_avg(cfs_rq, curr, UPDATE_TG);
	update_cfs_group(curr);

#ifdef CONFIG_SCHED_HRTICK
	/*
	 * queued ticks are scheduled to match the slice, so don't bother
	 * validating it and just reschedule.
	 */
	if (queued) {
		resched_curr(rq_of(cfs_rq));
		return;
	}
	/*
	 * don't let the period tick interfere with the hrtick preemption
	 */
	if (!sched_feat(DOUBLE_TICK) &&
			hrtimer_active(&rq_of(cfs_rq)->hrtick_timer))
		return;
#endif

	if (cfs_rq->nr_running > 1)
		check_preempt_tick(cfs_rq, curr);
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void
check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
	unsigned long ideal_runtime, delta_exec;
	struct sched_entity *se;
	s64 delta;

	ideal_runtime = sched_slice(cfs_rq, curr); // sched_slice 分析可以运行的时间
	delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
	if (delta_exec > ideal_runtime) {
		resched_curr(rq_of(cfs_rq));
		/*
		 * The current task ran long enough, ensure it doesn't get
		 * re-elected due to buddy favours.
		 */
		clear_buddies(cfs_rq, curr);
		return;
	}

	/*
	 * Ensure that a task that missed wakeup preemption by a
	 * narrow margin doesn't have to wait for a full slice.
	 * This also mitigates buddy induced latencies under load.
	 */
	if (delta_exec < sysctl_sched_min_granularity)
		return;

	se = __pick_first_entity(cfs_rq);
	delta = curr->vruntime - se->vruntime;

	if (delta < 0)
		return;

	if (delta > ideal_runtime)
		resched_curr(rq_of(cfs_rq));
}
```

所有的计算 share load_weight 之类的各种蛇皮，
一是为了确定 vruntime 时间是不是超过，
二是进行CPU迁移。

@todo 但是实际上，sched_slice 的计算仅仅利用了 cfs_rq.load 以及 se.load 进行比例计算而已。
和 avg 之类的没有任何关系，也没有处理其中的，而且和 task_group 没有任何关系 ?

> entity_tick()函数继续调用check_preempt_tick()函数，*这部分在之前的文章已经说过了。*
> check_preempt_tick()函数会根据满足抢占当前进程的条件下设置TIF_NEED_RESCHED标志位。
> 满足抢占条件也很简单，只要顺着`se->parent`这条链表遍历下去，如果有一个se运行时间超过分配限额时间就需要重新调度。

只要自己所在的任何一个级别中间出现了超过时间，那么就将自己自己发清理。

![](http://www-x-wowotech-x-net.img.abc188.com/content/uploadfile/201811/a0011541854246.png)

利用 share 确定其运行的时间量，通过其 share 进而得到其中的 group 的 se 的权重. share 并不是一个动态刷新的过程，而是当 task group 发生变化的时候才会发生变化的。

所以 reweight_entity 需要刷新整个所有的subtree 中间的内容吗 ?

最后分析一波 calc_group_shares 中间:
1. calc_group_shares 使用位置是唯一的，就是在 update_cfs_group 中间的
2. 然后在 reweight_entity 中间实现直接赋值给 se.load (此处的se 为 group se)
3. calc_group_shares 根本不是计算 share 的，而是计算 group se 对应的 load
  1. group se 的 load.weight 可以用于分析计算 time_slice
  2. 所以 share 是如何计算出来的 ? (除了，core.c 中间的来自于 cgroup 的设置，就是)

```c
// 就是简单的对于 tg.shares 进行赋值
// 然后传播一下影响:
int sched_group_set_shares(struct task_group *tg, unsigned long shares)
{
	int i;

	/*
	 * We can't change the weight of the root cgroup.
	 */
	if (!tg->se[0])
		return -EINVAL;

	shares = clamp(shares, scale_load(MIN_SHARES), scale_load(MAX_SHARES));

	mutex_lock(&shares_mutex);
	if (tg->shares == shares)
		goto done;

	tg->shares = shares;
	for_each_possible_cpu(i) {
		struct rq *rq = cpu_rq(i);
		struct sched_entity *se = tg->se[i];
		struct rq_flags rf;

		/* Propagate contribution to hierarchy */
		rq_lock_irqsave(rq, &rf);
		update_rq_clock(rq);
		for_each_sched_entity(se) {
			update_load_avg(cfs_rq_of(se), se, UPDATE_TG);
			update_cfs_group(se);
		}
		rq_unlock_irqrestore(rq, &rf);
	}

done:
	mutex_unlock(&shares_mutex);
	return 0;
}
```

所以，调用的来源在于何处 ?
1. core.c 中间的来源是: cgroup 
2. `int proc_sched_autogroup_set_nice(struct task_struct *p, int nice)` 利用 proc 设置
    1. 从nice 得到 share 的方法: `shares = scale_load(sched_prio_to_weight[idx]);`
    2. scale_load 只是在 64bit 的机器上提升精度

综合以上内容，share 就是 task group 的权重，而且除非管理员干预，整个就是静态的
  1. 但是 se.load.weight 并不是静止的

所以，为什么 task 需要含有三个 prio 而，这里就这么简单了，但是这一个这么复杂 ?

让我们分析一下，计算整个空间:


```
                    tg->shares * grq->load.weight
ge->load.weight = -------------------------------               (1)
		                \Sum grq->load.weight

注: grq := group cfs_rq
```
1. `tg->shares` : tg 的权重，static 的，和 其中的 task 的执行状态无关。
2. `grq->load.weight` : cfs_rq 中间所有的 entity 的 weight 求和，实时发生变化
3. \Sum 求和的对象，对于其中所有的 task_group 控制的所有的 `grq->load.weight` 得到其结果。
  1. 含义是什么: 当前的cfs_rq 持有的 share 权重，在一个 cfs_rq 中间的

2. 感觉和pelt 渐行渐远了!


分析一下 cfs_rq.load 的修改的内容，非常简单: account_entity_enqueue 添加进去的 sched_entity 的 se 就加进去。
> 所以，当在队列中间 se 的weight 发生修改的时候，是不是首先需要 dequeued 然后 enqueue


```c
static void
account_entity_enqueue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	update_load_add(&cfs_rq->load, se->load.weight);
  // 如果到达顶层了，那么就刷新一下 rq 
	if (!parent_entity(se))
		update_load_add(&rq_of(cfs_rq)->load, se->load.weight);

  // CONFIG_SMP 内的内容还是不知道呀!
#ifdef CONFIG_SMP
	if (entity_is_task(se)) {
		struct rq *rq = rq_of(cfs_rq);

		account_numa_enqueue(rq, task_of(se));
		list_add(&se->group_node, &rq->cfs_tasks);
	}
#endif
	cfs_rq->nr_running++;
}
```

两个调用位置:
1. enqueue_entity 
2. reweight_entity

```c
void set_user_nice(struct task_struct *p, long nice)
	if (queued)
		dequeue_task(rq, p, DEQUEUE_SAVE | DEQUEUE_NOCLOCK);

	if (queued) {
		enqueue_task(rq, p, ENQUEUE_RESTORE | ENQUEUE_NOCLOCK);
		/*
		 * If the task increased its priority or is running and
		 * lowered its priority, then reschedule its CPU:
		 */
		if (delta < 0 || (delta > 0 && task_running(rq, p)))
			resched_curr(rq);
	}

// 如果当前的 task_struct 在队列上，
// 想要修改其 weight 首先将其 dequeue 然后 enqueue
```




## Linux调度器：进程优先级 http://www.wowotech.net/process_management/process-priority.html
1. nice : 用户层次的内容。
2. priority : 被映射到统一的地址空间中间
3. weight : prio_to_weight 得到 weight 真正管理的硬通货
4. load_avg sched_avg share : 神仙打架 ?
