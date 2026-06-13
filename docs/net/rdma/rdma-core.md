# RDMA Core 详解

- 官方仓库：<https://github.com/linux-rdma/rdma-core>

## 什么是 RDMA Core

RDMA Core 是 Linux 内核 RDMA 子系统的**用户态组件**，提供用户空间程序访问 RDMA 硬件的能力。它本身不替代内核功能，而是作为**用户态通往 RDMA 硬件的桥梁**。

```
用户态程序 (SPDK, NCCL, MPI, Redis...)
         │
         ▼
┌─────────────────────────────────────┐
│          RDMA Core (用户态)          │
│  ┌──────────────┐ ┌──────────────┐  │
│  │ libibverbs   │ │ librdmacm    │  │
│  │ (数据传输)    │ │ (连接管理)    │  │
│  └──────────────┘ └──────────────┘  │
│  ┌────────────────────────────────┐ │
│  │ providers/ (mlx5, mlx4, efa...)│ │
│  │ 用户态驱动部分，直接硬件访问     │ │
│  └────────────────────────────────┘ │
└────────────┬────────────────────────┘
             │ /dev/infiniband/uverbsX
             │ (ioctl / mmap)
             ▼
┌─────────────────────────────────────┐
│        内核 RDMA 子系统              │
│    drivers/infiniband/core/         │
│    drivers/infiniband/hw/mlx5/      │
└─────────────────────────────────────┘
```

## 核心组件

### 1. 主要库

| 库 | 功能 |
|---|---|
| **libibverbs** | 提供 IB Verbs API，是 RDMA 编程的核心接口（设备管理、QP/CQ/MR 等） |
| **librdmacm** | 提供 RDMA 连接管理（Connection Manager），处理地址解析、连接建立 |
| **libibumad** | 提供 IB 管理数据报文接口（MAD），用于诊断和管理工具 |

### 2. 设备驱动用户态部分 (providers/)

```
providers/
├── mlx5/          # Mellanox ConnectX-5/6/7 网卡驱动用户态部分
├── mlx4/          # Mellanox ConnectX-3 网卡驱动
├── irdma/         # Intel E810 等网卡驱动
├── efa/           # AWS EFA (Elastic Fabric Adapter)
├── rxe/           # Soft-RoCE (软件 RoCE)
├── siw/           # Soft-iWARP
└── ...            # 其他厂商驱动
```

## 控制路径 vs 数据路径分离

RDMA Core 的关键设计：**控制路径在内核，数据路径在用户态**。

### 需要内核的操作（控制路径）

| 操作 | 为什么需要内核 |
|------|---------------|
| 设备发现/初始化 | 内核创建设备文件 `/dev/infiniband/uverbsX` |
| 创建 QP/CQ | 内核分配硬件资源，检查权限 |
| **内存注册 (MR)** | 内核 pin 住内存（防止 swap），建立 DMA 映射 |
| 连接建立 | 内核处理网络层面的连接协商 |
| 销毁资源 | 内核回收硬件资源 |

```c
// 这些操作通过 ioctl 进入内核
struct ibv_qp *qp = ibv_create_qp(pd, &attr);     // ← 内核协助
struct ibv_mr *mr = ibv_reg_mr(pd, buf, size, ...); // ← 内核 pin 内存
```

### 不需要内核的操作（数据路径）

| 操作 | 实现方式 |
|------|----------|
| `post_send` (发送) | 用户态直接写硬件 doorbell 寄存器（MMIO）|
| `post_recv` (接收) | 用户态直接写硬件 doorbell 寄存器（MMIO）|
| `poll_cq` (轮询完成) | 用户态直接读取内存映射的 CQ |
| RDMA Read/Write | 网卡硬件直接操作，CPU 不介入 |

```c
// 这些操作直接访问 mmap 映射的硬件，无系统调用
ibv_post_send(qp, &wr, &bad_wr);    // ← 用户态直接写 doorbell
ibv_poll_cq(cq, num_entries, wc);   // ← 用户态直接读内存
```

## RDMA Core 与内核的关系

### 关键机制：mmap

RDMA Core 通过 `mmap` 将内核准备好的资源映射到用户态，之后独立操作：

