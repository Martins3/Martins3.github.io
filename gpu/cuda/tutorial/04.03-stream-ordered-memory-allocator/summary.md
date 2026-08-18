## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/stream-ordered-memory-allocation.html>


本章介绍 CUDA 的 Stream-Ordered Memory Allocator（流顺序内存分配器），这是一套从 CUDA 11.2 开始引入的内存管理 API，核心函数为 `cudaMallocAsync` 与 `cudaFreeAsync`。与传统 `cudaMalloc`/`cudaFree` 不同，这套分配器将内存的分配与释放操作绑定到指定的 CUDA Stream 上，利用流之间的顺序语义来避免不必要的全局设备同步。其设计目标包括：降低自定义内存管理的实现成本、让多个库共享同一个由驱动管理的内存池以减少内存冗余、以及允许驱动根据其对分配器和流管理的全局认知执行更激进的优化。

本章内容可以划分为三个层面：
1. **API 使用层面**：`cudaMallocAsync` 和 `cudaFreeAsync` 的基本语义，以及它们与传统 `cudaMalloc`/`cudaFree` 的混用规则。
2. **Memory Pool 管理层面**：默认内存池与显式内存池的区别、多 GPU 可访问性配置、以及基于内存池的 IPC（进程间通信）共享机制。
3. **性能调优层面**：Release Threshold 的设定、`cudaMemPoolTrimTo` 的手动回收、资源使用统计查询、以及三种内存重用策略的开关控制。

Nsight Compute 和下一代 CUDA Debugger 从 CUDA 11.3 开始原生支持该分配器，这意味着调试器和性能分析工具能够正确识别通过流顺序分配器分配的内存。

## 背景与要解决的问题

在传统的 CUDA 编程中，`cudaMalloc` 和 `cudaFree` 是同步操作：它们会迫使当前设备在所有正在执行的流上完成所有 pending 的工作，然后才能返回。这种全局同步带来两个明显的性能问题：

1. **主机端延迟**：即使某个流只申请一小块内存，主机线程也必须等待整个 GPU 完成所有流上的工作，这破坏了异步执行流水线。
2. **并发度下降**：内存释放操作同样会阻塞其他流的执行，导致 GPU 计算资源出现空闲窗口。

对于需要频繁分配和释放内存的应用（例如图神经网络、动态批处理、递归数据结构等），`cudaMalloc`/`cudaFree` 的同步开销可能成为显著瓶颈。此外，各个库如果各自维护独立的内存池，会导致物理内存的重复占用和碎片化。

Stream-Ordered Memory Allocator 通过将内存操作流化，从根本上改变了这一模型：
- 分配和释放操作像核函数或 `cudaMemcpyAsync` 一样，被排入指定流的命令队列中。
- 驱动在后台维护一个内存池（Memory Pool），负责将释放的内存块标记为可回收，并在满足流顺序语义的前提下重新分配给后续请求。
- 当内存池中的缓存足够大时，分配请求可以完全在设备端完成，无需调用操作系统来分配或回收物理页。

## 核心概念与术语

### Stream-Ordered（流顺序）

流顺序是本章所有语义的基础。所谓流顺序，指的是 CUDA Stream 中命令的提交顺序和执行顺序之间的 happen-before 关系。`cudaMallocAsync` 的返回值（一个设备指针）在该流中后续提交的操作看来是可用的；但如果在其他流中访问该指针，必须通过 `cudaEventRecord` / `cudaStreamWaitEvent` 或显式同步来建立跨流的 happen-before 关系，否则行为未定义。

同理，`cudaFreeAsync` 的释放操作在流顺序上只保证：同一条流中，位于 `cudaFreeAsync` 之前的操作可以安全访问该内存；而在 `cudaFreeAsync` 之后提交到同一条流的操作不能再访问它。如果其他流仍在访问该内存，必须通过事件同步确保访问完成后才执行释放。

### Memory Pool（内存池）

内存池是 Stream-Ordered Allocator 的核心数据结构，它封装了虚拟地址和物理内存资源。所有 `cudaMallocAsync` 调用都会从某个内存池中获取内存。内存池的关键属性包括：

- **种类与位置**：可以是设备本地内存（`cudaMemLocationTypeDevice`），也可以是绑定到特定 CPU NUMA 节点的内存（`cudaMemLocationTypeHostNuma`）。
- **IPC 能力**：显式池可以在创建时指定 `handleType`，从而支持跨进程共享。
- **访问权限**：可以通过 `cudaMemPoolSetAccess` 控制哪些设备可以访问该池中的分配。

