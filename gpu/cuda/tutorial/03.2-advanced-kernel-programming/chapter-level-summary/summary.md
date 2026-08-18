## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/advanced-kernel-programming.html>

## CUDA Advanced Kernel Programming 章节总结

本章节深入介绍了 NVIDIA GPU 的硬件执行模型，并系统阐述了 CUDA 内核代码中的若干高级特性，旨在帮助开发者在理解底层机制的基础上编写出更高性能的 GPU 程序。内容涵盖 PTX 指令的使用、SIMT 执行模型与独立线程调度、线程作用域、高级同步原语（作用域原子操作、异步屏障、Pipeline）、异步数据拷贝机制（LDGSTS、TMA、STAS），以及 L1/Shared Memory 的配置方法。

---

### 1. 直接使用 PTX

PTX（Parallel Thread Execution）是 CUDA 用来抽象底层硬件 ISA 的虚拟指令集架构。直接编写 PTX 属于极为底层的优化手段，通常只有在性能极度敏感、需要逐条指令精细控制的场景下才会使用，应视为“最后手段”。

CUDA 提供了两种使用 PTX 的方式：

- **`cuda::ptx` 命名空间**：libcu++ 提供了一系列直接映射到 PTX 指令的 C++ 函数，降低了在 C++ 代码中嵌入 PTX 的难度。
- **Inline PTX**：与 CPU 上的内联汇编类似，可以直接在 CUDA C++ 中插入 PTX 汇编语句。

---

### 2. GPU 硬件实现

#### 2.1 SIMT 执行模型

流式多处理器（SM）采用 **SIMT（Single-Instruction, Multiple-Thread）** 模型来管理海量线程。SM 以 **Warp**（32 个线程为一组）为单位进行创建、调度、执行。

- 一个 Warp 内的 32 个线程从同一程序地址同时启动，但各自拥有独立的指令地址计数器（PC）和寄存器状态，因此可以独立分支。
- Warp 在任一时刻执行的是同一条指令；当 Warp 内线程因数据依赖产生条件分支而发散（Divergence）时，SM 会串行执行每条被采纳的分支路径，暂时屏蔽不在该路径上的线程。不同的 Warp 之间则完全独立执行，不会相互造成分支发散。
- **SIMT vs SIMD**：SIMT 与 SIMD 的关键区别在于 SIMD 将向量宽度暴露给软件，而 SIMT 的指令描述的是单个线程的执行与分支行为。程序员在编写正确性逻辑时可以忽略 SIMT 细节，但在追求峰值性能时，必须像考虑 Cache Line 一样考虑 Warp 内分支发散问题。

#### 2.2 独立线程调度（Independent Thread Scheduling）

- **Compute Capability < 7.0**：Warp 内所有线程共享一个程序计数器和一个活跃掩码（active mask）。因此，同 Warp 内处于不同执行状态或发散区域的线程之间无法通过锁或互斥量进行细粒度数据交换，否则可能产生死锁。
- **Compute Capability >= 7.0**（Volta 及以后）：引入了独立线程调度。GPU 为每个线程维护独立的执行状态（PC、调用栈），可以在线程粒度上进行切换与调度。调度优化器会将同一 Warp 内活跃的线程动态组合成 SIMT 单元执行，既保留了 SIMT 高吞吐，又提供了子 Warp 粒度的分支发散与收敛能力。

**重要影响**：独立线程调度打破了旧代码中对“隐式 Warp 同步”的假设。在 CC 7.0 之前的 GPU 上，程序员常常依赖 Warp 内线程在每个指令上锁步执行；这种假设在 Volta 及以后不再成立。任何基于隐式 Warp 同步的代码（如无需同步的 Warp 内归约）都应使用 `__syncwarp()` 进行显式同步，以确保跨代兼容性。

**内存一致性注意**：
- 如果 Warp 内多个线程通过**非原子指令**写入全局或共享内存的同一位置，最终哪个线程的写入生效是未定义的（但一定存在若干次串行化写入）。
- 如果 Warp 内多个线程通过**原子指令**对全局内存同一位置执行读-改-写操作，则每个操作都会发生且被串行化，但发生的顺序未定义。

#### 2.3 硬件多线程

SM 接收到一个或多个线程块（Block）后，会将其划分为 Warp，再由 Warp Scheduler 调度执行。Block 内的线程按连续的 thread ID 组成 Warp（第一个 Warp 包含 thread 0）。

Block 中 Warp 的总数计算公式为：

