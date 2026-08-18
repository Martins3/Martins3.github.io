## GPU architecture
<!-- 00c23e83-273f-4c57-9244-6e436bde857e -->
```txt
• SM（Streaming Multiprocessor，流式多处理器）和 CUDA Core 是层级包含关系：
  SM 是 GPU 的基本计算单元模块，CUDA Core 是 SM 内部实际执行算术运算的标量处理单元。
  结构关系
      GPU
      ├── SM (多个)
      ├── CUDA Cores (多个)
      ├── Tensor Cores (部分架构)
      ├── Special Function Units (SFU)
      ├── Load/Store Units
      ├── Shared Memory / L1 Cache
      └── Warp Scheduler

  各自职责
   组件        职责
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   SM          线程调度（Warp 调度）、内存访问管理、共享内存管理、寄存器分配、上下文切换
   CUDA Core   执行具体的整数和浮点运算（如加、减、乘、FMA）
  数量比例（随架构变化）
  不同架构中，每个 SM 包含的 CUDA Core 数量不同：
  • Maxwell (GM200): 128 CUDA Cores / SM
  • Pascal (GP100): 64 CUDA Cores / SM
  • Volta/Turing: 64 FP32 CUDA Cores / SM
  • Ampere/Ada/Hopper: 128 FP32 CUDA Cores / SM
  简单类比
  • SM 类似于 CPU 中的一个核心（带有自己的调度器和缓存）
  • CUDA Core 类似于 CPU 核心内部的 ALU（算术逻辑单元）
  补充：现代架构的演变

  从 Volta 架构开始，NVIDIA 在 SM 中引入了 Tensor Core，专门用于矩阵乘累加（MMA）运算。在 Hopper 等最新架构中，Tensor Core 的重要
  性已超过 CUDA Core，但 CUDA Core 仍是执行标量运算和通用计算的基础单元。
```

## cuda 的 block
<!-- 0b06faa3-3de2-4cda-ab7e-b0d77c2e929d -->

block 中才可以同步，这就是基本的原理定义的:
```txt
  - Max threads per block : 1024
    一个 block 里最多放 1024 个线程。
    例如 <<<..., 256>>>、<<<..., dim3(32,8)>>> 都可以，因为总线程数没超过 1024。
  - Max block dim : (1024, 1024, 64)
    block 在 3 个维度上的单独上限。
    也就是：
      - blockDim.x <= 1024
      - blockDim.y <= 1024
      - blockDim.z <= 64
        但同时 x * y * z <= 1024 也必须满足。
  - Max blocks per SM : 32
    一个 SM 上最多能同时驻留 32 个 block。
    这里的“同时”还会受到寄存器、shared memory、线程总数等资源限制，32 只是理论上限。
  - Shared memory per block : 48.0 KiB
    每个 block 最多可使用 48 KiB 共享内存。
  - Registers per block : 65536
    每个 block 最多可占用这么多寄存器资源。

  最核心的理解是：

  block 是 CUDA 调度和同步的基本合作单位。
  同一个 block 内的线程：

  - 可以用 __shared__ 共享内存
  - 可以用 __syncthreads() 做同步
```



## cuda GPU memory hierarchy
<!-- d933c076-549f-4e0e-ae3f-3f593c84efd3 -->

为了快速建立印象，下表总结了各个核心组件的关键特性：

| 内存/缓存类型 | 物理位置 | 速度与延迟 | 容量 | 作用域与共享范围 |
| :--- | :--- | :--- | :--- | :--- |
| **寄存器 (Register)** | GPU芯片 (SM内部) | 最快，1个周期延迟 | 极小 (每个线程私有，通常255个) | 单个线程，私有 |
| **L1 数据缓存 (L1 Cache)** | GPU芯片 (SM内部) | 非常快，1~32个周期左右 | KB级别 (常与共享内存共享物理空间) | 单个SM内的所有线程 |
| **共享内存 (Shared Memory)** | GPU芯片 (SM内部) | 接近寄存器 | 通常在几十到一百多KB之间 | 单个线程块 (Block) 内的所有线程 |
| **常量缓存 (Constant Cache)** | GPU芯片 (SM内部) | 快 (命中时) | KB级别 | 所有线程，只读 |
| **L2 缓存 (L2 Cache)** | GPU芯片 (全体SM共享) | 中等，32~64个周期左右 | MB级别，是性能关键 | 所有SM共享 |
| **全局内存 (Global Memory)** | 显卡PCB (GPU芯片外) | 最慢，400~600个周期延迟 | GB级别，是数据的大本营 | 所有线程共享 |
| **Host Memory** | 主机主板 | 最慢 (需通过PCIe总线) | GB级别 | CPU与GPU间数据交换 |

