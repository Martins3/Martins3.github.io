# LeetGPU Medium/Hard 题目整理
<!-- 8548561d-c34c-42c5-89c9-a9a2aa77a166 -->

本仓库当前包含 55 道 medium 和 13 道 hard。整体看下来，GPU 题目不是单纯考 CUDA 语法，而是在反复考这些能力：

1. **并行规约和扫描**：sum/max、softmax、prefix sum、segmented scan、stream compaction。
2. **内存访问模式**：coalesced load/store、shared memory tile、边界处理、2D/3D flatten index。
3. **同步与原子操作**：histogram、count、top-k、merge、sort、稀疏/不规则写入。
4. **矩阵乘和 Tensor Core 思维**：GEMM、batched matmul、FP16、INT8/INT4、反量化融合。
5. **数值稳定性**：softmax 的 max trick、loss 的 logsumexp、FP16 累加、量化 scale/zero-point。
6. **深度学习基础算子**：conv、pooling、normalization、loss、MLP、LoRA、MoE gating。
7. **LLM 推理核心算子**：attention、causal/sliding/linear/GQA、RoPE、KV cache、top-p、speculative decoding。
8. **并行算法改写**：排序、radix sort、FFT、BFS、Floyd-Warshall、k-means、stencil、recurrence/scan。

## 建议学习顺序

1. 先做规约/扫描：`4_reduction`、`17_dot_product`、`5_softmax`、`16_prefix_sum`、`70_segmented_prefix_sum`。
2. 再做内存布局和 stencil：`10_2d_convolution`、`11_3d_convolution`、`28_gaussian_blur`、`69_jacobi_stencil_2d`。
3. 然后做矩阵乘体系：`22_gemm`、`30_batched_matrix_multiplication`、`57_fp16_batched_matmul`、`32_int8_quantized_matmul`、`81_int4_matmul`。
4. 接着做 selection/atomic/irregular：`13_histogramming`、`29_top_k_selection`、`72_stream_compaction`、`71_parallel_merge`、`15_sorting`、`36_radix_sort`。
5. 最后集中刷 LLM kernel：`6_softmax_attention`、`53_casual_attention`、`80_grouped_query_attention`、`96_int8_kv_cache_attention`、`74_gpt2_block`、`93_llama_transformer_block`。

## 考点地图

### 规约、扫描、选择、数据重排

这些题主要考 block 内规约、跨 block 聚合、warp-level primitive、prefix-sum 构造、atomic 或稳定重排。

- `medium/4_reduction` - Reduction：float 数组求和。
- `medium/5_softmax` - Softmax：max + exp + sum + normalize，典型多阶段规约。
- `medium/13_histogramming` - Histogramming：整数直方图，考 atomic contention。
- `medium/16_prefix_sum` - Prefix Sum：全局 inclusive scan。
- `medium/17_dot_product` - Dot Product：乘加规约。
- `medium/29_top_k_selection` - Top K Selection：选最大 k 个并降序输出。
- `medium/43_count_array_element` - Count Array Element：1D 条件计数。
- `medium/44_count_2d_array_element` - Count 2D Array Element：2D 条件计数和 flatten index。
- `medium/45_count_3d_array_element` - Count 3D Array Element：3D 条件计数和 flatten index。
- `medium/47_subarray_sum` - Subarray Sum：区间求和。
- `medium/48_2d_subarray_sum` - 2D Subarray Sum：二维子区域求和。
- `medium/49_3d_subarray_sum` - 3D Subarray Sum：三维子区域求和。
- `medium/51_max_subarray_sum` - Max Subarray Sum：固定窗口最大和。
- `medium/60_top_p_sampling` - Top-p Sampling：排序/累计概率/采样，LLM decode 后处理。
- `medium/67_moe_topk_gating` - MoE Top-K Gating：每 token 选 top-k expert。
- `medium/70_segmented_prefix_sum` - Segmented Exclusive Prefix Sum：分段 scan。
- `medium/71_parallel_merge` - Parallel Merge：两个有序数组并行归并。
- `medium/72_stream_compaction` - Stream Compaction：保序过滤正数，scan + scatter。
- `hard/15_sorting` - Sorting：float 数组排序。
- `hard/36_radix_sort` - Radix Sort：uint32 基数排序，考 histogram/scan/scatter 组合。

