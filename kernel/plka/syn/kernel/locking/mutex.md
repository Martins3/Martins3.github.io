# mutex

1. 静态创建 mutex 的方法
2. 动态创建
3. 基本的操作 API
4. debug 的支持
5. 效率的提升


## Documentation
https://lwn.net/Articles/164802/

> In the Linux kernel, mutexes refer to a particular locking primitive
> that enforces serialization on shared memory systems, and not only to
> the generic term referring to 'mutual exclusion' found in academia
> or similar theoretical text books. Mutexes are sleeping locks which
> behave similarly to binary semaphores, and were introduced in 2006[1]
> as an alternative to these. This new data structure provided a number
> of advantages, including simpler interfaces, and at that time smaller
> code (see Disadvantages).


> When acquiring a mutex, there are three possible paths that can be
> taken, depending on the state of the lock:
>
> (i) fastpath: tries to atomically acquire the lock by cmpxchg()ing the owner with
>  the current task. This only works in the uncontended case (cmpxchg() checks
>  against 0UL, so all 3 state bits above have to be 0). If the lock is
>  contended it goes to the next possible path.
>
> (ii) midpath: aka optimistic spinning, tries to spin for acquisition
>  while the lock owner is running and there are no other tasks ready
>  to run that have *higher priority (need_resched)*. The rationale is
>  that if the lock owner is running, it is likely to release the lock
>  soon. The mutex spinners are queued up using MCS lock so that only
>  one spinner can compete for the mutex.
>
>  The MCS lock (proposed by Mellor-Crummey and Scott) is a simple spinlock
>  with the desirable properties of being fair and with each cpu trying
>  to acquire the lock spinning on a local variable. It avoids expensive
>  cacheline bouncing that common test-and-set spinlock implementations
>  incur. An MCS-like lock is specially *tailored for optimistic spinning
>  for sleeping lock implementation.* **An important feature of the customized
>  MCS lock is that it has the extra property that spinners are able to exit
>  the MCS spinlock queue when they need to reschedule. This further helps
>  avoid situations where MCS spinners that need to reschedule would continue
>  waiting to spin on mutex owner, only to go directly to slowpath upon
>  obtaining the MCS lock.**
>
> (iii) slowpath: last resort, if the lock is still unable to be acquired,
>  the task is added to the wait-queue and sleeps until woken up by the
>  unlock path. Under normal circumstances it blocks as TASK_UNINTERRUPTIBLE.

1. higher priority 和 need_resched() 有关系 ?
2. @todo MCS 的基础上如何实现睡眠 ?
3. owner 为什么可以实现 MCS ?



Statically define the mutex::

   DEFINE_MUTEX(name);

Dynamically initialize the mutex::

   mutex_init(mutex);

Acquire the mutex, uninterruptible::

```c
   void mutex_lock(struct mutex *lock);
   void mutex_lock_nested(struct mutex *lock, unsigned int subclass);
   int  mutex_trylock(struct mutex *lock);
```

Acquire the mutex, interruptible::

```c
   int mutex_lock_interruptible_nested(struct mutex *lock,
				       unsigned int subclass);
   int mutex_lock_interruptible(struct mutex *lock);
```

Acquire the mutex, interruptible, if dec to 0::

```c
   int atomic_dec_and_mutex_lock(atomic_t *cnt, struct mutex *lock);
```

Unlock the mutex::

```c
   void mutex_unlock(struct mutex *lock);
```

Test if the mutex is taken::

```c
   int mutex_is_locked(struct mutex *lock);
```

- Disadvantages :
Unlike its original design and purpose, 'struct mutex' is among the largest
locks in the kernel. E.g: on x86-64 it is 32 bytes, where 'struct semaphore'
is 24 bytes and rw_semaphore is 40 bytes. Larger structure sizes mean more CPU
cache and memory footprint.

- When to use mutexes:
Unless the strict semantics of mutexes are unsuitable and/or the critical
region prevents the lock from being shared, always prefer them to any other
locking primitive.


## owner 实现 CMS
1. CMS 为什么可以用一个 task_struct 指针的取值实现 ?
    1. 因为 mutex 不能在 interrupt context 中间使用，所以 mutex 使用的时候总是存在 owner
    2. 那么，CMS 所需要的队列怎么构成 ?

2. 当一个 task 睡眠，那么他可能被通知吗 ?
    1. 利用 CMS 构成队列，会不会，后面必须依赖于前面 wake 才可以继续


## wait/wound
1. https://lwn.net/Articles/548921/ 也就是处理的标准文档

处理死锁的，文档非常详细。

## core struct
```c
/*
 * Simple, straightforward mutexes with strict semantics:
 *
 * - only one task can hold the mutex at a time
 * - only the owner can unlock the mutex
 * - multiple unlocks are not permitted
 * - recursive locking is not permitted
 * - a mutex object must be initialized via the API
 * - a mutex object must not be initialized via memset or copying
 * - task may not exit with mutex held
 * - memory areas where held locks reside must not be freed
 * - held mutexes must not be reinitialized
 * - mutexes may not be used in hardware or software interrupt
 *   contexts such as tasklets and timers
 *
 * These semantics are fully enforced when DEBUG_MUTEXES is
 * enabled. Furthermore, besides enforcing the above rules, the mutex
 * debugging code also implements a number of additional features
 * that make lock debugging easier and faster:
 *
 * - uses symbolic names of mutexes, whenever they are printed in debug output
 * - point-of-acquire tracking, symbolic lookup of function names
 * - list of all locks held in the system, printout of them
 * - owner tracking
 * - detects self-recursing locks and prints out all relevant info
 * - detects multi-task circular deadlocks and prints out all affected
 *   locks and tasks (and only those tasks)
 */
struct mutex {
	atomic_long_t		owner;
	spinlock_t		wait_lock;
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
	struct optimistic_spin_queue osq; /* Spinner MCS lock */
#endif
	struct list_head	wait_list;
#ifdef CONFIG_DEBUG_MUTEXES
	void			*magic;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};
```
2. mutexes may not be used in hardware or software interrupt contexts such as tasklets and timers : 应该是因为 mutex 没有irqsave

