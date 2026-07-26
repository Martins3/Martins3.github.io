# amd

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

## 调查下 iommu_pgsize


## 这三个是做啥的?
```c
__setup("ivrs_ioapic",		parse_ivrs_ioapic);
__setup("ivrs_hpet",		parse_ivrs_hpet);
__setup("ivrs_acpihid",		parse_ivrs_acpihid);
```


## iommu v2
原来是处理嵌套虚拟化的，有趣:
- https://www.phoronix.com/news/AMD-IOMMU-v2-Linux-6.1
- https://static.sched.com/hosted_files/kvmforum2020/26/vIOMMU%20KVM%20Forum%202020.pdf
- https://kvm-forum.qemu.org/2021/vIOMMU%20KVM%20Forum%202021%20-%20v4.pdf

这两个 kvm forum 是 2021 的，但是到今天，qemu 中还是没有支持，暂时先不用管这个事情了。

- [iommu/amd: Introduce hardware info reporting and nested translation support](https://lwn.net/Articles/954730/)

> In addition, this series introduces nested translation support for the
> AMD IOMMU v2 page table (stage-1) via the IOMMUFD nesting infrastructure.
> This allows User-space to set up the v2 page table and communicate
> information via the struct iommu_hwpt_amd_v2 with enum
> IOMMU_HWPT_DATA_AMD_V2.

那么，如果真的有了这个功能，那么如果把 guest virtio-fs 直通到 l2 虚拟机中，
那么，l2 中可以直接访问 l0 的文件系统?

## 似乎在 amd 下，iommu 也是一个普通的设备

```txt
🤒  lspci -s 0000:00:00.2
00:00.2 IOMMU: Chengdu Haiguang IC Design Co., Ltd. I/O Memory Management Unit
~ 🎏
🧀  sudo cat /proc/iomem | grep iommu
8c880000-8c8fffff : amd_iommu
8d080000-8d0fffff : amd_iommu
a6880000-a68fffff : amd_iommu
a7080000-a70fffff : amd_iommu
c0880000-c08fffff : amd_iommu
c1080000-c10fffff : amd_iommu
df880000-df8fffff : amd_iommu
fd980000-fd9fffff : amd_iommu
~ 🎏
🧀  ls -la /sys/class/iommu
lrwxrwxrwx@ - root 19 Dec 22:21  ivhd0 -> ../../devices/pci0000:00/0000:00:00.2/iommu/ivhd0
lrwxrwxrwx@ - root 19 Dec 22:21  ivhd1 -> ../../devices/pci0000:20/0000:20:00.2/iommu/ivhd1
lrwxrwxrwx@ - root 19 Dec 22:21  ivhd2 -> ../../devices/pci0000:40/0000:40:00.2/iommu/ivhd2
lrwxrwxrwx@ - root 19 Dec 22:21  ivhd3 -> ../../devices/pci0000:60/0000:60:00.2/iommu/ivhd3
lrwxrwxrwx@ - root 19 Dec 22:21  ivhd4 -> ../../devices/pci0000:80/0000:80:00.2/iommu/ivhd4
lrwxrwxrwx@ - root 19 Dec 22:21  ivhd5 -> ../../devices/pci0000:a0/0000:a0:00.2/iommu/ivhd5
lrwxrwxrwx@ - root 19 Dec 22:21  ivhd6 -> ../../devices/pci0000:c0/0000:c0:00.2/iommu/ivhd6
lrwxrwxrwx@ - root 19 Dec 22:21  ivhd7 -> ../../devices/pci0000:e0/0000:e0:00.2/iommu/ivhd7
```

但是在 intel 下不是。

## 从 amd64/amd-iommu/3-registers.md 看，似乎也不难，配合手册

## amd_iommu_xt_mode 是什么东西

```txt
enum {
	IRQ_REMAP_XAPIC_MODE,
	IRQ_REMAP_X2APIC_MODE,
};
```

## 似乎 interrupt remapping 是默认启动的

```txt
🤒  dmesg | grep AMD-Vi
[    0.129050] AMD-Vi: ivrs, add hid:AMDI0020, uid:\_SB.FUR0, rdevid:160
[    0.129051] AMD-Vi: ivrs, add hid:AMDI0020, uid:\_SB.FUR1, rdevid:160
[    0.129052] AMD-Vi: ivrs, add hid:AMDI0020, uid:\_SB.FUR2, rdevid:160
[    0.129052] AMD-Vi: ivrs, add hid:AMDI0020, uid:\_SB.FUR3, rdevid:160
[    0.129053] AMD-Vi: Using global IVHD EFR:0x246577efa2254afa, EFR2:0x0
[    0.514728] pci 0000:00:00.2: AMD-Vi: IOMMU performance counters supported
[    0.515027] pci 0000:00:00.2: AMD-Vi: Found IOMMU cap 0x40
[    0.515028] AMD-Vi: Extended features (0x246577efa2254afa, 0x0): PPR NX GT [5] IA GA PC GA_vAPIC
[    0.515031] AMD-Vi: Interrupt remapping enabled
[    0.515270] AMD-Vi: Virtual APIC enabled
```

## 问题
2. iommu_enable_ga 的含义

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
