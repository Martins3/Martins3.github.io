# 进程管理

<!-- vim-markdown-toc GitLab -->

* [introduction](#introduction)
* [task_struct](#task_struct)
* [sched class and policy](#sched-class-and-policy)
* [runqueue](#runqueue)
* [task_group](#task_group)
* [rt](#rt)
* [schedule](#schedule)
* [load](#load)
* [process state](#process-state)
* [process relation](#process-relation)
* [vfork](#vfork)
* [clone](#clone)
* [signal](#signal)
    * [signal fork](#signal-fork)
    * [send signal](#send-signal)
    * [do signal](#do-signal)
* [idle](#idle)
    * [ptrace](#ptrace)
* [waitqueue](#waitqueue)
* [design](#design)
* [fork](#fork)
    * [copy_process](#copy_process)
    * [copy_thread](#copy_thread)
    * [stack's copy](#stacks-copy)
* [stack](#stack)
    * [x86 stack](#x86-stack)
* [pidfd](#pidfd)
* [pid](#pid)
* [green thread](#green-thread)
* [cpp thread keyword](#cpp-thread-keyword)
* [cpu](#cpu)
* [syscall](#syscall)
    * [x86 syscall(merge)](#x86-syscallmerge)
    * [syscall int](#syscall-int)
    * [syscall vsyscall](#syscall-vsyscall)
    * [syscall getcpu](#syscall-getcpu)
* [daemon](#daemon)
* [user group](#user-group)
* [smp](#smp)
* [`__schedule`](#__schedule)
* [kthread](#kthread)
* [first user process](#first-user-process)
* [TODO](#todo)
* [process group](#process-group)
* [sched_class ops](#sched_class-ops)
    * [enqueue](#enqueue)
* [runtime vruntime](#runtime-vruntime)
* [cfs](#cfs)
* [pt_regs](#pt_regs)
* [ipc](#ipc)
* [zombie 和 orphan](#zombie-和-orphan)

<!-- vim-markdown-toc -->

用户需要怎样执行程序: (调查一下所有相关的 syscall 都是什么东西)
1. 容器: cgroup 和 namespace
2. 多用户 : 调度器
    1. 不用的场景存在不同的 exec
3. 启动 : fork
    1. 第一个进程启动 : ?
    2. 启动第一个用户进程 : ?
    3. Posix 规定的 25个 flags
4. 执行 : exec
4. 暂停 : yield
5. 死亡 : kill exit
6. 交流 : signal


进入调度器之前 :
1. exit.c
2. pidfd
3. pidnamespace : 据说 namespace 下，可以没有 pid = 1
.....

- [ ] How to make so many policy work together ?
    - bandwidth :
    - cgroup
    - every process should run in the period
    - different scheduler : dead rt
    - different policy

**https://github.com/gatieme/LDD-LinuxDeviceDrivers/tree/master/study/kernel/01-process**

## introduction

| 主要内容       | 一句话说明要点                                                                                                     | 涉及的文件 | 关键的函数          |
|----------------|--------------------------------------------------------------------------------------------------------------------|------------|---------------------|
| 进程的层级关系 | thread group, process group 和 session, 逐级包含的关系，其中 thread group 用于pthread 线程的实现，后者主要用于终端 | fork.c     | fork.c:copy_process |
| 创建进程       | clone fork, vfork 全部汇集到 `_do_fork`, 其参数决定了复制的内容                                                    | fork.c     |
| 销毁进程       |
| 进程通信       | IPC 和 signal :
| 进程调度       |
| 进程地址空间   | brk


## task_struct

[^8]: task_struct's fields related with sched
```c
struct task_struct {
    /* ... */

    /* 进程状态 */
    volatile long			state;

    /* 调度优先级相关，策略相关 */
	int				prio;
	int				static_prio;
	int				normal_prio;
	unsigned int			rt_priority;
  unsigned int			policy;

  /* 调度类，调度实体相关，任务组相关等 */
  const struct sched_class	*sched_class;
	struct sched_entity		se;
	struct sched_rt_entity		rt;
#ifdef CONFIG_CGROUP_SCHED
	struct task_group		*sched_task_group;
#endif
	struct sched_dl_entity		dl;

  /* 进程之间的关系相关 */
  /* Real parent process: */
	struct task_struct __rcu	*real_parent;

	/* Recipient of SIGCHLD, wait4() reports: */
	struct task_struct __rcu	*parent;

	/*
	 * Children/sibling form the list of natural children:
	 */
	struct list_head		children;
	struct list_head		sibling;
	struct task_struct		*group_leader;

    /* ... */
}
```

- [ ] task_struct::flags
  - so many yummy flags to explore

```c
/*
 * Per process flags
 */
#define PF_VCPU			0x00000001	/* I'm a virtual CPU */
#define PF_IDLE			0x00000002	/* I am an IDLE thread */
#define PF_EXITING		0x00000004	/* Getting shut down */
#define PF_IO_WORKER		0x00000010	/* Task is an IO worker */
#define PF_WQ_WORKER		0x00000020	/* I'm a workqueue worker */
#define PF_FORKNOEXEC		0x00000040	/* Forked but didn't exec */
#define PF_MCE_PROCESS		0x00000080      /* Process policy on mce errors */
#define PF_SUPERPRIV		0x00000100	/* Used super-user privileges */
#define PF_DUMPCORE		0x00000200	/* Dumped core */
#define PF_SIGNALED		0x00000400	/* Killed by a signal */
#define PF_MEMALLOC		0x00000800	/* Allocating memory */
#define PF_NPROC_EXCEEDED	0x00001000	/* set_user() noticed that RLIMIT_NPROC was exceeded */
#define PF_USED_MATH		0x00002000	/* If unset the fpu must be initialized before use */
#define PF_USED_ASYNC		0x00004000	/* Used async_schedule*(), used by module init */
#define PF_NOFREEZE		0x00008000	/* This thread should not be frozen */
#define PF_FROZEN		0x00010000	/* Frozen for system suspend */
#define PF_KSWAPD		0x00020000	/* I am kswapd */
#define PF_MEMALLOC_NOFS	0x00040000	/* All allocation requests will inherit GFP_NOFS */
#define PF_MEMALLOC_NOIO	0x00080000	/* All allocation requests will inherit GFP_NOIO */
#define PF_LOCAL_THROTTLE	0x00100000	/* Throttle writes only against the bdi I write to, * I am cleaning dirty pages from some other bdi. */
#define PF_KTHREAD		0x00200000	/* I am a kernel thread */
#define PF_RANDOMIZE		0x00400000	/* Randomize virtual address space */
#define PF_SWAPWRITE		0x00800000	/* Allowed to write to swap */
#define PF_NO_SETAFFINITY	0x04000000	/* Userland is not allowed to meddle with cpus_mask */
#define PF_MCE_EARLY		0x08000000      /* Early kill for mce process policy */
#define PF_MEMALLOC_NOCMA	0x10000000	/* All allocation request will have _GFP_MOVABLE cleared */
#define PF_FREEZER_SKIP		0x40000000	/* Freezer should not count it as freezable */
#define PF_SUSPEND_TASK		0x80000000      /* This thread called freeze_processes() and should not be frozen */
```



## sched class and policy
[Fixing SCHED_IDLE](https://lwn.net/Articles/805317/)

I only skim one or two paragraph of it, there is some notes:
> The CFS (completely fair scheduling) class hosts most of the user tasks; it implements three scheduling policies: SCHED_NORMAL, SCHED_BATCH, and SCHED_IDLE. A task under any of these policies gets a chance to run only if no other tasks are enqueued in the deadline or realtime classes (though by default the scheduler reserves 5% of the CPU for CFS tasks regardless). The scheduler tracks the virtual runtime (vruntime) for all tasks, runnable and blocked. The lower a task's vruntime, the more deserving the task is for time on the processor. CFS accordingly moves low-vruntime tasks toward the front of the scheduling queue.
>
> The priority of a task is calculated by adding 120 to its nice value, which ranges from -20 to +19. The priority of the task is used to set the weight of the task, which in turn affects the vruntime of the task; the lower the nice value, the higher the priority. The task's weight will thus be higher, and its vruntime will increase more slowly as the task runs.
>
> The SCHED_NORMAL policy (called SCHED_OTHER in user space) is used for most of the tasks that run in a Linux environment, like the shell. The SCHED_BATCH policy is used for batch processing by non-interactive tasks — tasks that should run uninterrupted for a period of time and hence are normally scheduled only after finishing all the SCHED_NORMAL activity. The SCHED_IDLE policy is designed for the lowest-priority tasks in the system; these tasks get a chance to run only if there is nothing else to run. Though, in practice, even in the presence of other SCHED_NORMAL tasks a SCHED_IDLE task will get some time to run (around 1.4% for a task with a nice value of zero). This policy isn't widely used currently and efforts are being made to improve how it works.



## runqueue
![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200201170629353-2136884130.png)

```c
/*
 * This is the main, per-CPU runqueue data structure.
 *
 * Locking rule: those places that want to lock multiple runqueues
 * (such as the load balancing or the thread migration code), lock
 * acquire operations must be ordered by ascending &runqueue.
 */
struct rq {
	/* runqueue lock: */
	raw_spinlock_t lock;

	/*
	 * nr_running and cpu_load should be in the same cacheline because
	 * remote CPUs use both these fields when doing load calculation.
	 */
	unsigned int nr_running;

    /* 三个调度队列：CFS调度，RT调度，DL调度 */
	struct cfs_rq cfs;
	struct rt_rq rt;
	struct dl_rq dl;

    /* stop指向迁移内核线程， idle指向空闲内核线程 */
    struct task_struct *curr, *idle, *stop;

    /* ... */
}
```

## task_group
![](https://img2020.cnblogs.com/blog/1771657/202003/1771657-20200310214009477-225815245.png)
- task_group会为每个CPU再维护一个cfs_rq，这个cfs_rq用于组织挂在这个任务组上的任务以及子任务组，参考图中的Group A；
- 调度器在调度的时候，比如调用`pick_next_task_fair`时，会从遍历队列，选择sched_entity，如果发现sched_entity对应的是task_group，则会继续往下选择；
- 由于sched_entity结构中存在parent指针，指向它的父结构，因此，系统的运行也能从下而上的进行遍历操作，通常使用函数walk_tg_tree_from进行遍历；

- [ ] please notice that, when CONFIG_CFS_BADNWIDTH turned off, walk_tg_tree_from's users are disapeared, so why `bandwidth` need `walk_tg_tree_from` ?

- [ ] we create a `cfs_rq` for every group, verfiy it
- [ ] what kinds of process will be added to same process group ?


```c
/* task group related information */
struct task_group {
    /* ... */

    /* 为每个CPU都分配一个CFS调度实体和CFS运行队列 */
#ifdef CONFIG_FAIR_GROUP_SCHED
	/* schedulable entities of this group on each cpu */
	struct sched_entity **se;
	/* runqueue "owned" by this group on each cpu */
	struct cfs_rq **cfs_rq;
	unsigned long shares;
#endif

    /* 为每个CPU都分配一个RT调度实体和RT运行队列 */
#ifdef CONFIG_RT_GROUP_SCHED
	struct sched_rt_entity **rt_se;
	struct rt_rq **rt_rq;

	struct rt_bandwidth rt_bandwidth;
#endif

    /* task_group之间的组织关系 */
	struct rcu_head rcu;
	struct list_head list;

	struct task_group *parent;
	struct list_head siblings;
	struct list_head children;

    /* ... */
};
```


## rt

- [ ] [LoyenWang](https://www.cnblogs.com/LoyenWang/p/12584345.html)

## schedule
- [ ] https://oskernellab.com/

notes from [^8]:

1. 主动调度 - schedule()

![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200201170715768-1838632136.png)

2. 周期调度 - schedule_tick()
    - 时钟中断处理程序中，调用schedule_tick()函数；
    - 时钟中断是调度器的脉搏，内核依靠周期性的时钟来处理器CPU的控制权；
    - 时钟中断处理程序，检查当前进程的执行时间是否超额，如果超额则设置重新调度标志(_TIF_NEED_RESCHED)；
    - 时钟中断处理函数返回时，被中断的进程如果在用户模式下运行，需要检查是否有重新调度标志，设置了则调用schedule()调度；

![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200201170736505-296291185.png)

3. hrtick()
![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200201170755832-1551335560.png)


4. wake_up_process()
![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200201170804248-1658303951.png)
唤醒进程时调用wake_up_process()函数，被唤醒的进程可能抢占当前的进程；

- [ ] what's relation with hrtick and schedule_tick ?

## load
  - [ ] calc_group_shares

## process state
```c
/*
 * Task state bitmask. NOTE! These bits are also
 * encoded in fs/proc/array.c: get_task_state().
 *
 * We have two separate sets of flags: task->state
 * is about runnability, while task->exit_state are
 * about the task exiting. Confusing, but this way
 * modifying one set can't modify the other one by
 * mistake.
 */

/* Used in tsk->state: */
#define TASK_RUNNING			0x0000
#define TASK_INTERRUPTIBLE		0x0001
#define TASK_UNINTERRUPTIBLE		0x0002
#define __TASK_STOPPED			0x0004
#define __TASK_TRACED			0x0008
/* Used in tsk->exit_state: */
#define EXIT_DEAD			0x0010
#define EXIT_ZOMBIE			0x0020
#define EXIT_TRACE			(EXIT_ZOMBIE | EXIT_DEAD)
/* Used in tsk->state again: */
#define TASK_PARKED			0x0040
#define TASK_DEAD			0x0080
#define TASK_WAKEKILL			0x0100
#define TASK_WAKING			0x0200
#define TASK_NOLOAD			0x0400
#define TASK_NEW			0x0800
#define TASK_STATE_MAX			0x1000

/* Convenience macros for the sake of set_current_state: */
#define TASK_KILLABLE			(TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED			(TASK_WAKEKILL | __TASK_STOPPED)
#define TASK_TRACED			(TASK_WAKEKILL | __TASK_TRACED)

#define TASK_IDLE			(TASK_UNINTERRUPTIBLE | TASK_NOLOAD)

/* Convenience macros for the sake of wake_up(): */
#define TASK_NORMAL			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)

/* get_task_state(): */
#define TASK_REPORT			(TASK_RUNNING | TASK_INTERRUPTIBLE | \
					 TASK_UNINTERRUPTIBLE | __TASK_STOPPED | \
					 __TASK_TRACED | EXIT_DEAD | EXIT_ZOMBIE | \
					 TASK_PARKED)

#define task_is_traced(task)		((task->state & __TASK_TRACED) != 0)

#define task_is_stopped(task)		((task->state & __TASK_STOPPED) != 0)

#define task_is_stopped_or_traced(task)	((task->state & (__TASK_STOPPED | __TASK_TRACED)) != 0)
```

![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200201170358218-1930669459.png)

- [x] what's difference with `TASK_UNINTERRUPTIBLE` and `TASK_INTERRUPTIBLE` ?
  - https://stackoverflow.com/questions/223644/what-is-an-uninterruptible-process
  - https://lwn.net/Articles/288056/

> A process which is placed in the TASK_INTERRUPTIBLE state will sleep until either (1) something explicitly wakes it up, or (2) a non-masked signal is received. The TASK_UNINTERRUPTIBLE state, instead, ignores signals; processes in that state will require an explicit wakeup before they can run again.

> so Matthew created a new sleeping state, called TASK_KILLABLE; it behaves like TASK_UNINTERRUPTIBLE with the exception that fatal signals will interrupt the sleep.

- [ ] find a example to understand the difference between `TASK_UNINTERRUPTIBLE` and `TASK_INTERRUPTIBLE`
  - [ ] Not all syscall will lead to `TASK_UNINTERRUPTIBLE`, find a example to lead to `TASK_INTERRUPTIBLE` and it has to do that.


## process relation
- [ ] zombie
- [ ] orphan


- [ ] check code in kernel/exit.c
- [ ] why we need parent and real_parent fields in task_struct ?

## vfork
Currently, there are four interface to fork:
1. fork
2. vfork
3. clone
4. clone3

- [clone3 example code] https://lkml.org/lkml/2019/10/25/184


> CLONE_VFORK (since Linux 2.2)
>   If CLONE_VFORK is set, the execution of the calling process is suspended until the child releases its virtual memory resources via a call to execve(2) or `_exit`(2) (as with vfork(2)).
>
>   If CLONE_VFORK is not set, then both the calling process and the child are schedulable after the call, and an application should not rely on execution occurring in any particular order.

- [ ] If have time, check how child releases it's virtual memory

## clone
Man clone(2)
> By contrast with fork(2), these system calls provide more precise control over what pieces of execution context are shared between the calling process and the child process.
> For example, using these system calls, the caller can control whether or not the two processes share the virtual address space, the table of file descriptors, and the table of signal handlers.
> These system calls also allow the new child process to be placed in separate namespaces(7).


- [ ] Why we need a new clone3

- [ ] How to specify a valid stack ?
    - [ ] What if the stack is too small and overflow easily ?

- [ ] CLONE_PARENT && CLONE_THREAD

> Man clone(2)
>
> section CLONE_THREAD
>
> If `CLONE_THREAD` is set, the child is placed in the same thread group as the calling process.
>
> A new thread created with CLONE_THREAD has the same parent process as the process that made the clone call (i.e., like CLONE_PARENT), so that calls to getppid(2) return the same value for all of the threads in a thread group.  When a CLONE_THREAD thread terminates, the thread that created it is not sent a SIGCHLD (or other termination) signal; nor can the status of such a thread be obtained using wait(2).  (The thread is said to be detached.)
>
> After **all** of the threads in a thread group **terminate** the parent process of the thread group is sent a SIGCHLD (or other termination) signal.
>
> If any of the threads in a thread group performs an execve(2), then all threads other than the thread group leader are terminated, and the new program is executed in the thread group leader.
>
> If one of the threads in a thread group creates a child using fork(2), then any thread in the group can wait(2) for that child.
>
> Since Linux 2.5.35, the flags mask must also include CLONE_SIGHAND if CLONE_THREAD is specified (and note that, since Linux 2.6.0, CLONE_SIGHAND also requires CLONE_VM to be included).
>
> Signal dispositions and actions are process-wide: if an unhandled signal is delivered to a thread, then it will affect (terminate, stop, continue, be ignored in) all members of the thread group.
>
> Each thread has its own signal mask, as set by sigprocmask(2).
>
> A signal may be process-directed or thread-directed.  A process-directed signal is targeted at a thread group (i.e., a TGID), and is delivered to an arbitrarily selected thread from among those that are not blocking the signal.  A signal may be process-directed because it was generated by the kernel for reasons other than a hardware exception, or because it was sent using kill(2) or sigqueue(3).  A thread-directed signal is targeted at (i.e., delivered to) a specific thread.  A signal may be thread directed because it was sent using tgkill(2) or pthread_sigqueue(3), or because the thread executed a machine language instruction that triggered a hardware exception (e.g., invalid memory access triggering SIGSEGV or a floating-point exception triggering SIGFPE).
>
> A call to sigpending(2) returns a signal set that is the union of the pending process-directed signals and the signals that are pending for the calling thread.
>
> If a process-directed signal is delivered to a thread group, and the thread group has installed a handler for the signal, then the handler will be invoked in exactly one, arbitrarily selected member of the thread group that has not blocked the signal.  If multiple threads in a group are waiting to accept the same signal using sigwaitinfo(2), the kernel will arbitrarily select one of these threads to receive the signal.

| function | id   |
|----------|------|
| getpid   | TGID |
| gettid   | TID  |


- [ ] CLONE_THREAD must take CLONE_SIGHAND and CLONE_VM with it

- [ ] All thread in thread group share a single
```c
	struct signal_struct		*signal;
```

## signal
- https://questions.wizardzines.com/signals.html : nice post create by Evans, correct my understanding of process

- [ ] https://www.giovannimascellani.eu/having-fun-with-signal-handlers.htmlo
  - 实际上，可以使用 sigaction 来处理 seg fault
  - [ ] ucontext ?
  - [ ] 文章并没有看!

- [ ] A signal comes when I'm sleeping ?
  - [ ] Is there a mechanism to run action handler immediately even the process is sleeping

- [ ] Why everyone has to check signal_pending ?
```
➜  linux git:(master) ag signal_pending |wc -l
741
```

1. 还是先把信号的机制的 tlpi 看完看懂，写代码，做习题完成在看吧，其实挺重要的
2. fork 是如何处理 signal 共享的 ?

为什么需要信号机制:
1. 提供给用户操作进程的状态的接口，kill 等
2. 内核之间的通信
2. debug

需要的分析的细节:
2. generic_file_buffered_read 中间会调用 fatal_signal_pending，那么各种检查函数的要求是什么 ?

Man 分析的很清楚[^2] :
1. 发送
2. 接收
3. 屏蔽或者延迟信号的执行

- 当程序被 interrupt，或者 sleep 到来的信号，如何处理 ?
    1. sleep 会被唤醒, 参考 complete_signal 中的 signal_wake_up
    2. interrupt : 完成 interrupt 或者 exception 的时候

用户的 handler 如何注册的 ?

- [ ] 执行完成 handler 是在用户态 还是 内核态，如果是内核态，没有安全，如果用户态，这个上下文如何构造 ?
- [x] 当用户的 handler 执行完成之后，用户靠什么回到内核，此外，用户程序要是拒绝回到内核怎么办
  - [ ] 好吧，CNM, 居然在 x86/kernel/signal 中间

- [ ] find the caller of `signal_pending` ?

实际上的代码在 kernel/entry/
irqentry_exit_to_user_mode 和 syscall_exit_to_user_mode

#### signal fork
- [ ] how exit_signal registered to the newborn process
```c
	struct kernel_clone_args args = {
		.flags		= (clone_flags & ~CSIGNAL),
		.pidfd		= parent_tidptr,
		.child_tid	= child_tidptr,
		.parent_tid	= parent_tidptr,
		.exit_signal	= (clone_flags & CSIGNAL),
		.stack		= newsp,
		.tls		= tls,
	};
```

> Man vfork(2)
>
> A call to vfork() is equivalent to calling clone(2) with flags specified as:
>
>     CLONE_VM | CLONE_VFORK | SIGCHLD
>
> Man clone(2)
>
> If this signal is specified as anything other than SIGCHLD, then the parent process must specify the `__WALL` or `__WCLONE` options when waiting for the child with wait(2).  If no signal (i.e., zero) is specified, then the parent process is not signaled when the child terminates.

- signal 在 copy_process 的時候，使用兩個 CLONE_SIGHAND 和 CLONE_THREAD 來控制 signal 和 sighand 的拷貝

- [ ] 似乎一個 thread group 的普通成員(不是 thread group leader), signal_struct

```c
	/* Signal handlers: */
	struct signal_struct		*signal;
	struct sighand_struct __rcu		*sighand;
	sigset_t			blocked;
	sigset_t			real_blocked;
	/* Restored if set_restore_sigmask() was used: */
	sigset_t			saved_sigmask;
	struct sigpending		pending;
	unsigned long			sas_ss_sp;
	size_t				sas_ss_size;
	unsigned int			sas_ss_flags;
```


#### send signal
没想到吧，这其实发送 signal 的

do_send_sig_info : 万恶之源

`__send_signal`

#### do signal
exit_to_usermode_loop => do_signal() => handle_signal()

![](http://liujunming.top/images/2018/12/76.png)

- [x] sigreturn rt_sigreturn

- https://man7.org/linux/man-pages/man2/sigreturn.2.html
- https://stackoverflow.com/questions/31267825/rt-sigreturn-and-linux-kernels



## idle
1. fork_idle 的含义 ?
    - copy_process 对于 pid=0 的判断


#### ptrace
https://blog.0x972.info/?d=2014/11/13/10/40/50-how-does-a-debugger-work

https://github.com/x64dbg/x64dbg

http://longwei.github.io/How-Debuger-Works/


`_do_fork` 最后的 ptrace 部分:

```c
	/* forking complete and child started to run, tell ptracer */
	if (unlikely(trace))
		ptrace_event_pid(trace, pid);

	if (clone_flags & CLONE_VFORK) {
		if (!wait_for_vfork_done(p, &vfork))
			ptrace_event_pid(PTRACE_EVENT_VFORK_DONE, pid);
	}
```
> fork 工作完成之后，通知一下 tracer

signal.c :
```
ptrace_notify
  ptrace_stop
    do_notify_parent_cldstop
```

## waitqueue
1. https://github.com/cirosantilli/linux-kernel-module-cheat/blob/master/kernel_modules/wait_queue.c : 还是要好好分析一下其中的代码呀 !



do_wait() : 每个 `task_struct->signal->wait_chldexit` 上放置 wait queue
```c
  // TODO child_wait_callback 函数调用的时机 : 元素加入 还是 元素离开
  // child_wait_callback 会唤醒 current
	init_waitqueue_func_entry(&wo->child_wait, child_wait_callback);
	wo->child_wait.private = current; // 用于唤醒
	add_wait_queue(&current->signal->wait_chldexit, &wo->child_wait);

 // 最终去掉，如果捕获了多个 thread
 remove_wait_queue
```

wake up : do_notify_parent_cldstop 和 do_notify_parent


## design
1. 阅读一个那个 fork paper

如何创建第一个线程进程之类的问题理解了可以加深对于操作系统的代码的理解，关键的是:

1. 其实我对于 exec 的设计非常不满 ?
    1. 当使用 exec 的时候，总是 fork 然后 exec 的
    2. 那么，那么为什么不去创建一个新的 syscall 直接创建一个
        1. 为了维护 parent 和 child 的关系 ?

## fork
1. posix spawn 到底是靠什么实现的, 比如这里有一个用户库: https://github.com/rtomayko/posix-spawn
    1. 安装 : https://stackoverflow.com/questions/10412684/how-to-compile-my-own-glibc-c-standard-library-from-source-and-use-it
    2. 检查代码 : sysdeps/posix/spawni.c 中间的 `__spawni` 函数, 靠 vfork 维持生活
2. https://lwn.net/Articles/360556/ 说明了各种 fork + exec 模式


check 一下这个阅读:
https://stackoverflow.com/questions/4856255/the-difference-between-fork-vfork-exec-and-clone?rq=1

- [ ] how child process return to userspace, in another word, how does kernel set child process's return code ?


#### copy_process
fork ==============> copy_process ==> copy_thread
|                        |                  |
|                        |                  |
完成基本的工作，        各種拷貝         task_struct::stack 的拷貝
將 process 放入到
調度器中間

#### copy_thread
Apparently, it's architecture related.

- [x] are stack of kernel thread and kernel stack of user prcess different ?
    - [x] In another word, does pt_regs lay on the top of kernel thread stack ? YES

```c
	/* Kernel thread ? */
	if (unlikely(p->flags & PF_KTHREAD)) {
		memset(childregs, 0, sizeof(struct pt_regs)); // if kernel thread doesn't have pt_regs, why should it memset it.
		kthread_frame_init(frame, sp, arg);
		return 0;
	}
```

- [ ] so, there's mechanism for lzay copy fpu registers ?

#### stack's copy
- [ ] 两个 thread 共享地址空间，如何防止 stack 互相覆盖 ?


- [ ] 更多关于 pt_regs 的作用，应该是可以作为 syscall 的保存，似乎也可以作为 ptrace ? please read it and search it
https://stackoverflow.com/questions/52765688/in-linux-where-is-user-mode-stack-stored
https://stackoverflow.com/questions/33104091/how-are-system-calls-stored-in-pt-regs

- [ ] task_pt_regs 的原理
- [ ] task_stack_page : 才发现 `task->stack` 是用于指向 stack 的指针，这个变量的 ref 并不多，好好 check 一下.
  - [ ] fork 的时候，如何从 parent 的拷贝 stack 的
  - [ ] fork 的返回值为什么是两个的原因定位一下代码
- [ ] CLONE_THREAD 的時候，如何拷貝 stack 的 ?
  - [ ] 用戶態的 stack 不用拷貝，但是內核態的需要
  - [ ] 內核態的 stack 拷貝似乎和 CLONE_THREAD 其實沒有關系的
  - [ ] 如何使用 pthread 創建多個 thread, /proc/maps 會出現多個 stack 嗎 ？

> Man clone(2)
>
> The stack for the child process is specified via cl_args.stack, which points to the lowest byte of the stack area, and cl_args.stack_size, which specifies the size of the stack in bytes.  In the case where the CLONE_VM flag (see below) is specified, a stack must be explicitly allocated and specified.  Otherwise, these two fields can be specified as NULL and 0, which causes the child to use the same stack area as the parent (in the child's own virtual address space).

- [ ] verfiy what clone manual *above* say


## stack

- [ ] fork 的時候，如何分別設置內核和用戶態的 stack



notes from questions [^6]:

- [x] when is `task_stack` allocated and destroied ?

```c
static unsigned long *alloc_thread_stack_node(struct task_struct *tsk, int node)

static void release_task_stack(struct task_struct *tsk)
```
it's simple, allocate page and assign to `task->stack`

- [x] why `task_struct` has to access `stack` ?

because kernel put `task_pt_regs` on the stack

```c
static inline void *task_stack_page(const struct task_struct *task)
{
  return task->stack;
}


#ifndef current_pt_regs
#define current_pt_regs() task_pt_regs(current)
#endif

/*
 * unlike current_pt_regs(), this one is equal to task_pt_regs(current)
 * on *all* architectures; the only reason to have a per-arch definition
 * is optimisation.
 */
#ifndef signal_pt_regs
#define signal_pt_regs() task_pt_regs(current)
#endif

#define task_pt_regs(task) \
({									\
	unsigned long __ptr = (unsigned long)task_stack_page(task);	\
	__ptr += THREAD_SIZE - TOP_OF_KERNEL_STACK_PADDING;		\
	((struct pt_regs *)__ptr) - 1;					\
})
```

- [x] current_top_of_stack()

stored in the 'soft' TSS

```c
static inline unsigned long current_top_of_stack(void)
{
	/*
	 *  We can't read directly from tss.sp0: sp0 on x86_32 is special in
	 *  and around vm86 mode and sp0 on x86_64 is special because of the
	 *  entry trampoline.
	 */
	return this_cpu_read_stable(cpu_current_top_of_stack);
}

/*
 * per-CPU TSS segments. Threads are completely 'soft' on Linux,
 * no more per-task TSS's. The TSS size is kept cacheline-aligned
 * so they are allowed to end up in the .data..cacheline_aligned
 * section. Since TSS's are completely CPU-local, we want them
 * on exact cacheline boundaries, to eliminate cacheline ping-pong.
 */
__visible DEFINE_PER_CPU_PAGE_ALIGNED(struct tss_struct, cpu_tss_rw) = {
	.x86_tss = {
		/*
		 * .sp0 is only used when entering ring 0 from a lower
		 * privilege level.  Since the init task never runs anything
		 * but ring 0 code, there is no need for a valid value here.
		 * Poison it.
		 */
		.sp0 = (1UL << (BITS_PER_LONG-1)) + 1,

		/*
		 * .sp1 is cpu_current_top_of_stack.  The init task never
		 * runs user code, but cpu_current_top_of_stack should still
		 * be well defined before the first context switch.
		 */
		.sp1 = TOP_OF_INIT_STACK,

#ifdef CONFIG_X86_32
		.ss0 = __KERNEL_DS,
		.ss1 = __KERNEL_CS,
#endif
		.io_bitmap_base	= IO_BITMAP_OFFSET_INVALID,
	 },
};
```
check the code in the `__switch_to`
```c
	/*
	 * Switch the PDA and FPU contexts.
	 */
	this_cpu_write(current_task, next_p);
	this_cpu_write(cpu_current_top_of_stack, task_top_of_stack(next_p));
```

- [ ] so, what's difference between sp0 and sp1 ?


#### x86 stack
some x86 kernel notes:[^5]

*Switching to the kernel interrupt stack is done by software based on a per CPU interrupt nest counter. This is needed because x86-64 “IST” hardware stacks cannot nest without races.*

*x86_64 also has a feature which is not available on i386, the ability to automatically switch to a new stack for designated events such as double fault or NMI, which makes it easier to handle these unusual events on x86_64. This feature is called the Interrupt Stack Table (IST). There can be up to 7 IST entries per CPU. The IST code is an index into the Task State Segment (TSS). The IST entries in the TSS point to dedicated stacks; each stack can be a different size.*

- [ ] fine, too many difficult material related with **IDT**, **IST**, **TSS**
  - [ ] may review the article later

## pidfd
https://man7.org/linux/man-pages/man2/pidfd_open.2.html

rust 封装:
https://github.com/pop-os/pidfd

## pid
- [ ] 真的需要好好看看了，在目錄下 ag pid 出來的內容那麼多。。。。。, check 這些文件的內容

当我们发现
1. pid 在 namespace 的层次结构，需要在上层每一个都需分配一个 pid，
2. thread group, process group, session group 都存在對應的 id
3. task_struct::pid, task_struct::tgid 是顶层 namespace 的对应 pid tgid 的快捷表示, 具體代碼可以看 copy_process 對於 pid 的賦值
4. task_struct::thread_pid 是該 threadk

那么剩下的都很简单了:

```c
struct task_struct {
	pid_t				pid; // global pid
	pid_t				tgid; // global thread group pid

	/* PID/PID hash table linkage. */
	struct pid			*thread_pid;
	struct hlist_node		pid_links[PIDTYPE_MAX];
	struct list_head		thread_group;
	struct list_head		thread_node; // TODO
```

```c
/*
 * the helpers to get the pid's id seen from different namespaces
 *
 * pid_nr()    : global id, i.e. the id seen from the init namespace;
 * pid_vnr()   : virtual id, i.e. the id seen from the pid namespace of
 *               current.
 * pid_nr_ns() : id seen from the ns specified.
 *
 * see also task_xid_nr() etc in include/linux/sched.h
 */

static inline pid_t pid_nr(struct pid *pid)
{
	pid_t nr = 0;
	if (pid)
		nr = pid->numbers[0].nr;
	return nr;
}
```


```c
/*
 * struct upid is used to get the id of the struct pid, as it is
 * seen in particular namespace. Later the struct pid is found with
 * find_pid_ns() using the int nr and struct pid_namespace *ns.
 */

struct upid {
	int nr;
	struct pid_namespace *ns;
};

struct pid
{
	refcount_t count;
	unsigned int level; // 每一個 task 都會對應一個 pid, level 表示 task 當前所在 thread 的位置
	spinlock_t lock;
  // 用於指向其所在的 thread group, process group, session group 的 pid
  // 參考 attach_pid, 通過成員 tasks 可以將其掛在 task_struct::
	/* lists of tasks that use this pid */
	struct hlist_head tasks[PIDTYPE_MAX];
	struct hlist_head inodes; // TODO 應該是 pidfd
	/* wait queue for pidfd notifications */
	wait_queue_head_t wait_pidfd;
	struct rcu_head rcu;
	struct upid numbers[1];
};
```

attach_pid : 讓 thread group, process group, session group 的 leader 知道自己掌控的 pid 有那些
init_task_pid : 让 task 知道其 thread group, prcess group, session group leader 的 pid

```c
static inline void
init_task_pid(struct task_struct *task, enum pid_type type, struct pid *pid)
{
	if (type == PIDTYPE_PID)
		task->thread_pid = pid;
	else
		task->signal->pids[type] = pid;
}

/*
 * attach_pid() must be called with the tasklist_lock write-held.
 */
void attach_pid(struct task_struct *task, enum pid_type type)
{
	struct pid *pid = *task_pid_ptr(task, type);
	hlist_add_head_rcu(&task->pid_links[type], &pid->tasks[type]);
}
```

最後理解一下 pid 和 namespace :
```c
struct pid_namespace *task_active_pid_ns(struct task_struct *tsk)
{
  // task_pid(tsk) : task_struct->thread_pid
  // task_struct::thread_pid 就是該 task 的 pid
	return ns_of_pid(task_pid(tsk));
}

static inline struct pid_namespace *ns_of_pid(struct pid *pid)
{
	struct pid_namespace *ns = NULL;
	if (pid)
    // pid 是跟随 thread 的，通过 level 就可以知道其 ns
		ns = pid->numbers[pid->level].ns;
	return ns;
}

// ns 控制了一个 pid 空间，idr 加速访问
struct pid *find_pid_ns(int nr, struct pid_namespace *ns)
{
	return idr_find(&ns->idr, nr);
}
```

- 从 alloc_pid 中，對於一個 thread, 會給每一個 namespace 中間存放一個 id


綜合實踐，syscall getpid 是如何實現的 ?
```c
/**
 * sys_getpid - return the thread group id of the current process
 *
 * Note, despite the name, this returns the tgid not the pid.  The tgid and
 * the pid are identical unless CLONE_THREAD was specified on clone() in
 * which case the tgid is the same in all threads of the same group.
 *
 * This is SMP safe as current->tgid does not change.
 */
SYSCALL_DEFINE0(getpid)
{
	return task_tgid_vnr(current);
}

static inline pid_t task_tgid_vnr(struct task_struct *tsk)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_TGID, NULL);
}


pid_t __task_pid_nr_ns(struct task_struct *task, enum pid_type type,
			struct pid_namespace *ns)
{
	pid_t nr = 0;

	rcu_read_lock();
	if (!ns)
		ns = task_active_pid_ns(current); // 获取到 ns
	nr = pid_nr_ns(rcu_dereference(*task_pid_ptr(task, type)), ns); // 通过 task->thread_pid 获取 thread 对应的 pid
	rcu_read_unlock();

	return nr;
}

pid_t pid_nr_ns(struct pid *pid, struct pid_namespace *ns)
{
	struct upid *upid;
	pid_t nr = 0;

	if (pid && ns->level <= pid->level) {
		upid = &pid->numbers[ns->level]; // 获取到该 level 上的
		if (upid->ns == ns)
			nr = upid->nr;
	}
	return nr;
}

static struct pid **task_pid_ptr(struct task_struct *task, enum pid_type type)
{
	return (type == PIDTYPE_PID) ?
		&task->thread_pid :
		&task->signal->pids[type];
}
```





## green thread
https://github.com/google/marl : 为什么用户层可以控制 scheduler ?

## cpp thread keyword
如何实现的 ?


## cpu
https://superuser.com/questions/1217454/how-do-you-control-thread-affinity-across-multiple-processes-on-linux
1. cat /proc/cpuinfo |grep -P 'processor|physical id|core id' 中间的各种 id 的含义是什么 ?
2. 更进一步，/proc/cpuinfo 中的每一个含义是什么，数据从哪里导出的

## syscall
[What did hardware do in syscall](https://www.felixcloutier.com/x86/syscall)

[讲解极为清楚](http://arthurchiao.art/blog/system-call-definitive-guide-zh/)


这定义在 x86-64 ABI 的 A.2.1 小节:

> User-level applications use as integer registers for passing the sequence %rdi, %rsi, %rdx, %rcx, %r8 and %r9. The kernel interface uses %rdi, %rsi, %rdx, %r10, %r8 and %r9.
> A system-call is done via the syscall instruction. The kernel destroys registers %rcx and %r11.
> The number of the syscall has to be passed in register %rax.
> System-calls are limited to six arguments,no argument is passed directly on the stack.
> Returning from the syscall, register %rax contains the result of the system-call. A value in the range between -4095 and -1 indicates an error, it is -errno.
> Only values of class INTEGER or class MEMORY are passed to the kernel.

- [ ] 现在只有一个小问题，r11 是不是 syscall 指令自动将 user 的 eflags 保存到其中的，至少 rcx 是的


[^25] The two modes are distinguished by the `dpl` (descriptor privilege level) field in segment register cs. dpl=3  in cs for user-mode, and zero for kernel-mode (not sure if this "level" equivalent to so-called ring3 and ring0).
In real mode kernel should handle the segment registers carefully, while in x86-64, instructions syscall and sysret will properly set segment registers automatically, so we don't need to maintain segment registers manually
And another difference is the permission setting in page tables.


- [x] how kernel transfer sys_call_ptr_t's parameters to, code below is an example.
    - The answer lies in `SYSCALL_DEFINE5`
```c
typedef asmlinkage long (*sys_call_ptr_t)(const struct pt_regs *);

SYSCALL_DEFINE5(clone, unsigned long, clone_flags, unsigned long, newsp,
		 int __user *, parent_tidptr,
		 int __user *, child_tidptr,
		 unsigned long, tls)
```

```c
asmlinkage const sys_call_ptr_t sys_call_table[__NR_syscall_max+1] = {
	/*
	 * Smells like a compiler bug -- it doesn't work
	 * when the & below is removed.
	 */
	[0 ... __NR_syscall_max] = &sys_ni_syscall,
#include <asm/syscalls_64.h>
};

#ifdef CONFIG_X86
__SYSCALL_64(3, __x64_sys_close, )
#else /* CONFIG_UML */

#define __SYSCALL_64(nr, sym, qual) [nr] = sym,
```





- [ ] analyzer code here : arch/x86/entry/common.c

`__prepare_exit_to_usermode` => exit_to_usermode_loop

currently, exit_to_usermode_loop has tell really important two thing:
- schdule()
- do_signal()



// PLKA 上应该分析过和 mem 相关的 syscall 吧 !

https://lwn.net/Articles/604287/

- [ ] Read this chapter carefully: https://0xax.gitbooks.io/linux-insides/content/SysCall/

- [ ] This chapter too
/home/maritns3/core/vn/kernel/insides/Syscall/syscall_draft.md


- [ ] Something interesting wait for clear:

gcc a.c -v 可以获取很多东西:
```
Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/lto-wrapper
Target: x86_64-pc-linux-gnu
Configured with: /build/gcc/src/gcc/configure --prefix=/usr --libdir=/usr/lib --libexecdir=/usr/lib --mandir=/usr/share/man --infodir=/usr/share/info --with-bugurl=https://bugs.archlinux.org/ --enable-languages=c,c++,ada,fortran,go,lto,objc,obj-c++,d --with-isl --with-linker-hash-style=gnu --with-system-zlib --enable-__cxa_atexit --enable-cet=auto --enable-checking=release --enable-clocale=gnu --enable-default-pie --enable-default-ssp --enable-gnu-indirect-function --enable-gnu-unique-object --enable-install-libiberty --enable-linker-build-id --enable-lto --enable-multilib --enable-plugin --enable-shared --enable-threads=posix --disable-libssp --disable-libstdcxx-pch --disable-libunwind-exceptions --disable-werror gdc_include_dir=/usr/include/dlang/gdc
Thread model: posix
Supported LTO compression algorithms: zlib zstd
gcc version 10.1.0 (GCC)
COLLECT_GCC_OPTIONS='-v' '-mtune=generic' '-march=x86-64'
 /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/cc1 -quiet -v a.c -quiet -dumpbase a.c -mtune=generic -march=x86-64 -auxbase a -version -o /tmp/ccTZJNTI.s
GNU C17 (GCC) version 10.1.0 (x86_64-pc-linux-gnu)
	compiled by GNU C version 10.1.0, GMP version 6.2.0, MPFR version 4.0.2, MPC version 1.1.0, isl version isl-0.21-GMP

GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
ignoring nonexistent directory "/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../x86_64-pc-linux-gnu/include"
#include "..." search starts here:
#include <...> search starts here:
 /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/include
 /usr/local/include
 /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/include-fixed
 /usr/include
End of search list.
GNU C17 (GCC) version 10.1.0 (x86_64-pc-linux-gnu)
	compiled by GNU C version 10.1.0, GMP version 6.2.0, MPFR version 4.0.2, MPC version 1.1.0, isl version isl-0.21-GMP

gGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
Compiler executable checksum: 8cbeeaf463513b43dc3c4cd12e0bccb6
COLLECT_GCC_OPTIONS='-v' '-mtune=generic' '-march=x86-64'
 as -v --64 -o /tmp/ccVXGSbG.o /tmp/ccTZJNTI.s
GNU assembler version 2.34.0 (x86_64-pc-linux-gnu) using BFD version (GNU Binutils) 2.34.0
COMPILER_PATH=/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/:/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/:/usr/lib/gcc/x86_64-pc-linux-gnu/:/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/:/usr/lib/gcc/x86_64-pc-linux-gnu/
LIBRARY_PATH=/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/:/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib/:/lib/../lib/:/usr/lib/../lib/:/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../:/lib/:/usr/lib/
COLLECT_GCC_OPTIONS='-v' '-mtune=generic' '-march=x86-64'
 /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/collect2 -plugin /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/liblto_plugin.so -plugin-opt=/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/lto-wrapper -plugin-opt=-fresolution=/tmp/ccXugFlJ.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr --hash-style=gnu -m elf_x86_64 -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib/Scrt1.o /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib/crti.o /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/crtbeginS.o -L/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0 -L/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib -L/lib/../lib -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../.. /tmp/ccVXGSbG.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/crtendS.o /usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/../../../../lib/crtn.o
COLLECT_GCC_OPTIONS='-v' '-mtune=generic' '-march=x86-64'
```
我们会发现，直接 ld `gcc -c a.c`产生的.o 是没用的，但是如果使用上面结果的倒数第二行的内容，
将 /tmp/ 的 .o 替换为 `gcc -c a.c` 产生.o 就可以了, 参考 [^22]
但是如果是 nasm 编译的，ld 就不需要任何特别的参数，可能是因为不需要依赖外部库吧!
参考 compiler/programmer/extra/write.asm

关于 syscall 的参数传递，可以 system V abi 的内容: [^23] [^24] (其实只有 100 多页)

函数的 convention 和 syscall 的 convention 不同，所以需要调整一下:
[同时展示 user 和 kernel 的调用关系](https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/x86_64/syscall.S)
这段汇编 stub 代码非常酷，因为它同时展示了两个调用约定：传递给这个函数的参数 符合 用户空间调用约定，然后将这些参数移动到其他寄存器，使得它们在通过 syscall 进入内核之前符合 内核调用约定。

#### x86 syscall(merge)
https://david942j.blogspot.com/2018/10/note-learning-kvm-implement-your-own.html : 通过 cs 来区分 dpl
https:/www.felixcloutier.com/x86/syscall/
From the pseudo code of sysret you can see it sets attributes of cs and ss explicitly:

TODO 分别找到 legacy 和 sysenter 的注册位置(初始化 handler) 的方法.

[^3] 放到 insides 部分记录吧!


syscall_64.c:sys_call_table : 持有所有的 syscall

x86/common.c:do_syscall_64 : 从 sys_call_table 中简单的选择函数，进行调用
  1. syscall_enter_from_user_mode
  2. syscall_exit_from_user_mode

entry_64.S : entry_SYSCALL_64 是 syscall 的入口，这个入口的初始化 x86/kernel/cpu/common.c:syscall_init 中

这个教程很新，一共三个，非常适合阅读:
https://lwn.net/Articles/604287/ 而 [^4] 全面，但是有的没有讲解清楚.

TOOD :
1. `__kernel_vsyscall` 还是调用 sysenter 之类的，那么有什么意义啊
2. 为什么 syscall, sysenter, int 0x80 的性能区别的原因是什么 ?

#### syscall int
idt_setup_traps ==> def_idts ==> entry_INT80_compat

```c
/*
 * 32-bit legacy system call entry.
 *
 * 32-bit x86 Linux system calls traditionally used the INT $0x80
 * instruction.  INT $0x80 lands here.
 *
 * This entry point can be used by 32-bit and 64-bit programs to perform
 * 32-bit system calls.  Instances of INT $0x80 can be found inline in
 * various programs and libraries.  It is also used by the vDSO's
 * __kernel_vsyscall fallback for hardware that doesn't support a faster
 * entry method.  Restarted 32-bit system calls also fall back to INT
 * $0x80 regardless of what instruction was originally used to do the
 * system call.
 *
 * This is considered a slow path.  It is not used by most libc
 * implementations on modern hardware except during process startup.
 *
 * Arguments:
 * eax  system call number
 * ebx  arg1
 * ecx  arg2
 * edx  arg3
 * esi  arg4
 * edi  arg5
 * ebp  arg6
 */
SYM_CODE_START(entry_INT80_compat)
```
- [ ] int 80 处理了特殊的事情，相对于其他的 int

#### syscall vsyscall

跟踪 sysenter 工作的逻辑是一项相当复杂的工作，因为和软中断不同，sysenter 并 不保存返回地址。内核在调用 sysenter 之前所做的工作随着内核版本在不断变化（已经 变了，接下来在 Bugs 小节会看到）。

为了消除将来的变动带来的影响，用户程序使用一个叫 `__kernel_vsyscall` 的函数，它在内核实现，但每个用户进程启动的时候它会映射到用户进程。这颇为怪异，它 是内核函数，但在用户空间运行。其实，`__kernel_vsyscall` 是一种被称为虚拟动态共享库（virtual Dynamic Shared Object, vDSO）的一部分，这种技术允许在用户空间 执行内核代码。我们后面会深入介绍 vDSO 的原理和用途。现在，先看 `__kernel_vsyscall` 的实现。

[getauxval() and the auxiliary vector](https://lwn.net/Articles/519085/)
> There are many mechanisms for communicating information between user-space applications and the kernel.
> 1. syscall
> 2. pseudo fs
> 3. signal
> 4. These include the Linux-specific `netlink` sockets and user-mode helper features.
> 5. The auxiliary vector, a mechanism for communicating information from the kernel to user space



#### syscall getcpu

`__vdso_getcpu` ==> vdso_read_cpunode

- [ ] `__vdso_getcpu` 的调用者是谁 ?
- [ ] 所以，在进入 dune 之后，可以正确的使用 gdt ? 似乎 gdt 被错误的清理了 ?
- [ ] 为什么可以通过设置 got 实现 get node 和 cpu id
- [ ] 存在完全不同的方法，而且绝对不在一个位置


kernel/sys.c
```c
SYSCALL_DEFINE3(getcpu, unsigned __user *, cpup, unsigned __user *, nodep,
		struct getcpu_cache __user *, unused)
{
	int err = 0;
	int cpu = raw_smp_processor_id();

	if (cpup)
		err |= put_user(cpu, cpup);
	if (nodep)
		err |= put_user(cpu_to_node(cpu), nodep);
	return err ? -EFAULT : 0;
}
```



## daemon
systemctl 利用 /etc/init.d/cassandra 的来启动，还是很有意思的

## user group
了解设计思想，然后阅读代码

## smp
/home/maritns3/core/linux/arch/x86/kernel/smp.c
/home/maritns3/core/linux/arch/x86/kernel/smpboot.c

smp_call_function_many 之类的函数可以看看

## `__schedule`
- [ ] `cond_resched()`

```c
/*
 * __schedule() is the main scheduler function.
 *
 * The main means of driving the scheduler and thus entering this function are:
 *
 *   1. Explicit blocking: mutex, semaphore, waitqueue, etc.
 *
 *   2. TIF_NEED_RESCHED flag is checked on interrupt and userspace return
 *      paths. For example, see arch/x86/entry_64.S.
 *
 *      To drive preemption between tasks, the scheduler sets the flag in timer
 *      interrupt handler scheduler_tick().
 *
 *   3. Wakeups don't really cause entry into schedule(). They add a
 *      task to the run-queue and that's it.
 *
 *      Now, if the new task added to the run-queue preempts the current
 *      task, then the wakeup sets TIF_NEED_RESCHED and schedule() gets
 *      called on the nearest possible occasion:
 *
 *       - If the kernel is preemptible (CONFIG_PREEMPTION=y):
 *
 *         - in syscall or exception context, at the next outmost
 *           preempt_enable(). (this might be as soon as the wake_up()'s
 *           spin_unlock()!)
 *
 *         - in IRQ context, return from interrupt-handler to
 *           preemptible context
 *
 *       - If the kernel is not preemptible (CONFIG_PREEMPTION is not set)
 *         then at the next:
 *
 *          - cond_resched() call
 *          - explicit schedule() call
 *          - return from syscall or exception to user-space
 *          - return from interrupt-handler to user-space
 *
 * WARNING: must be called with preemption disabled!
 */
static void __sched notrace __schedule(bool preempt)
```

## kthread
kernel/kthread.c : how to schedule the kthread

- [ ] trace function create_worker()
  - [ ] kthreadd()
  - in fact, nothing special, create kthread with a daemon named `kthreadd` which call `fork`

`kernel_thread()` called by  `create_kthread()` in kthread.c
```c
/*
 * Create a kernel thread.
 */
pid_t kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
	struct kernel_clone_args args = {
		.flags		= ((lower_32_bits(flags) | CLONE_VM |
				    CLONE_UNTRACED) & ~CSIGNAL),
		.exit_signal	= (lower_32_bits(flags) & CSIGNAL),
		.stack		= (unsigned long)fn,
		.stack_size	= (unsigned long)arg,
	};

	return kernel_clone(&args);
}

static inline void kthread_frame_init(struct inactive_task_frame *frame,
				      unsigned long fun, unsigned long arg)
{
	frame->bx = fun;
	frame->r12 = arg;
}
```

```
/*
 * A newly forked process directly context switches into this address.
 *
 * rax: prev task we switched from
 * rbx: kernel thread func (NULL for user thread)
 * r12: kernel thread arg
 */
.pushsection .text, "ax"
SYM_CODE_START(ret_from_fork)
	UNWIND_HINT_EMPTY
	movq	%rax, %rdi
	call	schedule_tail			/* rdi: 'prev' task parameter */

	testq	%rbx, %rbx			/* from kernel_thread? */
	jnz	1f				/* kernel threads are uncommon */

2:
	UNWIND_HINT_REGS
	movq	%rsp, %rdi
	call	syscall_exit_to_user_mode	/* returns with IRQs disabled */
	jmp	swapgs_restore_regs_and_return_to_usermode

1:
	/* kernel thread */
	UNWIND_HINT_EMPTY
	movq	%r12, %rdi
	CALL_NOSPEC rbx
	/*
	 * A kernel thread is allowed to return here after successfully
	 * calling kernel_execve().  Exit to userspace to complete the execve()
	 * syscall.
	 */
	movq	$0, RAX(%rsp)
	jmp	2b
SYM_CODE_END(ret_from_fork)
```
注釋的三個寄存器的解釋:
1. rax : 因爲從 `__switch_to_asm` ==> `__switch_to` 返回，該函數返回的就是 prev task
2. rbx, r12 : kthread_frame_init

- [ ] schedule_tail

在 `kthread()` 中間，當 kernel thread 調用 fn 完成之後，需要


- [ ] kthread_queue_work : we encounter this function in the /home/maritns3/core/linux/arch/x86/kvm/i8254.c
  - my question is : what's the meaning of queue in the context of kthread ?

## first user process
```c
static int run_init_process(const char *init_filename)
{
	const char *const *p;

	argv_init[0] = init_filename;
	pr_info("Run %s as init process\n", init_filename);
	pr_debug("  with arguments:\n");
	for (p = argv_init; *p; p++)
		pr_debug("    %s\n", *p);
	pr_debug("  with environment:\n");
	for (p = envp_init; *p; p++)
		pr_debug("    %s\n", *p);
	return kernel_execve(init_filename, argv_init, envp_init);
}
```



## TODO
https://phoenixnap.com/kb/create-a-sudo-user-on-debian : 首先搞清楚这种简单的文章

- I need write a blog about how linux process works
  - [ ] **https://kernel.blog.csdn.net/** : dozens of blog
  - [ ] LoyenWang

疑问:
1. ucore lab1 的附加题的说明，实现地址空间切换的方法
    1. 似乎调整一下一个寄存器的属性就可以了，那么，syscall 是如何调整这些属性的
2. 似乎虚拟机利用中间层次的 ring  ?
3. 是什么表示当前是内核态还是用户态呀 ?

1. 第一个用户进程是如何产生的 ?
    1. idle 和 init 进程如何产生的 ?
3. IPC 和 signal 各自的适用范围是什么 ?
4. preemption
5. 为什么需要维持 parent child 的树状关系 ?
    1. exit 回收资源 ?
    2. 部分信号机制总是只是出现在 child 和 parent
6. 一个 thread group 都是 group leader 的 children 吗 ?

整理一下
> # ptrace 的代价是什么 ?
> 1. 为了支持ptrace syscall的努力是什么
> 2. ptrace 是不是实现debug 的基础
> 3. fork 等机制做出了何种支持
>
>
> # 内核和用户态之间拷贝数据
> 1. 用户读写文件　文件内容　是如何传递到达用户的 ?　显然不可能是通过copy_to_user 之类的函数
> 2. copy_to_user 的两个参数，都是虚拟地址，但是实际上，但是这两个地址是位于不同的pgdir 中间的
> 3. copy_to_user 检查内容是看该addr 是不是在对应用户的地址空间的 segment 而且保证没有segment fault，这一个函数从哪里获取的 mm_struct
>
>
> # 到底如何实现context switch
>
> 1. http://www.maizure.org/projects/evolution_x86_context_switch_linux/
> > 绝对清晰的讲解
> Many of these tasks float between the `switch_to()` and the scheduler across kernel versions. All I can guarantee is that we'll always see stack swaps and FPU switching in every version
>
> 2. https://eli.thegreenplace.net/2018/measuring-context-switching-and-memory-overheads-for-linux-threads/　
> > 分析context switch 的代价是什么 ?
>
> 3. https://stackoverflow.com/questions/2711044/why-doesnt-linux-use-the-hardware-context-switch-via-the-tss
> > 再次印证TSS 在 context switch 中间并没有什么作用，但是 @todo TSS 中间存储了ESP0 和 SS0 用于实现interrupt

## process group
- [ ] maybe build a shell: https://github.com/danistefanovic/build-your-own-x#build-your-own-shell, then process group is simple


![loading](https://unixism.net/wp-content/uploads/2020/06/Process-Group-Signals.png)


## sched_class ops
```c
struct sched_class {

#ifdef CONFIG_UCLAMP_TASK
	int uclamp_enabled;
#endif

	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*yield_task)   (struct rq *rq);
	bool (*yield_to_task)(struct rq *rq, struct task_struct *p);

	void (*check_preempt_curr)(struct rq *rq, struct task_struct *p, int flags);

	struct task_struct *(*pick_next_task)(struct rq *rq);

	void (*put_prev_task)(struct rq *rq, struct task_struct *p);
	void (*set_next_task)(struct rq *rq, struct task_struct *p, bool first);

#ifdef CONFIG_SMP
	int (*balance)(struct rq *rq, struct task_struct *prev, struct rq_flags *rf);
	int  (*select_task_rq)(struct task_struct *p, int task_cpu, int sd_flag, int flags);
	void (*migrate_task_rq)(struct task_struct *p, int new_cpu);

	void (*task_woken)(struct rq *this_rq, struct task_struct *task);

	void (*set_cpus_allowed)(struct task_struct *p,
				 const struct cpumask *newmask);

	void (*rq_online)(struct rq *rq);
	void (*rq_offline)(struct rq *rq);
#endif

	void (*task_tick)(struct rq *rq, struct task_struct *p, int queued);
	void (*task_fork)(struct task_struct *p);
	void (*task_dead)(struct task_struct *p);

	/*
	 * The switched_from() call is allowed to drop rq->lock, therefore we
	 * cannot assume the switched_from/switched_to pair is serliazed by
	 * rq->lock. They are however serialized by p->pi_lock.
	 */
	void (*switched_from)(struct rq *this_rq, struct task_struct *task);
	void (*switched_to)  (struct rq *this_rq, struct task_struct *task);
	void (*prio_changed) (struct rq *this_rq, struct task_struct *task,
			      int oldprio);

	unsigned int (*get_rr_interval)(struct rq *rq,
					struct task_struct *task);

	void (*update_curr)(struct rq *rq);

#define TASK_SET_GROUP		0
#define TASK_MOVE_GROUP		1

#ifdef CONFIG_FAIR_GROUP_SCHED
	void (*task_change_group)(struct task_struct *p, int type);
#endif
} __aligned(STRUCT_ALIGNMENT); /* STRUCT_ALIGN(), vmlinux.lds.h */
```

#### enqueue
```c
enqueue_task

	p->sched_class->enqueue_task(rq, p, flags);

```

With post of [LoyenWang](https://www.cnblogs.com/LoyenWang/p/12495319.html), we will understand every class function.

## runtime vruntime


## cfs
- [ ] 运行时间runtime可以转换成虚拟运行时间vruntime；
- [ ] what if vruntime overflow ?


```c
struct sched_entity {
	/* For load-balancing: */
	struct load_weight		load;     //调度实体的负载权重值
	struct rb_node			run_node;   //用于连接到CFS运行队列的红黑树中的节点
	struct list_head		group_node; //用于连接到CFS运行队列的cfs_tasks链表中的节点
	unsigned int			on_rq;        //用于表示是否在运行队列中

	u64				exec_start;           //当前调度实体的开始执行时间
	u64				sum_exec_runtime;     //调度实体执行的总时间
	u64				vruntime;             //虚拟运行时间，这个时间用于在CFS运行队列中排队
	u64				prev_sum_exec_runtime;//上一个调度实体运行的总时间

	u64				nr_migrations;        //负载均衡
```

## pt_regs
```c
struct pt_regs {
/*
 * C ABI says these regs are callee-preserved. They aren't saved on kernel entry
 * unless syscall needs a complete, fully filled "struct pt_regs".
 */
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long bp;
	unsigned long bx;
/* These regs are callee-clobbered. Always saved on kernel entry. */
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long ax;
	unsigned long cx;
	unsigned long dx;
	unsigned long si;
	unsigned long di;
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
	unsigned long orig_ax;
/* Return frame for iretq */
	unsigned long ip;
	unsigned long cs;
	unsigned long flags;
	unsigned long sp;
	unsigned long ss;
/* top of stack page */
};
```

- [ ] orig_ax
- [ ] how pt_regs constructed, by hardware or software ?
  - [ ] syscall, CPU exception and hardware has different ways to construct it ?


```c
struct fork_frame {
	struct inactive_task_frame frame;
	struct pt_regs regs;
};
```

- [ ] entry_64.S::entry_SYSCALL_64 中間的代码看，保存的內容就是 pt_regs 的，而 switch_to_asm 保存的內容是 inactive_task_frame 的

## ipc
- [ ] 有一个观测程序叫做 : ipcs
- [ ] 以前整理过的 ipc 和 namespace 相关的放到这里来

## zombie 和 orphan
https://stackoverflow.com/questions/20688982/zombie-process-vs-orphan-process

- orphan 根本不是一个问题，只是为了描述一种情况，parent 挂了, child 自动挂载到 init 上
- zombie : 因为 parent 需要利用 wait 来获取 child 的状态，如果 child 挂掉了, parent 不回收，那就出现问题


[^2]: https://man7.org/linux/man-pages/man7/signal.7.html
[^3]: https://0xax.gitbooks.io/linux-insides/content/SysCall/linux-syscall-2.html
[^4]: https://blog.packagecloud.io/eng/2016/04/05/the-definitive-guide-to-linux-system-calls/#64-bit-f
[^5]: https://www.kernel.org/doc/html/latest/x86/kernel-stacks.html
[^6]: https://stackoverflow.com/questions/61886139/why-thread-info-should-be-the-first-element-in-task-struct
[^8]: https://www.cnblogs.com/LoyenWang/p/12249106.html
[^10]: https://www.cnblogs.com/LoyenWang/p/12316660.html
[^22]: https://stackoverflow.com/questions/14163208/how-to-link-c-object-files-with-ld
[^23]: https://stackoverflow.com/questions/18133812/where-is-the-x86-64-system-v-abi-documented
[^24]: https://uclibc.org/docs/psABI-x86_64.pdf
[^25]: https://david942j.blogspot.com/2018/10/note-learning-kvm-implement-your-own.html
