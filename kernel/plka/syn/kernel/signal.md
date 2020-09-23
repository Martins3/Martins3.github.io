# kernel/signal.md


## KeyNote
1. sigset_t : 64bit 下就是 unsigned long
2. kernel_siginfo 的作用是什么 ?

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

// __sifields 中间的内容非常的多，令人窒息 !

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

## TODO
1. process 处理信号的时机是什么 ?
2. 什么时候信号是 shared ，什么时候是 private 的 ?

## question
1. handler 默认执行的 stack space 位置在于何处 ?
2. 能不能在不同的 namespace 中间实现切换 ?
3. task->signal_struct 的作用是什么 ? 为什么会有那么多内容 ?
4. 既然 handler 是在用户层次执行的，那么在切换到用户空间执行的 ? 至少需要知道 注册的函数的跳转地址是什么吧!
5. 那些默认的 handler 都是在哪里 ? 
7. 信号是内核态接受的，是如何切换到用户态空间，执行用户程序的

## principal
1. handler 在 kernel 中间注册，但是执行的位置在 user space
2. 文档 在 413
3. signal 种类都是固定的，只是有的可以自定义 handler 而已


## core struct

```c
struct task_struct {
  // ...
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


## ptrace

Usually, a large case structure that deals separately with each case (depending on the request
parameter) is employed for this purpose. I discuss only some important cases: `PTRACE_ATTACH` and
`PTRACE_DETACH`, `PTRACE_SYSCALL`, `PTRACE_CONT` as well as `PTRACE_PEEKDATA` and `PTRACE_POKEDATA`. 
The implementation of the remaining requests follows a similar pattern.

*All further tracing actions performed by the kernel are present in the signal handler code discussed in
Chapter 5*. When a signal is delivered, the kernel checks whether the `PT_TRACED` flag is set in the `ptrace`
field of `task_struct`. If it is, the state of the process is set to `TASK_STOPPED` (in `get_signal_to_deliver`
in `kernel/signal.c`) in order to interrupt execution. `notify_parent` with the `CHLD` signal is then used
to inform the tracer process. (The tracer process is woken up if it happens to be sleeping.) The tracer
process then performs the desired checks on the target process as specified by the remaining ptrace
options.

> 信号来自于 architecture 相关的代码: 比如 strace 的syscall 的，当设置 TIF 上的相关的flags，然后触发


1. 如何 install 一个 handler
2. 谁可以给 谁发送 消息 ? 没有限制的后果是什么 ? 
3. 一个正在执行的程序，靠检测什么东西来判断自己是否接收到了消息 ?
    1. 一个由于等待 signal 或者 各种蛇皮，sleep 之后，谁来唤醒我 ?
        1. 可能是 parent ?
        2. thread group leader ?
        3. init ?
        4. everyone ?

    2.
  
3. 接受到消息之后，处理消息的时机是什么 ?

4. 同时接收到多个消息，但是消息的处理只能一个个处理 ? 还是对于后面的进行忽视 ?
5. handler 能不能继承，当 fork 的时候 ?


## misc

```c
static void __user *sig_handler(struct task_struct *t, int sig)
{
	return t->sighand->action[sig - 1].sa.sa_handler;
}
```

## calculate_sigpending

## jobctl

tlpi chapter 34

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

## architecture
x86 提供的帮助是什么 ?

## flush

## send_signal
处理的各种业务:
1. queue 
2. 

## 向整个 group 发送信号

```c
int
__group_send_sig_info(int sig, struct siginfo *info, struct task_struct *p)
{
	return send_signal(sig, info, p, PIDTYPE_TGID);
}


static int
specific_send_sig_info(int sig, struct siginfo *info, struct task_struct *t)
{
	return send_signal(sig, info, t, PIDTYPE_PID);
}


/*
 * Nuke all other threads in the group.
 */
int zap_other_threads(struct task_struct *p)
{
	struct task_struct *t = p;
	int count = 0;

	p->signal->group_stop_count = 0;

	while_each_thread(p, t) {
		task_clear_jobctl_pending(t, JOBCTL_PENDING_MASK);
		count++;

		/* Don't bother with already dead threads */
		if (t->exit_state)
			continue;
		sigaddset(&t->pending.signal, SIGKILL);
		signal_wake_up(t, 1);
	}

	return count;
}


/*
 * send signal info to all the members of a group
 */
int group_send_sig_info(int sig, struct siginfo *info, struct task_struct *p,
			enum pid_type type)
{
	int ret;

	rcu_read_lock();
	ret = check_kill_permission(sig, info, p);
	rcu_read_unlock();

	if (!ret && sig)
		ret = do_send_sig_info(sig, info, p, type);

	return ret;
}
```

## force_sig_info


## cred


## kill_info 机制
1500 行左右的内容


## send_sigqueue

```c
int send_sigqueue(struct sigqueue *q, struct pid *pid, enum pid_type type)
```


## notify_parent

> 其实应该分析的内容叫做 ptrace 机制

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

## API : kill

```c
/**
 *  sys_kill - send a signal to a process
 *  @pid: the PID of the process
 *  @sig: signal to be sent
 */