```
ceil(T / Wsize, 1)
```

其中 `T` 为 Block 内的线程数，`Wsize` 为 Warp 大小（32），`ceil(x, y)` 表示将 `x` 向上取整到 `y` 的最近倍数。

- Warp 的上下文（PC、寄存器等）始终保存在芯片上，因此 Warp 之间切换没有任何开销。
- 每个 SM 的 32 位寄存器和共享内存资源在 Block/Warp 之间划分。对于给定内核，SM 上能同时驻留的 Block 和 Warp 数量取决于内核使用的寄存器数量、共享内存大小，以及 SM 的硬件上限。如果资源不足以至少驻留一个 Block，内核将无法启动。

#### 2.4 异步执行特性

现代 NVIDIA GPU 引入了异步执行能力，允许数据搬运、计算和同步在 GPU 内部进一步重叠。这里的“异步”指的是**单内核启动内部**的异步，与 `cudaMemcpyAsync` 等 CUDA API 级别的异步不同。

- **CC 8.0（Ampere）**：引入硬件加速的 global memory 到 shared memory 的异步数据拷贝，以及异步屏障。
- **CC 9.0（Hopper）**：扩展了异步执行能力，引入张量内存加速器（TMA），支持大块数据和多维张量的 global<->shared 异步传输；还引入了异步事务屏障和异步矩阵乘累加操作。

CUDA 将异步操作建模为由一个 **async thread**（异步线程）执行，initiator 是发起该操作的 CUDA 线程。async thread 通过同步对象（Barrier 或 Pipeline）通知完成状态。

**Proxy 概念**：
- **Generic Proxy**：普通的 load/store 操作走的通道。
- **Async Proxy**：某些异步指令（如 TMA、`tcgen05.*`、`wgmma.mma_async.*`）由 async thread 在 async proxy 中执行。
- 若 async thread 在 **generic proxy** 中执行（如 `LDGSTS`、`STAS/REDAS`），则该异步操作之前的普通访存会被保证按序发生在异步操作之前；但之后的普通访存不再保证顺序，需等待异步操作完成。
- 若 async thread 在 **async proxy** 中执行，则前后的普通访存均不保证与异步操作之间的顺序，必须使用 **proxy fence** 来跨 proxy 同步内存顺序。

---

### 3. 线程作用域（Thread Scopes）

CUDA 线程具有层次结构（Thread Hierarchy），不同层级上的线程观察到的内存操作可见性不同。CUDA 引入了 **Thread Scope** 概念，用于定义：

1. 哪些线程可以看到某个线程的 load/store 操作；
2. 哪些线程可以通过原子操作或屏障相互同步。

每个作用域对应内存层次结构中的一个一致性点（Point of Coherency）。

| CUDA C++ 作用域 | CUDA PTX 作用域 | 可见范围 | 一致性点 |
|---|---|---|---|
| `cuda::thread_scope_thread` | — | 仅本地线程可见 | — |
| `cuda::thread_scope_block` | `.cta` | 同一线程块（Block）内的线程 | L1 |
| `.cluster` | `.cluster` | 同一线程块集群（Cluster）内的线程 | L2 |
| `cuda::thread_scope_device` | `.gpu` | 同一 GPU 设备上的所有线程 | L2 |
| `cuda::thread_scope_system` | `.sys` | 同一系统内所有线程（CPU、其他 GPU） | L2 + 互联缓存 |

这些作用域在 `cuda::atomic`、libcu++ 的同步原语以及 PTX 中均可使用。

---

### 4. 高级同步原语

本节介绍了三类同步原语：作用域原子操作、异步屏障和 Pipeline。

#### 4.1 作用域原子操作（Scoped Atomics）

作用域原子操作将 **C++ 标准原子内存序** 与 **CUDA 线程作用域** 结合，可在 Block、Cluster、Device 或 System 级别安全高效地进行线程间通信。

**核心概念**：
- **Thread Scope**：定义原子操作效果的可见范围。
- **Memory Ordering**：定义该原子操作相对于其他内存操作的顺序约束（如 `memory_order_relaxed`、`acquire`、`release`、`acq_rel`、`seq_cst`）。

**典型用法示例**：
- **Block-scoped 计数器**：在 `__shared__` 内存中声明 `cuda::atomic<int, cuda::thread_scope_block>`，所有线程使用 `fetch_add` 原子递增。若仅需原子性而无跨地址顺序要求，使用 `memory_order_relaxed` 即可。
- **Acquire-Release 生产者-消费者**：生产者线程先写入数据，再对原子标志执行 `store(..., release)`；消费者线程通过 `load(..., acquire)` 轮询标志，确保一旦看到标志为真，则一定能看到生产者写入的数据。

