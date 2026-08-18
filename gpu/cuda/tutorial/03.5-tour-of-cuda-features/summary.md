## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/feature-survey.html>

## 核心概念

CUDA 3.5 节《A Tour of CUDA Features》是一份**高级特性导览**，将 CUDA 的众多进阶功能按解决的问题类型划分为五大类，并指出各特性适用的场景与依赖的硬件/软件条件。核心思想是：**并非所有特性都适用于所有用例**，开发者需要根据性能瓶颈（kernel 吞吐、启动延迟、功能扩展、跨 API 协作或底层控制）来选择合适的工具。

五大类别：

1. **提升 Kernel 性能（Improving Kernel Performance）**
   侧重在 kernel 内部挖掘指令级与内存级并行，减少线程空闲。包括异步屏障、异步数据拷贝/TMA、流水线、Cluster Launch Control 等。
2. **降低延迟（Improving Latencies）**
   侧重在 kernel 启动层面及以上（如上下文调度、内存分配、图执行）减少 CPU 侧或调度侧的延迟。包括 Green Contexts、Stream-Ordered Memory Allocation、CUDA Graphs、Programmatic Dependent Launch、Lazy Loading 等。
3. **功能扩展（Functionality Features）**
   为 GPU 程序提供新的编程能力，如 Extended GPU Memory、Dynamic Parallelism 等。
4. **CUDA 互操作（CUDA Interoperability）**
   使 CUDA 能与图形 API（OpenGL/Vulkan/Direct3D）或其他进程共享 GPU 缓冲区。
5. **细粒度控制（Fine-Grained Control）**
   提供对虚拟地址空间、驱动入口点、错误日志等底层行为的显式控制。

## 关键 API / 函数 / 宏

### 异步同步与数据搬运
- `cuda::barrier`（libcu++）：支持到达（arrive）与等待（wait）分离的异步屏障，可指定线程作用域（thread / block / device / system）。
- `cuda::pipeline`：用于多缓冲生产者-消费者流水线，协调异步拷贝与计算的重叠。
- `cuda::memcpy_async`：在 kernel 执行期间异步地将数据从全局内存搬至共享内存（sm_80+ 有硬件加速）。

### Cluster 与 Launch 控制
- `__cluster_dims__(X, Y, Z)`：编译时指定 thread block cluster 维度。
- `__block_size__((Bx, By, Bz), (Cx, Cy, Cz))`：编译时指定 block 大小与 cluster 大小，并将 `<<<...>>>` 的第一参数解释为 cluster 数量。
- `cudaLaunchKernelEx` / `cudaLaunchConfig_t` / `cudaLaunchAttribute` / `cudaLaunchAttributeClusterDimension`：运行时动态配置 cluster 启动。
- Cluster Group API：`cluster.sync()`、`num_threads()`、`num_blocks()`、`dim_threads()`、`dim_blocks()`。

### 延迟优化
- `cudaMallocAsync` / `cudaFreeAsync`：将 GPU 内存分配/释放插入流中，实现异步、按需的内存管理（要求 CUDA 11.2+ 与 compute capability 6.0+）。
- CUDA Graphs API：`cudaStreamBeginCapture`、`cudaStreamEndCapture`、`cudaGraphInstantiate`、`cudaGraphLaunch`、`cudaGraphExecDestroy`、`cudaGraphDestroy`。
- Programmatic Dependent Launch：通过 `cudaLaunchKernelEx` 的依赖启动属性，使下游 kernel 在上游 kernel 尚未完全结束时即可开始执行前导代码。

### 功能与互操作
- 动态并行（Dynamic Parallelism）：device 侧调用 `cudaLaunchKernel`、编译选项 `-rdc=true`、链接库 `cudadevrt`。
- IPC：`cudaIpcGetMemHandle`、`cudaIpcOpenMemHandle`、`cudaIpcCloseMemHandle`。
- 图形互操作：`cudaGraphicsGLRegisterBuffer`、`cudaGraphicsD3D9RegisterResource` 等。
- 虚拟内存管理（Driver API）：`cuMemAddressReserve`、`cuMemCreate`、`cuMemMap`、`cuMemSetAccess`。
- 驱动入口点访问：`cuGetProcAddress`、`cudaGetDriverEntryPoint`。
- 错误日志管理：环境变量 `CUDA_LOG_FILE`。

### 编译期宏
- `__CUDA_ARCH__`：仅在 device 代码中定义，值为编译目标的 compute capability（如 `800` 表示 sm_80）。
- `__CUDA_ARCH_FAMILY_SPECIFIC__`：针对 family-specific PTX 目标（如 `sm_100f`）时定义。
- `__CUDA_ARCH_SPECIFIC__`：针对 architecture-specific PTX 目标（如 `sm_90a`）时定义。

## 注意事项、限制条件与常见陷阱

