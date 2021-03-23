## workqueue
- [ ] wowotech 中间的东西可以看看:
- [ ] https://zhuanlan.zhihu.com/p/91106844

- [ ] 为什么需要 pool_workqueue 这个结构体
- [ ] work_struct 为什么放到 worker_pool::worklist 上
- [ ] pool_workqueue 会属于一个 pool_workqueue, 但是为什么需要将其放出来啊

- [ ] 内核默认创建了一些工作队列（用户也可以创建） 比如 system_mq system_highpri_mq 这些都是 unbounded 吗 ?

- [ ] 存在的对象 work , woker, worker_pool, workqueue 的创建和销毁

每个 cpu 创建两个 per_cpu 的 worker_pool.
unbounded 和 bounded 的 workqueue 的两个属性

- workqueue 的属性有哪些 ?
  - 

alloc_and_link_pwqs
  - init_pwq : 初始化，将自己分别指向 worker_pool 和 workqueue
  - link_pwq : 将自己挂载到创建 workqueue 上
> 一个 workqueue 可以持有多个 pwq, 比如给每一个 cpu 的 worker_pool 分配对应优先级的
> workqueue 划分为 bounded 和 unbounded 的属性, 划分优先级



> 针对非绑定类型的工作队列，worker_pool创建后会添加到unbound_pool_hash哈希表中；
> 
> 存在 unbounded 的 worker pool 吗?

> 判断workqueue的类型，如果是bound类型，根据CPU来获取pool_workqueue，如果是unbound类型，通过node号来获取pool_workqueue；

- `__queue_work`
  - wq_select_unbound_cpu : 如果是 unbounded, 最好是使用当前 cpu 
  - unbound_pwq_by_node : 否则使用 `___queue_work` 的内容, 这两个函数最终是为了找到 pwq, 实际上为了进一步的获取 worker_pool
  - get_work_pool : 直接获取到 worker_pool 的内容
  - find_worker_executing_work : 如果 work 上一次执行的 worker_pool 和 unbound_pwq_by_node/wq_select_unbound_cpu 得到 pwq 指向的 worker 不是一个东西，那么就要考虑 cache 的问题
  - insert_work : 挂到选出来的 worker_pool::worklist 链表上

给定一个 pwq，其唯一定位出来一个 workqueue 和 worker_pool

> worker_thread在开始执行时，设置标志位PF_WQ_WORKER，调度器在进行调度处理时会对task进行判断，针对workerqueue worker有特殊处理；
> - [ ] 我也知道存在特殊处理，但是，是什么 ?

