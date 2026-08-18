## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/writing-cuda-kernels.html>

## SIMT 与线程层次

- CUDA 线程是并行的基本单位。GPU 以 SIMT（单指令多线程）方式执行，32 个线程组成一个 warp。
- 线程被组织成 1D/2D/3D 的 thread block，block 再组成 1D/2D/3D 的 grid。
- 内核中可通过以下内置变量定位线程：
  - `gridDim.x/y/z`：grid 在各维度的大小。
  - `blockDim.x/y/z`：block 在各维度的大小。
  - `blockIdx.x/y/z`：当前 block 在 grid 中的索引。
  - `threadIdx.x/y/z`：当前线程在 block 中的索引。
- 线程线性化规则：`x` 变化最快，`y` 次之，`z` 最慢。连续 `threadIdx.x` 对应连续线程号。

## GPU 内存空间

| 内存类型 | 作用域 | 生命周期 | 物理位置 | 说明 |
|---|---|---|---|---|
| Global | Grid | Application | Device | 主内存，所有线程可访问，通过 `cudaMalloc`/`cudaMemcpy`/`cudaFree` 管理。 |
| Constant | Grid | Application | Device | 只读，通过 `__constant__` 声明，`cudaMemcpyToSymbol` 初始化，典型大小 64KB。 |
| Shared | Block | Kernel | SM | 位于 SM 上，速度极快，用于 block 内线程通信与数据重用。 |
| Register / Local | Thread | Kernel | SM / Device | 寄存器由编译器自动管理；Local memory 逻辑上线程私有，物理上在全局内存中（spill 时使用）。 |

### Global Memory

- 内核间唯一的数据交换方式（因内核返回类型为 `void`）。
- 需注意避免数据竞争（data races）。

### Constant Memory

- 适合所有线程只读访问的小数据量场景。
- API：`cudaMemcpyToSymbol`、`cudaGetSymbolAddress`、`cudaGetSymbolSize`。

### Shared Memory

- 与 L1 缓存共享同一块物理存储。若内核不使用 shared memory，则该空间全部用作 L1 缓存。
- 可用于解决全局内存非合并写（如矩阵转置）的问题。

## 内存性能：合并访问 (Coalesced Access)

- 全局内存以 32 字节事务为单位访问。
- **最佳实践**：同一个 warp 中的线程访问连续的 32 字节段内的数据，使得每个 32 字节事务都被充分利用。
- 连续线程访问连续 4 字节元素时，128 字节请求会被合并为 4 个 32 字节事务，利用率 100%。
- 若连续线程访问的数据间隔 >= 32 字节，则每个线程触发独立事务，利用率仅 12.5%。
- **矩阵转置示例**：
  - Naive 实现：读 `a` 合并，写 `c` 不合并（行/列索引互换导致 stride 为 leading dimension）。
  - 优化方案：使用 Shared Memory 做 tile 缓冲，先合并读入 tile，再同步后合并写出，实现读写双向合并。

## Atomics

- 当线程间不能完全独立时，可使用原子操作对全局内存进行同步的读-改-写。
- CUDA 提供 `cuda::std::atomic` / `cuda::std::atomic_ref`（与 C++ 标准库语义一致），以及扩展的 `cuda::atomic` / `cuda::atomic_ref`（可指定线程作用域，如 `cuda::thread_scope_device`）。
- 示例作用域：
  - `cuda::thread_scope_device`：跨整个 GPU 的所有线程。
- 原子操作会引入同步开销，应尽量避免频繁使用。

## Cooperative Groups

- 允许定义跨 block、跨 grid 甚至跨 GPU 的线程组并进行同步。
- 提供了比 CUDA 基本编程模型更灵活的线程分组与同步机制，但跨越 block/cluster 边界时存在语义限制与性能代价。

## Kernel Launch 与 Occupancy

- 内核通过 `<<<grid, block>>>` 配置启动，scheduler 将 block 分配到 SM 上执行，具体分配顺序不可控。
- Occupancy（占用率）= 当前活跃 warp 数 / SM 支持的最大活跃 warp 数。高占用率有助于隐藏延迟。
- 限制 SM 上并发 block 数量的资源包括：
  - `maxBlocksPerMultiProcessor`
  - `maxThreadsPerMultiProcessor`
  - `sharedMemPerMultiprocessor` / `sharedMemPerBlock`
  - `regsPerMultiprocessor` / `regsPerBlock`
- 可使用 `--resource-usage` 编译选项查看内核的寄存器与共享内存用量，进而估算占用率。

## 关键 API 列表

| API / 宏 | 用途 |
|---|---|
| `cudaMalloc` / `cudaFree` | 分配 / 释放全局内存 |
| `cudaMemcpy` / `cudaMemcpyToSymbol` | H2D/D2H 数据传输 / 初始化常量内存 |
| `cudaGetDeviceProperties` | 查询设备属性（SM 数量、缓存大小、寄存器数等） |
| `__syncthreads()` | block 内线程同步 |
| `cuda::atomic_ref<T, Scope>` | 设备端原子引用 |
| `threadIdx` / `blockIdx` / `blockDim` / `gridDim` | 内核内置索引变量 |

## 注意事项

1. **合并访问至关重要**：编写高性能 CUDA 内核时，确保全局内存读写合并是最重要的优化之一。
2. **避免 warp 内分支发散**：虽然 SIMT 允许每个线程独立控制流，但同 warp 内线程走不同分支会导致串行执行，降低效率。
3. **合理使用 Shared Memory**：它既能加速数据重用，也能将非合并的全局内存访问转换为合并访问（如 tile 算法）。
4. **原子操作慎用**：大量原子竞争会严重拉低并行度，应优先考虑规约（reduction）等算法替代。
5. **Occupancy 不是唯一指标**：虽然高占用率有助于隐藏延迟，但过度减少寄存器导致 spilling 到 local memory 可能反而降低性能。
