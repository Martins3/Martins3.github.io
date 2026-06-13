# vhost iotlb
<!-- c9c6f8da-4df0-4977-af3e-16ba391bf09a -->

思考 vhost iommu 之前，让我们来思考一个简单问题，
当普通的 virtio 要支持 iommu 的时候，QEMU 是如何模拟的?
1. dev 来操作内存的时候，需要读 iommu 的 tlb ，最后才可以知道真的要写什么地方
2. cpu 写入 mmio 空间的命令是需要读 iommu tlb 转换，之后才知道真的要读什么地方

同样的，当使用 vhost 的时候，dpdk 来直接读写共享内存，dpdk 通过共享内存的队列
来获取到命令，以前的 iommu 状态发在 qemu 中， 现在就需要使用一个接口让 qemu 提供给 dpdk 了。
就是这样吧


`vhost IOMMU`（通常与 `vIOMMU` 结合讨论）是虚拟化技术中一个关键的组件，主要用于增强 **虚拟机（VM）与其后端 I/O 服务（如 vhost-net/vhost-user）之间的安全隔离和地址转换能力**。

vhost IOMMU 解决了什么问题？

在没有 vIOMMU 的传统 `virtio` 场景中，虚拟机（Guest）直接将物理内存地址（GPA）传递给宿主机（Host）的后端进程。这存在以下几个核心问题：

* **安全隔离缺失（DMA 保护）：** 如果没有 IOMMU，后端驱动（或被分配的硬件）理论上可以访问虚拟机的**所有**内存。在多租户云场景下，如果后端进程被攻破，攻击者可以通过 DMA 方式窃取虚拟机中不属于 I/O 范围的敏感数据（如密钥）。
* **用户空间驱动的支持（VFIO）：** 如果你想在虚拟机内部运行 DPDK（使用 `VFIO` 驱动），虚拟机内部必须感知到一个 IOMMU。没有 vIOMMU，虚拟机内核无法建立有效的 DMA 映射，导致 DPDK 等高性能框架无法正常工作。
* **非连续内存的连续访问：**
类似于 CPU 的 MMU 将虚拟内存映射到物理内存，IOMMU 允许设备看到一个连续的“虚拟 I/O 地址空间”（IOVA），而底层对应的物理页可以是离散的。

## 2. vhost IOMMU 的核心原理

当开启 vhost IOMMU 支持后，其工作流程如下：

1. **地址校验：** 虚拟机驱动不再直接发送 GPA（Guest Physical Address），而是发送 IOVA。
2. **IOTLB 机制：** vhost 后端会维护一个 **IOTLB（I/O Translation Lookaside Buffer）**。
3. **动态映射：** 当 vhost 后端需要访问内存时，它会先查 IOTLB。如果未命中，它会通过 `vhost-user` 或 `vhost-kernel` 消息向 QEMU 发起请求，获取正确的内存映射关系。

要使用该功能，通常需要在 **QEMU 命令行**、**虚拟机系统参数** 以及 **宿主机后端配置**（如 OVS-DPDK）三个层面进行设置。

1. 在 QEMU 中启用虚拟 IOMMU

在启动虚拟机时，需要添加 `intel-iommu` 设备，并为 virtio 设备开启 `iommu_platform` 参数。

```bash
qemu-system-x86_64 \
  -machine q35,accel=kvm,kernel-irqchip=split \
  -device intel-iommu,intremap=on,device-iotlb=on \
  -netdev tap,id=net0,vhost=on \
  -device virtio-net-pci,netdev=net0,iommu_platform=on,ats=on \
  ...

```

* `iommu_platform=on`：这是关键，它告诉 virtio 设备必须通过 IOMMU 进行地址转换。
* `ats=on`：开启地址转换服务（Address Translation Services），提升性能。

2. 第二步：在虚拟机（Guest）中开启支持

虚拟机启动后，需要在内核引导参数中加入 IOMMU 支持（针对 Intel CPU）：

1. 编辑 `/etc/default/grub`。
2. 在 `GRUB_CMDLINE_LINUX_DEFAULT` 中添加 `intel_iommu=on`。
3. 更新 grub 并重启：`update-grub`。

3. 第三步：宿主机后端配置（以 OVS-DPDK 为例）

