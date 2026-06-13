# iouring 的 cancel 设计
<!-- 8bfea0fe-c824-476c-a224-a5e7a37f8a83 -->

仔细想想，cancel 需要考虑的问题:
- 如果没有提交给硬件?
- 如果已经提交给硬件?
- 如果没提交到硬件，但是已经进入到 iouring 的控制之外了，例如 fs 了

io_uring/cancel.c

https://man7.org/linux/man-pages/man3/io_uring_prep_cancel.3.html

一些讨论
https://www.reddit.com/r/rust/comments/1gfi5r1/async_rust_is_not_safe_with_io_uring/
https://news.ycombinator.com/item?id=41992975

## 基本使用

如果是异步提交:
```c
const struct io_issue_def io_issue_defs[] = {
	[IORING_OP_ASYNC_CANCEL] = {
		.audit_skip		= 1,
		.prep			= io_async_cancel_prep,
		.issue			= io_async_cancel,
	},
```

如果是同步提交:
```c
enum io_uring_register_op {
	/* sync cancelation API */
	IORING_REGISTER_SYNC_CANCEL		= 24,
```

无论是同步还是异步，提交的时候都是可以使用这些 flag 来确定
到底是取消哪些 io .
```c
/*
 * ASYNC_CANCEL flags.
 *
 * IORING_ASYNC_CANCEL_ALL	Cancel all requests that match the given key
 * IORING_ASYNC_CANCEL_FD	Key off 'fd' for cancelation rather than the
 *				request 'user_data'
 * IORING_ASYNC_CANCEL_ANY	Match any request
 * IORING_ASYNC_CANCEL_FD_FIXED	'fd' passed in is a fixed descriptor
 * IORING_ASYNC_CANCEL_USERDATA	Match on user_data, default for no other key
 * IORING_ASYNC_CANCEL_OP	Match request based on opcode
 */
#define IORING_ASYNC_CANCEL_ALL	(1U << 0)
#define IORING_ASYNC_CANCEL_FD	(1U << 1)
#define IORING_ASYNC_CANCEL_ANY	(1U << 2)
#define IORING_ASYNC_CANCEL_FD_FIXED	(1U << 3)
#define IORING_ASYNC_CANCEL_USERDATA	(1U << 4)
#define IORING_ASYNC_CANCEL_OP	(1U << 5)
```

`__set_notify_signal` 的 基本的调用路线为
- __io_wq_worker_cancel <- io_wq_worker_cancel <- io_acct_cancel_running_work <- io_wq_cancel_running_work <- io_wq_cancel_cb
- io_wq_worker_wake <- io_wq_exit_workers <- io_wq_put_and_exit <- io_uring_clean_tctx

的确就是用来 wake up 等待的 io wq 的。

## [ ] 当 cqe wait 的时候，应该等待在什么地方?

```txt
@[
        io_wake_function+5 (当唤醒的时候)
        __wake_up_common+117
        __wake_up+54
        __io_submit_flush_completions+433
        ctx_flush_and_put+49
        io_handle_tw_list+177
        tctx_task_work_run+81
        tctx_task_work+58
        task_work_run+89
        io_run_task_work+78 (本来是等待在这里的)
        io_cqring_wait+923
        __do_sys_io_uring_enter+321
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
```
按道理，这个 backtrace 应该是在中断中的啊。

既然 signal_pending() 关于 io_uring 的修改，就是为了是实现
对于内核中所有调用 signal_pending() 的位置自动修改。


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
