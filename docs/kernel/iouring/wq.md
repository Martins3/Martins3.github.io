# workqueue
## 分析下，但是也许我们自己也可以写 : io_uring/io-wq.c

- io_queue_iowq
  - io_wq_enqueue
    - io_wq_insert_work
    - io_wq_create_worker

现在是任务都提交到:

```c
struct io_wq_acct {
	unsigned nr_workers;
	unsigned max_workers;
	int index;
	atomic_t nr_running;
	raw_spinlock_t lock;
	struct io_wq_work_list work_list;
	unsigned long flags;
};
```

io thread 就是一个 kthread ，并没有什么奇怪的东西:

- io_wq_worker
  - io_worker_handle_work
    - io_get_next_work : io_wq_acct::work_list 中取出来

分析 io_req_task_submit 的三种场景:

### 一共存在多少个 kthread ?

### polling thread 应该是没有和 io-wq 不是一个进程吧 ?

### 找到为什么 io wq 创建自己 workqueue 的原因，找到对应的 patch

应该按照把这些实验都坐下的:


### struct io_uring_task *task_struct::io_uring 的作用是啥?

### io-wq 也是实现 poll ?

io-wq used to just block when offloaded.

## io-wq 为什么需要屏蔽掉 thread ?

https://lore.kernel.org/lkml/20210326003928.978750-2-axboe@kernel.dk/


## 当然是靠写代码的方式: io_run_local_work

- 为什么这里理解为 local ? 这个是分水岭吧

```c
void __io_req_task_work_add(struct io_kiocb *req, unsigned flags)
{
	if (req->ctx->flags & IORING_SETUP_DEFER_TASKRUN) {
		rcu_read_lock();
		io_req_local_work_add(req, flags);
		rcu_read_unlock();
	} else {
		io_req_normal_work_add(req);
	}
}
```

```txt
History:        #0
Commit:         c0e0d6ba25f180ab76d3c18f8b360a119dffa634
Author:         Dylan Yudaken <dylany@fb.com>
Committer:      Jens Axboe <axboe@kernel.dk>
Author Date:    Tue 30 Aug 2022 08:50:10 PM CST
Committer Date: Thu 22 Sep 2022 12:30:42 AM CST

io_uring: add IORING_SETUP_DEFER_TASKRUN

Allow deferring async tasks until the user calls io_uring_enter(2) with
the IORING_ENTER_GETEVENTS flag. Enable this mode with a flag at
io_uring_setup time. This functionality requires that the later
io_uring_enter will be called from the same submission task, and therefore
restrict this flag to work only when IORING_SETUP_SINGLE_ISSUER is also
set.

Being able to hand pick when tasks are run prevents the problem where
there is current work to be done, however task work runs anyway.

For example, a common workload would obtain a batch of CQEs, and process
each one. Interrupting this to additional taskwork would add latency but
not gain anything. If instead task work is deferred to just before more
CQEs are obtained then no additional latency is added.

The way this is implemented is by trying to keep task work local to a
io_ring_ctx, rather than to the submission task. This is required, as the
application will want to wake up only a single io_ring_ctx at a time to
process work, and so the lists of work have to be kept separate.

This has some other benefits like not having to check the task continually
in handle_tw_list (and potentially unlocking/locking those), and reducing
locks in the submit & process completions path.

There are networking cases where using this option can reduce request
latency by 50%. For example a contrived example using [1] where the client
sends 2k data and receives the same data back while doing some system
calls (to trigger task work) shows this reduction. The reason ends up
being that if sending responses is delayed by processing task work, then
the client side sits idle. Whereas reordering the sends first means that
the client runs it's workload in parallel with the local task work.
```

## [ ] 为什么将 workqueue 替代为 wq-io

https://lore.kernel.org/linux-block/20191024134439.28498-1-axboe@kernel.dk/T/

> This adds support for io-wq, a smaller and specialized thread pool
> implementation. This is meant to replace workqueues for io_uring. Among
> the reasons for this addition are:
>
> - We can assign memory context smarter and more persistently if we
>   manage the life time of threads.
>
> - We can drop various work-arounds we have in io_uring, like the
>   async_list.
>
> - We can implement hashed work insertion, to manage concurrency of
>   buffered writes without needing a) an extra workqueue, or b)
>   needlessly making the concurrency of said workqueue very low
>   which hurts performance of multiple buffered file writers.
>
> - We can implement cancel through signals, for cancelling
>   interruptible work like read/write (or send/recv) to/from sockets.
>
> - We need the above cancel for being able to assign and use file tables
>   from a process.
>
> - We can implement a more thorough cancel operation in general.
>
> - We need it to move towards a syslet/threadlet model for even faster
>   async execution. For that we need to take ownership of the used
>   threads.
>
> This list is just off the top of my head. Performance should be the
> same, or better, at least that's what I've seen in my testing. io-wq
> supports basic NUMA functionality, setting up a pool per node.
>
> io-wq hooks up to the scheduler schedule in/out just like workqueue
> and uses that to drive the need for more/less workers.


## io_uring 的 thread 是做啥的?
__io_run_local_work

同时，可以看看这个东西:
https://git.kernel.dk/cgit/linux-block/commit/?h=for-5.5/io_uring&id=771b53d033e8663abdf59704806aa856b236dcdb