### Dense/Sparse 线性代数与量化

这些题主要考 tile 化、shared memory、寄存器 blocking、访存合并、混合精度累加，以及稀疏格式的不规则访问。

- `medium/18_sparse_matrix_vector_multiplication` - Sparse Matrix-Vector Multiplication：稀疏矩阵乘向量。
- `medium/22_gemm` - General Matrix Multiplication：基础 GEMM，含 alpha/beta。
- `medium/30_batched_matrix_multiplication` - Batched Matrix Multiplication：batch 维度上的 FP32 matmul。
- `medium/32_int8_quantized_matmul` - INT8 Quantized MatMul：INT8 矩阵乘 + scale/zero-point。
- `medium/33_ordinary_least_squares` - Ordinary Least Squares：OLS 回归。
- `medium/34_logistic_regression` - Logistic Regression：逻辑回归。
- `medium/37_matrix_power` - Matrix Power：矩阵幂。
- `medium/57_fp16_batched_matmul` - FP16 Batched Matrix Multiplication：半精度 batch matmul。
- `medium/58_fp16_dot_product` - FP16 Dot Product：FP16 输入、累加精度和吞吐。
- `medium/64_weight_dequantization` - Weight Dequantization：tile 级 scale 反量化。
- `medium/75_sparse_matrix_dense_matrix_multiplication` - Sparse Matrix-Dense Matrix Multiplication：稀疏矩阵乘稠密矩阵。
- `medium/81_int4_matmul` - INT4 Weight-Only Quantized MatMul：W4A16，packed INT4 解包 + matmul。
- `medium/85_lora_linear` - LoRA Linear：base linear + low-rank update，适合融合。

### Convolution、Stencil、图像/网格算子

这些题主要考 2D/3D 索引、halo/boundary、局部邻域复用、shared memory tile 和边界分支。

- `medium/10_2d_convolution` - 2D Convolution：valid 2D 卷积。
- `medium/11_3d_convolution` - 3D Convolution：valid 3D 卷积。
- `medium/28_gaussian_blur` - Gaussian Blur：图像卷积滤波。
- `medium/42_2d_max_pooling` - 2D Max Pooling：kernel/stride/padding 下采样。
- `medium/69_jacobi_stencil_2d` - 2D Jacobi Stencil：5-point stencil。
- `medium/90_causal_depthwise_conv1d` - Causal Depthwise Conv1d：序列上逐通道 causal conv。

### Normalization、Loss、MLP 和常见 DL 算子

这些题主要考逐行/逐通道规约、广播、融合激活、数值稳定和小矩阵/小向量批处理。

- `medium/25_categorical_cross_entropy_loss` - Categorical Cross Entropy Loss：logits 上的 logsumexp 和平均 loss。
- `medium/27_mean_squared_error` - Mean Squared Error：差平方平均，规约。
- `medium/40_batch_normalization` - Batch Normalization：按 feature 计算 mean/var 并 affine。
- `medium/50_rms_normalization` - RMS Normalization：RMS 规约 + scale/shift。
- `medium/84_swiglu_mlp_block` - SwiGLU MLP Block：gate/up/down 三个投影 + SiLU 融合。

### Attention 与 LLM 推理

这是 medium/hard 的重心。核心考点是 QK^T、softmax、mask/bias、KV cache 带宽、head/group 映射、RoPE、MLP 和 kernel fusion。

- `medium/6_softmax_attention` - Softmax Attention：基础 QK softmax V。
- `medium/55_attn_w_linear_bias` - Attention with Linear Biases：ALiBi attention。
- `medium/61_rope_embedding` - Rotary Positional Embedding：RoPE 旋转。
- `medium/76_adder_transformer` - Adder Transformer Inference：小型 transformer 自回归推理。
- `medium/80_grouped_query_attention` - Grouped Query Attention：GQA，KV head 复用。
- `medium/87_speculative_decoding_verification` - Speculative Decoding Verification：draft token 接受/拒绝验证。
- `medium/92_decaying_causal_attention` - Decaying Causal Attention：带衰减因子的 causal attention。
- `medium/96_int8_kv_cache_attention` - INT8 KV-Cache Attention：decode 阶段 INT8 KV cache 反量化 + attention。
- `hard/12_multi_head_attention` - Multi-Head Attention：完整 MHA。
- `hard/53_casual_attention` - Causal Self-Attention：masked self-attention。
- `hard/56_linear_attention` - Linear Self-Attention：linear attention。
- `hard/59_sliding_window_attn` - Sliding Window Self-Attention：局部窗口 attention。
- `hard/74_gpt2_block` - GPT-2 Transformer Block：LayerNorm + MHA + FFN + residual。
- `hard/93_llama_transformer_block` - Llama Transformer Block：RMSNorm + GQA + RoPE + SwiGLU + residual。

