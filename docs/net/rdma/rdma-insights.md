# RDMA 杂谈
https://www.zhihu.com/column/c_1231181516811390976

- 使用 Soft Roce 进行实验 : https://zhuanlan.zhihu.com/p/361740115
	- 当时的记录在 : collei/nvme.sh

- RDMA之基于Socket API的QP间建链 : https://zhuanlan.zhihu.com/p/476407641
  - 基本通信的各种 key 都说明了

HCA 是 Host Channel Adapter。

### 操作类型
<!-- 8fad06db-dfe1-4310-a97b-02bb45f160dd -->
https://zhuanlan.zhihu.com/p/142175657

这个问题非常经典，才发现原来才可以有 READ / WRITE

### Memory Region
<!-- c9528961-1e75-4799-881d-881fc6ddddfd -->
https://zhuanlan.zhihu.com/p/156975042

你应该怎么理解 MR

可以把 MR 理解成三层约束的组合：
- 一段已经注册过的内存
- 一组访问权限
- 一对给硬件校验用的 key

lkey 和 rkey 是什么:
- lkey：本地 key，给本端网卡校验“你能不能访问本地这块 buffer”
- rkey：远端 key，给远端网卡校验“你能不能访问对端那块 buffer”

权限的含义:
IBV_ACCESS_* 在 libibverbs/verbs.h 里定义，常见的是：
- IBV_ACCESS_LOCAL_WRITE
- IBV_ACCESS_REMOTE_WRITE
- IBV_ACCESS_REMOTE_READ
- IBV_ACCESS_REMOTE_ATOMIC

也就是如果没有 lkey rkey ，那么完全无法访问，即便可以访问，也需要有权限的考虑。

对应场景：
- SEND/RECV、本地 SGE 引用本地 buffer 时，常见用 lkey
- RDMA READ/WRITE/ATOMIC 访问远端内存时，要提供远端地址 + rkey


如果按常见 RDMA 教学结构，这篇会沿着这条链讲：

1. 应用有一块普通内存
2. 调 ibv_reg_mr() 把它注册成 MR
3. 得到 lkey/rkey
4. 本地发 WR 时，SGE 里写本地地址和 lkey
5. 若是 RDMA WRITE/READ，还要写远端地址和 rkey
6. HCA 校验 key 和权限，合法才执行 DMA

这是 RDMA MR 最标准的认知框架。

最容易混淆的点
1. malloc() 出来的地址不能直接给 RDMA 用
   错。必须先注册成 MR。
2. 有虚拟地址就够了
   错。网卡需要的是注册后的 DMA 映射和 key，不是单纯用户态 VA。
3. rkey 只是个“句柄”
   不完整。它本质上还是访问控制的一部分，不只是索引。
4. 远端知道地址就能写
   错。还需要匹配的 rkey，并且权限允许远程写。

和你现在看内核/rdma-core 的关系

如果把这篇文章和源码对照，你可以这样映射：

- 用户态注册 MR：ibv_reg_mr()
- libibverbs/provider 把请求发到内核
- 内核/uverbs 和驱动创建对应 MR 对象
- 驱动把页 pin 住，并建立 HCA 侧 MTT/MTPT 一类映射
- 最后返回 lkey/rkey

### Protection Domain : https://zhuanlan.zhihu.com/p/159493100

1. 为什么已经有 lkey/rkey 了，还要 PD

只有 lkey/rkey 还不够。
文章举的意思是：如果没有 PD，那么只要远端已经和本端某个 QP 建好了连接，理论上它一旦拿到某个 MR 的 VA + rkey，就可能访问那块 MR。
PD 就是在此基 础上再加一层隔离。来源：

所以安全模型变成两层：
- 第一层：rkey/lkey + access flags
- 第二层：QP/MR 必须属于同一个 PD

也就是说，即使你“知道 key”，但如果 QP 和 MR 不在同一个 PD，硬件仍然会拒绝。

  2. PD 到底在隔离什么

  它隔离的不是“网络连通性”，而是“资源组合合法性”。

  文章里明确说了：

- 一个点可以有多个 PD
- 每个 QP 都属于一个 PD
- 每个 MR 也属于一个 PD
- 不同 PD 中的资源不能一起使用

3. 这和远端访问有什么关系

PD 是本地概念，但它会影响远端能访问什么。
这是这篇文章最重要的一点：
- 远端节点不是直接“访问 MR”
- 它是通过“和本端某个 QP 建立连接”来触发访问
- 而本端这个 QP 所在的 PD，会限制它能配合哪些本地 MR 使用

所以远端能访问到哪块内存，不只是由 rkey 决定，还取决于：

- 它连到了本端哪个 QP
- 这个 QP 和目标 MR 是否属于同一个 PD

