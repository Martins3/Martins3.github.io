## 章节概述


## The CUDA platform 章节总结

本章节介绍了 CUDA 平台的核心软硬件概念，包括内存层次结构、计算能力（Compute Capability）、CUDA 工具链与驱动、PTX 中间指令集、以及 GPU 二进制的兼容性与 JIT 编译机制。理解这些概念对于编写可移植、高性能的 CUDA 程序至关重要。

---

### 1. 内存层次结构与缓存

GPU 的内存资源在 SM（Streaming Multiprocessor）内部分层管理：

- **寄存器文件（Register File）**：按线程分配。若一个线程块所需的寄存器总量（`每线程寄存器数 × 线程数`）超过 SM 的寄存器文件容量，内核将无法启动，必须减小线程块尺寸。
- **共享内存（Shared Memory）**：在线程块级别统一分配，块内所有线程共享同一份共享内存。
- **L1 缓存**：属于统一数据缓存的一部分，每个 SM 独立拥有。
- **L2 缓存**：更大容量，由 GPU 上所有 SM 共享。
- **常量缓存（Constant Cache）**：每个 SM 拥有独立的常量缓存，用于缓存被声明为 `constant` 的全局内存数据，编译器也可能将内核参数放入常量内存以提升访问效率。

### 2. Unified Memory（统一内存）

传统上，CPU 内存和 GPU 内存互相独立，必须通过 `cudaMemcpy` 等 API 显式搬运数据。

**Unified Memory** 允许分配一块可被 CPU 和 GPU 共同访问的内存。CUDA 运行时或底层硬件会在访问发生时自动将数据迁移到正确的物理位置。

- **最佳实践**：即使使用统一内存，也应尽量减少数据迁移，并尽可能让处理器直接访问与其物理相连的内存，以获得最佳性能。
- 具体行为取决于系统硬件类别，详见文档 "Unified Memory" 章节。

### 3. Compute Capability（计算能力）

- 每款 NVIDIA GPU 都有一个 **Compute Capability（CC）** 号，格式为 `X.Y`（`X` 为主版本，`Y` 为次版本）。
- CC 直接对应 SM 的版本号。例如，CC 12.0 的 GPU 内部 SM 版本为 `sm_120`。
- CC 决定了 GPU 支持的特性集以及部分硬件参数（如寄存器数量、共享内存容量等）。
- 可通过 `cudaGetDeviceProperties` 查询当前 GPU 的 CC。

### 4. CUDA Toolkit 与 NVIDIA Driver

- **NVIDIA Driver**：可视为 GPU 的操作系统，必须安装在主机操作系统上。它不仅是 CUDA 的基础，也支持 Vulkan、Direct3D 等其他 GPU 使用方式。
- **CUDA Toolkit**：包含编写、构建和分析 GPU 程序所需的库、头文件和工具（如 `nvcc`）。它与 NVIDIA Driver 是独立的软件产品。
- **CUDA Runtime**：Toolkit 提供的一个特殊库，包含内存分配、数据拷贝、内核启动等常用功能的 API 和语言扩展，称为 **CUDA Runtime API**。

#### Runtime API vs Driver API

- **CUDA Runtime API** 构建在底层的 **CUDA Driver API** 之上。
- 本指南主要关注 Runtime API；所有 Runtime API 的功能均可通过 Driver API 实现，且某些高级特性仅能通过 Driver API 使用。
- 两种 API 可在同一应用中互操作。

### 5. PTX（Parallel Thread Execution）

- PTX 是一种针对 NVIDIA GPU 的**高级汇编语言 / 虚拟指令集架构（ISA）**，作为物理 GPU ISA 的抽象层。
- 领域专用语言（DSL）和高级语言编译器可将 PTX 作为**中间表示（IR）**，再经 NVIDIA 的离线或 JIT 编译器生成实际的 GPU 机器码。
- PTX 版本与 CC 对应。例如，支持 CC 8.0 全部特性的 PTX 版本称为 `compute_80`。

### 6. Cubin 与 Fatbin

- **Cubin**：CUDA binary，针对特定 SM 版本（如 `sm_120`）的真实机器码。
- **Fatbin**：可执行文件或库中用于封装 GPU 代码的容器。一个 fatbin 可以包含：
  - 多个不同 SM 版本的 cubin；
  - 一个或多个版本的 PTX 代码。
- 程序运行时，CUDA 驱动会从 fatbin 中自动选择最适合当前 GPU 的二进制代码执行。

### 7. 二进制兼容性与 JIT 编译

#### 7.1 Binary Compatibility

- 在**同一个 major version** 内，GPU 的 minor version **大于等于** cubin 编译目标时，可直接加载并执行该 cubin。
  - 例如：为 `sm_86` 编译的 cubin 可在 CC 8.6 和 8.9 上运行，但不能在 8.0 上运行（因为 minor 0 < 6）。
- **不同 major version 之间没有二进制兼容性**。例如 `sm_86` 的 cubin 无法在 CC 9.0 的 GPU 上运行。
- 兼容性承诺仅适用于 NVIDIA 官方工具（如 `nvcc`）生成的二进制文件。手动修改二进制将导致兼容性失效。

