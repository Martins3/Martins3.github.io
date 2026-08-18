## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/feature-survey.html>

## A Tour of CUDA Features 章节总结

本章是 CUDA Programming Guide 第 3.5 节（及前后相关小节）的内容概览，目的是在读者已经掌握了 CUDA 基础概念（第 1–3 章前半部分）之后，系统地介绍 CUDA 提供的各类高级特性，并说明每种特性适合解决什么问题。下文中将按照原文的分类方式逐一转述，并整理出核心概念、关键 API、注意事项与常见陷阱。

---

### 1. 多设备托管内存与系统配置前置知识

#### 1.1 Multi-Device Managed Memory（3.4.2.4）
Managed Memory 可以在具备点对点（peer-to-peer）支持的多 GPU 系统上使用。若要在多个设备上并发访问托管内存，需要满足特定的硬件和软件条件，并且可以使用 GPU 独占访问（GPU-exclusive access）相关的 API 来避免数据竞争。

#### 1.2 IOMMU、ACS 与虚拟机（3.4.2.5）
在 **Linux 裸金属（bare-metal）** 环境下，CUDA 和显示驱动**不支持**启用 IOMMU 的 PCIe peer-to-peer 内存传输。如果在裸金属系统上启用了 IOMMU，必须将其禁用，否则会导致**静默的设备内存损坏（silent device memory corruption）**。相反，在 **虚拟机透传（pass through）** 场景下，IOMMU 应该启用，并且配合 **VFIO 驱动** 使用，此时 CUDA 支持 IOMMU。

在 Windows 上不存在上述 IOMMU 限制。

此外，如果系统支持 IOMMU，通常也支持 **PCI Access Control Services (ACS)**。启用 ACS 后，所有 PCI 点对点流量都会被重定向到 CPU 的根复合体（root complex），这会导致整体对分带宽（bisection bandwidth）下降，从而带来显著的性能损失。因此在追求 P2P 带宽的场景下，通常建议**关闭 ACS**。

---

### 2. 提升 Kernel 性能（Improving Kernel Performance）

本节介绍的特性均面向 kernel 开发者，用于帮助其最大化 kernel 执行效率。

#### 2.1 异步屏障（Asynchronous Barriers）
异步屏障在 3.2.4.2 中已有初步介绍。与传统的 `__syncthreads()` 不同，异步屏障将**到达（arrival）**和**等待（wait）**两个阶段分离。线程在到达屏障后，可以继续执行那些不依赖该同步点的工作，而不必立即阻塞等待其他线程。等到确实需要同步结果时，再执行 wait 操作。异步屏障还可以指定不同的线程作用域（thread scopes），例如 block-level 或 device-level。

#### 2.2 异步数据拷贝与 TMA（Asynchronous Data Copies and TMA）
这里所说的异步数据拷贝特指 **kernel 内部** 在**共享内存（shared memory）和 GPU DRAM（global memory）**之间进行的异步数据移动。注意：不要将其与 **CPU 和 GPU 之间的异步内存拷贝**（如 `cudaMemcpyAsync`）混淆。

该特性允许 SM 在拷贝数据的同时继续执行计算，通常配合异步屏障使用。TMA（Tensor Memory Accelerator）是更新一代硬件上进一步加速这种异步拷贝的单元。

#### 2.3 流水线（Pipelines）
流水线是一种用于**分阶段（staging）工作**并协调**多缓冲生产者-消费者模式**的机制。典型用法是：在计算当前缓冲区数据的同时，异步将下一批数据从 global memory 加载到 shared memory，从而**重叠计算与异步数据拷贝**。

#### 2.4 工作窃取与集群启动控制（Work Stealing with Cluster Launch Control）
TODO
这是 **compute capability 10.0（Blackwell）** 引入的新特性。集群启动控制让 kernel 能够直接操控尚未启动的线程块（或线程块集群）的调度。

当工作负载不均匀时，已经提前完成任务的线程块可以**取消另一个尚未启动的线程块的调度，窃取（steal）它的索引，并立即执行该索引对应的工作**。这种工作窃取流程能够在 irregular data 或运行时间变化较大的场景下保持 SM 的高利用率，减少空闲等待时间，实现比硬件调度器更细粒度的负载均衡。

---

### 3. 降低延迟（Improving Latencies）