1. 有几条不能理解的规则
  - recursive locking is not permitted  : recursive owner 多次上锁吗 ?
  - task may not exit with mutex held : task exit 的含义是 ? exit sycall 吗 ? 如何保证 ? 利用 owner ?
  - mutexes may not be used in hardware or software interrupt contexts such as tasklets and timers : ???

## API : lock
1. 为什么非要区分 interruptible ?
2. nested 的含义 : 用于debug ，具体不知
3. trylock


```c
/**
 * mutex_trylock - try to acquire the mutex, without waiting
 * @lock: the mutex to be acquired
 *
 * Try to acquire the mutex atomically. Returns 1 if the mutex
 * has been acquired successfully, and 0 on contention.
 *
 * NOTE: this function follows the spin_trylock() convention, so
 * it is negated from the down_trylock() return values! Be careful
 * about this when converting semaphore users to mutexes.
 *
 * This function must not be used in interrupt context. The
 * mutex must be released by the same task that acquired it.
 */
int __sched mutex_trylock(struct mutex *lock)
{
	bool locked;

#ifdef CONFIG_DEBUG_MUTEXES
	DEBUG_LOCKS_WARN_ON(lock->magic != lock);
#endif

	locked = __mutex_trylock(lock);
	if (locked)
		mutex_acquire(&lock->dep_map, 0, 1, _RET_IP_); // 当不存在 CONFIG_LOCKDEP 的时候，这是一个空操作

	return locked;
}
EXPORT_SYMBOL(mutex_trylock);
```
1. mutex_trylock 的复杂度 : 当没有 CONFIG_LOCKDEP，和普通逻辑相同，试图走快速通道，如果不行，不做过多的尝试。


```c
void __sched mutex_lock(struct mutex *lock)
{
	might_sleep();

	if (!__mutex_trylock_fast(lock))
		__mutex_lock_slowpath(lock);
}
EXPORT_SYMBOL(mutex_lock);
```

```c
/**
 * mutex_lock_interruptible() - Acquire the mutex, interruptible by signals.
 * @lock: The mutex to be acquired.
 *
 * Lock the mutex like mutex_lock().  If a signal is delivered while the
 * process is sleeping, this function will return without acquiring the
 * mutex.
 *
 * Context: Process context.
 * Return: 0 if the lock was successfully acquired or %-EINTR if a
 * signal arrived.
 */
int __sched mutex_lock_interruptible(struct mutex *lock)
{
	might_sleep();

	if (__mutex_trylock_fast(lock))
		return 0;

	return __mutex_lock_interruptible_slowpath(lock);
}

static noinline void __sched __mutex_lock_slowpath(struct mutex *lock) { __mutex_lock(lock, TASK_UNINTERRUPTIBLE, 0, NULL, _RET_IP_); }
static noinline int __sched __mutex_lock_interruptible_slowpath(struct mutex *lock) { return __mutex_lock(lock, TASK_INTERRUPTIBLE, 0, NULL, _RET_IP_); }


static int __sched __mutex_lock(struct mutex *lock, long state, unsigned int subclass, struct lockdep_map *nest_lock, unsigned long ip) {
	return __mutex_lock_common(lock, state, subclass, nest_lock, ip, NULL, false);
}


/*
 * Lock a mutex (possibly interruptible), slowpath:
 */
static __always_inline int __sched
__mutex_lock_common(struct mutex *lock, long state, unsigned int subclass,
		    struct lockdep_map *nest_lock, unsigned long ip,
		    struct ww_acquire_ctx *ww_ctx, const bool use_ww_ctx)


    // todo ref 参数 state 的两个函数无法理解
    1. set_current_state
    2. signal_pending_state
```

1. signal 就是普通的 kernel/signal.c 而不是




## API : mutex_is_locked
1. 过于简单 : 让人疑惑于 owner 的使用 ?

```c
bool mutex_is_locked(struct mutex *lock)
{
	return __mutex_owner(lock) != NULL;
}

/*
 * Internal helper function; C doesn't allow us to hide it :/
 *
 * DO NOT USE (outside of mutex code).
 */
static inline struct task_struct *__mutex_owner(struct mutex *lock)
{
	return (struct task_struct *)(atomic_long_read(&lock->owner) & ~MUTEX_FLAGS);
}
```

## API : atomic_dec_and_mutex_lock

```c
/**
 * atomic_dec_and_mutex_lock - return holding mutex if we dec to 0
 * @cnt: the atomic which we are to dec
 * @lock: the mutex to return holding if we dec to 0
 *
 * return true and hold lock if we dec to 0, return false otherwise
 */
int atomic_dec_and_mutex_lock(atomic_t *cnt, struct mutex *lock)
{
	/* dec if we can't possibly hit 0 */
	if (atomic_add_unless(cnt, -1, 1))
		return 0;
	/* we might hit 0, so take the lock */
	mutex_lock(lock);
	if (!atomic_dec_and_test(cnt)) {
		/* when we actually did the dec, we didn't hit 0 */
		mutex_unlock(lock);
		return 0;
	}
	/* we hit 0, and we hold the lock */
	return 1;
}
EXPORT_SYMBOL(atomic_dec_and_mutex_lock);
```

1. 过于直白
