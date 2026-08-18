## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/driver-api.html>


本章首先介绍了内核启动前的属性配置（`cudaFuncSetAttribute`），随后重点阐述了 CUDA Driver API 的设计思想、核心对象、初始化流程、上下文管理、模块加载与链接，以及内核启动方式。Driver API 是比 Runtime API 更低层的接口，适合需要提前使用新特性或需要虚拟内存管理等高级功能的场景。

---

## 内核属性与动态共享内存

在详细介绍 Driver API 之前，本章先说明了 `cudaFuncSetAttribute` 的用法：

- 只能对用 `__global__` 声明的函数调用 `cudaFuncSetAttribute`。
- 该调用只是“提示（hint）”，驱动在必要时可以选择不同的配置。
- 与 `cudaFuncSetCacheConfig` 相比，`cudaFuncSetAttribute` 更受推荐，因为前者对 shared/L1 的配比是硬性要求，混用不同配置的内核会导致不必要的串行重配置；而后者允许驱动灵活调整，避免 thrashing。
- 若内核每个 block 需要超过 48 KB 的共享内存，必须使用动态共享内存（`extern __shared__`），并在主机端显式调用 `cudaFuncSetAttribute(MyKernel, cudaFuncAttributeMaxDynamicSharedMemorySize, maxbytes)` 进行 opt-in。

---

## CUDA Driver API 详解

### Driver API 与 Runtime API 的关系

- Runtime API 构建在 Driver API 之上。大多数应用永远不需要直接使用 Driver API。
- 但新接口有时会先在 Driver API 中提供；一些高级接口（如虚拟内存管理）只在 Driver API 中暴露。
- Driver API 的入口点全部以 `cu` 为前缀，属于基于句柄的命令式 API。

### 核心对象

| 对象 | 句柄 | 说明 |
|------|------|------|
| 设备 | `CUdevice` | 支持 CUDA 的 GPU |
| 上下文 | `CUcontext` | 类似于 CPU 进程，封装所有资源与操作 |
| 模块 | `CUmodule` | 类似于动态链接库，包含设备代码与数据 |
| 函数 | `CUfunction` | 内核（kernel） |
| 设备内存指针 | `CUdeviceptr` | 指向设备全局内存的整数句柄 |
| CUDA 数组 | `CUarray` | 一维或二维数据的设备端容器，供纹理/表面引用读取 |
| 纹理引用 | `CUtexref` | 描述如何解释纹理内存数据 |
| 表面引用 | `CUsurfref` | 描述如何读写 CUDA 数组 |
| 流 | `CUstream` | 描述 CUDA 流 |
| 事件 | `CUevent` | 描述 CUDA 事件 |

### 初始化与上下文

1. **初始化**：在任何 Driver API 函数调用之前，必须先调用 `cuInit(0)`。
2. **创建上下文**：使用 `cuCtxCreate` 将上下文绑定到特定设备，并使其对当前主机线程生效（current）。
3. **上下文栈**：每个主机线程维护一个当前上下文栈。
   - `cuCtxCreate` 将新上下文压入栈顶。
   - `cuCtxPopCurrent` 可将当前上下文弹出，使其变为“浮动”状态，之后可用 `cuCtxPushCurrent` 将其压入任意线程的栈。
4. **引用计数**：上下文维护使用计数。
   - `cuCtxCreate` 创建时计数为 1。
   - `cuCtxAttach` 增加计数，`cuCtxDetach` 减少计数。
   - 计数归零时，上下文被销毁（通过 `cuCtxDetach` 或 `cuCtxDestroy`）。
5. **与 Runtime 互操作**：可通过 `cuDevicePrimaryCtxRetain` 访问 Runtime 管理的主上下文（primary context）。

### 模块（Module）

模块是由 `nvcc` 输出的可动态加载的设备代码包，类似于 Windows 的 DLL。

#### 关键 API

- `cuModuleLoad(&cuModule, "myModule.ptx")`：从文件加载模块。
- `cuModuleGetFunction(&myKernel, cuModule, "MyKernel")`：从模块中获取内核句柄。
- `cuModuleLoadDataEx`：从内存中的 PTX/CUBIN 加载模块，并可指定 JIT 编译选项（如错误日志缓冲区、目标架构等）。
- `cuLinkCreate / cuLinkAddData / cuLinkComplete / cuLinkDestroy`：将多段 PTX 代码链接成一个 CUBIN，再加载为模块。

#### JIT 编译与链接选项示例

常用 `CUjit_option` 包括：

- `CU_JIT_ERROR_LOG_BUFFER` / `CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES`：捕获编译/链接错误。
- `CU_JIT_INFO_LOG_BUFFER` / `CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES`：捕获信息日志。
- `CU_JIT_WALL_TIME`：测量链接耗时。
- `CU_JIT_TARGET_FROM_CUCONTEXT`：从当前上下文推断目标架构。
- `CU_JIT_LOG_VERBOSE`：启用详细日志。
- `CU_JIT_BINARY_LOADER_THREAD_COUNT`：指定加载 CUBIN 时使用的线程数（设为 0 表示使用与 CPU 核心数相同的线程数）。

> **PTX 与二进制代码的选择**：若应用希望在未来架构上运行，必须加载 **PTX** 而非二进制代码。二进制代码是架构相关的，而 PTX 在加载时由设备驱动即时编译为当前架构的二进制代码。

### 内核执行（Kernel Execution）

使用 `cuLaunchKernel` 启动内核，其签名如下（概念上）：

```cpp
cuLaunchKernel(CUfunction f,
               unsigned int gridDimX, gridDimY, gridDimZ,
               unsigned int blockDimX, blockDimY, blockDimZ,
               unsigned int sharedMemBytes,
               CUstream hStream,
               void **kernelParams,
               void **extra);
```

