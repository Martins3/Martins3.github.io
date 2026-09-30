# FlashAttention 项目结构与核心机制
<!-- f668be4f-b133-43ee-851e-864fc2dbf514 -->

https://github.com/Dao-AILab/flash-attention

FlashAttention 仓库是多代实现的合集。当前最值得关注的开发主体是
`flash_attn/cute/` 下的 FlashAttention-4（FA4）。这一目录大约有 5 万行
CuTeDSL/Python 代码。项目真正复杂的地方是 GPU kernel 的数据流水、架构适配和
功能组合，而不是外层 PyTorch API。

## 仓库主要组成

| 目录                                     | 内容                            | 定位                         |
| ---------------------------------------- | ------------------------------- | ---------------------------- |
| `csrc/`                                  | CUDA/C++、CUTLASS、ROCm CK 实现 | FA2 主体，偏稳定版           |
| `hopper/`                                | Hopper 专用 C++/CUTLASS kernel  | FA3                          |
| `flash_attn/cute/`                       | Python + CuTeDSL kernel         | FA4，当前活跃开发核心        |
| `flash_attn/ops/`、`layers/`、`modules/` | 高层 PyTorch 组件               | FA2 生态和模型集成           |
| `tests/cute/`                            | FA4 正确性、功能和竞争条件测试  | FA4 主要测试集               |
| `AI/`                                    | GPU kernel 调试方法和事故复盘   | 排查 hang、race、SASS 等问题 |
| `benchmarks/`                            | 性能测试                        | 吞吐量和配置比较             |
| `training/`、`examples/`                 | 模型训练和推理示例              | 上层应用                     |

FA4 是一个独立 Python 包，包名为 `flash-attn-4`。依赖 NVIDIA CUTLASS DSL、
PyTorch、TVM FFI 和 quack-kernels，包定义位于 `flash_attn/cute/pyproject.toml`。

## FlashAttention 解决的核心问题

普通 Attention 可以概括为：

```text
S = Q x K^T
P = softmax(S)
O = P x V
```

其中 `S` 和 `P` 都是 `seqlen x seqlen`。如果将完整矩阵写入显存，会产生很大
的中间张量和显存读写开销。

FlashAttention 的核心做法是：

```text
取一个 Q tile
    |
逐块读取 K/V tile
    |
局部计算 QK^T
    |
立即执行 mask 和 online softmax
    |
立即累积 softmax(QK^T) x V
    |
处理完全部 K/V 后写出 O 和 LSE
```

它不将完整注意力矩阵写到全局显存。FlashAttention 仍然是精确 Attention，计算
复杂度没有从平方级变成线性；它主要优化的是显存 IO、中间结果大小和 GPU 并行
效率。

最能体现算法本质的代码包括：

- `flash_attn/cute/flash_fwd.py` 中计算单个 N block 的前向逻辑：执行 QK GEMM、
  mask、online softmax 和 PV GEMM。
- `flash_attn/cute/softmax.py` 中 `Softmax.online_softmax()`：维护每行的
  `row_max` 和 `row_sum`。
- `flash_attn/cute/softmax.py` 中
  `Softmax.rescale_O()`：遇到更大的局部最大值时， 重新缩放之前累计的 O。

Online softmax 是按块计算仍能保持数值稳定的数学核心。

## FA4 调用路径

FA4 从 `flash_attn/cute/__init__.py` 导出两个主要入口：

- `flash_attn_func()`：普通等长或批量 Attention。
- `flash_attn_varlen_func()`：变长序列 Attention。

调用链大致如下：

```text
flash_attn_func / flash_attn_varlen_func
                 |
       PyTorch autograd.Function
                 |
   参数校验、形状处理、功能规范化
                 |
  选择 tile、SplitKV、GQA packing、scheduler
                 |
       按 GPU 架构选择 kernel
                 |
     cute.compile -> PTX/CUBIN -> launch
                 |
      backward 时选择对应反向 kernel
```

