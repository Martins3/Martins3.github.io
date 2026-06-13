# QEMU `AioContext` 与 GLib 连接机制

本文记录三个问题：

1. `AioContext` 的 fd 是怎么注册进 `GSource` 的
2. `aio_notify()` 为什么能唤醒 glib loop
3. 主线程为什么不用 `g_main_loop_run()`，而要自己 poll

## 1. `AioContext` 的 fd 是怎么注册进 `GSource` 的

先说结论：`AioContext` 里维护了一组 `AioHandler`，每个 handler 带一个 `GPollFD pfd`。这些 `GPollFD` 会被加到 `ctx->source` 这个 `GSource` 上，所以 glib 在 poll 这个 source 时，实际上也在 poll 这些 fd。

### 1.1 `AioContext` 本身就是 `GSource`

`AioContext` 不是“额外包了一层 glib 对象”，而是直接用 `g_source_new()` 分配出来的：

- `ctx = (AioContext *) g_source_new(&aio_source_funcs, sizeof(AioContext));`
- 代码见 `util/async.c`

因此 `AioContext` 头部就是 `GSource`，后续 `aio_get_g_source()` 直接返回 `&ctx->source`：

- `GSource *aio_get_g_source(AioContext *ctx)`
- `g_source_ref(&ctx->source);`
- `return &ctx->source;`

### 1.2 fd handler 的注册入口

某个 fd 要进入 `AioContext`，通常走：

- `aio_set_fd_handler(ctx, fd, io_read, io_write, io_poll, io_poll_ready, opaque)`

这段代码在 `util/aio-posix.c`。它会：

1. 查找或创建一个 `AioHandler`
2. 填写 `new_node->pfd.fd`
3. 按读写回调设置 `new_node->pfd.events`
4. 调用 `ctx->fdmon_ops->update(ctx, old_node, new_node)`

最关键的是最后一步。`fdmon_ops` 决定“这个 fd 如何挂到 `GSource` 上”。

### 1.3 默认 poll backend 的做法

POSIX 下 `aio_context_setup()` 默认先用 poll backend：

- `ctx->fdmon_ops = &fdmon_poll_ops;`

如果当前 backend 是 `poll`，则 `update()` 会走 `util/fdmon-poll.c` 中的 `fdmon_poll_update()`：

- 删除旧 fd：`g_source_remove_poll(&ctx->source, &old_node->pfd)`
- 添加新 fd：`g_source_add_poll(&ctx->source, &new_node->pfd)`

这就是“fd 直接注册进 `GSource`”的实际实现。

所以在 poll backend 下：

- 每个业务 fd 都对应一个 `GPollFD`
- 每个 `GPollFD` 都直接挂在 `ctx->source` 上
- glib poll `ctx->source` 时，本质上也在 poll 这些 fd

### 1.4 epoll backend 的做法

当 fd 数量变多时，QEMU 会尝试从 poll 升级到 epoll：

- `fdmon_epoll_try_upgrade(ctx, npfd)`

升级后，glib 不再直接盯每个业务 fd，而是盯一个 `epollfd`。

在 `util/fdmon-epoll.c` 中：

- 业务 fd 通过 `epoll_ctl(EPOLL_CTL_ADD/MOD/DEL, ...)` 加入内核 epoll 实例
- `ctx->fdmon_ops = &fdmon_epoll_ops`
- 然后执行：
  `ctx->epollfd_tag = g_source_add_unix_fd(&ctx->source, ctx->epollfd, G_IO_IN);`

也就是说：

- poll backend：业务 fd 直接 `g_source_add_poll()` 到 `ctx->source`
- epoll backend：业务 fd 先进 `epollfd`，glib 只监听 `epollfd`

### 1.5 iouring backend

也是类似，通过 iouring 也是将 ring_fd 注册到 glib 中:
```c
bool fdmon_io_uring_setup(AioContext *ctx, Error **errp)
{
    int ret;

    ctx->io_uring_fd_tag = NULL;

    ret = io_uring_queue_init(FDMON_IO_URING_ENTRIES, &ctx->fdmon_io_uring, 0);
    if (ret != 0) {
        error_setg_errno(errp, -ret, "Failed to initialize io_uring");
        return false;
    }

    QSLIST_INIT(&ctx->submit_list);
    QSIMPLEQ_INIT(&ctx->cqe_handler_ready_list);
    ctx->fdmon_ops = &fdmon_io_uring_ops;
    ctx->io_uring_fd_tag = g_source_add_unix_fd(&ctx->source,
            ctx->fdmon_io_uring.ring_fd, G_IO_IN);
    return true;
}
```
其实都是类似的，也就是注册一个总的 fd ，然后接受到事件之后，逐个处理。

## 3. 主线程为什么不用 `g_main_loop_run()`，而要自己 poll

其实就是将 glib 需要 poll 的 fd 拿出来直接调用 ppoll

### 3.1 主线程的实际做法

主线程关键路径在 `util/main-loop.c` 的 `os_host_main_loop_wait()`。

它做的不是直接：

- `g_main_loop_run()`

而是下面这套流程：

