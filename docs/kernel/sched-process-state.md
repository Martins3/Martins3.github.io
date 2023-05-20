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

![](https://img2018.cnblogs.com/blog/1771657/202002/1771657-20200201170358218-1930669459.png)

- [x] what's difference with `TASK_UNINTERRUPTIBLE` and `TASK_INTERRUPTIBLE` ?
  - https://stackoverflow.com/questions/223644/what-is-an-uninterruptible-process
  - https://lwn.net/Articles/288056/

> A process which is placed in the TASK_INTERRUPTIBLE state will sleep until either (1) something explicitly wakes it up, or (2) a non-masked signal is received. The TASK_UNINTERRUPTIBLE state, instead, ignores signals; processes in that state will require an explicit wakeup before they can run again.

> so Matthew created a new sleeping state, called TASK_KILLABLE; it behaves like TASK_UNINTERRUPTIBLE with the exception that fatal signals will interrupt the sleep.

- [ ] find a example to understand the difference between `TASK_UNINTERRUPTIBLE` and `TASK_INTERRUPTIBLE`
  - [ ] Not all syscall will lead to `TASK_UNINTERRUPTIBLE`, find a example to lead to `TASK_INTERRUPTIBLE` and it has to do that.

- TASK_PARKED : 主要是给 kthread 使用，处于该状态的时候，表示 `__kthread_parkme`，其中一直睡眠，然后等待其他人唤醒。
