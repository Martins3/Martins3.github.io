# devmem tcp

Device Memory TCP（常简称 devmem TCP）在 Linux 内核中的主要实现位置和机制如下。

## 主要代码位置
核心基础设施

- Documentation/networking/devmem.rst：完整的用户态 API 和配置说明
- net/Kconfig：CONFIG_NET_DEVMEM（依赖 DMA_SHARED_BUFFER 和 PAGE_POOL）
- net/core/devmem.c / net/core/devmem.h：dmabuf 与 RX/TX 队列绑定的核心逻辑
- net/core/netmem.h：抽象 netmem_ref（可以是普通 struct page 或 struct net_iov）
- net/core/mp_dmabuf_devmem.h：dmabuf memory provider 的 ops
- net/core/page_pool.c / net/core/page_pool_user.c：page pool 接入自定义 memory provider
- net/core/netdev_rx_queue.c：RX queue 的 memory provider 启停/校验
- net/core/netdev-genl.c：netlink 接口 NETDEV_CMD_BIND_RX / NETDEV_CMD_BIND_TX

TCP / socket 数据路径

- net/ipv4/tcp.c
    - RX：tcp_recvmsg() → tcp_recvmsg_dmabuf()，生成 SCM_DEVMEM_DMABUF / SCM_DEVMEM_LINEAR cmsg
    - TX：tcp_sendmsg() 处理 SCM_DEVMEM_DMABUF cmsg，调用 skb_zerocopy_iter_stream()
- net/core/sock.c：SO_DEVMEM_DONTNEED setsockopt，用于归还 token/释放 frag
- net/core/skbuff.c：__zerocopy_sg_from_iter()、msg_zerocopy_* 等 zerocopy 基础设施
- net/core/datagram.c：zerocopy_fill_skb_from_devmem()，把 TX dmabuf 映射成 skb frag

示例 / 测试 / 驱动

- tools/testing/selftests/drivers/net/hw/ncdevmem.c：完整的 nc 风格测试程序
- 已有驱动支持（实现 netdev_queue_mgmt_ops + header split）：
    - drivers/net/ethernet/mellanox/mlx5/core/en_main.c
    - drivers/net/ethernet/meta/fbnic/fbnic_netdev.c
    - drivers/net/ethernet/google/gve/gve_main.c
    - drivers/net/ethernet/broadcom/bnxt/bnxt.c
    - drivers/net/netdevsim/netdev.c（测试/模拟用）

## 大致实现流程

RX 路径

1. 绑定 dmabuf 到 RX 队列
    - 用户通过 netlink 把 dmabuf fd 绑定到某个 RX queue。
    - 内核创建 struct net_devmem_dmabuf_binding：
          - dma_buf_attach() + dma_buf_map_attachment_unlocked() 拿到 sg table 和 dma_addr
          - 把 sg table 切成 PAGE_SIZE 粒度的 chunk，生成 struct net_iov 数组
          - 每个 net_iov 记录 dma_addr，并挂到该 binding 的 gen pool 里
    - 绑定成功后返回一个全局唯一的 dmabuf_id
2. 驱动分配 payload buffer
    - 驱动为该 RX queue 创建 page_pool 时带上 PP_FLAG_ALLOW_UNREADABLE_NETMEM
    - page pool 通过 rxq->mp_params 拿到 dmabuf_devmem_ops
    - mp_dmabuf_devmem_alloc_netmems() 从 gen pool 分配 net_iov，而不是分配普通 page
    - NIC 把数据包 payload 直接 DMA 到这些 net_iov，header 仍放到普通 host page
3. TCP 收包
    • tcp_recvmsg() 检查 skb_frags_readable(skb)：
          • 普通 skb：走正常 skb_copy_datagram_msg()
          • devmem skb（frags 不可读）：必须用户设置 MSG_SOCK_DEVMEM，否则返回 -EFAULT
    • tcp_recvmsg_dmabuf()：
          • 把 header 部分拷贝到用户缓冲区，并附带 SO_DEVMEM_LINEAR cmsg
          • 对每个 devmem frag，通过 cmsg 告诉用户：
                  • dmabuf_id：哪个 dmabuf
                  • frag_offset：payload 在 dmabuf 中的偏移
                  • frag_size：payload 长度
                  • frag_token：用于之后释放的 token
    • 用户处理完数据后，通过 SO_DEVMEM_DONTNEED 归还 token，内核减少引用并回收 net_iov
