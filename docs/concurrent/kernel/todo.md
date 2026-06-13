1. mutex 的 subclass 是什么意思?
2. lockdep 只能处理 Mutex lock 吗?
  - atomic , refcount memory barrier 可以么?
3. virtio-queue 、dpdk 的 queue 恐怕都是需要使用 memory barrier 的吧
找到对应代码的证据


## 似乎使用 ftrace function 功能可以知道一个函数执行的时候的上下文

在 中断，preemptoff ，软中断，workqueue 中？

当前持有了什么锁? (不去打开 CONFIG_PROVE_LOCKING 有办法吗?)

通过 perf 的方法是不行的
```txt
sudo perf probe -a 'enqueue_to_backlog'
sudo perf record -e probe:enqueue_to_backlog -aR sleep 1
sudo perf script
```

```txt
         swapper     0 [018]  7282.064196: probe:enqueue_to_backlog: (ffffffffba767aa0)
         swapper     0 [018]  7282.066346: probe:enqueue_to_backlog: (ffffffffba767aa0)
         swapper     0 [018]  7282.066348: probe:enqueue_to_backlog: (ffffffffba767aa0)
         swapper     0 [022]  7282.066425: probe:enqueue_to_backlog: (ffffffffba767aa0)
         swapper     0 [022]  7282.066431: probe:enqueue_to_backlog: (ffffffffba767aa0)
```

有办法使用 bpftrace 获取到吗?

## 那些 lock 中是 preemption disable 的

1. 一般来说 spin lock 关闭 preemption
2. mutex 可以打开抢占，的确可以

> [!NOTE]
> 参考 Deepseeek ，有待验证
>
> PREEMPT_RT (实时内核) 的特殊情况
> 值得一提的是，在打了 PREEMPT_RT 补丁的实时内核中，情况发生了根本性变化：
>
> 为了消除“优先级反转”并保证延迟确定性，spinlock_t 和 rwlock_t 被实现为支持优先级继承的互斥锁（rt_mutex）。
>
> 这意味着在实时内核中，连 spin_lock() 都变成了可能睡眠的锁，也就不再禁用抢占了！这是实时内核与主线内核在锁机制上的一个核心区别

# Professional Linux Kernel Architecture : Locking and Interprocess Communication

**On uniprocessor systems, spinlocks are defined as empty operations because critical sections cannot be
entered by several CPUs at the same time. However, this does not apply if kernel preemption is enabled**

#### 5.2.3 Semaphores

**Userspace semaphores are
implemented differently**, as described in Section 5.3.2
```
/* Please don't access any members of this structure directly */
struct semaphore {
	raw_spinlock_t		lock;
	unsigned int		count;
	struct list_head	wait_list;
};
```
In contrast to spinlocks, semaphores are **suitable** for protecting longer critical sections against parallel
access. However, they should **not** be used to protect shorter sections because it is very costly to put
processes to sleep and wake them up again — as happens when the semaphore is contended

When an attempt is made to acquire a reserved semaphore with down, the current process is put to sleep
and placed on the wait queue associated with the semaphore. At the same time, the process is placed
in the `TASK_UNINTERRUPTIBLE` state and *cannot receive signals while waiting to enter the critical region*
> 为什么需要屏蔽signals啊 ?


In addition to down, two other operations are used to reserve a semaphore (unlike spinlocks, only one up
function is available and is used to exit the section protected by a semaphore):
1. `down_interruptible` works in the same way as down but places the task in the
`TASK_INTERRUPTIBLE` state if the semaphore could not be acquired. As a result, the
process can be woken by signals while it is sleeping
2. `down_trylock` attempts to acquire a semaphore. If it fails, the process does not go to sleep to wait
for the semaphore but continues execution normally. If the semaphore is acquired, the function
returns a false value, otherwise a true value


This is possible because locking instructions will on
many architectures also act as memory barriers. However, this needs to be checked for the specific cases
that require memory barriers, and general advice is hard to give

#### 5.2.6 Reader/Writer Locks
The kernel therefore provides additional semaphore and spinlock versions to cater for the above — these
are known accordingly as Reader/Writer semaphores and Reader/Writer spinlocks

The `rwlock_t` data type is defined for Reader/Writer spinlocks. Locks must be acquired in different ways
in order to differentiate between read and write access.
1. `read_lock` and `read_unlock` must be executed before and after a critical region to which a process requires read access. The kernel grants any number of read processes concurrent access to
the critical region.
2. `write_lock` and `write_unlock` are used for write access. The kernel ensures that only one writer
(and no readers) is in the region

An `_irq _irqsave` variant is also available and functions in the same way as normal spinlocks. Variants
ending in `_bh` are also available. *They disable software interrupts, but leave hardware interrupts still
enabled*
> ??? 这两个variant 什么时候见过吗?

Read/write semaphores are used in a similar way. The equivalent data structure is struct `rw_semaphore`,
and `down_read` and `up_read` are used to obtain read access to the critical region. Write access is performed with the help of `down_write` and `up_write`. The `_trylock` variants are also available for all
commands — they also function as described above

#### 5.2.7 The Big Kernel Lock
A relic of earlier days is the option of locking the entire kernel to ensure that no processors run in parallel in kernel mode. This lock is known as the big kernel lock but is most frequently referred to by its
abbreviation, BKL.

> 这个东西，现在应该是没有了吧!

#### 5.2.8 Mutexes
Although semaphores can be used to implement the functionality of mutexes, the overhead imposed
by the generality of semaphores is often not necessary.
Because of this, the kernel contains a separate
implementation of special-purpose mutexes that are not based on semaphores.
> 原理上 mutex 和 semaphore 相同，但是实际上实现不同

> 说实话，有点想不懂为什么mutex 需要借助 spinlock 来实现
> spinlock 又是如何实现 ?

## 问题

# lock
- [ ] 验证一下，如果只有单核，内核中很多的 lock 都是会简化的
- [ ] 内核中 lock 和用户态的差别
- [ ] 在 有无preemption 以及 SMP 中间，需要何种锁?


## [Unreliable Guide To Locking](https://www.kernel.org/doc/html/latest/kernel-hacking/locking.html)
介绍各种 kernel 的 lock interface

## [Local locks in the kernel](https://lwn.net/Articles/828477/)
对于访问 percpu 的数据，就像是单核执行一下，是需要屏蔽 preempt_disable 的

但是无法理解，如果允许抢占了，之后只要抢占的 process 运行到相同的位置，那么不是一定 dead lock 吗？

## rtmutex 什么鬼
/home/martins3/core/linux/kernel/locking/rtmutex.c

## lockref
- https://lwn.net/Articles/565734/

# kernel/locking
3. rwsem 和 semaphore 实现可以首先猜测一下再去分析，都非常短小。

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

用 show_lock 验证一下 docs/kernel/mm/yes.md 中的想法

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
