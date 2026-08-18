## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/cuda-graphs.html>


CUDA Graphs 是 CUDA 提供的另一种工作提交模型。与传统逐条向 Stream 提交命令的方式不同，Graph 将一系列操作（如核函数启动、数据搬运、内存设置等）及其依赖关系预先定义为一张有向图，随后可以多次实例化并启动执行。本章系统性地介绍了图的结构与节点类型、图的创建方式（显式 API 与 Stream Capture）、实例化与执行流程、图的更新机制、条件节点、内存节点、设备端图启动以及 CUDA User Objects 等内容。其核心价值在于：将“工作定义”与“工作执行”分离，使得 CPU 启动开销被大幅摊平，同时允许 CUDA 运行时对整个工作流进行全局优化。

## 背景与要解决的问题

在传统的 CUDA Stream 编程模型中，每次向 Stream 放入一个核函数时，主机驱动都需要执行一系列准备操作：验证参数、分配资源、设置启动配置等。这些开销对于执行时间较短的轻量级核函数来说，可能占到端到端耗时的很大一部分。CUDA Graphs 的设计动机正是为了解决这一问题：通过预先一次性定义完整的工作流，驱动可以在实例化阶段完成绝大部分设置和验证工作，后续每次启动图的开销极低。此外，由于整个工作流在实例化时即对 CUDA 可见，运行时能够进行流式提交无法实现的拓扑优化、内存复用和调度优化。

## 核心概念与术语

- **Graph（cudaGraph_t）**：一个模板对象，描述了图中包含哪些节点以及节点之间的依赖边。它是图的“定义”阶段产物，本身不可直接执行。
- **Executable Graph（cudaGraphExec_t）**：由 Graph 实例化得到的可执行对象。实例化过程会验证模板、快照状态并预分配资源，之后可被反复启动。
- **Node（节点）**：图中的基本单元，代表一项具体操作。支持的节点类型包括：kernel 节点、CPU 函数回调节点（host node）、内存拷贝节点（memcpy）、内存设置节点（memset）、空节点（empty node）、事件记录/等待节点（event record/wait）、外部信号量信号/等待节点、条件节点（conditional node）、内存分配/释放节点（mem alloc/free）以及子图节点（child graph node）。
- **Edge（边）**：表示节点间的依赖关系。若节点 B 依赖于节点 A，则 A 完成后 B 方可被调度。
- **Edge Data（CUDA 12.3+）**：边可以携带额外的端口与类型信息，用于精细化控制依赖行为。 outgoing port 指定触发边的时机，incoming port 指定节点哪一部分被阻塞，type 修改端点关系。当前主要用途是启用 Programmatic Dependent Launch。
- **Capture Graph**：通过 Stream Capture 机制正在动态构建中的图。
- **Execution Environment（执行环境）**：在 Device Graph Launch 语境下，用于封装一次图启动及其所有派生 Fire-and-Forget 子图的工作边界概念，具有层次化特性。

## API / 机制详解

### 显式 Graph API（Explicit Graph APIs）

显式 API 允许开发者从零开始手动构建图结构。入口函数包括 `cudaGraphCreate` 创建空图，以及一系列 `cudaGraphAdd*` 函数（如 `cudaGraphAddKernelNode`、`cudaGraphAddMemcpyNode`、`cudaGraphAddMemsetNode` 等）向图中添加节点。添加节点时需要显式指定其前驱依赖节点数组。CUDA 12 还引入了统一的 `cudaGraphAddNode` 接口，通过 `cudaGraphNodeParams` 联合体根据 `cudaGraphNodeType` 区分节点类型。显式 API 的优势是控制精确，不依赖现有 Stream 代码；缺点是需要开发者手动管理所有依赖关系，代码量通常较大。

### Stream Capture

Stream Capture 提供了一种从现有基于 Stream 的代码生成图的能力。调用 `cudaStreamBeginCapture(stream, mode)` 将指定流置为捕获模式后，后续发往该流的所有异步操作（如 `kernel<<<...>>>()`、`cudaMemcpyAsync`、`cudaMemsetAsync`、库函数调用等）不会被真正执行，而是被追加到内部正在构建的图中，直到调用 `cudaStreamEndCapture(stream, &graph)` 返回构建好的图并结束捕获模式。

