## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/dynamic-parallelism.html>


CUDA Dynamic Parallelism（CDP，CUDA 动态并行）是 CUDA 编程模型中的一项核心特性，它允许已经在 GPU 上运行的设备代码（device code）直接发起新的 GPU 任务。在 CDP 出现之前，所有 kernel 的启动必须由主机端（host）完成，任何需要在运行时才决定并行度的算法都必须将控制权交回 CPU，由 CPU 根据中间结果重新配置并启动下一批 kernel。CDP 的出现打破了这种"单层扁平并行"的限制，使 GPU 线程能够在设备端以 `<<< >>>` 语法动态创建新的 grid，从而大幅减少主机与设备之间的控制流往返和数据传输开销。

本文档对应的是 CUDA 12.0 及之后默认启用的 CDP2（第二代动态并行）。对于计算能力（Compute Capability, CC）9.0 及更高的设备，CDP2 是唯一可用的动态并行版本；对于 CC 低于 9.0 的旧设备，仍可通过编译选项 `-DCUDA_FORCE_CDP1_IF_SUPPORTED` 回退到遗留的 CDP1，但 CDP1 已被标记为将在未来版本中移除。本章详细阐述了 CDP 的执行环境、内存一致性语义、设备端可用的 API 集合、流与事件的行为差异、编程限制以及从 PTX 层面发起设备端启动的低级机制。

## 背景与要解决的问题

在传统的 CUDA 编程模型中，并行结构必须在主机端静态确定：host 代码需要预先知道 grid 和 block 的维度、kernel 的启动次数以及任务之间的依赖关系。这种模型对数据依赖型或递归型算法极不友好，例如：

- **不规则循环与递归**：某些图遍历、自适应网格细化（AMR）、快速多极子（FMM）等算法天然具有递归或不规则循环结构，强行展平成单层循环会导致代码复杂且效率低下。
- **运行时才能确定的并行度**：某些计算任务的子任务数量只有在父 kernel 执行过程中才能根据数据内容确定。没有 CDP 时，必须在父 kernel 中将结果写回全局内存，然后由 host 读取、分析、再启动子 kernel。
- **控制流开销**：频繁地在 host 与 device 之间切换会引入同步延迟和 PCIe 数据传输瓶颈，降低整体吞吐量。

CDP 通过允许设备端线程直接启动新 grid，将"启动配置决策"下放到 GPU 内部完成，使得上述编程模式能够以更自然的方式表达，同时减少 host-device 之间的交互。

## 核心概念与术语

### Grid（网格）

Grid 是 kernel 的一次具体调用实例，包含特定的 block 维度和 grid 维度。在 CDP 语境中，区分"kernel 函数本身"与"某次具体的 grid 调用"非常重要，因为父子关系是围绕 grid 实例建立的，而不是围绕函数名建立的。

### Parent Grid 与 Child Grid

- **Parent Grid（父网格）**：包含那个发起新 kernel 启动的线程所在的 grid。
- **Child Grid（子网格）**：由父网格中的线程通过设备端 `<<< >>>` 语法启动的新 grid。

父子关系满足**严格嵌套（properly nested）**：父 grid 不会被视为已经完成，直到它所有线程创建的全部 child grid 都已完成。运行时会在父 grid 与其所有后代 grid 之间保证**隐式同步**。

### CUDA Device Runtime

设备端可用的 API 集合称为 CUDA Device Runtime。它的语法与主机端的 CUDA Runtime API 高度相似，但仅支持一个受限的子集。这些 API 在设备端调用，管理对象（如 stream、event）的作用域被限定在创建它们的 grid 内部。

### CDP1 与 CDP2

- **CDP1**：遗留版本，支持设备端 `cudaDeviceSynchronize()`，但在 CUDA 12.0+ 中默认被替换。
- **CDP2**：新版本，移除了设备端 `cudaDeviceSynchronize()`，引入了 `cudaStreamTailLaunch`（尾启动流）和 `cudaStreamFireAndForget`（fire-and-forget 流）等新语义。对于 sm_90+ 是唯一选择。

## API / 机制详解

### 设备端 Kernel 启动语法

设备端启动 kernel 的语法与主机端完全一致：

```cpp
kernel_name<<< Dg, Db, Ns, S >>>([kernel arguments]);
```

- `Dg`（`dim3`）：grid 的维度与大小。
- `Db`（`dim3`）：每个 block 的维度与大小。
- `Ns`（`size_t`）：除静态分配外，每个 block 额外动态分配的共享内存字节数，默认为 0。
- `S`（`cudaStream_t`）：关联的流。该流必须在发起调用的同一个 grid 内被创建；默认为 NULL stream。

