# Linux Programming Interface: Chapter 47 : SYSTEM V Semaphores
> 真的是完全不知道内核的这一部分在说什么，首先阅读对应的用户态是一个不错的选择
One
common use of a semaphore is to synchronize access to a block of shared memory,
in order to prevent one process from accessing the shared memory at the same
time as another process is updating it.

## 47.1 Overview
The general steps for using a System V semaphore are the following:
1. [`semget`](http://man7.org/linux/man-pages/man2/semget.2.html)
2. [`semctl`](http://man7.org/linux/man-pages/man2/semctl.2.html) SETVAL SETALL
3. [`semop`](http://man7.org/linux/man-pages/man2/semop.2.html)
4. semctl IPC_RMID

However, System V semaphores are rendered unusually complex
by the fact that they are allocated in groups called `semaphore sets`.
> 好端端的 semaphore 为什么非要搞一个 set 出来，需要完成什么特殊功能　是　一个 semaphore 完成不了的 ?

The number of
semaphores in a set is specified when the set is created using the semget() system
call. While it is common to operate on a single semaphore at a time, the semop() system
call allows us to atomically perform a group of operations on multiple semaphores
in the same set.
> 为什么 semaphore 需要 id

> 接下来分析了一个例子 `svsem/svsem_demo.c`，简要的利用以上的三个函数。

## 47.2
The semget() system call creates a new semaphore set or obtains the identifier of an
existing set.


## 47.3 Semaphore Control Operations
The `semctl()` system call performs a variety of control operations on a semaphore set
or on an individual semaphore within a set


1. Generic control operations
2. Retrieving and initializing semaphore values
3. Retrieving per-semaphore information
4. Monitoring a semaphore set

## 47.4 Semaphore Associated Data Structure
Each semaphore set has an associated semid_ds data structure of the following form:
```
struct semid_ds {
  struct ipc_perm sem_perm; /* Ownership and permissions */
  time_t sem_otime; /* Time of last semop() */
  time_t sem_ctime; /* Time of last change */
  unsigned long sem_nsems; /* Number of semaphores in set */
};
```
## 47.5 Semaphore Initialization
由于创建和初始化不是原子操作，所以 semaphore 的初始化　需要被注意。


## 47.6 Semaphore Operations
The `semop()` system call performs one or more operations on the semaphores in the
semaphore set identified by `semid`.

> semop 实现　semaphore set 中间的 semaphore 的数值加减等操作


## 47.7 Handling of Multiple Blocked Semaphore Operations
> 描述如何处理　多个被阻塞的进程同时获取　semaphore 时的问题

## 47.8 Semaphore Undo Values


## 47.8 Semaphore Undo Values
> 此处以及后面都没有看

## 47.9 Implementing a Binary Semaphores Protocol
> TO READ，综合训练，应该是很有意思的东西

