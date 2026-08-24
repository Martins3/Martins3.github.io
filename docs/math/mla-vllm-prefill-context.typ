#set heading(numbering: "1.")
#set text(font: ("PingFang SC", "Noto Sans CJK SC"), size: 11pt)
#set math.equation(numbering: "(1)")

#let softmax = math.op("softmax")
#let concat = math.op("Concat")
#let dtype = math.op("dtype")
#let cast = math.op("cast")

#align(center)[#text(size: 20pt, weight: "bold")[从 MLA 理论理解 vLLM 的 prefill context 实现]]

#outline(title: none, depth: 2)

#pagebreak()

= 问题背景

这篇笔记解释 vLLM 中 `vllm/model_executor/layers/attention/mla_attention.py`
里的 `_compute_prefill_context`，尤其是下面这段 dtype 转换为什么看起来和 MLA
理论公式不一致：

```python
if (
    use_fp8_prefill or _kv_b_proj_w_dtype != current_platform.fp8_dtype()
) and _kv_b_proj_w_dtype != torch.uint8:
    kv_c_normed = kv_c_normed.to(_kv_b_proj_w_dtype)
```

结论先说：

这段代码没有改变 MLA 的数学结构。它仍然在做
$K^c,V = c W_"up"$。`to(dtype)` 只是为了让这个解压投影在 BF16、FP8、
NVFP4、AWQ/GPTQ 等不同低精度实现下用正确的 kernel 执行。

= MLA 理论中的关键对象

MLA 的核心是：KV Cache 不再保存每个 head 的完整 Key 和 Value，而是保存一个
低维 latent 向量 $c$，以及一小段 RoPE 专用的位置 Key。

对当前层输入 $X in RR^(n times d_"model")$，理论上可以写成：

$ c = X W_"down" $

其中 $c in RR^(n times d_c)$。这个 $c$ 是需要缓存的压缩 KV latent。

计算 attention 时，从 $c$ 中解压出每个 head 的内容 Key 和 Value：

$ K_i^c = c W_i^("upK") $

$ V_i = c W_i^("upV") $

Decoupled RoPE 又额外保存一段位置 Key：

$ K_i = concat(K_i^c, K_i^r) $

$ Q_i = concat(Q_i^c, Q_i^r) $

最后 attention 仍然是标准 softmax attention：

$ "head"_i = softmax(frac(Q_i K_i^T, sqrt(d))) V_i $

所以 MLA 没有改变 attention 公式本身。它改变的是 KV Cache 中保存什么：

```text
普通 MHA cache: K, V
MLA cache:      c, K^r
```

= vLLM 代码里的对象映射

`_compute_prefill_context` 处理的是 chunked prefill 中的历史 context。它从 KV cache
里按 chunk 取出历史 token 的 MLA cache，然后临时解压成 attention kernel 需要的
Key 和 Value。

代码片段：

```python
kv_c_normed = workspace[:toks][..., : self.kv_lora_rank]
k_pe = workspace[:toks][..., self.kv_lora_rank :].unsqueeze(1)

kv_nope = self.kv_b_proj(kv_c_normed)[0].view(
    -1, self.num_heads, self.qk_nope_head_dim + self.v_head_dim
)

k_nope, v = kv_nope.split([self.qk_nope_head_dim, self.v_head_dim], dim=-1)
k = self._concat_k_nope_k_pe(k_nope, k_pe)
```

对应关系如下：

```text
理论符号        vLLM 变量
--------------------------------------------
c               kv_c_normed
K^r             k_pe
[W^upK; W^upV]  kv_b_proj
K^c             k_nope
V               v
Concat(K^c,K^r) k
```

因此：

```python
kv_nope = self.kv_b_proj(kv_c_normed)
```

就是理论里的：

$ [K^c, V] = c [W^("upK"), W^("upV")] $

注意这里 `kv_b_proj` 一次性算出 `k_nope` 和 `v`，只是把两个 up-projection 合并成
一个更大的线性层。它不代表理论上多了一个新对象。

= `kv_b_proj` 里的 b 是什么意思

`kv_b_proj` 这个名字容易误解。这里的 `b` 不是 bias。实际代码里这个线性层通常是
`bias=False`。

`b` 来自 DeepSeek MLA 的两段式投影命名：