1. `g_main_context_acquire(context)`
2. `g_main_context_prepare(context, &max_priority)`
3. `g_main_context_query(...)` 取出 glib 需要 poll 的 `GPollFD[]`
4. 把这些 fd 放入 QEMU 自己的 `gpollfds`
5. 解锁 `BQL` 和 `replay_mutex`
6. 调 `qemu_poll_ns(...)`
7. 重新加锁
8. 调 `g_main_context_check(...)`
9. 如果 ready，再 `g_main_context_dispatch(...)`

也就是说，主线程模式下：

- glib 提供“我想监听哪些 fd、超时多久”的信息
- 真正执行阻塞等待的是 QEMU 自己
- glib 只负责后半段的 check/dispatch

### 3.2 为什么不能直接 `g_main_loop_run()`

#### 原因一：QEMU 要统一 timeout 决策

glib 只知道 glib 自己的 source timeout。

但 QEMU 主循环还要考虑：

- QEMU clocks/timers
- 其他主循环事件源

所以 `glib_pollfds_fill()` 里会把 glib 给出的 timeout 和 QEMU 自己的 timeout 做合并，取最早那个：

- `*cur_timeout = qemu_soonest_timeout(timeout_ns, *cur_timeout);`

如果直接 `g_main_loop_run()`，这个“统一裁决 timeout”的能力就弱很多。

#### 原因二：QEMU 要精确控制锁边界

在真正阻塞前，主线程会：

- `bql_unlock();`
- `replay_mutex_unlock();`

阻塞返回后再：

- `replay_mutex_lock();`
- `bql_lock();`

这段边界非常关键，代码就在 `os_host_main_loop_wait()`。

如果直接进入 `g_main_loop_run()`，QEMU 很难精确规定“在哪个阻塞点前释放 BQL，在哪个点后重新拿回来”。

#### 原因三：QEMU 主线程本身就是总调度器

主线程除了 glib source，还要处理：

- QEMU 自己的 timer/clock 体系
- 信号相关逻辑
- replay 协调
- 其他需要在统一主循环中调度的机制

因此主线程不是把 glib 当总事件循环，而是把 glib 当作：

- 一个能够提供 `pollfd + timeout + dispatch` 能力的子系统

最终总控权仍然在 QEMU 自己的 main loop 手里。

## 为什么 main 中需要集成两个

- qemu_aio_context 里的东西，允许被 aio_poll() 直接驱动
- iohandler_ctx 里的东西，只允许作为“main loop 的 iohandler”被驱动

```c
/*
 * Functions to operate on the I/O handler AioContext.
 * This context runs on top of main loop. We can't reuse qemu_aio_context
 * because iohandlers mustn't be polled by aio_poll(qemu_aio_context).
 */
static AioContext *iohandler_ctx;
```

aio_ctx_dispatch 里的 ctx 指针是谁：
```txt
  - ctx == qemu_get_aio_context() -> qemu_aio_context
  - ctx == iohandler_get_aio_context() -> iohandler_ctx
  - 否则大概率是某个 IOThread 的 AioContext
```

原来是可以通过这个来研究的:

```txt
qemu handlers iohandler_ctx
qemu handlers qemu_aio_context
```
但是对比了一下，只能说，这个也完全看不懂这个差不啊

qemu handlers iohandler_ctx
```txt
  io_read = 0x556abfc364b0 <virtio_queue_host_notifier_read>, * 71
  io_read = 0x556abfc3ec50 <vhost_virtqueue_error_notifier>, * 4
  io_read = 0x556ac004e240 <sigfd_handler>,
  io_read = 0x556ac004d060 <aio_context_notifier_cb>,
```

qemu handlers qemu_aio_context
```txt
  io_read = 0x556abfc364b0 <virtio_queue_host_notifier_read>, * 65
  io_read = 0x556abff62b70 <qemu_laio_completion_cb>,
  io_read = 0x556ac004d060 <aio_context_notifier_cb>,
```

### aio_poll 是可以嵌套调用的

nbd_server_free 中就是一个经典案例:

首先发起请求，然后希望等所有的 callbakc 完成，直到 server->connections > 0
```c
    QLIST_FOREACH_SAFE(conn, &server->conns, next, tmp) {
        qio_channel_shutdown(QIO_CHANNEL(conn->cioc), QIO_CHANNEL_SHUTDOWN_BOTH,
                             NULL);
    }

    AIO_WAIT_WHILE_UNLOCKED(NULL, server->connections > 0);
```

## 为什么 iothread 中需要 glib

也不是什么大问题了，应该是很小的点了:
```c
static void *iothread_run(void *opaque)
{
    // ...
    while (iothread->running) {
        /*
         * Note: from functional-wise the g_main_loop_run() below can
         * already cover the aio_poll() events, but we can't run the
         * main loop unconditionally because explicit aio_poll() here
         * is faster than g_main_loop_run() when we do not need the
         * gcontext at all (e.g., pure block layer iothreads).  In
         * other words, when we want to run the gcontext with the
         * iothread we need to pay some performance for functionality.
         */
        aio_poll(iothread->ctx, true);

        /*
         * We must check the running state again in case it was
         * changed in previous aio_poll()
         */
        if (iothread->running && qatomic_read(&iothread->run_gcontext)) {
            g_main_loop_run(iothread->main_loop);
        }
    }
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
