# vhost 的 feature 协商

## 报文格式
报文头虽然有 version，但通常不会随功能增加而递增。

vhost-user 的报文结构是：

`request: u32 | flags: u32 | size: u32 | payload`

flags 的低两位表示协议版本，目前为 0x01。本地 QEMU 也直接定义：

```c
#define VHOST_USER_VERSION (0x1)
```

而且 QEMU 会检查回复的 flags，错误时返回 -EPROTO。

## VHOST_USER_SET_FEATURES 和 VHOST_USER_SET_PROTOCOL_FEATURES

因为 feautre 数量太多，而且一类是和 vhost 通信有关，一类是和
设备能力有关，所以是没有必要设置的。

这两个 feature 就是两个不同的 request 而已:

QEMU → 后端
    request = SET_FEATURES
    payload = 0x1
    含义：启用块设备的旧式 BARRIER 特性

QEMU → 后端
    request = SET_PROTOCOL_FEATURES
    payload = 0x1
    含义：启用 vhost-user 多队列协议扩展

```c
static int vhost_user_set_features(struct vhost_dev *dev,
                                   uint64_t features)
{
    /*
     * wait for a reply if logging is enabled to make sure
     * backend is actually logging changes
     */
    bool log_enabled = features & (0x1ULL << VHOST_F_LOG_ALL);
    // 有简化
    return vhost_user_set_u64(dev, VHOST_USER_SET_FEATURES, features, log_enabled);
}
```

```c
static int vhost_user_set_protocol_features(struct vhost_dev *dev,
                                            uint64_t features)
{
    return vhost_user_set_u64(dev, VHOST_USER_SET_PROTOCOL_FEATURES, features,
                              false);
}
```

### 基本流程
整个过程都是 qemu 和后端协商，不会出现 guest 和 spdk 协商

```txt
vhost 模式下，仍然由 QEMU 的 virtio 前端向 guest 暴露 feature。QEMU 先查询 vhost 后端能力，再通过普通的 virtio PCI/MMIO 接口让 guest 协商。

准确说，virtio_blk_get_features() 是生成“设备支持哪些 feature”的回调，只是协商流程的一部分。

以 vhost-user-blk 为例，整个过程是：

vhost 后端
    │  GET_FEATURES：返回后端能力
    ▼
QEMU vhost-user-blk 的 get_features 回调
    │  根据后端能力和 QEMU 配置筛选
    ▼
vdev->host_features
    │  guest 读取 virtio 的 device_feature 寄存器
    ▼
guest 选择需要的 feature，写入 driver_feature
    │
    ▼
vdev->guest_features
    │  SET_FEATURES：通知后端最终启用的能力
    ▼
vhost 后端按协商结果处理 virtqueue
```

FEATURES 位图
├── virtio / vhost 特性
└── bit 30：支持协议特性协商
                │
                ▼
       PROTOCOL_FEATURES 位图
       ├── REPLY_ACK
       ├── CONFIG
       └── CONFIGURE_MEM_SLOTS


## virtio 的协商过程

以 QEMU 自己处理 I/O 的 virtio-blk-pci 为例，流程如下：

QEMU 创建设备
    │
    ├─ virtio_blk_get_features()
    │    汇总设备支持的特性
    │    保存到 vdev->host_features_ex
    │
guest 驱动初始化
    │
    ├─ 读取 PCI device_feature
    │    得到 QEMU 提供的特性
    │
    ├─ 选择自己支持、准备使用的特性
    │
    ├─ 写入 PCI driver_feature
    │    QEMU 调用 virtio_set_features_ex()
    │    保存到 vdev->guest_features_ex
    │
    ├─ 设置 FEATURES_OK
    │    QEMU 校验特性组合
    │    guest 读回状态，确认设备接受
    │
    └─ 后续配置队列，设置 DRIVER_OK
         设备开始正常工作

## vhost-net 的协商过程

内核 vhost-net 模式下，guest 仍然与 QEMU 的 virtio-net 设备协商；QEMU 通过 /dev/vhost-net 的 ioctl 查询宿主机内核能力，并把协商结果设置回内核。
这里有三方：

- guest 内核：运行 virtio_net 驱动。
- QEMU：提供 virtio PCI 设备、feature 寄存器和设备配置。
- host 内核：运行 vhost_net，负责 RX/TX virtqueue 的数据处理。

