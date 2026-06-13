## vhost 协议的定义
<!-- 8b0cee55-e4a0-4512-befd-e6490c1919ac -->

https://qemu-project.gitlab.io/qemu/interop/vhost-user.html

vhost 只是传输层协议，它传输的“内容”（即 Virtqueue 的环形缓冲区结构、设备描述符等）是由 OASIS 标准组织 定义的。

该文档（`vhost-user.rst`）详细定义了 **vhost-user 协议**，这是一种用于在同一台主机上的两个用户态进程之间建立 virtqueue 共享的控制面协议 。

以下是该协议的核心内容总结：

1. 协议角色与通信
	* **前端（Front-end）**：共享其 virtqueues 的应用程序，典型代表是 **QEMU** 。
	* **后端（Back-end）**：virtqueues 的消费者，例如用户态以太网交换机（如 Snabbswitch）或块设备后端 。
	* **通信机制**：通过 **Unix domain socket** 进行通信 。
	* 利用辅助数据（ancillary data）中的 `SCM_RIGHTS` 来共享文件描述符（FD） 。
	* 两端均可作为客户端（连接方）或服务端（监听方） 。
2. 消息规范 : 每条 vhost-user 消息由 **消息头（Header）** 和 **有效载荷（Payload）** 组成：
	* **消息头**：包含 32 位的请求类型（request）、32 位的标志位（flags，包含版本、回复标识等）和 32 位的载荷大小（size） 。
	* **有效载荷**：根据请求类型不同，载荷可以是 64 位整数、vring 状态/地址描述、内存区域描述、IOTLB 消息或设备配置空间等 。
3. 核心功能与特性
	* **内存共享与映射**：前端通过 `VHOST_USER_SET_MEM_TABLE` 发送内存区域列表，后端据此将客体地址（Guest Address）映射到自己的进程空间 。
	* **多队列支持（MQ）**：通过协议扩展支持多队列，后端可通报其支持的最大队列数，前端使用唯一索引标识每个队列 。
	* **热迁移（Migration）**：
		* **脏页记录**：前端通过日志跟踪后端对内存的修改 。
		* **后端状态迁移**：支持将后端的内部状态从源端传输到目的端，实现透明切换 。
	* **IOMMU 与 IOTLB**：当协商了 `VIRTIO_F_IOMMU_PLATFORM` 时，通过 `VHOST_USER_IOTLB_MSG` 进行地址翻译条目的更新和使效 。
	* **Inflight I/O 跟踪**：为了支持后端崩溃后的重新连接，协议提供了共享缓冲区来记录处理中的描述符信息 。
4. 环（Ring）状态管理， Rings 具有独立的状态：
	* **Started/Stopped**：控制后端是否处理该环。当所有 vrings 停止时，设备进入 **挂起（Suspended）** 状态，不得修改客体内存或发送通知 。
	* **Enabled/Disabled**：控制后端处理环时的行为（如在禁用状态下丢弃发送的数据包） 。
5. 后端通信通道
	* 如果协商了 `VHOST_USER_PROTOCOL_F_BACKEND_REQ`，则会提供一个可选通道，允许后端主动向前端发起请求 。

- VIRTIO_F_IOMMU_PLATFORM 差不多可以理解
- VHOST_USER_PROTOCOL_F_BACKEND_REQ : 这个使用的场景是很少的，暂时不要考虑了

两个比较经典的复杂的问题，分别放到
./1-inflight-io.md
./2-migration.md


## vhost 支持那些后端类型
<!-- f663f6b8-25fb-451d-8ac3-a822857a2564 -->

一共三种，参考 include/hw/virtio/vhost-backend.h 中的定义:
```c
typedef enum VhostBackendType {
    VHOST_BACKEND_TYPE_NONE = 0,
    VHOST_BACKEND_TYPE_KERNEL = 1,
    VHOST_BACKEND_TYPE_USER = 2,
    VHOST_BACKEND_TYPE_VDPA = 3,
    VHOST_BACKEND_TYPE_MAX = 4,
} VhostBackendType;
```

