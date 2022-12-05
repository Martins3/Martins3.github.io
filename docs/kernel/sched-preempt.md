## preempt
抢占的含义 :
1. 用户确定决定自己的放弃的时机 ?
2. 时间片执行完成之后，放弃 ?
3. 时间片没有执行完被打断 ?


1. 所以内核中间，为什么有的工作不能被 preempt ，被 preempt 会发生什么情况 ?
2. 如果设置为不支持 preempt 整个工作流程是什么样子的 ?


抢占和多核的关系:
1. 是否允许抢占是多个 core 独立管理的


> 当发现当前进程应该被抢占，
> 不能直接把它踢下来，而是把它标记为应该被抢占。为什么呢？
> 因为进程调度第一定律呀，一定要等待正在运行的进程调用`__schedule`才行啊，所以这里只能先标记一下

ucore 为何可以如此简单: 设置一个标志即可 !

真正抢占的实际: 用户和 kernel 区分。

函数主动调用 schedule 函数，然后放弃。

所有进程的调用最终都会走`__schedule`函数。那这个定律在这一节还是要继续起作用。

1. scheduler_tick 导致的最后还是会进入到　`__schedule` 中间，其实 tick 就是内核态的时钟，所以其实 tick 一旦觉得 GG，tick 结束返回，进程就被切换了。
2. try_to_wake_up() // 加入高优先级的线程的时候，低优先级的被标记。

1. 对内核态的执行中，被抢占的时机一般发生在在 preempt_enable()中。
2. 在内核态也会遇到中断的情况，当中断返回的时候，返回的仍然是内核态。这个时候也是一个执行抢占的时机，现在我们再来上面中断返回的代码中返回内核的那部分代码，调用的是 preempt_schedule_irq

```c
// 那么如果，不调用这一个函数，岂不是内核就不会放弃时钟吗 ?

// 含有219个ref
#define preempt_enable() \
do { \
	barrier(); \
	if (unlikely(preempt_count_dec_and_test())) \
		__preempt_schedule(); \
} while (0)

// TODO 并不知道这一个判断的含义是什么
// unlikely 的含义 : enable 只是可能放弃 schedule 但是未必真的放弃。


// 才注意到，似乎 __preempt_schedule() 才是关键啊 !
// 进入到 /home/shen/linux/arch/x86/entry/thunk_64.S 中间

/*
 * this is the entry point to schedule() from in-kernel preemption
 * off of preempt_enable. Kernel preemptions off return from interrupt
 * occur there and call schedule directly.
 */
asmlinkage __visible void __sched notrace preempt_schedule(void)
{
	/*
	 * If there is a non-zero preempt_count or interrupts are disabled,
	 * we do not want to preempt the current task. Just return..
	 */
	if (likely(!preemptible()))
		return;

	preempt_schedule_common(); // 书上记载的，熟悉的内容 !
}
NOKPROBE_SYMBOL(preempt_schedule);
EXPORT_SYMBOL(preempt_schedule);
```




```c
// 这就很难理解了 : 为什么preempt的变化只是一个切换数值而已。

#define preempt_disable() \
do { \
	preempt_count_inc(); \
	barrier(); \
} while (0)

#define preempt_count_inc() preempt_count_add(1)

void preempt_count_add(int val)
{
#ifdef CONFIG_DEBUG_PREEMPT
	/*
	 * Underflow?
	 */
	if (DEBUG_LOCKS_WARN_ON((preempt_count() < 0)))
		return;
#endif
	__preempt_count_add(val);
#ifdef CONFIG_DEBUG_PREEMPT
	/*
	 * Spinlock count overflowing soon?
	 */
	DEBUG_LOCKS_WARN_ON((preempt_count() & PREEMPT_MASK) >=
				PREEMPT_MASK - 10);
#endif
	preempt_latency_start(val);
}

/*
 * The various preempt_count add/sub methods
 */

static __always_inline void __preempt_count_add(int val)
{
	raw_cpu_add_4(__preempt_count, val);
}

#define raw_cpu_add_4(pcp, val)		percpu_add_op((pcp), val)

DEFINE_PER_CPU(int, __preempt_count) = INIT_PREEMPT_COUNT;

/*
 * Disable preemption until the scheduler is running -- use an unconditional
 * value so that it also works on !PREEMPT_COUNT kernels.
 *
 * Reset by start_kernel()->sched_init()->init_idle()->init_idle_preempt_count().
 */
#define INIT_PREEMPT_COUNT	PREEMPT_OFFSET

#define PREEMPT_OFFSET	(1UL << PREEMPT_SHIFT)

#define PREEMPT_SHIFT	0
```

```c
// TODO 看懂注释 !
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


	if (!preempt && prev->state) {
  // ...
  // TODO 看到 preempt 脑袋痛啊 !

	next = pick_next_task(rq, prev, &rf); //
	clear_tsk_need_resched(prev);
	clear_preempt_need_resched();



/*
 * Pick up the highest-prio task:
 */
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{

	/*
	 * Optimization: we know that if all tasks are in the fair class we can
	 * call that function directly, but only if the @prev task wasn't of a
	 * higher scheduling class, because otherwise those loose the
	 * opportunity to pull in more work from other CPUs.
	 */
	if (likely((prev->sched_class == &idle_sched_class ||
		    prev->sched_class == &fair_sched_class) &&
		   rq->nr_running == rq->cfs.h_nr_running)) {

    // TODO 似乎 rq 和 sched_class 是分开的 ?
    // 不然这个参数根本没有必要使用rq
    // 但是 rq 中间持有 rbtree
		p = fair_sched_class.pick_next_task(rq, prev, rf);
```
