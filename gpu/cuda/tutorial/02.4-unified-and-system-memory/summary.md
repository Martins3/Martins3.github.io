## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/understanding-memory.html>

## 统一内存与系统内存概述

异构系统包含多个物理内存：主机 CPU 拥有独立的 DRAM，而系统中的每块 GPU
也有自己的 DRAM。当数据位于访问它的处理器的本地内存中时，性能最佳。CUDA
提供了显式管理内存放置的
API，但也提供了一系列特性来简化数据在不同物理内存之间的分配、放置和迁移。

本章主要介绍以下内容：

- **统一虚拟地址空间（Unified Virtual Address
  Space）**：在单个进程内，主机内存和所有 GPU 的全局内存共享同一个虚拟地址空间。
- **统一内存（Unified Memory）**：一种托管内存（managed memory），可被 CPU 和
  GPU 自动迁移访问。
- **页锁定主机内存（Page-Locked Host
  Memory）**：不可换页的主机内存，是异步传输和映射内存的基础。
- **映射内存（Mapped Memory）**：让 GPU
  核函数直接访问主机内存的机制（不同于统一内存）。

---

## 统一虚拟地址空间

在单个操作系统进程内，所有主机内存和系统中所有 GPU
的全局内存共享**同一个虚拟地址空间**。无论内存是通过 CUDA API（如
`cudaMalloc`、`cudaMallocHost`）还是系统 API（如
`new`、`malloc`、`mmap`）分配的，都位于该统一虚拟地址空间内。

这意味着：

- 可以通过 `cudaPointerGetAttributes()` 根据指针值判断内存实际位于 CPU 还是某块
  GPU。
- `cudaMemcpy*()` 的 `cudaMemcpyKind` 参数可以设置为
  `cudaMemcpyDefault`，由运行时根据指针自动推断拷贝方向。

---

## 统一内存

统一内存是一种 CUDA 内存特性，它允许称为**托管内存（managed memory）**的分配被
CPU 或 GPU 上的代码访问。几乎所有支持 CUDA 的系统都可用统一内存。

### 托管内存的分配方式

- **显式分配**：
  - `cudaMallocManaged`
  - `cudaMallocFromPoolAsync`（需将 pool 的 `allocType` 设为
    `cudaMemAllocationTypeManaged`）
  - 使用 `__managed__` 修饰符的全局变量
- **隐式托管**：在支持 **HMM** 或 **ATS**
  的系统上，所有系统分配的内存都自动成为托管内存，无需特殊分配 API。

### 四种统一内存范式

统一内存的实际行为和功能取决于操作系统、Linux 内核版本、GPU 硬件以及 CPU-GPU
互联方式。可通过查询以下设备属性来判断当前系统属于哪种范式：

| 属性                                                | 含义                                                                                 |
| --------------------------------------------------- | ------------------------------------------------------------------------------------ |
| `cudaDevAttrConcurrentManagedAccess`                | `1` 表示完整统一内存支持；`0` 表示有限支持                                           |
| `cudaDevAttrPageableMemoryAccess`                   | `1` 表示所有系统内存都是完整支持的托管内存；`0` 表示仅显式分配的托管内存才受完整支持 |
| `cudaDevAttrPageableMemoryAccessUsesHostPageTables` | `1` 表示使用硬件一致性；`0` 表示使用软件一致性                                       |

根据这三个属性的组合，统一内存分为四种范式：

1. **有限统一内存（Limited Unified
   Memory）**：`concurrentManagedAccess == 0`。常见于 Windows、WSL 和部分 Tegra
   设备。
2. **仅 CUDA 显式分配的完整统一内存**：`concurrentManagedAccess == 1` 且
   `pageableMemoryAccess == 0`。常见于 Linux 上的旧款 GPU 或未启用 HMM 的系统。
3. **软件一致性的完整统一内存（HMM）**：`concurrentManagedAccess == 1`、`pageableMemoryAccess == 1`、`pageableMemoryAccessUsesHostPageTables == 0`。需要
   Linux 内核 >= 6.1.24 / 6.2.11 / 6.3。
4. **硬件一致性的完整统一内存（ATS）**：`concurrentManagedAccess == 1`、`pageableMemoryAccess == 1`、`pageableMemoryAccessUsesHostPageTables == 1`。仅在配备
   NVLink Chip-to-Chip (C2C) 的 Grace Hopper / Grace Blackwell 等系统上可用。

