# IOMMU

- https://luohao-brian.gitbooks.io/interrupt-virtualization/content/vt-d-interrupt-remapping-fen-xi.html

## 问题
- iommu=pt 是导致不可用的原因吗?
- 有什么方法提前获取到 vfio cgroup 的数值

## [ ] 看看 iommu 的用户态接口

### debugfs

### sys
cd /sys 然后 fd iommu

intel 上观察到的
```txt
class/iommu/
class/misc/iommu
devices/pci0000:00/0000:00:00.0/iommu
devices/pci0000:00/0000:00:00.0/iommu_group
devices/pci0000:00/0000:00:01.0/0000:01:00.0/iommu
devices/pci0000:00/0000:00:01.0/0000:01:00.0/iommu_group
devices/pci0000:00/0000:00:01.0/0000:01:00.1/iommu
devices/pci0000:00/0000:00:01.0/0000:01:00.1/iommu_group
devices/pci0000:00/0000:00:01.0/iommu
devices/pci0000:00/0000:00:01.0/iommu_group
devices/pci0000:00/0000:00:02.0/iommu
devices/pci0000:00/0000:00:02.0/iommu_group
devices/pci0000:00/0000:00:06.0/0000:02:00.0/iommu
devices/pci0000:00/0000:00:06.0/0000:02:00.0/iommu_group
devices/pci0000:00/0000:00:06.0/iommu
devices/pci0000:00/0000:00:06.0/iommu_group
devices/pci0000:00/0000:00:0a.0/iommu
devices/pci0000:00/0000:00:0a.0/iommu_group
devices/pci0000:00/0000:00:0e.0/iommu
devices/pci0000:00/0000:00:0e.0/iommu_group
devices/pci0000:00/0000:00:14.0/iommu
devices/pci0000:00/0000:00:14.0/iommu_group
devices/pci0000:00/0000:00:14.2/iommu
devices/pci0000:00/0000:00:14.2/iommu_group
devices/pci0000:00/0000:00:14.3/iommu
devices/pci0000:00/0000:00:14.3/iommu_group
devices/pci0000:00/0000:00:15.0/iommu
devices/pci0000:00/0000:00:15.0/iommu_group
devices/pci0000:00/0000:00:15.1/iommu
devices/pci0000:00/0000:00:15.1/iommu_group
devices/pci0000:00/0000:00:15.2/iommu
devices/pci0000:00/0000:00:15.2/iommu_group
devices/pci0000:00/0000:00:16.0/iommu
devices/pci0000:00/0000:00:16.0/iommu_group
devices/pci0000:00/0000:00:17.0/iommu
devices/pci0000:00/0000:00:17.0/iommu_group
devices/pci0000:00/0000:00:1a.0/0000:03:00.0/iommu
devices/pci0000:00/0000:00:1a.0/0000:03:00.0/iommu_group
devices/pci0000:00/0000:00:1a.0/iommu
devices/pci0000:00/0000:00:1a.0/iommu_group
devices/pci0000:00/0000:00:1c.0/iommu
devices/pci0000:00/0000:00:1c.0/iommu_group
devices/pci0000:00/0000:00:1c.2/0000:05:00.0/iommu
devices/pci0000:00/0000:00:1c.2/0000:05:00.0/iommu_group
devices/pci0000:00/0000:00:1c.2/iommu
devices/pci0000:00/0000:00:1c.2/iommu_group
devices/pci0000:00/0000:00:1f.0/iommu
devices/pci0000:00/0000:00:1f.0/iommu_group
devices/pci0000:00/0000:00:1f.3/iommu
devices/pci0000:00/0000:00:1f.3/iommu_group
devices/pci0000:00/0000:00:1f.4/iommu
devices/pci0000:00/0000:00:1f.4/iommu_group
devices/pci0000:00/0000:00:1f.5/iommu
devices/pci0000:00/0000:00:1f.5/iommu_group
devices/virtual/iommu/
devices/virtual/iommu/dmar0/intel-iommu/
devices/virtual/iommu/dmar1/intel-iommu/
devices/virtual/misc/iommu/
kernel/btf/iommufd
kernel/btf/vfio_iommu_type1
kernel/iommu_groups/
kernel/slab/iommu_iova
module/iommufd/
module/vfio/holders/vfio_iommu_type1
module/vfio_iommu_type1/
```

