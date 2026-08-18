## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/driver-entry-point-access.html>


本章讨论 CUDA 的 `Driver Entry Point Access` 机制。它从 CUDA 11.3 开始提供一组专门的查询 API，使应用能够在运行时动态取得 CUDA Driver API 函数的地址，并以函数指针的形式调用这些入口点。这个能力在定位上非常接近 POSIX 的 `dlsym()` 或 Windows 的 `GetProcAddress()`，但又比直接对 `libcuda.so` 做符号查找更安全，因为 CUDA 会根据你请求的版本号和标志位，返回与目标 ABI 兼容的驱动符号。

这一机制主要解决四类问题：第一，应用希望在运行时选择某个 Driver API 的特定 ABI 版本，而不是使用头文件默认暴露的那个版本；第二，应用需要取得带有 per-thread default stream 语义的入口点变体；第三，应用在工具链较旧、驱动较新的环境上，希望尽早使用新驱动中已经提供但 Toolkit 头文件尚未默认暴露的新功能；第四，应用希望显式区分"查询 API 本身用法错误"和"驱动里确实没有该符号"这两类失败原因。

与很多 CUDA 高级特性不同，本章的重点不在 GPU 硬件本身，而在 **Driver API 版本化、符号解析、ABI 兼容性** 这三个软件层问题。文档的大部分篇幅都在提醒一个核心事实：**函数名相同，不代表 ABI 相同；拿到了函数指针，不代表它一定与当前编译时使用的 typedef 匹配。**

## 背景与要解决的问题

### 为什么需要 `Driver Entry Point Access`

传统 CUDA 程序通常直接链接 `cuda.h` / `cuda_runtime.h` 中声明的 API，并在编译期由头文件决定所调用的函数原型。例如，代码里直接写：

```cpp
cuCtxCreate(&ctx, 0, dev);
```

这种写法简单，但它有两个限制：

1. **头文件决定默认 ABI**
   某个 Driver API 在未来版本中可能引入 `_v2`、`_v3` 等新符号。为了维持 minor version compatibility，`cuda.h` 往往不会立刻把默认声明切换到最新版本。于是“直接调用 API”和“手动查询函数指针再调用”可能并不指向同一个底层符号。

2. **旧 Toolkit 看不到新 Driver API**
   如果机器上的 NVIDIA Driver 比本地 CUDA Toolkit 更新，那么驱动里可能已经带有新 API，但本地安装的 `cudaTypedefs.h` 和 `cuda.h` 还没有对应声明。此时直接写 API 名称无法编译，但通过入口点查询仍然可能成功访问新功能。

3. **`default stream semantics` 需要显式选择**
   某些 Driver API 同时提供 legacy default stream 和 per-thread default stream 两种语义，底层对应不同的入口点变体。传统调用方式对这一层控制不够细，而 `cuGetProcAddress` / `cudaGetDriverEntryPoint` 可以通过 flag 明确要求某种 `stream semantics`。

4. **失败原因需要分层诊断**
   一个符号查询失败，可能是：
   - 你传了错误的参数；
   - 传入的 CUDA 版本号过低；
   - 驱动过旧，不含该符号；
   - 符号名字拼错了。

   `Driver Entry Point Access` 将这些情况拆成两层结果：API 返回值负责报告“调用姿势对不对”，额外的 query result 负责报告“符号为什么没找到”。

### 为什么这是 ABI 问题，而不是单纯的“函数指针技巧”

CUDA Driver API 的很多符号采用版本化命名规则。首个版本通常没有 `_v*` 后缀，而后续 ABI 或语义变化会生成新的符号名，例如：

- `cuMemAlloc`
- `cuMemAlloc_v2`

与此同时，Toolkit 还会在 typedef 头文件中提供形如：

- `PFN_cuMemAlloc_v2000`
- `PFN_cuMemAlloc_v3020`
- `PFN_cuMemAlloc`

这些 typedef 与具体 ABI 严格绑定。如果你用 `PFN_cuCtxCreate` 这样的旧 typedef 去调用实际返回的 `cuCtxCreate_v3`，那已经不是普通逻辑错误，而是 **ABI 不匹配导致的未定义行为**。

## 核心概念与术语

### `cuGetProcAddress`

CUDA Driver API 提供的入口点查询函数。它根据：

- 符号字符串；
- 指定的 CUDA 版本号；
- `stream semantics` flag；

返回一个与目标 ABI 兼容的 Driver API 函数指针。

### `cudaGetDriverEntryPoint`

