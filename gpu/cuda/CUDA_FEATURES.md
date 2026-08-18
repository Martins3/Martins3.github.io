# CUDA Features Notes
<!-- 96aba063-7cb2-4424-a67d-fd718eb9a024 -->

4.1 Unified Memory
4.2 CUDA Graphs
4.3 Stream-Ordered Memory Allocator
4.4 Cooperative Groups
4.5 Programmatic Dependent Launch and Synchronization
4.6 Green Contexts
4.7 Lazy Loading
4.8 Error Log Management
4.9 Asynchronous Barriers
4.10 Pipelines
4.11 Asynchronous Data Copies
4.12 Work Stealing with Cluster Launch Control
4.13 L2 Cache Control
4.14 Memory Synchronization Domains
4.15 Interprocess Communication
4.16 Virtual Memory Management
4.17 Extended GPU Memory
4.18 CUDA Dynamic Parallelism
4.19 CUDA Interoperability with APIs
4.20 Driver Entry Point Access

## cuda stream
<!-- 7a2df808-4f19-4d25-9823-7009d10dcd10 -->

这里结合 `CUDA Programming Guide 13.2` 里的几节一起看：

- `2.3 Asynchronous Execution`
- `2.3.2 CUDA Streams`
- `2.3.5 CUDA Stream Ordering`
- `2.3.6 Blocking and non-blocking streams and the default stream`
- `2.3.9.2 Introduction to CUDA Graphs with Stream Capture`
- `4.2 CUDA Graphs`

最核心的结论是：

- `CUDA stream` 是软件层面的“工作队列 / 执行队列”抽象
- 它不是硬件单元
- 它和 `Streaming Multiprocessor (SM)` 完全不是一回事

可以把一个 stream 想成一条提交给 GPU 的时间线：

- 往 stream 里 enqueue kernel launch
- 往 stream 里 enqueue `cudaMemcpyAsync`
- 往 stream 里 enqueue event
- GPU 按这个 stream 的顺序处理这些工作

文档原意是：stream 用来表达一串 operation 的顺序关系。它像一个 work queue。

## 1.1 stream 到底在表达什么

同一个 stream 内，操作是 in-order 的。

也就是：

- 先入队的先执行
- 后面的操作不能“插队”
- 如果后一个 kernel 依赖前一个 kernel 的结果，那么把它们放在同一个 stream 里就能自然表达这种依赖

例如：

```cpp
cudaMemcpyAsync(d_in, h_in, n, cudaMemcpyHostToDevice, stream);
kernel<<<grid, block, 0, stream>>>(d_in, d_out);
cudaMemcpyAsync(h_out, d_out, n, cudaMemcpyDeviceToHost, stream);
```

这段代码的含义是：

1. 先把 H2D copy 放进 `stream`
2. 再把 kernel 放进 `stream`
3. 再把 D2H copy 放进 `stream`
4. GPU 在这条 stream 上按顺序处理

所以 stream 的第一层作用不是“并发”，而是“定义顺序”。

## 1.2 为什么多个 stream 会和并发联系起来

因为不同 stream 之间默认没有同样强的顺序约束。

例如：

```cpp
kernelA<<<grid, block, 0, stream1>>>(...);
kernelB<<<grid, block, 0, stream2>>>(...);
```

这表示：

- `kernelA` 在 `stream1`
- `kernelB` 在 `stream2`
- 这两项工作之间没有被你显式排成前后

因此 runtime 可以在资源允许时并发执行它们。

但是一定要注意：

- 多个 stream 只是“给并发提供表达方式”
- 不是“只要两个 stream 就一定并发”

是否真的并发，还取决于：

- GPU 资源是否足够
- 是否有 event 依赖
- 是否被默认流或其他隐式同步打断
- copy 和 compute 是否具备 overlap 条件

## 1.3 host 看 stream 时，为什么说它是异步的

文档在 `Asynchronous Execution` 里反复强调：

- 很多 CUDA API 对 host thread 来说是异步的
- 调用返回，不代表 GPU 上对应工作已经完成

例如 kernel launch：

```cpp
kernel<<<grid, block, 0, stream>>>(...);
```

通常 host 线程会立刻继续往下走，而 GPU 在后台执行这个 kernel。

如果你要确认 stream 里的工作做完了，需要显式同步，例如：