每个设备都有一个**默认内存池**（Default/Implicit Pool），如果用户没有显式指定池，也没有通过 `cudaDeviceSetMempool` 设置当前池，`cudaMallocAsync` 就会自动使用该默认池。默认池中的分配是非迁移的设备本地内存，始终可以从其所在设备访问，但不支持 IPC。

### Release Threshold（释放阈值）

Release Threshold 是内存池的一个属性（`cudaMemPoolAttrReleaseThreshold`），单位为字节。它定义了内存池在尝试将物理内存释放回操作系统之前，应当保留的最小内存量。当池持有的内存超过该阈值时，在下一次流同步、事件同步或设备同步时，分配器会尝试将多余内存归还给 OS。

默认值下，分配器会尽量最小化内存占用，这意味着频繁地向 OS 申请和归还物理页，可能带来较大的系统调用开销。如果应用愿意接受更大的常驻内存占用，可以将 Release Threshold 设为较高值（例如 `UINT64_MAX`），从而完全禁止自动 shrink。

### 内存重用策略

驱动在尝试从 OS 申请新物理页之前，会优先尝试重用之前通过 `cudaFreeAsync` 释放的内存。驱动支持三种可控的重用策略：

1. **`cudaMemPoolReuseFollowEventDependencies`**：允许 allocator 根据 CUDA Event 建立的跨流依赖信息，在一条流中重用另一条流中释放的内存。
2. **`cudaMemPoolReuseAllowOpportunistic`**：允许 allocator 检查释放操作在目标流中的执行进度。如果目标流已经执行过了 `cudaFreeAsync` 所在的位置，即使没有显式同步，也允许重用该内存。
3. **`cudaMemPoolReuseAllowInternalDependencies`**：当 allocator 无法从 OS 申请到更多物理内存时，它可以在分配流中自动插入必要的依赖（相当于隐式执行 `cudaStreamWaitEvent`），从而复用其他流中尚未完全释放的内存。

这些策略默认通常是开启的，但用户可以通过 `cudaMemPoolSetAttribute` 关闭它们。关闭后可以减少运行间的不确定性，避免 opportunistic reuse 导致的性能抖动。

## API / 机制详解

### 4.1 核心分配与释放 API

#### `cudaMallocAsync(void **devPtr, size_t size, cudaStream_t hStream)`

**功能**：在指定流上异步分配设备内存。分配操作本身不会阻塞主机，也不会阻塞其他流。

**关键参数**：
- `devPtr`：输出参数，返回分配到的设备指针。
- `size`：请求的字节数。
- `hStream`：关联的 CUDA Stream。分配在该流的命令队列中排队。

**重要语义**：
- `cudaMallocAsync` 忽略当前设备上下文（即忽略 `cudaSetDevice` 设置的当前设备），它根据提供的流或显式指定的内存池来决定内存归属的设备。
- 在同一条流中，`cudaMallocAsync` 返回的指针可以立即被后续排队的核函数或拷贝操作使用。
- 在其他流中访问该指针前，必须建立 happen-before 关系（如事件等待），否则行为未定义。

**常见错误**：
- 在没有适当同步的情况下，在另一条流中访问刚分配的内存。
- 假设 `cudaMallocAsync` 会阻塞到分配完成（实际上它只会在流中排队）。

#### `cudaFreeAsync(void *devPtr, cudaStream_t hStream)`

**功能**：在指定流上异步释放设备内存。释放操作在流中排队，不会阻塞主机或其他流。

**关键语义**：
- 用户必须保证在 `cudaFreeAsync` 之前，所有对该内存的访问都已经完成。同一条流中，`cudaFreeAsync` 之后的操作不能再使用该指针。
- 跨流场景下，必须通过 `cudaEventRecord` + `cudaStreamWaitEvent` 确保其他流的访问已经完成，然后再将 `cudaFreeAsync` 排入目标流。
- 内存实际归还到内存池的时间点由流执行进度决定，而不是 `cudaFreeAsync` 被主机调用的时刻。

