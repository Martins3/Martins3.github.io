# pid

asahi linux 中的 pstree 的部分 thread ，asahi 中没有安装图形界面
```txt
🧀  pstree -p
systemd(1)─┬─ModemManager(1177)─┬─{ModemManager}(1200)
           │                    ├─{ModemManager}(1201)
           │                    └─{ModemManager}(1204)
           ├─sshd(1403)─┬─sshd(63180)───sshd(63183)───zsh(63184)───tmux: client(64027)
           │            ├─sshd(64081)───sshd(64084)───zsh(64085)
           │            ├─sshd(64198)───sshd(64201)───zsh(64202)
           │            └─sshd(64492)───sshd(64495)───zsh(64496)
           ├─systemd(3042)─┬─(sd-pam)(3046)
           │               ├─dbus-broker-lau(3857)───dbus-broker(3877)
           │               └─pueued(3061)─┬─novnc_server(9364)───websockify(9381)
           │                              ├─{pueued}(3063)
           │                              ├─{pueued}(3064)
           │                              ├─{pueued}(3065)
           │                              ├─{pueued}(3066)
           │                              ├─{pueued}(3068)
           │                              └─{pueued}(3069)
           ├─tmux: server(3548)─┬─zsh(3856)───bash(9135)───bash(9157)───qemu-system-aar(9393)─┬─{qemu-system-aar}(9395)
           │                    │                                                             ├─{qemu-system-aar}(9396)
           │                    │                                                             ├─{qemu-system-aar}(9412)
           │                    │                                                             ├─{qemu-system-aar}(9413)
           │                    │                                                             ├─{qemu-system-aar}(9415)
           │                    │                                                             ├─{qemu-system-aar}(9428)
           │                    │                                                             ├─{qemu-system-aar}(9430)
           │                    │                                                             ├─{qemu-system-aar}(9434)
           │                    │                                                             ├─{qemu-system-aar}(9436)
           │                    │                                                             ├─{qemu-system-aar}(9437)
           │                    │                                                             ├─{qemu-system-aar}(9438)
           │                    │                                                             ├─{qemu-system-aar}(9439)
           │                    │                                                             ├─{qemu-system-aar}(9440)
           │                    │                                                             ├─{qemu-system-aar}(9443)
           │                    │                                                             └─{qemu-system-aar}(9444)
           │                    ├─zsh(9623)───bash(10691)───socat(10738)
           │                    └─zsh(11740)───pstree(64640)
           └─wpa_supplicant(1322)
```
可以观察到
1. tmux:client 不会 fork ，而是让 tmux:server 来处理的。pueued 也是模拟这种架构。
2. 试用了四个 ssh 到 asahi linux 上，所以 sshd fork 了四个 thread 出来

## 问题

### session
```txt
🧀  pstree -p 3548
tmux: server(3548)─┬─zsh(11740)─┬─sleep(66756)
                   │            └─wc(66757)
                   ├─zsh(64968)─┬─pstree(67178)
                   │            └─sleep(65193)
                   └─zsh(67021)─┬─sleep(67167)
                                └─wc(67168)

🧀  ps -o tid,pid,tpgid,sid -p 67168
    TID     PID   TPGID     SID
  67168   67168   67167   67021

🧀  ps -o tid,pid,tpgid,sid -p 65193
    TID     PID   TPGID     SID
  65193   65193   67435 (这个不存在对应的 process 了)   64968
```

man ps(1) 可以检查到:

pid         PID       a number representing the process ID (alias tgid).
tpgid       TPGID     ID  of  the  foreground process group on the tty (terminal) that the process is connected to, or -1 if the process is not connected to a tty.


- [ ] thread 和 process 就是是否共享地址空间吧，这个区分还是很清楚的

![loading](https://unixism.net/wp-content/uploads/2020/06/Process-Group-Signals.png)
这个解释非常好，通过 proces group 可以让信号同时发送给 pipe 两侧的 cpu

## therad process,  process, process group,  session
记录几个简单问题，但是应该比较容易解决:
- 测试了一下，当创建 tmux 的一个 pane 的时候，就会创建一个新的 session id ，而 zellij 是创建新的 window 才会
- [ ] 观察下创建 process group 的时间点，应该是 bash 来搞的，而且通过上面的测试，TPGID 是可以不用对应一个 process 的
- [ ] 如何在 session 中构建多个 process group
- [ ] maybe build a shell: https://github.com/danistefanovic/build-your-own-x#build-your-own-shell , then process group is simple


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

## 分析下 for_each_process_thread ，其实 kernel 是明确的 process thread 概念的

```c
	struct task_struct *g, *p;
```

## 应该参考这个函数 : task_state

## 结构
Daemons normally disassociate themselves from any controlling terminal by creating a new session without one
https://stackoverflow.com/questions/6548823/use-and-meaning-of-session-and-process-group-in-unix

https://stackoverflow.com/questions/9305992/if-threads-share-the-same-pid-how-can-they-be-identified
课代表总结：
用户看到的是 tgid: thread group id
kernel 看到的是 pid

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
