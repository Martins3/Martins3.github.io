## cuda Interprocess Communication 理解
<!-- fd5db4ca-d7e2-47ef-a88b-399b5e9b1c78 -->

<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/inter-process-communication.html>

不就是 cuda GPU 从 IPC 的角度来分析么:

## 一句话总结

这章的核心不是“把 GPU 指针传给另一个进程”，而是：

> 先把 GPU 资源导出成一个 **process-portable handle**，
> 再由另一个进程把它重新打开成自己上下文里的 **process-local device pointer**。

真正跨进程传递的是：

- handle

不是：

- 原始 device pointer 值

---

## 这章到底在解决什么问题

原文先强调一个最基本的事实：

### 同一个进程里

一个 host thread 创建的：

- device memory pointer
- event handle

可以被同进程里的其他线程直接引用。

### 不同进程之间

就不成立了。

原因很简单：

- device pointer 只在创建它的那个进程的 CUDA 上下文里有效
- event handle 也一样

所以如果进程 A 直接把一个 `cudaMalloc` 返回的指针值发给进程 B，进程 B 不能把它当成自己的有效 device pointer 来用。

这章的根问题就是：

> 在不打破进程隔离的前提下，如何让多个进程访问同一块 GPU 资源。

---

## 原文真正的核心抽象

原文反复在讲两个词：

- `process-portable handle`
- `process-local device pointer`

这两个词其实已经把整章讲完了。

可以把数据流画成这样：

```text
进程 A 里的本地 device pointer
  -> 导出成 portable handle
    -> 通过普通 OS IPC 机制发送
      -> 进程 B 收到 handle
        -> 在进程 B 里重新打开
          -> 得到进程 B 自己的本地 device pointer
```

所以本章真正共享的不是“指针值”，而是：

- 一个可以跨进程传输的句柄

然后每个进程都把这个句柄重新解码成：

- 自己上下文里有效的指针

---

## Legacy IPC API：最直接的做法

原文给出的最传统路径是 CUDA Runtime 的 IPC API。

最核心的两个函数就是：

- `cudaIpcGetMemHandle()`
- `cudaIpcOpenMemHandle()`

event 也有对应的共享接口。

它们表达的事情非常直接：

### 发送方

1. 先有一块自己的 device memory
2. 把它转换成 `cudaIpcMemHandle_t`
3. 用普通主机侧 IPC 手段把这个 handle 发给对方

### 接收方

1. 收到 handle
2. 在自己进程里调用 `cudaIpcOpenMemHandle()`
3. 得到一个属于自己上下文的有效 device pointer
4. 像普通 device pointer 一样使用它

原文给的典型场景是：

- 一个 primary process 生成一批输入数据
- 多个 secondary process 复用同一份 GPU 数据
- 避免重复生成或重复拷贝

所以你可以把 Legacy IPC 理解成：

> 最简单的“单节点 Linux 下跨进程共享 `cudaMalloc` 显存”的办法。

---

## VMM API：更现代、但更底层

原文同时明确说，IPC 不只有 Legacy API 这一条路。

另一条路是：

- `Virtual Memory Management (VMM) API`

原文对它的定位很明确：

- 可以创建 **IPC-shareable memory allocations**
- 支持多个操作系统
- 可以在 **分配时** 就控制共享属性和 peer accessibility

这和 Legacy IPC 的区别在于：

### Legacy IPC

更像是：

- 先 `cudaMalloc`
- 后面再问“我能不能把这块内存导出成 handle”

### VMM

更像是：

- 在内存创建的时候
- 就把“共享方式、访问权限、peer 可见性”设计进去

所以原文实际上是在给一个取舍：

- 简单场景：Legacy IPC
- 想要更细粒度控制：VMM

代价则是：

- VMM 需要 Driver API
- 编程模型更复杂

---

## Fabric Handle：把同样的思想扩到多节点

原文还提到多节点 NVLink 集群场景。

这里最值得注意的是：

- 原文没有把它说成“完全不同的机制”
- 而是把它描述成 **portable handle 思路的继续扩展**

也就是：

### 单节点 / 单 OS 实例

- 交换 IPC handle

### 多节点 / 多 OS 实例

- 交换 fabric handle

