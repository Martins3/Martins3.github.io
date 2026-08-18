# CUDA Graph 为什么会占用显存

(我们在调试 vllm 的时候，--enforce-eager 会导致显存不够，才意识到 cuda graph 会导致显存消耗增加)

CUDA Graph 占显存，核心原因是它为了能稳定 replay，必须把一部分“运行时动态资源”变成“固定地址、固定形状、固定生命周期”的资源。

## 1. replay 要求指针稳定

CUDA Graph capture 的不是抽象计算：

```text
matmul(A, B) -> C
```

而是更接近：

```text
在 stream S 上，用地址 ptr_A、ptr_B、ptr_C，按这些 kernel 参数执行这一串 CUDA kernel
```

所以 replay 时，很多输入、输出、中间 buffer 的地址必须稳定。PyTorch/vLLM 不能像普通 eager 那样每次临时分配、释放、换地址。

于是需要保留一批静态 buffer：

```text
input_ids / positions / hidden_states 静态 buffer
attention metadata 静态 buffer
logits / sampled token buffer
中间激活 buffer
部分 attention workspace
部分 backend persistent buffer
```

这些 buffer 一旦为 graph 准备好，就不能随便释放，否则下次 replay 指针就不对了。

## 2. PyTorch CUDA Graph 会使用私有 memory pool

普通 eager 下，PyTorch CUDA caching allocator 可以比较自由地复用显存：

```text
op1 分配临时 tensor
op1 用完
op2 复用同一块显存
op3 再复用
```

CUDA Graph capture 时，PyTorch 通常会给 graph 使用一个 private pool。capture 期间发生的 CUDA allocation 会从这个 graph pool 分配，并在 graph 存活期间保留。

原因是 replay 必须保证：

```text
同一个 graph
同一个 tensor 地址
同一个内存布局
```

所以这些 allocation 不能回到普通 allocator 里被别的 eager op 抢走。结果就是：

```text
普通 eager 可复用的临时显存
  -> 变成 graph 私有池里的保留显存
  -> nvidia-smi 看起来常驻占用增加
```

这部分通常是 CUDA Graph 显存增加的主要来源。

## 3. capture size 会导致 padding 和静态最大尺寸 buffer

vLLM 不可能为每个真实 batch shape 都 capture 一张 graph，所以会预先捕获一些固定 size，例如：

```text
1, 2, 4, 8, 16, 24, 32, ...
```

运行时如果真实 batch 是 13，可能会选择 16 的 graph：

```text
真实 batch = 13
graph batch = 16
多出来 3 个 padding slot
```

因此 graph buffer 经常按 capture size 分配，而不是按真实 batch 分配。

这会带来两类显存开销：

```text
1. 静态输入/输出 buffer 按 capture size 分配
2. 中间 tensor / workspace 也按 capture size 分配
```

如果最大 capture size 很大，比如接近 `max_num_seqs` 或 `max_num_batched_tokens`，静态 buffer 就会明显变大。

## 4. vLLM 会捕获多张 graph

不是只捕获一张。

可能按这些维度生成多张 graph：

```text
不同 batch size
decode-only graph
mixed prefill/decode graph
piecewise graph
full graph
spec decode query length
LoRA specialization 情况
不同 attention backend 支持模式
```

每多一类 graph，就可能多一组 graph executable、静态 buffer 或 private pool 高水位。

即使某些 graph 共用 pool，显存也通常按“所有 capture 过程中需要过的最高水位”保留。

所以你会看到这种现象：

```text
启动时 / warmup 时显存上涨
capture 完之后显存不降
后续推理显存稳定
```

这是正常的 CUDA Graph 行为。

## 5. capture 期间可能有额外峰值

capture 不是直接从零开始。通常流程类似：

```text
加载模型权重
分配 KV cache
warmup eager run
为 graph 准备静态 buffer
capture graph
实例化 graph executable
保留 graph memory pool
```

在 warmup/capture 过程中，普通 allocator 的缓存、graph private pool、backend workspace 可能短时间并存。因此峰值显存可能比最终稳定值更高。

这也是为什么有些模型：

```text
不开 CUDA Graph 能启动
开 CUDA Graph 在 capture 阶段 OOM
```

## 6. CUDA Graph 本身也有元数据，但通常不是大头

CUDA driver 需要保存 graph executable：

```text
kernel node
dependency edge
kernel 参数
launch 配置
内存节点信息
graph executable object
```

这部分也占 GPU/driver 相关内存，但通常比静态 tensor buffer、workspace、allocator pool 小很多。

大头一般还是：

```text
静态 tensor buffer
graph private memory pool
attention / matmul workspace
padding 后的中间激活
多 capture size 的累计
```

## 7. 它不会复制模型权重，但会增加运行时状态

容易误解的一点：CUDA Graph 通常不会复制一份模型权重。

模型权重本来就在 GPU 上，graph replay 只是引用这些权重地址。

但是它会增加这些东西：

```text
graph 输入输出静态 buffer
graph 内部中间 tensor
capture 时分配的 workspace
固定 shape 的 metadata tensor
graph executable 和 allocator pool
```

所以显存增加不是因为“模型多了一份”，而是“执行路径多保留了一批固定运行时内存”。

## 8. KV cache 和 CUDA Graph 的关系

KV cache 本身不是 CUDA Graph 才有的。vLLM 无论 eager 还是 graph，通常都需要预分配 KV cache。

但 CUDA Graph 会影响 KV cache 周边的访问和 metadata：

```text
slot_mapping
block_table
seq_lens
positions
query_start_loc
attention workspace
padding batch 的 dummy entries
```

这些东西为了 graph replay，也可能变成固定 buffer。

所以总显存可以粗略看成：

```text
总显存 =
  模型权重
+ KV cache
+ 普通运行时 workspace
+ CUDA Graph 静态 buffer
+ CUDA Graph private pool
+ graph executable/driver metadata
+ allocator 碎片和缓存
```

开 `--enforce-eager` 主要去掉后面 CUDA Graph 相关几项。

## 9. 为什么 vLLM 文档说 enforce_eager 能省显存

当前代码里 `enforce_eager=True` 会把：

```text
compilation_config.cudagraph_mode = NONE
max_cudagraph_capture_size = 0
cudagraph_capture_sizes = []
```

也就是不捕获 graph，不分配这些 graph 静态资源。对应结果：

```text
显存更省
启动更轻
不会有 graph capture OOM
但 decode 性能通常下降
```

## 直观类比

普通 eager 像每次做饭都临时拿锅碗瓢盆，用完放回公共架子。

CUDA Graph 像为了快速出餐，给某几种套餐固定摆好一整套锅碗瓢盆和工位，不能挪作他用。出餐快了，但厨房空间被长期占住了。

对应到 GPU：

```text
速度来自固定化和复用
显存开销也来自固定化和复用
```

所以 CUDA Graph 的取舍本质是：

```text
用更多常驻显存
换更少 CPU launch overhead
换更稳定、更低延迟的 decode replay
```
