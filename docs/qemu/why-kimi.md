# QEMU 为何如此复杂

QEMU 的核心使命是模拟一台完整的计算机，而是一台从 CPU 到固件、从磁盘到网卡、从键盘到 GPU 都可以随意组合的通用机器。这个定位决定了它的复杂性无法被简单压缩。

## 模拟完整的硬件

一台计算机由大量部件组成，QEMU 需要逐一模拟：

- **CPU**：支持 x86、ARM、RISC-V、PowerPC 等二十余种架构。x86  就有大量 CPU 模型（Sandy Bridge、Haswell、Ice Lake 等），每个模型的 cpuid 语义都不同。
- **设备**：可模拟数百个设备，仅设备相关代码就占 QEMU 总代码量的一半左右。
- **存储**：前端模拟 NVMe、IDE、VirtIO 等；后端对接 file、iscsi、nbd、ssh、nvme 等，格式覆盖 raw、qcow2、vmdk、vdi 等数十种。
- **网络**：后端涵盖 user、tap、vhost、bridge、ebpf 等，每种后端的语义和性能特征各异。
- **内存**： 三层模型支持别名、重叠、优先级、热插拔、IOMMU，只为应对真实硬件中同样复杂的地址映射。
- **其他**：USB、GPU、声卡、字符设备、固件（SeaBIOS/UEFI）、ACPI、SMBIOS。

每个部件单独看都只是一个驱动，合在一起就需要统一的抽象层来编排。

## 跨架构与执行模式

QEMU 不仅要模拟多种硬件，还要让它们在彼此之上运行：

- **TCG**（Tiny Code Generator）：将一种架构的指令翻译为另一种。特权指令语义复杂，需要大量 helper 函数；多线程 TCG 还要处理 vCPU 间的同步、TLB 一致性、atomic 指令的 exclusive context。
- **KVM**：利用硬件加速，但不同架构的 KVM 接口仍有差异，需在 `target/$(arch)/kvm` 中维护。
- **用户态模拟**：除了整机的 system mode，还可以只翻译单个用户态程序。

这三条路共享部分基础设施，却在关键路径上各自独立。

## 时间的重量：向后兼容

QEMU 已经发展了二十多年，时间带来了独特的复杂性：

- **机器类型版本**：x86 有 pc-i440fx 和 pc-q35 两大系列，每个系列又从 2.4 迭代到 9.x，保留 deprecated 类型只为保证旧虚拟机可以启动。ARM 的机器列表更长，从树莓派到服务器 BMC 一应俱全。
- **参数演进**：`-hda` 被 `-drive` 取代，`-drive` 又被 `-device` + `-blockdev` 取代；`-net` 被 `-nic` 取代。旧参数不能删，新接口要维护，参数解析的复杂度曾被核心 maintainer Paolo Bonzini 专门拿出来分析。
- **QOM（QEMU Object Model）**：为了让成千上万个设备都能通过统一的对象模型和属性系统来配置，引入了相当重量的抽象。

## 虚拟化的高级操作

如果只是一台静态模拟器，事情会简单很多。但 QEMU 支持在运行时对虚拟机做各种操作：

- **热迁移**：precopy、postcopy、dirty bitmap 同步、xbzrle 压缩、zero page 检测、balloon free-page hint，每一步都是为了在不停机的情况下把内存状态完整搬到另一台主机。
- **快照与克隆**：需要与 block layer 的 qcow2 快照、vmstate 保存、设备状态序列化协同。
- **安全**：盘加密、可信计算、vTPM、SEV/SNP。
- **热插拔**：CPU、内存、PCI 设备都可以在运行时添加或移除，这要求 MemoryRegion 支持动态调整，QDev 支持动态实例化。
- **多进程 QEMU**：将设备模拟拆到独立进程以隔离攻击面，又带来了跨进程通信和状态同步的问题。

## 并发的工程代价

为了性能，QEMU 不得不引入复杂的并发模型：

- **BQL**（Big QEMU Lock）：曾经是全局锁，现在逐步细化，但历史包袱仍在。
- **AioContext / IOThread**：让 block 和网络 IO 可以 offload 到独立线程。
- **协程**：block layer 大量使用协程来避免回调地狱，但协程与线程的交互增加了理解成本。
- **TCG 多线程**：rr（单线程轮转）和 mttcg（每 vCPU 一个线程）两种模式，各自有 halt、kick、exclusive context 等同步机制。
- MemoryRegion / AddressSpace / FlatView

## 总结

QEMU 的复杂性并非设计失误，而是其定位的必然结果：

> 模拟尽可能多的硬件，支持尽可能多的架构组合，保证二十年间的向后兼容，并在运行时完成热迁移、快照、热插拔等高级操作。

每一项单独拿出来都是一个中型项目。QEMU 把它们全部塞进了同一个代码库，于是复杂度呈指数级叠加。对比 Cloud Hypervisor、Firecracker 这类只支持 VirtIO + KVM + 单一架构的精简方案，QEMU 的复杂也就不难理解了。

想看懂某个局部，可以从 `tests/unit/` 入手；想理解整体，则需要承认：它本来就不是一个简单的程序。

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
