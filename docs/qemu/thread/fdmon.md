# FDMonOps

关联的文件:
- util/fdmon-epoll.c
- util/fdmon-io_uring.c
- util/fdmon-poll.c

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - fdmon_io_uring_wait
            - io_uring_submit_and_wait
              - io_uring_submit_and_wait


### 添加新的需要监听的 fd

- aio_set_fd_handler 会调用 FDMonOps::update 来需要监听的 fd 更新到 epollfd 中。
  - fdmon_epoll_update : 调用 epoll_ctl 系统调用而已
  - fdmon_poll_update : poll 不需要做任何事情，ppoll 调用的时候，需要自动组装所有的
  - fdmon_io_uring_update : io uring 通过调用 io_uring_prep_poll_add 来实现监听

在操作的时候，需要 QemuLockCnt list_lock; 来保护，防止

## main loop 下的两个模式

main loop thread 中总是用的 qemu_poll_ns 来监听:

```c
ctx->epollfd_tag = g_source_add_unix_fd(&ctx->source,
                         ctx->epollfd,
                         G_IO_IN);
```

poll 模式，poll 中直接监听所有的 fd:
```txt
qemu_aio_context source
├── fd1
├── fd2
├── fd3
└── ...
```

epoll 或者 io_uring 模式，poll 来监听一个 epollfd / io_uring_fd ，所有的 fd 都被 epollfd / io_uring_fd 来监听
```txt
qemu_aio_context source
└── epollfd
    ├── fd1
    ├── fd2
    ├── fd3
    └── ...
```

### iothread 不存在这个需求

FDMonOps::wait 注册的三个 hook ，这是只有 iothread 才会调用的
- fdmon_epoll_wait : 使用 AioContext::epollfd + epoll_wait
- fdmon_poll_wait : 使用全局变量 pollfds, pollfds 是 fdmon_poll_wait 从 AioContext::aio_handlers 初始化得到的，和 main loop 非常类似了
- fdmon_io_uring_wait : 调用 liburing 提供的 io_uring_submit_and_wait 然后来监听 AioContext::fdmon_io_uring


## 实现细节
```c
/*
 * These thread-local variables are used only in fdmon_poll_wait() around the
 * call to the poll() system call.  In particular they are not used while
 * aio_poll is performing callbacks, which makes it much easier to think about
 * reentrancy!
 *
 * Stack-allocated arrays would be perfect but they have size limitations;
 * heap allocation is expensive enough that we want to reuse arrays across
 * calls to aio_poll().  And because poll() has to be called without holding
 * any lock, the arrays cannot be stored in AioContext.  Thread-local data
 * has none of the disadvantages of these three options.
 */
static __thread GPollFD *pollfds;
static __thread AioHandler **nodes;
static __thread unsigned npfd, nalloc;
static __thread Notifier pollfds_cleanup_notifier;

static void fdmon_poll_update(AioContext *ctx,
                              AioHandler *old_node,
                              AioHandler *new_node)
{
    /* Do nothing, AioHandler already contains the state we'll need */
}
```

## qemu 中如何处理 epoll 返回 EINTR 的

1. AIO epoll/poll 主路径

aio_poll() 调用 ctx->fdmon_ops->wait(...) 后不看返回值，只继续 dispatch BH/timer/ready handler。

也就是根本就不关心这个错误。

2. 主循环 main_loop : main_loop_wait 中不去区分，设置 MAIN_LOOP_POLL_ERR

3. io_uring_submit() 中才特殊处理:

```c
    /*
     * Loop to handle signals in both cases:
     * 1. If no SQEs were submitted, then -EINTR is returned.
     * 2. If SQEs were submitted then the number of SQEs submitted is returned
     *    rather than -EINTR.
     */
    do {
        ret = io_uring_submit_and_wait(&ctx->fdmon_io_uring, wait_nr);
    } while (ret == -EINTR ||
             (ret >= 0 && wait_nr > io_uring_cq_ready(&ctx->fdmon_io_uring)));

    assert(ret >= 0);
```

其他情况，QEMU 有通用宏 include/qemu/osdep.h
RETRY_ON_EINTR(expr)

## iouring 的改动

  2025 年修改已经不只是为了 fd 监控性能，而是为了让每个 AioContext 都提供通用的：

  aio_add_sqe()

  这样块层、FUSE 等模块可以复用 AioContext 自带的 io_uring，而不必各自创建 ring。补丁系列说明 (https://lists.gnu.org/archive/html/qemu-block/2025-03/msg00432.html)

  随后：

  047dabef97bd
  block/io_uring: use aio_add_sqe()

  把磁盘 aio=io_uring 和 fdmon io_uring 合并到同一个 ring，收益是：

  - 少创建一个 io_uring；
  - 不再需要 fdmon 去监控另一个 disk-I/O ring fd；
  - SQE 可以在 event-loop iteration 末尾批量提交；
  - 减少 io_uring_enter()；
  - 删除重复的 completion/submission 管理代码。

这个其实改动其实有意思:
1. 如果是两个 iouring 队列
	- iouring 监听 ----> 磁盘文件 iouring 的 fd
2. 如果一个 iouring 队列
	- iouring 监听所有的 fd 以及提交的所有的 io
		- 这个只能是 io uring 给io handler 做，因为只有

## 参考
- https://vmsplice.net/~stefan/stefanha-fosdem-2021.pdf

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
