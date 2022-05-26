# RCU

- [ ] 将 QEMU 中对于 RCU 的使用移动到这里
- [ ] https://liburcu.org/ : 提供了三个很好的资源
- https://mp.weixin.qq.com/s/SZqmxMGMyruYUH5n_kobYQ


What is Rcu, Really[^1]:

RCU ensures that reads are coherent by maintaining multiple versions of objects and ensuring that they are not freed up until all pre-existing read-side critical sections complete.

[LoyenWang](https://www.cnblogs.com/LoyenWang/p/12681494.html)

- Reader
    - 使用 rcu_read_lock 和 rcu_read_unlock 来界定读者的临界区，访问受 RCU 保护的数据时，需要始终在该临界区域内访问；
    - 在访问受保护的数据之前，需要使用 rcu_dereference 来获取 RCU-protected 指针；
    - *当使用不可抢占的 RCU 时，rcu_read_lock/rcu_read_unlock 之间不能使用可以睡眠的代码；*
- Updater
    - 多个 Updater 更新数据时，需要使用互斥机制进行保护；
    - Updater 使用`rcu_assign_pointer`来移除旧的指针指向，指向更新后的临界资源；
    - Updater 使用 synchronize_rcu 或 call_rcu 来启动 Reclaimer，对旧的临界资源进行回收，其中 synchronize_rcu 表示同步等待回收，call_rcu 表示异步回收；
- Reclaimer
    - Reclaimer 回收的是旧的临界资源；
    - 为了确保没有读者正在访问要回收的临界资源，Reclaimer 需要等待所有的读者退出临界区，这个等待的时间叫做宽限期（Grace Period）；
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
