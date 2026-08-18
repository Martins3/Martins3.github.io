## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/advanced-host-programming.html>

## 核心概念

### 1. cudaLaunchKernelEx — 扩展内核启动接口

传统的 `<<< >>>` 三角括号启动方式只能配置四个参数：网格维度、线程块维度、动态共享内存大小、流。
`cudaLaunchKernelEx` 通过 `cudaLaunchConfig_t` 结构体封装了上述基本参数，
并允许附加一个或多个 `cudaLaunchAttribute`，从而在主机端向运行时传递更丰富的启动提示。例如：

- `cudaLaunchAttributePreferredSharedMemoryCarveout`：建议 L1/Shared Memory 的分配比例。
- `cudaLaunchAttributeClusterDimension`：指定线程块集群（cluster）维度。
- `cudaLaunchAttributeProgrammaticStreamSerialization`：启用 Programmatic Dependent Launch（PDL）。

该接口不改变内核代码本身，但能从主机侧影响调度行为与性能。

### 2. Thread Block Clusters（线程块集群）

它保证同一个 cluster 内的所有线程块被同时调度到同一个 GPC（Graphics Processing Cluster）上执行，
从而允许跨多个 SM 的线程之间进行数据交换与同步。

集群的指定方式有三种：

1. **编译时注解 `__cluster_dims__(x,y,z)`**：内核定义时固定 cluster 大小，启动时网格维度（block 数）必须是 cluster 大小的整数倍。
2. **运行时通过 `cudaLaunchKernelEx` + `cudaLaunchAttributeClusterDimension`**：不修改内核定义，可在每次启动时动态调整 cluster 大小，但同样要求网格维度可被 cluster 维度整除。
3. **`__block_size__((bx,by,bz), (cx,cy,cz))` 注解**：同时声明线程块大小和 cluster 大小。此时 `<<< >>>` 的第一个参数表示**集群数量**而非线程块数量；第二个参数必须传 `1`（表示默认共享内存），第三个参数传 `0`，最后才是流。该注解不能与 `__cluster_dims__` 同时使用。

还可以使用 `cudaLaunchAttributePreferredClusterDimension` 指定“首选” cluster 大小。
它必须是最小 cluster 大小的整数倍。运行时可能以最小大小或首选大小执行，内核必须对两种大小都能正确工作。

### 3. Streams 与 Events 的深入使用

CUDA 流内的操作默认按提交顺序串行执行（PDL 除外）。不同流之间的操作在满足资源与依赖条件时可以并发执行。

**隐式同步的陷阱**：
页锁定内存分配、设备内存分配、设备内存设置、同一设备内存之间的拷贝、NULL stream 上的任何操作、L1/shared 配置切换，都会在不同流之间引入隐式同步。

**最佳实践**：
- 创建非阻塞流：`cudaStreamCreateWithFlags(..., cudaStreamNonBlocking)`，避免 NULL stream 的隐式同步。
- 同步粒度最小化：等待单个流用 `cudaStreamSynchronize`，等待单个事件用 `cudaEventSynchronize`，避免直接使用 `cudaDeviceSynchronize`。
- 非阻塞轮询：`cudaStreamQuery` / `cudaEventQuery` 可在不阻塞 CPU 的情况下检查完成状态。
- 跨流依赖推荐禁用计时的 Event：`cudaEventCreateWithFlags(..., cudaEventDisableTiming)` 可提升仅用于同步的事件的性能。

**流优先级**：
通过 `cudaStreamCreateWithPriority` 可指定流的相对优先级。优先级只是**提示（hint）**，高优先级流的待处理任务在调度时更可能被优先选中，但**不会抢占**正在运行的低优先级任务，GPU 也不会在任务执行期间重新评估队列。

### 4. Programmatic Dependent Kernel Launch（PDL）

PDL 用于在**同一个流中**让两个存在数据依赖的内核实现**部分重叠执行**。传统流语义保证内核按顺序执行，但如果：
- 第一个内核（primary）在尚未全部结束时，已经把后续内核所需的数据写回全局内存；
- 第二个内核（secondary）在开始阶段有一些不依赖 primary 结果的独立计算；
那么就可以通过 PDL 让两者重叠，掩盖启动开销与执行延迟。

PDL 的三个核心组件：
1. **Primary 内核**调用 `cudaTriggerProgrammaticLaunchCompletion()`，表示“后续依赖内核所需的数据已就绪”。
2. **Secondary 内核**调用 `cudaGridDependencySynchronize()`，表示“独立部分已完成，现在阻塞等待 primary 的触发信号”。
3. Secondary 内核启动时必须携带 `cudaLaunchAttributeProgrammaticStreamSerialization` 属性，且 `programmaticStreamSerializationAllowed = 1`。

**启动顺序**：先提交 secondary 内核（它会阻塞在同步点），再提交 primary 内核；当 primary 触发后，secondary 的依赖部分才开始执行。

### 5. Batched Memory Transfers（批量内存拷贝）

`cudaMemcpyBatchAsync`（及其 3D 版本）将多个小规模拷贝合并为一次 API 调用，从而摊销 CPU 与驱动层的调度开销。运行时还能根据属性提示做进一步优化（如选择 Copy Engine 或 SM 执行拷贝、重叠计算等）。

关键设计点：
- 除了 `srcs`、`dsts`、`sizes` 数组外，还需提供 `cudaMemcpyAttributes` 属性数组及属性索引数组 `attrsIdxs`。
- `attrsIdxs[i]` 表示 `attrs[i]` 从第几个拷贝项开始生效，直到下一个索引或数组末尾。这样可以在同一个批次中为不同拷贝项配置不同属性。
- `srcAccessOrder` 决定源指针的生命周期与同步语义：
  - `cudaMemcpySrcAccessOrderStream`：源数据按流序访问，适用于固定设备/主机内存。
  - `cudaMemcpySrcAccessOrderDuringApiCall`：API 调用期间立即访问源数据，适用于**临时（ephemeral）指针**（如栈上的局部数组），因为这些指针在异步 API 返回后可能失效。
  - `cudaMemcpyAccessOrderAny`：让运行时根据系统是否有硬件统一内存或相干访问来决定最佳策略。
- 位置提示 `srcLocHint` / `dstLocHint`：使用 `cudaMemLocation` 结构体（类型+ID）提示数据当前所在位置（设备或主机 NUMA 节点），辅助运行时做预取与路径优化。
- 标志位 `flags`：
  - `cudaMemcpyFlagDefault`：默认行为。
  - `cudaMemcpyFlagPreferOverlapWithCompute`：优先使用 Copy Engine 以允许与计算重叠，目前仅在 Tegra 平台有效，非 Tegra 平台会被忽略。

### 6. 环境变量

CUDA 提供多种环境变量影响执行行为与性能：

- `CUDA_DEVICE_MAX_CONNECTIONS`：增加该值可减少不同流因映射到相同底层硬件连接资源而产生的**假依赖**。建议先用默认值，仅在发现流间意外串行化且排除其他原因（如 SM 资源不足）时再调整。在 MPS 模式下默认值更低。
- `CUDA_MODULE_LOADING=EAGER`：对延迟敏感的应用，可在初始化阶段一次性完成所有模块加载，避免在关键路径上出现懒加载开销。默认是懒加载（LAZY），也可以通过在初始化阶段做一次“warm-up”内核调用来达到类似效果。

> **注意**：环境变量应在应用启动前设置，在程序运行期间修改通常不会生效。