### 完整统一内存的行为特点

在支持完整统一内存的系统上：

- 托管内存通常在**首次被访问（first-touch）**的处理器内存空间中分配。
- 当另一处理器访问时，数据通常会自动**迁移**到该处理器本地。
- 迁移粒度为**页（page）**（软件一致性）或**缓存行（cache
  line）**（硬件一致性）。
- 允许**超量订阅（oversubscription）**：可分配超过 GPU 物理容量的托管内存。
- 若 `pageableMemoryAccess == 1`，通过 `mmap` 创建的文件-backed
  内存也享有完整统一内存支持。

### 硬件一致性（ATS）

在 Grace Hopper / Grace Blackwell 等使用 NVLink C2C
互联的系统上，**地址转换服务（Address Translation Services, ATS）**
提供硬件一致性。除了完整统一内存的所有特性外，ATS 还支持：

- 使用 `cudaMalloc` 分配的设备内存可被 CPU
  直接访问（`cudaDevAttrDirectManagedMemAccessFromHost == 1`）。
- CPU 与 GPU
  之间支持原生原子操作（`cudaDevAttrHostNativeAtomicSupported == 1`）。
- 硬件一致性通常比软件一致性性能更好。

> 若系统同时支持 ATS 和 HMM，ATS 会自动启用，HMM 被禁用。

### 软件一致性（HMM）

**异构内存管理（Heterogeneous Memory Management, HMM）** 是 Linux
内核的一项特性，可为 PCIe 连接的 GPU
提供软件一致性的完整统一内存支持。可通过以下命令检查是否启用：

```bash
nvidia-smi -q | grep Addressing
```

输出 `Addressing Mode : HMM` 即表示已启用。

### 有限统一内存的行为特点

在 Windows、WSL 和部分 Tegra 上，统一内存功能受限：

- 托管内存首先在 CPU 物理内存中分配。
- 迁移粒度大于虚拟内存页。
- GPU 开始执行后，CPU **不得**访问托管内存。
- GPU 同步后，内存才迁回 CPU。
- **不允许**超量订阅。
- 仅 CUDA 显式分配的托管内存才受支持。

### 内存建议与预取

程序员可以通过 API 向驱动提供提示，以优化统一内存的行为：

- **`cudaMemAdvise`**：为特定分配指定属性，影响其放置位置以及被其他设备访问时是否迁移。
- **`cudaMemPrefetchAsync`**：建议驱动异步将指定内存迁移到目标位置。常见用法是在内核启动前预取数据，使数据传输与内核计算重叠。

---

## 页锁定主机内存

页锁定内存（Page-locked Memory，又称 Pinned
Memory）是主机上不可被操作系统换页或搬迁的内存。通过 `cudaMallocHost`
分配的内存即为页锁定内存。

页锁定内存的作用：

- 是**异步拷贝**（`cudaMemcpyAsync`）的必要条件。
- 可提升**同步拷贝**的性能。
- 可被**映射到 GPU**，供核函数直接访问。

### 相关 API

| API                | 作用                                                               |
| ------------------ | ------------------------------------------------------------------ |
| `cudaMallocHost`   | 分配页锁定主机内存                                                 |
| `cudaHostAlloc`    | 默认行为同 `cudaMallocHost`，但可通过 flags 指定额外参数（如映射） |
| `cudaFreeHost`     | 释放由上述 API 分配的内存                                          |
| `cudaHostRegister` | 将已通过 `malloc` / `mmap` 等分配的现有内存页锁定                  |

> 注意：`cudaHostRegister` 在非 I/O 一致的 Tegra 设备上不受支持。

---

## 映射内存

映射内存（Mapped Memory）是一种让 GPU
核函数直接访问主机内存的机制，**不同于统一内存**。

- 在支持 **HMM** 或 **ATS** 的系统上，所有主机内存都可被 GPU
  直接用主机指针访问。
- 在不支持 HMM/ATS 的系统上，主机内存必须被**页锁定并映射**到 GPU
  地址空间后才能被核函数访问。


## 原子性
Atomic functions (see Atomic Functions) operating on mapped host memory are not atomic from the point of view of the host or other GPUs.

