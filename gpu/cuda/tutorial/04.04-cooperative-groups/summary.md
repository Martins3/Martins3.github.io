## cuda cooperative groups
<!-- a8837b6c-aeb4-4f41-b347-8bafcf5cfd58 -->

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/cooperative-groups.html>


本章介绍 CUDA 的 Cooperative Groups（协作组）编程模型。Cooperative Groups 是对 CUDA 原有同步与协作机制的系统化扩展，
允许开发者以显式、类型安全且可组合的方式组织协作线程，并在不同粒度（warp、thread block、grid、cluster）上执行同步与集合操作。
相较于早期仅靠 `__syncthreads()` 和手写 warp-level primitives 的方式，Cooperative Groups 提供了标准化的 API，
使代码更具可移植性、可维护性，并能在未来 GPU 架构上获得持续优化。

本章内容涵盖 Cooperative Group Handle 的语义、隐式组的获取方式、显式分区策略（tiled/stride/labeled/binary）、
多级同步原语（`sync` 与 `barrier`）、集合操作（`reduce`、`scan`、`invoke_one`）
以及异步数据搬运接口（`memcpy_async`）。


2026-08-16 的确，感觉这个 cooperative-groups 的作用主要是由于让 __syncthreads 用起来都更加正规，但是
似乎没添加什么新的东西。

注意和 thread block cluster 的关系是什么!

## 背景与要解决的问题

在 Cooperative Groups 出现之前，CUDA 只提供一种粗粒度的块内同步机制：`__syncthreads()`。该函数对整个 thread block 做屏障同步，虽然简单，但存在明显局限：

- **粒度单一**：无法安全、标准地对 warp 内子集线程或跨 block 的线程子集进行同步。许多性能敏感型开发者为了榨取 warp 级并行度，不得不手写基于 warp shuffle 或 voting intrinsics 的临时同步原语。
- **可移植性差**：这些手写原语往往依赖特定架构的指令行为（如 warp size 固定为 32 的假设），在新一代 GPU 上容易失效或产生性能退化。
- **维护成本高**：临时实现的同步代码 brittle（脆弱），难以在多个项目、多个架构代际之间复用和调优。

Cooperative Groups 的设计动机正是为了解决上述问题。它把"线程组"提升为一等概念，通过 Handle 封装组的元数据（大小、线程索引、维度等），并在此基础上提供标准化的分区、同步和集合操作。这样一来，开发者可以像搭积木一样将 grid 分解为 block，再将 block 分解为 warp 或更小的 tile，甚至跨 block 组成 cluster，而无需关心底层指令细节。

## 核心概念与术语

**Cooperative Group Handle（协作组句柄）**
这是 Cooperative Groups API 的中心抽象。一个 Handle 代表一组参与协作的线程，并提供查询接口（如 `thread_rank()`、`num_threads()`）和集体操作接口（如 `sync()`、`reduce()`）。Handle 必须在声明时初始化，没有默认构造函数，因此不能先声明后赋值。

**Implicit Group（隐式组）**
CUDA 运行时根据核函数启动配置自动创建的组。包括：
- `this_thread_block()`：当前 thread block 内的所有线程。
- `this_grid()`：当前 grid 内的所有线程。
- `coalesced_threads()`：当前时刻 warp 内处于活跃状态的线程集合（不保证具体是哪些线程，也不保证它们在整个执行期间保持合并）。
- `this_cluster()`：当前 cluster 内的所有线程块（要求 Compute Capability 9.0 或更高）。

**Partitioning（分区）**
将一个父组拆分为若干子组的操作。子组由新的 Handle 管理，所有线程必须参与分区调用。主要分区策略包括：
- `tiled_partition<N>`：按固定大小 N 将父组划分为一维、行主序的连续子组。
- `stride_partition`：以轮询（round-robin）方式将线程分配到子组。
- `labeled_partition`：根据整数标签将线程划分为一维子组，标签相同的线程进入同一子组。
- `binary_partition`：`labeled_partition` 的特化，标签只能为 0 或 1。

