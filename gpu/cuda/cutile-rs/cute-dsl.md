# cute dsl

先把名字理清：**CuTe DSL** 和刚才说的 **cuTile** 不是同一个项目。

- **CuTe DSL**：CUTLASS 4 引入的 Python DSL，提供线程、warp、Tensor Core、TMA、shared memory layout 等底层控制。
- **cuTile**：更偏 tile-level 的抽象，生成 CUDA Tile IR。
- CuTe DSL 更接近“用 Python 写 CuTe/CUTLASS kernel”，控制粒度通常更低，也更复杂。

CuTe DSL 生态可以分成下面几类。

## 1. 核心项目：NVIDIA CUTLASS

[NVIDIA/cutlass](https://github.com/NVIDIA/cutlass) 是最核心的仓库，同时包含：

```text
CUTLASS
├── CuTe C++：C++ 模板库
├── CuTe DSL：Python DSL
├── CUTLASS C++ kernels
├── CUTLASS Python DSLs
└── examples / profiler / tests
```

CuTe DSL 的基本代码长这样：

```python
import cutlass
import cutlass.cute as cute

@cute.kernel
def kernel(src: cute.Tensor, dst: cute.Tensor):
    tidx, _, _ = cute.arch.thread_idx()
    # 显式进行线程划分、tile copy、MMA 等操作
```

安装包通常是：

```bash
pip install nvidia-cutlass-dsl
```

官方资源：

- [CuTe DSL 文档](https://docs.nvidia.com/cutlass/latest/media/docs/pythonDSL/cute_dsl.html)
- [CUTLASS 仓库](https://github.com/NVIDIA/cutlass)
- [CuTe DSL 示例目录](https://github.com/NVIDIA/cutlass/tree/main/examples/python/CuTeDSL)
- [入门 notebooks](https://github.com/NVIDIA/cutlass/tree/main/examples/python/CuTeDSL/notebooks)

官方示例覆盖：

- Ampere、Ada、Hopper、Blackwell
- GEMM、Grouped GEMM、Block-scaled GEMM
- FlashAttention
- Mamba2 SSD
- TMA、WGMMA、TMEM、异步流水线
- PyTorch、JAX、DLPack、TVM FFI
- JIT 和 AOT 编译

它是学习 API 最权威的地方，但高性能 GEMM 示例比较长，不适合作为第一个 kernel。

## 2. QuACK：最推荐阅读的实际 kernel 库

[Dao-AILab/quack](https://github.com/Dao-AILab/quack) 全称是 “A Quirky Assortment of CuTe Kernels”，由 FlashAttention 团队维护。

包含：

- RMSNorm 前向、反向
- LayerNorm 前向、反向
- Softmax 前向、反向
- Cross Entropy 前向、反向
- Hopper GEMM
- Blackwell GEMM
- fused epilogue
- 部分 JAX 接口

它很适合学习，因为既有相对简单的 memory-bound kernel，也有 Tensor Core GEMM：

```text
RMSNorm / Softmax
       ↓
向量化 load/store、reduction
       ↓
GEMM
       ↓
TMA、MMA、流水线和 epilogue
```

如果你刚开始看 CuTe DSL，我会优先推荐 QuACK，而不是直接啃官方 Blackwell persistent GEMM。

## 3. FlashAttention-4

[FlashAttention-4](https://github.com/Dao-AILab/flash-attention/tree/main/flash_attn/cute) 是很有代表性的生产级 CuTe DSL 项目。

FA4 针对 Hopper 和 Blackwell，核心 kernel 使用 Python CuTe DSL 编写：

```python
from flash_attn.cute import flash_attn_func

out = flash_attn_func(q, k, v, causal=True)
```

这里可以学到：

- Q/K/V 的 tile 划分
- online softmax
- TMA 异步搬运
- Tensor Core MMA
- producer/consumer warp specialization
- variable-length attention
- causal、sliding-window 等 mask
- split-KV 和 paged KV cache

不过它属于高级材料。要是还不熟悉 CuTe layout algebra，直接看 FA4 会比较痛苦。

## 4. PyTorch Inductor 的 CuTe DSL 后端

PyTorch 已经在 TorchInductor 中加入 CuTe DSL 的模板和调度基础设施：

- [`torch/_inductor/codegen/cutedsl`](https://github.com/pytorch/pytorch/tree/main/torch/_inductor/codegen/cutedsl)
- [CuTe DSL Template System](https://github.com/pytorch/pytorch/blob/main/torch/_inductor/codegen/cutedsl/README.md)

它的目标是让 `torch.compile` 在 autotune 时选择 CuTe DSL kernel：

```text
PyTorch Graph
     ↓
TorchInductor
     ↓
ATen / Triton / CUTLASS / CuTe DSL 等候选
     ↓
autotune 选择
```

当前配置中，`CUTEDSL` 已经是 GEMM autotune backend 之一，主要面向 NVIDIA Blackwell；相关实现仍在快速演进。

这个项目值得看，但适合想研究以下问题的人：

- CuTe DSL 如何接入编译器框架
- kernel 模板生成
- autotune
- kernel 缓存
- PyTorch tensor 和 DSL kernel 的桥接

## 5. Liger-Kernel

[LinkedIn/Liger-Kernel](https://github.com/linkedin/Liger-Kernel) 原本主要是面向 LLM 训练的 Triton kernel 库，现在增加了实验性 CuTe DSL 后端。

目前相关算子包括：

- RMSNorm
- Cross Entropy
- fused linear cross entropy
- SwiGLU
- 部分 MoE kernel

使用方式类似：

```bash
pip install "liger-kernel[cutedsl]"
LIGER_KERNEL_IMPL=cutedsl python train.py
```

它适合观察一个成熟 Triton 项目如何逐步增加 CuTe DSL 实现，以及同一个算子在 Triton、cuTile、CuTe DSL 下的设计区别。

## 6. Meta Generalized Dot Product Attention

[facebookresearch/ads_model_kernel_library](https://github.com/facebookresearch/ads_model_kernel_library) 中的 GDPA 项目包含针对 Blackwell 的 CuTe DSL attention kernel。

它面向更加复杂的实际训练场景：

- MHA、GQA、MQA
- causal、non-causal、sliding window
- variable length/jagged tensor
- 自定义激活函数
- BF16、FP16、FP8
- 复杂 tile scheduling

它比标准 FlashAttention 更关注工业推荐/广告模型中形状不规则、序列不规则的问题。

## 7. TensorRT 集成示例

TensorRT 提供了一个用 CuTe DSL 实现 RMSNorm PluginV3 的示例：

[TensorRT CuTe DSL RMSNorm Plugin](https://github.com/NVIDIA/TensorRT/tree/main/samples/python/cute_dsl_plugin)

这个示例展示的是完整接入路径：

```text
CuTe DSL kernel
      ↓
CuPy / DLPack 零拷贝
      ↓
TensorRT IPluginV3
      ↓
TensorRT engine
```

如果你关注 vLLM、TensorRT-LLM 或推理引擎中的自定义算子，这个项目很值得看。NVIDIA 的 [TensorRT Edge-LLM](https://github.com/NVIDIA/TensorRT-Edge-LLM) 也包含 CuTe DSL kernel 的预编译和部署机制。

## 8. 社区学习项目

[learn-cutedsl](https://github.com/luongthecong123/learn-cutedsl) 是偏教学和实验的项目，优点是把概念拆得比较细，并提供对应 CUDA 实现进行比较。

覆盖：

- SM80、SM90、SM100、SM120
- 普通 CUDA load/store
- shared memory
- Tensor Core
- TMA
- WGMMA、UMMA、`tcgen05`
- attention 和 fused kernel

它不是官方项目，但比生产级源码更容易顺着学习。

## 我建议的阅读顺序

如果目标是学习写 kernel：

1. CUTLASS 官方 CuTe DSL notebooks：理解 `Shape`、`Stride`、`Layout`、`Tensor`。
2. 官方 vector add/copy/reduction 示例：理解 thread-to-data mapping。
3. QuACK 的 RMSNorm、Softmax：学习向量化和 reduction。
4. 官方 Ampere GEMM：理解 `TiledMma` 和 `TiledCopy`。
5. Hopper GEMM：加入 TMA、WGMMA 和 pipeline。
6. QuACK GEMM：看较完整的工程实现。
7. FlashAttention-4：学习 attention 的生产级调度。
8. PyTorch Inductor或 TensorRT 示例：学习框架集成。

最值得先看的三个仓库是：

- [NVIDIA/cutlass](https://github.com/NVIDIA/cutlass)：官方实现和文档
- [Dao-AILab/quack](https://github.com/Dao-AILab/quack)：适合阅读的高性能算子
- [Dao-AILab/flash-attention](https://github.com/Dao-AILab/flash-attention/tree/main/flash_attn/cute)：生产级 attention

另外，[NVIDIA/TileGym](https://github.com/NVIDIA/TileGym) 虽然也是 tile-based kernel 教程，但它主要讲的是 **cuTile/CUDA Tile IR**，不是 CuTe DSL，不要把这两个学习路径混在一起。
