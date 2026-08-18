## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/driver-api.html>

## 核心概念

CUDA Driver API 是比 CUDA Runtime API 更底层的接口。Runtime API 实际上是在 Driver API 之上构建的封装层。大多数应用无需直接使用 Driver API 即可达到最佳性能，但某些高级功能（如虚拟内存管理）仅通过 Driver API 暴露，且新接口有时会先在 Driver API 中提供。

### 1. Driver API 的基本特征

- **库实现**：所有入口点都位于 `cuda` 动态库（`cuda.so` / `cuda.dll`）中，安装设备驱动时即被复制到系统。
- **命名前缀**：所有函数均以 `cu` 开头（如 `cuInit`、`cuMemAlloc`）。
- **句柄式、命令式 API**：大多数对象通过不透明句柄引用，函数通过句柄操作对象。

### 2. 核心对象模型

| 对象 | 句柄 | 说明 |
|------|------|------|
| Device | `CUdevice` | 支持 CUDA 的物理设备 |
| Context | `CUcontext` | 类似于 CPU 进程，封装了该上下文内所有资源与操作 |
| Module | `CUmodule` | 类似于动态链接库（DLL），包含设备代码与数据 |
| Function | `CUfunction` | 内核函数句柄 |
| 堆内存 | `CUdeviceptr` | 设备内存指针（实质为整数类型） |
| CUDA Array | `CUarray` | 设备上的一维/二维数据容器，用于纹理/表面访问 |
| Texture Object | `CUtexref` | 描述如何解释纹理内存数据 |
| Surface Reference | `CUsurfref` | 描述如何读写 CUDA Array |
| Stream | `CUstream` | 描述 CUDA 流 |
| Event | `CUevent` | 描述 CUDA 事件 |

### 3. 上下文（Context）

- **类比 CPU 进程**：Context 内所有资源（模块、纹理引用、设备指针等）在 Context 销毁时由系统自动回收。
- **独立地址空间**：不同 Context 的 `CUdeviceptr` 指向完全不同的物理内存位置。
- **栈式 current context**：一个主机线程同一时刻只能有一个当前 Context。`cuCtxCreate` 将新 Context 压入线程栈顶；`cuCtxPopCurrent` 可将其弹出并恢复之前的 Context。弹出的 Context 是“浮动”的，可被任意主机线程重新压入。
- **引用计数**：Context 维护 usage count。`cuCtxCreate` 创建时计数为 1；`cuCtxAttach` 递增，`cuCtxDetach` / `cuCtxDestroy` 递减。计数为 0 时销毁。该机制便于多个第三方库共享同一个 Context。
- **Primary Context**：Runtime API 隐式创建的 Context 称为 Primary Context，可通过 `cuDevicePrimaryCtxRetain` 在 Driver API 中获取和管理。

### 4. 模块（Module）

- **PTX 与二进制码**：Module 可通过 `cuModuleLoad` 从 `.ptx` 文件或二进制文件加载。若应用希望兼容未来架构，**必须加载 PTX 代码**（而非二进制 cubin），因为二进制码是架构相关的，而 PTX 会在加载时由设备驱动即时编译（JIT）为对应架构的二进制码。
- **JIT 编译选项**：`cuModuleLoadDataEx` 允许通过 `CUjit_option` 数组传递编译选项，如错误日志缓冲区、目标架构、日志详细级别等。
- **链接器**：`cuLinkCreate` / `cuLinkAddData` / `cuLinkComplete` 支持将多段 PTX 代码链接为一个 cubin，再加载为 Module。
- **多线程加载加速**：通过 `CU_JIT_BINARY_LOADER_THREAD_COUNT` 选项可利用多线程加速 cubin 加载。

### 5. 内核执行（Kernel Execution）

- `cuLaunchKernel` 用于启动内核，需显式指定 grid/block 维度、共享内存大小、流、参数数组等。
- **参数传递方式**：
  1. **指针数组**：`void* args[] = { &d_A, &d_B, &d_C, &N };` 第 n 个指针指向第 n 个参数的内存副本。
  2. **额外选项（extra）**：通过 `CU_LAUNCH_PARAM_BUFFER_POINTER` 传递一个连续参数缓冲区，参数必须按设备端的对齐要求正确偏移。

