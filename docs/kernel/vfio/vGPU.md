## nvgrace-gpu
```txt
History:        #0
Commit:         701ab935859fcfd4a8c8a97f3ee4fb5294a9d481
Author:         Ankit Agrawal <ankita@nvidia.com>
Committer:      Alex Williamson <alex.williamson@redhat.com>
Author Date:    Tue 20 Feb 2024 07:50:55 PM CST
Committer Date: Fri 23 Feb 2024 03:23:37 AM CST

vfio/nvgrace-gpu: Add vfio pci variant module for grace hopper

NVIDIA's upcoming Grace Hopper Superchip provides a PCI-like device
for the on-chip GPU that is the logical OS representation of the
internal proprietary chip-to-chip cache coherent interconnect.

The device is peculiar compared to a real PCI device in that whilst
there is a real 64b PCI BAR1 (comprising region 2 & region 3) on the
device, it is not used to access device memory once the faster
chip-to-chip interconnect is initialized (occurs at the time of host
system boot). The device memory is accessed instead using the chip-to-chip
interconnect that is exposed as a contiguous physically addressable
region on the host. This device memory aperture can be obtained from host
ACPI table using device_property_read_u64(), according to the FW
specification. Since the device memory is cache coherent with the CPU,
it can be mmap into the user VMA with a cacheable mapping using
remap_pfn_range() and used like a regular RAM. The device memory
is not added to the host kernel, but mapped directly as this reduces
memory wastage due to struct pages.

There is also a requirement of a minimum reserved 1G uncached region
(termed as resmem) to support the Multi-Instance GPU (MIG) feature [1].
This is to work around a HW defect. Based on [2], the requisite properties
(uncached, unaligned access) can be achieved through a VM mapping (S1)
of NORMAL_NC and host (S2) mapping with MemAttr[2:0]=0b101. To provide
a different non-cached property to the reserved 1G region, it needs to
be carved out from the device memory and mapped as a separate region
in Qemu VMA with pgprot_writecombine(). pgprot_writecombine() sets the
Qemu VMA page properties (pgprot) as NORMAL_NC.

Provide a VFIO PCI variant driver that adapts the unique device memory
representation into a more standard PCI representation facing userspace.

The variant driver exposes these two regions - the non-cached reserved
(resmem) and the cached rest of the device memory (termed as usemem) as
separate VFIO 64b BAR regions. This is divergent from the baremetal
approach, where the device memory is exposed as a device memory region.
The decision for a different approach was taken in view of the fact that
it would necessiate additional code in Qemu to discover and insert those
regions in the VM IPA, along with the additional VM ACPI DSDT changes to
communicate the device memory region IPA to the VM workloads. Moreover,
this behavior would have to be added to a variety of emulators (beyond
top of tree Qemu) out there desiring grace hopper support.

Since the device implements 64-bit BAR0, the VFIO PCI variant driver
maps the uncached carved out region to the next available PCI BAR (i.e.
comprising of region 2 and 3). The cached device memory aperture is
assigned BAR region 4 and 5. Qemu will then naturally generate a PCI
device in the VM with the uncached aperture reported as BAR2 region,
the cacheable as BAR4. The variant driver provides emulation for these
fake BARs' PCI config space offset registers.

The hardware ensures that the system does not crash when the memory
is accessed with the memory enable turned off. It synthesis ~0 reads
and dropped writes on such access. So there is no need to support the
disablement/enablement of BAR through PCI_COMMAND config space register.

The memory layout on the host looks like the following:
               devmem (memlength)
|--------------------------------------------------|
|-------------cached------------------------|--NC--|
|                                           |
usemem.memphys                              resmem.memphys

PCI BARs need to be aligned to the power-of-2, but the actual memory on the
device may not. A read or write access to the physical address from the
last device PFN up to the next power-of-2 aligned physical address
results in reading ~0 and dropped writes. Note that the GPU device
driver [6] is capable of knowing the exact device memory size through
separate means. The device memory size is primarily kept in the system
ACPI tables for use by the VFIO PCI variant module.

Note that the usemem memory is added by the VM Nvidia device driver [5]
to the VM kernel as memblocks. Hence make the usable memory size memblock
(MEMBLK_SIZE) aligned. This is a hardwired ABI value between the GPU FW and
VFIO driver. The VM device driver make use of the same value for its
calculation to determine USEMEM size.

Currently there is no provision in KVM for a S2 mapping with
MemAttr[2:0]=0b101, but there is an ongoing effort to provide the same [3].
As previously mentioned, resmem is mapped pgprot_writecombine(), that
sets the Qemu VMA page properties (pgprot) as NORMAL_NC. Using the
proposed changes in [3] and [4], KVM marks the region with
MemAttr[2:0]=0b101 in S2.

If the device memory properties are not present, the driver registers the
vfio-pci-core function pointers. Since there are no ACPI memory properties
generated for the VM, the variant driver inside the VM will only use
the vfio-pci-core ops and hence try to map the BARs as non cached. This
is not a problem as the CPUs have FWB enabled which blocks the VM
mapping's ability to override the cacheability set by the host mapping.

This goes along with a qemu series [6] to provides the necessary
implementation of the Grace Hopper Superchip firmware specification so
that the guest operating system can see the correct ACPI modeling for
the coherent GPU device. Verified with the CUDA workload in the VM.

[1] https://www.nvidia.com/en-in/technologies/multi-instance-gpu/
[2] section D8.5.5 of https://developer.arm.com/documentation/ddi0487/latest/
[3] https://lore.kernel.org/all/20240211174705.31992-1-ankita@nvidia.com/
[4] https://lore.kernel.org/all/20230907181459.18145-2-ankita@nvidia.com/
[5] https://github.com/NVIDIA/open-gpu-kernel-modules
[6] https://lore.kernel.org/all/20231203060245.31593-1-ankita@nvidia.com/

Reviewed-by: Kevin Tian <kevin.tian@intel.com>
Reviewed-by: Yishai Hadas <yishaih@nvidia.com>
Reviewed-by: Zhi Wang <zhi.wang.linux@gmail.com>
Signed-off-by: Aniket Agashe <aniketa@nvidia.com>
Signed-off-by: Ankit Agrawal <ankita@nvidia.com>
Link: https://lore.kernel.org/r/20240220115055.23546-4-ankita@nvidia.com
Signed-off-by: Alex Williamson <alex.williamson@redhat.com>
```

