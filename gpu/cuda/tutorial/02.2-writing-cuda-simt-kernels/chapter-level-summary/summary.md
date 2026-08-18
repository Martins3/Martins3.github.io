## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/writing-cuda-kernels.html>

# Writing CUDA SIMT Kernels 章节总结

## 一、章节主要内容转述

### 2.2.1 SIMT 基础

CUDA C++ 核函数的编写方式与传统 CPU 代码非常相似，但充分利用 GPU 的独特特性可以显著提升性能。SIMT（Single Instruction, Multiple Thread）模型中，CUDA 线程是并行的基本单元。每个线程维护自己的状态和控制流，从功能上看可以执行不同的代码路径。然而，当同一个 warp 中的线程发生控制流分歧（divergence）时，会严重影响性能。因此，编写高性能核函数的关键目标之一是尽量减少 warp 内的线程分歧。

### 2.2.2 线程层次结构

CUDA 线程按层次组织：线程（Thread）组成线程块（Thread Block），线程块组成网格（Grid）。网格和线程块都可以是 1D、2D 或 3D 的。内核中可以通过以下内置变量查询执行配置和自身位置：

- `gridDim.[x|y|z]`：网格在各维度上的大小。
- `blockDim.[x|y|z]`：线程块在各维度上的大小。
- `blockIdx.[x|y|z]`：当前线程块在网格中的索引。
- `threadIdx.[x|y|z]`：当前线程在线程块中的索引。

多维索引纯粹是为了编程方便，不影响硬件性能。线程在线程块内的线性化顺序为：`x` 维度变化最快，`y` 次之，`z` 最慢。这意味着 `threadIdx.x` 连续的线程在硬件上是相邻的，`threadIdx.y` 的步长为 `blockDim.x`，`threadIdx.z` 的步长为 `blockDim.x * blockDim.y`。这种线性化方式直接影响 warp 的构成和内存访问模式。

### 2.2.3 GPU 设备内存空间

CUDA 设备拥有多种内存空间，它们的范围、生命周期和物理位置各不相同，如表所示：

| 内存类型 | 作用域 | 生命周期 | 物理位置 |
|---------|-------|---------|---------|
| Global | Grid | Application | Device (DRAM) |
| Constant | Grid | Application | Device (只读) |
| Shared | Block | Kernel | SM (片上) |
| Local | Thread | Kernel | Device (DRAM) |
| Register | Thread | Kernel | SM (片上) |

#### Global Memory（全局内存）

全局内存是所有线程都可访问的主存储空间，类似于 CPU 的系统内存。它通过 `cudaMalloc`、`cudaMallocManaged` 分配，通过 `cudaMemcpy` 与主机交换数据，通过 `cudaFree` 释放。由于所有线程都可以读写全局内存，必须注意避免数据竞争（data races）。全局内存的持久性跨越多个核函数调用，直到显式释放或应用程序结束。

#### Shared Memory（共享内存）

共享内存位于 SM 上，与 L1 缓存共享同一块物理存储，带宽高、延迟低。它是一块用户可管理的暂存存储器（scratchpad），仅供同一线程块内的所有线程访问。因为多线程可能同时读写共享内存，需要使用 `__syncthreads()` 进行块内同步，以避免数据竞争。

共享内存大小取决于 GPU 架构，可以通过 `cudaDeviceProp` 的 `sharedMemPerMultiprocessor` 和 `sharedMemPerBlock` 查询。使用 `cudaFuncSetCacheConfig` 可以提示运行时优先分配更多空间给共享内存或 L1 缓存，但不保证一定生效。

- **静态分配**：在核函数内使用 `__shared__ float arr[1024];`，大小必须在编译时确定。
- **动态分配**：在启动配置中指定第三个参数 `sharedMemoryBytes`，核函数内使用 `extern __shared__ float arr[];`。如果需要多个动态数组，必须通过指针算术手动划分同一块动态共享内存，并注意内存对齐。

#### Registers（寄存器）

寄存器位于 SM 上，具有线程局部作用域，由编译器自动管理。可以通过 `nvcc` 的 `-maxrregcount` 选项限制寄存器使用量，但这可能导致更多的寄存器溢出（spilling）到 local memory，从而降低性能。可以通过 `cudaDeviceProp` 查询 `regsPerMultiprocessor` 和 `regsPerBlock`。

#### Local Memory（局部内存）

局部内存逻辑上是线程私有的，但物理上位于全局内存空间中。编译器在以下情况下可能将变量放入局部内存：
- 无法确定数组索引是否为常量的数组；
- 太大而无法放入寄存器的结构体或数组；
- 寄存器溢出时的变量。

局部内存的延迟和带宽与全局内存相同，但其布局经过优化：连续的线程 ID 访问连续的 32-bit 字。因此，只要 warp 内所有线程访问相同的相对地址（如同一数组索引），局部内存访问就是完全合并的（coalesced）。

#### Constant Memory（常量内存）

