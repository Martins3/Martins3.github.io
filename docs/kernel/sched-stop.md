# sched/stop_task.c
> 绝大多数内容为 core.c 的

```c
// stop_sched_class 调用者
void sched_set_stop_task(int cpu, struct task_struct *stop)

// 外部唯一caller
static void cpu_stop_create(unsigned int cpu)
{
	sched_set_stop_task(cpu, per_cpu(cpu_stopper.thread, cpu));
}

// 其他的所有的调用者，当清空队列的时候循环过程
#define for_each_class(class) \
   for (class = sched_class_highest; class; class = class->next)

// 入口
const struct sched_class stop_sched_class = {
	.next			= &dl_sched_class,

const struct sched_class dl_sched_class = {
	.next			= &rt_sched_class,

// 其实从上往下进行的，依次清空队列吗 ?
extern const struct sched_class stop_sched_class;
extern const struct sched_class dl_sched_class;
extern const struct sched_class rt_sched_class;
extern const struct sched_class fair_sched_class;
extern const struct sched_class idle_sched_class;
```


```c
// for_each_class 的调用位置
// 为什么是 for_each_class 而不是 for_each_rq

/*
 * Pick up the highest-prio task:
 */
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)

// online offline TODO 做什么的 ?
void set_rq_online(struct rq *rq)
{
	if (!rq->online) {
		const struct sched_class *class;

		cpumask_set_cpu(rq->cpu, rq->rd->online);
		rq->online = 1;

		for_each_class(class) {
			if (class->rq_online)
				class->rq_online(rq);
		}
	}
}

void set_rq_offline(struct rq *rq)
{
	if (rq->online) {
		const struct sched_class *class;

		for_each_class(class) {
			if (class->rq_offline)
				class->rq_offline(rq);
		}

		cpumask_clear_cpu(rq->cpu, rq->rd->online);
		rq->online = 0;
	}
}

/*
 * Pick up the highest-prio task:
 */
 // 显然的核心业务
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)


// 检查当前进程是否应该发生抢占
void check_preempt_curr(struct rq *rq, struct task_struct *p, int flags)
{
	const struct sched_class *class;

	if (p->sched_class == rq->curr->sched_class) {
		rq->curr->sched_class->check_preempt_curr(rq, p, flags);
	} else {
    // 当发现过来抢占的级别更高，直接GG
		for_each_class(class) {
			if (class == rq->curr->sched_class)
				break;
			if (class == p->sched_class) {
				resched_curr(rq);
				break;
			}
		}
	}

	/*
	 * A queue event has occurred, and we're going to schedule.  In
	 * this case, we can save a useless back to back clock update.
	 */
	if (task_on_rq_queued(rq->curr) && test_tsk_need_resched(rq->curr))
		rq_clock_skip_update(rq);
}
```

## 跟踪一下 pick_next_task

```c
// 当拔出CPU的使用
/*
 * Migrate all tasks from the rq, sleeping tasks will be migrated by
 * try_to_wake_up()->select_task_rq().
 *
 * Called with rq->lock held even though we'er in stop_machine() and
 * there's no concurrency possible, we hold the required locks anyway
 * because of lock validation efforts.
 */
static void migrate_tasks(struct rq *dead_rq, struct rq_flags *rf)

#ifdef CONFIG_HOTPLUG_CPU
// 进一步在cpu.c中间被唯一调用
int sched_cpu_dying(unsigned int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	struct rq_flags rf;

	/* Handle pending wakeups and then migrate everything off */
	sched_ttwu_pending();
	sched_tick_stop(cpu);

	rq_lock_irqsave(rq, &rf);
	if (rq->rd) {
		BUG_ON(!cpumask_test_cpu(cpu, rq->rd->span));
		set_rq_offline(rq);
	}
	migrate_tasks(rq, &rf);
	BUG_ON(rq->nr_running != 1);
	rq_unlock_irqrestore(rq, &rf);

	calc_load_migrate(rq);
	update_max_interval();
	nohz_balance_exit_idle(rq);
	hrtick_clear(rq);
	return 0;
}
#endif


// with a really detailed comment
// schedule 的含义，当前进程觉得自己要GG了，于是决定选一个继任者
static void __sched notrace __schedule(bool preempt)
```

## 如何设置新的进程的各种

```c
/*
 * fork()/clone()-time setup:
 */
int sched_fork(unsigned long clone_flags, struct task_struct *p)
  // used  by idle too
  static void __sched_fork(unsigned long clone_flags, struct task_struct *p)
    // task_struct 持有了关于sched 的内容真的多
    void init_numa_balancing(unsigned long clone_flags, struct task_struct *p)

// 似乎开始的时候，只会赋值 rt 和 fair 两个队列
// 而且是按照prio 不是 policy 设置的
// stop 和 idle 是没有任何 prio 的吗 ?
	if (dl_prio(p->prio))
		return -EAGAIN;
	else if (rt_prio(p->prio))
		p->sched_class = &rt_sched_class;
	else
		p->sched_class = &fair_sched_class;


/*
 * SCHED_DEADLINE tasks has negative priorities, reflecting
 * the fact that any of them has higher prio than RT and
 * NORMAL/BATCH tasks.
 */

#define MAX_DL_PRIO		0

static inline int dl_prio(int prio)
{
	if (unlikely(prio < MAX_DL_PRIO))
		return 1;
	return 0;
}

static inline int rt_prio(int prio)
{
	if (unlikely(prio < MAX_RT_PRIO))
		return 1;
	return 0;
}

// TODO 还是很奇怪，policy 和 prio 各种判断
static inline bool task_is_realtime(struct task_struct *tsk)
{
	int policy = tsk->policy;

	if (policy == SCHED_FIFO || policy == SCHED_RR)
		return true;
	if (policy == SCHED_DEADLINE)
		return true;
	return false;
}

// dl idle stop 只会在特殊情况赋值上去:
void sched_set_stop_task(int cpu, struct task_struct *stop)
void init_idle(struct task_struct *idle, int cpu)　// 为idle进程专门设置一个task_struct吗?

void rt_mutex_setprio(struct task_struct *p, struct task_struct *pi_task)
static void __setscheduler(struct rq *rq, struct task_struct *p,
			   const struct sched_attr *attr, bool keep_boost)
```

## cpu 如何使用 idle

```c
/**
 * init_idle - set up an idle thread for a given CPU
 * @idle: task in question
 * @cpu: CPU the idle task belongs to
 *
 * NOTE: this function does not set the idle thread's NEED_RESCHED
 * flag, to make booting more robust.
 */
void init_idle(struct task_struct *idle, int cpu)
```