4. 引用管理
    • net_devmem_dmabuf_binding 使用 percpu_ref
    • TX/RX 的 net_iov 持有 binding 引用，skb 释放时 net_devmem_put_net_iov() 释放
    • binding 生命周期由 netlink socket 持有；socket 关闭自动解绑

TX 路径

1. 绑定 dmabuf 到 TX
    • 类似 RX，但方向为 DMA_TO_DEVICE
    • 绑定后建立 tx_vec[]，用于把用户给出的虚拟偏移快速映射到 net_iov
2. 用户发送
    • 通过 SCM_DEVMEM_DMABUF cmsg 传入 dmabuf_id
    • iov_base 填的是 dmabuf 内的偏移，iov_len 是要发送的字节数
    • 必须设置 SO_ZEROCOPY / MSG_ZEROCOPY
3. TCP 发送
    • tcp_sendmsg() 调用 net_devmem_get_binding() 校验 binding 与路由目的网卡一致
    • skb_zerocopy_iter_stream() → __zerocopy_sg_from_iter() → zerocopy_fill_skb_from_devmem()
    • 根据用户偏移从 tx_vec 找到对应 net_iov，直接挂到 skb frags 上
    • 发送完成后通过 MSG_ERRQUEUE 的 zerocopy completion 通知用户可以复用 dmabuf

## 硬件具备的功能

必须能力

• Header Split / TCP Data Split
    • NIC 必须能把数据包分成 header（到 host memory） 和 payload（到 device memory）。
    • 通过 ethtool 开启：ethtool -G ethX tcp-data-split on
    • 内核会检查 dev->cfg->hds_config == ETHTOOL_TCP_DATA_SPLIT_ENABLED 且 hds_thresh == 0
• Flow Steering + RSS
    • 只有目标 flow 的流量应该落到绑定了 dmabuf 的 RX queue。
    • 需要 ethtool -K ethX ntuple on 并通过 flow rule 把指定 TCP flow 导向该 queue
    • 通常还要调整 RSS indirection table，避免其它流量占用该 queue
• DMA 访问 device memory
    • NIC 必须能通过 DMA 读写 dmabuf 对应的 DMA 地址。
    • 这通常依赖：
          • IOMMU/SMMU 映射
          • PCIe P2P DMA（如果 dmabuf 背后是 GPU/加速器显存）
    • dmabuf exporter 必须提供 dma_buf_attach/map 后合法的 dma_addr
• 驱动实现 netdev_queue_mgmt_ops
    • ndo_queue_mem_size
    • ndo_queue_mem_alloc
    • ndo_queue_mem_free
    • ndo_queue_stop
    • ndo_queue_start
    • 并在创建 page pool 时支持 PP_FLAG_ALLOW_UNREADABLE_NETMEM

建议 / 限制

- TX 侧不需要 header split，但需要 zerocopy 能力和对 device memory 的 DMA 读能力
- devmem skb 的 payload 对内核不可读，因此：
    - loopback 不可用
    - 软件 checksum 会失败
    - tcpdump / BPF 无法访问 payload
- 目前只实现了 TCP；UDP 等未支持


## dmabuf 就是用于解决 device memory 的访问抽象

是的，基本流程为:
```txt
dmabuf

↓

dma_buf_map_attachment()

↓

sg_table

↓

struct dev_pagemap

↓

struct page
```

## TCP header 中到底存在什么东西?

1. TCP 头部里到底有什么

一个 TCP 头部（不算 IP/Ethernet 头）大致包含：

┌──────────────────┬──────────────────────────────────────────────────────────┐
│ 字段             │ 说明                                                     │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Source/Dest Port │ 源/目的端口                                              │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Sequence Number  │ 本报文段第一个字节序号                                   │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Ack Number       │ 期望收到的下一个序号                                     │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Data Offset      │ TCP 头部长度，单位是 4 字节，范围 5~15 → 20 ~ 60 字节    │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Flags            │ SYN/ACK/FIN/RST/PSH/URG 等                               │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Window Size      │ 接收窗口                                                 │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Checksum         │ 覆盖 TCP 头部 + payload                                  │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Urgent Pointer   │ 紧急指针                                                 │
├──────────────────┼──────────────────────────────────────────────────────────┤
│ Options          │ MSS、Window Scale、SACK、Timestamps 等，导致头部长度可变 │
└──────────────────┴──────────────────────────────────────────────────────────┘