```txt
00:00.0 Host bridge: Intel Corporation Device a700 (rev 01)
00:01.0 PCI bridge: Intel Corporation Device a70d (rev 01)
00:02.0 VGA compatible controller: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] (rev 04)
00:06.0 PCI bridge: Intel Corporation Device a74d (rev 01)
00:0a.0 Signal processing controller: Intel Corporation Device a77d (rev 01)
00:0e.0 RAID bus controller: Intel Corporation Volume Management Device NVMe RAID Controller Intel Corporation
00:14.0 USB controller: Intel Corporation Alder Lake-S PCH USB 3.2 Gen 2x2 XHCI Controller (rev 11)
00:14.2 RAM memory: Intel Corporation Alder Lake-S PCH Shared SRAM (rev 11)
00:14.3 Network controller: Intel Corporation Alder Lake-S PCH CNVi WiFi (rev 11)
00:15.0 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #0 (rev 11)
00:15.1 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #1 (rev 11)
00:15.2 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #2 (rev 11)
00:16.0 Communication controller: Intel Corporation Alder Lake-S PCH HECI Controller #1 (rev 11)
00:17.0 SATA controller: Intel Corporation Alder Lake-S PCH SATA Controller [AHCI Mode] (rev 11)
00:1a.0 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #25 (rev 11)
00:1c.0 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #1 (rev 11)
00:1c.2 PCI bridge: Intel Corporation Device 7aba (rev 11)
00:1f.0 ISA bridge: Intel Corporation Device 7a86 (rev 11)
00:1f.3 Audio device: Intel Corporation Alder Lake-S HD Audio Controller (rev 11)
00:1f.4 SMBus: Intel Corporation Alder Lake-S PCH SMBus Controller (rev 11)
00:1f.5 Serial bus controller: Intel Corporation Alder Lake-S PCH SPI Controller (rev 11)
01:00.0 VGA compatible controller: NVIDIA Corporation GP106 [GeForce GTX 1060 3GB] (rev a1)
01:00.1 Audio device: NVIDIA Corporation GP106 High Definition Audio Controller (rev a1)
02:00.0 Non-Volatile memory controller: Yangtze Memory Technologies Co.,Ltd ZHITAI TiPro7000 (rev 01)
03:00.0 Non-Volatile memory controller: Yangtze Memory Technologies Co.,Ltd Device 0071 (rev 01)
05:00.0 Ethernet controller: Intel Corporation Ethernet Controller I225-V (rev 03)
```

