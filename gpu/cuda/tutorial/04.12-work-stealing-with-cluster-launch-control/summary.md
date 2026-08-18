## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/cluster-launch-control.html>


本章出自《CUDA Programming Guide, Release 13.2》第 4.12 节，主题是通过 Cluster Launch Control（集群启动控制，简称 CLC）实现 Work Stealing（工作窃取）。该特性随 NVIDIA Blackwell 架构（compute capability 10.0）引入，核心目标是在 GPU 线程块调度层面提供一种动态负载均衡机制。它试图融合两种传统 CUDA 编程范式的优点：一方面保留"固定工作量 per 线程块"方案在负载均衡与抢占式调度上的优势，另一方面吸收"固定线程块数量"方案在降低启动开销与冗余计算上的长处。章节内容从问题背景出发，依次介绍了核心概念、CLC API 的详细语义、线程块取消的完整步骤、在 Thread Block Cluster 场景下的适配方式、关键约束与未定义行为、以及一个基于向量-标量乘法的完整代码示例。

## 背景与要解决的问题

在 CUDA 程序设计中，如何决定启动多少个线程块（thread block）是一个基础但影响深远的决策。传统上存在两种主流策略，它们各自对应不同的硬件调度行为和性能特征。

### 固定工作量 per 线程块（Fixed Work per Thread Block）

该策略的核心思想是让每个线程块处理固定大小的数据子集，因此线程块的总数直接由问题规模决定。例如，若总数据量为 N，每个线程块负责处理 blockSize 个元素，则启动的线程块数为 `(N + blockSize - 1) / blockSize`。

这种方案的最大优势体现在两个方面：

1. **负载均衡（Load Balancing）**：当不同线程块的执行时间存在差异，或者线程块总数远超 GPU 可同时执行的容量时，硬件调度器可以在某些 SM（Streaming Multiprocessor）上调度更多线程块，而在其他 SM 上调度较少，从而自动平滑尾部延迟（low-tail effect）。
2. **抢占支持（Preemption）**：GPU 调度器可以在低优先级内核已经开始执行后，仍然启动高优先级内核。具体做法是在低优先级内核的某些线程块完成后，将高优先级内核的线程块调度到腾出的 SM 上；待高优先级内核全部完成后，再继续调度剩余的低优先级线程块。

然而，该方案的缺点同样明显：线程块数量过多会放大设备侧 thread block 启动/调度相关开销；更重要的是，每个线程块都需要独立执行一些与 block index 无关的"序幕代码"（prologue），例如计算卷积核的系数。这些冗余计算在每个线程块中重复执行，造成显著的计算开销。这里的"启动/调度相关开销"不是指 host 侧一次 `cudaLaunchKernel` API 调用的延迟会按 block 数线性累积；一次 kernel launch 仍然只有一次 host 侧提交。它指的是 GPU 内部把大量 thread block 逐批投放到 SM 上执行时，每个 block 都会带来的调度、状态建立以及重复 prologue 成本。

(其实，这个就是关键，不然就是 block 越多越好了)

### 固定线程块数量（Fixed Number of Thread Blocks）

该策略通常以 block-stride 或 grid-stride loop 的形式实现。线程块总数不随问题规模变化，而是根据目标 GPU 的 SM 数量和期望占用率（occupancy）预先确定一个固定值（例如 1024 或 `SM_COUNT * BLOCKS_PER_SM`）。每个线程块通过循环跨越整个网格来遍历所有分配到的数据。

其优势在于：

1. **降低线程块开销**：设备侧 thread block 启动/调度开销被限制在固定数量的线程块上；同时，所有线程块共享的序幕计算（如卷积系数）只需执行固定次数，极大减少了冗余计算。

但代价也同样突出：当各线程块处理的数据量不均或执行时间差异较大时，固定数量的线程块无法让调度器在 SM 之间灵活重新分配工作，导致某些 SM 空闲而其他 SM 仍在忙碌，负载均衡能力较差。此外，由于线程块数量通常被刻意限制为与硬件资源匹配，抢占调度的灵活性也受到抑制。

### Cluster Launch Control 的融合思路

