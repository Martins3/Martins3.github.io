# 分析下 process 的状态

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

#define task_is_running(task)		(READ_ONCE((task)->__state) == TASK_RUNNING)

#define task_is_traced(task)		((READ_ONCE(task->jobctl) & JOBCTL_TRACED) != 0)
#define task_is_stopped(task)		((READ_ONCE(task->jobctl) & JOBCTL_STOPPED) != 0)
#define task_is_stopped_or_traced(task)	((READ_ONCE(task->jobctl) & (JOBCTL_STOPPED | JOBCTL_TRACED)) != 0)
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

> Kernel code which uses interruptible sleeps must always check to see whether it woke up as a result of a signal, and, if so, clean up whatever it was doing and return -EINTR back to user space. The user-space side, too, must realize that a system call was interrupted and respond accordingly; not all user-space programmers are known for their diligence in this regard. Making a sleep uninterruptible eliminates these problems, but at the cost of being, well, uninterruptible. If the expected wakeup event does not materialize, the process will wait forever and there is usually nothing that anybody can do about it short of rebooting the system. This is the source of the dreaded, unkillable process which is shown to be in the "D" state by ps.

这个总结的太好了。

所以这个组合非常合理。
```c
/* Convenience macros for the sake of set_current_state: */
#define TASK_KILLABLE			(TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
```

## `__TASK_STOPPED`
程序接受到了 stop 信号。


## 问题

```c
#define TASK_IDLE			(TASK_UNINTERRUPTIBLE | TASK_NOLOAD)
```
为什么 TASK_UNINTERRUPTIBLE 和 TASK_NOLOAD 组合起来叫做 TASK_IDLE
或者说，为什么 TASK_IDLE 非要 TASK_UNINTERRUPTIBLE
