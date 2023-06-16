# 真正的分析 vfio

```c
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

- [ ] vfio 只是支持 type1 ?
