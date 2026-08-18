## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/asynchronous-execution.html>

## CUDA 异步执行 (Asynchronous Execution) 章节总结

### 一、章节内容转述

#### 2.3.1 什么是异步并发执行
CUDA 允许主机计算、设备计算、主机到设备的数据传输、设备到主机的数据传输、设备内部的数据传输以及多设备之间的数据传输这六类操作并发（重叠）执行。这种并发能力通过**异步接口**来表达：调用发起函数（如内核启动、异步内存拷贝）后会**立即返回**，可能在操作尚未完成甚至尚未开始时就交还控制权给 CPU。当应用程序需要用到这些异步操作的结果时，必须显式地进行同步。异步接口通常提供三种同步方式：阻塞式等待、非阻塞轮询（polling）以及回调函数（callback）。需要注意的是，接口虽然是异步的，但真正的并发能力还取决于 CUDA 版本和 GPU 的计算能力。

CUDA 运行时提供了 `cudaDeviceSynchronize()` 作为阻塞式的全局同步函数，它会等待当前设备上所有已提交的工作完成。由于内核启动本身是异步的，因此常常需要调用该函数来确保结果可用。异步执行的核心 API 组件是 **CUDA Streams（流）** 和 **CUDA Events（事件）**。此外，**CUDA Graphs（图）** 允许预先定义一组异步操作构成的有向无环图（DAG），之后可以反复低开销地执行。

#### 2.3.2 CUDA 流 (Streams)
CUDA 流是一个抽象概念，代表一个**按顺序执行的操作序列**，类似于一个工作队列。程序员可以向其中添加内核启动或内存拷贝等操作，队列中的操作按入队顺序依次执行。一个应用程序可以同时使用多个流，运行时会根据 GPU 资源状态从有可用工作的流中挑选任务执行。流可以被赋予优先级，但这只是调度提示，不保证特定的执行顺序，也不会抢占正在执行的任务。

CUDA 有一个**默认流（default stream）**。所有未显式指定流参数的内核启动和同步型内存拷贝都会被放入默认流。默认流有特殊的同步语义，将在后文详细讨论。

**创建与销毁**：流通过 `cudaStreamCreate()` 创建，通过 `cudaStreamDestroy()` 销毁。如果销毁时流中仍有未完成的工作，销毁调用会等待这些工作完成后再释放资源。

**在流中启动内核**：通过尖括号语法第四个参数指定流：`kernel<<<grid, block, shared_mem, stream>>>(...)`。内核启动是异步的，CPU 可以立即执行其他任务。

**在流中执行内存传输**：使用 `cudaMemcpyAsync()` 代替 `cudaMemcpy()`，并传入流参数。注意，为了让涉及 CPU 内存的拷贝真正异步执行，主机缓冲区必须是**页锁定（pinned/page-locked）**内存。如果使用普通的可分页内存，`cudaMemcpyAsync()` 虽然能正确工作，但会退化为同步行为，无法与其他操作重叠。推荐使用 `cudaMallocHost()` 分配这类缓冲区。

**流同步**：
- `cudaStreamSynchronize(stream)`：阻塞，直到指定流中所有任务完成。
- `cudaStreamQuery(stream)`：非阻塞查询，返回 `cudaSuccess`（流已空）或 `cudaErrorNotReady`（仍有工作）。

#### 2.3.3 CUDA 事件 (Events)
CUDA 事件是插入到流中的标记，用于追踪流中任务的进度。例如，可以在两个内核之间插入一个事件，从而在不等待整个流排空的情况下，确认第一个内核已完成。事件还可以用于精确计时：通过记录两个事件并计算它们之间的时间差，可以得到内核执行或数据传输的耗时。

**创建与销毁**：使用 `cudaEventCreate()` 和 `cudaEventDestroy()`。

**插入事件**：使用 `cudaEventRecord(event, stream)` 将事件记录到指定流中。当该事件到达流头部时，会被触发并记录时间戳。

**计时示例**：在目标操作前后分别 `cudaEventRecord(start, stream)` 和 `cudaEventRecord(stop, stream)`，然后 `cudaStreamSynchronize(stream)` 等待流完成，最后调用 `cudaEventElapsedTime(&ms, start, stop)` 获取毫秒级耗时。

**事件状态检查**：
- `cudaEventSynchronize(event)`：阻塞直到该事件被触发。
- `cudaEventQuery(event)`：非阻塞查询，返回 `cudaSuccess`（事件已触发）或 `cudaErrorNotReady`。

#### 2.3.4 流回调函数 (Callback Functions)
CUDA 提供从流内部触发主机端函数执行的机制。应优先使用 `cudaLaunchHostFunc()`，`cudaStreamAddCallback()` 已被标记为废弃。

