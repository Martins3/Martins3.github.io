## cuda green context
<!-- 6713102c-2e21-4dca-b4d1-5b75efe5a018 -->

简单记忆，就是做一个 GPU 资源的独占。

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/green-contexts.html>

本章节介绍 CUDA 13.1 起在 Runtime API 中正式暴露的 **Green Contexts（GC，绿色上下文）** 机制。
Green Context 是一种轻量级的执行上下文（Execution Context），它在创建阶段即与一组特定的 GPU 硬件资源绑定，
目前支持对 **Streaming Multiprocessors（SM）** 和 **Work Queues（WQ，工作队列）** 进行静态分区。
通过 Green Context，开发者可以在不修改任何设备端内核代码的前提下，仅通过主机侧少量的 API 调用，
实现对 GPU 计算资源的显式隔离与分配。其核心目标是降低多流并发执行时的资源争抢，确保延迟敏感型任务能够尽快获得可用的 SM 资源并开始执行。

在 CUDA 13.1 之前，Green Context 的功能仅通过 CUDA Driver API 提供；
从 CUDA 13.1 开始，Runtime API 引入了 `cudaExecutionContext_t` 这一统一抽象，它既可以代表传统的 Primary Context（运行时用户默认隐式交互的上下文），也可以代表一个 Green Context。因此，本章节的示例和说明均基于 CUDA Runtime API。

## 背景与要解决的问题

在传统的 CUDA 编程模型中，当用户通过 `<<< >>>` 或 `cudaLaunchKernel` 启动内核时，**无法直接控制该内核将占用哪些具体的 SM**。开发者只能通过调整启动配置（grid/block 大小）或影响每个 SM 上的活跃线程块数量来间接触发调度器的行为。当多个内核通过不同的非阻塞流（non-blocking streams）或 CUDA Graph 并发提交到 GPU 时，它们可能在硬件层面竞争同一批 SM 资源。这种情况下，一个长时间运行的后台内核可能占满所有可用的 SM，导致随后提交的高优先级、延迟敏感的关键内核（critical kernel）被迫等待，直到后台内核的部分线程块释放资源后才能开始执行。

除了 SM 资源外，**工作队列（Work Queues）** 也是影响并发度的重要因素。工作队列是驱动内部用于管理提交顺序和依赖跟踪的抽象资源。如果两个逻辑上独立的流有序任务被映射到同一个工作队列，驱动可能引入一种“伪依赖”（false dependence），导致这些任务被串行化执行，即便此时 SM 资源仍然空闲。虽然用户可以通过环境变量 `CUDA_DEVICE_MAX_CONNECTIONS` 间接影响工作队列的上限，但在此之前，Runtime API 并未提供对工作队列进行显式、细粒度分配的能力。

Green Contexts 正是为了解决上述问题而设计的。通过静态分区，开发者可以为一个后台任务创建一个拥有大部分 SM 的 Green Context，同时为关键任务创建一个拥有少量但独立 SM 的 Green Context。由于关键任务的 Green Context 拥有专属的 SM 资源，当它的内核被提交后，即使后台内核仍在全力运行，关键内核也能立即在那些预留的 SM 上启动，从而显著降低端到端延迟。同样的逻辑也适用于工作队列：开发者可以向 Green Context 提示期望的并发负载数，驱动会据此尽量为不同的 Green Context 分配不重叠的工作队列，减少伪依赖导致的串行化。

最后，文档还将 Green Contexts 与 **MIG（Multi-Instance GPU）** 和 **MPS（Multi-Process Service）** 进行了对比：
- **MIG** 在硬件层面将 GPU 静态划分为多个完全隔离的实例，适用于多应用或多租户场景，但无法解决单个应用内部不同内核之间的 SM 争抢。
- **MPS** 主要面向多进程场景（如 MPI），允许不同进程的内核同时驻留 GPU。MPS 的动态分区（active thread percentage）限制的是进程可使用的 SM 百分比，但这些 SM 的具体编号可以随时间变化；而 Green Context 的静态分区则绑定到具体的 SM 集合。MPS 的静态分区模式不允许资源超配（oversubscription），而 Green Context 允许。此外，创建 Green Context 比创建 MPS 上下文要轻量得多，因为 Green Context 共享 Primary Context 的底层结构。

## 核心概念与术语