**跨流依赖与事件**：在捕获过程中，可以使用 `cudaEventRecord` 和 `cudaStreamWaitEvent` 表达跨流依赖，但要求被记录的事件必须属于同一个 capture graph。当 `cudaStreamWaitEvent` 引入另一个流时，该流会自动进入捕获模式。所有分叉出去的流最终必须通过事件等待机制“汇合（join）”回 origin stream（即调用 `cudaStreamBeginCapture` 的那个流），否则 `cudaStreamEndCapture` 会失败。

**禁止与未处理的操作**：在捕获期间，不能对被捕获的流或事件执行同步/查询操作；不能使用 legacy NULL stream（除非它以 `cudaStreamNonBlocking` 创建）；不能调用同步 API（如 `cudaMemcpy`，因为它会隐式同步 legacy stream）。如果尝试将两个独立的 capture graph 通过事件等待合并，或未指定 `cudaEventWaitExternal` 标志就等待非捕获事件，都会导致错误。

**Invalidation（失效）**：捕获过程中若发生非法操作，关联的 capture graph 会被标记为无效。此后所有涉及该图的被捕获流或捕获事件均会返回错误，直到 `cudaStreamEndCapture` 被调用以退出捕获模式（该调用本身也会返回错误并输出 NULL graph）。

**Capture Introspection**：`cudaStreamGetCaptureInfo` 可用于查询当前捕获状态、捕获 ID、底层图对象以及下一个待捕获节点的依赖信息和边数据，便于调试和动态修改依赖。

### Graph 实例化（Instantiation）

图创建完成后，必须通过 `cudaGraphInstantiate(&graphExec, graph, NULL, NULL, 0)` 进行实例化，才能得到 `cudaGraphExec_t`。实例化的作用是对模板图做快照、验证拓扑合法性、预计算调度信息并初始化与设备的交互状态。实例化是一次性成本，通常较重；但一旦完成，后续启动的开销极小。实例化时还可以传入标志，例如 `cudaGraphInstantiateFlagAutoFreeOnLaunch` 会在每次启动时自动释放图中未释放的图内存分配，适用于生产者-消费者场景。

### Graph 执行（Execution）

可执行图通过 `cudaGraphLaunch(graphExec, stream)` 启动。stream 参数仅用于与其他异步操作的排序，不限制图内部节点的并行度，也不决定节点在哪个设备上运行（这由实例化时的上下文决定）。同一个 `cudaGraphExec_t` 不能并发地启动自身；后续的启动会被隐式排序在前一次完成之后。

### Graph 更新（Update）

工作流变化时，如果拓扑不变而只有参数变化，CUDA 提供了比销毁重建更轻量的更新机制。

**Whole Graph Update**：`cudaGraphExecUpdate` 允许用一个拓扑完全相同的“更新图”去刷新已实例化图的参数。为了成功匹配节点，原始图和更新图的 API 调用顺序、依赖数组内的顺序、以及 sink node（无出边的节点）的生成顺序都必须一致。若更新失败，可通过 `errorNode` 和 `updateResult` 定位原因。失败时通常需要销毁旧 executable graph 并重新实例化。

**Individual Node Update**：当需要修改的节点数量较少时，直接调用对应 API（如 `cudaGraphExecKernelNodeSetParams`、`cudaGraphExecMemcpyNodeSetParams` 等）更为高效。这些 API 跳过拓扑比较，直接修改指定节点的参数。限制包括：kernel 节点的所属上下文和函数指针不能变（不能从非 CDP 函数切换到 CDP 函数）；memcpy/memset 节点的设备、内存类型、传输维度（仅 1D 可变）不能变；外部信号量节点的信号量数量不能变；条件节点的句柄创建顺序必须匹配，且不能修改节点参数（如 body graph 数量）。

**Individual Node Enable**：`cudaGraphNodeSetEnabled` 可用于在实例化后启用或禁用 kernel、memcpy、memset 节点。被禁用的节点在功能上等价于空节点，其参数不受影响，重新启用后仍保持最新参数。

### Conditional Graph Nodes

条件节点允许在图内部表达动态控制流（条件执行与循环），从而减轻主机 CPU 的调度负担。条件在设备端求值，类型包括：
- **IF 节点**：条件非零时执行第一个 body graph，为零时可选执行第二个 body graph。
- **WHILE 节点**：条件非零时反复执行 body graph，每次执行后重新求值。
- **SWITCH 节点**：条件值为 n 时执行第 n 个 body graph。

