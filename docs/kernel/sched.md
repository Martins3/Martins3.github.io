## 基本结构体
- sched_entity
- sched_rt_entity
- sched_dl_entity

- struct rq
```c
struct rq {
    // ...
    struct cfs_rq       cfs;
    struct rt_rq        rt;
    struct dl_rq        dl;
    // ...
```

- struct cfs_rq     cfs;
- struct rt_rq      rt;
- struct dl_rq      dl;

- 似乎 stop 和 idle 过于蛇皮，没有对应 rq 结构体
- 所以其实 rq 和 sched_class 是对应的!


- sched_class

- sched_entity : 应该有时候，指的是一个 thread ，而有的时候是 task group 的
  - [ ] 有待验证
  - [ ] 为什么不搞成 group 中的 entry 也是公平调度的
```c
#ifdef CONFIG_FAIR_GROUP_SCHED
/* An entity is a task if it doesn't "own" a runqueue */
#define entity_is_task(se)	(!se->my_q)
#else
#define entity_is_task(se)	1
#endif
// 有的 entity 是用于对应的用于管理的，而有的才是真的对应于process 的
```

sched_class 有几种实现：
* stop_sched_class 优先级最高的任务会使用这种策略，会中断所有其他线程，且不会被其他任务打断；
* dl_sched_class 就对应上面的 deadline 调度策略；
* rt_sched_class 就对应 RR 算法或者 FIFO 算法的调度策略，具体调度策略由进程的 task_struct->policy 指定；
* fair_sched_class 就是普通进程的调度策略；
* idle_sched_class 就是空闲进程的调度策略。

## 问题和记录
- [ ] 似乎存在一个机制，让 thread 一个时间段必须运行一段时间。
- [ ] 将奇怪的 thread process / process group / session 总结一下
- [ ] fg bg ctrl-z 都是如何实现的
- [ ] nohz

## cgroup  v2
- https://facebookmicrosites.github.io/cgroup2/docs/cpu-controller.html

## 总结
| func             | desc                                                                                                  |
|------------------|-------------------------------------------------------------------------------------------------------|
| update_curr      | Update the current task's runtime statistics : 针对其中运行时间的统计结果                             |
| update_cfs_group | calc_group_shares 和 calc_group_runnable 然后更新 task_group 中间的 se 的数值                         |
| update_load_avg  | `__update_load_avg_se` 以及 `update_cfs_rq_load_avg` `update_tg_load_avg` 就是各种无死角更新 load_avg |

## 备忘
1. update_curr 和 update_cfs_group 之间的区别是什么 ?
2. task group 在所有的 cpu 中间都创建一个，从而保证 hierarchy 在所有地方相同
3. `tg->load_avg := \sum tg->cfs_rq[]->avg.load_avg`. : update_tg_load_avg()
4. tg.share : 当我们 dequeue task、enqueue task 以及 task tick 的时候会通过 update_cfs_group()函数更新 group se 的权重信息。也就是说，tg.share 权重就是静态计算的。
  1. update_cfs_group 更新的是 group se 权重。
5. calc_group_shares()根据当前 group cfs_rq 负载情况计算新的权重。
6. `scale_load_down` `scale_load` 是很简单的内容，只是为了实现高精度而已。
```c
# define scale_load(w)		((w) << SCHED_FIXEDPOINT_SHIFT)
# define scale_load_down(w)	((w) >> SCHED_FIXEDPOINT_SHIFT)

# define scale_load(w)		(w)
# define scale_load_down(w)	(w)
```
7. sched_avg 中的四个成员 : runable_load_avg load_avg load_sum runable_load_sum


## 分析的入口
- kernel/sched/fair.c:6798 的注释

- propagate_entity_load_avg

- load_sum runable_load_sum 就是为了对应的计算 avg 而产生的，可以验证一下
  - 进一步的怀疑，之所以需要 load_avg 的存在，就是之前的各种近似公式的计算导致的
  - 如果不是多核，那么，


- load_avg 和 load 的关系是什么 ? 依靠几何级数吗 ?

- calc_group_runnable 的作用，而且其也是具有对应的一大段注释的

1. 各种 numa 函数 : account_numa_enqueue
2. `se->on_rq` 和 `taks_struct->on_rq` 两者的对比

1. include/linux/sched.h:400 的注释
2. task_group::shares 的计算
3. 关于新创建的 cfs_rq 的位置，可以查看一下 online 和 offline 的函数

```c
#ifdef CONFIG_FAIR_GROUP_SCHED
	/* list of leaf cfs_rq on this CPU: */
	struct list_head	leaf_cfs_rq_list;
	struct list_head	*tmp_alone_branch;
#endif /* CONFIG_FAIR_GROUP_SCHED */
```
> 其中的 rq 的更新的内容是什么 ?

5. attach_entity_cfs_rq 和向 group 中间添加 setsid 之后 fork 出来的，有没有关系 ?

6. static void update_blocked_averages(int cpu) block 在此处的含义是什么 ?

7. `cfs_rq->avg.load` 被 tg.load_avg 收集了，其他的属性怎么说 ?

9. balance 的测量标准是什么 ?

10. reweight_entity 中间依赖的函数 ?

11. 为什么需要分析 runable_load_avg 中间的　runable ?
  1. time_slice 和 vruntime 的各自的作用是什么 ?
  4. time_slice 和 time_period 的概念，导致 runnable 的总是运行其拥有的比例，那么现在还受 vruntime 控制了吗？
      1. vruntime 会选择正确的，如果没有 time_slice 的存在，那么可以在整个 period 中间始终运行该变量
      2. 而 time_slice 的存在只是为了处理 lantency 的存在，但是依旧高优先级持有更多的运行时间。
          1. 但是会不会导致出现所有人都是只能运行 min_grandlurity 的数值，即使是优先级更高的
3. 对于一个新添加的 process 显然其 vruntime 最少的，那岂不是总是选择执行新加入的 ?
    2. 我觉得可以通过 min_vruntime 确定的，新加入的 process 可能会马上开始运行，但是不会无穷无尽的占用。
    3. 所以 min_vruntime 的数值溢出如何处理 ? (难道没有一个定时一并处理的时间点吗 ?)

12. numa 到底意味着什么东西 ?
1. 所以，busiest 的确定的标准是什么 ?

## Linux 调度器：进程优先级 http://www.wowotech.net/process_management/process-priority.html
1. nice : 用户层次的内容。
2. priority : 被映射到统一的地址空间中间
3. weight : prio_to_weight 得到 weight 真正管理的硬通货
4. load_avg sched_avg share : 神仙打架 ?


# scheduler

## TODO
1. 阅读完成之前的所有的文章(整理) => 尽量搞清楚其中的问题
2. 重新阅读一遍发
3. 网课
4. 阅读 fair.c 和 core.c 中间的函数，大致确定函数的返回
5. vruntime min_vruntime 和 period 的分析
6. advance 部分分析一下

## 问题

#### 核心问题
1. 实现进程跨 CPU 之间调度的方法:
2. 不同粒度的实现 getpriority 中的
3. yield 和抢断等似乎比想象的更加复杂啊!
4. priority 到底如何计算得到 load weight 的 ?
5. sleeper 的 vruntime 是什么东西，为什么 vruntime 需要单独分析啊 ?
6. period : check_preempt_tick 中间需要保证最少执行 4ms
7. group 机制 for_each_sched_entity