### 序列模型、Scan 和状态空间

这些题表面是 recurrence，实质通常要改写成 scan 或分块 scan，否则串行依赖会限制并行度。

- `medium/82_linear_recurrence` - Linear Recurrence：`h[t] = a[t] * h[t-1] + x[t]`。
- `medium/94_ssm_selective_scan` - SSM Selective Scan：Mamba 风格 selective scan。

### 图算法、几何、模拟、聚类

这些题主要考不规则访存、分支发散、frontier/迭代算法、距离计算和收敛迭代。

- `medium/35_monte_carlo_integration` - Monte Carlo Integration：样本平均和积分估计。
- `medium/38_nearest_neighbor` - Nearest Neighbor：3D 点最近邻，O(N^2) 距离搜索。
- `hard/14_multi_agent_sim` - Multi-Agent Simulation：boids 模拟，邻域交互。
- `hard/20_kmeans_clustering` - K-Means Clustering：点分配 + centroid 更新。
- `hard/46_bfs_shortest_path` - BFS Shortest Path：网格 BFS。
- `hard/73_all_pairs_shortest_paths` - All-Pairs Shortest Paths：Floyd-Warshall。

### FFT 和频域算法

这些题主要考 butterfly、复数数据布局、分阶段同步/转置，以及从朴素 DFT 到 FFT 的并行化。

- `medium/78_2d_fft` - 2D FFT：2D DFT/FFT，行列分解。
- `hard/39_Fast_Fourier_transform` - Fast Fourier Transform：1D FFT。

## 详细版补充：GPU 题目到底在考什么

### 1. 规约、扫描和数据搬运是底层基本功

这一类题数量最多，因为很多复杂 kernel 最后都会退化成“局部计算 + 规约/扫描 + scatter/gather”。真正要练的是如何把串行依赖拆成分层并行：thread 内处理多个元素，warp 内用 shuffle，block 内用 shared memory，block 间再做二阶段或多阶段归并。

重点能力：

- **规约树设计**：sum/max/min/dot product 都需要减少同步次数，避免 shared memory bank conflict。
- **多阶段 kernel**：全局数组太大时，单个 block 得不到最终结果，需要 first-pass partials + second-pass reduce。
- **scan 构造**：prefix sum、segmented prefix sum、stream compaction、radix sort 都依赖 scan。
- **稳定输出**：stream compaction 和 merge 不只是算数量，还要保证顺序或正确位置。
- **原子热点处理**：histogram/count/top-k/MoE gating 容易出现多个 thread 写同一个位置。

逐题更细：

