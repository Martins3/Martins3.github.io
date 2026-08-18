# cuda: gpc / thread block cluster

## GPC 是什么

GPC 是 **Graphics Processing Cluster** 的缩写，是 NVIDIA GPU 内部的一个硬件组织层级。

平时 CUDA 编程最常见的视角是：

```text
grid
  -> thread block
      -> warp
          -> thread
```

但硬件内部还有另一套层级。粗略地看：

```text
GPU
  -> GPC
      -> TPC / SM 分组
          -> SM
              -> warp scheduler / CUDA core / shared memory 等执行资源
```

这里的 GPC 不是 CUDA C++ 里能直接索引的对象。普通 kernel 代码里没有类似
`gpcIdx` 的内置变量，也不能手动指定某个 block 去哪个 GPC 上执行。它主要是
GPU 调度和硬件局部性的概念。

## SM、GPC、Cluster 的关系

一个 thread block 仍然只在一个 SM 上执行。thread block cluster 不是让一个 block 横跨多个 SM，而是把多个 block 组织到一起：

```text
一个 cluster
  -> block 0 -> 某个 SM
  -> block 1 -> 同一个 GPC 内的另一个 SM
  -> block 2 -> 同一个 GPC 内的另一个 SM
```

所以可以这样理解：

- **SM**：真正执行 thread block 的基本硬件单元。
- **GPC**：包含一组 SM 的更大硬件局部区域。
- **thread block cluster**：CUDA 暴露给程序员的跨 block 协作单位。

CUDA 代码操作的是 cluster：

```cpp
cg::cluster_group cluster = cg::this_cluster();
cluster.sync();
int *remote = cluster.map_shared_rank(&local_shared_var, peer_rank);
```

硬件实现上，为了让这些操作可行，cluster 内的 block 会被共同调度到同一个 GPC。

## human

这里我还是感觉非常奇怪，似乎 gpc 既然是不可见的，那么如何知道当前的 thread block 是
如何放到那个 GPC 上的，似乎必须保证才可以。
