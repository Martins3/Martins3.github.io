# kernel/signal.md
**WHEN COMING BACK** : : 实际上，job control 比想想的更加简单，将 tlpi 34 的内容好好看看, 总结总结，顺便再去搞定 setsid 的问题

还是基于 arm64 架构吧!

首先阅读一下 man signal(7)

 A  process can change the disposition of a signal using sigaction(2) or signal(2).

- [x] By default, a signal handler is invoked on the normal process stack.  It is possible to arrange that the signal handler uses an alternate stack; see sigaltstack(2) for a discussion of how to do this and when it might be useful.
  - 只是修改一下执行的时候使用的 stack

- [ ] 如果 invalid 的 pagefault 发生了，如果导致 process 被 killed, 这个流程分析一下

- [ ] sigret 的工作原理
- [x] 一个进程在什么时候检查自己的信号(执行 signal handler)
  - [x] syscall 返回的时候 ? (不是，int / syscall 返回到用户态的时候)
      - 如果这是真的，那么 SA_RESTART 的作用无法解释，syscall 都返回了，怎么可能被 handler 打断 
        - 此时解释 syscall_restart 的理由就是，其实有的系统调用，在执行的会检查 signal_pending, 如果发生了 signal_pending, 会提前结束。
      - 通过检查 x86 的 signal.c 发生检查的位置在 	exit_to_user_mode_prepare 的调用下
      - arm64 的代码在 ret_to_user 的时候检查, 这一点是统一的

- do_notify_resume : 从目前的分析看，signal handler 的处理是在 syscall return 的时候，将返回的 pt_regs 修改
  - do_signal : TODO 需要处理一些 syscall restart 的东西
    - get_signal : 如果一个信号的 handler 只是内核的默认操作，那么无需切换到用户态，在此处进行操作即可
      - do_signal_stop
    - handle_signal : OK, we're invoking a handler
      - setup_rt_frame
        - get_sigframe
          - sigsp
        - setup_sigframe
        - setup_return
        - copy_siginfo_to_user
      - signal_setup_done

- [ ] clone(2) 中间处理信号的原则
  - [ ] thread group 中间，谁处理信号
  - [ ] 能不能只是 kill 一个 thread
    - tgkill
  - [ ] 能不能 kill process group 中间的一个 process, 如何 kill 所有的 process
    - killpg
  - 其实处理信号的基本在于: thread group 共享的 pending 和 私有的 pending
    - [ ] 信号应该是不可能那个被 thread group 的所有 thread 都接受一次，因为 pending 维护的是一个 bitmask, 处理完成之后就清空了
    - process group 让所有的信号接受一遍靠的是，在所有的 process 的 pending 上插入 flags

- [x] sigwaitinfo(2), sigtimedwait(2), and sigwait(3) suspend execution until one of the signals in a specified set is delivered.  Each of these calls returns information about the delivered signal.
  - [x] 所以，和 pause 和 sigsuspend 有什么区别吗 ? 应该只是一个等待所有信号，一个是针对的等待一个信号吧!
  - sigsuspend 会首先修改 mask, 然后睡眠，直到信号发生并且被处理之后，其就会返回
    - 没有 sigsuspend，这个功能: 在屏蔽特定的信号，并且执行完成之后，需要将遗失的信号执行一下, 是没有办法实现的
    - 如果 sigsuspend 没有收到信号，会一致睡眠，但是其实故意注入一个信号。
  - pause 不考虑 blocked signal
  - sigwaitinfo / sigtimedwait 如果存在 blocked signal, 可以立刻返回，否则等待这些信号
  - [ ] 从设想上，三者的实现应该很相似，但是实际上，sigtimedwait 的实现差别很大，而且另外两个也是很迷糊的

- [ ] A child created via fork(2) inherits a copy of its parent's signal mask; the signal mask is preserved across execve(2).
  - [ ] 都改朝换代了，为什么还要保持 signal mask

- 才发现 mask 的含义就是 block 