- `medium/4_reduction`：最基础的全局 sum。考 block-level reduction、grid-stride loop、partial sum 输出。性能瓶颈通常是 global memory bandwidth 和跨 block 汇总。
- `medium/17_dot_product`：比 reduction 多一步逐元素乘法。考 fused multiply-add 和累加精度，FP32 下通常是 memory-bound。
- `medium/5_softmax`：需要三次逻辑步骤：max、sum(exp(x-max))、normalize。考数值稳定、block 内多次规约、是否把中间结果写回 global memory。
- `medium/16_prefix_sum`：全局 scan。难点是单 block scan 不够，需要分块 scan、block sums scan、再 uniform add。
- `medium/70_segmented_prefix_sum`：scan 的升级版。flags 会打断累积状态，考 segmented operator 的结合律设计，以及 segment 跨 block 时的 carry 处理。
- `medium/72_stream_compaction`：典型 scan + scatter。先生成 predicate，再 scan 得到输出位置，最后按位置写出；还要填充尾部 0。
- `medium/13_histogramming`：考 atomicAdd。朴素全局 atomic 会有热点，优化方向是 per-block shared histogram，再合并到全局。
- `medium/43_count_array_element`、`44_count_2d_array_element`、`45_count_3d_array_element`：表面是 count，实际是索引映射 + 条件规约。3D 版本更容易暴露 flatten index 和边界错误。
- `medium/47_subarray_sum`、`48_2d_subarray_sum`、`49_3d_subarray_sum`：指定子区域求和。小范围可直接规约，大范围则关注连续内存方向、coalescing、二维/三维边界。
- `medium/51_max_subarray_sum`：固定窗口最大和。可以用滑动窗口/prefix sum，也可以并行计算每个窗口；考重复读取和局部缓存。
- `medium/29_top_k_selection`：top-k 不是全排序。k 小时常用局部 top-k + merge；k 大时接近 sort。考比较网络、局部排序和全局归并。
- `medium/60_top_p_sampling`：LLM sampling 后处理。通常需要 softmax/概率排序/前缀和/阈值截断/采样，考多个基础 primitive 组合。
- `medium/67_moe_topk_gating`：对每个 token 的 expert logits 做 top-k。考 row-wise top-k、输出 indices/weights、softmax normalize，以及 M 和 E 两个维度的并行划分。
- `medium/71_parallel_merge`：考 merge path 这类并行归并思想。关键是每个 thread/block 找到自己负责的 A/B 切分点，避免串行双指针。
- `hard/15_sorting`：float 数组排序。可用 bitonic/merge/radix 思路。难点是全局同步只能靠多 kernel，不能在一个 kernel 里完成任意规模全局排序。
- `hard/36_radix_sort`：radix sort 是 histogram + prefix sum + scatter 的组合题。每一轮处理若干 bit，核心是稳定分桶和多 pass 全局搬运。

### 2. 矩阵乘、稀疏和量化是性能核心

矩阵乘类题考的是 GPU 性能优化的主干：tile、shared memory、寄存器复用、访存合并、循环展开、混合精度。量化题会额外考解包、scale、zero-point 和反量化融合，稀疏题会考不规则访存。

重点能力：

- **tiling**：按 M/N/K 切块，复用 A/B tile，减少 global memory traffic。
- **数据布局**：row-major 下连续维度是谁，决定 thread 映射和 coalescing。
- **累加类型**：FP16 输入通常要 FP32 accumulation；INT8/INT4 要处理 scale 和 zero-point。
- **融合**：反量化、bias、activation、LoRA update 能否和 matmul 合并，决定额外带宽。
- **稀疏格式**：CSR/COO 等格式下负载不均衡和间接寻址会成为主要问题。

逐题更细：

- `medium/22_gemm`：标准 GEMM，含 `C = alpha * A @ B + beta * C`。考 tile 化矩阵乘、边界块、alpha/beta 融合。
- `medium/30_batched_matrix_multiplication`：batch 维度引入第三维 grid。难点是每个 batch 的矩阵地址偏移，以及小矩阵时 occupancy 和 launch overhead。
- `medium/57_fp16_batched_matmul`：FP16 输入输出。考 half/half2、Tensor Core 思维、FP32 accumulation 与误差容忍。
- `medium/58_fp16_dot_product`：半精度 dot product。重点是吞吐和精度之间的平衡，避免直接用 FP16 长链累加造成误差。
- `medium/32_int8_quantized_matmul`：INT8 A/B + scale/zero-point。关键是计算 `(a - zpA) * (b - zpB)` 的 int accumulation，再按 scale 转回输出。
- `medium/81_int4_matmul`：W4A16 weight-only matmul。核心难点是 packed INT4 解包、符号扩展、group/tile scale，以及把解包成本藏在计算里。
- `medium/64_weight_dequantization`：按 tile scale 反量化。考二维 tile scale 索引：`S[ceil(i/T), ceil(j/T)]`，以及输出连续写。
- `medium/85_lora_linear`：`x W^T + alpha/rank * (x A^T) B^T`。可分两次小 GEMM，也可融合部分中间结果；考低秩结构和临时缓冲权衡。
- `medium/18_sparse_matrix_vector_multiplication`：SpMV。难点是每行非零元数量不同，thread/block per row 的策略会影响负载均衡。
- `medium/75_sparse_matrix_dense_matrix_multiplication`：SpMM。比 SpMV 多一个 dense matrix 的 K 维，考稀疏行遍历和 dense 列方向 coalescing。
- `medium/37_matrix_power`：矩阵幂。通常用 repeated squaring 或多次 matmul；考临时矩阵管理、P 的二进制分解和边界规模。
- `medium/33_ordinary_least_squares`：OLS 本质是线性代数组合，可能涉及 `X^T X`、`X^T y` 和求解小线性系统。考矩阵规约和数值稳定。
- `medium/34_logistic_regression`：迭代优化类题。考 sigmoid、梯度规约、矩阵向量乘和多轮迭代中的同步边界。

