# process 的状态 procstat

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
#define TASK_RUNNING			0x00000000
#define TASK_INTERRUPTIBLE		0x00000001
#define TASK_UNINTERRUPTIBLE		0x00000002
#define __TASK_STOPPED			0x00000004
#define __TASK_TRACED			0x00000008
/* Used in tsk->exit_state: */
#define EXIT_DEAD			0x00000010
#define EXIT_ZOMBIE			0x00000020
#define EXIT_TRACE			(EXIT_ZOMBIE | EXIT_DEAD)
/* Used in tsk->state again: */
#define TASK_PARKED			0x00000040
#define TASK_DEAD			0x00000080
#define TASK_WAKEKILL			0x00000100
#define TASK_WAKING			0x00000200
#define TASK_NOLOAD			0x00000400
#define TASK_NEW			0x00000800
#define TASK_RTLOCK_WAIT		0x00001000
#define TASK_FREEZABLE			0x00002000
#define __TASK_FREEZABLE_UNSAFE	       (0x00004000 * IS_ENABLED(CONFIG_LOCKDEP))
#define TASK_FROZEN			0x00008000
#define TASK_STATE_MAX			0x00010000

#define TASK_ANY			(TASK_STATE_MAX-1)

/*
 * DO NOT ADD ANY NEW USERS !
 */
#define TASK_FREEZABLE_UNSAFE		(TASK_FREEZABLE | __TASK_FREEZABLE_UNSAFE)

/* Convenience macros for the sake of set_current_state: */
#define TASK_KILLABLE			(TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED			(TASK_WAKEKILL | __TASK_STOPPED)
#define TASK_TRACED			__TASK_TRACED

#define TASK_IDLE			(TASK_UNINTERRUPTIBLE | TASK_NOLOAD)

/* Convenience macros for the sake of wake_up(): */
#define TASK_NORMAL			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)

/* get_task_state(): */
#define TASK_REPORT			(TASK_RUNNING | TASK_INTERRUPTIBLE | \
					 TASK_UNINTERRUPTIBLE | __TASK_STOPPED | \
					 __TASK_TRACED | EXIT_DEAD | EXIT_ZOMBIE | \
					 TASK_PARKED)

```

## TASK_UNINTERRUPTIBLE TASK_INTERRUPTIBLE
```c
/*
 * Wait for writeback on an inode to complete. Called with i_lock held.
 * Caller must make sure inode cannot go away when we drop i_lock.
 */
