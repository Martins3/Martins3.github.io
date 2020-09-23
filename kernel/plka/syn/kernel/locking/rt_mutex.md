# kernel/locking/rt_mutex.md

## question

从 Documentation 看:
  1. 似乎是当 A 持有资源，当优先级更高的存在try_lock，那么 A 的优先级提高
  2. 等待队列是按照优先级排序的


需要处理 wait/wound ?

## futex 相关


```c
/**
 * rt_mutex_init_proxy_locked - initialize and lock a rt_mutex on behalf of a
 *				proxy owner
 *
 * @lock:	the rt_mutex to be locked
 * @proxy_owner:the task to set as owner
 *
 * No locking. Caller has to do serializing itself
 *
 * Special API call for PI-futex support. This initializes the rtmutex and
 * assigns it to @proxy_owner. Concurrent operations on the rtmutex are not
 * possible at this point because the pi_state which contains the rtmutex
 * is not yet visible to other tasks.
 */
void rt_mutex_init_proxy_locked(struct rt_mutex *lock,
				struct task_struct *proxy_owner)
{
	__rt_mutex_init(lock, NULL, NULL);
	debug_rt_mutex_proxy_lock(lock, proxy_owner);
	rt_mutex_set_owner(lock, proxy_owner);
}
```
1. 被 attach_to_pi_owner 唯一调用
2. 


## API
1. 这些接口都是按照类似的方法封装的，依旧需要处理:
    1. lockdep
    2. interruptible

```c
static inline void __rt_mutex_lock(struct rt_mutex *lock, unsigned int subclass)
{
	might_sleep();

	mutex_acquire(&lock->dep_map, subclass, 0, _RET_IP_); // lockdep
	rt_mutex_fastlock(lock, TASK_UNINTERRUPTIBLE, rt_mutex_slowlock);
}

/**
 * rt_mutex_lock_interruptible - lock a rt_mutex interruptible
 *
 * @lock:		the rt_mutex to be locked
 *
 * Returns:
 *  0		on success
 * -EINTR	when interrupted by a signal
 */
int __sched rt_mutex_lock_interruptible(struct rt_mutex *lock)
{
	int ret;

	might_sleep();

	mutex_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	ret = rt_mutex_fastlock(lock, TASK_INTERRUPTIBLE, rt_mutex_slowlock);
	if (ret)
		mutex_release(&lock->dep_map, _RET_IP_);

	return ret;
}
EXPORT_SYMBOL_GPL(rt_mutex_lock_interruptible);


/*
 * debug aware fast / slowpath lock,trylock,unlock
 *
 * The atomic acquire/release ops are compiled away, when either the
 * architecture does not support cmpxchg or when debugging is enabled.
 */
static inline int
rt_mutex_fastlock(struct rt_mutex *lock, int state,
		  int (*slowfn)(struct rt_mutex *lock, int state,
				struct hrtimer_sleeper *timeout,
				enum rtmutex_chainwalk chwalk))
{
	if (likely(rt_mutex_cmpxchg_acquire(lock, NULL, current))) // 如果开启 debug ，那么总是直接失败的
		return 0;

	return slowfn(lock, state, NULL, RT_MUTEX_MIN_CHAINWALK);
}



static int __sched rt_mutex_slowlock(struct rt_mutex *lock, int state, struct hrtimer_sleeper *timeout, enum rtmutex_chainwalk chwalk)

/**
 * __rt_mutex_slowlock() - Perform the wait-wake-try-to-take loop
 * @lock:		 the rt_mutex to take
 * @state:		 the state the task should block in (TASK_INTERRUPTIBLE
 *			 or TASK_UNINTERRUPTIBLE)
 * @timeout:		 the pre-initialized and started timer, or NULL for none
 * @waiter:		 the pre-initialized rt_mutex_waiter
 *
 * Must be called with lock->wait_lock held and interrupts disabled
 */
static int __sched
__rt_mutex_slowlock(struct rt_mutex *lock, int state,
		    struct hrtimer_sleeper *timeout,
		    struct rt_mutex_waiter *waiter)
```

## queue

```c
rt_mutex_enqueue
  rt_mutex_adjust_prio_chain
  task_blocks_on_rt_mutex
```
1. 其中的调用关系比较清晰吧，@todo 可以分析一下

## pi
rt_mutex_adjust_pi

> @todo 
