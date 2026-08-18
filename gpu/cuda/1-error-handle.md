## CUDA 错误检查
<!-- f51681ef-b01a-4558-be42-14f1aa932f53 -->

每个 CUDA API 都返回一个 `cudaError_t` 枚举值。生产代码中应始终检查返回值。无错误时返回 `cudaSuccess`。常见的做法是实现一个 `CUDA_CHECK` 宏，在出错时打印文件名、行号、错误码和错误描述字符串（通过 `cudaGetErrorString` 获取）。

**错误状态**：每个主机线程维护一个 `cudaError_t` 错误状态。`cudaGetLastError()` 返回并清除该状态；`cudaPeekLastError()` 仅返回不清除。

**异步错误**：核函数启动（三角括号语法）本身不返回 `cudaError_t`。建议在启动后立即调用 `CUDA_CHECK(cudaGetLastError())` 来检查启动参数是否合法以及是否有之前的异步错误遗留。但这**不能保证**核函数已成功执行。要捕获核函数执行期间的错误（如非法内存访问），应在同步操作（如 `cudaDeviceSynchronize()`）之后检查错误。

**CUDA_LOG_FILE**：从 NVIDIA 驱动 r570 版本开始，可通过设置环境变量 `CUDA_LOG_FILE` 将 CUDA 驱动遇到的错误信息输出到指定文件（或 `stdout`/`stderr`）。
这即使在没有应用层错误检查的代码中也能帮助定位问题。


原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/intro-to-cuda-cpp.html>

## Error Checking in CUDA 章节转述与总结

### 章节详细内容

CUDA 编程中的错误检查是生产环境应用不可或缺的一环。虽然示例代码中经常省略错误检查，但在实际应用中，**必须**始终检查并管理每个 CUDA API 调用的返回值。

#### 基本错误检查机制

每个 CUDA API 都返回一个 `cudaError_t` 类型的枚举值。当调用成功时，返回值为 `cudaSuccess`。为了方便检查，许多应用程序会实现一个工具宏（如 `CUDA_CHECK`），该宏会：

1. 执行传入的 CUDA 表达式并保存其返回值。
2. 将返回值与 `cudaSuccess` 比较。
3. 如果发生错误，利用 `cudaGetErrorString()` 将错误码转换为人类可读的字符串，并输出到标准错误流（`stderr`）。

通过将 CUDA 运行时 API 调用包装在 `CUDA_CHECK(expression)` 中，开发者可以在任何调用失败时立即获得诊断信息。对于小型项目，直接打印到 `stderr` 是常见做法；在大型应用中，则可以将此宏适配到日志系统或其他错误处理机制中。

#### 错误状态（Error State）

CUDA 运行时为**每个主机线程**维护一个 `cudaError_t` 错误状态，默认值是 `cudaSuccess`，每当发生错误时会被覆盖。

- `cudaGetLastError()`：返回当前错误状态，然后将其重置为 `cudaSuccess`。
- `cudaPeekLastError()`：返回当前错误状态，但不重置它。

**内核启动（使用 `<<< >>>` 语法）不返回 `cudaError_t`**。因此，在内核启动后立即检查错误状态是良好的实践，这可以检测到内核启动本身的参数错误，或者之前遗留的异步错误。但需要注意的是，**此时返回 `cudaSuccess` 仅表示内核启动参数和配置通过了运行时校验，并不保证内核已经成功执行甚至已经开始执行**。

#### 异步错误（Asynchronous Errors）

CUDA 内核启动和许多运行时 API 都是异步的。异步操作执行期间发生的错误（例如内核中的非法内存访问）不会立即报告，而是会在**下一次检查错误状态**时才会暴露出来。这可能发生在：

- 调用 `cudaGetLastError()` 或 `cudaPeekLastError()` 时
- 调用任何返回 `cudaError_t` 的 CUDA API 时

**关键特性**：当异步错误（如内核非法内存访问）产生后，CUDA 运行时的错误状态不会被自动清除。这意味着，在调用 `cudaGetLastError()` 之前，**每一个后续的 CUDA 运行时 API 调用都会返回同一个错误码**。必须通过显式调用 `cudaGetLastError()` 来清除该错误状态，否则程序会陷入持续的错误报告状态。

标准的内核错误检查模式如下：

1. 内核启动后，调用 `cudaGetLastError()` 检查启动参数和之前的遗留错误。
2. 调用 `cudaDeviceSynchronize()` 等待内核执行完成。
3. 再次检查错误，以捕获内核执行期间发生的异步错误。

#### CUDA_LOG_FILE 环境变量

除了代码中的宏检查，CUDA 驱动还提供了 `CUDA_LOG_FILE` 环境变量作为另一种排错手段。当设置该变量时，CUDA 驱动会将遇到的错误信息写入到指定路径的文件中。例如，当启动的线程块大小超过硬件支持的上限时，程序中的 `CUDA_CHECK` 会报告 "invalid argument"，而 `CUDA_LOG_FILE` 指定的文件中可能包含更详细的错误上下文信息。