- [x] 被 mask 的信号，当 unmask 之后，还可以处理，还是永久的丢失了 ?
  - Standard  signals  do  not  queue.  If multiple instances of a standard signal are generated while that signal is blocked, then only one instance of the signal is marked as pending (and the signal will be delivered just once when it is un‐
blocked).  In the case where a standard signal is already pending, the siginfo_t structure (see sigaction(2)) associated with that signal is not overwritten on arrival of subsequent instances of the same signal.  Thus, the process will re‐
ceive the information associated with the first instance of the signal.
  - standard signal : 收到的信号，不会丢失，但是只会处理第一个，其余的都要丢弃了
  - rt signal : 收到多少个信号，取消 block 之后，就需要处理多少次

Unlike standard signals, real-time signals have no predefined meanings: the entire set of real-time signals can be used for application-defined purposes.

The default action for an unhandled real-time signal is to terminate the receiving process.

Real-time signals are distinguished by the following:
1. Multiple instances of real-time signals can be queued.
2. If the signal is sent using sigqueue(3), an accompanying value (either an integer or a pointer) can be sent with the signal.
3. Real-time signals are delivered in a guaranteed order.

- [x] real-time signals 的功能并不是体现在 real-time 上，而是在一些 specification 加入了一些东西
  - [x] 阅读一下 rt 和代码 和 standard 的代码，两者如何复用代码的 ?

 | Linux 2.0 and earlier | Linux 2.2 and later | desc |
 |-----------------------|---------------------|------|
 | sigaction(2)          | rt_sigaction(2)     | 设置 handler     |
 | sigpending(2)         | rt_sigpending(2)    |  获取处于 pending 的信号    |
 | sigprocmask(2)        | rt_sigprocmask(2)   | 确定那些信号被关注   |
 | sigreturn(2)          | rt_sigreturn(2)     |      |
 | sigsuspend(2)         | rt_sigsuspend(2)    |      |
 | sigtimedwait(2)       | rt_sigtimedwait(2)  |  等待该信号的发生    |

 关于 rt 和 standard 的区别, 从
> Man sigpending(2)
>
> The  original  Linux  system  call was named sigpending().  However, with the addition of real-time signals in Linux 2.2, the fixed-size, 32-bit sigset_t argument supported by that system call was no longer fit for purpose.  Consequently, a new system call, rt_sigpend‐
> ing(), was added to support an enlarged sigset_t type.  The new system call takes a second argument, size_t sigsetsize, which specifies the size in bytes of the signal set in set.  The glibc sigpending() wrapper function hides these details from us, transparently call‐
> ing rt_sigpending() when the kernel provides it.

- [ ] 存在几个无法被篡改的，标准的 sigaction, 分析其 handler
  - SIGCHIL
  - SIGBUS


## KeyNote
1. sigset_t : 64bit 下就是 unsigned long
- [x] kernel_siginfo 的作用是什么 ? 参考 man sigaction(2)

```c
typedef struct kernel_siginfo {
	__SIGINFO;
} kernel_siginfo_t;

struct {				\
	int si_signo;			\
	int si_errno;			\
	int si_code;			\
	union __sifields _sifields;	\ 
}

/*
 * How these fields are to be accessed.
 */
#define si_pid		_sifields._kill._pid
#define si_uid		_sifields._kill._uid
#define si_tid		_sifields._timer._tid
#define si_overrun	_sifields._timer._overrun
#define si_sys_private  _sifields._timer._sys_private
#define si_status	_sifields._sigchld._status
#define si_utime	_sifields._sigchld._utime
#define si_stime	_sifields._sigchld._stime
#define si_value	_sifields._rt._sigval
#define si_int		_sifields._rt._sigval.sival_int
#define si_ptr		_sifields._rt._sigval.sival_ptr
#define si_addr		_sifields._sigfault._addr
#ifdef __ARCH_SI_TRAPNO
#define si_trapno	_sifields._sigfault._trapno
#endif
#define si_addr_lsb	_sifields._sigfault._addr_lsb
#define si_lower	_sifields._sigfault._addr_bnd._lower
#define si_upper	_sifields._sigfault._addr_bnd._upper
#define si_pkey		_sifields._sigfault._addr_pkey._pkey
#define si_band		_sifields._sigpoll._band
#define si_fd		_sifields._sigpoll._fd
#define si_call_addr	_sifields._sigsys._call_addr
#define si_syscall	_sifields._sigsys._syscall
#define si_arch		_sifields._sigsys._arch
```

