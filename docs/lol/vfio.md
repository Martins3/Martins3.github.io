# VFIO

通过 VFIO 可以将一个物理设备直接被 Guest 使用。

## 上手操作
参考[内核文档](https://www.kernel.org/doc/html/latest/driver-api/vfio.html)，我这里记录一下操作:

看看这个文章: https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm
这个教程也不错：https://github.com/bryansteiner/gpu-passthrough-tutorial
 -device vfio-pci,host=01:00.0,multifunction=on,x-vga=on 中的 x-vga 是什么含义？

nixos 下非常之好的讲解: https://astrid.tech/2022/09/22/0/nixos-gpu-vfio/

而修改 hard limit 的方法参考[此处](https://docs.oracle.com/cd/E19623-01/820-6168/file-descriptor-requirements.html)，有点麻烦。

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

# 具体的代码分析

- [ ] 类似 RISC-V 中存在用户态中断，那么是不是可以设计出来更加酷炫的用户态 driver 来。
- [ ] 内核文档 iommu.rst 中的
- [ ] 如何理解 container 中的内容: `vfio_group_set_container`
    - 所以 container 是个什么概念
- `vfio_iommu_type1_group_iommu_domain` 中的 domain 是个什么含义
- [ ] 应该是 container 中含有 group 的
- [ ] 难道一个主板上可以有多个 IOMMU，否则，为什么会存在 `iommu_group`

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

## 其他
- https://github.com/gnif/vendor-reset
- https://github.com/nutanix/libvfio-user : nutanix 的这个是做个啥的?

## 什么是 virt/kvm/vfio.c