常量内存位于设备上，对核函数只读，必须在主机端使用 `__constant__` 修饰符声明和初始化。典型大小为 64KB。它适用于所有线程只读访问的小量数据。相关 API 包括 `cudaMemcpyToSymbol`、`cudaMemcpyFromSymbol`、`cudaGetSymbolAddress` 和 `cudaGetSymbolSize`。

#### Caches（缓存）

GPU 具有 L1 和 L2 两级缓存。L2 缓存位于设备 DRAM 附近，被所有 SM 共享。L1 缓存与共享内存共享 SM 上的物理空间。如果核函数不使用共享内存，整块物理空间将用作 L1 缓存。开发者可以通过 API 控制缓存行为，但如果不使用这些提示，编译器和运行时会自动尽量高效地利用缓存。

#### Texture and Surface Memory（纹理和表面内存）

在旧款 GPU 上，纹理内存可能在某些场景下带来性能优势。但在当前所有受支持的 NVIDIA GPU 上，直接使用全局内存加载/存储指令已经能够高效处理这些场景，纹理和表面内存不再提供任何性能优势。新开发的 CUDA 程序可以忽略这些 API。

#### Distributed Shared Memory（分布式共享内存）

从计算能力 9.0 开始，Thread Block Cluster 允许同一个 cluster 内的线程块访问彼此的共享内存，称为分布式共享内存。其总大小等于 cluster 内的线程块数乘以每块的共享内存大小。访问前需要通过 `cluster.sync()` 确保所有线程块都已启动。此特性需要 `cooperative_groups` 支持，旧架构不支持。

### 2.2.4 内存性能

#### 2.2.4.1 合并全局内存访问（Coalesced Global Memory Access）

全局内存通过 32 字节的事务进行访问。当一个 warp 中的线程请求全局内存数据时，硬件会将这些请求合并成尽可能少的 32 字节事务。最高效的情况是：warp 中的连续线程访问内存中的连续 4 字节元素（例如 `float`），这样 128 字节的数据可以通过 4 个 32 字节事务完成，利用率 100%。

相反，如果连续线程访问的元素在内存中相隔 32 字节或更多，每个线程都会触发一个独立的 32 字节事务，总流量为 1024 字节，但实际只使用 128 字节，利用率仅 12.5%。

**实现合并访问的最直接方法**：让连续线程访问连续内存元素。只要 warp 内所有线程访问的地址落在相同的 32 字节段内（即使顺序有排列），合并访问仍然可以发生。核心目标是最大化"使用字节数 / 传输字节数"的比例。

**矩阵转置示例**：
- 朴素实现中，对输入矩阵 `a` 的读取是合并的（`threadIdx.x` 对应列索引，连续线程读连续地址），但对输出矩阵 `c` 的写入不是合并的（`threadIdx.x` 变成了行索引，连续线程写相距 `ld` 的地址）。
- 优化方法是使用共享内存作为中转：先将一个 tile 合并读入共享内存，同步后再合并写入全局内存的转置位置。

#### 2.2.4.2 共享内存访问模式与 Bank Conflicts

共享内存被划分为 32 个 bank，连续的 32-bit 字映射到连续的 bank。当同一个 warp 中的多个线程访问同一个 bank 中的不同元素时，会发生 bank conflict，导致访问串行化，降低性能。

两个例外情况不造成 conflict：
1. 多个线程访问同一个地址：读操作会广播（broadcast），写操作由其中一个线程执行（未定义具体是哪一个）。

**避免 bank conflicts 的常见技巧**：
-  stride 为 1 的访问（连续线程访问连续 32-bit 字）无 conflict。
-  stride 为 2 会造成 2-way conflict。
-  stride 为奇数通常无 conflict。

**矩阵转置中的 bank conflict 处理**：
- 如果声明 `__shared__ float smem[32][32];`，当 warp 按列访问时（第一维变化），连续线程的地址相隔 32 个 float，即 128 字节，正好落入同一个 bank，造成 32-way bank conflict。
- 解决方法是在数组的列维度上加 1：`__shared__ float smem[32][33];`。这样同一列中相邻行的元素会落入不同的 bank，彻底消除 bank conflicts。

### 2.2.5 原子操作（Atomics）

当线程之间无法完全独立时，需要使用原子操作来保证对全局内存（或共享内存）的同步读写。CUDA 提供 `cuda::std::atomic`、`cuda::std::atomic_ref` 以及扩展的 `cuda::atomic` 和 `cuda::atomic_ref`，允许指定线程作用域（如 `cuda::thread_scope_device`）。

原子操作会锁定内存位置，执行读-改-写，期间其他线程无法访问该位置。由于原子操作引入了隐式同步，应尽量减少使用，以避免性能瓶颈。一个典型应用是使用 `atomicAdd` 进行归约求和。

### 2.2.6 协作组（Cooperative Groups）

Cooperative Groups 允许开发者定义跨越线程块、网格甚至多 GPU 的线程组，并在这些组内进行同步。CUDA 编程模型本身只提供了线程块（或 cluster）级别的同步，而 Cooperative Groups 通过软件方式填补了更小粒度（如 warp、tile）和更大粒度（如 grid）的同步需求。跨线程块的同步有语义限制和性能影响，需要谨慎使用。

