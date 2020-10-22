## 待分析的资料
**[namespace with c code](https://windsock.io/uts-namespace/)**

The UTS namespace is used to isolate two specific elements of the system that relate to the uname system call

In short, the UTS namespace is about isolating hostnames.

So, how do you place processes into namespaces?
This is achieved with three different system calls: *clone*, *unshare* and *setns*.

* Man clone

       clone() creates a new process, in a manner similar to fork(2).

       This page describes both the glibc clone() wrapper function and the underlying system call on which it is based.  The main text describes the wrapper function; the differences for the raw system call are described toward the end of this page.

       Unlike fork(2), clone() allows the child process to share parts of its execution context with the calling process, such as the virtual address space, the table of file descriptors, and the table of signal handlers.  (Note that on this manual page, "calling process" normally corresponds to "parent process".  But see the description of CLONE_PARENT below.)

       One use of clone() is to implement threads: multiple flows of control in a program that run concurrently in a shared address space.

* Man unshare

       unshare() allows a process (or thread) to disassociate parts of its execution context that are currently being shared with other processes (or threads).  Part of the execution context, such as the mount namespace, is shared implicitly when a new process is created using fork(2) or vfork(2), while other parts, such as virtual memory, may be shared by explicit request when creating a process or thread using clone(2).

       The main use of unshare() is to allow a process to control its shared execution context without creating a new process.


* Man setns

        So, how do you place processes into namespaces? This is achieved with three different system calls: clone, unshare and setns.

The files are special symlinks bearing the namespace type (e.g. mnt), which point to inodes which represent unique namespaces, which the kernel reports as a conjugation of the namespace type and the inode number. Hence, if two different processes exist in the same namespace, their symlinks will point to the same inodes.

* https://en.wikipedia.org/wiki/Cgroups

* https://github.com/p8952/bocker 一百行实现 docker

## 问题
2. copy_namespace, 那么销毁一个 namespace 的时间点


## general
The namespace API consists of three system calls—clone(), unshare(), and setns()—and a number of /proc files. [^2]
```
ls -l /proc/$$/ns
```
两种 让当前 fs 不被销毁的方法 : 
- The /proc/PID/ns symbolic links also serve other purposes. If we open one of these files, then the namespace will continue to exist as long as the file descriptor remains open, even if all processes in the namespace terminate.
- The same effect can also be obtained by bind mounting one of the symbolic links to another location in the file system:

proc_ns_operation

```c
struct proc_ns_operations {
	const char *name;
	const char *real_ns_name;
	int type;
	struct ns_common *(*get)(struct task_struct *task);
	void (*put)(struct ns_common *ns);
	int (*install)(struct nsset *nsset, struct ns_common *ns);
	struct user_namespace *(*owner)(struct ns_common *ns);
	struct ns_common *(*get_parent)(struct ns_common *ns);
} __randomize_layout;
```



## mount


## UTS : hostname &&  domainname

https://unix.stackexchange.com/questions/183717/whats-a-uts-namespace
https://superuser.com/questions/59093/difference-between-host-name-and-domain-name


## pid
1. namespace 里面的 pid = 1 的那个如果挂掉了，那么剩下的 process 需要挂到哪里 ?
    - 全部 terminate [^3]
2. pid 的父子关系 在不同的 namespace 之间如何维持 ?
    - 在外层的 parent 挂掉之后，会将 children 放到内层的 init 中间

作用[^1]:
pid namespace isolate the process ID number space. 
In other words, processes in different PID namespaces can have the same PID. 
1. One of the main benefits of PID namespaces is that containers can be migrated between hosts while keeping the same process IDs for the processes inside the container. 
2. PID namespaces also allow each container to have its own init (PID 1), the "ancestor of all processes" that manages various system initialization tasks and reaps orphaned child processes when they terminate.

结构[^1]:
1. From the point of view of a particular PID namespace instance, a process has two PIDs: the PID inside the namespace, and the PID outside the namespace on the host system.
2. PID namespaces can be nested: a process will have one PID for each of the layers of the hierarchy starting from the PID namespace in which it resides through to the root PID namespace. 
3. A process can see (e.g., view via /proc/PID and send signals with kill()) only processes contained in its own PID namespace and the namespaces nested below that PID namespace.

unshare 和 setns 在处理 pid 不会把 caller 放到该 namespace 中间，原因是
这样就改变了 caller 的 pid，而很多程序是依赖于 pid 是 constant 的假设. [^3]

**TODO**:
1. task_struct 的 pid tgid thread_id 的。。。。。

https://stackoverflow.com/questions/26779416/what-is-the-relation-between-task-struct-and-pid-namespace

If you trace the steps needed to create a new process, you can find the relationship(s) between the `task_struct`, `struct nsproxyand`, `struct pid`.


## ipc


## net
能力:
provide isolation of the system resources associated with networking. Thus, each network namespace has its own network devices, IP addresses, IP routing tables, /proc/net directory, port numbers, and so on.[^1]

## user

## time

[^1]: https://lwn.net/Articles/531114/
[^2]: https://lwn.net/Articles/531381/
[^3]: https://lwn.net/Articles/532748/
