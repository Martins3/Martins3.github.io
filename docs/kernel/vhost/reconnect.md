# vhost 重连

## vhost 的重连机制
<!-- a36839e6-afda-46e1-8625-1c78106f1502 -->

(这个没写清楚)

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qmp_x_exit_preconfig
        - qemu_create_cli_devices
          - qemu_opts_foreach
            - device_init_func
              - qdev_device_add
                - qdev_device_add_from_qdict
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - pci_qdev_realize
                              - object_property_set_bool
                                - object_property_set_qobject
                                  - object_property_set
                                    - property_set_bool
                                      - device_set_realized
                                        - virtio_device_realize
                                          - vhost_user_blk_device_realize
                                            - vhost_user_blk_realize_connect
                                              - vhost_user_blk_connect
                                                - vhost_dev_init
                                                  - vhost_user_backend_init
                                                    - vhost_user_get_features
                                                      - vhost_user_get_u64
                                                        - vhost_user_write

看上去，guest os 发起，最后都是 qemu 代理的，然后发送给后端:

- thread_start
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_rw
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - flatview_write_continue_step
                    - memory_region_dispatch_write
                      - access_with_adjusted_size
                        - memory_region_write_accessor
                          - virtio_pci_common_write
                            - virtio_set_status
                              - vhost_user_blk_set_status
                                - vhost_user_blk_start
                                  - vhost_virtqueue_mask
                                    - vhost_set_vring_file
                                      - vhost_user_write

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_bh_poll
                      - vhost_user_async_close_bh
                        - vhost_user_blk_disconnect
                          - vhost_user_blk_stop
                            - vhost_dev_stop
                              - do_vhost_dev_stop
                                - vhost_dev_set_vring_enable
                                  - vhost_user_set_vring_enable
                                    - vhost_user_set_vring_enable
                                      - vhost_set_vring
                                        - vhost_user_write_sync
                                          - vhost_user_write


qemu 的实现只是需要注册这个就可以了:

```c
static void vhost_user_blk_event(void *opaque, QEMUChrEvent event)
{
    DeviceState *dev = opaque;
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VHostUserBlk *s = VHOST_USER_BLK(vdev);
    Error *local_err = NULL;

    switch (event) {
    case CHR_EVENT_OPENED:
        if (vhost_user_blk_connect(dev, &local_err) < 0) {
            error_report_err(local_err);
            qemu_chr_fe_disconnect(&s->chardev);
            return;
        }
        break;
    case CHR_EVENT_CLOSED:
        /* defer close until later to avoid circular close */
        vhost_user_async_close(dev, &s->chardev, &s->dev,
                               vhost_user_blk_disconnect);
        break;
    case CHR_EVENT_BREAK:
    case CHR_EVENT_MUX_IN:
    case CHR_EVENT_MUX_OUT:
        /* Ignore */
        break;
    }
}
```

之前将事件清零掉了，所以重新注册，不然之后是没有继续监听这个事件的。
```c
static void vhost_user_blk_disconnect(DeviceState *dev)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VHostUserBlk *s = VHOST_USER_BLK(vdev);

    if (!s->connected) {
        goto done;
    }
    s->connected = false;

    vhost_user_blk_stop(vdev);

    vhost_dev_cleanup(&s->dev);

done:
    /* Re-instate the event handler for new connections */
    qemu_chr_fe_set_handlers(&s->chardev, NULL, NULL, vhost_user_blk_event,
                             NULL, dev, NULL, true);
}
```

## usbredir 重连机制机制
<!-- 93fb7405-2b71-4dca-8c37-4654c874c96a -->

和上面的 vhost 重连差不多，不过我的确该搭建一下 usb redir 机制了

```c
static void usbredir_chardev_event(void *opaque, QEMUChrEvent event)
{
    USBRedirDevice *dev = opaque;

    switch (event) {
    case CHR_EVENT_OPENED:
        DPRINTF("chardev open\n");
        /* Make sure any pending closes are handled (no-op if none pending) */
        usbredir_chardev_close_bh(dev);
        qemu_bh_cancel(dev->chardev_close_bh);
        usbredir_create_parser(dev);
        break;
    case CHR_EVENT_CLOSED:
        DPRINTF("chardev close\n");
        qemu_bh_schedule(dev->chardev_close_bh);
        break;
    case CHR_EVENT_BREAK:
    case CHR_EVENT_MUX_IN:
    case CHR_EVENT_MUX_OUT:
        /* Ignore */
        break;
    }
}
```

## qemu vhost thread 重连的时候，如果 vhost backend 宕机，会有 bug
<!-- 000b3743-1e5c-4d44-965b-95351c0f0b59 -->

老版本 qemu 存在这个时序问题

```txt
迁移线程                              主线程
--------                              ------
recvmsg(... SCM_RIGHTS ...)
s->read_msgfds = msgfds
                                      收到 G_IO_HUP
                                      tcp_chr_free_connection()
                                      close(s->read_msgfds[0])
qemu_set_block(s->read_msgfds[0])
  -> fcntl(F_GETFL) = -1, EBADF
  -> assert
  -> QEMU abort
```

一个典型的 vhost-user  :：

1. live migration 期间，QEMU 向 vhost-user backend 发送 VHOST_USER_GET_INFLIGHT_FD。
2. backend 通过 Unix socket 返回应答，并用 SCM_RIGHTS 携带 inflight shared-memory FD。
3. backend 在发送应答后立即退出、重启或关闭控制连接。
4. QEMU 迁移线程在 vhost_user_read() 中接收应答和 FD
5. QEMU 主线程同时处理 socket HUP，并关闭 read_msgfds。
6. 接收线程继续初始化该 FD 时访问到已经关闭的描述符，可能触发同一个 qemu_set_block() 断言，使一次正常的 backend disconnect 升级为整个 QEMU 进程 abort。

因此，可以把问题概括为：

对于通过 chardev Unix socket 接收 SCM_RIGHTS FD 的场景，如果 peer 在发送“消息 + FD”后立即断开，QEMU 的接收线程与 HUP 清理线程可能并发操作同一个
read_msgfds。HUP 路径可能在接收线程完成 FD 初始化或取得所有权之前关闭该 FD，最终导致 EBADF 和 QEMU abort。

### 主线版本也有问题


上游主线已经不会沿你描述的路径触发 fcntl(F_GETFL) -> EBADF -> assert，
但 read_msgfds 的跨线程同步问题并没有被完整修复。

- chardev/char-socket.c:tcp_chr_recv 收到 FD 并保存到 read_msgfds 后，不再遍历 FD 调用 qemu_socket_set_block()。
- blocking/CLOEXEC 处理已下移到 io/channel-socket.c:qio_channel_handle_fds，发生在 FD 发布给 SocketChardev 之前。

  不过仍有残留风险：

- chardev/char-socket.c:tcp_chr_get_msgfds 无锁读取、复制、释放 read_msgfds。
- chardev/char-socket.c:tcp_chr_free_connection 也无锁关闭、释放同一数据。
- qemu_chr_fe_get_msgfds() 没有声明为 thread-safe。

所以由 vhost 线程执行同步 read/get-msgfds，同时主线程处理 HUP，
理论上仍可能出现 FD 被提前关闭、返回不到 FD，甚至数组释放竞态；

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