**与传统 API 的混用**：
- 用 `cudaMalloc` 分配的内存可以用 `cudaFreeAsync` 释放（需确保流顺序上所有访问已完成）。
- 用 `cudaMallocAsync` 分配的内存可以用 `cudaFree` 释放，但 **`cudaFree` 不会执行任何同步**，它假设所有访问已经完成。因此，在调用 `cudaFree` 之前，用户必须显式调用 `cudaStreamSynchronize`、`cudaDeviceSynchronize` 或事件同步 API 来确保安全。

### 4.2 Memory Pool 管理 API

#### `cudaDeviceGetDefaultMempool(cudaMemPool_t *memPool, int device)`

**功能**：获取指定设备的默认内存池句柄。

**调用时机**：需要查询或修改默认池属性时，例如设置 Release Threshold 或查询资源统计。

#### `cudaDeviceSetMemPool(int device, cudaMemPool_t memPool)` / `cudaDeviceGetMemPool`

**功能**：设置/获取指定设备的当前内存池。设置后，该设备上未显式指定内存池的 `cudaMallocAsync` 调用将使用新设置的池。

**限制**：当前内存池必须是设备本地池。如果尝试将导入的 IPC 池设为当前池，会失败。

#### `cudaMemPoolCreate(cudaMemPool_t *memPool, const cudaMemPoolProps *poolProps)`

**功能**：创建一个显式内存池，允许用户指定超出默认池的能力。

**关键参数（`cudaMemPoolProps`）**：
- `allocType`：固定为 `cudaMemAllocationTypePinned`。
- `location`：内存驻留位置，可以是 `cudaMemLocationTypeDevice`（指定 GPU）或 `cudaMemLocationTypeHostNuma`（指定 CPU NUMA 节点）。
- `handleType`：指定 IPC 句柄类型，例如 `cudaMemHandleTypePosixFileDescriptor`。非零值使池支持 IPC 导出。

**与默认池的区别**：
- 显式池可以支持 IPC；默认池不支持 IPC。
- 显式池可以设置最大容量等额外属性。

#### `cudaMemPoolSetAccess` / `cudaMemPoolGetAccess`

**功能**：修改/查询内存池分配的跨设备可访问性。

**关键限制**：
- 默认情况下，分配只对内存池所在设备可见。
- 不能撤销池所在设备自身的访问权限。
- 要允许其他设备访问，必须先通过 `cudaDeviceCanAccessPeer` 确认对等访问能力，否则 `cudaMemPoolSetAccess` 可能返回 `cudaErrorInvalidDevice`。
- 如果池中没有分配时调用 `cudaMemPoolSetAccess`，即使设备之间不对等，调用也可能成功；但下一次实际分配时会失败。
- 不建议频繁修改池的可访问性。建议一旦对某个 GPU 开放访问，就保持到池的生命周期结束。

### 4.3 IPC 内存池共享

基于内存池的 IPC 共享分为两个阶段：共享池的访问权限，以及共享具体的分配指针。

#### 第一阶段：共享池访问权限

1. **导出进程**：创建带 `handleType` 的显式池，然后调用 `cudaMemPoolExportToShareableHandle` 获取 OS 原生句柄（如 POSIX 文件描述符）。
2. **传输**：使用操作系统提供的 IPC 机制（如 Unix Domain Socket、`sendmsg`/`recvmsg`）将句柄传递给导入进程。
3. **导入进程**：调用 `cudaMemPoolImportFromShareableHandle` 创建导入池。

**导入池的限制**：
- 导入池的访问权限不会继承导出进程的设置。导入进程需要自行调用 `cudaMemPoolSetAccess` 启用所需的 GPU 访问。
- 导入池不能设为当前池，也不能用于 `cudaMallocFromPoolAsync`。它只能用于导入已经分配好的指针。
- 导入池和导出池目前都不支持将物理块释放回 OS（见下文 IPC 限制）。

#### 第二阶段：共享具体分配

1. **导出进程**：调用 `cudaMallocAsync` 分配内存，然后调用 `cudaMemPoolExportPointer` 获取 `cudaMemPoolPtrExportData`（一个不透明数据结构）。同时，创建一个 IPC Event（`cudaEventCreate` 带 `cudaEventInterprocess | cudaEventDisableTiming`），在分配流上 `cudaEventRecord`，并将 event handle 和 export data 传给导入进程。
2. **导入进程**：调用 `cudaIpcOpenEventHandle` 打开 IPC event，调用 `cudaMemPoolImportPointer` 导入指针。导入操作本身不阻塞。随后必须调用 `cudaStreamWaitEvent` 等待分配就绪 event 后，才能安全访问该指针。

