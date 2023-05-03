# lock
- [ ] qspinlock : zhihu 专栏的 lanxinyu 的文章
- [ ] 总结一下内核中带锁的位置
- [ ] 我们应该从 lock 的角度来重新思考内核的各种模块
  - vfs 的那些元素需要保护: inode dcache
  - memory 中的 : lru 的链表
- [ ] 验证一下，如果只有单核，内核中很多的 lock 都是会简化的
- [ ] 内核中 lock 和用户态的差别
  - [ ]  内核中存在 conditionnal variables 吗? 如果没有
    - 显然是可以实现的，但是似乎没有见到过
    - 似乎是被替代为 wait queue 了，如果的确是，为什么？

## qspinlock

### 虚拟化下的 spin lock
首先，大致看看代码吧!
```c
/*
 * Per-CPU queue node structures; we can never have more than 4 nested
 * contexts: task, softirq, hardirq, nmi.
 *
 * Exactly fits one 64-byte cacheline on a 64-bit architecture.
 *
 * PV doubles the storage and uses the second cacheline for PV state.
 */
static DEFINE_PER_CPU_ALIGNED(struct qnode, qnodes[MAX_NODES]);

/*
 * On 64-bit architectures, the mcs_spinlock structure will be 16 bytes in
 * size and four of them will fit nicely in one 64-byte cacheline. For
 * pvqspinlock, however, we need more space for extra data. To accommodate
 * that, we insert two more long words to pad it up to 32 bytes. IOW, only
 * two of them can fit in a cacheline in this case. That is OK as it is rare
 * to have more than 2 levels of slowpath nesting in actual use. We don't
 * want to penalize pvqspinlocks to optimize for a rare case in native
 * qspinlocks.
 */
struct qnode {
    struct mcs_spinlock mcs;
#ifdef CONFIG_PARAVIRT_SPINLOCKS
    long reserved[2];
#endif
};
```

```c
typedef struct qspinlock {
	union {
		atomic_t val;

		/*
		 * By using the whole 2nd least significant byte for the
		 * pending bit, we can allow better optimization of the lock
		 * acquisition for the pending bit holder.
		 */
#ifdef __LITTLE_ENDIAN
		struct {
			u8	locked;
			u8	pending;
		};
		struct {
			u16	locked_pending;
			u16	tail;
		};
#else
		struct {
			u16	tail;
			u16	locked_pending;
		};
		struct {
			u8	reserved[2];
			u8	pending;
			u8	locked;
		};
#endif
	};
} arch_spinlock_t;
```

各种蛇皮封装之后:
```c
static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
}
```

展开为:
```c
static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
 do { preempt_count_add(1); __asm__ __volatile__("": : :"memory"); } while (0);
 do { } while (0);
 do_raw_spin_lock(lock);
}
```

```c
static inline void do_raw_spin_lock(raw_spinlock_t *lock) __acquires(lock)
{
	__acquire(lock);
	arch_spin_lock(&lock->raw_lock);
	mmiowb_spin_lock();
}
```
展开为:
```c
static inline  void do_raw_spin_lock(raw_spinlock_t *lock)
{
 (void)0;
 queued_spin_lock(&lock->raw_lock);
 do { } while (0);
}
```

```c
static __always_inline void queued_spin_lock(struct qspinlock *lock)
{
	int val = 0;

	if (likely(atomic_try_cmpxchg_acquire(&lock->val, &val, _Q_LOCKED_VAL)))
		return;

	queued_spin_lock_slowpath(lock, val);
}
```

- [ ] 为什么要 preempt_disable 啊？
  -

```c
static inline void queued_spin_lock_slowpath(struct qspinlock *lock, u32 val)
{
	pv_queued_spin_lock_slowpath(lock, val);
}
```

### pv qspinlock
注释说的很清楚了:
```c
/*
 * Implement paravirt qspinlocks; the general idea is to halt the vcpus instead
 * of spinning them.
 *
 * This relies on the architecture to provide two paravirt hypercalls:
 *
 *   pv_wait(u8 *ptr, u8 val) -- suspends the vcpu if *ptr == val
 *   pv_kick(cpu)             -- wakes a suspended vcpu
 *
 * Using these we implement __pv_queued_spin_lock_slowpath() and
 * __pv_queued_spin_unlock() to replace native_queued_spin_lock_slowpath() and
 * native_queued_spin_unlock().
 */
```