```text
q_a_proj:  Query 侧第一段投影，通常是 down-projection，得到 q latent
q_b_proj:  Query 侧第二段投影，通常是 up-projection，得到各 head 的 Q

kv_a_proj_with_mqa:
           KV 侧第一段投影，从 hidden state 得到 MLA cache:
           c 和 K^r

kv_b_proj:
           KV 侧第二段投影，从 c 解压出各 head 的 K^c 和 V
```

也就是说，`a` / `b` 表示这条投影链上的第 1 段和第 2 段：

$ X ->^("a") c ->^("b") [K^c,V] $

在 MLA 的 KV 路径里：

```text
kv_a_proj_with_mqa(hidden_states)
  -> latent_cache = Concat(c, K^r)

kv_b_proj(c)
  -> Concat(K^c, V)
```

所以：

```text
kv_b_proj ≈ KV branch, stage b projection
```

从理论符号看，它就是合并后的 up-projection：

$ "kv_b_proj" approx [W^("upK"), W^("upV")] $

这里的 `b` 只是工程命名中“第二段”的意思。不要把它理解成线性层里的 bias，也不要把
`kv_b_proj` 理解成一个新的 MLA 理论组件。

= 为什么 prefill 会显式解压 Key/Value

MLA 理论里常强调推理时的矩阵吸收：

$ Q_i (K_i^c)^T = (X W_i^Q) (c W_i^("upK"))^T $

$ = X W_i^Q (W_i^("upK"))^T c^T $

令：

$ W_i^("eff") = W_i^Q (W_i^("upK"))^T $

则：

$ Q_i (K_i^c)^T = X W_i^("eff") c^T $

这样 decode 时可以避免显式物化每个 head 的完整内容 Key。

但 `_compute_prefill_context` 不是 decode 路径，而是 prefill context 路径。
vLLM 在 MLA 中大致有两种策略：

```text
decode:
  query token 少，历史 context 长；
  更重视减少 KV cache 读取和 per-head K 的物化；
  倾向使用 MQA / matrix absorption 风格。

prefill:
  query token 多，计算更像大块矩阵乘法；
  更重视计算吞吐；
  可以按 chunk 临时解压 K/V，然后跑 MHA-style attention。
```

所以 prefill 中显式出现：

```python
k_nope, v = kv_nope.split(...)
```

不违背 MLA 理论。它只是选择了 compute-friendly 路径：对当前 chunk 物化
$K^c$ 和 $V$，算完这个 chunk 后释放，不长期保存完整 K/V。

= dtype 转换到底在做什么

理论公式默认在实数域中计算：

$ [K^c,V] = c W_"up" $

工程实现中，$c$ 和 $W_"up"$ 不一定都是 BF16。可能有：

```text
c 来自普通 cache/workspace: BF16 或 FP16
c 来自 FP8 prefill/cache:   FP8
W_up 普通权重:              BF16 或 FP16
W_up FP8 权重:              FP8
W_up NVFP4 权重:            packed uint8
W_up AWQ/GPTQ 权重:         量化 layer 自己定义 params_dtype
```

这时真正执行的不是抽象的实数矩阵乘法，而是某个低精度 linear kernel：

$ [K^c,V] = "Linear"("activation", "weight") $

为了让这个 kernel 接收到它期望的 activation dtype，需要在调用 `kv_b_proj` 前处理：

$ c' = cast(c, "expected activation dtype") $

$ [K^c,V] = c' W_"up" $

这个 $cast$ 不应该理解成 MLA 公式的新一步，而是工程实现中的数据格式适配。

= 什么是 linear kernel

这里的 kernel 不是线性代数里的核空间，也不是操作系统 kernel，而是 GPU 编程里的
kernel：一段在 GPU 上执行的底层计算程序。

`linear kernel` 指执行线性层的底层算子。数学上它只是：

$ Y = X W $

在这段 MLA 代码中，它就是：

```python
kv_nope = self.kv_b_proj(kv_c_normed)[0]
```

对应理论公式：

$ [K^c,V] = c [W^("upK"), W^("upV")] $

但 GPU kernel 不只关心公式，还关心输入、权重、scale、输出的 dtype 和内存布局。
例如：

