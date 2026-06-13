# introduction to spinlock
1. spinlock 的硬件支持是什么 ： 原子操作原语
2. spinlock 向上提供的接口是什么
3. 为了防止死锁，有什么编程规范吗 ?
4. interupt exception and syscall 给 spin lock 带来了什么问题 ?
5. preemption 给 spinlock 带来什么问题 ?

> 单核非抢占，需要原子操作吗 ?
> 1. 我们连锁都可以不需要，因为切换进程都是程序自动放弃的, 应该不会智障到故意将需要原子的操作故意插入一个语句
> 2. 反对，非抢占不代表 想要的原子操作不会 被打断，比如read 操作完成之后 CPU 检查到需要中断了 
> 3. spinlock 表示 : 不可以持有 spinlock 的进程切换，中断也需要屏蔽



> why not defined spinlock as below

```c
		atomic_t val;
```

1. `spin_lock_init` - produces initialization of the given spinlock;
1. `spin_lock` - acquires given spinlock;
1. `spin_lock_bh` - disables software interrupts and acquire given spinlock;
1. `spin_lock_irqsave` and spin_lock_irq - disable interrupts on local processor, preserve/not preserve previous interrupt state in the flags and acquire given spinlock;
1. `spin_unlock` - releases given spinlock;
1. `spin_unlock_bh` - releases given spinlock and enables software interrupts;
1. `spin_is_locked` - returns the state of the given spinlock;

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

typedef struct raw_spinlock {
	arch_spinlock_t raw_lock;
} raw_spinlock_t;


typedef struct spinlock {
	union {
		struct raw_spinlock rlock;
	};
} spinlock_t;


#define spin_lock_init(_lock)				\
do {							\
	spinlock_check(_lock);				\
	raw_spin_lock_init(&(_lock)->rlock);		\
} while (0)

static __always_inline raw_spinlock_t *spinlock_check(spinlock_t *lock) // @todo 像这种操作不会被优化掉 ? 如何
{
    return &lock->rlock;
}

# define raw_spin_lock_init(lock)				\
	do { *(lock) = __RAW_SPIN_LOCK_UNLOCKED(lock); } while (0)

#define __RAW_SPIN_LOCK_UNLOCKED(lockname)	\
	(raw_spinlock_t) __RAW_SPIN_LOCK_INITIALIZER(lockname)


#define __RAW_SPIN_LOCK_INITIALIZER(lockname)	\
	{					\
	.raw_lock = __ARCH_SPIN_LOCK_UNLOCKED,	\
	SPIN_DEBUG_INIT(lockname)		\
	SPIN_DEP_MAP_INIT(lockname) }


#define	__ARCH_SPIN_LOCK_UNLOCKED	{ { .val = ATOMIC_INIT(0) } }

#define ATOMIC_INIT(i)	{ (i) }
// 简单来说，如果不考虑DEBUG就是给 atomic_t val 赋值为 0
```

> 为什么需要套这么多层次的宏啊!

```c
// 分析spin_lock

static __always_inline void spin_lock(spinlock_t *lock)
{
    raw_spin_lock(&lock->rlock);
}

// several macro expansion

static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
        preempt_disable(); 
        spin_acquire(&lock->dep_map, 0, 0, _RET_IP_); // 似乎这一个函数非常的长，但是不知道是做什么的
        LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock); // 根据后面的分析，此处才是重点
}
```
We need to do this to prevent the process from other processes to preempt it while it is spinning on a lock.
The `spin_acquire` macro which through a chain of other macros expands to the call of the:

```c
#define spin_acquire(l, s, t, i)                lock_acquire_exclusive(l, s, t, NULL, i)
#define lock_acquire_exclusive(l, s, t, n, i)           lock_acquire(l, s, t, 0, 1, n, i)

/*
 * We are not always called with irqs disabled - do that here,
 * and also avoid lockdep recursion:
 */
void lock_acquire(struct lockdep_map *lock, unsigned int subclass,
			  int trylock, int read, int check,
			  struct lockdep_map *nest_lock, unsigned long ip)
{
	unsigned long flags;

	if (unlikely(current->lockdep_recursion))
		return;

	raw_local_irq_save(flags); // 关中断，和猜测的完全一致
	check_flags(flags);

