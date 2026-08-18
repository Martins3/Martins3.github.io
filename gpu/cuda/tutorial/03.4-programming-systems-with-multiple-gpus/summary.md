## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/multi-gpu-systems.html>

## 3.4. Programming Systems with Multiple GPUs 核心概念总结

### 核心概念

1. **多 GPU 编程的价值**  
   通过聚合多个 GPU 的算力、显存容量和带宽，解决单 GPU 无法处理的问题规模，或达到更高的性能水平。

2. **常见多 GPU 编程模式**  
   CUDA 支持多种组织方式，开发者可根据算法并行度、代码结构选择最合适的映射：
   - 单个主机线程驱动多个 GPU；
   - 多个主机线程，每个线程独占一个 GPU；
   - 多个单线程进程，每个进程独占一个 GPU；
   - 多进程多线程混合；
   - 多节点 NVLink 集群，跨操作系统实例驱动 GPU。

3. **CUDA 提供的多 GPU 基础能力**  
   - 主机线程 CUDA 上下文管理（`cudaSetDevice` 等）；
   - 统一虚拟地址空间（UVA），同一进程内多 GPU 共享地址空间；
   - 点对点（Peer-to-Peer，P2P）批量显存传输；
   - 细粒度 P2P GPU load/store 直接内存访问；
   - 更高级抽象由 NCCL、NVSHMEM、MPI 等库提供，CUDA 本身不直接提供多 GPU 集合通信 API。

4. **跨进程多 GPU 通信**  
   同一主机进程内多 GPU 通过 UVA + P2P 即可高效通信；不同主机进程间则依赖 CUDA IPC（Interprocess Communication）和 Virtual Memory Management（VMM）API。

---

### 关键 API / 函数 / 宏

| API / 宏 | 用途说明 |
|----------|----------|
| `cudaGetDeviceCount(int *count)` | 查询系统中 CUDA 可用设备数量。 |
| `cudaGetDeviceProperties(cudaDeviceProp *prop, int device)` | 查询指定设备的属性（计算能力、显存大小、多处理器数量等）。 |
| `cudaSetDevice(int device)` | 设置当前主机线程的当前设备；后续显存分配、内核启动、流/事件创建均绑定该设备。 |
| `cudaMalloc(void **devPtr, size_t size)` | 在当前设备上分配设备显存。 |
| `cudaStreamCreate(cudaStream_t *stream)` | 创建与**当前设备**绑定的异步流。 |
| `cudaEventRecord(cudaEvent_t event, cudaStream_t stream)` | 在指定流中记录事件；**事件与流必须属于同一设备，否则失败**。 |
| `cudaEventElapsedTime(float *ms, cudaEvent_t start, cudaEvent_t end)` | 计算两个事件之间的时间；**两个事件必须属于同一设备，否则失败**。 |
| `cudaEventSynchronize(cudaEvent_t event)` | 阻塞直到事件完成；**可跨设备成功**。 |
| `cudaEventQuery(cudaEvent_t event)` | 查询事件是否已完成；**可跨设备成功**。 |
| `cudaStreamWaitEvent(cudaStream_t stream, cudaEvent_t event, unsigned int flags)` | 让流等待某个事件；**可跨设备成功**，是实现多 GPU 同步的关键 API。 |
| `cudaMemcpyPeer(void *dst, int dstDevice, const void *src, int srcDevice, size_t count)` | 同步 P2P 显存拷贝。 |
| `cudaMemcpyPeerAsync(..., cudaStream_t stream)` | 异步 P2P 显存拷贝，可与其他流中的操作重叠。 |
| `cudaMemcpy3DPeer` / `cudaMemcpy3DPeerAsync` | 三维数组的 P2P 拷贝。 |
| `cudaDeviceCanAccessPeer(int *canAccessPeer, int device, int peerDevice)` | 查询两个设备之间是否支持 P2P 直接内存访问（取决于 PCIe/NVLink 拓扑）。 |
| `cudaDeviceEnablePeerAccess(int peerDevice, unsigned int flags)` | 显式启用当前设备对 `peerDevice` 的 P2P 访问。 |
| `cudaMemcpy(..., cudaMemcpyDeviceToDevice)` / `cudaMemcpyDefault` | 标准内存拷贝 API 也可用于设备间拷贝。 |

---

### 注意事项、限制条件与常见陷阱

