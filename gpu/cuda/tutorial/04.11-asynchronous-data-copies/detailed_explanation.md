## cuda asynchronous data copies
<!-- e1f718eb-2c19-49d9-9238-a956464930b1 -->

这份文档基于 NVIDIA 官方文档 `4.11 Asynchronous Data Copies` 整理，但目标不是逐段翻译，而是把这章里最容易让人混淆的概念拆开讲清楚。

**在 kernel 内部，如何把数据在 GPU 内存层级之间异步搬运，并尽量让“搬运”和“计算”重叠。**

## 一、先纠正一个最常见的误解

很多人看到 “async copies” 会先想到：

- `cudaMemcpyAsync`
- Host 提交异步拷贝
- DMA engine 在 CPU/GPU 之间搬数据

但本章说的主要不是这个。

本章关心的是：

- 代码已经在 GPU 上运行
- 线程或线程块想把数据从 `global memory` 搬到 `shared memory`
- 或者从 `shared memory` 写回 `global memory`
- 或者在 cluster 内部搬到 distributed shared memory

也就是说，它讲的是 **device-side asynchronous data movement**。

所以一定要先分清两层：

### Host-side 异步拷贝

- API：`cudaMemcpyAsync`
- 方向：通常是 Host <-> Device
- 发起者：CPU
- 完成者：DMA / copy engine

### Device-side 异步拷贝

- API：`cuda::memcpy_async`、`cooperative_groups::memcpy_async`、`__pipeline_memcpy_async`、`cuda::ptx::cp_async_bulk` 等
- 方向：通常是 `global -> shared`，也可能是 `shared -> global` 或 cluster 内部路径
- 发起者：GPU 线程 / 线程块
- 完成者：GPU 内部异步数据通路（例如 `LDGSTS`、`TMA`、`STAS`）


## 二、为什么需要 device-side async copy

在很多高性能 kernel 里，真正的计算并不复杂，难的是喂数据。

典型流程是：

1. 从 `global memory` 读数据
2. 放到 `shared memory`
3. 所有线程在 `shared memory` 上重复使用这些数据

传统写法通常是：

```cpp
float x = global[idx];
shared[tid] = x;
__syncthreads();
// 然后再消费 shared 中的数据
```

这有几个问题：

### 1. 数据要先经过寄存器

`global -> register -> shared`

这样会增加寄存器压力。寄存器用量变大，可能降低 occupancy。

### 2. 搬运和计算难以重叠

线程通常要等 load/store 完成后，才能继续后面的工作。
这会让 global memory 延迟更明显地暴露出来。

### 3. 条件分支下容易生成不理想的指令序列

比如 stencil 里：

- 一部分线程加载 left halo
- 一部分线程加载 center
- 一部分线程加载 right halo

同步写法在编译后，可能形成交错的 `LDG` / `STS` 序列，而不是更理想的批量 in-flight 搬运。

### 4. 多维 tile 搬运的地址计算很复杂

对于卷积、矩阵乘、attention 这类多维 tile 访问，手写地址和步长很容易出错，而且重复劳动很多。

所以这章引入了三类机制：

- `LDGSTS`：小颗粒度、元素级的 `global -> shared`
- `TMA`：大颗粒度、bulk / tensor 级的拷贝
- `STAS`：`register -> distributed shared memory`

## 三、这章的三大机制到底分别是什么

### 1. LDGSTS

这是最适合先理解的一类。

可以把它看成：

**面向小颗粒度元素的 `global -> shared` 异步拷贝通路。**

典型特点：

- 从 `global memory` 直接搬到 `shared memory`
- 适合 4 / 8 / 16 字节的小块数据
- 常见场景是 tile loading、halo loading、prefetch
- Compute Capability 要求：`sm_80+`

它的价值在于：

- 减少 `global -> register -> shared` 这种中转
- 让多次内存搬运可以先发起，再统一等待

**支持的方向**
`LDGSTS` 只支持： `global -> shared::cta`
不支持： - `shared -> global` 和 `register -> shared::cluster`

这点要记住，因为很多人容易把它和 TMA 混在一起。

**拷贝粒度与对齐**
文档明确说支持：

- 4 bytes
- 8 bytes
- 16 bytes

并且：

- 源地址和目标地址必须按对应粒度对齐
- 最佳性能通常希望两边都尽量有 128B 对齐

还有一个很容易忽略的点：

- 4B / 8B 拷贝走 `L1 ACCESS`
- 16B 拷贝可以启用 `L1 BYPASS`

也就是说，16B 模式可以减少对 L1 的污染。


### 2. TMA

