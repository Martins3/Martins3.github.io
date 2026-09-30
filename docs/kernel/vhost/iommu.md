# vhost iotlb
<!-- c9c6f8da-4df0-4977-af3e-16ba391bf09a -->

## 文档
vhost IOMMU support <https://www.qemu.org/docs/master/interop/vhost-user.html#iommu-support>

- http://events17.linuxfoundation.org/sites/events/files/slides/vhost-user_iommu_prague.pdf

都是软件实现的，感觉也就是 DPDK 这种使用大页的情况可以勉强使用，不然 tlb miss 导致的开销让人难以接受。

## 动机

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

## 核心原理

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


## vdpa

### vdpa sim

2026-09-29 搭建一个环境，然后获取一个调用到 vdpasim_dma_map 的 backtrace 也许是很好的

```c
static int vdpasim_dma_map(struct vdpa_device *vdpa, unsigned int asid,
			   u64 iova, u64 size,
			   u64 pa, u32 perm, void *opaque)
{
	struct vdpasim *vdpasim = vdpa_to_sim(vdpa);
	int ret;

	if (asid >= vdpasim->dev_attr.nas)
		return -EINVAL;

	spin_lock(&vdpasim->iommu_lock);
	if (vdpasim->iommu_pt[asid]) {
		vhost_iotlb_reset(&vdpasim->iommu[asid]);
		vdpasim->iommu_pt[asid] = false;
	}
	ret = vhost_iotlb_add_range_ctx(&vdpasim->iommu[asid], iova,
					iova + size - 1, pa, perm, opaque);
	spin_unlock(&vdpasim->iommu_lock);

	return ret;
}
```

容易混淆的是：通用 vhost 软件后端有这套能力，但 vhost_vdpa 没有自动继承它。

当前 drivers/vhost/vhost.c:1825 中确实存在：

vhost_iotlb_miss()
  → 将 MISS 放入 read_list
  → 用户态通过 read() 取走消息
  → 用户态写回 UPDATE
  → vhost_iotlb_notify_vq()
  → 重新调度等待的 virtqueue

例如 vhost_net 接入了这些读写接口。但 vhost_vdpa 使用自己的 IOTLB 消息处理函数，数据面交给底层设备；vdpasim 又是通过 vringh 访问内存，因此没有走上面这
套 miss 处理流程。