```
1. 创建阶段（进入内核）
   ibv_create_cq() → ioctl → 内核分配资源 → 返回 fd

2. 映射阶段
   mmap(fd) → 用户态获得直接访问权限

3. 使用阶段（纯用户态）
   ibv_poll_cq() → 直接读内存，不进内核
```

### 与 DPDK 的区别

| 特性 | DPDK | RDMA (rdma-core) |
|------|------|------------------|
| 内核依赖 | 完全独立，通过 VFIO 直接控制 | 需要内核初始化，部分操作依赖内核 |
| 内存管理 | 自己管理 hugepage | 需要内核 pin 内存（`ibv_reg_mr`）|
| 设备发现 | 扫描 PCI 总线 | 依赖内核创建的设备文件 |
| 安全性 | 应用自己负责 | 内核检查权限 |

## 为什么内核 NVMe-oF 不需要 RDMA Core

内核态驱动直接使用**内核原生的 RDMA API**，与 rdma-core 完全无关：

```c
// 内核态 NVMe-oF (drivers/nvme/target/rdma.c)
#include <rdma/ib_verbs.h>      // 内核头文件
#include <rdma/rdma_cm.h>       // 内核连接管理

struct ib_qp *qp = ib_create_qp(pd, &attr);  // 直接内核函数调用
ib_post_send(qp, &wr, &bad_wr);              // 直接内核函数调用
```

对比用户态 SPDK：

```c
// 用户态 SPDK 通过 rdma-core
#include <infiniband/verbs.h>   // 用户态头文件

struct ibv_qp *qp = ibv_create_qp(pd, &attr);  // ioctl 到内核
ibv_post_send(qp, &wr, &bad_wr);               // 用户态直接写 doorbell
```

### 使用方对比

| 使用方 | 调用的 API | 是否需要 rdma-core | 运行位置 |
|--------|-----------|-------------------|----------|
| 内核 NVMe-oF | `<rdma/ib_verbs.h>` 内核 API | **不需要** | 内核态 |
| SPDK | `<infiniband/verbs.h>` libibverbs | **需要** | 用户态 |
| NCCL | `<infiniband/verbs.h>` libibverbs | **需要** | 用户态 |
| Lustre/CIFS | 内核 RDMA API | **不需要** | 内核态 |

## RDMA Core 的核心价值

RDMA Core 像 RDMA 领域的 **"libc"**——**只为用户态服务**：

1. **系统调用封装**：通过 `/dev/infiniband/uverbsX` 与内核通信
2. **mmap 管理**：将内核资源映射到用户态，实现零拷贝访问
3. **用户态 doorbell**：提供用户态直接写硬件的能力（性能关键）
4. **统一 API**：封装不同厂商硬件的差异

### 一句话总结

> **rdma-core 是"用户态通往 RDMA 硬件的桥梁"**
>
> - 内核驱动直接调用内核 RDMA API，**不需要** rdma-core
> - 用户态程序**必须**通过 rdma-core 才能使用 RDMA 硬件
> - 控制路径（资源管理）走内核，数据路径（实际传输）走用户态

## 内核中的 rdma 的代码分布

1. RoCEv2 相关代码

核心 RoCE 代码

 文件/目录                                 说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 drivers/infiniband/core/roce_gid_mgmt.c   RoCE GID (全局标识符) 管理
 drivers/infiniband/sw/rxe/                Soft RoCE - 软件实现的 RoCEv2，可在任何以太网卡上运行

硬件驱动 (RoCEv2 网卡驱动)

 目录                             厂商/芯片
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 drivers/infiniband/hw/mlx4/      Mellanox ConnectX-3 系列
 drivers/infiniband/hw/mlx5/      Mellanox ConnectX-4/5/6/7 系列
 drivers/infiniband/hw/bnxt_re/   Broadcom RoCE (BNXT 系列)
 drivers/infiniband/hw/irdma/     Intel RDMA (i40e, ice 网卡)
 drivers/infiniband/hw/qedr/      QLogic/Marvell RoCE
 drivers/infiniband/hw/hns/       华为 RoCE (HiNIC)
 drivers/infiniband/hw/erdma/     阿里/其他 RDMA
 drivers/infiniband/hw/ocrdma/    Emulex RoCE

