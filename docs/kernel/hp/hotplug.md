# hotplug 概述

- 调查下 HOTPLUG_PCI_ACPI  的影响吧

- 除了 acpi 还可以通过什么方式吗?


- hotplug 专题整理下
    - cpu
    - 内存的热插拔 : acpihp
    - nvme -> vmd / pciehp
    - sdd -> block events
    - usb
    - 测试 windows 的热拔盘的会出现占用吗?


分析 raid 的热插拔

Hotplug was introduced into the Linux kernel to implement the popular consumer
feature known as Plug and Play (PnP). This feature allows the kernel to detect the
insertion or removal of hot-pluggable devices and to notify a user-space application,
giving the latter enough details to make it able to load the associated driver if needed,
and to apply the associated configuration if one is present.

- [ ] 从设备插入到驱动被加载，整个过程是什么样子 0 的?

## 为什么 VFIO 的热插拔很难的处理?

- 设备直通的时候如何热插拔?
  - 拔掉的时候，host 如何将 acpihp 的信息传递搞 guest 中去
  - 热插入一个应该不难，首先 host 感知该设备，然后让 qemu 把设备热插入进去

## vmd 和 vroc

从这里看 https://www.intel.com/content/www/us/en/software/virtual-raid-on-cpu-vroc.html
vroc 是整个计数解决方案，包含了 vmd

- https://lenovopress.lenovo.com/lp1576.pdf
  - 直通 vmd

- https://www.intel.com/content/www/us/en/architecture-and-technology/intel-volume-management-device-overview.html
  - vmd 的介绍

## 使用 hacking_virtio_iommu=true 之后，很多盘都不可以热拔了

有趣的

```txt
(qemu) device_del root1
Error: Bus 'pcie.0' does not support hotplugging
```
## arm 上需要借助 pcie port 才可以热插

https://libvirt.org/pci-hotplug.html

所以，需要这两个选项打开:
```txt
CONFIG_PCIEPORTBUS=y
CONFIG_HOTPLUG_PCI_PCIE=y
```

## usb

virtio scsi, megaraid 以及 sata, usb 设备都会调用 sd_probe

```txt
@[
    sd_probe+5
    really_probe+415
    __driver_probe_device+120
    driver_probe_device+31
    __device_attach_driver+137
    bus_for_each_drv+146
    __device_attach_async_helper+165
    async_run_entry_fn+49
    process_one_work+479
    worker_thread+81
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 1
```

那么插上一个 U 盘，具体的探测过程是什么样子的
谁最先发现，又是如何一步步的通知的， 最后到 sd_probe 的。

## 还有从 pcie 的角度来看
pciehp

## 看看这个东西
https://news.ycombinator.com/item?id=47215602

## 现状
其实基本上已经成熟了

现在已经可以完善的做
nvme nic cpu memory 的热插拔，
所以，现在是需要的是 usb 热插拔

然后调查一下，除掉了这些热插拔，还有什么东西:

1. 直通到虚拟机的设备其实似乎可以热插拔的

可以想的角度:
1. 对于锁的实现
2. 从固件的角度 (ACPI)
3. pciehp 的角度

必须加上的东西，火影忍者的写轮眼的热插拔

## 思考一个问题，如何设计一个优秀的 hotplug 机制?

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