## io wq
- io_wq_submit_work
  - while loop { io_issue_sqe; io_arm_poll_handler; }
  - io_issue_sqe
  - io_arm_poll_handler
    - `__io_arm_poll_handler`
      - vfs_poll

## sched_submit_work

似乎这个也是关联的:
```txt
	/*
	 * If a worker goes to sleep, notify and ask workqueue whether it
	 * wants to wake up a task to maintain concurrency.
	 */
	if (task_flags & PF_WQ_WORKER)
		wq_worker_sleeping(tsk);
	else if (task_flags & PF_IO_WORKER)
		io_wq_worker_sleeping(tsk);
```


## kernel thread 可以接受信号吗?
不可以吧

## 为什么需要一个单独的 wq ?

```c
/*
 * This is like kernel_clone(), but shaved down and tailored to just
 * creating io_uring workers. It returns a created task, or an error pointer.
 * The returned task is inactive, and the caller must fire it up through
 * wake_up_new_task(p). All signals are blocked in the created task.
 */
struct task_struct *create_io_thread(int (*fn)(void *), void *arg, int node)
{
	unsigned long flags = CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|
				CLONE_IO;
	struct kernel_clone_args args = {
		.flags		= ((lower_32_bits(flags) | CLONE_VM |
				    CLONE_UNTRACED) & ~CSIGNAL),
		.exit_signal	= (lower_32_bits(flags) & CSIGNAL),
		.fn		= fn,
		.fn_arg		= arg,
		.io_thread	= 1,
		.user_worker	= 1,
	};

	return copy_process(NULL, 0, node, &args);
}
```

## 哦，原来如此哦

使用 op-fsync.c 来测试

```txt
@[
        create_io_thread+5
        create_io_worker+191
        io_wq_enqueue+446
        io_submit_sqes+668
        __do_sys_io_uring_enter+520
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 5
```

```txt
@[
        io_worker_handle_work+1
        io_wq_worker+222
        ret_from_fork+242
        ret_from_fork_asm+26
]: 1
```

```txt
sudo perf trace -e io_uring:io_uring_queue_async_work
     0.000 op-fsync.out/11437 io_uring:io_uring_queue_async_work(ctx: 0xffff8881114b0800, req: 0xffff888105b61a00, user_data: 123, opcode: 3, flags: 549756338192, work: 0xffff888105b61ad8, op_str: "FSYNC")
```

io_queue_iowq 的调用地方太多了


哦，原来还是可以选择的:
```c
void io_req_task_submit(struct io_kiocb *req, io_tw_token_t tw)
{
	struct io_ring_ctx *ctx = req->ctx;

	io_tw_lock(ctx, tw);
	if (unlikely(io_should_terminate_tw(ctx)))
		io_req_defer_failed(req, -EFAULT);
	else if (req->flags & REQ_F_FORCE_ASYNC)
		io_queue_iowq(req);
	else
		io_queue_sqe(req, 0);
}
```

REQ_F_FORCE_ASYNC 会根据一下地方判断:

```txt
@[
       create_io_thread+5
       create_io_worker+191
       io_wq_enqueue+446
       io_submit_sqes+668
       __do_sys_io_uring_enter+520
       do_syscall_64+132
       entry_SYSCALL_64_after_hwframe+118
]: 1
```

### bounded 和 unbounded worker
https://man7.org/linux/man-pages/man3/io_uring_register_iowq_max_workers.3.html

1. 什么时候 reap 掉多余的
2. 真的会创建两个类型吗？他们的区别是什么?
  - struct io_wq::acct 中多余的，但是好像也没啥差别

### REQ_F_FORCE_ASYNC 如何理解?

```c
void io_req_task_submit(struct io_kiocb *req, struct io_tw_state *ts)
{
	io_tw_lock(req->ctx, ts);
	/* req->task == current here, checking PF_EXITING is safe */
	if (unlikely(req->task->flags & PF_EXITING)) // TODO 太高级了
		io_req_defer_failed(req, -EFAULT);
	else if (req->flags & REQ_F_FORCE_ASYNC) // 异步的提交
		io_queue_iowq(req, ts);
	else
		io_queue_sqe(req); // 直接执行提交的函数
}
```

异步的执行，来释放所有的请求:

```txt
@[
        io_ring_ctx_wait_and_kill+1
        io_uring_release+24
        __fput+227
        task_work_run+89
        do_exit+483
        do_group_exit+48
        __x64_sys_exit_group+24
        x64_sys_call+5359
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

哦，才意识到，原来 io_uring 中才有 io_kiocb
```c
struct io_kiocb {
  // ...

	struct io_cqe			cqe;

	struct io_ring_ctx		*ctx;
	struct io_uring_task		*tctx;

  // ...
```

io_queue_iowq 中:

```txt
	struct io_uring_task *tctx = req->tctx;

	io_wq_enqueue(tctx->io_wq, &req->work);
```
也就是，req 会告知 io_uring_task

io_wq_insert_work : 先加入工作

io_wq_create_worker : 然后看情况创建 io-wq 来处理

## 到底是如何产生 worker 的

```txt
@[
        create_io_thread+5
        create_io_worker+174
        io_wq_enqueue+446
        io_submit_sqes+711
        __do_sys_io_uring_enter+516
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 7
```

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