### 6. 参数对齐要求

设备端的对齐规则与主机端基本一致，但存在以下关键差异：
- 内建向量类型（如 `float4`）在设备端的对齐要求见 CUDA 文档表 42（`float4` 为 16 字节，`float2` 为 8 字节）。
- `double`、`long long`、`long`（64 位系统）在设备端始终按双字（8 字节）对齐。若主机编译器使用 `-mno-align-double` 将其按单字对齐，则主机与设备的对齐要求不一致。
- `CUdeviceptr` 虽是整数类型，但语义上是指针，其对齐要求为 `__alignof(void*)`。
- **结构体对齐**：结构体的对齐要求等于其字段对齐要求的最大值。若结构体包含向量类型或 `CUdeviceptr`，其主机端与设备端的填充（padding）可能不同。

### 7. Runtime 与 Driver API 的互操作性

- 若通过 Driver API 创建并设为当前的 Context，后续 Runtime API 调用将复用该 Context，而不会再创建新的 Primary Context。
- 若 Runtime 已初始化，可通过 `cuCtxGetCurrent` 获取其 Primary Context，供后续 Driver API 使用。
- 内存分配可混用：`CUdeviceptr` 与常规指针（如 `float*`）可相互强制类型转换。这意味着基于 Driver API 的应用可以调用基于 Runtime API 的库（如 cuBLAS、cuFFT）。

---

## 关键 API/函数/宏

### 初始化与设备管理

| API | 用途 |
|-----|------|
| `cuInit(unsigned int Flags)` | 初始化 Driver API，任何其他 Driver API 调用之前必须先调用。 |
| `cuDeviceGetCount(int* count)` | 获取系统中支持 CUDA 的设备数量。 |
| `cuDeviceGet(CUdevice* device, int ordinal)` | 获取指定序号的设备句柄。 |
| `cuDeviceGetName(char* name, int len, CUdevice dev)` | 获取设备名称。 |
| `cuDeviceGetAttribute(int* value, CUdevice_attribute attrib, CUdevice dev)` | 查询设备属性（如计算能力 major/minor）。 |
| `cuDevicePrimaryCtxRetain(CUcontext* pctx, CUdevice dev)` | 获取 Runtime 管理的 Primary Context。 |

### Context 管理

| API | 用途 |
|-----|------|
| `cuCtxCreate(CUcontext* pctx, unsigned int flags, CUdevice dev)` | 为指定设备创建 Context，并设为当前线程的 current context。 |
| `cuCtxDestroy(CUcontext ctx)` | 销毁 Context（递减引用计数）。 |
| `cuCtxAttach(CUcontext* pctx, unsigned int flags)` | 递增 Context 的 usage count。 |
| `cuCtxDetach(CUcontext ctx)` | 递减 Context 的 usage count。 |
| `cuCtxPushCurrent(CUcontext ctx)` | 将 Context 压入当前线程的 context 栈。 |
| `cuCtxPopCurrent(CUcontext* pctx)` | 弹出栈顶 Context，恢复之前的 Context。 |
| `cuCtxGetCurrent(CUcontext* pctx)` | 获取当前线程的 current context。 |
| `cuCtxSynchronize()` | 阻塞当前线程，直到当前 Context 中所有先前任务完成。 |

### Module 管理

| API | 用途 |
|-----|------|
| `cuModuleLoad(CUmodule* module, const char* fname)` | 从文件（PTX 或 cubin）加载模块。 |
| `cuModuleLoadData(CUmodule* module, const void* image)` | 从内存中的 PTX/cubin 数据加载模块。 |
| `cuModuleLoadDataEx(CUmodule* module, const void* image, unsigned int numOptions, CUjit_option* options, void** optionValues)` | 从内存加载模块，并传递 JIT 编译选项。 |
| `cuModuleUnload(CUmodule hmod)` | 卸载模块。 |
| `cuModuleGetFunction(CUfunction* hfunc, CUmodule hmod, const char* name)` | 从模块中获取指定内核函数的句柄。 |

### 链接器（Linker）API

