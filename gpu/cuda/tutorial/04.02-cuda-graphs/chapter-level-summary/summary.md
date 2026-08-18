## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/cuda-graphs.html>

## CUDA Graphs 概述

CUDA Graphs 是 CUDA 提供的另一种任务提交模型。与基于流的逐次提交不同，Graph 将一系列操作（如核函数启动、数据拷贝等）及其依赖关系预先定义好，之后可以重复执行。这种“定义一次、多次启动”的方式能够显著降低 CPU 启动开销，并让 CUDA 驱动对整个工作流进行全局优化。

### 核心优势

- **降低 CPU 开销**：核函数的设置和启动准备工作在实例化阶段一次性完成，后续每次启动开销极小。
- **全局优化**：CUDA 可以看到整个工作流，从而应用流式提交无法实现的优化。

## 核心概念

### 1. 图的结构

- **节点（Node）**：代表一个操作，如核函数、内存拷贝、memset、Host 函数、事件记录/等待、外部信号量操作、条件节点、内存分配/释放节点或子图。
- **边（Edge）**：表示节点间的依赖关系，约束执行顺序。一旦某个节点的所有上游依赖完成，CUDA 即可调度该节点执行。

### 2. 边数据（Edge Data，CUDA 12.3+）

边数据可以修改依赖行为，包含：
- **出端口（outgoing port）**：指定上游任务完成到何种程度才触发边。
- **入端口（incoming port）**：指定下游任务的哪部分依赖该边。
- **类型（type）**：修改端点间的关系。零值表示默认的完全依赖（含内存同步）。

目前仅核函数节点定义了额外的出端口，唯一的非默认依赖类型是 `cudaGraphDependencyTypeProgrammatic`（用于 Programmatic Dependent Launch）。

### 3. 三个阶段

- **定义（Definition）**：使用显式 Graph API 或流捕获（Stream Capture）创建图模板（`cudaGraph_t`）。
- **实例化（Instantiation）**：`cudaGraphInstantiate` 对图模板进行验证、快照和初始化，生成**可执行图**（`cudaGraphExec_t`）。
- **执行（Execution）**：`cudaGraphLaunch` 将可执行图提交到流中，可重复启动而无需重新实例化。

## 构建与执行

### 显式 Graph API

使用 `cudaGraphCreate` 创建空图，再通过 `cudaGraphAddNode`、`cudaGraphAddKernelNode`、`cudaGraphAddMemcpyNode` 等添加节点和依赖。适合需要精细控制图结构的场景。

### 流捕获（Stream Capture）

通过 `cudaStreamBeginCapture(stream)` 和 `cudaStreamEndCapture(stream, &graph)` 将一段基于流的代码自动转换为图。

- **不支持 `cudaStreamLegacy`（NULL stream）**，但支持 `cudaStreamPerThread`。
- **跨流依赖**：在捕获过程中使用 `cudaEventRecord` 和 `cudaStreamWaitEvent` 可被正确捕获，但所有被捕获的流最终必须合并回**起始流（origin stream）**。
- **禁止操作**：在捕获期间，不能同步或查询被捕获的流/事件，不能使用同步 API（如 `cudaMemcpy`），不能使用 legacy stream。
- **失效**：任何非法操作会使捕获图失效，后续相关流操作都会报错，直到调用 `cudaStreamEndCapture` 结束捕获（此时返回错误和 NULL graph）。

## 图更新

当工作流参数变化但拓扑结构不变时，CUDA 提供了轻量级更新机制，比重实例化更高效。

### 整体更新（Whole Graph Update）

使用 `cudaGraphExecUpdate` 将一个拓扑完全相同的图模板的参数更新到现有可执行图。要求：
- 节点类型、拓扑、依赖顺序必须完全一致。
- Sink 节点（无出边的节点）的创建/移除顺序必须一致。

### 单个节点更新（Individual Node Update）

直接修改可执行图中特定节点的参数，适用于少量修改的场景，跳过拓扑比较，效率更高。例如：
- `cudaGraphExecKernelNodeSetParams`
- `cudaGraphExecMemcpyNodeSetParams`
- `cudaGraphExecMemsetNodeSetParams`
- `cudaGraphExecHostNodeSetParams`

### 启用/禁用节点

使用 `cudaGraphNodeSetEnabled` / `cudaGraphNodeGetEnabled` 可以临时禁用核函数、memcpy、memset 节点。被禁用的节点在功能上等价于空节点，参数不受影响。

### 更新限制

- **核函数节点**：不能更改上下文，不能从非 CDP 更新为使用 CDP。
- **memcpy/memset 节点**：操作数设备不能变，必须在同一上下文中分配，仅 1D 节点可修改；不能更改内存类型或传输类型（`cudaMemcpyKind`）。
- **外部信号量节点**：不能更改信号量数量。
- **条件节点**：不能更改节点参数（如条件体数量、节点上下文等），但条件体内部节点的参数更新受上述规则约束。

## 条件节点（Conditional Nodes）

条件节点允许在图内部进行条件执行和循环，减少 CPU 参与。

- **IF 节点**：条件非零时执行一个体图，可选为零时执行另一个体图。
- **WHILE 节点**：条件非零时重复执行体图。
- **SWITCH 节点**：根据条件值执行第 n 个体图。

条件值通过 `cudaGraphConditionalHandle` 表示，由设备代码通过 `cudaGraphSetConditional` 设置。创建时可指定默认值标志 `cudaGraphCondAssignDefault`。条件体图只能包含核函数、memcpy、memset、空节点、子图和条件节点，且必须位于单一设备上，不允许使用 CUDA Dynamic Parallelism。

## 图内存节点（Graph Memory Nodes）

图内存节点允许图自身创建和拥有内存分配，生命周期按 GPU 执行顺序管理，与 `cudaMallocAsync`/`cudaFreeAsync` 语义一致。

