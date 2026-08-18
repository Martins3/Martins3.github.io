## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/nvcc.html>

## 概述

NVCC（NVIDIA CUDA Compiler）是 CUDA Toolkit 中的核心工具链，负责协调编译器、链接器、PTX 汇编器以及 Cubin 汇编器等组件，完成 CUDA C/C++ 代码以及 PTX 代码的离线编译。与之相对，NVRTC 提供运行时的即时（JIT）编译能力。本章介绍 NVCC 最常见的工作流程、编译选项和实际使用细节，完整参考请查阅 nvcc 官方文档。

## CUDA 源文件与头文件

NVCC 根据文件扩展名区分宿主代码（Host Code）与设备代码（Device Code）：

| 扩展名 | 类型 | 内容 |
|--------|------|------|
| `.c` | C 源文件 | 仅 Host 代码 |
| `.cpp`、`.cc`、`.cxx` | C++ 源文件 | 仅 Host 代码 |
| `.h`、`.hpp`、`.hh`、`.hxx` | C/C++ 头文件 | Host 代码、Device 代码或混合 |
| `.cu` | CUDA 源文件 | Host 代码、Device 代码或混合 |
| `.cuh` | CUDA 头文件 | Host 代码、Device 代码或混合 |

含有设备代码的文件推荐使用 `.cu` / `.cuh`，以便 NVCC 正确识别并启动 GPU 编译流程。

## NVCC 编译流程

1. **分离阶段**：NVCC 首先将源文件中的 Host 代码与 Device 代码分离。
2. **Host 编译**：Host 代码交由系统兼容的 C/C++ 编译器（如 GCC、Clang、MSVC）编译。仅含 Host 代码的文件也可以直接用 Host 编译器编译，生成的目标文件可在链接阶段与 NVCC 产出的含 GPU 代码的目标文件合并。
3. **Device 编译**：Device 代码被 GPU 编译器编译为 PTX（虚拟指令集，如 `compute_90`）。
4. **PTX 汇编**：`ptxas` 将 PTX 进一步汇编为针对具体硬件 ISA（即 SM 版本，如 `sm_90`）的 Cubin。
5. **Fatbin**：一个二进制文件可以嵌入多组 PTX 和 Cubin，形成 Fatbin，从而支持多种虚拟架构和实际硬件。

常用查看/保留中间产物的标志：
- `-v`：显示完整编译流程及子工具调用命令。
- `-keep` / `--keep-dir`：保留中间文件到当前目录或指定目录。

## NVCC 基本用法

### 基础编译命令

```bash
nvcc <source_file>.cu -o <output_file>
```

常用标志与常规 C/C++ 编译器类似：
- `-I <path>`：指定头文件搜索路径。
- `-L <path>`：指定库文件搜索路径。
- `-l<library>`：链接指定库。
- `-D<macro>=<value>`：定义预处理宏。

示例：
```bash
nvcc example.cu -I path_to_include/ -L path_to_library/ -lcublas -o output_file
```

### PTX 与 Cubin 生成

默认情况下，NVCC 会生成兼容最早支持 GPU 架构的 PTX 和 Cubin，以最大化兼容性。

- `-arch=compute_<XY>`：仅生成 PTX（具备前向兼容性）。
- `-arch=sm_<XY>`：生成 PTX 和 Cubin（具备前向兼容性）。
- `-arch=native`：自动检测当前 GPU 并生成对应的 Cubin（不含 PTX，因此无前向兼容性）。
- `-arch=all`：为所有支持的 GPU 架构生成 Cubin，并包含最新 PTX。
- `-arch=all-major`：为所有主要架构生成 Cubin，并包含最新 PTX。

使用 `-gencode` 可以为多个架构同时生成代码：
```bash
nvcc example.cu -gencode=arch=compute_80,code=sm_86,sm_89
nvcc example.cu -gencode=arch=compute_80,code=sm_86,sm_89 -gencode=arch=compute_90,code=sm_90
```

