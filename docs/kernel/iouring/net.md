# net
## iouring 对于网络存在两个技术优化 : zero copy 和 async

首先，需要意识到网络根本就没有类似 aio 的异步提交的，
因为网络是需要报文重组的

## zero copy
https://speakerdeck.com/ennael/efficient-zero-copy-networking-using-io-uring
https://www.phoronix.com/news/Linux-6.15-IO_uring
https://docs.kernel.org/networking/iou-zcrx.html

https://news.ycombinator.com/item?id=35547316

## 介绍
https://developers.redhat.com/articles/2023/04/12/why-you-should-use-iouring-network-io
(这个还是要拷贝的)
net/core/netdev_rx_queue.c

## tcp devmem (2024 合并的)
https://lwn.net/Articles/937882/

利用网卡，直接把网卡中的数据写入到 GPU 内存中，也就是
说，其实也是可以直接写入到 nvme 中?
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

https://docs.kernel.org/networking/devmem.html
https://netdevconf.info/0x17/sessions/talk/device-memory-tcp.html

## IORING_OP_SEND_ZC
测试 code/src/c/iouring/op-send-zc.c

老技术了。


## 存储和网络上存在什么区别?
网络很喜欢使用 epoll ，因为远程的用户可能永远都不发送。

### 使用 poll 的方法
disk io 的 Polling 不是用的 napi 的方法，而是自己的机制:

dpdk 的 polling 是发生网卡侧的! 换言之，examples/napi-busy-poll-client.c 中似乎
只是发送数据之后马上开始 polling 的模式等待返回。

### 网络是 pollable 的，但是文件不是
https://man7.org/linux/man-pages/man3/io_uring_prep_read_multishot.3.html

> A multishot read request will repeatedly trigger a completion
> event whenever data is available to read from the file. Because of
> that, this type of request can only be used with a file type that
> is pollable.  Examples of that include pipes, tun devices, etc. If
> used with a regular file, or a wrong file type in general, the
> request will fail with -EBADFD in the CQE res field.

所以，在网络中提供了相应的 multishot 的
- IORING_RECV_MULTISHOT
- IORING_SEND_MULTISHOT
- IORING_ACCEPT_MULTISHOT

可以不断从 socket 收到新的消息，但是向磁盘提交了写操作，就只是需要一个 return 来
表示 io 完成了。

## [ ]  如果有 multishot ，似乎这些讨论有点老了?
https://developers.redhat.com/articles/2023/04/12/why-you-should-use-iouring-network-io

io_uring vs epoll 谁在网络编程领域更胜一筹？
https://developer.aliyun.com/article/834974

## 内核 iouring/napi.c 文件做什么的？

io_uring/napi.c 文件的作用

io_uring/napi.c 是 Linux 内核 io_uring 子系统的网络轮询（NAPI）集成模块，主要用于实现高效的网络 I/O 操作。具体功能包括：

主要功能：

 1. NAPI ID 管理：
    - 维护一个哈希表和链表，用于跟踪与 io_uring 实例关联的 NAPI ID
    - 提供添加、删除和查找 NAPI ID 的功能

 2. 网络忙轮询（Busy Polling）优化：
    - 在网络操作中启用忙轮询机制，避免不必要的上下文切换
    - 通过 __io_napi_busy_loop() 函数执行忙轮询循环

 3. 动态和静态跟踪模式：
    - 支持两种 NAPI 跟踪模式：静态（STATIC）和动态（DYNAMIC）
    - 静态模式下手动管理 NAPI ID
    - 动态模式下自动为套接字添加 NAPI ID

 4. 资源清理：
    - 定期清理过期的 NAPI 条目（超时超过60秒）
    - 提供初始化和释放函数来管理内存资源

关键数据结构：

 - io_napi_entry：表示单个 NAPI 条目，包含 NAPI ID、超时时间等信息

使用场景：

当应用程序使用 io_uring 进行网络 I/O
操作时，特别是涉及套接字操作时，该模块可以显著提高性能，通过直接在用户线程中进行网络轮询，减少内核和用户空间之间的上下文切换开销。

这个文件是 Linux 内核中 io_uring 子系统与网络子系统深度集成的一部分，旨在提供高性能的异步 I/O 操作，特别是在网络应用中。

## 网络真的开始发力了
https://blog.tohojo.dk/2026/02/the-inner-workings-of-tcp-zero-copy.html

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