──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
2. RDMA 标准/核心代码

核心子系统

 目录                       说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 drivers/infiniband/core/   RDMA 核心层 (348+ 个文件)
 include/rdma/              内核 RDMA 头文件
 include/uapi/rdma/         用户空间 API 头文件 (应用程序接口)

关键核心文件

drivers/infiniband/core/
├── verbs.c          # RDMA Verbs 核心实现
├── cma.c            # Connection Manager (连接管理)
├── cm.c             # Communication Manager
├── mad.c            # Management Datagram
├── sa_query.c       # Subnet Administrator query
├── uverbs_*.c       # 用户空间 verbs 接口
├── rdma_core.c      # RDMA 核心功能
└── addr.c           # 地址解析

ULP (上层协议)

 目录                            协议
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 drivers/infiniband/ulp/ipoib/   IP over IB (IPoIB)
 drivers/infiniband/ulp/iser/    iSCSI over RDMA
 drivers/infiniband/ulp/srpt/    SCSI RDMA Protocol Target
 drivers/infiniband/ulp/rtrs/    RDMA Transport

──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
3. 相关文档

Documentation/driver-api/infiniband.rst       # RDMA 内核 API 文档
Documentation/networking/device_drivers/ethernet/mellanox/mlx5/  # MLX5 文档
Documentation/admin-guide/nfs/nfs-rdma.rst    # NFS over RDMA

──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
4. 外部标准规范

RDMA 和 RoCEv2 的标准由以下组织定义：

 标准                           组织          描述
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 InfiniBand Architecture Spec   IBTA          InfiniBand 基础规范
 RoCEv2 Spec                    IBTA          RDMA over Converged Ethernet v2
 RDMA Verbs                     OFED / IBTA   编程接口标准
 iWARP (RFC 5040-5044)          IETF          Internet Wide Area RDMA Protocol
 libibverbs                     Linux-RDMA    用户空间库 (用户态)

## nfs over rdma 如何理解

NFS over RDMA 通过绕过传统 TCP/IP 协议栈，利用 RDMA 的零拷贝能力，实现了极高性能的 NFS 存储访问。内核中的实现主要分为客户端 (x
prtrdma) 和服务端 (svc_rdma) 两部分，通过高效的 FRWR 内存注册机制，最小化了内存管理开销。

## RDMA 是基于哪一层的
<!-- feaf3a32-28ca-4cf7-a6f0-9253320aaf44 -->

1. RoCEv2 与 UDP 的关系

RoCEv2 确实使用了 UDP 作为封装层，但这只是网络传输的需要：

RoCEv2 数据包结构：
┌─────────┬─────────┬─────────┬─────────┬─────────────────┐
│ Eth Hdr │ IP Hdr  │ UDP Hdr │ RoCEv2  │    RDMA Payload │
│         │         │ Dst=4791│  Header │                 │
└─────────┴─────────┴─────────┴─────────┴─────────────────┘
        ↑                                ↑
    路由可达性                      真正的 RDMA 语义
    (使用 IP/UDP)                   (Send/Write/Read/Atomic)

关键点：

• RoCEv2 使用 UDP 仅为了在 IP 网络中路由
• UDP 端口号固定为 4791
• RDMA 的操作语义（Send/Recv/RDMA Write/RDMA Read/Atomic）与 UDP 无关

## 3. Soft RoCE (RXE) 实现分析

1. 发送路径