`cudaLaunchHostFunc(stream, func, data)` 会在流执行到该回调时，在主机端调用 `func(data)`。**回调函数内部不允许调用任何 CUDA API**。对于统一内存（Unified Memory），回调执行期间其所在流被视为空闲状态，回调的开始等价于同步了流中前一个事件。

**异步错误处理**：流中的错误（内核失败、传输错误等）可能不会立即返回给主机，直到流被同步时才暴露。可以使用 `cudaGetLastError()`（获取并清除最后一个错误）或 `cudaPeekAtLastError()`（只获取不清除）来检查。对应的错误名称和描述可通过 `cudaGetErrorName()` 和 `cudaGetErrorString()` 获取。**调试技巧**：设置环境变量 `CUDA_LAUNCH_BLOCKING=1` 可让每个内核启动后自动同步，从而快速定位出错的内核，但会严重降低性能。

#### 2.3.5 CUDA 流的执行顺序
CUDA 流本质上是**顺序流（in-order streams）**。操作按入队顺序执行，后入队的操作不能超越先入队的操作。内存拷贝操作会被运行时追踪，确保在后续依赖该数据的操作开始之前完成。某些特殊优化（如 Programmatic Dependent Launch 或 `cudaMemcpyBatchAsync`）可能会在满足依赖的前提下并发执行非重叠的批量拷贝。

#### 2.3.6 阻塞流、非阻塞流与默认流
流分为**阻塞流（blocking）**和**非阻塞流（non-blocking）**，这一区分仅针对它们与**传统默认流（legacy default stream）**的同步行为。

- `cudaStreamCreate()` 创建的流默认是阻塞流。
- 使用 `cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking)` 可创建非阻塞流。

**传统默认流（Legacy Default Stream / NULL stream / stream 0）**：
这是所有主机线程共享的默认流。当一个操作被提交到传统默认流时，它会与所有其他**阻塞流**进行隐式同步——即默认流中的操作会等待所有阻塞流完成，反之，阻塞流中的后续操作也会等待默认流中的操作完成。这意味着，如果不使用非阻塞流，即使创建了多个流，它们也会通过默认流相互阻塞，失去并发能力。

**Per-thread Default Stream（CUDA 7+）**：
通过编译选项 `--default-stream per-thread` 或预定义宏 `CUDA_API_PER_THREAD_DEFAULT_STREAM`，可以让每个主机线程拥有独立的默认流。启用后，默认流不再与所有阻塞流全局同步，其行为类似于非阻塞流。

#### 2.3.7 显式同步
常用的显式同步机制包括：
- `cudaDeviceSynchronize()`：等待当前设备上所有流的所有任务完成。
- `cudaStreamSynchronize(stream)`：等待指定流完成。
- `cudaStreamWaitEvent(stream, event)`：让该流中此后入队的操作等待某个事件完成后再执行。这是实现**跨流依赖**的关键 API。
- `cudaStreamQuery(stream)`：非阻塞查询流是否已空。

#### 2.3.8 隐式同步
如果在两个不同流的操作之间插入了任何针对**传统默认流（NULL stream）**的操作，那么这两个流的操作通常无法并发，除非这些流被显式创建为非阻塞流。优化并发的最佳实践：
1. 先提交所有相互独立的操作，再提交有依赖的操作。
2. 尽可能推迟任何形式的同步。

#### 2.3.9 其他与高级主题
**流优先级（Stream Prioritization）**：
通过 `cudaStreamCreateWithPriority()` 创建带优先级的流。优先级范围可通过 `cudaDeviceGetStreamPriorityRange()` 查询，数值越小优先级越高。默认优先级为 0。优先级只是提示，不保证抢占或特定执行顺序。

**CUDA Graphs 与流捕获（Stream Capture）**：
对于需要反复执行的固定操作序列，可以先通过 `cudaStreamBeginCapture()` / `cudaStreamEndCapture()` 将其捕获为一个 `cudaGraph_t`，然后通过 `cudaGraphInstantiate()` 实例化为可执行图 `cudaGraphExec_t`，之后多次调用 `cudaGraphLaunch()` 执行。这样可以显著降低 CPU 发起调用的开销。

---

### 二、核心概念速查

| 概念 | 说明 |
|------|------|
| **异步执行** | 调用立即返回，操作在后台执行，需要显式同步获取结果。 |
| **CUDA Stream** | 操作的顺序队列，流内操作按 FIFO 执行；多流之间可并发。 |
| **CUDA Event** | 流中的标记点，用于同步、跨流依赖和精确计时。 |
| **Pinned/页锁定内存** | `cudaMallocHost()` 分配的内存，是实现真正异步 H<->D 传输的前提。 |
| **默认流** | 未显式指定流时使用的流。传统默认流会与所有阻塞流隐式同步。 |
| **阻塞流 vs 非阻塞流** | 区别在于是否与传统默认流隐式同步。多流并发应使用非阻塞流。 |
| **CUDA Graph** | 预定义的操作图，实例化后可低开销重复执行。 |