```c
static const struct vfio_device_ops nvgrace_gpu_pci_ops = {
	.name		= "nvgrace-gpu-vfio-pci",
	.init		= vfio_pci_core_init_dev,
	.release	= vfio_pci_core_release_dev,
	.open_device	= nvgrace_gpu_open_device,
	.close_device	= nvgrace_gpu_close_device,
	.ioctl		= nvgrace_gpu_ioctl,
	.device_feature	= vfio_pci_core_ioctl_feature,
	.read		= nvgrace_gpu_read,
	.write		= nvgrace_gpu_write,
	.mmap		= nvgrace_gpu_mmap,
	.request	= vfio_pci_core_request,
	.match		= vfio_pci_core_match,
	.match_token_uuid = vfio_pci_core_match_token_uuid,
	.bind_iommufd	= vfio_iommufd_physical_bind,
	.unbind_iommufd	= vfio_iommufd_physical_unbind,
	.attach_ioas	= vfio_iommufd_physical_attach_ioas,
	.detach_ioas	= vfio_iommufd_physical_detach_ioas,
};
```

drivers/vfio/pci/nvgrace-gpu/Kconfig 中去掉 COMPILE_TEST 才可以编译这个模块
```txt
# SPDX-License-Identifier: GPL-2.0-only
config NVGRACE_GPU_VFIO_PCI
	tristate "VFIO support for the GPU in the NVIDIA Grace Hopper Superchip"
	depends on ARM64 || (COMPILE_TEST && 64BIT)
	select VFIO_PCI_CORE
	help
	  VFIO support for the GPU in the NVIDIA Grace Hopper Superchip is
	  required to assign the GPU device to userspace using KVM/qemu/etc.

	  If you don't know what to do here, say N.
```

