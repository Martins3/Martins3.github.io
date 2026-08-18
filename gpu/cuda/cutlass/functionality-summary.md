# CUTLASS C++ Functionality 总结

来源：NVIDIA CUTLASS 官方文档  
链接：https://docs.nvidia.com/cutlass/latest/media/docs/cpp/functionality.html

本文总结 CUTLASS C++ `Functionality` 页面所描述的能力范围。该页面本质上不是一篇教程，而是一张能力矩阵：它回答的是 CUTLASS 在不同 CUDA Toolkit、GPU 计算能力、算子类型、数据类型、矩阵布局和执行后端上，已经提供哪些 device-level 或 warp-level kernel 模板，以及这些模板大致适用于哪些硬件路径。

## 总体定位

CUTLASS 是 NVIDIA 提供的 CUDA C++ 模板库，核心目标是把高性能矩阵乘、卷积以及相关线性代数算子的实现组件化。`Functionality` 页面强调一个前提：CUTLASS 3.x 要求 CUDA 11.4 或更新版本，并且目标架构至少是 SM70。虽然 CUTLASS 2.x 表格中仍然列出了一些 SM50、SM60、SM61 的 SIMT 或早期 Tensor Core 路径，但从新开发角度看，SM70 以上才是主线。

页面开头还解释了符号约定。`N` 表示 column-major 矩阵，`T` 表示 row-major 矩阵，`{N,T} x {N,T}` 表示输入 A 和 B 支持 NN、NT、TN、TT 四种组合。卷积布局中，`NHWC` 是常见四维张量布局，`NCxHWx` 是 interleaved 四维张量布局。数据类型缩写包括 `f` 浮点、`s` 有符号整数、`b` bit、`cf` complex float、`bf16` bfloat16、`tf32` tfloat32。执行路径方面，`Simt` 表示普通 CUDA Core MMA，`TensorOp` 表示 Tensor Core MMA，`SpTensorOp` 表示稀疏 Tensor Core MMA，`WmmaTensorOp` 表示通过 WMMA 抽象使用 Tensor Core。

## 基础概念补充

### 什么是 Device-level GEMM

`Device-level GEMM` 指的是可以从 host 端直接调用、在整个 GPU device 上完成完整矩阵乘的 GEMM operator。这里的 device-level 是 CUTLASS 的抽象层级名称，不只是泛泛地说代码运行在 GPU 上。

CUTLASS 中可以把 GEMM 拆成几层理解：

```text
Device-level
  Threadblock-level
    Warp-level
      Instruction-level
```

`Instruction-level` 面向单条 MMA 或 Tensor Core 指令，通常只处理一个很小的矩阵 fragment。`Warp-level` 表示一个 warp 协作完成一个小 tile。`Threadblock-level` 表示一个 CUDA block 协作完成更大的 tile，通常还会管理 shared memory staging、同步和流水。`Device-level` 则是最上层的完整算子：它接收完整的 A、B、C、D 矩阵指针和完整的 `M x N x K` 问题规模，负责启动 CUDA kernel，并让很多 threadblock 覆盖整个输出矩阵。

因此，文档中的 `Device-level GEMM` 可以理解成“用户可直接调用的完整 GPU 级矩阵乘封装”。典型形式类似：

```cpp
using Gemm = cutlass::gemm::device::Gemm<
    float, cutlass::layout::ColumnMajor,
    float, cutlass::layout::ColumnMajor,
    float, cutlass::layout::ColumnMajor>;
```

这个名字强调它不是单条 Tensor Core 指令，也不是一个 warp tile 或一个 block tile，而是面向整个 CUDA device 的顶层 GEMM operator。用户调用它时，关心的是完整 GEMM：

```text
D = alpha * A * B + beta * C
```

至于内部怎样拆 threadblock、怎样组织 warp、怎样选择 MMA instruction shape，则由 CUTLASS 模板参数和对应 kernel 实现决定。

### 什么是 GEMM convolution

`GEMM convolution` 的意思是把卷积运算转换成矩阵乘法来执行。GEMM 是 General Matrix Multiply，也就是通用矩阵乘：

```text
C = A x B
```

卷积表面上是对输入特征图做滑动窗口计算。例如一个 2D convolution 可以写成：

```text
output[n, h, w, k] =
  sum over r, s, c input[n, h+r, w+s, c] * filter[r, s, c, k]
```

这里每一个输出元素，本质上都是一个 input patch 和一个 filter 的点积。把所有输出位置和所有输出通道的点积排成矩阵形式后，就可以把卷积变成一次 GEMM。若输入是 `N x H x W x C`，filter 是 `R x S x C x K`，输出是 `N x P x Q x K`，一种常见展开方式是：

```text
A: (N * P * Q) x (R * S * C)
B: (R * S * C) x K
C: (N * P * Q) x K
```

这样卷积就变成：

```text
C = A x B
```

