## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/mathematical-functions.html>


本章是《CUDA Programming Guide》第 5 章（技术附录）中关于**浮点计算（Floating-Point Computation）**的完整说明。它并非简单的 API 速查，而是从 IEEE-754 标准出发，系统性地讲解了 CUDA 架构中浮点数的表示、精度、舍入行为、特殊值语义、运算结合性、Fused Multiply-Add（FMA）机制，以及 CUDA 各类数学库的分层与误差界限。理解这些内容对于在异构计算环境中编写**结果可复现、精度可控、性能最优**的数值代码至关重要，因为 GPU 与 CPU 在执行同一数学表达式时，可能因编译器优化、FMA 硬件支持、函数实现差异而给出不同的比特级结果。

## 背景与要解决的问题

自 1985 年 IEEE-754 标准成为主流以来，几乎所有计算系统（包括 NVIDIA GPU）都遵循该标准对二进制浮点格式的规定。然而，"遵循标准"不等于"所有平台结果完全一致"。在异构计算中，开发者经常面临以下问题：

- **同一表达式在 CPU 与 GPU 上结果不同**：这不仅源于硬件差异，还可能是因为编译器默认开启或关闭了 FMA、进行了不同的循环展开或常量折叠。
- **精度与性能之间的权衡**：Subnormal（非规格化数）保证精度渐进衰减，但会显著降低吞吐量；而 flush-to-zero 可以提升性能，却可能在极小数值区域引入误差。
- **数学库的精度差异**：IEEE-754 仅规定了基本运算（加减乘除、平方根、FMA 等）的舍入要求，对于三角函数、指数函数等并未强制要求正确舍入，因此不同平台、不同优化级别下的实现误差不同。
- **调试困难**：GPU 端不设置 errno、不报告浮点异常，使得传统 CPU 调试手段失效。

本章的目的正是帮助开发者建立对浮点行为的全局认知，以便在需要时做出合理的编译选项选择与算法调整。

## 核心概念与术语

### IEEE-754 编码结构

任何二进制浮点数由三个字段构成：
- **Sign（符号位）**：1 位，决定正负。
- **Exponent（指数位）**：经 Bias 偏移后的无符号整数，float 的偏移量为 127，double 为 1023。
- **Significand / Mantissa（尾数位）**：规格化数隐含最高位的 1，因此 float 的 23 位显式尾数实际提供 24 位精度。

其数值计算公式为：
- 规格化（Normal）：(−1)^sign × 1.mantissa × 2^(exponent−bias)
- 非规格化（Subnormal）：(−1)^sign × 0.mantissa × 2^(1−bias)

CUDA 支持的精度类型包括 Bfloat16（`__nv_bfloat16`）、Half（`__half`）、Single（`float`）、Double（`double`）和 Quad（`__float128` / `_Float128`）。此外，还有专门面向 Tensor Core 的 TF32、FP8、FP6、FP4 等微缩格式，它们不属于通用浮点计算范畴。

### 特殊值语义

- **零**：存在 `+0` 与 `-0`，但 `+0 == -0` 为 true。
- **无穷大（Infinity）**：指数全 1、尾数全 0。饱和运算中溢出会产生 Inf；`Inf * 0.0`、`Inf - Inf` 等不定式产生 NaN。
- **非数（NaN）**：指数全 1、尾数非 0。NaN 具有**非自反性**：`NaN == NaN` 为 false，`NaN != NaN` 为 true。任何涉及 NaN 的有序比较均返回 false。NaN 分为 Quiet NaN（qNaN，用于传播错误，最高尾数位为 1）与 Signaling NaN（sNaN，用于触发异常，最高尾数位为 0）。CUDA 提供 `cuda::std::numeric_limits<T>::quiet_NaN()` 等常量获取具体编码。

### Normal 与 Subnormal（Denormal）

Subnormal 填补了最小规格化正数（`FLT_MIN`）与 0 之间的空隙，实现精度渐进丢失。但它们在 GPU 上计算代价高昂。通过 `nvcc` 的 `-ftz=true`（或 `--use_fast_math`，其内部包含 `-ftz=true`）可将 Subnormal 刷新为 0，显著提升性能，但会牺牲极小数值的精度。

