## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/unified-memory.html>

## CUDA Unified Memory 章节总结

### 一、主要内容转述

CUDA 的 Unified Memory（统一内存）为 CPU 和 GPU 提供了统一的地址空间，使得同一块内存可以被两者访问，极大地简化了编程模型。根据硬件和系统支持程度，统一内存分为四种范式：

1. **显式托管内存分配（Full support for explicit managed memory allocations）**
2. **软件一致性全支持（Full support for all allocations with software coherence）**
3. **硬件一致性全支持（Full support for all allocations with hardware coherence）**
4. **有限统一内存支持（Limited unified memory support）**

前三种属于“完全支持”，在编程模型和行为上非常相似；第四种适用于 Windows、WSL 或 Tegra 等计算能力低于 6.0 的平台。

#### 1.1 完全支持统一内存的系统

这类系统包括 NVIDIA Grace Hopper（硬件一致性）以及启用了 HMM（Heterogeneous Memory Management）的现代 Linux 系统（内核 6.1.24+/6.2.11+/6.3+，计算能力 7.5+，CUDA 驱动 535+）。在这些系统上，GPU 可以访问主机进程拥有的**任何内存**，包括：

- `malloc`/`new` 分配的堆内存
- `cudaMallocManaged` 分配的托管内存
- 栈变量、文件作用域静态变量、全局变量
- 文件映射内存（file-backed memory，如 `mmap`）
- 通过 IPC 共享的系统分配内存

**关键限制**：
- 全局变量默认声明为 `__host__`，直接在设备代码中访问会导致编译错误，必须传递其地址指针给 kernel。
- 文件映射内存不支持原子访问（除非系统支持 `hostNativeAtomicSupported`）。

#### 1.2 性能调优

统一内存的性能与以下因素密切相关：

- **页大小（Page Size）**：GPU 倾向于使用 2MB 或更大的大页以减少 TLB miss；CPU 默认使用 4KB。应根据访问模式选择合适的虚拟页大小。
- **硬件一致性与软件一致性**：
  - 硬件一致性系统（如 Grace Hopper）使用 CPU 和 GPU 共享的页表，无需页错误即可实现细粒度缓存行一致性。
  - 软件一致性系统使用独立的页表，通过页错误和页面迁移来维护一致性，开销更大。
- **直接访问（Direct Managed Mem Access from Host）**：部分设备支持主机直接访问 GPU 驻留的托管内存（`cudaDevAttrDirectManagedMemAccessFromHost=1`），此时需配合 `cudaMemAdviseSetAccessedBy` 提示。
- **访问计数器迁移（Access Counter Migration）**：硬件一致性系统支持根据访问频率自动迁移页面。
- **避免 CPU 频繁写 GPU 驻留内存**：CPU 写 GPU 内存可能触发缓存未命中和额外迁移，建议由 CPU 写 CPU 驻留内存，GPU 直接读取。
- **异步访问系统内存**：
  - 显式拷贝（`cudaMemcpyAsync`）适合主机和设备都有独立计算任务时重叠传输。
  - 设备直接写 CPU 内存适合 GPU 写完后主机立即读取且设备空闲的场景。
  - 主机直接读 GPU 内存适合主机读取时 GPU 仍有其他独立任务的场景。

#### 1.3 仅支持 CUDA Managed Memory 的设备（计算能力 6.x）

计算能力 6.x 的设备支持 `cudaMallocManaged` 和一致性，但**不支持** GPU 访问系统分配内存（如 `malloc`）。因此，以下小节的内容不适用：
- 统一内存深入示例（系统分配内存）
- 硬件一致性与软件一致性对比
- 原子访问与同步原语
- 访问计数器迁移
- 避免 CPU 频繁写 GPU 驻留内存
- 异步访问系统内存

#### 1.4 Windows、WSL 和 Tegra 上的有限支持

计算能力低于 6.0 或 `concurrentManagedAccess=0` 的平台有以下限制：
- 不支持按需细粒度数据迁移；启动 kernel 时所有托管内存通常会被整体迁移到 GPU。
- 不支持 GPU 内存超配（oversubscription）。
- 不支持 CPU 和 GPU 并发访问托管内存（同时访问会导致段错误）。
- 多 GPU 环境下，托管内存通过 P2P 可见；若 P2P 不可用，Linux 会回退到系统内存，Windows 会回退到映射内存。
- **Stream Associated Unified Memory**：可以通过 `cudaStreamAttachMemAsync` 将托管内存与特定流关联，实现更细粒度的并发控制。未关联的分配对所有流全局可见。

#### 1.5 性能提示（Performance Hints）

性能提示不影响正确性，仅影响性能，适用于所有统一内存分配：

- **数据预取（`cudaMemPrefetchAsync`）**：异步地将数据迁移到指定处理器（GPU 或 CPU），是流序操作。
- **数据使用提示（`cudaMemAdvise`）**：
  - `cudaMemAdviseSetReadMostly`：数据主要是只读的，允许在多个处理器间复制以优化读带宽。
  - `cudaMemAdviseSetPreferredLocation`：设置数据优先驻留的物理位置。
  - `cudaMemAdviseSetAccessedBy`：告知系统某个处理器将频繁访问该数据，可提前建立映射；在硬件一致性系统上还会开启访问计数器迁移。
- **内存丢弃（`cudaMemDiscardBatchAsync` / `cudaMemDiscardAndPrefetchBatchAsync`）**：通知运行时某段内存内容已无用，无需在驱逐或预取时迁移旧数据，适用于内存超配场景。
- **查询属性（`cudaMemRangeGetAttribute` / `cudaMemRangeGetAttributes`）**：查询托管内存范围的 `ReadMostly`、`PreferredLocation`、`AccessedBy`、`LastPrefetchLocation` 等属性。