```cpp
cudaStreamSynchronize(stream);
```

或者非阻塞地查询：

```cpp
cudaStreamQuery(stream);
```

这也是为什么 stream 常被理解成“异步任务队列”。

## 1.4 default stream 是什么

如果 launch 或 API 调用里没有显式指定 stream，那么工作会进入 default stream。

文档里专门提醒：default stream 不是“一个普通但默认选中的 stream”这么简单，它有特殊同步语义。

最常见的 legacy 语义里：

- default stream 也叫 `NULL stream`
- 也叫 stream id `0`
- 它会和其他 blocking streams 发生额外同步

也就是说，如果你写：

```cpp
kernel1<<<grid, block, 0, stream1>>>(...);
kernel2<<<grid, block>>>(...);  // default stream
kernel3<<<grid, block, 0, stream2>>>(...);
```

那就不能简单理解成三个 kernel 完全独立。默认流可能在中间引入隐式同步，破坏你原本期待的并发。

这也是为什么工程上经常建议：

- 少依赖 legacy default stream
- 显式创建自己的 streams
- 需要时用 `cudaStreamCreateWithFlags(..., cudaStreamNonBlocking)`

## 1.5 blocking stream 和 non-blocking stream

文档说得很明确：

- 这里的 blocking / non-blocking，不是说“host 会不会 block”
- 而是说“这个 stream 和 default stream 之间怎么同步”

默认 `cudaStreamCreate()` 创建的是 blocking stream。

如果想创建 non-blocking stream：

```cpp
cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking);
```

non-blocking stream 的关键价值是：

- 避免 legacy default stream 带来的额外同步
- 提高不同 streams 之间并发的可能性

## 1.6 stream 之间如何表达依赖

一个 stream 内已经天然有顺序。

两个不同 stream 之间如果也要表达“你先做完，我再做”，通常靠 event：

```cpp
cudaEventRecord(ev, stream1);
cudaStreamWaitEvent(stream2, ev);
```

这表示：

- `stream1` 先记录一个 event
- `stream2` 后续工作要等这个 event 完成

这比把所有工作粗暴塞进同一个 stream 更灵活，因为你只建立必要依赖，而不是把整个 pipeline 完全串行化。

## 1.7 为什么用了 `cudaMemcpyAsync` 还是不一定真的异步

文档在 page-locked host memory 那一节明确说了：

- 涉及 CPU memory 的异步 copy，通常要求 host buffer 是 page-locked / pinned memory

所以如果只是把 `cudaMemcpy` 改成 `cudaMemcpyAsync`，但 host buffer 还是普通 `malloc` 出来的内存，那么：

- API 名字虽然叫 async
- 实际 overlap 效果可能没有你期待的那么好

stream 只是描述“工作排在哪条队列里”，它不自动保证底层一定能重叠。

## 1.8 stream 和 CUDA Graph 的关系

这个问题很容易混淆，但文档其实讲得很清楚。

先记一句话：

- `stream` 是逐条提交工作的模型
- `CUDA Graph` 是先定义整张依赖图，再重复执行的模型

文档在 `2.3.9.2 Introduction to CUDA Graphs with Stream Capture` 里明确说：

- 多个 streams 加上 `cudaStreamWaitEvent()` 这种跨 stream 依赖，本身就能表达一个完整 DAG
- 如果这套 sequence / DAG 需要重复执行很多次，CUDA Graph 更合适

也就是说，Graph 不是和 stream 对立，而是建立在 stream 之上的更高层提交模型。

### 1.8.1 stream 像什么，graph 像什么

可以这样理解：

- stream: 一边运行，一边把 kernel / memcpy / event 一条条塞进去
- graph: 先把整套 workflow 画成图，实例化一次，后面重复 launch

文档里强调 Graph 的两个主要收益：

- 降低 CPU launch overhead
- 因为 CUDA 能看到整张图，所以有机会做比 stream 模式更激进的优化

### 1.8.2 stream capture 是两者之间的桥

从现有 stream 代码迁移到 Graph，最自然的方法就是 stream capture：

```cpp
cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal);

kernelA<<<..., stream>>>(...);
cudaMemcpyAsync(..., stream);
kernelB<<<..., stream>>>(...);

cudaStreamEndCapture(stream, &graph);
cudaGraphInstantiate(&graphExec, graph, NULL, NULL, 0);
cudaGraphLaunch(graphExec, stream);
```

