2. vfio, mdev, vfio_mdev 分别是三个模块，分别负责什么?

## https://learn.microsoft.com/en-us/windows-hardware/drivers/network/overview-of-single-root-i-o-virtualization--sr-iov-

https://blog.blog.kernel.love/vfio-mdev.html

https://open-iov.org/index.php/GPU_Support

https://news.ycombinator.com/item?id=28944426

https://github.com/k8snetworkplumbingwg/sriov-network-device-plugin

## sriov : 真简单啊

- https://docs.kernel.org/PCI/pci-iov-howto.html

## [ ] 所以，如何配置 virtio 支持 sriov

```c
static struct pci_driver virtio_pci_driver = {
	.name		= "virtio-pci",
	.id_table	= virtio_pci_id_table,
	.probe		= virtio_pci_probe,
	.remove		= virtio_pci_remove,
#ifdef CONFIG_PM_SLEEP
	.driver.pm	= &virtio_pci_pm_ops,
#endif
	.sriov_configure = virtio_pci_sriov_configure,
};
```

## [x] sriov 需要 guest os 参与吗?

原则上来说，不需要

## i915-sriov-dkms
https://github.com/strongtz/i915-sriov-dkms


## 从 qemu 的角度看看
https://kernelnote.com/deep-dive-sriov-in-qemu.html

docs/pcie_sriov.txt

## 看看这个啊
https://kvm-forum.qemu.org/2024/Unleashing_SR-IOV_on_Virtual_Machines_qSX9OJ9.pdf
看看这个，从这个看，最近其实很多设备都支持

虚拟机中支持模拟 sriov ，有意义吗?

https://qemu-project.gitlab.io/qemu/system/devices/nvme.html

## 如果 vf 还是同一个设备，只是不同的 bar ，可以到物理机中观察一下

## 似乎 qemu 中的 hw/net/igbvf.c 就是为了 vf

## igb 才是 sriov 的
那么 igb 和 vfigb 分别是做什么的，看来 vf
是需要一个辅助的 driver 的。


```txt
🧀  lspci | grep Eth
00:03.0 Ethernet controller: Red Hat, Inc. Virtio network device
00:04.0 Ethernet controller: Red Hat, Inc. Virtio network device
00:05.0 Ethernet controller: Red Hat, Inc. Virtio network device
01:00.0 Ethernet controller: Intel Corporation 82576 Gigabit Network Connection (rev 01)
01:10.0 Ethernet controller: Intel Corporation 82576 Virtual Function (rev 01)

🧀  lsmod | grep igb
igbvf                  61440  0
igb                   323584  0
```

### 为什么 qemu 必需给给配置一个 port

arg_nvme+=" -device pcie-root-port,slot=3,id=pcie_port.3"

## 参考一下这个
- https://kernelnote.com/deep-dive-sriov-in-qemu.html

## enable 之后
echo 1 | sudo tee /sys/bus/pci/devices/0000:01:00.0/sriov_numvfs
```txt
[   61.311463] igb 0000:01:00.0: 1 VFs allocated
[   62.319029] pci 0000:01:10.0: [8086:10ca] type 00 class 0x020000 PCIe Endpoint
[   62.320888] pci 0000:01:10.0: enabling Extended Tags
[   62.331097] igbvf: Intel(R) Gigabit Virtual Function Network Driver
[   62.331680] igbvf: Copyright (c) 2009 - 2012 Intel Corporation.
[   62.332220] igbvf 0000:01:10.0: enabling device (0000 -> 0002)
[   62.346661] igbvf 0000:01:10.0: MAC address not assigned by administrator.
[   62.347462] igbvf 0000:01:10.0: Assigning random MAC address.
[   62.360960] igbvf 0000:01:10.0: Intel(R) 82576 Virtual Function
[   62.361785] igbvf 0000:01:10.0: Address: 76:9f:29:12:1d:e0
[   62.364587] igbvf 0000:01:10.0 enp1s0v0: renamed from eth0
[   62.370034] igb 0000:01:00.0: VF 0 attempted to set invalid MAC filter
[   62.397280] igb 0000:01:00.0: VF 0 attempted to set invalid MAC filter
[   62.400607] igb 0000:01:00.0: VF 0 attempted to set invalid MAC filter
[   62.688909] igb 0000:01:00.0 enp1s0: igb: enp1s0 NIC Link is Up 1000 Mbps Full Duplex, Flow Control: RX/TX
```


```txt
  fe800000-fe9fffff : PCI Bus 0000:01
    fe800000-fe81ffff : 0000:01:00.0
      fe800000-fe81ffff : igb
    fe820000-fe83ffff : 0000:01:00.0
      fe820000-fe83ffff : igb
    fe840000-fe843fff : 0000:01:00.0
      fe840000-fe843fff : igb
```