CUDA Runtime API 一侧提供的查询函数。它同样用于获取 **Driver API** 的入口点，但版本决策基于当前 Runtime 版本，而不是调用者手工传入的 CUDA 版本。

### `cudaGetDriverEntryPointByVersion`

Runtime API 的增强版本，允许调用者显式指定想要请求的 CUDA 版本，从而避免 `cudaGetDriverEntryPoint` 完全受当前 Runtime 版本支配。

### Typedef Header

Toolkit 自带的函数指针 typedef 头文件，例如：

- `cudaTypedefs.h`
- `cudaGLTypedefs.h`
- `cudaProfilerTypedefs.h`
- `cudaEGLTypedefs.h`
- `cudaD3D9Typedefs.h`
- `cudaD3D10Typedefs.h`
- `cudaD3D11Typedefs.h`

这些头文件**不定义函数指针变量本身**，只提供类型定义，方便用户声明正确 ABI 的函数指针。

### Versioned Symbol / ABI 版本

Driver API 的符号具有按 CUDA 版本递增的 ABI 版本。例如：

- `PFN_cuMemAlloc_v2000` 对应最初版本；
- `PFN_cuMemAlloc_v3020` 对应后续 ABI；
- `PFN_cuMemAlloc` 则表示“当前 Toolkit 发布时的最新版本 typedef”。

这里的关键不是函数名，而是 **typedef 是否和真实返回的符号 ABI 一致**。

### `CUdriverProcAddressQueryResult`

`cuGetProcAddress` 额外返回的查询结果，用于区分符号解析状态。典型取值包括：

- `CU_GET_PROC_ADDRESS_SUCCESS`
- `CU_GET_PROC_ADDRESS_VERSION_NOT_SUFFICIENT`
- `CU_GET_PROC_ADDRESS_SYMBOL_NOT_FOUND`

### Per-thread Default Stream 变体

某些 Driver API 除默认语义版本外，还提供 `_ptsz` 或 `_ptds` 变体。例如：

- `cuLaunchKernel`
- `cuLaunchKernel_ptsz`

它们主要区别在 `default stream semantics`。

## API / 机制详解

### 1. Driver 侧查询：`cuGetProcAddress`

#### 作用

根据符号名、目标 CUDA 版本和 `stream semantics` 标志，从当前安装的 CUDA Driver 中取回一个 Driver API 函数指针。

#### 典型调用形式

```cpp
void* pfn = nullptr;
CUdriverProcAddressQueryResult driverStatus;
CUresult status = cuGetProcAddress(
    "cuMemAlloc",
    &pfn,
    3020,
    CU_GET_PROC_ADDRESS_DEFAULT,
    &driverStatus);
```

#### 关键参数

- `symbol`：字符串形式的 Driver API 名称，例如 `"cuMemAlloc"`。
- `pfn`：输出参数，返回函数指针地址。
- `cudaVersion`：**最关键** 的版本选择参数。它不是“随便填当前驱动版本”，而是用来约束要取回哪个 ABI 版本。
- `flags`：决定默认流 / PTDS 语义。
- `symbolStatus`：补充报告符号查询状态。

#### 最重要的使用原则

如果你想得到某个 ABI 的特定版本，就应当传入**该版本首次引入时对应的 CUDA 版本号**，并使用对应的 typedef。例如：

- 想拿 `cuStreamBeginCapture` 的旧版 ABI，就传 `10000`，配 `PFN_cuStreamBeginCapture_v10000`
- 想拿 `_v2` 版本，就传 `10010`，配 `PFN_cuStreamBeginCapture_v10010`

不要为了“尽量新”而盲目传更高版本号，否则未来 Driver 一旦返回 `_v3`，你旧的 typedef 就会与真实符号不匹配。

#### `CUDA_VERSION` 的意义

如果你的目标不是锁死某个历史 ABI，而是“拿当前 Toolkit 已知的最新版本”，可以传：

```cpp
cuGetProcAddress("cuStreamBeginCapture",
                 &pfn,
                 CUDA_VERSION,
                 CU_GET_PROC_ADDRESS_DEFAULT,
                 &driverStatus);
```

此时应搭配**无 `_v*` 后缀**的 typedef，例如：

```cpp
PFN_cuStreamBeginCapture pfn;
```

这种写法只在“编译时 Toolkit 版本”和“你愿意接受该 Toolkit 所定义的最新 ABI”这两个条件都满足时安全。

### 2. Runtime 侧查询：`cudaGetDriverEntryPoint`

#### 作用

通过 CUDA Runtime API 获取 Driver API 函数指针。

#### 典型调用形式