所以，qemu 处理的比较简单，利用上 memory listener
对应 QEMU 中的 vhost_vdpa_listener_region_add() 和
vhost_vdpa_iommu_map_notify()。所以“主动下发”不一定意味着启动时一次性映射所有地址，也可以随映射变化动态维护。见 QEMU vhost-vDPA 源码
(https://github.com/qemu/qemu/blob/master/hw/virtio/vhost-vdpa.c)。

### mlnx 等物理硬件

2026-09-29 : 看上去的确是这么回事了。

当前上游 QEMU 的 vhost_vdpa_iommu_region_add() 注册：

iommu_notifier_init(...,
                    vhost_vdpa_iommu_map_notify,
                    IOMMU_NOTIFIER_IOTLB_EVENTS,
                    ...);

并通过 memory_region_iommu_replay() 同步已经存在的映射。后续 vhost_vdpa_iommu_map_notify() 把映射变化转换成 UPDATE/INVALIDATE。QEMU vhost-vDPA 源码
(https://github.com/qemu/qemu/blob/master/hw/virtio/vhost-vdpa.c)

在你当前内核中，接下来进入：

vhost_vdpa 更新映射
  → mlx5_vdpa_set_map()
  → set_map_data()
  → mlx5_vdpa_create_mr()
  → 更新硬件 MKey/MTT

见 mlx5_vdpa_set_map() (drivers/vdpa/mlx5/net/mlx5_vnet.c:3382)。

mlx5 硬件最终拿到的是 guest IOVA 到宿主机 DMA 地址的映射。如果宿主机还有物理 IOMMU，后面再由它转换到 HPA。mlx5 在这条路径中不直接遍历 guest 的 vIOMMU
页表。


## vhost-net

在内核中，我们观察到
vhost-net 模块是依赖的  vhost_iotlb 的

```c
const VhostOps kernel_ops = {
  // ...
        .vhost_set_iotlb_callback = vhost_kernel_set_iotlb_callback,
        .vhost_send_device_iotlb_msg = vhost_kernel_send_device_iotlb_msg,

```

### 基本流程

#### QEMU
1. 在 vhost fd 上注册读回调

vhost_dev_start() 启用 IOTLB 回调，最终进入 vhost_kernel_set_iotlb_callback() (/home/martins3/data/qemu/hw/virtio/vhost-kernel.c:349)：

qemu_set_fd_handler(vhost_fd, vhost_kernel_iotlb_read, NULL, dev);

内核产生 miss 后，fd 可读，QEMU 的事件循环调用 vhost_kernel_iotlb_read()。它通过 read() 读取 vhost_msg 或 vhost_msg_v2，然后交给：

vhost_kernel_iotlb_read()
  → vhost_handle_iotlb_msg()
    → vhost_device_iotlb_miss(dev, iova, write)

其中 write 来自 miss 的访问权限，用来告诉 vIOMMU：这次请求是设备读还是设备写。

2. 核心翻译入口是 vhost_device_iotlb_miss()

这个函数 (/home/martins3/data/qemu/hw/virtio/vhost.c:1368) 的主要逻辑可以简化为：

/* IOVA → GPA */
iotlb = address_space_get_iotlb_entry(dev->vdev->dma_as,
                                     iova, write,
                                     MEMTXATTRS_UNSPECIFIED);

/* GPA → QEMU 用户态地址 */
vhost_memory_region_lookup(dev, iotlb.translated_addr,
                           &uaddr, &len);

/* 映射长度受 IOMMU 页大小和 RAM region 边界限制 */
len = MIN(iotlb.addr_mask + 1, len);
iova &= ~iotlb.addr_mask;

/* 将翻译结果发给内核 */
vhost_update_device_iotlb(dev, iova, iotlb.translated_addr,
                         uaddr, len, iotlb.perm);

#### kernel


  vhost worker                         QEMU
       │                                 │
       ├─ translate_desc(IOVA)            │
       ├─ 缓存不存在                     │
       ├─ 生成 VHOST_IOTLB_MISS ─────────►│ read(vhost_fd)
       ├─ 返回 -EAGAIN，退出本次处理       │
       │                                 ├─ 查询设备地址空间 / vIOMMU
       │                                 ├─ IOVA → GPA → HVA
       │◄──────── VHOST_IOTLB_UPDATE ─────┤ write(vhost_fd)
       ├─ 插入 IOVA → HVA 映射            │
       ├─ 重新调度对应 virtqueue          │
       └─ 重试 descriptor，继续收发       │




### 替换算法
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


## TODO

### 之前记得如果 fio nvme + iommu

然后 fio 性能会特别差，每次数据传输的时候都是需要查询一下 iotlb ，
那么 vhost-net 也会如此么?
### 如果 qemu 打开了 iommu ，那么 vhost-net 的设备自动是打开了 iommu 吗?

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




## A journey to the vhost-users realm
<https://www.redhat.com/en/blog/journey-vhost-users-realm>

When a device that is being emulated in QEMU attempts to DMA to the guest’s virtio I/O space it will use the vIOMMU TLB to look up the page mapping and perform a secured DMA access.
Question is what happens if the actual DMA was being offloaded to an external process such as a DPDK application using vhost-user library?

### VHOST_USER_PROTOCOL_F_SLAVE_REQ

这里聊到了一个经典的特性: VHOST_USER_PROTOCOL_F_SLAVE_REQ

主 socket 的协议交互由 QEMU 发起，例如配置共享内存、virtqueue，后端接收并按需回复。

但启用 guest vIOMMU 后，后端处理 descriptor 时可能遇到一个 IOVA，却不知道它对应哪块内存。
这时需要后端主动问 QEMU：“这个 IOVA 怎么翻译？” 这是 SPDK 给 QEMU 发消息，所以就需要第二个通道。

#### 无须额外的
2026-09-29 : socketpair 还需要确认

QEMU 在 hw/virtio/vhost-user.c 的 vhost_setup_slave_channel() 中，逻辑如下：

QEMU 调用 socketpair()
          │
          └── 得到已经互相连接的两个端点：sv[0] 和 sv[1]

QEMU 保留 sv[0]
QEMU 通过主 socket 发送 VHOST_USER_SET_SLAVE_REQ_FD
     并用 SCM_RIGHTS 把 sv[1] 传给后端

最终：
QEMU 持有的端点  <────────────>  后端持有的端点

因此，无需另设 socket 路径、listen()、accept()。传 fd 也不是单纯发送一个整数，而是让接收进程获得引用同一 socket 端点的本地
fd；两边的 fd 数值可以不同。发送成功后，QEMU 关闭自己手中的 sv[1] 副本。QEMU v4.1.0：vhost_setup_slave_channel()
(https://github.com/qemu/qemu/blob/v4.1.0/hw/virtio/vhost-user.c)

#### IOTLB synchronization 同步细节

后端维护一份软件地址翻译缓存，即 IOTLB。例如，后端需要访问 IOVA = 0x1000，但缓存没有对应映射：

          ┌───────────────┐                               ┌───────────────┐
          │ OVS/DPDK 后端 │                               │ QEMU / vIOMMU │
          └───────────────┘                               └───────────────┘
                  ┌──────────────────────────────────────────┐    │
                  │ Note: 需要访问 IOVA 0x1000，IOTLB 未命中 │    │
                  └──────────────────────────────────────────┘    │
                  │ 第二条通道：IOTLB MISS                        │
                  ├───────────────────────────────────────────────▶
                  │                                               │
                  │                                               ┌──────────────────────────────────┐
                  │                                               │ Note: 查询 IOVA → GPA → QEMU HVA │
                  │                                               └──────────────────────────────────┘
                  │ 主通道：IOTLB UPDATE，提供映射和权限          │
                  ◀───────────────────────────────────────────────┤
                  │                                               │
                  ┌──────────────────────────────────────────────┐│
                  │ Note: 缓存映射，换算为自身 HVA，访问共享内存 ││
                  └──────────────────────────────────────────────┘│
                  │ 主通道：IOTLB INVALIDATE（映射撤销时）        │
                  ◀───────────────────────────────────────────────┤
                  │                                               │
                  ┌──────────────────────────┐                    │
                  │ Note: 删除失效的缓存映射 │                    │
                  └──────────────────────────┘                    │

并非所有 IOTLB 消息都走第二条通道：后端发出的 MISS 走第二条通道，QEMU 发出的 UPDATE、INVALIDATE
走主通道。缓存命中时，后端直接访问共享内存，无需每次询问 QEMU。

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
