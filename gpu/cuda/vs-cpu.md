# 和 CPU 对比

1. SIMT
2. 硬件上下文切换
	- 可以测试下，上下文切换的含义
3. 没有分支预测器
4. 需要有类似 TLB 的机制
	- 但是不会有类似 tlb remote soot 的操作吧
5. 他们都是有 leetcode 的:
	- https://leetgpu.com/challenges/vector-addition
6. 微架构上的不同
	- 都是需要 ROB 这样的，实现乱序提交?
7. 都是有指令集，类似 ptx 的指令
	- ptx 指令让 GPU 的指令迭代容易很多了
8. 对于 L2 cache 和 share memory 更强的控制
9. 同样有虚拟地址空间的概念，但是考虑的方向不同
	- 不然数组越界岂不是很难检测到?
10. 似乎 GPU 更加重视浮点运算，而 CPU 更加重视顶点运算
	- 经典的代表就是内核中，完全没有浮点计算的
	- 才发现类似指数运算，GPU 会有更多的支持吧

11. cuda 中存在 nvme 类似的队列机制，体现应该就是通过 steam 机制的

12. cpu 真的考虑了很多通用问题，而 GPU 真的就完全考虑一个问题就结束了。

从 gpu/cuda/tutorial/04-cooperative-groups/cooperative_groups_reduce.cu
看，似乎 GPU 的访问存储不在乎 cache 局部性?

例如这种 stride 的访问
这里 n 是元素总数
```cuda
  float sum = 0.0f;
  for (size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
       i < n; i += static_cast<size_t>(gridDim.x) * blockDim.x) {
    sum += input[i];
  }
```

12. 经典对比 blas 和 cublas 的实现差别是什么

13. 内存分配 vs 显存分配
	- 内存分配，无论是用户态还是内核态，都是相当复杂，考虑的问题很多，其中由于出现虚拟地址空间，
	极大的方便了各种操作，也引入很大的复杂度
	- 显存分配:

## 扩展一下

vs human : 也是如此，人的并行

## GPU 编程也是图灵完备的
是的(虽然我不知道图灵完备是啥意思)
这里的讨论非常有意思:
https://www.reddit.com/r/hardware/comments/184zeos/what_can_a_cpu_core_do_that_a_gpu_core_cant_do

可以访存，可以访问存储，所以 DMA 和中断的支持有点痛苦
但是不是理论的限制，而是没有必要这么做，
可以把所有的显存做内存用，然后 GPU 集成一个 interrupt controller

然后给 nvme DMA 请求和接受 nvme 中断

## 从一个 high level 来说

1. 加快经常性事件
2. core 之间的同步问题
3. 预取，async 提交
4. ease of use (更好编译器)


## 似乎 cache 局部性的思考也是不一样的
关于 cache 局部性：
  grid-stride loop 的访问模式是跨步的（strided），
  对于 L2 cache 和合并内存访问（coalesced access）是友好的，
  因为相邻线程访问相邻地址。但相比 block 连续处理一块数据，
  L1 cache 重用率稍低。。

由于 GPU 是重点，所以 cpu 端的代码就是 python 写的


## 2026-05-25 nv 发布自己的 CPU ，再次证明 cpu 和 GPU 设计是同一家
- https://www.nvidia.cn/data-center/grace-cpu/


## 异构计算

gpu direct memory

hmm 的管理:
Documentation/translations/zh_CN/mm/hmm.rst

zero copy
用户态驱动?

dma buf 到底有什么关系?

写一个 GPU 和 CPU 的对比?
1. GPU 体系结构的对比是什么
2. GPU 存在类似 perf book 类似的这种分析吗?
4. 继续看看 GPU 驱动都做什么的? (为什么 nova 驱动可以那么简单啊)
	- 不太容易啊

参考一下这个东西:
https://zhuanlan.zhihu.com/p/2024418152105214167 : ai 时代的 os

## GPU scheduler
这个 cuda 应该不会去用吧，或者说，
即便是我完全关闭掉 drm 子系统，也是每关系的吧。

https://zhuanlan.zhihu.com/p/641331417

到时候看下这个:
```txt
struct drm_gpu_scheduler
struct drm_sched_rq
```

CONFIG_DRM_SCHED 原来是关联到这个文件哦
https://zhuanlan.zhihu.com/p/641331417

到时候看下这个
```txt
struct drm_gpu_scheduler
struct drm_sched_rq
```

CONFIG_DRM_SCHED 原来是关联到这个文件哦

## What Every Developer Should Know About GPU Computing
https://blog.codingconfessions.com/p/gpu-computing
https://news.ycombinator.com/item?id=42042016


> A GPU consists of an array of streaming multiprocessors (SM). Each of these SMs in turn consists of several streaming processors or cores or threads. For instance, the Nvidia H100 GPU has 132 SMs with 64 cores per SM, totalling a whopping 8448 cores.
>
> Apart from these, each SM also has several functional units or other accelerated compute units such as tensor cores, or ray tracing units to serve specific compute demands of the workload that the GPU caters to.

SM : Streaming Multiprocessor

## bandwidth 很高，但是延迟也很高

解决办法，async

例如 TMA

## GPU 为了解决并行问题

并行的关键在于如何互相同步
1. SM 内同步
2. wrap 内同步 (一个 tile 内部是可以 reduce)