	current->lockdep_recursion = 1;
	trace_lock_acquire(lock, subclass, trylock, read, check, nest_lock, ip);

	__lock_acquire(lock, subclass, trylock, read, check,
		       irqs_disabled_flags(flags), nest_lock, ip, 0, 0); // 关键操作

	current->lockdep_recursion = 0;
	raw_local_irq_restore(flags);　// @todo 立刻恢复？ 难道发不应该出现在抢占设置上面吗 ?
}
EXPORT_SYMBOL_GPL(lock_acquire);
```

The main point of the `lock_acquire` function is to disable hardware interrupts by the call of the `raw_local_irq_save` macro,
because the given spinlock might be acquired with enabled hardware interrupts.


Actually this function is mostly related to the Linux kernel **lock validator** and it is not topic of this part. 
> lock validator : https://www.kernel.org/doc/Documentation/locking/lockdep-design.txt


> 分析其中 : LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
```c
#define LOCK_CONTENDED(_lock, try, lock) \
	lock(_lock)

static inline void do_raw_spin_lock(raw_spinlock_t *lock) __acquires(lock)
{
	__acquire(lock);
	arch_spin_lock(&lock->raw_lock);
}

# define __acquire(x) (void)0

#define arch_spin_lock(l)		queued_spin_lock(l)

static __always_inline void queued_spin_lock(struct qspinlock *lock)
{
	u32 val;

	val = atomic_cmpxchg_acquire(&lock->val, 0, _Q_LOCKED_VAL);
	if (likely(val == 0))
		return;
	queued_spin_lock_slowpath(lock, val);
}

// queued spinlock ? excuse me ?
```
> 只是象征性的对于lock 函数跟踪了一下而已.

# Queued Spinlocks 
Queued spinlocks is a locking mechanism in the Linux kernel which is replacement for the standard spinlocks. At least this is true for the x86_64 architecture.
> 两套机制? 默认实现方式是什么 ?

> 通常实现的方式，test and set 实现方法。
https://www.cs.rochester.edu/~scott/papers/1991_TOCS_synch.pdf
> 多核编程，简直完美!


```c
/*
 * Remapping spinlock architecture specific functions to the corresponding
 * queued spinlock functions.
 */
#define arch_spin_is_locked(l)		queued_spin_is_locked(l)
#define arch_spin_is_contended(l)	queued_spin_is_contended(l)
#define arch_spin_value_unlocked(l)	queued_spin_value_unlocked(l)
#define arch_spin_lock(l)		queued_spin_lock(l)
#define arch_spin_trylock(l)		queued_spin_trylock(l)
#define arch_spin_unlock(l)		queued_spin_unlock(l)

```

> 分析 queued_spin_lock 中间的 atomic_cmpxchg_acquire，经过漫长的各种简单的macro 的跳转:
```c
/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
#define __raw_cmpxchg(ptr, old, new, size, lock)			\
({									\
	__typeof__(*(ptr)) __ret;					\
	__typeof__(*(ptr)) __old = (old);				\
	__typeof__(*(ptr)) __new = (new);				\
	switch (size) {							\
	case __X86_CASE_B:						\
	{								\
		volatile u8 *__ptr = (volatile u8 *)(ptr);		\
		asm volatile(lock "cmpxchgb %2,%1"			\
			     : "=a" (__ret), "+m" (*__ptr)		\
			     : "q" (__new), "0" (__old)			\
			     : "memory");				\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile u16 *__ptr = (volatile u16 *)(ptr);		\
		asm volatile(lock "cmpxchgw %2,%1"			\
			     : "=a" (__ret), "+m" (*__ptr)		\
			     : "r" (__new), "0" (__old)			\
			     : "memory");				\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile u32 *__ptr = (volatile u32 *)(ptr);		\
		asm volatile(lock "cmpxchgl %2,%1"			\
			     : "=a" (__ret), "+m" (*__ptr)		\
			     : "r" (__new), "0" (__old)			\
			     : "memory");				\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile u64 *__ptr = (volatile u64 *)(ptr);		\
		asm volatile(lock "cmpxchgq %2,%1"			\
			     : "=a" (__ret), "+m" (*__ptr)		\
			     : "r" (__new), "0" (__old)			\
			     : "memory");				\
		break;							\
	}								\
	default:							\
		__cmpxchg_wrong_size();					\
	}								\
	__ret;								\
})

