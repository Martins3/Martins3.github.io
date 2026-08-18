## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/device-callable-apis.html>


本章节（Device-Callable APIs and Intrinsics）是 CUDA Programming Guide Release 13.2 的技术附录之一，系统整理了可以在 CUDA kernel 及设备代码中直接调用的 API 与内建函数。内容跨度很大，从编译期的快速数学函数映射，到低阶的内存屏障（Memory Barrier）与流水线（Pipeline）原语，再到高阶的 Cooperative Groups 编程模型，最后还包括完整的 CUDA Device Runtime 参考。理解这些内容对于编写需要跨线程块同步、异步数据搬运、动态并行（Dynamic Parallelism）或设备端图（Device Graph）启动的现代 CUDA 程序至关重要。

## 背景与要解决的问题

在早期的 CUDA 编程模型中，同步点几乎只能出现在 kernel 边界（即 kernel 结束时的隐式同步）。如果算法需要多阶段流水线处理，开发者通常只能把每个阶段拆成独立的 kernel，由主机端按顺序发射。这种模式带来了两个问题：一是频繁的 kernel 启动开销，二是阶段之间的数据在全局内存中来回传递，缺乏数据局部性。与此同时，Warp 内部的线程协作长期依赖底层的 PTX 指令（如 `shfl`、`vote`），缺乏类型安全且容易出错。

Device-Callable APIs 的引入旨在解决以下几个核心痛点：
- **设备端同步**：允许线程块内部、集群内部甚至整个 grid 内部进行屏障同步，而不仅限于 kernel 结束。
- **异步内存拷贝**：通过硬件加速的 `memcpy_async` 把数据从全局内存异步搬到共享内存，让计算与数据传输重叠。
- **结构化协作组**：用 Cooperative Groups 把 Warp、线程块、集群、Grid 抽象成统一的对象，提供类型安全的 `shfl`、`reduce`、`scan` 等操作。
- **动态并行与设备端运行时**：让 kernel 能在设备端启动子 kernel、管理流和事件，而不必返回主机。

## 核心概念与术语

在深入 API 之前，有必要先厘清本章反复出现的几个概念：

- **Collective Operation（集合操作）**：指一个组（group）内的所有线程都必须参与的 API。如果有线程未参与，行为是未定义的。这是 Cooperative Groups 和 Barrier 原语的设计基础。
- **Phase（阶段）**：Barrier 的一种状态抽象。每次所有线程到达屏障并完成等待后，屏障就进入下一个 phase。部分 API（如 `__mbarrier_test_wait_parity`）用布尔奇偶性来区分相邻 phase。
- **Token（令牌）**：`barrier_arrive()` 返回的到达凭证。只有持有正确 token 的线程才能在对应的 `barrier_wait()` 中通过。token 是一次性的，不能复用。
- **Coalesced Threads（合并线程）**：在 SIMT 执行模型中，当 Warp 内发生分支发散时，当前正在同一条执行路径上的活跃线程称为 coalesced。`coalesced_group` 允许把这些活跃线程临时组织成一个子组。
- **Tail Launch / Fire-and-Forget**：设备端运行时提供的两种特殊流。Tail Launch 保证在当前 grid 及其所有后代完成后才启动；Fire-and-Forget 则完全没有依赖关系，立即调度。
- **Scope（作用域）**：在原子操作和 barrier 中频繁出现，指同步或原子性需要覆盖的线程范围（例如 `thread_scope_block`、`thread_scope_thread`）。

## API / 机制详解

### 1. 受 `--use_fast_math` 影响的函数

虽然这只是个速查表，但它直接反映了精度与性能的权衡。当编译器启用 `--use_fast_math` 时，常规的数学函数（如 `sinf`、`logf`、`powf`）会被映射到对应的 intrinsic（如 `__sinf`、`__logf`、`__powf`）。这些 intrinsic 通常使用硬件近似指令，精度降低但吞吐大幅提升。对于需要 IEEE 754 严格语义的场景，应避免此标志，或直接调用非 intrinsic 版本。

### 2. Memory Barrier Primitives Interface

该接口位于 `<cuda_awbarrier_primitives.h>`，提供对 `cuda::barrier` 的 C 风格底层访问。

