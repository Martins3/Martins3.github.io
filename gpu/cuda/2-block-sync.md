# cuda block 间同步
<!-- 4cd7df09-2482-4ba7-8c16-5f5522254a31 -->

在 CUDA 中，**不同 block 之间没有原生的、高效的同步原语**（如 `__syncthreads()` 只能同步 block 内的线程）。这是由 GPU 的 SIMT 执行模型决定的：不同 block 的调度顺序和时机是不确定的，任何假设它们会同时执行的同步尝试都可能导致死锁。

## 1. 多 Kernel 启动（最常用、最推荐）

将需要同步的逻辑拆分到多个 kernel 中，通过 kernel 边界隐式实现全局同步。

```cuda
// Kernel 1: 阶段 A
kernel_phaseA<<<grid, block>>>(d_data);
cudaDeviceSynchronize();  // 全局同步点

// Kernel 2: 阶段 B（依赖阶段 A 的结果）
kernel_phaseB<<<grid, block>>>(d_data);
cudaDeviceSynchronize();
```

**优点**：安全、简单、由硬件保证所有 block 完成
**缺点**：有 kernel launch 开销，数据留在显存中无搬迁成本

## 2. Cooperative Groups（网格级同步）

CUDA 9 引入的 Cooperative Groups 提供了 `grid_group::sync()`，允许同 grid 内的所有线程同步。

```cuda
#include <cooperative_groups.h>

__global__ void myKernel(int *data) {
    namespace cg = cooperative_groups;
    cg::grid_group grid = cg::this_grid();

    // 阶段 1
    data[threadIdx.x] = threadIdx.x;
    grid.sync();  // 所有 block 同步

    // 阶段 2（可读取其他 block 的数据）
    int neighbor = data[threadIdx.x + 1];
}

// 启动时需使用 cooperative launch
cudaLaunchCooperativeKernel((void*)myKernel, gridDim, blockDim, args);
```

**前提条件**：
- 需要查询设备支持的最大协作 block 数：`cudaDevAttrCooperativeLaunch`
- Grid 尺寸不能超过该限制
- 使用 `cudaLaunchCooperativeKernel` 而非 `<<< >>>`

## 3. 原子操作 + 全局内存轮询（风险高，不推荐）

利用全局内存和原子操作实现屏障，但**极易出错且通常性能差**：

```cuda
__device__ volatile int globalCounter = 0;

__global__ void riskySyncKernel(int *data, int *barrier, int totalBlocks) {
    // 阶段 A
    data[threadIdx.x] = ...;

    __threadfence();  // 确保全局内存写可见

    // 仅线程 0 参与屏障
    if (threadIdx.x == 0) {
        atomicAdd((int*)barrier, 1);
        while (*barrier < totalBlocks);  // 自旋等待
    }
    __syncthreads();  // block 内同步
}
```

**严重问题**：
- 若 SM 同时容纳的 block 数有限，正在自旋的 block 会占用 SM 资源，可能导致等待的 block 无法被调度（死锁）
- 自旋消耗大量内存带宽和功率
- **仅在某些特殊场景（如减少 kernel 启动次数）且充分验证后使用**

## 4. CUDA 流和事件（Host 侧同步）

如果需要跨多个 kernel 或设备进行更复杂的同步，可以使用流和事件：

```cuda
cudaStream_t stream1, stream2;
cudaStreamCreate(&stream1);
cudaStreamCreate(&stream2);

kernel1<<<grid, block, 0, stream1>>>(d_a);
kernel2<<<grid, block, 0, stream2>>>(d_b);

cudaEvent_t event;
cudaEventCreate(&event);
cudaEventRecord(event, stream1);
cudaStreamWaitEvent(stream2, event, 0);  // stream2 等待 stream1 完成
```

## 总结建议

| 场景 | 推荐方案 |
|------|---------|
| 通用算法，可拆分为多阶段 | **多 Kernel 启动** |
| 单 kernel 内必须全局同步 | **Cooperative Groups** |
| 简单依赖关系、流水线 | **CUDA 流 + 事件** |
| 追求极致性能避免 launch 开销 | Cooperative Groups（在限制内） |
| 原子轮询屏障 | **尽量避免** |

发现了一个有趣的场景，就是可以其实可以用 atomic 来配置。
