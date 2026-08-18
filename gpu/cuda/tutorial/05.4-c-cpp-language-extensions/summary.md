## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cpp-language-extensions.html>


本章系统性地介绍了 NVIDIA CUDA 对标准 C/C++ 语言的核心扩展。这些扩展并非简单的语法糖，而是 CUDA 编程模型的基石——它们让开发者能够在同一份源文件中区分 CPU（Host）与 GPU（Device）代码路径，精确控制变量在复杂内存层次结构中的存放位置，配置大规模线程阵列的启动方式，以及利用底层同步、原子和 Warp 级原语实现高性能并行算法。本章内容跨度极大，从最基本的 `__global__` 和 `__shared__` 注解，到 Warp Matrix Functions（WMMA）对 Tensor Core 的封装，再到编译器优化提示与调试诊断工具，几乎覆盖了除标准库之外的全部 CUDA C++ 语言层面知识。

## 背景与要解决的问题

标准 C/C++ 语言的设计前提是在单一、共享的内存地址空间上顺序或粗粒度并发执行。GPU 的计算范式则截然不同：它拥有独立的设备内存（Device Memory）、只读常量内存（Constant Memory）、位于 SM（Streaming Multiprocessor）上的低延迟共享内存（Shared Memory），以及数以万计需要精细协作的轻量级线程。如果没有语言扩展，编译器无法区分一段函数应当编译为 x86/ARM 机器码还是 PTX/SASS 指令，也无法知道一个指针指向的是全局内存还是共享内存，更无法表达 "256 个线程组成一个 Block、若干个 Block 组成一个 Grid" 这样的执行模型。

CUDA 的语言扩展正是为了解决这一根本性的语义鸿沟：

1. **执行空间区分**：让编译器知道函数运行在 Host 还是 Device，从而生成正确的指令序列和调用约定。
2. **内存空间注解**：让编译器将变量分配到物理上不同的存储介质，并生成对应的寻址和缓存指令（如 `ld.global` vs `ld.shared`）。
3. **Kernel 启动语法**：`<<<...>>>` 执行配置是 CUDA 对 C++ 函数调用语法的核心扩展，它将并行度参数（Grid/Block 维度、动态共享内存、流）直接嵌入调用点。
4. **底层同步与通信**：GPU 线程层次（Grid - Block - Warp - Thread）要求提供多级同步原语（Block 级、Warp 级、内存栅栏），标准 C++ 没有对应概念。
5. **数据并行原语**：Warp 内 32 个线程需要高效的洗牌（Shuffle）、投票（Vote）、匹配（Match）、归约（Reduce）操作，这些无法通过标准 C++ 表达。

## 核心概念与术语

- **执行空间（Execution Space）**：函数运行的物理位置，分为 Host（CPU）、Device（GPU）以及同时编译两份代码的 `__host__ __device__`。
- **内存空间（Memory Space）**：变量的物理存储位置，包括全局内存（Global）、常量内存（Constant）、共享内存（Shared）、本地内存（Local）以及统一内存（Managed）。
- **执行配置（Execution Configuration）**：Kernel 启动运算符 `<<<gridDim, blockDim, dynamicSmemBytes, stream>>>` 中的参数集合。
- **Warp**：一个由 32 个线程（通常）组成的 SIMD 执行单元，是 GPU 调度和执行的基本向量宽度。
- **Memory Fence（内存栅栏）**：一种只保证内存操作顺序而不保证可见性的同步机制，分为 Block、Device、System 三级作用域。
- **Occupancy（占用率）**：每个 SM 上实际驻留的 Warp 数量与理论最大值的比率，受寄存器用量、共享内存用量和 Block 大小限制。
- **Tensor Core**：NVIDIA GPU 上的专用矩阵乘加单元，WMMA（Warp Matrix Multiply Accumulate）API 允许 Warp 级协作调用 Tensor Core。

## API / 机制详解

### 1. 函数与变量注解（Function and Variable Annotations）

#### 1.1 执行空间限定符（Execution Space Specifiers）

CUDA 通过 `__host__`、`__device__`、`__global__` 和 `__host__ __device__` 四个限定符明确函数的执行位置与可调用范围：

- **`__host__`（或无注解）**：仅在 Host 上执行，只能被 Host 代码调用。这是普通 C++ 函数的默认行为。
- **`__device__`**：仅在 Device 上执行，只能被 Device 代码调用（包括从 `__global__` 函数或其他 `__device__` 函数调用）。
- **`__global__`**：在 Device 上执行，但只能从 Host 代码（或某些 CUDA 运行时 API）中启动。它必须返回 `void`，不能是类/结构体/联合体的成员函数，不支持递归，且调用是异步的——Host 线程在提交启动命令后立即返回，不会等待 GPU 执行完毕。
- **`__host__ __device__`**：函数会被编译两次，分别生成 Host 版本和 Device 版本。开发者可以利用 `__CUDA_ARCH__` 宏在函数体内区分两条代码路径：
  ```cpp
  __host__ __device__ void func() {
  #if defined(__CUDA_ARCH__)
      // Device code path
  #else
      // Host code path
  #endif
  }
  ```

