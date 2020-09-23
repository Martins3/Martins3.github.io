# kernel/lock

| name                 | blank | comment | code | comment |
|----------------------|-------|---------|------|---------|
| lockdep.c            | 698   | 877     | 2963 |         |
| locktorture.c        | 143   | 111     | 808  | 测试    |
| rtmutex.c            | 234   | 903     | 760  |         |
| mutex.c              | 220   | 484     | 741  |         |
| lockdep_proc.c       | 102   | 23      | 538  | ?       |
| test-ww_mutex.c      | 123   | 17      | 507  |     |
| rwsem-xadd.c         | 89    | 279     | 339  |         |
| spinlock.c           | 41    | 48      | 303  |       |
| qspinlock_paravirt.h | 64    | 267     | 232  |         |
| rwsem-spinlock.c     | 60    | 77      | 202  |         |
| qspinlock.c          | 66    | 284     | 178  |         |
| spinlock_debug.c     | 31    | 26      | 169  |         |
| qspinlock_stat.h     | 34    | 106     | 151  |         |
| rwsem.c              | 52    | 30      | 142  |         |
| semaphore.c          | 31    | 91      | 141  |         |
| rtmutex-debug.c      | 27    | 27      | 128  |         |
| lockdep_internals.h  | 26    | 39      | 122  |         |
| rtmutex_common.h     | 26    | 38      | 102  |         |
| osq_lock.c           | 34    | 101     | 96   |         |
| percpu-rwsem.c       | 28    | 79      | 85   |         |
| mutex-debug.c        | 12    | 29      | 66   |         |
| mcs_spinlock.h       | 13    | 64      | 44   |         |
| rwsem.h              | 8     | 39      | 40   |         |
| qrwlock.c            | 8     | 49      | 35   |         |
| Makefile             | 3     | 3       | 26   |         |
| rtmutex-debug.h      | 3     | 11      | 23   |         |
| rtmutex.h            | 3     | 12      | 20   |         |
| mutex-debug.h        | 1     | 15      | 13   |         |
| mutex.h              | 3     | 11      | 11   |         |
| lockdep_states.h     | 0     | 6       | 2    |         |


内核同步的主要内容:
1. 各种锁机制的提供
2. memory-barrier
3. percpu
4. rcu 机制

## TODO
1. spinlock 和 qspinlock 的区别是什么 ?
2. 猜测 spinlock 的核心实现都是在 arch 中间的
3. rwsem 和 semaphore 实现可以首先猜测一下再去分析，都非常短小。

## Doc

> NOTE! RCU is better for list traversal, but requires careful
> attention to design detail (see Documentation/RCU/listRCU.txt).

> IFF you know that the spinlocks are
> never used in interrupt handlers, you can use the non-irq versions:
> 
> 	  spin_lock(&lock);
> 	  ...
> 	  spin_unlock(&lock);
> 
> (and the equivalent read-write versions too, of course). The spinlock will
> guarantee the same kind of exclusive access, and it will be much faster.
> This is useful if you know that the data in question is only ever
> manipulated from a "process context"
> The reasons you mustn't use these versions if you have interrupts that
> play with the spinlock is that you can get deadlocks:
> 
>     spin_lock(&lock);
>     ...
>       <- interrupt comes in:
>         spin_lock(&lock);
> 
> where an interrupt tries to lock an already locked variable. This is ok if
> the other interrupt happens on another CPU, but it is _not_ ok if the
> interrupt happens on the same CPU that already holds the lock, because the
> lock will obviously never be released (because the interrupt is waiting
> for the lock, and the lock-holder is interrupted by the interrupt and will
> not continue until the interrupt has been processed).
> 
> Note that you can be clever with read-write locks and interrupts. For
> example, if you know that the interrupt only ever gets a read-lock, then
> you can use a non-irq version of read locks everywhere - because they
> don't block on each other (and thus there is no dead-lock wrt interrupts.
> But when you do the write-lock, you have to use the irq-safe version.
> 
> For an example of being clever with rw-locks, see the "waitqueue_lock"
> handling in kernel/sched/core.c - nothing ever _changes_ a wait-queue from
> within an interrupt, they only read the queue in order to know whom to
> wake up. So read-locks are safe (which is good: they are very common
> indeed), while write-locks need to protect themselves against interrupts.

