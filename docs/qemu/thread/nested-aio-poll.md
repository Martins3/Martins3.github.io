# qemu nested aio_poll
<!-- 9006d727-2262-4917-930a-176c5f28eddf -->

aio_poll  可以在

1. blk_drain

AIO_WAIT_WHILE

nbd_server_free 中就是一个经典案例:

首先发起请求，然后希望等所有的 callbakc 完成，直到 server->connections > 0
```c
    QLIST_FOREACH_SAFE(conn, &server->conns, next, tmp) {
        qio_channel_shutdown(QIO_CHANNEL(conn->cioc), QIO_CHANNEL_SHUTDOWN_BOTH,
                             NULL);
    }

    // 这里的东西
    AIO_WAIT_WHILE_UNLOCKED(NULL, server->connections > 0);
```

实际上，我们出来的结果不是这样的:
- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_write
            - flatview_write
              - flatview_write_continue
                - flatview_write_continue_step
                  - memory_region_dispatch_write
                    - access_with_adjusted_size
                      - memory_region_write_accessor
                        - virtio_pci_common_write
                          - virtio_pci_stop_ioeventfd
                            - virtio_bus_stop_ioeventfd
                              - virtio_blk_stop_ioeventfd
                                - blk_drain

qmp_nbd_server_stop 是一个经典路线，这里就是会阻塞

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - handle_hmp_command_exec
                            - hmp_nbd_server_stop
                              - qmp_nbd_server_stop
                                - nbd_server_free (这里需要等所有的 in_flight 的 io 全部都结束)

## 这不会导致其他的任务阻塞，而是让代码继续在

AIO_WAIT_WHILE 中等待所有的任务，只是当前的任务会卡住而已，但是其他的 callback 会继续执行的。

### 为什么主线程还有两个 AioContext

你还观察到了：

```txt
qemu_aio_context
iohandler_ctx
```

这又容易让人感觉多出一套机制。

实际上两者都是 AioContext，也都作为 GSource 挂在默认 GMainContext。区别是
是否允许被嵌套 aio_poll() 驱动。

#### qemu_aio_context

可以显式调用：

```txt
aio_poll(qemu_aio_context, ...);
```

例如 AIO_WAIT_WHILE() 内部的嵌套事件循环。

#### iohandler_ctx

只应该由最外层 main loop 驱动，不能因为某个 block 请求调用了嵌套
aio_poll(qemu_aio_context)，就顺便执行全局 iohandler。

源码注释写得很直接：

```txt
/*
 * This context runs on top of main loop.
 * We can't reuse qemu_aio_context because iohandlers
 * mustn't be polled by aio_poll(qemu_aio_context).
 */
```

见 util/main-loop.c:617。

它是在隔离重入范围：

顶层 main loop
    ├── qemu_aio_context
    └── iohandler_ctx

嵌套 aio_poll(qemu_aio_context)
    ├── qemu_aio_context
    └── 不执行 iohandler_ctx

你用 qemu handlers 看到两个 context 中都有
virtio_queue_host_notifier_read，并不代表它们相同。callback 名字只能说明做
什么，context 归属决定它允许在哪种事件循环层级执行。


2026-08-20 : 其实关键在于 aio_poll 嵌套的问题了，到时候看看那些是 iohandler_ctx ，那些是 qemu_aio_context 就可以了吧

## main-loop.c 中的两个 AioContext 和一个 glib
<!-- 44da9544-dced-4fa1-897f-f84cf4a714f2 -->
```c
static AioContext *qemu_aio_context;
static AioContext *iohandler_ctx;
```

http://blog.vmsplice.net/2020/08/qemu-internals-event-loops.html

The reason why the main loop has two AioContexts is because
1. one, called **iohandler_ctx**, is used to implement older `qemu_set_fd_handler()` APIs whose handlers should
not run when the other AioContext, called **qemu_aio_context**, is run using aio_poll().
The QEMU block layer and newer code uses **qemu_aio_context** while older code uses **iohandler_ctx**.
Over time it may be possible to unify the two by converting iohandler_ctx handlers to safely execute in **qemu_aio_context**.

