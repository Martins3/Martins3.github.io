# AI Infra 到底在做什么
<!-- 7e1f0a4d-7799-4271-b2f3-52936dca1cb1 -->

## 限定范围
1. 不去考虑算法上的内容
[Do transformers need three projections? Systematic study of QKV variants](https://news.ycombinator.com/item?id=48405931)
2. 不考虑接入 openAI 的 api ，然后进一步开发应用的部分

## 计算机的几个基本思路

1. 存储，计算，网络
2. 加速经常性事件
3. 并行:
	- Instruction-Level Parallelism and Its Exploitation
	- Data-Level Parallelism in Vector, SIMD, and GPU Architectures
	- Thread-Level Parallelism
	- Warehouse-Scale Computers to Exploit Request-Level and Data-Level Parallelism
4. 带宽和延迟



## 基本工作原理

- https://bbycroft.net/llm
- https://mlu-explain.github.io/neural-networks/
- https://playground.tensorflow.org/

简单来说
1. 收集全人类所有的知识
2. 让权重矩阵吸收这些知识 (训练)
3. 从权重矩阵中获取到你的答案 (推理)

1. 训练框架
	- pytorch / jax
2. 推理框架
	- sglang
	- vllm

## 核心中的核心

[Attention Is All You Need](https://arxiv.org/abs/1706.03762) 中的

```typ
$ "Attention"(Q, K, V) = softmax(frac(Q K^T, sqrt(d_k))) V $
```

## 计算机体系结构的重心转移
如果只要做一件事情，而且要做的绝对好，那么该如何办?

1. 编译器
	- mlir
	- tvm
	- cutile
	- triton
2. 计算
	- GPU
3. 网络
	- nccl
	- nvlink
	- cxl
	- rdma
4. 存储
	- mooncacke
	- 3fs

### 编译器
细聊:
![](gpu/cuda/vs-cpu.md)

### 计算

### 网络

### 存储
https://www.zhihu.com/question/1956876400624669581/answer/1958354775042139942

## 调研结果
### 资源合集


- https://github.com/gpu-mode : 资源集合
- https://github.com/cuda-mode/lectures

- https://github.com/CalvinXKY/InfraTech : 文章合集
- https://github.com/Infrasys-AI/AISystem : 很久不更新了
- https://github.com/wdndev/llm_interview_note : 主要和推理相关

- https://github.com/jinbooooom/ai-infra-hpc : 主要将网络多一点
- https://github.com/caomaolufei/AIInfraGuide : pytorch 分布式训练?

- https://github.com/Jokeren/Awesome-GPU : 内容不多

- https://github.com/rasbt/LLMs-from-scratch : 一本书

### 经典项目
- https://github.com/dao-ailab/flash-attention
- https://github.com/deepseek-ai/DeepGEMM.git
- https://github.com/deepseek-ai/FlashMLA


<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
