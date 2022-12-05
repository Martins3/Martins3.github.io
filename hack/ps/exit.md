## exit

> Man `exit_group`
> 
>   This system call is equivalent to _exit(2) except that it terminates not only the calling thread, but all threads in the calling process's thread group.

- exit_group 
  - do_group_eixt : 其他人已经发送了通知 ?
  - zap_other_threads : 我来通知大家
  - **do_exit** : 将当前进程回收
    - ptrace_event(PTRACE_EVENT_EXIT, code);
    - exit_signals(tsk); 释放和 signal 相关的资源
      - `__cleanup_sighand`
    - exit_notify
      - forget_original_parent : 正如注释所说 1. 将 child 放到 init 上 2. (POSIX 3.2.2.2) 上要求的 SIGHUP  和 SIGCONT
        - find_child_reaper : 找到 init reaper
          - `struct task_struct *reaper = pid_ns->child_reaper;`  : 由于 namespace 的出现，所以不一定是 init
          - find_alive_thread : 如果是 namespace 中间的 init 要挂掉了，那么找一个 init 在一个 thread group  的其他的 thread 来作为 reaper
          - zap_pid_ns_processes : 如果 namespace 的 init 的整个 thread group 都没了，那么整个 namespace 被回收掉
        - find_new_reaper : 注释说的很清楚，首先找 thread group 内部的，其次 prctl, 最后 find_child_reaper 获取的 init 
          - reparent_leader : (POSIX 3.2.2.2) 的实现
        - release_task : 如果当时正在 trace 其他的进程，那么需要被 trace 的进程需要被 kill
    - exit_mm : 诸如此类的内容还有很多
    - do_task_dead : 将当前 schedule 出去，然后之后 parent 来回收剩下的资源

## difference between exit and exit_group
- `__do_sys_exit`
  - **do_eixt**

所以区别就是 : exit_group 会发送 KILL 信号给整个 process group 的开始结束
```c
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
```
