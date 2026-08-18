## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/unified-memory.html>


本章是 NVIDIA CUDA Programming Guide 中关于 **Unified Memory（统一内存）** 的完整技术说明。Unified Memory 的核心理念是让 CPU 和 GPU 共享同一个虚拟地址空间，使得开发者无需手动管理 `cudaMemcpy` 之类的显式数据传输，从而降低编程复杂度。然而，不同硬件平台和操作系统对 Unified Memory 的支持程度差异很大，因此本章按照支持级别将系统分为四大类，并分别阐述了它们的行为差异、编程模型、性能调优手段以及限制条件。

四类支持范式分别是：

1. **显式托管内存分配（cudaMallocManaged）的完全支持**
2. **所有分配均支持软件一致性（software coherence）**
3. **所有分配均支持硬件一致性（hardware coherence）**
4. **有限的 Unified Memory 支持**

前三种属于"完全支持"范畴，行为相似；第四种主要针对 Windows、WSL 以及低算力 Tegra 设备，限制较多。

---

## 背景与要解决的问题

在传统的 CUDA 编程模型中，开发者需要显式区分主机内存（host memory）和设备内存（device memory）。数据在两端之间传输必须通过 `cudaMemcpy` 或其异步变体完成。这种模式虽然性能可控，但带来了显著的编程负担：

- 需要手动追踪每个指针的物理驻留位置
- 容易出现"CPU 修改了数据但忘记拷回 GPU"之类的逻辑错误
- 对于指针嵌套、动态数据结构等场景，显式拷贝几乎不可行

Unified Memory 通过让操作系统和 CUDA 驱动共同维护一套页表映射机制，使得同一块内存在逻辑上同时对 CPU 和 GPU 可见。驱动会根据访问模式自动迁移数据页（page fault + migration），或者在硬件一致性系统中直接通过共享页表实现零拷贝访问。这样开发者就可以像写普通 C++ 程序一样分配内存，而 GPU 内核可以直接读写这些指针。

---

## 核心概念与术语

### 虚拟页（Virtual Page）与物理页（Physical Page）

所有支持 Unified Memory 的系统都使用虚拟地址空间。虚拟页是操作系统追踪的固定大小连续虚拟内存块，可以映射到物理内存。物理页则是处理器 MMU 实际支持的映射单位。

- x86_64 CPU 默认物理页大小为 **4 KiB**
- Arm CPU 支持 4 KiB / 16 KiB / 32 KiB / 64 KiB
- NVIDIA GPU 偏好 **2 MiB** 或更大的物理页

**TLB（Translation Lookaside Buffer）** 是页表缓存。TLB miss 在 GPU 上代价远高于 CPU，因此如果 GPU 线程频繁随机访问大范围 Unified Memory，使用更大的虚拟页（如 2 MiB huge page）能显著降低 TLB miss。但大页也会导致更粗粒度的数据迁移，可能在并发访问场景下引入更高的延迟峰值。

### 硬件一致性（Hardware Coherency）vs 软件一致性（Software Coherency）

- **硬件一致性系统**：CPU 和 GPU 使用逻辑上合并的页表（combined page table），例如 NVIDIA Grace Hopper 平台。这类系统不需要页错误来模拟一致性，缓存一致性在 cache-line 粒度上维护。多个处理器同时访问同一页内不同 cache line 不会互相干扰。
- **软件一致性系统**：CPU 和 GPU 各自拥有独立的页表。当一方访问驻留在另一方物理内存中的页时，会触发页错误，驱动需要：
  1. 使当前拥有方的页表项失效
  2. 为请求方创建/更新有效的页表项
  3. 将物理页迁移到请求方的内存中

硬件一致性在以下场景性能优势明显：
- CPU 和 GPU 同时原子更新同一地址
- CPU 与 GPU 线程之间的细粒度信号同步

### 设备属性查询

判断当前平台支持哪些 Unified Memory 特性，核心属性包括：

