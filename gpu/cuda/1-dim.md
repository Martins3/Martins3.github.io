## block 的维度参数
block 是有维度的，这很有趣:

例如二维/三维 block 时，通常要写成类似：

int local_tid =
    threadIdx.x +
    threadIdx.y * blockDim.x +
    threadIdx.z * blockDim.x * blockDim.y;
int idx = blockIdx.x * (blockDim.x * blockDim.y * blockDim.z) + local_tid;

硬件上的区别可以这样理解：

1. dim3 block(1024, 1, 1)
   这是 32 个 warp，每个 warp 的线程编号沿 x 连续增长。最符合一维数组处理，索引简单，访存也最直观。
2. dim3 block(32, 32, 1)
   也是 1024 线程，仍然是 32 个 warp，但线程被组织成二维。warp 的形成本质上还是按线性线程号切分，不是“先一行再一列”这种高
   层概念。它更适合矩阵、图像这类二维数据，因为 threadIdx.x/threadIdx.y 直接对应坐标。
3. dim3 block(16, 16, 4)
   也是 1024 线程，还是 32 个 warp，只是三维组织。适合体数据、3D stencil 之类。索引换算更复杂，如果数据本身不是 3D，通常没有收益。

不管如何写，其实本质都是一样的，只是为了表示方便一点，不会带来性能提升的。

在核函数内部，CUDA 提供了内置变量来访问执行配置参数和线程/块的索引：
- `threadIdx`：线程在其线程块内的索引。
- `blockDim`：线程块的维度（由启动配置指定）。
- `blockIdx`：线程块在网格内的索引。
- `gridDim`：网格的维度（由启动配置指定）。

**边界检查 (Bounds Checking)**：上述例子假设向量长度是线程块大小的整数倍。为了处理任意长度的向量，应在核函数中加入边界检查，并在启动时分配足够的线程块（向上取整）：
```cpp
if (workIndex < vectorLength) { ... }
```
线程块数量的计算方式为“向上取整除法”：
```cpp
int blocks = (vectorLength + threads - 1) / threads;
```
CUDA Core Compute Library (CCCL) 也提供了 `cuda::ceil_div`（需包含 `<cuda/cmath>`）来简化该计算。注意，虽然可以启动比实际需求更多的程（ inactive threads 开销很小），但应避免启动所有线程都不工作的线程块。

### grid 和 block 都是可以有维度的

因为 grid 表示的是“有多少个 block，以及这些 block 在问题空间里怎么排布”。

blockDim 解决的是“一个 block 内部线程怎么排”，而 gridDim 解决的是“很多个 block
在更大范围里怎么排”。两者是同一层思想，分别对应两个层级：

1. threadIdx 描述线程在 block 里的坐标
2. blockIdx 描述 block 在 grid 里的坐标

所以既然 threadIdx 有 .x/.y/.z，那 blockIdx 也要有 .x/.y/.z，否则你没法自然表示“第几行第几列的 block”。

### 矩阵转置里，常见写法是：

```txt
dim3 threadsPerBlock(16, 16);
dim3 blocksPerGrid((cols + 15) / 16, (rows + 15) / 16);
```

这里含义非常直观：

• 每个 block 负责一个 16 x 16 的小砖块
• 整个 grid 是很多个小砖块拼成的二维平面
• blockIdx.x 表示当前 block 是第几列砖块
• blockIdx.y 表示当前 block 是第几行砖块

于是全局坐标直接就是：

```txt
int col = blockIdx.x * blockDim.x + threadIdx.x;
int row = blockIdx.y * blockDim.y + threadIdx.y;
```

### 二维的 block 的 vector add

算是很经典了:

```cuda
__global__ void vector_add_kernel(const float *a, const float *b, float *c,
				  int n)
{
	int tid_in_block = threadIdx.y * blockDim.x + threadIdx.x;
	int threads_per_block = blockDim.x * blockDim.y;
	int idx = blockIdx.x * threads_per_block + tid_in_block;
	if (idx < n) {
		c[idx] = a[idx] + b[idx];
	}
}


dim3 block(kBlockDimX, kBlockDimY);
int threads_per_block = kBlockDimX * kBlockDimY;
int blocks = (kElementCount + threads_per_block - 1) / threads_per_block;
vector_add_kernel<<<blocks, block>>>(dev_a, dev_b, dev_c, kElementCount);
```
