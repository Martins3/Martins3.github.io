# dpdk

首先，先使用上再说吧！
- https://talawah.io/blog/linux-kernel-vs-dpdk-http-performance-showdown/
- https://doc.dpdk.org/guides/prog_guide/index.html#

# 我认为官方文档已经清晰的不得了
- https://doc.dpdk.org/guides/prog_guide/index.html#

- [ ] 所以如何正确的编译来着

## dpdk
- [dpdk 和 network stack 的关系](https://stackoverflow.com/questions/33292630/using-dpdk-to-create-a-tcp-ip-connection)
    - dpdk 只有 API，是没有网络栈的实现的，一些具体的实现:
    - 其对应的用户态的部分: https://github.com/F-Stack/f-stack
        - 核心开发人员的 blog :  http://lovelyping.com/
- [ ] https://blog.selectel.com/introduction-dpdk-architecture-principles/ ：基本原理介绍。
- https://www.dpdk.org/wp-content/uploads/sites/35/2017/04/DPDK-India2017-RamiaJain-ArchitectureRoadmap.pdf
    - 从 17 页之后开始，

## [ ] 如何 vDPA 封装 virtio 硬件的，为什么 vDPA 还出现在 DPDK 的代码中

## PMD 是什么
poll mode driver 的简称。

- https://doc.dpdk.org/guides/prog_guide/poll_mode_drv.html

## 可以在虚拟机中安装 dpdk 然后测试吗
- [ ] 虚拟机中使用 dpdk 就是说虚拟机中需要使用 IOMMU 吧，所以可以将 IOMMU 虚拟化吗?
- [ ] 嵌套虚拟机中可以直通吗?
- [ ] 可以将 RDMA 和 DPDK 一起使用吗?

## 真的是使用 VFIO 的吗

This is opposed to a Linux kernel where we have a scheduler and interrupts for switching between processes, in the DPDK architecture the devices are accessed by constant polling.[^1]
- 有趣啊，没有 scheduler 和 interrupts 了。

[^1]: https://www.redhat.com/en/blog/how-vhost-user-came-being-virtio-networking-and-dpdk

## [ ] dpdk 的源码中只是看到了对于 NIC 的 driver，但是网络栈在什么地方不知道


## [ ] 针对于 dpdk 的这些优化，为什么内核不能搞一个对应的模式
- 内核也可以直接将分配大页，或者使用 CPU 总是 poll
    - 可能，内核的改动太大了，还是存在无法逾越的挑战。


## 值得重点关注的几个 library
- mempool
- mbuf
- ring

## [ ] 居然也有 bpf，但是其中是做啥的哇


## DPDK 中间存在 virtio，如何理解
- [ ] 之前的看法中，DPDK 直接操作硬件的，除非其操作的硬件支持 virtio。
    - vhost 本来是 virtio 中用于加速 dataplane 的

## 简单分析一下 dpdk 和 vhost 关联的部分 ： https://www.cnblogs.com/ck1020/p/8341914.html

在 `net/vhost/rte_eth_vhost.c` 中定义了：

```c
static const struct eth_dev_ops ops = {
    // ...
    .dev_configure = eth_dev_configure,
    // ...
```

- `eth_dev_configure`
    - `vhost_driver_setup`
        - `rte_vhost_driver_register` ：这个是其 driver 的哇
            -  `create_unix_socket` ： 漫长的初始化参数 vsockets 的内容，然后调用此函数。
        - `rte_vhost_driver_start`
            - `rte_ctrl_thread_create` -> `fdset_event_dispatch`
            - `vhost_user_start_server`
                - `vhost_user_server_new_connection`
                    - `vhost_user_add_connection`
                        - `vhost_user_read_cb`
                            - `vhost_user_msg_handler` ：在这里处理和 QEMU 的交互
            - `vhost_user_start_client`
                - `vhost_user_add_connection`
                    - `vhost_user_read_cb`

## memory 共享

在 `vhost_user_msg_handler` 中，`VHOST_USER_SET_MEM_TABLE` 对应位置的 hook 为: `vhost_user_set_mem_table`
```c
VHOST_MESSAGE_HANDLER(VHOST_USER_SET_MEM_TABLE, vhost_user_set_mem_table, true) \
```

## 简单分析 dpdk 架构
- `virtio_user_dev_init` ：这个结构和 QEMU 中 `vhost_dev_init` 好对称啊
    - `virtio_user_dev_setup` ：注册三种 ops
        - `virtio_ops_user`
        - `virtio_ops_kernel`
        - `virtio_ops_vdpa`
    - `::set_owner`
    - `::get_backend_features`


- [ ] 应该尝试理解一下，为什么 `virtio_ops_user` 的替代者都是谁，都是什么定位的?
```c
struct virtio_user_backend_ops virtio_ops_user = {
```

## [ ] 从 pmd 到 vhost 的 qeueu 的注入的过程



driver 使用 null 作为例子:
```c
static struct rte_vdev_driver pmd_null_drv = {
	.probe = rte_pmd_null_probe,
	.remove = rte_pmd_null_remove,
};

static const struct eth_dev_ops ops = {
	.dev_close = eth_dev_close,
	.dev_start = eth_dev_start,
	.dev_stop = eth_dev_stop,
	.dev_configure = eth_dev_configure,
	.dev_infos_get = eth_dev_info,
	.rx_queue_setup = eth_rx_queue_setup,
	.tx_queue_setup = eth_tx_queue_setup,
	.rx_queue_release = eth_rx_queue_release,
	.tx_queue_release = eth_tx_queue_release,
	.mtu_set = eth_mtu_set,
	.link_update = eth_link_update,
	.mac_addr_set = eth_mac_address_set,
	.stats_get = eth_stats_get,
	.stats_reset = eth_stats_reset,
	.reta_update = eth_rss_reta_update,
	.reta_query = eth_rss_reta_query,
	.rss_hash_update = eth_rss_hash_update,
	.rss_hash_conf_get = eth_rss_hash_conf_get
};
```

实际上，还发现了 virtio ，猜测这个时候，dpdk 是在虚拟机中，和 vDPA 联合使用，在虚拟机中通过 VFIO 直接访问 virtio 硬件。

### 真正的纯粹的 virito 驱动`net/virtio/virtio_ethdev.c`
- `set_rxtx_funcs` 设置 virtio 的接受函数

### drivers/vhost

```c
static const struct eth_dev_ops ops = {
	.dev_start = eth_dev_start,
	.dev_stop = eth_dev_stop,
	.dev_close = eth_dev_close,
	.dev_configure = eth_dev_configure,
	.dev_infos_get = eth_dev_info,
	.rx_queue_setup = eth_rx_queue_setup,
	.tx_queue_setup = eth_tx_queue_setup,
	.rx_queue_release = eth_rx_queue_release,
	.tx_queue_release = eth_tx_queue_release,
	.tx_done_cleanup = eth_tx_done_cleanup,
	.link_update = eth_link_update,
	.stats_get = eth_stats_get,
	.stats_reset = eth_stats_reset,
	.xstats_reset = vhost_dev_xstats_reset,
	.xstats_get = vhost_dev_xstats_get,
	.xstats_get_names = vhost_dev_xstats_get_names,
	.rx_queue_intr_enable = eth_rxq_intr_enable,
	.rx_queue_intr_disable = eth_rxq_intr_disable,
	.get_monitor_addr = vhost_get_monitor_addr,
};

static struct rte_vdev_driver pmd_vhost_drv = {
	.probe = rte_pmd_vhost_probe,
	.remove = rte_pmd_vhost_remove,
};
```

看来这个是提供给 lib/vhost 使用的，但问题是，似乎 lib/vhost 不是和 QEMU 通信的吗?

- `rte_vhost_driver_start` 可以一路调用到 `vhost_user_msg_handler`

```c
static struct rte_vdev_driver virtio_user_driver = {
	.probe = virtio_user_pmd_probe,
	.remove = virtio_user_pmd_remove,
	.dma_map = virtio_user_pmd_dma_map,
	.dma_unmap = virtio_user_pmd_dma_unmap,
};
```
但是 virtio 的 `eth_dev_ops` 和 `rte_vdev_driver` 不是在一个文件中的


### 为什么 vhost user 的 backend 是 kernel

```c
struct virtio_user_backend_ops virtio_ops_kernel = {
	.setup = vhost_kernel_setup,
	.destroy = vhost_kernel_destroy,
	.get_backend_features = vhost_kernel_get_backend_features,
	.set_owner = vhost_kernel_set_owner,
	.get_features = vhost_kernel_get_features,
	.set_features = vhost_kernel_set_features,
	.set_memory_table = vhost_kernel_set_memory_table,
	.set_vring_num = vhost_kernel_set_vring_num,
	.set_vring_base = vhost_kernel_set_vring_base,
	.get_vring_base = vhost_kernel_get_vring_base,
	.set_vring_call = vhost_kernel_set_vring_call,
	.set_vring_kick = vhost_kernel_set_vring_kick,
	.set_vring_addr = vhost_kernel_set_vring_addr,
	.get_status = vhost_kernel_get_status,
	.set_status = vhost_kernel_set_status,
	.enable_qp = vhost_kernel_enable_queue_pair,
	.update_link_state = vhost_kernel_update_link_state,
	.get_intr_fd = vhost_kernel_get_intr_fd,
};
```

- `vhost_kernel_set_vring_kick`
    - `vhost_kernel_ioctl` : 然后居然就是真的调用的 kernel 的 ioctl
        - [ ] 找到内核对应的内容

- `vhost_user_set_vring_kick` : 有两个实现
    - `lib/vhost_user.c` 的版本，作为和 QEMU 的 server 来通信的
    - `drivers/net/virtio/virtio_user/` 的版本，当 dpdk 在虚拟机中的用户态的时候，这就是一个驱动，和 virtio device 打交道