```c
// drivers/infiniband/sw/rxe/rxe_net.c:360-384
static int prepare4(struct rxe_av *av, struct rxe_pkt_info *pkt,
                    struct sk_buff *skb)
{
    // ... 路由查找 ...

    // 步骤 1: 软件构建 UDP 头部
    prepare_udp_hdr(skb, cpu_to_be16(qp->src_port),
                    cpu_to_be16(ROCE_V2_UDP_DPORT));  // 4791

    // 步骤 2: 软件构建 IP 头部
    prepare_ipv4_hdr(dst, skb, saddr->s_addr, daddr->s_addr,
                     IPPROTO_UDP,  // 协议字段设置为 UDP
                     av->grh.traffic_class, av->grh.hop_limit, df, xnet);

    // ... 发送 ...
}

// UDP 头部构建
static void prepare_udp_hdr(struct sk_buff *skb, __be16 src_port,
                            __be16 dst_port)
{
    struct udphdr *udph;

    __skb_push(skb, sizeof(*udph));
    skb_reset_transport_header(skb);
    udph = udp_hdr(skb);

    udph->dest = dst_port;      // 4791
    udph->source = src_port;    // 动态源端口
    udph->len = htons(skb->len);
    udph->check = 0;            // RoCEv2 不使用 UDP 校验和
}
```

2. 接收路径

```c
// drivers/infiniband/sw/rxe/rxe_net.c:214-252
static int rxe_udp_encap_recv(struct sock *sk, struct sk_buff *skb)
{
    struct udphdr *udph;
    struct rxe_pkt_info *pkt = SKB_TO_PKT(skb);

    // 获取 UDP 头部
    udph = udp_hdr(skb);

    // 关键: 跳过 UDP 头部，直接指向 RDMA Payload
    pkt->hdr = (u8 *)(udph + 1);  // 指向 BTH/GRH 开始位置
    pkt->mask = RXE_GRH_MASK;
    pkt->paylen = be16_to_cpu(udph->len) - sizeof(*udph);

    // 移除 UDP 头部
    skb_pull(skb, sizeof(struct udphdr));

    // 处理 RDMA 数据包
    rxe_rcv(skb);
    return 0;
}
```

3. 初始化 UDP 监听

```c
// drivers/infiniband/sw/rxe/rxe_net.c:713-735
recv_sockets.sk4 = rxe_setup_udp_tunnel(&init_net,
                htons(ROCE_V2_UDP_DPORT), false);  // IPv4
recv_sockets.sk6 = rxe_setup_udp_tunnel(&init_net,
                htons(ROCE_V2_UDP_DPORT), true);   // IPv6
```

## 4. 硬件 RDMA (Mellanox mlx5) 实现

1. Doorbell 机制

```c
// drivers/infiniband/hw/mlx5/wr.c:1030-1050
void mlx5r_send_wqe(struct mlx5_ib_qp *qp, struct mlx5r_qp_sq *sq,
                    struct mlx5r_send_bf *bf, int size)
{
    // 1. 构建 WQE (Work Queue Entry) - 只包含 RDMA 语义
    struct mlx5_wqe_ctrl_seg *ctrl = ...;
    ctrl->opmod_idx_opcode = cpu_to_be32(...);

    // 2. 写入 doorbell (MMIO 到网卡硬件)
    wmb();  // 内存屏障
    mlx5_write64((__be32 *)ctrl, bf->bfreg->map + bf->offset);

    // 网卡硬件自动处理:
    // - 构建 UDP/IP 头部
    // - 计算校验和
    // - 直接 DMA 数据到对端
}
```

2. UAR (User Access Region) 映射

mlx5_ib_mmap 的:

```c
// drivers/infiniband/hw/mlx5/main.c:2350-2460
static int mlx5_ib_mmap(struct ib_ucontext *context, struct vm_area_struct *vma)
{
    // 将网卡寄存器映射到用户空间
    pfn = uar_index2pfn(dev, uar_index);
    vma->vm_page_prot = pgprot_writecombine(prot);
    io_remap_pfn_range(vma, vma->vm_start, pfn, PAGE_SIZE, vma->vm_page_prot);
}
```


