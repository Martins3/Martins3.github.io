## cutlass 是做什么的
CUTLASS 解决的核心问题是：怎么让普通开发者以可组合、可定制的方式，写出接近 cuBLAS/cuDNN 级别性能的 CUDA 矩阵计算 kernel。

更具体地说，它解决这几个痛点：

1. 手写高性能 GEMM : General Matrix Multiplication 太难

   GEMM，也就是矩阵乘法，是深度学习、科学计算、图形、推理里最核心的算子。
   但真正跑满 GPU 需要处理很多底层细节：tile 分解、shared memory 布局、warp/thread 映射、
   Tensor Core 指令、pipeline、寄存器复用、访存合并、epilogue 融合等。CUTLASS 把这些套路模板化、模块化。

2. cuBLAS 很快，但不够灵活

   如果只是 C = A * B，cuBLAS 很好。但实际模型里经常要：
    - GEMM 后接 bias、activation、scale、quant/dequant
    - FP8/FP4/int8/mixed precision
    - grouped GEMM、batched GEMM、特殊 layout
    - attention、convolution 转 GEMM
    - 自定义 epilogue fusion
   cuBLAS 不一定暴露你想要的融合和数据布局。CUTLASS 让你能自己拼装这些 kernel，同时尽量保留高性能。

3. 避免每代 GPU 都从头写 kernel
   Volta、Ampere、Hopper、Blackwell 的 Tensor Core、TMA、WGMMA、MMA 指令和内存层级都不一样。
   CUTLASS 把架构相关能力封装成 reusable building blocks，让同一类算法可以针对不同 GPU 架构实例化和调优。

4. 把“高性能矩阵计算”拆成可复用组件
   它不是单纯给几个 GEMM 函数，而是提供一套层次化抽象：
    - thread / warp / CTA / cluster 级别的 tile 组织
    - layout 和 tensor 表达
    - MMA atom / copy atom
    - mainloop pipeline
    - epilogue
    - profiler 和 kernel library
   所以 CUTLASS 更像是 CUDA 线性代数 kernel 的模板积木系统。

## 基本疑问和好奇的东西
首先，没有什么特别难的东西，就是为了做矩阵乘法:
https://docs.nvidia.com/cutlass/latest/media/docs/cpp/quickstart.html

```txt
$ ./tools/profiler/cutlass_profiler --kernels=sgemm --m=4352 --n=4096 --k=4096
```

这里的确有一些 nb 的概念:
	- https://docs.nvidia.com/cutlass/latest/media/docs/cpp/programming_guidelines.html

- 为什么叫作 wrap level 和 thread level ?

可以看看:
https://docs.nvidia.com/cutlass/latest/media/docs/cpp/functionality.html
https://docs.nvidia.com/cutlass/latest/media/docs/cpp/terminology.html

https://docs.nvidia.com/cutlass/latest/media/docs/cpp/efficient_gemm.html
https://docs.nvidia.com/cutlass/latest/media/docs/cpp/pipeline.html
https://docs.nvidia.com/cutlass/latest/media/docs/cpp/dependent_kernel_launch.html : 比较容易
https://docs.nvidia.com/cutlass/latest/media/docs/cpp/grouped_scheduler.html