amd 上观察到的:
```txt
bus/event_source/devices/amd_iommu_0
class/iommu/
class/misc/iommu
devices/amd_iommu_0/
devices/amd_iommu_0/events/mem_iommu_tlb_pde_hit
devices/amd_iommu_0/events/mem_iommu_tlb_pde_mis
devices/amd_iommu_0/events/mem_iommu_tlb_pte_hit
devices/amd_iommu_0/events/mem_iommu_tlb_pte_mis
devices/pci0000:00/0000:00:00.2/iommu/
devices/pci0000:00/0000:00:00.2/iommu/ivhd0/amd-iommu/
devices/pci0000:00/0000:00:01.0/iommu
devices/pci0000:00/0000:00:01.0/iommu_group
devices/pci0000:00/0000:00:01.1/0000:01:00.0/iommu
devices/pci0000:00/0000:00:01.1/0000:01:00.0/iommu_group
devices/pci0000:00/0000:00:01.1/0000:01:00.1/iommu
devices/pci0000:00/0000:00:01.1/0000:01:00.1/iommu_group
devices/pci0000:00/0000:00:01.1/iommu
devices/pci0000:00/0000:00:01.1/iommu_group
devices/pci0000:00/0000:00:01.2/0000:02:00.0/iommu
devices/pci0000:00/0000:00:01.2/0000:02:00.0/iommu_group
devices/pci0000:00/0000:00:01.2/iommu
devices/pci0000:00/0000:00:01.2/iommu_group
devices/pci0000:00/0000:00:02.0/iommu
devices/pci0000:00/0000:00:02.0/iommu_group
devices/pci0000:00/0000:00:02.1/iommu
devices/pci0000:00/0000:00:02.1/iommu_group
devices/pci0000:00/0000:00:02.2/0000:04:00.0/iommu
devices/pci0000:00/0000:00:02.2/0000:04:00.0/iommu_group
devices/pci0000:00/0000:00:02.2/iommu
devices/pci0000:00/0000:00:02.2/iommu_group
devices/pci0000:00/0000:00:03.0/iommu
devices/pci0000:00/0000:00:03.0/iommu_group
devices/pci0000:00/0000:00:03.1/iommu
devices/pci0000:00/0000:00:03.1/iommu_group
devices/pci0000:00/0000:00:03.2/iommu
devices/pci0000:00/0000:00:03.2/iommu_group
devices/pci0000:00/0000:00:03.3/0000:07:00.0/iommu
devices/pci0000:00/0000:00:03.3/0000:07:00.0/iommu_group
devices/pci0000:00/0000:00:03.3/iommu
devices/pci0000:00/0000:00:03.3/iommu_group
devices/pci0000:00/0000:00:04.0/iommu
devices/pci0000:00/0000:00:04.0/iommu_group
devices/pci0000:00/0000:00:08.0/iommu
devices/pci0000:00/0000:00:08.0/iommu_group
devices/pci0000:00/0000:00:08.1/0000:08:00.0/iommu
devices/pci0000:00/0000:00:08.1/0000:08:00.0/iommu_group
devices/pci0000:00/0000:00:08.1/0000:08:00.2/iommu
devices/pci0000:00/0000:00:08.1/0000:08:00.2/iommu_group
devices/pci0000:00/0000:00:08.1/0000:08:00.3/iommu
devices/pci0000:00/0000:00:08.1/0000:08:00.3/iommu_group
devices/pci0000:00/0000:00:08.1/0000:08:00.4/iommu
devices/pci0000:00/0000:00:08.1/0000:08:00.4/iommu_group
devices/pci0000:00/0000:00:08.1/0000:08:00.5/iommu
devices/pci0000:00/0000:00:08.1/0000:08:00.5/iommu_group
devices/pci0000:00/0000:00:08.1/0000:08:00.6/iommu
devices/pci0000:00/0000:00:08.1/0000:08:00.6/iommu_group
devices/pci0000:00/0000:00:08.1/iommu
devices/pci0000:00/0000:00:08.1/iommu_group
devices/pci0000:00/0000:00:08.3/0000:09:00.0/iommu
devices/pci0000:00/0000:00:08.3/0000:09:00.0/iommu_group
devices/pci0000:00/0000:00:08.3/iommu
devices/pci0000:00/0000:00:08.3/iommu_group
devices/pci0000:00/0000:00:14.0/iommu
devices/pci0000:00/0000:00:14.0/iommu_group
devices/pci0000:00/0000:00:14.3/iommu
devices/pci0000:00/0000:00:14.3/iommu_group
devices/pci0000:00/0000:00:18.0/iommu
devices/pci0000:00/0000:00:18.0/iommu_group
devices/pci0000:00/0000:00:18.1/iommu
devices/pci0000:00/0000:00:18.1/iommu_group
devices/pci0000:00/0000:00:18.2/iommu
devices/pci0000:00/0000:00:18.2/iommu_group
devices/pci0000:00/0000:00:18.3/iommu
devices/pci0000:00/0000:00:18.3/iommu_group
devices/pci0000:00/0000:00:18.4/iommu
devices/pci0000:00/0000:00:18.4/iommu_group
devices/pci0000:00/0000:00:18.5/iommu
devices/pci0000:00/0000:00:18.5/iommu_group
devices/pci0000:00/0000:00:18.6/iommu
devices/pci0000:00/0000:00:18.6/iommu_group
devices/pci0000:00/0000:00:18.7/iommu
devices/pci0000:00/0000:00:18.7/iommu_group
devices/virtual/misc/iommu/
kernel/btf/iommufd
kernel/btf/vfio_iommu_type1
kernel/iommu_groups/
kernel/slab/iommu_iova
module/iommufd/
module/vfio/holders/vfio_iommu_type1
module/vfio_iommu_type1/
```