每一个类型都是会定义自己的 VhostOps
```c
static int vhost_set_backend_type(struct vhost_dev *dev,
                                  VhostBackendType backend_type)
{
    int r = 0;

    switch (backend_type) {
#ifdef CONFIG_VHOST_KERNEL
    case VHOST_BACKEND_TYPE_KERNEL:
        dev->vhost_ops = &kernel_ops;
        break;
#endif
#ifdef CONFIG_VHOST_USER
    case VHOST_BACKEND_TYPE_USER:
        dev->vhost_ops = &user_ops;
        break;
#endif
#ifdef CONFIG_VHOST_VDPA
    case VHOST_BACKEND_TYPE_VDPA:
        dev->vhost_ops = &vdpa_ops;
        break;
#endif
    default:
        error_report("Unknown vhost backend type");
        r = -1;
    }

    if (r == 0) {
        assert(dev->vhost_ops->backend_type == backend_type);
    }

    return r;
}
```

### qemu 如何支持 vhost kernel backend
<!-- e1b8a95c-e77c-49b5-bbe5-c9a15c209bf1 -->

```c
const VhostOps kernel_ops = {
        .backend_type = VHOST_BACKEND_TYPE_KERNEL,
        .vhost_backend_init = vhost_kernel_init,
        .vhost_backend_cleanup = vhost_kernel_cleanup,
        .vhost_backend_memslots_limit = vhost_kernel_memslots_limit,
        .vhost_net_set_backend = vhost_kernel_net_set_backend,
        .vhost_scsi_set_endpoint = vhost_kernel_scsi_set_endpoint,
        .vhost_scsi_clear_endpoint = vhost_kernel_scsi_clear_endpoint,
        .vhost_scsi_get_abi_version = vhost_kernel_scsi_get_abi_version,
        .vhost_set_log_base = vhost_kernel_set_log_base,
        .vhost_set_mem_table = vhost_kernel_set_mem_table,
        .vhost_set_vring_addr = vhost_kernel_set_vring_addr,
        .vhost_set_vring_endian = vhost_kernel_set_vring_endian,
        .vhost_set_vring_num = vhost_kernel_set_vring_num,
        .vhost_set_vring_base = vhost_kernel_set_vring_base,
        .vhost_get_vring_base = vhost_kernel_get_vring_base,
        .vhost_set_vring_kick = vhost_kernel_set_vring_kick,
        .vhost_set_vring_call = vhost_kernel_set_vring_call,
        .vhost_set_vring_err = vhost_kernel_set_vring_err,
        .vhost_set_vring_busyloop_timeout =
                                vhost_kernel_set_vring_busyloop_timeout,
        .vhost_get_vring_worker = vhost_kernel_get_vring_worker,
        .vhost_attach_vring_worker = vhost_kernel_attach_vring_worker,
        .vhost_new_worker = vhost_kernel_new_worker,
        .vhost_free_worker = vhost_kernel_free_worker,
        .vhost_set_features = vhost_kernel_set_features,
        .vhost_get_features = vhost_kernel_get_features,
        .vhost_set_backend_cap = vhost_kernel_set_backend_cap,
        .vhost_set_owner = vhost_kernel_set_owner,
        .vhost_get_vq_index = vhost_kernel_get_vq_index,
        .vhost_vsock_set_guest_cid = vhost_kernel_vsock_set_guest_cid,
        .vhost_vsock_set_running = vhost_kernel_vsock_set_running,
        .vhost_set_iotlb_callback = vhost_kernel_set_iotlb_callback,
        .vhost_send_device_iotlb_msg = vhost_kernel_send_device_iotlb_msg,
};
```

- __clone3
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
                              - virtio_net_set_status
                                - virtio_net_vhost_status
                                  - vhost_net_start
                                    - vhost_net_start_one
                                      - vhost_dev_start
                                        - vhost_virtqueue_start
                                          - vhost_kernel_set_vring_base

```c
static int vhost_kernel_set_vring_base(struct vhost_dev *dev,
                                       struct vhost_vring_state *ring)
{
    return vhost_kernel_call(dev, VHOST_SET_VRING_BASE, ring);
}
```