**Collective Operation（集合操作）**
要求组内所有线程共同参与的操作。包括同步（`sync`）、归约（`reduce`）、
扫描（`scan`）以及单线程代理执行（`invoke_one`）。
如果集合操作中各线程对同一参数传入不同值（除非 API 显式允许），行为未定义。
`scan` 是前缀计算：每个线程得到“它之前所有线程的累计结果”，常用于给并行线程
分配不冲突的输出位置。例如筛选数组时，每个线程先得到 `keep` 标志，再对 `keep`
做 `exclusive_scan`，保留元素的线程即可用扫描结果作为紧凑输出数组下标。

**barrier_arrive / barrier_wait**
Cooperative Groups 提供的分阶段屏障机制。`barrier_arrive` 返回一个 `arrival_token`，该 token 必须被传递给对应的 `barrier_wait` 并在此消耗。与 `cuda::barrier` 不同，CG barrier 自动初始化，且要求组内所有线程每阶段都必须到达一次。

## API / 机制详解

### 隐式组的获取与 Handle 成员函数

每个参与 Cooperative Groups 执行的线程首先需要一个 Handle。对于隐式组，直接在设备代码中调用以下函数即可：

- `cg::this_thread_block()`：返回当前 block 的 Handle（类型为 `cg::thread_block`）。
- `cg::this_grid()`：返回当前 grid 的 Handle（类型为 `cg::grid_group`）。
- `cg::coalesced_threads()`：返回当前 warp 内活跃线程的 Handle（类型为 `cg::coalesced_group`）。
- `cg::this_cluster()`：返回当前 cluster 的 Handle（要求 sm_90+，即 Hopper 或更新架构；非 cluster 启动时默认视为 1x1x1 cluster）。

Handle 提供的重要查询接口包括：
- `thread_rank()`：调用线程在组内的线性排名（0 开始）。
- `num_threads()`：组内线程总数。
- `thread_index()`：线程在启动块内的三维索引。
- `dim_threads()`：启动块的三维尺寸（以线程为单位）。

**性能建议**：文档强烈建议在核函数开头、任何分支发生之前尽早创建隐式组的 Handle，并在整个核函数中复用该 Handle。此外，Handle 应该按引用传递给被调用函数，而不是值拷贝；拷贝构造 Handle 不被推荐，因为这可能引入不必要的寄存器开销。

### 显式创建子组（分区操作）

分区是集体操作，父组内所有线程都必须执行同一条分区语句，否则会导致死锁或数据损坏。以 `tiled_partition` 为例：

```cpp
namespace cg = cooperative_groups;
cg::thread_block cta = cg::this_thread_block();
cg::thread_block_tile<8> tile = cg::tiled_partition<8>(cta);
```

上述代码将 block 按每 8 个连续线程划分为一个 tile。每个线程得到的 `tile` Handle 只代表它所属的那个 8 线程子组。`thread_rank()` 在 `tile` 上返回的是该线程在子组内的局部排名，而不是整个 block 中的全局排名。

`stride_partition` 与 `tiled_partition` 的区别在于线程到子组的映射方式：前者是轮询分配（类似跨距访问），后者是连续分配。`labeled_partition` 和 `binary_partition` 则允许根据运行时条件（如数据特征）动态决定子组划分，例如只把处理同一键值的线程分到同一组。

**危险提示**：如果分区调用位于某个并非所有线程都能到达的条件分支内，就会发生组创建危险（Group Creation Hazards）。由于分区是集体操作，缺失的线程永远不会参与，其余线程将永远等待，导致死锁。


### 集合操作

#### Reduce（归约）

`cg::reduce(group, value, op)` 对组内每个线程提供的 `value` 执行并行归约。支持的算子包括：
- `cg::plus<T>`：求和。
- `cg::less<T>`：取最小值。
- `cg::greater<T>`：取最大值。
- `cg::bit_and<T>`、`cg::bit_or<T>`、`cg::bit_xor<T>`：按位与、或、异或。

**硬件加速与兼容性**：当 GPU Compute Capability 达到 8.0（Ampere）或更高时，4 字节类型的归约会被硬件加速；旧架构（如 Pascal、Turing 的 sm_75 以下）会回退到软件实现。软件回退路径功能正确，但延迟更高。

#### Scans（扫描）

Cooperative Groups 提供 `inclusive_scan` 与 `exclusive_scan`，可对任意大小的组执行扫描（前缀和）。扫描同样接受可选的归约算子。与 `reduce` 类似，scan 的实现会利用硬件加速（若可用），否则使用软件回退。