enable 之后多出来这部分:
```txt
  e0000000000-e01ffffffff : PCI Bus 0000:02
  e0200000000-e03ffffffff : PCI Bus 0000:01
    e0200000000-e020001ffff : 0000:01:00.0
      e0200000000-e0200003fff : 0000:01:10.0
        e0200000000-e0200003fff : igbvf
    e0200020000-e020003ffff : 0000:01:00.0
      e0200020000-e0200023fff : 0000:01:10.0
        e0200020000-e0200023fff : igbvf
```

驱动走的都不是一个:
```txt
@[
    igbvf_get_drvinfo+5
    ethtool_get_drvinfo+79
    dev_ethtool+2240
    dev_ioctl+560
    sock_do_ioctl+339
    sock_ioctl+648
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

## 大概率所有的 vf 都是共用一个 pci config

## 有办法知道一个 iommu 是否支持 pasid 吗?

## sysfs 中包含了什么东西

/sys/devices/pci0000:80/0000:80:00.0/0000:81:00.1
```txt
aer_dev_correctable       current_link_speed  infiniband        max_link_width    pools      revision                 sriov_vf_device
aer_dev_fatal             current_link_width  infiniband_mad    mlx5_core.eth.1   power      roce_enable              subsystem
aer_dev_nonfatal          d3cold_allowed      infiniband_verbs  mlx5_core.rdma.1  ptp        rom                      subsystem_device
ari_enabled               device              iommu             mlx5_num_vfs      real_miss  sriov                    subsystem_vendor
broken_parity_status      devspec             iommu_group       modalias          remove     sriov_drivers_autoprobe  uevent
class                     dma_mask_bits       irq               msi_bus           rescan     sriov_numvfs             vendor
commands_cache            driver              local_cpulist     msi_irqs          reset      sriov_offset             vpd
config                    driver_override     local_cpus        net               resource   sriov_stride
consistent_dma_mask_bits  enable              max_link_speed    numa_node         resource0  sriov_totalvfs
```

### sriov_numvfs 和 sriov_totalvfs

参考
- https://zhuanlan.zhihu.com/p/651023182
  - https://enterprise-support.nvidia.com/s/article/HowTo-Configure-SR-IOV-for-ConnectX-4-ConnectX-5-ConnectX-6-with-KVM-Ethernet

如果要修改  sriov_numvfs ，那么需要把所有的虚拟机中的 vf 拔掉，然后才可以。
如果要修改 sriov_totalvfs ，其实是修改的固件中的内容，修改之后，是需要重启的。

Mellanox 提供了 mlxconfig 来修改

```c
/**
 * pci_sriov_get_totalvfs -- get total VFs supported on this device
 * @dev: the PCI PF device
 *
 * For a PCIe device with SRIOV support, return the PCIe
 * SRIOV capability value of TotalVFs or the value of driver_max_VFs
 * if the driver reduced it.  Otherwise 0.
 */
int pci_sriov_get_totalvfs(struct pci_dev *dev)
{
	if (!dev->is_physfn)
		return 0;

	return dev->sriov->driver_max_VFs;
}
```

我靠，这个 total 是 pci config 中获取的
sriov_init 中看到这个:
```c
	pci_read_config_word(dev, pos + PCI_SRIOV_TOTAL_VF, &total);
	if (!total)
		return 0;
