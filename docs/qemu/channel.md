# qemu io/ 目录中的功能
<!-- 47de5f88-e8f1-41a1-a77b-94af8342c034 -->

简单来说，就是真正和外界打交道的代码:
```txt
  io
     channel-buffer.c
     channel-command.c
     channel-file.c
     channel-null.c
     channel-socket.c
     channel-tls.c
     channel-util.c
     channel-watch.c
     channel-websock.c
     channel.c
     dns-resolver.c
     meson.build
     net-listener.c
     task.c
     trace-events
     trace.h
```

从这里想到的:
```c
void socket_send_channel_create(QIOTaskFunc f, void *data)
{
    QIOChannelSocket *sioc = qio_channel_socket_new();
    qio_channel_socket_connect_async(sioc, outgoing_args.saddr,
                                     f, data, NULL, NULL);
}
```

应该是很简单的东西把，通过这两个 backtrace 基本上可以看到了，
channel 应该是 qemu 和外部世界打交道的，可以提供给 chardev ，也是可以提供给 multifd 的

而 ./chardev 的各种 char 的后端应该使用的也是使用的 ./channel

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - qio_net_listener_channel_func
                  - tcp_chr_new_client
                    - monitor_event
                      - monitor_printf
                        - monitor_vprintf
                          - monitor_puts
                            - monitor_puts_locked
                              - monitor_flush_locked
                                - monitor_flush_locked
                                  - qemu_chr_fe_write
                                    - qemu_chr_write
                                      - qemu_chr_write_buffer
                                        - tcp_chr_write
                                          - io_channel_send_full
                                            - qio_channel_socket_writev


- thread_start
  - start_thread
    - qemu_thread_start
      - multifd_send_thread
        - qio_channel_writev_full_all
          - qio_channel_socket_writev


## vu_message_read 来看 qemu io 的基本流程

首先在一个 while 循环中调用 qio_channel_readv_full ，将 header 读取到。

然后调用 qio_channel_readv_all_eof 来把所有。


https://github.com/yandex-cloud/yc-libvhost-server

## AioContext 在 QIOChannel 的作用是什么

没用那么大的作用:

在 fd 重新注册 handler
- qio_channel_yield
  - qio_channel_set_fd_handlers

配置上
qio_channel_restart_write
qio_channel_restart_read

当 fd 再有事件之后，那么就是调用这两个函数。

## migration/rdma.c 也是抽象为一个 channel 了
```c
static void qio_channel_rdma_class_init(ObjectClass *klass,
                                        const void *class_data G_GNUC_UNUSED)
{
    QIOChannelClass *ioc_klass = QIO_CHANNEL_CLASS(klass);

    ioc_klass->io_writev = qio_channel_rdma_writev;
    ioc_klass->io_readv = qio_channel_rdma_readv;
    ioc_klass->io_set_blocking = qio_channel_rdma_set_blocking;
    ioc_klass->io_close = qio_channel_rdma_close;
    ioc_klass->io_create_watch = qio_channel_rdma_create_watch;
    ioc_klass->io_set_aio_fd_handler = qio_channel_rdma_set_aio_fd_handler;
    ioc_klass->io_shutdown = qio_channel_rdma_shutdown;
}
```

## 和 chardev/char-socket.c 和 io/channel-socket.c 的关系
<!-- f22fc9b3-c90f-4fcb-a023-52a444ea2630 -->

chardev/char-socket.c 依赖底层的 io/channel-socket.c

chardev/char-socket.c 表示用 socket 来模拟 chardev ，核心是 chardev ，而
io/channel-socket.c 是 qemu 如何和 socket 打交道的封装，不一定用来模拟 chardev ，
也可以用于热迁移的数据发送。

他们的典型例子就是 chardev/char-socket.c:tcp_chr_write


```c
static int tcp_chr_write(Chardev *chr, const uint8_t *buf, int len)
{
    SocketChardev *s = SOCKET_CHARDEV(chr);

    if (s->state == TCP_CHARDEV_STATE_CONNECTED) {
        int ret =  io_channel_send_full(s->ioc, buf, len,
                                        s->write_msgfds,
                                        s->write_msgfds_num);

        /* free the written msgfds in any cases
         * other than ret < 0 && errno == EAGAIN
         */
        if (!(ret < 0 && EAGAIN == errno)
            && s->write_msgfds_num) {
            g_free(s->write_msgfds);
            s->write_msgfds = 0;
            s->write_msgfds_num = 0;
        }

        if (ret < 0 && errno != EAGAIN) {
            if (tcp_chr_read_poll(chr) <= 0) {
                /* Perform disconnect and return error. */
                trace_chr_socket_poll_err(chr, chr->label);
                tcp_chr_disconnect_locked(chr);
            } /* else let the read handler finish it properly */
        }

        return ret;
    } else {
        /* Indicate an error. */
        errno = EIO;
        return -1;
    }
}
```

## 问题
- io/channel-watch.c 是做什么的?
- io/channel-command.c 是做什么的? 似乎就是对于 fd 操作

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
