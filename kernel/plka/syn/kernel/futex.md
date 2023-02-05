# futex

如果不使用，在用户层就不能使用这个吗 ?

process A 无法获取到 lock，那么 sleep，当 process B 完事之后，将 sleep 在这一个位置上的所有 process 叫醒即可，就利用 signal 机制。

- [lwn](https://lwn.net/Articles/360699/)


- [blog](https://eli.thegreenplace.net/2018/basics-of-futexes/)
> @todo 就 futex 的使用讲解的非常好

However, in the unlikely event that another thread did try to take the lock at the same time, the atomic approach may fail. In this case there are two options. We can busy-loop using the atomic until the lock is cleared; while this is 100% userspace, it can also be extremely wasteful since looping can significantly occupy a core, and the lock can be held for a long time. The alternative is to "sleep" until the lock is free (or at least there's a high chance that it's free); we need the kernel to help with that, and this is where futexes come in.

Simply stated, a futex is a kernel construct that helps userspace code synchronize on shared events. Some userspace processes (or threads) can wait on an event (FUTEX_WAIT), while another userspace process can signal the event (FUTEX_WAKE) to notify waiters.


- [wiki](https://en.wikipedia.org/wiki/Futex)
A futex consists of a kernelspace wait queue that is attached to an atomic integer in userspace. Multiple processes or threads operate on the integer entirely in userspace (using atomic operations to avoid interfering with one another), and only resort to relatively expensive system calls to request operations on the wait queue (for example to wake up waiting processes, or to put the current process on the wait queue). A properly programmed futex-based lock will not use system calls except when the lock is contended; since most operations do not require arbitration between processes, this will not happen in most cases.

Man futex(2)

A futex is a 32-bit value—referred to below as a futex word—whose address is supplied to the `futex()` system call.
All futex operations are governed by this value.  In order to share a futex between processes, the futex is placed in a region of shared memory, created using (for example) mmap(2) or shmat(2).
(Thus, the futex word may have different virtual addresses in different processes, but these addresses all refer to the same location in physical memory.)  In a multithreaded program, it is sufficient to place the futex word in a global variable shared by all threads.

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

## futex2
- https://lkml.org/lkml/2021/4/27/1208

## set_tid_address
从 man 和 [^3] 看，似乎是配合 pthread 用于 pthread_join，而且会进一步依赖于 futex 来操作
[^3]: https://stackoverflow.com/questions/6975098/when-is-the-system-call-set-tid-address-used

[^2]: https://eli.thegreenplace.net/2018/basics-of-futexes/
