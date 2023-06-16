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