```text
BF16 linear kernel:
  activation 通常是 BF16
  weight 通常是 BF16
  output 通常是 BF16

FP8 linear kernel:
  activation 可能是 FP8，或者先由 layer 内部量化
  weight 是 FP8
  计算还需要 scale

NVFP4 linear kernel:
  weight 可能以 uint8 packed 格式保存两个 FP4 值
  activation 通常仍是 BF16/FP16
  kernel 内部根据 scale 处理量化权重
```

所以文档里说“linear kernel 能接受的 activation dtype”，意思是：

```text
执行 kv_b_proj 这个线性层的 CUDA/Triton/CUTLASS/AITER 底层程序，
要求传进去的 activation tensor 使用它支持的 dtype。
```

如果 kernel 期望 BF16 activation，却拿到 FP8 activation，可能不能编译、不能运行，
或者走到错误的 quantization 路径。`kv_c_normed.to(...)` 的作用就是在调用
`kv_b_proj` 前把 latent $c$ 调整成这条 linear 路径能接受的数据格式。

= 逐句解释条件

代码：

```python
_kv_b_proj_w_dtype = (
    self.kv_b_proj.weight.dtype
    if hasattr(self.kv_b_proj, "weight")
    else self.kv_b_proj.params_dtype
)

if (
    use_fp8_prefill or _kv_b_proj_w_dtype != current_platform.fp8_dtype()
) and _kv_b_proj_w_dtype != torch.uint8:
    kv_c_normed = kv_c_normed.to(_kv_b_proj_w_dtype)
```

第一部分：

```python
use_fp8_prefill
```

意思是 prefill attention 路径启用了 FP8。此时 workspace 里的 `kv_c_normed` 可能是
FP8，但 `kv_b_proj` 未必能直接吃 FP8 activation。为了避免 dtype 不匹配，需要转成
`kv_b_proj` 期望的 dtype。

第二部分：

```python
_kv_b_proj_w_dtype != current_platform.fp8_dtype()
```

意思是 `kv_b_proj` 的权重不是平台 FP8 dtype。比如权重是 BF16，那么输入 latent
也需要转成 BF16，才能走普通 BF16 linear。

第三部分：

```python
_kv_b_proj_w_dtype != torch.uint8
```

这是 NVFP4 的特殊保护。NVFP4 权重通常以 packed `uint8` 保存，但 activation 不应该
被转成 `uint8`。如果把 latent $c$ 转成 uint8，数学含义就坏了：

```text
正确:  浮点 latent c -> NVFP4 linear 内部量化/反量化路径
错误:  c.to(uint8) -> 浮点信息被整数截断
```

所以这句的含义是：

```text
如果不是 NVFP4 packed weight，就把 c 转成 kv_b_proj 期望的浮点/FP8 dtype；
如果是 NVFP4 packed weight，就保持 c 为 model dtype，让 linear layer 内部处理量化。
```

= 为什么看起来和理论不一致

不一致感主要来自三点。

第一，理论只写：

$ K^c,V = c W_"up" $

代码却写：

```python
kv_c_normed = kv_c_normed.to(...)
kv_nope = self.kv_b_proj(kv_c_normed)
```

这里的 `to(...)` 是低精度执行格式，不是模型结构。

第二，理论强调矩阵吸收避免显式展开 Key。代码却显式生成 `k_nope`。

原因是这段代码属于 prefill context。prefill 为了吞吐，可以按 chunk 临时物化 K/V。
decode 才更依赖 matrix absorption / MQA-style 路径来减少历史 KV 的搬运。

第三，`kv_b_proj.weight.dtype == torch.uint8` 容易让人误以为 activation 也要 uint8。

这是错误直觉。`uint8` 在这里只是 packed NVFP4 权重的存储格式，不是理论上的数值域。
activation latent $c$ 仍然应该作为浮点激活进入 quantized linear。

= 一句话总结

`_compute_prefill_context` 做的是：

```text
从 MLA KV cache 中按 chunk 取出 c 和 K^r，
用 kv_b_proj 把 c 临时解压成 K^c 和 V，
拼出 K = Concat(K^c, K^r)，
计算 query 对这一块历史 context 的 attention，
再用 LSE 合并多个 chunk 的 softmax 状态。
```

dtype 转换做的是：

```text
在执行 [K^c,V] = c W_up 前，
把 c 调整成 kv_b_proj 的低精度 linear kernel 能接受的 activation dtype。
它是工程执行细节，不是 MLA 理论公式的变化。
```
