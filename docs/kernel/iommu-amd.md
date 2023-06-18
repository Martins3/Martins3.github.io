# iommu 文件夹基本代码印象

## amd
init.c 和 iommu.c

iommu_go_to_state
state_next

- pci_iommu_alloc
  - pci_swiotlb_detect
  - amd_iommu_detect
    - x86_init.iommu.iommu_init = amd_iommu_init;
  - detect_intel_iommu
    - x86_init.iommu.iommu_init = intel_iommu_init;
  - swiotlb_init

- amd_iommu_init
  - iommu_go_to_state
    - state_next : 一系列的 swtich case
      - amd_iommu_init_pci
    - amd_iommu_debugfs_setup

## 分析的 for_each_iommu

- state_next
  - early_amd_iommu_init : 通过 acpi 获取到 ivrs
    - init_iommu_all
      - init_iommu_one : 中来加入的

这里存在多个 iommu ，到底如何理解?


## 一些关键的结构体
```c
static struct irq_chip intcapxt_controller = {
	.name			= "IOMMU-MSI",
	.irq_unmask		= intcapxt_unmask_irq,
	.irq_mask		= intcapxt_mask_irq,
	.irq_ack		= irq_chip_ack_parent,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_set_affinity       = intcapxt_set_affinity,
	.irq_set_wake		= intcapxt_set_wake,
	.flags			= IRQCHIP_MASK_ON_SUSPEND,
};

static const struct irq_domain_ops intcapxt_domain_ops = {
	.alloc			= intcapxt_irqdomain_alloc,
	.free			= intcapxt_irqdomain_free,
	.activate		= intcapxt_irqdomain_activate,
	.deactivate		= intcapxt_irqdomain_deactivate,
};
```

```c
const struct iommu_ops amd_iommu_ops = {
	.capable = amd_iommu_capable,
	.domain_alloc = amd_iommu_domain_alloc,
	.probe_device = amd_iommu_probe_device,
	.release_device = amd_iommu_release_device,
	.probe_finalize = amd_iommu_probe_finalize,
	.device_group = amd_iommu_device_group,
	.get_resv_regions = amd_iommu_get_resv_regions,
	.is_attach_deferred = amd_iommu_is_attach_deferred,
	.pgsize_bitmap	= AMD_IOMMU_PGSIZES,
	.def_domain_type = amd_iommu_def_domain_type,
	.default_domain_ops = &(const struct iommu_domain_ops) {
		.attach_dev	= amd_iommu_attach_device,
		.map_pages	= amd_iommu_map_pages,
		.unmap_pages	= amd_iommu_unmap_pages,
		.iotlb_sync_map	= amd_iommu_iotlb_sync_map,
		.iova_to_phys	= amd_iommu_iova_to_phys,
		.flush_iotlb_all = amd_iommu_flush_iotlb_all,
		.iotlb_sync	= amd_iommu_iotlb_sync,
		.free		= amd_iommu_domain_free,
		.enforce_cache_coherency = amd_iommu_enforce_cache_coherency,
	}
};
```

- amd_iommu_domain_alloc
  - protection_domain_alloc
    - protection_domain_init_v1
    - protection_domain_init_v2
    - alloc_io_pgtable_ops

- amd_iommu_attach_device
  - attach_device
    - do_attach
