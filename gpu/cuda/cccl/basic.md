## CCCL 是做什么的

CCCL 是 CUDA Core Compute Libraries，可以理解成：CUDA C++ 世界里的“标准库 + 并行算法基础库”集合。

它把 NVIDIA 以前分开的三个核心库统一到了一个仓库里：

- Thrust：高层并行算法库，风格类似 C++ STL，比如 sort、reduce、transform，但能跑在 GPU 上。
- CUB：更底层、更贴近 CUDA kernel 的高性能 primitives，比如 block reduce、warp scan、device-wide sort/reduce 等。
- libcu++ / libcudacxx：CUDA 版本的 C++ 标准库，让很多 std 风格的东西能在 host/device code 里用，还提供 CUDA 特有的 atomic、同步、
  内存模型等能力。

它解决的问题和 CUTLASS 不一样。

CUTLASS 主要解决的是：
高性能矩阵乘法、卷积、Tensor Core GEMM、低精度 fused kernel 怎么写。

CCCL 主要解决的是：
CUDA 程序里通用的并行算法、基础数据结构、标准库能力、kernel 内部 primitives 怎么复用。
