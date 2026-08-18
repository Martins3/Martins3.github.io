## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/multi-gpu-systems.html>

## Programming Systems with Multiple GPUs 章节总结

### 主要内容转述

本章节介绍如何在配备多张 GPU 的系统上进行 CUDA 编程。多 GPU 编程的核心目标是利用多张卡聚合起来的算力、显存容量和带宽，处理单卡无法承载的问题规模或性能瓶颈。

CUDA 为 multi-GPU 提供了一系列底层机制和高层抽象：
- **主机线程 CUDA 上下文管理**：通过切换当前设备，让单个或多个线程控制不同的 GPU。
- **统一虚拟寻址 (UVA)**：同一进程内的所有 GPU 共享统一的虚拟地址空间，简化跨设备指针传递。
- **Peer-to-Peer (P2P) 批量内存传输**：直接使用 `cudaMemcpyPeer` 等设备到设备拷贝。
- **P2P 细粒度 load/store**：在拓扑支持的情况下，一个 GPU 上的 kernel 可以直接解引用另一张 GPU 的显存指针。
- **高层通信库**：如 NCCL（集合通信）、NVSHMEM、MPI 结合 GPU-Direct RDMA 等。

常见的多 GPU 编程模式包括：
1. 单主机线程控制多张 GPU。
2. 多主机线程，每个线程独占一张 GPU。
3. 多进程（单线程），每个进程独占一张 GPU。
4. 多进程多线程混合模式。
5. 多节点 NVLink 集群。

### 核心概念

| 概念 | 说明 |
|------|------|
| **设备枚举与选择** | 使用 `cudaGetDeviceCount` 和 `cudaGetDeviceProperties` 枚举设备；使用 `cudaSetDevice` 设置当前线程的工作设备。 |
| **多设备流与事件** | 流和事件与创建时的设备绑定。Kernel 只能发射到属于当前设备的流；`cudaStreamWaitEvent` 可以跨设备同步。 |
| **P2P 内存传输** | `cudaMemcpyPeer` / `cudaMemcpyPeerAsync` 用于设备间显存拷贝。若启用了 P2P 访问，拷贝可绕过主机内存直接进行。 |
| **P2P 内存访问** | 通过 `cudaDeviceCanAccessPeer` 查询拓扑支持，再用 `cudaDeviceEnablePeerAccess` 开启。开启后 kernel 可直接访问 peer 设备指针。 |
| **P2P 内存一致性** | 跨设备并发访问需要 `thread_scope_system` 级别的同步；CUDA atomic 操作在仅一个 GPU 访问该对象时可用于 peer 显存。 |

### 关键 API / 函数

- `cudaGetDeviceCount(int *count)` — 获取系统中 CUDA 设备数量。
- `cudaGetDeviceProperties(cudaDeviceProp *prop, int device)` — 查询指定设备的属性。
- `cudaSetDevice(int device)` — 设置当前线程的活跃设备。
- `cudaStreamCreate(cudaStream_t *stream)` — 在当前设备上创建流。
- `cudaEventRecord(cudaEvent_t event, cudaStream_t stream)` — 记录事件；**要求事件与流属于同一设备**。
- `cudaStreamWaitEvent(cudaStream_t stream, cudaEvent_t event, unsigned int flags)` — 让流等待事件；**支持跨设备**。
- `cudaMemcpyPeer(void *dst, int dstDevice, const void *src, int srcDevice, size_t count)` — 同步设备间显存拷贝。
- `cudaMemcpyPeerAsync(..., cudaStream_t stream)` — 异步设备间显存拷贝。
- `cudaDeviceCanAccessPeer(int *canAccessPeer, int device, int peerDevice)` — 查询两张卡是否支持 P2P 访问。
- `cudaDeviceEnablePeerAccess(int peerDevice, unsigned int flags)` — 开启对指定 peer 设备的直接显存访问。

### 注意事项与常见陷阱

1. **Kernel 发射的流必须与当前设备匹配**。如果把 device 0 的流当成当前设备是 device 1 时的发射目标，kernel 启动会失败。
2. **`cudaEventRecord` 要求事件和流在同一设备**。跨设备记录事件会失败。
3. **`cudaEventElapsedTime` 要求两个事件在同一设备**。跨设备计时无效。
4. **`cudaStreamWaitEvent` 可以跨设备使用**，这是同步多张 GPU 的常用手段。
5. **默认流（legacy default stream）每张设备各有一个**，不同设备的默认流之间可以并发执行。
6. **P2P 访问默认关闭**，必须显式调用 `cudaDeviceEnablePeerAccess` 开启。开启后会对 peer 设备的所有显存分配产生额外运行时开销（需要让这些分配立即对当前设备可见）。
7. **P2P 连接数限制**：在非 NVSwitch 平台上，每张 GPU 最多支持系统范围内 **8 个 peer 连接**。
8. **更 scalable 的替代方案**：使用 CUDA Virtual Memory Management (VMM) API，在分配时显式指定 peer-accessible 区域，避免对所有分配都增加 P2P 开销。
9. **设备间拷贝的隐式同步**：在 NULL stream（默认流）中执行 `cudaMemcpyPeer` 会等待两张设备上此前所有命令完成，并且阻塞两张设备上后续命令，直到拷贝结束。