要正确使用 Green Context，必须理解以下几个核心数据结构及其语义：

**Execution Context（执行上下文，EC）**：CUDA 13.1 引入的抽象句柄类型为 `cudaExecutionContext_t`。它代表了一个 GPU 执行环境，可以是设备的 Primary Context，也可以是通过 `cudaGreenCtxCreate` 创建的 Green Context。大量新增的 Runtime API 均以 `cudaExecutionContext_t` 作为显式参数，从而摆脱了对线程局部状态（TLS）的依赖。

**cudaDevResource（设备资源）**：这是一个 tagged union，用于描述某 GPU 上的一类具体资源。其类型字段 `type` 可取：
- `cudaDevResourceTypeSm`：SM 资源，通过联合体内的 `sm` 字段访问。
- `cudaDevResourceTypeWorkqueueConfig`：工作队列配置资源，通过 `wqConfig` 字段访问。
- `cudaDevResourceTypeWorkqueue`：预存的工作队列资源，通过 `wq` 字段访问。
- `cudaDevResourceTypeInvalid`：无效资源。

**cudaDevResourceDesc_t（资源描述符）**：一个不透明句柄，用于将一份或多份 `cudaDevResource` 封装起来，供 `cudaGreenCtxCreate` 使用。一个 Green Context 只能访问其创建描述符中所封装的资源。

**SM-type resource 的关键字段**：当 `type` 为 `cudaDevResourceTypeSm` 时，`sm` 结构体包含以下由 CUDA API 填充的字段（用户不应直接修改）：
- `smCount`：该资源包含的 SM 数量。
- `minSmPartitionSize`：对该资源进行再分区所需的最小 SM 数，架构相关。
- `smCoscheduledAlignment`：保证被协同调度的 SM 数量，与 GPU 处理集群（Processing Cluster）相关。若 `flags` 为 0，则 `smCount` 必须是该值的整数倍。
- `flags`：资源标志，目前支持的标志包括 0（默认）和 `cudaDevSmResourceGroupBackfill`。

**WorkqueueConfig 的关键字段**：
- `wqConcurrencyLimit`：用户期望能够并发执行的流有序负载数量上限，作为驱动分配工作队列的提示。
- `sharingScope`：工作队列的共享范围。`cudaDevWorkqueueConfigScopeDeviceCtx` 表示在所有上下文间共享（默认）；`cudaDevWorkqueueConfigScopeGreenCtxBalanced` 表示驱动会尽量为不同的 Green Context 分配不重叠的工作队列资源。

**coscheduledSmCount 与 preferredCoscheduledSmCount**：这两个概念服务于 **Thread Block Clusters**。`coscheduledSmCount` 定义了协同调度组的大小，确保该资源至少能启动一个该尺寸的集群。`preferredCoscheduledSmCount` 是进一步的提示，用于在 Compute Capability 10.0+ 设备上支持 preferred cluster dimensions。若用户代码不使用集群，通常将两者设为 2 或 0（使用默认值）。

## API / 机制详解

Green Context 的创建与使用可以归纳为四个主要步骤：获取资源、分区资源、生成描述符、创建上下文。此外，还需了解如何在创建好的上下文上提交工作，以及相关的辅助 API。

### 4.1 获取设备资源（Step 1）

创建 Green Context 的第一步是获取初始资源。Runtime 提供了三套 API，分别针对设备、执行上下文和流：

- `cudaError_t cudaDeviceGetDevResource(int device, cudaDevResource* resource, cudaDevResourceType type)`
- `cudaError_t cudaExecutionCtxGetDevResource(cudaExecutionContext_t ctx, cudaDevResource* resource, cudaDevResourceType type)`
- `cudaError_t cudaStreamGetDevResource(cudaStream_t hStream, cudaDevResource* resource, cudaDevResourceType type)`

其中，`cudaStreamGetDevResource` **仅支持 SM-type 资源**。对于设备作为起点的查询，调用成功后，`cudaDevResource` 的联合体内会填入该设备对应类型的全部资源。例如，查询 SM 资源后，`resource.sm.smCount` 即为该设备的总 SM 数；查询 WorkqueueConfig 后，`wqConfig.wqConcurrencyLimit` 通常等于 `CUDA_DEVICE_MAX_CONNECTIONS` 环境变量的值或其默认值。

