## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/programmatic-dependent-launch.html>


本章节来自 NVIDIA CUDA Programming Guide Release 13.2，核心包含两大部分：前半部分（4.4.8 节）介绍了 Cooperative Groups 中的 Large Scale Groups 机制，即如何通过 `cudaLaunchCooperativeKernel` 实现跨越整个 grid 的线程同步；后半部分（4.5 节）引入了 CUDA 9.0 计算能力设备才支持的 Programmatic Dependent Launch and Synchronization（可编程依赖启动与同步）机制。后者允许在同一个 CUDA stream 中，后提交的 secondary kernel 不必等待先提交的 primary kernel 完全执行结束即可被提前启动，从而利用 secondary kernel 的 preamble 阶段（如清零缓冲区、加载常量等不依赖 primary kernel 结果的工作）与 primary kernel 的执行重叠，达到隐藏启动延迟和提升整体吞吐的目的。

这两个主题虽然在功能上有所区别——前者解决的是"grid 内跨 block 如何同步"的问题，后者解决的是"有依赖关系的两个 kernel 如何尽量并发"的问题——但它们都属于 CUDA 执行模型的高级控制手段，且都对 kernel 的启动方式和设备能力有严格限制。理解它们的前提条件、API 语义差异和常见陷阱，对于在支持硬件上榨取极致性能至关重要。

TODO 这个章节中东西写的不错，但是似乎章节覆盖范围有点不对，需要重新搞一下
## 背景与要解决的问题

### 跨 Block 同步的需求

传统的 CUDA kernel 中，`__syncthreads()` 只能同步单个 thread block 内的线程。如果算法本身需要所有线程（跨越多个 block）在某个阶段达成一致——例如分布式归约、全局栅栏、或者所有 block 共同更新一个全局数据结构后再继续下一阶段——开发者通常只能把任务拆分成多个 kernel 调用，依靠隐式的 stream 顺序保证来模拟全局同步。这会带来两个问题：一是 kernel 启动本身有延迟（launch latency），频繁拆分增加了总开销；二是全局数据需要写回全局内存，下一个 kernel 再读回，增加了不必要的内存流量。

Cooperative Groups 的 Large Scale Groups 正是为了解决这一痛点而设计，它允许将整个 grid 视为一个巨大的协作组，并通过 `grid.sync()` 在设备端直接完成全局同步。

### 依赖 Kernel 的串行瓶颈

在典型的 GPU 工作负载中，应用会按顺序向同一个 stream 提交多个 kernel。如果 secondary kernel 的数据依赖 primary kernel 的输出，那么默认情况下 CUDA driver 会严格保证执行顺序：primary kernel 全部执行完毕、结果刷新到全局内存后，secondary kernel 才开始执行。

然而，几乎所有 kernel 都有一个 preamble 阶段——例如初始化局部变量、清零输出缓冲区、加载只读常量、或者做一些与上游数据无关的预备计算。这一阶段理论上可以和 primary kernel 的后半段执行重叠。如果能让 secondary kernel 的 preamble 提前开始，就可以隐藏一部分启动延迟，并提高 GPU 的整体利用率。

Programmatic Dependent Launch 的设计动机就在于此：在保持语义正确（secondary kernel 读取 primary kernel 结果前必须完成同步）的前提下，允许 driver 在硬件层面提前发起 secondary kernel 的执行，从而创造更多的并发机会。

## 核心概念与术语

**Cooperative Groups / Large Scale Groups**
Cooperative Groups 是 CUDA 提供的一种线程协作抽象。除了传统的 thread block 级别分组外，Large Scale Groups 允许构造覆盖整个 grid 的 `grid_group`。所有之前介绍的 Cooperative Groups 功能都适用于这种大规模分组，唯一的例外是：整个 grid 的同步不能仅靠设备端代码完成，必须通过 `cudaLaunchCooperativeKernel` 这个运行时 API 来启动 kernel。

**cudaLaunchCooperativeKernel**
这是 CUDA runtime 提供的专门用于启动 cooperative kernel 的 API。它与普通的 `<<< >>>>` 语法最大区别在于：driver 会保证所有请求的 thread block 原子性地（atomically）被调度到设备上。如果调用成功，意味着所有 block 都会同时开始争夺 SM 资源；如果设备当前无法同时容纳这么多 block（例如同时驻留的 block 数超过了 occupancy 限制），调用会失败并返回错误。

