# 总结一些用户接口

## therad process,  process, process group,  session
记录几个简单问题，但是应该比较容易解决:
- [ ] thread 和 process 就是是否共享地址空间吧，这个区分还是很清楚的
- 测试了一下，当创建 tmux 的一个 pane 的时候，就会创建一个新的 session id ，而 zellij 是创建新的 window 才会
- [ ] 观察下创建 process group 的时间点，应该是 bash 构建的
- [ ] 如何在 session 中构建多个 process group


- [ ] maybe build a shell: https://github.com/danistefanovic/build-your-own-x#build-your-own-shell , then process group is simple

![loading](https://unixism.net/wp-content/uploads/2020/06/Process-Group-Signals.png)
这个解释非常好，通过 proces group 可以让信号同时发送给 pipe 两侧的 cpu


# kernel/pid.c 和 kernel/pid_namespace.c
plka 2.3.3
plka 28.2.1

首先阅读完成 Man futex 和 Man set_tid_address 以及对应的用户层程序。

http://man7.org/linux/man-pages/man7/credentials.7.html
Man credentials(7)

## KeyNote
1. pid_nr 的实现过于真实啊

## Doc
- [](https://lwn.net/Articles/801319/)

- [](https://lwn.net/Articles/794707/)

- [PID namespaces in the 2.6.24 kernel](https://lwn.net/Articles/259217/)

PID namespaces are hierarchical; once a new PID namespace is created, all the tasks in the current PID namespace will see the tasks (i.e. will be able to address them with their PIDs) in this new namespace. However, tasks from the new namespace will not see the ones from the current. This means that now each task has more than one PID -- one for each namespace.
> 下层做的事情，上层还是知道的，为什么要这样 : wait 机制否则无法工作

To create a new namespace, one should just call the clone(2) system call with the CLONE_NEWPID flag set. After this, it is useful to change the root directory and mount a new procfs instance in the /proc to make the common utilities like ps work. **Note that since the parent knows the PID of its child, it may wait() in the usual way for it to exit.**

The first task in a new namespace will have a PID of 1. Thus, it will be this namespace's init and child reaper, so all the orphaned tasks will be re-parented to it. Unlike the standalone machine, this "init" can die, and in this case, the whole namespace will be terminated.

Since now we will have isolated sets of tasks, we should make proc show only the set of PIDs which is visible for a particular task. **To achieve this goal, procfs should be mounted multiple times** -- once for each namespace. After this the PIDs that are shown in the mounted instance will be from the namespace which created that mount.

`..._nr()`

These operate with the so called "global" PIDs. Global PIDs are the numbers that are unique in the whole system, just like the old PIDs were. E.g. pid_nr(pid) will tell you the global PID of the given struct pid. These are only useful when the PID value is not going to leave the kernel. For example, some code needs to save the PID and then find the task by it. However, in this case saving the direct pointer on the struct pid is more preferable as global PIDs are going be used in kernel logs only.

`..._vnr()`

These helpers work with the "virtual" PID, i.e. with the ID as seen by a process. For example, task_pid_vnr(tsk) will tell you the PID of a task, as this task sees it (with sys_getpid()). Note that this value will most likely be useless if you're working in another namespace, so these are always used when working with the current task, since all tasks always see their virtual PIDs.

`..._nr_ns()`

These work with the PIDs as seen from the specified namespace. If you want to get some task's PID (for example, to report it to the userspace and find this task later), you may call task_pid_nr_ns(tsk, current->nsproxy->pid_ns) to get the number, and then find the task using find_task_by_pid_ns(pid, current->nsproxy->pid_ns). These are used in system calls, when the PID comes from the user space. In this case one task may address another which exists in another namespace.

- [](https://stackoverflow.com/questions/26779416/what-is-the-relation-between-task-struct-and-pid-namespace)

## signal 结构体到底是如何共享的, 创建的

## thread group

signal.h
```c
#define __for_each_thread(signal, t)	\
	list_for_each_entry_rcu(t, &(signal)->thread_head, thread_node)

#define for_each_thread(p, t)		\
	__for_each_thread((p)->signal, t)
```

pid.h
```c
#define do_each_pid_task(pid, type, task)				\
	do {								\
		if ((pid) != NULL)					\
			hlist_for_each_entry_rcu((task),		\
				&(pid)->tasks[type], pid_links[type]) {

			/*
			 * Both old and new leaders may be attached to
			 * the same pid in the middle of de_thread().
			 */
#define while_each_pid_task(pid, type, task)				\
				if (type == PIDTYPE_PID)		\
					break;				\
			}						\
	} while (0)

#define do_each_pid_thread(pid, type, task)				\
	do_each_pid_task(pid, type, task) {				\
		struct task_struct *tg___ = task;			\
		for_each_thread(tg___, task) {

#define while_each_pid_thread(pid, type, task)				\
		}							\
		task = tg___;						\
	} while_each_pid_task(pid, type, task)
```

1. 从这里的代码来看，其实，难道只有 process group 才会加入到 pid 的 hlist 中间吗 ?

## attach_pid
1. attach_pid 是唯一实现将 struct task 加入到 pid 中间的方法
    1. init_task_pid 初始化 task 指向的 pid
    2. init_task_pid 和 attach_pid 共同合作，当 task 指向某一个 pid，那么该 pid 就需要持有该 task
2. 用于 copy_process 和 change_pid 两个位置

```c
	if (likely(p->pid)) {
		ptrace_init_task(p, (clone_flags & CLONE_PTRACE) || trace);

		init_task_pid(p, PIDTYPE_PID, pid); // todo 目前看，这似乎只是为了维护一致性(task 关联的 pid 都会持有该 task)
		if (thread_group_leader(p)) { // 只有 group leader 才有发言权，thread 放到下一级管理
			init_task_pid(p, PIDTYPE_TGID, pid); // todo 并不能理解为什么不把功能进行合并
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
			refcount_inc(&current->signal->sigcnt);
			task_join_group_stop(p);
			list_add_tail_rcu(&p->thread_group,
					  &p->group_leader->thread_group); // todo 将自己加入到 thread group 中间，所以，下面这一行有什么关系吗 ?
			list_add_tail_rcu(&p->thread_node, // 那么 attach_pid(p, PIDTYPE_TGID); 和这里的加入工作的关系是什么 ?
					  &p->signal->thread_head);
		}
		attach_pid(p, PIDTYPE_PID);
		nr_threads++;
	}
```

## core struct and API

```c
enum pid_type
{
	PIDTYPE_PID, // todo 既然 task_struct 中间已经存在了，pid 和 tgid 为什么还会存在共享这个 ?
	PIDTYPE_TGID,
	PIDTYPE_PGID,
	PIDTYPE_SID,
	PIDTYPE_MAX,
};

/*
 * What is struct pid?
 *
 * A struct pid is the kernel's internal notion of a process identifier.
 * It refers to individual tasks, process groups, and sessions.  While
 * there are processes attached to it the struct pid lives in a hash
 * table, so it and then the processes that it refers to can be found
 * quickly from the numeric pid value.  The attached processes may be
 * quickly accessed by following pointers from struct pid.
 *
 * Storing pid_t values in the kernel and referring to them later has a
 * problem.  The process originally with that pid may have exited and the
 * pid allocator wrapped, and another process could have come along
 * and been assigned that pid.
 *
 * Referring to user space processes by holding a reference to struct
 * task_struct has a problem.  When the user space process exits
 * the now useless task_struct is still kept.  A task_struct plus a
 * stack consumes around 10K of low kernel memory.  More precisely
 * this is THREAD_SIZE + sizeof(struct task_struct).  By comparison
 * a struct pid is about 64 bytes.
 *
 * Holding a reference to struct pid solves both of these problems.
 * It is small so holding a reference does not consume a lot of
 * resources, and since a new struct pid is allocated when the numeric pid
 * value is reused (when pids wrap around) we don't mistakenly refer to new
 * processes.
 */

 // todo 所以什么是 THREAD_SIZE ?
 // 使用 pid_t 为什么会出现问题 ?

/*
 * struct upid is used to get the id of the struct pid, as it is
 * seen in particular namespace. Later the struct pid is found with
 * find_pid_ns() using the int nr and struct pid_namespace *ns.
 */

struct upid {
	int nr; // todo nr 是什么意思 ?
	struct pid_namespace *ns;
};

struct pid
{
	refcount_t count;
	unsigned int level;
	/* lists of tasks that use this pid */
	struct hlist_head tasks[PIDTYPE_MAX];  // todo 难道同一个pid 可以对应不同的 task ?
	/* wait queue for pidfd notifications */
	wait_queue_head_t wait_pidfd;
	struct rcu_head rcu;
	struct upid numbers[1];
};



extern struct pid init_struct_pid;

extern const struct file_operations pidfd_fops;

struct file;

extern struct pid *pidfd_pid(const struct file *file);

static inline struct pid *get_pid(struct pid *pid)
{
	if (pid)
		refcount_inc(&pid->count);
	return pid;
}

extern void put_pid(struct pid *pid);
extern struct task_struct *pid_task(struct pid *pid, enum pid_type);
static inline bool pid_has_task(struct pid *pid, enum pid_type type)
{
	return !hlist_empty(&pid->tasks[type]);
}
extern struct task_struct *get_pid_task(struct pid *pid, enum pid_type);

extern struct pid *get_task_pid(struct task_struct *task, enum pid_type type);

/*
 * these helpers must be called with the tasklist_lock write-held.
 */
extern void attach_pid(struct task_struct *task, enum pid_type);
extern void detach_pid(struct task_struct *task, enum pid_type);
extern void change_pid(struct task_struct *task, enum pid_type, struct pid *pid);
extern void transfer_pid(struct task_struct *old, struct task_struct *new, enum pid_type);

struct pid_namespace;
extern struct pid_namespace init_pid_ns;

/*
 * look up a PID in the hash table. Must be called with the tasklist_lock
 * or rcu_read_lock() held.
 *
 * find_pid_ns() finds the pid in the namespace specified
 * find_vpid() finds the pid by its virtual id, i.e. in the current namespace
 *
 * see also find_task_by_vpid() set in include/linux/sched.h
 */
extern struct pid *find_pid_ns(int nr, struct pid_namespace *ns);
extern struct pid *find_vpid(int nr);

/*
 * Lookup a PID in the hash table, and return with it's count elevated.
 */
extern struct pid *find_get_pid(int nr);
extern struct pid *find_ge_pid(int nr, struct pid_namespace *);

extern struct pid *alloc_pid(struct pid_namespace *ns, pid_t *set_tid,
			     size_t set_tid_size);
extern void free_pid(struct pid *pid);
extern void disable_pid_allocation(struct pid_namespace *ns);

/*
 * ns_of_pid() returns the pid namespace in which the specified pid was
 * allocated.
 *
 * NOTE:
 * 	ns_of_pid() is expected to be called for a process (task) that has
 * 	an attached 'struct pid' (see attach_pid(), detach_pid()) i.e @pid
 * 	is expected to be non-NULL. If @pid is NULL, caller should handle
 * 	the resulting NULL pid-ns.
 */
static inline struct pid_namespace *ns_of_pid(struct pid *pid)
{
	struct pid_namespace *ns = NULL;
	if (pid)
		ns = pid->numbers[pid->level].ns;
	return ns;
}

/*
 * is_child_reaper returns true if the pid is the init process
 * of the current namespace. As this one could be checked before
 * pid_ns->child_reaper is assigned in copy_process, we check
 * with the pid number.
 */
static inline bool is_child_reaper(struct pid *pid) // todo 这个也太棒了吧!
{
	return pid->numbers[pid->level].nr == 1;
}

/*
 * the helpers to get the pid's id seen from different namespaces
 *
 * pid_nr()    : global id, i.e. the id seen from the init namespace;
 * pid_vnr()   : virtual id, i.e. the id seen from the pid namespace of
 *               current.
 * pid_nr_ns() : id seen from the ns specified.
 *
 * see also task_xid_nr() etc in include/linux/sched.h
 */

static inline pid_t pid_nr(struct pid *pid)
{
	pid_t nr = 0;
	if (pid)
		nr = pid->numbers[0].nr;
	return nr;
}

pid_t pid_nr_ns(struct pid *pid, struct pid_namespace *ns);
pid_t pid_vnr(struct pid *pid);

// 两个 do_each 有意思
// while_each todo
#define do_each_pid_task(pid, type, task)				\
	do {								\
		if ((pid) != NULL)					\
			hlist_for_each_entry_rcu((task),		\
				&(pid)->tasks[type], pid_links[type]) {

			/*
			 * Both old and new leaders may be attached to
			 * the same pid in the middle of de_thread().
			 */
#define while_each_pid_task(pid, type, task)				\
				if (type == PIDTYPE_PID)		\
					break;				\
			}						\
	} while (0)

#define do_each_pid_thread(pid, type, task)				\
	do_each_pid_task(pid, type, task) {				\
		struct task_struct *tg___ = task;			\
		for_each_thread(tg___, task) {

#define while_each_pid_thread(pid, type, task)				\
		}							\
		task = tg___;						\
	} while_each_pid_task(pid, type, task)
#endif /* _LINUX_PID_H */
```

```c
/*
 * NOTE! "signal_struct" does not have its own
 * locking, because a shared signal_struct always
 * implies a shared sighand_struct, so locking
 * sighand_struct is always a proper superset of
 * the locking of signal_struct.
 */
struct signal_struct {
  // ...
	/* PID/PID hash table linkage. */
	struct pid *pids[PIDTYPE_MAX];
  // ...

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
```


##
1. 通过 struct pid 获取 task_struct ? 那么通过什么获取 struct pid ?


```c
struct task_struct *pid_task(struct pid *pid, enum pid_type type)
{
	struct task_struct *result = NULL;
	if (pid) {
		struct hlist_node *first;
		first = rcu_dereference_check(hlist_first_rcu(&pid->tasks[type]),
					      lockdep_tasklist_lock_is_held()); // 所以为什么 first 就可以了，那为什么需要使用 hlist ?
		if (first)
			result = hlist_entry(first, struct task_struct, pid_links[(type)]);// task_struct->pid_links[type] 挂到 pid->tasks[type] 中间
	}
	return result;
}

/*
 * Must be called under rcu_read_lock().
 */
struct task_struct *find_task_by_pid_ns(pid_t nr, struct pid_namespace *ns)
{
	RCU_LOCKDEP_WARN(!rcu_read_lock_held(),
			 "find_task_by_pid_ns() needs rcu_read_lock() protection");
	return pid_task(find_pid_ns(nr, ns), PIDTYPE_PID); // todo 所以说，其实 pid 也是可以被共享的 ?
}

struct pid *find_pid_ns(int nr, struct pid_namespace *ns)
{
	return idr_find(&ns->idr, nr); // 利用 idr 机制实现 struct pid 和 pid_t 之间映射关系
}

struct task_struct *find_task_by_vpid(pid_t vnr)
{
	return find_task_by_pid_ns(vnr, task_active_pid_ns(current));
}

struct task_struct *find_get_task_by_vpid(pid_t nr) // 原来
{
	struct task_struct *task;

	rcu_read_lock();
	task = find_task_by_vpid(nr);
	if (task)
		get_task_struct(task);
	rcu_read_unlock();

	return task;
}
```

```c
struct pid *get_task_pid(struct task_struct *task, enum pid_type type)
{
	struct pid *pid;
	rcu_read_lock();
	pid = get_pid(rcu_dereference(*task_pid_ptr(task, type)));
	rcu_read_unlock();
	return pid;
}

static struct pid **task_pid_ptr(struct task_struct *task, enum pid_type type)
{
	return (type == PIDTYPE_PID) ? // todo 无法理解此处的判断的含义
		&task->thread_pid :
		&task->signal->pids[type];
}
```

```c
static inline pid_t pid_nr(struct pid *pid)
{
	pid_t nr = 0;
	if (pid)
		nr = pid->numbers[0].nr;
	return nr;
}

pid_t pid_vnr(struct pid *pid)
{
	return pid_nr_ns(pid, task_active_pid_ns(current));
}

pid_t pid_nr_ns(struct pid *pid, struct pid_namespace *ns)
{
	struct upid *upid;
	pid_t nr = 0;

	if (pid && ns->level <= pid->level) {
		upid = &pid->numbers[ns->level];
		if (upid->ns == ns)
			nr = upid->nr;
	}
	return nr;
}
```

## alloc_pid : 核心
1. 对于每一个层次的 namespace  都是需要进行初始化
    1. 利用 dir 分配

```c
struct pid *alloc_pid(struct pid_namespace *ns, pid_t *set_tid,
		      size_t set_tid_size)
{
	struct pid *pid;
	enum pid_type type;
	int i, nr;
	struct pid_namespace *tmp;
	struct upid *upid;
	int retval = -ENOMEM;

	/*
	 * set_tid_size contains the size of the set_tid array. Starting at
	 * the most nested currently active PID namespace it tells alloc_pid()
	 * which PID to set for a process in that most nested PID namespace
	 * up to set_tid_size PID namespaces. It does not have to set the PID
	 * for a process in all nested PID namespaces but set_tid_size must
	 * never be greater than the current ns->level + 1.
	 */
	if (set_tid_size > ns->level + 1)
		return ERR_PTR(-EINVAL);

	pid = kmem_cache_alloc(ns->pid_cachep, GFP_KERNEL);
	if (!pid)
		return ERR_PTR(retval);

	tmp = ns;
	pid->level = ns->level;

	for (i = ns->level; i >= 0; i--) { // 对于 ns 的每一个层次都是需要进行分配的
		int tid = 0;

		if (set_tid_size) { // 如果 set_tid_size 的，那么 tid 就是从参数中间得到
			tid = set_tid[ns->level - i];

			retval = -EINVAL;
			if (tid < 1 || tid >= pid_max)
				goto out_free;
			/*
			 * Also fail if a PID != 1 is requested and
			 * no PID 1 exists.
			 */
			if (tid != 1 && !tmp->child_reaper)
				goto out_free;
			retval = -EPERM;
			if (!ns_capable(tmp->user_ns, CAP_SYS_ADMIN))
				goto out_free;
			set_tid_size--;
		}

		idr_preload(GFP_KERNEL);
		spin_lock_irq(&pidmap_lock);

		if (tid) {
			nr = idr_alloc(&tmp->idr, NULL, tid,
				       tid + 1, GFP_ATOMIC);
			/*
			 * If ENOSPC is returned it means that the PID is
			 * alreay in use. Return EEXIST in that case.
			 */
			if (nr == -ENOSPC)
				nr = -EEXIST;
		} else {
			int pid_min = 1;
			/*
			 * init really needs pid 1, but after reaching the
			 * maximum wrap back to RESERVED_PIDS
			 */
			if (idr_get_cursor(&tmp->idr) > RESERVED_PIDS)
				pid_min = RESERVED_PIDS;

			/*
			 * Store a null pointer so find_pid_ns does not find
			 * a partially initialized PID (see below).
			 */
			nr = idr_alloc_cyclic(&tmp->idr, NULL, pid_min,
					      pid_max, GFP_ATOMIC);
		}
		spin_unlock_irq(&pidmap_lock);
		idr_preload_end();

		if (nr < 0) {
			retval = (nr == -ENOSPC) ? -EAGAIN : nr;
			goto out_free;
		}

		pid->numbers[i].nr = nr;
		pid->numbers[i].ns = tmp;
		tmp = tmp->parent;
	}

	if (unlikely(is_child_reaper(pid))) {
		if (pid_ns_prepare_proc(ns))
			goto out_free;
	}

	get_pid_ns(ns); // todo 这个作用是什么 ?
	refcount_set(&pid->count, 1);
	for (type = 0; type < PIDTYPE_MAX; ++type)
		INIT_HLIST_HEAD(&pid->tasks[type]);

	init_waitqueue_head(&pid->wait_pidfd); // pidfd 的作用是什么 ?

	upid = pid->numbers + ns->level;
	spin_lock_irq(&pidmap_lock);
	if (!(ns->pid_allocated & PIDNS_ADDING))
		goto out_unlock;
	for ( ; upid >= pid->numbers; --upid) {
		/* Make the PID visible to find_pid_ns. */
		idr_replace(&upid->ns->idr, pid, upid->nr);
		upid->ns->pid_allocated++;
	}
	spin_unlock_irq(&pidmap_lock);

	return pid;

out_unlock:
	spin_unlock_irq(&pidmap_lock);
	put_pid_ns(ns);

out_free:
	spin_lock_irq(&pidmap_lock);
	while (++i <= ns->level) {
		upid = pid->numbers + i;
		idr_remove(&upid->ns->idr, upid->nr);
	}

	/* On failure to allocate the first pid, reset the state */
	if (ns->pid_allocated == PIDNS_ADDING)
		idr_set_cursor(&ns->idr, 0);

	spin_unlock_irq(&pidmap_lock);

	kmem_cache_free(ns->pid_cachep, pid);
	return ERR_PTR(retval);
}
```


## pid
- [ ] 真的需要好好看看了，在目錄下 ag pid 出來的內容那麼多。。。。。, check 這些文件的內容

当我们发现
1. pid 在 namespace 的层次结构，需要在上层每一个都需分配一个 pid，
2. thread group, process group, session group 都存在對應的 id
3. task_struct::pid, task_struct::tgid 是顶层 namespace 的对应 pid tgid 的快捷表示, 具體代碼可以看 copy_process 對於 pid 的賦值
4. task_struct::thread_pid 是該 threadk

那么剩下的都很简单了:

```c
struct task_struct {
	pid_t				pid; // global pid
	pid_t				tgid; // global thread group pid

	/* PID/PID hash table linkage. */
	struct pid			*thread_pid;
	struct hlist_node		pid_links[PIDTYPE_MAX];
	struct list_head		thread_group;
	struct list_head		thread_node; // TODO
```

```c
/*
 * the helpers to get the pid's id seen from different namespaces
 *
 * pid_nr()    : global id, i.e. the id seen from the init namespace;
 * pid_vnr()   : virtual id, i.e. the id seen from the pid namespace of
 *               current.
 * pid_nr_ns() : id seen from the ns specified.
 *
 * see also task_xid_nr() etc in include/linux/sched.h
 */

static inline pid_t pid_nr(struct pid *pid)
{
	pid_t nr = 0;
	if (pid)
		nr = pid->numbers[0].nr;
	return nr;
}
```


```c
/*
 * struct upid is used to get the id of the struct pid, as it is
 * seen in particular namespace. Later the struct pid is found with
 * find_pid_ns() using the int nr and struct pid_namespace *ns.
 */

struct upid {
	int nr;
	struct pid_namespace *ns;
};

struct pid
{
	refcount_t count;
	unsigned int level; // 每一個 task 都會對應一個 pid, level 表示 task 當前所在 thread 的位置
	spinlock_t lock;
  // 用於指向其所在的 thread group, process group, session group 的 pid
  // 參考 attach_pid, 通過成員 tasks 可以將其掛在 task_struct::
	/* lists of tasks that use this pid */
	struct hlist_head tasks[PIDTYPE_MAX];
	struct hlist_head inodes; // TODO 應該是 pidfd
	/* wait queue for pidfd notifications */
	wait_queue_head_t wait_pidfd;
	struct rcu_head rcu;
	struct upid numbers[1];
};
```

attach_pid : 讓 thread group, process group, session group 的 leader 知道自己掌控的 pid 有那些
init_task_pid : 让 task 知道其 thread group, prcess group, session group leader 的 pid

```c
static inline void
init_task_pid(struct task_struct *task, enum pid_type type, struct pid *pid)
{
	if (type == PIDTYPE_PID)
		task->thread_pid = pid;
	else
		task->signal->pids[type] = pid;
}

/*
 * attach_pid() must be called with the tasklist_lock write-held.
 */
void attach_pid(struct task_struct *task, enum pid_type type)
{
	struct pid *pid = *task_pid_ptr(task, type);
	hlist_add_head_rcu(&task->pid_links[type], &pid->tasks[type]);
}
```

最後理解一下 pid 和 namespace :
```c
struct pid_namespace *task_active_pid_ns(struct task_struct *tsk)
{
  // task_pid(tsk) : task_struct->thread_pid
  // task_struct::thread_pid 就是該 task 的 pid
	return ns_of_pid(task_pid(tsk));
}

static inline struct pid_namespace *ns_of_pid(struct pid *pid)
{
	struct pid_namespace *ns = NULL;
	if (pid)
    // pid 是跟随 thread 的，通过 level 就可以知道其 ns
		ns = pid->numbers[pid->level].ns;
	return ns;
}

// ns 控制了一个 pid 空间，idr 加速访问
struct pid *find_pid_ns(int nr, struct pid_namespace *ns)
{
	return idr_find(&ns->idr, nr);
}
```

- 从 alloc_pid 中，對於一個 thread, 會給每一個 namespace 中間存放一個 id


綜合實踐，syscall getpid 是如何實現的 ?
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
	return task_tgid_vnr(current);
}

static inline pid_t task_tgid_vnr(struct task_struct *tsk)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_TGID, NULL);
}


pid_t __task_pid_nr_ns(struct task_struct *task, enum pid_type type,
			struct pid_namespace *ns)
{
	pid_t nr = 0;

	rcu_read_lock();
	if (!ns)
		ns = task_active_pid_ns(current); // 获取到 ns
	nr = pid_nr_ns(rcu_dereference(*task_pid_ptr(task, type)), ns); // 通过 task->thread_pid 获取 thread 对应的 pid
	rcu_read_unlock();

	return nr;
}

pid_t pid_nr_ns(struct pid *pid, struct pid_namespace *ns)
{
	struct upid *upid;
	pid_t nr = 0;

	if (pid && ns->level <= pid->level) {
		upid = &pid->numbers[ns->level]; // 获取到该 level 上的
		if (upid->ns == ns)
			nr = upid->nr;
	}
	return nr;
}

static struct pid **task_pid_ptr(struct task_struct *task, enum pid_type type)
{
	return (type == PIDTYPE_PID) ?
		&task->thread_pid :
		&task->signal->pids[type];
}
```

## pidfd
- https://man7.org/linux/man-pages/man2/pidfd_open.2.html
- [Completing the pidfd API](https://lwn.net/Articles/794707/)

rust 封装:
https://github.com/pop-os/pidfd
