# Intel® Virtualization Technology for Directed I/O

## Introduction

| Term               | Definition                                                                                                                                                 |
|--------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Context            | A hardware representation of state that identifies a device and the domain to which the device is assigned.                                                |
| Device-TLB         | A translation cache at the endpoint device (as opposed to in the platform).                                                                                |
| Domain             | A collection of physical, logical, or virtual resources that are allocated to work together. Used as a generic term for virtual machines, partitions, etc. |
| First-Stage Paging | Paging structures used in scalable-mode for first-stage of DMA address translation.                                                                        |
| IEC                | Interrupt Entry Cache: A translation cache in remapping hardware unit that caches frequently used interruptremapping table entries.                        |
| IOVA               | I/O Virtual Address: Virtual address created by software for use in I/O requests.                                                                          |
| Second-Stage Caches| Translation caches used by remapping hardware units to cache intermediate (non-leaf) entries of the secondstage (SS) paging structures. Depending on the Guest Address Width supported by hardware, these may include SS-PML5 cache, SS-PML4 cache, SS-PDP cache, and SS-PDE cache.
| Source ID| A 16-bit identification number to identify the source of a DMA or interrupt request. For PCI family devices this is the ‘Requester ID’ which consists of PCI Bus number, Device number, and Function number.

- [ ] 是不是一个 context 里面可以含有多个 device 的映射

## 2 Overview

- Interrupt remapping: for supporting isolation and routing of interrupts from devices and external interrupt controllers to appropriate VMs.
- Interrupt posting: for supporting direct delivery of virtual interrupts from devices and external interrupt controllers to virtual processors.

### 2.5.1.3 DMA Remapping Usages by Guests
On hardware implementations supporting two stages of address translations (first-stage translation to remap a virtual address to intermediate (guest) physical address, and second-stage translations to remap a intermediate physical address to machine (host) physical address), a VMM may virtualize guest OS use of first-stage translations without shadowing page-tables, but by configuring hardware to perform nested translation of first and second stages.

### 2.5.2.1 Interrupt Isolation
On Intel® architecture platforms, interrupt requests are identified by the Root-Complex as DWORD sized write transactions targeting an architectural address range (FEEx_xxxxh).
