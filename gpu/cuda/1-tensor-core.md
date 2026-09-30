# cuda: tensor core

<!-- 4adcec8f-2d5e-40ba-bda6-5e0bfebf9fcd -->

测试代码：`gpu/cuda/tutorial/basic/tensor_core.cu`

这份教学实现仍不是硬件峰值实现。cuBLAS/CUTLASS
会使用更复杂的流水线、异步拷贝、双缓冲和架构专用指令，性能通常还会明显更高。代码已通过
clang-format、编译和运行验证，未执行 git stage 或 commit。

## 测试结果

| 实现             |        耗时 |        吞吐量 | 相对朴素实现的加速比 |
| ---------------- | ----------: | ------------: | -------------------: |
| 朴素 CUDA Core   | 约 88.60 ms |  1.55 TFLOP/s |                1.00× |
| 优化 CUDA Core   | 约 28.88 ms |  4.76 TFLOP/s |                3.07× |
| Tensor Core WMMA | 约 10.03 ms | 13.70 TFLOP/s |                8.83× |

Tensor Core 相比已经做过 shared-memory 和寄存器分块的 CUDA Core，仍快约 2.88×。

反汇编也确认了执行路径：

- CUDA Core kernel 使用 FFMA 指令。
- Tensor Core kernel 使用 HMMA.16816.F32 指令。

Tensor Core 和 CUDA Core 的区别可以概括为：

| 对比维度  | CUDA Core                | Tensor Core                         |
| --------- | ------------------------ | ----------------------------------- |
| 硬件定位  | 通用浮点/整数计算单元    | 专用矩阵乘加单元                    |
| 执行方式  | 每个线程执行普通算术指令 | 一个 warp 协作执行矩阵 tile         |
| GEMM 实现 | 拆成大量 FFMA            | 使用 HMMA 完成矩阵乘加              |
| 灵活性    | 运算和控制流较灵活       | 对数据类型、形状、布局和对齐有限制  |
| 典型用途  | 一般 CUDA kernel         | GEMM、神经网络和张量计算            |
| 常见精度  | FP32 等通用精度          | FP16/BF16/TF32 输入，常以 FP32 累加 |

最核心的区别不是“Tensor Core 的时钟更快”，而是执行粒度不同： CUDA Core
一次处理普通线程级算术；Tensor Core
一次处理整个矩阵小块，因此矩阵乘法吞吐量高得多。

## FFMA 和 HMMA
<!-- be88d8b7-4056-4c56-8c85-d245756afd32 -->

FFMA 是 Floating-point Fused Multiply-Add，即浮点融合乘加：

d = a × b + c

乘法和加法融合执行，中间结果不单独舍入，最终只舍入一次。NVIDIA PTX ISA
(https://docs.nvidia.com/cuda/parallel-thread-execution/index.html?highlight=reduction) 也据此定义 fma。

在 gpu/cuda/tutorial/basic/tensor_core.cu 的 matmul_cuda_core_naive() 和 matmul_cuda_core_tiled() 中：

sum = fmaf(a, b, sum);

编译后最终对应 SASS 层面的 FFMA。它是线程级标量运算：warp 中每个有效线程分别计算自己的一个 a × b + c。

HMMA 可理解为 Half-precision Matrix Multiply-Accumulate，是 Tensor Core 的矩阵乘加指令：

D = A × B + C

它不是单线程标量指令，而是整个 warp 共同操作分散在各线程寄存器中的矩阵 fragment。

本例的 HMMA.16816.F32 可拆解为：

- 16816：矩阵形状 m16n8k16，即 16×16 矩阵块乘以 16×8 矩阵块。
- F32：结合本例表示使用 FP32 累加/输出。
- 输入来自 FP16 的 A、B fragment。

NVIDIA PTX 中对应的形式是 mma.sync.aligned.m16n8k16...f32.f16.f16.f32。
PTX ISA 的 MMA 定义 (https://docs.nvidia.com/cuda/pdf/ptx_isa_8.5.pdf)

在 matmul_tensor_core_wmma() 中，源码调用的是 mma_sync()。一个 WMMA 16×16×16 运算通常会被编译器拆成多条 HMMA.16816，因为单条指令
只覆盖 N=8，不是“一条 HMMA 覆盖整个 16×16 tile”。

最直观的区别是：

- FFMA：一个线程计算一个标量乘加
- HMMA：一个 warp 协作计算一个矩阵子块乘加

所以 Tensor Core 的优势主要来自一次指令处理了大量矩阵乘加，而不是单纯因为执行频率更高。
