## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/advanced-kernel-programming.html>

## 3.2. Advanced Kernel Programming 核心概念总结

### 1. 核心概念

#### 1.1 PTX 内联汇编（Using PTX）
PTX（Parallel Thread Execution）是 CUDA 的虚拟 ISA。直接使用 PTX 属于极致优化手段，仅在性能极度敏感的场景下作为最后手段使用。CUDA 提供了两种使用方式：
- `cuda::ptx` 命名空间：将 PTX 指令封装为 C++ 函数，便于在 libcu++ 环境中调用。
- Inline PTX：通过内联汇编直接嵌入 PTX 指令。

#### 1.2 GPU 硬件执行模型（Hardware Implementation）

**独立线程调度（Independent Thread Scheduling，CC 7.0+）**
- Volta 及更新的架构为每个线程维护独立的执行上下文（PC、调用栈），支持在线程粒度上让出执行资源。
- 这使得线程可以在 sub-warp 粒度上发散和重新收敛，大大提升了灵活性。
- **破坏性变更**：旧代码中依赖隐式 warp 同步（warp-synchronous）的假设不再成立。任何同步无关的 warp 内归约等操作必须显式使用 `__syncwarp()` 保证跨代兼容性。

**异步执行特性（Asynchronous Execution Features）**
- **CC 8.0+（Ampere）**：引入硬件加速的异步数据拷贝（`LDGSTS`）和异步 barrier。
- **CC 9.0+（Hopper）**：扩展为 Tensor Memory Accelerator（TMA）、异步 transaction barrier、异步矩阵乘累加。
- 异步操作由 CUDA 线程发起，但由独立的 **async thread** 执行，通过 barrier 或 pipeline 等同步对象通知完成。
- **Generic Proxy vs Async Proxy**：
  - Generic proxy 中的异步操作（如 `LDGSTS`）与发起前的常规读写保持顺序，但与发起后的常规读写不保证顺序。
  - Async proxy 中的操作（如 TMA）与常规读写均不保证顺序，必须通过 **proxy fence** 进行同步。

#### 1.3 线程作用域（Thread Scopes）
CUDA 引入线程作用域概念，定义内存操作的可见范围和同步范围：

// FIXME
| CUDA C++ Thread Scope | 可见范围 | 一致性点 |
|---|---|---|
| `thread_scope_thread` | 仅本地线程 | – |
| `thread_scope_block` | 同一线程块 | L1 |
| `thread_scope_cluster` | 同一线程块 cluster | L2 |
| `thread_scope_device` | 同一 GPU 设备 | L2 |
| `thread_scope_system` | 整个系统（CPU + 多 GPU）| L2 + 连接缓存 |

使用最窄的作用域可以获得最佳性能。

#### 1.4 高级同步原语（Advanced Synchronization Primitives）

// FIXME
**Scoped Atomics**
- 将 C++ 标准内存序（`memory_order_relaxed` / `acquire` / `release` / `seq_cst`）与 CUDA 线程作用域结合。
- 通过 `cuda::atomic<T, Scope>` 或编译器内建函数（`__nv_atomic_*`）使用。
- **Acquire-Release 语义**：生产者通过 `release` store 写入数据并设置标志，消费者通过 `acquire` load 读取标志，确保数据可见性。

**Asynchronous Barriers**
- 将同步拆分为 **arrive**（到达，非阻塞）和 **wait**（等待，阻塞）两个阶段。
- 线程在 arrive 之后可以执行与同步无关的其他计算，从而隐藏等待延迟。
- CC 8.0+ 对 block scope 的异步 barrier 提供硬件加速；CC 9.0+ 扩展至 cluster scope。
- 可用 API：`cuda::barrier`、`cuda::ptx::mbarrier_*`、primitives API（`__mbarrier_*`）。

**Pipelines**
- 用于编排多阶段异步内存拷贝，支持双缓冲/多缓冲的生产者-消费者模式。
- libcu++ API：`cuda::pipeline`（`producer_acquire`、`producer_commit`、`consumer_wait`、`consumer_release`）。
- Primitives API：`__cp_async`、`__pipeline_*` 系列函数。

#### 1.5 异步数据拷贝（Asynchronous Data Copies）
- 将数据从 global memory 异步搬运到 shared memory（或反向），使线程在等待数据期间可以执行其他计算。
- 常用 API：`cooperative_groups::memcpy_async` / `wait`、`cuda::pipeline`、PTX `LDGSTS`、TMA。
- 经典模式：拷贝 -> 计算 -> 回写，通过异步操作将拷贝与计算重叠。

---

### 2. 关键 API / 函数 / 宏

#### PTX 相关
- `cuda::ptx::*`：libcu++ 中映射到 PTX 指令的 C++ 函数。
- Inline PTX：通过 `asm volatile (...)` 直接嵌入。