```cpp
PFN_cuMemAllocAsync pfn_cuMemAllocAsync;
cudaGetDriverEntryPoint("cuMemAllocAsync",
                        &pfn_cuMemAllocAsync,
                        cudaEnableDefault,
                        &runtimeStatus);
```

#### 语义特点

- 它会依据 **当前 Runtime 版本** 选择 ABI 兼容的 Driver 符号。
- 因为版本选择是 Runtime 决定的，所以调用者对 ABI 版本的控制不如 `cuGetProcAddress` 精细。
- 如果你明确需要指定版本，应优先考虑 `cudaGetDriverEntryPointByVersion`。

#### 何时适合使用

- 程序本身主要使用 Runtime API，只是想偶尔访问某些 Driver API
- 想减少对 `cuGetProcAddress` 版本参数的手工管理
- 能接受“ABI 选择跟着 Runtime 版本走”的约束

### 3. Runtime 显式版本查询：`cudaGetDriverEntryPointByVersion`

#### 作用

它是 Runtime API 侧更接近 `cuGetProcAddress` 的接口，允许调用者显式传入 CUDA 版本号，从而按指定 ABI 请求入口点。

#### 价值

相比 `cudaGetDriverEntryPoint`，它更容易实现：

- 明确锁定某个 ABI；
- 与手写 typedef 精确匹配；
- 避免 Runtime 版本隐式决定返回哪个符号。

### 4. Typedef 头文件与版本命名

Toolkit 会为 Driver API 准备独立 typedef 头文件。最常用的是 `cudaTypedefs.h`。例如：

```cpp
typedef CUresult (CUDAAPI *PFN_cuMemAlloc_v3020)(CUdeviceptr_v2 *dptr, size_t bytesize);
typedef CUresult (CUDAAPI *PFN_cuMemAlloc_v2000)(CUdeviceptr_v1 *dptr, unsigned int bytesize);
typedef CUresult (CUDAAPI *PFN_cuMemAlloc)(CUdeviceptr_v2 *dptr, size_t bytesize);
```

这里有三个层次：

1. `PFN_xxx_vYYYYY`：精确绑定到某个历史 ABI；
2. `PFN_xxx`：绑定到当前 Toolkit 认知下的“最新 ABI”；
3. 实际返回哪个版本，仍由查询 API 的版本参数和运行环境共同决定。

### 5. Per-thread Default Stream 版本获取

某些 Driver API 具有 legacy default stream 和 PTDS 两套语义。可以通过两类方式选择：

1. **编译期选择**
   - `--default-stream per-thread`
   - 或定义宏 `CUDA_API_PER_THREAD_DEFAULT_STREAM`

2. **查询时强制指定**
   - `CU_GET_PROC_ADDRESS_LEGACY_STREAM`
   - `CU_GET_PROC_ADDRESS_PER_THREAD_DEFAULT_STREAM`
   - Runtime 对应：
     - `cudaEnableLegacyStream`
     - `cudaEnablePerThreadDefaultStream`

例如对 `cuLaunchKernel`，可以显式请求带 PTDS 语义的入口点，而不是默认版本。

### 6. 用旧 Toolkit 访问新 Driver 特性

这是本章最实用的场景之一。假设：

- 本地 Toolkit 是 CUDA 11.3
- 机器上 Driver 已升级到 CUDA 12.0
- 你想调用 12.0 才引入的新 API `cuFoo`

此时 `cudaTypedefs.h` 里可能还没有 `PFN_cuFoo`，你需要：

1. 自己手写函数指针 typedef；
2. 通过 `cuDriverGetVersion()` 获取驱动版本；
3. 用 `cuGetProcAddress("cuFoo", ..., driverVersion, ...)` 查询；
4. 在 `status == CUDA_SUCCESS` 且 `driverStatus == CU_GET_PROC_ADDRESS_SUCCESS` 时调用。

这种方式本质上让“驱动能力”先于“Toolkit 头文件”被使用，但代价是**你自己负责原型正确性**。

## 典型工作流程 / 调用顺序

### 场景一：稳定地获取某个特定 ABI 版本

1. 包含对应 typedef 头文件，例如 `#include <cudaTypedefs.h>`。
2. 明确你要的不是“最新版本”，而是某个固定 ABI。
3. 用该 ABI 对应的 typedef 声明函数指针，例如 `PFN_cuMemAlloc_v3020`。
4. 调用 `cuGetProcAddress()`，把 `cudaVersion` 设置为该 ABI 首次引入的 CUDA 版本号。
5. 同时检查：
   - `CUresult`
   - `driverStatus`
   - `pfn != nullptr`