**释放顺序的严格要求**：
- 导入进程必须先释放分配，导出进程才能释放。
- 通常使用另一个 IPC event 来协调：导入进程在其 `cudaFreeAsync` 之后记录 `finishedIpcEvent`，导出进程在释放前调用 `cudaStreamWaitEvent` 等待该事件。

### 4.4 资源查询与调优 API

#### `cudaMemPoolSetAttribute` / `cudaMemPoolGetAttribute`

支持的属性包括：
- **`cudaMemPoolAttrReleaseThreshold`**（`cuuint64_t`）：设置/查询释放阈值。
- **`cudaMemPoolAttrReservedMemCurrent`**（`cuuint64_t`）：当前池占用的物理 GPU 内存总量。
- **`cudaMemPoolAttrReservedMemHigh`**（`cuuint64_t`）：自上次重置以来，reserved 内存的水位峰值。
- **`cudaMemPoolAttrUsedMemCurrent`**（`cuuint64_t`）：当前已从池中分配出去且尚未被重用的内存总量。
- **`cudaMemPoolAttrUsedMemHigh`**（`cuuint64_t`）：used 内存的水位峰值。
- **`cudaMemPoolReuseFollowEventDependencies`**（`int`）：是否允许基于 event 的跨流重用。
- **`cudaMemPoolReuseAllowOpportunistic`**（`int`）：是否允许基于流执行进度的机会性重用。
- **`cudaMemPoolReuseAllowInternalDependencies`**（`int`）：是否允许驱动在必要时自动插入跨流依赖以重用内存。

重置水位峰值的方法：使用 `cudaMemPoolSetAttribute` 将 `*MemHigh` 属性设为 0，驱动会自动将其重置为当前值。

#### `cudaMemPoolTrimTo(cudaMemPool_t memPool, size_t minBytesToKeep)`

**功能**：手动缩减内存池的物理内存占用。

**调用时机**：当应用进入低内存需求阶段，且之前设置了较高的 Release Threshold 以禁用自动 shrink 时，可以在显式同步后调用此函数释放多余内存。

**前置条件**：调用前应当执行 `cudaStreamSynchronize` 或 `cudaDeviceSynchronize`，确保池中待释放的内存块确实已经不再被使用。

### 4.5 支持性查询

#### 运行时查询

```c
int driverVersion = 0;
cudaDriverGetVersion(&driverVersion);

if (driverVersion >= 11020) {
    cudaDeviceGetAttribute(&deviceSupportsMemoryPools,
                           cudaDevAttrMemoryPoolsSupported, device);
}

if (driverVersion >= 11030) {
    cudaDeviceGetAttribute(&poolSupportedHandleTypes,
                           cudaDevAttrMemoryPoolSupportedHandleTypes, device);
}
```

**设计原因**：CUDA 11.2 引入了 `cudaDevAttrMemoryPoolsSupported`，CUDA 11.3 引入了 `cudaDevAttrMemoryPoolSupportedHandleTypes`。在旧驱动上查询未定义的属性会返回 `cudaErrorInvalidValue`。因此，建议先检查驱动版本再查询属性，或者在查询失败后用 `cudaGetLastError` 清除错误。

## 典型工作流程 / 调用顺序

### 单流基本模式

这是最简单的使用方式，分配、使用、释放都在同一条流中完成：

```c
void *ptr;
size_t size = 512;
cudaMallocAsync(&ptr, size, cudaStreamPerThread);
// 在同一流中排队使用 ptr 的核函数或拷贝操作
kernel<<<..., cudaStreamPerThread>>>(ptr, ...);
// 异步释放，不阻塞 CPU
cudaFreeAsync(ptr, cudaStreamPerThread);
```

### 跨流安全释放模式

当内存需要在多个流之间共享时，必须通过事件建立依赖链：

```c
// stream1 分配并使用
cudaMallocAsync(&ptr, size, stream1);
cudaEventRecord(event1, stream1);

// stream2 等待 allocation 就绪后访问
cudaStreamWaitEvent(stream2, event1);
kernel<<<..., stream2>>>(ptr, ...);
cudaEventRecord(event2, stream2);

// stream3 等待 stream2 使用完毕后释放
cudaStreamWaitEvent(stream3, event2);
cudaFreeAsync(ptr, stream3);
```