本质上仍然是：

1. 先生成一个可移植句柄
2. 再交换句柄
3. 再在本地得到 process-local device pointer

所以 fabric handle 可以理解成：

> IPC handle 在多节点场景下的扩展版本。

---

## 原文最值得记住的几个限制

我觉得这章最重要的内容，很多其实都不在 API 名字本身，而在这些 note 里。

### 1. Legacy IPC 只支持 Linux

原文写得很直接：

- `The IPC API is only currently supported on Linux platforms.`

这意味着：

- Legacy IPC 不是一个跨平台的通用抽象
- 如果你需要更跨平台的共享方案，原文其实是在暗示你看 VMM

### 2. `cudaMallocManaged` 不支持 Legacy IPC

原文明确说：

- `The IPC API is not supported for cudaMallocManaged allocations.`

这说明 Legacy IPC 假定自己共享的是：

- 比较传统的设备内存资源

而不是：

- 会迁移、会分页、会被 CPU/GPU 共同托管的 managed memory

### 3. 多个进程要使用相同的 driver/runtime

原文要求：

- 使用 CUDA IPC 通信的进程，应使用相同的 CUDA driver 和 runtime

这反映出 handle 不是一个和 CUDA 实现彻底解耦的“开放协议”，而是依赖驱动和运行时共同解释的资源句柄。

### 4. `cudaMalloc()` 可能来自更大底层块的子分配

这是原文里最值得单独记住的一点。

原文意思是：

- `cudaMalloc()` 出于性能原因，可能不是独占一整块底层内存
- 它可能只是某个更大 block 里的一个 sub-allocation
- 当你调用 IPC API 去分享这块 allocation 时
- 实际上被分享出去的，可能是整个底层 block

这就会带来一个很现实的问题：

- **information disclosure**

也就是：

- 你本来只想共享自己申请的那段内存
- 结果同一个底层大块里的其他子分配也可能被一并暴露出去

因此原文给出非常明确的建议：

- 只共享 **2 MiB aligned size** 的 allocations

它背后的动机是：

- 尽量让你的分配独占底层大页 / 底层块
- 降低和别的子分配混在一起的概率

这不是语法问题，而是很真实的工程和安全问题。

### 5. Tegra 上只支持 event IPC，不支持 memory IPC

原文明确指出：

- 在 L4T 和 embedded Linux Tegra 上
- 只支持 IPC event-sharing API
- 不支持 IPC memory-sharing API

这说明不同平台对 IPC 的支持范围并不对称。

---

## 这章没有重点讲什么

这点也很重要。

这章虽然在讲“跨进程通信”，但它没有把重点放在：

- lock-free 协议
- 跨进程一致性模型
- 多进程 ring buffer
- 高级同步协议

它更像是在解决：

> “别的进程怎么合法地拿到这块 GPU 资源的本地引用？”

也就是说，它重点解决的是：

- **资源引用建立**

而不是：

- **资源建立之后的复杂协作协议**

后者仍然需要依赖：

- event
- stream
- memory model
- 上层通信库
- 或应用自己设计的同步协议

---

## 原文的真正主线，其实比很多二次总结都更聚焦

如果完全按原文主线来整理，这章几乎可以压缩成下面五点：

1. 同进程里的 device pointer / event handle 可以直接共享给其他线程。
2. 但跨进程时，这些对象都失效，不能直接传。
3. 正确做法是先导出为 portable handle，再在接收进程里打开成 process-local pointer。
4. 单节点 Linux 下，Legacy IPC API 是最直接的方案。
5. 如果需要更细粒度控制或跨更多平台，就看 VMM；多节点 NVLink 集群则进一步扩展为 fabric handle。

---

## 你应该把这章记成什么

如果只记一句话，我建议记这个：

> Interprocess Communication 这章解决的不是“怎么传 GPU 指针”，而是“怎么跨进程建立对同一块 GPU 资源的合法引用”。

如果只记三个点，就是：

1. 跨进程不能直接传 device pointer，必须传 portable handle。
2. Legacy IPC 简单，但只支持 Linux，且不支持 `cudaMallocManaged`。
3. VMM 更灵活，Fabric Handle 则把同样的 handle 思路扩展到多节点。