查询支持的架构：
```bash
nvcc --list-gpu-code   # 真实 GPU 架构（sm_XY）
nvcc --list-gpu-arch   # 虚拟 GPU 架构（compute_XY）
```

### Host 代码编译说明

- 不含 Device 代码的编译单元可直接使用 Host 编译器编译。
- 若使用 CUDA Runtime API，必须链接 CUDA Runtime 库。默认链接静态版 `libcudart_static`，可通过 `--cudart=shared` 改为动态版 `libcudart`。
- 指定 Host 编译器：`-ccbin <compiler>` 或设置环境变量 `NVCC_CCBIN`。
- 向 Host 编译器透传参数：`-Xcompiler=-O3`。

### GPU 代码独立编译（Separate Compilation）

NVCC 默认采用全程序编译（Whole-Program Compilation），要求所有 GPU 符号都在使用它们的编译单元中可见。
若要在不同编译单元之间链接设备代码，需要开启独立编译：

- 编译时添加 `-rdc=true` 或 `-dc`。
- 非 `const` 的 `__device__` 变量在一个编译单元中定义后，其他单元需用 `extern` 声明。
- **所有** `const` 的 `__device__` 变量在定义和引用处都必须使用 `extern`。
- Host 函数与 Device 函数默认具有外部链接（`extern`），无需额外关键字。
- **注意**：自 CUDA 13 起，`__global__` 函数以及 `__managed__` / `__device__` / `__constant__` 变量默认具有内部链接（internal linkage）。

编译链接示例：
```bash
nvcc -dc definition.cu -o definition.o
nvcc -dc example.cu -o example.o
nvcc definition.o example.o -o program
```

独立编译可能带来性能损失，可通过链接时优化（LTO）缓解。

## 常用编译器选项

### 语言特性

- `--std={c++03|c++11|c++14|c++17|c++20}`：指定 C++ 语言标准。
- `-restrict`：断言所有内核指针参数均为 `restrict` 指针。
- `-extended-lambda`：允许在 Lambda 表达式中使用 `__host__`、`__device__` 标注。
- `-expt-relaxed-constexpr`：允许 Host 代码调用 `__device__ constexpr` 函数，反之亦然（实验性）。

### 调试选项

- `-g`：为 Host 代码生成调试信息（供 gdb / lldb 使用）。
- `-G`：为 Device 代码生成调试信息（供 cuda-gdb 使用），并定义宏 `__CUDACC_DEBUG__`。
- `-lineinfo`：为 Device 代码生成行号信息（不影响性能，配合 compute-sanitizer 使用）。

注意：NVCC 默认对 GPU 代码使用 `-O3`；`-G` 会禁用部分优化，导致性能下降。可用 `-DNDEBUG` 关闭运行时断言。

### 优化选项

- `-Xptxas=<arg>`：向 PTX 汇编器 `ptxas` 传递参数。例如 `-Xptxas=-maxrregcount=N` 限制每个线程的最大寄存器数量。
- `-extra-device-vectorization`：启用更激进的设备代码向量化。
- `-res-usage`：输出资源使用报告（寄存器、共享内存、常量内存、本地内存）。
- `-opt-info=inline`：输出内联函数信息。
- `-Xptxas=-warn-lmem-usage`：使用本地内存时发出警告。
- `-Xptxas=-warn-spills`：寄存器溢出到本地内存时发出警告。

### 链接时优化（LTO）

独立编译缺少跨文件优化机会，LTO 可在链接阶段执行跨文件优化，代价是增加编译时间：

```bash
nvcc -dc -dlto -arch=sm_100 definition.cu -o definition.o
nvcc -dc -dlto -arch=sm_100 example.cu -o example.o
nvcc -dlto definition.o example.o -o program
```

也可用 `lto_<SM version>` 作为架构目标：`-arch=lto_100`。

### 性能分析选项

- `-lineinfo`：使 Nsight Compute / Systems 等工具能够关联源代码。
- `-src-in-ptx`：在 PTX 中保留原始源代码（需配合 `-lineinfo`），避免源代码路径变更导致的问题。