传统实现可能会先把 input 的滑动窗口显式展开成大矩阵，这通常叫 `im2col`。这种方式概念直观，但会产生很大的中间矩阵，占用额外内存并增加带宽压力。

CUTLASS 文档中说的 `implicit GEMM convolution` 重点在 `implicit`：它不是先真的生成 `im2col` 后的 A 矩阵，而是在 kernel 内部按 GEMM 的坐标映射方式，边计算边从原始 tensor 中取出对应 input 元素和 filter 元素。也就是说，数学上按 GEMM 组织，存储上不显式 materialize 中间矩阵。

因此可以这样区分：

```text
GEMM convolution = 用矩阵乘法形式实现卷积
implicit GEMM convolution = 不显式 im2col，在 kernel 内隐式映射成 GEMM
```

这也是为什么 CUTLASS 能把卷积能力和 GEMM 能力放在同一个 functionality 页面中讨论：卷积 kernel 的高性能实现大量复用了 GEMM 的 tiling、MMA 指令、shared memory layout 和数据类型支持。

## Device-level GEMM

GEMM 是该页面最核心的部分。CUTLASS 按 opcode class、compute capability、CUDA Toolkit、数据类型和布局列出 device-level GEMM kernel。这里可以把支持能力理解为几层：

第一层是基础 SIMT GEMM。SM50+ 支持 FP32、FP64 这类传统浮点矩阵乘；SM60+ 开始覆盖 FP16 SIMT；SM61+ 覆盖 INT8 输入、INT32 累加并输出 INT32 或 INT8 的路径。这些路径不依赖 Tensor Core，通常是兼容性基础。

第二层是 WMMA TensorOp。SM70+ 开始出现 FP16 Tensor Core 的 WMMA 路径，既支持 FP16 累加输出，也支持 FP32 累加再输出 FP16 或 FP32。SM75+ 扩展到 INT8、INT4、bit-level 等更低精度整数路径。WMMA 更像 CUDA 提供的 warp-level Tensor Core 抽象接口，CUTLASS 在其上封装可复用模板。

第三层是 CUTLASS TensorOp 主路径。SM70/SM75/SM80/SM90 各自提供更直接、更丰富的 Tensor Core kernel。SM70/SM75 主要覆盖 FP16 和 INT8/INT4/bit 等类型；SM80 以后能力显著扩展，包含 BF16、TF32、FP64 Tensor Core、复数 FP32/FP64 等路径，还支持多种 `{N,T}` 输入布局组合。SM80 也是稀疏 Tensor Core 支持的重要分界，页面列出了 FP16、BF16、TF32、INT8、INT4 等 SpTensorOp GEMM。SM90 相关条目则面向 Hopper 及之后，包含 FP64 TensorOp；CUTLASS 3.x 表格还列出 SM90a 上的 FP16、BF16、TF32/FP32、INT8 等新 kernel，要求 CUDA 12.0+。

从使用角度看，GEMM 表格告诉用户两件事：一是某种数据类型组合是否存在官方模板实例或测试；二是布局组合是否覆盖。比如 FP32 SIMT 通常覆盖 `{N,T} x {N,T}` 到 `{N,T}`，而某些 TensorOp 或整数路径只列出特定布局，例如 A 为 row-major、B 为 column-major，再输出 N/T。工程上选择 CUTLASS kernel 时，不能只看数据类型，还必须同时看布局、目标 SM 和 Toolkit 版本。

## Device-level Implicit GEMM Convolution

页面第二个重要部分是 device-level implicit GEMM convolution。CUTLASS 把卷积转换为隐式 GEMM 的形式执行，因此卷积支持矩阵也按 opcode class、计算能力、数据类型和布局列出。表格主要展示 conv2d fprop 的支持情况，并说明 dgrad 和 wgrad 可以找到或构造对应 operator。

卷积部分的低端支持从 SM50 SIMT 开始，包括 FP32 和 complex FP32 的 NHWC 路径。SM70/SM75 TensorOp 支持 FP16 输入、FP32 累加并输出 FP16 或 FP32。SM75 开始支持 INT8 和 INT4 的 TensorOp 卷积，布局上除了 NHWC，还覆盖 NCxHWx 这种 interleaved 格式。SM80 后卷积能力继续扩展：SIMT 路径支持 FP32 和 complex FP32；TensorOp 路径支持 FP16、TF32、INT8、INT4 等，布局仍以 NHWC 和部分 NCxHWx 为主。

这说明 CUTLASS 的卷积能力不是孤立实现，而是与 GEMM 类型系统、Tensor Core 指令和内存布局设计强相关。对业务代码而言，如果数据已经在 NHWC 或合适的 interleaved layout 中，CUTLASS 可以提供较直接的 device-level operator；如果数据布局不同，可能需要额外变换，或者需要自己扩展 iterator/layout 组件。

## Warp-level Tensor Core Matrix Multiply

