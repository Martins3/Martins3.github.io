# Linux Programming Interface: Chapter 55

## 55.1 Overview


Although file locking is normally used in conjunction with file I/O, we can also use
it as a more general synchronization technique. Cooperating processes can follow a
convention that locking all or part of a file indicates access by a process to some
shared resource other than the file itself (e.g., a shared memory region)

> file lock 不仅仅可以运用于文件上，还可以运用于其他，比如 shared memory region 上

## 55.2 File Locking with flock()

Converting a shared lock to an exclusive lock will block if another process holds a
shared lock on the file, unless LOCK_NB was also specified.

A lock conversion is not guaranteed to be atomic. During conversion, the existing lock is first removed, and then a new lock is established.

> lock conversion 在这里指的是，本来一个 file 被 share lock，如果想要上一个 exclusive clock

A file lock obtained via flock() is associated with the open file description (Section 5.4), rather
than the file descriptor or the file (i-node) itself.

> - 由于 flock 创建的 lock 是基于 fd 的，这导致使用 dup 或者 fork 的时候，只需要在一个 fd 上 unlock
> - open 两次，创建两个 fd, 那么就可以分别对于这两个 fd 上锁

Placing locks with flock() suffers from a number of limitations:
1. Only whole files can be locked.
2. We can place only advisory locks with flock().
3. Many NFS implementations don’t recognize locks granted by flock().

## 55.3 Record Locking with fcntl()
A process can never lock itself out of a file region, even when placing locks via multiple file descriptors referring to the same file.
> ???

Closing a file descriptor has some unusual semantics with respect to file region locks.
> ???

Deadlock situations are detected even when placing locks on multiple different
files, as are circular deadlocks involving multiple processes.
> 有点好奇，内核是怎么实现检测这个的

The kernel data structure used to maintain information about locks is designed to
satisfy these requirements. Each open file has an associated linked list of locks held
against that file. Locks within the list are ordered, first by process ID, and then by
starting offset.

> 基于 inode 的 file lock 的效果 : 使用 open 两次，打开两次文件，创建两个 fd 和 file structure, 但是，只要对于其中任何一个关闭，那么这些锁就消失了

An unsurprising consequence of this association is that when a process terminates, all of its record locks are released. Less expected is that
whenever a process closes a file descriptor, all locks held by the process on the
corresponding file are released, regardless of the file descriptor(s) through
which the locks were obtained.

The semantics of fcntl() lock inheritance and release are an architectural blemish. For example, they make the use of record locks from library packages problematic, since a library function can’t prevent the possibility that its caller will close a
file descriptor referring to a locked file and thus remove a lock obtained by the
library code. A
> 在锁的释放上，基于inode 的语义很奇怪, 在一个 process 中间，对于任意的一个 fd 的 close, 导致该 process 释放对于该文件的 lock

In summary, the use of mandatory locks is best avoided.
> 使用 mandatory file lock 非常的麻烦，而且存在性能损失，漏洞，效果也不是想想的样子

> 利用 /proc/locks 可以找到是哪一个进程在lock 哪一个文件

Some programs—in particular, many daemons—need to ensure that only one
instance of the program is running on the system at a time. A common method of
doing this is to have the daemon create a file in a standard directory and place a
write lock on it.

> 更多的用户态参考 [^9]

[^8]: [filelock](https://man7.org/linux/man-pages/man3/flockfile.3.html)

[^9]: [blog : File locking in Linux](https://gavv.github.io/articles/file-locks/)

