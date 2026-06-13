# 如何理解 iommufd 这个东西

这是一个很的新的机制

## kvm forum : IOMMUFD Integration in QEMU
https://www.youtube.com/watch?v=PlEzLywexHE

操作 device 的接口:

- VFIO_DEVICE_GET_REGION_INFO
- VFIO_DEVICE_GET_INFO
- VFIO_DEVICE_GET_IRQ_INFO
- VFIO_DEVICE_SET_IRQS
- VFIO_DEVICE_RESET

操作 group 的接口:
- VFIO_GROUP_SET_CONTAINER
- VFIO_GROUP_GET_STATUS
- VFIO_GROUP_GET_DEVICE_FD

container 的接口:
- VFIO_SET_IOMMU
- VFIO_IOMMU_GET_INFO
- VFIO_IOMMU_MAP_DMA
- VFIO_CHECK_EXTENSION
- VFIO_GET_API_VERSION

相关会议可以看看:
- LPC 17 : vSVM IOMMU extension Highlevel component break down and Tech challenge
- vIOMMU implementation using hardware nested paging
- PASID Management in KVM



- IOMMU_IOAS_MAP
- IOMMU_IOAS_COPY
- IOMMU_IOAS_UNMAP
- IOMMU_IOAS_ALLOC : 创建一个 ioas
- IOMMU_IOAS_IOVA_RANGES
- IOMMU_IOAS_ALLOW_IOVAS
- IOMMU_IOAS_COPY : 让映射在不同的 ioas 中拷贝

```c
static const struct iommufd_ioctl_op iommufd_ioctl_ops[] = {
	IOCTL_OP(IOMMU_DESTROY, iommufd_destroy, struct iommu_destroy, id),
	IOCTL_OP(IOMMU_IOAS_ALLOC, iommufd_ioas_alloc_ioctl,
		 struct iommu_ioas_alloc, out_ioas_id),
	IOCTL_OP(IOMMU_IOAS_ALLOW_IOVAS, iommufd_ioas_allow_iovas,
		 struct iommu_ioas_allow_iovas, allowed_iovas),
	IOCTL_OP(IOMMU_IOAS_COPY, iommufd_ioas_copy, struct iommu_ioas_copy,
		 src_iova),
	IOCTL_OP(IOMMU_IOAS_IOVA_RANGES, iommufd_ioas_iova_ranges,
		 struct iommu_ioas_iova_ranges, out_iova_alignment),
	IOCTL_OP(IOMMU_IOAS_MAP, iommufd_ioas_map, struct iommu_ioas_map,
		 iova),
	IOCTL_OP(IOMMU_IOAS_UNMAP, iommufd_ioas_unmap, struct iommu_ioas_unmap,
		 length),
	IOCTL_OP(IOMMU_OPTION, iommufd_option, struct iommu_option,
		 val64),
	IOCTL_OP(IOMMU_VFIO_IOAS, iommufd_vfio_ioas, struct iommu_vfio_ioas,
		 __reserved),
#ifdef CONFIG_IOMMUFD_TEST
	IOCTL_OP(IOMMU_TEST_CMD, iommufd_test, struct iommu_test_cmd, last),
#endif
};
```

## 简要的代码分析

- vfio_iommufd_physical_attach_ioas
  - iommufd_device_attach
    - iommufd_device_auto_get_domain
      - iommufd_hw_pagetable_alloc
        - iopt_table_add_domain
          - iopt_fill_domain

## https://www.phoronix.com/news/IOMMUFD-Linux-6.2

> Further, we have advanced PCI features like Process Address Space ID (PASID) and Page Request Interface (PRI) that rely on the IOMMU HW to implement them.
> In particular PASID & PRI are used to create something called Shared Virtual Addressing (SVA or SVM) where DMA from a device can be directly delivered to a process virtual memory address by having the IOMMU HW directly walk the CPU's page table for the process, and trigger faults for DMA to non-present pages.

## 代码的简单阅读

| Files          | Lines | Code | Comments | Blanks | 主要内容 |
|----------------|-------|------|----------|--------|----------|
| pages.c        | 1991  | 1435 | 321      | 235    |
| io_pagetable.c | 1216  | 903  | 146      | 167    |
| selftest.c     | 1006  | 810  | 58       | 138    |
| device.c       | 721   | 445  | 188      | 88     |
| vfio_compat.c  | 539   | 385  | 91       | 63     |
| main.c         | 463   | 338  | 74       | 51     |
| ioas.c         | 398   | 322  | 18       | 58     |
| hw_pagetable.c | 105   | 64   | 23       | 18     |

## Documentation/userspace-api/iommufd.rst

```c
static struct miscdevice iommu_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "iommu",
	.fops = &iommufd_fops,
	.nodename = "iommu",
	.mode = 0660,
};


static struct miscdevice vfio_misc_dev = {
	.minor = VFIO_MINOR,
	.name = "vfio",
	.fops = &iommufd_fops,
	.nodename = "vfio/vfio",
	.mode = 0666,
};
```

字符设备的目录这个显示应该有点问题吧，都是 10,196 ?
```txt
🧀  ls -la /dev/vfio/vfio
crw-rw-rw- 10,196 root 16 6月  22:55  /dev/vfio/vfio
vn on  master [!+]
🧀  ls -la /dev/vfio
crw-rw-rw- 10,196 root 16 6月  22:55  vfio
```
