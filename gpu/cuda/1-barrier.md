## cuda barrier
<!-- 8e4d8cfc-4db9-4755-953b-9f3c8e260b7f -->

最典型的是 block 内同步：

```cpp
__syncthreads();
```

它要求同一个 block 中的所有线程都到达同步点，才能继续执行。

对应的 PTX 通常类似：

```ptx
bar.sync 0;
```

此外还包括：

```cpp
__syncthreads_count(predicate);
__syncthreads_and(predicate);
__syncthreads_or(predicate);
```

它们不仅同步线程，还会对条件进行统计或逻辑归约，同样需要 barrier 跟踪状态。

Cooperative Groups 的 block 级同步也会使用：

```cpp
namespace cg = cooperative_groups;

cg::thread_block block = cg::this_thread_block();
block.sync();
```

它通常最终也会映射到 block barrier。

某些异步流水线、TMA 或 `mbarrier` 操作也需要硬件同步状态，例如：

```cpp
cuda::barrier<cuda::thread_scope_block>
```

以及底层 PTX：

```ptx
mbarrier.init
mbarrier.arrive
mbarrier.try_wait
```

不过 `mbarrier` 的状态通常存放在共享内存中，同时由硬件协助追踪；它和传统 `bar.sync` 的资源记账方式可能因 GPU 架构而不同。

## 为什么 barrier 的实现需要额外的硬件资源
因为 barrier 不只是“一条等待指令”。当一个 thread block 内的线程执行同步时，SM 必须保存一份硬件状态：
- 哪个 block 的哪个 barrier；
- 哪些 warp/线程已经到达；
- 哪些还没有到达；
- 预计需要多少线程到达；
- 条件满足后，需要唤醒哪些 warp；
- 某些异步操作是否已经完成。

这些状态需要由 SM 中专门的 barrier 跟踪单元保存。
它的槽位数量有限，所以被视为类似寄存器、共享内存的硬件资源。

### 为什么每个并发 block 都需要独立状态？

假设一个 SM 同时驻留三个 block：

```text
Block 0：7 个 warp 已到达 barrier，1 个未到达
Block 1：4 个 warp 已到达 barrier，4 个未到达
Block 2：全部到达，可以释放
```

这三个 block 互不相关，因此 SM 必须分别记录它们的同步状态。不能让 Block 0 的线程错误地与 Block 1 的线程完成同步。

可以把 barrier 资源理解成 SM 内部的一组“同步记分牌槽位”：

```text
Barrier slot
  ├─ 所属 block
  ├─ barrier ID
  ├─ 到达线程/warp 状态
  └─ 等待与释放状态
```

并发 block 越多、每个 block 使用的独立 barrier 越多，所需的跟踪槽位就越多。