**数据类型**
- `__mbarrier_t`：屏障对象本身，必须存放在 `__shared__` 内存中。
- `__mbarrier_token_t`：到达令牌，用于后续等待。

**关键 API 及语义**
- `__mbarrier_maximum_count()`：返回屏障支持的最大到达计数值。
- `__mbarrier_init(__mbarrier_t* bar, uint32_t expected_count)`：初始化屏障，设定当前 phase 和下一 phase 的预期到达数。必须在任何 arrive 之前完成，且 `bar` 必须指向共享内存。
- `__mbarrier_inval(__mbarrier_t* bar)`：使屏障失效。如果你打算把原先存放屏障的共享内存挪作他用，必须先调用此函数，否则硬件状态可能与新用途冲突。
- `__mbarrier_arrive(__mbarrier_t* bar)`：原子地将当前 phase 的 pending count 减一，并返回一个 token。该 token 关联的是“减一之前”的屏障状态。调用前必须已完成初始化，且 pending count 不能为零。
- `__mbarrier_arrive_and_drop(__mbarrier_t* bar)`：与 `arrive` 类似，但额外将下一 phase 的 expected count 也减一。适用于某些线程永久退出后续同步轮次的情况。
- `__mbarrier_test_wait(__mbarrier_t* bar, __mbarrier_token_t token)`：非阻塞地检查 token 是否对应上一 phase。若是则返回 true，否则返回 false。不会挂起线程。
- `__mbarrier_try_wait(__mbarrier_t* bar, __mbarrier_token_t token, uint32_t max_sleep_nanosec)`：与 `test_wait` 不同，它允许线程在 token 尚未就绪时挂起，直到对应 phase 完成或超时。`max_sleep_nanosec` 可以覆盖系统默认的超时上限。

### 3. Pipeline Primitives Interface

该接口在 `<cuda_pipeline.h>`（或 C++11 之前的 `<cuda_pipeline_primitives.h>`）中声明，用于追踪从全局内存到共享内存的异步拷贝。

**`__pipeline_memcpy_async`**
- 功能：请求硬件执行一次从 `src_global` 到 `dst_shared` 的异步拷贝，支持 4、8、16 字节对齐的拷贝尺寸。
- `zfill` 参数：若提供，最后 `zfill` 字节会被零填充而非从源地址读取。
- 关键限制：`size_and_align` 必须同时是 `dst_shared` 和 `src_global` 的对齐值；在提交拷贝与等待完成之间，任何线程对 `dst_shared` 的加载、存储或原子操作，或对 `src_global` 的修改，都属于数据竞争。

**`__pipeline_commit()`**
- 将此前提交的所有 `memcpy_async` 请求打包为当前 batch。可以把 commit 理解为“封账”。

**`__pipeline_wait_prior(size_t N)`**
- 假设当前线程已调用 `__pipeline_commit()` 的批次索引序列为 `{0, 1, 2, ..., L}`，则此调用会等待至少到 `L-N` 批次的所有异步拷贝完成。这是实现流水线双缓冲（double buffering）的核心：在处理第 N 批数据时，提前提交第 N+1 批。

**`__pipeline_arrive_on(__mbarrier_t* bar)`**
- 当所有此前序列化的 `memcpy_async` 完成后，自动让屏障的到达计数减一。换句话说，它在屏障上产生一个“到达”事件，但仅当异步拷贝真正结束时才生效。使用者必须确保递增不会超出 `__mbarrier_maximum_count()`。

### 4. Cooperative Groups API

Cooperative Groups 提供了一套 C++ 风格的组抽象，核心头文件是 `<cooperative_groups.h>`，子功能需要额外包含 `<cooperative_groups/...>`。

#### 4.1 `thread_block`
由 `this_thread_block()` 构造，对应一个线程块。除了熟悉的 `sync()`（等价于 `__syncthreads()`）之外，它还提供了**分阶段屏障**接口：
- `barrier_arrive()`：到达线程块屏障，返回一个 `arrival_token`。
- `barrier_wait(arrival_token&&)`：等待屏障完成。把到达与等待拆成两步，允许线程在两者之间执行独立工作，从而隐藏同步延迟。