static void __inode_wait_for_writeback(struct inode *inode)
	__releases(inode->i_lock)
	__acquires(inode->i_lock)
{
	DEFINE_WAIT_BIT(wq, &inode->i_state, __I_SYNC);
	wait_queue_head_t *wqh;

	wqh = bit_waitqueue(&inode->i_state, __I_SYNC);
	while (inode->i_state & I_SYNC) {
		spin_unlock(&inode->i_lock);
		__wait_on_bit(wqh, &wq, bit_wait,
			      TASK_UNINTERRUPTIBLE);
		spin_lock(&inode->i_lock);
	}
}
```
这就是等待 io 总是在 uninterrupted 的证据。

- [ ] 但是为什么 io 等待的时候总是 uninterrupted 的?

## TASK_PARKED
主要是给 kthread 使用，处于该状态的时候，表示 `__kthread_parkme`，其中一直睡眠，然后等待其他人唤醒。

- [ ] 更多的背后的细节不清楚

## TASK_FROZEN TASK_FREEZABLE

似乎是 freeze 采用的:
- kernel/power/process.c
- kernel/freezer.c

## TASK_NEW
sched_fork 中赋值
```txt
#0  sched_fork (clone_flags=clone_flags@entry=18874368, p=p@entry=0xffff888108f7b480) at kernel/sched/core.c:4690
#1  0xffffffff8113e01c in copy_process (pid=pid@entry=0x0 <fixed_percpu_data>, trace=trace@entry=0, node=node@entry=-1, args=args@entry=0xffffc90002297eb0) at kernel/fork.c:2477
#2  0xffffffff8113f4df in kernel_clone (args=args@entry=0xffffc90002297eb0) at kernel/fork.c:2918
#3  0xffffffff8113f966 in __do_sys_clone (clone_flags=<optimized out>, newsp=<optimized out>, parent_tidptr=<optimized out>, child_tidptr=<optimized out>, tls=<optimized out>) at kernel/fork.c:3061
```

```c
#ifdef CONFIG_FAIR_GROUP_SCHED
static void task_change_group_fair(struct task_struct *p)
{
	/*
	 * We couldn't detach or attach a forked task which
	 * hasn't been woken up by wake_up_new_task().
	 */
	if (READ_ONCE(p->__state) == TASK_NEW)
		return;

	detach_task_cfs_rq(p);

#ifdef CONFIG_SMP
	/* Tell se's cfs_rq has been changed -- migrated */
	p->se.avg.last_update_time = 0;
#endif
	set_task_rq(p, task_cpu(p));
	attach_task_cfs_rq(p);
}
```

```txt
#0  wake_up_new_task (p=p@entry=0xffff888108f7b480) at kernel/sched/core.c:4810
#1  0xffffffff8113f53b in kernel_clone (args=args@entry=0xffffc90002297eb0) at kernel/fork.c:2949
#2  0xffffffff8113f966 in __do_sys_clone (clone_flags=<optimized out>, newsp=<optimized out>, parent_tidptr=<optimized out>, child_tidptr=<optimized out>, tls=<optimized out>) at kernel/fork.c:3061
```

好吧，需要更多的背景知识。


## TASK_NOLOAD
```c
/*
 * Like schedule_timeout_uninterruptible(), except this task will not contribute
 * to load average.
 */
signed long __sched schedule_timeout_idle(signed long timeout)
{
	__set_current_state(TASK_IDLE);
	return schedule_timeout(timeout);
}
```
等待 io 的时候也是计入到 LOAD 中的，所以

## TASK_WAKING
- try_to_wake_up

```c
	/*
	 * We're doing the wakeup (@success == 1), they did a dequeue (p->on_rq
	 * == 0), which means we need to do an enqueue, change p->state to
	 * TASK_WAKING such that we can unlock p->pi_lock before doing the
	 * enqueue, such as ttwu_queue_wakelist().
	 */
	WRITE_ONCE(p->__state, TASK_WAKING);
```

## TASK_WAKEKILL && TASK_KILLABLE
- [TASK_KILLABLE](https://lwn.net/Articles/288056/)

> Like most versions of Unix, Linux has two fundamental ways in which a process can be put to sleep. A process which is placed in the TASK_INTERRUPTIBLE state will sleep until either (1) something explicitly wakes it up, or (2) a non-masked signal is received. The TASK_UNINTERRUPTIBLE state, instead, ignores signals; processes in that state will require an explicit wakeup before they can run again.

- TASK_INTERRUPTIBLE 可以被 wake up 唤醒，或者信号
- TASK_UNINTERRUPTIBLE 只能别 wake up 唤醒

> Kernel code which uses interruptible sleeps must always check to see whether it woke up as a result of a signal, and, if so, clean up whatever it was doing and return -EINTR back to user space. The user-space side, too, must realize that a system call was interrupted and respond accordingly; not all user-space programmers are known for their diligence in this regard. Making a sleep uninterruptible eliminates these problems, but at the cost of being, well, uninterruptible. If the expected wakeup event does not materialize, the process will wait forever and there is usually nothing that anybody can do about it short of rebooting the system. This is the source of the dreaded, unkillable process which is shown to be in the "D" state by ps.

这个总结的太好了。

所以这个组合非常合理，拒绝信号，但是除了 kill 信号
```c
/* Convenience macros for the sake of set_current_state: */
#define TASK_KILLABLE			(TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
```

## `__TASK_STOPPED`
程序接受到了 stop 信号。

## top 的各种妙用

1. 观察各个 thread 的 CPU 利用率 top -H -p $pid
	- 不要用 pidstat ，那个不准的


## 理解 ps, top 以及 htop 关于 process state 的输出
<!-- a074d90c-0057-45ad-b206-ccb450246f05 -->

首先，他们的编码都是相同的，具体的含义为:

ps 的:
```txt
PROCESS STATE CODES
       Here are the different values that the s, stat and state output specifiers (header "STAT" or "S") will display to describe the
       state of a process:

               D    uninterruptible sleep (usually IO)
               I    Idle kernel thread
               R    running or runnable (on run queue)
               S    interruptible sleep (waiting for an event to complete)
               T    stopped by job control signal
               t    stopped by debugger during the tracing
               Z    defunct ("zombie") process, terminated but not reaped by its parent
