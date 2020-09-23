# Linux Programming Interface: Chapter 28


## 28.2 The clone() System Call

> Within the kernel, fork(), vfork(), and clone() are ultimately implemented by the
same function (do_fork() in kernel/fork.c). At this level, cloning is much closer
to forking: sys_clone() doesn’t have the func and func_arg arguments, and after
the call, sys_clone() returns in the child in the same manner as fork(). The main
text describes the clone() wrapper function that glibc provides for sys_clone().
(This function is defined in architecture-specific glibc assembler sources, such
as in sysdeps/unix/sysv/linux/i386/clone.S.) This wrapper function invokes
func after sys_clone() returns in the child.

Man clone(2) 会发现其第一个参数是函数指针，但是 sys_clone 并没有，这是 glibc 提供的


@todo clone 和 pthread_create 的关系是什么 ?

> The clone() flags argument serves two purposes.First, its lower byte specifies the
child’s termination signal, which is the signal to be sent to the parent when the child
terminates. (If a cloned child is stopped by a signal, the parent still receives SIGCHLD.)
This byte may be 0, in which case no signal is generated. (Using the Linux-specific
/proc/PID/stat file, we can determine the termination signal of any process; see the
proc(5) manual page for further details.)
> 
> The remaining bytes of the flags argument hold a bit mask that controls the operation of clone(). We summarize these bit-mask values in Table 28-2, and describe
them in more detail in Section 28.2.1.

clone 参数 flags 的作用，signal 和 各种 `CLONE_*`控制，包含大量关于 mamespace 的内容

@todo 接着讲解一个例子。

#### 28.2.1 The `clone()` flags Argument

At this point, it is worth remarking that, to some extent, we are playing with
words when trying to draw a distinction between the terms thread and process. It
helps a little to introduce the term `kernel scheduling entity (KSE)`, which is used in
some texts to refer to the objects that are dealt with by the kernel scheduler. Really,
threads and processes are simply KSEs that provide for greater and lesser degrees
of sharing of attributes (virtual memory, open file descriptors, signal dispositions,
process ID, and so on) with other `KSEs`. **The POSIX threads specification provides
just one out of various possible definitions of which attributes should be shared
between threads.**

* ***Sharing file descriptor tables: CLONE_FILES***
> skip 各种内容

* ***Threading library support: CLONE_PARENT_SETTID, CLONE_CHILD_SETTID, and CLONE_CHILD_CLEARTID***

The `CLONE_PARENT_SETTID`, `CLONE_CHILD_SETTID`, and `CLONE_CHILD_CLEARTID` flags were
added in Linux 2.6 to support the implementation of POSIX threads. These flags
affect how `clone()` treats its `ptid` and `ctid` arguments. `CLONE_PARENT_SETTID` and
`CLONE_CHILD_CLEARTID` are used in the NPTL threading implementation.

```c
int clone(int (*fn)(void *), void *stack, int flags, void *arg, ...
         /* pid_t *parent_tid, void *tls, pid_t *child_tid */ );
```

If the `CLONE_PARENT_SETTID` flag is set, then the kernel writes the thread ID of the
child thread into the location pointed to by `ptid`. The thread ID is copied into ptid
before the memory of the parent is duplicated. This means that, even if the `CLONE_VM`
flag is not specified, both the parent and the child can see the child’s thread ID in
this location. (As noted above, the `CLONE_VM` flag is specified when creating POSIX threads.)

The `CLONE_PARENT_SETTID` flag exists in order to provide a reliable means for a
threading implementation to **obtain the ID of the new thread**.
Note that it isn’t sufficient to obtain the thread ID of the new thread via the return value of `clone()`, like so:

```c
tid = clone(...);
```

The problem is that this code can lead to various race conditions, because the
assignment occurs only after `clone()` returns. For example, suppose that the new
thread terminates, and the handler for its termination signal is invoked before the
assignment to tid completes. In this case, the handler can’t usefully access tid.
(Within a threading library, `tid` might be an item in a global bookkeeping structure
used to track the status of all threads.) Programs that invoke `clone()` directly often
can be designed to work around this race condition. However, a threading library
can’t control the actions of the program that calls it. Using `CLONE_PARENT_SETTID` to
ensure that the new thread ID is placed in the location pointed to by ptid before
`clone()` returns allows a threading library to avoid such race conditions
> @todo 这么说，为什么 fork 采用 pid_t  = fork(....) 的时候就是没有问题的 ?

If the `CLONE_CHILD_SETTID` flag is set, then clone() writes the thread ID of the child
thread into the location pointed to by ctid. The setting of ctid is done only in the
child’s memory, but this will affect the parent if CLONE_VM is also specified. Although
NPTL doesn’t need CLONE_CHILD_SETTID, this flag is provided to allow flexibility for
other possible threading library implementations.

If the `CLONE_CHILD_CLEARTID` flag is set, then `clone()` zeros the memory location 
pointed to by ctid when the child terminates.

The `ctid` argument is the mechanism (described in a moment) by which the
NPTL threading implementation obtains notification of the termination of a
thread. Such notification is required by the `pthread_join()` function, which is the
POSIX threads mechanism by which one thread can wait for the termination of
another thread.

The kernel treats the location pointed to by ctid as a futex, an efficient synchronization mechanism. (See the futex(2) manual page for further details of futexes.)
Notification of thread termination can be obtained by performing a futex() system
call that blocks waiting for a change in the value at the location pointed to by ctid.
(Behind the scenes, this is what `pthread_join()` does.) At the same time that the kernel
clears ctid, it also wakes up any kernel scheduling entity (i.e., thread) that is blocked
performing a futex wait on that address. (At the POSIX threads level, this causes
the pthread_join() call to unblock.)