所以 TCP 头部有两个关键特点：
-  头部长度可变：最小 20 字节，最大 60 字节，取决于 Data Offset 和 Options。
-  没有“Payload Length”字段：payload 多长要靠 IP 层的 Total Length 减去 IP 头长度再减去 TCP 头长度来算。

2. 这和 devmem TCP 的 header split 有什么关系

devmem TCP 的核心诉求是：

• payload 直接 DMA 到 device memory（dmabuf）
• header 放到 host memory，让内核 TCP/IP 栈正常解析

要做到这一点，NIC 必须在收到包时准确找到 header 在哪里结束、payload 从哪里开始，然后把：

• header → host page（给内核看）
• payload → dmabuf net_iov（给应用/GPU/加速器看）

这个边界就是 header split 点。对 TCP 来说，NIC 必须：

1. 解析 Ethernet + IP 头，知道 IP 头长度；
2. 解析 TCP 头，读取 Data Offset，算出 TCP 头长度；
3. 用 IP Total Length - IP header - TCP header 得到 payload 长度；
4. 把 payload 部分 DMA 到绑定好的 dmabuf 里。

因为 TCP 头部长度可变，header split 不能像 UDP 那样固定偏移（UDP 头固定 8 字节），而必须做真正的 L4 解析。所以 devmem TCP 文档明确说：

> Header split is used to split incoming packets into a header buffer in host memory, and a payload buffer in device memory.

并且内核在绑定 dmabuf 到 RX queue 时会强制检查：

```c
  if (dev->cfg->hds_config != ETHTOOL_TCP_DATA_SPLIT_ENABLED) {
      NL_SET_ERR_MSG(extack, "tcp-data-split is disabled");
      return -EINVAL;
  }
  if (dev->cfg->hds_thresh) {
      NL_SET_ERR_MSG(extack, "hds-thresh is not zero");
      return -EINVAL;
  }
```

也就是说，NIC 必须支持 TCP Data Split 这种 header split 能力，才能正确把 payload 切到 dmabuf。

3. 另一个原因：TCP 栈必须读 header

TCP 是流式协议，内核必须看 header 才能做：

- 序号管理（Sequence/Ack）
- 窗口控制（Window）
- 状态机（SYN/FIN/RST/ACK）
- 重传、SACK、RTT 计算等

payload 只是字节流的一部分，内核不需要读 payload 内容，只要知道长度和 dmabuf 偏移就能交给用户态。因此 header split 对 TCP 是天然契合的：header 必须进
host memory 被解析，payload 可以 opaque 地留在 device memory。

如果是 UDP，虽然有固定头部，但 UDP 是消息边界型，头部里有明确的 Length 字段，更容易做 split；不过当前 devmem TCP 只实现了 TCP 路径。

一句话总结

你觉得奇怪的点是对的：TCP 头部没有直接的 payload 长度字段，且头部长度可变（20~60 字节），所以 NIC 必须支持 header split / TCP data
split，才能动态算出 header/payload 边界，把 payload 直接 DMA 到 device memory，而 header 留给 host 栈处理。

## 附录
- https://lwn.net/Articles/937882/
- https://docs.kernel.org/networking/devmem.html
- https://netdevconf.info/0x17/sessions/talk/device-memory-tcp.html