```txt
          ┌─────────────────────┐                         ┌─────────────────────┐                         ┌─────────────────────┐
          │ Guest virtio_net    │                         │ QEMU virtio-net     │                         │ Host 内核 vhost_net │
          └─────────────────────┘                         └─────────────────────┘                         └─────────────────────┘
                     │                                               │ ioctl(VHOST_GET_FEATURES)                     │
                     │                                               ├───────────────────────────────────────────────▶
                     │                                               │                                               │
                     │                                               │ 内核支持的 feature                            │
                     │                                               ◀┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
                     │                                               │                                               │
                     │                                               ┌────────────────────────────────────┐          │
                     │                                               │ Note: 结合 QEMU 配置、TAP 能力筛选 │          │
                     │                                               └────────────────────────────────────┘          │
                     │ 读取 device_feature                           │                                               │
                     ├───────────────────────────────────────────────▶                                               │
                     │                                               │                                               │
                     │ 最终对 guest 提供的 feature                   │                                               │
                     ◀┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤                                               │
                     │                                               │                                               │
                     │ 写 driver_feature                             │                                               │
                     ├───────────────────────────────────────────────▶                                               │
                     │                                               │                                               │
                     │                                               ┌──────────────────────────────────────────────┐│
                     │                                               │ Note: 保存 guest 选择，提取 vhost 需要的部分 ││
                     │                                               └──────────────────────────────────────────────┘│
                     │ FEATURES_OK，随后 DRIVER_OK                   │                                               │
                     ├───────────────────────────────────────────────▶                                               │
                     │                                               │                                               │
                     │                                               │ ioctl(VHOST_SET_FEATURES)                     │
                     │                                               ├───────────────────────────────────────────────▶
                     │                                               │                                               │
                     │                                               │ 设置内存、vring、eventfd、TAP 后端            │
                     │                                               ├───────────────────────────────────────────────▶
                     │                                               │                                               │
                     ┌───────────────────────────────────────────────────────────────────────────────────────────────┐
                     │ Note: 内核 vhost-net 开始处理 RX/TX 队列                                                      │
                     └───────────────────────────────────────────────────────────────────────────────────────────────┘
```


## 附录

### GET/SET_PROTOCOL_FEATURES：完整列表

[include/hw/virtio/vhost-user.h](include/hw/virtio/vhost-user.h)，协议说明在
[docs/interop/vhost-user.rst](docs/interop/vhost-user.rst)，

下表名称统一省略前缀 **`VHOST_USER_PROTOCOL_F_`**。

| bit | 名称                        | 大致作用                                                                        |
| --: | --------------------------- | ------------------------------------------------------------------------------- |
|   0 | `MQ`                        | 支持多队列协议扩展，例如查询后端支持的最大队列数                                |
|   1 | `LOG_SHMFD`                 | 通过共享内存 FD 共享脏页日志，供迁移使用                                        |
|   2 | `RARP`                      | 请求后端发送 RARP，帮助迁移后的网络更新 MAC 所在位置                            |
|   3 | `REPLY_ACK`                 | 允许命令要求后端回复执行成功或失败                                              |
|   4 | `NET_MTU`                   | QEMU 向后端传递 MTU                                                             |
|   5 | `BACKEND_REQ`               | 建立后端主动向 QEMU 发请求的通道，例如 IOTLB miss 通知                          |
|   6 | `CROSS_ENDIAN`              | 设置 legacy virtqueue 的字节序，支持跨大小端场景                                |
|   7 | `CRYPTO_SESSION`            | 创建、关闭加密会话                                                              |
|   8 | `PAGEFAULT`                 | 支持 postcopy 迁移中的缺页处理，涉及 userfaultfd                                |
|   9 | `CONFIG`                    | 通过 `GET_CONFIG/SET_CONFIG` 访问 virtio 设备配置空间                           |
|  10 | `BACKEND_SEND_FD`           | 后端在主动请求通道中向 QEMU 传递 FD                                             |
|  11 | `HOST_NOTIFIER`             | 将后端提供的通知区域映射为 guest 的队列通知 MMIO 区域                           |
|  12 | `INFLIGHT_SHMFD`            | 共享尚未完成的 I/O 状态，用于后端恢复或迁移                                     |
|  13 | `RESET_DEVICE`              | 重置后端设备状态、禁用所有队列，同时保留设备所有权                              |
|  14 | `INBAND_NOTIFICATIONS`      | 用 socket 协议消息传递队列 kick/call/error 通知                                 |
|  15 | `CONFIGURE_MEM_SLOTS`       | 查询内存槽上限，增量添加、删除后端的 guest 内存映射                             |
|  16 | `STATUS`                    | 读取、设置后端的 virtio device status，例如 `DRIVER_OK`                         |
|  17 | `XEN_MMAP`                  | Xen 内存映射扩展；当前 QEMU 前端枚举中以注释保留此编号                          |
|  18 | `SHARED_OBJECT`             | 按 UUID 查找、共享 dma-buf 对象，QEMU 可在后端之间中转                          |
|  19 | `DEVICE_STATE`              | 迁移时通过 FD 传输后端内部状态，并检查处理结果                                  |
|  20 | `GET_VRING_BASE_INFLIGHT`   | 停止队列时保留、迁移未完成 I/O 的能力，和 bit 12、23 配合                       |
|  21 | `GPA_ADDRESSES`             | 在协议指定的地址字段中使用 guest physical address；IOMMU 场景仍需遵守 IOVA 规则 |
|  22 | `SHMEM`                     | 查询 virtio Shared Memory Region 配置，并支持后端请求映射、解除映射             |
|  23 | `GET_VRING_BASE_SKIP_DRAIN` | 显式请求停止队列时不等待 I/O 全部完成，而是挂起并记录未完成请求                 |

