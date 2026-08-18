## 1. **Memory Management 优化**（核心基础）

既然你已经掌握了执行流程优化（Graphs），下一步应该深入内存管理：

- **Unified Memory / Managed Memory**：`cudaMallocManaged`，理解 page fault、oversubscription、prefetching（`cudaMemPrefetchAsync`）
- **Memory Pools & Async Allocation**：CUDA 11.3+ 引入的 `cudaMallocAsync`/`cudaFreeAsync`，配合 memory pools 减少分配开销
- **Host Memory 优化**：Pinned memory（`cudaMallocHost`）、Write-Combining、Mapped memory
- **Multi-Device Memory**：Peer-to-peer access、Unified Memory 跨 GPU 行为

---

## 2. **CUDA Streams & Concurrency**（与 Graphs 密切相关）

Graphs 底层依赖 Streams，深入理解 Streams 能让你更好地使用 Graphs：

- **Stream Priorities**：`cudaStreamCreateWithPriority`
- **Blocking vs Non-blocking Streams**：默认流的隐式同步陷阱
- **Stream Callbacks**：`cudaLaunchHostFunc`（你已经在 Graphs 的 Host Node 中见过）
- **Event-based Synchronization**：`cudaEventRecord`/`cudaStreamWaitEvent`，理解 elapsed time 测量

---

## 3. **Advanced Kernel Optimization**（性能关键）

- **Dynamic Parallelism**：Kernel 中启动子 Kernel（`<<< >>>` in device code），注意 Graphs 中对此有限- **Cooperative Groups**：`cg::this_thread_block()`，实现更灵活的线程协作模式
- **Warp-level Primitives**：`__shfl_sync`, `__ballot_sync`, `__reduce_add_sync` 等，用于高性能 reduction、scan
- **Tensor Cores & WMMA**：如果你做深度学习或 HPC，学习使用 Tensor Cores 进行混合精度计算
- **CUDA Math API**：intrinsics (`__sinf`, `__expf`) vs standard math，精度与速度权衡

---

## 4. **Multi-GPU & Scaling**

- **Multi-GPU Programming**：`cudaSetDevice`，device enumeration，P2P memory access
- **NVLink & NVSwitch**：理解带宽拓扑，优化多 GPU 通信模式
- **NCCL**（NVIDIA Collective Communications Library）：多 GPU 集合通信（AllReduce, Broadcast 等），训练大模型必备

---

## 5. **Interoperability & Integration**

- **CUDA with MPI**：大规模集群上的 CUDA 程序
- **CUDA with OpenMP**：Host 端并行 + Device 端并行
- **CUDA Python**（Numba, CuPy, PyCUDA）：快速原型验证
- **External Resource Interop**：
  - **Vulkan/OpenGL/DirectX Interop**：`cudaExternalMemoryHandleDesc`，图形与计算结合
  - **D3D12/Vulkan 外部信号量**：Graphs 文档中提到的 External Semaphore

---

## 6. **Profiling & Debugging Tools**（工程化必备）

- **Nsight Compute**：Kernel 级性能分析，occupancy、memory throughput、指令混合
- **Nsight Systems**：系统级时间线分析，看 CPU-GPU 交互、Stream 并发
- **cuda-gdb**：命令行调试
- **Compute Sanitizer**：`cuda-memcheck` 的继任者，检测内存错误、race condition

---

## 7. **Specialized Topics**（根据应用场景选择）

| 领域 | 技术点 |
|------|--------|
| **深度学习** | cuDNN, cuBLAS, cuDNN Graph API, CUTLASS |
| **图计算** | cuGraph, Gunrock |
| **信号处理** | cuFFT, cuDNN |
| **线性代数** | cuBLAS, cuSOLVER, cuSPARSE |
| **光线追踪** | OptiX |
| **视频编解码** | Video Codec SDK (NVDEC/NVENC) |

---

## 8. **最新 CUDA 特性**（保持更新）

- **CUDA 12.x 新特性**：
  - **Lazy Loading**：延迟加载 Kernel，减少启动时间
  - **Stream-ordered Memory Allocator**：前面提到的 memory pools
  - **FP8/FP16/BF16 支持**：低精度训练与推理
  - **Graphs 增强**：你已经看到的 Conditional Nodes、Programmatic Dependent Launch
