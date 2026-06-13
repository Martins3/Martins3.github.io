## 初始化过程

当使用 vendor 和 device id 告知进去，会自动创建 group 出来。
```sh
echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
```
- vfio_pci_probe
  - vfio_pci_core_register_device
    - __vfio_register_dev
      - vfio_device_set_group
        - vfio_group_find_or_alloc

## 基本的代码结构

```c
static struct pci_driver vfio_pci_driver = {
    .name           = "vfio-pci",
    .id_table       = vfio_pci_table,
    .probe          = vfio_pci_probe,
    .remove         = vfio_pci_remove,
    .sriov_configure    = vfio_pci_sriov_configure,
    .err_handler        = &vfio_pci_core_err_handlers,
    .driver_managed_dma = true,
};
```

最核心的数据结构:
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

static const struct vfio_iommu_driver_ops vfio_iommu_driver_ops_type1 = {
	.name			= "vfio-iommu-type1",
	.owner			= THIS_MODULE,
	.open			= vfio_iommu_type1_open,
	.release		= vfio_iommu_type1_release,
	.ioctl			= vfio_iommu_type1_ioctl,
	.attach_group		= vfio_iommu_type1_attach_group,
	.detach_group		= vfio_iommu_type1_detach_group,
	.pin_pages		= vfio_iommu_type1_pin_pages,
	.unpin_pages		= vfio_iommu_type1_unpin_pages,
	.register_device	= vfio_iommu_type1_register_device,
	.unregister_device	= vfio_iommu_type1_unregister_device,
	.dma_rw			= vfio_iommu_type1_dma_rw,
	.group_iommu_domain	= vfio_iommu_type1_group_iommu_domain,
};
```

- `vfio_pci_ops` 和 qemu 外层的函数对应，这里只是处理 pci 相关的情况，出现在 drivers/vfio/pci/vfio_pci_core.c
- `vfio_iommu_driver_ops_type1` 就是 vfio 的实现了，出现在 drivers/vfio/vfio_iommu_type1.c

对外提供的三个 fops:

| fops            | 获取 fd 的方法                |
|-----------------|-------------------------------|
| vfio_group_fops | /dev/vfio/2                   |
| vfio_fops       | /dev/vfio/vfio                |
| vfio_fops       | 通过 VFIO_GROUP_GET_DEVICE_FD |

```c
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

### QEMU 如何访问 mmio 的
QEMU 的映射函数 vfio_region_mmap

```c
const struct file_operations vfio_device_fops = {
  // ...
	.mmap		= vfio_device_fops_mmap, // 当 mmap 的时候，调用这个
};


static const struct vfio_device_ops vfio_pci_ops = {
  // ...
	.mmap		= vfio_pci_core_mmap, // 因为平台 pci ，所以调用这个
}
```

```c
const struct file_operations vfio_device_fops = {
   // ...
	.mmap		= vfio_device_fops_mmap, // page fault 从这里开始
};


static const struct vm_operations_struct vfio_pci_mmap_ops = {
   // ...
	.fault = vfio_pci_mmap_fault, // 调用到这个，这是注册的 hook
};
```
## vfio.c

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

- [ ] vfio 只是支持 type1 ?
```c
static const struct vfio_iommu_driver_ops vfio_iommu_driver_ops_type1;
```

- [ ] `vfio_iommu_type1_group_iommu_domain`
- [ ] `vfio_iommu_type1_pin_pages`

也就是通过
- `vfio_group_fops_unl_ioctl`
    - `vfio_group_set_container` ： 两个参数 group 和 `containter_fd`，其中 `containter_fd` 是用户传递的
        - `vfio_iommu_type1_attach_group`

## container 行为
- `vfio_ioctl_set_iommu`
    - 对于所有的 `vfio.iommu_drivers_list` 依次尝试，当然实际上就是 vfio_iommu_driver_ops_type1
        - `driver->ops->open(arg);` 也就是 vfio_iommu_type1_open
        - `__vfio_container_attach_groups`
            - 对于 container 中的每一个 group 调用 `driver->ops->attach_group` : 也就是 vfio_iommu_type1_attach_group
              - vfio_iommu_domain_alloc
                - bus->iommu_ops->domain_alloc(type) : 也就是 **amd_iommu_domain_alloc**
              - iommu_attach_group : 将 iommu domain 和 iommu group 联系起来
                - `__iommu_attach_group`
                  - iommu_group_do_attach_device
                    - `__iommu_attach_device` 最后调用到 **amd_iommu_attach_device**
## group 行为

### VFIO_IOMMU_MAP_DMA

- vfio_iommu_type1_map_dma
  - vfio_dma_do_map
    - vfio_find_dma
      - vfio_pin_map_dma
        - vfio_pin_pages_remote : 好家伙啊，所有的内存全部 pin 住啊
        - vfio_iommu_map
          - iommu_map : 参数是 iommu_domain
            - `__iommu_map`
              - `__iommu_map_pages`
                - iommu_domain_ops::map_pages : **amd_iommu_map_pages**
            - iommu_domain_ops::iotlb_sync_map : **amd_iommu_iotlb_sync_map**

## 一会来清理
不懂，为什么启动的时候还是 unmap
```txt
@[
    iommu_iova_to_phys+5
    vfio_unmap_unpin+258
    vfio_remove_dma+42
    vfio_iommu_type1_ioctl+2815
    __x64_sys_ioctl+145
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 76685
@[
    iommu_iova_to_phys+5
    vfio_unmap_unpin+340
    vfio_remove_dma+42
    vfio_iommu_type1_ioctl+2815
    __x64_sys_ioctl+145
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 814635
```

关闭 qemu
```txt
@[
    iommu_iova_to_phys+5
    vfio_unmap_unpin+340
    vfio_remove_dma+42
    vfio_iommu_type1_detach_group+1531
    vfio_group_detach_container+80
    vfio_group_fops_release+72
    __fput+134
    task_work_run+90
    do_exit+834
    do_group_exit+49
    get_signal+2440
    arch_do_signal_or_restart+62
    exit_to_user_mode_prepare+415
    syscall_exit_to_user_mode+27
    do_syscall_64+74
    entry_SYSCALL_64_after_hwframe+114
]: 266265
```


## 其他扩展阅读

# 具体的代码分析

- [ ] 类似 RISC-V 中存在用户态中断，那么是不是可以设计出来更加酷炫的用户态 driver 来。
- [ ] 内核文档 iommu.rst 中的
- [ ] 如何理解 container 中的内容: `vfio_group_set_container`
    - 所以 container 是个什么概念
- `vfio_iommu_type1_group_iommu_domain` 中的 domain 是个什么含义
- [ ] 应该是 container 中含有 group 的
- [ ] 难道一个主板上可以有多个 IOMMU，否则，为什么会存在 `iommu_group`
