# cuda arch
<!-- a40f3b4b-8d86-49b4-9351-1db1fd754981 -->

一个拥有几千个 CUDA Core 的超级大核，总体来说，
SM 就是相当于 CPU 的一个 core ，cuda core 相当于是其中的 CPU 的执行

在一个 SM 中同步的代价很低，但是在不同的 SM 中同步代价高。

同步的想一想这个问题，就是 SM 内部共享 register file 和 L1 cache ，其中可以编程的
部分就是 share memory 就可以了。

https://developer.nvidia.com/blog/cuda-refresher-cuda-programming-model/

当认为发送到 share memory 中的时候，可以自动的选择 warp

所以，定义上有:
- 一个 block 只能驻留在一个 SM 上，不能拆到多个 SM。
- block 内部可以同步，但是不同的 block 之间同步就是很难的事情了

进一步的，由于多个 SM 合并到一起，所以就是需要:
thread block cluster 功能
```txt
GPU
└── GPC（多个）
    ├── Raster Engine
    └── TPC（多个）
        ├── SM
        │   ├── CUDA Core
        │   ├── Tensor Core
        │   └── RT Core
        └── Texture Unit
```

## cuda : SM 中存在 ROB 这样的结构吗?

严格说，**CUDA Core 内部一般不存在 CPU 那种 ROB（Reorder Buffer，重排序缓冲区）**。

真正负责取指、依赖判断和调度的是 SM 中的 warp scheduler、scoreboard 等结构。

### CPU 与 GPU 的区别

现代高性能 CPU 通常采用乱序执行：

```text
指令流
 → 解码
 → 重命名
 → 放入调度窗口
 → 乱序执行
 → ROB 按程序顺序提交
```

ROB 主要负责：

- 保存尚未提交的指令状态
- 保证指令按程序顺序退休
- 支持精确异常
- 分支预测错误时恢复状态
- 配合寄存器重命名进行乱序执行

而 NVIDIA GPU 大致是：

```text
大量驻留 warp
 → scoreboard 判断哪些 warp 已准备好
 → warp scheduler 选择一个 ready warp
 → 将下一条指令发往相应执行单元
```

当某个 warp 因内存访问或数据依赖停顿时，
调度器通常不是深入挖掘该 warp 后面的独立指令，
而是切换到另一个已经 ready 的 warp。

主要依赖 **多线程并发隐藏延迟，而不是大型乱序执行窗口隐藏延迟。**

### Scoreboard 不等于 ROB

SM 内通常有 scoreboard，用来记录 warp 指令的操作数是否已经就绪。例如：

```cpp
x = global_memory[i];  // 长延迟
y = x + 1;             // 依赖 x，暂时不能执行
```

scoreboard 会发现第二条指令依赖尚未返回的 `x`，于是阻止它发射。调度器转而选择其他 ready warp。

但 scoreboard 通常不承担 ROB 的这些职责：

* 不保存大量投机执行指令
* 不负责按序退休
* 不负责完整的精确异常恢复
* 不相当于 CPU 的重排序窗口

### 是否完全没有指令级并行？

也不能简单理解为“GPU 完全顺序执行”。

NVIDIA 没有公开所有微架构细节；
不同架构也可能具有有限的流水化、双发射、独立执行管线和一定程度的指令重叠。
例如两个没有依赖的指令：

```cpp
a = b + c;       // FP32
p = q & r;       // INT32
```

在某些架构和条件下可能被重叠或双发射到不同执行管线。不过，这不等价于 CPU 那种依靠寄存器重命名、乱序窗口和 ROB 的通用乱序执行。

还要区分层次：

| 层次           | 作用                     | 类似 CPU ROB 吗？                |
| -------------- | ------------------------ | -------------------------------- |
| CUDA Core      | 执行 FP/INT 运算         | 否                               |
| Warp Scheduler | 选择 ready warp 发射     | 否                               |
| Scoreboard     | 跟踪寄存器与指令依赖     | 部分功能相关，但不是 ROB         |
| SM 执行管线    | 流水化、可能双发射       | 不代表存在 ROB                   |
| 内存系统       | 合并、排队、返回次序处理 | 可能发生事务重排，但不是指令 ROB |

最准确的结论是：

> CUDA Core 本身没有 CPU 式 ROB；公开资料表明 NVIDIA SM 主要使用 scoreboard 加 warp 调度来管理依赖和隐藏延迟。
至于特定架构内部是否存在小型、未公开的重排结构，不能绝对断言，
但它并不是 CUDA 编程模型或 GPU 性能机制中的核心结构。

## cuda : warp scheduler 是什么
<!-- 1c8ade03-eb81-4fbd-8c84-8b08315befde -->

简单来说: 每个时钟周期，从当前 SM 上所有“已经准备好”的 warp 中，挑选一个或多个，把它们的下一条指令送到执行单元。

它不是负责把 thread block 分配给 SM 的全局调度器，而是 **SM 内部的指令发射调度器**。

wrap scheduler 的 scoreboard 设计就是在说明，一个 wrap 的所有的 thread 都是要执行相同的指令，

1. 一个 SM 只有一个 scheduler 吗？
现代 NVIDIA SM 一般会划分成多个处理分区，每个分区具有自己的 warp scheduler 和执行资源。具体数量和双发射能力依赖 GPU 架构。
因此不要简单理解成， 一个 SM 每周期只能执行一个 warp。
更准确的是，每个 scheduler 每周期从它管理的 ready warp 中选择指令发射；一个 SM 可能有多个 scheduler，并行向不同执行管线发射。

2. 分支时会怎样？

如果同一个 warp 的线程走不同分支：

```cpp
if (threadIdx.x % 2 == 0)
    pathA();
else
    pathB();
```

warp scheduler 并不会把它变成两个完全独立的 warp。硬件会维护线程掩码，分阶段执行不同路径：

```text
执行 pathA：偶数线程启用，奇数线程关闭
执行 pathB：奇数线程启用，偶数线程关闭
```

因此 warp divergence 会降低执行单元利用率。
