# net

## iouring 对于网络存在两个技术优化 : zero copy 和 async

首先，需要意识到网络根本就没有类似 aio 的异步提交的感觉
是我的错觉吗?

## zero copy
https://speakerdeck.com/ennael/efficient-zero-copy-networking-using-io-uring
https://www.phoronix.com/news/Linux-6.15-IO_uring
https://docs.kernel.org/networking/iou-zcrx.html

https://news.ycombinator.com/item?id=35547316

## 介绍
https://developers.redhat.com/articles/2023/04/12/why-you-should-use-iouring-network-io
(这个还是要拷贝的)
net/core/netdev_rx_queue.c

## IORING_OP_SEND_ZC
测试 code/src/c/iouring/op-send-zc.c

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

### 再问一次

• 这个文件实现的是：让 io_uring 在等待网络请求完成时，主动轮询网卡的 NAPI 接收队列，以降低中断、调度和唤醒带来的网络延迟。

  它是网络 NAPI busy-poll，不是 IORING_SETUP_IOPOLL 的块设备轮询。

  ### 工作流程

  io_uring 网络请求
      ↓
  记录 socket 的 sk_napi_id
      ↓
  io_uring_enter() 等待 CQE
      ↓
  主动调用 napi_busy_loop_rcu()
      ↓
  驱动 NAPI poll 收包
      ↓
  请求完成并生成 CQE

  这样可能在进程睡眠前直接把包轮询出来，减少：

  网卡中断 → softirq → 唤醒进程 → 调度

  ### 主要功能

  1. 管理 NAPI ID

  每个网络 socket 会关联一个 sk_napi_id，标识处理该 socket 流量的网卡 RX queue/NAPI 实例。

  动态模式下，io_uring/napi.h:42 从 socket 读取这个 ID：

  __io_napi_add_id(ctx, READ_ONCE(sock->sk->sk_napi_id));

  io_uring/napi.c:41 将 ID 同时保存到：

  - ctx->napi_list：用于遍历并执行 busy poll。
  - ctx->napi_ht：用于快速查重和查找。

  动态跟踪的 ID 如果 60 秒没有再次使用，会被清理。静态模式的 ID 由用户态显式添加、删除，不会自动过期。

  2. 执行 busy poll

  核心调用在 io_uring/napi.c:163：

  napi_busy_loop_rcu(e->napi_id, ...);

  它让当前线程直接运行对应 NAPI 实例的 poll，从网卡 RX queue 收包。

  循环遇到以下条件会退出：

  - CQ 中已经有足够的完成事件；
  - io_uring 有待处理工作；
  - 收到信号；
  - busy_poll_to 超时。

  3. 接入两种等待路径

  普通 io_uring 等待：

  io_uring/wait.c:249 在真正睡眠之前执行 busy poll。

  SQPOLL 模式：

  io_uring/sqpoll.c:353 由 SQPOLL 内核线程轮询 NAPI。

  4. 提供用户态配置接口

  用户通过 IORING_REGISTER_NAPI 配置：

  - busy-poll 持续时间；
  - prefer_busy_poll；
  - 动态或静态 NAPI ID 跟踪模式。

  ABI 定义在 include/uapi/linux/io_uring.h:911。

  ### 并发设计

  这里使用：

  - spinlock：保护添加、删除操作；
  - RCU：busy-poll 热路径可以无锁遍历；
  - hash table：快速查重；
  - kfree_rcu()：避免遍历期间释放对象。


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