`__global__` 函数的约束尤为严格：除必须返回 `void` 外，它还要求调用时必须提供执行配置（`<<<...>>>`），且其参数存在额外限制（例如不可为引用类型）。

#### 1.2 内存空间限定符（Memory Space Specifiers）

`__device__`、`__managed__`、`__constant__` 和 `__shared__` 用于指示变量的存储位置，决定了谁可以访问它、生命周期有多长、以及每个执行实体看到的是否是同一份实例。

| 限定符 | 位置 | 可访问者 | 生命周期 | 实例唯一性 |
|--------|------|----------|----------|------------|
| `__device__` | 设备全局内存 | Device 线程 / CUDA Runtime API | 程序/CUDA 上下文 | 每设备一份 |
| `__constant__` | 设备常量内存 | Device 线程 / CUDA Runtime API（只读） | 程序/CUDA 上下文 | 每设备一份 |
| `__managed__` | Host 与 Device（自动迁移） | Host/Device 线程 | 程序 | 每程序一份 |
| `__shared__` | SM 上的共享内存 | Block 内线程 | Block | 每 Block 一份 |
| 无限定符 | 寄存器（或溢出到本地内存） | 单个线程 | 单个线程 | 每线程一份 |

关键细节：
- `__device__` 和 `__constant__` 变量可以通过 Host 端的 `cudaGetSymbolAddress()`、`cudaGetSymbolSize()`、`cudaMemcpyToSymbol()`、`cudaMemcpyFromSymbol()` 访问。
- `__constant__` 变量在 Device 代码中只读，修改只能通过 Host 端的 CUDA Runtime API 完成。
- **`__shared__` 内存**：可以静态分配（编译期确定大小，如 `__shared__ int smem[256]`），也可以动态分配（运行时通过 Kernel 配置指定，如 `extern __shared__ char dynamic_smem[]`）。静态大小的 `__shared__` 变量不能在声明时初始化。
- **`__managed__` 内存**：地址不是常量表达式，因此不能用于模板非类型参数；不能声明为引用类型；不能在静态/动态初始化或析构函数中使用（因为此时 CUDA Runtime 可能未初始化或已失效）；不能作为未加括号的 `decltype()` 参数。

#### 1.3 内联限定符（Inlining Specifiers）

`__noinline__`、`__forceinline__` 和 `__inline_hint__` 用于控制 `__host__` 和 `__device__` 函数的内联行为。三者互斥：`__noinline__` 禁止内联；`__forceinline__` 强制在单个翻译单元内内联；`__inline_hint__` 在使用链接时优化（LTO）时允许跨翻译单元的激进内联。

#### 1.4 `__restrict__` 指针

`__restrict__` 向编译器承诺：在指针的生命周期内，其指向的内存只会通过该指针访问（不存在指针别名 Alias）。这使得编译器可以安全地将内存加载缓存到寄存器、进行指令重排和公共子表达式消除。例如，在未加 `__restrict__` 时，编译器无法确定写入 `c[0]` 是否会修改 `a[0]`，因此每次乘法都必须从内存重新加载；加上 `__restrict__` 后，`a[0]` 和 `b[0]` 可以只加载一次并存入寄存器复用。

特别地，当 `__global__` 函数的 `const` 指针参数带有 `__restrict__` 时，访存会被编译为只读缓存加载（PTX `ld.global.nc`），类似于显式调用 `__ldg()`。

代价是寄存器压力上升。如果寄存器用量导致 Occupancy 下降，整体性能反而可能降低。

#### 1.5 `__grid_constant__` 参数

为 `__global__` 函数的参数添加 `__grid_constant__` 后，编译器不会为每个线程创建该参数的私有副本，而是让整个 Grid 的所有线程通过单一地址访问该参数。其效果类似于将参数放入只读缓存，能减少寄存器消耗并提升性能。

要求：参数类型必须是 `const` 限定的非引用类型；该参数在整个 Kernel 生命周期内只读；所有函数声明（包括模板特化/实例化）必须保持一致。

### 2. 内置类型与变量（Built-in Types and Variables）

#### 2.1 Host 编译器类型扩展

CUDA 允许使用 Host 编译器支持的非标准算术类型：`__int128`（Linux 上若定义了 `__SIZEOF_INT128__`）、`__float128` / `_Float128`（计算能力 10.0+）、`_Complex`（仅 Host 代码）。