本节特性主要用于减少 **kernel 启动级别或更高层次** 的延迟，而不是 kernel 内部访存延迟。

#### 3.1 Green Contexts（执行上下文）
Green Contexts（也称为 execution contexts）允许程序创建一个 CUDA 上下文，使其**仅在 GPU 的一部分 SM 上执行工作**。默认情况下，kernel 的线程块可以被调度到任何满足资源需求的 SM 上；而 green context 会进一步限制可用的 SM 集合，并且**其他上下文（包括默认的 primary context）不会在这些被保留的 SM 上调度线程块**。

这使得部分 SM 可以被专门预留出来，运行**高优先级或对延迟敏感**的任务。Green Contexts 在 **CUDA 13.1 及更高版本** 的运行时 API 中可用。

#### 3.2 流排序内存分配（Stream-Ordered Memory Allocation）
传统的 `cudaMalloc` / `cudaFree` 会立即在 CPU 侧同步执行；而 `cudaMallocAsync` / `cudaFreeAsync` 则将内存分配/释放操作**插入到 CUDA 流（stream）中**，使其按流顺序执行。这可以减少 CPU 同步开销，并且释放的内存可以更快地在同一条流中被复用。

#### 3.3 CUDA Graphs
CUDA Graphs 允许应用程序预先定义一组 CUDA 操作（如 kernel 启动、内存拷贝）及其依赖关系，然后将整个图实例化并重复执行。

创建方式主要有两种：
1. **Stream Capture（流捕获）**：将一条流上的一系列操作记录成图。
2. **Graphs API**：使用 `cudaGraphAddKernelNode`、`cudaGraphAddMemcpyNode` 等 API 手动构建。

优势在于：
- 减少 CPU 提交命令的开销；
- 允许驱动对整个工作负载进行全局优化。

#### 3.4 程序化依赖启动（Programmatic Dependent Launch）
该特性允许一个**依赖 kernel** 在它所依赖的**主 kernel** 尚未完全结束时就开始执行。依赖 kernel 可以先执行 setup 代码以及与主 kernel 数据无关的工作，直到需要主 kernel 的输出数据时主动阻塞。主 kernel 在数据就绪后发出信号，即可唤醒依赖 kernel 继续执行。

这种**kernel 间的重叠执行**有助于保持 GPU 高利用率，并缩短关键数据路径的端到端延迟。

#### 3.5 延迟加载（Lazy Loading）
对于包含大量 PTX 代码、需要在应用启动时通过 JIT 编译成 cubin 的应用，如果一次性编译所有 kernel，启动时间会很长。**默认行为是按需编译（lazy loading）**，即模块直到首次被调用时才编译。开发者可以通过环境变量改变这一行为（详见 CUDA Programming Guide 4.7）。

---

### 4. 功能特性（Functionality Features）

#### 4.1 扩展 GPU 内存（Extended GPU Memory, EGM）
EGM 适用于 **NVLink-C2C 互联的系统**。它允许 GPU kernel 高效地访问系统中所有可用的内存（包括 CPU 内存或其他 GPU 的内存），而不仅限于本卡的显存。

#### 4.2 动态并行（Dynamic Parallelism）
通常 kernel 是从 CPU 代码中启动的。而 CUDA 动态并行允许**正在 GPU 上运行的 kernel 直接启动新的 kernel**。这在递归算法、自适应网格细化、基于 GPU 的任务调度等场景中非常有用。

---

### 5. CUDA 互操作性（CUDA Interoperability）

#### 5.1 与其他 GPU API 的互操作
GPU 除了用于通用计算外，最早也用于图形加速（3D 渲染）。CUDA 提供了与 **Direct3D、Vulkan、OpenGL** 等图形 API 的互操作机制，允许在这些 API 和 CUDA 之间**共享 GPU 缓冲区**。例如：先用 CUDA 做物理模拟，再用 Direct3D 渲染结果，而无需在 CPU 和 GPU 之间来回拷贝数据。

同样的共享缓冲区机制也可以用于支持**多节点 GPU 直接通信**的通信库。

#### 5.2 进程间通信（CUDA IPC）
在多 GPU 大规模计算中，除了单进程多 GPU 外，也常常使用**多个主机进程**。CUDA IPC 提供了在不同进程之间**共享 GPU 缓冲区句柄**的机制（`cudaIpcGetMemHandle`、`cudaIpcOpenMemHandle` 等），使得不同进程可以直接访问同一块设备内存，避免通过主机内存中转。