这段代码表达的是：

1. 原来你就在用 stream 提交工作
2. capture 期间，这些工作不再直接排队执行
3. 它们被记录成一个 graph
4. graph instantiate 一次以后，可以反复 launch

所以：

- stream capture = 从 stream 提交流程“录制”出 graph

### 1.8.3 graph 最后还是 launch 到 stream 里

这点非常重要。

文档在 `4.2 CUDA Graphs` 里写得很清楚：

- executable graph 仍然是 launched into a stream

也就是说：

- graph 不是完全脱离 stream 单独运行
- graph 的执行仍然通过 stream 发射
- 只是提交单位从“单个 operation”变成了“整张已经实例化好的图”

所以最准确的关系是：

- stream 是基础异步队列抽象
- graph 是把多项工作和依赖关系预先固化好的 DAG 提交模型
- stream capture 是二者的桥
- graph 执行时仍然落在 stream 上

## 1.9 一个更实用的心智模型

可以把 CUDA 里的这几个概念按层次记成：

1. `stream`
   - 表达一条工作时间线
   - 同一条线内按顺序
2. `event`
   - 表达不同时间线之间的依赖
3. `graph`
   - 把多条时间线及其依赖固化成一个可重复执行的 DAG

换句话说：

- stream 负责“排队”
- event 负责“连边”
- graph 负责“把整个图录下来并高效重放”

## 1.10 什么时候优先用 stream，什么时候考虑 graph

优先用 stream 的情况：

- 工作流经常变化
- 只是临时发几个 kernel
- 还在调试和探索阶段

优先考虑 graph 的情况：

- 工作流结构基本固定
- 需要高频重复执行
- kernel 很短，CPU launch overhead 占比高
- 推理、固定 batch pipeline、规则 DAG 场景

因此，Graph 不是 stream 的替代品，而是当“同一套 stream/DAG 结构反复执行”时，对 stream 模型的一次工程化升级。

## 4.1 Unified Memory

Unified Memory 的核心是：CPU 和 GPU 共享同一份 managed 虚拟地址，程序不必自己写显式 `cudaMemcpy` 才能让两边访问同一份数据。

它解决的问题是：

- 降低 CPU/GPU 数据管理复杂度
- 让代码先跑起来，再逐步优化迁移路径
- 让 oversubscription、prefetch、memory advice 之类优化成为可能

你仍然要理解一点：Unified Memory 不是“数据永远不移动”，而是“数据移动由运行时按页管理”。如果页面当前在 CPU，而 kernel 要访问，就可能发生迁移或 page fault。

本目录对应 demo：

- `unified_memory.cu`

在 `sm_61` 上：

- 可以用
- 但性能上要警惕页面迁移成本
- 如果访问模式稳定，通常仍建议配合 `cudaMemPrefetchAsync`

## 4.2 CUDA Graphs

CUDA Graphs 是把一串固定的 GPU 工作流先录制成图，再高效重复提交。图里的节点可以是 kernel、memcpy、event、host callback 等。

它解决的问题是：

- 重复提交小 kernel 时 CPU launch overhead 过高
- 固定 DAG 工作流每次都重新发命令太贵

它特别适合：

- 推理
- 固定批处理流水线
- 重复执行相同拓扑的 HPC 任务

本目录对应 demo：

- `cuda_graph_demo.cu`

直观理解：

- 普通 launch 像“每次都重新下发一遍命令”
- graph replay 像“提前把命令装订好，后面重复按模板提交”

## 4.3 Stream-Ordered Memory Allocator

这是 `cudaMallocAsync` / `cudaFreeAsync` 这套机制。它让内存分配和释放服从 stream 顺序，而不是像传统 `cudaMalloc/cudaFree` 那样更全局、更容易引入同步。

它解决的问题是：

- 高频动态分配造成的同步和碎片化问题
- 多 stream 场景下内存生命周期难以高效管理

直观理解：

- 传统分配更像“全局资源操作”
- stream-ordered allocator 更像“把分配释放插入某条 stream 的时间线”

适合：

- 短生命周期 buffer
- 图像/推理框架中的中间张量池

本目录暂时没有专门 demo。

