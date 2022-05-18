# VFIO

## 基本原理
[An Introduction with PCI Device Assignment with VFIO](http://events17.linuxfoundation.org/sites/events/files/slides/An%20Introduction%20to%20PCI%20Device%20Assignment%20with%20VFIO%20-%20Williamson%20-%202016-08-30_0.pdf)

首先分析一下 QEMU 的图形显示系统:
- https://www.kraxel.org/blog/2019/09/display-devices-in-qemu/#tldr
  - **这篇文章非常重要** 只是现在实在是没有 disk 内存空间去测试 ubuntu 了
  - 可以用于了解图形系统的栈是怎么搞的

## 直通 GPU

顺便直通一下固态硬盘
- https://www.baeldung.com/linux/check-monitor-active-gpu

## vfio 基础知识
https://www.kernel.org/doc/html/latest/driver-api/vfio.html
https://www.kernel.org/doc/html/latest/driver-api/vfio-mediated-device.html
https://www.kernel.org/doc/html/latest/driver-api/vfio.html
https://zhuanlan.zhihu.com/p/27026590

- [x] 似乎可以用于 GPU 虚拟化 (YES)

- [ ] 使用的时候 vfio 为什么需要和驱动解绑， 因为需要绑定到 vfio-pci 上
    - [ ] vfio-pci 为什么保证覆盖原来的 bind 的驱动的功能
    - [ ] /sys/bus/pci/drivers/vfio-pci 和 /dev/vfio 的关系是什么 ?

- [ ] vfio 使用何种方式依赖于 iommu 驱动 和 pci

- [ ]  据说 iommu 可以软件实现，从 make meueconfig 中间的说法

- [ ] config `KVM_VFIO`, config VFIO in Kconfig, what's the relation ?

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

## how to use vfio
before assigning the device to VM, we need to unbind its original driver and bind it to vfio-pci driver firstly. [^4][^5]

- [ ] if we **unbind** the server firstly, so host will not work normally because of lost it's driver
  - [ ] give it a try, afterall, reboot is the worst case

[^1]: http://www.linux-kvm.org/images/5/54/01x04-Alex_Williamson-An_Introduction_to_PCI_Device_Assignment_with_VFIO.pdf
[^2]: https://www.kernel.org/doc/html/latest/driver-api/uio-howto.html
[^3]: [populate the empty /sys/kernel/iommu_groups](https://unix.stackexchange.com/questions/595353/vt-d-support-enabled-but-iommu-groups-are-missing)
[^4]: https://www.reddit.com/r/VFIO/comments/cbkg6j/devvfioxx_not_being_created/
[^5]: https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/08/16/vfio-usage