#### 流与设备的严格绑定
- **内核启动**如果目标流不属于**当前设备**，会**失败**。  
- **显存拷贝**即使目标流不属于当前设备，也会**成功**。  
- 每个设备有自己的**默认流（NULL stream）**，不同设备的默认流之间可以**乱序或并发执行**。

#### 事件跨设备行为
- `cudaEventRecord`：事件与流必须同设备，否则失败。  
- `cudaEventElapsedTime`：两个事件必须同设备，否则失败。  
- `cudaEventSynchronize` / `cudaEventQuery`：即使事件属于其他设备也能成功。  
- `cudaStreamWaitEvent`：流与事件可以属于不同设备，因此是**多 GPU 同步的推荐手段**。

#### P2P 拷贝的同步语义
- 在**隐式 NULL 流**上执行的两设备间拷贝：  
  1. 必须等待两个设备上此前所有命令完成后才开始；  
  2. 拷贝完成前，两个设备上后续所有命令都不能开始。  
- 在**非 NULL 流**上的异步 P2P 拷贝可以与其它流中的内核或拷贝**并发/重叠**。
- 若已启用 P2P 访问，P2P 拷贝无需经过主机内存中转，速度更快。

#### P2P 访问的连接数限制
- 在非 NVSwitch 系统上，**每个设备最多支持系统范围内 8 个 P2P peer 连接**。

#### `cudaDeviceEnablePeerAccess` 的性能副作用
- 该调用会对 peer 设备上**所有已有和后续的显存分配**产生全局影响。
- 每次在该 peer 设备上执行 `cudaMalloc` 时，都需要确保新分配对当前设备（及其它已启用访问的 peer）立即可见，导致**显存分配运行时开销随 peer 数量乘性增长**。
- **更 scalable 的替代方案**：使用 CUDA Virtual Memory Management (VMM) API，仅在需要时显式分配 peer-accessible 内存，避免全局开销。

#### IOMMU、ACS 与虚拟机（Linux 特有限制）
- **Linux 裸机**：若启用了 IOMMU，CUDA **不支持** PCIe P2P 传输，可能导致**静默的显存损坏**；必须在裸机上禁用 IOMMU。  
- **虚拟机**：可通过 PCIe pass-through（VFIO 驱动）使用 IOMMU，此时 P2P 受支持。  
- **Windows**：不存在上述 IOMMU 限制。
- **PCI Access Control Services (ACS)**：启用后会将所有 PCI 点对点流量重定向到 CPU root complex，**显著降低 P2P 有效带宽**。

---

### 底层原理与硬件行为

#### 1. 统一虚拟地址空间（Unified Virtual Addressing, UVA）
在同一主机进程内，所有 GPU 设备共享一个统一的虚拟地址空间。这意味着：  
- 同一块显存指针可以在多个设备上直接使用（只要启用了 P2P 访问）；  
- 内核可以直接解引用（dereference）指向另一设备显存的指针，无需通过主机中转。

#### 2. P2P 内存访问的硬件基础
设备之间能否直接访问对方显存，取决于系统的 **PCIe 拓扑和/或 NVLink 连接**：  
- `cudaDeviceCanAccessPeer` 查询的就是这一硬件能力；  
- NVLink 提供比 PCIe 更高的带宽和更低的延迟，支持更细粒度的 load/store 操作；  
- CUDA 驱动会利用专门的**拷贝引擎（copy engines）**和 NVLink 硬件来最大化 P2P 传输性能。

#### 3. 跨设备内存一致性模型
- 分布在多个设备上的并发线程网格必须通过**显式同步**来保证内存访问的顺序和正确性；  
- 跨设备同步操作属于 **`thread_scope_system`** 同步范围；  
- 相应的内存操作也属于 **`thread_scope_system`** 内存同步域；  
- 使用 `cudaStreamWaitEvent` 或设备级同步 API 可建立跨设备的 happens-before 关系。

#### 4. 跨设备原子操作
- CUDA 原子函数（如 `atomicAdd`）**可以**对 peer 设备内存执行读-改-写操作；  
- 但限制条件是：**仅当单个 GPU 访问该对象时**才能保证原子性；  
- 若多个设备并发访问同一 peer 内存位置的原子操作，需要参考 CUDA C++ 内存模型中更严格的原子性要求（通常需要显式同步或特定的内存顺序约束）。