Blackwell 架构引入的 Cluster Launch Control 旨在同时获得上述两种方案的优点：既保持"固定线程块数量"带来的低开销，又通过硬件级的动态工作窃取恢复负载均衡与抢占能力。其基本思想是：允许一个正在执行的线程块向硬件发起请求，"取消"另一个尚未开始执行的线程块的启动；若取消成功，则发起者可以借用被 Cancel 掉的线程块的索引（block index）来完成额外的工作。这本质上是一种硬件辅助的 work stealing，将传统上由软件原子队列实现的动态任务分配，下沉到了 GPU 线程块调度器中。

---

## 核心概念与术语

### Work Stealing（工作窃取）

Work stealing 是并行计算中的一种经典动态负载均衡技术。与静态任务分配不同，work stealing 允许空闲的处理器主动从忙碌处理器的任务队列中"窃取"任务，而不是被动等待任务分配。在 CLC 的语境下，一个正在运行的线程块可以窃取另一个尚未启动的线程块的索引及其对应的数据范围，从而动态扩展自身的工作负载。

### Cluster Launch Control（集群启动控制）

CLC 是 Blackwell 架构提供的一套硬件机制与对应的软件 API，允许内核中的线程块异步提交对其他线程块（或线程块集群）的取消请求。取消操作是异步的，需要配合 shared memory barrier（mbarrier）进行同步。CLC 操作在底层被建模为 async proxy 操作，这意味着它们遵循异步拷贝代理的编程模型，涉及 `fence_proxy_async_generic_sync_restrict` 等内存一致性原语。

### Thread Block Cancellation（线程块取消）

这是 CLC 的核心原语。一个线程块通过调用 `clusterlaunchcontrol_try_cancel`（非集群场景）或 `clusterlaunchcontrol_try_cancel_multicast`（集群场景）向硬件提交取消请求。硬件会尝试取消一个尚未开始执行的线程块（或集群）。请求的结果被编码写入一个 `__shared__ uint4` 变量中，后续需要通过解码指令提取成功/失败状态和被取消的线程块索引。

### mbarrier（Shared Memory Barrier）

CLC 的取消请求使用 shared memory barrier 进行同步。`mbarrier` 是 CUDA 中用于线程块或集群级同步的细粒度屏障，支持基于到达计数（arrival count）和事务字节数（transaction count）的等待机制。在 CLC 场景中，通常由单个线程发起请求并将 barrier 的到达计数初始化为 1，然后通过 `mbarrier_arrive_expect_tx` 声明期望的事务大小（`sizeof(uint4)`），最后通过 `mbarrier_try_wait_parity` 轮询等待完成。

### Async Proxy Operation（异步代理操作）

CLC 操作在硬件层面被归类为 async proxy 操作。这意味着取消请求的提交、完成通知与后续对结果变量的读取，都需要考虑 async proxy 与 generic proxy 之间的内存可见性。代码中必须在适当位置插入 `fence_proxy_async_generic_sync_restrict`，以确保异步操作的结果对通用内存访问可见，并防止连续迭代之间的数据竞争。

### Thread Block Cluster（线程块集群）

Thread Block Cluster 是 Hopper/Blackwell 架构引入的一种执行模型，允许将多个线程块（通常是 2x1x1 或类似配置）组合成一个集群，集群内的线程块可以通过 cluster-wide shared memory 进行通信。在 CLC 的集群版本中，取消请求由单个集群线程提交，但结果会被多播（multicast）到集群内所有线程块的 shared memory 中。

---

## API / 机制详解

### 线程块取消的完整五步流程

CUDA 官方推荐由单个线程提交取消请求，以避免多线程并发取消带来的复杂性。整个流程分为"初始化"与"工作窃取循环"两个阶段。

#### 步骤 1：声明共享内存变量

```cpp
__shared__ uint4 result;      // 用于存放硬件返回的编码结果
__shared__ uint64_t bar;      // 用于同步的 shared memory barrier
int phase = 0;                // barrier 的 phase 变量，用于 parity wait
```

