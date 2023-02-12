# VFIO

通过 VFIO 可以将一个物理设备直接被 Guest 使用。

## 上手操作
测试机器是小米笔记本 pro 2019 版本，其显卡为 Nvidia MX150，在 Linux 上，其实 Nvidia 的显卡一般工作就很不正常，所以直通给 Guest 使用。
参考[内核文档](https://www.kernel.org/doc/html/latest/driver-api/vfio.html)，我这里记录一下操作:


1. 确定 GPU 的 bdf (*B*us *D*evice *F*unction)
```txt
➜  vn git:(master) ✗ lspci
00:00.0 Host bridge: Intel Corporation Xeon E3-1200 v6/7th Gen Core Processor Host Bridge/DRAM Registers (rev 08)
00:02.0 VGA compatible controller: Intel Corporation UHD Graphics 620 (rev 07)
00:08.0 System peripheral: Intel Corporation Xeon E3-1200 v5/v6 / E3-1500 v5 / 6th/7th/8th Gen Core Processor Gaussian Mixture Model
00:14.0 USB controller: Intel Corporation Sunrise Point-LP USB 3.0 xHCI Controller (rev 21)
00:14.2 Signal processing controller: Intel Corporation Sunrise Point-LP Thermal subsystem (rev 21)
00:15.0 Signal processing controller: Intel Corporation Sunrise Point-LP Serial IO I2C Controller #0 (rev 21)
00:15.1 Signal processing controller: Intel Corporation Sunrise Point-LP Serial IO I2C Controller #1 (rev 21)
00:16.0 Communication controller: Intel Corporation Sunrise Point-LP CSME HECI #1 (rev 21)
00:1c.0 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #1 (rev f1)
00:1c.4 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #5 (rev f1)
00:1c.7 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #8 (rev f1)
00:1d.0 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #9 (rev f1)
00:1f.0 ISA bridge: Intel Corporation Sunrise Point LPC Controller/eSPI Controller (rev 21)
00:1f.2 Memory controller: Intel Corporation Sunrise Point-LP PMC (rev 21)
00:1f.3 Audio device: Intel Corporation Sunrise Point-LP HD Audio (rev 21)
00:1f.4 SMBus: Intel Corporation Sunrise Point-LP SMBus (rev 21)
01:00.0 3D controller: NVIDIA Corporation GP108M [GeForce MX150] (rev ff)
02:00.0 Non-Volatile memory controller: ADATA Technology Co., Ltd. Device 0021 (rev 01)
03:00.0 Network controller: Intel Corporation Wireless 8265 / 8275 (rev 78)
04:00.0 Non-Volatile memory controller: Samsung Electronics Co Ltd NVMe SSD Controller SM961/PM961
➜  vn git:(master) ✗
```

2. 检测 iommu 是否支持，如果这个命令失败，那么修改 grub ，内核启动参数中增加上 `intel_iommu=on` [^4]
```txt
➜  vn git:(master) ✗ readlink /sys/bus/pci/devices/0000:01:00.0/iommu_group
../../../../kernel/iommu_groups/11
```

3. 如果 GPU 之前在被使用，那么首先需要解绑
```sh
sudo su
echo 0000:01:00.0 > /sys/bus/pci/devices/0000:01:00.0/driver/unbind
```

4. lspci -nn 获取设备的 vender 和 device id
```txt
01:00.0 VGA compatible controller [0300]: NVIDIA Corporation GP106 [GeForce GTX 1060 3GB] [10de:1c02] (rev a1)
01:00.1 Audio device [0403]: NVIDIA Corporation GP106 High Definition Audio Controller [10de:10f1] (rev a1)
```

5. 创建 vfio

```sh
echo 0000:01:00.0 | sudo tee /sys/bus/pci/devices/0000:01:00.0/driver/unbind
echo 10de 1c02 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id

echo 0000:01:00.1 | sudo tee /sys/bus/pci/devices/0000:01:00.1/driver/unbind
echo 10de 10f1 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
```

<!--
看看这个文章: https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm
这个教程也不错：https://github.com/bryansteiner/gpu-passthrough-tutorial
 -device vfio-pci,host=01:00.0,multifunction=on,x-vga=on 中的 x-vga 是什么含义？

nixos 下非常之好的讲解: https://astrid.tech/2022/09/22/0/nixos-gpu-vfio/
-->
<!-- @todo 可以集成显卡直通吗? -->
<!-- @todo 如何实现 QEMU 的复制粘贴 -->

6. 无需管理员权限运行
```txt
➜  vn git:(master) ✗ sudo chown maritns3:maritns3 /dev/vfio/11
```
实际上，因为 ulimit 的原因，会存在如下报错:
```txt
qemu-system-x86_64: -device vfio-pci,host=00:1f.3: vfio 0000:00:1f.3: failed to setup container for group 10: memory listener initialization failed: Region pc.ram: vfio
_dma_map(0x558becc6b3b0, 0xc0000, 0x7ff40000, 0x7f958bec0000) = -12 (Cannot allocate memory)
```
而修改 hard limit 的方法参考[此处](https://docs.oracle.com/cd/E19623-01/820-6168/file-descriptor-requirements.html)，有点麻烦。

6. qemu 中运行

```sh
-device vfio-pci,host=01:00.0
```

## usb 直通
- https://unix.stackexchange.com/questions/452934/can-i-pass-through-a-usb-port-via-qemu-command-line

```txt
/:  Bus 02.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/9p, 20000M/x2
    |__ Port 8: Dev 2, If 0, Class=Hub, Driver=hub/4p, 5000M
/:  Bus 01.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/16p, 480M
    |__ Port 1: Dev 21, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 1: Dev 21, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 1: Dev 21, If 0, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 2: Dev 3, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 2: Dev 3, If 0, Class=Vendor Specific Class, Driver=, 12M
    |__ Port 7: Dev 15, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 0, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 3, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 9: Dev 4, If 0, Class=Hub, Driver=hub/4p, 480M
        |__ Port 4: Dev 20, If 0, Class=Human Interface Device, Driver=usbhid, 12M
        |__ Port 4: Dev 20, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 11: Dev 5, If 0, Class=Human Interface Device, Driver=usbhid, 1.5M
    |__ Port 14: Dev 7, If 0, Class=Wireless, Driver=btusb, 12M
    |__ Port 14: Dev 7, If 1, Class=Wireless, Driver=btusb, 12M
```

```txt
Bus 002 Device 002: ID 174c:3074 ASMedia Technology Inc. ASM1074 SuperSpeed hub
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 001 Device 020: ID 0c45:7638 Microdia AKKO 3084BT
Bus 001 Device 004: ID 174c:2074 ASMedia Technology Inc. ASM1074 High-Speed hub
Bus 001 Device 015: ID 2717:003b Xiaomi Inc. MI Wireless Mouse
Bus 001 Device 003: ID 0b05:19af ASUSTek Computer, Inc. AURA LED Controller
Bus 001 Device 007: ID 8087:0026 Intel Corp. AX201 Bluetooth
Bus 001 Device 005: ID 17ef:6019 Lenovo M-U0025-O Mouse
Bus 001 Device 021: ID 2f68:0082 Hoksi Technology DURGOD Taurus K320
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
```

## 但是
我始终没有搞定笔记本上的 GPU 的直通，而且在台式机上直通成功的案例中，发现由于英雄联盟的翻作弊机制，也是无法成功运行的，不过可以运行原神。

## 其他的 vfio 尝试
```txt
# 00:17.0 SATA controller [0106]: Intel Corporation Alder Lake-S PCH SATA Controller [AHCI Mode] [8086:7ae2] (rev 11)
echo 0000:00:17.0 | sudo tee /sys/bus/pci/devices/0000:00:17.0/driver/unbind
echo 8086 7ae2 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
```

## 关键参考
- https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF#Passing_through_a_device_that_does_not_support_resetting
- https://superuser.com/questions/1293112/kvm-gpu-passthrough-of-nvidia-dgpu-on-laptop-with-hybrid-graphics-without-propri
- https://github.com/jscinoz/optimus-vfio-docs

## 其他扩展阅读
- https://www.kraxel.org/blog/2021/05/virtio-gpu-qemu-graphics-update/
- https://czak.pl/2020/04/09/three-levels-of-qemu-graphics.html

# 具体的代码分析

- [ ] 类似 RISC-V 中存在用户态中断，那么是不是可以设计出来更加酷炫的用户态 driver 来。
- [ ] 内核文档 iommu.rst 中的
- [ ] 如何理解 container 中的内容: `vfio_group_set_container`
    - 所以 container 是个什么概念
- `vfio_iommu_type1_group_iommu_domain` 中的 domain 是个什么含义
- [ ] 应该是 container 中含有 group 的
- [ ] 难道一个主板上可以有多个 IOMMU，否则，为什么会存在 `iommu_group`

## 其实这个事情应该很简单才对，映射中断和 bar

## 记录小米笔记本中处理
lspci -n -s 02:00.0
```txt
02:00.0 0108: 1cc1:0021 (rev 01)
```

echo 0000:02:00.0 > /sys/bus/pci/devices/0000:02:00.0/driver/unbind
echo 1cc1 0021 > /sys/bus/pci/drivers/vfio-pci/new_id

## 如何理解 kvm_device_ioctl

只有 kvm_vfio_ops 被 kvm_device_ioctl 使用过:
```c
static struct kvm_device_ops kvm_vfio_ops = {
	.name = "kvm-vfio",
	.create = kvm_vfio_create,
	.destroy = kvm_vfio_destroy,
	.set_attr = kvm_vfio_set_attr,
	.has_attr = kvm_vfio_has_attr,
};
```


## 需要被 ebpf trace 的东西
- `vfio_pci_mmap_fault`


当打开 vifo 选项，重新编译的内容:
```txt
CC      arch/x86/kvm/x86.o
CC      drivers/virtio/virtio_pci_common.o
CC      drivers/vfio/vfio.o
CC      drivers/vfio/vfio_iommu_type1.o
CC      drivers/pci/msi/msi.o
```

```txt
  CC      drivers/vfio/vfio.o
  CC      drivers/vfio/vfio_iommu_type1.o
  CC      drivers/vfio/virqfd.o
  CC      drivers/vfio/pci/vfio_pci_core.o
  CC      drivers/vfio/pci/vfio_pci_intrs.o
  CC      drivers/vfio/pci/vfio_pci_rdwr.o
  CC      drivers/vfio/pci/vfio_pci_config.o
  CC      drivers/vfio/pci/vfio_pci.o
  CC      drivers/vfio/pci/vfio_pci_igd.o
```


```c
// - [ ] 其中的 probe 函数是如何被联系上的；我感觉就是普通的 pci driver 就可以 bind 上的。
static struct pci_driver vfio_pci_driver = {
    .name           = "vfio-pci",
    .id_table       = vfio_pci_table,
    .probe          = vfio_pci_probe,
    .remove         = vfio_pci_remove,
    .sriov_configure    = vfio_pci_sriov_configure,
    .err_handler        = &vfio_pci_core_err_handlers,
    .driver_managed_dma = true,
};

static const struct file_operations vfio_device_fops = {

 // - [ ] 简单的跟踪一下，发现 vfio_pci_core_read 最后就是对于设备的 PCI 配置空间读写，将这个 backtrace 在小米上用 bpf 打印一下
static const struct vfio_device_ops vfio_pci_ops = {
    .name       = "vfio-pci",
    .open_device    = vfio_pci_open_device,
    .close_device   = vfio_pci_core_close_device,
    .ioctl      = vfio_pci_core_ioctl,
    .device_feature = vfio_pci_core_ioctl_feature,
    .read       = vfio_pci_core_read,
    .write      = vfio_pci_core_write,
    .mmap       = vfio_pci_core_mmap,
    .request    = vfio_pci_core_request,
    .match      = vfio_pci_core_match,
};

// - [ ] 下面两个 vfio fops 的关系是什么?
static const struct file_operations vfio_group_fops = {
    .owner      = THIS_MODULE,
    .unlocked_ioctl = vfio_group_fops_unl_ioctl,
    .compat_ioctl   = compat_ptr_ioctl,
    .open       = vfio_group_fops_open,
    .release    = vfio_group_fops_release,
};


static const struct file_operations vfio_fops = {
    .owner      = THIS_MODULE,
    .open       = vfio_fops_open,
    .release    = vfio_fops_release,
    .unlocked_ioctl = vfio_fops_unl_ioctl,
    .compat_ioctl   = compat_ptr_ioctl,
};

static const struct file_operations vfio_device_fops = {
    .owner      = THIS_MODULE,
    .release    = vfio_device_fops_release,
    .read       = vfio_device_fops_read,
    .write      = vfio_device_fops_write,
    .unlocked_ioctl = vfio_device_fops_unl_ioctl,
    .compat_ioctl   = compat_ptr_ioctl,
    .mmap       = vfio_device_fops_mmap,
};
```

## `vfio_pci_igd.c`

只是为了特殊支持 VFIO PCI Intel Graphics

## `vfio_pci_rdwr.c`
- [ ] 如何理解其中的 eventfd 的

## `vfio_pci_intrs.c`

- `vfio_intx_set_signal` : 注册中断

## `vfio_pci_core.c`

如何理解?
```c
static const struct vm_operations_struct vfio_pci_mmap_ops = {
    .open = vfio_pci_mmap_open,
    .close = vfio_pci_mmap_close,
    .fault = vfio_pci_mmap_fault,
};
```
这是 `vfio_pci_ops::mmap` 注册的时候需要使用的 hook :


## `vfio_pic.c`
在此处注册了一个 pci 驱动: `vfio_pci_driver`


应该查看一下，下面还有什么 device 的。
```sh
echo 10de 1d12 > /sys/bus/pci/drivers/vfio-pci/new_id
```

## vfio.c
- [ ] group 是什么概念
- [ ] 这个操作是?
- [ ] `vfio_fops`



 - `vfio_init` ：调用 chadev 的标准接口注册 IO
    - `misc_register` ： 注册 `vfio_fops`
    - `alloc_chrdev_region`


- `vfio_fops_unl_ioctl`
    - 其他的几个功能都不重要的
    - `vfio_group_get_device_fd`
        - `vfio_device_open` ：注册上 `vfio_device_fops`
    - `vfio_ioctl_set_iommu`
        - 对于所有的 `vfio.iommu_drivers_list` 依次尝试:
            - `driver->ops->open(arg);`
            - `__vfio_container_attach_groups`
                - `driver->ops->attach_group`

- `vfio_group_fops_unl_ioctl`
    - `VFIO_GROUP_GET_DEVICE_FD` ： `vfio_group_get_device_fd`
        - `vfio_device_open`
            - `vfio_device_get_from_name`
                - 对于 `vfio_group::device_list` 进行遍历
                - 调用 hook : `vfio_pci_core_match` 将参数对比，而 buf 是就是 /sys/bus/pci/devices/ 下的 bfd
            - `vfio_device_assign_container`

- `vfio_device_fops_unl_ioctl`
    - `vfio_device::vfio_device_ops::ioctl`
        - `vfio_pci_core_ioctl`


尝试跟踪一下，`vfio_device` 是如何添加到 `vfio_group` 中的:

- `vfio_register_group_dev`
    - `vfio_group_find_or_alloc` 参数为 `vfio_device::dev`
        - `iommu_group_get` ：**真难啊**, 通过 kobject 来获取
        - `vfio_group_get_from_iommu` ：通过
    - `__vfio_register_dev`


## `vfio_iommu_type1`

暂时，不看，但是 vfio.c 中的内容是需要使用其中的内容的:

```c
static const struct vfio_iommu_driver_ops vfio_iommu_driver_ops_type1 = {
    .name           = "vfio-iommu-type1",
    .owner          = THIS_MODULE,
    .open           = vfio_iommu_type1_open,
    .release        = vfio_iommu_type1_release,
    .ioctl          = vfio_iommu_type1_ioctl,
    .attach_group       = vfio_iommu_type1_attach_group, // 深渊
    .detach_group       = vfio_iommu_type1_detach_group,
    .pin_pages      = vfio_iommu_type1_pin_pages,
    .unpin_pages        = vfio_iommu_type1_unpin_pages,
    .register_notifier  = vfio_iommu_type1_register_notifier,
    .unregister_notifier    = vfio_iommu_type1_unregister_notifier,
    .dma_rw         = vfio_iommu_type1_dma_rw,
    .group_iommu_domain = vfio_iommu_type1_group_iommu_domain,
    .notify         = vfio_iommu_type1_notify,
};
```

- [ ] `vfio_iommu_type1_group_iommu_domain`
- [ ] `vfio_iommu_type1_pin_pages`

也就是通过
- `vfio_group_fops_unl_ioctl`
    - `vfio_group_set_container` ： 两个参数 group 和 `containter_fd`，其中 `containter_fd` 是用户传递的
        - `vfio_iommu_type1_attach_group`

## 其他
- https://github.com/gnif/vendor-reset
- https://github.com/nutanix/libvfio-user : nutanix 的这个是做个啥的?

## 什么是 virt/kvm/vfio.c

# QEMU vfio
secure userspace driver framework


IOMMU API(type 1)
## TODO
- [ ] 嵌套虚拟化中，如何处理 iommu
- [ ] vfio-mdev
- [ ] SR-IOV
- [ ] 中断如何注入到 Guest 中
    - eventfd / irqfd
- [ ] Guest 使用 DMA 的时候，提前需要将物理内存准备好?
    - 提前准备?
    - 否则，Guest 发起 DMA 操作的行为无法被捕获，所以物理设备发起 DMA 操作的时候，从 GPA 到 HVA 的映射来靠 ept 才可以?
- [ ] 访问 PCI bar 的行为是如何的?
    - QEMU -> kvm  : region
    - VFIO 提供接口
- [ ] 一共只有一个 container 吧

## 结构体
- `VFIO_DEVICE_GET_INFO` : 可以获取 `struct vfio_device_info`


## overview
> Let's start with a device[^1]
> - How does a driver program a device ?
> - How does a device signal the driver ?
> - How does a device transfer data ?

And, this page will contains anything related device except pcie, mmio, pio, interupt and dma.

- [ ] maybe tear this page into device model and concrete device

- [ ] 其中还提到了 VT-d 和 apic virtualization 辅助 VFIO，思考一下，如何使用?
- [ ] memory pin 之类的操作，不是特别相信，似乎 mmu notifier 不能用吗?


## vfio 基础知识
https://www.kernel.org/doc/html/latest/driver-api/vfio.html
https://www.kernel.org/doc/html/latest/driver-api/vfio-mediated-device.html
https://www.kernel.org/doc/html/latest/driver-api/vfio.html

https://zhuanlan.zhihu.com/p/27026590

> `vfio_container` 是访问的上下文，`vfio_group` 是 vfio 对 `iommu_group` 的表述。
>
> ![](https://pic2.zhimg.com/80/v2-bc6cabfb711139f884b1e7c596bdb051_720w.jpg)

- [ ] 使用的时候 vfio 为什么需要和驱动解绑， 因为需要绑定到 vfio-pci 上
    - [ ] vfio-pci 为什么保证覆盖原来的 bind 的驱动的功能
    - [ ] /sys/bus/pci/drivers/vfio-pci 和 /dev/vfio 的关系是什么 ?

- [ ] vfio 使用何种方式依赖于 iommu 驱动 和 pci

- [ ]  据说 iommu 可以软件实现，从 make meueconfig 中间的说法
- [ ] ioctl : get irq info / get

## kvmtool/include/linux/vfio.h
- [ ] software protocol version, or because using different hareware ?
```c
#define VFIO_TYPE1_IOMMU        1
#define VFIO_SPAPR_TCE_IOMMU        2
#define VFIO_TYPE1v2_IOMMU      3
```
- [ ] `vfio_info_cap_header`

## group and container

## uio
- location : linux/drivers/uio/uio.c

- [ ] VFIO is essential for `uio`  ?

## TODO
- [ ] 如何启动已经安装在硬盘上的 windows


## vfio
- [ ] `device__register` is a magic, I believe any device register here will be probe by kernel
  - [ ] so, I can provide a fake device driver
    - [ ] provide a tutorial for beginner to learn device model

## ccw
- https://www.kernel.org/doc/html/latest/s390/vfio-ccw.html
- https://www.ibm.com/support/knowledgecenter/en/linuxonibm/com.ibm.linux.z.lkdd/lkdd_c_ccwdd.html

[^1]: http://www.linux-kvm.org/images/5/54/01x04-Alex_Williamson-An_Introduction_to_PCI_Device_Assignment_with_VFIO.pdf
[^2]: https://www.kernel.org/doc/html/latest/driver-api/uio-howto.html
[^3]: [populate the empty /sys/kernel/iommu_groups](https://unix.stackexchange.com/questions/595353/vt-d-support-enabled-but-iommu-groups-are-missing)

## common.c

## pci.c

1. 处理 read

```c
static const MemoryRegionOps vfio_rom_ops = {
    .read = vfio_rom_read,
    .write = vfio_rom_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};
```

- `vfio_bar_register`
    - `vfio_region_mmap`
        - `mmap`


`VFIORegion` 会持有一个 `VFIODevice`，而 VFIODevice 持有一个 fd

ccw 可以使用这个进行 ioctl ，和 io

除非这个就是一个 bar 指向的空间

- `vfio_get_device`
    - 通过 VFIOGroup::fd 调用 `VFIO_GROUP_GET_DEVICE_FD`

## platform.c

## 处理下这个问题
```txt
VFIO_MAP_DMA failed: Cannot allocate memory
```

## iommu group ?


## 观测下 QEMU


### 中断注入 ?
```txt
#0  vfio_intx_eoi (vbasedev=0x5555577baa10) at ../hw/vfio/pci.c:106
#1  vfio_intx_update (vdev=vdev@entry=0x5555577b9ff0, route=route@entry=0x7fffe28fe380) at ../hw/vfio/pci.c:233
#2  0x0000555555af8277 in vfio_intx_routing_notifier (pdev=<optimized out>) at ../hw/vfio/pci.c:248
#3  0x00005555559529be in pci_bus_fire_intx_routing_notifier (bus=0x555556a52bd0) at ../hw/pci/pci.c:1631
#4  0x00005555558fa297 in piix3_write_config (address=<optimized out>, val=<optimized out>, len=<optimized out>, dev=<optimized out>)
    at ../hw/isa/piix3.c:118
#5  piix3_write_config (dev=<optimized out>, address=<optimized out>, val=<optimized out>, len=<optimized out>) at ../hw/isa/piix3.c:110
#6  0x0000555555b399c0 in memory_region_write_accessor (mr=mr@entry=0x555556935230, addr=0, value=value@entry=0x7fffe28fe518, size=size@entry=1,
    shift=<optimized out>, mask=mask@entry=255, attrs=...) at ../softmmu/memory.c:493
#7  0x0000555555b371a6 in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7fffe28fe518, size=size@entry=1,
    access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0x555555b39940 <memory_region_write_accessor>, mr=0x555556935230,
    attrs=...) at ../softmmu/memory.c:555
#8  0x0000555555b3b46a in memory_region_dispatch_write (mr=mr@entry=0x555556935230, addr=0, data=<optimized out>, op=<optimized out>,
    attrs=attrs@entry=...) at ../softmmu/memory.c:1522
#9  0x0000555555b423c0 in flatview_write_continue (fv=fv@entry=0x7ffcd4260740, addr=addr@entry=3324, attrs=..., attrs@entry=...,
    ptr=ptr@entry=0x7ffff4e0b000, len=len@entry=1, addr1=<optimized out>, l=<optimized out>, mr=0x555556935230)
    at /home/martins3/core/qemu/include/qemu/host-utils.h:165
#10 0x0000555555b42680 in flatview_write (fv=0x7ffcd4260740, addr=addr@entry=3324, attrs=attrs@entry=..., buf=buf@entry=0x7ffff4e0b000,
    len=len@entry=1) at ../softmmu/physmem.c:2868
#11 0x0000555555b45a89 in address_space_write (len=1, buf=0x7ffff4e0b000, attrs=..., addr=3324, as=0x5555564c9a00 <address_space_io>)
    at ../softmmu/physmem.c:2964
#12 address_space_rw (as=0x5555564c9a00 <address_space_io>, addr=addr@entry=3324, attrs=attrs@entry=..., buf=0x7ffff4e0b000, len=len@entry=1,
    is_write=is_write@entry=true) at ../softmmu/physmem.c:2974
#13 0x0000555555b6390b in kvm_handle_io (count=1, size=1, direction=<optimized out>, data=<optimized out>, attrs=..., port=3324)
    at ../accel/kvm/kvm-all.c:2719
#14 kvm_cpu_exec (cpu=cpu@entry=0x555556826160) at ../accel/kvm/kvm-all.c:2970
#15 0x0000555555b64d9d in kvm_vcpu_thread_fn (arg=arg@entry=0x555556826160) at ../accel/kvm/kvm-accel-ops.c:51
#16 0x0000555555cdb249 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:505
#17 0x00007ffff6888e86 in start_thread () from /nix/store/9xfad3b5z4y00mzmk2wnn4900q0qmxns-glibc-2.35-224/lib/libc.so.6
#18 0x00007ffff690fd70 in clone3 () from /nix/store/9xfad3b5z4y00mzmk2wnn4900q0qmxns-glibc-2.35-224/lib/libc.so.6
```
- 为什么写 piix3 最后会到

### 是不是所有的写都需要 QEMU 抓发给 Guest ?

## 解决一个小问题
- `vfio_pin_map_dma`
  - vfio_pin_pages_remote ?
  - vfio_iommu_map

调试这个，速度太慢了!
```txt
[139455.344323] vfio_pin_pages_remote: RLIMIT_MEMLOCK (8388608) exceeded
[139455.347374] vfio_pin_pages_remote: RLIMIT_MEMLOCK (8388608) exceeded
[139508.784534] vfio_pin_pages_remote: RLIMIT_MEMLOCK (8388608) exceeded
[139508.787581] vfio_pin_pages_remote: RLIMIT_MEMLOCK (8388608) exceeded
```

## 分析下 memlock 为什么是 GPU 需要的

kvm notifier 和这个是什么关系？

应该是 IOMMNU

## iommu=pt


## /dev/vfio/10 是做什么的

## echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id 的行为是什么
这是 generic 的
https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-pci

```c
static struct pci_driver vfio_pci_driver = {
	.name			= "vfio-pci",
	.id_table		= vfio_pci_table,
	.probe			= vfio_pci_probe,
	.remove			= vfio_pci_remove,
	.sriov_configure	= vfio_pci_sriov_configure,
	.err_handler		= &vfio_pci_core_err_handlers,
	.driver_managed_dma	= true,
};
```

分析 probe 的过程:
- vfio_pci_core_register_device
  - vfio_register_group_dev
    - `__vfio_register_dev`

## vfio_pci_ioeventfd 不知道为什么，完全没有人用


## iommufd.c 中主要是做什么的？
- [ ]


## 参考
- https://kernelgo.org/vfio-insight.html
- 1 https://www.openeuler.org/zh/blog/wxggg/2020-11-29-vfio-passthrough-1.html
- 2 https://www.openeuler.org/zh/blog/wxggg/2020-11-29-vfio-passthrough-2.html
- 3 https://www.openeuler.org/zh/blog/wxggg/2020-11-21-iommu-smmu-intro.html

## iommu container

## 观察下中断如何实现的

核心数据结构: struct VFIOPCIDevice

- vfio_realize
  - vfio_bars_prepare
  - vfio_intx_enable
    - vfio_set_irq_signaling

```c
/**
 * VFIO_DEVICE_SET_IRQS - _IOW(VFIO_TYPE, VFIO_BASE + 10, struct vfio_irq_set)
 *
 * Set signaling, masking, and unmasking of interrupts.  Caller provides
 * struct vfio_irq_set with all fields set.  'start' and 'count' indicate
 * the range of subindexes being specified.
 *
 * The DATA flags specify the type of data provided.  If DATA_NONE, the
 * operation performs the specified action immediately on the specified
 * interrupt(s).  For example, to unmask AUTOMASKED interrupt [0,0]:
 * flags = (DATA_NONE|ACTION_UNMASK), index = 0, start = 0, count = 1.
 *
 * DATA_BOOL allows sparse support for the same on arrays of interrupts.
 * For example, to mask interrupts [0,1] and [0,3] (but not [0,2]):
 * flags = (DATA_BOOL|ACTION_MASK), index = 0, start = 1, count = 3,
 * data = {1,0,1}
 *
 * DATA_EVENTFD binds the specified ACTION to the provided __s32 eventfd.
 * A value of -1 can be used to either de-assign interrupts if already
 * assigned or skip un-assigned interrupts.  For example, to set an eventfd
 * to be trigger for interrupts [0,0] and [0,2]:
 * flags = (DATA_EVENTFD|ACTION_TRIGGER), index = 0, start = 0, count = 3,
 * data = {fd1, -1, fd2}
 * If index [0,1] is previously set, two count = 1 ioctls calls would be
 * required to set [0,0] and [0,2] without changing [0,1].
 *
 * Once a signaling mechanism is set, DATA_BOOL or DATA_NONE can be used
 * with ACTION_TRIGGER to perform kernel level interrupt loopback testing
 * from userspace (ie. simulate hardware triggering).
 *
 * Setting of an event triggering mechanism to userspace for ACTION_TRIGGER
 * enables the interrupt index for the device.  Individual subindex interrupts
 * can be disabled using the -1 value for DATA_EVENTFD or the index can be
 * disabled as a whole with: flags = (DATA_NONE|ACTION_TRIGGER), count = 0.
 *
 * Note that ACTION_[UN]MASK specify user->kernel signaling (irqfds) while
 * ACTION_TRIGGER specifies kernel->user signaling.
 */
struct vfio_irq_set {
	__u32	argsz;
	__u32	flags;
#define VFIO_IRQ_SET_DATA_NONE		(1 << 0) /* Data not present */
#define VFIO_IRQ_SET_DATA_BOOL		(1 << 1) /* Data is bool (u8) */
#define VFIO_IRQ_SET_DATA_EVENTFD	(1 << 2) /* Data is eventfd (s32) */
#define VFIO_IRQ_SET_ACTION_MASK	(1 << 3) /* Mask interrupt */
#define VFIO_IRQ_SET_ACTION_UNMASK	(1 << 4) /* Unmask interrupt */
#define VFIO_IRQ_SET_ACTION_TRIGGER	(1 << 5) /* Trigger interrupt */
	__u32	index;
	__u32	start;
	__u32	count;
	__u8	data[];
};
```

在内核这一侧:
- vfio_msi_set_vector_signal
  - request_irq
    - vfio_msihandler : 注册的代码的 hook 为 vfio_msihandler


```c
  vfio_msihandler
  __handle_irq_event_percpu
  handle_irq_event
  handle_edge_irq
  __common_interrupt
  common_interrupt
  asm_common_interrupt
```

存在两个技术，默认是 `kvm_interrupt` 的吧
```c
typedef struct VFIOMSIVector {
    /*
     * Two interrupt paths are configured per vector.  The first, is only used
     * for interrupts injected via QEMU.  This is typically the non-accel path,
     * but may also be used when we want QEMU to handle masking and pending
     * bits.  The KVM path bypasses QEMU and is therefore higher performance,
     * but requires masking at the device.  virq is used to track the MSI route
     * through KVM, thus kvm_interrupt is only available when virq is set to a
     * valid (>= 0) value.
     */
    EventNotifier interrupt;
    EventNotifier kvm_interrupt;
    struct VFIOPCIDevice *vdev; /* back pointer to device */
    int virq;
    bool use;
} VFIOMSIVector;
```

中断是 msix 的：
```txt
memory-region: pci_bridge_pci
  0000000000000000-ffffffffffffffff (prio 0, i/o): pci_bridge_pci
    00000000fe800000-00000000fe803fff (prio 1, i/o): nvme-bar0
      00000000fe800000-00000000fe801fff (prio 0, i/o): nvme
      00000000fe802000-00000000fe80240f (prio 0, i/o): msix-table
      00000000fe803000-00000000fe80300f (prio 0, i/o): msix-pba
```

## vtd_realize 为什么从来不会被调用

## 直接转发给 kvm 直接注入，不用切换到用户态来注入的
- https://stackoverflow.com/questions/29461518/interrupt-handling-for-assigned-device-through-vfio#:~:text=An%20interrupt%20from%20the%20device,QEMU)%20has%20configured%20via%20ioctl.

```txt
🧀  sudo stackcount kvm_arch_irq_bypass_add_producer

  kvm_arch_irq_bypass_add_producer
  __connect
  irq_bypass_register_producer
  vfio_msi_set_vector_signal
  vfio_msi_set_block
  vfio_pci_set_msi_trigger
  vfio_pci_core_ioctl
  vfio_device_fops_unl_ioctl
  __x64_sys_ioctl
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    44

Detaching...
```


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

[^4]: https://unix.stackexchange.com/questions/595353/vt-d-support-enabled-but-iommu-groups-are-missing
