## ptrace


*All further tracing actions performed by the kernel are present in the signal handler code discussed in
Chapter 5*. When a signal is delivered, the kernel checks whether the `PT_TRACED` flag is set in the `ptrace`
field of `task_struct`. If it is, the state of the process is set to `TASK_STOPPED` (in `get_signal_to_deliver`
in `kernel/signal.c`) in order to interrupt execution. `notify_parent` with the `CHLD` signal is then used
to inform the tracer process. (The tracer process is woken up if it happens to be sleeping.) The tracer
process then performs the desired checks on the target process as specified by the remaining ptrace
options.

- [ ] notify_parent
- [ ] when a signal is delivered, the kernel checks whether the `PT_TRACED` flag is set  ?
  - 这种机制是不存在的吧 ？

> 信号来自于 architecture 相关的代码: 比如 strace 的syscall 的，当设置 TIF 上的相关的flags，然后触发

[GDB Remote Serial Protocol](https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.html#id3056712)
[strace](https://github.com/strace/strace)

- [ ] PTRACE_SYSCALL
- [ ] PTRACE_SINGLESTEP
- [ ] PTRACE_ATTACH
  - [ ] 和 PTRACE_ME 的功能重复了 ?
- [ ] PTRACE_ME  : 让自己的 parent 作为 trace 自己的工具
  - `__do_sys_ptrace`
    - ptrace_traceme
      - ptrace_link
        - `child->parent = new_parent;`

关键变量:
```c
struct task_struct {
	/*
	 * 'ptraced' is the list of tasks this task is using ptrace() on.
	 *
	 * This includes both natural children and PTRACE_ATTACH targets.
	 * 'ptrace_entry' is this task's link on the p->parent->ptraced list.
	 */
	struct list_head		ptraced;
	struct list_head		ptrace_entry;
```



## checklist
1. strace 实现 : PTRACE_SYSCALL
2. wait + signal 实现 ptracee 和 ptracer 之间的交互，利用 architecture 来实现　PTRACE_SYSCALL , PTRACE_SINGLESTEP

3. Man ptrace(2)
4. plka 的说明书

4. 如何在运行的过程中间 attach ? (似乎总是动态运行的)

2. 当 ptracee 在执行的时候，靠什么通知 ptracer ?

3. ptracee 怎么知道执行到哪一个位置的时候停止 ? 
    1. x86 下面到底有什么东西可以用来支持 ptrace ?
      1. 各种 watchpoint 是怎么打上去 ?

6. task->real_parent 的作用还有什么 ?
    1. plka : P845 中间 STOP 的 step 4

##  ref && doc

#### Man ptrace(2)

> A process can initiate a trace by calling fork(2) and having the resulting child do a `PTRACE_TRACEME`, followed (typically) by an execve(2).  Alternatively, one process may commence tracing another process using PTRACE_ATTACH or PTRACE_SEIZE.


> While being traced, the tracee will stop each time a signal is delivered, even if the signal is being ignored.  (An  exception  is
> SIGKILL,  which  has  its usual effect.)  The tracer will be notified at its next call to waitpid(2) (or one of the related "wait"
> system calls); that call will return a status value containing information that indicates the cause of the  stop  in  the  tracee.
> While  the tracee is stopped, the tracer can use various ptrace requests to inspect and modify the tracee.  The tracer then causes
> the tracee to continue, optionally ignoring the delivered signal (or even delivering a different signal instead).

- tracer 利用 waitpid 可以检查到 tracee 被任何信号 stop 下来了, 停下来之后，tracer 利用各种 ptrace 接口来对于 tracee 进行操作

> If the PTRACE_O_TRACEEXEC option is not in effect, all successful calls to execve(2) by the traced process will  cause  it  to  be
> sent a SIGTRAP signal, giving the parent a chance to gain control before the new program begins execution.

- [ ] PTRACE_O_TRACEEXEC 起作用的时候咋办 ?
- [ ] exec 的代码看看 ?


> CLONE_PTRACE (since Linux 2.2)
> 
>        If  CLONE_PTRACE  is  specified,  and  the calling process is being
>        traced, then trace the child also (see ptrace(2)).

- [ ] 很怪异，为什么 CLONE_PTRACE 表示 calling process 被 traced

> PTRACE_ATTACH
>        Attach to the process specified in pid, making it a tracee of the calling process.  The tracee is sent a SIGSTOP, but  will
>        not  necessarily  have stopped by the completion of this call; use waitpid(2) to wait for the tracee to stop.  See the "At‐
>        taching and detaching" subsection for additional information.  (addr and data are ignored.)
> 
>        Permission to perform a PTRACE_ATTACH is governed by a ptrace access mode PTRACE_MODE_ATTACH_REALCREDS check; see below.
> 
> PTRACE_SEIZE (since Linux 3.4)
>        Attach to the process specified in pid, making it a tracee of the calling process.  Unlike PTRACE_ATTACH, PTRACE_SEIZE does
>        not  stop  the process.  Group-stops are reported as PTRACE_EVENT_STOP and WSTOPSIG(status) returns the stop signal.  Auto‐
>        matically attached children stop with PTRACE_EVENT_STOP and WSTOPSIG(status) returns SIGTRAP instead of having SIGSTOP sig‐
>        nal delivered to them.  execve(2) does not deliver an extra SIGTRAP.  Only a PTRACE_SEIZEd process can accept PTRACE_INTER‐
>        RUPT and PTRACE_LISTEN commands.  The "seized" behavior just described is inherited by children that are automatically  at‐
>        tached  using  PTRACE_O_TRACEFORK,  PTRACE_O_TRACEVFORK,  and PTRACE_O_TRACECLONE.  addr must be zero.  data contains a bit
>        mask of ptrace options to activate immediately.
> 
>        Permission to perform a PTRACE_SEIZE is governed by a ptrace access mode PTRACE_MODE_ATTACH_REALCREDS check; see below.


这里是描述了三种方法吗 ? fork execve ，PTRACE_SEIZE 和 PTRACE_ATTACH 

## kernel/trace/ 下的内容
1. tracefs ?
2. 使用的标准框架，用户实现 trace 的，是什么 ? 


```c
SYSCALL_DEFINE4(ptrace, long, request, long, pid, unsigned long, addr,
		unsigned long, data)
{
	struct task_struct *child;
	long ret;

	if (request == PTRACE_TRACEME) {
		ret = ptrace_traceme(); // 如果是被跟踪者
		goto out;
	}

	child = find_get_task_by_vpid(pid); // todo 为什么是 by_vpid ，如果由于权限问题找不到如何 ?
	if (!child) {
		ret = -ESRCH;
		goto out;
	}

	if (request == PTRACE_ATTACH || request == PTRACE_SEIZE) {
		ret = ptrace_attach(child, request, addr, data);
		/*
		 * Some architectures need to do book-keeping after
		 * a ptrace attach.
		 */
		if (!ret)
			arch_ptrace_attach(child);
		goto out_put_task_struct;
	}

	ret = ptrace_check_attach(child, request == PTRACE_KILL ||
				  request == PTRACE_INTERRUPT); // 检查 ptracee 的工作是否完成
	if (ret < 0)
		goto out_put_task_struct;

	ret = arch_ptrace(child, request, addr, data); // 进入到 x86 中间，疯狂调用 copy_regset_to_user 等操作，这就是完全各种请求的地方
	if (ret || request != PTRACE_DETACH)
		ptrace_unfreeze_traced(child);

 out_put_task_struct:
	put_task_struct(child);
 out:
	return ret;
}

/**
 * ptrace_traceme  --  helper for PTRACE_TRACEME
 *
 * Performs checks and sets PT_PTRACED.
 * Should be used by all ptrace implementations for PTRACE_TRACEME.
 */
static int ptrace_traceme(void)
{
	int ret = -EPERM;

	write_lock_irq(&tasklist_lock);
	/* Are we already being traced? */
	if (!current->ptrace) { // todo current->ptrace 变量的作用是什么 ?
		ret = security_ptrace_traceme(current->parent);
		/*
		 * Check PF_EXITING to ensure ->real_parent has not passed
		 * exit_ptrace(). Otherwise we don't report the error but
		 * pretend ->real_parent untraces us right after return.
		 */
		if (!ret && !(current->real_parent->flags & PF_EXITING)) { // current->real_parent 真正的 parent 是什么东西呀 ?
			current->ptrace = PT_PTRACED; 
			ptrace_link(current, current->real_parent);
		}
	}
	write_unlock_irq(&tasklist_lock);

	return ret;
}

/*
 * ptrace a task: make the debugger its new parent and
 * move it to the ptrace list.
 *
 * Must be called with the tasklist lock write-held.
 */
static void ptrace_link(struct task_struct *child, struct task_struct *new_parent)
{
	rcu_read_lock();
	__ptrace_link(child, new_parent, __task_cred(new_parent));
	rcu_read_unlock();
}


```


## ptrace_freeze_traced 和 ptrace_unfreeze_traced

```c
/* Ensure that nothing can wake it up, even SIGKILL */
static bool ptrace_freeze_traced(struct task_struct *task)
{
	bool ret = false;

	/* Lockless, nobody but us can set this flag */
	if (task->jobctl & JOBCTL_LISTENING)
		return ret;

	spin_lock_irq(&task->sighand->siglock);
	if (task_is_traced(task) && !__fatal_signal_pending(task)) {
		task->state = __TASK_TRACED;
		ret = true;
	}
	spin_unlock_irq(&task->sighand->siglock);

	return ret;
}

static void ptrace_unfreeze_traced(struct task_struct *task)
{
	if (task->state != __TASK_TRACED) // todo task->flags 和 task->state 的各自的作用是什么 ?
		return;

	WARN_ON(!task->ptrace || task->parent != current);

	/*
	 * PTRACE_LISTEN can allow ptrace_trap_notify to wake us up remotely.
	 * Recheck state under the lock to close this race.
	 */
	spin_lock_irq(&task->sighand->siglock);
	if (task->state == __TASK_TRACED) {
		if (__fatal_signal_pending(task)) // todo signal 机制是什么
			wake_up_state(task, __TASK_TRACED);
		else
			task->state = TASK_TRACED;
	}
	spin_unlock_irq(&task->sighand->siglock);
}

int wake_up_state(struct task_struct *p, unsigned int state)
{
	return try_to_wake_up(p, state, 0);
}
```

## how to trace multiple task ?

```c
	/*
	 * 'ptraced' is the list of tasks this task is using ptrace() on.
	 *
	 * This includes both natural children and PTRACE_ATTACH targets.
	 * 'ptrace_entry' is this task's link on the p->parent->ptraced list.
	 */
	struct list_head		ptraced;
	struct list_head		ptrace_entry;
  // 这两个变量用来实现挂载其中的函数，
  // todo natural children 的内容到底是什么 ? 
```

## ptrace_readdata && ptrace_writedata

> 居然没有调用者

> 而且读取 register 的方法也不知道在哪里

```c
int ptrace_readdata(struct task_struct *tsk, unsigned long src, char __user *dst, int len)

int ptrace_writedata(struct task_struct *tsk, char __user *src, unsigned long dst, int len)
```


## [年轻人的第一个 gdb](https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/)

- [ ] fix the bug out of LLVM 10

- [ ] personality : 进一步的调查一下

- [ ] initialise_load_address

- [x] break 靠什么通过函数知道我们将会放置的地址
  - dwarf
  
```c
minidbg> break main
Set breakpoint at address 0x4011b8
```

