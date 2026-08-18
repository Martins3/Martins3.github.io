## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/virtual-memory-management.html>

## 虚拟内存管理 (Virtual Memory Management)

### 章节概述

传统的 CUDA 内存分配（如 `cudaMalloc`）返回一个 GPU 内存地址，开发者可以通过
`cudaEnablePeerAccess`
开启对等访问，使不同设备的内核能够访问同一块数据。然而，这种做法会把**所有**已有和未来的
`cudaMalloc`
分配都映射到目标对等设备上。在多数场景下，应用只需共享少量分配，却不得不为全部映射付出运行时开销，且这种方式很难扩展到多节点环境。

为此，CUDA
提供了**虚拟内存管理（VMM）API**，让开发者像操作系统管理虚拟内存那样，显式地、细粒度地控制
GPU 内存：

- **Reserve（预留）**：先预留一段连续的虚拟地址（VA）范围，不分配物理内存；
- **Map（映射）**：将实际分配的物理内存（Physical Memory）绑定到预留的 VA 上；
- **Access（授权）**：显式设置哪些设备可以访问这段映射。

本章介绍的所有 API 都要求系统支持 UVA（Unified Virtual Addressing）。

### 为什么需要提供 share memory 出来

通过将虚拟地址与物理内存解耦，VMM 能够减少内存碎片、支持动态扩容、实现高效的多
GPU / 多进程内存共享，并为构建自定义分配器、缓存管理系统（如大模型 KV
Cache）提供底层能力。 NCCL、NVShmem 等库也在内部使用 VMM。

这个比较有意思，把原文也放到这里来:

Developers can benefit from the VMM API in several key ways:

- Fine-grained control over virtual and physical memory management, allowing
  allocation and mapping of non-contiguous physical memory chunks to contiguous
  virtual address spaces. This helps reduce GPU memory fragmentation and improve
  memory utilization, especially for large workloads like deep neural network
  training.
- Efficient memory allocation and deallocation by separating the reservation of
  virtual address space from the physical memory allocation. Developers can
  reserve large virtual memory regions and map physical memory on demand without
  costly memory copies or reallocations, leading to performance improvements in
  dynamic data structures and variable-sized memory allocations.
- The ability to grow GPU memory allocations dynamically without needing to copy
  and reallocate all data, similar to how realloc or std::vector works in CPU
  memory management. This supports more flexible and efficient GPU memory use
  patterns.
- Enhancements to developer productivity and application performance by
  providing low-level APIs that allow building sophisticated memory allocators
  and cache management systems, such as dynamically managing key-value caches in
  large language models, improving throughput and latency.

- The CUDA VMM API is highly valuable in distributed multi-GPU settings as it
  enables efficient memory sharing and access across multiple GPUs. By
  decoupling virtual addresses from physical memory, the API allows developers
  to create a unified virtual address space where data can be dynamically mapped
  to different GPUs. This optimizes memory usage and reduces data transfer
  overhead. For instance, NVIDIA’s libraries like NCCL, and NVShmem actively
  uses VMM.

### 核心概念

| 概念                        | 说明                                                                                                                                                       |
| --------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Fabric Memory**           | 通过 NVLink / NVSwitch 高速互联结构访问的内存。CUDA 12.4+ 支持 `CU_MEM_HANDLE_TYPE_FABRIC`，可在单节点或多节点（需 IMEX daemon）间共享内存。               |
| **Memory Handles**          | 物理内存分配的不透明标识符。用于引用、导出和导入内存，实现跨进程/跨设备共享，而不暴露直接指针。                                                            |
| **IMEX Channels**           | internode memory exchange 的缩写，是多节点 GPU 通信的安全隔离机制。驱动通过字符设备 `nvidia-caps-imex-channels` 提供通道，需管理员创建节点（如 `mknod`）。 |
| **Unicast Memory Access**   | 单播访问：将物理内存显式映射到某个特定设备的 VA 空间，一对一授权。                                                                                         |
| **Multicast Memory Access** | 多播访问：通过 `cuMulticastCreate` 创建多播对象，使同一块物理内存的副本映射到多个设备的 VA 空间，利用 NVLink SHARP 加速广播与归约。                        |
| **Virtual Aliasing**        | 对同一块物理内存创建多个不同的 VA 映射（多个 "proxy"）。默认情况下，通过不同 proxy 同时写入是**不一致**的，需要显式同步或 `fence.proxy.alias`。            |

---

### 关键 API 与工作流程

1. 查询设备支持 在使用 VMM 前，必须先查询设备是否支持相关特性：

- `CU_DEVICE_ATTRIBUTE_VIRTUAL_MEMORY_MANAGEMENT_SUPPORTED`：VMM 基础支持
- `CU_DEVICE_ATTRIBUTE_HANDLE_TYPE_FABRIC_SUPPORTED`：Fabric Memory 支持
- `CU_DEVICE_ATTRIBUTE_MULTICAST_SUPPORTED`：多播对象支持
- `CU_DEVICE_ATTRIBUTE_GENERIC_COMPRESSION_SUPPORTED`：可压缩内存支持