页面还总结了 warp-level Matrix Multiply with Tensor Cores。这个部分列出 TensorOp 指令形状与支持的 warp shapes。常见 TensorOp 指令形状包括 `8x8x4`、`16x8x8`、`16x8x16`、`8x8x16`、`8x8x32`、`16x8x32`、`16x8x64`、`8x8x128`、`16x8x256` 等；稀疏 TensorOp 则列出 `16x8x16`、`16x8x32`、`16x8x64`、`16x8x128` 等形状。对应 warp shapes 通常是 `32x32xK`、`32x64xK`、`64x32xK`、`64x64xK`，稀疏路径也列出类似 `64x64xK`、`64x32xK`、`32x64xK`、`32x32xK`。

这个表格的意义在于，它位于 device-level GEMM 之下。device-level kernel 由 threadblock、warp、instruction 多层 tile 组合而成；warp-level shape 决定一个 warp 内部如何组织 Tensor Core 指令。调优 CUTLASS kernel 时，用户通常不会只改一个参数，而是在 instruction shape、warp shape、threadblock shape、pipeline stage 和 memory layout 之间做组合选择。

## TensorOp 的共享内存布局约束

页面后半部分详细列出 TensorOp 指令依赖的 shared memory layout。Tensor Core 指令并不只是要求数据类型正确，还要求数据从 global memory 到 shared memory 的排列方式可以被高效加载。文档假设每个线程从 global memory 加载 128-bit 向量，然后根据操作数 A、B、C、元素类型和 GMEM layout，选择对应的 SMEM layout。

例如 FP16 Volta TensorOp `8x8x4` 会使用 `ColumnMajorVoltaTensorOpCongruous<16>`、`RowMajorVoltaTensorOpCrosswise<16>` 等布局；Ampere 及后续的 FP16/BF16 `16x8x16` 使用 `ColumnMajorTensorOpCongruous<16>`、`RowMajorTensorOpCrosswise<16>` 等布局；TF32 使用 `<32>` 粒度；FP64 使用 `<64>` 粒度；INT8、INT4、bit 类型也分别有对应的 crosswise/congruous 布局。稀疏 TensorOp 还会出现带第二模板参数的 layout，例如 `RowMajorTensorOpCrosswise<16, 64>` 或 `ColumnMajorTensorOpCrosswise<8, 128>`。

这部分对写自定义 CUTLASS kernel 很关键。很多性能问题并不是 GEMM 数学公式写错，而是内存布局和 Tensor Core 期望不匹配，导致无法走目标指令、访问不合并、bank conflict 增多，或者需要额外转换。CUTLASS 把这些 layout 类型显式暴露出来，让用户能够把 global memory layout、shared memory layout 和 MMA instruction 对齐。

## 实际使用时的理解方式

这页文档适合当作“能力索引”使用。开发者可以先确定目标 GPU 架构，比如 SM80、SM90 或 SM120；再确定算子类型，是 GEMM、conv2d implicit GEMM，还是更底层的 warp-level MMA；然后确认输入、累加、输出数据类型和矩阵/张量布局是否在表格中出现。若表格中有对应条目，通常可以进一步查看链接到的 unit test，学习具体模板参数写法。

需要注意的是，表格中的 compute capability 是能力边界，不等于性能最优建议。比如某些 SIMT kernel 可以在新 GPU 上编译运行，但新架构上通常应优先考虑 TensorOp 或更新的 CUTLASS 3.x collective API。相反，一些旧架构虽然能在 CUTLASS 2.x 表格中找到条目，但主线支持重点已经转向 SM70+，在最新 CUDA 和最新硬件上可能会遇到 deprecated 或 unsupported 的提示。

结合本机 RTX 5060 Ti / `sm_120` 的验证经验，这个页面也解释了为什么 `cutlass_profiler` 构建时会出现大量 `sm50/sm75/sm80` 命名的 generated operation：这些名字表示 CUTLASS operation catalog 的历史架构族、数据类型和指令类别，而不是说 nvcc 没有使用当前 `CUTLASS_NVCC_ARCHS=120`。也就是说，functionality 页面列出的能力矩阵，会被 profiler library 以 operation registry 的形式展开，供运行时筛选、验证和测速。

## 总结

CUTLASS C++ functionality 页面可以概括为三句话。第一，CUTLASS 的主能力是覆盖多架构、多数据类型、多布局的高性能 GEMM 和 implicit GEMM convolution。第二，CUTLASS 不只是封装一个矩阵乘 API，而是把 SIMT、WMMA、TensorOp、SpTensorOp、warp shape、instruction shape、shared memory layout 都变成可组合模板。第三，使用 CUTLASS 时应同时检查 Toolkit 版本、SM 架构、数据类型、输入输出布局和算子后端，不能只看一个维度。对工程实践来说，这页文档最适合作为选型入口：先查功能矩阵，再进入对应 unit test 或 example，最后结合 profiler 在目标机器上实测。