## vGPU 的诡异之处

1. 观察一个事情，无论如何切分 vGPU，vGPU 的 vf 数量都是一样的吗?
2. 既然 vGPU 还是

切分为  2Q * 24
```txt
51:00.0 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:00.4 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:00.5 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:00.6 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:00.7 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:01.0 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:01.1 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:01.2 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:01.3 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:01.4 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:01.5 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:01.6 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:01.7 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:02.0 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:02.1 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:02.2 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:02.3 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:02.4 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:02.5 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:02.6 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:02.7 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:03.0 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:03.1 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:03.2 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:03.3 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:03.4 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:03.5 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:03.6 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:03.7 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:04.0 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:04.1 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:04.2 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
51:04.3 3D controller: NVIDIA Corporation GA102GL [A40] (rev a1)
```
基本是切分为 24 ，但是还是有 32 个 vGPU

ls -la /sys/class/mdio_bus
```txt
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:00.4 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:00.4
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:00.5 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:00.5
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:00.6 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:00.6
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:00.7 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:00.7
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:01.0 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:01.0
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:01.1 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:01.1
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:01.2 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:01.2
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:01.3 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:01.3
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:01.4 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:01.4
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:01.5 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:01.5
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:01.6 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:01.6
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:01.7 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:01.7
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:02.0 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:02.0
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:02.1 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:02.1
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:02.2 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:02.2
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:02.3 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:02.3
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:02.4 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:02.4
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:02.5 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:02.5
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:02.6 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:02.6
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:02.7 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:02.7
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:03.0 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:03.0
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:03.1 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:03.1
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:03.2 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:03.2
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:03.3 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:03.3
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:03.4 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:03.4
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:03.5 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:03.5
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:03.6 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:03.6
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:03.7 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:03.7
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:04.0 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:04.0
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:04.1 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:04.1
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:04.2 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:04.2
lrwxrwxrwx  1 root root 0 Dec 19 10:57 0000:51:04.3 -> ../../devices/pci0000:50/0000:50:01.2/0000:51:04.3
```