`flash_attn/cute/interface.py` 中 `_flash_attn_fwd()` 和 `_flash_attn_bwd()`
是总调度 中心，负责：

- 检查 Q/K/V 的 dtype、shape、stride 和对齐。
- 判断 causal、local/sliding-window 和 varlen。
- 决定 GQA 是否启用 `pack_gqa`。
- 选择 tile 大小和流水级数。
- 决定是否使用 SplitKV。
- 分配输出、LSE 和 partial buffer。
- 生成 kernel 编译 key。
- 按架构创建 kernel 对象并调用 `cute.compile()`。
- 对接 PyTorch autograd。

所以 `interface.py` 有 4000 多行，但其中大部分是功能组合和编译调度，真正的 GPU
计算位于架构专用 kernel 文件。

## 架构专用 kernel

当前代码实际支持的范围比“主要面向 Hopper/Blackwell”的介绍更广：

| 架构      | 前向                 | 反向                 | 特点                                               |
| --------- | -------------------- | -------------------- | -------------------------------------------------- |
| SM80/8.x  | `flash_fwd.py`       | `flash_bwd.py`       | Ampere 风格基础实现，cp.async/MMA                  |
| SM90/9.x  | `flash_fwd_sm90.py`  | `flash_bwd_sm90.py`  | Hopper，TMA、WGMMA、warp-group 流水                |
| SM100/110 | `flash_fwd_sm100.py` | `flash_bwd_sm100.py` | Blackwell，UMMA、TMEM、2CTA、persistent scheduler  |
| SM120     | `flash_fwd_sm120.py` | `flash_bwd_sm120.py` | Blackwell 消费级路径，复用 SM80 风格 MMA，功能较少 |

架构选择位于 `flash_attn/cute/interface.py` 的 `_flash_attn_fwd()` 和
`_flash_attn_bwd()`。

其中最复杂的是 Blackwell：

- `flash_fwd_sm100.py` 中的 `FlashAttentionForwardSm100`。
- `flash_bwd_sm100.py` 中的 `FlashAttentionBackwardSm100`。
- `head_dim=256` 还有单独的 2CTA 前后向实现。

SM100 前向会进一步划分线程职责：

- load warp：使用 TMA 搬运 Q/K/V。
- MMA warp：执行 QK 和 PV 矩阵乘。
- softmax warp：执行 mask、score modifier 和 softmax。
- correction warp：修正或重新缩放累计输出。
- scheduler warp：取得下一个工作 tile。

这些 warp 通过 mbarrier 和多级 pipeline 异步协作。这一部分是 FA4 最核心、也最
容易出现 deadlock 或 race 的地方。

## 重要公共抽象

### 数学和语义层

- `softmax.py`：online softmax 和 score modifier。
- `mask.py`：causal、local、自定义 `mask_mod`、SM100 fused mask。
- `block_info.py` 中 `BlockInfo`：计算一个 Q tile 实际需要访问哪些 K/V tile。
- `seqlen_info.py`：维护定长和 varlen 的长度与偏移。

一个重要优化是：不仅在 score 计算完成后 mask，还提前通过 `BlockInfo` 排除整块
不可能参与计算的 K/V tile。

### 数据流水和硬件层

- `pipeline.py`：producer/consumer pipeline、stage index、phase 和 mbarrier。
- `tile_scheduler.py`：静态、动态、persistent、CLC 和 varlen scheduler。
- `hopper_helpers.py`：SM90 WGMMA/TMA 辅助函数。
- `blackwell_helpers.py`：SM100 UMMA/TMEM/2CTA 辅助函数。
- `copy_utils.py`：global/shared/register 之间的数据搬运和布局转换。

GPU kernel 的性能很大程度上取决于这一层：什么时候 load、什么时候计算、使用几级
pipeline、线程之间如何同步，以及数据放在寄存器还是 shared memory。

### 高级功能