#### 2.2 内置变量

这些变量仅在 Device 代码中可用：
- `gridDim`（`dim3` 类型）：Grid 在 x、y、z 维度的尺寸。
- `blockDim`（`dim3` 类型）：Block 在 x、y、z 维度的线程数。
- `blockIdx`（`uint3` 类型）：当前 Block 在 Grid 中的索引。
- `threadIdx`（`uint3` 类型）：当前线程在 Block 中的索引。
- `warpSize`（`int` 类型）：一个 Warp 中的线程数，通常为 32，是运行时值。

#### 2.3 向量类型

CUDA 提供从基本整型和浮点类型派生的向量类型，如 `int4`、`float4`、`char2` 等。它们是结构体，通过 `.x`、`.y`、`.z`、`.w` 访问分量，并通过 `make_int4()` 等工厂函数构造。每种类型有严格的尺寸和对齐要求（例如 `float4` 为 16 字节大小、16 字节对齐）。值得注意的是，`long4`、`ulong4`、`longlong4`、`ulonglong4` 和 `double4` 已在 CUDA 13 中标记为废弃，建议使用带显式对齐后缀的版本（如 `long4_16a`、`double4_32a`）。

### 3. Kernel 配置（Kernel Configuration）

#### 3.1 执行配置语法

调用 `__global__` 函数时，必须在函数名和参数列表之间插入 `<<<grid_dim, block_dim, dynamic_smem_bytes, stream>>>`。其中 `dynamic_smem_bytes` 和 `stream` 是可选的，默认值分别为 0 和 `NULL`。执行配置的参数在实际函数参数之前求值。如果 `grid_dim` 或 `block_dim` 超出设备限制，或动态共享内存超过剩余容量，启动将失败。

#### 3.2 Thread Block Cluster

计算能力 9.0+ 支持在编译时通过 `__cluster_dims__(x, y, z)` 属性指定 Thread Block Cluster 维度，也可以在运行时通过 `cudaLaunchKernelEx` API 配置。Cluster 允许 Block 之间进行更高效的协作和同步。若设备不支持 Cluster（如 sm_61），则无法使用此特性。

#### 3.3 Launch Bounds（`__launch_bounds__`）

`__launch_bounds__(maxThreadsPerBlock, minBlocksPerMultiprocessor, maxBlocksPerCluster)` 向编译器提供占用率提示：
- `maxThreadsPerBlock`：应用启动该 Kernel 时使用的最大线程数，编译为 `.maxntid` PTX 指令。
- `minBlocksPerMultiprocessor`：期望的每个 SM 最小驻留 Block 数，编译为 `.minnctapersm`。
- `maxBlocksPerCluster`：最大 Cluster Block 数，编译为 `.maxclusterrank`。

编译器会根据这些限制推导寄存器使用上限 L。如果初始寄存器用量超过 L，编译器会设法降低（可能以增加指令数和本地内存用量为代价）；如果低于 L，则可能增加寄存器使用来减少指令数。指定 `__launch_bounds__` 后，若实际启动配置超出限定值，Kernel 启动将失败。

#### 3.4 每线程最大寄存器数（`__maxnreg__`）

`__maxnreg__(maxNumberRegistersPerThread)` 直接限制单个线程可分配的寄存器数量，编译为 `.maxnreg` PTX 指令。它与 `__launch_bounds__` 不能同时用于同一个 Kernel。文件级的 `--maxrregcount` 编译选项对所有 `__global__` 函数生效，但会被 `__maxnreg__` 覆盖。

### 4. 同步原语（Synchronization Primitives）

#### 4.1 Block 级同步（`__syncthreads` 家族）

`__syncthreads()` 等待 Block 内所有未退出的线程到达同一点后才能继续。它提供参与线程间的内存序保证：Fence 之前的读写操作在逻辑上先于 Fence 之后的读写。变体包括：
- `__syncthreads_count(int predicate)`：返回 predicate 非零的线程数。
- `__syncthreads_and(int predicate)`：所有线程 predicate 非零时返回非零。
- `__syncthreads_or(int predicate)`：任一线程 predicate 非零时返回非零。

**致命陷阱**：`__syncthreads*()` 允许出现在条件代码中，但条件必须在整个 Block 内均匀求值（uniform condition）。如果某些线程进入 `if` 分支而另一些没有，调用 `__syncthreads()` 将导致未定义行为（通常是死锁）。

#### 4.2 Warp 级同步（`__syncwarp`）

`__syncwarp(unsigned mask = 0xFFFFFFFF)` 同步 Warp 内由 mask 指定的线程。它提供参与线程间的内存序，常用于避免 Warp 内共享内存或全局内存的读写冒险。

#### 4.3 内存栅栏（Memory Fence）

