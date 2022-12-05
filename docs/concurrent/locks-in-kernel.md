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

## preempt
- 中断的屏蔽和 preempt 的 disable 是两个事情
- [ ] 那些地方是必须 `preempt_disable` 但是无需屏蔽中断

## qspinlock
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
1. PV 是什么概念 ?

[术道经纬](https://zhuanlan.zhihu.com/p/100546935) 比 奔跑吧更加好，大致的想法是:

使用 mcs 增加了一个指针，这会导致任何包含了 `mcs_spinlock` 大小都增加 4 byte, 很难接受。


当只有三个 CPU 之内进行访问，那么使用 ticket spinlock 类似，都是在一个字段上，如果超过了，那么使用 mcs 的方式。

## seqlock
dcache.c:d_lookup 的锁

### https://github.com/rigtorp/Seqlock
超级简单清晰的分析

使用 segnum 当 reader 在临界区的时候，writer 是否进行过操作，如果是，那么重新尝试。

其实关键在于 memory order 的

## lockless
让人想起了 slab 的内容:
https://lwn.net/SubscriberLink/827180/a1c1305686bfea67/

## futex
基本介绍 [^2]

这个解释了，既然可以使用 userspace 的 spinlock，为什么还是要使用内核:
https://linuxplumbersconf.org/event/4/contributions/286/attachments/225/398/LPC-2019-OptSpin-Locks.pdf

用户态的 spinlock 就是直接使用 原子操作的:
```c
int pthread_spin_lock(pthread_spinlock_t *s)
{
    while (*(volatile int *)s || a_cas(s, 0, EBUSY)) a_spin();
    return 0;
}
```
但是 mutex 的实现, 最终会调用的 futex 的:
```c
int __pthread_mutex_lock(pthread_mutex_t *m)
{
    if ((m->_m_type&15) == PTHREAD_MUTEX_NORMAL
        && !a_cas(&m->_m_lock, 0, EBUSY))
        return 0;

    return __pthread_mutex_timedlock(m, 0);
}
```

> 这个时候, 再来说明一下，为什么 futex 的要义:

The futex() system call provides a method for waiting until a certain condition becomes true.  It is typically used
as a blocking construct in the context of shared-memory synchronization.  When using futexes, the majority  of  the
synchronization  operations are performed in user space.  A user-space program employs the futex() system call only
when it is likely that the program has to block for a longer time until the condition becomes true.  Other  futex()
operations can be used to wake any processes or threads waiting for a particular condition.

在用户态, 如果仅仅靠 spinlock，两个毫不相关的进程上锁，当一个进程 pthread_mutex_unlock 之后，根本无法无法去通知另一个等待在其上的 process

比如 `FUTEX_WAKE` 在内核对应代码:
- `futex_wake`
  - `wake_up_q`
    - `wake_up_process`

## set_tid_address
从 man 和 [^3] 看，似乎是配合 pthread 用于 pthread_join，而且会进一步依赖于 futex 来操作

## TODO
面试:
- [ ]  spinlock 和 spinlock_bh
- [ ]  ksoftirqd 的优先级
- [ ]  memcg 如何操作 slab (本来认为 slab 作为内核的部分，不会被 memcg 控制)
- [ ]  ticket spinlock

- https://lkml.org/lkml/2021/4/27/1208

## 记录一下遇到的问题

```c
static struct page *page_idle_get_page(unsigned long pfn)
{
    struct page *page = pfn_to_online_page(pfn);

    if (!page || !PageLRU(page) ||
        !get_page_unless_zero(page))
        return NULL;

    if (unlikely(!PageLRU(page))) {
    // 这一段的内容是啥意思，在上面的检查中，这不是必然 not
    // 如果是从 lock 的角度考虑，还是存在问题的啊，就算这个地方通过，那么也没有上锁，下面直接过去了，怎么办?
        put_page(page);
        page = NULL;
    }
    return page;
}
```

[^1]: https://lwn.net/Articles/262464/
[^2]: https://eli.thegreenplace.net/2018/basics-of-futexes/
[^3]: https://stackoverflow.com/questions/6975098/when-is-the-system-call-set-tid-address-used