`result` 必须声明为 `__shared__`，因为硬件会将取消结果直接写入共享内存。`bar` 是 64 位的 mbarrier 对象，`phase` 用于 `mbarrier_try_wait_parity` 的轮询等待。Parity wait 是一种非阻塞的同步方式：调用者等待 barrier 的 phase 位翻转，每次成功后需要将 `phase ^= 1` 以准备下一次等待。

#### 步骤 2：初始化 mbarrier

```cpp
if (cg::thread_block::thread_rank() == 0)
    ptx::mbarrier_init(&bar, 1);
__syncthreads();
```

仅由线程块中的 0 号线程执行初始化，将 barrier 的到达计数设为 1。随后调用 `__syncthreads()` 确保所有线程在进入工作窃取循环前都能看到已初始化的 barrier。

#### 步骤 3：提交异步取消请求

```cpp
if (cg::thread_block::thread_rank() == 0) {
    cg::invoke_one(cg::coalesced_threads(), [&](){
        ptx::clusterlaunchcontrol_try_cancel(&result, &bar);
    });
    ptx::mbarrier_arrive_expect_tx(
        ptx::sem_relaxed, ptx::scope_cta, ptx::space_shared,
        &bar, sizeof(uint4)
    );
}
```

这一段包含三个关键操作：

1. **`clusterlaunchcontrol_try_cancel`**：这是实际的取消请求指令。它向硬件发起异步请求，尝试取消一个尚未执行的线程块。硬件会将编码后的结果异步写入 `result`，并在完成后通知 `bar`。
2. **`invoke_one`**：由于 `clusterlaunchcontrol_try_cancel` 是 uniform 指令（即所有活跃线程的输入必须一致），使用 `cg::invoke_one` 可以告知编译器仅由单个线程执行，从而避免编译器生成额外的 peeling loop 或谓词化开销。
3. **`mbarrier_arrive_expect_tx`**：单一线程在提交取消请求后，向 barrier 注册一个"到达"事件，并声明期望接收 `sizeof(uint4)` 字节的事务。Barrier 会在硬件完成写入并达到声明的事务量后变为完成状态。

#### 步骤 4：同步等待取消请求完成

```cpp
while (!ptx::mbarrier_try_wait_parity(ptx::sem_acquire, ptx::scope_cta, &bar, phase))
{}
phase ^= 1;
```

`mbarrier_try_wait_parity` 是非阻塞的：若 barrier 尚未完成，立即返回 false；调用者需要在 while 循环中反复轮询。一旦返回 true，表示取消请求已经完成，`result` 中的数据已可用。随后必须执行 `phase ^= 1` 翻转 phase 位，为下一次同步做准备。

#### 步骤 5：解码取消结果

```cpp
bool success = ptx::clusterlaunchcontrol_query_cancel_is_canceled(result);
if (success) {
    int bx = ptx::clusterlaunchcontrol_query_cancel_get_first_ctaid_x<int>(result);
    int by = ptx::clusterlaunchcontrol_query_cancel_get_first_ctaid_y<int>(result);
    int bz = ptx::clusterlaunchcontrol_query_cancel_get_first_ctaid_z<int>(result);
}
```

解码分为两层：首先通过 `query_cancel_is_canceled` 判断取消是否成功。若成功，则可以通过 `get_first_ctaid_{x,y,z}` 提取被取消线程块的索引。对于 1D 或 2D 线程块网格，可能只需要提取 x 分量。获得索引后，发起线程块即可使用该索引替代自身原始的 block index 来处理对应的数据范围，从而完成一次"窃取"。

### 内存一致性：Proxy Fence 的使用

CLC 操作作为 async proxy 操作，其结果写入共享内存的过程对 generic proxy 并非立即可见。因此，在工作窃取循环的边界需要插入 fence：

- **循环开始前（Acquire）**：确保上一次迭代对 `result` 的读取不会与本迭代新的 async 写入发生数据竞争，同时保证 barrier 初始化对所有线程可见。
- **循环结束后（Release）**：确保本迭代对 `result` 的读取结果已经同步回 async proxy，防止下一次迭代的请求在旧数据尚未消费完毕时覆盖 `result`。

具体指令为：