## 4.4 Cooperative Groups

Cooperative Groups 是对线程协作范围的显式抽象。它不只支持整个 block，还支持 warp、tile、grid 等分组方式。

它解决的问题是：

- 传统 CUDA 同步和协作原语过于依赖 block 粒度
- 程序员想更明确地表达“哪些线程是一组”

典型用途：

- block reduction
- warp-level collectives
- tiled 协作加载和计算

本目录对应 demo：

- `cooperative_groups_reduce.cu`

直观理解：

- `__syncthreads()` 是“整个 block 一起等”
- cooperative groups 允许你说“就这 32 个线程一组”或者“这个 tile 一组”

## 4.5 Programmatic Dependent Launch and Synchronization

这是让一个 launch 或任务依赖另一个任务时，能够以更程序化、低开销的方式表达“先后关系”和“就绪点”的能力。它通常出现在更高级的执行模型、图和依赖提交流程里。

它解决的问题是：

- 多个 kernel 之间有强依赖，但不想退回 CPU 做重同步
- 希望更细粒度表达生产者/消费者关系

直观理解：

- 不是“kernel A 完了，CPU 再去发 kernel B”
- 而是“把依赖关系本身交给 CUDA 执行系统”

这类能力在现代高性能库里很重要，但在教学 demo 里通常不单独出现。

## 4.6 Green Contexts

Green Contexts 可以理解为一种更轻量、更可控的 GPU 执行上下文管理方式，用于改善上下文隔离、资源切分和调度开销。

它解决的问题是：

- 传统 context 过重
- 多租户或复杂运行时想更细粒度地管理 GPU 执行环境

这类特性更偏运行时系统和平台能力，不是日常写 kernel 最常直接接触的东西。

对这个项目的意义：

- 理解成“更现代的 GPU 上下文管理能力”即可
- 一般不在简单 CUDA demo 里直接使用

## 4.7 Lazy Loading

Lazy Loading 指 CUDA 运行时/驱动对模块、kernel、设备代码采用按需加载，而不是程序一启动就把所有东西都装进去。

它解决的问题是：

- 启动慢
- 显存或设备代码装载开销大
- 大程序包含很多 kernel，但一次运行只会用到其中少数

好处：

- 降低初始化延迟
- 降低未使用代码的装载成本

代价：

- 第一次调用某个 kernel 时，可能出现额外延迟

## 4.8 Error Log Management

这是 CUDA 对错误日志、诊断输出、调试信息的管理能力。重点不是普通 `cudaGetLastError()`，而是更系统化地收集和输出设备侧、驱动侧、工具链侧错误信息。

它解决的问题是：

- GPU 错误定位难
- 异步执行导致“报错位置”和“真正出错点”分离

工程上要区分：

- API 错误码
- 异步执行错误
- 工具链/运行时日志

本目录 demo 主要还是用简单错误检查：

- `cudaGetLastError()`
- `cudaDeviceSynchronize()`

## 4.9 Asynchronous Barriers

Asynchronous Barriers 是更现代的同步原语，允许线程在等待某个阶段完成时，把同步和数据搬运/流水线阶段协调得更细。

它解决的问题是：

- `__syncthreads()` 太粗
- staged pipeline 中需要“这一批数据到了就继续下一步”，而不是整个 block 粗暴同步

它通常与以下概念一起出现：

- pipeline
- async copy
- producer/consumer stage

本目录对应理解型 demo：

- `pipeline_teaching_demo.cu`

要注意：

- `sm_61` 没有 Hopper/Ampere 上那种更新的硬件级 async copy 配套能力
- 但概念上可以先学“为什么需要这样的 barrier”

## 4.10 Pipelines

Pipelines 是把计算拆成多个阶段，让“加载下一块数据”和“计算当前块数据”重叠起来。

它解决的问题是：

- global memory 延迟高
- 线程先等数据、再计算，资源利用率差

典型模式：

1. 第 0 阶段加载 tile A
2. 计算 tile A 的同时预取 tile B
3. 再切换到 tile B

这本质上是 GPU 上的流水线化。

本目录对应理解型 demo：

- `pipeline_teaching_demo.cu`

## 4.11 Asynchronous Data Copies