**WHEN COMMING BACK**
0. 好好看看 worker_thread 的部分
1. 整理一下笔记
2. 奔跑 4 的书看看
## LoyenWang
[LoyenWang](https://www.cnblogs.com/LoyenWang/p/13185451.html)


#### struct work_struct
`struct work_struct`用来描述`work`，初始化一个`work`并添加到工作队列后，将会将其传递到合适的内核线程来进行处理，它是用于调度的最小单位。

```c
struct work_struct {
	atomic_long_t data;     //低比特存放状态位，高比特存放worker_pool的ID或者pool_workqueue的指针
	struct list_head entry; //用于添加到其他队列上
	work_func_t func;       //工作任务的处理函数，在内核线程中回调
#ifdef CONFIG_LOCKDEP
	struct lockdep_map lockdep_map;
#endif
};
```

![](https://img2020.cnblogs.com/blog/1771657/202006/1771657-20200623234322425-1856230121.png)

check code get_work_pool(), graph above is kind of misleading.

#### struct workqueue
- [ ] why we need `struct workqueue`, I think `struct worker_pool` is enough.

#### struct worker

- [x] worker can be create and destroied dynamically.

#### struct worker_pool


#### struct pool_workqueue
`pool_workqueue`充当纽带的作用，用于将`workqueue`和`worker_pool`关联起来；

```c
struct pool_workqueue {
	struct worker_pool	*pool;		/* I: the associated pool */    //指向worker_pool
	struct workqueue_struct *wq;		/* I: the owning workqueue */   //指向所属的workqueue

	int			nr_active;	/* L: nr of active works */     //活跃的work数量
	int			max_active;	/* L: max active works */   //活跃的最大work数量
	struct list_head	delayed_works;	/* L: delayed works */      //延迟执行的work挂入本链表
	struct list_head	pwqs_node;	/* WR: node on wq->pwqs */      //用于添加到workqueue链表中
	struct list_head	mayday_node;	/* MD: node on wq->maydays */   //用于添加到workqueue链表中
    ...
} __aligned(1 << WORK_STRUCT_FLAG_BITS);
```

- [ ] I think `struct pool_workqueue` is child of `struct worker_pool` and `struct workqueue_struct`


# kernel/workqueue.md 实现


## principal
1. workqueue_struct : externally visible workqueue.  It relays the issued work items to the appropriate `worker_pool` through its `pool_workqueues`.
2. 我们知道，在CMWQ中，workqueue和thread pool没有严格的一一对应关系了，因此，系统中的workqueue们共享一组thread pool，因此，workqueue中的成员包括两个类别：global类型和per thread pool类型的，我们把那些per thread pool类型的数据集合起来就形成了pool_workqueue的定义。
    1. workqueue_struct 需要持有对于其关联的 worker_pool 保存更加多的消息，建立了 `pool_workqueue`

3. 和这个workqueue相关的pool_workqueue被挂入一个链表，链表头就是workqueue_struct中的pwqs成员。

## doc && ref

- [Concurrency Managed Workqueue之（二）：CMWQ概述](http://www.wowotech.net/irq_subsystem/cmwq-intro.html)

保持 workqueue 的接口不变，利用 work pool 和 worker thread

用户可以创建workqueue（不创建worker pool）并通过flag来约束挂入该workqueue上work的处理方式。
workqueue会根据其flag将work交付给系统中某个worker pool处理。
例如如果该workqueue是bounded类型并且设定了high priority，那么挂入该workqueue的work将由per cpu的highpri worker-pool来处理。

> 由此看来，workqueue 只是简单的前端接口，像是一个入口，之后放到此处的 work 由此导入到具体的 workqueue 中间
> 但是，workqueue 还会持有一些限制在其中。

- [Concurrency Managed Workqueue之（三）：创建workqueue代码分析](http://www.wowotech.net/irq_subsystem/alloc_workqueue.html)

在kernel中，有两种线程池，一种是线程池是per cpu的，也就是说，系统中有多少个cpu，就会创建多少个线程池，cpu x上的线程池创建的worker线程也只会运行在cpu x上。另外一种是unbound thread pool，该线程池创建的worker线程可以调度到任意的cpu上去。
> cpu x 上创建的，只会在 cpu x 上的 worker pool 上运行，如何实现这一个效果。


和旧的workqueue机制一样，系统维护了一个所有workqueue的list，list head定义如下：
```c
static LIST_HEAD(workqueues);		/* PR: list of all workqueues */
```


挂入workqueue的work终究是需要worker线程来处理，针对worker线程有下面几个考量点（我们称之attribute）：
1. 该worker线程的优先级
2. 该worker线程运行在哪一个CPU上
3. 如果worker线程可以运行在多个CPU上，且这些CPU属于不同的NUMA node，那么是否在所有的NUMA node中都可以获取良好的性能。

对于per-CPU的workqueue，2和3不存在问题，哪个cpu上queue的work就在哪个cpu上执行，由于只能在一个确定的cpu上执行，
因此起NUMA的node也是确定的（一个CPU不可能属于两个NUMA node）。置于优先级，per-CPU的workqueue使用WQ_HIGHPRI来标记。
综上所述，per-CPU的workqueue不需要单独定义一个workqueue attribute，这也是为何在workqueue_struct中只有unbound_attrs这个成员来记录unbound workqueue的属性。

NUMA是内存管理的范畴，本文不会深入描述，我们暂且放开NUMA，先思考这样的一个问题：一个确定属性的unbound workqueue需要几个线程池？
看起来一个就够了，毕竟workqueue的属性已经确定了，一个线程池创建相同属性的worker thread就行了。
但是我们来看一个例子：假设workqueue的work是可以在node 0中的CPU A和B，以及node 1中CPU C和D上处理，如果只有一个thread pool，
那么就会存在worker thread在不同node之间的迁移问题。**为了解决这个问题，实际上unbound workqueue实际上是创建了per node的pool_workqueue（thread pool)**

> 实际上，诡异的

```c
/**
 * struct workqueue_attrs - A struct for workqueue attributes.
 *
 * This can be used to change attributes of an unbound workqueue.
 */
struct workqueue_attrs {
	/**
	 * @nice: nice level
	 */
	int nice;

	/**
	 * @cpumask: allowed CPUs
	 */
	cpumask_var_t cpumask;

	/**
	 * @no_numa: disable NUMA affinity
	 *
	 * Unlike other fields, ``no_numa`` isn't a property of a worker_pool. It
	 * only modifies how :c:func:`apply_workqueue_attrs` select pools and thus
	 * doesn't participate in pool hash calculations or equality comparisons.
	 */
	bool no_numa;
};
```

- [Concurrency Managed Workqueue之（四）：workqueue如何处理work](http://www.wowotech.net/irq_subsystem/queue_and_handle_work.html)

> 可以细品，总体来说，内容是清晰的


分配pool workqueue的内存并建立workqueue和pool workqueue的关系，这部分的代码主要涉及alloc_and_link_pwqs函数

## 问题
1. workqueue 显然是被 scheduler 处理的，其运行在 process context 中间
    1. 那么 irq handler 执行是在 interrupt context 

2. local_irq_disable 的作用是什么
    1. interrupt context 的出现导致 那些可能 sleep 的函数不可以被调用(kmalloc 就会 sleep)
    2. 

3. workqueue 实现利用了 softirq 吗 ?
    1. 不是
    2. 那么从 TOP 如何调用 workqueue 


## todo
1. worker pool 的创建和销毁。
2. worker 的创建和销毁。
3. core-api/workqueue.rst
4. 找到 worker 如何通过 workqueue_struct 到达 worker_pool 的 ?

5. 猜测主要的内容大纲: 
    1. 初始化各种结构体
    2. 构建之间的关系
    3. worker 命运的管理





## alloc_workqueue
```c
/**
 * alloc_workqueue - allocate a workqueue
 * @fmt: printf format for the name of the workqueue
 * @flags: WQ_* flags
 * @max_active: max in-flight work items, 0 for default
 * @args...: args for @fmt
 *
 * Allocate a workqueue with the specified parameters.  For detailed
 * information on WQ_* flags, please refer to
 * Documentation/core-api/workqueue.rst.
 *
 * The __lock_name macro dance is to guarantee that single lock_class_key
 * doesn't end up with different namesm, which isn't allowed by lockdep.
 *
 * RETURNS:
 * Pointer to the allocated workqueue on success, %NULL on failure.
 */
#ifdef CONFIG_LOCKDEP
#define alloc_workqueue(fmt, flags, max_active, args...)		\
({									\
	static struct lock_class_key __key;				\
	const char *__lock_name;					\
									\
	__lock_name = "(wq_completion)"#fmt#args;			\
									\
	__alloc_workqueue_key((fmt), (flags), (max_active),		\
			      &__key, __lock_name, ##args);		\
})
#else
#define alloc_workqueue(fmt, flags, max_active, args...)		\
	__alloc_workqueue_key((fmt), (flags), (max_active),		\
			      NULL, NULL, ##args)
#endif


struct workqueue_struct *__alloc_workqueue_key(const char *fmt,
					       unsigned int flags,
					       int max_active,
					       struct lock_class_key *key,
					       const char *lock_name, ...)
  // ...

	/* init wq */
	wq->flags = flags;
	wq->saved_max_active = max_active;
	mutex_init(&wq->mutex);
	atomic_set(&wq->nr_pwqs_to_flush, 0);
	INIT_LIST_HEAD(&wq->pwqs);
	INIT_LIST_HEAD(&wq->flusher_queue);
	INIT_LIST_HEAD(&wq->flusher_overflow);
	INIT_LIST_HEAD(&wq->maydays);

	lockdep_init_map(&wq->lockdep_map, lock_name, key, 0);
	INIT_LIST_HEAD(&wq->list);

  // ....
```

## apply_workqueue_attrs

```c
/**
 * apply_workqueue_attrs - apply new workqueue_attrs to an unbound workqueue
 * @wq: the target workqueue
 * @attrs: the workqueue_attrs to apply, allocated with alloc_workqueue_attrs()
 *
 * Apply @attrs to an unbound workqueue @wq.  Unless disabled, on NUMA
 * machines, this function maps a separate pwq to each NUMA node with
 * possibles CPUs in @attrs->cpumask so that work items are affine to the
 * NUMA node it was issued on.  Older pwqs are released as in-flight work
 * items finish.  Note that a work item which repeatedly requeues itself
 * back-to-back will stay on its current pwq.
 *
 * Performs GFP_KERNEL allocations.
 *
 * Return: 0 on success and -errno on failure.
 */
int apply_workqueue_attrs(struct workqueue_struct *wq,
			  const struct workqueue_attrs *attrs)
{
	int ret;

	apply_wqattrs_lock();
	ret = apply_workqueue_attrs_locked(wq, attrs);
	apply_wqattrs_unlock();

	return ret;
}
```
> 除非没有限制，那么就是会对于每一个 可能的 node 创建对应的 pwq 


```c
static int apply_workqueue_attrs_locked(struct workqueue_struct *wq,
					const struct workqueue_attrs *attrs)
{
	struct apply_wqattrs_ctx *ctx;

	/* only unbound workqueues can change attributes */
	if (WARN_ON(!(wq->flags & WQ_UNBOUND)))
		return -EINVAL;

	/* creating multiple pwqs breaks ordering guarantee */
	if (!list_empty(&wq->pwqs)) {
		if (WARN_ON(wq->flags & __WQ_ORDERED_EXPLICIT))
			return -EINVAL;

		wq->flags &= ~__WQ_ORDERED;
	}

	ctx = apply_wqattrs_prepare(wq, attrs);
	if (!ctx)
		return -ENOMEM;

	/* the ctx has been prepared successfully, let's commit it */
	apply_wqattrs_commit(ctx);
	apply_wqattrs_cleanup(ctx);

	return 0;
}
```

>  apply_wqattrs_prepare 和　apply_wqattrs_commit 之类的很复杂

1. 为unbound workqueue的pool workqueue寻找对应的线程池？
2. 在创建unbound workqueue的时候，pool workqueue对应的worker thread pool需要在这个哈希表中搜索，如果有相同属性的worker thread pool的话，那么就不需要创建新的线程池
```c
/**
 * get_unbound_pool - get a worker_pool with the specified attributes
 * @attrs: the attributes of the worker_pool to get
 *
 * Obtain a worker_pool which has the same attributes as @attrs, bump the
 * reference count and return it.  If there already is a matching
 * worker_pool, it will be used; otherwise, this function attempts to
 * create a new one.
 *
 * Should be called with wq_pool_mutex held.
 *
 * Return: On success, a worker_pool with the same attributes as @attrs.
 * On failure, %NULL.
 */
static struct worker_pool *get_unbound_pool(const struct workqueue_attrs *attrs)
```


```
static void __queue_work(int cpu, struct workqueue_struct *wq,
			 struct work_struct *work)
       // 真正的业务处理者

/**
 * insert_work - insert a work into a pool
 * @pwq: pwq @work belongs to
 * @work: work to insert
 * @head: insertion point
 * @extra_flags: extra WORK_STRUCT_* flags to set
 *
 * Insert @work which belongs to @pwq after @head.  @extra_flags is or'd to
 * work_struct flags.
 *
 * CONTEXT:
 * spin_lock_irq(pool->lock).
 */
static void insert_work(struct pool_workqueue *pwq, struct work_struct *work,
			struct list_head *head, unsigned int extra_flags)
{
	struct worker_pool *pool = pwq->pool;

	/* we own @work, set data and link */
	set_work_pwq(work, pwq, extra_flags);
	list_add_tail(&work->entry, head);
	get_pwq(pwq);

	/*
	 * Ensure either wq_worker_sleeping() sees the above
	 * list_add_tail() or we see zero nr_running to avoid workers lying
	 * around lazily while there are works to be processed.
	 */
	smp_mb();

  // 有点对称的内容 !
	if (__need_more_worker(pool))
		wake_up_worker(pool);
}
```

```c
/**
 * init_worker_pool - initialize a newly zalloc'd worker_pool
 * @pool: worker_pool to initialize
 *
 * Initialize a newly zalloc'd @pool.  It also allocates @pool->attrs.
 *
 * Return: 0 on success, -errno on failure.  Even on failure, all fields
 * inside @pool proper are initialized and put_unbound_pool() can be called
 * on @pool safely to release it.
 */
 // 一共两个调用者 : get_unbound_pool 和 workqueue_init_early 两个
static int init_worker_pool(struct worker_pool *pool)

// worker_pool 按照 attr 创建的，所以，每一个cpu中间应该含有多个 worker_pool 
// TODO 或者说 worker_pool 其实和cpu 并没有强的绑定关系 !


static struct pool_workqueue *alloc_unbound_pwq(struct workqueue_struct *wq,
					const struct workqueue_attrs *attrs)
  -> static struct worker_pool *get_unbound_pool(const struct workqueue_attrs *attrs)
```

## core struct

```c
// TODO 这个结构体区别 ? 至少知道都是描述work queue的
  struct workqueue_struct *wq;
	struct pool_workqueue *pwq;
	struct worker_pool *pool;

/*
 * The externally visible workqueue.  It relays the issued work items to
 * the appropriate worker_pool through its pool_workqueues.
 */
struct workqueue_struct {

/*
 * The per-pool workqueue.  While queued, the lower WORK_STRUCT_FLAG_BITS
 * of work_struct->data are used for flags and the remaining high bits
 * point to the pwq; thus, pwqs need to be aligned at two's power of the
 * number of flag bits.
 */
struct pool_workqueue {

struct worker_pool {

// 用来描述work的两个结构体
struct delayed_work {
	struct work_struct work;
	struct timer_list timer;

	/* target workqueue and CPU ->timer uses to queue ->work */
	struct workqueue_struct *wq;
	int cpu;
};

struct work_struct {
	atomic_long_t data;
	struct list_head entry;
	work_func_t func;
#ifdef CONFIG_LOCKDEP
	struct lockdep_map lockdep_map;
#endif
};
```



```c
// workqueue 的初始化工作
/**
 * workqueue_init_early - early init for workqueue subsystem
 *
 * This is the first half of two-staged workqueue subsystem initialization
 * and invoked as soon as the bare basics - memory allocation, cpumasks and
 * idr are up.  It sets up all the data structures and system workqueues
 * and allows early boot code to create workqueues and queue/cancel work
 * items.  Actual work item execution starts only after kthreads can be
 * created and scheduled right before early initcalls.
 */
int __init workqueue_init_early(void)
```


## workqueue.rst
In order to ease the asynchronous execution of functions a new
abstraction, the work item, is introduced.

Special purpose threads, called worker threads, execute the functions
off of the queue, one after the other.  If no work is queued, the
worker threads become idle.  These worker threads are managed in so
called worker-pools.
> 特殊线程来分别执行这些函数，类似于syscall 一样吗 ?

For any worker pool implementation, managing the concurrency level
**(how many execution contexts are active)** is an important issue.  cmwq
tries to keep the concurrency at a minimal but sufficient level.
Minimal to save resources and sufficient in that the system is used at
its full capacity.


Each worker-pool bound to an actual CPU implements concurrency
management by hooking into the scheduler.  The worker-pool is notified
whenever an active worker wakes up or sleeps and keeps track of the
number of the currently runnable workers.

For unbound workqueues, the number of backing pools is dynamic.
Unbound workqueue can be assigned custom attributes using
``apply_workqueue_attrs()`` and workqueue will automatically create
backing worker pools matching the attributes.


## 总结一下外部接口吧!
> 直接阅读过于痛苦 !

1. delay 机制如何实现的 ?
2. 如何让调度器执行的我的函数 ? 执行的时候，stack 在哪里 ?

The above functions use a predefined workqueue (called events),
and they run in the context of the events/x thread, as noted above.
Although this is sufficient in most cases,
*it is a shared resource and large delays in work items handlers can cause delays for other queue users.*
For this reason there are functions for creating additional queues.
> 默认的叫做 events，但是为什么share resource 和 large delays 啊 !


> 总体来说(外部接口)
> 1. work item 的操作(delay cancell init) 
> 2. work queue 的维护(flush create )
> 3. schedule 的操作


内部:
1. worker 和 workqeue 之间的 attach  detach bind 以及 worker 的各种操作
2. attr

一些sfsfs 和 watch dog 的内容:

```c
// 各种work 没有仔细分析了
work_on_cpu_safe
work_on_cpu
queue_rcu_work
work_busy
__init_work

bool queue_work_on(int cpu, struct workqueue_struct *wq, struct work_struct *work);
void drain_workqueue(struct workqueue_struct *wq);
/* obtain a pool matching @attr and create a pwq associating the pool and @wq */
static struct pool_workqueue *alloc_unbound_pwq(struct workqueue_struct *wq, const struct workqueue_attrs *attrs)

struct workqueue_struct *__alloc_workqueue_key(const char *fmt,
					       unsigned int flags,
					       int max_active,
					       struct lock_class_key *key,
					       const char *lock_name, ...)

// 各种修改属性模型
int apply_workqueue_attrs(struct workqueue_struct *wq,
			  const struct workqueue_attrs *attrs)

GPL(workqueue_set_max_active);


// 在delay时钟之后将work queue在特定的cpu上
bool queue_delayed_work_on(int cpu, struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay)
bool mod_delayed_work_on(int cpu, struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay)
bool cancel_delayed_work(struct delayed_work *dwork)
delayed_work_timer_fn
cancel_delayed_work
flush_delayed_work

// 取消队列中间的内容
cancel_work_sync

flush_rcu_work
flush_work


// 创建两个自定义的queue的方法
#define create_singlethread_workqueue(name)				\
	alloc_ordered_workqueue("%s", __WQ_LEGACY | WQ_MEM_RECLAIM, name)

#define create_workqueue(name)						\
	alloc_workqueue("%s", __WQ_LEGACY | WQ_MEM_RECLAIM, 1, (name))

flush_workqueue // 各种flush




workqueue_congested

/**
 * execute_in_process_context - reliably execute the routine with user context
 * @fn:		the function to execute
 * @ew:		guaranteed storage for the execute work structure (must
 *		be available when the work executes)
 *
 * Executes the function immediately if process context is available,
 * otherwise schedules the function for delayed execution.
 *
 * Return:	0 - function was executed
 *		1 - function was scheduled for execution
 */
 // 其实非常奇怪，为什么如此功能强大的(user ) 没有人使用!
int execute_in_process_context(work_func_t fn, struct execute_work *ew)


// 内部定义的各种workqueue
system_unbound_wq
system_highpri_wq
system_wq
system_long_wq


set_worker_desc
current_work // * current_work - retrieve %current task's work struct

// 两个在头文件中间定义的关键api
schedule_work(struct work_struct *work);
schedule_delayed_work(struct delayed_work *work, unsigned long delay);
```