| 属性 | 含义 |
|------|------|
| `cudaDevAttrManagedMemory` | 设备是否支持 `cudaMallocManaged` |
| `cudaDevAttrConcurrentManagedAccess` | GPU 是否支持对 managed memory 的页错误（允许 CPU/GPU 并发访问） |
| `cudaDevAttrPageableMemoryAccess` | GPU 是否能直接访问系统分配内存（malloc/new） |
| `cudaDevAttrDirectManagedMemAccessFromHost` | 主机是否能直接访问 GPU 驻留的 managed memory 而不触发迁移 |
| `cudaDevAttrHostNativeAtomicSupported` | 设备是否支持对主机内存的硬件加速原子访问 |

---

## API / 机制详解

### 1. 内存分配 API

#### `cudaMallocManaged(void** devPtr, size_t size)`

- **作用**：分配对 CPU 和 GPU 同时可见的托管内存
- **特点**：分配的内存在物理上初始位置通常取决于 first-touch（首次由谁访问）。如果 GPU 先访问，页可能驻留在 GPU 显存中；如果 CPU 先初始化，页通常驻留在主机内存中
- **兼容性**：从 compute capability 3.x 起即有基础支持，但 6.x 以下不支持并发访问，Windows/有限平台也有额外限制
- **释放**：使用 `cudaFree()` 释放

#### 系统分配器（malloc / new / mmap）

- **前提**：仅在 `pageableMemoryAccess = 1` 的系统上，GPU 才能直接访问 `malloc` 分配的内存
- **适用平台**：Linux HMM（kernel >= 6.1.24/6.2.11/6.3+, driver >= 535 + Open Kernel Modules）或 Grace Hopper 等硬件一致性平台
- **注意**：在 sm_61 (Pascal) 等 `pageableMemoryAccess = 0` 的设备上，将 `malloc` 指针传给 GPU 内核会导致非法访问

#### `cudaMemAttachHost` 标志

- `cudaMallocManaged(..., cudaMemAttachHost)` 创建的分配默认对所有 GPU 流不可见，仅对 CPU 可见。需要调用 `cudaStreamAttachMemAsync` 将其关联到特定流后，GPU 内核才能访问
- 用途：多线程程序中避免新分配的 managed memory 被其他线程正在运行的内核误访问

### 2. 流关联机制：`cudaStreamAttachMemAsync`

```cpp
cudaError_t cudaStreamAttachMemAsync(cudaStream_t stream, void* ptr,
                                     size_t length=0, unsigned int flags=0);
```

- **作用**：将一段 managed memory 与指定流绑定。绑定后，只有该流中的内核才能访问这段内存，从而允许 CPU 在该流完成工作后安全访问数据，即使其他流仍有内核在运行
- **关键语义**：如果分配不与任何流关联（默认状态），它对**所有** GPU 流全局可见，此时只要 GPU 上有任何内核运行，CPU 就不能触碰这段数据（在 `concurrentManagedAccess = 0` 的平台上会 segfault）
- **典型用法**：多线程 CPU 程序中，每个线程创建私有流，将自己的 managed allocation 通过 `cudaStreamAttachMemAsync` 绑定到该流，实现线程级任务并行

### 3. 性能提示 API

#### `cudaMemPrefetchAsync`

```cpp
cudaError_t cudaMemPrefetchAsync(const void* devPtr, size_t count,
                                 cudaMemLocation location, unsigned int flags,
                                 cudaStream_t stream=0);
```

- **作用**：异步地将数据预迁移到指定处理器附近，减少运行时的页错误开销
- **语义**：迁移操作在该 stream 中排队，等前面所有操作完成后才开始，并在后续操作开始前完成。因此数据可在 GPU 使用前被静默搬移到位
- **使用场景**：数据初始化在 CPU 上完成，但接下来主要由 GPU 读取，可在启动内核前先 `PrefetchAsync` 到 GPU；反之亦然
- **与 `cudaMemcpy` 的区别**：`PrefetchAsync` 只是迁移建议，不会导致数据重复；原位置数据失效后由新位置接管

#### `cudaMemAdvise`

```cpp
cudaError_t cudaMemAdvise(const void* devPtr, size_t count,
                          enum cudaMemoryAdvise advice,
                          struct cudaMemLocation location);
```

提供了三种核心提示，可单独使用或组合使用：

**`cudaMemAdviseSetReadMostly`**
- 暗示数据以读为主、极少写入。系统可在多个处理器上同时建立只读副本，换取读带宽提升，但写入时需要使多份副本失效，写带宽会下降
- 适用于查找表、常量数据等场景

