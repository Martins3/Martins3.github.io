## drivers/vfio/pci/virtio 是做什么的
<!-- 99e832c7-8618-44d8-8839-4aef52992ab4 -->

对应这个邮件: https://lore.kernel.org/all/20231219093247.170936-1-yishaih@nvidia.com/

> [!NOTE]
> 参考神奇海螺的意见，有待验证

这个代码是 Linux 内核中的 **virtio-vfio-pci** 驱动（也叫 **VIRTIO VFIO PCI**），它是 NVIDIA（以及社区）为支持 **在虚拟机中通过 VFIO 直通现代 virtio 设备**（尤其是 BlueField DPU/SmartNIC 上的 virtio-net/virtio-blk VF）而开发的“用户态元驱动”（user-level meta-driver）。

简单来说：**它让 QEMU/KVM 里的虚拟机可以直接使用宿主机上的高性能 virtio-net 设备（而不是走慢的模拟路径），并且支持 live migration（实时迁移）**。

### 它到底解决了什么问题？

传统 virtio 设备有两种驱动方式：

1. **Legacy/transitional mode**：通过 PCI BAR0 的 I/O 端口访问（类似老的 virtio-pci），性能差，迁移也麻烦。
2. **Modern-only mode**：只用队列通知 + 配置文件通过 admin queue，不暴露 BAR0，性能最好，但旧的 virtio-pci 驱动不认识。

而在 BlueField-3 等 DPU 上，PF（物理功能）可以创建很多 virtio-net VF，这些 VF 默认是 **modern-only + 没有 BAR0** 的，所以普通 virtio-pci 驱动根本认不出来，更别说直通给 VM 用了。

NVIDIA 又想让客户在 VM 里也能拿到接近裸机的 virtio-net 性能，同时还要支持迁移，于是就写了这个驱动。

### 这个驱动的核心作用

| 功能                                                    | 传统 vfio-pci 能做到吗？                | virtio-vfio-pci 做到了什么                        |
|---------------------------------------------------------|-----------------------------------------|---------------------------------------------------|
| 直通 modern-only virtio 设备                            | 否（没 BAR0，guest 驱动找不到配置空间） | 是，通过虚拟出一个“假的” BAR0 + legacy 配置空间   |
| 支持 guest 使用旧的 virtio-pci legacy/transitional 驱动 | 是                                      | 是（故意伪装成 transitional device）              |
| 支持 live migration（含 pre-copy）                      | 部分设备支持                            | 完全支持，通过 virtio admin cmd 的 dev_parts 机制 |
| 支持 P2P migration                                      | 否                                      | 是（支持 VFIO_MIGRATION_P2P）                     |
| 性能                                                    | 直通现代设备本来就快                    | 几乎无损（只是多了一层很薄的转接）                |

### 代码各文件是干什么的？

| 文件            | 主要功能                                                                                                                                                                                                  |
|-----------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **legacy_io.c** | 最重要的部分：虚拟出一个 BAR0（在内存中分配一段 buffer），当 guest 访问这个假的 I/O BAR 时，驱动把读写翻译成 `virtio_pci_admin_legacy_*` 命令发给 PF，从而让 guest 以为自己在跟一个传统 virtio 设备对话。 |
| **main.c**      | 驱动入口、probe、各种 vfio_device_ops 的注册。根据设备是否支持 legacy IO 或 live migration，选择不同的 ops（有三个 ops 集合）。                                                                           |
| **migrate.c**   | 实现完整的 VFIO live migration 接口（STOP_COPY + PRE_COPY + P2P），通过 virtio admin cmd 的 `dev_parts_get/set` 把设备完整状态拉出来/塞回去，支持 QEMU 的迁移流程。                                       |
| **common.h**    | 共享结构体和宏定义                                                                                                                                                                                        |

### 在 PCI 配置空间里伪装成了什么设备？

从代码可以看到它把设备伪装成：

```txt
Device ID  : 0x1001 → 显示为 VIRTIO_TRANS_ID_NET (0x1000) + 1 = transitional virtio-net
Subsystem ID : 0x0001 → VIRTIO_ID_NET (老的 virtio-net ID)
Subsystem Vendor : Red Hat (0x1af4)
Revision ID : 0 (transitional 必须是 0)
BAR0 : 伪造成 I/O 端口空间，大小 = 配置区 + device config space
```

这样 Linux 里任何旧的 virtio_net 驱动（包括很老的发行版）都能认出来并正常工作。

### 实际使用场景（典型例子）

```bash
# 宿主机（BlueField DPU）
modprobe virtio-vfio-pci

# 创建一个 virtio-net VF（PF 用 mlx5 驱动）
echo 8 > /sys/class/net/enp0s13f0/device/sriov_numvfs

# 把这个 VF bind 到 virtio-vfio-pci 驱动（而不是 vfio-pci）
echo 1af4 1041 > /sys/bus/pci/drivers/virtio_vfio_pci/new_id

# QEMU 启动参数里直接 -device vfio-pci,host=0000:03:00.0
```

这样 VM 里看到的就是一个普通的 virtio-net-pci 设备，性能接近裸机，还能无缝迁移。

## 直接在虚拟机中测试似乎也可以

```sh
bdf=0000:00:05.0
modprobe virtio-vfio-pci
echo "$bdf" | sudo tee /sys/bus/pci/devices/"$bdf"/driver/unbind
echo 1af4 1041 > /sys/bus/pci/drivers/virtio_vfio_pci/new_id
lspci -s $bdf -v
```

测试，会遇到这个问题:
```txt
[   22.402129] virtio_vfio_pci 0000:00:05.0: probe with driver virtio_vfio_pci failed with error -22
```
也许虚拟机没有打开 iommu 导致的吗？ 但是 不管怎么说，都是可以确认的，这个 virtio-vfio-pci 就是为了让
virtio 设备直通到虚拟机


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