**Programmatic Dependent Launch**
这是 CUDA 从计算能力 9.0 开始引入的执行模型扩展。它包含三个核心概念：
- **Primary kernel**：先被提交到 stream 的 kernel，负责产生下游需要消费的数据。
- **Secondary kernel**：后被提交到同一 stream 的 kernel，它在逻辑上依赖 primary kernel，但部分工作（preamble）可以与之并发。
- **Programmatic Stream Serialization**：通过启动属性 `cudaLaunchAttributeProgrammaticStreamSerialization` 告诉 driver，secondary kernel 可以被提前调度。

**Preamble Section**
Secondary kernel 中不依赖 primary kernel 结果的那部分代码。这是产生并发收益的关键所在。如果 secondary kernel 从第一条指令起就必须读取 primary kernel 的输出，那么 Programmatic Dependent Launch 就不会带来任何性能好处。

**Implicit Trigger**
如果 primary kernel 没有显式调用 `cudaTriggerProgrammaticLaunchCompletion()`，driver 会在 primary kernel 的所有 thread block 退出后隐式触发完成信号。这保证了即使开发者忘记插入 trigger，依赖关系仍然保持正确，但并发机会可能丧失。

## API / 机制详解

### 1. cudaLaunchCooperativeKernel

**作用**
启动一个使用 cooperative groups 的 kernel，确保 grid 内的所有 thread block 可以同时被调度并参与 `grid.sync()` 等跨 block 同步操作。

**函数签名（C API）**
```cpp
cudaError_t cudaLaunchCooperativeKernel(
    const void *func,
    dim3 gridDim,
    dim3 blockDim,
    void **args,
    size_t sharedMem,
    cudaStream_t stream
);
```

**关键参数说明**
- `func`：指向 kernel 函数的指针。在 C++ 中通常通过 `(void*)kernel_name` 传入。
- `args`：`void*` 指针数组，每个元素指向一个 kernel 参数的地址。注意这与 `<<< >>>>` 语法的直接传参不同，需要显式构造指针数组。
- `sharedMem`：动态共享内存字节数。如果 kernel 使用了 `extern __shared__` 数组，必须在这里指定大小。

**前置条件**
1. 设备必须支持 cooperative launch。通过查询 `cudaDevAttrCooperativeLaunch` 设备属性确认。
2. 计算能力 >= 6.0（Pascal 及更新架构）。
3. 平台限制：
   - Linux 平台且不使用 MPS（Multi-Process Service）；或
   - Linux 平台使用 MPS，但设备计算能力 >= 7.0；或
   - 最新的 Windows 平台。
4. Grid 大小不能超过设备同时驻留 block 的最大数量。即 `gridDim.x * gridDim.y * gridDim.z` 必须 <= `maxBlocksPerMultiprocessor * multiProcessorCount`。超出此限制会导致 `cudaErrorCooperativeLaunchTooLarge`。

**调用后得到什么**
如果返回 `cudaSuccess`，则所有 block 已经被原子性地提交到设备，kernel 可以安全执行 `cg::this_grid().sync()`。如果返回错误，kernel 不会启动，也不会产生部分执行的结果。

**与传统 `<<< >>>>` 启动的区别**
- `<<< >>>>` 只保证单个 block 内的 `__syncthreads()` 有效；`cudaLaunchCooperativeKernel` 额外保证了 grid 级别的同步有效。
- `<<< >>>>` 的 block 调度是流水式的，设备可以分批启动 block；cooperative launch 要求所有 block 同时获得调度机会（尽管不一定同时全部执行完）。
- 参数传递方式不同：`<<< >>>>` 直接传值，`cudaLaunchCooperativeKernel` 要求 `void**` 数组。

### 2. cudaTriggerProgrammaticLaunchCompletion

**作用**
由 primary kernel 的设备端代码调用，向 driver 发出信号：primary kernel 已经进展到了足以让 secondary kernel 提前启动的阶段。

**调用时机**
当 primary kernel 完成了所有 secondary kernel 的 preamble 阶段可能依赖的内存写入之后，在所有 thread block 中调用。

**关键语义**
- 这是一个**所有 block 都必须执行**的集体操作（collective operation）。如果某些 block 没有执行到这条指令，driver 会等待这些 block 自然退出后才隐式触发。
- 调用之后，primary kernel 的线程**不会停止**，而是继续执行后续工作。因此后续代码不能与 secondary kernel 的 dependent 阶段发生数据竞争。
- 它只是给 driver 一个"可以提前启动 secondary"的许可，不保证 secondary 一定会在 primary 结束前启动。并发是机会性的（opportunistic）。

