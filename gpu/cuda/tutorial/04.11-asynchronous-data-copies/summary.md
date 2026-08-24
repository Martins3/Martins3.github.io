## cuda async copy
<!-- f60a2774-6157-4451-a6ca-c2c998c6784d -->

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/async-copies.html>


应该先阅读下:
https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/advanced-kernel-programming.html#asynchronous-data-copies

可以继续参考下
https://liujunming.top/2025/12/07/Notes-about-NVIDIA-TMA-Tensor-Memory-Access/

"Asynchronous Data Copies" 系统性地介绍了从 Compute Capability 8.0（Ampere）到 9.0（Hopper）引入的多种**设备端异步数据搬运机制** 也就是
**Kernel 内部**、**线程级或线程块级**的异步内存操作，核心目标是在 Global Memory 与 Shared Memory（以及 Cluster 内的分布式 Shared Memory）之间高效搬运数据，
并通过与计算重叠来隐藏延迟、降低寄存器压力。

章节按硬件能力递进组织：
1. 首先介绍 **LDGSTS**（Load Global Store Shared，sm_80+），用于细粒度的元素级异步拷贝；
2. 随后展开 **TMA**（Tensor Memory Accelerator，`sm_90+`），面向大批量一维或多维张量传输，并引入 Tensor Map 以 offload 复杂的多维地址计算;
3. 最后介绍 **STAS**（sm_90+），支持寄存器到分布式共享内存的异步写入，用于 Thread Block Cluster 内部的高效通信。

全章通过 stencil、数据预取、生产者-消费者、矩阵转置等具体示例，
展示了不同抽象层级 API 的用法、同步语义及性能权衡。

## 背景与要解决的问题

在大量 CUDA 应用（如 stencil、矩阵乘、卷积、Transformer）中，数据需频繁在 Global Memory 和 Shared Memory 之间搬运。传统做法是由线程显式执行 `LDG`（Load Global）到寄存器，再 `STS`（Store Shared）到共享内存。这种方式存在几个显著瓶颈：

1. **寄存器占用高**：数据必须先经过寄存器，增加了寄存器压力，可能限制 occupancy，进而降低调度效率。
2. **延迟暴露不足**：同步拷贝会阻塞线程，无法 overlap 数据搬运与计算，导致执行单元空闲。
3. **条件分支下的指令序列化**：当拷贝代码存在条件分支（如 stencil 中不同线程负责加载 center/halo）时，编译器可能生成非最优的 LDG/STS 交错序列，而非批量加载，无法充分利用全局内存带宽。
4. **多维数组地址计算复杂**：对于高维张量，子块（tile）搬运需要繁琐的步长、偏移计算，易出错且重复。

异步拷贝机制通过硬件层面的 `cp.async` 类指令（如 LDGSTS、TMA、STAS），
允许线程发起传输后立即返回执行其他计算或发起更多传输，由专用硬件单元完成实际数据搬运。这不仅减少了寄存器中介，
还显著提升了，使得"预取未来数据"与"计算当前数据"能够真正并行。

## 核心概念与术语

- **LDGSTS**：PTX 指令 `cp.async.ca.shared.global` 的抽象，支持 4/8/16 字节粒度的 Global 到 Shared 异步拷贝。16 字节拷贝可启用 L1 Bypass 模式，避免污染 L1 Cache。
- **Async Proxy / Async Thread**：CUDA 执行模型中将异步拷贝视为异步代理操作。发起线程不必等待完成，但需通过同步原语（barrier/pipeline）确认完成。
- **Pipeline**：`cuda::pipeline` 是 C++ 抽象，用于管理多阶段（multi-stage）的异步操作队列，支持 `producer_acquire` / `producer_commit` / `consumer_wait` / `consumer_release` 语义。
- **Partitioned Pipeline**：当线程块内部分线程充当 producer、部分充当 consumer 时，需使用基于 `cuda::pipeline_shared_state` 的分区流水线，配合 `cuda::thread_scope_block`。
- **Shared Memory Barrier (`cuda::barrier`)**：一种异步屏障，不仅跟踪线程到达（arrival），还通过 transaction count 跟踪异步内存操作的完成。
- **TMA (Tensor Memory Accelerator)**：Hopper (sm_90+) 引入的硬件单元，专用于 Global 与 Shared Memory 之间的大批量（bulk）数据传输，支持一维连续数组和多维张量。
- **Bulk-Asynchronous Copy**：TMA 对一维连续数据的拷贝，无需 Tensor Map，仅需指针和大小。
- **Bulk-Tensor Asynchronous Copy**：TMA 对多维张量的拷贝，依赖 **Tensor Map** 描述 Global/Shared 内存布局。
- **Tensor Map (`CUtensorMap`)**：Host 端通过 `cuTensorMapEncodeTiled` 创建的结构体，描述张量基地址、各维度大小、步长、共享内存 box 尺寸等，可通过 `__grid_constant__` 传入设备。
- **Bulk Async-Group**：TMA 写回（Shared 到 Global）时的线程本地完成机制，通过 `cp.async.bulk.commit_group` 和 `cp.async.bulk.wait_group` 管理。
- **STAS (Store Async)**：支持将寄存器数据异步写入 Cluster 内的分布式 Shared Memory（remote shared memory），通过 `cuda::ptx::st_async` 暴露。
- **Swizzle Mode**：TMA 多维拷贝时可选的共享内存重排模式，用于避免共享内存 bank conflict。

