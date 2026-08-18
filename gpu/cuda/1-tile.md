## cuda tile-based 计算
<!-- 18a6ead4-ee72-4caa-8cf0-24be54643b4c -->

(继续看看，将其中的东西合并下，感觉这是之前没完全懂的时候写的东西)

Tile-based 计算是 GPU 编程中最基础、最重要的性能优化策略之一。

它的核心思想非常简单：

> 把大问题切成小方块（tile），每次只拿一块放进离计算单元最近的快速缓存里，算完再拿下一块。

在 CUDA 的语境下，"最近的快速缓存"通常就是 **Shared Memory**。

---

## 一个直观的类比

想象你在一张巨大的地图上计算每个区域的统计数据。

- **朴素做法**：每次需要哪个格子的数据，就派人去档案室（Global Memory）取一份。档案室很远，来回一趟很慢。
- **Tile-based 做法**：把地图切成若干小块（tile），每次派一队人去档案室把一整块搬回临时工作站（Shared Memory）。工作站在计算单元旁边，所有人都从这里快速取数据。这块算完了，再搬下一块。

关键区别：

- 减少了去档案室的次数
- 一次搬一批，比零散地搬更高效
- 数据搬到工作站后，可以被多个计算人员反复复用

---

## 为什么在 GPU 上必须这么做

### 1. Global Memory 很慢，Shared Memory 很快

| 内存类型 | 位置 | 延迟 |
|---------|------|------|
| Global Memory | 设备 DRAM | 几百个时钟周期 |
| Shared Memory | SM 片上 | 几十个时钟周期 |

如果 kernel 每次都直接从 Global Memory 读写，计算单元大部分时间都在等数据。

### 2. 合并访问的要求

GPU 的 Global Memory 以 32 字节事务为单位访问。一个 warp（32 个线程）最理想的情况是：

- 连续线程访问连续的内存地址
- 这样 32 个线程的访问请求可以合并成极少的事务

但如果线程访问的地址分散（比如矩阵按列访问），每个线程都可能触发独立的事务，带宽利用率暴跌。

Tile-based 计算配合 Shared Memory 可以解决这个问题：

- **读阶段**：按合并友好的方式把 tile 从 Global Memory 读入 Shared Memory
- **计算阶段**：在 Shared Memory 中按任意模式访问（转置、滑动窗口等），都不影响 Global Memory 的合并性
- **写阶段**：再把结果按合并友好的方式写回 Global Memory

### 3. 数据复用
(TODO 这里的细节不要放弃了，还是需要继续理解的)

很多算法中，同一份数据会被多个线程多次使用。

以矩阵乘法 `C = A * B` 为例：

- 结果矩阵 C 的每个元素都是 A 的一行和 B 的一列的点积
- 如果不分 tile，A 的同一行会被 C 的同一行所有元素重复读取
- 使用 tile 后，A 的一个子块被加载到 Shared Memory，可以被当前 block 内的多个线程复用

---

## Tile-Based 计算的标准流程

一个典型的 tiled kernel 遵循以下模式：

```cpp
__global__ void tiled_kernel(float* input, float* output, int width) {
    // 1. 声明 Shared Memory 缓冲区
    __shared__ float tile[TILE_SIZE][TILE_SIZE];

    // 2. 计算当前线程负责的全局内存坐标
    int row = blockIdx.y * TILE_SIZE + threadIdx.y;
    int col = blockIdx.x * TILE_SIZE + threadIdx.x;

    // 3. 协作加载：整个 block 一起把当前 tile 从 Global Memory 搬到 Shared Memory
    tile[threadIdx.y][threadIdx.x] = input[row * width + col];

    // 4. 同步：确保整个 block 都加载完毕
    __syncthreads();

    // 5. 计算：从 Shared Memory 读取数据做计算
    //    此时访问模式可以是任意的（转置、邻域等），不影响 Global Memory 性能
    float result = compute(tile, threadIdx.x, threadIdx.y);

    // 6. 再同步（如果需要）
    __syncthreads();

    // 7. 写回：把结果按合并方式写回 Global Memory
    output[row * width + col] = result;
}
```

这个流程可以总结为三步：

1. **Load**：整个 block 协作，按合并访问把 tile 从 Global Memory 加载到 Shared Memory
2. **Sync**：`__syncthreads()` 保证 tile 完整就绪
3. **Compute & Store**：在 Shared Memory 上自由计算，然后合并写回 Global Memory

---

## 典型例子：矩阵转置