**常见错误**
- 只在部分 block 中调用：虽然不会导致崩溃，但会丧失并发机会，因为 driver 必须等待未调用的 block 退出。
- 在调用之前写入的数据被 secondary kernel 的 preamble 错误读取：如果 secondary kernel 的 preamble 阶段意外访问了 primary kernel 的输出缓冲区，而 primary kernel 尚未完成写入，将导致数据竞争。这是逻辑错误，API 本身不会报错。

### 3. cudaGridDependencySynchronize

**作用**
由 secondary kernel 的设备端代码调用，建立一个硬性屏障：在此点之前的所有代码（preamble）可以安全地与 primary kernel 并发执行；在此点之后（dependent 阶段）的代码必须等到所有依赖的 primary kernel 完全结束，并且其全局内存写入对所有线程可见后才能继续。

**调用时机**
在 secondary kernel 中，所有需要读取 primary kernel 结果的代码之前。

**关键语义**
- 这是 secondary kernel 内部的分界点：之前是 independent work，之后是 dependent work。
- 它同时承担了内存可见性屏障的功能：不仅同步执行进度，还保证 primary kernel 的 global memory 写入已经 flush 到对 secondary kernel 可见。
- 如果 secondary kernel 没有配置 Programmatic Dependent Launch 属性，调用此函数在语义上等同于等待 stream 中之前所有 kernel 完成（因为默认 stream 顺序已经保证了这一点）。但在配置了 Dependent Launch 时，它是确保正确性的必需操作。

**常见错误**
- 忘记调用：如果 secondary kernel 在启动时配置了 `cudaLaunchAttributeProgrammaticStreamSerialization`，但又没有调用 `cudaGridDependencySynchronize` 或其他等效同步手段就去读取 primary kernel 的数据，将读取到过期数据。
- 调用位置过早：如果过早调用，preamble 阶段就没有机会与 primary kernel 重叠，丧失了性能收益。

### 4. cudaLaunchKernelEx 与 Programmatic Stream Serialization

**作用**
`cudaLaunchKernelEx` 是 CUDA 11.6+ 引入的可扩展启动 API，允许在启动 kernel 时附加额外的属性。Programmatic Dependent Launch 正是通过这一 API 的 `cudaLaunchAttributeProgrammaticStreamSerialization` 属性来配置的。

**配置方式**
```cpp
cudaLaunchConfig_t config = {};
config.gridDim = ...;
config.blockDim = ...;
config.stream = stream;
config.numAttrs = 1;

cudaLaunchAttribute attr[1];
attr[0].id = cudaLaunchAttributeProgrammaticStreamSerialization;
attr[0].val.programmaticStreamSerializationAllowed = 1;
config.attrs = attr;

cudaLaunchKernelEx(&config, secondary_kernel, args...);
```

**关键参数说明**
- `cudaLaunchAttributeProgrammaticStreamSerialization`：id 值为 6。设置 `programmaticStreamSerializationAllowed = 1` 表示允许 driver 在 primary kernel 尚未完全结束时提前启动此 kernel。
- `config.stream`：必须与 primary kernel 的提交 stream 相同，否则依赖关系无法建立。

**与传统启动的区别**
- 传统 `<<< >>>>` 或 `cudaLaunchKernel` 不提供启动属性扩展能力。
- `cudaLaunchKernelEx` 支持模板参数包传参（C++11 起），也可以像 `cudaLaunchCooperativeKernel` 一样通过 `void**` 传参（C API 为 `cudaLaunchKernelExC`）。

## 典型工作流程 / 调用顺序

### Cooperative Launch 工作流程

1. **查询设备能力**：调用 `cudaDeviceGetAttribute(&supports, cudaDevAttrCooperativeLaunch, dev)` 确认支持。
2. **计算 Occupancy**：使用 `cudaOccupancyMaxActiveBlocksPerMultiprocessor` 计算每个 SM 能同时运行的最大 block 数，乘以 SM 数量得到最大合法 grid 大小。
3. **准备数据**：分配 device 内存，将输入数据拷贝到 device。
4. **构造参数数组**：`void* args[] = {&d_input, &d_output, &n};`
5. **启动 kernel**：`cudaLaunchCooperativeKernel((void*)kernel, gridDim, blockDim, args, sharedMem, stream);`
6. **设备端执行**：kernel 内部通过 `cg::this_grid().sync()` 完成全局同步。
7. **取回结果**：`cudaMemcpy` 或统一内存直接读取。

