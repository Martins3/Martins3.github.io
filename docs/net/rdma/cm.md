# rdma cm
RDMA CM 是 RDMA Connection Manager。它主要负责“怎么连上”，而不是“连上以后怎么收发数据”。

如果把 RDMA 程序拆层看：

- libibverbs / QP / CQ / MR：负责数据面
- rdma_cm / librdmacm：负责连接面

它做的事可以概括成这几类。

1. 地址解析

把你给的地址信息解析成 RDMA 能用的目标。

例如：

- IP 地址
- 端口
- 本地绑定地址
- 路由信息
- 对应 RDMA 设备/GID/path

典型接口是：

- rdma_resolve_addr()
- rdma_resolve_route()

所以它的第一步工作是：
把“我要连这个 IP:port”转换成“应该走哪块 RDMA 设备、哪个路径”。

2. 建立连接

它负责 client/server 建连流程：

- server rdma_bind_addr()
- server rdma_listen()
- client rdma_connect()
- server 收到 CONNECT_REQUEST
- server rdma_accept() 或 rdma_reject()

也就是把“谁来建 QP、谁来交换连接参数、什么时候连接进入 established”这些流程都封装起来。

3. 事件通知

rdma_cm 是事件驱动的。它会给你各种连接相关事件，例如：

- ADDR_RESOLVED
- ROUTE_RESOLVED
- CONNECT_REQUEST
- ESTABLISHED
- DISCONNECTED
- REJECTED
- TIMEWAIT_EXIT

所以它像一个状态机框架，帮你把连接过程变成一串事件。

4. 帮你协同 QP 参数

它不直接替代 verbs，但它能把连接流程和 QP 初始化串起来。

常见模式是：

- 先创建 rdma_cm_id
- 用 CM 走解析和建连
- 用 rdma_create_qp() 把 QP 和这个连接对象绑起来
- CM 在合适时机帮你完成一部分 QP 状态转换需要的信息准备

所以你可以理解为：

- verbs 管 QP 本身
- CM 管 QP 如何和一个对端建立关系

5. 断开与清理

连接断开时它也负责：

- rdma_disconnect()
- 给对端发断连事件
- 协助释放连接相关状态

一句话区分

- verbs：怎么发包、怎么 poll CQ、怎么注册内存
- rdma_cm：怎么找到对端、怎么连、怎么收连接事件

最典型调用链

client 侧常见流程：

1. rdma_create_id()
2. rdma_resolve_addr()
3. rdma_resolve_route()
4. rdma_create_qp()
5. rdma_connect()
6. 收到 ESTABLISHED
7. 开始用 verbs 收发

server 侧常见流程：

1. rdma_create_id()
2. rdma_bind_addr()
3. rdma_listen()
4. 收到 CONNECT_REQUEST
5. rdma_create_qp()
6. rdma_accept()
7. 收到 ESTABLISHED

内核里它大概在哪

你前面问过内核位置，对应主要是：

- drivers/infiniband/core/cma.c
- drivers/infiniband/core/ucma.c
- include/rdma/rdma_cm.h
- include/uapi/rdma/rdma_user_cm.h

用户态对应是：

- librdmacm/

什么时候该用 rdma_cm

适合：

- 你想像 socket 一样按地址建连
- 你不想手工交换 QP/LID/GID/PSN 等连接参数
- 你做 RC 类连接型程序

不一定用它的情况：

- 你自己有控制面
- 你自己交换所有 QP 参数
- 你做更底层 verbs 实验
- 某些 UD/特殊场景

一句话总结

rdma_cm 解决的是“如何把两个 RDMA 端点连起来并维护连接状态”，
而 libibverbs 解决的是“连起来之后如何操作 QP/CQ/MR 传数据”。

如果你愿意，我下一步可以直接按源码给你讲 rdma_create_id -> rdma_resolve_addr -> rdma_connect 这一条完整路径，
在 librdmacm 和内核 cma.c/ucma.c 里分别在哪里。

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