矩阵转置是最能体现 tile-based 价值的例子之一。

**朴素实现的问题**：

```cpp
// 读 input[row][col] 是合并的（连续线程读连续地址）
// 但写 output[col][row] 不是合并的（连续线程写相距很远的地址）
output[col * height + row] = input[row * width + col];
```

**Tile-based 优化**：

```cpp
__shared__ float tile[TILE_SIZE][TILE_SIZE + 1];  // +1 是为了避免 bank conflict

// Step 1: 合并读取 input，写入 shared memory tile
tile[threadIdx.y][threadIdx.x] = input[row * width + col];
__syncthreads();

// Step 2: 从 shared memory 转置读取，合并写入 output
// 注意这里 threadIdx.x/y 交换了，实现了转置
output[col * height + row] = tile[threadIdx.x][threadIdx.y];
```

关键点：

- 读 Global Memory 时是合并的
- 写 Global Memory 时也是合并的
- 转置操作发生在 Shared Memory 内部，不接触慢速 Global Memory
- `TILE_SIZE + 1` 的 padding 避免了 Shared Memory 的 bank conflict

---

## Tile 大小的选择

Tile 大小不是越大越好，需要考虑以下因素：

1. **Shared Memory 容量**：每个 SM 的 Shared Memory 有限（如 48KB、96KB、164KB）。如果一个 block 用太多，SM 上能同时驻留的 block 数就会减少，可能降低 occupancy。

2. **寄存器压力**：更大的 tile 通常意味着更多的中间变量和更复杂的索引计算，可能增加寄存器用量，同样会降低 occupancy。

3. **数据复用率**：tile 越大，单块内数据被复用的机会越多。但如果问题本身数据局部性不好，大 tile 的收益有限。

4. **warp 利用率**：tile 尺寸最好是 warp 大小（32）的倍数，这样每个 warp 都能满载工作。

常见的 tile 尺寸：16x16、32x32、64x8 等，具体取决于算法和硬件。

---

## Tile-Based 计算与 Pipeline 的关系

在 `04-pipelines` 章节中，pipeline 被描述为"管理多块缓冲区何时能写、何时能读、何时能复用"的协议。这与 tile-based 计算天然契合：

**朴素的 tile-based kernel**：

```
加载 tile 0 -> 等待 -> 计算 tile 0 -> 加载 tile 1 -> 等待 -> 计算 tile 1 -> ...
```

读数据和算数据是串行的。

**使用 Pipeline 的 tile-based kernel**：

```
预加载 tile 0
预加载 tile 1

循环：
    等待 tile N 就绪
    计算 tile N
    释放 tile N 的缓冲区
    同时发起 tile N+2 的加载
```

这样实现了：

- **正在计算当前 tile**
- **同时把下一个 tile 往 Shared Memory 里搬**

这就是 pipeline 化 tile-based 计算的核心价值：把"数据搬运"和"数据计算"真正重叠起来。

---

## 常见陷阱

### 1. 忘记 `__syncthreads()`

Shared Memory 是整个 block 共享的。如果线程 A 还在写 tile，线程 B 就开始读，结果是未定义的。必须在加载后和计算前同步。

### 2. Shared Memory Bank Conflict

Shared Memory 被分成 32 个 bank。如果同一个 warp 中的多个线程同时访问同一个 bank 的不同地址，访问会被串行化。

典型场景：

```cpp
__shared__ float tile[32][32];
// 当 warp 按列访问时：tile[0][threadIdx.x], tile[1][threadIdx.x], ...
// 连续线程访问的地址间隔 32*4 = 128 字节，全部落入同一个 bank
// 造成 32-way bank conflict
```

解决：加 padding，`tile[32][33]`。

### 3. 越界访问

Tile 的边缘可能超出实际数据边界。加载时需要判断：

```cpp
if (row < height && col < width) {
    tile[ty][tx] = input[row * width + col];
} else {
    tile[ty][tx] = 0;  // 或填充其他边界值
}
```

### 4. 把 Pipeline 当成万能药

Pipeline 能让 tile-based 计算更快的前提是：你的 kernel 真的有"搬运"和"计算"可以重叠。如果 tile 太小、计算太简单，pipeline 的开销可能比收益还大。

---

## 一句话总结

> Tile-based 计算就是：把大数据切成小块，搬进快速的 Shared Memory，让计算单元在"家门口"高效地复用数据。Pipeline 则是让"搬下一块"和"算当前块"同时进行，进一步榨干硬件并行度。