条件值通过 `cudaGraphConditionalHandle` 表示，由 `cudaGraphConditionalHandleCreate` 创建。设备代码中通过 `cudaGraphSetConditional(handle, value)` 设置。创建时可指定 `cudaGraphCondAssignDefault` 标志，使得每次图启动时条件值自动恢复为默认值；否则启动时值未定义。条件节点的 body graph 有严格限制：所有节点必须位于同一设备；只能包含 kernel、memcpy、memset、空节点、子图节点和条件节点；body graph 中的 kernel 不能使用 CUDA Dynamic Parallelism；memcpy/memset 只能涉及设备内存或固定的设备映射主机内存。

### Graph Memory Nodes

图内存节点允许图自身创建并持有内存分配（allocation node）和释放（free node）。分配节点在创建时即确定虚拟地址，该地址在图的整个生命周期（包括重复实例化和启动）中保持不变，因此图内其他节点可以直接引用该指针而无需每次更新。

**生命周期语义**：图分配的生命周期始于 GPU 执行到达分配节点，终于到达对应的释放节点、对应的 `cudaFreeAsync` 流调用、或 `cudaFree` 调用。图销毁不会自动释放未释放的图内存。分配与释放必须通过依赖边正确排序：任何访问该内存的操作必须位于分配节点之后、释放节点之前。

**创建方式**：可通过显式 API `cudaGraphAddNode` 指定 `cudaGraphNodeTypeMemAlloc`/`MemFree` 创建；也可以通过 Stream Capture 捕获 `cudaMallocAsync`/`cudaFreeAsync` 自动生成。

**外部访问与释放**：图分配不一定要由创建它的图来释放。如果图未释放某分配，该分配在图执行结束后仍然存在，可通过事件排序后的其他流操作或其他图访问，并在后续通过 `cudaFree`/`cudaFreeAsync` 或另一个图的 free node 释放。

**AutoFreeOnLaunch**：实例化时传入 `cudaGraphInstantiateFlagAutoFreeOnLaunch`，允许图在仍有未释放分配时被重新启动，启动时会自动插入异步释放操作。这对于单生产者多消费者场景非常有用，因为消费者无需追踪生产者分配的内存。

**内存复用**：CUDA 通过虚拟地址复用和物理内存虚拟别名（virtual aliasing）两种方式复用内存。同一图内生命周期不重叠的分配可能共用虚拟地址；顺序启动的不同图可能共用物理页。若将图切换到不同流执行（导致可能并发），则 CUDA 必须重新映射物理内存以避免数据损坏。

**Peer Access**：图分配可配置多 GPU 访问。显式 API 通过 `accessDescs` 数组指定；Stream Capture 方式则记录捕获时刻内存池的 peer accessibility 状态，后续修改池的访问权限不会影响已捕获的图。

**物理内存占用**：图内存池不会随图销毁立即将物理内存归还 OS。如需显式释放，应调用 `cudaDeviceGraphMemTrim`。可通过 `cudaDeviceGetGraphMemAttribute` 查询当前保留和实际使用的物理内存量。

### Device Graph Launch

Device Graph Launch 允许从设备端启动图，实现设备端的动态控制流（如循环、调度器），避免设备与主机间的往返延迟。

**创建要求**：必须通过 `cudaGraphInstantiateFlagDeviceLaunch` 显式实例化。图必须满足：所有节点位于单一设备；仅允许 kernel、memcpy、memset、子图节点；kernel 不能使用 CDP；memcpy 只能涉及设备内存或固定的设备映射主机内存，且操作数必须在实例化时能被当前设备访问。

**Upload**：设备图在设备启动前必须先上传到设备。可通过 `cudaGraphUpload` 显式上传、在实例化时通过 `cudaGraphInstantiateWithParams` 指定 upload stream，或先通过主机端 `cudaGraphLaunch` 隐式上传。

**设备端启动模式**：设备端不能将图启动到普通 CUDA stream，只能使用特殊的 graph launch stream：
- **Fire-and-Forget（`cudaStreamGraphFireAndForget`）**：图作为子任务立即提交，独立于启动图运行。
- **Tail Launch（`cudaStreamGraphTailLaunch`）**：图在当前图的执行环境（environment）被认为“完成”后执行，即当前图及其所有 Fire-and-Forget 子图都完成后才启动。用于表达串行依赖。最多支持 255 个 pending tail launches。
- **Sibling Launch（`cudaStreamGraphFireAndForgetAsSibling`）**：在启动图的父执行环境中启动，等价于在父环境中做 Fire-and-Forget。