#### 4.2 `cluster_group`（CC 9.0+）
由 `this_cluster()` 构造，代表一个簇（cluster）内的所有线程。如果启动时未使用 cluster，则默认视为 1x1x1 的 cluster。关键特性包括：
- `map_shared_rank(T* addr, int rank)`：获取 cluster 内另一个线程块的共享内存变量地址，实现分布式共享内存（distributed shared memory）访问。
- `query_shared_rank(const void* addr)`：查询某个共享地址属于 cluster 内的哪个 block rank。
- `sync()` 与 `barrier_arrive/barrier_wait`：与 `thread_block` 语义相同，但覆盖整个 cluster。

#### 4.3 `grid_group`
由 `this_grid()` 构造，代表整个 grid。注意，`sync()` 并非随时可用，必须通过 **cooperative launch**（`cudaLaunchCooperativeKernel`）启动 kernel，并且设备属性 `cudaDevAttrCooperativeLaunch` 必须为 1。此外还提供了：
- `is_valid()`：在运行时检查该 grid_group 是否可以同步。
- 各种 rank/index 查询：`thread_rank()`、`block_rank()`、`cluster_rank()`、`dim_blocks()` 等。

#### 4.4 `thread_block_tile`
模板类 `thread_block_tile<Size, ParentT>` 把一个父组切分成固定大小的一维瓦片。`Size` 必须是 2 的幂且不超过 1024。
- **Warp 级原语**：提供 `shfl`、`shfl_up`、`shfl_down`、`shfl_xor`、`any`、`all`、`ballot`、`match_any`、`match_all`。对于大小超过 32 的 tile，`shfl_up/down/xor` 与 `ballot`/`match_*` 不可用。
- **类型泛化**：在 C++11 及以后，`shfl` 系列可以接受任何 trivially copyable 且尺寸不超过限制的类型（tile <=32 时 `sizeof(T) <= 32`；更大 tile 时 `sizeof(T) <= 8`）。
- **block_tile_memory**：在 CC 7.5 或更低的硬件上，如果 tile 大小超过 32，需要在共享内存或全局内存中预留 `block_tile_memory<MaxBlockSize>`，并通过 `this_thread_block(shared)` 传入。CC 8.0+ 则不需要额外内存。

#### 4.5 `coalesced_group`
由 `coalesced_threads()` 构造，捕获调用时刻 Warp 内所有活跃（未发散）的线程。它是机会性的（opportunistic）：不保证包含哪些具体线程，也不保证这些线程在后续会一直活跃。但它支持 Warp 级的 `shfl`、`any`、`all`、`ballot`、`match_any`、`match_all`，非常适合在分支内部对活跃线程做快速规约或投票。

#### 4.6 `memcpy_async`（Cooperative Groups 版本）
位于 `<cooperative_groups/memcpy_async.h>`，是组范围的集体异步拷贝。与底层 Pipeline 原语相比，它更强调**组内所有线程协作完成一次大块拷贝**。为了实现硬件加速的异步传输：
- 源必须是全局内存，目的必须是共享内存。
- 两者都需满足 16 字节（或 8/4 字节）对齐。
- 真正的异步性仅在 CC 8.0+ 上实现；更早的架构会回退到同步拷贝。
- 被异步拷贝的共享内存数据，只有在调用 `cg::wait()` 或 `cg::wait_prior<NumStages>()` 后才能安全读取。

#### 4.7 `wait` 与 `wait_prior`
- `wait(TyGroup& group)`：阻塞直到该组此前所有的 `memcpy_async` 完成。
- `wait_prior<NumStages>(TyGroup& group)`：允许最近 `NumStages` 个请求仍在进行中，只等待更早的请求。这是重叠计算与传输的关键。

#### 4.8 `tiled_partition`、`labeled_partition`、`binary_partition`
- `tiled_partition`：将父组按固定大小切分为一维瓦片。模板版本要求父组大小被 `Size` 整除。
- `labeled_partition`：根据线程提供的整型 label 进行分组，相同 label 的线程被分到一个 `coalesced_group`。要求 CC 7.0+。
- `binary_partition`：`labeled_partition` 的特化，label 只能是布尔值，分成两组。