**前置条件**：建议在调用前已通过 `cudaSetDevice` 或 `cudaInitDevice` 初始化目标设备的 Primary Context，以避免后续 Green Context 创建时的额外初始化开销。

### 4.2 同质分区：cudaDevSmResourceSplitByCount（Step 2）

在获得初始 SM 资源后，需要将其划分为一个或多个分区。`cudaDevSmResourceSplitByCount` 用于创建**同质分区**（所有结果组的 SM 数相同），其签名为：

```cpp
cudaError_t cudaDevSmResourceSplitByCount(
    cudaDevResource* result, unsigned int* nbGroups,
    const cudaDevResource* input, cudaDevResource* remaining,
    unsigned int flags, unsigned int minCount);
```

**作用**：请求将 `input` SM 资源划分为 `*nbGroups` 个组，每组至少包含 `minCount` 个 SM。但由于架构相关的粒度和对齐要求，API 返回时 `*nbGroups` 可能会被**向下调整**（例如请求的组数过多导致超出总 SM 数），而每组的实际 SM 数 `N` 可能会被**向上调整**以满足对齐要求。最终结果是：`*nbGroups` 个同质组，每组 `N` 个 SM，外加一个可选的 `remaining` 剩余分区。

**关键参数**：
- `result`：输出数组，大小至少为请求时的 `*nbGroups`。可传 `nullptr` 以仅查询实际可创建的组数。
- `nbGroups`：输入/输出指针。输入为期望组数，输出为实际组数。
- `remaining`：可选的剩余分区输出。可传 `nullptr` 以忽略剩余资源。
- `flags`：默认值为 0。还可取 `cudaDevSmResourceSplitIgnoreSmCoscheduling`（降低对齐要求）或 `cudaDevSmResourceSplitMaxPotentialClusterSize`。
- `minCount`：每组期望的最小 SM 数。

**约束与风险**：
- 对于 Compute Capability 9.0，默认 `flags=0` 时，最小分区大小为 8，且每组 SM 数必须是 8 的倍数。
- `remaining` 剩余分区**不具备** `result` 中各组同等的功能或性能保证（例如不保证协同调度结构）。
- `result` 中的资源**不能再被直接分区**。若需进一步细分，必须先将其封装为描述符并创建 Green Context，再对新的 EC 调用资源查询和拆分。

### 4.3 异质分区：cudaDevSmResourceSplit（Step 2 进阶）

当不同 Green Context 需要不同数量的 SM 时，`cudaDevSmResourceSplitByCount` 的同质限制会带来不便。`cudaDevSmResourceSplit` 允许在**单次调用中创建异质分区**，其签名为：

```cpp
cudaError_t cudaDevSmResourceSplit(
    cudaDevResource* result, unsigned int nbGroups,
    const cudaDevResource* input, cudaDevResource* remainder,
    unsigned int flags, cudaDevSmResourceGroupParams* groupParams);
```

**作用**：根据 `groupParams` 数组中每个元素的规格，将 `input` 资源划分为 `nbGroups` 个组，写入 `result`。每个组可以拥有不同的 SM 数量，但绝不能为 0（除非是在 `result==nullptr` 的 dry-run 模式下）。

**groupParams 各字段详解**：

- `smCount`：控制对应结果组的 SM 数量。
  - 取值为 `0` 时进入 **discovery mode**。API 会尽可能多地分配 SM 给该组，同时满足其他约束（如 `coscheduledSmCount`）。调用成功后，`groupParams[i].smCount` 和 `result[i].sm.smCount` 会被回填为实际发现的非零值。
  - 取非零值时，必须是 **2 的倍数**，位于 `[2, input->sm.smCount]` 范围内。此外，若 `flags==0`，必须是实际 `coscheduledSmCount` 的倍数；若启用了 backfill，则只需 `>= coscheduledSmCount`。

- `coscheduledSmCount`：控制协同调度组大小，影响集群启动能力。
  - 取值为 `0` 时使用架构默认值（CC 9.0+ 默认为 8，旧架构为 2；最大值分别为 32 和 2）。
  - 取非零值时必须是 2 的倍数。
  - 它提供了一种“底层结构保证”：即使其他条件变化，该资源组至少能运行一个该尺寸的集群。此保证不适用于 `remainder`。