CUDA 采用弱序内存模型，读写顺序在多线程间不一定被直接观察到。内存栅栏只保证操作顺序，不保证立即可见性：
- **Block 级**：`__threadfence_block()` / `cuda::atomic_thread_fence(..., thread_scope_block)`。确保同 Block 内线程观察到调用者的先写先于后写、先读先于后读。
- **Device 级**：`__threadfence()` / `cuda::atomic_thread_fence(..., thread_scope_device)`。确保同设备内任何线程不会观察到调用者的后写先于先写。
- **System 级**：`__threadfence_system()` / `cuda::atomic_thread_fence(..., thread_scope_system)`。确保设备线程、Host 线程和 Peer 设备线程都观察到正确的顺序。

一个经典用例是多 Block 协作求和：某 Block 先将局部和写入全局内存，再执行 `__threadfence()`（或 Device 级 Fence），最后原子递增完成计数器。Fence 确保了局部和在计数器到达阈值之前确实已写入内存。

### 5. 原子函数（Atomic Functions）

CUDA 提供四种层次的原子操作接口：
1. **Extended CUDA C++ (`cuda::atomic`, `cuda::atomic_ref`)**：推荐。Host 和 Device 均可用，遵循 C++ 标准语义，可指定线程作用域。
2. **Standard C++ (`cuda::std::atomic`, `cuda::std::atomic_ref`)**：Host 和 Device 均可用，但不能显式指定线程作用域。
3. **Compiler Built-in (`__nv_atomic_*`)**：CUDA 12.8+，仅 Device 代码。遵循 C++ 内存序语义，可指定线程作用域。支持 4/8/16 字节类型（16 字节需 sm_90+）。`order` 和 `scope` 参数必须是整数字面量。
4. **Legacy Atomic (`atomicAdd`, `atomicSub` 等)**：仅 Device 代码。语义等效于 `memory_order_relaxed`，可通过后缀指定作用域（无后缀为 device，`_block` 为 block，`_system` 为 system）。

Legacy 原子函数支持的数据类型因操作而异：`atomicAdd` 支持 `int`、`unsigned`、`unsigned long long`、`float`、`double`、`__half`、`__half2` 等；`atomicCAS` 支持到 128 位（需 sm_90+），但要求类型对齐到 16 字节且 trivially copyable。向量类型（如 `__half2`、`float4`）的原子性是对每个分量分别保证的，而非整个向量作为一个原子访问。

### 6. Warp 函数（Warp Functions）
TODO

#### 6.1 Warp Active Mask

`__activemask()` 返回一个 32 位掩码，表示调用时刻 Warp 中哪些线程是活跃的。**不能**用它来判断哪些线程执行了某个分支——它只是瞬时快照，编译器可能重排指令导致活跃集合变化。

#### 6.2 Warp Vote

- `__all_sync(mask, predicate)`：mask 中所有未退出线程的 predicate 均非零时返回非零。
- `__any_sync(mask, predicate)`：mask 中任一未退出线程的 predicate 非零时返回非零。
- `__ballot_sync(mask, predicate)`：返回一个掩码，第 N 位为 1 当且仅当第 N 个线程的 predicate 非零且活跃。

#### 6.3 Warp Match

- `__match_any_sync(mask, value)`：返回具有相同 bitwise value 的线程掩码。
- `__match_all_sync(mask, value, int* pred)`：若 mask 中所有未退出线程的 value 相同则返回 mask，否则返回 0；`pred` 用于传出一致性结果。

#### 6.4 Warp Reduce

`__reduce_add_sync`、`__reduce_min_sync`、`__reduce_max_sync`、`__reduce_and_sync`、`__reduce_or_sync`、`__reduce_xor_sync` 在 mask 指定的线程间执行归约操作。需要 sm_80+。这些原语不提供内存序保证。

#### 6.5 Warp Shuffle

Warp Shuffle 允许 Warp 内线程直接交换数据而无需共享内存：
- `__shfl_sync(mask, value, srcLane, width)`：从指定 lane 直接复制值。
- `__shfl_up_sync(mask, value, delta, width)`：从 lane ID 更小的线程获取值，向上平移。
- `__shfl_down_sync(mask, value, delta, width)`：从 lane ID 更大的线程获取值，向下平移。
- `__shfl_xor_sync(mask, value, laneMask, width)`：通过与 `laneMask` 做 XOR 计算源 lane，实现蝶形寻址，常用于树形归约和广播。

`width` 必须是 1、2、4、8、16、32 中的幂次。当 `width < warpSize` 时，Warp 被划分为多个子分区，每个分区独立操作。目标线程必须活跃，否则读取值未定义。

#### 6.6 Warp `__sync` Intrinsic 约束