1. 最接近计算的“极速梯队”：片上内存
*   **寄存器 (Register) - 线程的私有极速空间**
    作为内存金字塔的顶端，寄存器速度最快，用于存储线程的局部变量。
*   **共享内存 & L1 缓存 - "一块空间，两种用途"**
    共享内存是由程序员显式管理的高速可编程缓存，用于同一线程块内的数据交换和复用，常通过 `__shared__` 关键字编程使用。它与L1缓存共用物理存储，开发者可按需调整其分配比例。通过**合并访问（Coalesced Access）** 等技术可以最大化其效率，避免**共享内存的存储体冲突（Bank Conflict）**。

2. 承上启下的“中枢”：L2 缓存
L2缓存是所有流式多处理器（SM）共享的统一缓存，是片上与片外内存间的桥梁。在现代GPU中，增大L2缓存可以显著提升缓存命中率，从而有效降低对远距离显存的访问次数，提升能效表现。

3. 存储的“主力仓库”：全局内存
全局内存，即常说的“显存”，是GPU板卡上容量最大的内存，用于存储所有需要计算的数据。

4. 专用的“加速通道”：纹理内存与常量内存
两者都是全局内存中划分出的**只读**存储空间，但配备了专用片上缓存，来应对特定的访问模式。
*   **纹理内存 (Texture Memory)**：为具有**2D空间局部性**的访问而优化，如图像处理。
*   **常量内存 (Constant Memory)**：当线程束（Warp）内所有线程都读取同一地址时效率最高，适合存储物理常数等。

5. 数据交换的“外部公路”：Host Memory
这是CPU的系统内存。GPU需通过PCIe总线进行数据交换，速度是瓶颈，因此性能优化的核心原则之一就是**尽量减少CPU与GPU之间的数据拷贝**。

架构特色：NVIDIA 与 AMD 的方案对比
两大主要厂商在内存架构设计上各有侧重：
*   **NVIDIA 的方案**：近年来的设计趋势是**大幅增加L2缓存容量**（如在Ada Lovelace架构中增大16倍），以此提升命中率，减少对全局内存的访问。
*   **AMD 的方案**：AMD在其RDNA/CDNA架构中引入了**Infinity Cache**作为“最后一级缓存”。它可被视为一个更大但稍慢的、全芯片共享的缓存层，旨在将更多数据留在片上，降低对高延迟显存访问的需求。

所以，差不多理解，L1 和 share memory ，其实就是 SM 内的，然后 L2 是外部的。

## CPU SIMT vs SIMD
<!-- 973f8da0-7845-4379-9c9c-6bb20dc65d0e -->

**SIMT（Single-Instruction, Multiple-Thread）**
- SM 以 **warp**（32 个线程）为单位进行调度、执行。
- 一个 warp 内的线程共享同一条指令流，但各自拥有独立的 PC（Program Counter）和寄存器状态，因此可以独立分支。
- 当 warp 内发生分支发散（branch divergence）时，SM 会串行执行每条被采用的分支路径，暂时禁用不在该路径上的线程。
- 与 SIMD 的区别：SIMD 将向量宽度暴露给软件，而 SIMT 以单线程语义编程，由硬件隐式管理向量执行。

Warp 大小 32 和 SM 内 CUDA Core 数量没有直接的因果关系，但 SM 的微观架构是围绕 32 来优化的。32 是 SIMT 执行模型的宽度常数
  ，SM 内的各种执行单元数量则是这个常数的倍数，以保证一个 warp 的 32 个线程能被均匀、高效地执行。

**硬件多线程（Hardware Multithreading）**
- SM 将线程块划分为 warp，warp 的组成方式固定：按线程 ID 连续递增排列，每 32 个线程为一个 warp。
- warp 总数公式：`ceil(T / 32, 1)`，其中 T 为块内线程数。
- warp 的上下文（PC、寄存器）全部保存在芯片上，因此 warp 切换零开销。
- 每个 SM 有固定的寄存器共享内存总量，这些资源决定了可同时驻留的 block 和 warp 数量。