- `preferredCoscheduledSmCount`：作为提示，请求驱动将多个 `coscheduledSmCount` 组合并成更大的组。
  - 仅当值为实际 `coscheduledSmCount` 的倍数时有效。
  - 主要用于 CC 10.0+ 的 preferred cluster dimensions 特性。
  - 若不使用集群，建议设为与 `coscheduledSmCount` 相同，或置 0 使用默认值。

- `flags`：
  - `0`（默认）：结果 SM 数必须是实际 `coscheduledSmCount` 的倍数。
  - `cudaDevSmResourceGroupBackfill`：允许驱动向该组回填额外的 SM，使得结果 SM 数 `>=` 请求的 `smCount`。这些回填的 SM 不保证提供额外的协同调度能力，但该组仍至少支持一个 `coscheduledSmCount` 大小的集群。

**discovery mode 与 dry-run**：
若将某组的 `smCount` 设为 0，API 会在满足约束的前提下尽可能多分配 SM。若同时传 `result==nullptr`，则进入纯粹的 dry-run 模式：即使某组的实际 SM 数为 0，API 也可能返回 `cudaSuccess`。这对探索硬件支持的分配策略非常有用。

**顺序敏感性**：`groupParams` 按数组索引从左到右依次评估。若前面的组占用了大量资源，后面的组可能分不到足够的 SM。因此，将“需求固定且较小”的组放在前面，将“discovery mode”或“需求较大”的组放在后面，是一种更稳妥的策略。

### 4.4 工作队列资源配置（Step 2 补充）

除了 SM 资源外，用户还可以显式配置工作队列资源。这不能通过“split API”自动生成，而是需要手动构造一个 `cudaDevResource`，将其 `type` 设为 `cudaDevResourceTypeWorkqueueConfig`，并填充 `wqConfig` 字段：

```cpp
cudaDevResource wq_res = {};
wq_res.type = cudaDevResourceTypeWorkqueueConfig;
wq_res.wqConfig.device = device;
wq_res.wqConfig.wqConcurrencyLimit = 4;
wq_res.wqConfig.sharingScope = cudaDevWorkqueueConfigScopeGreenCtxBalanced;
```

`wqConcurrencyLimit` 向驱动提示：该 Green Context 的用户预计最多同时存在 4 个并发的流有序工作负载。驱动会尝试据此分配独立的工作队列，减少与其他 Green Context 的冲突。如果 `sharingScope` 设为 `cudaDevWorkqueueConfigScopeDeviceCtx`（默认值），则所有上下文共享同一套工作队列资源。

### 4.5 生成资源描述符（Step 3）

当所需的资源（一个或多个）准备就绪后，需要调用 `cudaDevResourceGenerateDesc` 将它们合并为一个描述符：

```cpp
cudaError_t cudaDevResourceGenerateDesc(
    cudaDevResourceDesc_t* phDesc,
    cudaDevResource* resources,
    unsigned int nbResources);
```

**作用**：将连续内存中的 `nbResources` 个 `cudaDevResource` 封装进一个 `cudaDevResourceDesc_t` 句柄。

**调用约束**：
- 所有资源必须属于同一个 GPU 设备。
- 若合并多个 SM-type 资源，它们必须来自**同一次 split API 调用**，且非 remainder 资源的 `coscheduledSmCount` 必须一致。
- 描述符中**最多只能包含一个** `WorkqueueConfig` 或 `Workqueue` 类型的资源。

**调用后得到什么**：一个可用于 `cudaGreenCtxCreate` 的不透明描述符句柄。该句柄代表了 Green Context 将能访问的全部硬件资源。

### 4.6 创建 Green Context（Step 4）

最后一步是通过描述符创建 Green Context：

```cpp
cudaError_t cudaGreenCtxCreate(
    cudaExecutionContext_t* phCtx,
    cudaDevResourceDesc_t desc,
    int device,
    unsigned int flags);
```

**作用**：在指定设备上创建一个 Green Context，其可访问的资源严格限定为 `desc` 中描述的内容。创建过程即完成资源的“供给”（provisioning）。

**参数**：
- `flags`：目前必须为 0，保留给未来扩展。
- `desc`：通过 Step 3 生成的资源描述符。

**前置条件**：强烈建议在此之前已通过 `cudaSetDevice(device)` 或 `cudaInitDevice(device, 0, 0)` 完成 Primary Context 的初始化，否则 Green Context 创建时可能触发隐式的 Primary Context 初始化，引入额外延迟。