### Programmatic Dependent Launch 工作流程

1. **查询计算能力**：确认 `major * 10 + minor >= 90`。
2. **准备内存**：为 primary kernel 的输出和 secondary kernel 的输入/输出分配 device 内存。
3. **提交 primary kernel**：使用普通方式启动，如 `primary_kernel<<<grid, block, 0, stream>>>(d_data, n);`
4. **配置 secondary kernel 的启动属性**：填充 `cudaLaunchConfig_t` 和 `cudaLaunchAttributeProgrammaticStreamSerialization`。
5. **提交 secondary kernel**：`cudaLaunchKernelEx(&config, secondary_kernel, d_data, d_output, n);`
6. **设备端执行（primary）**：完成写入后调用 `cudaTriggerProgrammaticLaunchCompletion()`，然后继续执行其他无关工作。
7. **设备端执行（secondary）**：先执行 preamble（独立工作），然后调用 `cudaGridDependencySynchronize()`，最后执行 dependent 工作。
8. **同步 stream**：`cudaStreamSynchronize(stream)` 等待两者都完成。

## 关键限制、边界条件与兼容性

### Cooperative Launch 限制

- **计算能力下限**：仅 Pascal（sm_60）及以上支持。Maxwell 及更早架构不支持。
- **平台限制**：Linux 无 MPS 时全支持；Linux 有 MPS 时需要 Volta（sm_70）及以上；Windows 需要较新版本。
- **Grid 大小上限**：不能超过设备同时驻留 block 的总容量。如果尝试启动过多的 block，`cudaLaunchCooperativeKernel` 会明确报错，而不是静默截断。
- **原子性保证**：如果 API 返回成功，所有 block 一定会被调度；不会出现在传统启动中可能出现的"部分 block 因资源不足被推迟"的情况。
- **Multi-device 支持移除**：CUDA 13 起，多设备 cooperative launch API 已被移除。现在 `cudaLaunchCooperativeKernel` 仅支持单设备。

### Programmatic Dependent Launch 限制

- **严格的计算能力要求**：必须 sm_90（Hopper）或更高。这是硬门槛，不是通过软件模拟可以绕过的。
- **Stream 一致性**：primary 和 secondary 必须在同一个 stream 中提交。跨 stream 的依赖需要通过传统同步机制（如 event）管理，不能通过此机制加速。
- **内存可见性不自动保证**：即使 secondary kernel 被提前启动，它的 thread block 可能在 primary kernel 的数据可见之前就开始执行。因此 `cudaGridDependencySynchronize`（或等效 PTX 指令）是强制性的。
- **并发是机会性的**：driver 和硬件可能由于资源限制、调度策略或其他原因，选择不并发执行。应用程序不能依赖这种并发来保证正确性或避免死锁。如果代码逻辑假设 secondary 一定会在 primary 结束前启动，可能导致死锁。
- **Implicit trigger 的语义**：如果 primary kernel 忘记调用 trigger，driver 会在 kernel 退出后隐式触发。这保证了正确性，但此时 secondary kernel 的 preamble 已经与 primary kernel 完全串行，失去了并发意义。

## 常见陷阱与调试建议

1. **混淆 grid.sync() 与 __syncthreads()**
   `grid.sync()` 只能在使用 `cudaLaunchCooperativeKernel` 启动的 kernel 中工作。如果在普通 `<<< >>>>` 启动的 kernel 中调用 `cg::this_grid().sync()`，行为是未定义的，通常会导致死锁或非法指令错误。

2. ** cooperative kernel 的 grid 尺寸过大**
   务必先用 occupancy API 计算最大合法 grid 尺寸。一个常见错误是直接把数据量除以 block size 得到 grid size，而忽略了 SM 数量和每个 SM 的最大 block 数限制。

3. **Programmatic Dependent Launch 的 device 端代码在旧 arch 上编译失败**
   `cudaTriggerProgrammaticLaunchCompletion()` 和 `cudaGridDependencySynchronize()` 在 PTX 层面生成 `griddepcontrol` 指令，该指令要求 `.target sm_90`。如果在 `nvcc -arch=sm_61` 下编译包含这些调用的 kernel，ptxas 会直接报错。解决方案是使用 `#if __CUDA_ARCH__ >= 900` 进行条件编译，或者将代码拆分为两个文件分别针对不同架构编译。