所有 Warp `__sync` 类原语（Shuffle、Vote、Match、Reduce、`__syncwarp`）共享一套严格的 mask 约束：
- 每个调用线程必须在 mask 中设置对应的位。
- 每个未调用的非退出线程在 mask 中对应位必须为 0（退出线程被忽略）。
- mask 中所有未退出线程必须使用相同的 mask 值执行该原语。
- 允许并发执行不同的 mask 值，前提是这些 mask 互不相交。
- 在条件代码中，条件必须在 mask 的所有未退出线程中一致求值。

违反上述约束将导致未定义行为，最常见的是 Kernel 挂起（死锁）。

### 7. CUDA 专用宏

#### 7.1 `__CUDA_ARCH__`

该宏仅在 Device 代码中定义，值为虚拟架构版本号（如 `compute_80` 对应 `800`）。它用于根据架构裁剪代码路径，但有严格限制：
- **`__global__` 函数、函数模板、`__device__`/`__constant__` 变量、纹理/表面类型的签名不能依赖于 `__CUDA_ARCH__` 是否定义或其值。** 否则会导致 Host 和 Device 侧看到不同的类型，链接时出错。
- 从 Host 侧实例化并启动的 `__global__` 函数模板，其模板实参不能因 `__CUDA_ARCH__` 而不同。
- 在单独编译（Separate Compilation）模式下，外部链接的函数/变量定义的存在性不能依赖于 `__CUDA_ARCH__`。
- 在头文件中定义的弱符号或模板函数，其行为不应依赖 `__CUDA_ARCH__`，否则不同对象文件若以不同架构编译，链接时可能只保留一个版本，导致行为不一致。

#### 7.2 `__CUDA_ARCH_SPECIFIC__` 与 `__CUDA_ARCH_FAMILY_SPECIFIC__`

用于识别架构特定（`-a` 后缀）或家族特定（`-f` 后缀）的特性。例如 `compute_100a` 会定义 `__CUDA_ARCH_SPECIFIC__ = 1000`，而 `compute_100f` 会定义 `__CUDA_ARCH_FAMILY_SPECIFIC__ = 1000`。

#### 7.3 特性测试宏

- `__CUDACC_DEVICE_ATOMIC_BUILTINS__`：支持 `__nv_atomic_*` 内置函数。
- `__NVCC_DIAG_PRAGMA_SUPPORT__`：支持诊断控制 pragma。
- `__CUDACC_EXTENDED_LAMBDA__`：支持扩展 lambda（需 `--expt-extended-lambda`）。
- `__CUDACC_RELAXED_CONSTEXPR__`：支持放宽的 constexpr（需 `--expt-relaxed-constexpr`）。

### 8. CUDA 专用函数

#### 8.1 地址空间谓词与转换

- 谓词函数：`__isGlobal(ptr)`、`__isShared(ptr)`、`__isConstant(ptr)`、`__isGridConstant(ptr)`、`__isLocal(ptr)` 返回 1 若指针指向对应地址空间。
- 转换函数：`__cvta_generic_to_global/shared/constant/local` 将通用地址转换为特定地址空间的原始地址；`__cvta_global/shared/constant/local_to_generic` 执行反向转换。这在内嵌 PTX 汇编或进行 32 位地址优化时非常有用。由于 Shared/Local/Constant 地址空间的有效地址范围通常小于 32 位，可将 64 位指针截断为 32 位整数以节省寄存器，需要时再零扩展回 64 位并转换。

#### 8.2 底层 Load/Store

- `__ldg(const T*)`：通过只读 L1/Tex 缓存加载（PTX `ld.global.nc`），支持基本类型、向量类型和扩展浮点类型。
- `__ldcg`、`__ldca`、`__ldcs`、`__ldlu`、`__ldcv`：分别对应 PTX 的不同缓存操作符。
- `__stwb`、`__stcg`、`__stcs`、`__stwt`：对应不同缓存策略的存储操作。
TODO 什么叫做 tex 缓存?

#### 8.3 `__trap()` 与 `__nanosleep()`

- `__trap()` 立即终止 Kernel 执行并触发 Host 程序中断，但会**破坏 CUDA 上下文**，导致后续 CUDA 调用失败。建议使用 `cuda::std::terminate()` 作为可移植替代。 TODO
- `__nanosleep(unsigned nanoseconds)` 使线程休眠约指定纳秒（最大约 1 毫秒），常用于实现自旋锁的指数退避策略，减少内存总线争用。
TODO 似乎 cuda 实现 __nanosleep 不容易，需要操作系统的协助才可以的

#### 8.4 DPX 指令

