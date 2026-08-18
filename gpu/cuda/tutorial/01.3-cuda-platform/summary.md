## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/01-introduction/cuda-platform.html>

## CUDA 平台概述

本章节介绍 CUDA 平台的核心组成与基本概念，这些内容适用于所有使用 CUDA 平台的编程语言与工具链。

---

### 计算能力（Compute Capability）与 SM 版本

- 每块 NVIDIA GPU 都有一个**计算能力（Compute Capability, CC）**编号，格式为 `X.Y`（如 12.0）。
- CC 直接对应 Streaming Multiprocessor（SM）的版本号，例如 CC 12.0 的 SM 版本为 `sm_120`。
- CC 决定了 GPU 支持的硬件特性以及部分硬件参数，具体可查官方附录。

---

### CUDA Toolkit 与 NVIDIA 驱动

- **NVIDIA Driver**：可视为 GPU 的操作系统，是所有 GPU 功能（包括 CUDA、Vulkan、Direct3D 等）的基础，必须与宿主操作系统一起安装。
- **CUDA Toolkit**：独立的软件开发包，包含库、头文件和工具（如 `nvcc`），用于编写、构建和分析 GPU 程序。
- **CUDA Runtime**：Toolkit 提供的一个特殊库，提供 CUDA Runtime API 和语言扩展，负责内存分配、数据传输、核函数启动等常见任务。

---

### CUDA Runtime API 与 Driver API

- **CUDA Runtime API** 构建在更低层的 **CUDA Driver API** 之上。
- 本指南主要围绕 Runtime API 展开；所有 Runtime API 的功能都可以通过 Driver API 实现，且某些高级特性只能通过 Driver API 使用。
- 两者可以在同一应用中混合使用。

---

### 并行线程执行（PTX）

- **PTX（Parallel Thread Execution）** 是 NVIDIA GPU 的虚拟指令集架构，属于高级汇编语言。
- 它为真实 GPU 的物理 ISA 提供了一层抽象。
- 高级语言编译器可以生成 PTX 作为中间表示（IR），再通过离线或 JIT 编译工具生成可在 GPU 上执行的二进制代码。
- PTX 版本与计算能力对应，例如支持 CC 8.0 的 PTX 称为 `compute_80`。

---

### Cubin 与 Fatbin

- **Cubin**：CUDA 二进制文件，是针对特定 SM 版本（如 `sm_120`）的真实机器码。
- **Fatbin**：包含 CPU 和 GPU 代码的可执行文件或库中用于存储 GPU 代码的容器。一个 fatbin 可以包含多个 cubin（对应不同 SM 版本）以及一个或多个 PTX 版本。
- 程序运行时，CUDA 驱动会从 fatbin 中选择最适合当前 GPU 的二进制代码加载执行。

---

#### 二进制兼容性（Binary Compatibility）

- 在同一**主版本号**的计算能力内，GPU 可以加载并执行针对**相同或更低次版本号**编译的 cubin。
- 例如：为 `sm_86`（CC 8.6）编译的 cubin 可在 CC 8.6 和 CC 8.9 上运行，但**不能**在 CC 8.0 上运行。
- **不同主版本号之间没有二进制兼容性**：例如 `sm_86` 的 cubin 无法在 CC 9.0 的 GPU 上运行。
- 只有 NVIDIA 官方工具（如 `nvcc`）生成的二进制代码才享有兼容性承诺。

#### PTX 兼容性（PTX Compatibility）

- PTX 代码可以在运行时被 JIT 编译为**大于或等于**该 PTX 计算能力的任何 SM 版本的二进制代码。
- 例如：包含 `compute_80` PTX 的应用可以在未来的 `sm_120` GPU 上运行，从而实现对新硬件的**向前兼容**。

#### 即时编译（JIT Compilation）

- 驱动程序在运行时将 PTX 编译为机器码的过程称为 **JIT 编译**。
- JIT 会增加程序启动时间，但带来两个好处：
  1. 可以运行在当时尚未发布的新 GPU 上；
  2. 可以享受新版驱动中编译器改进带来的性能提升。
- 驱动会自动将 JIT 编译结果缓存到 **compute cache** 中，避免重复编译；升级驱动后缓存会自动失效。
- 除 `nvcc` 离线编译外，还可以使用 **NVRTC** 在运行时将 CUDA C++ 代码编译为 PTX。

---

### 关键 API 与注意事项

| 项目 | 说明 |
|------|------|
| `cudaRuntimeGetVersion` / `cudaDriverGetVersion` | 获取 CUDA Runtime / Driver 版本号 |
| `cudaGetDeviceCount` / `cudaGetDeviceProperties` | 查询 GPU 数量及属性（含计算能力） |
| `nvcc -arch=sm_XX` | 指定编译目标 SM 版本，生成对应 cubin |
| `nvcc -gencode arch=compute_XX,code=sm_XX` | 同时生成特定 SM 的 cubin 与 PTX |
| Fatbin | 可包含多架构 cubin + PTX，运行时自动匹配最佳代码 |
| 兼容性原则 | 同主版本内向下兼容（次版本号 >= 目标次版本号）；PTX 提供跨主版本的向前兼容 |

**注意事项**：
- 生产环境建议为常见目标架构都生成 cubin，并附带一个较低版本的 PTX，以兼顾性能与向前兼容性。
- 仅通过 `nvcc` 生成的原始二进制文件享有官方兼容性承诺，手动修改二进制将失效。
- JIT 编译虽提供灵活性，但会增加首次启动延迟；对启动时间敏感的应用应尽量减少对 JIT 的依赖。