### ULP（Unit in the Last Place）

ULP 用于量化浮点函数的误差。一个函数的"最大 ULP 误差"指的是：在大量测试输入上，函数返回值与对应精度下"正确舍入到最近偶数"结果之间相差的 ULP 数的最大绝对值。ULP 为 0 表示函数结果是 correctly rounded 的。

### 结合性（Associativity）与舍入

有限精度浮点加法与乘法**不满足结合律**。例如：
```
A = 2^1 * 1.000...01
B = 2^0 * 1.000...01
C = 2^3 * 1.000...01
```
在数学上 `(A+B)+C == A+(B+C)`，但在 round-to-nearest 的单精度浮点中，由于中间结果需不断舍入到可表示值，两种结合顺序会得出不同的二进制结果。这说明：即使每一步都符合 IEEE-754，**运算顺序也会影响最终精度**。

## API / 机制详解

### 1. Fused Multiply-Add（FMA）

FMA 计算 `a * b + c` 时，乘积以扩展精度（相当于两倍宽）保存，最后与 c 相加后仅执行**一次舍入**。相比之下，分离的乘法与加法需要两次舍入，容易在"相消抵消（subtractive cancellation）"场景中丢失全部有效位。

CUDA 提供多种方式使用 FMA：
- **隐式使用**：编译标志 `-fmad=true`（默认开启）或 `--use_fast_math` 会让编译器将 `x * y + z` 自动收缩为 FMA。
- **标准库函数**：`fma(x,y,z)` / `fmaf(x,y,z)`（C 标准库），`cuda::std::fma(x,y,z)`（CUDA C++ 标准库）。
- **Intrinsics**：`__fmaf_rn` / `__fma_rn` 等，可显式指定舍入模式；还有 IEEE 严格版本 `__fmaf_ieee_rn` 等。

在比较 CPU 与 GPU 结果时，必须注意主机平台是否同样支持 FMA（如 x86 AVX2 `-mfma`、Arm64 Neon）。

### 2. 舍入模式（Rounding Modes）

IEEE-754 定义四种舍入模式，CUDA 均支持：
- **rn**：Round to nearest, ties to even（默认）
- **rz**：Round toward zero（截断）
- **ru**：Round toward +infinity（向上）
- **rd**：Round toward −infinity（向下）

CUDA **没有全局动态舍入模式寄存器**，默认全部使用 rn。若要对单条指令指定其他模式，必须调用带后缀的 intrinsic，例如 `__fadd_rz`、`__fmul_ru`、`__fdiv_rd`、`__fmaf_rn` 等。这类函数是 device-only 的，且具有 0 ULP 误差。

### 3. 点积的三种算法

以两个四维向量点积为例，三种实现策略的结果可能不同：
- **算法 1（顺序乘加）**：`((((a1*b1) + (a2*b2)) + (a3*b3)) + (a4*b4))`。每次乘法后舍入，每次加法后舍入。
- **算法 2（顺序 FMA）**：`(a4*b4) + ((a3*b3) + ((a2*b2) + (a1*b1 + 0)))`。每一步使用 FMA，减少中间舍入。
- **算法 3（分治/并行）**：`((a1*b1)+(a2*b2)) + ((a3*b3)+(a4*b4))`。两个子部分可并行计算，但最终相加时两个部分和已分别被舍入。

这三种算法都符合 IEEE-754，但因舍入顺序与 FMA 使用不同，最终比特级结果可能不一致。开发者应根据精度需求与并行度选择合适的归约策略。

### 4. CUDA 数学功能的分层暴露

CUDA 将数学功能分为五个层级，它们在**支持类型、主机/设备可用性、受编译标志影响程度、精度**上各不相同：

#### (a) 内置 C/C++ 运算符
`+`、`-`、`*`、`/`、`++`、`--` 等。支持 `float`、`double`、`__half`、`__nv_bfloat16`、`__float128` 以及 `cuda::std::complex`。host 与 device 均可用，行为受 `nvcc` 优化标志影响。对于 `float` 和 `double`，保证 0 ULP 误差（rn 模式）。当 `-fmad=true` 时，`x*y+z` 可能被收缩为 FMA；当 `-prec-div=false` 时，除法最大误差变为 2 ULP。