```txt

* TL;DR:

Device memory TCP (devmem TCP) is a proposal for transferring data to and/or
from device memory efficiently, without bouncing the data to a host memory
buffer.

* Problem:

A large amount of data transfers have device memory as the source and/or
destination. Accelerators drastically increased the volume of such transfers.
Some examples include:
- ML accelerators transferring large amounts of training data from storage into
  GPU/TPU memory. In some cases ML training setup time can be as long as 50% of
  TPU compute time, improving data transfer throughput & efficiency can help
  improving GPU/TPU utilization.

- Distributed training, where ML accelerators, such as GPUs on different hosts,
  exchange data among them.

- Distributed raw block storage applications transfer large amounts of data with
  remote SSDs, much of this data does not require host processing.

Today, the majority of the Device-to-Device data transfers the network are
implemented as the following low level operations: Device-to-Host copy,
Host-to-Host network transfer, and Host-to-Device copy.

The implementation is suboptimal, especially for bulk data transfers, and can
put significant strains on system resources, such as host memory bandwidth,
PCIe bandwidth, etc. One important reason behind the current state is the
kernel’s lack of semantics to express device to network transfers.

* Proposal:

In this patch series we attempt to optimize this use case by implementing
socket APIs that enable the user to:

1. send device memory across the network directly, and
2. receive incoming network packets directly into device memory.

Packet _payloads_ go directly from the NIC to device memory for receive and from
device memory to NIC for transmit.
Packet _headers_ go to/from host memory and are processed by the TCP/IP stack
normally. The NIC _must_ support header split to achieve this.

Advantages:

- Alleviate host memory bandwidth pressure, compared to existing
 network-transfer + device-copy semantics.

- Alleviate PCIe BW pressure, by limiting data transfer to the lowest level
  of the PCIe tree, compared to traditional path which sends data through the
  root complex.

With this proposal we're able to reach ~96.6% line rate speeds with data sent
and received directly from/to device memory.

* Patch overview:

** Part 1: struct paged device memory

Currently the standard for device memory sharing is DMABUF, which doesn't
generate struct pages. On the other hand, networking stack (skbs, drivers, and
page pool) operate on pages. We have 2 options:

1. Generate struct pages for dmabuf device memory, or,
2. Modify the networking stack to understand a new memory type.

This proposal implements option #1. We implement a small framework to generate
struct pages for an sg_table returned from dma_buf_map_attachment(). The support
added here should be generic and easily extended to other use cases interested
in struct paged device memory. We use this framework to generate pages that can
be used in the networking stack.

** Part 2: recvmsg() & sendmsg() APIs

We define user APIs for the user to send and receive these dmabuf pages.

** part 3: support for unreadable skb frags

Dmabuf pages are not accessible by the host; we implement changes throughput the
networking stack to correctly handle skbs with unreadable frags.

** part 4: page pool support

We piggy back on Jakub's page pool memory providers idea:
https://github.com/kuba-moo/linux/tree/pp-providers

It allows the page pool to define a memory provider that provides the
page allocation and freeing. It helps abstract most of the device memory TCP
changes from the driver.

This is not strictly necessary, the driver can choose to allocate dmabuf pages
and use them directly without going through the page pool (if acceptable to
their maintainers).

Not included with this RFC is the GVE devmem TCP support, just to
simplify the review. Code available here if desired:
https://github.com/mina/linux/tree/tcpdevmem

This RFC is built on top of v6.4-rc7 with Jakub's pp-providers changes
cherry-picked.

* NIC dependencies:

1. (strict) Devmem TCP require the NIC to support header split, i.e. the
   capability to split incoming packets into a header + payload and to put
   each into a separate buffer. Devmem TCP works by using dmabuf pages
   for the packet payload, and host memory for the packet headers.

2. (optional) Devmem TCP works better with flow steering support & RSS support,
   i.e. the NIC's ability to steer flows into certain rx queues. This allows the
   sysadmin to enable devmem TCP on a subset of the rx queues, and steer
   devmem TCP traffic onto these queues and non devmem TCP elsewhere.

The NIC I have access to with these properties is the GVE with DQO support
running in Google Cloud, but any NIC that supports these features would suffice.
I may be able to help reviewers bring up devmem TCP on their NICs.

* Testing:

The series includes a udmabuf kselftest that show a simple use case of
devmem TCP and validates the entire data path end to end without
a dependency on a specific dmabuf provider.

Not included in this series is our devmem TCP benchmark, which
transfers data to/from GPU dmabufs directly.

With this implementation & benchmark we're able to reach ~96.6% line rate
speeds with 4 GPU/NIC pairs running bi-direction traffic, with all the
packet payloads going straight to the GPU memory (no host buffer bounce).

** Test Setup

Kernel: v6.4-rc7, with this RFC and Jakub's memory provider API
cherry-picked locally.

Hardware: Google Cloud A3 VMs.

NIC: GVE with header split & RSS & flow steering support.

Benchmark: custom devmem TCP benchmark not yet open sourced.

Mina Almasry (10):
  dma-buf: add support for paged attachment mappings
  dma-buf: add support for NET_RX pages
  dma-buf: add support for NET_TX pages
  net: add support for skbs with unreadable frags
  tcp: implement recvmsg() RX path for devmem TCP
  net: add SO_DEVMEM_DONTNEED setsockopt to release RX pages
  tcp: implement sendmsg() TX path for for devmem tcp
  selftests: add ncdevmem, netcat for devmem TCP
  memory-provider: updates core provider API for devmem TCP
  memory-provider: add dmabuf devmem provider
```


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