---

### 6. 细粒度控制（Fine-Grained Control）

#### 6.1 虚拟内存管理（Virtual Memory Management）
系统中的所有 GPU 和 CPU 共享一个**统一虚拟地址空间（UVA）**。大多数应用使用 CUDA 默认的内存管理即可。但对于需要在多 GPU、多节点之间精细控制缓冲区映射行为的高级场景，CUDA 驱动 API 提供了诸如 `cuMemAddressReserve`、`cuMemCreate`、`cuMemMap`、`cuMemSetAccess` 等函数，允许开发者精确管理虚拟地址空间的布局。

#### 6.2 驱动入口点访问（Driver Entry Point Access）
从 **CUDA 11.3** 开始，开发者可以动态获取 CUDA 驱动和运行时 API 的函数指针（`cuGetProcAddress`）。这带来了两个好处：
1. 可以调用特定变体的驱动函数；
2. 可以在安装了比 CUDA Toolkit 版本更新的驱动的系统上，访问新驱动中新增的 API。

#### 6.3 错误日志管理（Error Log Management）
CUDA 提供了一套简便的错误日志工具：
- 设置环境变量 `CUDA_LOG_FILE` 即可将 CUDA 运行时的错误信息输出到 **stderr、stdout 或指定文件**；
- 应用程序还可以注册一个**错误回调函数**，当 CUDA 遇到错误时自动触发。

---

### 7. 核心概念速查

| 特性 | 解决的问题 | 关键硬件/软件要求 |
|------|------------|-------------------|
| 异步屏障 | 更细粒度的线程同步，隐藏等待延迟 | 较新的 compute capability（sm_80+ 完全支持） |
| 异步数据拷贝 / TMA | 重叠 kernel 内计算与数据搬运 | sm_80+（TMA 需 Hopper/Blackwell） |
| 流水线 | 多缓冲生产者-消费者 | 配合异步屏障使用 |
| 工作窃取 / 集群启动控制 | 不均匀负载下的 SM 利用率 | **sm_100 (Blackwell)** |
| Green Contexts | 为关键任务预留 SM | **CUDA 13.1+** |
| 流排序内存分配 | 降低 `cudaMalloc/Free` 的 CPU 同步开销 | CUDA 11.2+ |
| CUDA Graphs | 降低重复工作负载的 CPU 发射开销 | CUDA 10.0+ |
| 程序化依赖启动 | 重叠依赖 kernel 的执行 | **sm_90 (Hopper)** |
| 延迟加载 | 减少含大量 PTX 应用的启动时间 | 默认开启，可用环境变量控制 |
| 扩展 GPU 内存 (EGM) | GPU 访问全系统内存 | NVLink-C2C 系统 |
| 动态并行 | GPU 上启动新 kernel | sm_35+ |
| CUDA IPC | 多进程共享设备内存 | P2P 支持的多 GPU 系统 |
| 虚拟内存管理 | 精细控制 UVA 布局 | 驱动 API |
| 驱动入口点访问 | 动态获取 API 函数指针 | CUDA 11.3+ |
| 错误日志管理 | 捕获和记录 CUDA 错误 | `CUDA_LOG_FILE` 环境变量 |

---

### 8. 关键 API / 函数

#### 8.1 同步与拷贝
- `cuda::barrier` / `cooperative_groups` 系列（异步屏障）
- `cuda::memcpy_async`（kernel 内异步拷贝）
- `cuda::pipeline`（流水线 API）

#### 8.2 内存分配
- `cudaMallocAsync(void** devPtr, size_t size, cudaStream_t stream)`
- `cudaFreeAsync(void* devPtr, cudaStream_t stream)`

#### 8.3 CUDA Graphs
- `cudaStreamBeginCapture(cudaStream_t stream, cudaStreamCaptureMode mode)`
- `cudaStreamEndCapture(cudaStream_t stream, cudaGraph_t* pGraph)`
- `cudaGraphInstantiate(cudaGraphExec_t* pGraphExec, cudaGraph_t graph, ...)`
- `cudaGraphLaunch(cudaGraphExec_t graphExec, cudaStream_t stream)`
- `cudaGraphDestroy(cudaGraph_t graph)`
- `cudaGraphExecDestroy(cudaGraphExec_t graphExec)`

