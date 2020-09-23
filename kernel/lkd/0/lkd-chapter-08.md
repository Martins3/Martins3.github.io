# Linux Kernel Development : Bottom Halves and Deferring Works

## quesiton && todo
1. 从 top 切换到 bottom 到过程是什么 ? 这个过程其实就是将 context 封装，然后挂载一个队列上，当然这个队列可能存在很多内容。
    1. 将 top 上下文保存到 bottom 中间

3. 如何从队列上取下来，然后开始执行 ?

2. tasklet 真的还存在吗 ?

## 1 Bottom Halves

Although it is not always clear how to divide the work
between the top and bottom half, a couple of useful tips help:
- If the work is time sensitive, perform it in the interrupt handler.
- If the work is related to the hardware, perform it in the interrupt handler.
- If the work needs to ensure that another interrupt (particularly the same interrupt) does not interrupt it, perform it in the interrupt handler.
- For everything else, consider performing the work in the bottom half

#### 1.1 Why Bottom Halves

#### 1.2 A World of Bottom Halves

Softirqs are a set of **statically defined** bottom halves that
can run simultaneously on any processor; even two of the same type can run concurrently.Tasklets,
which have an awful and confusing name, are flexible, dynamically created bottom halves **built on top of softirqs**.
Two different tasklets can run concurrently on different processors, but two of the same type of tasklet cannot run simultaneously.

Work queues are a simple
yet useful method of queuing work to later be performed in process context.

## 2 Softirqs

Unlike tasklets, you cannot dynamically
register and destroy softirqs. Softirqs are represented by the softirq_action structure,
which is defined in `<linux/interrupt.h>`:
```c
static struct softirq_action softirq_vec[NR_SOFTIRQS] __cacheline_aligned_in_smp;

struct softirq_action
{
	void	(*action)(struct softirq_action *);
};

enum
{
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	BLOCK_SOFTIRQ,
	IRQ_POLL_SOFTIRQ,
	TASKLET_SOFTIRQ,
	SCHED_SOFTIRQ,
	HRTIMER_SOFTIRQ, /* Unused, but kept as tools rely on the
			    numbering. Sigh! */
	RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

	NR_SOFTIRQS
};
```
It seems a bit odd that the kernel passes the entire structure to the softirq handler.This
trick enables future additions to the structure without requiring a change in every softirq
handler.

A softirq never preempts another softirq.The only event that can preempt a softirq is
an interrupt handler.Another softirq—even the same one—can run on another processor,
however.

This is called raising the softirq.
Usually, an interrupt handler marks its softirq for execution before returning.Then, at a
suitable time, the softirq runs. Pending softirqs are checked for and executed in the following places:
- In the return from hardware interrupt code path
- In the `ksoftirqd` kernel thread
- In any code that explicitly checks for and executes pending softirqs, such as the networking subsystem
## 2.1 Implementing softirqs
```c
asmlinkage __visible void do_softirq(void)
{
	__u32 pending;
	unsigned long flags;

	if (in_interrupt())
		return;

	local_irq_save(flags);

	pending = local_softirq_pending();

	if (pending && !ksoftirqd_running(pending))
		do_softirq_own_stack();

	local_irq_restore(flags);
}
```


```c
asmlinkage __visible void __softirq_entry __do_softirq(void)
```

> 书中讲解其中的循环，似乎的确如此，但是
> 1. 利用 pending 数组，如果出现多次 IRQ  那么我怎么知道
> 2. 参数无法传递啊


## 2.2 Using Softirqs

Softirqs are reserved for the most timing-critical and important bottom-half processing
on the system. Currently, only two subsystems—networking and block devices—directly
use softirqs.Additionally, kernel timers and tasklets are built on top of softirqs. If you add a
new softirq, you normally want to ask yourself why using a tasklet is insufficient.Tasklets
are dynamically created and are simpler to use because of their weaker locking requirements, and they still perform quite well. Nonetheless, for timing-critical applications that
can do their own locking in an efficient way, softirqs might be the correct solution.

Softirqs with the lowest numerical priority execute before those with a higher numerical priority.


```c
void open_softirq(int nr, void (*action)(struct softirq_action *))
{
	softirq_vec[nr].action = action;
}
```