---

### 二、核心概念

| 概念 | 说明 |
|------|------|
| **Managed Memory** | 通过 `cudaMallocManaged` 或 `__managed__` 分配的内存，自动在 CPU 和 GPU 间迁移。 |
| **System-Allocated Memory** | 通过 `malloc`/`mmap` 等系统调用分配的内存。仅在“完全支持”系统上可被 GPU 直接访问。 |
| **Hardware Coherency** | CPU 和 GPU 共享逻辑页表，缓存行级别一致，无需页错误迁移。 |
| **Software Coherency** | CPU 和 GPU 各自拥有独立页表，通过页错误和页面迁移维护一致性。 |
| **Page Fault** | 当处理器访问未映射到本地物理内存的虚拟页时触发的异常，统一内存利用它实现按需迁移。 |
| **TLB (Translation Lookaside Buffer)** | 页表缓存。大页能减少 TLB miss，但会增加迁移粒度和内存碎片。 |
| **Oversubscription** | 分配的托管内存总量超过单个 GPU 物理内存容量，依赖自动页错误和迁移机制。 |
| **Stream Attachment** | 通过 `cudaStreamAttachMemAsync` 将托管内存绑定到特定流，限制其在其他流上的可见性，实现更细粒度并发。 |

---

### 3. 关键 API / 函数

#### 内存分配与释放
- `cudaMallocManaged(void **devPtr, size_t size, unsigned int flags=0)`
- `cudaFree(void *devPtr)`

#### 性能提示
- `cudaMemPrefetchAsync(const void *devPtr, size_t count, cudaMemLocation location, unsigned int flags, cudaStream_t stream=0)`
- `cudaMemAdvise(const void *devPtr, size_t count, cudaMemoryAdvise advice, cudaMemLocation location)`

#### 流关联
- `cudaStreamAttachMemAsync(cudaStream_t stream, void *ptr, size_t length=0, unsigned int flags=0)`
  - `cudaMemAttachGlobal`（默认）：对所有流可见。
  - `cudaMemAttachHost`：初始对设备不可见，适合多线程安全分配后再绑定到私有流。

#### 内存丢弃
- `cudaMemDiscardBatchAsync(void **dptrs, size_t *sizes, size_t count, unsigned long long flags, cudaStream_t stream)`
- `cudaMemDiscardAndPrefetchBatchAsync(...)`

#### 属性查询
- `cudaMemRangeGetAttribute(void *data, size_t dataSize, cudaMemRangeAttribute attribute, const void *devPtr, size_t count)`
- `cudaMemRangeGetAttributes(...)`

#### 设备属性查询
- `cudaDeviceGetAttribute(int *value, cudaDeviceAttr attr, int device)`
  - `cudaDevAttrConcurrentManagedAccess`：是否支持并发托管访问（GPU page faulting）。
  - `cudaDevAttrPageableMemoryAccess`：是否支持 GPU 访问系统分配内存。
  - `cudaDevAttrDirectManagedMemAccessFromHost`：主机是否能直接访问 GPU 驻留托管内存。
  - `cudaDevAttrHostNativeAtomicSupported`：是否支持主机原生原子操作。

---

### 4. 注意事项和常见陷阱

1. **全局变量默认 `__host__`**
   - 全局作用域变量（未加 `__managed__`）不能在设备代码中直接引用，必须传递其地址指针给 kernel。

2. **`cudaMemcpyKind` 对统一内存是性能提示**
   - 当参数包含托管内存指针时，`cudaMemcpyKind` 的方向提示对性能影响较大。若不确定物理位置，优先使用 `cudaMemcpyDefault`。
   - 避免用 `cudaMemcpy*` 初始化未填充的托管内存缓冲区。
   - 若两个指针都是系统分配内存，优先在 host 端用 `std::memcpy` 或在 device 端启动 kernel 拷贝，而不是 `cudaMemcpy*`。

3. **原子访问限制**
   - 在软件一致性系统上，设备对文件映射内存的原子访问不被支持。
   - 在硬件一致性系统上，主机与设备间的原子操作无需页错误，但仍可能因其他原因触发 fault。

4. **IPC 限制**
   - `cudaMallocManaged` 分配的内存**不能**通过 CUDA IPC 共享。
   - 只有系统分配内存（如 `mmap(MAP_SHARED)`）才能在完全支持系统上通过 IPC 共享给其他进程。
   - IPC 共享内存不能跨不同主机。

5. **`concurrentManagedAccess=0` 平台的并发限制**
   - 当 GPU kernel 正在执行时，CPU 访问**任何**托管内存（即使不是该 kernel 正在使用的变量）都可能导致段错误。
   - 必须显式调用 `cudaDeviceSynchronize()` 后才能从 CPU 访问托管内存。
   - `cudaStreamAttachMemAsync` 可以缓解这一问题，将内存与特定流关联，从而只受该流活动的限制。

6. **大页与迁移的权衡**
   - GPU 偏好 2MB 或更大的大页以减少 TLB miss，但页面迁移的粒度也变大，可能导致延迟尖峰。
   - 建议针对**虚拟页大小**进行调优，而不是硬编码特定硬件的物理页大小。

7. **内存丢弃后的读取**
   - `cudaMemDiscardBatchAsync` 标记为丢弃的内存，若未经过写入或预取就直接读取，其值是**不确定的**。
   - 丢弃操作不能与并发访问或预取重叠，否则行为未定义。
   - 使用丢弃功能要求所有设备 `concurrentManagedAccess != 0`。

8. **`cudaMemAdvise` 不会强制固定数据位置**
   - `cudaMemAdviseSetPreferredLocation` 只是“鼓励”系统保持数据在指定位置，不保证。
   - 当 backing memory 空间不足时，即使设置了访问提示，数据仍可能被迁移。
