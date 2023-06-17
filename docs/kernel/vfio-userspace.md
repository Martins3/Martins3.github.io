# vfio 用户态部分分析


<!-- vim-markdown-toc GitLab -->

* [中断特别说明](#中断特别说明)
* [container 的接口](#container-的接口)
  * [VFIO_IOMMU_MAP_DMA](#vfio_iommu_map_dma)
* [device 的接口](#device-的接口)
* [文件结构](#文件结构)

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
      - memory_listener_register
        - listener_add_address_space
          - vfio_listener_region_add
            - vfio_dma_map : [VFIO_IOMMU_MAP_DMA](#vfio_iommu_map_dma) ，通过内核的 `vfio_iommu_driver_ops_type1` 中注册的
  - vfio_get_device
    - **VFIO_GROUP_GET_DEVICE_FD** : 获取的到 fd 为在内核中为 `vfio_device_fops`，因为在内核中用的是 pci 的实现，所以操作都会调用到 `vfio_pci_ops` 上
  - vfio_bars_register
  - vfio_intx_enable
    - event_notifier_get_fd


## 中断特别说明
当时之后，会修改为 msix

```txt
#0  vfio_msix_enable (vdev=vdev@entry=0x555557bd0f60) at ../hw/vfio/pci.c:610
#1  0x0000555555bd34a0 in vfio_pci_write_config (pdev=0x555557bd0f60, addr=178, val=49155, len=2) at ../hw/vfio/pci.c:1232
#2  0x0000555555c0c320 in memory_region_write_accessor (mr=0x555556c4eda0, addr=2, value=<optimized out>, size=2, shift=<optimized out>, mask=<optimized out>, attrs=...) at ../softmmu/memory.c:493
```
vfio_msi_enable 从来没有被调用过

- vfio_msix_enable
  - msix_set_vector_notifiers : 注册 vfio_msix_vector_use

```txt
#0  vfio_msix_vector_use (pdev=0x555557bd0f60, nr=0, msg=...) at ../hw/vfio/pci.c:559
#1  0x00005555559ae942 in msix_fire_vector_notifier (is_masked=false, vector=0, dev=0x555557bd0f60) at ../hw/pci/msix.c:120
#2  msix_handle_mask_update (dev=0x555557bd0f60, vector=0, was_masked=<optimized out>) at ../hw/pci/msix.c:140
#3  0x0000555555c0c320 in memory_region_write_accessor (mr=0x555557bd15a0, addr=12, value=<optimized out>, size=4, shift=<optimized out>, mask=<optimized out>, attrs=...) at ../softmmu/memory.c:493
```
- vfio_msix_vector_use
  - qemu_set_fd_handler : 给 eventfd 注册上 vfio_msi_interrupt

- [ ] 但是 vfio_msi_interrupt 一次没有被调用过

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