```

其中关于 T 和 t 的差别:

sleep 被 ctrl z 之后
```txt
0 T martins3   89366   89218  0  80   0 -  1380 do_sig 08:30 pts/3    00:00:00 sleep 1000
```

如果被 gdb 调试
```txt
0 t martins3   91061   91051  0  80   0 -   628 ptrace 08:32 pts/3    00:00:00 /tmp/a.out
```

也就是这两个的区别:
```c
#define JOBCTL_STOPPED_BIT	26	/* do_signal_stop() */
#define JOBCTL_TRACED_BIT	27	/* ptrace_stop() */
```


## 问题

```c
#define TASK_IDLE			(TASK_UNINTERRUPTIBLE | TASK_NOLOAD)
```
为什么 TASK_UNINTERRUPTIBLE 和 TASK_NOLOAD 组合起来叫做 TASK_IDLE
或者说，为什么 TASK_IDLE 非要 TASK_UNINTERRUPTIBLE

## [ ] 阅读一下这个： The Unix process API is unreliable and unsafe
https://news.ycombinator.com/item?id=35264487

#### 2.1 Process State
As its name implies, the state field of the process descriptor describes what is currently happening to the process. It consists of an array of flags, each of which
describes a possible process state. In the current Linux version, these states are mutually exclusive, and **hence exactly one flag of state always is set**; the remaining flags
are cleared. The following are the possible process states:

- TASK_RUNNING

The process is either executing on a CPU or waiting to be executed.

- TASK_INTERRUPTIBLE

**The process is suspended (sleeping) until some condition becomes true.** Raising
a hardware interrupt, releasing a system resource the process is waiting for, or
delivering a signal are examples of conditions that might wake up the process
(put its state back to TASK_RUNNING).

- TASK_UNINTERRUPTIBLE

Like TASK_INTERRUPTIBLE, except that delivering a signal to the sleeping process
leaves its state unchanged. This process state is seldom used. It is valuable, however, under certain specific conditions in which a process must wait until a given
event occurs without being interrupted. For instance, this state may be used
when a process opens a device file and the corresponding device driver starts
probing for a corresponding hardware device. The device driver must not be
interrupted until the probing is complete, or the hardware device could be left in
an unpredictable state.

- TASK_STOPPED

Process execution has been stopped; the process enters this state after receiving a
SIGSTOP, SIGTSTP, SIGTTIN, or SIGTTOU signal.

- TASK_TRACED

Process execution has been stopped by a debugger. When a process is being monitored by another (such as when a debugger executes a ptrace() system call to
monitor a test program), each signal may put the process in the TASK_TRACED state.

Two additional states of the process can be stored both in the state field and in the
`exit_state` field of the process descriptor; as the field name suggests, a process
reaches one of these two states only when its execution is terminated:

- EXIT_ZOMBIE

Process execution is terminated, but the parent process has not yet issued a
`wait4()` or `waitpid()` system call to return information about the dead process.
Before the `wait()`-like call is issued, the kernel cannot discard the data contained in the dead process descriptor because the parent might need it. (See the
section “Process Removal” near the end of this chapter.)

- EXIT_DEAD

The final state: the process is being removed by the system because the parent
process has just issued a `wait4()` or `waitpid()` system call for it. Changing its
state from `EXIT_ZOMBIE` to `EXIT_DEAD` avoids race conditions due to other threads
of execution that execute `wait()`-like calls on the same process (see Chapter 5).

> @todo 似乎的确是这个 flag, 但是实际上存在更多的 flags

```c
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
```


#### (process) task 不可以被 interruptible 是什么意思 ? 当其运行的时候，屏蔽所有的中断吗 ?

```c
/* set task’s state to interruptible sleep */
set_current_state(TASK_INTERRUPTIBLE)
```

## 会看 ps -elf 的结果都是很不错的

例如，ps -elf 可以看一个程序到底用了多少 CPU 时间

## 这个整理到 放到 process state 中去
- https://awakening-fong.github.io/posts/scheduler/scheduler_01_lost_wake-up/
- https://www.linuxjournal.com/article/8144

## kthread 休眠的时候的状态是 I
<!-- 63c75918-57b8-402c-b624-82038bda6f61 -->

```txt
0 S martins3  389867    4921  0  80   0 -   469 wait_w 21:55 pts/5    00:00:00 -zsh
1 I root      390410       2  0  80   0 -     0 -      21:56 ?        00:00:00 [kworker/5:1-mm_percpu_wq]
1 I root      390620       2  0  80   0 -     0 -      22:00 ?        00:00:00 [kworker/u193:3-hclge]
1 I root      390691       2  0  80   0 -     0 -      22:02 ?        00:00:00 [kworker/u194:2-events_unbound]
1 I root      390949       2  0  80   0 -     0 -      22:03 ?        00:00:00 [kworker/u192:1-hinic3_nic_dev_wq]
1 I root      390963       2  0  80   0 -     0 -      22:03 ?        00:00:00 [kworker/7:1-mm_percpu_wq]
1 I root      390984       2  0  80   0 -     0 -      22:03 ?        00:00:00 [kworker/10:0]
```

```txt
 cat /proc/self/status
