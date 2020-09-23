# sys.c
1. pid 
2. rlimit
3. prctl


## KeyNote
- [](https://medium.com/hungys-blog/linux-kernel-process-99629d91423c)

The field parent in task_struct usually matches the process descriptor pointed by real_parent.
- real_parent: Points to the process descriptor of the process that created P or to the descriptor of process 1 (init) if the parent process no longer exists.
- parent: Points to the current parent of P (this is the process that must be signaled when the child process terminates, i.e. SIGCHLD). It may occasionally differ from real_parent in some cases, such as when another process issues a ptrace() system call requesting that it be allowed to monitor P.

## Question
pid.c 中间的疑惑:
1. 为什么将 pid->tasks[PIDTYPE_PID] 总是单独说明 ?
    1. 既然总是被 struct task->thread_pid 替代，为什么不直接将其删除掉 ?
2. 如果 attach 真的是将 task 加入到 pid hlist 的唯一方法，change_pid 的含义到底是什么 ?
    1. 比如 clone ，难道不是自动加入到 thread group 中间吗 ?
3. pid_task 的实现 : 
    1. 由于初始化的时候，第一个才是该 pid 的代表，所以各种，find_task 之类的函数的实现非常科学

4. pidfd 的两个 lwn 值的分析一下 !

5. 一个 task 一共可以持有 5 个 pid 指针 : thread_pid 和 signal
  1. 同时，pid 中间可以持有四种 task

6. 到底为什么 task 需要共享 pid ?
    1. tgid pgid sid 的存在, 之所以存在 PIDTYPE_PID ，是因为 thread_pid 需要使用
    2. @todo 找到加入 thread group 的方法

## prctl : process control 各种蛇皮业务的处理者
A process can set and reset the `keep_capabilities` flag by means of the Linux-specific prctl() system call.

- [](https://unix.stackexchange.com/questions/250153/what-is-a-subreaper-process)

- [](https://stackoverflow.com/questions/284325/how-to-make-child-process-die-after-parent-exits)

## API : get
1. pid 
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
	return task_tgid_vnr(current); // 无非是从 current 中间获取到 struct  pid 和 ns，然后获取到 upid 中间的数值而已
}

/* Thread ID - the internal kernel "pid" */
SYSCALL_DEFINE0(gettid)
{
	return task_pid_vnr(current); // 和上面的变化在于访问的 pid 不同而已
}
/*
 * Accessing ->real_parent is not SMP-safe, it could
 * change from under us. However, we can use a stale
 * value of ->real_parent under rcu_read_lock(), see
 * release_task()->call_rcu(delayed_put_task_struct).
 */
SYSCALL_DEFINE0(getppid)
{
	int pid;

	rcu_read_lock();
	pid = task_tgid_vnr(rcu_dereference(current->real_parent)); // todo 各种 get 函数的访问
	rcu_read_unlock();

	return pid;
}



SYSCALL_DEFINE1(getpgid, pid_t, pid)
{
	return do_getpgid(pid);
}

#ifdef __ARCH_WANT_SYS_GETPGRP

SYSCALL_DEFINE0(getpgrp)
{
	return do_getpgid(0);
}

#endif

static int do_getpgid(pid_t pid)
{
	struct task_struct *p;
	struct pid *grp;
	int retval;

	rcu_read_lock();
	if (!pid)
		grp = task_pgrp(current);
	else {
		retval = -ESRCH;
		p = find_task_by_vpid(pid);
		if (!p)
			goto out;
		grp = task_pgrp(p);
		if (!grp)
			goto out;

		retval = security_task_getpgid(p);
		if (retval)
			goto out;
	}
	retval = pid_vnr(grp);
out:
	rcu_read_unlock();
	return retval;
}

SYSCALL_DEFINE1(getsid, pid_t, pid) // session 和 process group 实现都是对称的
{
	struct task_struct *p;
	struct pid *sid;
	int retval;

	rcu_read_lock();
	if (!pid)
		sid = task_session(current);
	else {
		retval = -ESRCH;
		p = find_task_by_vpid(pid);
		if (!p)
			goto out;
		sid = task_session(p);
		if (!sid)
			goto out;

		retval = security_task_getsid(p);
		if (retval)
			goto out;
	}
	retval = pid_vnr(sid);
out:
	rcu_read_unlock();
	return retval;
}
```

2. cred
```c
SYSCALL_DEFINE0(getuid)
{
	/* Only we change this so SMP safe */
	return from_kuid_munged(current_user_ns(), current_uid());
}

SYSCALL_DEFINE0(geteuid)
{
	/* Only we change this so SMP safe */
	return from_kuid_munged(current_user_ns(), current_euid());
}

SYSCALL_DEFINE0(getgid)
{
	/* Only we change this so SMP safe */
	return from_kgid_munged(current_user_ns(), current_gid());
}

SYSCALL_DEFINE0(getegid)
{
	/* Only we change this so SMP safe */
	return from_kgid_munged(current_user_ns(), current_egid());
}
```

## pidfd