这就是 PD 的真实价值：
把“一个连接能碰到哪些本地资源”限定在一个更小的集合里。

4. 软件上怎么体现

文章里也讲了软件模型：

- 用户先创建 PD
- 再创建 QP/MR
- 创建时把这个 PD 的句柄传进去
- 后续硬件收发时，会检查资源的 PD 是否匹配

这和 rdma-core 的接口是完全一致的：

- ibv_alloc_pd() 先拿一个 PD
- ibv_reg_mr(pd, ...) 把 MR 挂到这个 PD
- ibv_create_qp(pd, ...) 把 QP 挂到这个 PD

所以从 API 设计上你就能看到 PD 是“上游父对象”。

5. 你该怎么用它

这篇文章的建议很实用：

- 如果你想隔离不同连接、不同租户、不同业务流，就分多个 PD
- 如果你不追求更细粒度隔离，一个进程/一个上下文只用一个 PD 也可以

也就是说，PD 不是“越多越对”，而是一个安全与管理粒度的设计选择。

6. 用一句工程化的话总结

你可以把 PD 理解成：

- MR 负责“这块内存能不能被访问”
- rkey/lkey 负责“访问时你有没有钥匙”
- PD 负责“这把钥匙和这个通道是不是属于同一个隔离域”

三者叠加，才构成完整的访问控制。

如果你愿意，我下一步可以直接结合 rdma-core 源码，把：

- ibv_alloc_pd()
- ibv_reg_mr()
- ibv_create_qp()

这三路径里 PD 是怎么一路传到内核和驱动里的，给你串一遍。


### Memory Window : https://zhuanlan.zhihu.com/p/353590347
1. MW 是什么

MW 不是新的内存注册对象来替代 MR，而是在 MR 之上增加一层“可动态授权的远程访问窗口”。

核心目的就是：

- 不想频繁 dereg/reg/rereg MR
- 但又想动态授予或撤销远程访问权限
- 或者想只开放一个 MR 里的某一段给远端

所以可以把它理解成：

- MR：底层真正被注册的内存
- MW：临时绑在 MR 上的一张“远程访问通行证”

二级来源明确提到：MW 的用途是“动态地授予和收回已注册缓冲区的远程访问权限”，并且不同 MW 可以绑定到同一个 MR，且权限可不同。来源：

- https://blog.csdn.net/Xinzhaohaha/article/details/118939545

2. 为什么有 MR 了还要 MW

因为 MR 太“硬”。

如果你只靠 MR 控制远程访问，常见问题是：

- 权限一旦注册好，不够灵活
- 想临时开放一小段内存给某个远端时，代价大
- 想快速撤销远端访问时，重注册 MR 成本高

MW 解决的是“动态授权”问题。

你可以把它看成：

- MR 决定“这块内存总体可不可以被硬件管理”
- MW 决定“眼下远端能不能、以及能访问哪一段”

3. MW 的关键动作是 bind

文章的核心一定会讲 bind，因为 MW 不是单独工作的。

MW 必须绑定到某个 MR 上，绑定时会指定：

- 关联哪个 MR
- 起始地址
- 长度
- 远程访问权限
- 新的 rkey

也就是说，MW 本身不拥有内存，它只是把“MR 的一个子范围”以新的授权方式暴露出去。

所以你可以这样理解：

- MR 是底板
- MW 是在底板上贴出来的一小块可访问区域

4. MW 和 rkey 的关系

这里是最容易搞清楚的点。

MW 绑定之后，会产生或使用一个新的 rkey 语义。
远端如果想通过这个 window 访问内存，不是拿原始 MR 的 rkey，而是拿这个 window 当前有效的 rkey。

因此：

- MR 有自己的 key
- MW 绑定后也体现为一个单独的远程访问 capability

这就是它比裸 MR 更灵活的地方：
你可以通过重新 bind / invalidate，改变“当前有效的远程钥匙”。

5. MW 最重要的能力：撤销快

MW 的真正价值不在“能访问”，而在“能收回”。

如果远端访问授权只靠 MR，那么收回权限通常更重。
如果用 MW，就可以：

- bind 一个 window 开放权限
- 用完后 invalidate
- 这样远端即使还记得旧地址，也不能再用旧 key 访问

所以 MW 非常适合：

- 临时共享缓冲区
- 分阶段开放远程写权限
- 做更细粒度的会话级授权

6. 它和 PD/MR/QP 的关系

你前面已经看了 MR 和 PD，这里刚好能串起来：

- PD：隔离域，保证资源组合合法
- MR：注册内存，给硬件稳定映射
- MW：在 MR 上动态开放一个可远程访问的窗口
- QP：通信通道，真正发起 bind、send、rdma write/read 等操作