## core struct

```c
struct task_struct {
  // ...
	/* Signal handlers: */
	struct signal_struct		*signal;
	struct sighand_struct __rcu		*sighand;
	sigset_t			blocked; // TODO 这两个的作用，以及
	sigset_t			real_blocked;
	/* Restored if set_restore_sigmask() was used: */
	sigset_t			saved_sigmask;
	struct sigpending		pending;
	unsigned long			sas_ss_sp;
	size_t				sas_ss_size;
	unsigned int			sas_ss_flags;
  // ...
}

/*
 * Types defining task->signal and task->sighand and APIs using them:
 */
struct sighand_struct {
	spinlock_t		siglock;
	refcount_t		count;
	wait_queue_head_t	signalfd_wqh;
	struct k_sigaction	action[_NSIG];
};


struct sigpending {
	struct list_head list;
	sigset_t signal;
};

struct sigqueue {
	struct list_head list; // 利用此成员挂到 sigpending->list 上
	int flags;
	kernel_siginfo_t info;
	struct user_struct *user;
};

typedef struct kernel_siginfo {
	__SIGINFO;
} kernel_siginfo_t;


/*
 * NOTE! "signal_struct" does not have its own
 * locking, because a shared signal_struct always
 * implies a shared sighand_struct, so locking
 * sighand_struct is always a proper superset of
 * the locking of signal_struct.
 */
struct signal_struct { // todo 太复杂了啊!
```

- [x] calculate_sigpending : Have any signals or users of TIF_SIGPENDING been delayed until after fork?



```c
/*
 * Let a parent know about the death of a child.
 * For a stopped/continued status change, use do_notify_parent_cldstop instead.
 *
 * Returns true if our parent ignored us and so we've switched to
 * self-reaping.
 */
bool do_notify_parent(struct task_struct *tsk, int sig)
```
- kill_pgrp : 被 tty driver 疯狂调用(tty, 永远的痛)
  - `__kill_pgrp_info` : 对于 process group 的循环调用
    - group_send_sig_info 
      - do_send_sig_info
        - do_send_sig_info
          - send_signal

flush : 只是一些表示 pending 上的信号清理掉
- flush_sigqueue_mask:780   
- flush_signal_handlers:539 
- `__flush_itimer_signals`:489
- flush_sigqueue:461        
- flush_signals:476         
- flush_itimer_signals:512  

## send_sig : 发送信号
- send_sig
  - send_sig_info
    - do_send_sig_info
      - send_signal
        - `send_signal`
          - `__sigqueue_alloc` : 向目标 task_struct 中间 pending 中间加入内容
          - pending = (type != PIDTYPE_PID) ? &t->signal->shared_pending : &t->pending;
          - list_add_tail(&q->list, &pending->list);
          - sigaddset(&pending->signal, sig);

## jobctl

```c
void task_join_group_stop(struct task_struct *task)
{
	/* Have the new thread join an on-going signal group stop */
	unsigned long jobctl = current->jobctl;
	if (jobctl & JOBCTL_STOP_PENDING) {
		struct signal_struct *sig = current->signal;
		unsigned long signr = jobctl & JOBCTL_STOP_SIGMASK;
		unsigned long gstop = JOBCTL_STOP_PENDING | JOBCTL_STOP_CONSUME;
		if (task_set_jobctl_pending(task, signr | gstop)) {
			sig->group_stop_count++;
		}
	}
}
```

- [ ] task_clear_jobctl_pending


- [ ] thread 被拉入到同一个 thread group 在于 fork 的时候控制，但是 process group 是如何创建起来的
  - [ ] 同样的问题也在于 session group，我怎么知道新创建的 process 是否在另一个 session ?

/home/maritns3/core/arm64-linux/include/linux/sched/jobctl.h

- [ ] `task->jobctl`

```c
bool task_set_jobctl_pending(struct task_struct *task, unsigned long mask);
void task_clear_jobctl_trapping(struct task_struct *task);
void task_clear_jobctl_pending(struct task_struct *task, unsigned long mask);
```

