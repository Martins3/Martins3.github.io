# SIOV

## 参考

https://zhuanlan.zhihu.com/p/591845101

在灵活性方面
1. siov 既能让系统上的应用通过 system call 的形式直接访问其底层的 queue 资源，
2. 也能通过 virtual device interfaces 的形式直通给 vm。
3. 还有就是在 siov 的场景下，两个 VDEV 之间的 Queue 资源是可以 share 的；
4. 另外一个就是良好的兼容性，vmm 可以使用 virtual devevice composition 在不同代的硬件设备上呈现相同的 VDEV 功能，这样即使在部署了不同代的 siov 设备的 host 之间虚拟机也能正常迁移。

intel scalable IOV 主要是以 queue 为粒度来给上层应用提供服务，
为此 siov 在硬件侧引入了一个 ADI(Assignable Device Interfaces)的概念。在功能上它与 VF 相似，
但是不同点在于所有的 ADI 共享同一个 PF 的 BDF 号、pci config_space 和 BAR 空间。但是这个也引入了额外的问题：
1. 一是所有的 ADI share 同一个 BDF 号那在 iommu 这一侧如何区分来自不同的 ADI 的设备的 DMA 请求呢？
2. 另外一个就是一个 pcie 设备即使在 msix 的情况下他支持的最大中断数目也只能到 2048，那如果一个 PF 上支持的 ADI 数量所使用的总的中断数量超过了这个 limit 将如何处理呢？

1. siov 为了解决这个问题给每个 ADI 的 tlp request 额外加上了一个全局唯一的 Process Address Space Identifier 简称 PASID 的标识，
但是如果多个 ADI 直通给同一个 vm 则这些 ADI 的 PASID 是相同的。
在 iommu 侧这种支持有 PASID 的 io 页表翻译模式称为 scalable Mode Address Translation，没有 PASID 的称为 legacy Mode。
如果 iommu 的 Extend Capability Register 的 SMTS( Scalable Mode Translation)位置 1 则表明支持 scalable mode，
在 scalabe mode 模式下 Root Table Address Register(RTADDR_REG)的 TTM 为 01b。
2. 再说问题二，为了解决这个中断 limit 的问题 SIOV 引入了新的中断存储机制叫 IMS(interrupt message storage)，
理论上 IMS 在支持的中断数量是没有上限的，从实现原理上来讲其仍然是 message 格式的中断触发机制，
每个 message 有一个 DWORD 大小的 payload 和 64-bit 的 address。
这些 message 存储在 IMS 的 table 里面，这个 table 可以有全部缓存在硬件上，也可以全部放在 host memory 里面。
ADI 是不能直接操作(比如修改)IMS 的，它只能通过 PF 的 Interrupt Message Generation Logic 来触发中断。


主要参考这个:
- https://static.sched.com/hosted_files/kvmforum2019/5e/Bring%20a%20scalable%20IOV%20capable%20device%20into%20Linux%20-%20KVM%202019.pdf?_gl=1*g1ihtp*_gcl_au*MTc3ODI4MTg1OS4xNzM0NDg5Mjg2

## 问题
1. Extend Capability Register 找一下
2. io 页表不一样
3. DSA 的功能为什么需要 SIOV 的实现?
4. ADI 是如何理解的?

## 细读这个文档
https://www.kernel.org/doc/html/latest//arch/x86/sva.html

*PCIe Address Translation Services* (ATS) along with
*Page Request Interface* (PRI) allow devices to function much the same way as
the CPU handling application page-faults.

Device TLB support - Device requests the IOMMU to lookup an address before
use via Address Translation Service (ATS) requests.  If the mapping exists
but there is no page allocated by the OS, IOMMU hardware returns that no
mapping exists.

Device requests the virtual address to be mapped via Page Request
Interface (PRI). Once the OS has successfully completed the mapping, it
returns the response back to the device. The device requests again for
a translation and continues.

- ATS : 如果 iotlb 不命中，自动 walk page table 来加载
- PRI : 如果 page table 确实，那么触发 page fault 给 CPU


- What is the benefit and micro-ops of ENQCMD instruction?
  - https://stackoverflow.com/questions/72506541/what-is-the-benefit-and-micro-ops-of-enqcmd-instruction

https://stackoverflow.com/questions/72515486/how-does-intel-dsa-data-streaming-accelerators-bring-the-work-descriptor-fro


Creating PCIe SR-IOV type Virtual Functions (VF) is expensive. VFs require
duplicated hardware for PCI config space and interrupts such as MSI-X.
Resources such as interrupts have to be hard partitioned between VFs at
creation time, and cannot scale dynamically on demand. The VFs are not
completely independent from the Physical Function (PF). Most VFs require
some communication and assistance from the PF driver. SIOV, in contrast,
creates a software-defined device where all the configuration and control
aspects are mediated via the slow path. The work submission and completion
happen without any mediation.

SIORV 是如何使用中断


## amd 的 SVA 的支持
https://lore.kernel.org/all/20240418103400.6229-1-vasant.hegde@amd.com/


## 还有 iommufd 的使用需要先了解一下
https://lwn.net/Articles/945588/

## io page fault
drivers/iommu/io-pgfault.c

https://patchwork.kernel.org/project/kvm/patch/20180511190641.23008-8-jean-philippe.brucker@arm.com/


https://lore.kernel.org/lkml/bc7142a1-82d3-43bf-dee2-25f9297e7182@arm.com/T/#r3782e4597655386562ae2f1661f96ebe9c98101c

https://blog.linuxplumbersconf.org/2014/ocw/system/presentations/2253/original/iommuv2.pdf

https://lwn.net/Articles/942793/


1. 请问 intel 的 io page fault 走的是这个路线吗?
- dmar_fault
- dmar_fault_do_one

目前看

```txt
       swapper/0-1       [000] ...1.     0.658947: dmar_set_interrupt <-intel_iommu_init
```


## 先看看 intel 的手册中东西吧
- https://cdrdv2-public.intel.com/671403/intel-scalable-io-virtualization-technical-specification.pdf

|-------|-----------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| ADI   | Assignable Device Interface       | Assignable Device Interface is the unit of assignment for a device.                                                                                                                        |
| PASID | Process Address Space Identifier  | Process Address Space ID and its TLP prefix as specified by the PCI Express Base Specification.                                                                                            |
| RID   | Requester ID                      | Bus/Device/Function number identity for a PCI Express function (PF or VF).
| DWQ   | Dedicated Work Queue              | A work queue that can be assigned to a single address domain at a time.                                                                                                                    |
| SWQ   | Shared Work Queue                 | A work queue that can be assigned to multiple address domains simultaneously.                                                                                                              |
| IMS   | Interrupt Message Storage         | Device-specific interrupt message storage for ADIs.                                                                                                                                        |
| FLR   | Function Level Reset              | Function Level Reset as defined by the PCI Express Base Specification.
| VDCM  | Virtual Device Composition Module | A device-specific component that is responsible for composing a Virtual Device. Typically, this is a software component in the VMM or host driver, but other implementations are possible. |
| ATS   | Address Translation Services      | Ability for device to request and cache address translations. Refer to the PCI Express Specification.
| DMWr  | Deferrable Memory Write           | Refer to the PCI Express ECN for Deferrable Memory Write (DMWr) .                                                                                                                          |

## 大致的描述，没有什么细节

https://mp.weixin.qq.com/s/JPbsFWS51PIsBZR0v66pqQ

SIOV 也没有细节啊

## SVA
<!-- 0d87834b-5c7c-42f5-b8fb-796b63d5bf7f -->

SVA = 设备与某个进程共享同一套虚拟地址空间

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