#### (b) CUDA C++ Standard Library Mathematical Functions
位于 `<cuda/std/cmath>`，命名空间 `cuda::std::`。完整映射 C++ `<cmath>`，支持 `__half`、`__nv_bfloat16`、`float`、`double`、`__float128`。host 与 device 均可用，但底层可能依赖 CUDA Math API，因此 host 与 device 的误差可能不同。 constexpr 支持部分函数。精度以最大 ULP 误差给出，例如 `expf` 为 2 ULP，`exp` 为 1 ULP，`powf` 为 4 ULP。

#### (c) CUDA C Standard Library Mathematical Functions（CUDA Math API）
对应传统 C `<math.h>`，支持 `float` 与 `double`，`__half` 和 `__nv_bfloat16` 有限支持，`__float128` device-only。无需额外头文件即可使用（`__float128` 除外）。是 host 和 device 代码最常见的入口。

#### (d) Non-standard CUDA Mathematical Functions
不属于 C/C++ 标准的扩展，例如 `exp10f`、`rsqrtf`、`cyl_bessel_i0f`、`normcdff` 等。大部分是 device-only。误差 bounds 在文档中逐函数给出，例如 `rsqrtf` 为 2 ULP，`rcbrt` 为 1 ULP。对于 `__half` 和 `__nv_bfloat16`，也有对应的 `hexp10`、`hrsqrt` 等。

#### (e) Intrinsic Mathematical Functions
以双下划线前缀命名，如 `__sinf`、`__expf`、`__logf`、`__powf`。它们是**更快但更低精度**的 device-only 实现，映射到更少的原生指令。`--use_fast_math` 会自动将标准 Math API 翻译成这些 intrinsic。它们的**行为不受** `-prec-div=false`、`-prec-sqrt=false`、`-fmad=true` 影响，唯独受 `-ftz=true`（或 `--use_fast_math`）影响。精度误差显著高于标准库版本，例如 `__expf` 的误差为 `2 + floor(|1.173*x|)` ULP，`__sinf` 在 `[-pi, pi]` 内约为 `2^-21.41` 绝对误差。

### 5. 编译标志对浮点语义的影响

- `-ftz={true|false}`：是否将 Subnormal flush to zero。
- `-prec-div={true|false}`：控制单精度除法精度；false 时映射到 `__fdividef`（2 ULP）。
- `-prec-sqrt={true|false}`：控制单精度平方根精度。
- `-fmad={true|false}`：是否允许将乘加收缩为 FMA。
- `--use_fast_math`：同时启用 `-ftz=true`、`-prec-div=false`、`-prec-sqrt=false`、`-fmad=true`，并将大量 Math API 替换为 intrinsic。
- `-G`：关闭所有 device 代码优化，可用于对比优化前后的结果差异。

## 典型工作流程 / 调用顺序

在实际开发中，通常按以下流程处理浮点需求：

1. **确定精度与范围需求**：根据数值范围选择 `float`、`double` 或 `__float128`；若仅需存储与低带宽，可考虑 `__half`。
2. **选择函数层级**：若追求最高精度且跨平台一致性重要，优先使用 `cuda::std::` 或标准 Math API；若在性能关键路径且可接受稍大误差，使用 intrinsic 或开启 `--use_fast_math`。
3. **配置编译标志**：
   - 默认场景：保持 `-ftz=false`、`-prec-div=true`、`-prec-sqrt=true`。
   - 性能优先且数值范围远离 Subnormal：启用 `--use_fast_math`。
   - 需要强制分离乘法和加法（例如对比 CPU 非 FMA 结果）：使用 `__fmul_rn` + `__fadd_rn`，或关闭 `-fmad=false`。
