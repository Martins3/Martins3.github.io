# vfio

- [ ] 类似 RISC-V 中存在用户态中断，那么是不是可以设计出来更加酷炫的用户态 driver 来。
- [ ] 内核文档 iommu.rst 中的
- [ ] 如何理解 container 中的内容: `vfio_group_set_container`
    - 所以 container 是个什么概念
- `vfio_iommu_type1_group_iommu_domain` 中的 domain 是个什么含义
- [ ] 应该是 container 中含有 group 的
- [ ] 难道一个主板上可以有多个 IOMMU，否则，为什么会存在 `iommu_group`

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