vhost-net 是需要 qemu 在控制面的支持的，基本过程就是，guest 写配置空间，还是先需要退出
到 qemu 中，然后 qemu 将这些地址提交给内核，内核的工作就是完成数据面就可以了。

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

## vhost user net 的处理办法: net_vhost_user_event

```c
    case CHR_EVENT_CLOSED:
        /* a close event may happen during a read/write, but vhost
         * code assumes the vhost_dev remains setup, so delay the
         * stop & clear to idle.
         * FIXME: better handle failure in vhost code, remove bh
         */
        if (s->watch) {
            AioContext *ctx = qemu_get_current_aio_context();

            g_source_remove(s->watch);
            s->watch = 0;
            qemu_chr_fe_set_handlers(&s->chr, NULL, NULL, NULL, NULL,
                                     NULL, NULL, false);

            aio_bh_schedule_oneshot(ctx, chr_closed_bh, opaque);
        }
        break;
```

## 如果完成之后，如果发现当前是打开的，会立刻调用注册的 hook

例如在 vhost 中:
```c
static void vhost_user_blk_device_realize(DeviceState *dev, Error **errp)
{
    // ...
    do {
        if (*errp) {
            error_prepend(errp, "Reconnecting after error: ");
            error_report_err(*errp);
            *errp = NULL;
        }
        ret = vhost_user_blk_realize_connect(s, errp);
    } while (ret < 0 && retries--);

    if (ret < 0) {
        goto virtio_err;
    }

    /* we're fully initialized, now we can operate, so add the handler */
    qemu_chr_fe_set_handlers(&s->chardev,  NULL, NULL,
                             vhost_user_blk_event, NULL, (void *)dev,
                             NULL, true);
```

开机的时候可以观察到这样的日志:

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
                                            - qemu_chr_fe_set_handlers
                                              - vhost_user_blk_event

## 在 callback 中真的可以执行重连操作吗?
对于 vhost 不可以，但是更加简单的系统是没问题的

callback 中本来就是网络操作，在重连中

可以，虽然细节有待处理，但是可以测试出来如下结果:

- vhost_user_blk_realize_connect
  - vhost_user_blk_connect  (这个就是了)
    - vhost_dev_init
      - vhost_user_backend_init
        - vhost_user_get_features
          - vhost_user_get_u64
            - vhost_user_write
            - vhost_user_read

## vhost-vsock 看看如何使用


## vhost 的 request 那些是需要回复的?

```c
typedef struct {
    VhostUserRequest request;

#define VHOST_USER_VERSION_MASK     (0x3)
#define VHOST_USER_REPLY_MASK       (0x1 << 2)
#define VHOST_USER_NEED_REPLY_MASK  (0x1 << 3)
    uint32_t flags;
    uint32_t size; /* the following payload size */
} QEMU_PACKED VhostUserHeader;
```


1. 原始规范就要求后端必须回复的请求
这些请求自带回复语义，不需要设置 need_reply 标志，后端也必须回复。根据 docs/interop/vhost-user.rst：
• VHOST_USER_GET_FEATURES
• VHOST_USER_GET_PROTOCOL_FEATURES
• VHOST_USER_GET_VRING_BASE
• VHOST_USER_SET_LOG_BASE（如果协商了 VHOST_USER_PROTOCOL_F_LOG_SHMFD）
• VHOST_USER_GET_INFLIGHT_FD（如果协商了 VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD）