**`cudaMemAdviseSetPreferredLocation`**
- 设置数据的首选物理位置（某 GPU 或 CPU）。系统会尽量将数据保留在该位置，但不保证。可被 `cudaMemPrefetchAsync` 覆盖
- 例如：设置 CPU 为首选位置，然后让 GPU 直接读取，避免 CPU 频繁写入 GPU 驻留内存导致的 cache 失效问题

**`cudaMemAdviseSetAccessedBy`**
- 提示某处理器会频繁访问该数据，让驱动提前建立页表映射，避免首次访问时的映射建立开销
- 在硬件一致性系统上，该提示还会**开启 access counter migration**
- 若与 `cudaMemAdviseSetPreferredLocation` 配合使用，可实现更精细的驻留控制

#### `cudaMemDiscardBatchAsync` 与 `cudaMemDiscardAndPrefetchBatchAsync`

- **作用**：批量标记 managed memory 的某些范围"内容已无用"，驱动在后续预取或页驱逐时无需迁移旧数据
- **使用场景**：循环计算中，上一轮结果不再需要，下一轮会完全覆盖。丢弃旧数据可避免驱动做无意义的迁移，显著提升 oversubscription 场景性能
- **重要限制**：
  - 丢弃后读取会得到不确定值
  - 丢弃操作可通过后续写入或 `cudaMemPrefetchAsync` 撤销
  - 不能与并发访问/预取同时进行（会导致 UB）
  - 要求所有设备的 `concurrentManagedAccess != 0`

### 4. 查询 API：`cudaMemRangeGetAttribute`

允许程序查询 managed memory 上设置的属性，包括：
- `cudaMemRangeAttributeReadMostly`
- `cudaMemRangeAttributePreferredLocation`
- `cudaMemRangeAttributeAccessedBy`
- `cudaMemRangeAttributeLastPrefetchLocation`
- 以及对应的 Type/Id 细分查询

用途：在复杂的多阶段流水线中，程序可以根据数据的当前属性决定下一步是走 CPU staging 还是 GPU direct access。

### 5. `cudaMemcpy` / `cudaMemset` 与 Unified Memory 的交互

- `cudaMemcpy*` 的 `cudaMemcpyKind` 参数在涉及 Unified Memory 时是一个**性能提示**，而非严格语义约束
- **建议**：
  - 若已知物理位置，使用准确的 `cudaMemcpyKind`
  - 不确定时，优先使用 `cudaMemcpyDefault` 而非错误的显式方向
  - 总是用已初始化（populated）的缓冲区做拷贝，不要用 `cudaMemcpy*` 来初始化内存
  - 如果两个指针都是系统分配内存，避免使用 `cudaMemcpy*`，改用 CPU `std::memcpy` 或启动 kernel 做 device-side copy

---

## 典型工作流程 / 调用顺序

### 基础使用流程（cudaMallocManaged）

```cpp
int* data;
cudaMallocManaged(&data, N * sizeof(int));
// CPU 初始化
data[i] = ...;
// GPU 使用
kernel<<<...>>>(data);
cudaDeviceSynchronize();
// CPU 读取结果
printf("%d\n", data[0]);
cudaFree(data);
```

### 带性能提示的完整流水线

```cpp
// 1. 分配
cudaMallocManaged(&data, size);

// 2. CPU 初始化
init_data(data, size);

// 3. 预取到 GPU
cudaMemLocation dev_loc = {cudaMemLocationTypeDevice, gpuId};
cudaMemPrefetchAsync(data, size, dev_loc, 0, stream);

// 4. GPU 计算
kernel<<<..., stream>>>(data);

// 5. 预取回 CPU
cudaMemLocation host_loc = {cudaMemLocationTypeHost, 0};
cudaMemPrefetchAsync(data, size, host_loc, 0, stream);
cudaStreamSynchronize(stream);

// 6. CPU 使用后处理
use_data(data, size);

cudaFree(data);
```

### 多线程流关联模式