- `pack_gqa.py`：把多个 Q head 打包到同一个 KV head 的工作 tile 中。
- `paged_kv.py`：分页 KV cache。
- `flash_fwd_combine.py`：合并 SplitKV 的 partial output 和 LSE。
- `block_sparsity.py`：块稀疏 Attention。
- `flash_fwd_mla_sm100.py`、`flash_bwd_mla_*`：DeepSeek MLA/稀疏 MLA 路径。
- `prepare_scheduler.py`：提前生成动态调度元数据。

这些功能并非在每种 GPU 架构上都完整支持，目前 SM100/110 的功能最全。

## CuTeDSL 的角色

虽然 FA4 kernel 文件是 `.py`，但它们不是普通 Python 数值程序。代码中的：

- `@cute.jit`
- `cutlass.Constexpr`
- `cute.Tensor`
- `cute.make_layout()`
- `cute.compile()`

描述的是 GPU tensor layout、线程映射、MMA、TMA 和同步关系。CuTeDSL 将这些 Python
DSL 编译为 MLIR、PTX 和 CUBIN，再从 PyTorch 调用。因此可以将其理解为：

```text
Python API
  + 模板元编程
  + CUDA kernel DSL
  + JIT 编译系统
```

大量 `Constexpr` 参数意味着 causal、head dimension、tile size、mask 和 score
modifier 等特征会被编译成不同的 kernel specialization。这也是第一次运行较慢，
以及测试需要先做并行编译 pass 的原因。

`cache_utils.py` 中 `JITCache` 和 `JITPersistentCache`
实现了内存缓存和磁盘缓存。 源码或 CUTLASS/TVM ABI 变化时，通过 source
fingerprint 自动使旧缓存失效。

## 前向和反向的复杂度

概念上前向比较直接：

```text
S = QK^T
P = softmax(S)
O = PV
```

反向需要计算：

```text
dV = P^T dO
dP = dO V^T
dS = softmax_backward(dP)
dQ = dS K
dK = dS^T Q
```

同时还必须：

- 避免保存完整 P。
- 利用前向保存的 LSE 重建概率。
- 处理 dQ 的跨 tile 累积。
- 处理 GQA/MQA 中多个 Q head 对同一个 KV head 的归约。
- 支持 deterministic、varlen、mask、score modifier 和 block sparse。

因此反向 kernel 通常比前向更加复杂。`flash_bwd_sm100.py` 本身就有约 4100 行。

## 理解代码的推荐顺序

如果目标是理解算法，建议依次阅读：

1. `flash_fwd.py` 中单个 N block 的计算流程。
2. `softmax.py` 中 `Softmax.online_softmax()`。
3. `block_info.py` 中 `BlockInfo`。
4. `mask.py` 中 `AttentionMask`。

如果目标是理解 FA4 的硬件性能技术，再依次阅读：

1. `flash_fwd_sm90.py` 中 `FlashAttentionForwardSm90`。
2. `flash_fwd_sm100.py` 中 `FlashAttentionForwardSm100`。
3. `pipeline.py` 中各类 producer/consumer pipeline。
4. `tile_scheduler.py` 中各类 tile scheduler。

整体上，这是一个以“减少 Attention 显存 IO”为算法基础，以“架构专用异步流水
线”为性能核心，以“CuTeDSL JIT specialization”为工程载体的高性能 GPU kernel
项目。

# Candle FlashAttention v3 的代码为什么少很多
<!-- dd9cc183-10dc-4b5f-bf74-90357f3635b3 -->

`candle-flash-attn-v3/hkernel/` 大约有 9300 行，而 FA4 的 `flash_attn/cute/`
大约有 5.1 万行。主要原因不是前者用更少的代码完成了相同功能，而是两者的工程
范围不同。

`candle-flash-attn-v3/hkernel` 更接近“从 FA3 裁剪出来、专供 Candle 推理使用的
SM90 前向 kernel”；`flash_attn/cute` 则是多架构、完整前后向并支持大量扩展功能 的
FA4 kernel 平台。

