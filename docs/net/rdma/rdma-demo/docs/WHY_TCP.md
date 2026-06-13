# 为什么 RDMA 程序需要 TCP？

## 简短回答

**TCP 只用于连接建立阶段交换元信息，实际数据传输完全通过 RDMA。**

## 详细解释

### RDMA 连接建立需要什么？

RDMA 连接（Queue Pair, QP）建立之前，两端需要交换以下信息：

```c
struct conn_info {
    uint16_t lid;       // Local ID，本地标识符
    uint32_t qpn;       // QP Number，队列对编号
    uint32_t psn;       // Packet Sequence Number，包序列号
    union ibv_gid gid;  // Global ID，用于 RoCE/iWARP
};
```

这些信息是 RDMA 硬件级别的地址，**不能通过 RDMA 本身交换**（因为连接还没建立）。

### 连接建立的流程

```
阶段 1: TCP 连接建立 (带外通道)
================================
服务端: socket() -> bind() -> listen() -> accept()
客户端: socket() -> connect()
        |
        | TCP 连接已建立
        v

阶段 2: 交换 RDMA 连接信息 (通过 TCP)
====================================
服务端: 获取本地 lid, qpn, psn
       |
       | write(connfd, &local_info)
       v
客户端: read(sockfd, &remote_info)
       | 获取服务端信息
       | write(sockfd, &local_info)
       v
服务端: read(connfd, &remote_info)
       | 获取客户端信息
       v

阶段 3: TCP 断开 (信息交换完成)
===============================
close(sockfd)  <-- TCP 连接关闭！

阶段 4: RDMA QP 状态转换
=========================
服务端/客户端:
    |
    v
ibv_modify_qp(INIT)  <-- 使用交换的 qpn, lid 等信息
    |
    v
ibv_modify_qp(RTR)   <-- Ready To Receive
    |
    v
ibv_modify_qp(RTS)   <-- Ready To Send
    |
    v
RDMA 连接建立完成！

阶段 5: 纯 RDMA 数据传输
=========================
服务端/客户端:
    |
    v
ibv_post_send()  ---->  网卡硬件  ---->  远端网卡硬件
                              |
                              | 数据传输完全绕过 TCP！
                              v
ibv_poll_cq()  <----  完成通知
```

### 代码中的体现

```c
// 1. TCP 连接建立
int sockfd = tcp_client_connect(server_ip, 18515);  // TCP

// 2. 交换 RDMA 连接信息 (通过 TCP)
exchange_conn_info(sockfd, rdma_ctx, &remote_info);  // TCP send/recv

// 3. TCP 连接关闭 (重要！)
close(sockfd);  // TCP 不再需要

// 4. RDMA QP 状态转换
modify_qp_to_init(rdma_ctx);    // 使用交换的信息
modify_qp_to_rtr(rdma_ctx, &remote_info);  // Ready To Receive
modify_qp_to_rts(rdma_ctx);     // Ready To Send

// 5. 纯 RDMA 数据传输 (无 TCP 参与！)
post_send(rdma_ctx, IBV_WR_SEND);  // RDMA!
poll_cq(rdma_ctx);                  // RDMA!
```

## TCP vs RDMA 使用对比

| 阶段 | TCP | RDMA | 说明 |
|------|-----|------|------|
| **连接建立** | 使用 | 未建立 | TCP 交换元信息 |
| **QP 初始化** | 不使用 | 使用 | RDMA 状态转换 |
| **数据传输** | 不使用 | 使用 | 纯 RDMA |
| **完成通知** | 不使用 | 使用 | RDMA CQ |

## 可以不要 TCP 吗？

**可以！** 但需要使用其他带外通道来交换初始信息：

### 替代方案 1: RDMA Connection Manager (CM)

```c
// 使用 librdmacm，内置了连接管理
#include <rdma/rdma_cma.h>

// CM 会自动处理连接建立
rdma_connect();   // 替代 TCP + 手动交换信息
rdma_accept();
```

### 替代方案 2: 共享内存/文件

```c
// 两端通过共享文件交换信息
write(fd, &local_info, sizeof(local_info));
// 另一端读取
read(fd, &remote_info, sizeof(remote_info));
```

### 替代方案 3: 预先配置

```c
// 在配置文件中硬编码
struct conn_info remote_info = {
    .lid = 0x0001,
    .qpn = 0x0089,
    .psn = 0x123456,
};
```

## 为什么示例使用 TCP？

1. **简单通用**：TCP 是最通用的网络协议
2. **无需额外依赖**：不需要安装 librdmacm
3. **易于理解**：展示 RDMA 连接建立的本质
4. **跨平台**：在不同网络环境下都能工作

## 总结

```
┌─────────────────────────────────────────────────────────────┐
│                    RDMA 程序生命周期                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  阶段 1: 带外通道 (TCP/CM/文件)                               │
│  ┌─────────────────────────────────────────┐                 │
│  │ 交换 lid, qpn, psn, gid 等元信息         │                 │
│  └─────────────────────────────────────────┘                 │
│                    ↓                                         │
│              close(sockfd)  ← TCP 关闭！                     │
│                    ↓                                         │
│  阶段 2: RDMA 连接建立                                        │
│  ┌─────────────────────────────────────────┐                 │
│  │ ibv_modify_qp(INIT → RTR → RTS)         │                 │
│  └─────────────────────────────────────────┘                 │
│                    ↓                                         │
│  阶段 3: 纯 RDMA 数据传输                                     │
│  ┌─────────────────────────────────────────┐                 │
│  │ ibv_post_send() / ibv_post_recv()       │                 │
│  │ ibv_poll_cq() / ibv_get_cq_event()      │                 │
│  └─────────────────────────────────────────┘                 │
│                    ↓                                         │
│              数据传输完成 (全程无 TCP！)                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**关键点**：
- TCP 只在开始时使用，用于交换"连接参数"
- 一旦 RDMA QP 建立，TCP 就可以断开
- 后续所有数据传输都是纯 RDMA，不经过 TCP/IP 协议栈

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