CUDA runtime requires that 1-byte, 2-byte, 4-byte, 8-byte, and 16-byte naturally aligned loads and stores to host memory initiated from the device are preserved as single accesses from the point of view of the host and other devices. On some platforms, atomics to memory may be broken by the hardware into separate load and store operations. These component load and store operations have the same requirements on preservation of naturally aligned accesses. The CUDA runtime does not support a PCI Express bus topology where a PCI Express bridge splits 8-byte naturally aligned operations and NVIDIA is not aware of any topology that splits 16-byte naturally aligned operations.

## 总结与注意事项

- **Linux + HMM/ATS**：所有系统分配的内存自动成为托管内存，统一内存功能最完整。
- **Linux（无 HMM/ATS）/ Tegra / Windows**：托管内存必须通过
  `cudaMallocManaged`、`cudaMallocFromPoolAsync` 或 `__managed__`
  显式分配；Windows 和 Tegra 上功能受限。
- **ATS 系统**：`cudaMalloc` 分配的设备内存可被 CPU
  直接访问，且支持原生原子操作。
- **映射内存**可作为补充手段，但不应替代统一内存或显式内存管理来满足核函数的主要内存需求，
因其性能远低于设备本地内存。

- 编程时建议先查询设备属性，了解当前系统的统一内存范式，再选择合适的内存管理策略。

---

## 关键 API 速查

核心关键 API 的结果:

| API                 | 用途                                                                             |
|---------------------|----------------------------------------------------------------------------------|
| `cudaMalloc`        | 直接分配显存                                                                     |
| `cudaMallocManaged` | 分配内存，但是自动迁移                                                           |
| `cudaMallocHost`    | 分配页锁定主机内存（自动映射）                                                   |
| `cudaHostAlloc`     | 分配页锁定主机内存，支持 flags（如 `cudaHostAllocMapped`），也就是更加底层的 API |

其他辅助扩展 api
| API                        | 用途                                                       |
| -------------------------- | ---------------------------------------------------------- |
| `cudaMemAdvise`            | 为统一内存提供放置与迁移提示                               |
| `cudaMallocFromPoolAsync`  | 从指定内存池中异步分配（可设为 managed 类型）              |
| `cudaMemPrefetchAsync`     | 异步预取统一内存到指定设备                                 |
| `cudaFreeHost`             | 释放页锁定主机内存                                         |
| `cudaHostRegister`         | 页锁定并映射已有的主机内存                                 |
| `cudaHostUnregister`       | 解除页锁定                                                 |
| `cudaHostGetDevicePointer` | 获取已映射主机内存对应的设备指针                           |
| `cudaPointerGetAttributes` | 查询指针属性（判断内存位置）                               |
| `cudaMemcpyDefault`        | 让 `cudaMemcpy` 自动推断拷贝方向                           |
| `cudaDeviceGetAttribute`   | 查询设备属性（用于判断统一内存范式）                       |


## 章节概述

### 统一虚拟地址空间（Unified Virtual Address Space, UVAS）

- 单个 OS 进程内，CPU 内存与所有 GPU 的全局内存共享同一个虚拟地址空间。
- 无论使用 CUDA API（`cudaMalloc`、`cudaMallocHost`）还是系统
  API（`malloc`、`new`、`mmap`）分配的内存都位于该空间内。
- 可通过 `cudaPointerGetAttributes()` 根据指针值判断内存实际位于 CPU 还是某个
  GPU。
- `cudaMemcpy*` 的 `cudaMemcpyKind` 可设为
  `cudaMemcpyDefault`，由运行时自动推断拷贝方向。

### 统一内存（Unified Memory）

- **托管内存（Managed Memory）**：可由 CPU 或 GPU 代码访问的内存。
- **显式分配方式**：
  - `cudaMallocManaged`
  - `cudaMallocFromPoolAsync`（`allocType = cudaMemAllocationTypeManaged`）
  - 全局变量使用 `__managed__` 修饰符
- **隐式托管**：在支持 HMM 或 ATS 的 Linux 系统上，所有系统分配的内存（包括
  `mmap` 的文件-backed 内存）都自动成为托管内存，无需特殊 API。

### 统一内存的四种范式

通过查询设备属性确定当前系统属于哪种范式：