Asynchronous Data Copies 指数据搬运与线程执行解耦，让数据从更慢层级搬到更快层级时，不一定要像传统 `load -> wait -> use` 那样阻塞。

常见语境是：

- global -> shared 的 async copy
- DMA 风格的异步传输
- 搭配 pipeline 做双缓冲

它解决的问题是：

- 数据搬运延迟暴露在关键路径上

直观理解：

- 传统方式：线程自己读，读完才能继续
- async copy：先发起搬运，稍后在需要时同步

在 `sm_61` 上：

- 没有新架构上的代表性硬件 async copy 指令能力
- 但可以通过软件结构理解它的设计目标

## 4.12 Work Stealing with Cluster Launch Control

这是更高级的线程块集群调度能力。它允许 block cluster 在执行中更灵活地分配未完成工作，减少某些 block 提前干完、某些 block 拖尾造成的不均衡。

它解决的问题是：

- 不规则任务负载不均
- 静态分块导致 tail effect 明显

直观理解：

- 原来像“任务发下去就定死”
- 现在更像“做完的执行单元可以偷剩余工作”

这是较新的架构能力，和 `sm_61` 无关，当前卡不需要把它当作可直接使用的能力。

## 4.13 L2 Cache Control

L2 Cache Control 是指 CUDA/驱动提供的一些接口或策略，让程序影响数据在 L2 中的缓存行为，例如持久化、访问策略窗口等。

它解决的问题是：

- 某些热点数据很适合尽量留在 L2
- 另一些流式数据不值得污染缓存

适合：

- 访问模式可预测的高性能内核
- 重复读同一批小工作集

注意：

- 这是性能调优特性，不是正确性特性
- 不合理使用也可能没收益，甚至变差

## 4.14 Memory Synchronization Domains

Memory Synchronization Domains 是对“哪些内存操作需要彼此可见、在哪个范围内同步”的更明确建模。

它解决的问题是：

- GPU 内存一致性不是“所有写立刻全局可见”
- 不同代理之间的同步需求不同：线程、block、device、system

你可以把它理解为：

- CUDA 不只是有“同步没同步”
- 还要问“在什么域内同步”

这和以下概念相关：

- memory fence
- scope
- system/device/block 可见性

## 4.15 Interprocess Communication

CUDA IPC 允许不同进程共享 GPU 资源，比如 device memory、event 等。

它解决的问题是：

- 多进程服务想共享同一块 GPU buffer
- 一个进程生产数据，另一个进程消费，不想绕回 CPU 拷贝

典型场景：

- 多进程推理服务
- 多进程视频处理流水线
- 父子进程共享 GPU 资源

注意：

- IPC 是进程间共享，不是主机间共享
- 往往要配合句柄导出/导入、生命周期管理和同步

## 4.16 Virtual Memory Management

CUDA Virtual Memory Management 是更底层、更灵活的显存虚拟内存接口，允许你像操作现代操作系统虚拟内存那样，分别做保留地址、创建物理分配、映射、改权限等动作。

它解决的问题是：

- 传统 `cudaMalloc` 太“一步到位”，不够灵活
- 大型内存池、稀疏映射、按需扩展、跨设备映射很难做

典型能力：

- reserve virtual address range
- create physical allocation
- map/unmap
- set access permissions

适合：

- 深度学习框架内存池
- 稀疏数据结构
- 大规模运行时系统

## 4.17 Extended GPU Memory

Extended GPU Memory 可以理解为超出传统“单卡本地显存”视角的更大范围 GPU 内存使用能力，常和统一寻址、系统内存扩展、分页迁移或更大内存空间管理能力相关。

它解决的问题是：

- 本地显存有限
- 应用需要访问比板载显存更大的数据集

常见理解方式：

- 不是“GPU  suddenly 拥有无限快显存”
- 而是 CUDA 允许更大范围的地址空间和后备存储参与 GPU 访问

代价通常是：

- 延迟更高
- 带宽更低
- 更依赖迁移策略

## 4.18 CUDA Dynamic Parallelism

Dynamic Parallelism 指 GPU 上运行的 kernel 自己再启动新的 kernel，而不需要 CPU 介入。

它解决的问题是：

- 递归或自适应细分任务不适合每次回到 CPU 再 launch
- GPU 端发现新任务后，希望就地扩展并行工作

典型场景：