```c
#ifdef CONFIG_PARAVIRT_SPINLOCKS
extern void native_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val);
extern void __pv_init_lock_hash(void);
extern void __pv_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val);
extern void __raw_callee_save___pv_queued_spin_unlock(struct qspinlock *lock);
extern bool nopvspin;

#define	queued_spin_unlock queued_spin_unlock
/**
 * queued_spin_unlock - release a queued spinlock
 * @lock : Pointer to queued spinlock structure
 *
 * A smp_store_release() on the least-significant byte.
 */
static inline void native_queued_spin_unlock(struct qspinlock *lock)
{
	smp_store_release(&lock->locked, 0);
}

static inline void queued_spin_lock_slowpath(struct qspinlock *lock, u32 val) // 在正常模式，可以直接调用 queued_spin_lock_slowpath
{
	pv_queued_spin_lock_slowpath(lock, val);
}

static inline void queued_spin_unlock(struct qspinlock *lock)
{
	kcsan_release();
	pv_queued_spin_unlock(lock);
}

#define vcpu_is_preempted vcpu_is_preempted
static inline bool vcpu_is_preempted(long cpu)
{
	return pv_vcpu_is_preempted(cpu);
}
#endif
```

```c
struct pv_lock_ops {
 void (*queued_spin_lock_slowpath)(struct qspinlock *lock, u32 val);
 struct paravirt_callee_save queued_spin_unlock;

 void (*wait)(u8 *ptr, u8 val);
 void (*kick)(int cpu);

 struct paravirt_callee_save vcpu_is_preempted;
} ;
```

#ifdef CONFIG_PARAVIRT_SPINLOCKS
#define queued_spin_lock_slowpath	native_queued_spin_lock_slowpath
#endif

```c
/**
 * queued_spin_lock_slowpath - acquire the queued spinlock
 * @lock: Pointer to queued spinlock structure
 * @val: Current value of the queued spinlock 32-bit word
 *
 * (queue tail, pending bit, lock value)
 *
 *              fast     :    slow                                  :    unlock
 *                       :                                          :
 * uncontended  (0,0,0) -:--> (0,0,1) ------------------------------:--> (*,*,0)
 *                       :       | ^--------.------.             /  :
 *                       :       v           \      \            |  :
 * pending               :    (0,1,1) +--> (0,1,0)   \           |  :
 *                       :       | ^--'              |           |  :
 *                       :       v                   |           |  :
 * uncontended           :    (n,x,y) +--> (n,0,0) --'           |  :
 *   queue               :       | ^--'                          |  :
 *                       :       v                               |  :
 * contended             :    (*,x,y) +--> (*,0,0) ---> (*,0,1) -'  :
 *   queue               :         ^--'                             :
 */
void __lockfunc queued_spin_lock_slowpath(struct qspinlock *lock, u32 val)
```


pv_queued_spin_lock_slowpath 的展开是编译技术上的黑科技，但是最后是调用 :
- pv_lock_ops::queued_spin_lock_slowpath

注册者分别是:
- `__pv_queued_spin_lock_slowpath` : kvm 在 kvm_spinlock_init 中注册的，通过一些技术，应该是最后变为 queued_spin_lock_slowpath 的。
- native_queued_spin_lock_slowpath

- `__pv_queued_spin_unlock_slowpath`
  - pv_kick(node->cpu); : 告诉正在等待的 CPU


参考 https://www.kernel.org/doc/Documentation/virtual/kvm/hypercalls.txt

```txt
5. KVM_HC_KICK_CPU
------------------------
Architecture: x86
Status: active
Purpose: Hypercall used to wakeup a vcpu from HLT state
Usage example : A vcpu of a paravirtualized guest that is busywaiting in guest
kernel mode for an event to occur (ex: a spinlock to become available) can
execute HLT instruction once it has busy-waited for more than a threshold
time-interval. Execution of HLT instruction would cause the hypervisor to put
the vcpu to sleep until occurrence of an appropriate event. Another vcpu of the
same guest can wakeup the sleeping vcpu by issuing KVM_HC_KICK_CPU hypercall,
specifying APIC ID (a1) of the vcpu to be woken up. An additional argument (a0)
is used in the hypercall for future use.
```
1. busywaiting 导致执行 halt 指令；
2. guest 执行 halt 指令导致 Host 让 guest 睡眠，去做其他的事情；
3. KVM_HC_KICK_CPU 主动告诉 Host 可以启动了。