6. 再调用函数指针。

### 场景二：获取当前 Toolkit 能理解的最新 ABI

1. 使用无后缀 typedef，例如 `PFN_cuMemAlloc`。
2. 传 `CUDA_VERSION` 给 `cuGetProcAddress()`。
3. 仅在你接受“跟随当前 Toolkit 最新 ABI”时使用这一模式。

### 场景三：Runtime 程序偶尔调用 Driver 新接口

1. 正常包含 `cuda_runtime.h` 和 `cudaTypedefs.h`。
2. 用 `cudaGetDriverEntryPoint()` 或 `cudaGetDriverEntryPointByVersion()` 查询。
3. 根据 `cudaDriverEntryPointQueryResult` 与返回码决定是否可用。
4. 不可用时优雅降级。

### 场景四：旧 Toolkit + 新 Driver 访问新增功能

1. 手动声明新 API 的 typedef。
2. 通过 `cuDriverGetVersion()` 获取驱动版本。
3. 用 `cuGetProcAddress()` 查询。
4. 若拿到函数指针，则启用新功能；否则回退旧路径。

## 关键限制、边界条件与兼容性

### 1. `cuGetProcAddress` 返回的是“与你请求版本兼容的符号”，不是“你心里想的那个 typedef”

这句话非常关键。查询 API 不知道你代码里拿的是哪个 typedef，它只知道：

- 符号名；
- 版本号；
- flag。

因此，**typedef 与版本号匹配** 是调用者责任。

### 2. 直接调用 API 与入口点查询，行为可能不同

文档用 `cuDeviceGetUuid` 说明了这一点。由于 minor version compatibility 的要求，`cuda.h` 中默认暴露的 API 版本切换可能滞后于驱动符号的实际版本演进。所以：

- 直接写 `cuDeviceGetUuid(...)`
- 与 `cuGetProcAddress("cuDeviceGetUuid", ..., CUDA_VERSION, ...)`

不一定调到同一个底层版本。

### 3. 用 `cuDriverGetVersion()` 动态传版本号并不总是安全

很多人会直觉写成：

```cpp
cuDriverGetVersion(&driverVersion);
cuGetProcAddress("xxx", &pfn, driverVersion, ...);
```

问题在于：驱动升级后，返回版本号也会升高，`cuGetProcAddress` 可能因此返回更新的 `_v2` / `_v3` 符号，而你的 typedef 仍然是旧 Toolkit 的版本，导致 ABI 风险。

### 4. 显式版本判断也无法自动覆盖未来 minor bump

即使你写了：

- `< 11.5` 走 `PFN_cuFoo_v11040`
- `>= 11.5` 走 `PFN_cuFoo_v11050`

将来若 11.6 引入 `cuFoo_v3` 且 ABI 变化，你旧代码仍可能把 `v3` 当 `v2` 用。

### 5. Runtime 查询 API 可能比 Driver 查询 API 更难控制

`cudaGetDriverEntryPoint()` 默认依据 Runtime 版本选择 ABI。这样在下列组合中容易出现“编译期 typedef”和“运行时返回符号”不一致：

- 新 Runtime + 旧 Driver
- 旧 Toolkit + 动态链接到不同版本 Runtime

因此，如果代码对 ABI 敏感，通常更推荐：

- 直接使用 `cuGetProcAddress`
- 或使用 `cudaGetDriverEntryPointByVersion`

### 6. ABI 不匹配时可能不是“调用失败”，而是未定义行为

文档特别指出 `cuCtxCreate` 就是高风险案例。`cuCtxCreate_v3` 比旧版多参数，如果你仍按旧 typedef 调用，即便拿到了非空函数指针，也可能：

- 传参错位；
- 栈布局错误；
- 表面看似成功，实际引入隐蔽损坏。

## 常见陷阱与调试建议

### 1. 误把 `CUDA_VERSION` 当成“永远安全的最新值”

`CUDA_VERSION` 只表示**当前编译所用 Toolkit** 的版本。它适合搭配当前 Toolkit 自带的无后缀 typedef，但不适合在需要锁死 ABI 时偷懒使用。

### 2. 运行时拿 Driver 版本直接喂给 `cuGetProcAddress`

这在“想用新特性”时是合理的，但前提是你：

- 确认 typedef 与目标 API 版本一致；
- 或自己手写了正确 prototype。

否则一旦驱动比 Toolkit 新，就可能拿到更高版本符号。

### 3. 把“返回码成功”误解为“可以安全调用”

