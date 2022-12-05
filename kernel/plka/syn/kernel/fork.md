# kernel/fork.c

## TODO
1. sched_fork && wake_up_new_task : 一生之敌
2. copy_process 中间的 copy 函数真的让人窒息
    1. copy_semundo : IPC 需要拷贝吗 ?
3. ftrace_graph_exit_task : ftrace_graph 是个啥
4. task_struct::stack task_struct::thread_info 的作用

4. 内核如何产生自己的线程的 ?

## 设置 context 的位置
1. eip 指向同一个位置才对
2. 内核的 stack 应该拷贝，这样才可以返回到相同的位置啊 !
    1. 但是 alloc_thread_stack_node 找到分配的内容
    2. 始终不知道 fork 架构相关的内容在哪里

## syscall
- clone
- unshare
- vfork
- fork
- set_tid_address

## children 和 sibling
```c
struct task_struct {
  // ...
	/*
	 * Children/sibling form the list of natural children:
	 */
	struct list_head		children; // parent 依赖这一个挂载自己的 children
	struct list_head		sibling; // children 依赖这个将自己挂到parent 的 children 上
  // ...
```

```c
void walk_process_tree(struct task_struct *top, proc_visitor visitor, void *data) // todo 好吧，有点恶心
{
	struct task_struct *leader, *parent, *child;
	int res;

	read_lock(&tasklist_lock);
	leader = top = top->group_leader;
down:
	for_each_thread(leader, parent) {
		list_for_each_entry(child, &parent->children, sibling) {
			res = visitor(child, data);
			if (res) {
				if (res < 0)
					goto out;
				leader = child; // 一旦找到立刻重新搜索
				goto down;
			}
up:
			;
		}
	}

	if (leader != top) {
		child = leader;
		parent = child->real_parent;
		leader = parent->group_leader;
		goto up; // todo 除非理解了 for_each_thread 和 list_for_each_entry 在忽然切换了 parent 和 leader 的效果是什么 ? 诡异的 goto up
	}
out:
	read_unlock(&tasklist_lock);
}
```

## `_do_fork`
1. stack 的处理 ?  copy_process
2. 加入的调用的队列中间 ? wake_up_new_task

3. wait_for_vfork_done ?
4. pid 相关的处理

```c
/*
 *  Ok, this is the main fork-routine.
 *
 * It copies the process, and if successful kick-starts
 * it and waits for it to finish using the VM if required.
 *
 * args->exit_signal is expected to be checked for sanity by the caller.
 */
long _do_fork(struct kernel_clone_args *args)
{
	u64 clone_flags = args->flags;
	struct completion vfork;
	struct pid *pid;
	struct task_struct *p;
	int trace = 0;
	long nr;

	/*
	 * Determine whether and which event to report to ptracer.  When
	 * called from kernel_thread or CLONE_UNTRACED is explicitly
	 * requested, no event is reported; otherwise, report if the event
	 * for the type of forking is enabled.
	 */
	if (!(clone_flags & CLONE_UNTRACED)) {
		if (clone_flags & CLONE_VFORK)
			trace = PTRACE_EVENT_VFORK;
		else if (args->exit_signal != SIGCHLD)
			trace = PTRACE_EVENT_CLONE;
		else
			trace = PTRACE_EVENT_FORK;

		if (likely(!ptrace_event_enabled(current, trace))) // todo prace 相关的 flag 处理，不懂
			trace = 0;
	}

	p = copy_process(NULL, trace, NUMA_NO_NODE, args);
	add_latent_entropy();

	if (IS_ERR(p))
		return PTR_ERR(p);

	/*
	 * Do this prior waking up the new thread - the thread pointer
	 * might get invalid after that point, if the thread exits quickly.
	 */
	trace_sched_process_fork(current, p); // todo 神经病

	pid = get_task_pid(p, PIDTYPE_PID); // 支持 CLONE_PARENT_SETTID
	nr = pid_vnr(pid);

	if (clone_flags & CLONE_PARENT_SETTID)
		put_user(nr, args->parent_tid);

	if (clone_flags & CLONE_VFORK) { // todo vfork
		p->vfork_done = &vfork;
		init_completion(&vfork);
		get_task_struct(p);
	}

	wake_up_new_task(p); // todo

	/* forking complete and child started to run, tell ptracer */
	if (unlikely(trace))
		ptrace_event_pid(trace, pid);

	if (clone_flags & CLONE_VFORK) { // todo 对称的 vfork
		if (!wait_for_vfork_done(p, &vfork))
			ptrace_event_pid(PTRACE_EVENT_VFORK_DONE, pid);
	}

	put_pid(pid);
	return nr;
}
```


## signal

## mm : mmput mm_alloc
1. mm_init
    1. init_new_context : 架构相关
    2. mm_alloc_pgd : 架构相关

## copy(0) : dup_task_struct :  创建新的 task_struct 出来
1. alloc_thread_stack_node : 创建 `task_struct->stack`

## copy(1) : copy_mm
1. copy_mm => dup_mm => dup_mmap => vm_area_dup, anon_vma_fork

## copy(1) : copy_fs
> @todo 以及 各种内容
