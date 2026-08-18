## cuda 如何做加法
在 CUDA 中，GPU 计算向量和（vector sum）通常使用 **并行归约（parallel reduction）** 技术。简单来说，思路是把向量分成很多小块，每个线程先计算自己负责的部分，然后逐步合并结果。具体步骤如下：

1. 将向量划分给线程

假设有一个长度为 `N` 的向量 `v`，你启动一个 CUDA kernel，给每个线程一个或多个元素：

```cpp
int idx = threadIdx.x + blockIdx.x * blockDim.x;
if (idx < N) {
    partial_sum[idx] = v[idx];
}
```

* 每个线程先处理自己的元素（或一小块元素）。
* 使用 `blockDim.x * gridDim.x` 的线程覆盖整个向量。


2. 使用共享内存进行 block 内归约

GPU 的线程是分 block 执行的，每个 block 内的线程可以利用 **shared memory** 做快速归约：

```cpp
__shared__ float sdata[BLOCK_SIZE];

int tid = threadIdx.x;
sdata[tid] = (global_idx < N) ? v[global_idx] : 0;
__syncthreads();

// 归约循环
for (unsigned int s = blockDim.x/2; s>0; s>>=1) {
    if (tid < s) {
        sdata[tid] += sdata[tid + s];
    }
    __syncthreads();
}

if (tid == 0) {
    block_sums[blockIdx.x] = sdata[0];  // 每个 block 的总和
}
```

* `sdata` 是 block 内的共享内存数组。
* 归约时每次折半，最终 `tid == 0` 保存该 block 的总和。

3. 对 block sums 进行全局归约

所有 block 的总和存储在 `block_sums` 中，可以通过：

* **CPU 端累加**（简单但需要拷贝回主机）。
* **递归 CUDA kernel**：再次启动归约 kernel，直到只剩一个结果。

### CUDA 官方库

* 对于实用场景，NVIDIA 提供了 **Thrust** 库，可以直接做：

```cpp
#include <thrust/device_vector.h>
#include <thrust/reduce.h>

thrust::device_vector<float> d_vec(v, v+N);
float sum = thrust::reduce(d_vec.begin(), d_vec.end(), 0.0f, thrust::plus<float>());
```

* 这个 `thrust::reduce` 会自动选择最优归约算法，无需手写 kernel。
