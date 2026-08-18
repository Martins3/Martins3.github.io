# GPUDirect RDMA 及相关技术文档索引

> 整理自 NVIDIA 官方文档，涵盖网卡直接访问 GPU 内存（GPUDirect RDMA）、多 GPU 通信库（NCCL / NVSHMEM / MPI）以及底层网络驱动（Mellanox OFED / DOCA）的关键资料。
> 所链接均使用 HTTPS。

---

## 1. NVIDIA GPUDirect RDMA 开发者指南

| 文档 | 链接 | 说明 |
|------|------|------|
| GPUDirect RDMA — CUDA Docs (HTML) | https://docs.nvidia.com/cuda/gpudirect-rdma/ | **最权威的开发者指南**。涵盖工作原理、Userspace API (`cuPointerSetAttribute`、`cuPointerGetAttribute`)、Kernel API (`nvidia_p2p_get_pages`、`nvidia_p2p_dma_map_pages`)、同步与内存顺序、注册缓存设计等。 |
| GPUDirect RDMA — PDF 版 | https://docs.nvidia.com/cuda/pdf/GPUDirect_RDMA.pdf | 同上内容的 PDF 版本，适合离线阅读。 |
| NVIDIA GPUDirect RDMA User Manual v1.8 | https://docs.nvidia.com/networking/display/GPUDirectRDMAv18 | 网络产品线文档，侧重 IB/RoCE HCA 与 GPU 的对接。 |
| NVIDIA GPUDirect 官方概览 | https://developer.nvidia.com/gpudirect | GPUDirect 家族（RDMA / Storage / P2P / Video）的整体介绍。 |
| Minimal GPUDirect RDMA Demonstration (GitHub) | https://github.com/NVIDIA/jetson-rdma-picoevb | 带真实硬件示例的开源代码，包含内核驱动和 userspace 应用，适合想写驱动的人参考。 |

---

## 2. NCCL / NVSHMEM / MPI (OpenMPI + UCX) 文档与示例

### 2.1 NCCL

| 文档 | 链接 | 说明 |
|------|------|------|
| NCCL 官方文档主页 | https://docs.nvidia.com/deeplearning/nccl/ | 含 Release Notes、Installation Guide、User Guide。 |
| NCCL 使用示例 | https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/examples.html | 官方完整代码示例：单进程多卡、MPI 多进程单卡、MPI 多进程多卡、非阻塞 Communicator、AllReduce / Broadcast / SendRecv 等。 |
| NCCL Communicator 创建详解 | https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/usage/communicators.html | `ncclCommInitRank` / `ncclCommInitAll` / `ncclCommSplit` 的用法与故障恢复。 |

### 2.2 NVSHMEM

| 文档 | 链接 | 说明 |
|------|------|------|
| NVSHMEM API Examples | https://docs.nvidia.com/nvshmem/api/examples.html | 官方代码示例，包含基础 On-Stream 示例（含 MPI 集成）、`nvshmemx_float_put_block` 用法、Ring Broadcast / **Ring Allreduce**（针对 IB/RoCE 远程互联优化）、**GEMM + AllReduce Fused Kernel**（CUTLASS + NVSHMEM 融合通信计算）。 |

### 2.3 OpenMPI + UCX

| 文档 | 链接 | 说明 |
|------|------|------|
| Open MPI CUDA/GPUDirect 文档 | https://docs.open-mpi.org/en/v5.0.7/tuning-apps/networking/cuda.html | 讲解 OpenMPI 的 CUDA-aware 支持、如何通过 UCX 启用 GPUDirect RDMA、环境变量（`UCX_IB_GPU_DIRECT_RDMA`）和注册缓存调优。 |
| UCX GitHub Releases | https://github.com/openucx/ucx/releases | 下载带 CUDA/GDRcopy 支持的 UCX 二进制或源码。 |
| Open MPI + GPUDirect 技术讲座 (PDF) | https://www.open-mpi.org/video/general/easybuild_tech_talks_01_OpenMPI_part1_20200623.pdf | 配置 OpenMPI 与 UCX 以支持 GDR（GPUDirect RDMA）的入门资料。 |

---

## 3. Mellanox OFED / DOCA 相关文档

| 文档 | 链接 | 说明 |
|------|------|------|
| GPU Operator + GPUDirect RDMA 配置指南 | https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/gpu-operator-rdma.html | 在 Kubernetes 或裸金属上配置 GPUDirect RDMA 的完整步骤，包含 **MLNX_OFED / DOCA-OFED** 驱动安装、`nvidia-peermem` 模块加载、以及用 `cuda-perftest` 验证 RDMA 传输。 |
| DOCA RDMA Programming Guide | https://docs.nvidia.com/doca/sdk/DOCA-RDMA/index.html | 使用 DOCA SDK 编写 RDMA 应用的官方指南，涵盖连接建立、Write/Read/Atomic 任务。 |
| DOCA GPUNetIO Programming Guide | https://docs.nvidia.com/doca/archive/doca-v2.2.0/gpunetio-programming-guide/index.html | **GPU-centric 网络编程指南**，包含：<br>• **GPUDirect RDMA**（包直接收到 GPU 显存）<br>• **GPUDirect Async Kernel-Initiated Network (GDAKIN)**（CUDA kernel 直接控制网卡收发，无需 CPU 介入）<br>• GDRCopy、信号量、以太网协议管理。 |
| GPUDirect Storage Overview Guide | https://docs.nvidia.com/gpudirect-storage/overview-guide/index.html | 如果想扩展了解“存储设备直接读写 GPU 内存”，这是官方指南。 |

---

## 4. 快速建议的阅读顺序

1. **先读** [GPUDirect RDMA CUDA Docs](https://docs.nvidia.com/cuda/gpudirect-rdma/) 的第 1~3 章，理解 BAR 映射、`nvidia_p2p_get_pages` 和控制流。
2. **再看** [Open MPI CUDA 文档](https://docs.open-mpi.org/en/v5.0.7/tuning-apps/networking/cuda.html) 和 [NCCL 示例](https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/examples.html)，了解用户态库如何利用 GPUDirect。
3. **如果要写高性能内核或库**，深入看 [NVSHMEM Ring Allreduce 示例](https://docs.nvidia.com/nvshmem/api/examples.html) 和 [DOCA GPUNetIO](https://docs.nvidia.com/doca/archive/doca-v2.2.0/gpunetio-programming-guide/index.html)。