Dynamic Programming eXtension (DPX) 提供硬件加速的三操作数 min/max、融合 add+min/max，以及可选的 ReLU（截断到零）。例如 `__vimax3_s32_relu(a, b, c)` 计算 `max(a, b, c, 0)`。这些指令对基因组学（Smith-Waterman、Needleman-Wunsch）和路径优化（Floyd-Warshall）等动态规划算法极为高效。支持情况取决于计算能力。
TODO 这个和我理解的 DP 似乎不一样啊

### 9. 编译器优化提示

- **`#pragma unroll`**：置于循环前，控制展开行为。无参数时完全展开已知次数的循环；参数为 0 或 1 时禁用展开；参数为负或大于 `INT_MAX` 时忽略。
- **`__builtin_assume_aligned(ptr, align, offset)`**：告诉编译器返回的指针对齐到至少 `align` 字节（考虑 `offset`）。
- **`__builtin_assume(bool)` / `__assume(bool)`**：断言布尔条件为真，若为假则未定义行为。编译器可据此消除不可达分支。
- **`__builtin_expect(input, expected)`**：分支预测提示，类似 C++20 的 `[[likely]]` / `[[unlikely]]`。
- **`__builtin_unreachable()`**：标记代码永不可达，消除警告并优化代码生成。
- **`#pragma nv_abi preserve_n_data(EXPR) preserve_n_control(EXPR)`**：在单独编译模式下，限制函数调用时保留的寄存器数量，使性能接近全程序编译。注意它**仅对间接函数调用有效**，直接调用会被编译器忽略。

### 10. 调试与诊断

- **`assert(expression)`**：若 expression 为 0，Kernel 中止，触发 Host 断点或同步后向 stderr 输出文件名、行号、函数名、Block/Thread 索引和失败表达式。它也会破坏 CUDA 上下文。可通过定义 `NDEBUG` 宏在编译期禁用。
- **`__brkpt()`**：从 Device 线程触发断点。
- **诊断 Pragmas**：`#pragma nv_diag_suppress/warning/error/default/once <error_number>` 控制特定诊断信息的严重程度；`#pragma nv_diagnostic push/pop` 保存和恢复诊断状态。这些仅影响 nvcc 前端。

### 11. Warp 矩阵函数（WMMA）

通过 `nvcuda::wmma` 命名空间调用 Tensor Core 执行 `D = A * B + C` 形式的矩阵乘加：
- `fragment<Use, m, n, k, T, Layout>`：Warp 级矩阵分块，分布在 Warp 的 32 个线程中，内部映射未公开。
- `load_matrix_sync` / `store_matrix_sync`：同步加载/存储 fragment。要求 `mptr` 256 位对齐，`ldm` 满足元素类型对应的字节倍数。
- `fill_fragment`：用常量填充 fragment。
- `mma_sync`：执行矩阵乘加。`satf=true` 时对 Inf/NaN 做饱和处理。

Tensor Core 支持多种精度组合。`__nv_bfloat16` 需与 `float` 累加器配对；`tf32` 需手动通过 `__float_to_tf32` 转换输入，且仅支持 16x16x8 (m-n-k)。双精度支持需 sm_80+。Sub-byte 操作（4-bit/1-bit）目前为预览特性，API 可能变更。

## 典型工作流程 / 调用顺序

一个典型的、利用本章语言扩展的高性能 CUDA Kernel 开发流程如下：

1. **定义 Kernel 接口**：使用 `__global__` 声明 Kernel，为只读指针参数添加 `const` 和 `__restrict__`，必要时为参数添加 `__grid_constant__`。若对寄存器用量或占用率有精确要求，附加 `__launch_bounds__`。
2. **配置启动参数**：在 Host 侧通过 `cudaGetDeviceProperties` 查询设备限制，计算合理的 `gridDim` 和 `blockDim`。若需动态共享内存，计算大小并传入第三个执行配置参数。
3. **线程身份识别**：Kernel 内通过 `blockIdx`、`threadIdx`、`blockDim`、`gridDim` 计算全局线程 ID。若涉及 Warp 级操作，计算 `laneId = threadIdx.x % warpSize`。
4. **内存分配与数据加载**：根据访问模式选择内存空间。需要 Block 内协作的临时数据放入 `__shared__`；只读小表通过 `__constant__` 或 `__ldg()` 访问；全局输入输出通过 `__restrict__` 指针访问。
5. **执行计算与多级同步**：
   - 线程完成局部计算后，若需 Warp 内交换数据，使用 Warp Shuffle（`__shfl_xor_sync` 等）。
   - 若需 Block 内协作（如共享内存归约），写入 `__shared__` 后执行 `__syncthreads()`。
   - 若多 Block 协作写入全局内存后需通知其他 Block，先写内存，再执行 `__threadfence()`（或 `cuda::atomic_thread_fence(..., thread_scope_device)`），最后执行原子操作（`atomicInc` 等）标记完成。
