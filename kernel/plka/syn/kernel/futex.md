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