`TMA` 是 Hopper (`sm_90+`) 引入的更强大的数据搬运硬件。

可以把它理解成：

**面向“大块、规则、多维”数据传输的专用搬运引擎。**

相比 `LDGSTS`，它的几个关键升级是：

- 不只支持小元素，而是支持 bulk copy
- 支持一维连续数组
- 更重要的是支持多维 tensor tile
- 能把复杂地址计算 offload 到硬件

#### TMA 主要解决什么问题

在矩阵乘、卷积、attention 里，经常要把一个二维或三维 tile 从 global 搬到 shared。
如果手工写：

- 每个线程算自己的 global 地址
- 每个线程算自己的 shared 地址
- 再处理边界和 stride

会很复杂，也容易错。

TMA 的思路是：

- 先在 host 端或 device 端准备一个 `Tensor Map`
- 这个 map 描述张量布局、维度、步长、tile 形状等
- 然后设备端只需要说“把这个 tile 搬过来”

#### TMA 支持的方向

比 `LDGSTS` 更丰富：

- `global -> shared::cta`
- `shared::cta -> global`
- `global -> shared::cluster`
- `shared::cta -> shared::cluster`

此外在 cluster 场景里还可以做 multicast。

#### TMA 有两种主要风格

1. Bulk-Asynchronous Copy
针对一维连续数组。
特点：
- 不需要 Tensor Map
- 给指针和大小就行

2. Bulk-Tensor Asynchronous Copy
针对多维数组 / 张量。

特点：
- 需要 `Tensor Map`
- 最多支持到 5 维
- 适合 tile 化加载

### 3. STAS

`STAS` 可以看成这章里最特殊的一类。

它不是 `global -> shared`，而是：

**`register -> distributed shared memory (shared::cluster)` 的异步写入。**

所以它更多用于：

- thread block cluster 内部通信
- 一个 block 把结果写到另一个 block 的 distributed shared memory

这是 cluster 级编程场景才会真正用到的能力，门槛比前两者更高。

---

## 四、异步拷贝的多层 API

这章容易乱的一个原因是：**同一类硬件能力，CUDA 暴露了多层 API。**

以 `LDGSTS` 为例，大致有三层。

### 1. `cuda::memcpy_async`

这是最高层、最现代 C++ 风格的接口。

优点：

- 写法更自然
- 能和 `cuda::barrier`、`cuda::pipeline` 很好配合
- 更抽象，表达力强

代价：

- 不是任何情况下都保证一定走最底层那条硬件路径

### 2. `cooperative_groups::memcpy_async`

这是 cooperative groups 风格的接口。

优点：

- 很适合集体拷贝
- 用 `cg::wait(block)` 同步，模型简单

缺点：

- 官方文档特别提到，在一些例子里它效率会差些
- 一个关键原因是：它会更倾向于“每次 copy 立即 commit”，不利于把多个 copy 批量合并后一次提交

### 3. `__pipeline_memcpy_async`

这是底层 primitive。

优点：

- 最直接
- 控制最强
- 你可以显式 `commit` 和 `wait`
- 官方文档明确说它能确保使用 `LDGSTS`

缺点：

- 代码更啰嗦
- 更容易写错同步

所以不是“哪个 API 更先进”，而是：

- 越高层越好写
- 越低层越可控

---

## 五、最关键的语义：异步 copy 发起了，不等于数据已经能用了

这是整章最重要的一句话。

无论你用：

- `cuda::memcpy_async`
- `cg::memcpy_async`
- `__pipeline_memcpy_async`
- `cp_async_bulk`

本质上都是：

**先发起传输，再在后面某个点等待它真正完成。**

所以常见模式一定是：

1. 发起若干个 async copy
2. commit 或绑定到 barrier / pipeline
3. wait
4. 如有需要，再 `__syncthreads()`
5. 然后才能读 `shared memory`

如果你跳过第 4/5 步，就很容易读到尚未完成的内容。

---

## 六、为什么等完 async copy 之后，很多时候还要 `__syncthreads()`

这是很多人最不明白的地方。

文档里的关键提醒是：

**默认情况下，每个线程只等待它自己发起的异步拷贝完成。**

这意味着：

- 线程 A 只知道自己搬的数据完成了
- 线程 B 只知道自己搬的数据完成了
- 但如果后续 A 要读 B 搬的数据，仅仅“各自 wait 完成”还不够

所以：

### 如果每个线程只消费自己拷的数据

有时可以不需要额外 `__syncthreads()`。

### 如果数据会被 block 内其他线程共享消费

那通常还需要：

```cpp
wait_for_copy_completion();
__syncthreads();
```

