# vfio 用户态部分分析


<!-- vim-markdown-toc GitLab -->

  * [container 的接口](#container-的接口)
    * [VFIO_IOMMU_MAP_DMA](#vfio_iommu_map_dma)
  * [device 的接口](#device-的接口)
  * [文件结构](#文件结构)
* [等待整理](#等待整理)
  * [TODO](#todo)
  * [overview](#overview)
  * [vfio 基础知识](#vfio-基础知识)
  * [kvmtool/include/linux/vfio.h](#kvmtoolincludelinuxvfioh)
  * [group and container](#group-and-container)
  * [uio](#uio)
  * [TODO](#todo-1)
  * [vfio](#vfio)
  * [ccw](#ccw)
  * [common.c](#commonc)
  * [pci.c](#pcic)
    * [中断注入 ?](#中断注入-)
    * [是不是所有的写都需要 QEMU 抓发给 Guest ?](#是不是所有的写都需要-qemu-抓发给-guest-)
  * [解决一个小问题](#解决一个小问题)
  * [分析下 memlock 为什么是 GPU 需要的](#分析下-memlock-为什么是-gpu-需要的)
  * [/dev/vfio/10 是做什么的](#devvfio10-是做什么的)
  * [echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id 的行为是什么](#echo-1e49-0071-sudo-tee-sysbuspcidriversvfio-pcinew_id-的行为是什么)
  * [vfio_pci_ioeventfd 不知道为什么，完全没有人用](#vfio_pci_ioeventfd-不知道为什么完全没有人用)
  * [iommufd.c 中主要是做什么的？](#iommufdc-中主要是做什么的)
  * [参考](#参考)
  * [iommu container](#iommu-container)
  * [观察下中断如何实现的](#观察下中断如何实现的)
  * [vtd_realize 为什么从来不会被调用](#vtd_realize-为什么从来不会被调用)
  * [直接转发给 kvm 直接注入，不用切换到用户态来注入的](#直接转发给-kvm-直接注入不用切换到用户态来注入的)
  * [intel-iommu 如何设置](#intel-iommu-如何设置)
  * [分析下 DMA 映射的规则](#分析下-dma-映射的规则)
  * [VFIO User - Using VFIO as the IPC Protocol in Multi-process QEMU - John Johnson & Jagannathan Raman](#vfio-user-using-vfio-as-the-ipc-protocol-in-multi-process-qemu-john-johnson-jagannathan-raman)
  * [An Introduction to IOMMU Infrastructure in the Linux Kernel](#an-introduction-to-iommu-infrastructure-in-the-linux-kernel)
  * [操作的内容](#操作的内容)
  * [http://xillybus.com/tutorials/iommu-swiotlb-linux](#httpxillybuscomtutorialsiommu-swiotlb-linux)

<!-- vim-markdown-toc -->

使用 QEMU 作为，这是一切的入口。

```txt
#6  0x0000555555bd5219 in vfio_realize (pdev=0x555557bd0f60, errp=0x7ffffffef230) at ../hw/vfio/pci.c:2959
#7  0x00005555559b49e7 in pci_qdev_realize (qdev=<optimized out>, errp=<optimized out>) at ../hw/pci/pci.c:2115
#8  0x0000555555c74d1b in device_set_realized (obj=<optimized out>, value=<optimized out>, errp=0x7ffffffef460) at ../hw/core/qdev.c:510
#9  0x0000555555c78ba8 in property_set_bool (obj=0x555557bd0f60, v=<optimized out>, name=<optimized out>, opaque=0x5555567c8370, errp=0x7ffffffef460) at ../qom/object.c:2285
#10 0x0000555555c7bc47 in object_property_set (obj=obj@entry=0x555557bd0f60, name=name@entry=0x555555f5e0c7 "realized", v=v@entry=0x555557bd3030, errp=errp@entry=0x7ffffffef460) at ../qom/object.c:1420
#11 0x0000555555c7efaf in object_property_set_qobject (obj=obj@entry=0x555557bd0f60, name=name@entry=0x555555f5e0c7 "realized", value=value@entry=0x555557bd2e00, errp=errp@entry=0x7ffffffef460) at ../qom/qom-qobject.c:28
#12 0x0000555555c7c264 in object_property_set_bool (obj=obj@entry=0x555557bd0f60, name=name@entry=0x555555f5e0c7 "realized", value=value@entry=true, errp=errp@entry=0x7ffffffef460) at ../qom/object.c:1489
#13 0x0000555555c757bc in qdev_realize (dev=dev@entry=0x555557bd0f60, bus=bus@entry=0x555556d50c60, errp=errp@entry=0x7ffffffef460) at ../hw/core/qdev.c:292
#14 0x0000555555a68553 in qdev_device_add_from_qdict (opts=opts@entry=0x555557869e20, from_json=from_json@entry=false, errp=0x7ffffffef460, errp@entry=0x555556737338 <error_fatal>) at ../softmmu/qdev-monitor.c:714
#15 0x0000555555a689b1 in qdev_device_add (opts=0x5555567c14b0, errp=errp@entry=0x555556737338 <error_fatal>) at ../softmmu/qdev-monitor.c:733
#16 0x0000555555a6d50f in device_init_func (opaque=<optimized out>, opts=<optimized out>, errp=0x555556737338 <error_fatal>) at ../softmmu/vl.c:1152
#17 0x0000555555df8121 in qemu_opts_foreach (list=<optimized out>, func=func@entry=0x555555a6d500 <device_init_func>, opaque=opaque@entry=0x0, errp=errp@entry=0x555556737338 <error_fatal>) at ../util/qemu-option.c:1135
#18 0x0000555555a6fc7a in qemu_create_cli_devices () at ../softmmu/vl.c:2573
#19 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2641
#20 0x0000555555a7369e in qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2635
#21 qemu_init (argc=<optimized out>, argv=<optimized out>) at ../softmmu/vl.c:3648
#22 0x000055555586ad79 in main (argc=<optimized out>, argv=<optimized out>) at ../softmmu/main.c:47
```

几乎没有什么代码处理 iova，都是处理中断相关的代码

- vfio_realize
  - VFIODevice::ops 注册为 vfio_pci_ops
  - 通过 host 的 bdf 可以找到 group 路径
    - 传递的参数是 0000:07:00.0
    - 找到设备所在的 /sys/bus/pci/devices/0000:07:00.0/iommu_group
    - 其软连接指向 : ../../../../kernel/iommu_groups/2
    - 好的，现在知道 group id 是 2
  - vfio_get_group
    - 打开 /dev/vfio/2 放到 VFIOGroup::fd 中
    - vfio_connect_container
      - 打开 /dev/vfio/vfio 放到 VFIOContainer::fd 中
      - vfio_init_container
        - vfio_get_iommu_type
          - 通过 **VFIO_CHECK_EXTENSION** 获取到 iommu 的类型，在 AMD 上测试得到是 VFIO_TYPE1v2_IOMMU
        - **VFIO_GROUP_SET_CONTAINER** : 将 group 和 container 联系起来，也就是 /dev/vfio/2 和 /dev/vfio/vfio 联系起来
        - **VFIO_SET_IOMMU** : 设置 iommu 的类型，其实最后就是用内核中 `vfio_iommu_driver_ops_type1`
      - memory_listener_register
        - listener_add_address_space
          - vfio_listener_region_add
            - vfio_dma_map : [VFIO_IOMMU_MAP_DMA](#vfio_iommu_map_dma) ，通过内核的 `vfio_iommu_driver_ops_type1` 中注册的
  - vfio_get_device
    - **VFIO_GROUP_GET_DEVICE_FD** : 获取的到 fd 为在内核中为 `vfio_device_fops`，因为在内核中用的是 pci 的实现，所以操作都会调用到 `vfio_pci_ops` 上
    - **VFIO_DEVICE_GET_INFO** : 关于设备的基本信息
  - vfio_bars_register
    - vfio_bar_register
      - vfio_region_mmap : 映射 mmap
  - vfio_intx_enable : msi 和 msix 差不多的调用流程，但是不是在这里调用的
    - event_notifier_get_fd
    - vfio_set_irq_signaling
      - **VFIO_DEVICE_SET_IRQS** : 注册 irqfd


## container 的接口

合格比较难触发，和热迁移相关的:
- vfio_get_dirty_bitmap
  - vfio_query_dirty_bitmap

### VFIO_IOMMU_MAP_DMA
- VFIO_GROUP_GET_DEVICE_FD

在  vfio_dma_map 中增加
```c
    printf("[huxueshi:%s:%d] %lx %lx\n", __FUNCTION__, __LINE__, iova, size);
```

启动的是:
```txt
[huxueshi:vfio_dma_map:671] 0 a0000
[huxueshi:vfio_dma_map:671] c0000 20000
[huxueshi:vfio_dma_map:671] e0000 20000
[huxueshi:vfio_dma_map:671] 100000 bff00000
[huxueshi:vfio_dma_map:671] fffc0000 40000
[huxueshi:vfio_dma_map:671] 100000000 740000000 # 启动的 32G 的空间全部注册上
[huxueshi:vfio_dma_map:671] c0000 10000
[huxueshi:vfio_dma_map:671] d0000 10000
[huxueshi:vfio_dma_map:671] e0000 10000
[huxueshi:vfio_dma_map:671] f0000 bff10000
[huxueshi:vfio_dma_map:671] c0000 bff40000
[huxueshi:vfio_dma_map:671] fd000000 1000000
[huxueshi:vfio_dma_map:671] fea51000 3000
[huxueshi:vfio_dma_map:671] fea40000 10000
[huxueshi:vfio_dma_map:671] fea00000 40000
[huxueshi:vfio_dma_map:671] c0000 b000
[huxueshi:vfio_dma_map:671] cb000 3000
[huxueshi:vfio_dma_map:671] ce000 2000
[huxueshi:vfio_dma_map:671] d0000 20000
[huxueshi:vfio_dma_map:671] f0000 10000
[huxueshi:vfio_dma_map:671] 100000 bff00000
[huxueshi:vfio_dma_map:671] ce000 1a000
[huxueshi:vfio_dma_map:671] e8000 8000
```

Guest 启动之后，似乎发生了重新映射:
```txt
[    0.369150] PCI host bridge to bus 0000:00
[    0.370060] pci_bus 0000:00: root bus resource [io  0x0000-0x0cf7 window]
[    0.371060] pci_bus 0000:00: root bus resource [io  0x0d00-0xffff window]
[    0.372060] pci_bus 0000:00: root bus resource [mem 0x000a0000-0x000bffff window]
[    0.373061] pci_bus 0000:00: root bus resource [mem 0x40000000-0xfebfffff window]
[    0.374060] pci_bus 0000:00: root bus resource [mem 0x100000000-0x17fffffff window]
[    0.375060] pci_bus 0000:00: root bus resource [bus 00-ff]
[    0.376154] pci 0000:00:00.0: [8086:1237] type 00 class 0x060000
[    0.378143] pci 0000:00:01.0: [8086:7000] type 00 class 0x060100
[    0.381087] pci 0000:00:01.1: [8086:7010] type 00 class 0x010180
[    0.387197] pci 0000:00:01.1: reg 0x20: [io  0xd300-0xd30f]
[    0.390166] pci 0000:00:01.1: legacy IDE quirk: reg 0x10: [io  0x01f0-0x01f7]
[    0.391060] pci 0000:00:01.1: legacy IDE quirk: reg 0x14: [io  0x03f6]
[    0.392060] pci 0000:00:01.1: legacy IDE quirk: reg 0x18: [io  0x0170-0x0177]
[    0.393060] pci 0000:00:01.1: legacy IDE quirk: reg 0x1c: [io  0x0376]
[    0.394160] pci 0000:00:01.3: [8086:7113] type 00 class 0x068000
[    0.396166] pci 0000:00:01.3: quirk: [io  0x0600-0x063f] claimed by PIIX4 ACPI
[    0.397065] pci 0000:00:01.3: quirk: [io  0x0700-0x070f] claimed by PIIX4 SMB
[    0.398172] pci 0000:00:02.0: [1234:1111] type 00 class 0x030000
[huxueshi:vfio_dma_map:671] fd000000 1000000
[    0.401060] pci 0000:00:02.0: reg 0x10: [mem 0xfd000000-0xfdffffff pref]
[huxueshi:vfio_dma_map:671] fd000000 1000000
[huxueshi:vfio_dma_map:671] fd000000 1000000
[    0.406061] pci 0000:00:02.0: reg 0x18: [mem 0xfea58000-0xfea58fff]
[huxueshi:vfio_dma_map:671] fd000000 1000000
[huxueshi:vfio_dma_map:671] fd000000 1000000
[huxueshi:vfio_dma_map:671] fd000000 1000000
[huxueshi:vfio_dma_map:671] fd000000 1000000
[    0.415061] pci 0000:00:02.0: reg 0x30: [mem 0xfea40000-0xfea4ffff pref]
[    0.416088] pci 0000:00:02.0: Video device with shadowed ROM at [mem 0x000c0000-0x000dffff]
[    0.418061] pci 0000:00:03.0: [1af4:1001] type 00 class 0x010000
[    0.421061] pci 0000:00:03.0: reg 0x10: [io  0xd100-0xd17f]
[    0.424061] pci 0000:00:03.0: reg 0x14: [mem 0xfea59000-0xfea59fff]
[    0.432060] pci 0000:00:03.0: reg 0x20: [mem 0xfe200000-0xfe203fff 64bit pref]
[    0.438079] pci 0000:00:04.0: [1b36:0001] type 01 class 0x060400
[    0.442060] pci 0000:00:04.0: reg 0x10: [mem 0xfea5a000-0xfea5a0ff 64bit]
[    0.448106] pci 0000:00:05.0: [1af4:1000] type 00 class 0x020000
[    0.451060] pci 0000:00:05.0: reg 0x10: [io  0xd2c0-0xd2df]
[    0.454059] pci 0000:00:05.0: reg 0x14: [mem 0xfea5b000-0xfea5bfff]
[    0.461061] pci 0000:00:05.0: reg 0x20: [mem 0xfe204000-0xfe207fff 64bit pref]
[    0.464061] pci 0000:00:05.0: reg 0x30: [mem 0xfea00000-0xfea3ffff pref]
[    0.468180] pci 0000:00:06.0: [10ec:8168] type 00 class 0x020000
[huxueshi:vfio_dma_map:671] fea51000 3000
[    0.475064] pci 0000:00:06.0: reg 0x10: [io  0xd000-0xd0ff]
[huxueshi:vfio_dma_map:671] fea51000 3000
[huxueshi:vfio_dma_map:671] fea51000 3000
[    0.481063] pci 0000:00:06.0: reg 0x18: [mem 0xfea5c000-0xfea5cfff 64bit]
[huxueshi:vfio_dma_map:671] fea51000 3000
[    0.485063] pci 0000:00:06.0: reg 0x20: [mem 0xfea50000-0xfea53fff 64bit]
[huxueshi:vfio_dma_map:671] fea51000 3000
[    0.490210] pci 0000:00:06.0: supports D1 D2
[    0.494065] pci 0000:00:07.0: [1af4:1009] type 00 class 0x000200
[    0.497061] pci 0000:00:07.0: reg 0x10: [io  0xd2e0-0xd2ff]
[    0.500061] pci 0000:00:07.0: reg 0x14: [mem 0xfea5d000-0xfea5dfff]
[    0.507061] pci 0000:00:07.0: reg 0x20: [mem 0xfe208000-0xfe20bfff 64bit pref]
[    0.513142] pci 0000:00:08.0: [1b36:0010] type 00 class 0x010802
[    0.515156] pci 0000:00:08.0: reg 0x10: [mem 0xfea54000-0xfea57fff 64bit]
[    0.523204] pci 0000:00:09.0: [1af4:1001] type 00 class 0x010000
[    0.526061] pci 0000:00:09.0: reg 0x10: [io  0xd180-0xd1ff]
[    0.529061] pci 0000:00:09.0: reg 0x14: [mem 0xfea5e000-0xfea5efff]
[    0.536061] pci 0000:00:09.0: reg 0x20: [mem 0xfe20c000-0xfe20ffff 64bit pref]
[    0.542117] pci 0000:00:0a.0: [1af4:1004] type 00 class 0x010000
[    0.545060] pci 0000:00:0a.0: reg 0x10: [io  0xd280-0xd2bf]
[    0.548060] pci 0000:00:0a.0: reg 0x14: [mem 0xfea5f000-0xfea5ffff]
[    0.555061] pci 0000:00:0a.0: reg 0x20: [mem 0xfe210000-0xfe213fff 64bit pref]
[    0.562081] pci 0000:00:0b.0: [1af4:1001] type 00 class 0x010000
[    0.565061] pci 0000:00:0b.0: reg 0x10: [io  0xd200-0xd27f]
[    0.568061] pci 0000:00:0b.0: reg 0x14: [mem 0xfea60000-0xfea60fff]
[    0.576061] pci 0000:00:0b.0: reg 0x20: [mem 0xfe214000-0xfe217fff 64bit pref]
[    0.597128] pci_bus 0000:01: extended config space not accessible
[    0.599065] pci 0000:01:01.0: [1b36:0010] type 00 class 0x010802
[    0.601156] pci 0000:01:01.0: reg 0x10: [mem 0xfe800000-0xfe803fff 64bit]
[    0.608108] pci 0000:01:01.0: 0.000 Gb/s available PCIe bandwidth, limited by Unknown x0 link at 0000:00:04.0 (capable of 2.000 Gb/s with 2.5 GT/s PCIe x1 link)
[    0.634107] pci 0000:00:04.0: PCI bridge to [bus 01]
[    0.635069] pci 0000:00:04.0:   bridge window [io  0xc000-0xcfff]
[    0.636068] pci 0000:00:04.0:   bridge window [mem 0xfe800000-0xfe9fffff]
[    0.637075] pci 0000:00:04.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
```
简而言之，就是使用的所有的物理映射，全部都要搞上。

## device 的接口

## 文件结构
- hw/vfio/pci.c 提供 VFIODeviceOps vfio_pci_ops，而同级目录下的 ccw.c 以及 platform.c 都是对称的关系
- util/vfio-helpers.c 的代码暂时不用看，都是 nvme 在调用

# 等待整理


- qemu/hw/vfio/pci.c

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


```txt
VFIO_MAP_DMA failed: Cannot allocate memory
```

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
- https://kernel.love/vfio-insight.html
- 1 https://www.openeuler.org/zh/blog/wxggg/2020-11-29-vfio-passthrough-1.html
- 2 https://www.openeuler.org/zh/blog/wxggg/2020-11-29-vfio-passthrough-2.html
- 3 https://www.openeuler.org/zh/blog/wxggg/2020-11-21-iommu-smmu-intro.html
- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/08/31/vfio-passthrough

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

## intel-iommu 如何设置
```plain
       -device intel-iommu[,option=...]
              This  is  only  supported by -machine q35, which will enable Intel VT-d emulation within the guest.  It sup‐
              ports below options:

              intremap=on|off (default: auto)
                     This enables interrupt remapping feature.  It's required to enable  complete  x2apic.   Currently  it
                     only  supports kvm kernel-irqchip modes off or split, while full kernel-irqchip is not yet supported.
                     The default value is "auto", which will be decided by the mode of kernel-irqchip.

              caching-mode=on|off (default: off)
                     This enables caching mode for the VT-d emulated device.  When caching-mode is enabled, each guest DMA
                     buffer  mapping  will generate an IOTLB invalidation from the guest IOMMU driver to the vIOMMU device
                     in a synchronous way.  It is required for -device vfio-pci to work with the VT-d device, because host
                     assigned devices requires to setup the DMA mapping on the host before guest DMA starts.

              device-iotlb=on|off (default: off)
                     This enables device-iotlb capability for the emulated VT-d device.  So far virtio/vhost should be the
                     only real user for this parameter, paired with ats=on configured for the device.

              aw-bits=39|48 (default: 39)
                     This decides the address width of IOVA address space.  The  address  space  has  39  bits  width  for
                     3-level IOMMU page tables, and 48 bits for 4-level IOMMU page tables.

              Please   also   refer   to   the   wiki   page   for   general   scenarios   of   VT-d  emulation  in  QEMU:
              https://wiki.qemu.org/Features/VT-d.
```

## 分析下 DMA 映射的规则

观测 QEMU 侧的: vfio_dma_map
```txt
#0  vfio_dma_map (container=container@entry=0x5555577c2090, iova=iova@entry=4294967296, size=size@entry=9663676416, vaddr=vaddr@entry=0x7ffd9be00000, readonly=false) at ../hw/vfio/common.c:496
#1  0x0000555555aeb7c1 in vfio_listener_region_add (listener=0x5555577c20a0, section=0x7fffffff0f00) at /home/martins3/core/qemu/include/qemu/int128.h:33
#2  0x0000555555b3f606 in listener_add_address_space (as=<optimized out>, listener=0x5555577c20a0) at ../softmmu/memory.c:2975
#3  memory_listener_register (listener=0x5555577c20a0, as=<optimized out>) at ../softmmu/memory.c:3045
#4  0x0000555555aed2e0 in vfio_connect_container (errp=0x7fffffff2210, as=<optimized out>, group=0x5555577c2010) at ../hw/vfio/common.c:2164
#5  vfio_get_group (groupid=17, as=<optimized out>, errp=errp@entry=0x7fffffff2210) at ../hw/vfio/common.c:2290
#6  0x0000555555afbdba in vfio_realize (pdev=0x5555577bab70, errp=0x7fffffff2210) at ../hw/vfio/pci.c:2905
```
vfio_dma_map 传递给内核， host 的虚拟地址 和 guest 的物理地址

然后在 host 中，将 host 的虚拟地址装换为物理地址，最后 host 的物理地址转换为虚拟地址。

在内核中，大致的流程为:
```txt
@[
    __domain_mapping+1
    intel_iommu_map_pages+177
    __iommu_map+211
    iommu_map+65
    vfio_iommu_type1_ioctl+1956
    __x64_sys_ioctl+135
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 641774
```

## [VFIO User - Using VFIO as the IPC Protocol in Multi-process QEMU - John Johnson & Jagannathan Raman](https://www.youtube.com/watch?v=NBT8rImx3VE)

## [An Introduction to IOMMU Infrastructure in the Linux Kernel](https://lenovopress.lenovo.com/lp1467.pdf)

## 操作的内容
1. 找到支持 srIOV 的设备
2. mddev
3. QEMU 中测试下 CONFIG_VIRTIO_IOMMU
4. iommu 的参数收集下
5. vfio 如何实现 iommu 接口的?

## http://xillybus.com/tutorials/iommu-swiotlb-linux


[^4]: https://unix.stackexchange.com/questions/595353/vt-d-support-enabled-but-iommu-groups-are-missing
