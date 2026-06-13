# introduction to spinlock

1. 为了防止死锁，有什么编程规范吗 ?
2. 如何防止锁导致的性能问题?

1. spinlock 的硬件支持是什么 ： 原子操作原语
2. spinlock 向上提供的接口是什么
4. interupt exception and syscall 给 spin lock 带来了什么问题 ?
5. preemption 给 spinlock 带来什么问题 ?

单核非抢占，需要原子操作吗 ?
1. 我们连锁都可以不需要，因为切换进程都是程序自动放弃的, 应该不会智障到故意将需要原子的操作故意插入一个语句
2. 反对，非抢占不代表 想要的原子操作不会 被打断，比如 read 操作完成之后 CPU 检查到需要中断了
3. spinlock 表示 : 不可以持有 spinlock 的进程切换，中断也需要屏蔽


> why not defined spinlock as below

```c
		atomic_t val;
```

- `spin_lock_init` - produces initialization of the given spinlock;
- `spin_lock` - acquires given spinlock;
- `spin_lock_bh` - disables software interrupts and acquire given spinlock;
- `spin_lock_irqsave` and spin_lock_irq - disable interrupts on local processor, preserve/not preserve previous interrupt state in the flags and acquire given spinlock;
- `spin_unlock` - releases given spinlock;
- `spin_unlock_bh` - releases given spinlock and enables software interrupts;
- `spin_is_locked` - returns the state of the given spinlock;

## 在 d_walk 中存在直接使用

如何理解?
```c
			spin_release(&dentry->d_lock.dep_map, _RET_IP_);
			this_parent = dentry;
			spin_acquire(&this_parent->d_lock.dep_map, 0, 1, _RET_IP_);
```


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

> 分析 queued_spin_lock 中间的 atomic_cmpxchg_acquire，经过漫长的各种简单的 macro 的跳转:
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


# 附带问题
1. 当内核使用锁的时候，似乎只是为了避免内核数据的互斥，关于用户态的数据的 data race :
    1. 用户态使用的各种锁机制，从 plka 中间介绍的，是如何实现的，应该是不可能调用 syscall 获取的吧! 看一下 glibc 对应的实现.

2. 锁: 只能保护一个 bit 或者一个 int，进而防止多个线程同时进入到 critical region 或者 同时访问数据
    1. 锁可以保护(不让多个 thread 同时访问)代码， 也可以保护 data
    2. 保护代码的方法是在 代码的首尾设置
    3. 保护数据的方法是结构体中间含有一个锁

3. 实现一个锁非常的简单，真正的难度在于:
    1. 无法确定什么被保护，文件系统中间到处都是锁，和进程相关的几乎没有锁(task_struct 之类的结构体，fork 之类的东西)
        1. 如果子系统，被多个 thread public，那么需要锁，但是如果数据或者函数是 thraad private 的话，就不需要
        2. 为什么 slab, buddy system 没有 rmap 中间如此复杂的锁层级结构(还是说没有找到而已)
    2. 锁的粒度如何确定 ? 粒度过小的问题是什么 ?
    3. 高效的锁实现，queued_spin_lock  以及 rcu 机制

> 2. atomic_read 和 普通的 read 有什么区别吗 ? read 操作本身就是 atomic 的吧!

## hc
https://news.ycombinator.com/item?id=38652796

## 这里讲了 spinlock
https://zhuanlan.zhihu.com/p/93289632

## spinlock
- [ ] qspinlock
- [ ] ticket spinlock
- https://jgsun.github.io/2021/09/12/ARM-ticket-spinlock/

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