```cpp
ptx::fence_proxy_async_generic_sync_restrict(
    ptx::sem_acquire, ptx::space_cluster, ptx::scope_cluster
);
// ... 提交取消请求 ...
ptx::fence_proxy_async_generic_sync_restrict(
    ptx::sem_release, ptx::space_shared, ptx::scope_cluster
);
```

注意空间（space）的选择：对于映射到集群地址空间的远程 barrier，应使用 `space_cluster`；对于本地共享内存变量，应使用 `space_shared`。scope 则根据同步范围选择 `scope_cta`（线程块）或 `scope_cluster`（集群）。

### Thread Block Cluster 场景下的调整

当内核使用 `__cluster_dims__` 修饰符定义线程块集群时，CLC 的行为需要做以下适配：

1. **提交者身份**：取消请求仍然由单个集群线程提交（通过 `cg::cluster_group::thread_rank() == 0` 判断），而非每个线程块各提交一次。
2. **多播结果**：在集群场景下使用 `clusterlaunchcontrol_try_cancel_multicast`，硬件会将相同的编码结果多播到集群内所有线程块的 `result` 变量中。由于结果对应的是集群内的本地块索引 `{0,0,0}`，各线程块在解码后需要加上自己在集群中的局部偏移才能得到全局 block index。
3. **同步范围**：集群内的 barrier 操作需要使用 `scope_cluster` 而非 `scope_cta`，以确保集群内所有线程块都参与同步。
4. **存活保证**：集群场景下的取消要求集群内所有线程块都处于运行状态。用户可以通过 `cg::cluster_group::sync()` 确保这一点后再提交取消请求。
5. **初始化 fence**：集群场景下，barrier 初始化后需要额外插入 `fence_mbarrier_init`（release语义，cluster scope），确保集群内所有线程块都能看到初始化完成的 barrier。

---

## 典型工作流程 / 调用顺序

一个完整的、基于 CLC 的 work stealing kernel 通常遵循以下控制流：

1. **初始化阶段**：
   - 声明 `__shared__ uint4 result` 和 `__shared__ uint64_t bar`。
   - 单一线程调用 `mbarrier_init(&bar, 1)`。
   - 全线程块（或全集群）同步，确保 barrier 就绪。
   - 执行与 block index 无关的 prologue 计算（如标量系数、卷积核参数等）。

2. **设置当前工作索引**：
   - 用 `bx = blockIdx.x`（1D 场景）作为初始要处理的逻辑线程块索引。

3. **进入 Work-Stealing Loop**：
   - **边界同步**：`__syncthreads()`（非集群）或 `cg::cluster_group::sync()`（集群），保护 `result` 不被下一次迭代过早覆盖。
   - **提交取消请求**：单一线程执行 `fence_proxy_async_generic_sync_restrict(acquire)`，然后提交 `clusterlaunchcontrol_try_cancel`（或 `..._multicast`），再执行 `mbarrier_arrive_expect_tx`。
   - **执行当前任务**：用当前的 `bx` 计算数据偏移，执行实际计算（如 `data[i] *= alpha`）。注意，计算与取消请求的同步是重叠进行的，以隐藏同步延迟。
   - **等待取消完成**：所有相关线程通过 `mbarrier_try_wait_parity` 轮询等待 barrier。
   - **解码结果**：调用 `query_cancel_is_canceled` 判断是否成功。若失败，跳出循环，当前线程块完成并退出；若成功，提取被取消线程块的索引并赋给 `bx`。
   - **释放 fence**：执行 `fence_proxy_async_generic_sync_restrict(release)`，确保当前迭代对 `result` 的消费已完成。
   - **继续循环**：回到步骤 3 的边界同步，尝试窃取下一个线程块。

4. **退出条件**：
   - 当取消请求返回失败时，说明没有更多可窃取的工作，线程块退出。
   - 硬件调度器可以将腾出的 SM 资源用于其他内核或剩余线程块。

---

## 关键限制、边界条件与兼容性

### 架构限制

Cluster Launch Control 仅在 compute capability 10.0（Blackwell）及更高版本的 GPU 上可用。早期架构（如 Ampere、Ada、Hopper 的常规模式）均不支持相关 PTX 指令。