居然，sriov 切分出来的 mdev 自己继续是一个单独的 VFIO group 的:
```txt
IOMMU Group 138:
        51:00.4 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 139:
        51:00.5 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 140:
        51:00.6 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 141:
        51:00.7 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 142:
        51:01.0 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 143:
        51:01.1 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 144:
        51:01.2 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 145:
        51:01.3 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 146:
        51:01.4 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 147:
        51:01.5 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 148:
        51:01.6 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 149:
        51:01.7 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 150:
        51:02.0 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 151:
        51:02.1 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 152:
        51:02.2 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 153:
        51:02.3 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 154:
        51:02.4 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 155:
        51:02.5 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 156:
        51:02.6 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 157:
        51:02.7 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 158:
        51:03.0 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 159:
        51:03.1 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 160:
        51:03.2 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 161:
        51:03.3 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 162:
        51:03.4 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 163:
        51:03.5 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 164:
        51:03.6 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 165:
        51:03.7 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 166:
        51:04.0 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 167:
        51:04.1 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 168:
        51:04.2 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 169:
        51:04.3 3D controller [0302]: NVIDIA Corporation GA102GL [A40] [10de:2235] (rev a1)
IOMMU Group 170:
2d6f1e40-daa4-4d54-861f-51ee76900379
IOMMU Group 171:
35dcceda-72d3-4d01-a249-7a69c1aaf66f
IOMMU Group 172:
a8dedc61-7887-4adf-bb48-7a5351aad166
IOMMU Group 173:
ef5949a7-8cb4-4136-b963-c053004aad14
IOMMU Group 174:
09ae5db1-c6e6-484e-a4f6-afbc24835492
IOMMU Group 175:
83849653-d8e5-4d2f-bd42-4da6e5e6e0cc
IOMMU Group 176:
4b5a56e6-22e1-4881-b6b2-c712b4ab62a6
IOMMU Group 177:
7b9799b7-10ff-4c6d-bc9a-b7b48a7c3b28
IOMMU Group 178:
2adabfef-11b5-4d9a-9a5c-eec2d7850874
IOMMU Group 179:
593fa765-c96a-43a5-b7f0-19f243bdae88
IOMMU Group 180:
5efbed8d-582c-4d5d-9490-1ebeebf1d382
IOMMU Group 181:
62f5b3b5-927c-4dcb-bf47-1e6a66995dff
IOMMU Group 182:
83ef77da-94a5-4daf-87c5-be5bb657dd20
IOMMU Group 183:
90b20513-014e-401d-8eaa-27a90ea926d4
IOMMU Group 184:
86e45837-8886-4b64-8e77-e36195f3ba02
IOMMU Group 185:
bcb5ad18-485e-4de9-b20a-79546556b582
IOMMU Group 186:
594e0e2e-efe3-4d4e-88dc-1cbf52323596
IOMMU Group 187:
0d8408b9-7652-4c37-a5b5-0c384f804821
IOMMU Group 188:
19e981ac-a000-4b22-a995-e94db6f86e08
IOMMU Group 189:
72e3413d-fe07-4a04-866c-359b0a1591f5
IOMMU Group 190:
0d0a82f7-da98-4f0c-a9a4-79ea200a7a94
IOMMU Group 191:
977fa6e0-97f7-43ad-854a-1b0368c8ae81
IOMMU Group 192:
78f10607-ef84-4a9c-9d74-6dddc64011b7
IOMMU Group 193:
59ae2496-a7c8-4ffc-ab38-4ac8169625f1
```

## vGPU 常用命令

systemctl status nvidia-vgpu-mgr.service
systemctl status nvidia-vgpud.service

journalctl -u nvidia-vgpu-mgr.service
journalctl -u nvidia-vgpud.service

## nvidia-vgpu-vfio-pci
这到底是想做什么
- V1:  https://lore.kernel.org/kvm/20240924164151.GJ9417@nvidia.com/T/#m219a9503d570772306defac223545c006d43cb0a
- V2: https://lore.kernel.org/all/20250903221111.3866249-1-zhiw@nvidia.com/#r

git clone --depth=1 --branch zhi/vgpu-rfc-v2 https://github.com/zhiwang-nvidia/linux.git zhiwang-nvidia

结果这个新定义出来了一个东西，之前的是 "nvgrace-gpu-vfio-pci"
```c
static const struct vfio_device_ops nvidia_vgpu_vfio_ops = {
	.name           = "nvidia-vgpu-vfio-pci",
	.init		= vfio_pci_core_init_dev,
	.release	= vfio_pci_core_release_dev,
	.open_device    = nvidia_vgpu_vfio_open_device,
	.close_device   = nvidia_vgpu_vfio_close_device,
	.ioctl          = nvidia_vgpu_vfio_ioctl,
	.device_feature = vfio_pci_core_ioctl_feature,
	.read           = nvidia_vgpu_vfio_read,
	.write          = nvidia_vgpu_vfio_write,
	.mmap           = nvidia_vgpu_vfio_mmap,
	.request	= vfio_pci_core_request,
	.match		= vfio_pci_core_match,
	.bind_iommufd	= vfio_iommufd_physical_bind,
	.unbind_iommufd	= vfio_iommufd_physical_unbind,
	.attach_ioas	= vfio_iommufd_physical_attach_ioas,
	.detach_ioas	= vfio_iommufd_physical_detach_ioas,
};
```

那么第一个问题就是，当使用 vGPU 的时候，iommu 的映射是发生在什么地方?


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