2. 通过 VHOST_USER_PROTOCOL_F_REPLY_ACK 扩展，QEMU 显式要求回复的请求
如果前后端协商了 VHOST_USER_PROTOCOL_F_REPLY_ACK，QEMU 会在发送某些 SET 请求时主动设置 need_reply，等待后端 ack。具体在代码里体现为：

 请求                                                 是否设 need_reply          说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 VHOST_USER_SET_MEM_TABLE					条件（若支持 REPLY_ACK）   等待后端确认内存表更新
 VHOST_USER_SET_VRING_ENABLE                          		无条件等待回复             防止控制平面和数据平面乱序，必须等后端 ack 才让 g uest 发请求
 VHOST_USER_SET_VRING_KICK / SET_VRING_CALL / SET_VRING_ERR	条件（若支持 REPLY_ACK）   等后端确认 FD 已安装
 VHOST_USER_SET_VRING_ADDR					条件（若 logging 启用）    确保后端真的在记录 dirty page
 VHOST_USER_SET_FEATURES                              		条件（若 logging 启用）    同上
 VHOST_USER_SET_BACKEND_REQ_FD                        		条件（若支持 REPLY_ACK）
 VHOST_USER_POSTCOPY_LISTEN                           		无条件                     postcopy 必须等后端准备好
 VHOST_USER_POSTCOPY_END                              		无条件                     同上
 VHOST_USER_NET_SET_MTU                               		条件（若支持 REPLY_ACK）   等后端确认 MTU 合法
 VHOST_USER_IOTLB_MSG                                 		无条件                     IOTLB 更新必须等确认
 VHOST_USER_SET_CONFIG                                		条件（若支持 REPLY_ACK）

3. 默认不需要回复的请求
大部分普通的 SET 操作默认不等待回复，例如：
• VHOST_USER_SET_OWNER
• VHOST_USER_SET_VRING_NUM
• VHOST_USER_SET_VRING_BASE（注意：GET 才需要回复，SET 不需要）
• VHOST_USER_SET_STATUS
• VHOST_USER_SET_PROTOCOL_FEATURES
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
总结：need_reply 不是按请求类型硬性写死的，而是取决于两点：
1. 原始规范已经规定某些 GET 请求必须有回复；
2. REPLY_ACK 扩展让 QEMU 可以对任何 SET 请求追加 need_reply，在代码中表现为：关键路径（如 SET_VRING_ENABLE、postcopy、IOTLB）无条件
   待回复；其他 SET 请求只在后端支持 VHOST_USER_PROTOCOL_F_REPLY_ACK 时才设标志。


```txt
History:        #0
Commit:         699f2e535d930017153423e0f326b34b29ad1f54
Author:         Denis Plotnikov <den-plotnikov@yandex-team.ru>
Committer:      Michael S. Tsirkin <mst@redhat.com>
Author Date:    Mon 09 Aug 2021 06:48:24 PM CST
Committer Date: Sat 04 Sep 2021 09:07:45 PM CST

vhost: make SET_VRING_ADDR, SET_FEATURES send replies

On vhost-user-blk migration, qemu normally sends a number of commands
to enable logging if VHOST_USER_PROTOCOL_F_LOG_SHMFD is negotiated.
Qemu sends VHOST_USER_SET_FEATURES to enable buffers logging and
VHOST_USER_SET_VRING_ADDR per each started ring to enable "used ring"
data logging.
The issue is that qemu doesn't wait for reply from the vhost daemon
for these commands which may result in races between qemu expectation
of logging starting and actual login starting in vhost daemon.

The race can appear as follows: on migration setup, qemu enables dirty page
logging by sending VHOST_USER_SET_FEATURES. The command doesn't arrive to a
vhost-user-blk daemon immediately and the daemon needs some time to turn the
logging on internally. If qemu doesn't wait for reply, after sending the
command, qemu may start migrateing memory pages to a destination. At this time,
the logging may not be actually turned on in the daemon but some guest pages,
which the daemon is about to write to, may have already been transferred
without logging to the destination. Since the logging wasn't turned on,
those pages won't be transferred again as dirty. So we may end up with
corrupted data on the destination.
The same scenario is applicable for "used ring" data logging, which is
turned on with VHOST_USER_SET_VRING_ADDR command.

To resolve this issue, this patch makes qemu wait for the command result
explicitly if VHOST_USER_PROTOCOL_F_REPLY_ACK is negotiated and logging enabled.

Signed-off-by: Denis Plotnikov <den-plotnikov@yandex-team.ru>

Message-Id: <20210809104824.78830-1-den-plotnikov@yandex-team.ru>
Reviewed-by: Michael S. Tsirkin <mst@redhat.com>
Signed-off-by: Michael S. Tsirkin <mst@redhat.com>
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
