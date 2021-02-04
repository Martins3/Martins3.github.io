# kernel/softirq.c 分析

1. 处理 softirq 以及其上 tasklet 
2. ksoftirqd 
3. Most important, work queues are schedulable and can therefore sleep
    1. softirq 和 tasklet 都是不能 sleep


## todo
1. handler 到达 softirq 的具体过程是什么 ?
2. 


## question 
> 对于其理论一窍不通啊 !
> 如何被封装成为庞杂的的workqueue 的 ?

其中还处理一些hrtimer的东西。

为什么一个irq机制成为处理内核处理所有的耗时操作的interface比如fs的写会操作。


## ref & doc

- [](https://lwn.net/Articles/520076/)

So, for example, when a kernel subsystem calls `tasklet_schedule()`, the TASKLET_SOFTIRQ bit is set on the corresponding CPU and, when softirqs are processed, the tasklet will be run.

## softirq 执行的时机
> Pending softirqs are checked for and executed in the following places:
> - In the return from hardware interrupt code path
> - In the ksoftirqd kernel thread
> - In any code that explicitly checks for and executes pending softirqs, such as the networking subsystem
>
> LKD chapter 8

- [Understanding Linux Network Internals](https://book.douban.com/subject/1475839/)

https://books.google.co.id/books?id=ALapr7CvAKkC&pg=PT225&lpg=PT225&dq=arch_has_do_softirq&source=bl&ots=gWexbRiXMv&sig=ACfU3U3ARM8fRWLAzqzRvPIIOx4l93Jjhw&hl=en&sa=X&ved=2ahUKEwiFn8DftMHnAhXTGs0KHT6mDSEQ6AEwBHoECAkQAQ#v=onepage&q=arch_has_do_softirq&f=false

> 有的架构存在 `__ARCH_HAS_DO_SOFTIRQ`


```
/* Call softirq on interrupt stack. Interrupts are off. */
ENTRY(do_softirq_own_stack)
	pushq	%rbp
	mov	%rsp, %rbp
	ENTER_IRQ_STACK regs=0 old_rsp=%r11
	call	__do_softirq
	LEAVE_IRQ_STACK regs=0
	leaveq
	ret
ENDPROC(do_softirq_own_stack)
```

```c
#ifdef __ARCH_HAS_DO_SOFTIRQ
void do_softirq_own_stack(void); // 看汇编中间的变化，最后还是调用到制定的位置
#else
static inline void do_softirq_own_stack(void)
{
	__do_softirq();
}
#endif
```

top halves 怎么可能调用 do_softirq , 只是向其中 pending 上写入即可 !
我猜测，网卡的中断系统中间: 事先存在约定吧! 

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

## irq enter exit

```c
// 调用者 : sched apic 以及 smp
// TODO 为什么需哟啊确定当前的context是不是irq的

/*
 * Enter an interrupt context.
 */
 // 似乎并不是真正的执行工作，只是进入的时候的一个时间统计而已
void irq_enter(void)
{
	rcu_irq_enter();  // TODO 恐怖的rcu机制
	if (is_idle_task(current) && !in_interrupt()) {
		/*
		 * Prevent raise_softirq from needlessly waking up ksoftirqd
		 * here, as softirq will be serviced on return from interrupt.
		 */
		local_bh_disable();
		tick_irq_enter();
		_local_bh_enable();
	}

	__irq_enter();
}

void irq_exit(void)
  -> static inline void invoke_softirq(void)
    -> asmlinkage __visible void __softirq_entry __do_softirq(void)
    // 真正调用 softirq_action 中间注册的函数的

```


## ksoftirqd

```c
static __init int spawn_ksoftirqd(void)
{
	cpuhp_setup_state_nocalls(CPUHP_SOFTIRQ_DEAD, "softirq:dead", NULL,
				  takeover_tasklets);
	BUG_ON(smpboot_register_percpu_thread(&softirq_threads));
	return 0;
}
early_initcall(spawn_ksoftirqd);
// TODO 神奇的结构体，让无法推测出来softirq的执行的机制是什么!
static struct smp_hotplug_thread softirq_threads = {
	.store			= &ksoftirqd,
	.thread_should_run	= ksoftirqd_should_run,
	.thread_fn		= run_ksoftirqd,
	.thread_comm		= "ksoftirqd/%u",
};
static void run_ksoftirqd(unsigned int cpu)
  -> asmlinkage __visible void __softirq_entry __do_softirq(void)
```


## tasklet

```c
void __init softirq_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		per_cpu(tasklet_vec, cpu).tail =
			&per_cpu(tasklet_vec, cpu).head;
		per_cpu(tasklet_hi_vec, cpu).tail =
			&per_cpu(tasklet_hi_vec, cpu).head;
	}

	open_softirq(TASKLET_SOFTIRQ, tasklet_action);
	open_softirq(HI_SOFTIRQ, tasklet_hi_action);
}

static __latent_entropy void tasklet_action(struct softirq_action *a)
{
	tasklet_action_common(a, this_cpu_ptr(&tasklet_vec), TASKLET_SOFTIRQ);
}

static __latent_entropy void tasklet_hi_action(struct softirq_action *a)
{
	tasklet_action_common(a, this_cpu_ptr(&tasklet_hi_vec), HI_SOFTIRQ);
}

// 当 do_softirq 检查到 tasklet 对应的bit 被设置(被raise_softirq)
// 那么就会调用此函数，其将对于 tasklet 定义的队列中间的函数进行清空
static void tasklet_action_common(struct softirq_action *a,
				  struct tasklet_head *tl_head,
				  unsigned int softirq_nr)
{
	struct tasklet_struct *list;

	local_irq_disable();
	list = tl_head->head;
	tl_head->head = NULL;
	tl_head->tail = &tl_head->head;
	local_irq_enable();

	while (list) {
		struct tasklet_struct *t = list;

		list = list->next;

		if (tasklet_trylock(t)) {
			if (!atomic_read(&t->count)) {
				if (!test_and_clear_bit(TASKLET_STATE_SCHED,
							&t->state))
					BUG();
				t->func(t->data); // XXX 实际上，TOP 根本不需要保存那么多的上下文，stack frame register file 之类的
				tasklet_unlock(t);
				continue;
			}
			tasklet_unlock(t);
		}

		local_irq_disable();
		t->next = NULL;
		*tl_head->tail = t;
		tl_head->tail = &t->next;
		__raise_softirq_irqoff(softirq_nr);
		local_irq_enable();
	}
}


// func被注册的位置
// 被各种外部driver调用，主要是网卡
void tasklet_init(struct tasklet_struct *t,
		  void (*func)(unsigned long), unsigned long data)
{
	t->next = NULL;
	t->state = 0;
	atomic_set(&t->count, 0);
	t->func = func;
	t->data = data;
}
EXPORT_SYMBOL(tasklet_init);

/* The Linux kernel provides three following functions to mark a tasklet as ready to run: */
void tasklet_schedule(struct tasklet_struct *t); // XXX 其功能简单，将 tasklet_struct 放到队列中间
void tasklet_hi_schedule(struct tasklet_struct *t);
void tasklet_hi_schedule_first(struct tasklet_struct *t);
```


## raise_softirq

```c
// https://0xax.gitbooks.io/linux-insides/content/Interrupts/linux-interrupts-9.html
// 这才是科学的调用路线
// 只有三个位置会直接调用 raise_softirq 

void raise_softirq(unsigned int nr)
{
	unsigned long flags;

	local_irq_save(flags);
	raise_softirq_irqoff(nr);
	local_irq_restore(flags);
}


/*
 * This function must run with irqs disabled!
 */
inline void raise_softirq_irqoff(unsigned int nr) 
// 更多的函数是直接调用此处进行的
// 对于实现 raise 操作并没有什么神奇的地方
// 利用三个提供给 local_softirq_pending_ref 的三个 macro 实现的
// todo 如果多次 raise ，虽然可以保证 raise 的时候 irq off (todo 也没有仔细想过为什么需要保持irq off)，但是连续两次从 TOP 下来，如何保证 !
// 只有一种可能，实现存在约定，每次TOP 将事情注册到指定的位置
{
	__raise_softirq_irqoff(nr);

	/*
	 * If we're in an interrupt or softirq, we're done
	 * (this also catches softirq-disabled code). We will
	 * actually run the softirq once we return from
	 * the irq or softirq.
	 *
	 * Otherwise we wake up ksoftirqd to make sure we
	 * schedule the softirq soon.
	 */
	if (!in_interrupt()) // TODO 判断的原则是什么，为什么要进行判断 !
		wakeup_softirqd();
}

/*
 * we cannot loop indefinitely here to avoid userspace starvation,
 * but we also don't want to introduce a worst case 1/HZ latency
 * to the pending events, so lets the scheduler to balance
 * the softirq load for us.
 */
static void wakeup_softirqd(void)
{
  // 似乎可以部分关键可以理解了, 依靠的是 task_struct 的注册和wakeup 实现的
  // 只需要知道 ksoftirqd 的如何运行即可 !
  // 实际上，根本找不到 ksoftirqd 这个task_struct 初始化的位置

	/* Interrupts are disabled: no need to stop preemption */
	struct task_struct *tsk = __this_cpu_read(ksoftirqd);

	if (tsk && tsk->state != TASK_RUNNING)
		wake_up_process(tsk);
}

#define local_softirq_pending()	(__this_cpu_read(local_softirq_pending_ref))
#define set_softirq_pending(x)	(__this_cpu_write(local_softirq_pending_ref, (x)))
#define or_softirq_pending(x)	(__this_cpu_or(local_softirq_pending_ref, (x)))
```