## API / 机制详解

### LDGSTS：Global 到 Shared 的细粒度异步拷贝

#### 操作规格与模式
LDGSTS 仅支持 **Global -> Shared::cta** 方向。拷贝粒度为 4、8 或 16 字节，源地址和目的地址需按拷贝尺寸对齐（最佳性能建议 128 字节对齐）。4/8 字节拷贝使用 L1 ACCESS 模式（数据缓存于 L1）；16 字节拷贝可使用 L1 BYPASS 模式，避免 L1 污染。完成信号可通过 Shared Memory Barrier 或 Pipeline 传递。

CUDA 提供了三层抽象，从高级到低级依次为：
1. **`cuda::memcpy_async`**（`<cuda/barrier>` 或 `<cuda/pipeline>`）：最符合 C++ 标准的方式，自动处理 barrier 的 transaction count 递增/递减。
2. **`cooperative_groups::memcpy_async`**（`<cooperative_groups/memcpy_async.h>`）：以线程组（block/thread）为单位集体发起，自动分派拷贝任务，但**每次调用立即 commit**，无法 batch 优化。
3. **`__pipeline_memcpy_async`**（`<cuda_pipeline.h>`）：最底层，直接映射到 `cp.async`，**保证**使用 LDGSTS，但代码最冗长。需手动 `__pipeline_commit()` 和 `__pipeline_wait_prior(N)`。

#### 条件代码中的批量加载（Stencil 示例）
在 stencil 计算中，线程块需加载 center 区域及左右 halo。传统同步代码中，条件分支可能导致编译器生成交错的 LDG/STS 而非批量加载。使用 `cuda::memcpy_async` 后：
- **Version 1（线程级）**：每个线程独立发起自己的异步拷贝，传入 `cuda::aligned_size_t<4>` 指示 4 字节对齐，目标为 block-wide `cuda::barrier`。
- **Version 2（集体级）**：以 `cg::this_thread_block()` 为参数调用 `cuda::memcpy_async(block, ...)`，API 自动在底层将拷贝任务分派给所有线程。
- **CUDA C primitives**：使用 `__pipeline_memcpy_async(dst, src, size)` 逐个元素发起，然后 `__pipeline_commit()` 提交整批，`__pipeline_wait_prior(0)` 等待全部完成。

关键语义差异：`cuda::memcpy_async` 与 barrier 绑定时，会**自动**将 barrier 的 expected count 加 1（表示有一个异步操作待完成），操作完成后硬件自动减 1。因此 barrier 的 phase 翻转条件是"所有参与线程已到达"且"所有绑定的异步操作已完成"。而 `cooperative_groups::memcpy_async` 使用 `cg::wait(block)` 作为完成信号，不暴露底层 barrier 细节。对于底层 primitive，开发者必须显式调用 `__pipeline_commit()` 才能将已排队的操作真正提交给硬件；否则拷贝不会执行。

#### 数据预取（Prefetching）与 Pipeline
对于迭代式"拷贝-计算"模式，多阶段流水线可将未来迭代的数据传输与当前迭代的计算重叠。以两阶段流水线为例：
1. **初始化**：创建 `cuda::pipeline<cuda::thread_scope_thread>`（或 block-scope），预提交 `num_stages` 个 `memcpy_async`。
2. **主循环**：对每个 batch，先 `consumer_wait_prior<pending_batches>(pipeline)` 等待当前可用数据，然后计算，再 `consumer_release()` 释放当前阶段，接着 `producer_acquire()` 并提交下一批的 `memcpy_async`，最后 `producer_commit()`。