```
┌──────────────────────────────────────────────────────────────────────────┐
│                      Soft RoCE (RXE) vs 硬件 RDMA                         │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  RXE (软件实现)                      硬件 RDMA (mlx5/mlx4)               │
│  ─────────────                      ────────────────────                 │
│                                                                          │
│  用户态                              用户态                               │
│    │                                   │                                  │
│    ▼                                   ▼                                  │
│  libibverbs ──► 内核驱动              libibverbs ──► 直接 MMIO (mmap)     │
│                    │                            (用户态直接写网卡)        │
│                    ▼                                                      │
│              prepare4()                       ┌─────────────────┐        │
│                    │                          │  网卡硬件 (NIC)  │        │
│                    ▼                          │                 │        │
│              build UDP hdr                    │ • 自动构建 ETH   │        │
│                    │                          │ • 自动构建 IP    │        │
│                    ▼                          │ • 自动构建 UDP   │        │
│              build IP hdr                     │ • 自动 DMA 数据  │        │
│                    │                          │ • 硬校验和计算   │        │
│                    ▼                          └─────────────────┘        │
│              Linux 网络栈                                                 │
│                    │                                                      │
│                    ▼                                                      │
│              网卡驱动 (常规发送)                                          │
│                                                                          │
│  ⚠️ 经过内核协议栈                    ✅ 绕过内核，用户态直达网卡         │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```



┌────────────────────────────────────────────────────────────────────┐
│                         RoCEv2 的本质                               │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  UDP 层的作用 (仅用于网络传输):                                     │
│  ├── 目的端口 4791 用于标识这是 RoCEv2 流量                         │
│  ├── 允许数据包通过标准 IP 路由器                                   │
│  ├── 支持 ECN (Explicit Congestion Notification)                   │
│  └── 不依赖专门的 InfiniBand 交换机                                 │
│                                                                    │
│  UDP 层不做的事:                                                   │
│  ├── 不处理重传 (RDMA 自己做)                                       │
│  ├── 不做流量控制 (RDMA Credit 机制)                                │
│  └── 不做可靠性保证 (RDMA PSN 机制)                                 │
│                                                                    │
│  RDMA 头部 (BTH + 扩展头部):                                        │
│  ├── 包含完整的 RDMA 操作语义                                       │
│  ├── 支持 Send/Write/Read/Atomic 等操作                             │
│  ├── 实现可靠传输 (RC) 和不可靠传输 (UD)                            │
│  └── 与 InfiniBand 原生头部兼容                                     │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘

## RDMA 中断的处理模式
<!-- d83452b0-c94c-47be-89f2-7eddc7e46bde -->

(感觉就是这么回事了，但是需要继续核查一下)

用户态接收完成的两种模式

┌───────────────────────────────────────────────────────────────────────────┐
│                     RDMA 用户态完成通知机制                                │
├─────────────────────────────┬─────────────────────────────────────────────┤
│      1. 轮询 (Polling)       │        2. 中断/事件通知 (Event)              │
├─────────────────────────────┼─────────────────────────────────────────────┤
│                             │                                             │
│  用户态直接读取 CQ 内存      │  通过文件描述符等待中断通知                  │
│  (mmap 的 CQ 缓冲区)         │  (comp_channel)                             │
│                             │                                             │
│  ├── 延迟最低               │  ├── 节省 CPU                               │
│  ├── 100% 用户态            │  ├── 适合低频率完成场景                      │
│  └── 持续占用 CPU           │  └── 有上下文切换开销                        │
│                             │                                             │
└─────────────────────────────┴─────────────────────────────────────────────┘

1. 轮询模式 (Polling)

用户态通过 mmap 直接访问 CQ (Completion Queue) 缓冲区，不断轮询检查新的完成条目。

1.1 CQ 内存映射流程

```txt
// drivers/infiniband/hw/mlx5/cq.c:718-863
static int create_cq_user(struct mlx5_ib_dev *dev, struct ib_udata *udata,
                          struct mlx5_ib_cq *cq, int entries, ...)
{
    // 1. 用户态分配 CQ 缓冲区内存
    // 2. 内核通过 ib_umem_get() 获取该内存的物理页
    cq->buf.umem = ib_umem_get(&dev->ib_dev, ucmd.buf_addr,
                               entries * ucmd.cqe_size,
                               IB_ACCESS_LOCAL_WRITE);

    // 3. 将物理页地址 (PAS) 传递给硬件
    pas = (__be64 *)MLX5_ADDR_OF(create_cq_in, *cqb, pas);
    mlx5_ib_populate_pas(cq->buf.umem, page_size, pas, 0);

    // 4. 硬件直接 DMA 完成事件到这个缓冲区
    // 用户态通过 mmap 可以直接读取
}
```

1.2 用户态轮询代码