### 三、关键 API / 函数

**流管理**
- `cudaStreamCreate(cudaStream_t *stream)`
- `cudaStreamCreateWithFlags(cudaStream_t *stream, unsigned int flags)` （如 `cudaStreamNonBlocking`）
- `cudaStreamCreateWithPriority(cudaStream_t *stream, unsigned int flags, int priority)`
- `cudaStreamDestroy(cudaStream_t stream)`
- `cudaStreamSynchronize(cudaStream_t stream)`
- `cudaStreamQuery(cudaStream_t stream)`
- `cudaStreamWaitEvent(cudaStream_t stream, cudaEvent_t event, unsigned int flags)`

**事件管理**
- `cudaEventCreate(cudaEvent_t *event)`
- `cudaEventDestroy(cudaEvent_t event)`
- `cudaEventRecord(cudaEvent_t event, cudaStream_t stream)`
- `cudaEventSynchronize(cudaEvent_t event)`
- `cudaEventQuery(cudaEvent_t event)`
- `cudaEventElapsedTime(float *ms, cudaEvent_t start, cudaEvent_t stop)`

**异步内存拷贝**
- `cudaMemcpyAsync(void *dst, const void *src, size_t count, cudaMemcpyKind kind, cudaStream_t stream)`
- `cudaMallocHost(void **ptr, size_t size)` / `cudaFreeHost(void *ptr)`

**回调与错误**
- `cudaLaunchHostFunc(cudaStream_t stream, cudaHostFn_t fn, void *userData)`
- `cudaGetLastError()` / `cudaPeekAtLastError()`
- `cudaGetErrorName(cudaError_t error)` / `cudaGetErrorString(cudaError_t error)`

**图（Graph）**
- `cudaStreamBeginCapture(cudaStream_t stream, cudaStreamCaptureMode mode)`
- `cudaStreamEndCapture(cudaStream_t stream, cudaGraph_t *pGraph)`
- `cudaGraphInstantiate(cudaGraphExec_t *pGraphExec, cudaGraph_t graph, cudaGraphNode_t *pErrorNode, char *pLogBuffer, size_t bufferSize)`
- `cudaGraphLaunch(cudaGraphExec_t graphExec, cudaStream_t stream)`

### 四、注意事项与常见陷阱

1. **页锁定内存是异步传输的前提**
   - 误用普通 `malloc` 分配的内存传给 `cudaMemcpyAsync` 会导致隐式同步，失去重叠执行的优势。

2. **默认流的隐式同步陷阱**
   - 使用 `cudaStreamCreate` 创建多个流后，如果在代码中不经意地调用了未指定流的内核启动或同步型 `cudaMemcpy`，这些操作会进入传统默认流，从而与所有阻塞流产生隐式同步，彻底破坏并发。
   - **最佳实践**：多流并发时，始终使用 `cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking)` 创建非阻塞流。

3. **同步位置过早泄露性能**
   - 在提交完所有独立操作之前就调用 `cudaStreamSynchronize` 或 `cudaDeviceSynchronize` 会打断并发流水线。

4. **事件计时需同步**
   - 调用 `cudaEventElapsedTime` 之前，必须确保两个事件都已触发（通常通过 `cudaStreamSynchronize` 或 `cudaEventSynchronize`），否则结果为未定义。

5. **回调函数中禁止调用 CUDA API**
   - 在 `cudaLaunchHostFunc` 注册的回调里调用任何 CUDA API（包括内存分配、内核启动等）会导致未定义行为或程序崩溃。

6. **异步错误延迟暴露**
   - 内核启动失败不会立刻在启动语句处抛出错误，往往要到后续同步时才发现。应在关键同步点后检查 `cudaGetLastError()`。

7. **流优先级并非实时调度保证**
   - 高优先级流不会抢占已经在 GPU 上执行的低优先级任务，只是在新任务调度时更有可能被优先选择。

8. **Graph 捕获期间禁止某些操作**
   - 在 `cudaStreamBeginCapture` 和 `cudaStreamEndCapture` 之间，不能进行同步型 CUDA 调用、设备内存分配等可能破坏捕获一致性的操作。

9. **销毁流会隐式等待**
   - `cudaStreamDestroy` 在流非空时会阻塞等待所有工作完成。如果希望在销毁前确保完成，建议显式同步以便更精确地控制时机和错误处理。