- 自适应网格
- 树遍历
- 不规则细分计算

优点：

- 避免部分 CPU 往返控制

缺点：

- launch 开销仍然不低
- 编程和调试复杂
- 不是所有场景都比 CPU 发起更划算

## 4.19 CUDA Interoperability with APIs

这指 CUDA 和其他 API 互操作，比如：

- OpenGL
- Direct3D
- Vulkan
- EGL
- 外部同步对象或外部内存

它解决的问题是：

- 图形 API 生成的数据想直接交给 CUDA 计算
- CUDA 计算结果想直接给图形/视频/显示 API 使用
- 避免多余拷贝

典型场景：

- 图形渲染后处理
- 视频编解码管线
- 计算和显示共享一块 GPU 资源

## 4.20 Driver Entry Point Access

这是让程序更灵活地访问 CUDA Driver API 的入口点，常见于动态加载、版本兼容、运行时查询函数指针等场景。

它解决的问题是：

- 程序不想在编译期强绑定某个固定 driver symbol
- 框架想按运行环境动态解析可用能力

直观理解：

- 像普通系统库里的 `dlopen + dlsym`
- 但语义是“更可控地访问 CUDA driver entry points”

这类能力更常见于：

- 运行时框架
- 兼容层
- 插件系统

## 怎么把这些特性分组理解

如果一次看 20 个名词太散，可以先按下面分组记忆：

- 编程模型类
  - Unified Memory
  - Cooperative Groups
  - Dynamic Parallelism
- 执行提交与调度类
  - CUDA Graphs
  - Programmatic Dependent Launch and Synchronization
  - Work Stealing with Cluster Launch Control
  - Green Contexts
- 同步与流水线类
  - Asynchronous Barriers
  - Pipelines
  - Asynchronous Data Copies
  - Memory Synchronization Domains
- 内存管理类
  - Stream-Ordered Memory Allocator
  - L2 Cache Control
  - Virtual Memory Management
  - Extended GPU Memory
  - Interprocess Communication
- 运行时与系统集成类
  - Lazy Loading
  - Error Log Management
  - CUDA Interoperability with APIs
  - Driver Entry Point Access

## 和本目录 demo 的对应关系

- 已有直接例子
  - Unified Memory -> `unified_memory.cu`
  - CUDA Graphs -> `cuda_graph_demo.cu`
  - Cooperative Groups -> `cooperative_groups_reduce.cu`
  - Asynchronous Barriers / Pipelines / Asynchronous Data Copies -> `pipeline_teaching_demo.cu` 用“教学解释”方式覆盖概念
- 当前没有直接例子
  - Stream-Ordered Memory Allocator
  - Programmatic Dependent Launch and Synchronization
  - Green Contexts
  - Lazy Loading
  - Error Log Management
  - Work Stealing with Cluster Launch Control
  - L2 Cache Control
  - Memory Synchronization Domains
  - Interprocess Communication
  - Virtual Memory Management
  - Extended GPU Memory
  - CUDA Dynamic Parallelism
  - CUDA Interoperability with APIs
  - Driver Entry Point Access

## 对 GTX 1060 3GB 的现实建议

对当前 `Pascal / sm_61`，建议优先把下面几类先吃透：

1. block / grid / warp 的基本执行模型
2. global memory / shared memory / registers 的关系
3. streams 与 overlap
4. Unified Memory 的语义和性能代价
5. CUDA Graphs 的“减少提交开销”思想
6. cooperative groups 的基本协作方式

至于下面这些，更适合当“知道它们解决什么问题”：

- Green Contexts
- Cluster Launch Control
- Virtual Memory Management
- Driver Entry Point Access

因为这些更偏现代运行时系统、框架设计或新架构特性，不是你现在这张卡上最值得先深挖的第一批内容。


## 3.5. A Tour of CUDA Features

官方入口：

- https://docs.nvidia.com/cuda/archive/13.1.0/cuda-programming-guide/part3.html

### 这一节在讲什么

`3.5` 更像导航页，而不是某一个 feature 的完整教程。

它的作用是：

- 把 `Part 4` 里那些 feature 放回“什么时候会需要它”的语境

如果不先有这层导航，直接冲进 `4.1` 到 `4.20`，很容易变成：

- 名词都看过
- 但不知道什么时候该用