#### 4.9 `reduce`（`<cooperative_groups/reduce.h>`）
`reduce(group, val, op)` 对组内每个线程提供的 `val` 做归约。在 CC 8.0+ 上，如果类型是 4 字节整型且操作是 `cg::plus`、`less`、`greater`、`bit_and`、`bit_xor`、`bit_or`，会调用硬件加速指令（如 `__reduce_add_sync`）；否则使用 Warp shuffle 的软件回退。重要的是：**lambda 和自定义函数对象无法被编译器静态分派到硬件加速路径**，即使语义相同，也会走 shuffle 回退。

此外还提供异步归约：
- `reduce_update_async`：用归约结果原子更新一个 `cuda::atomic` 或 `cuda::atomic_ref`。
- `reduce_store_async`：弱序地把结果写到指定指针。

#### 4.10 `inclusive_scan` 与 `exclusive_scan`（`<cooperative_groups/scan.h>`）
- `inclusive_scan`：包含当前线程数据的累积结果。
- `exclusive_scan`：仅累加线程排名更低的数据，当前线程数据不包含在内。
- 支持同样的类型约束与函数对象（`plus`、`less`、`greater`、`bit_and`、`bit_xor`、`bit_or`）。
- `*_scan_update` 变体额外接受原子对象，在计算 scan 的同时把组内总和原子加到该对象上，并把旧值纳入每个线程的 scan 结果。典型用途是动态缓冲区分配（每个线程申请一段空间，scan 给出偏移）。

#### 4.11 `sync.h`：屏障拆分与 Grid 同步
`barrier_arrive` 与 `barrier_wait` 的分体式设计允许线程先“报到”，然后做一点独立的局部计算，最后再“等待”。这能把屏障延迟与其他有用工作重叠。对于 Grid 级别的同步，必须在主机侧使用 `cudaLaunchCooperativeKernel` 或驱动等效 API 启动，并且在启动前通过 `cudaOccupancyMaxActiveBlocksPerMultiprocessor` 计算每个 SM 能同时驻留多少个线程块，以确保所有启动的块能够真正同时驻留在 GPU 上（co-residency）。

### 5. CUDA Device Runtime

设备运行时让 kernel 可以像主机端一样调用 CUDA Runtime API，最常用于动态并行和设备图启动。

#### 5.1 内存管理差异
- `cudaMalloc` / `cudaFree` 在设备端实际上映射到设备端的 `malloc` / `free`，受限于 `cudaLimitMallocHeapSize`，而不是整个空闲显存。
- **主机与设备的内存池互不兼容**：主机 `cudaMalloc` 分配的指针不能从设备端 `cudaFree` 释放，反之亦然。
- `__device__` 与 `__constant__` 变量在所有 kernel 中可见，无论由主机还是设备启动。
- 设备运行时**不支持**创建/销毁纹理或表面对象，也不支持旧版模块级纹理。
- `__constant__` 数据在设备端是只读的；设备运行时没有 `cudaMemcpyToSymbol` 等价物，因为设备端可以直接用 `&` 取符号地址。

#### 5.2 设备端流
- 所有设备流必须通过 `cudaStreamCreateWithFlags(..., cudaStreamNonBlocking)` 创建。`cudaStreamCreate` 在设备端不可用。
- **隐式 NULL 流**：设备端每个线程块共享一个 NULL 流，但它不具备主机端 NULL 流跨流屏障的语义；它不会与任何其他流产生隐式依赖。
- `cudaStreamFireAndForget`：发射后立即调度，不依赖此前工作，也不能记录/等待事件。在功能上等价于“每发射一次新建一个流”，但开销更小。
- `cudaStreamTailLaunch`：在当前 grid 及其所有非尾流后代全部完成后，才启动下一个 grid。在多数场景下可以替代 `cudaDeviceSynchronize()`。

#### 5.3 启动机制与错误处理
- 设备端 `<<< >>>` 语法被编译器前端翻译成 `cudaGetParameterBuffer` 和 `cudaLaunchDevice`。
- 错误码是**每线程状态**（per-thread），不是每线程块。每个线程需要自行调用 `cudaGetLastError()` 检查最近的错误。
- 子 grid 中的异常（如非法地址访问）会向上冒泡到主机。