**性能建议**：
- 使用**最小可能的作用域**（Block 级别远快于 System 级别）。
- 使用**最弱的内存序**（仅在正确性需要时才使用更强的序）。
- 优先在 **shared memory** 中使用原子操作（比 global memory 更快）。

#### 4.2 异步屏障（Asynchronous Barriers）

异步屏障将同步过程拆分为 **arrive（到达）** 和 **wait（等待）** 两个阶段。线程在 arrive 后可以执行不依赖同步结果的其他工作，从而提高执行效率。异步屏障还可用于跟踪异步数据拷贝的完成状态。

- **可用性**：CC 7.0+。CC 8.0+ 在 shared memory 中提供硬件加速，并支持 Block 内任意子集线程的同步（此前架构仅加速整 Warp `__syncwarp()` 或整 Block `__syncthreads()`）。CC 9.0+ 扩展至 Cluster 级别。
- **API 层级**：
  - **高级**：`cuda::barrier`（libcu++，ISO C++ 兼容，支持选择 thread scope）。
  - **中级**：`cuda::ptx`（`mbarrier_init`、`mbarrier_arrive`、`mbarrier_try_wait` 等）。
  - **低级**：primitives API（`__mbarrier_t`、`__mbarrier_init`、`__mbarrier_arrive`、`__mbarrier_try_wait` 等）。

**时间拆分（Temporal Splitting）**：
- 传统同步：`code_before` -> `__syncthreads()`（阻塞）-> `code_after`。
- 异步同步：`code_before` -> `arrive()`（非阻塞，带隐式 seq_cst 内存栅栏）-> `unrelated_work` -> `wait(token)`（阻塞）-> `code_after`。

内存可见性保证：所有在参与线程调用 `arrive()` 之前发生的内存更新，都会在其调用 `wait()` 之后对所有参与线程可见。

#### 4.3 Pipelines

Pipeline 是一种协调多阶段异步内存拷贝的同步对象，常用于实现**双缓冲/多缓冲**的生产者-消费者模式，以掩盖内存延迟。

- 本质是一个带有 head 和 tail 的 FIFO 双端队列。
- 生产者向 head 提交工作；消费者从 tail 拉取工作。

**`cuda::pipeline` API**：
- `producer_acquire()`：获取 Pipeline 内部队列中的一个可用阶段。
- `producer_commit()`：提交自上一次 `producer_acquire` 以来在该阶段上发出的所有异步操作。
- `consumer_wait()`：等待 Pipeline 最老阶段（tail）中的异步操作完成。
- `consumer_release()`：释放最老阶段，使其可被生产者重新获取。

**Primitives API**：
- `__pipeline_memcpy_async(...)`：提交 global -> shared 的异步拷贝。
- `__pipeline_commit()`：提交当前阶段的操作。
- `__pipeline_wait_prior(N)`：等待除最近 N 次 commit 之外的所有操作完成。

Primitives API 功能受限（仅支持特定大小和对齐要求的 global->shared 拷贝），等价于 `cuda::thread_scope_thread` 的 `cuda::pipeline`。

---

### 5. 异步数据拷贝

GPU 通过大量线程切换来隐藏内存延迟，但当内存延迟成为瓶颈时，会同时影响带宽利用率和计算资源效率。异步数据拷贝机制允许线程在发起内存传输后继续执行其他计算，从而在内核内部重叠计算与数据搬运。

#### 5.1 基本原理

GPU 计算中常见的“拷贝-计算”模式包含三个阶段：
1. 从 global memory 读取数据；
2. 写入 shared memory；
3. 在 shared memory 上计算，并可能将结果写回 global memory。

传统实现中，`shared[local] = global[global_idx]` 会被编译器展开为“global -> register -> shared”的两步操作。在迭代算法中，每次迭代都需要 `__syncthreads()` 保证共享内存写入完成，计算后还需要再次同步。

使用异步拷贝后，可以通过 `cooperative_groups::memcpy_async` 等 API 将 global -> shared 的传输异步化。线程发起拷贝后即可执行其他无关计算，随后通过 `cooperative_groups::wait` 等待拷贝完成。这样就把内存传输的等待时间转化为了有用计算时间。

#### 5.2 硬件机制