*The softirq handlers run with interrupts enabled and cannot sleep.* While a handler
runs, softirqs on the current processor are disabled.
Another processor, however, can execute other softirqs. If the same softirq is raised again while it is executing, another processor can run it simultaneously.This means that any shared data—even global data used only
within the softirq handler—needs proper locking (as discussed in the next two chapters).
This is an important point, and it is the reason tasklets are usually preferred. *Simply preventing your softirqs from running concurrently is not ideal*. If a softirq obtained a lock to
prevent another instance of itself from running simultaneously, there would be no reason
to use a softirq. Consequently, most softirq handlers resort to per-processor data (data
unique to each processor and thus not requiring locking) and other tricks to avoid
explicit locking and provide excellent scalability.
> @todo 为什么说 : softirq handler run with interrupt enabled and cannot sleep 就会导致其最好不要用锁

**Tasklets are essentially softirqs in which multiple instances of
the same handler cannot run concurrently on multiple processors.**

To mark it pending, so it is run at the next invocation of `do_softirq()`, call `raise_softirq()`. 

```c

void raise_softirq(unsigned int nr)
{
	unsigned long flags;

	local_irq_save(flags);
	raise_softirq_irqoff(nr);
	local_irq_restore(flags);
}
```
> @todo 为什么这个函数只有三个调用位置，而不是对于 enum 中间的每一个存在调用

Softirqs are most often raised from within interrupt handlers. In the case of interrupt
handlers, the interrupt handler performs the basic hardware-related work, raises the
softirq, and then exits.When processing interrupts, the kernel invokes `do_softirq()`. The
softirq then runs and picks up where the interrupt handler left off. In this example, the
“top half” and “bottom half” naming should make sense.
> 这个说法很好 :
> 1. 参数如何传递，top 形成的上下文
> 2. 根本找不到证据 : 从 handler 到 raise_softirq 的调用

Softirqs are reserved for the most timing-critical and important bottom-half processing on the system

## 3 Tasklet
> skip

#### 3.1 Implementing Tasklets

#### 3.2 Using Tasklets

#### 3.3 ksoftirqd

#### 3.4 The Old BH Mechanism

## 4 Work Queue
Work queues are a different form of deferring work from what we have looked at so far.
Work queues defer work into a *kernel thread*—this bottom half always runs in *process
context*. Thus, code deferred to a work queue has all the usual benefits of process context.
**Most important, work queues are schedulable and can therefore sleep.**

If you need a schedulable entity to perform your bottom-half processing, you need
work queues.They are the only bottom-half mechanisms that run in process context, and
thus, the only ones that can sleep.

These kernel threads are called worker threads. 
Work queues let your driver create a special worker thread to handle deferred work.

The default worker threads are called events/n where n is the processor number;
there is one per processor.

The default worker thread handles deferred work from multiple locations. Many drivers in the
kernel defer their bottom-half work to the default thread. Unless a driver or subsystem
has a strong requirement for creating its own thread, the default thread is preferred.

Nothing stops code from creating its own worker thread, however.This might be
advantageous if you perform large amounts of processing in the worker thread.
Processorintense and performance-critical work might benefit from its own thread.This also lightens the load on the default threads, which prevents starving the rest of the queued work.

#### 4.1 Implementing Work Queues
> 比较怀疑，这只是曾经只有 800 行的版本

```c
/*
 * The externally visible workqueue.  It relays the issued work items to
 * the appropriate worker_pool through its pool_workqueues.
 */
struct workqueue_struct {
	struct list_head	pwqs;		/* WR: all pwqs of this wq */
	struct list_head	list;		/* PR: list of all workqueues */

	struct mutex		mutex;		/* protects this wq */
	int			work_color;	/* WQ: current work color */
	int			flush_color;	/* WQ: current flush color */
	atomic_t		nr_pwqs_to_flush; /* flush in progress */
	struct wq_flusher	*first_flusher;	/* WQ: first flusher */
	struct list_head	flusher_queue;	/* WQ: flush waiters */
	struct list_head	flusher_overflow; /* WQ: flush overflow list */

	struct list_head	maydays;	/* MD: pwqs requesting rescue */
	struct worker		*rescuer;	/* I: rescue worker */

	int			nr_drainers;	/* WQ: drain in progress */
	int			saved_max_active; /* WQ: saved pwq max_active */

	struct workqueue_attrs	*unbound_attrs;	/* PW: only for unbound wqs */
	struct pool_workqueue	*dfl_pwq;	/* PW: only for unbound wqs */

#ifdef CONFIG_SYSFS
	struct wq_device	*wq_dev;	/* I: for sysfs interface */
#endif
#ifdef CONFIG_LOCKDEP
	struct lockdep_map	lockdep_map;
#endif
	char			name[WQ_NAME_LEN]; /* I: workqueue name */

	/*
	 * Destruction of workqueue_struct is sched-RCU protected to allow
	 * walking the workqueues list without grabbing wq_pool_mutex.
	 * This is used to dump all workqueues from sysrq.
	 */
	struct rcu_head		rcu;

	/* hot fields used during command issue, aligned to cacheline */
	unsigned int		flags ____cacheline_aligned; /* WQ: WQ_* flags */
	struct pool_workqueue __percpu *cpu_pwqs; /* I: per-cpu pwqs */
	struct pool_workqueue __rcu *numa_pwq_tbl[]; /* PWR: unbound pwqs indexed by node */
};
```