调用 `cuGetProcAddress` 时需要同时看三层：

1. `CUresult status`
2. `driverStatus`
3. `pfn != nullptr`

仅仅 `status == CUDA_SUCCESS` 还不够，`driverStatus` 可能仍然告诉你版本不足或符号不存在。

### 4. 忽略 `CU_GET_PROC_ADDRESS_VERSION_NOT_SUFFICIENT`

这个状态说明：

- Driver 里有这个符号；
- 但你要求的 `cudaVersion` 太低，不允许返回它。

这通常表示“驱动够新，代码版本请求太保守”，而不是“驱动不支持”。

### 5. 忽略 `CU_GET_PROC_ADDRESS_SYMBOL_NOT_FOUND`

它可能表示：

- 驱动太旧；
- 符号名拼错；
- 请求了当前驱动根本没有的 API。

例如把 `"cuDeviceGetExecAffinitySupport"` 写成错误大小写，就会得到同样的结果。

### 6. 在旧 Toolkit 中使用新 Driver API，却忘了自定义 typedef

旧 `cudaTypedefs.h` 里没有的新符号，不能指望头文件帮你兜底。此时必须自己声明与目标驱动一致的 prototype，否则即便查询成功也无法安全调用。

### 7. 混淆 Runtime 语义与 Driver 语义 flag

Driver 侧用：

- `CU_GET_PROC_ADDRESS_DEFAULT`
- `CU_GET_PROC_ADDRESS_LEGACY_STREAM`
- `CU_GET_PROC_ADDRESS_PER_THREAD_DEFAULT_STREAM`

Runtime 侧则是：

- `cudaEnableDefault`
- `cudaEnableLegacyStream`
- `cudaEnablePerThreadDefaultStream`

不要把两套 flag 混着传。

## 一个最小可运行示例的说明

本目录下的 `chapter_demo.cu` 对应本章内容，设计成一个“以 `cuGetProcAddress` / `cudaGetDriverEntryPoint` 为主线”的小型实验程序，适合用来验证：

1. **显式版本获取 Driver 函数**
   通过 `cuGetProcAddress("cuMemAlloc", ..., 3020, ...)` 取得 `cuMemAlloc_v2`，然后实际完成一次显存分配与释放。

2. **使用 `CUDA_VERSION` 获取当前 Toolkit 认知下的最新版本**
   通过无后缀 typedef 获取 `cuMemAlloc` 的当前默认 ABI。

3. **通过 Runtime API 获取 Driver 入口点**
   使用 `cudaGetDriverEntryPoint("cuMemAllocAsync", ...)` 尝试查询异步分配接口，并在可用时实际调用。

4. **获取 PTDS 变体入口点**
   使用 `CU_GET_PROC_ADDRESS_PER_THREAD_DEFAULT_STREAM` 取回 `cuLaunchKernel` 的 per-thread default stream 版本。

5. **区分失败原因**
   通过查询 `cuDeviceGetExecAffinitySupport` 演示：
   - `CU_GET_PROC_ADDRESS_SYMBOL_NOT_FOUND`
   - `CU_GET_PROC_ADDRESS_VERSION_NOT_SUFFICIENT`
   - `CU_GET_PROC_ADDRESS_SUCCESS`

6. **显式锁定旧 ABI，避免 typedef 漂移**
   通过 `cuDeviceGetUuid` 的 `v9020` 与 `v11040` 两种调用，演示“版本号和 typedef 必须一起锁定”的原则。

7. **说明隐式链接与入口点查询的差异**
   示例最后用注释和输出提醒：当驱动升级而 Toolkit 不升级时，`cuGetProcAddress` 返回的符号版本可能与直接链接的默认 API 不同。

### 编译与运行

```bash
make
make run
```

当前 `Makefile` 使用：

- `nvcc`
- `-std=c++17`
- `-arch=sm_61`
- `-lcuda`

这说明示例重点在 **Driver 入口点查询机制本身**，并不依赖 Hopper/Blackwell 等新硬件特性；即使在 Pascal (`sm_61`) 这样的老架构上，也可以演示大部分查询与 ABI 处理逻辑。

### 如何理解这个示例的边界

- 示例能证明“查询 API 的使用方式”和“不同状态码的判定方式”；
- 但它不能替代 ABI 审计。若你准备在生产代码里通过旧 Toolkit 调新 Driver 的新 API，仍应以对应版本的官方头文件或文档为准，手工核对 prototype；
- `cuMemAllocAsync` 等接口即便查询成功，具体功能是否可用，还取决于驱动版本、设备能力与运行时环境。