bit 20、23 有历史演进关系：早期 bit 20 用来改变 `GET_VRING_BASE` 的 drain
行为；当前增加了独立的 `GET_VRING_BASE_SKIP_DRAIN` 消息，让 QEMU
可以**每次停止队列时选择是否等待 I/O 完成**。

如果看的是后端库，它另有一份枚举：[subprojects/libvhost-user/libvhost-user.h](subprojects/libvhost-user/libvhost-user.h)。**定义了协议编号，不代表每个后端都实现了该能力。**

### GET/SET_FEATURES

#### virtio 通用特性

主要定义在：

- [virtio_config.h](include/standard-headers/linux/virtio_config.h)
- [virtio_ring.h](include/standard-headers/linux/virtio_ring.h)

| bit | 完整名称                      | 大致作用                                                                    |
| --: | ----------------------------- | --------------------------------------------------------------------------- |
|  24 | `VIRTIO_F_NOTIFY_ON_EMPTY`    | legacy 特性：队列中可用 buffer 耗尽时通知 guest                             |
|  27 | `VIRTIO_F_ANY_LAYOUT`         | legacy 特性：允许更灵活的 descriptor 布局                                   |
|  28 | `VIRTIO_RING_F_INDIRECT_DESC` | 支持间接描述符表                                                            |
|  29 | `VIRTIO_RING_F_EVENT_IDX`     | 用事件索引控制通知，减少不必要的 kick/interrupt                             |
|  32 | `VIRTIO_F_VERSION_1`          | 符合 virtio 1.0 及后续版本的现代接口                                        |
|  33 | `VIRTIO_F_ACCESS_PLATFORM`    | 按平台 DMA 规则访问内存，例如经过 IOMMU；旧名称为 `VIRTIO_F_IOMMU_PLATFORM` |
|  34 | `VIRTIO_F_RING_PACKED`        | 使用 packed virtqueue                                                       |
|  35 | `VIRTIO_F_IN_ORDER`           | 设备按 buffer 提交顺序使用它们                                              |
|  36 | `VIRTIO_F_ORDER_PLATFORM`     | 采用平台规定的内存访问顺序规则                                              |
|  37 | `VIRTIO_F_SR_IOV`             | 支持 SR-IOV                                                                 |
|  38 | `VIRTIO_F_NOTIFICATION_DATA`  | guest 通知设备时携带额外队列信息                                            |
|  39 | `VIRTIO_F_NOTIF_CONFIG_DATA`  | 使用设备提供的值标识通知所针对的队列                                        |
|  40 | `VIRTIO_F_RING_RESET`         | 支持单独重置某个队列                                                        |
|  41 | `VIRTIO_F_ADMIN_VQ`           | 支持 administration virtqueue                                               |

这是头文件里的通用定义，**并非这些位都能被当前某个 vhost-user 设备协商启用**。

#### GET/SET_FEATURES：设备专用特性

这部分按设备分别定义，**相同 bit
在不同设备中可以有不同含义**。下面列出相关设备的定义入口，不是所有 virtio
设备的穷举。

