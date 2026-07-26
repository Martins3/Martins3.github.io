## 看看这个 ACS override 是什么鬼?
https://superuser.com/questions/1350451/isolate-single-device-into-separate-iommu-group-for-pci-passthrough
 - https://vfio.blogspot.com/2014/08/iommu-groups-inside-and-out.html
 - https://queuecumber.gitlab.io/linux-acs-override/
 - https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF#Bypassing_the_IOMMU_groups_(ACS_override_patch)

## 看看内核是如何确定加入方法的

- iommu_init_device
  -	group = ops->device_group(dev);

viommu 和 intel 都是如此
```c
static struct iommu_group *viommu_device_group(struct device *dev)
{
	if (dev_is_pci(dev)
		return pci_device_group(dev);
	else
		return generic_device_group(dev);
}
```

amd 这样的:
```c
static struct iommu_group *amd_iommu_device_group(struct device *dev)
{
	if (dev_is_pci(dev))
		return pci_device_group(dev);

	return acpihid_device_group(dev);
}
```

aarch64 的: arm_smmu_device_group

- pci_device_group
  - get_pci_function_alias_group
    - get_pci_alias_group

这三个设备是由于分别几个 function
```txt
00:1f.0 ISA bridge: Intel Corporation 82801IB (ICH9) LPC Interface Controller (rev 02)
00:1f.2 SATA controller: Intel Corporation 82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode] (rev 02)
00:1f.3 SMBus: Intel Corporation 82801I (ICH9 Family) SMBus Controller (rev 02)
```

使用 alpine.sh 中的 nvme sriov ，
```txt
[   95.376646] pci 0000:01:00.1: [1b36:0010] type 00 class 0x010802 PCIe Endpoint
[   95.376877] pci 0000:01:00.1: enabling Extended Tags
[   95.377270] pci 0000:01:00.1: Adding to iommu group 16
[   95.377976] nvme nvme1: pci function 0000:01:00.1
[   95.378138] nvme 0000:01:00.1: enabling device (0000 -> 0002)
[   95.381532] nvme nvme1: Device not ready; aborting initialisation, CSTS=0x2
```
产生新的 group 的过程是:
```txt
01:00.0 Non-Volatile memory controller: Red Hat, Inc. QEMU NVM Express Controller (rev 02)
01:00.1 Non-Volatile memory controller: Red Hat, Inc. QEMU NVM Express Controller (rev 02)
```

探测过程也是在 iommu_probe_device 中
```txt
@[
    iommu_group_alloc+5
    pci_device_group+330
    __iommu_probe_device+290
    iommu_probe_device+36
    iommu_bus_notifier+43
    notifier_call_chain+90
    blocking_notifier_call_chain+63
    bus_notify+52
    device_add+1591
    pci_device_add+577
    pci_iov_add_virtfn+446
    sriov_enable+502
    pci_sriov_configure_simple+56
    sriov_numvfs_store+191
    kernfs_fop_write_iter+289
    vfs_write+673
    ksys_write+110
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```
其区别在于:
pci_acs_enabled(pdev, REQ_ACS_FLAGS) -> pci_dev_specific_acs_enabled

## 使用 alpine.sh 模拟的时候，观察到
qemu 模拟的，两个 nvme 在一个
```txt
IOMMU Group 9:
        00:0a.0 PCI bridge [0604]: Red Hat, Inc. QEMU PCI-PCI bridge [1b36:0001]
        03:01.0 Non-Volatile memory controller [0108]: Red Hat, Inc. QEMU NVM Express Controller [1b36:0010] (rev 02)
        03:02.0 Non-Volatile memory controller [0108]: Red Hat, Inc. QEMU NVM Express Controller [1b36:0010] (rev 02)
```
但是 sriov 可以每一个都切分一个:
```txt
IOMMU Group 16:
        01:00.0 Ethernet controller [0200]: Intel Corporation 82576 Gigabit Network Connection [8086:10c9] (rev 01)
IOMMU Group 17:
        01:10.0 Ethernet controller [0200]: Intel Corporation 82576 Virtual Function [8086:10ca] (rev 01)
IOMMU Group 18:
        01:10.2 Ethernet controller [0200]: Intel Corporation 82576 Virtual Function [8086:10ca] (rev 01)
IOMMU Group 19:
        01:10.4 Ethernet controller [0200]: Intel Corporation 82576 Virtual Function [8086:10ca] (rev 01)
```

## 在 qemu 中，在 pci bridge 下的 nvme 无法被直通
用这个查找
```sh
find /sys -name "*iommu*"
```
根本找不到

## PCI ACS

pci_device_group

```txt
	/*
	 * Continue upstream from the point of minimum IOMMU granularity
	 * due to aliases to the point where devices are protected from
	 * peer-to-peer DMA by PCI ACS.  Again, if we find an existing
	 * group, use it.
	 */
```


## 经典问题，有看到了 attach_device_to_domain 和 add_device_to_group

所以是什么区别呢?
```txt
 tracepoint -w -s
sudo perf stat -e iommu:*
^C
 Performance counter stats for 'system wide':

                 0      iommu:io_page_fault
              8674      iommu:unmap
             12641      iommu:map
                 0      iommu:attach_device_to_domain
                 0      iommu:remove_device_from_group
                 0      iommu:add_device_to_group

      12.426687319 seconds time elapsed
```

## [ ] 原来，iommu_groups 是可以动态创建的

通过 mtty 可以发现
```txt
/sys/kernel/iommu_groups/21/devices:
 83b8f4f2-509f-382f-3c1e-e6bfe0fa1001

🧀  ls /sys/kernel/iommu_groups/
 0   1   2   3   4   5   6   7   8   9   10   11   12   13   14   15   16   17   18   19   20   21
```

将 mdev device 删除之后:
```txt
🧀  ls /sys/kernel/iommu_groups/
 0   1   2   3   4   5   6   7   8   9   10   11   12   13   14   15   16   17   18   19   20
```

在虚拟机中测试更加直接:
```txt
cd /sys/devices/virtual/mtty/mtty/mdev_supported_types/mtty-1
uuidgen | sudo tee create
```

即便是虚拟机中，完全没有 iommu 支持，结果可以发现 iommu_groups
```txt
🧀  ls -la /sys/kernel/iommu_groups/0/devices
lrwxrwxrwx - root 15 Mar 11:35 4d29d626-f797-42b7-8dd9-08bc30ee719b -> ../../../../devices/virtual/mtty/mtty/4d29d626-f797-42b7-8dd9-08bc30ee719b
ls -la /sys/class/iommu
```

## pci_acs_enabled
看看这个函数的定义

## ats 和 acs

https://liujunming.top/2019/11/24/Introduction-to-PCIe-Access-Control-Services/

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