2. 分配物理内存 使用 `cuMemCreate`
   分配物理内存。它不返回可直接访问的指针，而是返回
   `CUmemGenericAllocationHandle`。 分配前需通过 `cuMemGetAllocationGranularity`
   获取粒度并对齐大小。

```cpp
CUmemAllocationProp prop = {};
prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
prop.location.id = device;
// prop.requestedHandleType = ...  // 若用于 IPC，需指定句柄类型
```

3. 导出与共享句柄（IPC）

- **OS-Specific Handle**：如 Linux 的
  `CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR`，仅限同一节点内的进程间共享。
- **Fabric Handle**：`CU_MEM_HANDLE_TYPE_FABRIC`，支持单节点和多节点，但要求启用
  IMEX 通道。

通过 `cuMemExportToShareableHandle` 导出，经 socket、MPI 等发送给对端；对端用
`cuMemImportFromShareableHandle` 导入。

4. 保留虚拟地址范围

`cuMemAddressReserve` 预留一段连续 VA，此时没有物理内存 backing。概念上类似于
Linux 的 `mmap` 或 Windows 的
`VirtualAlloc`。返回的地址可以跨越多个设备的物理内存，形成统一的连续视图。

5. 映射物理内存到 VA `cuMemMap` 将 `cuMemCreate` 或
   `cuMemImportFromShareableHandle` 得到的物理内存与预留的 VA 关联。同一个 VA
   区域可以在 unmap 后重复映射不同的物理内存。

6. 设置访问权限 **仅调用 `cuMemMap` 并不会让地址可被内核访问**。 必须显式调用
   `cuMemSetAccess` 为特定设备授予 `CU_MEM_ACCESS_FLAGS_PROT_READWRITE` 等权限，
   否则访问会导致程序崩溃。

7. 释放资源 释放必须遵循严格顺序：
8. `cuMemUnmap(ptr, size)` — 解除 VA 与物理内存的映射；
9. `cuMemRelease(handle)` — 释放物理内存；
10. `cuMemAddressFree(ptr, size)` — 归还虚拟地址范围。

必须先 `cuMemUnmap`，再 `cuMemRelease`，最后
`cuMemAddressFree`。顺序错误可能导致未定义行为或资源泄漏。

> 对于 OS-specific IPC，还需额外关闭导出的文件描述符（如 `close(fd)`）。

### 高级配置

#### 可压缩内存 (Compressible Memory)

在支持计算数据压缩的设备上，可将
`CUmemAllocationProp::allocFlags::compressionType` 设为
`CU_MEM_ALLOCATION_COMP_GENERIC`。分配后可通过
`cuMemGetAllocationPropertiesFromHandle` 验证是否实际获得压缩属性。

#### Virtual Aliasing 的一致性

VMM 支持对同一物理内存创建多个 VA 映射（Virtual Aliasing）。**默认规则**：

- 在一个内核中通过不同 proxy 先写后读是**未定义行为**；
- 若两个内核/操作通过流或事件保证单调顺序（写操作完成后再读），则是安全的；
- 若必须在同一内核中通过不同 proxy 访问同一块内存，需在两次访问间插入 PTX 指令
  `fence.proxy.alias`。

#### OS 特定句柄细节

- **Linux**：使用 `CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR`，导出为 `int`
  类型的 fd，可通过 Unix Domain Socket 的 `SCM_RIGHTS` 辅助数据传递。
- **Windows**：使用 `CU_MEM_HANDLE_TYPE_WIN32`，需在分配属性中提供
  `LPSECURITY_ATTRIBUTES`，以定义导出内存的安全范围。

### 注意事项与常见陷阱

4. **区分句柄类型适用范围**
   - OS-specific handle 只能在**同一操作系统**的单节点内使用；
   - Fabric handle 可用于多节点，但需要 IMEX 通道和 NVIDIA IMEX daemon；

5. **Virtual Aliasing 不是透明缓存别名** 不要假设两个不同 VA
   指向同一块物理内存就能自动保持一致。
   要么通过流/事件在操作边界处同步，要么在内核内使用 `fence.proxy.alias`。

6. **多播对象有严格的设备添加顺序** 使用 `cuMulticastAddDevice`
   将所有参与设备加入多播团队后，才能调用 `cuMulticastBindMem` 绑定物理内存。

7. **VMM 仅提供 Driver API** VMM 是非常底层的接口，没有直接的 Runtime API
   封装。使用时需要包含 `<cuda.h>` 并链接 `-lcuda`。

## human 这个算是 GPU 中相当有意思的部分

1. 似乎提到这个问题的关键在于发，我们需要管理多个 GPU
2. 如果可以直接使用 cudamalloc ，岂不是，在虚拟地址空间有一个 malloc ，在物理地址空间也会有一个 malloc 吗？
3. UVA 在这里需要体现一下吧