### 3. 卷积、stencil 和网格题在考局部性

这类题不是单纯套公式，而是考“邻域访问如何复用”。如果每个输出点都从 global memory 重新读完整窗口，性能会被带宽限制；shared memory halo tile 是常见优化方向。

重点能力：

- **2D/3D flatten index**：row/col/depth 的地址计算必须稳定。
- **halo 区域**：卷积和 stencil 需要读输出 tile 外一圈或多圈数据。
- **边界处理**：valid/padding/causal/boundary copy 都容易产生 off-by-one。
- **channel 独立 vs channel 混合**：depthwise conv 与普通 conv 的并行策略不同。

逐题更细：

- `medium/10_2d_convolution`：valid 2D convolution。考每个输出点读取 `kernel_h * kernel_w` 的邻域，适合 shared memory 缓存 input tile。
- `medium/11_3d_convolution`：3D convolution。索引更复杂，窗口体积更大，shared memory 压力和寄存器压力都更明显。
- `medium/28_gaussian_blur`：图像滤波。常见优化是利用 separable Gaussian，但题目若给通用 kernel，则重点是局部邻域缓存。
- `medium/42_2d_max_pooling`：max pooling。考 stride/padding 下输出到输入窗口的映射，以及负无穷初始化。
- `medium/69_jacobi_stencil_2d`：5-point stencil。典型 halo tile 题；边界通常保持或特殊处理，内部点读上下左右。
- `medium/90_causal_depthwise_conv1d`：序列 depthwise causal conv。每个 channel 独立，位置 t 只能读过去窗口；考 `(B, L, D)` 布局下沿 D 还是 L 并行更合适。

### 4. Normalization、loss 和 MLP 题在考“行级规约 + 融合”

这类题通常是深度学习模型里的小算子。它们的特点是单个 row/channel 上有规约，然后把结果 broadcast 回每个元素。性能好坏取决于是否把多次读写融合起来。

重点能力：

- **row-wise reduction**：每个样本/feature 独立规约，block per row 很常见。
- **数值稳定**：softmax loss 要先减 max，variance/RMS 要加 epsilon。
- **广播写回**：规约结果需要复用，避免重复从 global memory 读。
- **激活融合**：SiLU、GELU、SwiGLU 适合和 matmul 后处理合并。

逐题更细：

- `medium/25_categorical_cross_entropy_loss`：对每行 logits 做 logsumexp，并取 true label 的 logit。考稳定 softmax loss 和 batch 平均。
- `medium/27_mean_squared_error`：差平方再规约。简单但能暴露 reduction 模板是否通用。
- `medium/40_batch_normalization`：按 feature 统计 batch 上的 mean/variance，再 normalize。难点是 N/C 维度映射和两阶段规约。
- `medium/50_rms_normalization`：计算 `sqrt(mean(x^2)+eps)` 后归一化。类似 layer norm 但不减均值；典型 row-wise kernel。
- `medium/84_swiglu_mlp_block`：`SiLU(x W_gate) * (x W_up)` 再乘 `W_down`。考三个矩阵乘、激活逐元素融合、临时 buffer 和带宽。

### 5. Attention 和 LLM kernel 是当前最核心方向

LLM 相关题覆盖很广：普通 attention、causal mask、ALiBi、GQA、RoPE、KV cache 量化、sampling、speculative decoding、完整 transformer block。它们共同考察的是矩阵乘 + softmax + 数据布局 + 内存带宽控制。