### 查询统计并手动回收模式

```c
// 查询当前使用情况
cudaMemPoolGetAttribute(memPool, cudaMemPoolAttrReservedMemCurrent, &reserved);
cudaMemPoolGetAttribute(memPool, cudaMemPoolAttrUsedMemCurrent, &used);

// 在一个阶段结束后，显式同步并回收内存
cudaStreamSynchronize(stream);
cudaMemPoolTrimTo(memPool, 0); // 尽量释放所有未使用的物理内存
```

### IPC 共享模式（导出侧）

```c
// 1. 创建 IPC 能力的显式池
cudaMemPoolCreate(&memPool, &poolProps);

// 2. 分配并导出指针
cudaMallocAsync(&ptr, size, stream, memPool);
cudaEventRecord(readyEvent, stream);
cudaMemPoolExportPointer(&exportData, ptr);
cudaIpcGetEventHandle(&readyHandle, readyEvent);

// 3. 通过 IPC 机制发送 readyHandle 和 exportData
```

### IPC 共享模式（导入侧）

```c
// 1. 接收 readyHandle 和 exportData，打开 event
cudaIpcOpenEventHandle(&readyEvent, &readyHandle);

// 2. 导入指针（不阻塞）
cudaMemPoolImportPointer(&ptr, importedMemPool, &exportData);

// 3. 等待分配就绪后才能使用
cudaStreamWaitEvent(stream, readyEvent);
kernel<<<..., stream>>>(ptr, ...);
```

## 关键限制、边界条件与兼容性

### 架构与驱动兼容性

- Stream-Ordered Memory Allocator 需要 CUDA 11.2 或更高版本的驱动。GTX 1060（sm_61）完全支持该功能，因为该功能由驱动实现，与计算能力无直接绑定。
- IPC 内存池需要 CUDA 11.3 或更高版本才能查询 `cudaDevAttrMemoryPoolSupportedHandleTypes`。

### 默认池的限制

- 默认池不支持 IPC。如果需要跨进程共享，必须创建显式池并设置 `handleType`。
- 默认池位于其所属设备上，分配总是设备本地内存。

### 显式池与 IPC 的特殊限制

- **IPC 导出池**：目前不支持将物理块释放回操作系统。`cudaMemPoolTrimTo` 对其无效，`cudaMemPoolAttrReleaseThreshold` 也被忽略。这是驱动层面的限制，非运行时限制，未来驱动更新可能改变。
- **IPC 导入池**：不能用于新的分配（不能设为当前池，也不能传给 `cudaMallocFromPoolAsync`）。它的资源统计只反映已导入的分配及其关联的物理内存。

### cudaFree 的语义陷阱

- `cudaFreeAsync` 分配的内存若要用 `cudaFree` 释放，**必须**先显式同步对应的流（或设备）。`cudaFree` 不做任何同步，直接回收内存。
- 反过来，`cudaMalloc` 分配的内存可以直接用 `cudaFreeAsync` 释放，只要流顺序上所有访问已经完成。

### 访问权限修改的限制

- `cudaMemPoolSetAccess` 影响池中**所有**已有和未来分配，而不仅仅是未来的。
- 频繁修改访问权限不被推荐，因为这可能触发驱动内部的地址映射调整，带来额外开销。

## 常见陷阱与调试建议

### 陷阱一：跨流访问未同步

最常见的错误是在 stream A 中调用 `cudaMallocAsync`，然后在 stream B 中直接访问返回的指针，而没有使用 `cudaEventRecord` + `cudaStreamWaitEvent`。这会导致数据竞争或未定义行为。

**调试建议**：在怀疑存在此类问题时，临时插入 `cudaStreamSynchronize(streamA)` 和 `cudaStreamSynchronize(streamB)`，如果 bug 消失，则几乎可以确定是跨流同步缺失。

### 陷阱二：cudaFreeAsync 后继续使用内存

在同一条流中，`cudaFreeAsync` 之后的操作不能访问该指针。但由于 `cudaFreeAsync` 是异步的，主机代码可能立即继续执行，容易在逻辑上误以为内存已经"安全释放"，而实际上 GPU 可能还在使用它（如果后续错误地又提交了访问操作）。

**调试建议**：使用 cuda-memcheck 或 Compute Sanitizer 的 `initcheck` / `racecheck` 工具来检测释放后使用（use-after-free）错误。