#### 5.4 支持的设备端 API 子集
表格列出了大量可在设备端使用的 Runtime API，包括 `cudaDeviceGetLimit`、`cudaStreamCreateWithFlags`、`cudaEventCreateWithFlags`、`cudaMemcpyAsync`（仅限设备到设备）、`cudaMalloc`、`cudaFree`、`cudaOccupancyMaxActiveBlocksPerMultiprocessor` 等。限制包括：memcpy/memset 只允许异步版本、只允许设备到设备、不允许传入局部或共享内存指针。

## 典型工作流程 / 调用顺序

### 屏障原语的工作流
1. 在共享内存中声明 `__mbarrier_t`。
2. 一个线程（通常是 thread 0）调用 `__mbarrier_init(&bar, expected_count)`。
3. 所有线程执行 `__mbarrier_arrive(&bar)` 得到 token。
4. 需要等待的线程调用 `__mbarrier_test_wait(&bar, token)` 或 `__mbarrier_try_wait(&bar, token, timeout)`。
5. 若共享内存需要回收，先调用 `__mbarrier_inval(&bar)`。

### Pipeline 双缓冲工作流
1. 阶段 0：`__pipeline_memcpy_async(dst[0], src, 16)`；`__pipeline_commit()`。
2. 阶段 1：`__pipeline_memcpy_async(dst[1], src+16, 16)`；`__pipeline_commit()`；同时调用 `__pipeline_wait_prior(1)` 等待阶段 0。
3. 处理 `dst[0]` 的数据。
4. 循环往复，用 `stage ^= 1` 切换缓冲区。

### Cooperative Groups Grid 同步工作流
1. 主机侧查询 `cudaDevAttrCooperativeLaunch`。
2. 用 `cudaOccupancyMaxActiveBlocksPerMultiprocessor` 计算每个 SM 的最大活跃块数。
3. 启动网格：`gridDim = numMultiProcessors * numBlocksPerSm`。
4. Kernel 内：`grid_group grid = this_grid(); grid.sync();`
5. 所有线程块到达 `grid.sync()` 后，才能继续执行后续代码。

### 设备端尾流（Tail Launch）工作流
1. 父 kernel 完成自身计算后，将子 kernel 发射到 `cudaStreamTailLaunch`。
2. 父 kernel 继续执行，甚至可以再发射其他工作到普通流或 Fire-and-Forget 流。
3. 只有当父 grid 及其所有普通流/Fire-and-Forget 后代全部完成后，Tail Launch 的子 grid 才开始执行。

## 关键限制、边界条件与兼容性

- **Compute Capability 差异**：
  - `cluster_group` 需要 CC 9.0+；在更早的硬件上不存在。
  - `grid_group::sync` 需要 CC 6.0+ 且驱动/平台支持 cooperative launch（Linux 无 MPS，或 CC 7.0+ 有 MPS，或最新 Windows）。
  - `memcpy_async` 的完整硬件异步需要 CC 8.0+；CC 5.0~7.x 可编译运行，但可能退化为同步拷贝。
  - `reduce` 的硬件加速需要 CC 8.0+，且仅限 4 字节整型的特定操作。
  - `thread_block_tile` 大小超过 32 时，在 CC 7.5 及以下需要显式分配 `block_tile_memory`。
  - `labeled_partition` 需要 CC 7.0+。

- **集体操作刚性要求**：所有被命名在组中的线程都必须调用同一个集体 API。即使有线程因为分支发散临时不活跃，一旦它们重新汇聚，也必须参与，否则行为未定义。

- **内存指针合法性**：设备端异步拷贝（`cudaMemcpyAsync`）不允许传入局部（local）或共享（shared）内存指针；只允许设备到设备。

- **Barrier 的共享内存约束**：`__mbarrier_t` 必须位于共享内存；`__mbarrier_inval` 必须在共享内存复用前调用。

- **Pipeline Race Condition**：在 `memcpy_async` 提交后到等待完成前，读取 `dst_shared` 或写入 `src_global` 都属于数据竞争。

- **Token 的一次性**：`barrier_arrive()` 返回的 token 只能传给对应的 `barrier_wait()` 一次，不能复用或用于其他屏障实例。

## 常见陷阱与调试建议