#### 细节问题
1. `rq->min_vruntime` 的计算 似乎和 `en->load_weight` `sum_exec_runtime` `curr->vruntime`
2. `rq->vruntime` 的作用是什么 ? 如何处理数值溢出?
3. sched_slice 相关的内容
4. 了解一下这一个函数
```c
/*
 * resched_curr - mark rq's current task 'to be rescheduled now'.
 *
 * On UP this means the setting of the need_resched flag, on SMP it
 * might also involve a cross-CPU call to trigger the scheduler on
 * the target CPU.
 */
void resched_curr(struct rq *rq)
```


## 统计一些数据
**Man sched_setattr(2)**
> 对于这些疑惑的最佳总结

4. entity

```c
struct task_struct {
// 如果把 se 当做 fair 的内容，也是 rt dl 和 fair
// 那么就是rq 完全对应起来的
// include/linux/sched.h
	struct sched_entity		se;
	struct sched_rt_entity		rt;
#ifdef CONFIG_CGROUP_SCHED
	struct task_group		*sched_task_group;
#endif
	struct sched_dl_entity		dl;
```

> 1. policy 的含义是什么，没有 STOP 的原因很简单

* SCHED_IDLE 的使用位置，fair.c

> 2. 三种 policy 如何影响到 fair 的工作的不同的

> 3. entity 如果像是 list 的东西，那么为什么这么大，到底放了什么东西，如果仅仅是为了放到 rb tree 显然就不用拆分出来是三个了!


# more analyze on the kernel/sched/core.c

## preempt 实现的原理到底是什么

## cgroup 机制

## hrtimer

## schedule() 实现的原理
```c
// TODO 这个有文档可以阅读呀!
asmlinkage __visible void __sched schedule(void)
{
	struct task_struct *tsk = current;

	sched_submit_work(tsk);
	do {
		preempt_disable();
		__schedule(false);
		sched_preempt_enable_no_resched();
	} while (need_resched());
}
EXPORT_SYMBOL(schedule);
```
1. 为什么关掉 preempt
2. 为什么可以使用 preempt_disable 关掉 ?
3. 为什么需要 sched_submit_work 将 blk 中间的工作进行缓冲一下 ?

## `__schedule` 函数的作用

```c
/*
 * __schedule() is the main scheduler function.
 *
 * The main means of driving the scheduler and thus entering this function are:
 *
 *   1. Explicit blocking: mutex, semaphore, waitqueue, etc.
 *
 *   2. TIF_NEED_RESCHED flag is checked on interrupt and userspace return
 *      paths. For example, see arch/x86/entry_64.S.
 *
 *      To drive preemption between tasks, the scheduler sets the flag in timer
 *      interrupt handler scheduler_tick().
 *
 *   3. Wakeups don't really cause entry into schedule(). They add a
 *      task to the run-queue and that's it.
 *
 *      Now, if the new task added to the run-queue preempts the current
 *      task, then the wakeup sets TIF_NEED_RESCHED and schedule() gets
 *      called on the nearest possible occasion:
 *
 *       - If the kernel is preemptible (CONFIG_PREEMPT=y):
 *
 *         - in syscall or exception context, at the next outmost
 *           preempt_enable(). (this might be as soon as the wake_up()'s
 *           spin_unlock()!)
 *
 *         - in IRQ context, return from interrupt-handler to
 *           preemptible context
 *
 *       - If the kernel is not preemptible (CONFIG_PREEMPT is not set)
 *         then at the next:
 *
 *          - cond_resched() call
 *          - explicit schedule() call
 *          - return from syscall or exception to user-space
 *          - return from interrupt-handler to user-space
 *
 * WARNING: must be called with preemption disabled!
 */
static void __sched notrace __schedule(bool preempt)
```
1. 不能理解为什么会有 preemptible 的配置 ? 参数 preempt 的影响是什么 ?
2. 设计到 PF_WQ_WORKER 的部分细节
3. balance_callback




## sched_tick_

为了处理一下 nohz 导致的问题

## context_switch() 实现的原理

```c
/*
 * context_switch - switch to the new MM and the new thread's register state.
 */
static __always_inline struct rq *
context_switch(struct rq *rq, struct task_struct *prev,
	       struct task_struct *next, struct rq_flags *rf)
```
1. 首先进行 MM 的切换，其中含有涉及到 tlb 之类的切换
> 不是很懂，为什么 mm 相等的情况，没有单独分析
2. switch_to 进行真正的切换
  1. `__switch_to_asm` 利用汇编切换 stack 和 thread's register


## ttwu

wake_up_process

```c
/**
 * wake_up_process - Wake up a specific process
 * @p: The process to be woken up.
 *
 * Attempt to wake up the nominated process and move it to the set of runnable
 * processes.
 *
 * Return: 1 if the process was woken up, 0 if it was already running.
 *
 * This function executes a full memory barrier before accessing the task state.
 */
int wake_up_process(struct task_struct *p)
{
	return try_to_wake_up(p, TASK_NORMAL, 0);
}
EXPORT_SYMBOL(wake_up_process);

int wake_up_state(struct task_struct *p, unsigned int state)
{
	return try_to_wake_up(p, state, 0);
}

int default_wake_function(wait_queue_entry_t *curr, unsigned mode, int wake_flags,
			  void *key)
{
	return try_to_wake_up(curr->private, mode, wake_flags);
}
EXPORT_SYMBOL(default_wake_function);
```

三个简单的封装函数

```c
/*
 * Notes on Program-Order guarantees on SMP systems.
 *
 *  MIGRATION
 *
 * The basic program-order guarantee on SMP systems is that when a task [t]
 * migrates, all its activity on its old CPU [c0] happens-before any subsequent
 * execution on its new CPU [c1].
 *
 * For migration (of runnable tasks) this is provided by the following means:
 *
 *  A) UNLOCK of the rq(c0)->lock scheduling out task t
 *  B) migration for t is required to synchronize *both* rq(c0)->lock and
 *     rq(c1)->lock (if not at the same time, then in that order).
 *  C) LOCK of the rq(c1)->lock scheduling in task
 *
 * Release/acquire chaining guarantees that B happens after A and C after B.
 * Note: the CPU doing B need not be c0 or c1
 *
 * Example:
 *
 *   CPU0            CPU1            CPU2
 *
 *   LOCK rq(0)->lock
 *   sched-out X
 *   sched-in Y
 *   UNLOCK rq(0)->lock
 *
 *                                   LOCK rq(0)->lock // orders against CPU0
 *                                   dequeue X
 *                                   UNLOCK rq(0)->lock
 *
 *                                   LOCK rq(1)->lock
 *                                   enqueue X
 *                                   UNLOCK rq(1)->lock
 *
 *                   LOCK rq(1)->lock // orders against CPU2
 *                   sched-out Z
 *                   sched-in X
 *                   UNLOCK rq(1)->lock
 *
 *
 *  BLOCKING -- aka. SLEEP + WAKEUP
 *
 * For blocking we (obviously) need to provide the same guarantee as for
 * migration. However the means are completely different as there is no lock
 * chain to provide order. Instead we do:
 *
 *   1) smp_store_release(X->on_cpu, 0)
 *   2) smp_cond_load_acquire(!X->on_cpu)
 *
 * Example:
 *
 *   CPU0 (schedule)  CPU1 (try_to_wake_up) CPU2 (schedule)
 *
 *   LOCK rq(0)->lock LOCK X->pi_lock
 *   dequeue X
 *   sched-out X
 *   smp_store_release(X->on_cpu, 0);
 *
 *                    smp_cond_load_acquire(&X->on_cpu, !VAL);
 *                    X->state = WAKING
 *                    set_task_cpu(X,2)
 *
 *                    LOCK rq(2)->lock
 *                    enqueue X
 *                    X->state = RUNNING
 *                    UNLOCK rq(2)->lock
 *
 *                                          LOCK rq(2)->lock // orders against CPU1
 *                                          sched-out Z
 *                                          sched-in X
 *                                          UNLOCK rq(2)->lock
 *
 *                    UNLOCK X->pi_lock
 *   UNLOCK rq(0)->lock
 *
 *
 * However, for wakeups there is a second guarantee we must provide, namely we
 * must ensure that CONDITION=1 done by the caller can not be reordered with
 * accesses to the task state; see try_to_wake_up() and set_current_state().
 */

```
似乎，就上锁而言，wake up 需要保证的内容似乎稍微简单一点