为什么在 interrupt context 需要使用 spin_lock_irqsave , 因为防止其他 interrupt line 中间的 handler 在已经持有lock, 然后重新获取锁。
spin_lock_irqsave : 会 disable 当前 CPU 的 interrupt ，不仅仅是当前的 interrupt line 上的(这个是默认的)，而是几乎所有的位置的

## MCS

```java
// 加入队列，等待队列为空
class Qnode {
boolean locked = false;
Qnode next = null;
}
class MCSLock implements Lock {
  tail = new AtomicReference<Qnode>(null);
  public void lock() {
  Qnode Qnode = new Qnode(); //新结点
  Qnode pred = tail.getAndSet(Qnode); //加入队尾
  if (pred != null) { //若队列不空
  Qnode.locked = true; //准备自旋
  pred.next = Qnode; //将前驱结点的 next 指向新结点
  while (Qnode.locked) {} //在新结点上自旋
}}

// 通知后继节点
public void unlock() {
if (Qnode.next == null) {
if (tail.CAS(Qnode, null) //没有后继线程
return;
while (Qnode.next == null) {} //等待后继结点加入队尾
}
Qnode.next.locked = false; //通知后继结点
}}
```

## 问题
1. CONFIG_LOCKDEP 的作用是什么 ?
2. spinlock 和 qspinlock 的区别是什么 ?
3. 

# READ_ONCE
https://github.com/google/ktsan/wiki/READ_ONCE-and-WRITE_ONCE
https://lwn.net/Articles/508991/

```c
    #define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
```
曾经有一段时间，使用 ACCESS_ONCE 维持生活，现在已经完全清除掉了, 但是和 READ_ONCE WRITE_ONCE 的目的相同的。


## PER_CPU

```c
static void __lru_cache_add(struct page *page)
{
	struct pagevec *pvec = &get_cpu_var(lru_add_pvec);

	page_cache_get(page);
	if (!pagevec_space(pvec))
		__pagevec_lru_add(pvec);
	pagevec_add(pvec, page);
	put_cpu_var(lru_add_pvec);
}
```

**Since the function accesses a CPU-specific data structure, it must prevent the kernel from interrupting
execution** and resuming later on another CPU. This form of protection is enabled implicitly by invoking
`get_cpu_var`, which **not only disables preemption, but also returns the per-CPU variable**.

```c
/*
 * Must be an lvalue. Since @var must be a simple identifier,
 * we force a syntax error here if it isn't.
 */
#define get_cpu_var(var)						\
(*({									\
	preempt_disable();						\
	this_cpu_ptr(&var);						\
}))

/*
 * The weird & is necessary because sparse considers (void)(var) to be
 * a direct dereference of percpu variable (var).
 */
#define put_cpu_var(var)						\
do {									\
	(void)&(var);							\
	preempt_enable();						\
} while (0)
```
> 龟龟，这说的是什么东西啊。


http://www.makelinux.co.il/ldd3/chp-8-sect-5.shtml
https://lwn.net/Articles/22911/

> 接口实现还是很简单的



```c
struct per_cpu_pageset {
	struct per_cpu_pages pcp;
#ifdef CONFIG_NUMA
	s8 expire;
	u16 vm_numa_stat_diff[NR_VM_NUMA_STAT_ITEMS];
#endif
#ifdef CONFIG_SMP
	s8 stat_threshold;
	s8 vm_stat_diff[NR_VM_ZONE_STAT_ITEMS];
#endif
};

struct per_cpu_nodestat {
	s8 stat_threshold;
	s8 vm_node_stat_diff[NR_VM_NODE_STAT_ITEMS];
};
// 一般都是遇到的变量为文档
```

```c
__this_cpu_read(pcp->stat_threshold);
__this_cpu_write(*p, x);

// @todo 硬件是如何实现识别当前cpu 的

// @todo 可不可以写一个用户态 __percpu　的程序

// @todo 找到clang 的 __percpu 的文档是什么 
```

The allocated structures are stored in a per-CPU variable, meaning that the calling function must perform the insertion before it can schedule or be moved to a different processor.
> percup 有趣的限制，同时，我们是如何保证的这一个要求的

