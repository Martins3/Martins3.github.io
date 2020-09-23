# Linux Programming Interface: Threads : Introduction

# 29 Threads:Introduction
## 29.1 Overview
1. disadvantage of fork()
    1. It is difficult to share information between processes.
    2. Process creation with fork() is relatively expensive.
2. the attributes threads shared
    1. process ID and parent process ID;
    2. process group ID and session ID;
controlling terminal;
process credentials (user and group IDs);
open file descriptors;
record locks created using fcntl();
signal disposition
file system–related information: umask, current working directory, and root directory;
interval timers (setitimer()) and POSIX timers (timer_create());
System V semaphore undo (semadj) values (Section 47.8);
resource limits;
CPU time consumed (as returned by times());
resources consumed (as returned by getrusage()); and
nice value (set by setpriority() and nice()).

3. the attributes threads don't shared
thread ID (Section 29.5);
signal mask;
thread-specific data (Section 31.3);
alternate signal stack (sigaltstack());
the errno variable;
floating-point environment (see fenv(3));
realtime scheduling policy and priority (Sections 35.2 and 35.3);
CPU affinity (Linux-specific, described in Section 35.4);
capabilities (Linux-specific, described in Chapter 39); and
stack (local variables and function call linkage information).
## 29.2 Background Details of the Pthreads API
1. threads and errno
2. return value from Pthreads functions

## 29.3 Thread Creation
```c
#include <pthread.h>
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,void *(*start)(void *), void *arg);
// Returns 0 on success, or a positive error number on error
```
## 29.4 Thread Termination
The thread’s start function performs a return specifying a return value for the
thread.
z The thread calls pthread_exit() (described below).
z The thread is canceled using pthread_cancel() (described in Section 32.1).
z Any of the threads calls exit(), or the main thread performs a return (in the
main() function), which causes all threads in the process to terminate immediately.

## 29.5 Thread IDs
## 29.6 Joining with a Terminated Thread
1. Calling pthread_join() for a thread ID that has been previously joined can lead
to unpredictable behavior

## 29.7 Detaching a Thread
By default, a thread is joinable, meaning that when it terminates, another thread
can obtain its return status using `pthread_join()`. Sometimes, we don’t care about
the thread’s return status; we simply want the system to automatically clean up and
remove the thread when it terminates. In this case, we can mark the thread as detached,
by making a call to `pthread_detach()` specifying the thread’s identifier in thread.

Detaching a thread doesn’t make it immune to a call to `exit()` in another thread
or a return in the main thread. In such an event, all threads in the process are immediately terminated,
regardless of whether they are joinable or detached. To put
things another way, `pthread_detach()` simply controls what happens after a thread
terminates, not how or when it terminates.

## 29.8 Thread Attributes

## 29.9 Threads Versus Processes
In this section, we briefly consider some of the factors that might influence our
choice of whether to implement an application as a group of threads or as a group
of processes. We begin by considering the advantages of a multithreaded approach:
- Sharing data between threads is easy. By contrast, sharing data between processes requires more work (e.g., creating a shared memory segment or using a pipe).
- Thread creation is faster than process creation; context-switch time may be lower for threads than for processes.

Using threads can have some disadvantages compared to using processes:
－ When programming with threads, we need to ensure that the functions we call
are thread-safe or are called in a thread-safe manner. (We describe the concept
of thread safety in Section 31.1.) Multiprocess applications don’t need to be
concerned with this.
－ A bug in one thread (e.g., modifying memory via an incorrect pointer) can damage all of the threads in the process, since they share the same address space and
other attributes. By contrast, processes are more isolated from one another.
－ Each thread is competing for use of the finite virtual address space of the host
process. In particular, each thread’s stack and thread-specific data (or threadlocal storage) consumes a part of the process virtual address space, which is
consequently unavailable for other threads. Although the available virtual
address space is large (e.g., typically 3 GB on x86-32), this factor may be a significant limitation for processes employing large numbers of threads or
threads that require large amounts of memory. By contrast, separate processes
can each employ the full range of available virtual memory (subject to the limitations of RAM and swap space).
> 坏处，缺乏隔离，最后的虚拟地址空间的那个根本不值一提

The following are some other points that may influence our choice of threads versus processes:
- Dealing with signals in a multithreaded application requires careful design. (As
a general principle, it is usually desirable to avoid the use of signals in multithreaded programs.) We say more about threads and signals in Section 33.2.
- In a multithreaded application, all threads must be running the same program
(although perhaps in different functions). In a multiprocess application, different processes can run different programs.
- Aside from data, threads also share certain other information (e.g., file descriptors, signal dispositions, current working directory, and user and group IDs). This may be an advantage or a disadvantage, depending on the application.

## 29.10 Summary
Threads are created using `pthread_create()`. Each thread can then independently
terminate using `pthread_exit()`. (If any thread calls `exit()`, then all threads immediately terminate.) Unless a thread has been marked as detached (e.g., via a call to
`pthread_detach()`), it must be joined by another thread using `pthread_join()`, which
returns the termination status of the joined thread.

