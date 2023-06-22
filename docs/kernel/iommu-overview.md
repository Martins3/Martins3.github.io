# IOMMU

- https://luohao-brian.gitbooks.io/interrupt-virtualization/content/vt-d-interrupt-remapping-fen-xi.html

## 问题
- 有什么方法提前获取到 vfio cgroup 的数值
- [ ] 必须深入理解 dma 的 coherent 的行为

## 看看 iommu 的用户态接口

### debugfs

### [ ] sys
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

才知道 intel 是默认不打开的 iommu 的(和 distrubution 有关?)，打开之后，可以搜索到如下的内容:
```txt
[    0.678026] iommu: Default domain type: Translated
[    0.678027] iommu: DMA domain TLB invalidation policy: lazy mode
[    0.736751] pci 0000:00:02.0: Adding to iommu group 0
[    0.737027] pci 0000:00:00.0: Adding to iommu group 1
[    0.737031] pci 0000:00:01.0: Adding to iommu group 2
[    0.737035] pci 0000:00:06.0: Adding to iommu group 3
[    0.737038] pci 0000:00:0a.0: Adding to iommu group 4
[    0.737042] pci 0000:00:0e.0: Adding to iommu group 5
[    0.737048] pci 0000:00:14.0: Adding to iommu group 6
[    0.737052] pci 0000:00:14.2: Adding to iommu group 6
[    0.737055] pci 0000:00:14.3: Adding to iommu group 7
[    0.737063] pci 0000:00:15.0: Adding to iommu group 8
[    0.737066] pci 0000:00:15.1: Adding to iommu group 8
[    0.737069] pci 0000:00:15.2: Adding to iommu group 8
[    0.737073] pci 0000:00:16.0: Adding to iommu group 9
[    0.737076] pci 0000:00:17.0: Adding to iommu group 10
[    0.737089] pci 0000:00:1a.0: Adding to iommu group 11
[    0.737095] pci 0000:00:1c.0: Adding to iommu group 12
[    0.737100] pci 0000:00:1c.2: Adding to iommu group 13
[    0.737109] pci 0000:00:1f.0: Adding to iommu group 14
[    0.737113] pci 0000:00:1f.3: Adding to iommu group 14
[    0.737116] pci 0000:00:1f.4: Adding to iommu group 14
[    0.737120] pci 0000:00:1f.5: Adding to iommu group 14
[    0.737127] pci 0000:01:00.0: Adding to iommu group 15
[    0.737131] pci 0000:01:00.1: Adding to iommu group 15
[    0.737135] pci 0000:02:00.0: Adding to iommu group 16
[    0.737148] pci 0000:03:00.0: Adding to iommu group 17
[    0.737152] pci 0000:05:00.0: Adding to iommu group 18
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

## 禁用 iommu

网卡无法启动了。
```txt
[    2.517363] ------------[ cut here ]------------
[    2.517364] mt7921e 0000:04:00.0: DMA addr 0x0000000117eaf000+4096 overflow (mask ffffffff, bus limit 0).
[    2.517367] WARNING: CPU: 19 PID: 1054 at kernel/dma/direct.h:103 dma_map_page_attrs+0x242/0x280
[    2.517371] Modules linked in: ip6_tables xt_conntrack ip6t_rpfilter ipt_rpfilter xt_pkttype xt_LOG nf_log_syslog xt_tcpudp snd_sof_amd_rembrandt mt7921e(+) snd_sof_amd_renoir snd_sof_amd_acp snd_hda_codec_realtek mt7921_common snd_sof_pci snd_sof_xtensa_dsp mt76_connac_lib snd_hda_codec_generic mousedev joydev hid_multitouch nft_compat ledtrig_audio snd_hda_codec_hdmi snd_sof mt76 snd_hda_intel snd_sof_utils snd_intel_dspcfg snd_soc_core snd_intel_sdw_acpi uvcvideo mac80211 snd_hda_codec snd_compress videobuf2_vmalloc ac97_bus uvc videobuf2_memops snd_pcm_dmaengine videobuf2_v4l2 snd_pci_ps snd_rpl_pci_acp6x snd_hda_core snd_acp_pci videodev nf_tables snd_pci_acp6x edac_mce_amd snd_hwdep intel_rapl_msr snd_pci_acp5x edac_core nls_iso8859_1 snd_rn_pci_acp3x snd_pcm intel_rapl_common snd_acp_config nls_cp437 crc32_pclmul videobuf2_common snd_soc_acpi vfat polyval_clmulni sch_fq_codel nfnetlink polyval_generic fat cfg80211 mc hid_generic snd_timer snd_pci_acp3x gf128mul ghash_clmulni_intel btusb snd i2c_hid_acpi r8169
[    2.517389]  sha512_ssse3 sha512_generic sp5100_tco btrtl wdat_wdt ucsi_acpi aesni_intel btbcm typec_ucsi btintel realtek crypto_simd i2c_hid mdio_devres ideapad_laptop wmi_bmof cryptd btmtk rapl typec watchdog libphy k10temp sparse_keymap i2c_piix4 soundcore libarc4 nvidia_drm(PO) roles battery platform_profile bluetooth drm_kms_helper evdev tpm_crb input_leds tiny_power_button led_class tpm_tis tpm_tis_core syscopyarea mac_hid sysfillrect sysimgblt i2c_designware_platform acpi_cpufreq i2c_designware_core ac button usbhid serio_raw ecdh_generic nvidia_modeset(PO) hid rfkill ecc video libaes wmi nvidia_uvm(PO) nvidia(PO) ctr loop xt_nat br_netfilter veth tap macvlan bridge stp llc openvswitch nsh nf_conncount nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 libcrc32c tun kvm_amd ccp kvm drm fuse deflate backlight efi_pstore i2c_core configfs efivarfs tpm rng_core dmi_sysfs ip_tables x_tables autofs4 ext4 crc32c_generic crc16 mbcache jbd2 xhci_pci xhci_pci_renesas xhci_hcd atkbd nvme libps2 vivaldi_fmap usbcore
[    2.517415]  nvme_core t10_pi crc32c_intel crc64_rocksoft crc64 crc_t10dif usb_common crct10dif_generic crct10dif_pclmul i8042 crct10dif_common rtc_cmos serio dm_mod dax vfio_pci vfio_pci_core irqbypass vfio_iommu_type1 vfio iommufd
[    2.517420] CPU: 19 PID: 1054 Comm: (udev-worker) Tainted: P           O       6.3.5 #1-NixOS
[    2.517422] Hardware name: LENOVO 82WM/LNVNB161216, BIOS LPCN41WW 05/24/2023
[    2.517422] RIP: 0010:dma_map_page_attrs+0x242/0x280
[    2.517424] Code: 8b 5d 00 48 89 ef e8 bd 68 63 00 4d 89 e9 4d 89 e0 48 89 da 41 56 48 89 c6 48 c7 c7 40 05 5c a5 48 8d 4c 24 10 e8 9e 8e f4 ff <0f> 0b 58 e9 7b ff ff ff 48 c7 44 24 08 ff ff ff ff 4d 85 c0 0f 84
[    2.517425] RSP: 0018:ffff998b43b3ba08 EFLAGS: 00010282
[    2.517426] RAX: 0000000000000000 RBX: ffff8b6784b5cae0 RCX: 0000000000000027
[    2.517426] RDX: ffff8b766dae14c8 RSI: 0000000000000001 RDI: ffff8b766dae14c0
[    2.517427] RBP: ffff8b6784c070c8 R08: 0000000000000000 R09: ffff998b43b3b8b0
[    2.517427] R10: 0000000000000003 R11: ffffffffa5d38868 R12: 0000000000001000
[    2.517428] R13: 00000000ffffffff R14: 0000000000000000 R15: ffff8b67888f3908
[    2.517428] FS:  00007f14a4b49c40(0000) GS:ffff8b766dac0000(0000) knlGS:0000000000000000
[    2.517429] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[    2.517429] CR2: 00007ff0bce47dfc CR3: 0000000106cf6000 CR4: 0000000000750ee0
[    2.517430] PKRU: 55555554
[    2.517430] Call Trace:
[    2.517431]  <TASK>
[    2.517431]  ? dma_map_page_attrs+0x242/0x280
[    2.517433]  ? __warn+0x81/0x130
[    2.517436]  ? dma_map_page_attrs+0x242/0x280
[    2.517437]  ? report_bug+0x171/0x1a0
[    2.517439]  ? handle_bug+0x41/0x70
[    2.517441]  ? exc_invalid_op+0x17/0x70
[    2.517442]  ? asm_exc_invalid_op+0x1a/0x20
[    2.517445]  ? dma_map_page_attrs+0x242/0x280
[    2.517446]  ? dma_map_page_attrs+0x242/0x280
[    2.517447]  page_pool_dma_map+0x30/0x70
[    2.517449]  __page_pool_alloc_pages_slow+0x133/0x3d0
[    2.517451]  ? sched_clock_cpu+0xf2/0x190
[    2.517453]  page_pool_alloc_frag+0x14c/0x1d0
[    2.517455]  mt76_dma_rx_fill.isra.0+0x132/0x390 [mt76]
[    2.517460]  ? __pfx_mt7921_poll_rx+0x10/0x10 [mt7921e]
[    2.517463]  ? napi_kthread_create+0x48/0x90
[    2.517465]  ? __pfx_mt7921_poll_rx+0x10/0x10 [mt7921e]
[    2.517467]  mt76_dma_init+0x110/0x140 [mt76]
[    2.517471]  ? __pfx_mt7921_rr+0x10/0x10 [mt7921e]
[    2.517473]  mt7921_dma_init+0x18b/0x1f0 [mt7921e]
[    2.517475]  mt7921_pci_probe+0x387/0x430 [mt7921e]
[    2.517477]  local_pci_probe+0x3f/0x90
[    2.517480]  pci_device_probe+0xc3/0x240
[    2.517482]  ? sysfs_do_create_link_sd+0x6e/0xe0
[    2.517484]  really_probe+0x19f/0x400
[    2.517487]  ? __pfx___driver_attach+0x10/0x10
[    2.517488]  __driver_probe_device+0x78/0x160
[    2.517489]  driver_probe_device+0x1f/0x90
[    2.517490]  __driver_attach+0xd2/0x1c0
[    2.517491]  bus_for_each_dev+0x85/0xd0
[    2.517493]  bus_add_driver+0x116/0x220
[    2.517494]  driver_register+0x59/0x100
[    2.517495]  ? __pfx_init_module+0x10/0x10 [mt7921e]
[    2.517497]  do_one_initcall+0x5a/0x240
[    2.517500]  do_init_module+0x4a/0x200
[    2.517501]  __do_sys_init_module+0x17f/0x1b0
[    2.517503]  do_syscall_64+0x3b/0x90
[    2.517505]  entry_SYSCALL_64_after_hwframe+0x72/0xdc
[    2.517507] RIP: 0033:0x7f14a4722b5e
[    2.517531] Code: 48 8b 0d bd e2 0c 00 f7 d8 64 89 01 48 83 c8 ff c3 66 2e 0f 1f 84 00 00 00 00 00 90 f3 0f 1e fa 49 89 ca b8 af 00 00 00 0f 05 <48> 3d 01 f0 ff ff 73 01 c3 48 8b 0d 8a e2 0c 00 f7 d8 64 89 01 48
[    2.517531] RSP: 002b:00007ffda4e74928 EFLAGS: 00000246 ORIG_RAX: 00000000000000af
[    2.517532] RAX: ffffffffffffffda RBX: 00005616c16d0060 RCX: 00007f14a4722b5e
[    2.517532] RDX: 00007f14a4cecb0d RSI: 0000000000019bc8 RDI: 00005616c1ee5290
[    2.517533] RBP: 00007f14a4cecb0d R08: 0000000000000007 R09: 00005616c16ccd20
[    2.517533] R10: 0000000000000005 R11: 0000000000000246 R12: 00005616c1ee5290
[    2.517533] R13: 0000000000000000 R14: 00005616c16bcfe0 R15: 0000000000000000
[    2.517534]  </TASK>
[    2.517534] ---[ end trace 0000000000000000 ]---
[    2.517778] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.592359] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.667874] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.739891] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.780523] Bluetooth: BNEP (Ethernet Emulation) ver 1.3
[    2.780527] Bluetooth: BNEP socket layer initialized
[    2.781084] Bluetooth: MGMT ver 1.22
[    2.782399] NET: Registered PF_ALG protocol family
[    2.812364] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.887399] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.962403] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    3.037498] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    3.112444] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    3.152283] Generic FE-GE Realtek PHY r8169-0-700:00: attached PHY driver (mii_bus:phy_addr=r8169-0-700:00, irq=MAC)
[    3.187426] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    3.262368] mt7921e 0000:04:00.0: hardware init failed
[    3.308670] r8169 0000:07:00.0 enp7s0: No native access to PCI extended config space, falling back to CSI
[    3.330433] memfd_create() without MFD_EXEC nor MFD_NOEXEC_SEAL, pid=1582 'systemd'
[    3.331709] r8169 0000:07:00.0 enp7s0: Link is Down
[    3.463048] systemd-journald[944]: /var/log/journal/c4d5dffd3fce45ff9046f3017148dd83/user-1000.journal: Monotonic clock jumped backwards relative to last journal entry, rotating.
[    3.562042] ACPI Warning: \_SB.NPCF._DSM: Argument #4 type mismatch - Found [Buffer], ACPI requires [Package] (20221020/nsarguments-61)
[    3.562087] ACPI Warning: \_SB.PCI0.GPP0.PEGP._DSM: Argument #4 type mismatch - Found [Buffer], ACPI requires [Package] (20221020/nsarguments-61)
```

## [ ] 请问 iommu=off 和 amd_iommu=off 有啥区别?

只是关掉 amd_iommu 的时候:
```txt
[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_nslabs
32768

[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_used
2801

[root@nixos:/sys/kernel/debug/swiotlb]# cat /proc/cmdline
initrd=\efi\nixos\i7ijxg4186dp43vzkavh7gb1vq8dp916-initrd-linux-6.3.5-initrd.efi init=/nix/store/17yknrp3kdnzar0mxymvyca8ppjcgcd8-nixos-system-nixos-23.05
.563.70f7275b32f/init transparent_hugepage=always intel_iommu=on amd_iommu=off ftrace=function_graph ftrace_filter=iommu_setup_dma_ops fsck.mode=force fsc
k.repair=yes loglevel=4
```

应该是缺少 iommu 导致的
```txt
[  613.669612] vfio-pci: probe of 0000:07:00.0 failed with error -22
```

## [ ] 为什么要强调是通过 cmdline 设置的

```txt
[    0.498658] iommu: Default domain type: Passthrough (set via kernel command line)
```

## [ ] 当 amd_iommu=off 的时候，采用

设置为
```txt
[    0.495931] iommu: Default domain type: Translated
[    0.495931] iommu: DMA domain TLB invalidation policy: lazy mode
```

## iommu=pt 导致可能使用上 swiotlb

```txt
[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_nslabs
32768

[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_used
2877

[root@nixos:/sys/kernel/debug/swiotlb]# cat /proc/cmdline
initrd=\efi\nixos\zkf5v40bsl6lzvgbi9a4mnf5wm55aqh6-initrd-linux-6.3.5-initrd.efi init=/nix/store/np8xhzj01vrhld37vhcd0zlgan1v9kwc-nixos-system-nixos-23.05.563.70f7275b32f/init
 transparent_hugepage=always intel_iommu=on iommu=pt fsck.mode=force fsck.repair=yes loglevel=4
```

参数处理函数: `iommu_setup`

代码上的证据: `iommu_def_domain_type = IOMMU_DOMAIN_IDENTITY`

- dma_map_page_attrs


## 分析下 typo 结构

```txt
[    0.497832] pci 0000:00:01.0: Adding to iommu group 0
[    0.497836] pci 0000:00:01.1: Adding to iommu group 0
[    0.497840] pci 0000:00:01.2: Adding to iommu group 0
[    0.497848] pci 0000:00:02.0: Adding to iommu group 1
[    0.497852] pci 0000:00:02.1: Adding to iommu group 1
[    0.497856] pci 0000:00:02.2: Adding to iommu group 1
[    0.497867] pci 0000:00:03.0: Adding to iommu group 2
[    0.497871] pci 0000:00:03.1: Adding to iommu group 2
[    0.497875] pci 0000:00:03.2: Adding to iommu group 2
[    0.497879] pci 0000:00:03.3: Adding to iommu group 2
[    0.497884] pci 0000:00:04.0: Adding to iommu group 3
[    0.497893] pci 0000:00:08.0: Adding to iommu group 4
[    0.497897] pci 0000:00:08.1: Adding to iommu group 4
[    0.497901] pci 0000:00:08.3: Adding to iommu group 4
[    0.497909] pci 0000:00:14.0: Adding to iommu group 5
[    0.497914] pci 0000:00:14.3: Adding to iommu group 5
[    0.497931] pci 0000:00:18.0: Adding to iommu group 6
[    0.497935] pci 0000:00:18.1: Adding to iommu group 6
[    0.497940] pci 0000:00:18.2: Adding to iommu group 6
[    0.497944] pci 0000:00:18.3: Adding to iommu group 6
[    0.497949] pci 0000:00:18.4: Adding to iommu group 6
[    0.497953] pci 0000:00:18.5: Adding to iommu group 6
[    0.497957] pci 0000:00:18.6: Adding to iommu group 6
[    0.497961] pci 0000:00:18.7: Adding to iommu group 6
[    0.497964] pci 0000:01:00.0: Adding to iommu group 0
[    0.497965] pci 0000:01:00.1: Adding to iommu group 0
[    0.497967] pci 0000:02:00.0: Adding to iommu group 0
[    0.497969] pci 0000:03:00.0: Adding to iommu group 1
[    0.497972] pci 0000:04:00.0: Adding to iommu group 1
[    0.497975] pci 0000:07:00.0: Adding to iommu group 2
[    0.497977] pci 0000:08:00.0: Adding to iommu group 4
[    0.497980] pci 0000:08:00.2: Adding to iommu group 4
[    0.497982] pci 0000:08:00.3: Adding to iommu group 4
[    0.497984] pci 0000:08:00.4: Adding to iommu group 4
[    0.497986] pci 0000:08:00.5: Adding to iommu group 4
[    0.497988] pci 0000:08:00.6: Adding to iommu group 4
[    0.497990] pci 0000:09:00.0: Adding to iommu group 4
```

想不到 iommu=pt 之后，根本没有人走 iommu_dma_unmap_page

nvme 也是走的这个路线:
```txt
dma_map_page_attrs+5
nvme_prep_rq.part.0+1460
nvme_queue_rq+123
__blk_mq_try_issue_directly+348
blk_mq_try_issue_directly+22
blk_mq_submit_bio+1199
submit_bio_noacct_nocheck+818
__blkdev_direct_IO_async+260
blkdev_read_iter+295
aio_read+306
io_submit_one+1451
__x64_sys_io_submit+173
do_syscall_64+59
entry_SYSCALL_64_after_hwframe+114
```

```sh
sudo bpftrace -e 'kfunc:dma_map_page_attrs {  @[args->dev->dma_ops]=count() }'
```
得到：
```txt
@[0x0]: 562818
```

看来的确是所有的 guest 都没有的。