1. set_current_state



```c
/**
 * try_to_wake_up - wake up a thread
 * @p: the thread to be awakened
 * @state: the mask of task states that can be woken
 * @wake_flags: wake modifier flags (WF_*)
 *
 * If (@state & @p->state) @p->state = TASK_RUNNING.
 *
 * If the task was not queued/runnable, also place it back on a runqueue.
 *
 * Atomic against schedule() which would dequeue a task, also see
 * set_current_state().
 *
 * This function executes a full memory barrier before accessing the task
 * state; see set_current_state().
 *
 * Return: %true if @p->state changes (an actual wakeup was done),
 *	   %false otherwise.
 */
static int
try_to_wake_up(struct task_struct *p, unsigned int state, int wake_flags)
{
	unsigned long flags;
	int cpu, success = 0;

	/*
	 * If we are going to wake up a thread waiting for CONDITION we
	 * need to ensure that CONDITION=1 done by the caller can not be
	 * reordered with p->state check below. This pairs with mb() in
	 * set_current_state() the waiting thread does.
	 */
	raw_spin_lock_irqsave(&p->pi_lock, flags);
	smp_mb__after_spinlock();
	if (!(p->state & state))
		goto out;

	trace_sched_waking(p);

	/* We're going to change ->state: */
	success = 1;
	cpu = task_cpu(p);

	/*
	 * Ensure we load p->on_rq _after_ p->state, otherwise it would
	 * be possible to, falsely, observe p->on_rq == 0 and get stuck
	 * in smp_cond_load_acquire() below.
	 *
	 * sched_ttwu_pending()			try_to_wake_up()
	 *   STORE p->on_rq = 1			  LOAD p->state
	 *   UNLOCK rq->lock
	 *
	 * __schedule() (switch to task 'p')
	 *   LOCK rq->lock			  smp_rmb();
	 *   smp_mb__after_spinlock();
	 *   UNLOCK rq->lock
	 *
	 * [task p]
	 *   STORE p->state = UNINTERRUPTIBLE	  LOAD p->on_rq
	 *
	 * Pairs with the LOCK+smp_mb__after_spinlock() on rq->lock in
	 * __schedule().  See the comment for smp_mb__after_spinlock().
	 */
	smp_rmb();
	if (p->on_rq && ttwu_remote(p, wake_flags))
		goto stat;

#ifdef CONFIG_SMP
	/*
	 * Ensure we load p->on_cpu _after_ p->on_rq, otherwise it would be
	 * possible to, falsely, observe p->on_cpu == 0.
	 *
	 * One must be running (->on_cpu == 1) in order to remove oneself
	 * from the runqueue.
	 *
	 * __schedule() (switch to task 'p')	try_to_wake_up()
	 *   STORE p->on_cpu = 1		  LOAD p->on_rq
	 *   UNLOCK rq->lock
	 *
	 * __schedule() (put 'p' to sleep)
	 *   LOCK rq->lock			  smp_rmb();
	 *   smp_mb__after_spinlock();
	 *   STORE p->on_rq = 0			  LOAD p->on_cpu
	 *
	 * Pairs with the LOCK+smp_mb__after_spinlock() on rq->lock in
	 * __schedule().  See the comment for smp_mb__after_spinlock().
	 */
	smp_rmb();

	/*
	 * If the owning (remote) CPU is still in the middle of schedule() with
	 * this task as prev, wait until its done referencing the task.
	 *
	 * Pairs with the smp_store_release() in finish_task().
	 *
	 * This ensures that tasks getting woken will be fully ordered against
	 * their previous state and preserve Program Order.
	 */
	smp_cond_load_acquire(&p->on_cpu, !VAL);

	p->sched_contributes_to_load = !!task_contributes_to_load(p);
	p->state = TASK_WAKING;

	if (p->in_iowait) {
		delayacct_blkio_end(p);
		atomic_dec(&task_rq(p)->nr_iowait);
	}

	cpu = select_task_rq(p, p->wake_cpu, SD_BALANCE_WAKE, wake_flags);
	if (task_cpu(p) != cpu) {
		wake_flags |= WF_MIGRATED;
		set_task_cpu(p, cpu);
	}

#else /* CONFIG_SMP */

	if (p->in_iowait) {
		delayacct_blkio_end(p);
		atomic_dec(&task_rq(p)->nr_iowait);
	}

#endif /* CONFIG_SMP */

	ttwu_queue(p, cpu, wake_flags);
stat:
	ttwu_stat(p, cpu, wake_flags);
out:
	raw_spin_unlock_irqrestore(&p->pi_lock, flags);

	return success;
}
```


## select_task_rq 和 set_task_cpu 看上去很简单呀

两者都是调用了 `sched_class->select_task_rq` 维持生活的

set_task_cpu 会调用 set_task_rq 进行一些设置的工作。

rq 和 cpu 在此处的含义有不同 ?
```c
/*
 * The caller (fork, wakeup) owns p->pi_lock, ->cpus_allowed is stable.
 */
static inline
int select_task_rq(struct task_struct *p, int cpu, int sd_flags, int wake_flags)
{
	lockdep_assert_held(&p->pi_lock);

	if (p->nr_cpus_allowed > 1)
		cpu = p->sched_class->select_task_rq(p, cpu, sd_flags, wake_flags);
	else
		cpu = cpumask_any(&p->cpus_allowed);

	/*
	 * In order not to call set_task_cpu() on a blocking task we need
	 * to rely on ttwu() to place the task on a valid ->cpus_allowed
	 * CPU.
	 *
	 * Since this is common to all placement strategies, this lives here.
	 *
	 * [ this allows ->select_task() to simply return task_cpu(p) and
	 *   not worry about this generic constraint ]
	 */
	if (unlikely(!is_cpu_allowed(p, cpu)))
		cpu = select_fallback_rq(task_cpu(p), p); // 稍微有点复杂

	return cpu;
}
```

> 核心业务交给了 sched_class 了

## smp_wmb() 是什么东西


## upadte_rq_clock

函数简单，但是 为什么是这种更新机制 ?

# kernel/sched/core.c

