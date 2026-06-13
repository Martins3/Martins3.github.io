## vfio 对外提供的三个 fops
<!-- 8e0e6be3-38c6-4bb6-9f76-b800b0067688 -->

| fops             | 关联的 fd 的方法                                                     |
| ---------------  | -----------------------------                                      |
| vfio_fops        | /dev/vfio/vfio 打开这个文件获取到 container fd                     |
| vfio_group_fops  | /dev/vfio/2 对于这个调用 VFIO_GROUP_GET_DEVICE_FD 来获取 `device_id` fd |
| vfio_device_fops | 通过 `device_id` fd  来获取 memory region 和中断信息                                        |

所以是相当有层次的，vfio -> vfio group -> vfio device

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

简而言之:
dev_vfio_info.container = open(VFIO_CONTAINER_PATH, O_RDWR);
dev_vfio_info.group = open(group_addr, O_RDWR);
ioctl(info->group, VFIO_GROUP_SET_CONTAINER, &info->container);

ioctl(dev_vfio_info.group, VFIO_GROUP_GET_DEVICE_FD, dev.name);

ioctl(info->container, VFIO_IOMMU_MAP_DMA, map);
ioctl(dev_fd, VFIO_DEVICE_GET_INFO, info);
ioctl(dev->device_fd, VFIO_DEVICE_GET_REGION_INFO, reg);
ioctl(dev->device_fd, VFIO_DEVICE_GET_IRQ_INFO, irq);


## VFIO_SET_IOMMU 如何实现的
<!-- 555ccec4-a0cd-4017-b80d-f3017a4583f7 -->

对应 vfio-host-test
```c
/* return 1 on fail */
int set_iommu_type(struct vfio_info *info)
{
	return ioctl(info->container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);
}
```

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

这里的所谓的 iommu_drivers_list 也就是 iommu 和 noiommu 了

## VFIO_GROUP_SET_CONTAINER 如何实现的
<!-- 7677bd8c-2e73-4c2e-bc4d-cbe948f008c6 -->

将 group 和 container 联系起来，也就是 /dev/vfio/2 和 /dev/vfio/vfio 联系起来

具体实现，我现在没有兴趣

## VFIO_GROUP_GET_DEVICE_FD 的如何实现的
<!-- 38715772-6a4e-4ee1-a94a-fe3ab8d92d84 -->

- `VFIO_GROUP_GET_DEVICE_FD` ： `vfio_group_get_device_fd`
  - `vfio_device_open`
    - `vfio_device_get_from_name`
      - 对于 `vfio_group::device_list` 进行遍历
      - 调用 hook : `vfio_pci_core_match` 将参数对比，而 buf 是就是 /sys/bus/pci/devices/ 下的 bfd
    - `vfio_device_assign_container`

- `vfio_device_fops_unl_ioctl`
  - `vfio_device::vfio_device_ops::ioctl`
    - `vfio_pci_core_ioctl`


## mmap pci mmio 空间的基本路径是什么?

```txt
@[
        vfio_pci_core_mmap+5 / vfio_device_fops_mmap+5
        __mmap_new_vma+260
        __mmap_region+2654
        mmap_region+132
        do_mmap+1162
        vm_mmap_pgoff+293
        ksys_mmap_pgoff+331
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 2
```

QEMU 的映射函数 vfio_region_mmap

```c
const struct file_operations vfio_device_fops = {
	// ...
	.mmap		= vfio_device_fops_mmap, // 当 mmap 的时候，首先调用这个
};


static const struct vfio_device_ops vfio_pci_ops = {
	.name		= "vfio-pci",
	// ...
	.mmap		= vfio_pci_core_mmap, // 如果后端是 vfio-pci ，那么会调用这个回调，还可能是 nvgrace_gpu_mmap
}
```

最后为该区域注册上 vfio_pci_mmap_fault ，然后 page fault 的时候，就可以配置了:

```c
static const struct vm_operations_struct vfio_pci_mmap_ops = {
	// ...
	.fault = vfio_pci_mmap_fault,
};
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
