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