`cooperative_groups::memcpy_async` 的预取版本无需显式 pipeline 对象，但无法做到"空提交"（no-op commit），这是其局限。低层 primitive 版本通过 `__pipeline_wait_prior<pending_batches>()` 实现类似语义。

#### 基于 Warp Specialization 的生产者-消费者模式
当线程块内部分 warps 专职做数据搬运（producer）、其余 warps 专职计算（consumer）时，需要**分区流水线（Partitioned Pipeline）**：
- 创建 `cuda::pipeline_shared_state<cuda::thread_scope_block, num_stages>` 共享状态。
- 调用 `cuda::make_pipeline(block, &shared_state, producer_count)` 生成 pipeline，其中 `producer_count` 是参与生产的线程数。
- Producer warp 使用 `producer_acquire / memcpy_async / producer_commit` 填充双缓冲（double-buffering）。
- Consumer warps 使用 `consumer_wait / compute / consumer_release`。
- 通过阶段轮转 `stage = (stage + 1) % num_stages` 实现乒乓缓冲。

底层实现也可结合 `__mbarrier_t`（共享内存 barrier）和 `__pipeline_memcpy_async`，通过 `__pipeline_arrive_on()` 将异步拷贝与 barrier 关联，显式等待 buffer ready/filled 状态。

### TMA：Tensor Memory Accelerator

TMA 是 sm_90+ (Hopper) 引入的专用硬件单元，用于 offloading 地址计算和执行 bulk 数据传输。

#### 一维 Bulk-Asynchronous 拷贝
对于一维连续数组，无需 Tensor Map。可用 API 包括：
- `cuda::memcpy_async`：若地址和尺寸满足 16 字节对齐，自动选择 TMA；否则回退同步拷贝。
- `cuda::device::memcpy_async_tx` / `cuda::ptx::cp_async_bulk`：**强制**使用 TMA，不满足对齐要求则导致未定义行为。

典型的 Read-Modify-Write 流程：
1. 单线程（通过 `is_elected()` + `ptx::elect_sync` 选出，避免 `if(threadIdx.x==0)` 导致的 warp 序列化）发起 `cp_async_bulk` 从 Global 到 Shared。
2. 若为 `memcpy_async_tx` 或 `cp_async_bulk`，需显式调用 `barrier_arrive_tx` 或 `mbarrier_expect_tx` 通知 barrier 预期接收的字节数。
3. 所有线程 `barrier.arrive()` 并 `bar.wait(token)`，等待数据就绪。
4. 线程并行修改 Shared Memory。
5. 调用 `ptx::fence_proxy_async(ptx::space_shared)` 保证 Shared Memory 写操作对后续 TMA 可见（generic proxy -> async proxy 顺序）。
6. `__syncthreads()` 确保全 block 的写都已完成。
7. 单线程发起 `cp_async_bulk` 从 Shared 写回 Global。
8. 创建 bulk async-group (`cp_async_bulk_commit_group()`) 并等待 (`cp_async_bulk_wait_group_read` 或 `write`)，等待只可由**发起线程**执行。

#### 多维 Bulk-Tensor 拷贝与 Tensor Map
多维张量拷贝必须依赖 Tensor Map。核心步骤：
- **Host 端创建**：通过 `cuTensorMapEncodeTiled`（Driver API）填充 `CUtensorMap` 结构。参数包括：
  - `globalAddress`：基地址（16 字节对齐）。
  - `tensorRank`：维数（1-5）。
  - `globalDim`：各维元素数。
  - `globalStrides`：各维步长（字节，需 16 字节倍数）。
  - `boxDim`：共享内存 buffer 各维尺寸。
  - `elementStrides`：元素间距（通常设为 1）。
  - `swizzle`, `L2promotion`, `OOBfill` 等模式。
- **传入 Device**：推荐方式是将 `CUtensorMap` 作为 `const __grid_constant__` 内核参数传入。也可通过 `__constant__` 内存或 Global 内存指针传递（后者需在 kernel 内执行 `fence_proxy_tensormap_generic` acquire fence）。
- **Device 端使用**：单线程调用 `ptx::cp_async_bulk_tensor` 并传入 `tensor_coords`（多维偏移），目标 Shared Memory 需 **128 字节对齐**。完成后通过 shared memory barrier 同步。