### 1. 硬件能力门槛
- **sm_80+ (Ampere)**：`cuda::memcpy_async`、TMA、异步屏障的硬件加速、`cuda::pipeline` 的某些优化才生效。在旧架构上这些 API 可能无法编译或退化为同步实现。
- **sm_90+ (Hopper)**：Programmatic Dependent Launch 需要 Hopper 的调度硬件支持。
- **sm_100+ (Blackwell)**：Work Stealing / Cluster Launch Control 需要 Blackwell 的块级调度能力。
- **CUDA 13.1+**：Green Contexts 需要运行时/驱动版本同时支持；低于此版本的环境不可用。
- **NVLink-C2C**：Extended GPU Memory 仅在该互联架构的系统上有效。

### 2. Cluster 启动的互斥属性
- `__cluster_dims__` 与 `__block_size__` 的第二个 tuple **不能同时出现**。
- 当使用 `__block_size__` 的第二个 tuple 时，`<<<...>>>` 中的第一参数表示 **cluster 数量**，而非 block 数量。

### 3. Graph Capture 限制
- 并非所有 CUDA API 都能被图捕获。例如 `cudaStreamSynchronize`、`cudaEventQuery` 以及某些版本的 `cudaFreeAsync` 在捕获期间会返回 `cudaErrorStreamCaptureUnsupported`。
- 捕获期间若发生错误，建议在 `cudaStreamEndCapture` 后再调用 `cudaGetLastError()` 检查，因为在 capture 内调用 `cudaGetLastError` 本身也可能不被允许。

### 4. 动态并行的编译要求
- 必须在 `nvcc` 中加上 `-rdc=true`（Relocatable Device Code），并链接 `cudadevrt` 库。忘记此步骤会导致链接错误。

### 5. PTX 兼容性陷阱
- Architecture-specific PTX（如 `sm_90a`）**不具备前向/后向兼容性**，只能在精确匹配的架构上运行。
- Family-specific PTX（如 `sm_100f`）仅能在同一家族内前向兼容，不能向后兼容。
- 由低版本 PTX JIT 编译到高版本架构的二进制，**不会使用新架构的专属指令**（如 Pascal PTX 编译到 Volta 不会自动使用 Tensor Core），可能导致性能损失。

### 6. Stream-Ordered 内存分配回退
- `cudaMallocAsync` 需要驱动支持内存池（memory pool）。在部分旧驱动或特定配置下会返回 `cudaErrorNotSupported`，生产代码应准备回退到 `cudaMalloc` / `cudaFree`。

## 底层硬件行为解释

### 异步屏障与异步拷贝
在 Ampere 及更新的架构中，GPU 引入了**独立的异步拷贝单元**（Async Copy Engines）。`cuda::memcpy_async` 会生成 `cp.async` PTX 指令，这些指令由专门的硬件队列处理，可以在 SM 执行整数/浮点运算的同时，将全局内存数据直接搬运到共享内存，无需经过寄存器文件。异步屏障（`cuda::barrier`）则利用硬件的 barrier arrival/await 计数器，允许线程在“到达”屏障后继续执行不依赖同步结果的前置计算，直到显式“等待”时才阻塞，从而隐藏同步延迟。

### Cluster Launch Control 与 Work Stealing
Blackwell（sm_100+）的硬件调度器允许同一个 cluster 内的 thread block 看到彼此的启动状态。通过 cluster launch control，一个已经执行完毕的 block 可以向硬件发送信号，**取消尚未启动的 peer block 的调度**，并直接接管其 grid/block 索引，立即在该 SM 上执行新的工作项。这避免了传统硬件调度器按顺序派发 block 带来的空闲等待，对负载不均衡（如稀疏矩阵、图遍历）的场景能显著提升 SM 利用率。

### Green Contexts 的 SM 隔离
默认情况下，CUDA 上下文可以使用 GPU 上的任意 SM。Green Contexts（Execution Contexts）在硬件层面维护了一张**SM 掩码**。当 kernel 被提交到 Green Context 时，硬件调度器只会将 block 派发到掩码允许的 SM 子集；同时，其他上下文（包括 Primary Context）的调度器被禁止向这些 SM 发送任务。这为低延迟、高优先级任务提供了类似 CPU 绑核（core pinning）的 QoS 保障。

### Programmatic Dependent Launch 的重叠执行
在 Hopper（sm_90+）上，GPU 支持**跨 kernel 的依赖信号机制**。下游 kernel 在启动后可以被硬件“提前释放”到执行状态，执行与前序 kernel 数据无关的 setup 代码；当它需要读取上游结果时，会阻塞在一条硬件信号量上。上游 kernel 在产出所需数据后，通过 PTX 指令向该信号量写入，立即解除下游 kernel 的阻塞。这样两个 kernel 在时间轴上部分重叠，既保持了数据依赖的正确性，又减少了整体流水线空置时间。