- [ ] 没有找到层级式的计算每一个 CPU shared 的地方

## 关键函数
- scheduler_tick : scheduler 面对始终的 hook
- resched_curr : 当前 thread 决定释放 CPU

## 基本的分析一下选择过程

```txt
#0  select_task_rq_fair (p=0xffff88814a11ae80, prev_cpu=0, wake_flags=8) at kernel/sched/fair.c:7015
#1  0xffffffff8113cff4 in select_task_rq (wake_flags=8, cpu=0, p=0xffff88814a11ae80) at kernel/sched/core.c:3489
#2  try_to_wake_up (p=0xffff88814a11ae80, state=state@entry=3, wake_flags=wake_flags@entry=0) at kernel/sched/core.c:4183
#3  0xffffffff8113d3cc in wake_up_process (p=<optimized out>) at kernel/sched/core.c:4314
#4  0xffffffff8119b859 in hrtimer_wakeup (timer=<optimized out>) at kernel/time/hrtimer.c:1939
#5  0xffffffff8119bde2 in __run_hrtimer (flags=2, now=0xffffc90000003f48, timer=0xffffc900005bb910, base=0xffff888333c1e0c0, cpu_base=0xffff888333c1e080) at kernel/time/hrtimer.c:1685
#6  __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff888333c1e080, now=48626544325410, flags=flags@entry=2, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#7  0xffffffff8119ca71 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
```

- [ ] 在这里， select_task_rq 中，根据当时参数的就直接找到

```txt
#0  resched_curr (rq=0xffff888333c2b2c0) at kernel/sched/core.c:1027
#1  0xffffffff8114599b in check_preempt_tick (curr=0xffff888145898a00, cfs_rq=0xffff888333c2b3c0) at kernel/sched/sched.h:1169
#2  entity_tick (queued=0, curr=0xffff888145898a00, cfs_rq=0xffff888333c2b3c0) at kernel/sched/fair.c:4761
#3  task_tick_fair (rq=0xffff888333c2b2c0, curr=0xffff888100a22e80, queued=0) at kernel/sched/fair.c:11416
#4  0xffffffff8113f392 in scheduler_tick () at kernel/sched/core.c:5453
#5  0xffffffff8119b2b1 in update_process_times (user_tick=0) at kernel/time/timer.c:1844
#6  0xffffffff811ad85f in tick_sched_handle (ts=ts@entry=0xffff888333c1e5c0, regs=regs@entry=0xffffc9000086be88) at kernel/time/tick-sched.c:243
#7  0xffffffff811ada3c in tick_sched_timer (timer=0xffff888333c1e5c0) at kernel/time/tick-sched.c:1480
#8  0xffffffff8119bde2 in __run_hrtimer (flags=130, now=0xffffc90000003f48, timer=0xffff888333c1e5c0, base=0xffff888333c1e0c0, cpu_base=0xffff888333c1e080) at kernel/time/hrtimer.c:1685
#9  __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff888333c1e080, now=48647509299866, flags=flags@entry=130, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#10 0xffffffff8119ca71 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#11 0xffffffff810e25d7 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1095
#12 __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1112
#13 0xffffffff81f4137d in sysvec_apic_timer_interrupt (regs=0xffffc9000086be88) at arch/x86/kernel/apic/apic.c:1106
```
- [ ] scheduler_tick : 还是根据 curr 进行确定的

## TODO
cgroup 在 7000 line 注册的函数无人使用呀!

rq 内嵌的 cfs_rq 的作用到底是什么 ? 为什么 pick_next_task_fair 总是从其中 pick 但是依旧可以到 root_task_group 上，
`__sched_init` 中间说明了其结果!

task_group 的 share 的计算方法是什么 ?


## analyze

## rq 中间的 cfs_rq 的地位
rq 中间的似乎是根基，然后利用这个实现 group 找到其他的之类的 ?

cfs_rq entity 以及 task_group 初始化的时候，都是一套的，猜测此时创建的
entity 就是一个 group 的代表

所以通过这种方法就用 rq owned by entity/group 的效果。

现在的问题 : 其他的 entitiy 如何添加上来的 ?

> attach_entity_cfs_rq

> set_task_rq
> task_set_group_fair

> 好像也不是


> 1. malloc 出来的如何关联上去 ?
> 2. 和 rq 中间的关系是什么 ?

task_group  CONFIG_FAIR_GROUP_SCHED 以及 CONFIG_CFS_BANDWIDTH 三者逐渐递进的


## group 之间如何均衡

```c
int alloc_fair_sched_group(struct task_group *tg, struct task_group *parent)

	tg->shares = NICE_0_LOAD;
```

1. tg->shares 相关的计算
    1. 赋值永远都是 NICE_0_LOAD

```c

/*
 * Increase resolution of nice-level calculations for 64-bit architectures.
 * The extra resolution improves shares distribution and load balancing of
 * low-weight task groups (eg. nice +19 on an autogroup), deeper taskgroup
 * hierarchies, especially on larger systems. This is not a user-visible change
 * and does not change the user-interface for setting shares/weights.
 *
 * We increase resolution only if we have enough bits to allow this increased
 * resolution (i.e. 64-bit). The costs for increasing resolution when 32-bit
 * are pretty high and the returns do not justify the increased costs.
 *
 * Really only required when CONFIG_FAIR_GROUP_SCHED=y is also set, but to
 * increase coverage and consistency always enable it on 64-bit platforms.
 */
#ifdef CONFIG_64BIT
# define NICE_0_LOAD_SHIFT	(SCHED_FIXEDPOINT_SHIFT + SCHED_FIXEDPOINT_SHIFT)
# define scale_load(w)		((w) << SCHED_FIXEDPOINT_SHIFT)
# define scale_load_down(w)	((w) >> SCHED_FIXEDPOINT_SHIFT)
#else
# define NICE_0_LOAD_SHIFT	(SCHED_FIXEDPOINT_SHIFT)
# define scale_load(w)		(w)
# define scale_load_down(w)	(w)
#endif

/*
 * Task weight (visible to users) and its load (invisible to users) have
 * independent resolution, but they should be well calibrated. We use
 * scale_load() and scale_load_down(w) to convert between them. The
 * following must be true:
 *
 *  scale_load(sched_prio_to_weight[USER_PRIO(NICE_TO_PRIO(0))]) == NICE_0_LOAD
 *
 */
#define NICE_0_LOAD		(1L << NICE_0_LOAD_SHIFT)


/*
 * Integer metrics need fixed point arithmetic, e.g., sched/fair
 * has a few: load, load_avg, util_avg, freq, and capacity.
 *
 * We define a basic fixed point arithmetic range, and then formalize
 * all these metrics based on that basic range.
 */
# define SCHED_FIXEDPOINT_SHIFT		10
# define SCHED_FIXEDPOINT_SCALE		(1L << SCHED_FIXEDPOINT_SHIFT)
```
> @todo 还有 freq capacity load_avg 等

A priority number of 120, which is the priority of a normal task, is mapped to a load of 1024, which is the value that the kernel uses to represent the capacity of a single standard CPU.
> @todo 为什么会映射到 1024 上，利用 prio_to_weight 吗 ?

A run queue (`struct cfs_rq`) is also characterized by a "weight" value that is the accumulation of weights of all tasks on its run queue.