```cpp
// 每个 CPU 线程拥有独立流
void run_task(...) {
    cudaStream_t stream;
    cudaStreamCreate(&stream);

    int* data;
    cudaMallocManaged(&data, length, cudaMemAttachHost);
    cudaStreamAttachMemAsync(stream, data);
    cudaStreamSynchronize(stream); // 等待关联完成

    for (int i = 0; i < N; ++i) {
        transform<<<..., stream>>>(in, data, length);
        cudaStreamSynchronize(stream);
        host_process(data, length);        // CPU 安全访问
        convert<<<..., stream>>>(out, data, length);
    }
    cudaStreamSynchronize(stream);
    cudaStreamDestroy(stream);
    cudaFree(data);
}
```

---

## 关键限制、边界条件与兼容性

### 按平台分类的限制

| 平台/算力 | `managedMemory` | `concurrentManagedAccess` | `pageableMemoryAccess` | 关键限制 |
|-----------|----------------|---------------------------|------------------------|----------|
| sm_61 (6.x Pascal, Linux) | 1 | 1 | 0 | 仅 `cudaMallocManaged`；不能直接用 `malloc` |
| sm_70+ with HMM (Linux 6.1+) | 1 | 1 | 1 | 系统分配内存可直接访问；需 Open Kernel Modules |
| Grace Hopper | 1 | 1 | 1 | 硬件一致性；支持 access counter；host native atomics |
| Windows / WSL / pre-6.0 / Tegra | 1 | 0 | 0 | 内核启动时所有 managed memory 必须整体迁到 GPU；不支持 oversubscription；CPU/GPU 不能并发访问 |

### 具体限制细节

**Windows / pre-6.0 平台（`concurrentManagedAccess = 0`）**

- 内核启动时，驱动会将**所有** managed memory 整体拷贝到 GPU，而非按需 fault
- 不支持 GPU 内存 oversubscription：managed memory 总量不能超过物理 GPU 显存
- CPU 在 GPU 内核运行期间**绝对不可**访问任何 managed memory（即使内核根本没用到那块数据），否则 segfault
- `cudaStreamAttachMemAsync` 的流关联语义仍然存在，但主要作用是控制哪些数据需要在内核启动时整体迁移
- `cudaMallocManaged` 动态分配时如果 GPU 正在运行，新分配的行为未定义，必须同步后才能安全使用

**Multi-GPU 行为差异**

- Linux 上，如果所有活跃 GPU 之间支持 P2P，managed memory 会分配在其中一个 GPU 的显存中，其他 GPU 通过 P2P 访问
- 如果加入了一个不支持 P2P 的 GPU，驱动会**自动将所有 managed allocation 迁到系统内存**，所有 GPU 都走 PCIe，带宽大幅下降
- Windows 上若 P2P 不可用（如不同架构 GPU），会自动回退到 mapped memory。若程序实际只用一个 GPU，需要设置 `CUDA_VISIBLE_DEVICES` 来避免此问题，或设置 `CUDA_MANAGED_FORCE_DEVICE_ALLOC=1` 强制使用设备内存

**文件映射内存（mmap）**

- 仅在 `pageableMemoryAccess = 1` 的系统上，GPU 才能直接访问 `mmap` 映射的文件内容
- 在不含 `hostNativeAtomicSupported` 的系统（包括 Linux HMM）上，对 file-backed memory 的原子访问**不被支持**

**IPC（进程间通信）**

- `cudaMallocManaged` 分配的内存**不能**通过 CUDA IPC 共享
- 系统分配的 IPC-capable 内存（如 `mmap(MAP_SHARED)`、`memfd_create`）可以在进程间共享，并被多 GPU 访问
- 不能跨主机（multiple hosts）共享

---

## 常见陷阱与调试建议

### 1. 混淆 `pageableMemoryAccess` 与 `managedMemory`

`managedMemory = 1` 只说明 `cudaMallocManaged` 可用。`pageableMemoryAccess = 1` 才说明 GPU 可以直接访问 `malloc/new/mmap` 分配的指针。很多开发者在 sm_61 这样的 Pascal 卡上误以为 Unified Memory 意味着所有主机指针都能直接传给 kernel，结果遇到非法访问。

### 2. 大页与迁移延迟的权衡

GPU 偏好 2 MiB 大页，但迁移是按整页进行的。如果程序访问模式是细粒度、随机的，大页会导致每次 fault 迁移 2 MiB 数据，引入明显延迟尖峰。此时应考虑使用较小的虚拟页（如 4 KiB），但要注意 TLB miss 的代价。