[术道经纬](https://zhuanlan.zhihu.com/p/100546935) 比 奔跑吧更加好，大致的想法是:

使用 mcs 增加了一个指针，这会导致任何包含了 `mcs_spinlock` 大小都增加 4 byte, 很难接受。


当只有三个 CPU 之内进行访问，那么使用 ticket spinlock 类似，都是在一个字段上，如果超过了，那么使用 mcs 的方式。

## seqlock
dcache.c:d_lookup 的锁

简而言之:
```c
static inline void write_seqlock(seqlock_t *sl)
{
	spin_lock(&sl->lock);
	do_write_seqcount_begin(&sl->seqcount.seqcount);
}

static inline void do_raw_write_seqcount_end(seqcount_t *s)
{
	smp_wmb();
	s->sequence++;
}

static inline unsigned read_seqbegin(const seqlock_t *sl)
{
	return read_seqcount_begin(&sl->seqcount);
}
```

- 为什么需要使用 memory barrier ?
  - spin lock 可以保证只有一个 writer 存在。
  - 如果不使用，write barrier ，可以相当于 writer 不写该字段了，对于 read barrier 类似。那么就可能让 reader 直接通过了。

### QEMU 的实现

include/qemu/seqlock.h 中，更加简洁明了:

```c
/* Lock out other writers and update the count.  */
static inline void seqlock_write_lock_impl(QemuSeqLock *sl, QemuLockable *lock)
{
    qemu_lockable_lock(lock);
    seqlock_write_begin(sl);
}
```

### rwlock 和 seqlock 的差别
https://stackoverflow.com/questions/55746320/why-rwlock-is-more-popular-than-seqlock-in-linux-kernel

### 其他
- 其他的实现 https://github.com/rigtorp/Seqlock

## TODO
- [ ]  spinlock 和 spinlock_bh
- [ ]  ksoftirqd 的优先级
- [ ]  ticket spinlock


## [Unreliable Guide To Locking](https://www.kernel.org/doc/html/latest/kernel-hacking/locking.html)
介绍各种 kernel 的 lock interface

## local locks
https://lwn.net/Articles/828477/ : 目前唯一的资料
对于访问 percpu 的数据，就像是单核执行一下，是需要屏蔽 preempt_disable 的

但是无法理解，如果允许抢占了，之后只要抢占的 process 运行到相同的位置，那么不是一定 dead lock 吗？


### 如何避免一个 CAS
```c
/**
 * page_counter_try_charge - try to hierarchically charge pages
 * @counter: counter
 * @nr_pages: number of pages to charge
 * @fail: points first counter to hit its limit, if any
 *
 * Returns %true on success, or %false and @fail if the counter or one
 * of its ancestors has hit its configured limit.
 */
bool page_counter_try_charge(struct page_counter *counter,
			     unsigned long nr_pages,
			     struct page_counter **fail)
{
	struct page_counter *c;

	for (c = counter; c; c = c->parent) {
		long new;
		/*
		 * Charge speculatively to avoid an expensive CAS.  If
		 * a bigger charge fails, it might falsely lock out a
		 * racing smaller charge and send it into reclaim
		 * early, but the error is limited to the difference
		 * between the two sizes, which is less than 2M/4M in
		 * case of a THP locking out a regular page charge.
		 *
		 * The atomic_long_add_return() implies a full memory
		 * barrier between incrementing the count and reading
		 * the limit.  When racing with page_counter_set_max(),
		 * we either see the new limit or the setter sees the
		 * counter has changed and retries.
		 */
		new = atomic_long_add_return(nr_pages, &c->usage);
		if (new > c->max) {
			atomic_long_sub(nr_pages, &c->usage);
			/*
			 * This is racy, but we can live with some
			 * inaccuracy in the failcnt which is only used
			 * to report stats.
			 */
			data_race(c->failcnt++);
			*fail = c;
			goto failed;
		}
		propagate_protected_usage(c, new);
		/*
		 * Just like with failcnt, we can live with some
		 * inaccuracy in the watermark.
		 */
		if (new > READ_ONCE(c->watermark))
			WRITE_ONCE(c->watermark, new);
	}
	return true;

failed:
	for (c = counter; c != *fail; c = c->parent)
		page_counter_cancel(c, nr_pages);

	return false;
}
```


## 如何理解 data_race
[^1]: https://lwn.net/Articles/262464/