```txt
00:00.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14d8
00:00.2 IOMMU: Advanced Micro Devices, Inc. [AMD] Device 14d9
00:01.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14da
00:01.1 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14db
00:01.2 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14db
00:02.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14da
00:02.1 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14db
00:02.2 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14db
00:03.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14da
00:03.1 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14db
00:03.2 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14db
00:03.3 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14db
00:04.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14da
00:08.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14da
00:08.1 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14dd
00:08.3 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 14dd
00:14.0 SMBus: Advanced Micro Devices, Inc. [AMD] FCH SMBus Controller (rev 71)
00:14.3 ISA bridge: Advanced Micro Devices, Inc. [AMD] FCH LPC Bridge (rev 51)
00:18.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14e0
00:18.1 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14e1
00:18.2 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14e2
00:18.3 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14e3
00:18.4 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14e4
00:18.5 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14e5
00:18.6 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14e6
00:18.7 Host bridge: Advanced Micro Devices, Inc. [AMD] Device 14e7
01:00.0 VGA compatible controller: NVIDIA Corporation AD107M [GeForce RTX 4060 Max-Q / Mobile] (rev a1)
01:00.1 Audio device: NVIDIA Corporation Device 22be (rev a1)
02:00.0 Non-Volatile memory controller: Samsung Electronics Co Ltd NVMe SSD Controller PM9A1/PM9A3/980PRO
04:00.0 Network controller: MEDIATEK Corp. MT7922 802.11ax PCI Express Wireless Network Adapter
07:00.0 Ethernet controller: Realtek Semiconductor Co., Ltd. RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller (rev 15)
08:00.0 Non-Essential Instrumentation [1300]: Advanced Micro Devices, Inc. [AMD] Device 14de (rev d8)
08:00.2 Encryption controller: Advanced Micro Devices, Inc. [AMD] VanGogh PSP/CCP
08:00.3 USB controller: Advanced Micro Devices, Inc. [AMD] Device 15b6
08:00.4 USB controller: Advanced Micro Devices, Inc. [AMD] Device 15b7
08:00.5 Multimedia controller: Advanced Micro Devices, Inc. [AMD] ACP/ACP3X/ACP6x Audio Coprocessor (rev 62)
08:00.6 Audio device: Advanced Micro Devices, Inc. [AMD] Family 17h/19h HD Audio Controller
09:00.0 USB controller: Advanced Micro Devices, Inc. [AMD] Device 15b8
```

- [ ] PCI 和 Host Bridge 是什么关系?

## [ ] iommu 的 kernel 参数理解

### AMD 的参数
```txt
        amd_iommu=      [HW,X86-64]
                        Pass parameters to the AMD IOMMU driver in the system.
                        Possible values are:
                        fullflush - Deprecated, equivalent to iommu.strict=1
                        off       - do not initialize any AMD IOMMU found in
                                    the system
                        force_isolation - Force device isolation for all
                                          devices. The IOMMU driver is not
                                          allowed anymore to lift isolation
                                          requirements as needed. This option
                                          does not override iommu=pt
                        force_enable - Force enable the IOMMU on platforms known
                                       to be buggy with IOMMU enabled. Use this
                                       option with care.
                        pgtbl_v1     - Use v1 page table for DMA-API (Default).
                        pgtbl_v2     - Use v2 page table for DMA-API.

        amd_iommu_dump= [HW,X86-64]
                        Enable AMD IOMMU driver option to dump the ACPI table
                        for AMD IOMMU. With this option enabled, AMD IOMMU
                        driver will print ACPI tables for AMD IOMMU during
                        IOMMU initialization.

        amd_iommu_intr= [HW,X86-64]
                        Specifies one of the following AMD IOMMU interrupt
                        remapping modes:
                        legacy     - Use legacy interrupt remapping mode.
                        vapic      - Use virtual APIC mode, which allows IOMMU
                                     to inject interrupts directly into guest.
                                     This mode requires kvm-amd.avic=1.
                                     (Default when IOMMU HW support is present.)
```

似乎当没有 amd_iommu=on 中启动的时候，存在如下的
```txt
qemu-system-x86_64: -device vfio-pci,host=09:00.0: vfio 0000:09:00.0: group 4 is not viable
Please ensure all devices within the iommu_group are bound to their vfio bus driver.
```

看内核参数:

```c
__setup("amd_iommu_dump",	parse_amd_iommu_dump);
__setup("amd_iommu=",		parse_amd_iommu_options);
__setup("amd_iommu_intr=",	parse_amd_iommu_intr);
__setup("ivrs_ioapic",		parse_ivrs_ioapic);
__setup("ivrs_hpet",		parse_ivrs_hpet);
__setup("ivrs_acpihid",		parse_ivrs_acpihid);
```

### Intel 参数
```txt
        intel_iommu=    [DMAR] Intel IOMMU driver (DMAR) option
                on
                        Enable intel iommu driver.
                off
                        Disable intel iommu driver.
                igfx_off [Default Off]
                        By default, gfx is mapped as normal device. If a gfx
                        device has a dedicated DMAR unit, the DMAR unit is
                        bypassed by not enabling DMAR with this option. In
                        this case, gfx device will use physical address for
                        DMA.
                strict [Default Off]
                        Deprecated, equivalent to iommu.strict=1.
                sp_off [Default Off]
                        By default, super page will be supported if Intel IOMMU
                        has the capability. With this option, super page will
                        not be supported.
                sm_on
                        Enable the Intel IOMMU scalable mode if the hardware
                        advertises that it has support for the scalable mode
                        translation.
                sm_off
                        Disallow use of the Intel IOMMU scalable mode.
                tboot_noforce [Default Off]
                        Do not force the Intel IOMMU enabled under tboot.
                        By default, tboot will force Intel IOMMU on, which
                        could harm performance of some high-throughput
                        devices like 40GBit network cards, even if identity
                        mapping is enabled.
                        Note that using this option lowers the security
                        provided by tboot because it makes the system
                        vulnerable to DMA attacks.

        intremap=       [X86-64, Intel-IOMMU]
                        on      enable Interrupt Remapping (default)
                        off     disable Interrupt Remapping
                        nosid   disable Source ID checking
                        no_x2apic_optout
                                BIOS x2APIC opt-out request will be ignored
                        nopost  disable Interrupt Posting
```