// 只是处理各种 字长，最后仅仅调用 cmpxchg? 的指令就结束了
```

`cmpxchgq`
which compares the old with the value pointed to by ptr. If they differ, it stores the new in the memory location which is pointed by the ptr and returns the initial value in this memory location.

分析 : `queued_spin_lock_slowpath`

This bit represents thread which wanted to acquire lock, but it is already acquired by the other thread and queue is empty at the same time
```c
	/*
	 * Wait for in-progress pending->locked hand-overs with a bounded
	 * number of spins so that we guarantee forward progress.
	 *
	 * 0,1,0 -> 0,0,1
	 */
	if (val == _Q_PENDING_VAL) {                     // 不知道这一段在说什么鬼东西 ?
		int cnt = _Q_PENDING_LOOPS;
		val = atomic_cond_read_relaxed(&lock->val,
					       (VAL != _Q_PENDING_VAL) || !cnt--);
	}
```
> skip some paragraph about `queued_spin_lock_slowpath`

Before diving into queueing, we'll see about **MCS** lock mechanism first.
As we already know, each processor in the system has own copy of the lock. The lock is represented by the following structure:

```c
struct mcs_spinlock {
	struct mcs_spinlock *next;
	int locked; /* 1 if lock acquired */
	int count;  /* nesting count, see qspinlock.c */
};
```
> 后面继续分析 queued_spin_lock_slowpath 但是我没有理解，

In the previous part we already met the first synchronization primitive spinlock provided by the Linux kernel which is implemented as **ticket spinlock**.
In this part we saw another implementation of the spinlock mechanism - queued spinlock
> ticket spinlock ? 什么鬼东西 ?

#  Semaphores


# R/W semaphore
**read operation is performed more often than write operation**. In this case, it would be logical to we may lock data in such way.

As you may guess, implementation of the reader/writer semaphore is based on the implementation of the normal semaphore.

> 神奇啊，rw semaphore 和 普通的 semaphore 实现没有区别啊!

As we already saw in previous parts of this chapter, all synchronization primitives may be initialized in two ways: statically and dynamically.

- `void down_read(struct rw_semaphore *sem)` - lock for reading;
- `int down_read_trylock(struct rw_semaphore *sem)` - try lock for reading;
- `void down_write(struct rw_semaphore *sem)` - lock for writing;
- `int down_write_trylock(struct rw_semaphore *sem)` - try lock for writing;
- `void up_read(struct rw_semaphore *sem)` - release a read lock;
- `void up_write(struct rw_semaphore *sem)` - release a write lock;


`rwsem.c` /home/shen/linux/kernel/locking/rwsem.c



# 附带问题
1. 当内核使用锁的时候，似乎只是为了避免内核数据的互斥，关于用户态的数据的data race : 
    1. 用户态使用的各种锁机制，从plka 中间介绍的，是如何实现的，应该是不可能调用 syscall 获取的吧! 看一下glibc 对应的实现.

2. 锁: 只能保护一个bit 或者一个int，进而防止多个线程同时进入到 critical region 或者 同时访问数据
    1. 锁可以保护(不让多个thread 同时访问)代码， 也可以保护data
    2. 保护代码的方法是在 代码的首尾设置
    3. 保护数据的方法是结构体中间含有一个锁

3. 实现一个锁非常的简单，真正的难度在于:
    1. 无法确定什么被保护，文件系统中间到处都是锁，和进程相关的几乎没有锁(task_struct 之类的结构体，fork 之类的东西)
        1. 如果子系统，被多个thread public，那么需要锁，但是如果数据或者函数是 thraad private的话，就不需要
        2. 为什么slab, buddy system 没有 rmap 中间如此复杂的锁层级结构(还是说没有找到而已)
    2. 锁的粒度如何确定 ? 粒度过小的问题是什么 ?
    3. 高效的锁实现，queued_spin_lock  以及 rcu 机制
    4. 
  
4. lock free 是怎么回事 ?

5. rcu 机制

> 1. 是时候解决READ_ONCE 的问题了
> 2. atomic_read 和 普通的read 有什么区别吗 ? read 操作本身就是atomic 的吧!