#### 8.4 动态并行
- 从设备代码中调用 `cudaLaunchKernel(...)`

#### 8.5 CUDA IPC
- `cudaIpcGetMemHandle(cudaIpcMemHandle_t* handle, void* devPtr)`
- `cudaIpcOpenMemHandle(void** devPtr, cudaIpcMemHandle_t handle, unsigned int flags)`
- `cudaIpcCloseMemHandle(void* devPtr)`

#### 8.6 虚拟内存管理（驱动 API）
- `cuMemAddressReserve(...)`
- `cuMemCreate(...)`
- `cuMemMap(...)`
- `cuMemSetAccess(...)`
- `cuMemUnmap(...)`
- `cuMemRelease(...)`

#### 8.7 驱动入口点
- `cuGetProcAddress(const char* symbol, void** pfn, int cudaVersion, cuuint64_t flags)`

---

### 9. 注意事项与常见陷阱

#### 9.1 关于 IOMMU 与 ACS（Linux 裸金属）
- **陷阱**：在 Linux 裸金属服务器上默认开启 IOMMU 后，CUDA 的 PCIe P2P 传输会失败，且不会报错，表现为**静默的数据损坏**。
- **正确做法**：裸金属系统禁用 IOMMU；仅在 VM 透传场景下启用 IOMMU 并配合 VFIO 驱动。

#### 9.2 异步数据拷贝的语义混淆
- **陷阱**：将 kernel 内的 `cuda::memcpy_async` 与主机侧的 `cudaMemcpyAsync` 混为一谈。
- **正确理解**：前者发生在 **shared memory <-> global memory** 之间，由 SM 的异步拷贝引擎执行；后者发生在 **CPU <-> GPU** 之间，由 DMA 引擎执行。

#### 9.3 Green Contexts 与 Primary Context 的隔离
- **陷阱**：以为创建了 green context 后，其他非 green context 的 kernel 会自动避让，但实际上 green context 只在**同一张卡上**对 SM 进行划分，不影响其他 GPU。
- **注意**：green context 目前仅在 CUDA 13.1+ 中可用，CUDA 12.x 及以下版本不支持。

#### 9.4 CUDA Graphs 的捕获限制
- **陷阱**：在 stream capture 期间调用了不支持捕获的 API（如同步 API `cudaDeviceSynchronize`、`cudaStreamSynchronize`），会导致捕获失败。
- **正确做法**：捕获期间只能使用异步 API；所有同步点应通过事件（event）或显式的图节点依赖来表达。

#### 9.5 Stream-Ordered Memory Allocator 的显式同步
- **陷阱**：使用 `cudaFreeAsync(ptr, stream)` 后立即在同一流上访问该内存是安全的，但如果**跨流访问**，必须确保已经通过事件或流同步建立了正确的 happens-before 关系。
- **最佳实践**：尽量在同一条流内完成分配、使用、释放的生命周期；跨流共享时显式插入 `cudaEventRecord` / `cudaStreamWaitEvent`。

#### 9.6 动态并行的资源与同步开销
- **陷阱**：在设备端无节制地启动大量子 kernel，导致启动开销累积或显存耗尽。
- **最佳实践**：动态并行适合粗粒度任务划分；避免在 device 端进行过度细粒度的 kernel 启动。

#### 9.7 延迟加载的环境变量
- **陷阱**：某些性能分析或调试工具需要在启动时看到所有 kernel 符号。如果此时关闭了延迟加载，启动时间可能异常长。
- **正确做法**：仅在确实需要快速启动的场景下保持默认的 lazy loading；需要完整加载时使用 `CUDA_MODULE_LOADING=EAGER`。

#### 9.8 Work Stealing 与 Programmatic Dependent Launch 的硬件限制
- **陷阱**：在未满足目标 compute capability 或驱动版本要求的设备上尝试使用这些特性，会导致编译失败或运行时异常。
- **正确做法**：
  - 工作窃取 / 集群启动控制 需要 **sm_100 (Blackwell)**；
  - 程序化依赖启动 需要 **sm_90 (Hopper)**。
  在代码中应通过 `cudaGetDeviceProperties` 检查 `major` / `minor` 版本，并选择匹配当前设备能力的代码路径。