// 用户态代码 (libibverbs)
```c
struct ibv_cq *cq;  // 已创建并 mmap 的 CQ

// 轮询完成 (无系统调用，纯用户态)
int poll_cq() {
    struct ibv_wc wc;
    int n;

    // 直接读取 mmap 的 CQ 内存
    while ((n = ibv_poll_cq(cq, 1, &wc)) == 0) {
        // 自旋等待，无中断
        // 可选：pause/yield 避免过度占用 CPU
    }

    // 处理完成
    if (n > 0) {
        handle_completion(&wc);
    }
}
```
(这个就有待确认了)

2. 中断/事件通知模式 (Event)

当完成频率较低时，使用 中断 + 文件描述符 机制避免 CPU 空转。

2.1 创建完成通道 (Completion Channel)

```txt
// drivers/infiniband/core/uverbs_cmd.c:998-1020
static int ib_uverbs_create_comp_channel(
    struct uverbs_attr_bundle *attrs)
{
    struct ib_uverbs_completion_event_file *ev_file;

    // 创建一个文件描述符 (fd)
    ev_file = container_of(uobj, struct ib_uverbs_completion_event_file,
                           uobj);

    // 初始化事件队列
    ib_uverbs_init_event_queue(&ev_file->ev_queue);

    // 返回 fd 给用户态
    resp.fd = uobj->id;
}
```

2.2 CQ 关联完成通道

```txt
// drivers/infiniband/core/uverbs_std_types_cq.c:102-108
ev_file_uobj = uverbs_attr_get_uobject(attrs,
                   UVERBS_ATTR_CREATE_CQ_COMP_CHANNEL);
if (!IS_ERR(ev_file_uobj)) {
    ev_file = container_of(ev_file_uobj,
                   struct ib_uverbs_completion_event_file, uobj);
    // 将 CQ 与完成通道关联
    cq->cq_context = &ev_file->ev_queue;
}
```

2.3 中断处理流程

┌─────────────────────────────────────────────────────────────────────┐
│                     完成中断处理流程                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  1. 网卡硬件产生完成中断                                             │
│           │                                                         │
│           ▼                                                         │
│  2. 内核中断处理程序 (mlx5_ib_cq_comp)                              │
│           │                                                         │
│           ▼                                                         │
│  3. 调用 ib_uverbs_comp_handler()                                   │
│     // drivers/infiniband/core/uverbs_main.c:369                   │
│           │                                                         │
│           ▼                                                         │
│  4. 将事件加入队列                                                  │
│     list_add_tail(&entry->list, &ev_queue->event_list);             │
│           │                                                         │
│           ▼                                                         │
│  5. 唤醒等待的用户态进程                                            │
│     wake_up_interruptible(&ev_queue->poll_head);                    │
│           │                                                         │
│           ▼                                                         │
│  6. 用户态通过 read(fd) 读取事件                                    │
│     // 返回 ib_uverbs_comp_event_desc                              │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

2.4 内核事件处理代码

// drivers/infiniband/core/uverbs_main.c:369-399
void ib_uverbs_comp_handler(struct ib_cq *cq, void *cq_context)
{
    struct ib_uverbs_event_queue *ev_queue = cq_context;
    struct ib_uverbs_event *entry;

    // 分配事件条目
    entry = kzalloc(sizeof(*entry), GFP_ATOMIC);

    // 记录完成信息
    entry->desc.comp.cq_handle = cq->uobject->uevent.uobject.user_handle;
    entry->counter = &uobj->comp_events_reported;

    // 加入事件队列
    list_add_tail(&entry->list, &ev_queue->event_list);
    list_add_tail(&entry->obj_list, &uobj->comp_list);

    // 唤醒等待的用户态进程
    wake_up_interruptible(&ev_queue->poll_head);
    kill_fasync(&ev_queue->async_queue, SIGIO, POLL_IN);
}

2.5 用户态事件接收代码

// 用户态代码 (libibverbs)
struct ibv_comp_channel *channel = ibv_create_comp_channel(ctx);
struct ibv_cq *cq = ibv_create_cq(ctx, 100, NULL, channel, 0);