### 通用的参数
```txt
        iommu=          [X86]
                off
                force
                noforce
                biomerge
                panic
                nopanic
                merge
                nomerge
                soft
                pt              [X86]
                nopt            [X86]
                nobypass        [PPC/POWERNV]
                        Disable IOMMU bypass, using IOMMU for PCI devices.

        iommu.forcedac= [ARM64, X86] Control IOVA allocation for PCI devices.
                        Format: { "0" | "1" }
                        0 - Try to allocate a 32-bit DMA address first, before
                          falling back to the full range if needed.
                        1 - Allocate directly from the full usable range,
                          forcing Dual Address Cycle for PCI cards supporting
                          greater than 32-bit addressing.

        iommu.strict=   [ARM64, X86] Configure TLB invalidation behaviour
                        Format: { "0" | "1" }
                        0 - Lazy mode.
                          Request that DMA unmap operations use deferred
                          invalidation of hardware TLBs, for increased
                          throughput at the cost of reduced device isolation.
                          Will fall back to strict mode if not supported by
                          the relevant IOMMU driver.
                        1 - Strict mode.
                          DMA unmap operations invalidate IOMMU hardware TLBs
                          synchronously.
                        unset - Use value of CONFIG_IOMMU_DEFAULT_DMA_{LAZY,STRICT}.
                        Note: on x86, strict mode specified via one of the
                        legacy driver-specific options takes precedence.

        iommu.passthrough=
                        [ARM64, X86] Configure DMA to bypass the IOMMU by default.
                        Format: { "0" | "1" }
                        0 - Use IOMMU translation for DMA.
                        1 - Bypass the IOMMU for DMA.
                        unset - Use value of CONFIG_IOMMU_DEFAULT_PASSTHROUGH.
```

#### iommu.passthrough

```c
/*
 * This are the possible domain-types
 *
 *	IOMMU_DOMAIN_BLOCKED	- All DMA is blocked, can be used to isolate
 *				  devices
 *	IOMMU_DOMAIN_IDENTITY	- DMA addresses are system physical addresses
 *	IOMMU_DOMAIN_UNMANAGED	- DMA mappings managed by IOMMU-API user, used
 *				  for VMs
 *	IOMMU_DOMAIN_DMA	- Internally used for DMA-API implementations.
 *				  This flag allows IOMMU drivers to implement
 *				  certain optimizations for these domains
 *	IOMMU_DOMAIN_DMA_FQ	- As above, but definitely using batched TLB
 *				  invalidation.
 *	IOMMU_DOMAIN_SVA	- DMA addresses are shared process addresses
 *				  represented by mm_struct's.
 */
```
- [ ] IOMMU_DOMAIN_SVA 这个是给 dpdk 使用吗?


```c
phys_addr_t iommu_iova_to_phys(struct iommu_domain *domain, dma_addr_t iova)
{
	if (domain->type == IOMMU_DOMAIN_IDENTITY)
		return iova;

	if (domain->type == IOMMU_DOMAIN_BLOCKED)
		return 0;

	return domain->ops->iova_to_phys(domain, iova);
}
```

#### [ ] iommu=pt 为什么还是可以使用 GPU 直通，无法理解啊

#### [ ] iommu=pt 和 swiotlb 啥关系?

## 问题 && TODO
- [ ] drivers/iommu/hyperv-iommu.c 是个什么概念 ?
- [ ] 能不能 hacking 一个 minimal 的用户态 nvme 驱动，能够读取一个 block 上来的那种
- [ ] 在代码中找到 device page table 的内容，以及 IOMMU 防护恶意驱动
- [ ] 据说 IOMMU 对于性能会存在影响。
- [ ] IOMMU 能不能解决 cma 的问题，也就是，使用离散的物理内存提供给设备，让设备以为自己访问的是连续的物理内存