```c
#define JOBCTL_STOP_DEQUEUED_BIT 16	/* stop signal dequeued */
#define JOBCTL_STOP_PENDING_BIT	17	/* task should stop for group stop */
#define JOBCTL_STOP_CONSUME_BIT	18	/* consume group stop count */
#define JOBCTL_TRAP_STOP_BIT	19	/* trap for STOP */
#define JOBCTL_TRAP_NOTIFY_BIT	20	/* trap for NOTIFY */
#define JOBCTL_TRAPPING_BIT	21	/* switching to TRACED */
#define JOBCTL_LISTENING_BIT	22	/* ptracer is listening for events */
#define JOBCTL_TRAP_FREEZE_BIT	23	/* trap for cgroup freezer */
```
- [ ] 似乎 三个 STOP 就是用于 group stop 的

- [ ] task_participate_group_stop

- task_join_group_stop : 当成为 thread group 的一员的时候， group_stop_count 和 JOBCTL_STOP_CONSUME


```c
static int ptrace_attach(struct task_struct *task, long request,
			 unsigned long addr,
			 unsigned long flags)
{
  // ...
	spin_lock(&task->sighand->siglock);

	/*
	 * If the task is already STOPPED, set JOBCTL_TRAP_STOP and
	 * TRAPPING, and kick it so that it transits to TRACED.  TRAPPING
	 * will be cleared if the child completes the transition or any
	 * event which clears the group stop states happens.  We'll wait
	 * for the transition to complete before returning from this
	 * function.
	 *
	 * This hides STOPPED -> RUNNING -> TRACED transition from the
	 * attaching thread but a different thread in the same group can
	 * still observe the transient RUNNING state.  IOW, if another
	 * thread's WNOHANG wait(2) on the stopped tracee races against
	 * ATTACH, the wait(2) may fail due to the transient RUNNING.
	 *
	 * The following task_is_stopped() test is safe as both transitions
	 * in and out of STOPPED are protected by siglock.
	 */
	if (task_is_stopped(task) &&
	    task_set_jobctl_pending(task, JOBCTL_TRAP_STOP | JOBCTL_TRAPPING))
		signal_wake_up_state(task, __TASK_STOPPED);

	spin_unlock(&task->sighand->siglock);
```

- [ ] ptrace 似乎存在一个问题在于，如果 attach 的时候，这个进程本身就是被 STOP 的


```c
static void ptrace_stop(int exit_code, int why, int clear_code, kernel_siginfo_t *info)
	__releases(&current->sighand->siglock)
	__acquires(&current->sighand->siglock)

	set_special_state(TASK_TRACED); // TODO 这是啥啊 ？

	/*
	 * We're committing to trapping.  TRACED should be visible before
	 * TRAPPING is cleared; otherwise, the tracer might fail do_wait().
	 * Also, transition to TRACED and updates to ->jobctl should be
	 * atomic with respect to siglock and should be done after the arch
	 * hook as siglock is released and regrabbed across it.
	 *
	 *     TRACER				    TRACEE
	 *
	 *     ptrace_attach()
	 * [L]   wait_on_bit(JOBCTL_TRAPPING)	[S] set_special_state(TRACED)
	 *     do_wait()
	 *       set_current_state()                smp_wmb();
	 *       ptrace_do_wait()
	 *         wait_task_stopped()
	 *           task_stopped_code()
	 * [L]         task_is_traced()		[S] task_clear_jobctl_trapping();
	 */
	smp_wmb();
```

```c
static void ptrace_unfreeze_traced(struct task_struct *task)
{
	if (task->state != __TASK_TRACED)
		return;

	WARN_ON(!task->ptrace || task->parent != current);

	/*
	 * PTRACE_LISTEN can allow ptrace_trap_notify to wake us up remotely.
	 * Recheck state under the lock to close this race.
	 */
	spin_lock_irq(&task->sighand->siglock);
	if (task->state == __TASK_TRACED) {
		if (__fatal_signal_pending(task))
			wake_up_state(task, __TASK_TRACED);
		else
			task->state = TASK_TRACED;
	}
	spin_unlock_irq(&task->sighand->siglock);
}
```