6. **Host 同步与错误检查**：Kernel 启动后调用 `cudaDeviceSynchronize()` 或 `cudaStreamSynchronize()`。使用 `cudaGetLastError()` 捕获启动错误。若使用了 `assert`，同步后检查 stderr 输出。
7. **调试与优化**：开发阶段保留 `assert` 和边界检查；性能调优阶段使用 `--resource-usage` 查看寄存器用量，通过 Profiler 查看 Occupancy，必要时调整 `__launch_bounds__` 或重构算法以减少寄存器压力。

## 关键限制、边界条件与兼容性

- **架构差异**：大量特性有严格的计算能力门槛。sm_61（如 GTX 1060）不支持 Tensor Core/WMMA（需 sm_70+）、Warp Reduce 原语（需 sm_80+）、Thread Block Cluster（需 sm_90+）、128-bit 原子操作（需 sm_90+）以及 DPX 指令。代码若需跨架构部署，必须通过 `__CUDA_ARCH__` 进行条件编译，或在运行时查询设备属性并回退到通用实现。
- **`__global__` 函数签名不可变**：`__CUDA_ARCH__` 不能影响 `__global__` 函数或其模板实例化的类型签名，否则 Host 和 Device 侧的符号表会不一致。
- **`__syncthreads` 的死锁条件**：任何导致 Block 内线程以不同控制流路径到达 `__syncthreads` 的代码都是错误的，包括基于 `threadIdx.x` 的非均匀分支、循环中每个线程执行不同次数的同步等。
- **Warp Mask 一致性**：Warp 同步原语的 mask 必须在所有参与线程中严格一致，且调用线程必须把自己对应的位设为 1。mask 不一致或重叠（非不相交）的并发调用将导致未定义行为。
- **`__managed__` 静态初始化陷阱**：`__managed__` 变量的地址不能出现在全局/静态对象的构造函数或析构函数中，也不能出现在 `__attribute__((constructor))` / `__attribute__((destructor))` 标记的函数中，因为此时 CUDA Runtime 可能尚未初始化或已经终止。
- **上下文破坏**：`assert()` 失败和 `__trap()` 会立即使 CUDA 上下文失效，后续所有 CUDA API 调用都会返回 `cudaErrorAssert` 或类似错误。调试代码中应做好准备捕获此类错误并复位设备。
- **`__launch_bounds__` 与 `__maxnreg__` 互斥**：不能同时对同一个 Kernel 使用两者。`__maxnreg__` 的优先级高于文件级 `--maxrregcount`。
- **原子操作的内存序**：Legacy 原子函数（如 `atomicAdd`）等效于 `memory_order_relaxed`，不建立同步点（fence）。如果需要在原子操作前后建立 happens-before 关系，必须显式使用 `cuda::atomic_thread_fence` 或 `__threadfence` 系列函数。
- **Sub-byte / tf32 预览状态**：Sub-byte WMMA 和 tf32 精度属于预览特性，其数据结构签名和 API 在后续 CUDA 版本中可能不兼容，生产代码中应谨慎使用。

## 常见陷阱与调试建议

- **过度使用 `__restrict__`**：虽然它能消除别名分析障碍、减少指令数，但缓存到寄存器会增加寄存器压力。在寄存器本就紧张的 Kernel 中，这可能降低 Occupancy，反而损害性能。应通过 Profiler 验证实际效果。
- **Launch Bounds 与实际启动不匹配**：若 Kernel 声明了 `__launch_bounds__(256)`，却在 Host 侧以 512 线程启动，Kernel 将启动失败。Host 侧用于决定线程数的宏不应依赖 `__CUDA_ARCH__`（该宏在 Host 代码中未定义）。
- **忽略 `__syncthreads` 的 uniform 条件要求**：初学者常在 `if (threadIdx.x < some_value)` 分支内调用 `__syncthreads()`，若 `some_value` 不是 blockDim.x 的完整倍数或条件非均匀，Kernel 将死锁。应将同步点放在分支之外，或确保所有线程都走同一条路径。
- **Warp Shuffle 的越界与 mask 错误**：使用 `__shfl_sync` 时，若目标 lane 不在当前活跃 mask 内，读取值未定义。另外 `width` 若不是 2 的幂或不在 [1, 32] 范围内，结果也是未定义的。实现归约时应优先使用 `__shfl_xor_sync` 的蝶形模式。
- **原子操作与内存栅栏混淆**：内存栅栏只保证**顺序**，不保证**可见性**。即使插入了 `__threadfence()`，其他线程仍需通过同步或原子操作才能看到最新值。在多 Block 协作的算法中，通常需要将结果变量声明为 `volatile` 以绕过 L1 缓存，确保后续读取拿到全局内存的最新值。
- **编译器优化提示的副作用**：`__builtin_assume(predicate)` 若 predicate 带有副作用（如函数调用），行为未指定；若运行时 predicate 为假，则是未定义行为。这些提示应仅用于编译器确实无法推断的不变式。
- **单独编译时的 `__CUDA_ARCH__` 陷阱**：在头文件中编写依赖 `__CUDA_ARCH__` 的模板函数，若不同 `.cu` 文件以不同架构编译，链接时可能只选择一个版本，导致运行时的行为与预期不符。解决方案是统一架构编译，或将架构相关逻辑移出共享头文件。

