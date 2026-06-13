# FDMonOps

## qemu 有那些多路复用技术
<!-- 9635c129-3222-429d-8c4f-e89a74ef1b20 -->

基本流程:

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - fdmon_poll_wait
            - qemu_poll_ns
              - ppoll
                - ppoll

关联的文件:
- util/fdmon-epoll.c
- util/fdmon-io_uring.c
- util/fdmon-poll.c


### FDMonOps::wait
- fdmon_epoll_wait : 使用 AioContext::epollfd
- fdmon_poll_wait : 使用全局变量 pollfds, 这个东西是在 fdmon_poll_wait 从 AioContext::aio_handlers 初始化得到的
- fdmon_io_uring_wait : 调用 liburing 提供的 io_uring_submit_and_wait 然后来监听 AioContext::fdmon_io_uring

### 添加新的需要监听的 fd

- aio_set_fd_handler 会调用 FDMonOps::update 来需要监听的 fd 更新到 epollfd 中。
  - fdmon_epoll_update : 调用 epoll_ctl 系统调用而已
  - fdmon_poll_update : poll 不需要做任何事情，ppoll 调用的时候，需要自动组装所有的
  - fdmon_io_uring_update : io uring 通过调用 io_uring_prep_poll_add 来实现监听

在操作的时候，需要 QemuLockCnt list_lock; 来保护，防止

## qemu 中 glib 的监听机制的地方
<!-- b79d423e-f613-40ef-8032-0c67601170e5 -->

其实就是使用 g_source_new 的地方:

1. qio channel 的所有后端都需要定义

```c
static void
qio_channel_null_class_init(ObjectClass *klass,
                            const void *class_data G_GNUC_UNUSED)
{
    QIOChannelClass *ioc_klass = QIO_CHANNEL_CLASS(klass);

    ioc_klass->io_writev = qio_channel_null_writev;
    ioc_klass->io_readv = qio_channel_null_readv;
    ioc_klass->io_set_blocking = qio_channel_null_set_blocking;
    ioc_klass->io_seek = qio_channel_null_seek;
    ioc_klass->io_close = qio_channel_null_close;
    ioc_klass->io_create_watch = qio_channel_null_create_watch; // 这里必须获取到一个 channel 的
    ioc_klass->io_set_aio_fd_handler = qio_channel_null_set_aio_fd_handler;
}
```

2. chardev/char-fd.c 和 chardev/char-io.c : 暂时不看了

3. aio_context_new 中

在 aio_set_fd_handler 中会去调用 g_source_add_poll 的
但是需要知道，aio context 未必一定回去使用 glib 的

(或者说，qemu 中哪些地方可以完全不用 g_source_new 的
所以，他们需要注册 AioContext 吗?
)

## qemu 中有三个 thread 在调用 ppoll
<!-- f6cd118b-8c5e-4cea-a1af-97461adb9917 -->

### 为什么是调用 poll

```c
void aio_context_setup(AioContext *ctx)
{
    ctx->fdmon_ops = &fdmon_poll_ops;
    ctx->epollfd = -1;

    /* Use the fastest fd monitoring implementation if available */
    if (fdmon_io_uring_setup(ctx)) {
        return;
    }

    fdmon_epoll_setup(ctx);
}
```

测试来看， fdmon_io_uring_setup 会成功，所以就是用 iouring 了，
但是我监控到的全是 ppoll 啊

一通调试，发现在这个函数中重新选择使用 ppoll:
```c
void aio_context_use_g_source(AioContext *ctx)
{
    /*
     * Disable io_uring when the glib main loop is used because it doesn't
     * support mixed glib/aio_poll() usage. It relies on aio_poll() being
     * called regularly so that changes to the monitored file descriptors are
     * submitted, otherwise a list of pending fd handlers builds up.
     */
    fdmon_io_uring_destroy(ctx);
    aio_free_deleted_handlers(ctx);
}
```

这个函数基本上，必然被调用

### 为什么有三个 thread

#### monitor 专用的 iothread

- thread_start
  - start_thread
    - qemu_thread_start
      - iothread_run
        - g_main_loop_run
          - g_main_context_iterate_unlocked.isra
            - ppoll

- thread_start
  - start_thread
    - qemu_thread_start
      - iothread_run
        - g_main_loop_run
          - g_main_context_iterate_unlocked.isra
            - g_main_context_prepare_unlocked
              - io_watch_poll_prepare
                - tcp_chr_read_poll
                  - monitor_can_read : monitor/qmp.c

居然 qmp 在这个 iothread 中做的

```c
static void monitor_iothread_init(void)
{
    mon_iothread = iothread_create("mon_iothread", &error_abort);
}
```

#### virtio blk iothread
- thread_start
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - fdmon_poll_wait
            - qemu_poll_ns
              - ppoll
                - ppoll

这种的 iothread 是通过 qom 构建的:
- main
  - qemu_init
    - qemu_create_early_backends
      - object_option_foreach_add
        - user_creatable_add_qapi
          - user_creatable_add_type
            - object_new_with_type
              - object_initialize_with_type
                - object_init_with_type
                  - iothread_instance_init

#### main loop
基本上监听任何东西:

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - qemu_poll_ns
            - ppoll
              - ppoll

#### 总结，从这里我们发现，iothread_run 有两个向下调用的路线
- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - fdmon_poll_wait
            - qemu_poll_ns
              - ppoll
                - ppoll

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - g_main_loop_run
          - g_main_context_iterate_unlocked.isra
            - ppoll

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

## 2026-03-25 发现默认的 fdmon 已经修改 iouring 了
```txt
$ p ctx->fdmon_ops
$1 = (const FDMonOps *) 0x556ac1192308 <fdmon_io_uring_ops>
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