**执行环境层次**：每次设备图启动都会创建自己的 execution environment，封装该图及其所有 Fire-and-Forget 子图。环境是层次化的。主机端启动时，stream environment 作为父环境。

**自启动（Self-launch）**：设备图可以通过 `cudaGetCurrentGraphExec()` 获取自身句柄，并通过 Tail Launch 重新启动自己，实现设备端循环。同一时刻只能有一个 self-launch 在队列中。

**更新限制**：设备图只能从主机端更新，更新后必须重新 upload 才能生效。设备端在更新过程中启动图会导致未定义行为。

### CUDA User Objects

CUDA User Objects 用于管理异步工作所使用的资源生命周期，特别适用于图和流捕获场景。它通过 `cudaUserObjectCreate` 创建一个带有用户指定析构回调和引用计数的对象，行为类似于 C++ 的 `shared_ptr`。创建后可通过 `cudaGraphRetainUserObject` 将引用转移给图。图克隆（`cudaGraphClone`）和实例化都会复制这些引用。当 `cudaGraphExec_t` 被销毁且其执行已完成时，引用计数才会递减，从而确保资源在异步操作完成后才被释放。这解决了事件池分配、异步资源销毁等难以与图生命周期对齐的问题。

## 典型工作流程 / 调用顺序

1. **选择创建方式**：若已有基于 Stream 的代码，使用 Stream Capture（`cudaStreamBeginCapture` ... `cudaStreamEndCapture`）；若需精确控制或没有现有流代码，使用显式 Graph API（`cudaGraphCreate` + `cudaGraphAdd*`）。
2. **定义节点与依赖**：显式 API 需手动构造参数结构体并指定依赖数组；Stream Capture 则通过正常的流编程表达依赖，注意跨流时必须 join 回 origin stream。
3. **实例化**：调用 `cudaGraphInstantiate`（或 `cudaGraphInstantiateWithParams`）得到 `cudaGraphExec_t`。若需设备端启动，传入 `cudaGraphInstantiateFlagDeviceLaunch`；若需自动释放未释放分配，传入 `cudaGraphInstantiateFlagAutoFreeOnLaunch`。
4. **（可选）上传**：设备图需调用 `cudaGraphUpload` 或在实例化时指定 upload stream。
5. **执行**：调用 `cudaGraphLaunch(graphExec, stream)` 启动图。可多次启动而无需重新实例化。
6. **（可选）更新**：若参数变化而拓扑不变，使用 Individual Node Update（如 `cudaGraphExecKernelNodeSetParams`）或 Whole Graph Update（`cudaGraphExecUpdate`）修改参数。
7. **清理**：调用 `cudaGraphExecDestroy` 销毁可执行图，调用 `cudaGraphDestroy` 销毁图模板。若使用了图内存分配且未被释放，需额外通过 `cudaFreeAsync`/`cudaFree` 释放。

## 关键限制、边界条件与兼容性

- **并发限制**：同一个 `cudaGraphExec_t` 不能与自身并发执行。多次向同一流或不同流启动同一 executable graph，CUDA 会自动将其串行化。
- **线程安全**：`cudaGraph_t` 对象不是线程安全的，多线程不得并发访问同一个 `cudaGraph_t`。`cudaGraphExec_t` 的启动虽然可被多线程调用，但同一对象的不同启动会被排序。
- **架构限制**：Device Graph Launch 和某些高级条件节点特性需要 sm_70 及以上架构。sm_61 可以运行基本的图创建、实例化、主机端启动和更新，但无法从设备端启动图，也无法使用依赖于 sm_70+ 的某些边缘数据特性。
- **Stream Capture 的流限制**：不能捕获 `cudaStreamLegacy`（默认阻塞流），但可以捕获 `cudaStreamPerThread`。捕获期间任何关联的同步 API 或 legacy stream 操作均非法。
- **图更新限制**：Whole Graph Update 要求拓扑、节点类型、依赖顺序、sink node 顺序完全一致；Individual Node Update 不能改变 kernel 的上下文、不能改变 memcpy 的内存类型或维度、不能改变外部信号量的数量等。
- **图内存地址复用**：图内不同分配若生命周期不重叠，虚拟地址可能被复用。因此不能假设图分配的指针在主机端具有唯一性，尤其不能将其用作长期哈希键。
- **Peer Access 冻结**：通过 Stream Capture 生成的内存分配节点，其 peer accessibility 在捕获时刻即冻结，后续修改内存池权限不影响已捕获图的行为。
- **物理内存重映射代价**：频繁改变图的启动流、调用 `cudaDeviceGraphMemTrim`、或在未释放分配的情况下并发启动不同图，都可能触发物理内存重映射，带来额外开销。