**使用限制**：在 CUDA 12.x 的实现中，`scan` 接口通常要求组类型为 tile（如 `thread_block_tile<N>`），而不是任意 `thread_block`。直接对整个 `thread_block` 调用 `exclusive_scan` 会在编译期触发静态断言失败。开发者需要先将 block 分区为合适大小的 tile，再在 tile 上执行 scan。

#### `invoke_one` 与 `invoke_one_broadcast`

当组内需要串行执行一段代码时（例如只让一个线程打印日志或更新全局计数器），可使用 `invoke_one`。该函数从调用组中任意选择一个线程执行传入的可调用对象，线程选择机制不保证确定性。

- `invoke_one(group, callable, args...)`：仅执行，不返回值。
- `invoke_one_broadcast(group, callable, args...)`：执行并将返回值广播给组内所有线程。

**重要约束**：在可调用对象内部，不能对调用组进行任何通信或同步操作；但与组外线程通信是允许的。通常在 `invoke_one` 之后需要显式调用 `cg::sync`，确保组内其他线程等待该串行操作完成后再继续。

### Cluster Group（线程块簇）

`cg::this_cluster()` 返回当前 thread block cluster 的 Handle（类型为
`cg::cluster_group`）。Cluster 是位于 thread block 和 grid 之间的层级：
一个 grid 仍然由多个 block 组成，但启动时可以把相邻 block 按固定尺寸打包成
cluster。cluster 内的 block 保证共同调度，并支持 cluster 级同步和
distributed shared memory（DSMEM）。

常用接口包括：
- `cluster.block_rank()`：当前 block 在本 cluster 内的编号。
- `cluster.num_blocks()`：本 cluster 包含多少个 block。
- `cluster.num_threads()`：本 cluster 中的总线程数，等于
  `cluster.num_blocks() * blockDim.x * blockDim.y * blockDim.z`。
- `cluster.sync()`：cluster 内所有 block 的所有线程都到达后才继续。
- `cluster.map_shared_rank(ptr, rank)`：把当前 block 的 shared memory 地址
  映射为同 cluster 内第 `rank` 个 block 的 distributed shared memory 地址。

新增的 `cluster.cu` 是本章的最小 cluster demo。它用
`cudaLaunchKernelEx` 设置 `cudaLaunchAttributeClusterDimension = (2, 1, 1)`，
启动 4 个 block，每 2 个 block 组成一个 cluster：

```text
cluster 0: block 0, block 1
cluster 1: block 2, block 3
```

每个 block 先把 `blockIdx.x + 1` 写到自己的 shared memory；随后
`cluster.sync()` 保证同 cluster 内所有 block 都初始化完成；最后每个 block
用 `map_shared_rank()` 读取同 cluster 内其他 block 的 shared memory 并求和。
因此 block 0/1 都得到 `1 + 2 = 3`，block 2/3 都得到 `3 + 4 = 7`。

这个 demo 的重点是说明 cluster 和 grid 的区别：grid 级别包含所有 block，
但 cluster 同步和 DSMEM 访问只覆盖同一个 cluster 内的 block。不同 cluster
之间不能用 `cluster.sync()` 同步，也不能通过 DSMEM 直接访问彼此的 shared
memory。

## 典型工作流程 / 调用顺序

一个典型的 Cooperative Groups kernel 通常遵循以下步骤：

1. **尽早获取隐式组 Handle**：在 kernel 入口、任何条件分支之前，调用 `cg::this_thread_block()` 或 `cg::this_grid()` 获取顶层 Handle。
2. **执行初始同步（可选）**：若需要确保所有线程从一致状态开始，调用 `cg::sync(cta)`。
3. **分区（可选）**：根据算法需要将 block 划分为 tile 或其他子组，例如 `cg::tiled_partition<32>(cta)`。
4. **子组内并行计算**：在 tile 上执行 `reduce`、`scan` 或 shuffle 操作。
5. **组间同步**：子组操作完成后，若需要跨子组写共享内存或全局内存，再次调用 `cg::sync(cta)`。
6. **串行代理执行**：若需要单线程执行 I/O 或初始化，`cg::invoke_one(cta, ...)` 并在其后同步。
7. **最终同步与退出**：确保所有内存操作对后续 grid 级操作可见。

