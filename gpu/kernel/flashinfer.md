# FlashInfer 实际解决什么？

FlashInfer 不主要是在发明新的 Attention 数学算法，而是在解决它的高性能工程实现。

KV Cache 怎么存

KV Cache 往往被拆成很多不连续的小块，而不是一张完整、连续的矩阵。FlashInfer 使用 block-sparse 等统一格式描述这些块，让 GPU 能高效找到并读取它们。

Attention kernel 怎么写

即使公式相同，不同场景需要不同的 GPU kernel：

prefill 和 decode 不同；
单请求和批量请求不同；
MHA、GQA、MQA 不同；
不同 GPU 架构也不同。

FlashInfer通过模板和 JIT 编译，为具体场景生成优化过的 kernel。

## 简单解释

对，你理解得已经很接近了：它最终还是算 **Q、K、V 的 Attention**。FlashInfer 的优化重点不是改变公式，而是让 GPU 在各种推理场景下少搬数据、少等待、少启动 kernel。

假设现在要生成一个新 token：

* 新 token 产生一个 (Q)
* 之前 8,000 个 token 的 (K,V) 已经存在 KV Cache
* 需要计算：

[
O=\operatorname{softmax}(QK^\top)V
]

FlashInfer 主要在下面几个地方优化。

### 1. 不生成完整的 Attention 矩阵

最朴素的计算是：

1. 从显存读取全部 (K)
2. 计算 (QK^\top)
3. 把分数写回显存
4. 再读取分数
5. 做 softmax
6. 再读取 (V)
7. 计算最终结果

这里最慢的往往不是乘法，而是反复读写显存。

FlashInfer采用类似 FlashAttention 的分块方式：

* 每次只加载一小块 K、V 到更快的片上内存；
* 立即计算这块的 (QK^\top)；
* 在线更新 softmax；
* 立即乘这一块 V；
* 最后只写回结果。

因此，中间那个巨大的 (QK^\top) 分数矩阵不需要完整落到显存。

```mermaid
flowchart TD
    A["读取一块 K、V"] --> B["计算局部 QKᵀ"]
    B --> C["在线更新 Softmax"]
    C --> D["累积局部输出"]
    D --> E{"还有 KV 块？"}
    E -- 有 --> A
    E -- 无 --> F["写回最终结果"]
```

第一类收益就是：**减少显存访问和中间结果。**

### 2. 把零散的 KV Cache 统一表示

真实系统中的 KV Cache 往往不是一张连续矩阵。

例如，某个请求的 KV 可能位于：

[
[\text{物理块 }7,\ \text{物理块 }19,\ \text{物理块 }3]
]

另外一个请求可能和它共享前两个块。SGLang 的 Radix Tree、vLLM 的 paged KV cache，具体组织方式又不完全相同。

FlashInfer把这些情况统一表示成一种 **block-sparse matrix**：

* 哪些 KV 块存在；
* 它们存在哪里；
* Q 应该读取哪些块；
* 哪些块可以跳过。

这样 GPU kernel 不需要假设 KV 必须连续，也不必为了计算先把零散 KV 复制成一张连续矩阵。

第二类收益是：**避免 KV Cache 的复制、重排和冗余存储。**

### 3. 根据场景选择不同的 tile 大小

GPU 不会一次计算整个矩阵，而是把计算切成许多小块，也就是 tiles。

但不同任务适合的 tile 不一样：

* Prefill：Q 很长，适合较大的二维 tile；
* Decode：通常只有一个 Q，但 KV 很长；
* GQA：多个 Q head 共享一个 KV head；
* 短 KV 和超长 KV 需要不同划分；
* 不同型号 GPU 的共享内存和 tensor core 也不同。

固定使用一种 tile，某些场景就会浪费 GPU。

FlashInfer准备多种 microkernel 和 tile 配置，并根据具体 Attention 形状选择合适的实现。

第三类收益是：**让 kernel 与当前任务和硬件匹配。**

### 4. 把长请求拆开，做负载均衡

假设一个 batch 里有三个请求：

| 请求 |  KV 长度 |
| -- | -----: |
| A  |    100 |
| B  |  1,000 |
| C  | 10,000 |

如果每个请求交给一个 GPU thread block，那么 A 很快完成，B 随后完成，最后整个 GPU 都在等 C。

FlashInfer会把 C 的 KV 拆成多段：

[
10{,}000=2{,}500+2{,}500+2{,}500+2{,}500
]

多个 GPU thread block 同时计算这些部分，然后把部分结果合并。这类方法通常叫 **Split-K**。

注意：这里不是简单地平均 (V) 的结果，因为每一段 softmax 的归一化范围不同。每段还要保存局部最大值和归一化统计量，最后用数学上等价的方式合并。

第四类收益是：**避免一些 GPU 核心已经闲置，另一些还在处理超长请求。**

### 5. 用 JIT 生成专用 kernel

现实中的 Attention 不止标准公式，还可能有：

* causal mask；
* sliding-window mask；
* logit soft-cap；
* ALiBi 或其他位置偏置；
* 不同的数据类型；
* MHA、MQA、GQA；
* 自定义 score transformation。

如果每一步都拆成不同的 kernel：

[
QK^\top \rightarrow \text{加偏置}
\rightarrow \text{加 mask}
\rightarrow \text{softmax}
\rightarrow V
]

每一步都要启动 kernel、读写显存。

FlashInfer允许用户描述 Attention 变体，再通过 JIT 编译，把这些操作融合进一个专用 kernel。

第五类收益是：**kernel fusion，减少中间张量和 kernel 启动次数。**

### 6. 兼顾动态请求和 CUDA Graph

推理服务器里的请求每一步都在变化：