| API | 用途 |
|-----|------|
| `cuLinkCreate(unsigned int numOptions, CUjit_option* options, void** optionValues, CUlinkState* stateOut)` | 创建链接器状态对象。 |
| `cuLinkAddData(CUlinkState state, CUjitInputType type, void* data, size_t size, const char* name, unsigned int numOptions, CUjit_option* options, void** optionValues)` | 向链接器添加一段输入数据（如 PTX）。 |
| `cuLinkComplete(CUlinkState state, void** cubinOut, size_t* sizeOut)` | 完成链接，输出 cubin 及其大小。 |
| `cuLinkDestroy(CUlinkState state)` | 销毁链接器状态。 |

### 内存管理

| API | 用途 |
|-----|------|
| `cuMemAlloc(CUdeviceptr* dptr, size_t bytesize)` | 在设备上分配线性内存。 |
| `cuMemFree(CUdeviceptr dptr)` | 释放设备内存。 |
| `cuMemcpyHtoD(CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount)` | 从主机内存复制到设备内存。 |
| `cuMemcpyDtoH(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount)` | 从设备内存复制到主机内存。 |

### 内核执行

| API | 用途 |
|-----|------|
| `cuLaunchKernel(CUfunction f, unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ, unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ, unsigned int sharedMemBytes, CUstream hStream, void** kernelParams, void** extra)` | 启动内核。`kernelParams` 为参数指针数组；`extra` 为额外选项（如连续参数缓冲区）。 |

### JIT 编译选项枚举（`CUjit_option`）

| 宏/枚举 | 用途 |
|---------|------|
| `CU_JIT_ERROR_LOG_BUFFER` | 指定 JIT 编译错误日志缓冲区。 |
| `CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES` | 指定错误日志缓冲区大小。 |
| `CU_JIT_INFO_LOG_BUFFER` | 指定 JIT 编译信息日志缓冲区。 |
| `CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES` | 指定信息日志缓冲区大小。 |
| `CU_JIT_TARGET_FROM_CUCONTEXT` | 从当前 Context 推断目标架构。 |
| `CU_JIT_LOG_VERBOSE` | 启用详细日志输出。 |
| `CU_JIT_WALL_TIME` | 输出 JIT 编译的墙钟时间。 |
| `CU_JIT_BINARY_LOADER_THREAD_COUNT` | 指定二进制加载时使用的线程数（0 表示使用所有 CPU 核心）。 |

### 内核启动额外选项宏

| 宏 | 用途 |
|----|------|
| `CU_LAUNCH_PARAM_BUFFER_POINTER` | 指定连续参数缓冲区的指针。 |
| `CU_LAUNCH_PARAM_BUFFER_SIZE` | 指定参数缓冲区的大小。 |
| `CU_LAUNCH_PARAM_END` | 标记 `extra` 数组的结束。 |

### 对齐辅助宏（文档示例）

| 宏 | 用途 |
|----|------|
| `ALIGN_UP(offset, alignment)` | 将偏移量向上对齐到指定对齐边界。 |
| `ADD_TO_PARAM_BUFFER(value, alignment)` | 将值按对齐要求拷贝到参数缓冲区，并更新偏移量。 |

---

## 注意事项、限制条件与常见陷阱

### 1. 必须先调用 `cuInit(0)`

在任何 Driver API 函数调用之前，必须调用 `cuInit(0)` 完成初始化。若遗漏，后续调用行为未定义或返回错误。

### 2. 必须有 current context

绝大多数 Driver API 函数（除设备枚举和 Context 管理外）要求调用线程有一个有效的 current context。若缺失，将返回 `CUDA_ERROR_INVALID_CONTEXT`。

### 3. PTX vs 二进制码的前向兼容性

- **二进制 cubin** 与特定架构强绑定，无法在新架构上运行。
- **PTX** 是 JIT 编译的输入，驱动程序会在加载时将其编译为当前 GPU 架构的二进制码。因此，为保证应用能在未来 GPU 上运行，Module 必须加载 PTX 代码。

### 4. 参数传递的两种模式不可混用

`cuLaunchKernel` 的 `kernelParams` 和 `extra` 参数是互斥的，只能使用其中一种方式传递参数。若同时使用，行为未定义。

### 5. 主机端与设备端的对齐差异

