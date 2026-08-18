## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/lazy-loading.html>

## Lazy Loading 章节总结

### 内容转述

Lazy Loading（延迟加载）是 CUDA 11.7 引入的一项优化机制，旨在通过推迟 CUDA 模块的加载时机来减少程序初始化时间。具体而言，只有当程序真正需要某个 kernel 或设备变量时，对应的 CUDA 模块才会被加载到设备内存中。对于大型应用程序或库（其中包含大量 kernel，但运行时仅调用其中一小部分），这一机制能显著缩短启动耗时。自 CUDA 12.3 起，Lazy Loading 在所有平台上默认启用，但用户仍可通过环境变量对其进行控制。

### 核心概念

1. **延迟加载 vs 立即加载**
   - 延迟加载将模块的内存分配和加载从程序初始化阶段推迟到首次使用附近。
   - 立即加载（Eager Loading）在程序初始化时一次性加载所有模块。

2. **适用范围**
   - 仅对 CUDA Runtime 11.7+ 和 Driver 515+ 有效。
   - 旧版编译器生成的 SASS/PTX 也能受益，只要链接了 11.7+ 的 Runtime。
   - **例外**：包含 Unified Memory（managed variables）的模块仍会立即加载，不受延迟加载影响。

3. **运行时控制与检测**
   - 环境变量 `CUDA_MODULE_LOADING` 控制行为：`LAZY`（默认）或 `EAGER`。
   - Driver API `cuModuleGetLoadingMode` 可在运行时查询当前模式。

4. **强制预加载**
   - `cuModuleGetFunction()` 和 `cudaFuncGetAttributes()` 可在不实际执行 kernel 的情况下将其加载到设备内存。
   - 注意：`cuModuleLoad()` 本身不保证模块立即被加载到设备。

### 关键 API / 函数 / 宏

| 名称 | 所属 API | 用途 |
|------|----------|------|
| `CUDA_MODULE_LOADING` | 环境变量 | 设置为 `LAZY` 启用延迟加载，设置为 `EAGER` 禁用 |
| `cuModuleGetLoadingMode` | Driver API | 运行时查询当前模块加载模式 |
| `CUmoduleLoadingMode` | Driver API 枚举 | 加载模式的数据类型 |
| `CU_MODULE_LAZY_LOADING` | Driver API 枚举值 | 表示当前处于延迟加载模式 |
| `cuInit(0)` | Driver API | 初始化 CUDA Driver，调用 `cuModuleGetLoadingMode` 前必须执行 |
| `cuModuleGetFunction()` | Driver API | 获取 kernel 函数句柄，同时会强制将所在模块加载到设备内存 |
| `cudaFuncGetAttributes()` | Runtime API | 获取 kernel 属性，同时会强制将该 kernel 加载到设备内存 |
| `cuModuleLoad()` | Driver API | 加载模块文件，但**不保证**立即将模块内容加载到设备内存 |
| `cudaMallocAsync()` | Runtime API | 异步内存分配，可作为解决大内存分配冲突的方案之一 |

### 注意事项、限制条件与常见陷阱

1. **版本门槛**
   - CUDA Runtime 必须 >= 11.7，Driver 必须 >= 515。两者缺一不可。

2. **Managed Variables 例外**
   - 包含 `__managed__` 变量的模块不受延迟加载影响，仍会立即加载。

3. **并发内核执行风险**
   - 延迟加载可能在加载过程中临时串行化内核提交，导致原本可并发的内核被串行执行。
   - 若程序错误地假设并发执行是保证的，并使用了跨内核同步，可能发生死锁。
   - **解决方案**：在启动前预加载所有需要并发执行的 kernel，或设置 `CUDA_MODULE_LOADING=EAGER`。

4. **大内存分配冲突**
   - 若程序在启动时通过某种分配器占满了整个显存，后续延迟加载模块时可能因显存不足而失败。
   - **解决方案**：
     - 使用 `cudaMallocAsync()` 替代独占式分配器；
     - 为模块预留显存缓冲；
     - 在初始化分配器之前预加载所有必要 kernel。

5. **性能测量偏差**
   - 延迟加载会将模块初始化开销移动到首次执行窗口内，导致测得的 kernel 首次执行时间偏长。
   - **解决方案**：在正式计时前执行至少一次 warmup，或预加载被测 kernel。

### 底层原理

Lazy Loading 的本质是**将 CUDA 模块的元数据解析、内存分配和代码传输从应用程序初始化路径推迟到执行路径**。

- **内存分配角度**：在 Eager 模式下，CUDA Runtime 在 `cuInit` 或 `cudaFree(0)` 等初始化触发点就会为所有模块分配设备内存并上传代码。而在 Lazy 模式下，这些操作被挂起，直到发生以下任一事件：
  - Kernel 被实际启动；
  - 显式调用 `cuModuleGetFunction` / `cudaFuncGetAttributes` 等会触发加载的 API；
  - 访问模块内的设备变量。

- **调度角度**：当 Lazy Loading 触发模块加载时，Driver 需要在加载完成前阻止该模块内 kernel 的执行。在内部实现中，这可能表现为在加载期间暂时阻塞对应的 stream 或 context 活动。如果多个 kernel 来自不同模块且同时触发加载，Driver 的加载操作可能与 kernel 提交发生交错，从而破坏用户预期的并发性。因此，对于依赖并发执行的正确性（如 producer-consumer 模式中的跨 stream 同步），建议预加载相关模块以消除加载阶段的调度不确定性。
