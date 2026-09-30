# vhost

## 先用起来
https://kvm-forum.qemu.org/2022/libblkio-kvm-forum-2022.pdf
- virtio-blk + vhost-user to communicate with a user-space storage server (e.g., SPDK, qemu-storage-daemon)
- 测试 vhost 的一个简单方法就是 qemu-storage-daemon ，而不是搭建 SPDK

https://github.com/yandex-cloud/yc-libvhost-server
和 subprojects/libvhost-user/ 是什么关系?
它们是实现同一套 vhost-user 后端协议的两个库。 不过 yc-libvhost-server

https://github.com/rust-vmm/vhost-device
- 基于 rust-vmm 实现的各种后端 https://github.com/rust-vmm/vhost/blob/main/vhost-user-backend/src/handler.rs

https://github.com/dragonflyoss/nydus/blob/master/src/bin/nydusd/virtiofs.rs
- virtiofs

## 参考文档

- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/04/18/vsock-internals
- https://www.redhat.com/en/blog/deep-dive-virtio-networking-and-vhost-net
- https://www.redhat.com/en/blog/introduction-virtio-networking-and-vhost-net
- https://www.redhat.com/en/blog/virtio-devices-and-drivers-overview-headjack-and-phone
- https://access.redhat.com/solutions/3394851

## 问题
- [ ] vhost 出现的位置不只是 hw/virtio 中位置
	- 例如 net/vhost-user.c

在例如
/home/martins3/data/qemu/hw/virtio/vhost-user-blk-pci.c
/home/martins3/data/qemu/hw/block/vhost-user-blk.c

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
- https://www.qemu.org/docs/master/system/devices/vhost-user.html

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

## 几个小问题
- 存在 qemu 帮助 backend 保管内存的情况吗?

- 驱动对于 vhost 有感知吗?
或者说，linux kernel 中的 virtio-blk 需要由于 blk 做修改吗?

2. vhost 的 backend 如果忽然被 kill 掉 ，恢复后，可以正常运行吗?

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