- 使用 `extra` 方式传递参数时，必须严格保证参数缓冲区中的每个参数按**设备端**的对齐要求放置。
- 若主机编译器对 `double` / `long long` 使用单字对齐（如 gcc 的 `-mno-align-double`），而设备端要求双字对齐，则直接按主机端 `sizeof` / `__alignof` 构建的参数缓冲区可能在设备端触发未对齐访问。
- `CUdeviceptr` 的对齐要求不是 `__alignof(CUdeviceptr)`（该类型为整数），而是 `__alignof(void*)`。
- 包含向量类型的结构体在主机端和设备端的填充可能不同，跨 API 传递此类结构体时需格外谨慎。

### 6. Context 引用计数与生命周期

- `cuCtxCreate` 创建 Context 后 usage count 为 1。
- 若库 A 和库 B 共享同一 Context，应分别使用 `cuCtxAttach` / `cuCtxDetach` 管理引用，避免某一方提前销毁 Context 导致另一方崩溃。
- `cuCtxDestroy` 与 `cuCtxDetach` 在 usage count 降为 0 时都会触发销毁。

### 7. Driver API 与 Runtime API 混用的指针转换

`CUdeviceptr` 与 `void*` / `float*` 等可以相互强制类型转换，但本质上 `CUdeviceptr` 是整数类型。转换仅在**同一 Context 内**有效，跨 Context 的指针值不代表相同的物理地址。

### 8. `cuModuleLoadData` 与字符串常量

向 `cuModuleLoadData` / `cuModuleLoadDataEx` 传递 PTX 代码时，数据必须是有效的、以 null 结尾的 C 字符串（对于 PTX），且其生命周期需保持到加载完成。若将 PTX 嵌入代码中为 `const char[]`，通常可安全使用。

---

## 底层原理说明

### Driver API 的调用路径

CUDA Runtime API 本质上是对 Driver API 的封装。例如 `cudaMalloc` 内部会调用 `cuMemAlloc`，`cudaLaunchKernel` 内部会调用 `cuLaunchKernel`。Driver API 直接暴露这些底层操作，给予开发者更精细的控制权，但也要求开发者手动管理 Context、Module 和内核参数对齐。

### Context 与地址空间隔离

CUDA Context 在驱动层面对应一个独立的 GPU 执行环境，包含：
- 独立的虚拟地址空间（VA space）
- 独立的模块符号表
- 独立的流、事件、纹理引用状态

这类似于操作系统中的进程隔离机制。当多个主机线程或第三方库需要共享 GPU 时，它们可以通过 `cuCtxPushCurrent` / `cuCtxPopCurrent` 在同一个线程上切换 Context，或通过引用计数共享同一个 Context。

### PTX JIT 编译机制

PTX（Parallel Thread Execution）是一种虚拟 ISA。当调用 `cuModuleLoadDataEx` 加载 PTX 时，NVIDIA 驱动中的 JIT 编译器（nvvm 或 ptxas）会将 PTX 翻译成目标 GPU 架构的 SASS（机器码）。这个过程发生在主机端，在模块加载完成后，内核即可直接以 SASS 形式在 SM 上调度执行。由于 JIT 发生在加载时，因此 PTX 可以兼容未来新架构（只要新架构的驱动支持该 PTX 版本），而无需重新编译应用。

### 内核参数布局与 ABI

`cuLaunchKernel` 将参数从主机端复制到设备端的常数内存或参数内存区域。设备端内核函数按照 CUDA ABI 规定的对齐规则从该区域读取参数。若主机端构建的参数缓冲区未满足这些对齐规则，设备端读取时可能发生未对齐访问，导致性能下降或计算结果错误（在某些旧架构上甚至可能引发异常）。

### Warp 调度与内核启动（本章未深入）

本章主要聚焦于 Driver API 的接口与资源管理，未详细讨论 SM 内部的 Warp 调度。但就内核执行而言，`cuLaunchKernel` 提交的内核会被驱动转换为一系列 grid/block 配置，最终由硬件按 Warp（32 线程）为单位调度到 SM 上执行。GTX 1060（sm_61）每个 SM 可同时驻留多个 Warp，通过零开销线程切换隐藏延迟。