Name:   cat
Umask:  0022
State:  R (running)
```

- proc_pid_status
  - task_state
    - get_task_state
      - task_state_index


一共就是这些状态:
```c
/*
 * The task state array is a strange "bitmap" of
 * reasons to sleep. Thus "running" is zero, and
 * you can test for combinations of others with
 * simple bit tests.
 */
static const char * const task_state_array[] = {

	/* states in TASK_REPORT: */
	"R (running)",		/* 0x00 */
	"S (sleeping)",		/* 0x01 */
	"D (disk sleep)",	/* 0x02 */
	"T (stopped)",		/* 0x04 */
	"t (tracing stop)",	/* 0x08 */
	"X (dead)",		/* 0x10 */
	"Z (zombie)",		/* 0x20 */
	"P (parked)",		/* 0x40 */

	/* states beyond TASK_REPORT: */
	"I (idle)",		/* 0x80 */
};
```

```c
/* Used in tsk->__state: */
#define TASK_RUNNING			0x00000000
#define TASK_INTERRUPTIBLE		0x00000001
#define TASK_UNINTERRUPTIBLE		0x00000002
#define __TASK_STOPPED			0x00000004
#define __TASK_TRACED			0x00000008
/* Used in tsk->exit_state: */
#define EXIT_DEAD			0x00000010
#define EXIT_ZOMBIE			0x00000020
#define EXIT_TRACE			(EXIT_ZOMBIE | EXIT_DEAD)
/* Used in tsk->__state again: */
#define TASK_PARKED			0x00000040
#define TASK_DEAD			0x00000080
```

这些就是可以被 report 的
存在了可以被 report 之外的 status ，那么配置就是 I
```c
#define TASK_REPORT			(TASK_RUNNING | TASK_INTERRUPTIBLE | \
					 TASK_UNINTERRUPTIBLE | __TASK_STOPPED | \
					 __TASK_TRACED | EXIT_DEAD | EXIT_ZOMBIE | \
					 TASK_PARKED)
```
可以写一个程序测试一下，kthread 额外设置了什么，不过兴趣不大了。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