理由是：

- `wait` 解决“我的 copy 完没完成”
- `__syncthreads()` 解决“整个 block 是否都到了可以安全消费共享缓冲区的时刻”

这是 barrier/pipeline 和 `__syncthreads()` 分工不同导致的。

---

## 七、怎么理解 barrier 和 pipeline

这章另一个容易绕的点是：为什么又有 barrier，又有 pipeline？

可以这样区分：

## 1. barrier 更像“完成通知 + 相位同步”

比如 `cuda::barrier`：

- 跟踪有多少线程到达
- 还跟踪绑定到这个 phase 的异步拷贝有没有完成

所以它特别适合这种情况：

- 一批线程共同参与
- 希望在“大家都到齐 + 相关异步操作也完成”后，再进入下一阶段

这就是 stencil 例子里 barrier 很自然的原因。

## 2. pipeline 更像“多阶段流水线管理器”

pipeline 适合：

- 有多个 stage
- 希望当前 stage 在算的时候，下一 stage 的数据已经开始预取
- 需要 producer / consumer 明确分工

比如常见的多阶段预取：

- stage 0 正在算
- stage 1 正在从 global 往 shared 搬
- stage 2 已经准备好待用

pipeline 会提供：

- `producer_acquire`
- `producer_commit`
- `consumer_wait`
- `consumer_release`

把这些阶段关系表达出来。

一句话说：

- `barrier` 更像“这一批什么时候都完成了”
- `pipeline` 更像“多批数据怎样轮转前进”

---

## 八、LDGSTS 的三个典型使用场景

官方文档其实主要拿三个例子来建立直觉。

## 1. 条件代码中的批量加载

这是 stencil halo loading 的例子。

核心问题是：

- 左 halo、中心、右 halo 由不同线程条件分支加载
- 同步代码可能让编译器生成不够理想的 load/store 顺序

改成 async copy 后，好处是：

- 可以把多个 global->shared 的传输先都发出去
- 所有 load 能更早 in-flight
- 减少寄存器中转

这里的重点不只是“异步”，而是**更容易形成更好的内存发射模式**。

---

## 2. 预取（Prefetching）

这是这章最重要的性能模式之一。

本质思想是：

- 当前 batch 正在算
- 同时把未来 batch 的数据提前从 global 搬到 shared

这就是经典的：

**overlap data movement with compute**

如果你用多 stage pipeline，那么典型会是：

- 先把前 `num_stages` 个 batch 填满
- 然后循环：
  - 等当前需要的 stage ready
  - 计算当前 stage
  - 释放当前 stage
  - 立刻用它去预取更远的未来 stage

这时共享内存就是一个循环缓冲区。

---

## 3. Producer-Consumer Through Warp Specialization

这个例子更进阶。

思路是：

- 一个 warp 专门负责搬数据（producer）
- 其他 warp 专门负责计算（consumer）

再配合双缓冲：

- buffer 0 正在被 consumer 用
- buffer 1 正在被 producer 填
- 下一轮交换

这样可以让搬运和计算更明确并行。

这类模式在高性能 GEMM、卷积、attention kernel 中非常常见。

---

## 九、TMA 为什么被单独拿出来讲

因为它和 `LDGSTS` 最大的不同，不只是“更快”，而是：

**它把复杂地址计算也 offload 掉了。**

对于多维张量，真正烦的往往不是 copy 本身，而是：

- 这个 tile 的 global 起点在哪
- 每一维 stride 是多少
- shared 内的布局是什么
- 是否要 swizzle

TMA 的 `Tensor Map` 就是在解决这个问题。

你可以把 `Tensor Map` 理解成：

**“给硬件看的张量布局说明书”。**

有了它以后，设备端不需要每个线程都自己算一大堆地址。

---

## 十、为什么 TMA 读和写的完成机制不一样

这也是非常关键的一点。

### `global -> shared`

这时目标是 `shared memory`，而整个 block 里的线程都可能要读这块 shared。
所以很自然，完成机制常常是：

- shared memory barrier

意思是：

- 当 barrier 满足条件后，block 内线程可以把 shared 当成可读

### `shared -> global`

这时是把 shared 里的东西写出去。
官方文档强调：**等待完成的通常只能是发起线程。**

所以这里使用的是：

- bulk async-group

这是更接近“发起者自己跟踪这组 bulk copy 有没有真正完成”的机制。

这就是为什么 TMA 的读路径和写路径，在同步方式上明显不同。

---

## 十一、TMA 的几个硬要求，必须强记

如果只记几个硬门槛，至少记这些：

### 1. TMA 主要是 `sm_90+`