* 有请求刚加入；
* 有请求生成结束；
* KV 长度每一步加一；
* batch 组成一直变化。

但 CUDA Graph 为了降低 CPU 启动 kernel 的开销，希望计算图的形状尽量固定。

FlashInfer把两部分分开：

* 外部 workspace 和 kernel 结构保持固定，方便 CUDA Graph；
* 每一步只更新轻量的调度信息，告诉 GPU 当前有哪些任务、KV 有多长、每个 thread block 做哪一段。

第六类收益是：**既保留 CUDA Graph 的低启动开销，又能适应动态请求。**

所以你可以把 FlashInfer 的优化归纳成三个词：

[
\boxed{\text{少搬数据}+\text{充分并行}+\text{专用编译}}
]

它不是把 (QK^\top V) 换成了另一套算法，而是尽量让计算变成：

> KV 数据从显存读一次，在片上完成尽可能多的运算，同时让所有 GPU 核心都有工作可做。

而且对 **decode** 来说，Q 往往只有一个或几个 token，计算量相对不大，却要读取很长的 KV Cache。因此 decode Attention 经常是 **memory-bound（受显存带宽限制）**，不是算力不够。FlashInfer 很多设计的中心目的，正是把 GPU 的显存带宽尽可能吃满。



一句话概括：

> **FlashAttention 是高效计算 Attention 的算法和 kernel 库；FlashInfer 是面向 LLM 推理服务的整套 GPU kernel 库，其中 Attention 可以使用 FlashAttention-2/3 等多种后端。**

它们是两个独立项目，FlashInfer 不是 FlashAttention 的新版本。

## flash attention 和 flashinfer 的关系
### 定位区别

| | FlashAttention | FlashInfer |
|---|---|---|
| 核心定位 | 高效、显存友好的精确 Attention | LLM 推理服务 GPU kernel 库 |
| 主要场景 | 训练、prefill，也支持推理 | prefill、decode、continuous batching |
| Attention | 项目核心 | 只是其中一部分 |
| KV Cache | 有相关推理支持 | 核心设计，尤其是 paged/ragged KV Cache |
| 反向传播 | 重点支持 | 推理为主，通常不关注 backward |
| 其他算子 | 主要围绕 Attention | GEMM、MoE、采样、RoPE、Norm、通信等 |
| 后端 | 自己的 FA2/FA3/FA4 实现 | 可选择 FA2、FA3、cuDNN、CUTLASS、TensorRT-LLM 等 |

FlashInfer 官方也明确说它受到 FlashAttention、vLLM、CUTLASS 等项目启发。[FlashInfer](https://github.com/flashinfer-ai/flashinfer)

### FlashAttention 解决什么问题？

普通 Attention：

\[
O=\operatorname{softmax}(QK^\top)V
\]

最朴素的实现会把巨大的 \(QK^\top\) 矩阵写到显存，再读回来做 softmax 和乘 V。

FlashAttention 使用分块和 online softmax：

```text
Q tile + K/V tile
        ↓
局部 QK
        ↓
online softmax
        ↓
累加输出
```

避免完整物化 \(N\times N\) attention matrix，核心目标是减少 HBM 读写。

这在训练和长序列 prefill 中非常重要，因为此时：

```text
Q 长度 ≈ KV 长度 ≈ N
计算量 ≈ O(N²)
```

[FlashAttention 官方仓库](https://github.com/Dao-AILab/flash-attention) 提供 FA2、FA3，以及基于 CuTe DSL 的 FA4。

### FlashInfer 为什么还需要自己的 Attention？

LLM decode 的形状与训练不同。每次通常只有一个或少数几个新 query token：

```text
Q:        [batch, 1, heads, dim]
KV Cache: [batch, 很长的历史序列, heads, dim]
```

这时主要问题不再是巨大的 \(N\times N\) 中间矩阵，而是：

- 如何读取很长的 KV Cache；
- 不同请求的 KV 长度不同；
- KV Cache 分散在不同物理 page；
- continuous batching 中请求不断加入和退出；
- GQA/MQA 下 Q head 和 KV head 数量不同；
- 如何同时处理 prefill 与 decode；
- 如何配合 CUDA Graph，减少 launch overhead。

因此 FlashInfer 提供专门的：

- paged/ragged KV Cache attention
- batch decode
- batch prefill/append
- mixed prefill+decode
- MLA
- cascade/shared-prefix attention
- block-sparse attention
- speculative decoding
- FP8/FP4 KV Cache

这些更贴近 vLLM、SGLang、TensorRT-LLM 一类 serving system 的需求。[FlashInfer Attention API](https://docs.flashinfer.ai/api/attention.html)

### 两者怎么组合？

FlashInfer 可以看成调度和算子层：

```text
vLLM / SGLang / 推理服务
              ↓
          FlashInfer API
              ↓
     根据 GPU 和 workload 选择后端
       ├── FlashAttention-2
       ├── FlashAttention-3
       ├── CuTe DSL kernel
       ├── cuDNN
       ├── CUTLASS
       ├── TensorRT-LLM kernel
       └── FlashInfer 自有 decode/paged kernel
```

所以 FlashInfer 不只是包装 FlashAttention。它有自己的 KV Cache 数据结构、planning、dispatch、JIT 和大量推理专用 kernel。

一个实用判断是：

- 想学习 **online softmax、IO-aware attention、训练 Attention kernel**：先看 FlashAttention。
- 想学习 **paged KV Cache、batch decode、LLM serving kernel**：看 FlashInfer。
- 想研究 **CuTe DSL 的生产级 Attention**：看 FlashAttention-4，同时也可以看 FlashInfer 的 Blackwell CuTe DSL 后端。