```c
struct sched_entity {
	/* For load-balancing: */
	struct load_weight		load;
	unsigned long			runnable_weight; // 难道 bandwidth 使用的 ?
	struct rb_node			run_node;
	struct list_head		group_node; // task group ?
	unsigned int			on_rq; // why not boolean ?

  // @todo how runtime works ?
	u64				exec_start;
	u64				sum_exec_runtime;
	u64				vruntime;
	u64				prev_sum_exec_runtime;

	u64				nr_migrations;
```
> 1. load 和 runnable_weight 之间的关系是什么 ?

The time slice can now be calculated as:
    time_slice = (sched_period() * se.load.weight) / cfs_rq.load.weight;
where `sched_period()` returns the scheduling period as a factor of the number of running tasks on the CPU.
We see that the higher the load, the higher the fraction of the scheduling period that the task gets to run on the CPU.
> 下面的两个函数似乎说明了 : time_slice 的效果，但是由于 group 的存在，其计算过程变成了递归的过程。
> 所以 time_slice 相当于一个 taks 允许运行的时间吗 ?

```c
/*
 * We calculate the wall-time slice from the period by taking a part
 * proportional to the weight.
 *
 * s = p*P[w/rw]
 */
static u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	u64 slice = __sched_period(cfs_rq->nr_running + !se->on_rq);

	for_each_sched_entity(se) {
		struct load_weight *load;
		struct load_weight lw;

		cfs_rq = cfs_rq_of(se);
		load = &cfs_rq->load;

		if (unlikely(!se->on_rq)) {
			lw = cfs_rq->load;

			update_load_add(&lw, se->load.weight);
			load = &lw;
		}
		slice = __calc_delta(slice, se->load.weight, load);
	}
	return slice;
}

/*
 * The idea is to set a period in which each task runs once.
 *
 * When there are too many tasks (sched_nr_latency) we have to stretch
 * this period because otherwise the slices get too small.
 *
 * p = (nr <= nl) ? l : l*nr/nl
 */
static u64 __sched_period(unsigned long nr_running)
{
	if (unlikely(nr_running > sched_nr_latency))
		return nr_running * sysctl_sched_min_granularity;
	else
		return sysctl_sched_latency;
}
```

Each time a new task forks or a task wakes up, its vruntime is assigned to a value that is the maximum of its last updated value and `cfs_rq.min_vruntime`.
If not for this, its vruntime would be very small as an effect of not having run for a long time (or at all)
and would take an unacceptably long time to catch up to the vruntime of its sibling tasks and hence starve them of CPU time.
> 不是很懂，对于 `cfs_rq.min_vruntime` 的更新其实也是不清楚的

Every periodic tick, the vruntime of the currently-running task is updated as follows:
    vruntime += delta_exec * (NICE_0_LOAD/curr->load.weight);
where delta_exec is the time spent by the task since the last time vruntime was updated, NICE_0_LOAD is the load of a task with normal priority, and curr is the currently-running task. We see that vruntime progresses slowly for tasks of higher priority. It has to, because the time slice for these tasks is large and they cannot be preempted until the time slice is exhausted.


> 找到了 vruntime 中间的 delta_exec 位置在于何处 ?

```c
// update_curr 中间的内容:
	curr->vruntime += calc_delta_fair(delta_exec, curr);

/*
 * delta /= w
 */
static inline u64 calc_delta_fair(u64 delta, struct sched_entity *se)
{
	if (unlikely(se->load.weight != NICE_0_LOAD))
		delta = __calc_delta(delta, NICE_0_LOAD, &se->load);

	return delta;
}

/*
 * delta_exec * weight / lw.weight
 *   OR
 * (delta_exec * (weight * lw->inv_weight)) >> WMULT_SHIFT
 *
 * Either weight := NICE_0_LOAD and lw \e sched_prio_to_wmult[], in which case
 * we're guaranteed shift stays positive because inv_weight is guaranteed to
 * fit 32 bits, and NICE_0_LOAD gives another 10 bits; therefore shift >= 22.
 *
 * Or, weight =< lw.weight (because lw.weight is the runqueue weight), thus
 * weight/lw.weight <= 1, and therefore our shift will also be positive.
 */
static u64 __calc_delta(u64 delta_exec, unsigned long weight, struct load_weight *lw)
// 这个函数虽然恶心，但是内容就是根据实际的运行时间片段，然后得到 vruntime 得到的片段，并且和 vruntime 无关。
```

**Per-entity load-tracking metrics**

```c
// sched entity 中间保证的变量。
#ifdef CONFIG_SMP
	/*
	 * Per entity load average tracking.
	 *
	 * Put into separate cache line so it does not
	 * collide with read-mostly values above.
	 */
	struct sched_avg		avg;
#endif
```
> pelt 还是和其含有关系的 ?

https://lwn.net/Articles/531853/

放到 SMP 中间，就是为了处理其中的 pelt ?

pelt.c 中间是什么个情况 ?

> 问一下蜗壳科技 ?

## sched_entity.avg 是什么情况

> 说了这么多，到底和 SMP 有什么蛇皮关系呀 ?

```c
/*
 * sched_entity:
 *
 *   task:
 *     se_runnable() == se_weight()
 *
 *   group: [ see update_cfs_group() ]
 *     se_weight()   = tg->weight * grq->load_avg / tg->load_avg
 *     se_runnable() = se_weight(se) * grq->runnable_load_avg / grq->load_avg
 *
 *   load_sum := runnable_sum
 *   load_avg = se_weight(se) * runnable_avg
 *
 *   runnable_load_sum := runnable_sum
 *   runnable_load_avg = se_runnable(se) * runnable_avg
 *
 * XXX collapse load_sum and runnable_load_sum
 *
 * cfq_rq:
 *
 *   load_sum = \Sum se_weight(se) * se->avg.load_sum
 *   load_avg = \Sum se->avg.load_avg
 *
 *   runnable_load_sum = \Sum se_runnable(se) * se->avg.runnable_load_sum
 *   runnable_load_avg = \Sum se->avg.runable_load_avg
 */

int __update_load_avg_blocked_se(u64 now, int cpu, struct sched_entity *se)
{
	if (entity_is_task(se))
		se->runnable_weight = se->load.weight;

	if (___update_load_sum(now, cpu, &se->avg, 0, 0, 0)) {
		___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
		return 1;
	}

	return 0;
}

int __update_load_avg_se(u64 now, int cpu, struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (entity_is_task(se))
		se->runnable_weight = se->load.weight;

	if (___update_load_sum(now, cpu, &se->avg, !!se->on_rq, !!se->on_rq,
				cfs_rq->curr == se)) {

		___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
		cfs_se_util_change(&se->avg);
		return 1;
	}

	return 0;
}
```
> 1. se_runnable 和 se_weight 的关系是什么 ?
> 2. update_cfs_group 的作用是什么 ?


> update_cfs_group 中间的内容和注释中间描述的一致吗 ?