SYSCALL_DEFINE2(kill, pid_t, pid, int, sig)
{
	struct kernel_siginfo info;

	prepare_kill_siginfo(sig, &info);

	return kill_something_info(sig, &info, pid); // todo
}
```


## API : sigaction
0. sigaction : Man @todo 没有看懂!
1. sigaction 的 syscall 接口一共存在四个，没有仔细分析
```c
int do_sigaction(int sig, struct k_sigaction *act, struct k_sigaction *oact)
{
	struct task_struct *p = current, *t;
	struct k_sigaction *k;
	sigset_t mask;

	if (!valid_signal(sig) || sig < 1 || (act && sig_kernel_only(sig)))
		return -EINVAL;

	k = &p->sighand->action[sig-1];

	spin_lock_irq(&p->sighand->siglock);
	if (oact)
		*oact = *k; // 简单的拷贝

	sigaction_compat_abi(act, oact);

	if (act) {
		sigdelsetmask(&act->sa.sa_mask,
			      sigmask(SIGKILL) | sigmask(SIGSTOP));
		*k = *act; // 还是简单的拷贝
		/*
		 * POSIX 3.3.1.3:
		 *  "Setting a signal action to SIG_IGN for a signal that is
		 *   pending shall cause the pending signal to be discarded,
		 *   whether or not it is blocked."
		 *
		 *  "Setting a signal action to SIG_DFL for a signal that is
		 *   pending and whose default action is to ignore the signal
		 *   (for example, SIGCHLD), shall cause the pending signal to
		 *   be discarded, whether or not it is blocked"
		 */
		if (sig_handler_ignored(sig_handler(p, sig), sig)) { // 当这个handler 被注册为该信号被忽略，那么需要清理掉信号
			sigemptyset(&mask); 
			sigaddset(&mask, sig);
			flush_sigqueue_mask(&mask, &p->signal->shared_pending);
			for_each_thread(p, t)
				flush_sigqueue_mask(&mask, &t->pending); // todo 居然存在两个 pending，有意思
		}
	}

	spin_unlock_irq(&p->sighand->siglock);
	return 0;
}
```
2. 


## flush : 辅助理解

```c
/*
 * Remove signals in mask from the pending set and queue.
 * Returns 1 if any signals were found.
 *
 * All callers must be holding the siglock.
 */
static void flush_sigqueue_mask(sigset_t *mask, struct sigpending *s)
{
	struct sigqueue *q, *n;
	sigset_t m;

	sigandsets(&m, mask, &s->signal); // 判断当前是否存在 pending
	if (sigisemptyset(&m))
		return;

	sigandnsets(&s->signal, &s->signal, mask); // 将该 bit 上清理掉
	list_for_each_entry_safe(q, n, &s->list, list) {
		if (sigismember(mask, q->info.si_signo)) { // 这个 pending 的 signal 就是 mask 上的 
			list_del_init(&q->list);
			__sigqueue_free(q); // 有意思
		}
	}
}
```

## send_sig : 发送信号

```c
int
send_sig(int sig, struct task_struct *p, int priv)
{
	return send_sig_info(sig, __si_special(priv), p);
}

#define __si_special(priv) \
	((priv) ? SEND_SIG_PRIV : SEND_SIG_NOINFO)

/*
 * These are for backward compatibility with the rest of the kernel source.
 */
int send_sig_info(int sig, struct kernel_siginfo *info, struct task_struct *p)
{
	/*
	 * Make sure legacy kernel users don't send in bad values
	 * (normal paths check this in check_kill_permission).
	 */
	if (!valid_signal(sig))  // 测试 sig 数值范围要求
		return -EINVAL;

	return do_send_sig_info(sig, info, p, PIDTYPE_PID);
}

int do_send_sig_info(int sig, struct kernel_siginfo *info, struct task_struct *p, // lock 封装
			enum pid_type type)
{
	unsigned long flags;
	int ret = -ESRCH;

	if (lock_task_sighand(p, &flags)) {
		ret = send_signal(sig, info, p, type);
		unlock_task_sighand(p, &flags);
	}

	return ret;
}

static int send_signal(int sig, struct kernel_siginfo *info, struct task_struct *t,
			enum pid_type type)
{
	/* Should SIGKILL or SIGSTOP be received by a pid namespace init? */
	bool force = false;