IOThreads have an AioContext and a glib GMainContext.
The AioContext is run using the aio_poll() API, which enables the advanced features of the event loop.
If a glib event loop is needed then the GMainContext can be run using g_main_loop_run() and the AioContext event sources will be included.

Code that relies on the AioContext `aio_*()` APIs will work with both the main loop and IOThreads.
Older code using `qemu_*()` APIs only works with the main loop. glib code works with both the main loop and IOThreads.

总结就是，由于各种历史兼容性原因，导致有两个 AioContext 和 glib


传统方法:
```c
void qemu_set_fd_handler(int fd,
                         IOHandler *fd_read,
                         IOHandler *fd_write,
                         void *opaque)
{
    iohandler_init();
    aio_set_fd_handler(iohandler_ctx, fd, fd_read, fd_write, NULL, NULL,
                       opaque);
}
```

## 两个东西

  ### 2. 挂入正常 main loop

  初始化中又执行：

  src = aio_get_g_source(qemu_aio_context);
  g_source_attach(src, NULL);

  因此它作为 GSource 挂在默认 GMainContext 上。

  QEMU 主循环不断执行：

  while (!main_loop_should_exit(&status)) {
      main_loop_wait(false);
  }

  见 system/runstate.c:945。

  调用关系是：

  qemu_main_loop()
    └─ main_loop_wait()
        └─ os_host_main_loop_wait()
            └─ g_main_context_dispatch()
                └─ dispatch qemu_aio_context 的 fd/timer/BH/coroutine

  因此正常情况下，每轮 QEMU main loop 都会推进它。

  ### 3. 嵌套等待时也会单独 poll 它

  同步等待常使用：

  aio_poll(qemu_get_aio_context(), true);

  尤其是 AIO_WAIT_WHILE()：

  while (cond) {
      aio_poll(qemu_get_aio_context(), true);
  }

  见 include/qemu/aio-wait.h:80。

  这时候只推进 qemu_aio_context，不会执行 iohandler_ctx。这正是上一篇博客所说的两个 context 必须分开的原因。

  ### 4. 哪些东西挂在上面

  当前代码通过 qemu_get_aio_context() 把多种任务放到主循环：

  - BlockBackend 默认归属，例如 blockdev.c:587
  - Job 的 timer/BH，例如 job.c:439
  - migration snapshot 回到主线程执行，例如 migration/savevm.c:3665
  - coroutine 切换回主循环，例如 qapi/qmp-dispatch.c:225
  - NBD、network、TPM、UI 等需要主循环执行的 callback
  - qemu_bh_new() 创建的传统主循环 BH，最终进入 util/main-loop.c:606 的 qemu_bh_new_full()

  一个典型模式是：

  aio_bh_schedule_oneshot(qemu_get_aio_context(), callback, opaque);

  含义是：“无论当前在哪个线程，把这个 callback 投递到 QEMU 主循环线程执行。”

  ### 5. 与 current AioContext 的区别

  qemu_get_aio_context()

  始终返回全局的主循环 qemu_aio_context。

  qemu_get_current_aio_context()

  返回当前执行线程所属的 context：

  - 主线程：qemu_aio_context
  - IOThread：该 IOThread 自己的 AioContext
  - 持有 BQL 的某些 vCPU 路径：回退到 qemu_aio_context

  实现见 util/async.c:726。

  最后要注意：

  qemu_set_fd_handler(...)

  并不使用 qemu_aio_context，而是注册到 iohandler_ctx。

  所以可以把两者概括为：

  qemu_aio_context
    主循环的现代 AIO、block、timer、BH、coroutine
    正常 main loop 和嵌套 aio_poll() 都可能执行

  iohandler_ctx
    qemu_set_fd_handler() 等传统 handler
    正常 main loop 执行
    aio_poll(qemu_aio_context) 不会执行

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
