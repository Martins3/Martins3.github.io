# lock

## seqlock
dcache.c:d_lookup 的锁


## lockless
让人想起了 slab 的内容:
https://lwn.net/SubscriberLink/827180/a1c1305686bfea67/

## RCU
What is Rcu, Really[^1]:

RCU ensures that reads are coherent by maintaining multiple versions of objects and ensuring that they are not freed up until all pre-existing read-side critical sections complete. 

[LoyenWang](https://www.cnblogs.com/LoyenWang/p/12681494.html)

- Reader
    - 使用rcu_read_lock和rcu_read_unlock来界定读者的临界区，访问受RCU保护的数据时，需要始终在该临界区域内访问；
    - 在访问受保护的数据之前，需要使用rcu_dereference来获取RCU-protected指针；
    - *当使用不可抢占的RCU时，rcu_read_lock/rcu_read_unlock之间不能使用可以睡眠的代码；*
- Updater
    - 多个Updater更新数据时，需要使用互斥机制进行保护；
    - Updater使用`rcu_assign_pointer`来移除旧的指针指向，指向更新后的临界资源；
    - Updater使用synchronize_rcu或call_rcu来启动Reclaimer，对旧的临界资源进行回收，其中synchronize_rcu表示同步等待回收，call_rcu表示异步回收；
- Reclaimer
    - Reclaimer回收的是旧的临界资源；
    - 为了确保没有读者正在访问要回收的临界资源，Reclaimer需要等待所有的读者退出临界区，这个等待的时间叫做宽限期（Grace Period）；
```c
// if debug config is closed
static __always_inline void rcu_read_lock(void)
{
  __rcu_read_lock(); // preempt_disable();
  // NO !!!!!!!!!!!!! this is impossible
}

#define rcu_assign_pointer(p, v)					      \
do {									      \
	uintptr_t _r_a_p__v = (uintptr_t)(v);				      \
									      \
	if (__builtin_constant_p(v) && (_r_a_p__v) == (uintptr_t)NULL)	      \
		WRITE_ONCE((p), (typeof(p))(_r_a_p__v));		      \
	else								      \
		smp_store_release(&p, RCU_INITIALIZER((typeof(p))_r_a_p__v)); \
} while (0)

void synchronize_rcu(void)
{
	RCU_LOCKDEP_WARN(lock_is_held(&rcu_bh_lock_map) ||
			 lock_is_held(&rcu_lock_map) ||
			 lock_is_held(&rcu_sched_lock_map),
			 "Illegal synchronize_rcu() in RCU read-side critical section");
	if (rcu_blocking_is_gp())
		return;
	if (rcu_gp_is_expedited())
		synchronize_rcu_expedited();
	else
		wait_rcu_gp(call_rcu);
}
```




## SRCU
e.g., kvm_mmu_notifier_invalidate_range_start

sleepable rcu

## futex
基本介绍 [^2]

这个解释了，既然可以使用 userspace 的 spinlock，为什么还是要使用内核:
https://linuxplumbersconf.org/event/4/contributions/286/attachments/225/398/LPC-2019-OptSpin-Locks.pdf


用户态的lock:
```c
void lock() {
  while (__sync_lock_test_and_set(&exclusion, 1)) {
    // Do nothing. This GCC builtin instruction
    // ensures memory barrier.
  }
}

void unlock() {
  __sync_synchronize(); // Memory barrier.
  exclusion = 0;
}
```

## set_tid_address
从 man 和 [^3] 看，似乎是配合 pthread 用于 pthread_join，而且会进一步依赖于 futex 来操作




## TODO
考试:
1. spinlock 和 spinlock_bh
2. ksoftirqd 的优先级
3. memcg 如何操作 slab (本来认为 slab 作为内核的部分，不会被 memcg 控制)
4. ticket spinlock


[^1]: https://lwn.net/Articles/262464/
[^2]: https://eli.thegreenplace.net/2018/basics-of-futexes/
[^3]: https://stackoverflow.com/questions/6975098/when-is-the-system-call-set-tid-address-used