**启动是异步的**：与主机端一样，`<<<>>>` 调用会立即返回，启动线程继续向下执行。子 grid 的实际开始时间不受保证，直到启动线程遇到某个隐式同步点（例如 tail launch）时，才会强制等待之前启动的子 grid。

### Tail Launch Stream（cudaStreamTailLaunch）

CDP2 中最重要的新机制之一是 `cudaStreamTailLaunch`。将 kernel 启动到该流中意味着：

- 该 kernel 会在**当前 grid 的所有先前启动的子工作（包括其他 stream 中的工作）完成后**才开始执行。
- 它提供了一种在父 grid 退出前读取子 grid 对全局内存修改结果的**唯一可靠方式**。
- 因为 CDP2 已经移除了设备端 `cudaDeviceSynchronize()`，如果不使用 tail launch，父 grid 的线程无法安全地看到子 grid 对全局内存的写入。

例如，文档中的示例展示了 `child_launch` 修改数据后，`tail_launch` 被发射到 `cudaStreamTailLaunch`，从而保证 `tail_launch` 能看到 `child_launch` 的结果。

### Fire-and-Forget Stream（cudaStreamFireAndForget）

文档虽未在本节选段中展开，但提到了该流的存在。Fire-and-forget 启动允许子 grid 脱离严格的父子嵌套同步语义，适用于不需要被父 grid 等待的场景。它进一步提升了某些独立子任务的并发性。

### 设备端 Stream 与 Event

**Stream**：
- 设备端创建的 stream 可被同一个 grid 内的所有线程共享。一个线程创建的 stream，另一个线程可以合法使用。
- stream 的作用域仅限于创建它的 grid。在 grid 外部使用会产生未定义行为。
- 同一 grid 内的多个线程向**同一个命名 stream** 发射 kernel，这些 kernel 按 CUDA stream 语义顺序执行。若多个线程向**隐式 NULL stream** 发射，且这些线程位于**同一个 block** 内，则顺序执行；若位于**不同 block**，则可能并发执行。

**Event**：
- 仅支持**跨流同步**能力，即 `cudaStreamWaitEvent()` 可用。
- `cudaEventSynchronize()`、`cudaEventElapsedTime()`、`cudaEventQuery()` **不可用**。
- 由于不支持计时，event 必须通过 `cudaEventCreateWithFlags(&event, cudaEventDisableTiming)` 创建。
- Event 的作用域同样限定于创建它的 grid，跨 grid 使用 event handle 是未定义行为。

### 设备端同步的缺失与替代

CDP2 明确**移除了设备端 `cudaDeviceSynchronize()`**。这意味着：

- 父 grid 的线程**无法显式等待**由同 grid 内其他线程启动的子 grid。
- 如果线程 A 启动了子 grid，线程 B 无法通过任何 API 确保该子 grid 已完成。
- 所有子 grid 的完成保证仅通过**隐式同步**实现：父 grid 整体被视为完成，当且仅当所有它直接或间接启动的 child grid 都已完成。

因此，如果某个线程需要依赖由其他线程启动的子 grid 结果，必须通过 CUDA Event（`cudaStreamWaitEvent`）来建立跨线程的流依赖关系。

### 设备管理 API

设备端运行时仅允许查询和控制**当前正在运行 kernel 的那个设备**：

- `cudaSetDevice()` **不支持**。
- `cudaGetDevice()` 返回的 device ID 与主机端视角一致。
- `cudaDeviceGetAttribute()` 支持查询其他设备的信息（因为它接受 device ID 参数）。
- `cudaGetDeviceProperties()` **不支持**；属性必须逐个查询。

### 环境配置继承

子 grid 继承父 grid 的全局设备配置，例如：

- 共享内存 / L1 缓存配置（`cudaDeviceGetCacheConfig` 的返回值）。
- 设备限制（`cudaDeviceGetLimit` 的返回值，如栈大小）。
- 对于主机端启动的 kernel，其通过 `cudaFuncSetCacheConfig` 等 API 设置的 per-kernel 配置也会被设备端启动继承。

**无法从设备端重新配置**这些环境参数。

## 典型工作流程 / 调用顺序

一个典型的 CDP2 程序通常遵循以下步骤：