	if (info == SEND_SIG_NOINFO) {
		/* Force if sent from an ancestor pid namespace */
		force = !task_pid_nr_ns(current, task_active_pid_ns(t));
	} else if (info == SEND_SIG_PRIV) {
		/* Don't ignore kernel generated signals */
		force = true;
	} else if (has_si_pid_and_uid(info)) { // todo 虽然不知道是什么意思，猜测是这些 signal 中间持有 pid 和 uid
		/* SIGKILL and SIGSTOP is special or has ids */
		struct user_namespace *t_user_ns;

		rcu_read_lock();
		t_user_ns = task_cred_xxx(t, user_ns); // todo 看不懂
		if (current_user_ns() != t_user_ns) {
			kuid_t uid = make_kuid(current_user_ns(), info->si_uid);
			info->si_uid = from_kuid_munged(t_user_ns, uid);
		}
		rcu_read_unlock();

		/* A kernel generated signal? */
		force = (info->si_code == SI_KERNEL);

		/* From an ancestor pid namespace? */
		if (!task_pid_nr_ns(current, task_active_pid_ns(t))) {
			info->si_pid = 0;
			force = true;
		}
	}
	return __send_signal(sig, info, t, type, force);
}

static int __send_signal(int sig, struct kernel_siginfo *info, struct task_struct *t, // 终极 boss，创建 sigqueue 插入队列，通知一下受害人
			enum pid_type type, bool force)
{
	struct sigpending *pending;
	struct sigqueue *q;
	int override_rlimit;
	int ret = 0, result;

	assert_spin_locked(&t->sighand->siglock);

	result = TRACE_SIGNAL_IGNORED;
	if (!prepare_signal(sig, t, force))
		goto ret;

	pending = (type != PIDTYPE_PID) ? &t->signal->shared_pending : &t->pending;
	/*
	 * Short-circuit ignored signals and support queuing
	 * exactly one non-rt signal, so that we can get more
	 * detailed information about the cause of the signal.
	 */
	result = TRACE_SIGNAL_ALREADY_PENDING;
	if (legacy_queue(pending, sig))
		goto ret;

	result = TRACE_SIGNAL_DELIVERED;
	/*
	 * Skip useless siginfo allocation for SIGKILL and kernel threads.
	 */
	if ((sig == SIGKILL) || (t->flags & PF_KTHREAD))
		goto out_set;

	/*
	 * Real-time signals must be queued if sent by sigqueue, or
	 * some other real-time mechanism.  It is implementation
	 * defined whether kill() does so.  We attempt to do so, on
	 * the principle of least surprise, but since kill is not
	 * allowed to fail with EAGAIN when low on memory we just
	 * make sure at least one signal gets delivered and don't
	 * pass on the info struct.
	 */
	if (sig < SIGRTMIN)
		override_rlimit = (is_si_special(info) || info->si_code >= 0);
	else
		override_rlimit = 0;

	q = __sigqueue_alloc(sig, t, GFP_ATOMIC, override_rlimit); // 创建 sigqueue 
	if (q) {
		list_add_tail(&q->list, &pending->list); // 挂到 pending->list 上
		switch ((unsigned long) info) {
		case (unsigned long) SEND_SIG_NOINFO: // 初始化 sigqueue 的 info
			clear_siginfo(&q->info);
			q->info.si_signo = sig;
			q->info.si_errno = 0;
			q->info.si_code = SI_USER;
			q->info.si_pid = task_tgid_nr_ns(current,
							task_active_pid_ns(t));
			rcu_read_lock();
			q->info.si_uid =
				from_kuid_munged(task_cred_xxx(t, user_ns),
						 current_uid());
			rcu_read_unlock();
			break;
		case (unsigned long) SEND_SIG_PRIV:
			clear_siginfo(&q->info);
			q->info.si_signo = sig;
			q->info.si_errno = 0;
			q->info.si_code = SI_KERNEL;
			q->info.si_pid = 0;
			q->info.si_uid = 0;
			break;
		default:
			copy_siginfo(&q->info, info);
			break;
		}
	} else if (!is_si_special(info) &&
		   sig >= SIGRTMIN && info->si_code != SI_USER) { // todo 这对应什么情况 ?
		/*
		 * Queue overflow, abort.  We may abort if the
		 * signal was rt and sent by user using something
		 * other than kill().
		 */
		result = TRACE_SIGNAL_OVERFLOW_FAIL;
		ret = -EAGAIN;
		goto ret;
	} else {
		/*
		 * This is a silent loss of information.  We still
		 * send the signal, but the *info bits are lost.
		 */
		result = TRACE_SIGNAL_LOSE_INFO;
	}

out_set:
	signalfd_notify(t, sig); // todo
	sigaddset(&pending->signal, sig);

	/* Let multiprocess signals appear after on-going forks */
	if (type > PIDTYPE_TGID) { // todo 
		struct multiprocess_signals *delayed;
		hlist_for_each_entry(delayed, &t->signal->multiprocess, node) {
			sigset_t *signal = &delayed->signal;
			/* Can't queue both a stop and a continue signal */
			if (sig == SIGCONT)
				sigdelsetmask(signal, SIG_KERNEL_STOP_MASK);
			else if (sig_kernel_stop(sig))
				sigdelset(signal, SIGCONT);
			sigaddset(signal, sig);
		}
	}

	complete_signal(sig, t, type); // todo 具体有待分析
ret:
	trace_signal_generate(sig, info, t, type != PIDTYPE_PID, result);
	return ret;
}
```

