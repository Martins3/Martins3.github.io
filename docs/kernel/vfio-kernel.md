## new_id 是什么含义?
```sh
echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
```

当使用 vendor 和 device id 告知进去:

- vfio_pci_probe
  - vfio_pci_core_register_device
    - __vfio_register_dev
      - vfio_device_set_group
        - vfio_group_find_or_alloc

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

## 结构体
- `VFIO_DEVICE_GET_INFO` : 可以获取 `struct vfio_device_info`
提供 ioctl

## group 行为

### VFIO_IOMMU_MAP_DMA

- vfio_iommu_type1_map_dma
  - vfio_dma_do_map
    - vfio_find_dma
      - vfio_pin_map_dma
        - vfio_pin_pages_remote : 好家伙啊，所有的内存全部 pin 住啊
        - vfio_iommu_map

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