1. **主机端准备**：分配全局内存，将输入数据拷贝到设备，配置任何需要的 per-kernel 属性。
2. **启动 Parent Grid**：从 host 启动父 kernel。父 kernel 内部通常只有一个线程（如 `threadIdx.x == 0`）负责启动子任务，以避免重复启动。
3. **父线程设置数据**：父线程将中间结果写入全局内存，并视情况调用 `__syncthreads()` 确保同 block 内其他线程的写入对启动线程可见。
4. **异步启动 Child Grid**：父线程使用 `<<<>>>` 启动 child kernel。该调用立即返回。
5. **（可选）启动 Tail Launch Kernel**：将需要读取子 grid 结果的后续 kernel 启动到 `cudaStreamTailLaunch`，确保它在所有先前子工作完成后执行。
6. **父 Grid 隐式同步退出**：当父 grid 的所有线程执行完毕时，运行时自动等待所有子 grid（以及 tail launch）完成，然后才将父 grid 标记为完成。
7. **主机端同步与读取**：Host 调用 `cudaDeviceSynchronize()` 等待 parent grid 完成，然后将结果从设备内存拷贝回主机。

需要注意的是，父 grid 中的普通线程**不能在设备端查询 child grid 的完成状态**。如果必须在父 grid 执行期间获得子 grid 的结果，唯一合法的方式是通过 tail launch。

## 内存一致性、作用域与访问规则

### Global Memory（全局内存）

- 父子 grid 共享全局内存地址空间，指针可以直接传递。
- **弱一致性保证**：子 grid 在被调用时，只能确保看到**调用它的那个父线程**在调用点之前对全局内存的写入。如果父线程之前调用了 `__syncthreads()`，则同 block 内其他线程的写入也可能对子 grid 可见。
- **子 grid 的修改对父 grid 不可见**：除非通过 tail launch，否则父 grid 的线程永远无法安全读取子 grid 修改后的全局内存内容。

### Mapped Memory（映射内存）

映射到设备地址空间的系统内存具有一致性和一致性保证，与全局内存完全相同。设备端代码不能分配或释放映射内存，但可以使用从主机传入的映射内存指针。

### Shared Memory 与 Local Memory

- **Shared Memory**：仅对创建它的 block 可见。父 grid 不能访问子 grid 的 shared memory，反之亦然。将 shared memory 指针作为子 kernel 的参数传递是**未定义行为**，可能导致错误。
- **Local Memory**：属于单个线程的私有存储。将局部数组或局部变量的地址传递给子 kernel 是**非法的**。编译器会尝试对此发出警告，运行时可通过 `__isGlobal()` 内建函数检查指针是否指向全局内存。

### Texture Memory（纹理内存）

- 纹理内存访问是只读的。
- 对映射纹理的全局内存写入在纹理访问看来是 incoherent 的。
- 一致性在子 grid 调用时以及子 grid 完成时强制刷新。因此，父 grid 在子 grid 调用前的写入对子 grid 的纹理访问可见；但子 grid 的写入**不保证**对父 grid 的纹理访问可见（同样只能通过 tail launch 读取）。

## 关键限制、边界条件与兼容性

### 编译与链接要求

- 必须启用 **relocatable device code（`-rdc=true`）**，因为设备端 kernel 调用需要跨编译单元的链接能力。
- 必须链接 **设备运行时库（`-lcudadevrt`）**。

### CDP1 / CDP2 兼容性矩阵

| 编译方式 | CC < 9.0 | CC >= 9.0 |
|---|---|---|
| CUDA 12.0+ 默认 | CDP2（新接口） | CDP2（唯一接口） |
| 带 `-DCUDA_FORCE_CDP1_IF_SUPPORTED` | CDP1（遗留接口） | CDP2（唯一接口，若代码引用 `cudaDeviceSynchronize` 且编译目标为 sm_90+，则编译报错；若在 sm_90 上加载含 CDP1 的函数，返回 `cudaErrorSymbolNotFound`） |

- 在同一个 CUDA context 中，CDP1 函数和 CDP2 函数可以**共存并同时运行**。
- **互斥规则**：CDP1 函数不能启动 CDP2 函数，CDP2 函数也不能启动 CDP1 函数。若调用图中出现版本混合，加载时会返回 `cudaErrorCdpVersionMismatch`。

### 运行时资源限制

- **Pending Kernel Launches**：设备运行时需要维护一个固定大小的 launch pool 来跟踪尚未完成的 kernel 启动。可以通过主机端调用 `cudaDeviceSetLimit(cudaLimitDevRuntimePendingLaunchCount, ...)` 调整该池大小。
- **内存占用**：设备运行时的跟踪和管理软件会占用一部分设备内存，并可能对当前正在运行的任何 kernel（无论是否使用 CDP）引入少量性能开销。

