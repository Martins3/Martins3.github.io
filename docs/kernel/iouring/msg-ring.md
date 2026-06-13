## iouring msg ring
<!-- 98416107-abb8-4e5d-8531-da35e2325c61 -->

实现源码: io_uring/msg_ring.c

op 为 IORING_OP_MSG_RING ，一共两个模式:
```c
/*
 * IORING_OP_MSG_RING command types, stored in sqe->addr
 */
enum io_uring_msg_ring_flags {
	IORING_MSG_DATA,	/* pass sqe->len as 'res' and off as user_data */
	IORING_MSG_SEND_FD,	/* send a registered fd to another ring */
};
```

commit e6130eba8a84 ("io_uring: add support for passing fixed file descriptors")

这里解释了两个使用场景:

> Two common use cases for this are:
>
> 1) Server needs to be shutdown or restarted, pass file descriptors to
>    another one
>
> 2) Backend is split, and one accepts connections, while others then get
>   the fd passed and handle the actual connection.


哦，提供了
io_uring_prep_msg_ring 和 io_uring_prep_msg_ring_fd 来作为
内核中，不得不说，使用 io_uring_sqe::addr 来区分，还是有点傻的。

```c
IOURINGINLINE void io_uring_prep_msg_ring(struct io_uring_sqe *sqe, int fd,
					  unsigned int len, __u64 data,
					  unsigned int flags)
	LIBURING_NOEXCEPT
{
	io_uring_prep_rw(IORING_OP_MSG_RING, sqe, fd, NULL, len, data);
	sqe->msg_ring_flags = flags;
}

IOURINGINLINE void io_uring_prep_msg_ring_fd(struct io_uring_sqe *sqe, int fd,
					     int source_fd, int target_fd,
					     __u64 data, unsigned int flags)
	LIBURING_NOEXCEPT
{
	io_uring_prep_rw(IORING_OP_MSG_RING, sqe, fd,
			 (void *) (uintptr_t) IORING_MSG_SEND_FD, 0, data);
	sqe->addr3 = source_fd;
	/* offset by 1 for allocation */
	if ((unsigned int) target_fd == IORING_FILE_INDEX_ALLOC)
		target_fd--;
	__io_uring_set_target_fixed_file(sqe, target_fd);
	sqe->msg_ring_flags = flags;
}
```

## 一些 lock 的优化
https://lore.kernel.org/all/cover.1670384893.git.asml.silence@gmail.com/

https://www.phoronix.com/news/Decouple-TASK_WORK-TWA_SIGNAL

commit 0617bb500bfa ("io_uring/msg_ring: improve handling of target CQE posting")

这个解释了 io_msg_send_fd 中为什么去使用 remote install 的操作:
```c
	if (io_msg_need_remote(target_ctx))
		return io_msg_fd_remote(req);
	return io_msg_install_complete(req, issue_flags);
```

没细看，但是优化就是说，让任务在

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