#### 7.2 PTX Compatibility

- 当 fatbin 中不包含可直接匹配的 cubin 时，若包含对应（或更低）CC 的 PTX 代码，驱动可在运行时通过 **JIT（Just-In-Time）编译** 将其编译为目标 GPU 的机器码。
- PTX 的 JIT 编译规则：**PTX 可向前兼容**，即 `compute_80` 的 PTX 可被 JIT 编译到 `sm_120` 等更高 CC 的目标上。这使得应用无需重新编译即可支持未来发布的 GPU。

#### 7.3 JIT 编译细节

- JIT 由设备驱动完成，会增加程序加载时间，但能让应用享受新驱动带来的编译器优化，并支持编译时还不存在的 GPU。
- 驱动会自动缓存 JIT 生成的二进制代码到 **compute cache** 中，避免重复编译。驱动升级时缓存会自动失效，以便应用受益于新版 JIT 编译器。
- 除 `nvcc` 外，**NVRTC** 也支持在运行时将 CUDA C++ 代码编译为 PTX。

---

## 核心概念速查表

| 概念 | 含义 | 关键要点 |
|------|------|----------|
| **Compute Capability (CC)** | GPU 功能与硬件参数版本 | 格式 `X.Y`；同 major 内向下二进制兼容；不同 major 不兼容 |
| **SM 版本** | 物理流多处理器版本 | 与 CC 一一对应，如 CC 12.0 对应 `sm_120` |
| **PTX** | 虚拟 ISA / 高级汇编 | `compute_XY` 命名；可作为 IR；支持向前 JIT 编译 |
| **Cubin** | 针对特定 SM 的真实二进制 | 如 `sm_120`；只能在兼容的 GPU 上直接运行 |
| **Fatbin** | 包含多种 GPU 代码的容器 | 可包含多个 cubin 和多个 PTX，运行时自动选择最佳匹配 |
| **Unified Memory** | CPU/GPU 统一访问的内存 | 由运行时自动迁移；最佳性能仍需减少迁移 |
| **CUDA Runtime API** | 高级易用 API | 构建在 Driver API 之上，适合大多数开发场景 |
| **CUDA Driver API** | 底层 API | 更灵活，部分高级特性仅此 API 支持 |

---

## 关键 API / 函数

| API / 函数 | 所属层级 | 作用 |
|------------|----------|------|
| `cudaGetDeviceProperties` | Runtime API | 查询 GPU 属性（CC、SM 数量、内存容量、统一内存支持等） |
| `cudaMalloc` / `cudaFree` | Runtime API | 在 GPU 上显式分配 / 释放设备内存 |
| `cudaMemcpy` | Runtime API | 在 Host 与 Device（或 Device 之间）显式拷贝数据 |
| `cudaMallocManaged` | Runtime API | 分配 Unified Memory，可被 CPU 和 GPU 共同访问 |
| `cudaLaunchKernel` | Runtime API | 启动 CUDA 内核（通常由 `<<< >>>` 语法隐式调用） |
| `nvcc` | Toolkit 工具 | NVIDIA CUDA 编译器，负责将 C++ / CUDA 代码编译为 PTX 和 cubin |

---

## 注意事项与常见陷阱

1. **寄存器分配超限导致内核无法启动**
   - 若 `threads_per_block × registers_per_thread > SM_register_file_size`，内核将启动失败。可通过减小线程块大小、减少内核中变量数量或使用 `__launch_bounds__` 提示编译器来控制寄存器使用。

2. **二进制兼容性误区**
   - 误以为高 minor 版本的 cubin 可以在低 minor 版本上运行（如 `sm_86` 在 8.0 上运行）。**实际上 minor 只能向上兼容，不能向下兼容。**
   - 误以为不同 major 版本之间有二进制兼容性（如 `sm_86` 在 9.0 上运行）。**Major 版本之间完全不兼容。**

3. **忽视 PTX 的前向兼容价值**
   - 若应用只包含特定 SM 的 cubin，未来新 GPU 发布时可能无法直接运行。建议在 fatbin 中嵌入适当版本的 PTX，以利用 JIT 编译获得前向兼容性。

4. **Unified Memory 的隐性开销**
   - 虽然统一内存简化了编程，但频繁的页迁移会带来显著性能损失。性能敏感代码应仍使用显式 `cudaMemcpy` 或确保数据局部性。

5. **Driver API 与 Runtime API 混用时的上下文管理**
   - 两者可互操作，但 Driver API 要求显式管理 CUDA context。若混用不当，可能导致上下文错误或资源泄漏。

6. **JIT 编译的加载延迟**
   - 首次在新驱动或新 GPU 上运行含 PTX 的应用时，驱动会进行 JIT 编译，导致启动时间增加。这是正常现象，后续运行会利用 compute cache 加速。

7. **常量缓存的隐式使用**
   - 编译器可能自动将内核参数放入常量内存。若内核参数过大，可能超出常量内存限制，导致编译错误或性能下降。
