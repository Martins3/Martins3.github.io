# DMA Trace for `memcpy_roundtrip.out`

本文档记录如何从内核 DMA API 的角度观察 CUDA memcpy demo。

观测时间: 2026-03-31
观测环境:

- 宿主机: Fedora 42
- 内核: `6.19.8-100.fc42.x86_64`
- GPU: `0000:01:00.0`, `NVIDIA GeForce GTX 1060 3GB`
- 工具: `bpftrace v0.24.1`

## 观测目标

目标不是看 CUDA Runtime API 本身，而是回答更底层的问题:

- NVIDIA 驱动是否真的走到了内核 DMA API
- `cudaMemcpy` 期间，内核里出现的是 `dma_map_phys()` 还是 `dma_map_sg()`
- 这些 DMA 映射大致呈现什么形态

## 运行方法

先运行过滤后的 trace:

```bash
cd /home/martins3/data/vn/gpu_demo/cuda
printf 'a\n' | sudo -S bash ./run_trace_dma_memcpy.sh
```

脚本会做两件事:

1. 启动 [trace_dma_memcpy.bt](/home/martins3/data/vn/gpu_demo/cuda/trace_dma_memcpy.bt)
2. 运行 `./memcpy_roundtrip.out`

输出文件:

- Trace 日志: [trace_dma_memcpy.log](/home/martins3/data/vn/gpu_demo/cuda/trace_dma_memcpy.log)
- Demo 日志: [trace_dma_memcpy.demo.log](/home/martins3/data/vn/gpu_demo/cuda/trace_dma_memcpy.demo.log)

## 这次实际跑到的结果

Demo 本身成功:

```text
[OK] memcpy roundtrip passed with 4194304 bytes
```

Trace 里只保留了 GPU `0000:01:00.0` 的 DMA 事件。末尾统计如下:

```text
@map_sg_count: 23
@unmap_sg_count: 20
@sync_dev_count: 0
@sync_cpu_count: 0
@map_phys_count: 551
@map_phys_bytes: 2428928
@unmap_phys_count: 551
@unmap_phys_bytes: 2428928
```

日志中可以看到典型片段:

```text
MAP_SG dev=0000:01:00.0 dir=0 nents=105 ents=105 truncated=0
MAP_SG dev=0000:01:00.0 dir=0 nents=2178 ents=2178 truncated=1
MAP_SG dev=0000:01:00.0 dir=0 nents=4098 ents=4098 truncated=1
MAP_SG dev=0000:01:00.0 dir=0 nents=5998 ents=5998 truncated=1
MAP_PHYS dev=0000:01:00.0 dir=0 size=65536
MAP_PHYS dev=0000:01:00.0 dir=0 size=4096
UNMAP_SG dev=0000:01:00.0 dir=0
UNMAP_PHYS dev=0000:01:00.0 dir=0 size=65536
```

其中 `dir=0` 在当前内核中对应 `DMA_BIDIRECTIONAL`。

## 如何理解这些事件

可以得到几个直接结论:

1. `cudaMemcpy` 确实会把 NVIDIA GPU 驱动带到 Linux DMA API 层。
2. 这条路径里同时存在 `dma_map_phys()` 和 `dma_map_sg()`。
3. `dma_map_sg()` 出现了很大的 `nents`，说明驱动确实在处理离散页组成的 scatter-gather 列表，而不是只做单一连续物理页映射。
4. `dma_sync_sg_for_cpu/device` 没出现，至少在这次路径里没有走到这些 tracepoint。

更细一点地看:

- 大量 `MAP_PHYS size=4096` 更像是驱动内部的页级别 DMA 映射。
- `MAP_PHYS size=16384`, `24576`, `65536` 这种更像较大的内部缓冲区或分配单元。
- `MAP_SG nents=105/2178/4098/5998` 表明驱动在某些阶段会把很多离散页组织成 SG 表，然后交给 DMA 层建立设备可访问的 DMA 地址视图。

这说明 `cudaMemcpy` 并不是一个单纯的“用户态 memcpy 换个目标地址”。
内核侧至少参与了:

- host pages 的组织与映射
- SG 表建立
- DMA address 的建立与回收
- 设备侧缓冲区或内部 staging/bounce buffer 的管理

## 机制上的解释

从当前 trace 可以做出一个保守、但和现象一致的解释:

1. 用户态调用 `cudaMemcpy()`
2. NVIDIA 内核驱动准备传输所需的 host/device 内存描述
3. 对某些连续物理区域走 `dma_map_phys()`
4. 对离散页集合走 `dma_map_sg()`
5. GPU 通过 DMA engine 访问这些已经映射好的 DMA 地址
6. 传输结束后，驱动调用 `dma_unmap_phys()` / `dma_unmap_sg()` 回收映射

这里不能仅凭这份 trace 就断言“4 MiB host buffer 一定完整地直接映射成某一个 SG 表”。
因为当前 demo 使用的是普通 `std::vector`，不是显式 pin 的 host memory，而且 NVIDIA 闭源主体驱动内部还可能有:

- staging buffer
- 分批传输
- 额外的控制面 DMA
- 自己维护的页管理和映射缓存

所以更合理的说法是:
这份 trace 证明了 `cudaMemcpy` 的确触发了 GPU 设备相关的 DMA map/unmap 行为，并且其中包含 SG 映射，但不能仅凭这份 trace 还原每一个字节的完整搬运路径。

## 一个重要注意点

[memcpy_roundtrip.cu](/home/martins3/data/vn/gpu_demo/cuda/memcpy_roundtrip.cu) 的修改时间晚于 [memcpy_roundtrip.out](/home/martins3/data/vn/gpu_demo/cuda/memcpy_roundtrip.out)。

也就是说，这次 trace 实际观测的是当前磁盘上的 `memcpy_roundtrip.out` 二进制，而不是严格保证与当前源码完全一致的一次新编译结果。写结论时要以“实际跑到的二进制”作为准绳。

## 后续可以怎么继续

如果要继续把机制看深，可以做两类增强:

1. 在 `dma:*` tracepoint 之外，再挂 NVIDIA 相关 `kprobe`，把 `nvidia_ioctl` 和 `dma_map_*` 串起来。
2. 把 demo 改成显式使用 pinned host memory，例如 `cudaHostAlloc()`，再比较 trace 形态是否变化。