## 常见陷阱与调试建议

- **Capture 期间误用同步 API**：很多开发者在捕获过程中习惯性地调用 `cudaMemcpy`（同步版）或在被捕获流上调用 `cudaStreamSynchronize`，这将立即导致 capture invalidation。应全部使用异步 API（如 `cudaMemcpyAsync`）。
- **跨流捕获未汇合**：使用事件将流分叉后，务必在 `cudaStreamEndCapture` 前通过事件等待将所有流汇合回 origin stream，否则 capture 会以错误和 NULL graph 结束。
- **更新图时节点不匹配**：`cudaGraphExecUpdate` 失败最常见的原因是拓扑或节点顺序不一致。建议使用完全相同的代码路径生成原始图和更新图，并确保 sink node 的生成顺序一致。
- **设备图未 upload**：首次从设备端启动图前，若未通过主机端启动或显式 upload，会导致错误。
- **图内存泄漏**：图内存分配的生命周期独立于图对象本身。即使销毁了 `cudaGraph_t` 和 `cudaGraphExec_t`，未释放的图内存仍然存在，必须通过 `cudaFreeAsync`/`cudaFree` 或带 `AutoFreeOnLaunch` 的重新启动来释放。
- **可视化调试**：使用 `cudaGraphDebugDotPrint` 将图结构导出为 DOT 格式，配合 Graphviz 可视化，可快速检查拓扑和节点类型是否与预期一致。
- **Device Graph 与 Host Graph 混用**：不要尝试在设备端启动一个未使用 `cudaGraphInstantiateFlagDeviceLaunch` 实例化的图，也不要在设备图运行期间从主机更新它。

## 一个最小可运行示例的说明

本目录下的 `chapter_demo.cu` 是一个面向 sm_61 / CUDA 12.8 的最小可运行示例，设计目标如下：

**为什么选择显式 Graph API**：虽然 Stream Capture 更适合将已有流代码快速转为图，但显式 API 更能直接展示“节点-依赖”这一核心概念，且对应本章 4.2.2.1.1 节的典型用法。

**示例结构与演示能力**：示例构造了一条四节点的线性依赖链：
1. H2D memcpy 节点（将主机输入数据搬至设备）；
2. `scale_kernel` 节点（将输入数组按标量缩放）；
3. `sum_kernel` 节点（对数组做归约求和）；
4. D2H memcpy 节点（将求和结果搬回主机）。

这四个节点依次依赖，形成一个完整的可复用工作流。示例随后调用 `cudaGraphInstantiate` 生成 executable graph，并在循环中启动三次。

**Individual Node Update 的展示**：循环中每次都会修改 `scale` 变量的值，并调用 `cudaGraphExecKernelNodeSetParams` 更新 scale kernel 节点的参数。这对应本章 4.2.3.2 节“Individual Node Update”的内容。由于只修改了 kernel 参数（标量值），未改变函数指针、网格维度或拓扑，因此完全符合 API 的限制条件。

**sm_61 兼容性处理**：
- 示例未使用 Device Graph Launch（4.2.6），因为该特性要求 sm_70+；代码在开头检测计算能力并打印提示，说明在 sm_61 上跳过这些高级特性。
- 未使用 Conditional Nodes（4.2.4）和 Graph Memory Nodes（4.2.5）中需要特殊标志或较新驱动行为的复杂路径，确保在 GTX 1060 上直接编译运行。
- 使用的是传统 `cudaGraphAddKernelNode` 和 `cudaGraphAddMemcpyNode` 接口（而非 CUDA 12 统一节点接口），以保证与旧架构工具链的兼容性。

**验证方式**：示例在主机端预先计算期望值（`scale * sum(1..256)`），并在每次启动后与设备返回结果比对，打印 `PASS`/`FAIL`。编译命令已在 `Makefile` 中固定为 `nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2`，直接执行 `make` 即可生成 `chapter_demo.out`。