```c
struct work_struct {
	atomic_long_t data;
	struct list_head entry;
	work_func_t func;
#ifdef CONFIG_LOCKDEP
	struct lockdep_map lockdep_map;
#endif
};
```

These structures are strung into a linked list, one for each type of queue on each
processor. For example, there is one list of deferred work for the generic thread, per
processor. When a worker thread wakes up, it runs any work in its list. As it completes
work, it removes the corresponding `work_struct` entries from the linked list.When the
list is empty, it goes back to sleep.
> struct work_struct 是 worker 的代表

```c
/**
 * worker_thread - the worker thread function
 * @__worker: self
 *
 * The worker thread function.  All workers belong to a worker_pool -
 * either a per-cpu one or dynamic unbound one.  These workers process all
 * work items regardless of their specific target workqueue.  The only
 * exception is work items which belong to workqueues with a rescuer which
 * will be explained in rescuer_thread().
 *
 * Return: 0
 */
static int worker_thread(void *__worker)
{
	struct worker *worker = __worker;
	struct worker_pool *pool = worker->pool;

	/* tell the scheduler that this is a workqueue worker */
	set_pf_worker(true);
woke_up:
	spin_lock_irq(&pool->lock);

	/* am I supposed to die? */
	if (unlikely(worker->flags & WORKER_DIE)) {
		spin_unlock_irq(&pool->lock);
		WARN_ON_ONCE(!list_empty(&worker->entry));
		set_pf_worker(false);

		set_task_comm(worker->task, "kworker/dying");
		ida_simple_remove(&pool->worker_ida, worker->id);
		worker_detach_from_pool(worker);
		kfree(worker);
		return 0;
	}

	worker_leave_idle(worker);
recheck:
	/* no more worker necessary? */
	if (!need_more_worker(pool))
		goto sleep;

	/* do we need to manage? */
	if (unlikely(!may_start_working(pool)) && manage_workers(worker))
		goto recheck;

	/*
	 * ->scheduled list can only be filled while a worker is
	 * preparing to process a work or actually processing it.
	 * Make sure nobody diddled with it while I was sleeping.
	 */
	WARN_ON_ONCE(!list_empty(&worker->scheduled));

	/*
	 * Finish PREP stage.  We're guaranteed to have at least one idle
	 * worker or that someone else has already assumed the manager
	 * role.  This is where @worker starts participating in concurrency
	 * management if applicable and concurrency management is restored
	 * after being rebound.  See rebind_workers() for details.
	 */
	worker_clr_flags(worker, WORKER_PREP | WORKER_REBOUND);

	do {
		struct work_struct *work =
			list_first_entry(&pool->worklist,
					 struct work_struct, entry);

		pool->watchdog_ts = jiffies;

		if (likely(!(*work_data_bits(work) & WORK_STRUCT_LINKED))) {
			/* optimization path, not strictly necessary */
			process_one_work(worker, work);
			if (unlikely(!list_empty(&worker->scheduled)))
				process_scheduled_works(worker);
		} else {
			move_linked_works(work, &worker->scheduled, NULL);
			process_scheduled_works(worker);
		}
	} while (keep_working(pool));

	worker_set_flags(worker, WORKER_PREP);
sleep:
	/*
	 * pool->lock is held and there's no work to process and no need to
	 * manage, sleep.  Workers are woken up only while holding
	 * pool->lock or from local cpu, so setting the current state
	 * before releasing pool->lock is enough to prevent losing any
	 * event.
	 */
	worker_enter_idle(worker);
	__set_current_state(TASK_IDLE);
	spin_unlock_irq(&pool->lock);
	schedule();
	goto woke_up;
}
```

> 面目全非了，skip the whole workqueue

#### 4.2 Using Work Queues

#### 4.3 The Old Task Queue Mechanism

## 5 Which Bottom Halves Should I Use ?

## 6 Locking Between the Bottom Halves

## 7 Disabling Bottom Halves

## 8 Conclusion