```c
/*
 * Recomputes the group entity based on the current state of its group
 * runqueue.
 */
static void update_cfs_group(struct sched_entity *se)
{
	struct cfs_rq *gcfs_rq = group_cfs_rq(se);
	long shares, runnable;

  // 似乎仅仅分析 group 的 owner 的
	if (!gcfs_rq)
		return;

  // 如果 bandwidth 没有通过 ?
	if (throttled_hierarchy(gcfs_rq))
		return;

    // 似乎分别计算出来 group 的 weight 和 runnable 的内容
    // TODO 所以啥几把是 grq 呀!
  /**
   *   group: [ see update_cfs_group() ]
   *     se_weight()   = tg->weight * grq->load_avg / tg->load_avg
   *     se_runnable() = se_weight(se) * grq->runnable_load_avg / grq->load_avg
   */
	shares   = calc_group_shares(gcfs_rq);
	runnable = calc_group_runnable(gcfs_rq, shares);

	reweight_entity(cfs_rq_of(se), se, shares, runnable);
}

// 调用位置 : enqueue_entity enqueue_task_fair entity_tick 以及 sched_group_set_shares
// 其中 : int sched_group_set_shares(struct task_group *tg, unsigned long shares) 设置位置在 core.c 中间
// update_curr 算是 group 发生变化然后更新的方法了
```

0. 但是我怀疑，其中只有在 涉及到什么的时候
1. grq 到底是什么 ? group runqueue ? sched:776 并不是 rq 中的，就是 task_group 中间的
2. 不如直接搜索 pelt 的内容 ?
3. 如果知道其中的

## 分析一下 : `static long calc_group_shares(struct cfs_rq *cfs_rq)` 上的注释

```c
/*
 * All this does is approximate the hierarchical proportion which includes that
 * global sum we all love to hate.
 *
 * That is, the weight of a group entity, is the proportional share of the
 * group weight based on the group runqueue weights. That is:
 *
 *                     tg->weight * grq->load.weight
 *   ge->load.weight = -----------------------------               (1)
 *			  \Sum grq->load.weight
 *
 * Now, because computing that sum is prohibitively expensive to compute (been
 * there, done that) we approximate it with this average stuff. The average
 * moves slower and therefore the approximation is cheaper and more stable.
 *
 * So instead of the above, we substitute:
 *
 *   grq->load.weight -> grq->avg.load_avg                         (2)
 *
 * which yields the following:
 *
 *                     tg->weight * grq->avg.load_avg
 *   ge->load.weight = ------------------------------              (3)
 *				tg->load_avg
 *
 * // 也就是说 : tg->load_avg 其实就是 share ?
 * Where: tg->load_avg ~= \Sum grq->avg.load_avg
 *
 * That is shares_avg, and it is right (given the approximation (2)).
 *
 * The problem with it is that because the average is slow -- it was designed
 * to be exactly that of course -- this leads to transients in boundary
 * conditions. In specific, the case where the group was idle and we start the
 * one task. It takes time for our CPU's grq->avg.load_avg to build up,
 * yielding bad latency etc..
 *
 * Now, in that special case (1) reduces to:
 *
 *                     tg->weight * grq->load.weight
 *   ge->load.weight = ----------------------------- = tg->weight   (4)
 *			    grp->load.weight
 *
 * That is, the sum collapses because all other CPUs are idle; the UP scenario.
 *
 * So what we do is modify our approximation (3) to approach (4) in the (near)
 * UP case, like:
 *
 *   ge->load.weight =
 *
 *              tg->weight * grq->load.weight
 *     ---------------------------------------------------         (5)
 *     tg->load_avg - grq->avg.load_avg + grq->load.weight
 *
 * But because grq->load.weight can drop to 0, resulting in a divide by zero,
 * we need to use grq->avg.load_avg as its lower bound, which then gives:
 *
 *
 *                     tg->weight * grq->load.weight
 *   ge->load.weight = -----------------------------		   (6)
 *				tg_load_avg'
 *
 * Where:
 *
 *   tg_load_avg' = tg->load_avg - grq->avg.load_avg +
 *                  max(grq->load.weight, grq->avg.load_avg)
 *
 * And that is shares_weight and is icky. In the (near) UP case it approaches
 * (4) while in the normal case it approaches (3). It consistently
 * overestimates the ge->load.weight and therefore:
 *
 *   \Sum ge->load.weight >= tg->weight
 *
 * hence icky!
 */
```
1. task_group 和 cfs_rq 都有 weight 吗 ? 并不是，所以 weight 在什么地方 ?
2. 观察一下　calc_group_shares 中间的内容吧!



```c
/* Task group related information */
struct task_group {
	unsigned long		shares;

#ifdef	CONFIG_SMP
	/*
	 * load_avg can be heavily contended at clock tick time, so put
	 * it in its own cacheline separated from the fields above which
	 * will also be accessed at each tick.
	 */
	atomic_long_t		load_avg ____cacheline_aligned;
#endif



/* CFS-related fields in a runqueue */
struct cfs_rq {
	struct load_weight	load;
	unsigned long		runnable_weight;
	unsigned int		nr_running;
	unsigned int		h_nr_running;


#ifdef CONFIG_SMP
	/*
	 * CFS load tracking
	 */
	struct sched_avg	avg;



struct sched_entity {
	/* For load-balancing: */
	struct load_weight		load;
	unsigned long			runnable_weight;
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
```
> 所以，所有的疑惑都在 tg 中间 : share 和 load_avg　　



```c
/**
 * update_tg_load_avg - update the tg's load avg
 * @cfs_rq: the cfs_rq whose avg changed
 * @force: update regardless of how small the difference
 *
 * this function 'ensures': tg->load_avg := \sum tg->cfs_rq[]->avg.load.
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
{
	long delta = cfs_rq->avg.load_avg - cfs_rq->tg_load_avg_contrib;
  // 其实，所以这些 tg_load_avg_contrib 的使用真的是过于简单呀!

	/*
	 * No need to update load_avg for root_task_group as it is not used.
	 */
	if (cfs_rq->tg == &root_task_group)
		return;

	if (force || abs(delta) > cfs_rq->tg_load_avg_contrib / 64) {
		atomic_long_add(delta, &cfs_rq->tg->load_avg);
		cfs_rq->tg_load_avg_contrib = cfs_rq->avg.load_avg;
	}
}
```

> 忽然，意识到，其实，tg 其实所有的 cpu 的 rq 的总和
> this function 'ensures': `tg->load_avg := \sum tg->cfs_rq[]->avg.load`.

This metric calculates task load as the amount of time that the task was runnable during the time that it was alive.
This is kept track of in the sched_avg data structure (stored in the sched_entity structure):

## reweight_entity 内容分析
1. runnable_avg 和 avg 的关系是什么 ?
2. reweight_entity 其实就是直接对于 se 的 runnable_weight 和 load.weight 进行赋值。

```c
static void reweight_entity(struct cfs_rq *cfs_rq, struct sched_entity *se,
			    unsigned long weight, unsigned long runnable)
{
	if (se->on_rq) {
		/* commit outstanding execution time */
		if (cfs_rq->curr == se)
			update_curr(cfs_rq);
		account_entity_dequeue(cfs_rq, se);
		dequeue_runnable_load_avg(cfs_rq, se);
	}
	dequeue_load_avg(cfs_rq, se);

	se->runnable_weight = runnable;
	update_load_set(&se->load, weight);

#ifdef CONFIG_SMP
	do {
		u32 divider = LOAD_AVG_MAX - 1024 + se->avg.period_contrib;

		se->avg.load_avg = div_u64(se_weight(se) * se->avg.load_sum, divider);
		se->avg.runnable_load_avg =
			div_u64(se_runnable(se) * se->avg.runnable_load_sum, divider);
	} while (0);
#endif

	enqueue_load_avg(cfs_rq, se);
	if (se->on_rq) {
		account_entity_enqueue(cfs_rq, se);
		enqueue_runnable_load_avg(cfs_rq, se);
	}
}
```