**Warp 调度与执行流水线**
- SM 的 warp scheduler 在每个指令发射周期选择一个就绪 warp，将其指令发射给活跃线程。
- GPU 没有分支预测和推测执行，指令按顺序发射。
- 隐藏内存延迟的关键在于**同时驻留足够多的 warp**：当某个 warp 等待内存操作时，scheduler 立即切换到其他就绪 warp。
- 寄存器和共享内存是 SM 上的稀缺资源，增加其中任意一个都会减少可同时驻留的 warp 数量，进而降低 latency hiding 能力。

**SIMT 与 Warp Divergence**
- **分支发散只发生在 warp 内部**，不同 warp 之间不会相互影响。
- 设计高性能代码时，应尽量减少 warp 内的条件分支；但设计正确性时可以忽略 SIMT 细节。
- 非原子指令若被 warp 内多个线程写入同一 global/shared 地址，最终由哪个线程完成写入是**未定义行为**（UB），且写入次数随架构而异。
- 原子指令若被 warp 内多个线程操作同一 global 地址，每次 RMW 都会发生且行化，但**顺序未定义**。




(所以
1. 也就是 SIMT 中，独立的 PC 和 register 状态 ，所以可以部分禁用
2. 硬件保存上下文，所以没有上下文，
)

## wrap 就绪的条件
warp ready 到底是怎么判定的，尤其是 load 指令回来时是不是要等 32 个lane 全回来？

不是“等 32 个 scheduler 资源”，而是“等这条 warp load 对应的所有活跃 lane 的结果都准备好”。

更准确一点说，warp ready 往往是由 scoreboard/依赖检查 判定的：

1. 一个 warp 发出一条 load
2. 硬件把这 32 个 lane 的地址按访问模式做合并，拆成若干个 memory
   transaction
3. 这条 load 的目标寄存器会被标记成“pending”
4. 只有当这条 load 涉及的 所有活跃 lane
   的数据都返回并写回后这个目标寄存器才算 ready
5. 此时，依赖这个寄存器的后续指令才可以再发射

所以你问的核心答案是：

• 如果 32 个 lane 都是 active，那么通常要等这 32 个 lane 对应的数据都回来
• 如果只有部分 lane active，就只等这些 active lane
• 不是按“32 个线程各拿一个独立调度名额”来等
• 也不是“只回来 1 个 lane 就能继续执行依赖这次 load 的下一条指令”

warp ready 对 load 来说，本质上不是等 32 个独立调度资源，而是等这条
warp 指令涉及的所有活跃 lane 的数据都准备好；只要还有一个活跃 lane
没回来，依赖它的后续指令通常就不能发。

(也就是，需要等需要执行的资源全部都回来)

## green context 其实就是 SM 独占啊

它关联的硬件资源，本质上是 GPU 前端 command processor / Hyper-Q / channel
这类有限的“工作提交通道状态”。不是 SM，但会影响工作能否及时到达 SM。

## cuda `__syncthreads` 和 `__syncwarp` 的关系是什么?
<!-- 8e176800-26b6-4401-bfa7-0546a486664d -->

核心区别
- `__syncthreads()`：同步整个 block 的所有线程（本代码中是 128 个）
- `__syncwarp()`：只同步一个 warp 内的 32 个线程

**唯一可能使用 `__syncwarp()` 的场景**
如果 block 大小 <= 32（即只有一个 warp），
且不需要跨 warp 的 shared memory 协作，才可以考虑替换。
但本代码的 block size 是 128，不满足这个条件。

## launch kernel 的集中方法

- cudaLaunchCooperativeKernel : 如果想要跨 block 同步
- cudaLaunchKernelEx : 如果添加 cluster 属性的话
- 两种 launch 的方法，其实就是 share memory
	- group_metadata_kernel : 最普通的
	- reduce_with_cooperative_groups : 带上 share memory 的

而且 /home/martins3/data/vn/gpu/cuda/tutorial/04-cluster-launch-control/
中的 cluster 明显变成了不是概念啊，这是两个东西啊
gpu/cuda/tutorial/04-cluster-launch-control/

## 常见问题

1. 如何计算 exp
```txt
  float  result_f = expf(x);   // float 版本
  double result_d = exp(x);    // double 版本
```

### cuda : 每次 share memory 都是需要将 memory 全部都拷贝进来才可以吗?
不是，只是大家经常这样的优化

## cg::reduce
这个东西只能用于 tile ，不可以用于 block 级别

thread_block_tile 的参数是可以大于 32 的:
```txt
  cg::thread_block_tile<64> tile32 = cg::tiled_partition<64>(cta);
```