```
## 惭愧，惭愧

> [!NOTE]
> 参考 Deepseeek ，有待验证

网卡多PF与SR-IOV的详细对比
为了更好地理解，我们可以用一个比喻：假设一张物理网卡是一栋办公楼。
切分为多个PF：相当于把这栋楼在物理上划分成几个完全独立的“公司”。每个公司（PF）都有自己的前台、会议室、电源和网络接口，彼此完全独立，甚至可以有不同的“公司文化”（驱动和配置）。
使用SR-IOV：相当于楼里只有一个大公司（PF），但这个大公司为员工设立了大量的直达“个人邮箱”（VF）。员工（虚拟机）可以直接从自己的邮箱收发信件，无需经过公司的中央收发室（Hypervisor虚拟交换机），速度极快。但所有邮箱的管理和维护都由这个大公司负责。
对比表格

|------------------|
| 特性             | 切分为多个PF (Multi-Host / NPAR) 使用SR-IOV
| 基本概念         | 将一个物理设备在硬件层面分割成多个独立的物理功能 (PF)                                                                                                          | 。 从一个物理功能 (PF) 中创建出多个轻量级的虚拟功能 (VF)。                                                                                                          |
| 呈现给系统的方式 | 系统（如通过 lspci）会看到多个独立的PCIe设备，就像插了多张物理网卡一样                                                                                         | 。 系统会看到一个PCIe设备（PF）和许多附属在其下的VF。                                                                                                               |
| 资源划分         | 硬件级硬划分                                                                                                                                                   | 。每个PF都分配有自己独立的、受保护的资源（如带宽、队列、MAC地址）。 共享资源。所有VF共享同一个PF的物理资源。PF负责管理和配置这些资源，可以为VF设置带宽限制（QoS）。 |
| 隔离性           | 极强                                                                                                                                                           | 。每个PF之间是完全隔离的，一个PF的故障或重置通常不会影响其他PF。 强。VF之间的数据路径是隔离的，但它们最终都由同一个PF管理。PF的故障或重置会影响其下所有的VF。       |
| 驱动和管理       | 每个PF都需要独立的驱动程序实例来管理                                                                                                                           | 。它们可以被分配给不同的裸金属服务器或虚拟机。 只需要一个PF驱动程序来管理物理网卡和其下所有的VF。VF通常使用一个功能简化的驱动。                                     |
| 性能             | 每个PF都拥有完整的物理功能，性能高且稳定                                                                                                                       | 。 VF的数据路径直接访问硬件，绕过了Hypervisor，网络性能极高，延迟极低，接近物理网卡的性能。                                                                         |
| 灵活性和规模     | 规模较小                                                                                                                                                       | 。一张网卡通常只能切分成少数几个PF（如2个、4个、8个）。 规模较大。一张网卡可以创建出非常多（几十甚至上百个）的VF。                                                  |
| 典型用例         | • 多主机（Multi-Host）：一张网卡服务多个独立的裸金属服务器。<br>• 高安全隔离：为需要强隔离环境的虚拟机或容器分配独立的“物理”网卡。<br>• 裸金属即服务 (BMaaS) • | 大规模虚拟化：为大量虚拟机（VM）或容器提供高性能网络。<br>• 网络功能虚拟化 (NFV)：如虚拟路由器、防火墙等。<br>• 云计算和数据中心                                    |

详细解释
切分为多个PF (Multi-Host / NPAR)
这种技术通常由特定的网卡硬件支持，例如NVIDIA (Mellanox) 的 Multi-Host 技术或 Broadcom/QLogic 的 NPAR (NIC Partitioning)。
- 工作原理：网卡内部集成了一个PCIe交换机，它将网卡的内部资源（如处理核心、内存、队列）硬性地划分成几个独立的分区。每个分区都通过PCIe总线暴露为一个完整的、独立的物理功能（PF）。
- 系统视角：当您在服务器上执行 lspci 时，您会看到多个PCIe设备，它们都来自同一张物理卡，但拥有不同的PCIe总线地址。操作系统会为每一个PF加载一个独立的驱动程序实例。
- 优势：提供了最强的隔离性。您可以将一个PF直通（Passthrough）给一个虚拟机，而将另一个PF分配给宿主机或其他虚拟机，甚至通过线缆连接到另一台物理服务器。它们就像是完全不相关的两张卡。

使用SR-IOV (Single Root I/O Virtualization)
SR-IOV是PCI-SIG定义的一项标准化技术，被广泛支持。
- 工作原理：SR-IOV允许一个PCIe设备（如网卡）创建出两种功能：
- 物理功能 (Physical Function, PF)：这是一个功能完整的PCIe设备，拥有发现、管理和配置SR-IOV功能的能力。PF驱动由Hypervisor（宿主机）加载和管理。
- 虚拟功能 (Virtual Function, VF)：这是一个轻量级的PCIe设备，它只包含数据传输所必需的资源，而没有完整的配置能力。VF的配置由其所属的PF来完成。
系统视角：lspci 会显示一个PF设备。在启用SR-IOV并创建VF后，您会看到许多新的、功能类型为VF的PCIe设备“挂”在那个PF下面。
优势：极高的性能和扩展性。当一个VF被直通给虚拟机后，虚拟机的数据包可以直接在VF和物理网卡之间传输，完全绕过了Hypervisor的虚拟交换机（如vSwitch），从而消除了软件交换带来的CPU开销和延迟。这使得虚拟机的网络性能可以接近物理机的水平，并且一张卡可以支持非常多的虚拟机。

结论：如何选择？
如果您需要将一张昂贵的高性能网卡分享给少数几个需要强隔离的实体（例如，两个裸金属服务器，或者一个宿主机和一个需要独占“物理”网卡的高安全虚拟机），切分为多个PF是合适的选择。
如果您需要在单一宿主机上为大量虚拟机或容器提供高性能、低延迟的网络连接，SR-IOV是行业标准和最佳实践。
希望这份详细的对比能帮助您清晰地理解这两种技术的区别！

<!-- ds 结束-->

有办法测试一下 pf 的切分吗? 似乎需要使用配套的工具。

## /sys/class/net/*/device 下的这些 sriov 如何理解?
```txt
sriov_drivers_autoprobe:1
sriov_numvfs:0
sriov_offset:2
sriov_stride:1
sriov_totalvfs:8
sriov_vf_device:1018
sriov_vf_total_msix:0
```

## vf 驱动
<!-- c1852adb-77b2-4100-b703-2684117b4e75 -->

有的网卡 vf pf 都是一个驱动，有的是两个驱动。

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
