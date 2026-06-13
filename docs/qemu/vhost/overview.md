# 参考文档

- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/04/18/vsock-internals
- https://www.redhat.com/en/blog/deep-dive-virtio-networking-and-vhost-net
- https://www.redhat.com/en/blog/introduction-virtio-networking-and-vhost-net
- https://www.redhat.com/en/blog/journey-vhost-users-realm
- https://www.redhat.com/en/blog/virtio-devices-and-drivers-overview-headjack-and-phone
- https://access.redhat.com/solutions/3394851


- QEMU vhost-user 的使用文档:
  - https://qemu.readthedocs.io/en/latest/interop/vhost-user.html


## [ ] vhost 出现的位置不只是 hw/virtio 中位置

## 问题
- [ ] vsock 是什么鬼?
- vhost 和 vsock 的关系？

## vhost-net / virtio-net / vhost-sock
这三个技术都是用于处理网路的

## 在 virtio fs 也是需要使用 vhost
- [ ] vhost 是啥

## vhost-user


## vhost

- [x] LoyenWang 页分析过 virtio

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


## net/vhost-user.c

## https://www.cnblogs.com/ck1020/p/8341914.html

vhost-user 下，UNIX 本地 socket 代替了之前 kernel 模式下的设备文件进行进程间的通信（qemu 和 vhost-user app）,而通过 mmap 的方式把 ram 映射到 vhost-user app 的进程空间实现内存的共享。其他的部分和 vhost-kernel 原理基本一致。这种情况下一般 qemu 作为 client，而 vhost-user app 作为 server 如 DPDK。而本文对于 vhost-user server 端的分析主要也是基于 DPDK 源码。本文主要分析涉及到的三个重要机制：qemu 和 vhost-user app 的消息传递，guest memory 和 vhost-user app 的共享，guest 和 vhost-user app 的通知机制。