### 取消失败后再次提交的未定义行为（Critical Constraint）

这是最严格的语义限制：**一旦某个线程块观察到一次失败的取消请求（即 `query_cancel_is_canceled` 返回 false），则该线程块后续再提交任何新的取消请求都属于未定义行为（Undefined Behavior）**。

文档中给出了明确的对比例子：

- **错误代码**：先提交请求 A，同步并查询 A 失败，然后再提交请求 B。这是 UB。
- **正确代码**：先提交请求 A，紧接着提交请求 B，然后再同步并查询 A。即使 A 失败，B 也是合法的，因为在提交 B 时还没有"观察"到 A 的失败。

这条限制的原因是硬件状态机在一次失败的取消查询后可能进入特殊状态，无法安全地处理后续请求。

### 查询失败请求的线程块索引是 UB

如果取消请求失败，调用 `query_cancel_get_first_ctaid_*` 来提取线程块索引也是未定义行为。只有在确认 `is_canceled` 为 true 后，才能安全解码索引。

### 多线程并发提交不推荐

虽然硬件允许多个线程各自提交取消请求，但官方强烈不推荐这样做，因为它会带来额外的复杂性：

- 每个提交线程必须提供独立的 `__shared__ result` 指针，否则会发生数据竞争。
- 若多个线程共用同一个 barrier，必须正确调整到达计数（arrival count）和事务计数（transaction count），否则同步会出错。

### 抢占与高优先级内核的交互

CLC 的取消请求可能因硬件调度原因而失败。例如，当一个更高优先级的内核被提交到 GPU 上时，当前内核的某些尚未启动的线程块可能会被保留以让出资源。此时取消请求会失败，但失败并非错误，而是正常调度行为。当前线程块在观察到失败后应正常退出，这样调度器就可以将 SM 分配给高优先级内核。

---

## 常见陷阱与调试建议

### 1. Barrier 初始化遗漏或 scope 错误

`mbarrier_init` 只能由单一线程执行，且执行后必须通过 `__syncthreads()`（线程块级）或 `cg::cluster_group::sync()`（集群级）广播。在集群场景下，忘记插入 `fence_mbarrier_init(release, scope_cluster)` 会导致其他线程块看到未初始化的 barrier 状态。

### 2. Proxy Fence 的语义与方向混淆

- `acquire` fence 应在提交取消请求之前执行，确保 async proxy 对共享内存的写入权限被正确获取。
- `release` fence 应在解码结果之后、进入下一次循环之前执行，确保 generic proxy 对 `result` 的读取已经完成并同步回 async proxy。
- `space_cluster` 用于远程（集群地址空间）的映射变量，`space_shared` 用于本地共享内存变量。在混合使用两者时极易混淆。

### 3. Phase 变量未翻转

`mbarrier_try_wait_parity` 依赖 phase 位翻转来区分连续的同步事件。如果忘记执行 `phase ^= 1`，下一次等待会立即成功（因为 phase 匹配），导致读取到陈旧数据或发生数据竞争。

### 4. 计算与同步的重叠顺序不当

在推荐的编程模式中，线程块在提交取消请求后立即开始处理当前 `bx` 对应的任务，而不是先等待取消完成。这种"计算与通信重叠"的策略可以隐藏同步延迟。但如果代码逻辑导致在 `mbarrier_try_wait_parity` 返回前就需要使用新的 `bx` 值，就会发生逻辑错误。必须确保只有在 barrier 完成后才解码并使用新的 block index。

### 5. 集群内本地索引与全局索引的转换

在集群场景下，`query_cancel_get_first_ctaid_x` 返回的是被 Cancel 掉的集群的本地块索引（相对于集群原点）。如果集群大小为 `(2,1,1)`，则第二个线程块需要将返回值加上自己的局部偏移才能得到正确的全局 block index。忽略这一点会导致数据访问越界或重复计算。

### 6. 未处理失败即退出

当 `is_canceled` 返回 false 时，线程块应当立即退出循环并最终结束内核。如果代码在失败后仍然尝试使用旧的 `bx` 继续计算，不仅会导致重复处理，还可能因为后续的取消提交触发 UB。

