# ptrace.md

[](https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/)

https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.html#id3056712

## checklist
1. strace 实现 : PTRACE_SYSCALL
2. wait + signal 实现 ptracee 和 ptracer 之间的交互，利用 architecture 来实现　PTRACE_SYSCALL , PTRACE_SINGLESTEP
3. 

## todo
1. read the code
2. plka 838
3. Man ptrace(2)
    1. 

4. 如何在运行的过程中间 attach ? (似乎总是动态运行的)
    1. 如果 创建了大量的 chilren，如何处理 ?
        1. 要不要首先 debug 测试一下

2. 当 ptracee 在执行的时候，靠什么通知 ptracer ?

3. ptracee 怎么知道执行到哪一个位置的时候停止 ? 
    1. watchpoint 是怎么打上去 ?
    2. reg 的 monitor 和 addresss 的 monitor 是如何处理的 ?
  
5. x86 下面到底有什么东西可以用来支持 ptrace ?


6. task->real_parent 的作用还有什么 ?
    1. plka : P845 中间 STOP 的 step 4



##  ref && doc

#### Man ptrace(2)

> A process can initiate a trace by calling fork(2) and having the resulting child do a PTRACE_TRACEME, followed (typically) by an execve(2).  Alternatively, one process may commence tracing another process using PTRACE_ATTACH or PTRACE_SEIZE.

这里是描述了三种方法吗 ? fork execve ， PTRACE_SEIZE 和 PTRACE_ATTACH 

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