- **固定虚拟地址**：图分配在节点的生命周期内拥有固定的虚拟地址，可被图中其他节点直接引用。
- **分配节点**（`cudaGraphNodeTypeMemAlloc`）和**释放节点**（`cudaGraphNodeTypeMemFree`）通过依赖边排序。
- **图外访问**：未在图内释放的分配可以跨图或通过流操作访问，但必须通过事件等机制建立正确的执行顺序。
- **Auto Free on Launch**：使用 `cudaGraphInstantiateFlagAutoFreeOnLaunch` 实例化图后，每次启动会自动异步释放未释放的分配，便于“单生产者多消费者”模式。
- **内存裁剪**：`cudaDeviceGraphMemTrim` 可将未使用的物理内存归还 OS；`cudaDeviceGetGraphMemAttribute` 可查询内存占用。

## 设备端图启动（Device Graph Launch）

允许从设备端（GPU 核函数内部）启动图，实现动态控制流而无需回传 CPU。

- **实例化**：必须通过 `cudaGraphInstantiateFlagDeviceLaunch` 显式实例化。
- **上传**：首次设备端启动前需通过 `cudaGraphUpload` 或隐式 host launch 将图上传到设备。
- **启动模式**：
  - `cudaStreamGraphFireAndForget`：立即独立执行。
  - `cudaStreamGraphTailLaunch`：当前图环境完成后串行执行。
  - `cudaStreamGraphFireAndForgetAsSibling`：在父环境中作为兄弟图执行。
- **限制**：设备图只能从单一设备启动；只能包含核函数、memcpy、memset、子图节点；不支持 CDP；不能同时从设备端重复启动同一个图。

## CUDA 用户对象（CUDA User Objects）

用于管理异步工作所需资源的生命周期，特别适用于图和流捕获。

- `cudaUserObjectCreate` 创建带有析构回调和引用计数的用户对象。
- `cudaGraphRetainUserObject` 将对象引用关联到图中。图克隆、实例化时会保留引用，可执行图销毁/同步完成后才释放引用。

## 关键 API/函数汇总

| 功能 | API |
|------|-----|
| 创建图 | `cudaGraphCreate(&graph, 0)` |
| 添加核函数节点 | `cudaGraphAddKernelNode` / `cudaGraphAddNode` |
| 添加 memcpy/memset 节点 | `cudaGraphAddMemcpyNode` / `cudaGraphAddMemsetNode` |
| 添加内存分配/释放节点 | `cudaGraphAddNode`（`cudaGraphNodeTypeMemAlloc` / `MemFree`） |
| 流捕获开始/结束 | `cudaStreamBeginCapture` / `cudaStreamEndCapture` |
| 实例化 | `cudaGraphInstantiate` / `cudaGraphInstantiateWithFlags` / `cudaGraphInstantiateWithParams` |
| 启动 | `cudaGraphLaunch(graphExec, stream)` |
| 整体更新 | `cudaGraphExecUpdate` |
| 单个节点更新 | `cudaGraphExecKernelNodeSetParams` 等系列 API |
| 启用/禁用节点 | `cudaGraphNodeSetEnabled` / `cudaGraphNodeGetEnabled` |
| 条件句柄 | `cudaGraphConditionalHandleCreate` |
| 设备端设置条件 | `cudaGraphSetConditional`（设备端） |
| 设备端获取当前图 | `cudaGetCurrentGraphExec`（设备端） |
| 图上传 | `cudaGraphUpload` |
| 内存裁剪/查询 | `cudaDeviceGraphMemTrim` / `cudaDeviceGetGraphMemAttribute` |
| 用户对象 | `cudaUserObjectCreate` / `cudaGraphRetainUserObject` |

## 注意事项与常见陷阱

1. **流捕获期间严禁同步**：在 `cudaStreamBeginCapture` 和 `cudaStreamEndCapture` 之间调用 `cudaMemcpy`（同步版）、`cudaStreamSynchronize`、`cudaDeviceSynchronize` 或查询被捕获流的状态都是非法的。
2. **NULL Stream 限制**：进行流捕获时，任何非 `cudaStreamNonBlocking` 的流都会使 legacy NULL stream 的使用变为非法。
3. **跨流捕获必须合并回起始流**：通过事件分叉到其它流后，最终必须通过事件将其他流合并回调用 `cudaStreamBeginCapture` 的起始流，否则捕获失败。
4. **图更新对顺序敏感**：使用 `cudaGraphExecUpdate` 时，不仅拓扑要相同，API 调用顺序、依赖数组中的顺序、sink 节点的生成顺序也必须一致。
5. **设备图不能自并发**：`cudaGraphExec_t` 不能并发地自我运行，后续启动会自动排在前一次完成后。
6. **设备端启动限制**：从设备端启动同一个图时，不能在前一次设备端启动尚未完成时再次启动（会返回 `cudaErrorInvalidValue`）。
7. **图内存未自动释放**：图的销毁不会自动释放图内存节点产生的未释放内存，必须通过显式 free 节点、`cudaFreeAsync`/`cudaFree` 或 `AutoFreeOnLaunch` 标志来避免泄漏。
8. **物理内存重映射开销**：频繁切换图启动的流、或在不同流中并发启动含内存节点的图，可能导致 CUDA 重新映射物理内存，引入额外开销。建议在同一个流中启动以复用映射。
9. **子图内存节点限制**（CUDA 12.9+）：包含内存分配/释放的子图在被移动到父图后，不能再独立实例化、更新或添加到其他父图中，也不能再新增内存节点。
10. **`cudaGraph_t` 非线程安全**：多个线程不能并发访问同一个 `cudaGraph_t` 对象，需由用户自行加锁保护。
