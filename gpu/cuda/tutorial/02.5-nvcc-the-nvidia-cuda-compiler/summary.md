## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/nvcc.html>

## 2.5 NVCC：NVIDIA CUDA 编译器

NVCC（NVIDIA CUDA Compiler）是 CUDA Toolkit 提供的顶层编译驱动工具，负责协调编译器、链接器、PTX 汇编器（ptxas）和 Cubin 汇编器，完成 CUDA C/C++ 以及 PTX 代码的**离线编译**（offline compilation）。它与运行时的 JIT 编译器 NVRTC 相对。

---

### CUDA 源文件与头文件

| 扩展名 | 说明 | 内容 |
|--------|------|------|
| `.c` / `.cpp` / `.cc` / `.cxx` | C/C++ 源文件 | 仅 Host 代码 |
| `.h` / `.hpp` / `.hh` / `.hxx` | C/C++ 头文件 | Host 代码或混合代码 |
| `.cu` | CUDA 源文件 | Host 代码、Device 代码或混合代码 |
| `.cuh` | CUDA 头文件 | 通常包含 Device 代码或混合代码 |

---

### NVCC 编译工作流程

1. **分离阶段**：NVCC 将 `.cu` 文件中的 Host 代码与 Device 代码分离。
2. **Host 编译**：Host 代码交由系统 Host 编译器（如 g++、clang++）编译。
3. **Device 编译**：Device 代码先编译为 PTX（虚拟架构，如 `compute_61`），再由 `ptxas` 汇编为针对具体 GPU 的 Cubin（真实架构，如 `sm_61`）。
4. **链接阶段**：将 Host 目标文件与包含 GPU 代码的 Fatbin 目标文件链接，生成最终可执行文件。

常用辅助选项：
- `-v`：显示完整的工具调用流程。
- `-keep` / `--keep-dir`：保留中间文件。

---

### NVCC 基本用法

```bash
nvcc <source_file>.cu -o <output_file>
```

常用通用选项：
- `-I <path>`：指定头文件搜索路径。
- `-L <path>`：指定库文件搜索路径。
- `-l<library>`：链接指定库。
- `-D<macro>=<value>`：定义宏。

#### PTX 与 Cubin 生成

- 默认情况下，NVCC 为 CUDA Toolkit 支持的最早 GPU 架构生成 PTX 和 Cubin，以保证最大兼容性。
- `-arch=sm_<XY>`：为指定真实架构生成 PTX 和 Cubin（具有前向兼容性）。
- `-arch=compute_<XY>`：仅生成 PTX（仅前向兼容，无 Cubin）。
- `-arch=native`：自动检测当前 GPU 并生成 Cubin（无 PTX，无 GPU 前向兼容性）。
- `-arch=all`：为所有支持的架构生成 Cubin，并包含最新 PTX。
- `-arch=all-major`：为所有主要架构生成 Cubin，并包含最新 PTX。
- `-gencode=arch=compute_<XY>,code=sm_<XY>,sm_<ZW>`：为多个架构生成代码，可精确控制 PTX 和 Cubin 的组合。

查看支持的架构：
```bash
nvcc --list-gpu-code   # 真实架构
nvcc --list-gpu-arch   # 虚拟架构
```

#### Host 代码编译注意事项

- 纯 Host 代码可直接使用 Host 编译器编译，但如果使用了 CUDA Runtime API，则最终必须链接 CUDA Runtime 库（`libcudart_static` 或 `libcudart`）。
- 默认链接静态 CUDA Runtime；如需动态库，传递 `--cudart=shared`。
- 指定 Host 编译器：
  ```bash
  nvcc -ccbin /usr/bin/g++-14 example.cu -o example
  ```
- 环境变量 `NVCC_CCBIN` 也可用于指定 Host 编译器。
- 向 Host 编译器透传选项：`-Xcompiler=-O3`。

#### GPU 代码独立编译（Separate Compilation）

默认情况下，NVCC 采用**整程序编译**（whole-program compilation），要求所有 Device 代码和符号都在使用它们的编译单元中可见。若需跨编译单元链接 Device 代码，必须开启**独立编译**：

- 编译时添加 `-dc` 或 `-rdc=true`（`rdc` = Relocatable Device Code）。
- 跨文件引用的非 `const` `__device__` 变量，需在其它编译单元中用 `extern` 声明。
- 所有 `const` `__device__` 变量也需在定义和引用处使用 `extern`。
- Host 函数和 Device 函数默认具有外部链接，无需 `extern`。

**注意**：从 CUDA 13 开始，`__global__` 函数以及 `__managed__` / `__device__` / `__constant__` 变量默认具有内部链接。

独立编译可能影响性能，可通过**链接时优化（LTO）**缓解：
```bash
nvcc -dc -dlto -arch=sm_80 a.cu -o a.o
nvcc -dc -dlto -arch=sm_80 b.cu -o b.o
nvcc -dlto a.o b.o -o program
```

---

### 常用编译器选项

#### 语言特性

- `-std={c++03|c++11|c++14|c++17|c++20}`：指定 C++ 语言标准。
- `-restrict`：断言所有内核指针参数均为 `restrict` 指针。
- `-extended-lambda`：允许在 lambda 声明中使用 `__host__` / `__device__` 注解。
- `-expt-relaxed-constexpr`：允许 Host 代码调用 `__device__ constexpr` 函数，反之亦然（实验性）。

#### 调试选项

- `-g`：为 Host 代码生成调试信息。
- `-G`：为 Device 代码生成调试信息（供 `cuda-gdb` 使用），并定义 `__CUDACC_DEBUG__` 宏。会降低性能。
- `-lineinfo`：为 Device 代码生成行号信息（不影响性能），可与 `compute-sanitizer` 或 Nsight Compute 配合使用。
- `-DNDEBUG`：禁用运行时断言，提升执行速度。

#### 优化选项

- `-Xptxas=-maxrregcount=N`：限制每个线程的最大寄存器使用量。
- `-extra-device-vectorization`：启用更激进的 Device 代码向量化。
- `-res-usage`：编译后打印资源使用报告（寄存器、共享内存、常量内存、本地内存）。
- `-opt-info=inline`：打印内联函数信息。
- `-Xptxas=-warn-lmem-usage`：如果使用本地内存则发出警告。
- `-Xptxas=-warn-spills`：如果寄存器溢出到本地内存则发出警告。

#### 链接时优化（LTO）

- `-dlto` 或 `-arch=lto_<SM>`：在链接阶段对跨文件的 Device 代码执行优化，恢复整程序编译的部分性能，代价是编译时间增加。

#### 性能分析选项

- `-lineinfo`：在性能分析工具（Nsight Compute / Systems）中关联源代码。
- `-src-in-ptx`：在 PTX 中保留原始源代码（需配合 `-lineinfo`），避免仅依赖行号信息的局限。

#### Fatbin 压缩

- `-no-compress`：禁用 Fatbin 压缩。
- `--compress-mode={default|size|speed|balance|none}`：设置压缩模式。默认 `speed` 关注解压速度；`size` 追求最小体积；`balance` 折中。

#### 编译器性能控制

- `-t <N>`：使用 N 个线程并行编译单个编译单元中的多架构代码。
- `-split-compile <N>`：使用 N 个线程并行优化阶段。
- `-split-compile-extended <N>`：更激进的拆分编译（需要 LTO）。
- `-Ofc <N>`：控制 Device 代码编译速度级别。
- `-time <filename>`：生成各编译阶段耗时的 CSV 报告。
- `-fdevice-time-trace`：生成 Device 编译的时间追踪文件。
