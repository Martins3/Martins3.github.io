# kernel/exit.c

## KeyNote

## exit_group : 调用 do_group_eixt() => do_exit

将任务交给了 signal 机制和 do_exit 了

Man `exit_group`
    This system call is equivalent to _exit(2) except that it terminates not only the calling thread, but all threads in the calling process's thread group.

```c
/*
 * Take down every thread in the group.  This is called by fatal signals
 * as well as by sys_exit_group (below).
 */
void
do_group_exit(int exit_code)
{
	struct signal_struct *sig = current->signal;

	BUG_ON(exit_code & 0x80); /* core dumps don't get here */

	if (signal_group_exit(sig))
		exit_code = sig->group_exit_code;
	else if (!thread_group_empty(current)) {
		struct sighand_struct *const sighand = current->sighand;

		spin_lock_irq(&sighand->siglock);
		if (signal_group_exit(sig))
			/* Another thread got here before we took the lock.  */
			exit_code = sig->group_exit_code;
		else {
			sig->group_exit_code = exit_code;
			sig->flags = SIGNAL_GROUP_EXIT;
			zap_other_threads(current);
		}
		spin_unlock_irq(&sighand->siglock);
	}

	do_exit(exit_code);
	/* NOTREACHED */
}
```


## do_exit : 绝对核心业务

虽然，流程很简单，但是设计内容很多!

ipc 相关的:
1. free_pipe_info
2. exit_sem.c
3. exit_signal.c

cgroup_exit : 仅仅被 do_exit 调用
exit_task_namespace

准备下一步的操作:
schedule()
do_task_dead()
preempt_count_add()


资源:
exit_files
exit_fs
put_page

## exit_notify :  用于 do_exit 处理父辈和字辈的事情。

通知一下 task_group 之类的东西，似乎这是接着 exit.c 前面上千行的内容。

整个调用过程大概是这样的，之所以比 ucore 更加复杂，其中的原因是 process group 吧!

```c
/*
 * Send signals to all our closest relatives so that they know
 * to properly mourn us..
 */
static void exit_notify(struct task_struct *tsk, int group_dead)
  - static void kill_orphaned_pgrp(struct task_struct *tsk, struct task_struct *parent)
  - void release_task(struct task_struct *p)
  - static void forget_original_parent(struct task_struct *father, struct list_head *dead)
      - static void reparent_leader(struct task_struct *father, struct task_struct *p, struct list_head *dead)
          - static void reparent_leader(struct task_struct *father, struct task_struct *p, struct list_head *dead)
      - static struct task_struct *find_new_reaper(struct task_struct *father, struct task_struct *child_reaper)
      - static struct task_struct *find_child_reaper(struct task_struct *father)
          - static struct task_struct *find_alive_thread(struct task_struct *p)
```

## release_task
1. `__exit_signal`
2. do_notify_parent 


## do_wait

从 exit.c:1500 之后都是其各种系统调用的封装函数。

1. 存在 waitqueue 相关的内容。@todo
```c
	init_waitqueue_func_entry(&wo->child_wait, child_wait_callback);
	wo->child_wait.private = current;
	add_wait_queue(&current->signal->wait_chldexit, &wo->child_wait);
```

2. forget_original_parent 和 do_wait_thread 算是主要利用 parent 和 sibling 的地方，所以，其实可以得出结论，之所以为了形成 tree 的结构，就是为了实现 wait 机制来回收资源的。

```c
/*
 * Do the work of do_wait() for one thread in the group, @tsk.
 *
 * -ECHILD should be in ->notask_error before the first call.
 * Returns nonzero for a final return, when we have unlocked tasklist_lock.
 * Returns zero if the search for a child should continue; then
 * ->notask_error is 0 if there were any eligible children,
 * or still -ECHILD.
 */
static int do_wait_thread(struct wait_opts *wo, struct task_struct *tsk)
{
	struct task_struct *p;

	list_for_each_entry(p, &tsk->children, sibling) {
		int ret = wait_consider_task(wo, 0, p);

		if (ret)
			return ret;
	}

	return 0;
}
```