4. **处理 Host/Device 差异**：避免直接进行比特级结果比较；如需对比，应使用允许一定 ULP 差异的阈值比较，或统一在 host 和 device 端都使用 `std::fma` / `cuda::std::fma`。
5. **原子操作与 Denormal**：若使用单精度浮点原子加，需注意 global memory 始终 flush-to-zero，而 shared memory 保留 Subnormal 支持。这一差异**不受** `-ftz` 编译标志控制。

## 关键限制、边界条件与兼容性

### IEEE-754 合规性限制

CUDA 设备遵循 IEEE 754-2019，但存在以下限制：
- **无动态舍入模式**：不能通过修改全局状态改变舍入模式，只能通过不同名称的 intrinsic 函数逐指令选择。
- **无浮点异常检测**：所有操作 behave as if 异常被 mask。即使存在 sNaN 编码，也不会触发硬件异常，而是被当作 qNaN 处理。
- **NaN Payload 不保证保留**：运算可能改变输入 NaN 的 payload；`abs()` 和取反操作可能以实现定义的方式修改 NaN 的符号位。

### 浮点到整数转换的未定义行为

当浮点值舍入后超出目标整数类型范围时，直接通过 PTX 转换指令会 clamp 到整数范围，但如果编译器通过优化路径（而非显式 PTX intrinsic）执行转换，则属于 C/C++ 未定义行为。例如 `__double2int_rz()` 与主机编译器行为可能不同。

### 数据类型的硬件可用性

| 类型 | 最低计算能力 | 备注 |
|---|---|---|
| `__half` | sm_53 | sm_61 支持存储与部分运算，但部分操作可能模拟 |
| `__nv_bfloat16` | sm_80 | sm_61 不可用 |
| `double` | sm_13+ | sm_61 完全支持 |
| `__float128` / `_Float128` | sm_100 | 需主机编译器支持，device-only 数学函数需 `<crt/device_fp128_functions.h>` |
| WMMA | sm_70 | Tensor Core 专用 |

### 原子操作与 Denormal

- **Global memory `atomicAdd(float*, float)`**：始终等效于 `add.rn.ftz.f32`，即 Subnormal 被 flush to zero。
- **Shared memory `atomicAdd(float*, float)`**：始终等效于 `add.rn.f32`，保留 Subnormal 行为。

## 常见陷阱与调试建议

### 陷阱 1：假设浮点运算满足结合律与分配律

在编写并行归约（reduction）或矩阵乘法时，不同线程、不同分块的求和顺序会改变最终结果。调试时应认识到：只要在 ULP bounds 内，差异即为正常，而非 bug。

### 陷阱 2：盲目比较 CPU 与 GPU 的比特级结果

如果主机编译器未开启 FMA（如未使用 `-mfma`），而 `nvcc` 默认开启 `-fmad=true`，同一表达式的 CPU 与 GPU 结果几乎必然不同。调试时应使用相对误差或 ULP 差异作为判断标准。

### 陷阱 3：开启 `--use_fast_math` 后未检查精度是否可接受

`--use_fast_math` 不仅改变编译标志，还会将 `sinf`、`cosf`、`expf`、`logf` 等替换为 `__sinf`、`__cosf` 等 intrinsic。这些 intrinsic 的误差可能是输入相关的（如 `__expf` 的误差随 `|x|` 线性增长），在科学计算中可能导致灾难性精度损失。

### 陷阱 4：忽视 NaN 的比较语义

代码中若使用 `if (x == x)` 判断 NaN 会永远失败；应使用 `isnan(x)`（或 `__hisnan`、`cuda::std::isnan`）。同样，`NaN < 5.0f` 也为 false，这可能导致排序或 min/max 逻辑异常。

### 陷阱 5：Subnormal 导致性能骤降

若程序在极小数值区域运行缓慢，检查是否产生了大量 Subnormal。可在数值安全的前提下使用 `-ftz=true` 提升性能。

### 陷阱 6：混淆 Host 与 Device 数学库实现

`erfinv`、`lgamma`、`j0` 等函数在 host 与 device 上是独立实现的。若结果差异较大，首先查阅文档中的最大 ULP 误差表，确认差异是否在预期范围内。

### 调试建议