如果你使用的是 vhost-user（如 Open vSwitch），需要显式开启 IOMMU 支持：

```bash
ovs-vsctl set Open_vSwitch . other_config:vhost-iommu-support=true
```

虽然 vhost IOMMU 提高了安全性，但它并非没有代价：

| 维度         | 影响                                                                                    |
| ---          | ---                                                                                     |
| **性能**     | **下降**。由于增加了地址转换步骤和 IOTLB 查找，I/O 延迟会增加，吞吐量可能下降 10%~30%。 |
| **内存开销** | **增加**。需要额外的物理内存来存储页表和缓存。                                          |
| **复杂度**   | 配置链路较长，排查 I/O 错误（如 DMA-API error）的难度增加。                             |

**总结建议：** 除非你需要**在虚拟机内运行 VFIO/DPDK**，或者对**多租户安全隔离**有极高要求，否则在普通虚拟化场景下，默认不建议开启此功能以保持最高性能。

```c
bool vhost_dev_has_iommu(struct vhost_dev *dev)
{
    VirtIODevice *vdev = dev->vdev;

    /*
     * For vhost, VIRTIO_F_IOMMU_PLATFORM means the backend support
     * incremental memory mapping API via IOTLB API. For platform that
     * does not have IOMMU, there's no need to enable this feature
     * which may cause unnecessary IOTLB miss/update transactions.
     */
    if (vdev) {
        return virtio_bus_device_iommu_enabled(vdev) &&
            virtio_host_has_feature(vdev, VIRTIO_F_IOMMU_PLATFORM);
    } else {
        return false;
    }
}
```

- http://events17.linuxfoundation.org/sites/events/files/slides/vhost-user_iommu_prague.pdf
- https://www.redhat.com/en/blog/journey-vhost-users-realm

都是软件实现的，感觉也就是 DPDK 这种使用大页的情况可以勉强使用，
不然 tlb miss 导致的开销让人难以接受。

## TODO

那么内核中 drivers/vhost/iotlb.c 又是咋用的，当 qemu 使用 viommu 的时候，vhost-net 自动需要使用这个?

这两个做啥用的?
```c
const VhostOps kernel_ops = {
  // ...
        .vhost_set_iotlb_callback = vhost_kernel_set_iotlb_callback,
        .vhost_send_device_iotlb_msg = vhost_kernel_send_device_iotlb_msg,

```

## 为什么 vhost-net 需要依赖 vhost_iotlb 啊

## 如果 qemu 打开了 iommu ，那么 vhost-net 的设备自动是打开了 iommu 吗?

不去添加这个可以么?
```txt
iommu_platform=on,disable-legacy=on
```

奇怪，这么想，如果 vhost 后端不支持 iommu ，其实这个东西根本没有办法正常工作啊。

现在更加懵逼了，为什么 tlb miss 的消息是从 vq 中获取的
```txt
@[
    vhost_iotlb_miss.isra.0+1
    translate_desc+306
    vhost_get_vq_desc+611
    get_tx_bufs.constprop.0+66
    handle_tx_copy+159
    handle_tx+161
    vhost_run_work_list+66
    vhost_task_fn+85
    ret_from_fork+45
    ret_from_fork_asm+27
]: 2
@[
    vhost_iotlb_miss.isra.0+1
    translate_desc+306
    vhost_get_vq_desc+385
    get_rx_bufs+153
    handle_rx+509
    vhost_run_work_list+66
    vhost_task_fn+85
    ret_from_fork+45
    ret_from_fork_asm+27
]: 11
```

## 似乎还需要有自己的替换算法?
drivers/vhost/vhost.c
```c
static ushort max_mem_regions = 64;
module_param(max_mem_regions, ushort, 0444);
MODULE_PARM_DESC(max_mem_regions,
	"Maximum number of memory regions in memory map. (default: 64)");
static int max_iotlb_entries = 2048;
module_param(max_iotlb_entries, int, 0444);
MODULE_PARM_DESC(max_iotlb_entries,
	"Maximum number of iotlb entries. (default: 2048)");
```

## 之前记得如果 fio nvme + iommu

然后 fio 性能会特别差，每次数据传输的时候都是需要查询一下 iotlb ，
那么 vhost-net 也会如此么?

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
