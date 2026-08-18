# cuda : thread block cluster
<!-- 6e301e80-1aaf-4be0-b892-296bba77900f -->

线程块簇 (Thread Block Clusters)

从计算能力 9.0 (sm_90) 开始，CUDA 引入了可选的**线程块簇**层级。
簇内的线程块保证在同一个 GPU Processing Cluster (GPC) 上共同调度，
并支持通过 Cooperative Groups API (`cluster.sync()`) 进行硬件级同步。
簇内的线程块还可以访问 distributed shared memory。

启动簇的方式有两种：
1. 编译时核函数属性：`__cluster_dims__(X, Y, Z)`，之后仍可用 `<<< >>>` 启动。
2. 运行时 API：`cudaLaunchKernelEx`。

注意，在支持簇的核函数中，`gridDim` 仍表示线程块的数量。


Compute Capability 9.0 及以上支持 thread block cluster。一个 cluster 内的 block 保证协同调度，
可以通过 cluster group 同步：

```cpp
cg::cluster_group cluster = cg::this_cluster();
cluster.sync();
```

它只同步**同一个 cluster 内**的 block，并不是任意整个 grid。
适合使用 distributed shared memory、多个 block 紧密协作的场景。

### Cooperative Groups 的 grid 同步

可以使用：

```cpp
#include <cooperative_groups.h>
namespace cg = cooperative_groups;

__global__ void kernel(...) {
    cg::grid_group grid = cg::this_grid();

    // phase 1
    ...

    grid.sync();

    // phase 2
    ...
}
```

但必须通过 cooperative launch 启动，例如 `cudaLaunchCooperativeKernel`，
并且 GPU 和运行环境需要支持 cooperative launch。

关键限制是：**整个 grid 必须能够同时驻留在 GPU 上**。因此允许启动的 block 数量受 SM 数量、寄存器、shared memory 和 occupancy 限制。
这种方法适用于 persistent kernel、反复迭代算法等，但不如拆 kernel 通用。

## 2026-08-16
为什么感觉和 cooperative-groups 还是有关系的