### Fatbin 压缩

NVCC 默认压缩 Fatbin。控制选项：
- `-no-compress`：禁用压缩。
- `--compress-mode={default|size|speed|balance|none}`：`speed` 侧重解压速度（默认），`size` 侧重减小体积，`balance` 折中，`none` 禁用压缩。

### 编译器性能控制

用于分析和加速编译过程本身：
- `-t <N>`：并行编译单个文件到多个 GPU 架构时使用的 CPU 线程数。
- `-split-compile <N>`：并行化优化阶段的线程数。
- `-split-compile-extended <N>`：更激进的拆分编译（需要 LTO）。
- `-Ofc <N>`：设备代码编译速度级别。
- `-time <filename>`：生成各阶段耗时的 CSV 报表。
- `-fdevice-time-trace`：生成设备代码编译的时间追踪文件。

---

## 核心概念总结

| 概念 | 说明 |
|------|------|
| **PTX** | 并行线程执行（Parallel Thread Execution）虚拟指令集，具备 GPU 前向兼容性。 |
| **Cubin** | 针对特定 SM 版本的二进制机器码。 |
| **Fatbin** | 可包含多组 PTX 和 Cubin 的容器，使单个二进制支持多种 GPU。 |
| **Whole-Program Compilation** | NVCC 默认模式，不允许跨编译单元链接设备符号。 |
| **Separate Compilation** | 通过 `-dc` / `-rdc=true` 启用，允许跨编译单元链接设备代码和变量。 |
| **LTO** | 链接时优化，用于恢复独立编译带来的性能损失。 |
| **Offline vs JIT** | NVCC 为离线编译；NVRTC 为运行时即时编译。 |

## 关键 API / 函数 / 标志速查

- `nvcc <file>.cu -o <out>`：基础编译。
- `-arch=sm_120` / `-arch=compute_120`：指定目标架构。
- `-gencode=arch=compute_XX,code=sm_YY`：精细控制多架构输出。
- `-dc` / `-rdc=true`：启用设备代码独立编译。
- `-dlto` / `-arch=lto_XX`：启用链接时优化。
- `-ccbin <compiler>` / `NVCC_CCBIN`：指定 Host 编译器。
- `--cudart=shared`：链接动态 CUDA Runtime。
- `-Xcompiler=-O3`：向 Host 编译器透传参数。
- `-Xptxas=-maxrregcount=N`：限制寄存器数量。
- `-G` / `-g` / `-lineinfo`：调试信息控制。

## 注意事项与常见陷阱

1. **架构选择与前向兼容性**：仅生成 Cubin（如 `-arch=native` 或 `-gpu-code=sm_XX`）的二进制无法在更新的 GPU 上运行；保留 PTX（如 `-arch=compute_XX`）可确保未来 GPU 通过 JIT 编译支持。发布应用时建议同时包含目标架构的 Cubin 和最新虚拟架构的 PTX。

3. **Host 编译器兼容性**：CUDA Toolkit 对 Host 编译器版本有明确支持策略。使用不兼容的 GCC/Clang 版本可能导致编译失败或运行时异常。

4. **默认链接静态 Runtime**：若需要共享库版本（例如多个插件共享同一 CUDA Runtime 状态），必须显式传递 `--cudart=shared`。

5. **调试与优化的权衡**：`-G` 会显著降低内核性能；若仅需行号映射进行性能分析，使用 `-lineinfo` 即可，不影响执行速度。

6. **Fatbin 压缩**：默认压缩策略为 `speed`。若对二进制体积敏感（如边缘部署），可改为 `size`；若对启动延迟敏感，可显式使用 `speed` 或 `balance`。

7. **寄存器与溢出**：使用 `-res-usage` 和 `-Xptxas=-warn-spills` 可帮助发现寄存器压力过大的内核，及时调整线程块大小或拆分内核逻辑。
