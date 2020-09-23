# Linux Programming Interface: Chapter 48: System V  Shared Memory
Since a shared memory segment becomes part of a process’s user-space
memory, no kernel intervention is required for IPC
## 48.1 Overview

In order to use a shared memory segment, we typically perform the following steps:
1. Call `shmget()` to create a new shared memory segment or obtain the identifier of
an existing segment (i.e., one created by another process). This call returns a
shared memory identifier for use in later calls.
2. Use `shmat()` to attach the shared memory segment; that is, make the segment
part of the virtual memory of the calling process.
3. At this point, the shared memory segment can be treated just like any other
memory available to the program. In order to refer to the shared memory, the
program uses the addr value returned by the shmat() call, which is a pointer to
the start of the shared memory segment in the process’s virtual address space.
4. Call `shmdt()` to detach the shared memory segment. After this call, the process
can no longer refer to the shared memory. This step is optional, and happens
automatically on process termination.
5. Call `shmctl()` to delete the shared memory segment. The segment will be
destroyed only after all currently attached processes have detached it. Only
one process needs to perform this step


1. http://man7.org/linux/man-pages/man2/shmget.2.html


## 48.2 Creating or Opening a Shared Memory Segment
>  shmget 和 semget 的参数列表很类似


## 48.3 Using Shared Memory
The `shmat()` system call attaches the shared memory segment identified by shmid to
the calling process’s virtual address space.

## 48.4 Example: Transferring Data via Shared Memory
> 使用reader writer 作为一个例子。
> 使用方法: 打开两个终端 运行writer 运行reader 在writer中输出reader 中可以回显
> but the shmem key is hardcoded !

## 48.5 Location of Shared Memory in Virtual Memory
In Section 6.3, we considered the layout of the various parts of a process in virtual
memory. It is useful to revisit this topic in the context of attaching System V
shared memory segments.

```c
/*
 * This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	(PAGE_ALIGN(TASK_SIZE / 3))
```
> 64位 内核上的定义

## 48.6 Storing Pointers in Shared Memory
> skip


## 48.7 Storing Pointers in Shared Memory
The `shmctl()` system call performs a range of control operations on the shared memory segment identified by `shmid`.

A shared memory segment can be locked into RAM, so that it is never swapped
out.

## 48.8 Shared Memory Associated Data Structure
> 内核中间 对于shmid_ds 的定义　比如下的用户态的定义要麻烦的多。
```
struct shmid_ds {
  struct ipc_perm shm_perm; /* Ownership and permissions */
  size_t shm_segsz; /* Size of segment in bytes */
  time_t shm_atime; /* Time of last shmat() */
  time_t shm_dtime; /* Time of last shmdt() */
  time_t shm_ctime; /* Time of last change */
  pid_t shm_cpid; /* PID of creator */
  pid_t shm_lpid; /* PID of last shmat() / shmdt() */
  shmatt_t shm_nattch; /* Number of currently attached processes */
};
```

## 48.9 Shared Memory Limits
Most UNIX implementations impose various limits on System V shared memory.
Below is a list of the Linux shared memory limits. The system call affected by the
limit and the error that results if the limit is reached are noted in p


## 48.10 Summary
The recommended approach when attaching a shared memory segment is to
allow the kernel to choose the address at which the segment is attached in the process’s virtual address space. This means that the segment may reside at different
virtual addresses in different processes. For this reason, any references to
addresses within the segment should be maintained as relative offsets, rather than
as absolute pointers.
> 不同进程中间　shared memory 的起始位置不同，采用相对偏移。



## 问题
1. 如果shared memory 的id 被泄露出去，那么岂不是通信内容也就被泄露了，还是其中都是加密的
2. binder 对于 shared memory 的改进是什么



## 233

```
𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓀈𓀉𓀊𓀋𓀌𓀍𓀎𓀏𓀐𓀑𓀒𓀓𓀔𓀕𓀖𓀗𓀘𓀙𓀚𓀛𓀜𓀝𓀞𓀟𓀠𓀡𓀢𓀣𓀤𓀥𓀦𓀧𓀨𓀩𓀪𓀫𓀬𓀻𓀼𓀽𓀾𓀿𓁀𓁁𓁂𓁃
```