重点能力：

- **QK^T 分块**：避免生成完整 attention matrix，或者至少按 tile 计算。
- **online softmax**：FlashAttention 的核心思想，分块时维护 row max 和 row sum。
- **mask/bias**：causal、sliding window、ALiBi、decay 都是在 score 上加限制或偏置。
- **head 映射**：MHA/GQA/MQA 的 Q head 到 KV head 映射必须清楚。
- **KV cache 带宽**：decode 阶段通常 batch/head/seq_len/head_dim 的读带宽是瓶颈。
- **RoPE**：半维旋转，cos/sin 的位置和维度索引容易写错。
- **完整 block 融合**：LayerNorm/RMSNorm、attention、MLP、residual 的中间张量会占大量带宽。

逐题更细：

- `medium/6_softmax_attention`：基础 `softmax(QK^T / sqrt(d)) V`。考 attention 的三个阶段和 softmax 稳定性。
- `medium/55_attn_w_linear_bias`：ALiBi 在 score 上加入与距离相关的线性 bias。考 bias 生成和 mask/bias 融合。
- `medium/61_rope_embedding`：RoPE 对向量前后半维做旋转。难点是 `(x1, x2)` 拆分、cos/sin 广播和批量/head 维索引。
- `medium/80_grouped_query_attention`：GQA 让多个 Q head 共享一组 K/V head。关键是 `kv_head = q_head / group_size` 的映射，以及 KV cache 读取复用。
- `medium/96_int8_kv_cache_attention`：decode-phase attention，K/V cache 是 int8，每 token 有 scale。核心是边读边反量化，不要先完整展开成 FP32。
- `medium/92_decaying_causal_attention`：causal attention 加 decay。考 causal mask 和位置距离权重，可能可以利用递推/前缀结构。
- `medium/76_adder_transformer`：小 transformer 自回归推理。虽然参数少，但流程完整：prompt、decode step、logits 输出。
- `medium/87_speculative_decoding_verification`：speculative decoding 的 token 接受/拒绝。考批量序列控制流、概率比较和前缀接受长度。
- `hard/12_multi_head_attention`：完整 MHA。比基础 attention 多 head reshape、concat 和 output layout。
- `hard/53_casual_attention`：causal self-attention。核心是上三角 mask，不允许位置 i 看到未来 token。
- `hard/56_linear_attention`：linear attention 用 kernel feature map 改写 attention，避免 O(N^2)。考公式变换和前缀状态。
- `hard/59_sliding_window_attn`：每个 token 只看局部窗口。考窗口边界和减少无效 QK 计算。
- `hard/74_gpt2_block`：完整 GPT-2 decoder block。考 LayerNorm、QKV projection、causal MHA、output projection、FFN、residual、packed weights。
- `hard/93_llama_transformer_block`：完整 Llama block。比 GPT-2 多 RMSNorm、GQA、RoPE、SwiGLU，无 bias；是 LLM kernel 综合题。

### 6. 序列递推和 SSM 题考“如何打破串行依赖”

递推题看起来天然串行，但 GPU 版本通常要找到结合运算，把它改写成 scan。这个方向和 Mamba/SSM 很相关，也是近几年高性能序列模型 kernel 的重点。

重点能力：

- **associative operator**：把递推状态封装成可结合的 pair/tuple。
- **block scan + carry**：长序列需要分块扫描并传递块间状态。
- **batch 维并行**：B 和 L 的映射要平衡并行度和连续访存。
- **数值范围**：指数衰减、状态转移可能产生 underflow/overflow。

逐题更细：

- `medium/82_linear_recurrence`：`h[t] = a[t] * h[t-1] + x[t]`。可以表示成 affine transform 的前缀组合，是 scan 改写的经典例子。
- `medium/94_ssm_selective_scan`：Mamba-style selective scan。考 per-token 参数、状态维循环、skip connection，以及如何把 recurrence 分块并行。

### 7. 图、几何、模拟和聚类题考不规则并行

这类题的难点不是算术密度，而是不规则访存、分支发散、负载不均衡和迭代同步。GPU 上做图算法通常需要 frontier、bitmap、atomic 和多 kernel 迭代。

