## QEMU Event Loop

和 QEMU Event Loop 关联的文件

- util/async.c : AioContext 处理 bh 和 coroutine 相关的操作
- util/aio-posix.c : 定义了 aio_dispatch_handler, aio_poll, aio_set_fd_handler 等核心函数
- util/thread-pool.c
- main-loop.c : main loop thread
- iothread.c : IOThread thread

在 QEMU 中，用于 event loop 的线程为 main loop thread 和 IOThread，其中 IOThread 需要 explicit 的配置才可以被使用。
也就是说，默认情况下就是 main loop thread 和 vCPU thread 相互交互。

### main loop thread
QEMU 的第一个 thread 启动了各个 vCPU 之后，然后就会调用 ppoll 来进行实践监听。

相关代码在 main-loop.c 中间，下面是 main loop

- main
  - qemu_main_loop
    - main_loop_wait
      - os_host_main_loop_wait
        - qemu_poll_ns
          - ppoll

在 glib 中进行 event loop 是通过调用 g_main_loop_run 来进行进行的，但是 QEMU 的 main loop thread 存在更强的自定义，也就是
os_host_main_loop_wait

- os_host_main_loop_wait
  - glib_pollfds_fill
    - g_main_context_query : 调用 glib 的库，将需要监听的 fd 取出来，放到 gpollfds 中
  - qemu_poll_ns : 调用 poll 来监听保存到 gpollfds 中的 fd
  - glib_pollfds_poll
    - g_main_context_dispatch : 调用监听的 fd 的 callback 函数

当存在 fd ready 之后，其执行流程为:
- g_main_context_dispatch
  - aio_ctx_dispatch
    - aio_dispatch
        - aio_dispatch_handlers
            - aio_dispatch_handler
              -  qemu_luring_completion_cb

这就是 aio_set_fd_handler 的任务，对于一个监听的 fd, 会创建 `AioHandler` 来保存这个 fd 关联的 hook 函数

需要指出的是，AioHandler::io_poll 用于用户态的 poll 操作，找到其注册的三个 hook 函数，都是简单查询一下一个变量，
如果发现已经存在 fd ready 了，那么就可以直接返回。io_poll 注册的 hook 为:
- aio_context_notifier_poll
- qemu_luring_poll_cb : Returns how many unconsumed entries are ready in the CQ ring
- virtio_queue_host_notifier_aio_poll

实现真的非常的平易近人:
```c
/* Returns true if aio_notify() was called (e.g. a BH was scheduled) */
static bool aio_context_notifier_poll(void *opaque)
{
    EventNotifier *e = opaque;
    AioContext *ctx = container_of(e, AioContext, notifier);

    return qatomic_read(&ctx->notified);
}
```

- main
  - `main_loop`
    - `main_loop_should_exit`
    - `main_loop_wait`
      - `os_host_main_loop_wait`
        - `qemu_poll_ns`
          - ppoll
        - `glib_pollfds_poll` : 获取所需要监听的 fd，并且计算一个最小的超时时间
          - `g_main_context_check`
          - `g_main_context_dispatch`


使用 signalfd 作为例子来分析事件处理过程:
- `qemu_init_main_loop`
  - `qemu_signal_init`
    - `qemu_signalfd` : 调用 syscall 获取一个 sysfd 啊
  - `aio_context_new`

使用 signalfd 作为例子的确是不错的


## 所以，glib + 两个 context

1. 一个循环中，如何保证不会出现互相的阻塞
2. 都是分别监听那些 fd 的，可以列出来吗?

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
