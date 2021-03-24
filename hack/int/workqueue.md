## workqueue

## TODO
4. 找到 worker 如何通过 workqueue_struct 到达 worker_pool 的 ?
- [ ] wowotech 中间的东西可以看看:
[LoyenWang](https://www.cnblogs.com/LoyenWang/p/13185451.html)
[兰新宇](https://zhuanlan.zhihu.com/p/91106844)
- [ ] color 的含义 : https://lore.kernel.org/patchwork/patch/204822/
  - 用于 flush work 的

## 总结

- [x] 内核默认创建了一些工作队列（用户也可以创建） 比如 system_mq system_highpri_mq 这些都是 unbounded
  - 不是, system_wq 是 bounded 的，但是 system_unbound_wq 是

- workqueue 的几个对象 : work, worker, worker_pool, workqueue, pool_workqueue 的创建
  - work 完全用户动态创建的
  - worker 根据负载决定
  - worker_pool 会默认创建一些
  - workqueue 的创建会进一步创建 pool_workqueue，可能创建符合其属性的 worker_pool
    - 对于 bounded 的，其会关联到 cpu_worker_pools 上
    - 对于 unbounded 的， 如果不存在满足该 workqueue 的属性代码，那么就需要创建新的 worker_pool，参考 alloc_unbound_pwq
```c
/* the per-cpu worker pools */
static DEFINE_PER_CPU_SHARED_ALIGNED(struct worker_pool [NR_STD_WORKER_POOLS], cpu_worker_pools);
```
  

总体来说，queue_work 将 work 放到 workqueue 上，然后转移到 worker_pool 上，最后被 worker 执行

每个 cpu 创建两个 per_cpu 的 worker_pool.
unbounded 和 bounded 的 workqueue 的两个属性

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

worker_thread在开始执行时，设置标志位PF_WQ_WORKER，调度器在进行调度处理时会对task进行判断，针对workerqueue worker有特殊处理；
- 在 `__schedule` 和 `ttwu_active` 的时候，会对于 `pool->nr_running` 进行统计，并且同时辅助 worker 的唤醒，创建等操作

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

get_work_pool 使用方法：
1. 如果 work 关联了 pool_workqueue, 可以找到对应的 pool_workqueue
2. 否则找到其曾经使用过的 worker_pool, 当然，如果有的话

flush_work : 利用 barrier work 的方法，一直阻塞，直到目标工作完成了


## principal

1. 和这个workqueue相关的pool_workqueue被挂入一个链表，链表头就是workqueue_struct中的pwqs成员。

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

- [Concurrency Managed Workqueue之（四）：workqueue如何处理work](http://www.wowotech.net/irq_subsystem/queue_and_handle_work.html)

> 可以细品，总体来说，内容是清晰的


分配pool workqueue的内存并建立workqueue和pool workqueue的关系，这部分的代码主要涉及alloc_and_link_pwqs函数


```c
/* content equality test */
static bool wqattrs_equal(const struct workqueue_attrs *a,
			  const struct workqueue_attrs *b)
{
	if (a->nice != b->nice)
		return false;
	if (!cpumask_equal(a->cpumask, b->cpumask))
		return false;
	return true;
}
```
所谓的 wqattrs 就是优先级和可以运行的 cpu 限制, wqattrs 不仅仅描述 workqueue 的属性，还可以描述 worker_pool 的属性.
当创建 workqueue 的时候，如果不存在对应属性的 worker_pool, 在 get_unbound_pool 会动态创建的

workqueue_init 中间对于每一个 cpu 的两个 worker_pool 和 所有的 unbounded 的 workqueue 创建 worker



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
The above functions use a predefined workqueue (called events),
and they run in the context of the events/x thread, as noted above.
Although this is sufficient in most cases,
*it is a shared resource and large delays in work items handlers can cause delays for other queue users.*
For this reason there are functions for creating additional queues.
> 默认的叫做 events，但是为什么share resource 和 large delays 啊 !


> 总体来说(外部接口)
> 1. work item 的操作(delay cancel init) 
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

set_worker_desc
current_work // * current_work - retrieve %current task's work struct

// 两个在头文件中间定义的关键api
schedule_work(struct work_struct *work);
schedule_delayed_work(struct delayed_work *work, unsigned long delay);
```

## 用户
有的是自己创建 workqueue 的，比如 virtio blk

```c
static struct virtio_driver virtio_blk = {
	.feature_table			= features,
	.feature_table_size		= ARRAY_SIZE(features),
	.feature_table_legacy		= features_legacy,
	.feature_table_size_legacy	= ARRAY_SIZE(features_legacy),
	.driver.name			= KBUILD_MODNAME,
	.driver.owner			= THIS_MODULE,
	.id_table			= id_table,
	.probe				= virtblk_probe,
	.remove				= virtblk_remove,
	.config_changed			= virtblk_config_changed,
#ifdef CONFIG_PM_SLEEP
	.freeze				= virtblk_freeze,
	.restore			= virtblk_restore,
#endif
};
```

有的是使用系统的 workqueue:
snd_timer_interrupt => `queue_work(system_highpri_wq, &timer->task_work);`