#### 参数传递方式

1. **指针数组（`kernelParams`）**：第 n 个指针指向第 n 个参数的内存副本。这是最常见、最方便的方式。
2. **Extra 选项（`extra`）**：通过 `CU_LAUNCH_PARAM_BUFFER_POINTER` 传递一个已打包的参数缓冲区，参数在缓冲区中必须按照设备代码的对齐要求进行偏移。

#### 参数对齐要求

- 内建向量类型的对齐要求见 CUDA 文档中的表格。
- 其他基本类型的对齐要求与主机代码一致，可通过 `__alignof()` 获取。
- 例外：若主机编译器将 `double`、`long long`（以及 64 位系统上的 `long`）按单字对齐（如 gcc 的 `-mno-align-double`），设备代码中这些类型仍按双字对齐。
- `CUdeviceptr` 虽然是整数，但本质是指针，对齐要求为 `__alignof(void*)`。
- 结构体的对齐等于其字段对齐要求的最大值。

示例中提供了 `ALIGN_UP` 和 `ADD_TO_PARAM_BUFFER` 宏，用于手动构建满足对齐要求的参数缓冲区。

---

## 核心概念总结

1. **层级关系**：Driver API 是 Runtime API 的底层实现，提供更早暴露的新特性和更细粒度的控制。
2. **句柄模型**：所有资源（设备、上下文、模块、内存、流等）均通过不透明句柄操作。
3. **上下文即进程**：上下文封装了地址空间、模块、纹理引用等资源；不同上下文的 `CUdeviceptr` 指向不同物理内存。
4. **PTX 的前向兼容性**：为了保证未来架构的可运行性，必须分发/加载 PTX，而不是仅分发 CUBIN。
5. **动态共享内存 >48 KB 的限制**：必须使用 `extern __shared__` 动态分配，并通过 `cudaFuncSetAttribute` 显式申请。

---

## 关键 API/函数

| API | 作用 |
|-----|------|
| `cuInit(0)` | 初始化 Driver API，必须在任何其他 Driver API 调用之前执行 |
| `cuDeviceGetCount` / `cuDeviceGet` | 枚举并获取设备句柄 |
| `cuCtxCreate` / `cuCtxDestroy` | 创建/销毁上下文 |
| `cuCtxPushCurrent` / `cuCtxPopCurrent` | 管理上下文栈，实现跨线程共享 |
| `cuCtxAttach` / `cuCtxDetach` | 管理上下文引用计数 |
| `cuModuleLoad` / `cuModuleLoadDataEx` | 从文件或内存加载模块（PTX/CUBIN） |
| `cuModuleGetFunction` | 从模块中获取内核函数句柄 |
| `cuLinkCreate` / `cuLinkAddData` / `cuLinkComplete` / `cuLinkDestroy` | 多段 PTX 的链接与加载 |
| `cuMemAlloc` / `cuMemFree` | 设备内存分配与释放 |
| `cuMemcpyHtoD` / `cuMemcpyDtoH` | 主机与设备间数据拷贝 |
| `cuLaunchKernel` | 启动内核 |
| `cudaFuncSetAttribute` | 设置内核属性（如最大动态共享内存大小） |

---

## 注意事项与常见陷阱

### 1. 忘记调用 `cuInit`
任何 Driver API 调用前若未执行 `cuInit(0)`，会导致未定义行为或错误。这是最常见的 Driver API 入门错误。

### 2. 当前上下文（Current Context）缺失
大多数 Driver API 函数要求调用线程有一个“当前上下文”。若上下文未通过 `cuCtxCreate` 或 `cuCtxPushCurrent` 设为当前，将返回 `CUDA_ERROR_INVALID_CONTEXT`。

### 3. 混淆 `CUdeviceptr` 与普通指针
`CUdeviceptr` 是一个整数类型（非结构体指针），在进行指针运算时要注意类型转换。参数传递时，其内存对齐要求与 `void*` 相同。

### 4. PTX vs CUBIN 的兼容性陷阱
- **CUBIN** 是架构特定的（如 sm_120），只能在兼容的目标架构上直接运行。
- **PTX** 在加载时由驱动即时编译（JIT），因此具有前向兼容性。若应用需要在未来 GPU 上运行，务必保留/加载 PTX。

### 5. 上下文引用计数管理不当
在多库共享同一上下文的场景中，必须使用 `cuCtxAttach` / `cuCtxDetach` 正确维护引用计数。过早销毁上下文会导致其他库崩溃。

### 6. `cuLaunchKernel` 参数对齐错误（使用 extra 方式时）
当通过 `CU_LAUNCH_PARAM_BUFFER_POINTER` 传递参数时，必须确保缓冲区中每个参数都满足设备端的对齐要求。结构体包含 `float4`、`double` 或 `CUdeviceptr` 时尤其容易出错。

### 7. 共享内存超过 48 KB 的隐式限制
在较新架构上，若内核每个 block 使用超过 48 KB 的共享内存，不能声明静态数组，必须使用 `extern __shared__` 动态分配，并在主机端调用 `cudaFuncSetAttribute(..., cudaFuncAttributeMaxDynamicSharedMemorySize, ...)` 进行显式 opt-in。

### 8. `cuModuleGetFunction` 的符号名
从 C++ 编译的 PTX 中获取函数时，函数名可能经过 C++ Name Mangling（如 `_Z6VecAddPKfS0_Pfi`）。若找不到符号，应检查 PTX 中的实际入口名，或在编译设备代码时使用 `extern "C"` 避免修饰。