**资源超配（Oversubscription）**：多个 Green Context 的描述符可以包含重叠的 SM 资源。例如，GC-A 使用 result[0]，GC-B 使用 result[0] 和 result[1]。此时 result[0] 的 SM 被两个 Green Context 共享。这种超配应根据具体场景谨慎使用。

### 4.7 在 Green Context 上启动工作

Green Context 本身不直接执行内核；开发者需要为该上下文创建专用的 CUDA 流：

```cpp
cudaError_t cudaExecutionCtxStreamCreate(
    cudaStream_t* phStream,
    cudaExecutionContext_t ctx,
    unsigned int flags,
    int priority);
```

**关键区别**：与 `cudaStreamCreateWithPriority` 不同，此 API 将新创建的流显式绑定到 `ctx` 代表的执行上下文上。对于 Green Context，传入的默认 flags（`0`）在语义上等价于 `cudaStreamNonBlocking`。

当通过 `<<< >>>` 或 `cudaLaunchKernel` 在此类流上启动内核时，该内核**只能使用**该 Green Context 被供给的 SM 和工作队列资源。这是 Green Context 实现资源隔离的核心机制——它不需要修改任何内核代码，仅通过“在哪个流上启动”这一主机侧决策即可生效。

**占用 API 的适配**：如果内核使用了 Thread Block Clusters，且需要将 Green Context 的 SM 限制纳入考量，可以在调用 `cudaOccupancyMaxPotentialClusterSize` 或 `cudaOccupancyMaxActiveClusters` 时，将 `cudaLaunchConfig_t` 的 `stream` 字段设为 Green Context 流。这样，占用计算会基于该 GC 的可用 SM 数而非整卡的 SM 数。

### 4.8 CUDA Graphs 与 Green Context

对于通过 CUDA Graph 启动的内核，需要注意一个与常规流启动不同的细节：**Graph 所发射的流不决定其中内核节点的 SM 资源**，该流仅用于依赖跟踪。内核节点实际运行的执行上下文是在**节点创建时**确定的。

- **Stream Capture**：如果在 Stream Capture 过程中使用的流属于某个 Green Context，则捕获生成的 Graph 中对应节点会自动继承该 GC。
- **Graph API 显式创建**：当使用 `cudaGraphAddNode` 这一多态 API 添加 `cudaGraphNodeTypeKernel` 类型节点时，需要设置 `cudaKernelNodeParamsV2` 结构体中的 `.ctx` 字段。**旧的 `cudaGraphAddKernelNode` 不支持指定执行上下文，应避免使用**。
- 同一张 Graph 中的不同节点可以属于不同的执行上下文，从而实现更细粒度的资源调度。

验证手段：使用 Nsight Systems 的 node tracing 模式（`--cuda-graph-trace node`）可以观察到各节点实际归属的 Green Context。默认的 graph tracing 模式只会将整个 Graph 显示在发射流所属的上下文下，这并不能反映各节点的真实执行上下文。

### 4.9 其他执行上下文 API

除了创建流和启动内核外，Runtime 还提供了一组以 `cudaExecutionContext_t` 为中心的同步与查询 API：

- `cudaExecutionCtxRecordEvent(ctx, event)`：记录一个事件，捕获该执行上下文**所有流**在调用时刻之前的全部活动。这比在每个流上单独调用 `cudaEventRecord` 更为便捷。
- `cudaExecutionCtxWaitEvent(ctx, event)`：让该执行上下文**所有流**上后续提交的工作等待指定事件完成。等价于在每个流上调用 `cudaStreamWaitEvent`。
- `cudaExecutionCtxSynchronize(ctx)`：阻塞 CPU，直到该执行上下文的所有工作完成。**特别注意**：如果 `ctx` 是 Primary Context，此调用还会同步该设备上所有属于它的 Green Context。
- `cudaExecutionCtxGetDevice(ctx, *device)`：查询该 EC 关联的设备 ID。
- `cudaExecutionCtxGetId(ctx, *id)`：查询该 EC 的唯一标识符。
- `cudaExecutionCtxDestroy(ctx)`：销毁显式创建的 Green Context。调用者需确保此时没有正在进行的 API 调用使用该上下文。
