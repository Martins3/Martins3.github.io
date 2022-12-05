# PROCESS GROUPS, SESSIONS, AND JOB CONTROL 

## task_struct::thread_group

signal.h 中的三个辅助函数:
```c
// same_thread_group 的判断居然是 signal 是否相同的 ?
static inline
bool same_thread_group(struct task_struct *p1, struct task_struct *p2)
{
	return p1->signal == p2->signal;
}

static inline struct task_struct *next_thread(const struct task_struct *p)
{
	return list_entry_rcu(p->thread_group.next,
			      struct task_struct, thread_group);
}

static inline int thread_group_empty(struct task_struct *p)
{
	return list_empty(&p->thread_group);
}
```

```c
	if (likely(p->pid)) {
		ptrace_init_task(p, (clone_flags & CLONE_PTRACE) || trace);

		init_task_pid(p, PIDTYPE_PID, pid);
		if (thread_group_leader(p)) {
			init_task_pid(p, PIDTYPE_TGID, pid);
			init_task_pid(p, PIDTYPE_PGID, task_pgrp(current));
			init_task_pid(p, PIDTYPE_SID, task_session(current));

			if (is_child_reaper(pid)) {
				ns_of_pid(pid)->child_reaper = p;
				p->signal->flags |= SIGNAL_UNKILLABLE;
			}
			p->signal->shared_pending.signal = delayed.signal;
			p->signal->tty = tty_kref_get(current->signal->tty);
			/*
			 * Inherit has_child_subreaper flag under the same
			 * tasklist_lock with adding child to the process tree
			 * for propagate_has_child_subreaper optimization.
			 */
			p->signal->has_child_subreaper = p->real_parent->signal->has_child_subreaper ||
							 p->real_parent->signal->is_child_subreaper;
			list_add_tail(&p->sibling, &p->real_parent->children);
			list_add_tail_rcu(&p->tasks, &init_task.tasks);
			attach_pid(p, PIDTYPE_TGID);
			attach_pid(p, PIDTYPE_PGID);
			attach_pid(p, PIDTYPE_SID);
			__this_cpu_inc(process_counts);
		} else {
			current->signal->nr_threads++;
			atomic_inc(&current->signal->live);
			atomic_inc(&current->signal->sigcnt);
			task_join_group_stop(p);
			list_add_tail_rcu(&p->thread_group,
					  &p->group_leader->thread_group);
			list_add_tail_rcu(&p->thread_node,
					  &p->signal->thread_head);
		}
		attach_pid(p, PIDTYPE_PID);
		nr_threads++;
	}
```

init_task_pid 和 attach_pid 了解一下

## task_struc::tgid
tgkill :
       tgkill() sends the signal sig to the thread with the thread ID tid in the thread group tgid.  (By contrast, kill(2) can be used to send a signal only to a process (i.e., thread group) as a whole, and the signal will be delivered to an arbitrary thread within that process.)

setpgid setpgrp : 其实和 tgid 没有任何关系 :)

```c
/*
 * This needs some heavy checking ...
 * I just haven't the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 *
 * OK, I think I have the protection semantics right.... this is really
 * only important on a multi-user system anyway, to make sure one user
 * can't send a signal to a process owned by another.  -TYT, 12/12/91
 *
 * !PF_FORKNOEXEC check to conform completely to POSIX.
 */
SYSCALL_DEFINE2(setpgid, pid_t, pid, pid_t, pgid)
```

主要使用:
pid.c : 
1. find_task_by_vpid
2. taks_pid_vnr
6. find_vpid
8. change_pid
7. pid_task

sched.h 中的辅助函数:
3. thread_group_leader
4. same_thread_group
5. task_pid

fork() 和 tgid 相关的部分片段:
```c
	/* ok, now we should be set up.. */
	p->pid = pid_nr(pid);
	if (clone_flags & CLONE_THREAD) {
    // 在一个 group
		p->exit_signal = -1;
		p->group_leader = current->group_leader;
		p->tgid = current->tgid;
	} else {
		if (clone_flags & CLONE_PARENT)
			p->exit_signal = current->group_leader->exit_signal;
		else
			p->exit_signal = (clone_flags & CSIGNAL);
    // 重新创建一个group
		p->group_leader = p;
		p->tgid = p->pid;
	}
```

Man clone(2): CLONE_THREAD 和 CLONE_PARENT 相关的说明
    Thread groups were a feature added in Linux 2.4 to support the POSIX threads notion of a set of threads that share a single PID.  Internally, this shared PID is the so-called thread group identifier (TGID) for the thread group.  Since Linux 2.4, calls to getpid(2) return the TGID of the caller.

    The threads within a group can be distinguished by their (system-wide) unique thread IDs (TID).  A new thread's TID is available as the function result returned to the caller, and a thread can obtain its own TID using gettid(2).

    When a clone call is made without specifying CLONE_THREAD, then the resulting thread is placed in a new thread group whose TGID is the same as the thread's TID.  This thread is the leader of the new thread group.

> 1. tid 和 pid
> 2. CLONE_THREAD 会将 thread 放到同一个位置中间

> 感觉 thread group 和 之前想象的process 的内容非常的相似
> 但是，不是在于mm 共享上，而是处理父子关系和 signal 上

## thread_group_leader 的含义

```c
static inline bool thread_group_leader(struct task_struct *p)
{
	return p->exit_signal >= 0;
}

static inline
bool same_thread_group(struct task_struct *p1, struct task_struct *p2)
{
	return p1->signal == p2->signal;
}
```

@todo signal 和 exit_signal 的各自作用 ?



```c
/*
 * Without tasklist or RCU lock it is not safe to dereference
 * the result of task_pgrp/task_session even if task == current,
 * we can race with another thread doing sys_setsid/sys_setpgid.
 */
static inline struct pid *task_pgrp(struct task_struct *task)
{
	return task->signal->pids[PIDTYPE_PGID];
}

static inline struct pid *task_session(struct task_struct *task)
{
	return task->signal->pids[PIDTYPE_SID];
}

static inline int get_nr_threads(struct task_struct *tsk)
{
	return tsk->signal->nr_threads;
}

static inline bool thread_group_leader(struct task_struct *p)
{
	return p->exit_signal >= 0;
}


static inline struct pid *task_pid(struct task_struct *task)
{
	return task->thread_pid;
}
```