若使用 `memcpy_async`，流程变为：发起异步拷贝 -> 执行与数据无关的计算（隐藏延迟）-> `cg::wait(group)` -> 访问共享内存数据。

## 关键限制、边界条件与兼容性

1. **Compute Capability 要求**：
   - `this_cluster()` 要求 sm_90+（Hopper 或更新架构）。在旧设备上不可用。
   - `reduce` 的硬件加速要求 sm_80+（Ampere），旧设备使用软件回退。
   - `memcpy_async` 的底层异步指令在 sm_80+ 才原生存在；旧设备上功能正确但非异步。

2. **集合操作参数一致性**：
   所有线程在调用 `reduce`、`scan`、`sync` 等集体操作时，必须对对应参数传入相同的值（除非 API 文档显式允许不同值）。否则行为未定义。

3. **分区操作的集体性**：
   `tiled_partition`、`stride_partition`、`labeled_partition`、`binary_partition` 都要求父组内所有线程同时参与。任何线程若因条件分支跳过分区调用，都会导致未定义行为（通常是死锁）。

4. **Handle 生命周期与传递**：
   Handle 没有默认构造函数，必须在声明时初始化。建议按引用传递，避免不必要的拷贝。

5. **`coalesced_threads()` 的非确定性**：
   该函数返回当前时刻 warp 内的活跃线程，但不保证具体包含哪些线程，也不保证这些线程在后续执行中继续合并。不应依赖其成员构成做算法决策。

6. **`cudaPointerGetAttributes` 与 stream-ordered 内存**：
   虽然不属于 Cooperative Groups 本身，但在同一章节上下文中提到：对已通过 `cudaFreeAsync` 释放的内存调用 `cudaPointerGetAttributes` 会导致未定义行为，无论该内存是否仍能被某条流访问。

7. **`cudaGraphAddMemsetNode` 限制**：
   `cudaGraphAddMemsetNode` 不能直接与 stream-ordered allocator 分配的内存一起工作，但可以通过流捕获（stream capture）记录 memset 操作。

8. **对齐限制**：
   `memcpy_async` 要求源和目标至少 4 字节对齐，推荐 16 字节对齐。不对齐时功能仍正确，但性能会下降。

## 常见陷阱与调试建议

1. **在条件分支中创建或使用组 Handle**
   这是新手最容易犯的错误。例如：
   ```cpp
   if (threadIdx.x < 64) {
       auto tile = cg::tiled_partition<16>(cta); // 危险！
   }
   ```
   只有部分线程进入分支，其余线程不会参与 `tiled_partition`，导致死锁。**修复方法**：在所有线程都可达的路径上执行分区，或在分区后再根据排名做条件判断。

2. **混淆局部 rank 与全局 rank**
   `tile.thread_rank()` 返回的是子组内的局部排名，而 `cta.thread_rank()` 返回的是 block 内的全局排名。在子组操作后直接用局部排名索引全局数组会导致越界或数据竞争。

3. **`invoke_one` 内部尝试同步**
   在 `invoke_one` 的可调用对象内部调用 `cg::sync` 或访问同一组的共享内存屏障是未定义行为。应将对组的同步放在 `invoke_one` 调用之外。

4. **忽略 `barrier_arrive` 与 `barrier_wait` 的配对规则**
   `barrier_arrive` 返回的 token 必须传给对应的 `barrier_wait`，且不能跨阶段复用。错误地丢弃 token 或重复使用会导致屏障语义混乱。

5. **过度依赖 warp size 为 32 的假设**
   虽然当前 NVIDIA GPU 的 warp size 都是 32，但 Cooperative Groups 的设计目标之一是屏蔽这种底层假设。使用 `tiled_partition<32>` 是可移植的，但在核函数中硬编码 `32` 做循环步长而不通过 Handle 查询组大小，会降低代码的前向兼容性。

6. **编译错误：scan 不支持 thread_block**
   若直接对整个 `cg::thread_block` 调用 `exclusive_scan`，编译器会报静态断言失败（"This group does not exclusively represent a tile"）。正确做法是先 `tiled_partition` 到合适大小的 tile，再执行 scan。