| 范式                        | 关键属性条件                                                                                            | 说明                                                                                                             |
| --------------------------- | ------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| **Limited**                 | `cudaDevAttrConcurrentManagedAccess = 0`                                                                | Windows、WSL、部分 Tegra。仅显式 CUDA 托管内存可用，不能 oversubscription，CPU 在 GPU 活动期间不可访问托管内存。 |
| **Full (显式分配)**         | `ConcurrentManagedAccess = 1`，`PageableMemoryAccess = 0`                                               | 仅 `cudaMallocManaged` 等显式分配的内存享有完整统一内存特性。                                                    |
| **Full + 软件一致性 (HMM)** | `ConcurrentManagedAccess = 1`，`PageableMemoryAccess = 1`，`PageableMemoryAccessUsesHostPageTables = 0` | Linux Kernel >= 6.1.24/6.2.11/6.3。所有系统内存均为托管内存，一致性由软件维护。                                  |
| **Full + 硬件一致性 (ATS)** | 上述前两个为 1，且 `PageableMemoryAccessUsesHostPageTables = 1`                                         | Grace Hopper / Grace Blackwell + NVLink C2C。硬件维护一致性，支持 CPU 直接访问 GPU 显存分配、原生原子操作。      |

### 页锁定主机内存（Page-Locked / Pinned Memory）

- 传统 `malloc/new/mmap` 分配的内存是可换页的（pageable），异步 CPU-GPU
  拷贝要求内存必须页锁定。
- 页锁定内存还能提升同步拷贝性能，并可被映射到 GPU 地址空间供核函数直接访问。
- 关键
  API：`cudaMallocHost`、`cudaHostAlloc`、`cudaFreeHost`、`cudaHostRegister`。

### 映射内存（Mapped Memory）

- 使 GPU 核函数能够直接通过指针访问主机物理内存（零拷贝，zero-copy）。
- **与统一内存的区别**：
  - 映射内存始终驻留在 CPU 物理内存中，GPU 访问需经过
    PCIe/NVLink，延迟高、带宽低。
  - 统一内存通常会根据访问处理器自动迁移到对应物理内存，迁移后可享受本地内存带宽。
  - 映射内存不保证支持所有访问类型（如跨系统的原子操作），而统一内存保证支持。

## 注意事项与常见陷阱

1. **Windows / WSL / Tegra 上的 Limited 支持**
   - 托管内存首先在 CPU 物理内存中分配，GPU 开始执行时整块迁移到
     GPU，同步后再迁回。
   - **CPU 在 GPU 活跃期间不得访问托管内存**，否则会导致未定义行为。
   - **不支持 oversubscription**（分配的托管内存不能超过 GPU 物理显存）。
   - 只有显式 CUDA 托管内存才是统一内存。

2. **Full Unified Memory 的行为预期**
   - 托管内存通常在**首次 touch** 的处理器内存空间中分配。
   - 当另一个处理器访问时，通常以**页（软件一致性）或缓存行（硬件一致性）**粒度迁移。
   - **支持 oversubscription**：可分配超过 GPU 物理显存的托管内存。

3. **HMM 与 ATS 的互斥**
   - 若系统同时支持 HMM 和 ATS，驱动会自动禁用 HMM 并启用 ATS，因为 ATS
     能力更强（硬件一致性、CPU 访问 GPU 显存、原生原子操作）。
   - 可通过 `nvidia-smi -q | grep Addressing` 查看当前是否为 HMM 模式。

4. **映射内存的性能误区**
   - 映射内存（包括 `cudaMallocHost` 和
     `cudaHostRegister`）虽然能在核函数里直接访问，但数据始终留在主机内存。
   - 大量或频繁的 GPU 访问会受限于互联带宽（PCIe / NVLink
     C2C），**不应将其作为高性能统一内存或显式管理的替代品**。
   - 在映射内存上执行的原子操作，对主机或其他 GPU 而言**不是原子的**。

5. **cudaHostRegister 的使用限制**
   - 在没有 ATS/HMM 的系统上，`cudaHostRegister`
     映射后的内存**不能直接用原始主机指针**在核函数中访问，必须通过
     `cudaHostGetDevicePointer` 获取设备指针。
   - 非 I/O 相干 Tegra 设备不支持 `cudaHostRegister()`。
   - `cudaHostRegister` 要求传入的指针按主机页大小对齐，普通 `malloc`
     可能不满足对齐要求，应使用 `posix_memalign` 或类似接口。

## human

1. 那么，unified memory 和 system memory 是两个东西？