```c
static inline void
enqueue_runnable_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	cfs_rq->runnable_weight += se->runnable_weight;

	cfs_rq->avg.runnable_load_avg += se->avg.runnable_load_avg;
	cfs_rq->avg.runnable_load_sum += se_runnable(se) * se->avg.runnable_load_sum;
}

static inline void
dequeue_runnable_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	cfs_rq->runnable_weight -= se->runnable_weight;

	sub_positive(&cfs_rq->avg.runnable_load_avg, se->avg.runnable_load_avg);
	sub_positive(&cfs_rq->avg.runnable_load_sum,
		     se_runnable(se) * se->avg.runnable_load_sum);
}

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

## Nohz 的影响是什么

## attach 和 detach 三个函数

```c
static void attach_entity_cfs_rq(struct sched_entity *se)

/*
 * attach_tasks() -- attaches all tasks detached by detach_tasks() to their
 * new rq.
 */
static void attach_tasks(struct lb_env *env)

/**
 * attach_entity_load_avg - attach this entity to its cfs_rq load avg
 * @cfs_rq: cfs_rq to attach to
 * @se: sched_entity to attach
 * @flags: migration hints
 *
 * Must call update_cfs_rq_load_avg() before this, since we rely on
 * cfs_rq->avg.last_update_time being current.
 */
static void attach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
```


1. `static void attach_tasks(struct lb_env *env)`

利用 lb_env (load_balance environment ?) 将 task 一个一个的从 链表中间迁移。

```c
void activate_task(struct rq *rq, struct task_struct *p, int flags)
{
	if (task_contributes_to_load(p)) // 这两行看不懂啊!
		rq->nr_uninterruptible--;

	enqueue_task(rq, p, flags);
}
```

detach_tasks 和 attach_tasks 对称，但是其中存在部分检查 `env->loop` 和 `can_migrate_task`

2. `attach_entity_cfs_rq` 和 `attach_task_cfs_rq`

```c
static void attach_entity_cfs_rq(struct sched_entity *se)
{
	struct cfs_rq *cfs_rq = cfs_rq_of(se);

#ifdef CONFIG_FAIR_GROUP_SCHED
	/*
	 * Since the real-depth could have been changed (only FAIR
	 * class maintain depth value), reset depth properly.
	 */
	se->depth = se->parent ? se->parent->depth + 1 : 0;
#endif

	/* Synchronize entity with its cfs_rq */
	update_load_avg(cfs_rq, se, sched_feat(ATTACH_AGE_LOAD) ? 0 : SKIP_AGE_LOAD);
	attach_entity_load_avg(cfs_rq, se, 0);
	update_tg_load_avg(cfs_rq, false);
	propagate_entity_cfs_rq(se);
}

static void attach_task_cfs_rq(struct task_struct *p)
{
	struct sched_entity *se = &p->se;
	struct cfs_rq *cfs_rq = cfs_rq_of(se);

	attach_entity_cfs_rq(se);

	if (!vruntime_normalized(p))
		se->vruntime += cfs_rq->min_vruntime;
}
```
3. `attach_entity_load_avg` 因为切换 rq 之类需要更新 load_avg

attach_task_cfs_rq 最后 switch_to_fair 以及 task_move_group_fair (task_change_group_fair)调用。
都是 sched_class 中间的标准的函数。

其实 attach_tasks 和 其他的各种蛇皮是两个体系

## affine
都是 select_task_rq_fair 的辅助函数，task 开始执行需要选择最佳的 rq

wake_affine
    - wake_affine_idle
    - wake_affine_weight

wake_wide :

# sched overview
> fair rt deadline 核心实现

1. 一共含有什么调度器
2. 调度器中间含有什么函数 : 循环的时候
3. 调度器的架构:
    1. cpu : rq 管理学 : 对于四个 rq 进行循环使用 next 找到
    2. wake_up 机制中间的内容是什么 ?
5. 优先级数值的计算的方法是什么 ?
6. 一直想要 express 的内容 : 线程其实和之间的是一个连续的变化过程，但是用户为什么看到的是进程的概念
https://stackoverflow.com/questions/9305992/if-threads-share-the-same-pid-how-can-they-be-identified
7. 实现迁移 task 的方法
8. 当 wakeup 一个线程的时候，如何确定其被加载哪一个 rq 中间的 ?
9. 总结一下状态的变迁内容。
10. 一个进程如何确定自己的优先级，是不是 rt 的 ?

调度器的原则: 选择下一个 ready 的进程执行以及决定执行的 tick 数目 ?

1. 几个很怪异的地方: policy 常量 和 (sched_class sched_entity rq) 的数量不是对应的啊!
2. pid 和 tgid 的各自使用的地方



| Filename            | blank | comment | code | explanation                                                                                                                                                                                            |
|---------------------|-------|---------|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| fair.c              | 1659  | 2975    | 5572 |                                                                                                                                                                                                        |
| core.c              | 1033  | 1986    | 4036 |                                                                                                                       |
| rt.c                | 515   | 504     | 1712 |                                                                                                                                                                                                        |
| deadline.c          | 406   | 858     | 1493 |                                                                                                                                                                                                        |
| sched.h             | 365   | 445     | 1436 |                                                                                                                                                                                                        |
| topology.c          | 315   | 446     | 1164 | 处理负载在核之间的平衡，需要考虑 SMT，多核，NUMA 和大小核                                                                                                                                                                                              |
| debug.c             | 166   | 36      | 804  | 看来，只要含有不确定性的东西(sched lock 之类的)，那么就必定含有 debug 的内容                                                                                                                             |
| cputime.c           | 142   | 229     | 524  | 利用 kernel/timer 测量 cpu 执行的时间                                                                                                                                                                    |
| cpufreq_schedutil.c | 156   | 206     | 523  |                                                                                                                                                                                                        |
| idle.c              | 78    | 134     | 270  | idle 进程的实现 ? 空转 ? 低功耗模式 ?                                                                                                                                                                  |
| cpuacct.c           | 65    | 41      | 269  | 统计 cpu 时间                                                                                                                                                                                            |
| wait.c              | 59    | 138     | 245  | wait queue 实现，被大量的驱动使用，应该不是 workqueue 的实现基础                                                                                                                                        |
| clock.c             | 71    | 176     | 234  | @todo                                                                                                                                                                                                  |
| autogroup.c         | 51    | 40      | 179  | 没有配置                                                                                                                                                                                               |
| wait_bit.c          | 41    | 31      | 177  | 新的等待机制?                                                                                                                                                                                          |
| membarrier.c        | 29    | 114     | 175  | membarrier syscall 支持                                                                                                                                                                                |
| cpudeadline.c       | 42    | 67      | 167  | 似乎和 deadline.c 有关，但是 ...                                                                                                                                                                       |
| pelt.c              | 50    | 194     | 155  | Per Entity Load Tracking                                                                                                                                                                               |
| loadavg.c           | 48    | 202     | 150  | hhhh loadavg 找到 average 时间                                                                                                                                                                         |
| completion.c        | 28    | 157     | 144  | completion 机制                                                                                                                                                                                        |
| isolation.c         | 26    | 10      | 117  | 不知道为什么取名字叫做 isolation 听到这个东西就烦! Housekeeping management. Manage the targets for routine code that can run on any CPU: unbound workqueues, timers, kthreads and any offloadable work. |
| stats.h             | 20    | 41      | 106  |                                                                                                                                                                                                        |
| stop_task.c         | 30    | 21      | 94   | Simple, special scheduling class for the per-CPU stop tasks 最简单的调度器了，是认识架构的好入口，@todo 所以，谁会使用这一个蛇皮。                                                                     |
| swait.c             | 21    | 18      | 93   | simple wait queue 又一个 wait 机制 ?                                                                                                                                                                   |
| cpupri.c            | 31    | 118     | 92   | numa 系统 ?，cpu                                                                                                                                                                                       |
| stats.c             | 19    | 19      | 90   | /proc/sys/kernel/sched_schedstats 相关的内容                                                                                                                                                           |
| pelt.h              | 14    | 9       | 49   | Per Entity Load Tracking 各种调度器的 loadavg                                                                                                                                                           |
| autogroup.h         | 13    | 6       | 41   |                                                                                                                                                                                                        |
| features.h          | 14    | 54      | 24   |                                                                                                                                                                                                        |
| cpudeadline.h       | 4     | 1       | 21   |                                                                                                                                                                                                        |
| Makefile            | 4     | 8       | 19   |                                                                                                                                                                                                        |
| cpufreq.c           | 5     | 38      | 19   | Scheduler code and data structures related to cpufreq  @todo 并不是非常清楚 cpufreq f                                                                                                                   |
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

```c
// 存在 rq ，cfs_rq ，sched_entity
struct task_struct {

