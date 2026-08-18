# cuda 中一般来说如何同步
<!-- 003412c2-e0f8-4fb4-a044-a9916d7f3333 -->

CUDA 中，**block 之间同步最通用、最稳妥的做法是：拆成多个 kernel，利用 kernel 边界作为全局同步点**。

```cpp
kernel_stage1<<<grid, block, 0, stream>>>(...);
kernel_stage2<<<grid, block, 0, stream>>>(...);
```

同一 stream 中，`kernel_stage2` 会在 `kernel_stage1` 完成后执行。因此：

* stage1 的所有 block 都已结束；
* stage1 对 global memory 的写入对 stage2 可见；
* 不需要在两个 kernel 之间额外调用 `cudaDeviceSynchronize()`；
* 只有 CPU 也必须等待结果时，才需要 `cudaDeviceSynchronize()` 或事件同步。

## 为什么普通 kernel 内不能直接同步所有 block？

`__syncthreads()` **只同步当前 block 内的线程**，不能同步整个 grid。

普通 kernel 的 block：

* 调度顺序不确定；
* 可能不会同时驻留在 GPU 上；
* 后面的 block 可能要等前面的 block 退出才能获得执行资源。

所以自己用“全局计数器 + 自旋等待”实现 grid barrier，可能死锁：

```cpp
// 危险示意
if (threadIdx.x == 0)
    atomicAdd(&arrived, 1);

while (arrived < gridDim.x) {
    // 可能永久等待
}
```

假设 GPU 同时只能驻留 16 个 block，却启动了 100 个。前 16 个 block 在这里等待剩余 block，而剩余 block 又没有资源启动，于是死锁。

另外：

* `__threadfence()` 是**内存顺序/可见性屏障**；
* atomic 操作保证特定访问的原子性；
* 它们本身都不是“所有 block 到齐后一起继续”的执行 barrier。

## 必须在单个 kernel 内同步时

3. 原子变量、内存栅栏和无锁协议

如果需要的不是“所有 block 同时到达”，而只是：

* 工作队列；
* 生产者通知消费者；
* 全局任务计数；
* 动态负载均衡；
* 最后一个 block 负责归并；

可以使用 atomic、`__threadfence()` 或 CUDA C++ scoped atomics。

这属于**跨 block 通信协议**，不等价于通用 barrier。设计不正确依然可能死锁或读到旧数据。

利用一些高级算法了。

## 什么时候需要 block 间同步？

只有当一个 block 后续读取的数据依赖其他 block 已经完成的写入时，才需要同步。典型情况包括：

* 多阶段计算：第一阶段生成中间结果，第二阶段消费；
* 全局归约：各 block 先计算局部结果，再进行最终归约；
* scan/prefix sum：block 局部扫描后，需要传播 block 间偏移；
* 图算法、动态规划、迭代求解：下一轮依赖上一轮的全局状态；
* persistent kernel 中，多轮任务之间需要统一推进；
* cluster 内多个 block 共享 distributed shared memory。

如果每个 block 处理彼此独立的数据，或者只在 kernel 完成后由下一个 kernel 使用结果，就不需要在当前 kernel 内做 block 间同步。

简化选择原则：

| 需求                       | 推荐方法                              |
| ------------------------ | --------------------------------- |
| 普通多阶段计算                  | 拆成多个 kernel                       |
| 同一 stream 的前后 kernel     | 天然有序，不必同步 CPU                     |
| CPU 要读取 kernel 结果        | event 或 `cudaDeviceSynchronize()` |
| 单 kernel、整个 grid barrier | Cooperative Groups `grid.sync()`  |
| Hopper 及以后、少量相邻 block 协作 | Thread Block Cluster              |
| 工作队列、计数、生产者/消费者          | atomic + 正确的内存顺序                  |
| 只是保证 global memory 写入可见  | `__threadfence()`，但它不是 barrier    |

实践中，优先顺序通常是：**拆 kernel > cooperative grid sync > 自定义原子同步协议**。