所以 MW 不是独立于 MR 的体系，而是建立在 MR 之上的控制层。

7. 用一句工程化的话总结

这篇文章的本质是在讲：

MR 负责“把内存交给 RDMA 系统管理”，
MW 负责“把其中某个范围以更灵活、可撤销的方式授权给远端访问”。

如果你只记一个结论，就记这个。

8. 和 rdma-core/verbs 对应

在 verbs 语义里，这通常对应：

- 先有 ibv_reg_mr()
- 再有 ibv_alloc_mw()
- 然后通过 bind 操作把 MW 绑到 MR 上
- 远端拿到对应 rkey 才能访问
- 之后可以 local invalidate / send with invalidate / rebind 来收回或更新权限

你前面问过 lkey/rkey 和 invalidate_rkey，这和 MW 正好强相关。MW 就是让 rkey 的生命周期管理更灵活。

如果你愿意，我下步可以直接结合 rdma-core 源码，把：

- ibv_alloc_mw
- bind_mw
- invalidate_rkey




## 基本内容
PD (Protection Domain)
MR (Memory Region)
CQ (Completion Queue)
QP (Send Queue & Receive Queue)

## 为什么 rdma 需要这四个模式
- RC = Reliable Connected
- UC = Unreliable Connected
- UD = Unreliable Datagram

## RDMA 的内存如何释放问题
<!-- eb2c72b8-a586-40fd-a224-9c7bf9b7a4ed -->

› 我想知道，当访问一个远端的内存的时候，如何保证远端的内存被正确的释放，如果远端的用户态进程 crash 了，如何保证 client 依旧可以 WRITE ，如果
  client 想要释放内存，server 如何释放，即便 server 的用户态程序 crash 了

> [!NOTE]
> 参考神奇海螺的意见，有待验证

做不到“远端用户态进程 crash 了，但 client 还能继续可靠地写那块原内存，并且最后还能正确释放”这一组要同时成立。

因为 RDMA 远端内存不是独立资源，它依附于远端进程的：

- ibv_context
- PD
- MR
- 用户虚拟地址空间
- 相关页 pin 和驱动对象

远端进程一旦 crash 或退出，内核会清理这些对象，MR/rkey/QP 最终都会失效。之后 client 不能再合法地继续 WRITE 到原来的那块用户态内存。

先说结论

你要区分两件事：

1. “client 能不能在 server 用户态死后继续写原 buffer”

- 正常 RDMA 用户态模型下，不能保证
- server 进程死了，MR 最终会被注销，rkey 失效，QP/连接也通常会断

2. “server 用户态死了以后，内存和资源能不能最终正确释放”

- 可以，靠内核清理
- 但这意味着 client 失去访问能力，而不是继续访问

所以你的三个目标里：

- 正确释放
- server crash 后 client 继续 write
- client 主动释放时 server 即使 crash 也能配合释放

这三个不能直接靠普通 userspace RDMA 一起满足。

为什么不能

因为远端 RDMA write 依赖两个前提：

- 远端那块内存对应的 MR 还存在
- 远端给你的 rkey 还有效

而这两者都是远端进程活着时建立的 userspace/verbs 资源。

远端进程 crash 后，内核会做清理：

- fd 关闭
- uverbs 对象销毁
- QP/CQ/MR/PD 被释放
- pin 的页解除
- rkey 失效

所以 client 后续再写，最好的情况是失败并得到错误；更糟时如果你设计不当，会遇到连接中断或远端访问错误。

“正确释放远端内存”到底由谁负责

普通模型下，远端内存释放责任在 server 本地：

- server 分配内存
- server 注册 MR
- server 把 addr/rkey/len 发给 client
- server 决定何时 revoke / invalidate / dereg / free

client 只有“使用权”，没有真正“释放远端内存”的所有权。

所以如果你问：

> client 想要释放内存，server 如何释放

标准答案是：

- client 不能直接 free 远端内存
- client 只能发一个协议消息，通知 server “这块远端 buffer 我不用了”
- server 收到后自己 dereg_mr + free

如果 server 用户态 crash 了怎么办

那就别指望通过原用户态协议完成“优雅释放”了。此时只有两种可能：

1. 内核自动回收

- 这是默认情况
- 好处：不会永久泄漏 verbs 资源
- 坏处：client 不能再继续访问

2. 这块内存本来就不是 server 普通用户进程私有

- 比如它来自某个长期存活的 owner
- server 只是控制面，不是资源真正 owner
- 那 server crash 后仍可由别的控制实体接管

这已经不是普通“一个 server 进程 malloc+reg_mr”的模型了，而是更复杂的资源托管模型。

如果你真的想让 server crash 后 client 还继续写，怎么做

只能改变资源所有权模型。常见思路有几种。