1. https://wiki.qemu.org/Features/VT-d 分析了下为什么 guest 需要 vIOMMU
2. [oracle 的 blog](https://blogs.oracle.com/linux/post/a-study-of-the-linux-kernel-pci-subsystem-with-qemu) 告诉了 iommu 打开的方法 : `-device intel-iommu` + `-machine q35`
  - iommu 在 QEMU 中就是调试的作用
  - 还分析过 pcie switch 的

## https://events19.linuxfoundation.cn/wp-content/uploads/2017/11/Shared-Virtual-Addressing_Yisheng-Xie-_-Bob-Liu.pdf

## [isca_iommu_tutorial](http://pages.cs.wisc.edu/~basu/isca_iommu_tutorial/IOMMU_TUTORIAL_ASPLOS_2016.pdf)

> Extraneous IPI adds overheads => Each extra interrupt can add 5-10K cycles ==> Needs dynamic remapping of interrupts

似乎是 core 1 setup 了 io device 的中断，那么之后，io device 的中断到其他的 core 都需要额外的 ipi.
然后使用 iommu 之后，这个中断 remap 的事情不需要软件处理了

在异构计算中间，可以实现 GPU 共享 CPU 的 page table 之后，获取相同的虚拟地址空间。

> IOMMU IS PART OF PROCESSOR COMPLEX

io device 经过各级 pci hub 到达 root complex,  进入 iommu 翻译，然后到达 mmu controller

> Better solution: IOMMU remaps 32bit device physical
> address to system physical address beyond 32bit
> ‒ DMA goes directly into 64bit memory
> ‒ No CPU transfer
> ‒ More efficient

> If access occurs, OS gets notified and can shut the device & driver down and notifies the user or administrator

> Some I/O devices can issue DMA requests to system memory
> directly, without OS or Firmware intervention
> ‒ e.g.,1394/Firewire, network cards, as part of network boot
> ‒ That allows attacks to modify memory before even the OS has a chance to protect against the attacks

> IOMMU redirects device physical address set up by Guest OS driver (= Guest Physical Addresses) to the actual Host System Physical Address (SPA)

> Some memory copies are gone, because the same memory is accessed
>
> ‒ But the memory is not accessible concurrently, because of cache policies
>
> Two memory pools remain (cache coherent + non-coherent memory regions)
>
> Jobs are still queued through the OS driver chain and suffer from overhead
>
> Still requires expert programmers to get performance

> IOMMU Driver (running on CPU) issues commands to IOMMU
> ‒ e.g., Invalidate IOMMU TLB Entry, Invalidate IOTLB Entry
> ‒ e.g., Invalidate Device Table Entry
> ‒ e.g., Complete PPR, Completion Wait , etc.
>
> Issued via Command Buffer
> ‒ Memory resident circular buffer
> ‒ MMIO registers: Base, Head, and Tail register

> ![](./img/c.png)
> device remapping table
> ![](./img/b.png)
> interrupt remapping table
> ![](./img/a.png)

## https://kernel.love/intel_iommu.html
- 解释了一下 intel iommu 启动的过程

## https://kernel.love/interrupt-remapping.html

- https://zhuanlan.zhihu.com/p/372385232 ：分析了初始化的过程

- 在 guest 中以为持有了某一个设备，那么如何才可以真正的使用

这两个函数正好描述了 MSI 的样子:
- `__irq_msi_compose_msg` 是普通的 irq 组装的样子
- linux/drivers/iommu/intel/irq_remapping.c 中的 fill_msi_msg

- [ ] 是不是只有在直通的时候，interrupt remapping 才需要，似乎打开 IOMMU 和使用 interrupt remapping 不是总是绑定的?

```c
static const struct irq_domain_ops intel_ir_domain_ops = {
    .select = intel_irq_remapping_select,
    .alloc = intel_irq_remapping_alloc,
    .free = intel_irq_remapping_free,
    .activate = intel_irq_remapping_activate,
    .deactivate = intel_irq_remapping_deactivate,
};
```

在其中 intel_irq_remapping_alloc 的位置将会来创建 IRTE


在 intel_setup_irq_remapping 中，调用 arch_get_ir_parent_domain
```c
/* Get parent irqdomain for interrupt remapping irqdomain */
static inline struct irq_domain *arch_get_ir_parent_domain(void)
{
    return x86_vector_domain;
}
```


## https://kernel.love/posted-interrupt.html


## 有趣

intremap=nosid

- https://www.reddit.com/r/linuxquestions/comments/8te134/what_do_nomodeset_intremapnosid_and_other_grub/
- https://www.greenbone.net/finder/vt/results/1.3.6.1.4.1.25623.1.0.870999
- https://serverfault.com/questions/1077297/ilo4-and-almalinux-centos8-do-not-work-properly

## AMD 手册
- https://www.amd.com/system/files/TechDocs/48882_IOMMU.pdf

## [An Introduction to IOMMU Infrastructure in the Linux Kernel](https://lenovopress.lenovo.com/lp1467.pdf)

- 主要是，这 DMA coherent 是什么关系哇

```c
#define dma_map_page(d, p, o, s, r) dma_map_page_attrs(d, p, o, s, r, 0)
```

- 这是一个直接映射的驱动的样子:

```txt
#0  dma_map_page_attrs (dev=0xffff8881007ac8c8, page=0xffffea0004067580, offset=0, size=4096, dir=DMA_FROM_DEVICE, attrs=0) at kernel/dma/mapping.c:145
#1  0xffffffff819dde37 in nvme_setup_prp_simple (dev=0xffff8881019d3000, dev=0xffff8881019d3000, bv=<synthetic pointer>, cmnd=0xffff88814046c928, req=0xffff88814046c800) at include/linux/blk-mq.h:203
#2  nvme_map_data (dev=dev@entry=0xffff8881019d3000, req=req@entry=0xffff88814046c800, cmnd=cmnd@entry=0xffff88814046c928) at drivers/nvme/host/pci.c:840
#3  0xffffffff819de0b0 in nvme_prep_rq (req=<optimized out>, dev=0xffff8881019d3000) at drivers/nvme/host/pci.c:910
#4  nvme_prep_rq (req=<optimized out>, dev=0xffff8881019d3000) at drivers/nvme/host/pci.c:896
#5  nvme_queue_rq (hctx=<optimized out>, bd=0xffffc90000c73bb8) at drivers/nvme/host/pci.c:952
#6  0xffffffff8161412f in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff88814053a600, list=list@entry=0xffffc90000c73c08, nr_budgets=nr_budgets@entry=0) at block/blk-mq.c:1902
#7  0xffffffff8161a423 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff88814053a600) at block/blk-mq-sched.c:306
#8  0xffffffff8161a500 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff88814053a600) at block/blk-mq-sched.c:339
#9  0xffffffff81610f50 in __blk_mq_run_hw_queue (hctx=0xffff88814053a600) at block/blk-mq.c:2020
#10 0xffffffff81611200 in __blk_mq_delay_run_hw_queue (hctx=<optimized out>, async=<optimized out>, msecs=msecs@entry=0) at block/blk-mq.c:2096
#11 0xffffffff81611469 in blk_mq_run_hw_queue (hctx=<optimized out>, async=<optimized out>) at block/blk-mq.c:2144
#12 0xffffffff8161a6c8 in blk_mq_sched_insert_request (rq=rq@entry=0xffff88814046c800, at_head=<optimized out>, at_head@entry=false, run_queue=run_queue@entry=true, async=async@entry=false) at block/blk-mq-sched.c:458
#13 0xffffffff81611a55 in blk_execute_rq (rq=rq@entry=0xffff88814046c800, at_head=at_head@entry=false) at block/blk-mq.c:1278
#14 0xffffffff819cf8a8 in nvme_execute_rq (at_head=false, rq=0xffff88814046c800) at drivers/nvme/host/core.c:1005
#15 __nvme_submit_sync_cmd (q=<optimized out>, cmd=cmd@entry=0xffffc90000c73d08, result=result@entry=0x0 <fixed_percpu_data>, buffer=0xffff8881019d6000, bufflen=bufflen@entry=4096, qid=qid@entry=-1, at_head=0, flags=0) at drivers/nvme/host/core.c:1041
#16 0xffffffff819d2b74 in nvme_submit_sync_cmd (bufflen=4096, buffer=<optimized out>, cmd=<optimized out>, q=<optimized out>) at drivers/nvme/host/core.c:1053
#17 nvme_identify_ctrl (dev=dev@entry=0xffff8881019d3210, id=id@entry=0xffffc90000c73d80) at drivers/nvme/host/core.c:1295
#18 0xffffffff819d50ce in nvme_init_identify (ctrl=0xffff8881019d3210) at drivers/nvme/host/core.c:3081
#19 nvme_init_ctrl_finish (ctrl=ctrl@entry=0xffff8881019d3210) at drivers/nvme/host/core.c:3246
#20 0xffffffff819df784 in nvme_reset_work (work=0xffff8881019d3608) at drivers/nvme/host/pci.c:2870
#21 0xffffffff81123d37 in process_one_work (worker=worker@entry=0xffff88814049ca80, work=0xffff8881019d3608) at kernel/workqueue.c:2289
#22 0xffffffff811242c8 in worker_thread (__worker=0xffff88814049ca80) at kernel/workqueue.c:2436
#23 0xffffffff8112ac73 in kthread (_create=0xffff8881404a0d40) at kernel/kthread.c:376
#24 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#25 0x0000000000000000 in ?? ()
```

- [ ] 即使不是直通，那么存在保护的功能吗?


如果不是直接 iommu_dma_map_page 的，

## iommu_dma_ops

```c
static const struct dma_map_ops iommu_dma_ops = {
	.flags			= DMA_F_PCI_P2PDMA_SUPPORTED,
	.alloc			= iommu_dma_alloc,
	.free			= iommu_dma_free,
	.alloc_pages		= dma_common_alloc_pages,
	.free_pages		= dma_common_free_pages,
	.alloc_noncontiguous	= iommu_dma_alloc_noncontiguous,
	.free_noncontiguous	= iommu_dma_free_noncontiguous,
	.mmap			= iommu_dma_mmap,
	.get_sgtable		= iommu_dma_get_sgtable,
	.map_page		= iommu_dma_map_page,
	.unmap_page		= iommu_dma_unmap_page,
	.map_sg			= iommu_dma_map_sg,
	.unmap_sg		= iommu_dma_unmap_sg,
	.sync_single_for_cpu	= iommu_dma_sync_single_for_cpu,
	.sync_single_for_device	= iommu_dma_sync_single_for_device,
	.sync_sg_for_cpu	= iommu_dma_sync_sg_for_cpu,
	.sync_sg_for_device	= iommu_dma_sync_sg_for_device,
	.map_resource		= iommu_dma_map_resource,
	.unmap_resource		= iommu_dma_unmap_resource,
	.get_merge_boundary	= iommu_dma_get_merge_boundary,
	.opt_mapping_size	= iommu_dma_opt_mapping_size,
};
```

## 如果是给 guest 映射的 DMA 空间，需要同步的修改 IOMMU 才对

# iommu

- [ ] iommu 是可以实现每一个 device 都存在各自的映射，而不是一个虚拟机使用一个映射的。

- 为什么 IOMMU 需要搞出来 iommu group，需要更加详细的解释。

## [ ] vIOMMU 和 virtio iommu 是一个东西吗
- QEMU 的解释 : https://wiki.qemu.org/Features/VT-d

## [ ] IOMMU interrupt remapping 是如何实现的
参考 intel vt-d 标准吧！


# VT-d
How to enable `IRQ_REMAP` in `make menuconfig`:
Device Drivers ==> IOMMU Hareware Support ==> Support for Interrupt Remapping

intel_setup_irq_remapping ==> iommu_set_irq_remapping, setup `Interrupt Remapping Table Address Register` which hold address **IRET** locate [^3] 163,164

好家伙，才意识到 ITRE 其实存在两种格式，remapped interrupt 的格式下，其功能相当于 IO-APIC 的功能，作为设备和 CPU 之间的联系，而 Posted-interrupt 的格式下，就是我们熟悉的内容。
在 Posted-interrupt 格式下，IRET 中间没有目标 CPU 等字段，而是 posted-interrupt descriptor 的地址

### 5 Interrupt Remapping
This chapter discuss architecture and hardware details for interrupt-remapping and interruptposting.These features collectively are called the interrupt remapping architecture.


## my question
- [ ] mmio 可以 remap 吗 ?
- [ ] dma engine 是一个需要的硬件支持放在哪里了 ?
- [ ] 怎么知道一个设备在进行 dma ?
  - [x] 一个真正的物理设备，当需要发起 dma 的时候，进行的 IO 地址本来应该在 pa, 由于 vm 的存在，实际上是在 gpa 上，需要进行在 hpa 上


## [VT-d Posted Interrupts](https://events.static.linuxfound.org/sites/events/files/slides/VT-d%20Posted%20Interrupts-final%20.pdf)
1. Motivation
  - Interrupt virtualization efficiency
  - *Interrupt migration complexity*
  - *Big requirement of host vector for different assigned devices*

- [ ] migration ?
- [ ] host **vector** for different assigned devices ?

![](../img/vt-d-1.png)

Xen Implementation Details:
- Update IRET according to guest’s modification to the interrupt configuration (MSI address, data)
- Interrupt migration during VCPU scheduling

## manual
- https://www.amd.com/en/support/tech-docs/amd-io-virtualization-technology-iommu-specification

## 从 vfio 通往 iommu