---

## 一个最小可运行示例的说明

本章生成的示例代码 `chapter_demo.cu` 及其配套的 `Makefile` 旨在演示 Cluster Launch Control 的核心思想，同时确保能够在 sm_61（如 GTX 1060）等不支持硬件 CLC 的 GPU 上编译运行。以下是设计决策与实现细节的解释。


### 模拟版本的工作原理

在 `kernel_simulated_clc` 中，逻辑线程块总数 `total_logical_blocks` 仍由问题规模决定（与 `kernel_fixed_work` 相同），但实际启动的物理线程块数被限制为一个较小的固定值（例如 256，或更少）。每个物理线程块首先处理与其 `blockIdx.x` 对应的逻辑线程块；完成后，0 号线程通过 `atomicAdd` 原子地递增全局计数器，领取下一个尚未被处理的逻辑线程块索引。该索引通过 `__shared__` 变量广播给块内所有线程，随后线程块继续处理新索引对应的数据。当所有逻辑线程块都被领完后，原子计数器超过 `total_logical_blocks`，线程块检测到这一条件即退出循环。

这种软件模拟与真实 CLC 的异同：

- **相同点**：都实现了物理线程块数量固定、逻辑任务数量动态分配的负载均衡策略；都通过某种形式的"窃取"让先完成的线程块承担更多工作。
- **不同点**：
  - 软件模拟使用全局内存原子操作，会引入额外的内存竞争与延迟；真实 CLC 由硬件调度器直接完成索引重映射，延迟极低且无原子竞争。
  - 软件模拟的"窃取"范围仅限于逻辑任务索引的分配；真实 CLC 是在硬件层面取消一个尚未启动的线程块，这意味着被 Cancel 的线程块根本不会占用任何 SM 资源，也不会执行任何 prologue 代码。
  - 软件模拟无法利用 Blackwell 的抢占优势；真实 CLC 在取消失败时可以让线程块立即退出，为更高优先级内核腾挪资源。

### 兼容性与降级处理

在 sm_61 设备上运行本示例时，程序会输出如下提示：

```
Cluster Launch Control requires compute capability 10.0 (Blackwell) or higher.
This device (sm_61) does not support hardware CLC.
Running software-simulated work stealing instead.
```

随后程序依次执行三种 kernel，并对结果进行数值验证（所有元素应当等于 `1.0f * 2.0f = 2.0f`）。无论真实硬件是否支持 CLC，用户都可以观察到：固定工作量方案启动了大量线程块，固定线程块数方案启动较少但循环次数较多，而模拟 CLC 方案在两者之间取得了平衡，同时通过原子计数器实现了一定程度的动态负载均衡。

### Makefile 说明

Makefile 中指定的编译参数严格遵循要求：

- `-ccbin /usr/bin/g++-14`：指定主机编译器为 g++-14。
- `-std=c++17`：启用 C++17 标准，以支持 lambda 等特性（虽然模拟版本未使用 lambda，但为与真实 CLC 代码风格保持一致）。
- `-arch=sm_61`：针对 Pascal 架构生成代码，确保兼容性。
- `-O2`：启用优化，使性能对比更具参考价值。

默认目标为 `chapter_demo.out`，执行 `make run` 即可编译并运行。

## 简单的去理解几个 api 的含义

### `clusterlaunchcontrol_try_cancel(...)`

作用：

- 发起一次“尝试取消未启动 block”的异步请求

注意：

- 这是异步的
- 调用完不代表你立刻知道结果

### `mbarrier`

作用：

- 等硬件把取消结果写回 shared memory

可以把它理解成：

- 请求发出去了
- 结果稍后会回来
- `mbarrier` 用来等“结果包”真的到达

### `clusterlaunchcontrol_query_cancel_is_canceled(result)`

作用：

- 看这次偷工作有没有成功

### `clusterlaunchcontrol_query_cancel_get_first_ctaid_x(result)`

作用：

- 如果成功，就把被取消 block 的索引解码出来

有了这个索引，当前 block 就知道自己下一轮该处理哪份工作。
