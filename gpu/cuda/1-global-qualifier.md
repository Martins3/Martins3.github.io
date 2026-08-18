## CUDA `__global__` 限定符详解
<!-- 6b65d762-8617-4de8-a73f-3f3e44bda7cb -->

`__global__` 不只是"标记核函数"，它实际上在 CUDA
编程模型中扮演着一个**跨执行空间的边界声明**：告诉编译器"这个函数需要特殊的发射机制（`<<< >>>`），它的代码要放在设备端，但入口要暴露给主机端（或支持
CDP 的设备端）调度器"。

`__global__` 的核心角色是**执行空间限定符（execution space specifier）**，但它的作用不止"标记核函数入口"这么简单。

1. 最基本的语义：定义"CPU 发起、GPU 执行"的函数

这是你最熟悉的作用：

| 限定符             | 调用方                       | 执行方  | 能否用 `<<< >>>` 启动 |
| ------------------ | ---------------------------- | ------- | --------------------- |
| `__host__`（默认） | CPU                          | CPU     | 否                    |
| `__device__`       | GPU                          | GPU     | 否                    |
| `__global__`       | **CPU**（或支持 CDP 的 GPU） | **GPU** | **是**                |

2. 编译器会为其生成两份代码

一个 `__global__` 函数在编译时会产生两份产物：

- **设备端代码**：真正的 GPU 机器码，在 SM 上执行。
- **主机端 stub**：一个由编译器自动生成的包装函数。当你写 `my_kernel<<<2, 128>>>(args)` 时，实际调用的是这个 stub。它负责：
  - 把启动配置（grid/block 维度、共享内存大小、流）传给 CUDA Runtime
  - 把参数打包到常量内存或参数缓冲区
  - 最终调用 `cudaLaunchKernel`

这也是为什么 `__global__` 函数**必须返回 `void`**——内核启动是异步的，CPU 无法直接接收 GPU 函数的返回值。

3. 限制函数签名与行为

`__global__` 带来了一系列编译期约束，这些约束本身就体现了它的"作用"：

- **必须返回 `void`**（历史遗留 + 异步执行模型决定的）
- **不支持可变参数**（`...`）
- **不能是类的普通成员函数**（无法携带隐式的 `this` 指针穿越 CPU/GPU 边界；但可以是 static member function，在较新 CUDA 中支持）
- **不能取地址获得设备函数指针直接在主机端使用**（早期版本限制，现在可通过 `cudaMemcpyFromSymbol` 等机制获取）

### 扩展话题

1. 支持 CUDA Dynamic Parallelism（CDP）

从计算能力 3.5（Kepler+）开始，`__global__` 函数也可以**由 GPU 代码调用**。
这就是 CUDA Dynamic Parallelism，俗称"设备端启动内核"：

```cuda
__global__ void child_kernel(int *data) { ... }

__global__ void parent_kernel(int *data) {
    // 在 GPU 上直接启动另一个内核
    child_kernel<<<2, 128>>>(data);
}
```

所以严格来说，`__global__` 的调用方不一定是 CPU，而是"**当前执行上下文之外的调度器**"。早期 CUDA 中这个调度器只能是 CPU；CDP 出现后，GPU 上的运行时也具备了这个能力。

2. 与 `__launch_bounds__` 配合影响寄存器分配

虽然这是配合属性使用的，但 `__global__` 函数是唯一能使用 `__launch_bounds__(maxThreadsPerBlock, minBlocksPerMultiprocessor)` 的函数类型。编译器根据这个提示调整寄存器分配策略，以优化占用率（occupancy）。

```cuda
__launch_bounds__(256, 2)
__global__ void my_kernel(...) { ... }
```