也就是 Hopper 及以上。

### 2. 一维 bulk copy 常要求 16B 对齐

尤其是：

- source / destination 16-byte aligned
- size 是 16 的倍数

### 3. 多维 tensor copy 的 shared memory buffer 往往要求 128B 对齐

这点非常容易忘。

### 4. `cuda::memcpy_async` 不是所有情况下都强制走 TMA

文档明确说：

- 满足 16B 对齐和 size 为 16 的倍数时，它会用 TMA
- 否则可能回退成同步 copy

而：

- `cuda::device::memcpy_async_tx`
- `cuda::ptx::cp_async_bulk`

是“你必须满足条件，否则行为未定义”的那类接口。

---

## 十二、STAS 什么时候值得关心

如果你还没写到 cluster-level cooperative kernel，先不用在 `STAS` 上花太多时间。

它更像是：

- thread block cluster 内部的高级通信能力
- 用于把寄存器值直接异步送到 remote shared memory

适用范围比 `LDGSTS` 和 `TMA` 小很多。

对大多数刚读这一章的人来说，优先顺序通常应该是：

1. 先理解 `LDGSTS`
2. 再理解 `pipeline / barrier`
3. 然后再看 `TMA`
4. 最后才看 `STAS`

---

## 十三、这章最重要的实践结论

如果把整章压缩成几条真正有用的结论，我会给这几条：

### 1. 小颗粒度 `global -> shared`，优先先理解 `LDGSTS`

它是最常见、也是最接近日常 tile loading 的路径。

### 2. 多阶段预取时，`pipeline` 是最核心的抽象

因为重点不是“发起了一次异步 copy”，而是“怎样维持一个稳定流动的多 stage 缓冲体系”。

### 3. `wait` 之后是否还要 `__syncthreads()`，取决于数据是不是要被其他线程共享消费

这是这章里最容易写错、也最容易误判的一点。

### 4. `TMA` 的真正价值不只是吞吐，更是把多维地址计算和 tile 搬运交给硬件

这在 Hopper 之后尤其重要。

### 5. 高层 API 更好写，低层 primitive 更可控

一般规律是：

- 先用高层 API 建立正确性
- 对热点路径再考虑是否下探到 primitive / PTX 层

---

## 十四、什么时候该选哪种机制

可以粗略这样选：

### 选 `LDGSTS`

当你满足这些特征：

- `sm_80+`
- 搬的是小元素
- 主要是 `global -> shared`
- 你在做 tile loading / halo loading / prefetch

### 选 `TMA`

当你满足这些特征：

- `sm_90+`
- 数据块比较大
- 是一维大块搬运，或者尤其是多维 tile 搬运
- 手写地址和 stride 已经很复杂

### 选 `STAS`

当你满足这些特征：

- 在写 cluster 内部通信
- 需要 `register -> distributed shared memory`

---

## 十五、最常见的坑

最后把这章最常见的坑单独列一下。

### 1. 把 `cudaMemcpyAsync` 和本章的 async copy 混为一谈

一个是 host-side，一个是 device-side。

### 2. 发起了 async copy，就以为数据已经能用了

一定要有：

- barrier
- pipeline wait
- 或 bulk async-group wait

### 3. 忘记 block 级 `__syncthreads()`

如果数据会被其他线程共享消费，仅仅线程自己 wait 完成还不够。

### 4. 对齐条件不满足还强行使用底层接口

特别是 TMA 相关接口，很多情况下不是“慢一点”，而是直接 Undefined Behavior。

### 5. 以为高层 API 一定会映射到底层异步硬件路径

文档明确告诉你，有些高层接口在不满足条件时会回退。

### 6. 忽略 commit 的语义

特别是低层 primitive 和 pipeline 风格代码里：

- 你不是发起完 copy 就完了
- 还要正确 commit，才能真正形成 batch 并进入流水线

---

## 十六、一句话总结

如果你只想记一句最本质的话：

**Asynchronous Data Copies 这一章讲的是：如何在 GPU 内部把数据搬运从“线程同步执行的一部分”变成“可被显式调度、分阶段等待、并尽量与计算重叠的独立活动”。**

而三条主线分别是：

- `LDGSTS`：小颗粒度 `global -> shared`
- `TMA`：大颗粒度 / 多维 tile 的 bulk copy
- `STAS`：`register -> distributed shared memory`

如果你接下来还想继续深入，最建议的阅读顺序是：

1. 先看 `LDGSTS`
2. 再看 `pipeline` 和 `barrier`
3. 然后看 `TMA`
4. 最后再看 `STAS`

这样理解成本最低。
