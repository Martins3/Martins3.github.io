# cuda 变量修饰符
<!-- c199dac7-816c-4d51-a61e-ee91f26cc457 -->

- `__device__`：变量存储在全局内存 (Global Memory)。
- `__constant__`：变量存储在常量内存 (Constant Memory)。
- `__managed__`：变量作为统一内存 (Unified Memory)。
- `__shared__`：变量存储在共享内存 (Shared Memory)。

在 `__device__` 或 `__global__` 函数内部，未加修饰符的变量默认优先分配到寄存器，必要时 spill 到局部内存 (Local Memory)。在外部则分配到系统内存。

??? 这是什么意思?
**检测设备编译**：在 `__host__ __device__` 函数中，可以通过检查预处理器宏 `__CUDA_ARCH__` 是否定义来区分 GPU 端和 CPU 端的代码路径。

2. **独立编译的链接规则**：
   - 不要忘记在最终链接命令中包含所有 `.o` 文件。
   - 自 CUDA 13 起，`__global__`、`__managed__`、`__device__`、`__constant__` 默认内部链接。
   若要在多文件中使用，需显式加 `extern`（变量）或确保函数声明可见。

`__shared__` 可以动态分配吗?
