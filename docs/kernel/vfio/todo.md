## 被物理机使用的 GPU 居然可以直接被拔掉直通给虚拟机
```txt
[  180.508065] Console: switching to colour dummy device 80x25
[  180.523120] vfio-pci 0000:01:00.0: vgaarb: deactivate vga console
[  180.523126] vfio-pci 0000:01:00.0: vgaarb: VGA decodes changed: olddecodes=io+mem,decodes=io+mem:owns=none
[  180.782300] rfkill: input handler enabled
```
是可以直接拔掉的

## 关键参考

- https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF#Passing_through_a_device_that_does_not_support_resetting
- https://superuser.com/questions/1293112/kvm-gpu-passthrough-of-nvidia-dgpu-on-laptop-with-hybrid-graphics-without-propri
- https://github.com/jscinoz/optimus-vfio-docs
- https://github.com/gnif/vendor-reset
- https://arccompute.com/blog/libvfio-add-gvm-support/
- https://clayfreeman.github.io/gpu-passthrough/#imaging-the-gpu-rom
- https://open-iov.org/index.php/Virtual_I/O_Internals#SR-IOV_Mode_Requirements
- https://astrid.tech/2022/09/22/0/nixos-gpu-vfio/ : 分析 nixos 如何
- https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm
- https://github.com/bryansteiner/gpu-passthrough-tutorial
- https://docs.oracle.com/cd/E19623-01/820-6168/file-descriptor-requirements.html

## 确认一下，这个是不是 vfio 的 bug

如果一个虚拟机启动之后，正在使用这个设备做直通，这个时候，另外一个
thread 来尝试 unbind ，那么该 thread 就是会卡住的

```txt
 sudo cat /proc/160437/stack

[<0>] vfio_unregister_group_dev+0x94/0x140 [vfio]
[<0>] vfio_pci_core_unregister_device+0x2c/0x108 [vfio_pci_core]
[<0>] vfio_pci_remove+0x24/0x48 [vfio_pci]
[<0>] pci_device_remove+0x4c/0x100
[<0>] device_remove+0x54/0x90
[<0>] device_release_driver_internal+0x1d4/0x240
[<0>] device_driver_detach+0x20/0x40
[<0>] unbind_store+0xbc/0xd0
[<0>] drv_attr_store+0x2c/0x48
[<0>] sysfs_kf_write+0x60/0x80
[<0>] kernfs_fop_write_iter+0x140/0x1f8
[<0>] vfs_write+0x234/0x3a0
[<0>] ksys_write+0x74/0x120
[<0>] __arm64_sys_write+0x24/0x40
[<0>] invoke_syscall+0x50/0x120
[<0>] el0_svc_common.constprop.0+0x48/0xf0
[<0>] do_el0_svc+0x24/0x38
[<0>] el0_svc+0x34/0x120
[<0>] el0t_64_sync_handler+0x10c/0x138
[<0>] el0t_64_sync+0x190/0x198
```

当时已经运行的 qemu 会有这个报错

```txt
warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
qemu-system-aarch64: warning: vfio 0000:82:02.0: Bus 'pcie.0' does not support hotplugging
```


## 我真的想要知道的两个问题
2. mdev 工作原理
	- nv-grace 是什么情况?
3. 这几个模块的作用是什么?
```txt
vfio_iommu_type1
vfio_pci_core/ : 这个作用是什么?
vfio/
vfio_pci/
```

4. 既然 mdev 也是定义这个，那么说明了什么?
```c
static const struct vfio_device_ops vfio_pci_ops = {
	.name		= "vfio-pci",
	.init		= vfio_pci_core_init_dev,
	.release	= vfio_pci_core_release_dev,
	.open_device	= vfio_pci_open_device,
	.close_device	= vfio_pci_core_close_device,
	.ioctl		= vfio_pci_core_ioctl,
	.device_feature = vfio_pci_core_ioctl_feature,
	.read		= vfio_pci_core_read,
	.write		= vfio_pci_core_write,
	.mmap		= vfio_pci_core_mmap,
	.request	= vfio_pci_core_request,
	.match		= vfio_pci_core_match,
	.bind_iommufd	= vfio_iommufd_physical_bind,
	.unbind_iommufd	= vfio_iommufd_physical_unbind,
	.attach_ioas	= vfio_iommufd_physical_attach_ioas,
};
```

5. 这个说啥呢?
`vfio_pci_ops` 和 qemu 外层的函数对应，这里只是处理 pci 相关的情况，出现在 drivers/vfio/pci/vfio_pci_core.c

## 为什么需要这些日志
```txt
# TODO 奇怪的选项
CONFIG_VFIO_PCI_VGA=y
CONFIG_VFIO_PCI_IGD=y
```
配合:
docs/kernel/vfio/vGPU.md
docs/kernel/vfio/virtio.md

他们和 mdev 的关系是什么?

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
