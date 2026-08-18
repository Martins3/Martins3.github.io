## CUDA Graph 优化详解
<!-- a10df343-6d5e-4f5e-b3dd-bd444d02d11b -->

一、什么是 CUDA Graph？

CUDA Graph 是 NVIDIA GPU 的一种执行机制，它将一系列 GPU 操作（kernels）预先录制下来，形成一个"图"，之后可以一键重放，避免重复的 CPU 开销。

传统执行方式的问题：

每次推理 → CPU 逐个提交 kernel → GPU 执行
         ↓
    重复的 CPU 开销（launch overhead）

CUDA Graph 执行方式：

录制阶段：CPU 一次性录制所有 kernels → 形成 Graph
重放阶段：一键 replay Graph → 零 CPU 开销

二、vLLM 为什么要用 CUDA Graph？

 问题                        CUDA Graph 解决方案
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 CPU launch overhead         预录制，一键重放
 Kernel 调度间隙             减少 kernel 间隔，提高 GPU 利用率
 动态 shape 带来的编译开销   预 capture 常见 shape
 长尾延迟                    稳定的执行时间

三、vLLM 的 CUDA Graph 架构

┌─────────────────────────────────────────────────────────┐
│                 CudagraphDispatcher                     │
│  (调度器：决定使用哪种 CUDA Graph 模式)                    │
└─────────────────────────────────────────────────────────┘
                           ↓
         ┌─────────────────┼─────────────────┐
         ↓                 ↓                 ↓
    ┌─────────┐      ┌──────────┐      ┌──────────┐
    │  NONE   │      │   FULL   │      │PIECEWISE │
    │ (无CG)  │      │ (全图)    │      │ (分段图)  │
    └─────────┘      └──────────┘      └──────────┘
                          ↑                  ↑
                    ┌─────────────┐     ┌─────────────┐
                    │CUDAGraphWrapper    │CUDAGraphWrapper
                    │  (单一大图)   │     │ (逐层小图)   │
                    └─────────────┘     └─────────────┘

四、三种运行模式详解

1. NONE 模式 - 不使用 CUDA Graph

• 用于：预热、profile、debug
• 每次 forward 都是 eager 模式执行

2. FULL 模式 - 完整图模式

# 整个模型 forward 被 capture 为一张大图
with torch.cuda.graph(graph, pool):
    output = model(**inputs)

• 优点: 最大性能，零 CPU 开销
• 缺点: 需要固定 shape（batch size、sequence length）
• 适用场景: Decode 阶段（token 数固定为1）

3. PIECEWISE 模式 - 分段图模式

# 每一层/每一部分单独 capture
for layer in model.layers:
    with torch.cuda.graph(graph, pool):
        hidden_states = layer(hidden_states)

• 优点: 更灵活，支持动态 batch size
• 缺点: 层间仍有少量 CPU 开销
• 适用场景: Prefill 阶段或动态 batch

五、核心组件代码解析

1. CudagraphDispatcher - 调度中心

# 根据输入特征决定使用哪种模式
def dispatch(self, num_tokens, uniform_decode, has_lora, ...):
    # 检查是否匹配 FULL 模式的 graph
    if batch_desc in self.cudagraph_keys[CUDAGraphMode.FULL]:
        return CUDAGraphMode.FULL, batch_desc

    # 检查是否匹配 PIECEWISE 模式的 graph
    if batch_desc in self.cudagraph_keys[CUDAGraphMode.PIECEWISE]:
        return CUDAGraphMode.PIECEWISE, batch_desc

    # 都不匹配，回退到 eager
    return CUDAGraphMode.NONE, batch_desc

2. CUDAGraphWrapper - Graph 包装器

class CUDAGraphWrapper:
    def __call__(self, *args, **kwargs):
        # 获取当前的 batch 描述符
        batch_descriptor = forward_context.batch_descriptor

        # 检查是否已 capture
        if entry.cudagraph is None:
            # 首次遇到这个 shape，进行 capture
            with torch.cuda.graph(cudagraph, pool=self.graph_pool):
                output = self.runnable(*args, **kwargs)
            entry.cudagraph = cudagraph
            return output

        # 已 capture，直接 replay
        entry.cudagraph.replay()
        return entry.output

3. CudaGraphManager - Graph 管理器

class CudaGraphManager:
    def capture(self, create_forward_fn, ...):
        # 为每种预设的 shape capture graph
        for desc in capture_descs:
            # 1. 准备 dummy inputs
            forward_fn = create_forward_fn(desc)

            # 2. Warmup（确保内存分配完成）
            forward_fn(CUDAGraphMode.NONE)

            # 3. Capture CUDA Graph
            graph = torch.cuda.CUDAGraph()
            with torch.cuda.graph(graph, self.pool):
                forward_fn(CUDAGraphMode.NONE)

            self.graphs[desc] = graph

六、Batch Size Padding 机制

由于 CUDA Graph 要求固定 shape，vLLM 使用 padding 策略：

实际请求：3 个 tokens
         ↓
查表找到：padding 到 8 个 tokens（预设的 capture size）
         ↓
使用预先 capture 的 8-token graph 执行
         ↓
只取前 3 个 token 的结果

Capture Sizes 配置：

# 默认预设的 capture sizes（可配置）
cudagraph_capture_sizes = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048]

七、Decode 阶段特殊优化

Decode 阶段每个请求只生成 1 个 token，非常适合 FULL 模式：

# Decode 专用 graph
desc = BatchExecutionDescriptor(
    cg_mode=CUDAGraphMode.FULL,
    num_tokens=num_reqs * 1,  # 每个请求 1 个 token
    num_reqs=num_reqs,
    uniform_token_count=1,     # 均匀的 decode
)

八、与 PyTorch Compile 的集成

```txt
# vLLM 支持编译 + CUDA Graph 组合使用
compilation_config = {
    "mode": "VLLM_COMPILE",           # 启用 Inductor 编译
    "cudagraph_mode": "PIECEWISE",    # 同时启用分段 CUDA Graph
}
```

执行流程：

1. PyTorch Inductor 编译模型（融合 kernels）
         ↓
2. 在编译后的模型上 capture CUDA Graph
         ↓
3. 运行时一键 replay 优化后的 graph

九、关键配置参数

 参数                         说明                   默认值
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 cudagraph_mode               CUDA Graph 模式        NONE
 cudagraph_capture_sizes      预设 capture 大小      [1,2,4,8,...]
 max_cudagraph_capture_size   最大 capture size      10000
 cudagraph_specialize_lora    为 LoRA 专门 capture   False

十、性能收益

 场景                  典型收益
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Decode（小 batch）    2-3x 加速
 Decode（大 batch）    1.2-1.5x 加速
 Prefill（固定长度）   1.1-1.3x 加速
 端到端延迟            减少 20-50%

十一、使用示例

```txt
from vllm import LLM

# 启用 CUDA Graph
llm = LLM(
    model="meta-llama/Llama-2-7b",
    compilation_config={
        "cudagraph_mode": "FULL",  # 或 "PIECEWISE"
        "cudagraph_capture_sizes": [1, 2, 4, 8, 16, 32],
    }
)

# 之后的推理会自动使用 CUDA Graph
output = llm.generate("Hello, world!")
```

────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
总结：vLLM 的 CUDA Graph 优化通过预录制 GPU 执行图，消除了 CPU launch overhead，特别是在 Decode 阶段能带来显著性能提升。通过灵活的 FULL/PIECEWISE 模式切换，vLLM 在性能
和灵活性之间取得了很好的平衡。