4. **Secondary kernel 没有调用 cudaGridDependencySynchronize**
   这是最危险的逻辑错误。由于 secondary kernel 可能被提前启动，其 thread block 完全可能在 primary kernel 的 global memory 写入完成前就开始执行 dependent 代码。如果此时读取输入数据，会得到旧值或半写入状态，且 CUDA 工具通常不会报告内存错误（因为地址本身是合法的）。

5. **错误地假设并发一定会发生**
   NVIDIA 文档明确指出并发是"opportunistic"（机会性的）。性能测试时如果观察到两者完全串行，不一定代表代码有 bug；可能只是当前 GPU 负载、资源限制或调度策略导致。不要编写在并发不发生时会死锁的代码。

6. **Stream 混用**
   如果将 primary kernel 提交到 stream A，而 secondary kernel 提交到 stream B，即使 secondary kernel 配置了 Programmatic Stream Serialization，也不会形成依赖加速关系。默认的 stream 顺序语义会保证正确性，但不会提前启动。

## 一个最小可运行示例的说明

本目录下的 `chapter_demo.cu` 是一个为 GTX 1060（sm_61, CUDA 12.8）设计的兼容示例，配套的 `Makefile` 使用 `nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2` 进行编译。

### 示例设计思路

由于当前设备（sm_61）不支持 Programmatic Dependent Launch（需要 sm_90+），但支持 Cooperative Launch（需要 sm_60+），示例采用了"双路径"设计：

1. ** Cooperative Launch 路径（必执行）**：演示 `cudaLaunchCooperativeKernel` 和 `cg::this_grid().sync()` 的用法。Kernel 逻辑是一个简单的数组归约：每个 block 内部做 shared memory 归约，然后通过 `grid.sync()` 全局同步，最后由 block 0 汇总所有 block 的部分和。输入为 1..1024，期望总和 524800，示例会验证结果并打印 `PASS`。

2. **Programmatic Dependent Launch 路径（条件执行）**：代码中完整定义了 `primary_kernel` 和 `secondary_kernel`，并在主机端编写了通过 `cudaLaunchKernelEx` 配置 `cudaLaunchAttributeProgrammaticStreamSerialization` 的启动逻辑。但在设备端，这些 kernel 内部的真实 API 调用（`cudaTriggerProgrammaticLaunchCompletion` 和 `cudaGridDependencySynchronize`）被 `#if __CUDA_ARCH__ >= 900` 条件编译保护。这样在 `nvcc -arch=sm_61` 编译时，这些函数体退化为空操作（no-op），ptxas 不会尝试生成 sm_90 专有的 `griddepcontrol` 指令，从而确保编译通过。

### sm_61 上的降级与兼容处理

- **编译时兼容**：通过 `__CUDA_ARCH__` 宏在设备代码中隔离 sm_90 专属指令，使同一份源代码可以用 `-arch=sm_61` 成功编译。
- **运行时检测**：主机端通过 `cudaDevAttrComputeCapabilityMajor/Minor` 查询计算能力。如果小于 9.0，则跳过 dependent launch 路径的执行，并打印详细的兼容性说明，告知用户该功能在当前硬件上不可用以及代码是如何做到向前兼容的。
- **功能降级**：在 sm_61 上，示例实际演示的是 cooperative launch（4.4.8 节内容），这与 Programmatic Dependent Launch（4.5 节内容）同属于"执行模型高级控制"主题，且前者是后者在旧硬件上能实际运行的最接近替代方案。

### 运行效果

在 GTX 1060 上运行会看到：
- 计算能力 6.1 的识别信息
- Cooperative launch 支持状态：yes
- Programmatic dependent launch 支持状态：no
- Cooperative kernel 的归约结果：524800.0 -- PASS
- 一段兼容性说明，解释为什么跳过了 dependent launch 测试

如果同一程序被编译到 sm_90+ 设备上运行（例如保留 `-arch=sm_61` 但运行在向前兼容的硬件上，或重新编译为 `-arch=sm_90`），则会额外执行 dependent launch 路径，验证 `primary_kernel` 和 `secondary_kernel` 的协同执行结果。
