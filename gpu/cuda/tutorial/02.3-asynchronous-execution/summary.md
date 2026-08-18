## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/asynchronous-execution.html>

## 2.3 异步执行 (Asynchronous Execution)

### 2.3.1 什么是异步并发执行

CUDA 允许多种任务并发（重叠）执行：
- Host 端计算
- Device 端计算
- Host to Device 内存拷贝
- Device to Host 内存拷贝
- 同一 Device 内部的内存拷贝
- 多 Device 间的内存拷贝

这种并发通过**异步接口**表达：调度函数调用或内核启动会立即返回，调用时操作可能尚未完成，甚至尚未开始。当需要最终结果时，应用必须通过某种形式的同步确保操作已完成。典型的并发模式是将内存传输与计算重叠，从而降低或消除传输开销。

异步接口通常提供三种同步方式：
- **阻塞式**：调用函数并等待操作完成
- **非阻塞式/轮询**：调用函数立即返回，提供操作状态信息
- **回调式**：操作完成后执行预注册的回调函数

实际能否并发执行取决于 CUDA 版本和硬件计算能力。核心 API 组件是 **CUDA Streams** 和 **CUDA Events**。

### 2.3.2 CUDA 流 (Streams)

流是一个抽象，用于表达一系列按顺序执行的操作。流相当于一个工作队列，程序可以向其中添加内存拷贝或内核启动等操作。同一流中的操作按入队顺序依次执行。

应用可以同时使用多个流。运行时会根据 GPU 资源状态，从有待处理工作的流中选择任务执行。流可以被分配优先级，但这只是提示，不保证特定执行顺序。

CUDA 有一个**默认流 (default stream)**。未显式指定流的操作和内核启动都会被排入默认流。

1. 创建与销毁流
```cpp
cudaStream_t stream;
cudaStreamCreate(&stream);
// ... 使用流 ...
cudaStreamDestroy(stream);
```
若销毁时流中仍有工作，流会先完成所有工作再被销毁。

2. 在流中启动内核
```cpp
kernel<<<grid, block, shared_mem_size, stream>>>(...);
```
内核启动是异步的，调用立即返回。

3. 在流中执行内存传输
```cpp
cudaMemcpyAsync(dst, src, size, cudaMemcpyHostToDevice, stream);
```
与 `cudaMemcpy()` 不同，`cudaMemcpyAsync()` 是异步的。注意：**涉及 CPU 内存的异步拷贝必须使用页锁定 (pinned/page-locked) 内存**。如果传入非页锁定内存，函数仍能正确工作，但会回退为同步行为，失去重叠执行的性能优势。推荐使用 `cudaMallocHost()` 分配这类缓冲区。

4. 流同步
- `cudaStreamSynchronize(stream)`：阻塞，直到流中所有工作完成
- `cudaStreamQuery(stream)`：非阻塞查询流状态
  - 返回 `cudaSuccess`：流已空
  - 返回 `cudaErrorNotReady`：流中还有工作

### 2.3.3 CUDA 事件 (Events)

事件是插入到流中的标记，用于跟踪流中任务的进度。事件还可用于测量时间。

1. 创建与销毁
```cpp
cudaEvent_t event;
cudaEventCreate(&event);
cudaEventDestroy(event);
```

2. 记录事件
```cpp
cudaEventRecord(event, stream);
```

3. 用事件计时
```cpp
cudaEventRecord(start, stream);
kernel<<<...>>>(...);
cudaEventRecord(stop, stream);
cudaStreamSynchronize(stream);
float elapsedTime;
cudaEventElapsedTime(&elapsedTime, start, stop);
```

4. 事件同步与查询
- `cudaEventSynchronize(event)`：阻塞直到事件完成
- `cudaEventQuery(event)`：非阻塞查询事件状态

事件可以用于构建操作和流之间的依赖图，例如通过 `cudaStreamWaitEvent()` 让一个流等待另一个流中的事件。

### 2.3.4 流回调函数 (Host Functions)

CUDA 提供从流中启动 Host 端函数的机制。推荐使用 `cudaLaunchHostFunc()`（`cudaStreamAddCallback()` 已计划弃用）。

