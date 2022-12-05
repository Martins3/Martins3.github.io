
- [ ] https://github.com/hamadmarri/cacule-cpu-scheduler : 新的一个 scheduler patch
- https://mp.weixin.qq.com/s/eFvu-tZNpIXaadYHDdkFSQ

## 主要参考资料
- loyenwang
- wowo
- 奔跑

## 总结
### task_tick_fair

![](https://img2020.cnblogs.com/blog/1771657/202003/1771657-20200314235418624-1811691775.png)

- check_preempt_tick : 检查当前进程的 vruntime ，如果是，那么可以继续运行

```txt
#0  update_curr (cfs_rq=cfs_rq@entry=0xffff88807dc2b340) at kernel/sched/fair.c:887
#1  0xffffffff81142b50 in dequeue_entity (flags=9, se=0xffff8881001b8f80, cfs_rq=0xffff88807dc2b340) at kernel/sched/fair.c:4517
#2  dequeue_task_fair (rq=0xffff88807dc2b2c0, p=0xffff8881001b8f00, flags=9) at kernel/sched/fair.c:5835
#3  0xffffffff81f40186 in dequeue_task (flags=9, p=0xffff8881001b8f00, rq=0xffff88807dc2b2c0) at kernel/sched/core.c:2086
#4  deactivate_task (flags=9, p=0xffff8881001b8f00, rq=0xffff88807dc2b2c0) at kernel/sched/core.c:2100
#5  __schedule (sched_mode=sched_mode@entry=0) at kernel/sched/core.c:6448
#6  0xffffffff81f40595 in schedule () at kernel/sched/core.c:6570
#7  0xffffffff8112aee1 in kthreadd (unused=<optimized out>) at kernel/kthread.c:733
#8  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#9  0x0000000000000000 in ?? ()
```

### enqueue_task_fair

![](https://img2020.cnblogs.com/blog/1771657/202003/1771657-20200314235453309-1613887161.png)

当一个 thread 就绪之后就会通过调用 enqueue_task_fair 放到 cfs 的 rb tree 中:

```txt
#0  dequeue_task_fair (rq=0xffff88807dc2b2c0, p=0xffff8881001b8f00, flags=9) at kernel/sched/fair.c:5824
#1  0xffffffff81f40186 in dequeue_task (flags=9, p=0xffff8881001b8f00, rq=0xffff88807dc2b2c0) at kernel/sched/core.c:2086
#2  deactivate_task (flags=9, p=0xffff8881001b8f00, rq=0xffff88807dc2b2c0) at kernel/sched/core.c:2100
#3  __schedule (sched_mode=sched_mode@entry=0) at kernel/sched/core.c:6448
#4  0xffffffff81f40595 in schedule () at kernel/sched/core.c:6570
#5  0xffffffff8112aee1 in kthreadd (unused=<optimized out>) at kernel/kthread.c:733
#6  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#7  0x0000000000000000 in ?? ()
```

## dequeue_task_fair
> 总体来说， dequeue_task_fair 在于和 bandwidth group 相关的更新
> enqueue_task_fair 和其效果非常的相似

![](../../img/source/update_load_avg.png)
![](../../img/source/update_cfs_group.png)
![](../../img/source/dequeue_task_fair.png)
![](../../img/source/cfs_rq_throttled.png)

### pick_next_task

![](https://img2020.cnblogs.com/blog/1771657/202003/1771657-20200314235527658-222083399.png)

- `__schedule`
  - pick_next_task
    - pick_task : 使用 for_each_class 将所有的 sched_class::pick_task 遍历一次
      - `__pick_next_task`
        - pick_next_task_fair

## select_task_rq_fair

- [ ] 这个是做啥的
```txt
#0  select_task_rq_fair (p=0xffff88814a11ae80, prev_cpu=0, wake_flags=8) at kernel/sched/fair.c:7015
#1  0xffffffff8113cff4 in select_task_rq (wake_flags=8, cpu=0, p=0xffff88814a11ae80) at kernel/sched/core.c:3489
#2  try_to_wake_up (p=0xffff88814a11ae80, state=state@entry=3, wake_flags=wake_flags@entry=0) at kernel/sched/core.c:4183
#3  0xffffffff8113d3cc in wake_up_process (p=<optimized out>) at kernel/sched/core.c:4314
#4  0xffffffff8119b859 in hrtimer_wakeup (timer=<optimized out>) at kernel/time/hrtimer.c:1939
#5  0xffffffff8119bde2 in __run_hrtimer (flags=2, now=0xffffc90000003f48, timer=0xffffc900005bb910, base=0xffff888333c1e0c0, cpu_base=0xffff888333c1e080) at kernel/time/hrtimer.c:1685
#6  __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff888333c1e080, now=48877213737236, flags=flags@entry=2, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#7  0xffffffff8119ca71 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#8  0xffffffff810e25d7 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1095
#9  __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1112
#10 0xffffffff81f4137d in sysvec_apic_timer_interrupt (regs=0xffffc9000086bd18) at arch/x86/kernel/apic/apic.c:1106
```

## vruntime 的计算

- update_curr 中
  - curr->vruntime += calc_delta_fair(delta_exec, curr);
  - update_min_vruntime : Each time a new task forks or a task wakes up, its vruntime is assigned to a value that is the maximum of its last updated value and cfs_rq.min_vruntime.

> The lowest vruntime found in the queue is stored in `cfs_rq.min_vruntime`. When a new task is picked to run, the leftmost node of the red-black tree is chosen since that task has had the least running time on the CPU. *Each time a new task forks or a task wakes up, its vruntime is assigned to a value that is the maximum of its last updated value and cfs_rq.min_vruntime.* If not for this, its vruntime would be very small as an effect of not having run for a long time (or at all) and would take an unacceptably long time to catch up to the vruntime of its sibling tasks and hence starve them of CPU time.



使用的 rb_tree 的比较函数为 `__entity_less`，使用 sched_entity::vruntime 实现的



## [ ]  如何实现在 sysctl_sched_latency 时间内中所有的程序都可以运行的


## policy
sched_yield 为什么需要考虑 SCHED_BATCH 的情况:
```c
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

	set_skip_buddy(se);
}
```


## 整理一下 kernel 的 doc
> A group’s unassigned quota is globally tracked, being refreshed back to cfs_quota units at each period boundary. As threads consume this bandwidth it is transferred to cpu-local “silos” on a demand basis. The amount transferred within each of these updates is tunable and described as the “slice”.

> For efficiency run-time is transferred between the global pool and CPU local “silos” in a batch fashion.

## sched_entity 是否对应的
看上去，sched_entity 和 rq 对应:
1. se 特指给 cfs_rq 使用 ?
2. 如果真的是仅仅作为 rb tree 中间的一个 node 显然没有必要高处三个来！

- [ ] so many update load avg:
  - update_tg_load_avg
  - update_load_avg
  - rebalance_domains : 处理 idle , balance 的时机
    - load_balance