    /* Scheduler bits, serialized by scheduler locks: */
    unsigned            sched_reset_on_fork:1;
    unsigned            sched_contributes_to_load:1;
    unsigned            sched_migrated:1;
    unsigned            sched_remote_wakeup:1;
    /* Force alignment to the next boundary: */
    unsigned            :0;


    int             on_rq; // 和迁移有关的

    int             prio;
    int             static_prio;
    int             normal_prio;
    unsigned int            rt_priority;

    // TODO 通过这一个sched_class 进行赋值
    const struct sched_class    *sched_class;

    // TODO 为什么只有这三个，或者说，为什么会多出来下面两个。
    struct sched_entity     se;
    struct sched_rt_entity      rt;
    struct sched_dl_entity      dl;
#ifdef CONFIG_CGROUP_SCHED
    struct task_group       *sched_task_group;
#endif

#ifdef CONFIG_PREEMPT_NOTIFIERS
    /* List of struct preempt_notifier: */
    struct hlist_head       preempt_notifiers;
#endif

#ifdef CONFIG_BLK_DEV_IO_TRACE
    unsigned int            btrace_seq;
#endif

    // 同时可以设置 policy 为 : SCHED_NORMAL / SCHED_FIFO / SCHED_RR / SCHED_BATCH
    unsigned int            policy;
    int             nr_cpus_allowed;
    cpumask_t           cpus_allowed;

#ifdef CONFIG_PREEMPT_RCU
    int             rcu_read_lock_nesting;
    union rcu_special       rcu_read_unlock_special;
    struct list_head        rcu_node_entry;
    struct rcu_node         *rcu_blocked_node;
#endif /* #ifdef CONFIG_PREEMPT_RCU */

#ifdef CONFIG_TASKS_RCU
    unsigned long           rcu_tasks_nvcsw;
    u8              rcu_tasks_holdout;
    u8              rcu_tasks_idx;
    int             rcu_tasks_idle_cpu;
    struct list_head        rcu_tasks_holdout_list;
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

1. stop_sched_class 优先级最高的任务会使用这种策略，会中断所有其他线程，且不会被其他任务打断； // TODO 有这个

1. dl_sched_class 就对应上面的 deadline 调度策略；
1. rt_sched_class 就对应 RR 算法或者 FIFO 算法的调度策略，具体调度策略由进程的`task_struct->policy`指定；

1. fair_sched_class 就是普通进程的调度策略；
1. idle_sched_class 就是空闲进程的调度策略。

对于调度策略，其中 SCHED_FIFO、SCHED_RR、SCHED_DEADLINE 是实时进程的调度策略。

SCHED_NORMAL、SCHED_BATCH、SCHED_IDLE。

> 调度策略和调度常量不一致

两个关键函数 :
> scheduele 进入到 ready 的状态，被 semaphore 捕获，成为不是 ready 的状态。

## [ ] task_struct::policy 到底是做啥的

- 通过系统调用 sched_setattr 来设置

测试 SCHED_IDLE 的效果:

# The Linux Scheduler: a Decade of Wasted Cores
- http://www.ece.ubc.ca/~sasha/papers/eurosys16-final29.pdf

Our recent experience with the Linux scheduler revealed
that the pressure to work around the challenging properties
of modern hardware, such as non-uniform memory access
latencies (NUMA), high costs of cache coherency and synchronization, and diverging CPU and memory latencies, resulted in a scheduler with an incredibly complex implementation. As a result, the very basic function of the scheduler,
which is to make sure that runnable threads use idle cores,
fell through the cracks.

These bugs have different root causes, but a common
symptom. The scheduler unintentionally and for a long
time leaves cores idle while there are runnable threads
waiting in runqueues.


The rest of the paper is organized as follows.
- Section 2 describes the architecture of the Linux scheduler.
- Section 3 introduces the bugs we discovered, analyzes their root causes
and reports their effect on performance.
- Section 4 presents the tools.
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
> 随着进程数量的增加，timeslice 是不是越来越小。会不会进而导致 timeslice 小到连 context switch 的时间都没，如果没有设置限制的话!
> 不会吧 ! 至少也会运行一个时钟周期。

Context switches are on a critical path, so they
must be fast. Accessing only a core-local queue prevents the
scheduler from making potentially expensive synchronized
accesses, which would be required if it accessed a globally
shared runqueue.

> 由于 context switch 导致需要在 local queue 中间获取 !

However, in order for the scheduling algorithm to still
work correctly and efficiently in the presence of per-core
runqueues, the runqueues must be kept balanced.
> 周期性的 balance 算法，但是问题是: 确定 balance 的标准是什么， balance 的过程是什么 ?

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
core. Unfortunately this would result in threads being migrated across the machine without considering cache locality or NUMA. Instead, the load balancer uses a hierarchical
strategy.
> @todo load : how to calculate it ?

The load balancing algorithm is summarized in Algorithm 1. Load balancing is run for each scheduling domain,
starting from the bottom to the top.
> 自底向上计算 load 来实现 balance

Algorithm 1:
> scheduling group && sched domain ?

## IRQ_TIME_ACCOUNTING

```c
config IRQ_TIME_ACCOUNTING
	bool "Fine granularity task level IRQ time accounting"
	depends on HAVE_IRQ_TIME_ACCOUNTING && !VIRT_CPU_ACCOUNTING_NATIVE
	help
	  Select this option to enable fine granularity task irq time
	  accounting. This is done by reading a timestamp on each
	  transitions between softirq and hardirq state, so there can be a
	  small performance impact.

	  If in doubt, say N here.
```