- 使用 `cuda::std::numeric_limits<T>` 查询各浮点类型的属性（最小正值、epsilon、最大指数等）。
- 对关键计算路径，使用带舍入模式后缀的 intrinsic（如 `__fadd_rn`）强制固定舍入行为，排除编译器重结合的影响。
- 使用 `-G` 编译无优化版本，与 `-O2` / `-O3` 版本对比，定位是否由常量折叠、循环展开或重结合引起差异。
- 若需 deterministic 结果，确保固定硬件、编译器版本、线程配置，并避免依赖原子操作顺序。

## 一个最小可运行示例的说明

本目录下的 `chapter_demo.cu` 是一个面向 **sm_61（GTX 1060）** 的最小完整示例，使用 `nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2` 编译。它不包含任何 sm_61 不支持的特性（如 WMMA、`__nv_bfloat16`、`__float128`），而是通过 `float` 与标准 intrinsic 演示本章的四个核心概念。

### 示例结构与演示目标

1. **Associativity（结合性）**：
   - 选取 `a = 16777216.0f`（即 2^24）、`b = 1.0f`、`c = -16777216.0f`。
   - 在 float 中，2^24 处的 ulp 为 2，因此 `a + b` 恰好处于半 ulp 位置，向最近的偶数舍入后回到 `a`；于是 `(a+b)+c` 得到 `0.0f`。
   - 而 `b + c` 得到 `-16777215.0f`（可精确表示），再加 `a` 得到 `1.0f`。
   - 该 kernel 直观地展示了相同操作数因结合顺序不同而产生截然不同的结果。

2. **FMA 精度优势（相消抵消场景）**：
   - 选取 `A = 1.0f + 2^-23`、`B = -(1.0f + 2^-22)`，计算 `A*A + B`。
   - 数学上 `A*A = 1 + 2^-22 + 2^-46`，加 `B` 后精确结果为 `2^-46`。
   - 使用 `__fmul_rn` 与 `__fadd_rn` 强制分离乘法与加法：中间乘积舍入后 `2^-46` 因低于半 ulp 而丢失，再加 `B` 得到 `0.0f`。
   - 使用 `__fmaf_rn` 执行真正的 FMA：乘积以扩展精度保留，与 `B` 相加时仅一次舍入，成功保留 `2^-46`（约 `1.42e-14`）。
   - 该示例直接复现了原文中"分离运算导致全部精度丢失，FMA 得到数学精确值"的经典场景。

3. **特殊值行为（IEEE-754）**：
   - 通过 `1.0f / 0.0f` 产生 `+inf`，`-1.0f / 0.0f` 产生 `-inf`，`0.0f / 0.0f` 产生 `NaN`。
   - 验证 `NaN == NaN` 为 false、`+0.0f == -0.0f` 为 true、`inf * 0.0f` 产生 NaN 等关键语义。

4. **四种舍入模式**：
   - 以 `1.0f / 10.0f`（二进制无限循环小数）为输入，分别调用 `__fdiv_rn`、`__fdiv_rz`、`__fdiv_ru`、`__fdiv_rd`。
   - 输出展示四种模式对同一不可精确表示值的不同取舍方向，验证 `RD <= RN <= RU`（正值下）以及 RZ 的截断特性。

### sm_61 兼容性处理

- **设备检测**：`main()` 中调用 `cudaGetDeviceProperties`，若检测到计算能力低于 sm_70，则打印提示：WMMA 不可用、`__half` 算术有限、float/double 完全支持。若低于 sm_80，提示 `__nv_bfloat16` 不可用。
- **避免不支持的类型与 API**：全程仅使用 `float` 和标准单精度浮点 intrinsic（`__fmul_rn`、`__fadd_rn`、`__fmaf_rn`、`__fdiv_*`），这些在 sm_61 上均有原生指令支持。
- **错误检查**：所有 `cudaMalloc`、`cudaMemcpy`、kernel 启动与同步均通过 `CUDA_CHECK` 宏检查错误，符合生产代码规范。

通过编译并运行 `make run`，开发者可以在 sm_61 硬件上直接观察本章所述的浮点特性，而无需担心架构不兼容导致的编译失败或运行时异常。