1. **混淆主机与设备的 malloc/free**：从设备端 `cudaMalloc` 拿到的指针，如果误传到主机端 `cudaFree`，会导致错误。设备端 `cudaMalloc` 实际受限于 `cudaLimitMallocHeapSize`，而不是整块空闲显存，大分配可能失败。

2. **Grid sync 用错启动 API**：如果仍然用 `<<< >>>` 启动使用了 `this_grid().sync()` 的 kernel，运行时可能直接报错或死锁。必须改用 `cudaLaunchCooperativeKernel`，并通过占用率计算器保证所有块能同时驻留。

3. **忘记 commit 或 wait**：Pipeline 原语中，只调用 `memcpy_async` 而不 `commit`，请求不会进入流水线；只 `commit` 而不 `wait` 就读取共享内存，结果是数据竞争。建议始终成对使用。

4. **Barrier phase parity 误用**：`__mbarrier_test_wait_parity` 的 `phase_parity` 参数必须对应当前 phase 或前一 phase 的奇偶性。如果线程因为重排导致判断错了 phase，可能永远等不到 true。

5. **reduce/scan 的 lambda 陷阱**：即使 lambda 语义上与 `cg::plus<int>()` 完全一致，编译器也无法将其映射到 CC 8.0+ 的硬件加速指令。如果追求极限性能，应使用 Cooperative Groups 提供的预定义函数对象。

6. **SMID / WarpID 的易变性**：设备端 `%smid` 和 `%warpid` 是易失值（volatile）。调度器可能在线程块生命周期内将其迁移到不同 SM，因此不要把这些 ID 用作持久状态键。

7. **设备端流创建标志**：在设备端忘记传入 `cudaStreamNonBlocking` 会导致编译失败，因为 `cudaStreamCreate` 在此上下文中根本不存在。

## 一个最小可运行示例的说明

配套的 `chapter_demo.cu` 与 `Makefile` 旨在 GTX 1060（sm_61, CUDA 12.8）上编译运行，同时体现章节中“在受限硬件上做降级或兼容处理”的思路。

### 设计目标
示例分为两个互补的演示：
1. **Warp 级规约与线程块同步**：使用 `cg::this_thread_block()` 和 `cg::tiled_partition<32>()`，在所有 CC >= 5.0 的硬件上均可运行。每个 warp 通过 `shfl_down` 完成规约，展示了 Cooperative Groups 如何把底层 Warp shuffle 包装成类型安全的成员函数。
2. **Grid 级同步**：使用 `cg::this_grid()` 和 `grid.sync()`，但仅在检测到 `cudaDevAttrCooperativeLaunch` 为真时才执行。这对应了章节中强调的“必须通过 cooperative launch API 启动”的要求。

### sm_61 上的兼容处理
- **未使用 `cluster_group`**：因为 cluster 需要 CC 9.0+，示例在运行时打印了“NO (sm_61 unsupported)”的明确提示，避免编译期错误。
- **未使用 `memcpy_async` 硬件异步路径**：虽然 `__pipeline_memcpy_async` 在 CC 5.0+ 即可编译，但示例未引入该原语，因为真正的异步流水收益在 sm_61 上有限；如果引入，需要额外处理对齐和数据竞争检查。
- **未使用硬件加速 reduce/scan**：CC 8.0+ 才有的 `__reduce_add_sync` 等指令在 sm_61 上不存在，因此 warp 规约通过软件 shuffle 循环完成，与章节描述的 fallback 行为一致。
- **tile 大小限制为 32**：避免触及 `thread_block_tile` 在 >32 尺寸时需要在旧架构上分配 `block_tile_memory` 的复杂路径。
- ** Cooperative Launch 检测**：示例在主机侧查询设备属性，若不支持 cooperative launch（例如某些驱动配置或 Windows 旧版本），则跳过 Demo 2 并打印说明，而不是直接崩溃。

### 预期输出
在支持 cooperative launch 的 sm_61 平台上，Demo 1 会输出前 10 个 warp 的局部和；Demo 2 会输出经过“+1 再 x2”变换后的数组元素，验证 grid sync 确实让所有线程在第二阶段看到了第一阶段的完整结果。若 cooperative launch 不可用，程序会优雅降级，继续完成 Demo 1 并给出明确的跳过提示。