| 设备     | 定义位置                                                          | 大致内容                                                                  |
| -------- | ----------------------------------------------------------------- | ------------------------------------------------------------------------- |
| 网络     | [virtio_net.h](include/standard-headers/linux/virtio_net.h)       | checksum、TSO/UFO/USO、合并接收 buffer、多队列、RSS、MTU、MAC、通知合并等 |
| 块设备   | [virtio_blk.h](include/standard-headers/linux/virtio_blk.h)       | 只读、块大小、flush、discard、write zeroes、多队列等                      |
| SCSI     | [virtio_scsi.h](include/standard-headers/linux/virtio_scsi.h)     | 双向请求、热插拔、参数变化等                                              |
| 文件系统 | [virtio_fs.h](include/standard-headers/linux/virtio_fs.h)         | 文件系统设备相关特性                                                      |
| vsock    | [virtio_vsock.h](include/standard-headers/linux/virtio_vsock.h)   | 例如 seqpacket 支持                                                       |
| GPU      | [virtio_gpu.h](include/standard-headers/linux/virtio_gpu.h)       | 3D、EDID、资源管理等                                                      |
| 加密     | [virtio_crypto.h](include/standard-headers/linux/virtio_crypto.h) | 加密设备相关特性                                                          |

拿 **virtio-blk** 举例，它的设备专用位完整定义如下，名称省略 `VIRTIO_BLK_F_`：

| bit | 名称           | 作用                               |
| --: | -------------- | ---------------------------------- |
|   0 | `BARRIER`      | 旧式 barrier 支持                  |
|   1 | `SIZE_MAX`     | 提供单个 segment 的最大大小        |
|   2 | `SEG_MAX`      | 提供最大 segment 数量              |
|   4 | `GEOMETRY`     | 提供旧式磁盘几何信息               |
|   5 | `RO`           | 只读磁盘                           |
|   6 | `BLK_SIZE`     | 提供逻辑块大小                     |
|   7 | `SCSI`         | 旧式 SCSI 命令透传                 |
|   9 | `FLUSH`        | 支持 flush；`WCE` 是该位的历史别名 |
|  10 | `TOPOLOGY`     | 提供物理块大小、对齐等拓扑信息     |
|  11 | `CONFIG_WCE`   | 通过配置空间控制 writeback 模式    |
|  12 | `MQ`           | 多请求队列                         |
|  13 | `DISCARD`      | 支持 discard/TRIM                  |
|  14 | `WRITE_ZEROES` | 支持写零请求                       |
|  16 | `SECURE_ERASE` | 支持安全擦除                       |
|  17 | `ZONED`        | 支持 zoned block device            |

当前 QEMU vhost-user-blk 实际用于后端特性筛选的列表，是
[hw/block/vhost-user-blk.c](hw/block/vhost-user-blk.c) 中的
`user_feature_bits[]`。它没有包含上述全部设备位，例如 `ZONED`、`SECURE_ERASE`
就不在该列表中。

#### vhost 专用位

| bit | 名称                             | 作用及适用范围                                       |
| --: | -------------------------------- | ---------------------------------------------------- |
|  26 | `VHOST_F_LOG_ALL`                | 记录后端对 guest 内存的写入，用于迁移脏页跟踪        |
|  27 | `VHOST_NET_F_VIRTIO_NET_HDR`     | 内核 vhost-net 添加 RX、移除 TX 的 virtio-net header |
|  30 | `VHOST_USER_F_PROTOCOL_FEATURES` | vhost-user 协议特性协商的入口位                      |

前两个定义在 [vhost_types.h](include/standard-headers/linux/vhost_types.h)，bit
30 定义在 [vhost.h](include/hw/virtio/vhost.h)。

该头文件还保留了
`VHOST_F_DEVICE_IOTLB = 63`，当前源码没有找到使用它的实现路径，不宜当作当前
vhost-user 的通用能力。

### 同名或同编号不代表同一个特性

最容易混淆的一组是：

```text
VIRTIO_BLK_F_MQ               = 12   FEATURES 位图
    guest 看到的块设备支持多队列

VHOST_USER_PROTOCOL_F_MQ      = 0    PROTOCOL_FEATURES 位图
    QEMU 与后端支持多队列相关控制协议

VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD = 12
    虽然也是 bit 12，但在另一张位图里，与 BLK_F_MQ 完全不同
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