```

       PTRACE_ATTACH
              Attach to the process specified in pid, making it a tracee of the calling process.  The tracee is sent a SIGSTOP, but  will
              not  necessarily  have stopped by the completion of this call; use waitpid(2) to wait for the tracee to stop.  See the "At‐
              taching and detaching" subsection for additional information.  (addr and data are ignored.)

              Permission to perform a PTRACE_ATTACH is governed by a ptrace access mode PTRACE_MODE_ATTACH_REALCREDS check; see below.

       PTRACE_SEIZE (since Linux 3.4)
              Attach to the process specified in pid, making it a tracee of the calling process.  Unlike PTRACE_ATTACH, PTRACE_SEIZE does
              not  stop  the process.  Group-stops are reported as PTRACE_EVENT_STOP and WSTOPSIG(status) returns the stop signal.  Auto‐
              matically attached children stop with PTRACE_EVENT_STOP and WSTOPSIG(status) returns SIGTRAP instead of having SIGSTOP sig‐
              nal delivered to them.  execve(2) does not deliver an extra SIGTRAP.  Only a PTRACE_SEIZEd process can accept PTRACE_INTER‐
              RUPT and PTRACE_LISTEN commands.  The "seized" behavior just described is inherited by children that are automatically  at‐
              tached  using  PTRACE_O_TRACEFORK,  PTRACE_O_TRACEVFORK,  and PTRACE_O_TRACECLONE.  addr must be zero.  data contains a bit
              mask of ptrace options to activate immediately.

              Permission to perform a PTRACE_SEIZE is governed by a ptrace access mode PTRACE_MODE_ATTACH_REALCREDS check; see below.

       PTRACE_ATTACH  sends  SIGSTOP  to this thread.  If the tracer wants this SIGSTOP to have no effect, it needs to suppress it.  Note
       that if other signals are concurrently sent to this thread during attach, the tracer may see the tracee enter signal-delivery-stop
       with  other  signal(s) first!  The usual practice is to reinject these signals until SIGSTOP is seen, then suppress SIGSTOP injec‐
       tion.  The design bug here is that a ptrace attach and a concurrently delivered SIGSTOP may race and the concurrent SIGSTOP may be
       lost.

```
Only a PTRACE_SEIZEd process can accept PTRACE_INTERRUPT and PTRACE_LISTEN commands.  The "seized" behavior just described is inherited by children that are automatically  at‐

- [ ] PTRACE_ATTACH 会自动向 tracee 发送一个 SIGSTOP，但是如果此时，tracee 接受到了来自于其他来源的 SIGSTOP，可能其会忽视。


       Since attaching sends SIGSTOP and the tracer usually suppresses it, this may cause a stray EINTR return from the currently execut‐
       ing system call in the tracee, as described in the "Signal injection and suppression" section.

- [ ] 所以，为什么要进行 suppress signal 啊


Since Linux 3.4, PTRACE_SEIZE can be used instead of PTRACE_ATTACH.  PTRACE_SEIZE does not stop the attached process.  If you need
to stop it after attach (or at any other time) without sending it any signals, use **PTRACE_INTERRUPT** command.


       where cmd is PTRACE_CONT, PTRACE_LISTEN, PTRACE_DETACH, PTRACE_SYSCALL, PTRACE_SINGLESTEP,  PTRACE_SYSEMU,  or  PTRACE_SYSEMU_SIN‐
       GLESTEP.   If  the  tracee is in signal-delivery-stop, sig is the signal to be injected (if it is nonzero).  Otherwise, sig may be
       ignored.  (When restarting a tracee from a ptrace-stop other than signal-delivery-stop, recommended practice is to always  pass  0
       in sig.)

- [ ] 还是很奇怪的机制，当我被 STOP 了，然后首先向 tracer 发送信号，tracer 发送了信号之后，才可以让其他的 thread 停止 ？                                           
  - [ ] 是发送信号，还是首先 STOP 吗 ?

- [ ] https://www.cnblogs.com/mysky007/p/11047943.html : ptrace 的更加高级应用

[Daemons normally disassociate themselves from any controlling terminal by creating a new session without one](https://stackoverflow.com/questions/6548823/use-and-meaning-of-session-and-process-group-in-unix)

https://stackoverflow.com/questions/9305992/if-threads-share-the-same-pid-how-can-they-be-identified
课代表总结：
用户看到的是tgid: thread group id
kernel看到的是pid