| 机制 | 最低 CC | 功能描述 |
|---|---|---|
| `LDGSTS` | 8.0 (Ampere) | 高效的小规模 global -> shared 异步传输 |
| TMA | 9.0 (Hopper) | 大块数据、多维张量的 bulk-asynchronous 拷贝（global <-> shared） |
| STAS | 9.0 (Hopper) | 小规模寄存器 -> cluster distributed shared memory 的异步传输 |

**支持的内存路径概览**（来自原文 Table 5）：
- global -> shared (CTA)：支持 `LDGSTS` (8.0+) 和 TMA (9.0+)。
- global -> shared (cluster)：支持 TMA (9.0+)。
- shared (cluster) -> shared (CTA)：支持 TMA (9.0+)。
- shared (CTA) -> global：支持 TMA (9.0+)。
- registers -> shared (cluster)：支持 STAS (9.0+)。

**注意**：在异步拷贝完成之前，修改源 global 数据或读写目标 shared 数据都会引入数据竞争。

---

### 6. 配置 L1 / Shared Memory 比例

SM 上的 L1 数据缓存和 shared memory 共享同一物理资源，称为统一数据缓存（unified data cache）。如果内核很少或不使用 shared memory，可以通过配置将更多资源分配给 L1 缓存。

使用 `cudaFuncSetAttribute` 在内核启动前设置 carveout（优先共享内存容量）：

```cpp
cudaFuncSetAttribute(kernel_name,
                     cudaFuncAttributePreferredSharedMemoryCarveout,
                     carveout);
```

- `carveout` 可以是一个整数百分比（相对于该架构最大 shared memory 容量的百分比）。
- 也可以使用三个便捷枚举值：
  - `cudaSharedmemCarveoutDefault`
  - `cudaSharedmemCarveoutMaxL1`
  - `cudaSharedmemCarveoutMaxShared`

实际分配的容量会向上取整到该架构支持的最近一个离散档位。例如，CC 12.0 支持 0、8、16、32、64、100 KB，若设置 50%，则会分配 64 KB 而非 50 KB。

---

### 核心概念速查表

| 概念 | 一句话解释 |
|---|---|
| **PTX** | CUDA 的虚拟 ISA，直接编写属于极端优化手段。 |
| **Warp** | SM 调度的基本单位，包含 32 个线程。 |
| **SIMT** | 单指令多线程，Warp 内线程共享指令流但可独立分支。 |
| **分支发散** | Warp 内线程走不同分支路径时，SM 串行执行各路径，降低效率。 |
| **独立线程调度** | CC 7.0+ 特性，每线程独立 PC 和栈，可子-Warp 粒度发散/收敛。 |
| **隐式 Warp 同步** | CC 7.0 前可依赖的行为，新架构上不再安全，需改用 `__syncwarp()`。 |
| **Async Thread** | 执行异步操作的逻辑线程，与发起操作的 CUDA 线程不同。 |
| **Generic Proxy** | 普通 load/store 的内存访问通道。 |
| **Async Proxy** | TMA 等高级异步指令的内存访问通道，需 proxy fence 保证顺序。 |
| **Thread Scope** | 定义内存操作可见性和同步范围的层级概念。 |
| **Scoped Atomics** | 结合 C++ memory order 与 CUDA thread scope 的原子操作。 |
| **Async Barrier** | 将同步拆分为 arrive 和 wait 两阶段，可重叠其他工作。 |
| **Pipeline** | 用于多阶段异步拷贝的 FIFO 队列，实现双缓冲/多缓冲。 |
| **LDGSTS** | CC 8.0+ 的 global -> shared 异步拷贝指令。 |
| **TMA** | CC 9.0+ 的张量内存加速器，支持 bulk 异步多维张量传输。 |
| **STAS** | CC 9.0+ 的 registers -> cluster shared memory 异步传输指令。 |
| **Carveout** | 配置统一数据缓存中 shared memory 与 L1 的比例。 |

---

### 关键 API / 函数

#### PTX 与底层
- `cuda::ptx::` 命名空间函数（直接映射 PTX 指令）
- Inline PTX 汇编语法

#### Warp 同步
- `__syncwarp(mask)` — 显式同步 Warp 内指定掩码的线程，跨代兼容。
- `__shfl_down_sync(mask, val, offset)` / `__shfl_sync(mask, val, srcLane)` — Warp 内数据交换。