### 并发性保证的缺失

- CUDA 执行模型**不保证**不同 block 之间的并发执行。这一定律同样适用于 parent grid 与 child grid。
- 子 grid 可能在 stream 依赖满足且硬件资源可用时开始执行，但**不保证**在父 grid 到达隐式同步点之前一定开始执行。
- 因此，**绝不能依赖**不同 block 或不同 grid 之间的并发性来编写正确性逻辑。

## 常见陷阱与调试建议

### 1. 误以为可以在设备端同步任意子 grid

CDP2 已经移除 `cudaDeviceSynchronize()`。如果你在设备代码中调用它，对于 sm_90+ 会直接编译失败；对于 sm_61 等旧设备，若使用默认 CDP2 编译，同样不可用。正确做法是使用 `cudaStreamTailLaunch` 来串连依赖。

### 2. 传递局部变量指针给子 kernel

这是最常见的错误之一：

```cpp
__device__ void bad() {
    int value = 5;
    child<<<1,1>>>(&value);  // 错误：value 在父线程的 local memory 中
}
```

应改为全局内存分配：`cudaMalloc`、`new()` 或声明 `__device__` 全局变量。

### 3. 跨 grid 使用 stream 或 event

在 parent grid 中创建的 stream 不能在 child grid 中使用，反之亦然。Event handle 在不同 grid 之间不保证唯一性，误用会导致未定义行为。

### 4. 忽略隐式 NULL stream 的 block 级作用域

同一 block 内的线程向隐式 NULL stream 发射 kernel 是顺序的；但**不同 block** 的线程向隐式 NULL stream 发射的 kernel 可能并发。如果需要跨 block 的顺序保证，必须显式创建并使用命名 stream。

### 5. 编译时忘记 `-rdc=true` 或 `-lcudadevrt`

缺少这两者会导致链接错误，因为设备端的 kernel 符号解析需要 relocatable device code 支持，而启动管理逻辑在 `cudadevrt` 库中。

### 6. 性能开销误判

只要应用程序链接了设备运行时库，动态并行管理软件的跟踪开销就可能影响**所有**正在运行的 kernel，无论它们自己是否发起设备端启动。因此，如果 CDP 使用频率很低，需要评估这种固定开销是否划算。

## 一个最小可运行示例的说明

本目录下的 `chapter_demo.cu` 是一个针对 **GTX 1060 (sm_61, CUDA 12.8)** 环境设计的可编译示例。它的设计目标与兼容处理如下：

### 设计目标

1. **演示设备端 kernel 启动**：父 kernel `parentKernel` 由 host 启动，其内部由 `threadIdx.x == 0` 的线程启动子 kernel `childKernel`。
2. **演示 Tail Launch 语义**：`tailKernel` 被发射到 `cudaStreamTailLaunch`，展示 CDP2 中"在父 grid 退出前安全读取子 grid 修改结果"的唯一合法路径。
3. **验证全局内存数据流**：host 分配设备内存，子 grid 对数组每个元素加 1，tail kernel 再对每个元素加 1，最终 host 读取并验证结果为 `原始值 + 2`。
4. **运行时降级检测**：若当前设备的计算能力低于 3.5（不支持 CDP），程序会打印说明并优雅退出，而不是崩溃。

### 在 sm_61 上的兼容处理

- **编译参数**：使用 `-arch=sm_61` 编译，CDP 从 sm_35 起即受支持，因此 sm_61 完全支持动态并行。
- **CDP2 默认**：CUDA 12.8 的默认行为即为 CDP2。示例使用 `cudaStreamTailLaunch`，这是 CDP2 的标准特性，在 sm_61 上编译和运行均合法。
- **避免已移除的 API**：示例中**没有**调用 `cudaDeviceSynchronize()` 设备端版本，因此不会因为 CDP2 的移除策略而编译失败。
- **错误检查**：所有 CUDA Runtime API 调用均包裹在 `CUDA_CHECK` 宏中，任何错误都会立即输出文件名、行号和错误字符串，便于在 sm_61 真机上快速定位问题。

### 编译与运行

```bash
make        # 编译生成 chapter_demo.out
make run    # 编译并运行
```

Makefile 中显式指定了 `nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2 -rdc=true`，并链接 `-lcudadevrt`，满足 CDP 的编译链接要求。