重点能力：

- **frontier 扩展**：BFS 每一层扩展都需要同步，通常是一层一个 kernel 或多阶段。
- **atomic 更新**：距离、label、centroid 累加都可能需要 atomic。
- **负载均衡**：不同点/节点的邻居数量不同，thread 分配会影响效率。
- **迭代收敛**：k-means、simulation 都需要多轮 kernel launch 和状态交换。

逐题更细：

- `medium/35_monte_carlo_integration`：样本函数值求平均再乘区间长度。考 reduction，也可作为随机采样类 workload 的简化版。
- `medium/38_nearest_neighbor`：每个点找最近的其他 3D 点。朴素 O(N^2)，考距离计算 tile 化和避免重复 global read。
- `hard/14_multi_agent_sim`：boids/flocking simulation。每个 agent 受邻居影响，考邻域搜索、分支和状态更新。
- `hard/20_kmeans_clustering`：每轮包含点到 centroid 距离、label assignment、centroid sum/count reduction。难点是 atomic 聚合和空 cluster。
- `hard/46_bfs_shortest_path`：网格 BFS。考 frontier、visited 标记、障碍判断和层级同步。
- `hard/73_all_pairs_shortest_paths`：Floyd-Warshall。三重循环中 k 是全局阶段，GPU 优化通常是 blocked Floyd-Warshall。

### 8. FFT 和频域算法考分阶段全局重排

FFT 题的核心不是复数乘加本身，而是 butterfly stage、bit reversal、跨阶段同步和内存重排。每个 stage 都依赖前一个 stage 的结果，因此通常需要多 kernel 或在单 block 内处理小规模。

重点能力：

- **复数布局**：interleaved real/imag 的地址计算。
- **butterfly**：每一层 pair 的索引和 twiddle factor。
- **同步边界**：block 内可 `__syncthreads()`，block 间只能多 kernel。
- **2D 分解**：2D FFT/DFT 通常按 row 再按 column，中间涉及转置或跨 stride 访问。

逐题更细：

- `medium/78_2d_fft`：2D complex signal 的频域变换。考 row-column decomposition、复数乘加和 stride 访问。
- `hard/39_Fast_Fourier_transform`：1D FFT。考 radix-2 butterfly、bit reversal、twiddle factor 和多 stage 调度。

## Medium/Hard 的难度差异

Medium 通常是在考一个相对独立的 GPU primitive：一个规约、一个 scan、一个 matmul、一个 attention 变体、一个 normalization。Hard 往往把多个 primitive 组合起来，或者引入全局同步/迭代/复杂数据依赖，比如排序、FFT、BFS、Floyd-Warshall、完整 transformer block。

更具体地说：

- **Medium 的目标**：能写出正确 kernel，并在一个主要瓶颈上做优化。例如 softmax 关注规约，GEMM 关注 tiling，RoPE 关注索引和融合。
- **Hard 的目标**：能设计一组 kernel 或一个复杂 fused kernel。比如 radix sort 需要 histogram/scan/scatter，多 head/transformer block 需要多个矩阵乘、normalization、attention 和 residual 串起来。
- **常见升级路径**：`reduction -> softmax -> attention -> transformer block`，`prefix sum -> stream compaction -> radix sort`，`GEMM -> quantized GEMM -> LLM block`，`stencil -> convolution -> depthwise causal conv`。

## 刷题时建议记录的维度

每道题做完后，不建议只记录“AC 了”。更有价值的是记录这些信息：

- 输入输出 shape 和 dtype。
- 主瓶颈是算力、带宽、同步、atomic 还是 launch 次数。
- thread/block/grid 如何映射到数据维度。
- 是否使用 shared memory，缓存的是 input tile、partial result 还是 histogram。
- 是否需要多 kernel；如果需要，kernel 间传递什么中间结果。
- 数值稳定策略是什么，比如 max trick、epsilon、FP32 accumulation。
- 对齐/coalescing 情况如何，是否存在 stride 访问。
- 哪些边界最容易错，比如 padding、causal mask、segment carry、packed INT4 高低 nibble。

## 类似的项目
- https://www.deep-ml.com/projects/flash-attention-in-cuda-from-scratch
