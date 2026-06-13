## vhost 协议基本分析
<!-- 24fdd793-a4eb-41cd-abea-43b41f19e632 -->

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

## 参考文档

- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/04/18/vsock-internals
- https://www.redhat.com/en/blog/deep-dive-virtio-networking-and-vhost-net
- https://www.redhat.com/en/blog/introduction-virtio-networking-and-vhost-net
- https://www.redhat.com/en/blog/journey-vhost-users-realm
- https://www.redhat.com/en/blog/virtio-devices-and-drivers-overview-headjack-and-phone
- https://access.redhat.com/solutions/3394851

## 问题
- [ ] vhost 出现的位置不只是 hw/virtio 中位置
	- 例如 net/vhost-user.c

## 简要的代码分析

- `vhost_dev_init`
  - `vhost_set_backend_type`
    - `kernel_ops`
    - `user_ops`
    - `vdpa_ops`
  - `::vhost_backend_init`
  - `::vhost_set_owner`
  - `::vhost_get_features`
  - `vhost_virtqueue_init` ：对于每一个 virtqueue 进行调用
  - 注册 `memory_listener` ： TODO 似乎重新需要理解一下 memory listener 中间的事情哇
  - 注册 `iommu_listener` ：这又是什么

- `vhost_user_set_vring_base`
  - `vhost_set_vring` ：构建消息结构体 `VhostUserMsg`
    - `vhost_user_write`
      - `vhost_user_one_time_request` ：消息类型都是在 `VhostUserRequest`  中定义的
      - `qemu_chr_fe_set_msgfds`
      - `qemu_chr_fe_write_all` : 使用 `CharBackend` 发送消息


下面来分析一下，VhostUserState::chr 是如何初始化，以及消息是发送给谁。
- `vhost_user_init` ：TODO 我发现这个函数的调用位置特别多，似乎每一个 backennd 都有一个对应 vhost-user 的。


## https://www.cnblogs.com/ck1020/p/8341914.html

vhost-user 下，UNIX 本地 socket 代替了之前 kernel 模式下的设备文件进行进程间的通信（qemu 和 vhost-user app）,而通过 mmap 的方式把 ram 映射到 vhost-user app 的进程空间实现内存的共享。其他的部分和 vhost-kernel 原理基本一致。这种情况下一般 qemu 作为 client，而 vhost-user app 作为 server 如 DPDK。而本文对于 vhost-user server 端的分析主要也是基于 DPDK 源码。本文主要分析涉及到的三个重要机制：qemu 和 vhost-user app 的消息传递，guest memory 和 vhost-user app 的共享，guest 和 vhost-user app 的通知机制。

https://mp.weixin.qq.com/s?__biz=MzA5NDQzODQ3MQ==&mid=2648182374&idx=1&sn=ad3fcffcc9d238a0d2c58bc734f19cc5&chksm=88623b4ebf15b2586f2a7e9cdc2fbb7e5ae6b0988ad2a815a8df05f8938f889a5b685c93fd65&scene=21#wechat_redirect

## 似乎大家都是只是故
https://wiki.qemu.org/Features/VirtioVhostUser

## 分析 vhost 下的 vsock.c 和 net.c ，看上去内核中实际上

vhost_vsock_dev_open 中所有的程序都是在注册:
```c
	vqs[VSOCK_VQ_TX] = &vsock->vqs[VSOCK_VQ_TX];
	vqs[VSOCK_VQ_RX] = &vsock->vqs[VSOCK_VQ_RX];
	vsock->vqs[VSOCK_VQ_TX].handle_kick = vhost_vsock_handle_tx_kick;
	vsock->vqs[VSOCK_VQ_RX].handle_kick = vhost_vsock_handle_rx_kick;
```
具体执行是在 drivers/vhost/vhost.c

相当于在内核中实现的 vhost 机制

vhost_vsock_handle_tx_kick 的调用路径是:

- vhost_poll_start
  - vfs_poll : 等待 vhost_net_chr_poll 返回 ，当 guest 产生中断的时候返回
  - vhost_poll_wakeup : 执行 vhost_vsock_handle_tx_kick ，来处理数据

## links
- https://juejin.cn/post/7111539346867486756 : 基本介绍

https://www.qemu.org/docs/master/system/devices/vhost-user.html

## 用用 user
name "vhost-user-blk", bus virtio-bus
name "vhost-user-blk-pci", bus PCI
name "vhost-user-blk-pci-non-transitional", bus PCI
name "vhost-user-blk-pci-transitional", bus PCI
name "vhost-user-fs-device", bus virtio-bus <--- 这个应该是 virtio fs 的实现吧
name "vhost-user-fs-pci", bus PCI
name "vhost-user-scsi", bus virtio-bus
name "vhost-user-scsi-pci", bus PCI
name "vhost-user-scsi-pci-non-transitional", bus PCI
name "vhost-user-scsi-pci-transitional", bus PCI

name "vhost-user-gpio-device", bus virtio-bus
name "vhost-user-gpio-pci", bus PCI
name "vhost-user-i2c-device", bus virtio-bus
name "vhost-user-i2c-pci", bus PCI
name "vhost-user-input", bus virtio-bus
name "vhost-user-input-pci", bus PCI
name "vhost-user-rng", bus virtio-bus
name "vhost-user-rng-pci", bus PCI

https://stackoverflow.com/questions/75906208/how-to-connect-via-virtio-gui-running-on-host-with-gpio-in-a-qemu-emulated-virtu

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