1. 独立的内存 owner 进程
不要让业务 server 直接拥有 MR，而是：

- 单独起一个长期存活的 memory daemon
- 由它负责分配、注册、维护 MR
- 业务 server 只是把这些 token 分发给 client
- server crash 后，memory daemon 还活着，MR 还在

这样 client 理论上还能继续写，但前提是：

- QP/连接路径也还有效，或者能被重建
- owner 进程保留 MR 和访问授权
- 资源生命周期由 owner 管，而不是业务 server

2. 共享内存/外部内存池 + 独立注册者
比如：

- hugepage/file-backed/dmabuf/shared memory
- 由稳定进程注册 MR
- 业务进程 crash 不等于资源 owner 消失

本质和上面一样，都是把“内存 owner”和“业务逻辑进程”分离。

3. 内核态/设备态 owner
如果资源由内核驱动、target、或设备侧长期对象持有，那用户态 crash 不一定导致资源立刻消失。
但这已经超出一般 libibverbs userspace buffer 模型了。

推荐的工程做法

如果你是在设计普通 client/server RDMA 协议，正确做法通常不是“server crash 后 client 继续写”，而是：

- 接受 server crash 会使远端 MR 失效
- 用协议做租约/epoch/generation 管理
- client 每次访问都基于当前有效的 descriptor
- server 重启后重新分配/注册 MR，发新的 addr/rkey
- client 检测连接断开或访问失败后重新握手

也就是把它设计成：

- descriptor = {addr, rkey, len, generation}
- generation 变化就说明旧授权作废
- client 不得持久相信旧 rkey 永远有效

如果你想要“client 触发释放，即使 server crash 也最终能释放”

那释放动作不能只依赖 crash 的 server 进程本身。可选办法：

- 引入独立 resource manager
- client 向 resource manager 报告 release
- manager 负责真正 dereg/free
- 若业务 server crash，manager 仍能清理

或者更简单：

- 不让 client 负责远端内存释放
- 全部交给 lease/timeout/GC
- server 或 manager 周期性回收过期 MR

这通常比“client 精确命令远端 free”更稳。

一句话总结

普通 RDMA 用户态模型下：

- server crash 后，不能保证 client 继续写原远端内存
- 能保证的是内核最终清理这些资源
- client 不能直接释放远端内存，只能通过协议请求 server 或独立 owner 回收
- 如果你想在 server crash 后仍保持远端内存有效，必须把 MR 的所有权从业务 server 进程里拆出去

如果你愿意，我可以下一步直接给你画三种可落地的生命周期设计：

1. 普通 client/server MR 模型
2. 带 lease 的可恢复模型
3. 独立 memory daemon 的强托管模型




## virtio rdma 总结
https://zhuanlan.zhihu.com/p/651023182 : 很好的总结

https://patchew.org/QEMU/20210902130625.25277-1-weijunji@bytedance.com/ : 一个 pr

vmware PVRDMA :
- https://blogs.vmware.com/vsphere/2020/10/para-virtual-rdma-support-for-native-endpoints.html
- https://www.vmware.com/docs/pvrdma-hpc-setup-perf

- bytedance virtio rdma : https://kvm-forum.qemu.org/2022/virtio-net%20with%20RoCE%20v1.1.pptx
  - 至少可以从这里容易知道内核中的网络如何工作的

## 为什么 RoceV2 要把 UDP 和 IP 加进来?
<!-- b968a716-bebd-4ed1-a1eb-7bf2a71a19e1 -->

RoCE v1: Ethernet link layer protocol
RoCE v2: Internet layer protocol which exists on top of either the UDP/IPv4 or the UDP/IPv6 protocol

## RDMA HCA 是什么东西?
<!-- ac4f991e-4bd4-4f61-93a8-eff9e31b4abc -->

The RDMA application speaks to the Host Channel Adapter (HCA) direclty using the RDMA Verbs API.
You can see a HCA as a RDMA capable Network Interface Card (NIC).
To transport RDMA over a network fabric, InfiniBand, RDMA over Converged Ethernet (RoCE), and iWARP are supported.

https://www.vmware.com/docs/the-basics-of-remote-direct-memory-access-rdma-in-vsphere

## 开始的
- https://winddoing.github.io/post/f4fa9e36.html

## 协议的对比

> [!NOTE]
> 参考神奇海螺的意见，有待验证

| 指标       | InfiniBand         | RoCEv2       | 差距         |
| :----      | :----------------- | :----------- | :-----       |
| 端到端延迟 | ~100ns             | ~230ns+      | 2.3×+        |
| 链路带宽   | 400-800 Gb/s (NDR) | 100-400 Gb/s | 领先一代     |
| 路由机制   | 自适应线性路由     | 标准IP路由   | 更优负载均衡 |


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