## 一个最小可运行示例的说明

本章配套的最小可运行示例 `chapter_demo.cu` 被设计为在 GTX 1060（sm_61，CUDA 12.8）上直接编译运行，同时尽量覆盖本章的核心语言扩展。示例实现了一个向量缩放后求和的操作，其设计意图和兼容性处理如下：

### 示例覆盖的语言特性

1. **执行空间限定符**：`vector_scale_and_reduce` 被声明为 `__global__`，`warp_reduce_sum` 被声明为 `__device__`，`print_arch_info` 被声明为 `__host__ __device__` 以展示双路径编译。在 `main` 中调用了 Host 侧的 `print_arch_info()`。
2. **内存空间限定符**：使用了 `__constant__ float c_scale_factor` 存储全局只读的缩放系数；Kernel 内部使用 `__shared__ float sdata[256]` 进行 Block 级归约；输入输出通过 `__device__` 指针访问全局内存。
3. **`__restrict__` 指针**：Kernel 的两个指针参数都标记为 `__restrict__`，向编译器承诺无别名，以便生成更高效的加载指令。
4. **`__launch_bounds__`**：Kernel 声明中附加了 `__launch_bounds__(256, 2)`，提示编译器该 Kernel 最大 Block 大小为 256，且期望每个 SM 至少驻留 2 个 Block。这有助于编译器在寄存器分配和占用率之间做权衡。
5. **内置变量**：使用了 `blockIdx.x`、`threadIdx.x`、`blockDim.x`、`gridDim.x` 计算全局索引，并使用了 `warpSize`（通过 `threadIdx.x % warpSize` 获取 lane ID）。
6. **Warp Shuffle**：由于 sm_61 不支持 sm_80+ 才引入的 `__reduce_add_sync`，示例退而使用传统的 `__shfl_xor_sync` 蝶形归约模式，在 Warp 内 32 个线程间通过 `offset = 16, 8, 4, 2, 1` 的 XOR 洗牌完成求和。这是 Warp Shuffle 最经典的用法之一。
7. **Block 同步**：Warp 归约结果写入共享内存后，调用 `__syncthreads()` 确保所有 Warp 完成写入；最终归约仅在 `threadIdx.x < warpSize` 的线程中进行，条件在整个 Block 上是均匀的（因为 `warpSize` 为 32，而 Block 大小为 256，所有线程都走同一条 `if` 评估路径——前 32 个线程进入执行，其余不进入，这是合法的，因为这里只有一个 `__syncthreads()` 且所有线程都到达了它）。
8. **内存栅栏**：Block 归约完成后，Thread 0 在调用 `atomicAdd` 之前先执行 `__threadfence_block()`。虽然对于同一个 Block 内的原子操作到全局内存而言，`__syncthreads()` 已经保证了可见性，但此处显式加入 Fence 是为了演示其语法和语义位置。
9. **原子操作**：使用 Legacy 的 `atomicAdd(d_total_sum, block_sum)` 将所有 Block 的局部和累加到单个全局变量中。
10. **`__CUDA_ARCH__` 条件编译**：`print_arch_info()` 函数体内通过 `#if defined(__CUDA_ARCH__)` 分别打印 Host 和 Device 侧的信息。

### sm_61 兼容性处理

代码在 `main` 函数开始时通过 `cudaGetDeviceProperties` 查询设备的计算能力，并打印明确的降级说明：
- 若 `prop.major < 7`，提示 Tensor Core（WMMA）需要 sm_70+，本示例未使用。
- 若 `prop.major < 9`，提示 Thread Block Cluster 需要 sm_90+，本示例未使用。
- 若 `prop.major < 8`，提示 Warp Reduce 原语需要 sm_80+，本示例已使用 Shuffle 回退。

这些运行时检测确保即使代码被移植到更旧的架构上，开发者也能从控制台输出中立即了解哪些高级特性被自动跳过。

### 编译与运行

使用项目根目录下的 `Makefile` 编译：
```bash
make
```
`Makefile` 指定了 `nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2` 作为编译命令，与任务要求完全一致。生成的二进制为 `chapter_demo.out`，运行后应输出总和平局值为 `2097152.00`（因为输入是 1024*1024 个 1.0，乘以 2.0 后求和），并显示 `Result: PASS`。