// 请求通知 (启用中断模式)
ibv_req_notify_cq(cq, 0);

// 等待完成事件
struct ibv_cq *ev_cq;
void *ev_ctx;
ibv_get_cq_event(channel, &ev_cq, &ev_ctx);  // 阻塞等待

// 收到事件后，仍需 poll_cq 获取具体完成
int n = ibv_poll_cq(ev_cq, max_wc, wc_array);

// 确认事件已处理
ibv_ack_cq_events(ev_cq, 1);

───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
3. 关键数据结构

// 完成事件文件 (对应用户态文件描述符)
struct ib_uverbs_completion_event_file {
    struct ib_uobject uobj;
    struct ib_uverbs_event_queue ev_queue;  // 事件队列
};

// 事件队列
struct ib_uverbs_event_queue {
    spinlock_t lock;
    wait_queue_head_t poll_head;      // 等待队列 (用于 wake_up)
    struct fasync_struct *async_queue; // 异步通知 (SIGIO)
    struct list_head event_list;       // 事件链表
};

// 完成事件描述符
struct ib_uverbs_comp_event_desc {
    __u64 cq_handle;   // CQ 句柄
};

───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
4. 混合使用策略

高性能应用通常结合两种模式：

// 1. 忙轮询一段时间 (低延迟)
for (int i = 0; i < max_poll; i++) {
    n = ibv_poll_cq(cq, batch, wc);
    if (n > 0)
        process_completions(wc, n);
}

// 2. 如果没有完成，切换到中断模式 (节省 CPU)
if (n == 0) {
    ibv_req_notify_cq(cq, 0);  // 启用中断
    // 再次检查避免竞态
    n = ibv_poll_cq(cq, batch, wc);
    if (n == 0) {
        ibv_get_cq_event(channel, &ev_cq, &ev_ctx);  // 阻塞等待中断
    }
}

5. 总结

 机制                触发方式      延迟   CPU 占用   适用场景
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Doorbell (发送)     用户态 MMIO   极低   无         发送请求
 CQ Polling (接收)   用户态轮询    极低   高         高频交易
 CQ Event (接收)     内核中断      较高   低         低频 I/O

关键理解：虽然 mlx5_ib_mmap 让用户态可以直接通过 MMIO 发送命令（Doorbell），但接收完成有两种路径：

1. 直接轮询：用户态直接读取 mmap 的 CQ 内存
2. 中断通知：通过文件描述符等待内核唤醒，然后 poll CQ 获取具体完成

## rdma librdmacm 是做什么的
<!-- fa6f93ad-2cc1-42be-85fe-57d12435bad7 -->

cm 是 Connection Manager 的缩写。

在 RDMA 语境里：
- rdma_cm 是内核里的 RDMA Connection Manager
- librdmacm 是用户态对应的库


librdmacm/ 是 RDMA Connection Manager 的用户态库实现，也就是 librdmacm。它的作用不是做真正的数据收发，而是帮应用完成“怎么找到对端、怎么建连、
怎么交换队列参数、怎么断开连接”这层工作。

在职责上可以粗分成几块：

- 地址解析：把 IP/端口或 RDMA 地址解析成可用于 RDMA 建连的目标。
- 连接管理：发起连接、监听、接受连接、断开连接。
- CM 事件：把内核 rdma_cm 设备上的连接事件包装成用户态 API。
- QP 关联：把 ibverbs 里的 QP 和连接流程串起来，让应用少直接处理底层细节。

和别的目录关系也很明确：

- libibverbs/ 负责更底层的 verbs 操作，比如 CQ/QP/MR/WR。
- librdmacm/ 负责连接流程，通常建立连接后再调用 verbs 发收数据。
- 简单说：librdmacm 管“连上”，libibverbs 管“怎么传”。

如果你想从代码入口看，先看这些位置：

- librdmacm/CMakeLists.txt
- librdmacm/man
- librdmacm/examples

内核中对应的代码为:

1. 先看   include/uapi/rdma/rdma_user_cm.h
2. 再看   drivers/infiniband/core/ucma.c
3. 最后看 drivers/infiniband/core/cma.c

这样最容易把用户态 librdmacm 的调用链对应到内核。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