## 功能范围对比

| 维度                   | Candle `hkernel`     | FA4 `flash_attn/cute`                   |
| ---------------------- | -------------------- | --------------------------------------- |
| 代码量                 | 约 9.3k 行           | 约 51k 行                               |
| 代际                   | FA3 的下游移植       | FA4 主项目                              |
| GPU                    | 只面向 SM90a/Hopper  | SM80、SM90、SM100/110、SM120            |
| 前向                   | 支持                 | 支持                                    |
| 反向                   | 实际构建中关闭       | 完整支持                                |
| dtype                  | 实际启用 FP16/BF16   | FP16/BF16/FP8 及架构专用路径            |
| head dimension         | 固定 64/128/256/512  | 更灵活，还有 MLA 和不同 QK/V 维度       |
| 自定义 mask/score      | 不支持               | 支持 `mask_mod`、`score_mod`            |
| Block sparse           | 不支持               | 支持                                    |
| MLA                    | 不支持               | 支持                                    |
| Learnable sink/softcap | 不支持               | 支持                                    |
| 编译方式               | NVCC AOT 模板实例化  | CuTeDSL JIT specialization              |
| 框架适配               | Candle 前向 CustomOp | PyTorch autograd、FakeTensor 和编译缓存 |

## Candle 版本没有真正启用 backward

`candle-flash-attn-v3/build.rs` 的 `KERNEL_FILES` 实际编译的都是
`flash_fwd_*.cu`。反向文件被明确注释：

```rust
// "flash_bwd_hdim64_fp16_sm90.cu",
// "flash_bwd_hdim96_fp16_sm90.cu",
// "flash_bwd_hdim128_fp16_sm90.cu",
```

Candle 暴露的是 `src/lib.rs` 中 `FlashAttn` 和 `FlashAttnVarLen` 的
`CustomOp3::cuda_fwd()`，没有相应的反向 CustomOp。

相比之下，FA4 仅反向部分就包括：

- `flash_bwd.py`：约 1300 行。
- `flash_bwd_sm90.py`：约 2000 行。
- `flash_bwd_sm100.py`：约 4200 行。
- SM100 head-dim 256 两个专用反向 kernel：约 5700 行。
- MLA backward：约 4600 行。
- backward preprocess/postprocess：约 1100 行。

仅 FA4 的反向及其辅助代码就已经远超 Candle 整个 `hkernel/`。

## Candle 只维护一套硬件数据流

Candle 版本只支持 Hopper `sm90a`，所以只需维护一套：

```text
SM90 TMA load
    |
WGMMA QK
    |
online softmax
    |
WGMMA PV
    |
SM90 TMA epilogue
```

主要实现在：

- `hkernel/mainloop_fwd_sm90_tma_gmma_ws.hpp` 中 `CollectiveMainloopFwd`。
- `hkernel/flash_fwd_kernel.h` 中前向 kernel。
- `hkernel/kernel_traits.h` 中 kernel traits。
- `hkernel/softmax.h` 中 `Softmax`。

FA4 则同时维护 SM80、SM90、SM100/110 和 SM120。尤其 SM100 的线程分工、
TMEM、UMMA、scheduler 和 2CTA 协同并不能通过修改几个 SM90 模板参数得到，需要
重新设计。

## C++ 模板将大量变体压在同一份源码中

Candle 目录中存在大量如下文件：

```text
flash_fwd_hdim64_fp16_sm90.cu
flash_fwd_hdim128_bf16_sm90.cu
flash_fwd_hdim256_fp16_gqa8_sm90.cu
```

但每个文件通常只有 9 行。例如一个 GQA 实例化本质上只是：

```cpp
template<>
void run_mha_fwd_gqa_<cutlass::bfloat16_t, 256, 32>(...) {
    run_mha_fwd_hdim256_gqa<cutlass::bfloat16_t, 32>(...);
}
```