**越界处理**：读取时，若 tile 超出全局张量边界，越界部分自动 zero-fill；左上角坐标可为负值。写回时，越界部分被忽略，但左上角不可为负。

#### Device 端 Tensor Map 的编码与修改
当 kernel 需要处理一批不同尺寸的张量时，可在设备端修改 Tensor Map：
1. Host 先用 Driver API 创建**模板** Tensor Map（`template_tensor_map`）。
2. Kernel 内将模板拷贝到 Shared Memory（128 字节对齐）。
3. 使用 `ptx::tensormap_replace_*` 系列函数修改字段（基地址、rank、box_dim、global_dim、stride、element_stride、elemtype、swizzle 等）。
4. 同 warp 线程 `__syncwarp()`。
5. 调用 `ptx::tensormap_cp_fenceproxy(release, gpu, ...)` 将修改后的 Tensor Map 写回 Global Memory，并建立 release-acquire 语义。
6. 消费者线程需执行 `ptx::fence_proxy_tensormap_generic(acquire, sys/gpu, ...)` 获取最新 Tensor Map，之后才能用于 `cp.async.bulk.tensor`。

**注意**：on-device 修改仅支持 tiled-type tensor map，且需使用 `-arch=sm_90a` 编译。

#### Shared Memory Bank Swizzling
TMA 默认按 Global Memory 原始顺序写入 Shared Memory，可能导致 bank conflict。通过 Tensor Map 的 `CUtensorMapSwizzle` 可启用硬件重排：
- **128B / 64B / 32B / None**：定义了数据在 shared memory 中的交错周期。
- 共享内存 buffer 的**内维（inner dimension）**字节数必须小于等于 swizzle 宽度。
- 共享内存需按 swizzle 周期对齐（128B swizzle 需 1024B 对齐，但 buffer 本身仍需 128B 对齐）。
- 访问 swizzled shared memory 时，索引需按公式调整：`smem[y][((y+offset)%N)^x]`，其中 offset 由 shared memory 指针相对于 128B 边界的偏移决定。

### STAS：寄存器到分布式 Shared Memory 的异步拷贝
STAS (`st.async`) 仅支持 **Register -> Shared::cluster** 方向，粒度 4/8/16 字节，用于 Thread Block Cluster 内的跨 block 通信。

示例场景：8 个 block 组成 ring，每个 block 同时作为 producer（向右邻写入）和 consumer（从左邻读取）。需要：
- 每个 block 两个 barrier：`ready`（通知 producer 缓冲区可写）和 `filled`（通知 consumer 数据已到）。
- 使用 `cluster.map_shared_rank()` 获取 remote block 的 shared memory 地址和 barrier 地址。
- Producer 线程调用 `cuda::ptx::st_async(&remote_buffer[idx], value, remote_barrier)` 异步写入。
- Consumer 线程通过 `mbarrier_try_wait_parity` 等待本地 `filled` barrier。
- 消费完成后，consumer 向左侧 neighbor 的 `ready` barrier 发起 `mbarrier_arrive`。
- Producer 等待本地 `ready` barrier，确保右邻已消费完毕。

## 典型工作流程 / 调用顺序

### LDGSTS 三步走（使用底层 Primitives）
1. **Initiate**：各线程调用 `__pipeline_memcpy_async(dst_smem, src_global, size)`。每个调用排队一个异步操作。
2. **Commit**：调用 `__pipeline_commit()` 将此前所有操作绑定为一个 batch。此步骤是必要的，否则操作不会真正提交给硬件。
3. **Wait**：调用 `__pipeline_wait_prior(N)` 等待"相对于当前 batch 往前数第 N 个 batch"完成。`wait_prior(0)` 表示等待当前 batch 全部完成。
4. **Sync Threads**：若数据需被同 block 内其他线程共享，在 pipeline wait 后还需 `__syncthreads()`。

### TMA 1D Read-Modify-Write 流程
1. **初始化 barrier**：线程 0 调用 `init(&bar, blockDim.x)`。
2. **发起读**：选举出的单线程调用 `cp_async_bulk`（或 `memcpy_async`），指定 `smem` 为目的地、`global` 为源。
3. **设置 transaction count**：
   - `cuda::memcpy_async` 自动处理。
   - 其他 API 需显式 `barrier_arrive_tx(bar, 1, num_bytes)` 或 `mbarrier_expect_tx`。