### 3. `cudaMemAdvise` 的误用

性能提示本身在主机侧有开销。如果提示带来的收益不足以覆盖设置提示的成本，反而会降低性能。建议：
- 仅在热点循环、大数据块上使用
- 通过实际 profiling（Nsight Systems）验证效果，不要凭假设设置

### 4. CPU 写入 GPU 驻留内存的隐式开销

如果数据已位于 GPU，CPU 每次写入都可能触发 cache miss，数据先从 GPU 搬回 CPU cache，写入后再可能写回。对于频繁的小数据交换，更好的做法是将数据固定在 CPU 内存（`cudaMemAdviseSetPreferredLocation` 到 host），让 GPU 直接读取，避免 CPU cache 抖动。

### 5. 流关联的可见性陷阱

默认情况下 `cudaMallocManaged` 分配全局可见。多线程程序中，线程 A 的内核可能意外访问线程 B 的数据。应养成习惯：多线程场景下分配时加 `cudaMemAttachHost`，随后显式 `cudaStreamAttachMemAsync` 绑定到私有流。

### 6. `cudaMemDiscardBatchAsync` 的 UB

丢弃操作和并发预取/读写不能重叠。在异步流水线中，务必在丢弃前通过 stream synchronize 确保该范围内没有正在进行的访问。

---

## 一个最小可运行示例的说明

本章配套的 `chapter_demo.cu` 演示了 Unified Memory 在 **GTX 1060 (sm_61, Pascal)** 上的实际行为。该卡属于 CUDA 12.8 环境下 `managedMemory=1`、`concurrentManagedAccess=1`、`pageableMemoryAccess=0` 的平台，即"仅完全支持 CUDA Managed Memory"类别。示例包含四个测试，分别覆盖了本章的核心知识点：

### Test 1: `cudaMallocManaged` 基础用法

验证最核心的能力：分配 managed memory、CPU 写入字符串、GPU kernel 读取并打印。这是 sm_61 上最基础且始终可用的 Unified Memory 模式。

### Test 2: 系统分配内存（malloc）

该测试检测 `pageableMemoryAccess`。在 sm_61 上此属性为 0，因此 GPU 无法直接访问 `malloc` 指针，测试会**跳过**并打印说明。这直接对应文档中"Unified Memory on Devices with only CUDA Managed Memory Support"章节的限制说明：system allocators 不可用。

### Test 3: `cudaMemPrefetchAsync` + `cudaMemAdviseSetPreferredLocation`

演示性能提示 API 的使用：
1. CPU 初始化一个 1024 元素的 int 数组
2. 使用 `cudaMemAdviseSetPreferredLocation` 建议 GPU 为首选位置
3. 使用 `cudaMemPrefetchAsync` 将数据异步预迁移到 GPU
4. GPU kernel 对每个元素加 100
5. 再用 `cudaMemPrefetchAsync` 将数据迁回 CPU
6. CPU 验证结果

此流程展示了完整的数据预取-计算-回收流水线，是实际性能调优中常见的模式。

### Test 4: 并发 CPU/GPU 访问

利用 `concurrentManagedAccess=1`（sm_61 支持）测试 GPU 写入 managed memory 后 CPU 读取的能力。注意：虽然 Pascal 支持并发访问，但为了测试结果确定性，示例在 CPU 读取前调用了 `cudaDeviceSynchronize()`。这与文档中 4.1.3.2 节的示例一致：在 `concurrentManagedAccess=0` 的平台上，CPU 在内核活跃期间访问 managed memory 会导致 segfault。

### 降级与兼容处理

- 所有设备能力通过 `cudaDeviceGetAttribute` 运行时查询，不依赖编译期假设
- `pageableMemoryAccess=0` 时自动跳过 Test 2，避免非法访问
- `concurrentManagedAccess=0` 时自动跳过 Test 4
- 代码使用 `CUDA_CHECK` 宏对所有 CUDA API 做错误检查
- Makefile 使用 `-arch=sm_61` 精确匹配目标设备，使用 `/usr/bin/g++-14` 和 `-std=c++17`
