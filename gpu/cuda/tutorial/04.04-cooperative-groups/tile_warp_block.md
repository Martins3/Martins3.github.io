# cuda  中 Tile、Warp、Thread Block 的关系
<!-- 2f5cff84-4289-478d-afb7-9e77271e27bc -->

## 结论

Cooperative Groups 里的 tile 不是 CUDA 硬件层级里的固定一层。它是从某个父组
切出来的“线程子组”。

最常见的 tile 是 32 个线程：

```cpp
cg::thread_block cta = cg::this_thread_block();
auto tile = cg::tiled_partition<32>(cta);
```

因为当前 NVIDIA GPU 的 warp size 是 32，所以 `tile<32>` 正好对应一个 warp。
这也是教程和示例里最常看到 `tiled_partition<32>` 的原因。

但 tile 不一定只能是 32，也不一定只能等于一个 warp。

## 小于等于 32 的 tile

`tile<1>`、`tile<2>`、`tile<4>`、`tile<8>`、`tile<16>`、`tile<32>` 都是常见
的单 warp 内 tile。比如：

```cpp
auto tile16 = cg::tiled_partition<16>(cta);
```

如果一个 block 有 128 个线程，那么它会先按连续线程切分：

```text
thread 0..15    -> tile 0
thread 16..31   -> tile 1
thread 32..47   -> tile 2
thread 48..63   -> tile 3
thread 64..79   -> tile 4
thread 80..95   -> tile 5
thread 96..111  -> tile 6
thread 112..127 -> tile 7
```

`tile<16>` 在硬件上仍然落在 warp 内。一个 warp 里的 32 个线程会被分成两个
16 线程 tile。

这类 tile 的优点是实现很轻，很多通信可以直接用 warp mask 和 shuffle 指令完成。

## 大于 32 的 tile

Cooperative Groups 的编译期 tile 也可以跨多个 warp，例如：

```cpp
auto tile64 = cg::tiled_partition<64>(cta);
```

如果一个 block 有 128 个线程，那么：

```text
thread 0..63    -> tile 0
thread 64..127  -> tile 1
```

`tile<64>` 就是 multi-warp tile：一个 tile 内有两个 warp。它仍然只在同一个
thread block 内部，不能跨 block。

所以 tile 的范围关系更准确地说是：

```text
thread
  -> warp
    -> tile 可以小于、等于或大于 warp
      -> tile 不能超过它的父组，通常也不能超过 thread block
        -> cluster
          -> grid
```

这里的缩进不是严格的硬件层级，而是在表达包含关系：tile 是从父组切出来的逻辑
子组。`tile<64>` 会包含多个 warp，但仍然属于同一个 thread block。

## 运行时 tile 和编译期 tile 的区别

Cooperative Groups 有两种常见写法：

```cpp
auto static_tile = cg::tiled_partition<32>(cta);
auto dynamic_tile = cg::tiled_partition(cta, 32);
```

第一种 `<32>` 是编译期常量，返回 `thread_block_tile<32>` 这种静态类型。
第二种 `32` 是运行时参数，返回更通用的 `thread_group`。

在当前 CUDA 头文件实现中，运行时版本 `tiled_partition(parent, tilesz)` 对
`tilesz` 的限制更窄：必须是 1、2、4、8、16、32 这类不超过 32 的 2 的幂。

编译期版本 `tiled_partition<N>` 支持 multi-warp tile，例如 `N = 64` 或
`N = 128`。这类 tile 的集合操作通常需要跨 warp 协调，可能用到 shared memory
和额外同步，所以成本比单 warp tile 更高。

## 哪些操作仍然限制在 32 以内

需要注意：并不是所有 Cooperative Groups API 都对 multi-warp tile 开放。

例如当前 CUDA 实现里：

```cpp
cg::binary_partition(tile, pred);
cg::labeled_partition(tile, label);
```

这类动态分组接口只支持 `Size <= 32` 的 tile。原因是它们依赖 warp 内 active
mask、ballot、match 等机制，天然是 warp 粒度能力。

`reduce`、`scan` 这类集合操作可以用于 `thread_block_tile<N>`，其中 `N` 可以
大于 32；只是 multi-warp 情况下实现路径会更复杂，性能模型也不同。

## 和硬件 warp 的关系

warp 是硬件调度和执行的基本 SIMD/SIMT 单位，当前 NVIDIA GPU 上是 32 个线程。
tile 是 Cooperative Groups 提供的软件抽象，用来给一组线程命名，并在这组线程
上执行 `sync`、`reduce`、`scan`、`shfl` 等操作。

因此：

- `tile<32>` 通常就是“把一个 warp 包装成 Cooperative Groups handle”。
- `tile<16>` 是“把一个 warp 再切成两个逻辑子组”。
- `tile<64>` 是“把同一个 block 内的两个 warp 组成一个逻辑子组”。
- tile 不能跨 thread block；跨 block 协作要看 cluster 或 grid group。

## Grid 和 Cluster 能不能跨多个 GPU

普通 CUDA kernel 的 grid 不能跨 GPU。一次 kernel launch 发生在当前 CUDA
device 上，`gridDim` 描述的是这个 device 上启动了多少个 block，不包含“第几个
GPU”这一维。`cg::this_grid()` 得到的 `grid_group` 也只覆盖当前这一次 launch
在同一个 GPU 上的所有线程。

cluster 也不能跨 GPU。thread block cluster 是单 GPU 内部的硬件调度层级：
cluster 内的 block 被保证共同调度到同一个 GPU 的 GPC 上，并且可以使用
`cluster.sync()` 和 distributed shared memory。不同 GPU 没有共享同一个 GPC，
也不能把一个 cluster 的 block 分散到多个 GPU 上。

CUDA 历史上提供过更特殊的 multi-device cooperative launch：

```cpp
cudaLaunchCooperativeKernelMultiDevice(...);
cg::this_multi_grid();
```

这套接口试图把多个 GPU 上同时启动的 cooperative grids 组织成
`multi_grid_group`，允许跨 device 的同步语义。但在当前 CUDA 头文件中，
`cudaLaunchCooperativeKernelMultiDevice` 和相关能力已经标记为 deprecated。
所以学习和新代码里不要把普通 `grid_group` 理解成可以自然跨 GPU。

如果程序确实要用多个 GPU，通常做法是每个 GPU 各自启动一个 grid，然后用 host
端同步、CUDA event、NCCL、NVSHMEM、MPI 或其他通信库在 GPU 之间协调。GPU 内部
的层级边界仍然是：

```text
单个 GPU 内：
thread -> warp -> tile -> thread block -> cluster -> grid

多个 GPU 间：
多个独立 grid 由 host 或通信库协调
```

## 实际使用建议

默认优先使用 `tile<32>`。它和 warp 对齐，语义直观，通常性能最好。

如果算法天然只需要半个 warp 或更小的线程组，可以用 `tile<16>`、`tile<8>`。
例如一个 warp 同时处理两个小任务，每个任务 16 个线程。

如果算法确实需要 64、128 个线程作为一个整体做 `reduce` 或 `scan`，可以使用
multi-warp tile。但要确认父 block 至少有这么多线程，并接受额外同步和 shared
memory 协调的成本。

不要把 tile 理解成 CUDA 硬件层级中固定等于 warp 的东西。更好的理解是：

```text
tile = 从父 cooperative group 中切出来的逻辑线程子组
```

它最常见地等于 warp，但概念上不局限于 warp。
