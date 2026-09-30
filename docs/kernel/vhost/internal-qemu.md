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

## 一些代码分析

```c
typedef struct VhostUserMemory {
    uint32_t nregions;
    uint32_t padding;
    VhostUserMemoryRegion regions[VHOST_MEMORY_BASELINE_NREGIONS];
} VhostUserMemory;
```

为什么 VhostUserMemoryRegion regions 最多就是定义 8 个就够了，


为什么会定义两组东西?
```c
typedef struct VhostUserMemoryRegion {
    uint64_t guest_phys_addr;
    uint64_t memory_size;
    uint64_t userspace_addr;
    uint64_t mmap_offset;
} VhostUserMemoryRegion;

typedef struct VhostUserMemory {
    uint32_t nregions;
    uint32_t padding;
    VhostUserMemoryRegion regions[VHOST_MEMORY_BASELINE_NREGIONS];
} VhostUserMemory;

typedef struct VhostUserMemRegMsg {
    uint64_t padding;
    VhostUserMemoryRegion region;
} VhostUserMemRegMsg;
```

  - 协商了这个特性后，QEMU 会通过 VHOST_USER_GET_MAX_MEM_SLOTS 获取更大的上限，然后用
    VHOST_USER_ADD_MEM_REG / VHOST_USER_REM_MEM_REG 做增量维护，而不是让一个 VhostUserMemory 里塞超过 8 个


### 附录
1. mmap_offset 都是做什么用的?

```txt
• mmap_offset 表示该内存区域在共享文件描述符（fd）中的起始偏移量。
  具体来说，它的作用体现在两个方面：
  1. 告诉后端从文件的哪个位置开始 mmap()
  当 QEMU 使用 memory-backend-file 等后端时，Guest 内存块在文件中可能并不是从偏移 0 开始的。QEMU 在发送 VHOST_USER_SET_MEM_TABL…
  消息时，会把该内存块在文件中的偏移（ram_block->fd_offset）填入 mmap_offset 字段：
  // hw/virtio/vhost-user.c
  vhost_user_fill_msg_region(&region_buffer, reg, offset);
  vhost-user 后端（如 libvhost-user）收到消息后，会根据这个 mmap_offset 对 fd 进行 mmap()：
  // subprojects/libvhost-user/libvhost-user.c
  mmap_addr = mmap(0, msg_region->memory_size + mmap_offset,
                   prot, MAP_SHARED | MAP_NORESERVE, fd, fd_offset);
  2. 处理大页（hugepage）对齐的补偿
  如果底层使用了大页，mmap() 的文件偏移必须对齐到大页大小。当 mmap_offset 不对齐时，后端会：
  - 将对齐的部分（ALIGN_DOWN(mmap_offset, hugepagesize)）作为 mmap() 的 fd_offset；
  - 把剩余未对齐的差值保留为内部的 mmap_offset。
  这样，通过 mmap_addr + mmap_offset 就能得到 Guest 内存区域在文件中的实际起始地址，从而正确进行 GPA（Guest Physical Address HVA 的转换。

  mmap_offset 就是该 VhostUserMemoryRegion 在其对应文件描述符中的字节偏移，后端靠它来确定 mmap() 的起点，从而与 QEMU 的内存布局
  保持一致。
```
差不多是这个意思，也就是由于都是传输 fd 过去，所以，如果 qemu 这端配置的是有 offset ，那么 client 端也是需要有 offset 的。

2. 的确，是探测到具体的设备，才需要发送 memory region 给 vhost

当时的虚拟机 dmesg 可以看到，正好探测到 virtiofs 的结果:
```txt
fuse: init (API version 7.45)
zram: Added device: zram0
VFIO - User Level meta-driver version: 0.3
[martins3:send_add_regions:727]
[martins3:send_add_regions:727]
[martins3:send_add_regions:727]
virtiofs virtio6: discovered new tag: myfs
virtio_blk virtio1: 2/0/0 default/read/poll queues
virtio_blk virtio1: [vda] 20971520 512-byte logical blocks (10.7 GB/10.0 GiB)
```
- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_write
            - flatview_write
              - flatview_write_continue,
                - flatview_write_continue_step
                  - memory_region_dispatch_write
                    - access_with_adjusted_size
                      - memory_region_write_accessor
                        - virtio_pci_common_write
                          - virtio_set_status
                            - vuf_set_status
                              - vuf_start
                                - vhost_dev_start
                                  - vhost_user_set_mem_table
                                    - vhost_user_add_remove_regions
                                      - send_add_regions

## vhost.c
- `vhost_vq_reset`
- `vhost_dev_set_owner`
- `vhost_dev_ioctl`


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