### 陷阱三：IPC 释放顺序颠倒

导入进程尚未释放分配时，导出进程就调用了 `cudaFreeAsync` 或 `cudaFree`，这会导致导入进程的访问出错。

**调试建议**：在 IPC 代码中始终使用双向 IPC event 进行协调。导入侧在最终 `cudaFreeAsync` 后记录 event，导出侧在释放前等待该 event。

### 陷阱四：旧驱动上的属性查询错误

在 CUDA 11.2 之前的驱动上查询 `cudaDevAttrMemoryPoolSupportedHandleTypes` 会返回 `cudaErrorInvalidValue`。如果不处理这个错误，可能导致程序错误地认为设备不支持内存池。

**调试建议**：始终先查询 `cudaDriverGetVersion`，再进行属性查询；或者在查询失败后用 `cudaGetLastError` 清错。

### 陷阱五：Release Threshold 设置过高导致 OOM

将 Release Threshold 设为 `UINT64_MAX` 虽然能避免 OS 调用开销，但如果应用进入低内存使用阶段，物理内存会一直驻留在池中，可能导致其他进程或分配失败。

**调试建议**：在应用的不同阶段采用不同的策略。高内存需求阶段可以禁用 shrink；低需求阶段在同步后调用 `cudaMemPoolTrimTo` 主动回收。

## 一个最小可运行示例的说明

本章提供的示例程序为 `chapter_demo.cu`，配合 `Makefile` 可在 GTX 1060（sm_61, CUDA 12.8）上编译运行。

### 示例设计目标

该示例旨在覆盖 Stream-Ordered Memory Allocator 的核心能力，而非展示复杂的算法。它演示了以下内容：

1. **支持性查询**：运行时检测驱动版本和设备是否支持 `cudaMallocAsync` 与内存池。
2. **默认内存池获取与配置**：通过 `cudaDeviceGetDefaultMemPool` 获取默认池，并设置 `cudaMemPoolAttrReleaseThreshold` 为 64 MiB，避免频繁向 OS 归还内存。
3. **基本异步分配/释放**：在同一条流（`stream1`）中分配三个浮点数组，启动一个 `vector_add` 核函数，然后在另一条流（`stream3`）中异步释放。
4. **跨流事件同步**：使用 `cudaEventRecord` 和 `cudaStreamWaitEvent` 在三条流之间建立 happen-before 链，确保 stream2 在分配完成后访问，stream3 在 stream2 使用完毕后释放。
5. **内存重用观察**：在释放后，于 stream1 中重新分配一个同样大小的数组，并打印内存池统计信息（`cudaMemPoolAttrReservedMemCurrent` 等），可以观察到 `used` 内存下降但 `reserved` 内存可能保持不变（因为阈值设定阻止了立即 shrink）。
6. **显式同步与手动回收**：调用 `cudaStreamSynchronize` 和 `cudaMemPoolTrimTo(memPool, 0)`，展示如何在确认所有工作完成后手动释放物理内存。
7. **新旧 API 混用**：分别演示 `cudaMalloc` + `cudaFreeAsync` 以及 `cudaMallocAsync` + `cudaFree`（后者在 `cudaFree` 前必须显式同步）。

### sm_61 兼容性与降级处理

GTX 1060 的计算能力为 sm_61，其硬件不支持某些较新的 CUDA 特性（例如 Tensor Core、某些异步拷贝指令等），但 Stream-Ordered Memory Allocator 是纯驱动层功能，与计算能力无关。因此本示例在 sm_61 上无需任何特性降级或 workaround。

不过，Makefile 中显式指定了 `-arch=sm_61`，确保生成的代码兼容目标 GPU。nvcc 编译时可能会提示 sm_61 的离线编译支持将在未来版本中移除，但这不影响当前 CUDA 12.8 的编译和运行。

示例中包含 `CUDA_CHECK` 宏，对所有 CUDA API 调用进行错误检查。如果设备不支持内存池（例如驱动过旧），程序会提前打印说明并安全退出，而不会触发未定义行为。

### 编译与运行

```bash
make        # 编译生成 chapter_demo.out
make run    # 运行示例
```

成功运行时，控制台会输出设备信息、驱动版本、内存池支持情况，以及多次内存池统计的快照，直观展示分配、释放、重用和 trim 过程中 reserved/used 内存的变化。