```cpp
cudaError_t cudaLaunchHostFunc(cudaStream_t stream, void (*func)(void*), void *data);
```

**注意事项**：
- 回调函数中**不能调用任何 CUDA API**
- 回调执行期间，该流被视为空闲状态
- 流在回调完成前不会继续执行后续任务

### 2.3.5 异步错误处理

流中的错误（内核启动、内存传输等）可能不会立即传播给调用者，直到流被同步时才暴露。可通过以下函数查询：
- `cudaGetLastError()`：返回并清除当前上下文中最后一个错误
- `cudaPeekAtLastError()`：返回最后一个错误，但不清除

还可使用 `cudaGetErrorName()` 和 `cudaGetErrorString()` 获取可打印的错误信息。

**调试技巧**：设置环境变量 `CUDA_LAUNCH_BLOCKING=1` 可使每次内核启动后都强制同步，帮助定位出错的内核或传输操作（但会显著降低性能）。

### 2.3.6 CUDA 流的执行顺序

CUDA 流是**按序流 (in-order streams)**：流中操作的执行顺序与入队顺序相同。内存操作会被运行时追踪，确保在后续依赖操作开始前完成。

### 2.3.7 阻塞流、非阻塞流与默认流

- `cudaStreamCreate()` 创建的流默认是**阻塞流**
- 使用 `cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking)` 创建**非阻塞流**
- 阻塞/非阻塞的语义仅针对与**默认流 (legacy default stream / NULL stream)** 的同步行为

1. Legacy Default Stream
Legacy 默认流（stream ID 0）是所有 Host 线程共享的阻塞流。当向默认流中提交操作时，它会与所有其他阻塞流同步：默认流中的操作会等待所有阻塞流完成，反之亦然。

2. Per-thread Default Stream
从 CUDA 7 开始，可通过编译选项 `--default-stream per-thread` 或宏 `CUDA_API_PER_THREAD_DEFAULT_STREAM` 启用**每线程默认流**。启用后，每个 Host 线程拥有独立的默认流，不会与其他流产生 Legacy 默认流那样的隐式同步。

### 2.3.8 显式同步

- `cudaDeviceSynchronize()`：等待所有 Host 线程的所有流中的先前命令完成
- `cudaStreamSynchronize(stream)`：等待指定流中的先前命令完成
- `cudaStreamWaitEvent(stream, event)`：让指定流中后续命令延迟到事件完成后执行
- `cudaStreamQuery(stream)`：非阻塞查询流是否已完成所有先前命令

### 2.3.9 隐式同步

如果两个不同流的操作之间插入了默认流（NULL stream）上的操作，则这两个流的操作不能并发执行，除非这些流是非阻塞流。

提升并发执行潜力的建议：
- 先提交所有独立操作，再提交依赖操作
- 尽可能推迟同步

### 2.3.10 其他高级主题

1. 流优先级
通过 `cudaStreamCreateWithPriority()` 创建带优先级的流。优先级范围可通过 `cudaDeviceGetStreamPriorityRange()` 查询。**数值越小优先级越高**。优先级只是提示，不保证抢占已执行的工作，也不保证特定执行顺序。

2. CUDA Graphs
对于需要重复执行的固定操作序列或 DAG，CUDA Graphs 可通过**流捕获 (stream capture)** 或手动构建图来降低 CPU 开销。流程为：
1. **捕获图**：`cudaStreamBeginCapture()` / `cudaStreamEndCapture()`
2. **实例化图**：`cudaGraphInstantiate()`
3. **执行图**：`cudaGraphLaunch()`

由于执行时所有运行时结构已准备就绪，CPU 开销最小化。

### 2.3.11 本章要点总结

- 异步 API 允许表达任务的并发执行，实际并发度取决于硬件资源和计算能力
- CUDA 异步执行的核心抽象是：**Streams、Events、Callback Functions**
- 同步可在事件级、流级和设备级进行
- Legacy 默认流是阻塞流，会与所有阻塞流隐式同步；非阻塞流和 per-thread 默认流可避免此行为
- 流优先级是提示，不保证抢占或固定顺序
- CUDA 提供了 CUDA Graphs、Batched Memory Transfers 等 API 来降低内核启动和内存传输的开销
