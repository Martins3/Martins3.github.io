## signal pending

1. 调用 signal_pending : 在无限循环中间，需要等待可以让用户程序 kill
2. 调用 fatal_signal_pending :

- `__alloc_contig_migrate_range`
  - fatal_signal_pending 为什么要在这里调用?
    - 或者说，调用 fatal_signal_pending 的原理是什么?

- [x] 一个进程在什么时候检查自己的信号(执行 signal handler)
  - [x] syscall 返回的时候 ? (不是，int / syscall 返回到用户态的时候)
      - 如果这是真的，那么 SA_RESTART 的作用无法解释，syscall 都返回了，怎么可能被 handler 打断
        - 此时解释 syscall_restart 的理由就是，其实有的系统调用，在执行的会检查 signal_pending, 如果发生了 signal_pending, 会提前结束。
      - 通过检查 x86 的 signal.c 发生检查的位置在 	exit_to_user_mode_prepare 的调用下
      - arm64 的代码在 ret_to_user 的时候检查, 这一点是统一的

- hugetlbfs_fallocate : 根本看不懂这个哇
```c
        cond_resched();

        /*
         * fallocate(2) manpage permits EINTR; we may have been
         * interrupted because we are using up too much memory.
         */
        if (signal_pending(current)) {
            error = -EINTR;
            break;
        }
```

- [ ] find the caller of `signal_pending` ?

实际上的代码在 kernel/entry/
irqentry_exit_to_user_mode 和 syscall_exit_to_user_mode


需要的分析的细节:
2. generic_file_buffered_read 中间会调用 fatal_signal_pending，那么各种检查函数的要求是什么 ?

```txt
➜  linux git:(master) ag signal_pending | wc -l
741
```

- [ ] 假如一个 process 在睡眠，当其接受到信号了，如果让调度器开始调度它

## 是醒过来之后必须调用 signal_pending
https://www.kernel.org/doc/html/latest/kernel-hacking/hacking.html#ioctls-not-writing-a-new-system-call

> After you slept you should check if a signal occurred: the Unix/Linux way of handling signals is to temporarily exit the system call with the -ERESTARTSYS error. The system call entry code will switch back to user context, process the signal handler and then your system call will be restarted (unless the user disabled that). So you should be prepared to process the restart, e.g. if you’re in the middle of manipulating some data structure.
>
> if (signal_pending(current))
>         return -ERESTARTSYS;

醒来之后，就需要检查一下 signal pending ，如果存在 pending 的 signal，之后重启该系统调用

```c
SYSCALL_DEFINE0(pause)
{
	while (!signal_pending(current)) {
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}
	return -ERESTARTNOHAND;
}
```
- ERESTARTNOHAND 和 ERESTARTSYS 什么关系?

## 为什么 generic_perform_write 调用 fatal_signal_pending 而不是 signal_pending 啊

```c
static inline int restart_syscall(void)
{
	set_tsk_thread_flag(current, TIF_SIGPENDING);
	return -ERESTARTNOINTR;
}
```

## 分析一下这个基本函数
```c
static inline int task_sigpending(struct task_struct *p)
{
	return unlikely(test_tsk_thread_flag(p,TIF_SIGPENDING));
}

static inline int signal_pending(struct task_struct *p)
{
	/*
	 * TIF_NOTIFY_SIGNAL isn't really a signal, but it requires the same
	 * behavior in terms of ensuring that we break out of wait loops
	 * so that notify signal callbacks can be processed.
	 */
	if (unlikely(test_tsk_thread_flag(p, TIF_NOTIFY_SIGNAL)))
		return 1;
	return task_sigpending(p);
}
```

- shrink_inactive_list
```c
/*
 * shrink_inactive_list() is a helper for shrink_node().  It returns the number
 * of reclaimed pages
 */
static unsigned long shrink_inactive_list(unsigned long nr_to_scan,
		struct lruvec *lruvec, struct scan_control *sc,
		enum lru_list lru)
{
	LIST_HEAD(folio_list);
	unsigned long nr_scanned;
	unsigned int nr_reclaimed = 0;
	unsigned long nr_taken;
	struct reclaim_stat stat;
	bool file = is_file_lru(lru);
	enum vm_event_item item;
	struct pglist_data *pgdat = lruvec_pgdat(lruvec);
	bool stalled = false;

	while (unlikely(too_many_isolated(pgdat, file, sc))) {
		if (stalled)
			return 0;

		/* wait a bit for the reclaimer. */
		stalled = true;
		reclaim_throttle(pgdat, VMSCAN_THROTTLE_ISOLATED);

		/* We are about to die and free our memory. Return now. */
		if (fatal_signal_pending(current))
			return SWAP_CLUSTER_MAX;
	}
```
为什么这些地方不是检查 signal 的?


## 为什么 signal_pending 的
```c
/*
 * Reclaims as many pages from the given memcg as possible.
 *
 * Caller is responsible for holding css reference for memcg.
 */
static int mem_cgroup_force_empty(struct mem_cgroup *memcg)
{
	int nr_retries = MAX_RECLAIM_RETRIES;

	/* we call try-to-free pages for make this cgroup empty */
	lru_add_drain_all();

	drain_all_stock(memcg);

	/* try to free all pages in this cgroup */
	while (nr_retries && page_counter_read(&memcg->memory)) {
		if (signal_pending(current))
			return -EINTR;

		if (!try_to_free_mem_cgroup_pages(memcg, 1, GFP_KERNEL,
						  MEMCG_RECLAIM_MAY_SWAP))
			nr_retries--;
	}

	return 0;
}
```

## 问题并不简单
- https://stackoverflow.com/questions/41474299/checking-if-errno-eintr-what-does-it-mean