4. **等待到达**：所有线程 `bar.arrive()` 获取 token，然后 `bar.wait(token)`。
5. **计算**：全 block 线程读写 Shared Memory。
6. **Proxy Fence**：`fence_proxy_async(space_shared)` + `__syncthreads()`，确保 Shared Memory 写对 async proxy 可见。
7. **发起写**：单线程调用 `cp_async_bulk`（global <- shared）。
8. **Bulk Async-Group 同步**：`cp_async_bulk_commit_group()` + `cp_async_bulk_wait_group_read/write(0)`，仅发起线程执行。

### TMA 2D Tensor 流程
1. Host 调用 `cuTensorMapEncodeTiled` 创建 `CUtensorMap`。
2. 以 `__grid_constant__` 参数传入 kernel。
3. Kernel 内初始化 block-wide barrier。
4. 单线程调用 `cp_async_bulk_tensor` 并传入坐标数组 `{x, y}`。
5. 到达 barrier 并等待。
6. 修改 Shared Memory 后，proxy fence + syncthreads。
7. 单线程调用写回方向的 `cp_async_bulk_tensor`，commit group 并等待。

## 关键限制、边界条件与兼容性

| 机制 | 最低 CC | 支持方向 | 对齐要求 | 其他限制 |
|------|---------|----------|----------|----------|
| LDGSTS | 8.0 | Global -> Shared::cta | 4/8/16B（最佳 128B） | 仅元素级（≤16B）；L1 Bypass 仅 16B |
| TMA 1D Bulk | 9.0 | Global <-> Shared::cta | Global/SMEM 16B，Size 16B 倍数 | 发起写后仅发起线程可等待 |
| TMA Multicast | 9.0 (sm_90a) | Global -> Shared::cluster | 同上 | 建议仅在 sm_90a 使用，否则性能大降 |
| TMA 2D Tensor | 9.0 | Global <-> Shared::cta/cluster | Global 16B，SMEM 128B，Stride 16B 倍数 | 需 Tensor Map；尺寸 ≥1；越界读自动补零 |
| STAS | 9.0 | Register -> Shared::cluster | 4/8/16B | 仅 cluster 内；需 `__cluster_dims__` |
| Device Tensor Map 修改 | 9.0 (sm_90a) | N/A | SMEM 128B | 仅 tiled-type；需 `tensormap.replace` PTX |

**重要语义限制**：
- `cooperative_groups::memcpy_async` 每次调用隐式 commit，无法将多个拷贝 batch 到一次 commit 中，效率通常低于其他 API。
- TMA 写回（Shared -> Global）不能用 shared memory barrier 跟踪完成，必须使用 bulk async-group（线程本地）。
- 使用 `if (threadIdx.x == 0)` 发起 TMA 可能导致编译器插入 peel loop，引起 warp 内序列化。应使用 `ptx::elect_sync` 或 `cg::invoke_one` 明确单线程发起。

## 常见陷阱与调试建议

1. **忽略 `__syncthreads()`**：LDGSTS 默认只保证**发起线程**看到的完成。若数据供同 block 其他线程使用，必须在 pipeline/barrier wait 后再执行 `__syncthreads()`。
2. **Barrier Transaction Count 不匹配**：手动使用 `mbarrier_expect_tx` 时，若指定的字节数与实际拷贝量不符，barrier 永远不会翻转。使用 `cuda::memcpy_async` 可减少此类错误。
3. **TMA 对齐违规**：Global Memory 基地址未 16B 对齐、或 1D 传输尺寸非 16B 倍数、或 2D 的 global stride 非 16B 倍数，都会导致未定义行为或静默错误。
4. **Shared Memory 对齐不足**：2D TMA 的 Shared Memory buffer 必须 128B 对齐，使用 `alignas(128)`。
5. **Swizzle 索引计算错误**：启用 Swizzle 后，Shared Memory 的访问索引必须按 `((y+offset)%N)^x` 调整，直接使用原始坐标会导致数据错位。
6. **Tensor Map 的 Acquire-Release 缺失**：若 Tensor Map 通过 Global Memory 指针传递， consumer 必须在首次使用前执行 `fence_proxy_tensormap_generic(acquire)`，否则可能读到旧值。
7. **编译架构不匹配**：Device 端修改 Tensor Map 需 `-arch=sm_90a`，普通 `-arch=sm_90` 可能无法编译或运行。