#### Warp 同步
- `__syncwarp(mask)`：显式同步 warp 内线程，替代隐式 warp 同步；CC 7.0+ 代码兼容必备。

#### Scoped Atomics
- `cuda::atomic<T, cuda::thread_scope_*> counter;`
- `counter.store(val, cuda::memory_order_release);`
- `counter.load(cuda::memory_order_acquire);`
- `counter.fetch_add(delta, cuda::memory_order_relaxed);`
- `__nv_atomic_store_n(ptr, val, mem_order, scope);`
- `__nv_atomic_load_n(ptr, mem_order, scope);`
- `__nv_atomic_fetch_add(ptr, delta, mem_order, scope);`

#### 异步 Barriers
- `cuda::barrier<cuda::thread_scope_block> bar;`
- `cuda::barrier<...>::arrival_token token = bar.arrive();`
- `bar.wait(std::move(token));`
- `cuda::ptx::mbarrier_init(&bar, count);`
- `cuda::ptx::mbarrier_arrive(&bar);`
- `cuda::ptx::mbarrier_try_wait(&bar, token);`
- `__mbarrier_init(&bar, count);`
- `__mbarrier_arrive(&bar);`
- `__mbarrier_try_wait(&bar, token, timeout);`

#### Pipeline / 异步拷贝
- `cooperative_groups::memcpy_async(group, dst, src, size);`
- `cooperative_groups::wait(group);`
- `cuda::pipeline` 的 `producer_acquire`、`producer_commit`、`consumer_wait`、`consumer_release`。
- `__cp_async(...)`：原始 API，发起 global -> shared 的异步拷贝。
- `__pipeline_producer_acquire()` / `__pipeline_producer_commit()` / `__pipeline_consumer_wait(N)` / `__pipeline_consumer_release()`。

#### 线程作用域枚举
- `cuda::thread_scope_thread`
- `cuda::thread_scope_block`
- `cuda::thread_scope_cluster`
- `cuda::thread_scope_device`
- `cuda::thread_scope_system`

#### 内存序枚举
- `cuda::memory_order_relaxed`
- `cuda::memory_order_acquire`
- `cuda::memory_order_release`
- `cuda::memory_order_acq_rel`
- `cuda::memory_order_seq_cst`

---

### 3. 注意事项、限制条件与常见陷阱

#### 独立线程调度（CC 7.0+）
- 旧的 warp-synchronous 代码（如 `shfl` 归约不加 `__syncwarp`）在 Volta 及更新架构上可能产生死锁或错误结果。
- **迁移建议**：所有依赖 warp 内同步的代码必须显式调用 `__syncwarp()`。

#### 异步操作与内存顺序
- async thread 在 generic proxy 中：发起异步操作**之后**的常规读写可能与其发生竞争，必须等待异步操作完成后再访问相同地址。
- async thread 在 async proxy 中：发起前后的常规读写均不保证顺序，必须使用 **proxy fence**。

#### 硬件资源限制
- 如果 kernel 的寄存器 + 共享内存需求导致 SM 无法容纳至少一个 block，kernel 启动**直接失败**。
- 具体限制（最大驻留 block/warp 数、寄存器/共享内存总量）取决于 compute capability，详见 Compute Capabilities 章节。

#### 异步特性版本要求
- `cuda::barrier` / `mbarrier` 相关功能：**CC 7.0+** 可用，**CC 8.0+** 硬件加速。
- `cooperative_groups::memcpy_async` / `LDGSTS`：**CC 8.0+**。
- TMA / STAS / cluster scope barrier：**CC 9.0+**。
- 在旧架构上尝试使用这些 API 会导致编译错误或运行时异常，必须根据目标架构进行条件编译或运行时检测。


### 4. 硬件行为底层原理

#### LDGSTS 与异步拷贝管线
- `LDGSTS`（Load Global Store Shared）是 Ampere 引入的专用硬件指令。
- 它绕过通用寄存器文件，直接将数据从 global memory 搬运到 shared memory，减少寄存器压力和指令数。
- 该操作由独立的硬件单元异步执行，线程通过 barrier/pipeline 等待完成通知。

#### TMA（Tensor Memory Accelerator）
- Hopper 架构引入的专用 DMA 引擎，位于 SM 内部。
- 支持多维张量的 bulk-asynchronous 拷贝，由单个线程发起即可搬运大块数据。
- TMA 操作在 async proxy 中执行，因此需要通过 proxy fence 与普通 load/store 保持顺序。

#### Proxy 与内存一致性
- Generic proxy：常规 load/store 和一部分异步指令（如 `LDGSTS`、`STAS`）共用同一地址空间映射，但异步指令由不同线程上下文执行。
- Async proxy：TMA 和部分 Tensor Core 操作使用独立的内存访问代理。Proxy fence 的作用是在两个代理之间建立 happens-before 关系，保证数据可见性。