#### 线程作用域与原子操作
- `cuda::atomic<T, cuda::thread_scope_block>` 等（需 `#include <cuda/atomic>`）
- `cuda::memory_order_relaxed` / `acquire` / `release` / `acq_rel` / `seq_cst`
- 内置原子函数：`__nv_atomic_store_n`、`__nv_atomic_load_n`、`__nv_atomic_fetch_add` 等（带 scope 和 memory order 参数）

#### 异步屏障
- `cuda::barrier<Scope>`（`#include <cuda/barrier>`）
  - `init(&bar, count)`
  - `bar.arrive()` -> 返回 `arrival_token`
  - `bar.wait(token)`
- `cuda::ptx::mbarrier_init`、`mbarrier_arrive`、`mbarrier_try_wait`
- Primitives：`__mbarrier_init`、`__mbarrier_arrive`、`__mbarrier_try_wait`
- `cuda::device::barrier_native_handle()` — 获取 `cuda::barrier` 的原生句柄以与 PTX 互操作

#### Pipeline
- `cuda::pipeline` 方法：`producer_acquire`、`producer_commit`、`consumer_wait`、`consumer_release`
- Primitives：`__pipeline_memcpy_async`、`__pipeline_commit`、`__pipeline_wait_prior(N)`

#### 异步数据拷贝（Cooperative Groups）
- `cooperative_groups::memcpy_async(block, dst, src, bytes)`
- `cooperative_groups::wait(block)`

#### L1/Shared 配置
- `cudaFuncSetAttribute(kernel, cudaFuncAttributePreferredSharedMemoryCarveout, value)`
- `cudaSharedmemCarveoutDefault`、`cudaSharedmemCarveoutMaxL1`、`cudaSharedmemCarveoutMaxShared`

---

### 注意事项与常见陷阱

1. **PTX 不是常规优化手段**
   - 直接写 PTX 会严重降低代码可维护性，只有在 profile 后确认瓶颈且高级优化已穷尽时才考虑。

2. **不要依赖隐式 Warp 同步**
   - 在 CC 7.0+ 的设备上，Warp 内线程可以在子-Warp 粒度发散和收敛。任何假设“同 Warp 内所有线程在每个指令上都锁步执行”的代码（如手动 Warp 归约未加 `__syncwarp`）都可能出错。
   - **修复方案**：在 Warp 内协作操作前后显式调用 `__syncwarp()`。

3. **Warp 内并发写同一地址结果未定义**
   - 如果 Warp 内多个线程通过**非原子**操作写入同一 global/shared 地址，最终生效的线程是不确定的。即使使用原子操作，多个线程执行的原子操作顺序也是未定义的。
   - **修复方案**：确保同一 Warp 内只有一个线程写入特定地址，或使用原子操作并逻辑上接受任意顺序。

4. **异步屏障的可用性与硬件加速差异**
   - `cuda::barrier` 等 API 需要 CC 7.0+；硬件加速的异步屏障需要 CC 8.0+（Block 级别）或 CC 9.0+（Cluster 级别）。
   - 异步屏障依赖较新的硬件能力；编写示例时应明确目标架构并用设备属性确认运行条件。

5. **异步拷贝期间的内存竞争**
   - 在 `memcpy_async` 发起后、`wait` 完成前，**不要**读取目标 shared memory 或修改源 global memory，否则会产生数据竞争。

6. **Proxy Fence 不可遗漏**
   - 当使用 TMA 等 **async proxy** 机制时，异步操作前后的普通 load/store 与该异步操作之间不保证顺序。必须通过 proxy fence（如 `cuda::ptx` 中的相关指令）显式建立跨 proxy 的内存顺序。

7. **Carveout 向上取整**
   - `cudaFuncSetAttribute` 设置的 carveout 百分比不一定精确映射到实际容量。系统会向上取整到该 CC 支持的最近档位。如果不确定，建议通过 occupancy API 或运行时查询确认实际分配的 shared memory 大小。

8. **作用域原子操作的性能选择**
   - 不要为了“保险”而盲目使用 `cuda::thread_scope_system` 和 `memory_order_seq_cst`。这会显著降低性能。应遵循“最小 scope + 最弱 order”原则。

9. **`__shared__` 上的类类型对象初始化**
   - 将 `cuda::atomic` 等类类型对象放在 `__shared__` 中时，不能像普通变量那样假设构造函数已被调用。应如官方示例所示，由单个线程（如 `threadIdx.x == 0`）显式调用 `store` 进行初始化，并在之后调用 `__syncthreads()` 确保所有线程看到初始化结果。