### 2.2.7 核函数启动与占用率（Occupancy）

核函数启动后，线程块由调度器分配给 SM。开发者无法控制具体哪个线程块运行在哪个 SM 上，也不能依赖任何调度顺序。每个 SM 能同时驻留的线程块数量取决于资源限制：

- `maxBlocksPerMultiProcessor`：每个 SM 最大驻留线程块数。
- `sharedMemPerMultiprocessor`：每个 SM 的共享内存总量。
- `regsPerMultiprocessor`：每个 SM 的寄存器总量。
- `maxThreadsPerMultiProcessor`：每个 SM 最大驻留线程数。
- `sharedMemPerBlock`：每个线程块最大共享内存。
- `regsPerBlock`：每个线程块最大寄存器数。
- `maxThreadsPerBlock`：每个线程块最大线程数。

**占用率（Occupancy）**定义为：活跃 warp 数 / SM 支持的最大活跃 warp 数。高占用率有助于隐藏延迟，提高性能。占用率受线程块大小、共享内存用量和寄存器用量共同影响。例如，如果线程块有 768 个线程，而 SM 最多支持 2048 个线程，则每个 SM 最多驻留 2 个线程块，占用率为 (768*2)/2048 = 75%。

编译时可以使用 `nvcc --resource-usage` 查看每个核函数的寄存器和共享内存用量，从而辅助占用率分析。

---

## 二、核心概念速查

### 核心概念

- **SIMT（Single Instruction, Multiple Thread）**：GPU 执行的基本模型。warp 内线程应尽量执行相同控制流路径，避免 divergence。
- **线程线性化**：线程块内 `x` 变化最快，`y` 次之，`z` 最慢。连续 `threadIdx.x` 的线程在硬件上相邻。
- **合并访问（Coalescing）**：warp 内线程的内存请求被合并为少量事务。连续线程访问连续地址是最佳实践。
- **共享内存 Bank Conflict**：同一 warp 内多个线程访问同一 bank 的不同地址会导致串行化。可通过 padding 避免。
- **Occupancy**：SM 上活跃 warp 的比例。高占用率有助于隐藏内存和指令延迟。

### 关键 API / 函数 / 修饰符

| 名称 | 作用 |
|------|------|
| `__global__` | 声明从主机调用、在设备上执行的核函数。 |
| `__shared__` | 声明静态共享内存变量。 |
| `extern __shared__` | 声明动态共享内存变量。 |
| `__syncthreads()` | 线程块内同步，确保所有线程到达该点后才继续执行。 |
| `__constant__` | 声明常量内存变量（主机端初始化，设备端只读）。 |
| `cudaMalloc` / `cudaFree` | 分配 / 释放全局内存。 |
| `cudaMemcpy` / `cudaMemcpyToSymbol` | 主机与设备间数据传输。 |
| `cudaGetDeviceProperties` | 查询设备属性（如共享内存大小、寄存器数量、计算能力等）。 |
| `cudaFuncSetCacheConfig` | 提示运行时共享内存与 L1 缓存的分配偏好。 |
| `atomicAdd` / `cuda::atomic_ref` | 原子操作，保证对内存位置的互斥读-改-写。 |
| `cooperative_groups` | 提供跨线程块甚至跨网格的线程分组和同步能力。 |

### 注意事项与常见陷阱

1. **共享内存同步不可或缺**：在写入共享内存后再由其他线程读取前，必须调用 `__syncthreads()`，否则会产生数据竞争或未定义行为。
2. **动态共享内存的对齐问题**：通过 `extern __shared__` 手动划分多个数组时，必须确保每个指针按其所指类型正确对齐。否则会导致未定义行为或性能下降。
3. **Bank Conflict 的隐蔽性**：二维共享内存 `smem[32][32]` 按列访问时，由于 C++ 行优先存储，连续线程地址间隔 32 * 4 = 128 字节，正好落在同一 bank，产生 32-way conflict。通常通过 `smem[32][33]` 的 padding 解决。
4. **局部内存并非在片上**：Local memory 物理上在全局内存空间中，访问延迟与全局内存相同。过度使用局部内存（如大数组、寄存器溢出）会严重降低性能。
5. **原子操作的性能代价**：原子操作会序列化对同一内存地址的访问，应尽量减少 contention。在可能的情况下，先在共享内存中进行局部归约/统计，再一次性用原子操作写回全局内存。
6. **Occupancy 不是唯一指标**：追求 100% occupancy 并不总是意味着最高性能。有时降低 occupancy 以换取更高的指令级并行（ILP）或更好的缓存利用率反而更优。
7. **纹理内存已过时**：在当前支持的 GPU 上，纹理和表面内存不再提供性能优势，新项目应直接使用全局内存加载/存储。
8. **分布式共享内存的架构限制**：Distributed Shared Memory 和 Thread Block Cluster 需要计算能力 9.0+，运行前应检测设备属性。