实际 kernel 只有一份模板实现，NVCC 在编译阶段展开：

```text
dtype x head_dim x GQA ratio x causal/local x split
```

因此源代码行数少不代表最终生成的 CUBIN 简单。许多复杂度存在于 C++ 模板实例化
和编译产物中。

FA4 也会做 specialization，但由于采用 CuTeDSL JIT，还需要在自己的代码中实现：

- 编译 key。
- FakeTensor 构造。
- 运行时架构选择。
- 磁盘缓存。
- 源码 fingerprint。
- modifier hash。
- 动态 scheduler metadata。

这些都会直接计入仓库源码。

## 很多底层复杂度由 CUTLASS 提供

Candle 的 `build.rs` 使用 `KernelBuilder.with_cutlass()` 自动下载固定 commit 的
CUTLASS。`hkernel` 大量包含：

```cpp
#include <cute/tensor.hpp>
#include <cutlass/pipeline/pipeline.hpp>
#include <cutlass/gemm/collective/collective_builder.hpp>
#include <cutlass/cluster_launch.hpp>
```

所以布局系统、TMA atom、WGMMA atom、pipeline 基础设施和 cluster launch 等大量
代码位于外部 CUTLASS，不能只统计 `hkernel/`。

FA4 同样依赖 CUTLASS DSL，但为了支持新架构和运行时 JIT，又增加了
`pipeline.py`、`tile_scheduler.py`、`blackwell_helpers.py` 等自己的基础设施。

## Candle 的功能集合比较克制

Candle 当前公开 API 主要包括：

- 普通 fixed-length attention。
- varlen attention。
- causal/local window。
- ALiBi。
- MQA/GQA packing。
- FP16/BF16。
- 固定 head dimension 64/128/256/512。

FA4 还要组合处理：

- `score_mod` 和 `score_mod_bwd`。
- `mask_mod`。
- auxiliary tensors/scalars。
- block sparsity。
- paged KV。
- top-k gather KV。
- SplitKV 和动态 per-batch split。
- persistent/CLC scheduler。
- learnable sink。
- softcap。
- FP8 descale。
- Q/K 与 V 不同的 head dimension。
- DeepSeek MLA。
- 专用 head-dim 256 2CTA kernel。
- deterministic backward。
- SM100 block-sparse backward。

这些功能之间还存在组合复杂度：

```text
varlen
x GQA packing
x causal/local
x SplitKV
x paged KV
x architecture
x dtype
x head dimension
```

这也是 FA4 的 `interface.py` 单独就有 4000 多行的主要原因。

## Candle 目录中还有未参与当前构建的代码

- `hkernel/flash_api.cpp` 约有 1745 行，但 `build.rs` 实际编译的是
  `flash_api.cu`。
- FP8 `.cu` 实例化文件仍在目录中，但在 `build.rs` 中被注释。
- backward 接口的残留代码仍然存在，但当前构建不支持 backward。

因此 9300 行甚至高估了当前 Candle crate 真正启用的能力。

## 对比结论

两者的核心 FlashAttention 算法相同：

```text
QK GEMM -> mask -> online softmax -> PV GEMM
```

Candle 版本选择了较窄的工程边界：

```text
FA3
+ 只做 SM90a
+ 只做前向
+ 固定 dtype/head dimension/GQA 组合
+ CUTLASS C++ 模板 AOT 实例化
+ Candle 薄封装
```

FA4 则是：

```text
多架构 kernel 平台
+ 完整前后向
+ 大量新 Attention 变体
+ 动态调度
+ CuTeDSL JIT 编译和缓存系统
```

所以不能仅从代码量得出 Candle 实现更简洁或 FA4 更臃肿。更准确的说法是： Candle
是经过裁剪的专用 FA3 部署实现，FA4 是持续演进的通用 kernel 研发平台。