所以这一节真正要解决的问题其实很现实：

- 当你面对一长串高级 feature 时，怎么建立优先级
- 遇到真实工程问题时，怎么从“问题类型”反推“该去看哪一节”

这也是 `3.5` 和前面几节的角色区别：

- `3.1` 到 `3.4` 更像是在扩展能力地图
- `3.5` 则是在给你一张导航图和索引图

它的重要性在于：

- `Part 4` 不是按最平滑的学习路径写的
- 而是按 feature 分类展开的

所以如果没有 `3.5` 这层“什么时候该看什么”的判断，后面阅读会很容易碎掉：

- 知道有 `CUDA Graphs`
- 知道有 `Cooperative Groups`
- 知道有 `Dynamic Parallelism`
- 但不知道它们分别是为哪类问题准备的

因此，这节读完后你应该获得的能力不是某个 feature 的细节，而是：

- 遇到 submission overhead，会想到 graph
- 遇到协作表达不足，会想到 cooperative groups
- 遇到数据移动管理复杂，会想到 unified memory 或 async copy 相关 feature
- 遇到系统级集成需求，会想到 driver/interprocess/interoperability 那一组内容

也就是说，`3.5` 的价值不是增加知识点，而是降低你后续学习 `Part 4` 时的搜索成本和决策成本。

### 推荐的功能分组

本地学习时，我更推荐把 `Part 4` 的 feature 先按问题域分成四组。

#### 1. 内存与数据移动

对应 feature：

- `4.1 Unified Memory`
- `4.3 Stream-Ordered Memory Allocator`
- `4.11 Asynchronous Data Copies`
- `4.13 L2 Cache Control`
- `4.16 Virtual Memory Management`
- `4.17 Extended GPU Memory`

本地先行锚点：

- [unified_memory.cu](/home/martins3/data/vn/gpu/cuda/basic/unified_memory.cu)
- [memory_bandwidth.cu](/home/martins3/data/vn/gpu/cuda/basic/memory_bandwidth.cu)

#### 2. 提交与同步

对应 feature：

- `4.2 CUDA Graphs`
- `4.5 Programmatic Dependent Launch and Synchronization`
- `4.8 Error Log Management`
- `4.9 Asynchronous Barriers`
- `4.10 Pipelines`

本地先行锚点：

- [vector_add.cu](/home/martins3/data/vn/gpu/cuda/basic/vector_add.cu)
- [memory_bandwidth.cu](/home/martins3/data/vn/gpu/cuda/basic/memory_bandwidth.cu)
- [stream_overlap.cu](/home/martins3/data/vn/gpu/cuda/basic/stream_overlap.cu)
- [cuda_graph_demo.cu](/home/martins3/data/vn/gpu/cuda/basic/cuda_graph_demo.cu)
- [pipeline_teaching_demo.cu](/home/martins3/data/vn/gpu/cuda/basic/pipeline_teaching_demo.cu)

#### 3. 高级 kernel / 协作

对应 feature：

- `4.4 Cooperative Groups`
- `4.12 Work Stealing with Cluster Launch Control`
- `4.18 CUDA Dynamic Parallelism`

本地先行锚点：

- [memory_bandwidth.cu](/home/martins3/data/vn/gpu/cuda/basic/memory_bandwidth.cu)
- [cooperative_groups_reduce.cu](/home/martins3/data/vn/gpu/cuda/basic/cooperative_groups_reduce.cu)
- [pipeline_teaching_demo.cu](/home/martins3/data/vn/gpu/cuda/basic/pipeline_teaching_demo.cu)
- [cutlass.md](/home/martins3/data/vn/gpu/cuda/basic/cutlass.md)

#### 4. 系统级能力与集成

对应 feature：

- `4.6 Green Contexts`
- `4.7 Lazy Loading`
- `4.14 Memory Synchronization Domains`
- `4.15 Interprocess Communication`
- `4.19 CUDA Interoperability with APIs`
- `4.20 Driver Entry Point Access`


1. `4.1 Unified Memory`
2. `4.2 CUDA Graphs`
3. `4.4 Cooperative Groups`
4. `4.9 Asynchronous Barriers`
5. `4.10 Pipelines`
6. `4.11 Asynchronous Data Copies`
7. `4.18 CUDA Dynamic Parallelism`
