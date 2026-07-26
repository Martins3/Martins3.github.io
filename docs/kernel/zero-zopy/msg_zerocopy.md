# network zero copy
https://www.kernel.org/doc/html/latest/networking/msg_zerocopy.html

net/core/skbuff.c

测试代码在 ./iouring/op-send-zc.c

```txt
@[
        loopback_xmit+5
        dev_hard_start_xmit+96
        __dev_queue_xmit+1744
        ip_finish_output2+587
        __ip_queue_xmit+873
        __tcp_transmit_skb+2326
        tcp_write_xmit+641
        __tcp_push_pending_frames+57
        tcp_sendmsg_locked+1992 在这里去调用 -> skb_zerocopy_iter_stream+68 -> io_link_skb+5
        tcp_sendmsg+47
        sock_sendmsg+262
        io_send_zc+150
        __io_issue_sqe+56
        io_issue_sqe+55
        io_submit_sqes+274
        __do_sys_io_uring_enter+516
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1
```
也就是在组装 skb 的时候，只是使用用户态的内存来拷贝。
算是一个软件上折中优化。

## scatter-gather

网卡是否有
```c
    for iface in $(ls /sys/class/net/); do
         echo "=== $iface ==="
         sudo ethtool -k "$iface" | grep -i scatter
     done
```

sendfile 要真正实现“零拷贝”，确实依赖网卡的 scatter-gather (SG) 能力，原因如下：

没有 sendfile 时的两次拷贝

```text
  磁盘 -> 内核 Page Cache -> 用户缓冲区 -> 内核 Socket Buffer -> 网卡 DMA
                       ↑ 拷贝 1          ↑ 拷贝 2
```

普通 read() + write() 有 两次 CPU 拷贝。

有 sendfile 但网卡不支持 SG

```text
  磁盘 -> 内核 Page Cache -> 内核 Socket Buffer -> 网卡 DMA
                       ↑ 拷贝 1
```

sendfile() 把文件页缓存直接读到 Socket Buffer，省掉了“用户态缓冲区”那一次拷贝，但 Page Cache -> Socket Buffer 这次拷贝还在。

有 sendfile + 网卡支持 SG

```text
  磁盘 -> 内核 Page Cache ───────┐
                                 ├──> 网卡 DMA（SG 直接收集多段内存）
        Socket Buffer 只存元数据 ┘
```

网卡支持 SG 后，内核不必把 Page Cache 里的页面内容复制到 Socket Buffer。它可以：

1. 构造一个 scatter-gather 列表，让 DMA 直接从 Page Cache 页面取数据。
2. Socket Buffer 里只放头部/元数据。
3. 网卡 DMA 把多个不连续的物理页“收集”起来发出去。

这样才真正接近零拷贝。

还需要 TSO/GSO 配合

如果数据包大于 MTU，内核需要把大段数据拆成多个 MTU 大小的包。
如果网卡支持 TSO (TCP Segmentation Offload)，网卡可以自己拆分，CPU 完全不用碰
payload；否则内核还得把数据复制/切分到 Socket Buffer 里做分片，就又破坏了零拷贝。

## tx-scatter-gather 和 tx-scatter-gather-fraglist

tx-scatter-gather 和 tx-scatter-gather-fraglist 都是网卡 TX offloading 特性，区别主要在于"分散"的粒度不同：

tx-scatter-gather（SG）

- 数据可以分散在同一个 skb 的多个 page fragment 中
- 网卡能直接从多个不连续的内存页 DMA 数据并发送
- 避免内核把数据拷贝成连续缓冲区
- 典型场景：大页内存、零拷贝 sendfile、TSO/GSO 之前的数据包

tx-scatter-gather-fraglist（FRAGLIST）

• 数据可以分散在多个 skb 组成的链表中（skb_shinfo(skb)->frag_list）
• 主 skb 后面挂一串子 skb，网卡能把这一串当作一个整体发送
• 典型场景：GSO 分段后的包、某些隧道封装、需要将多个 packet 一起 offloaded 的情况

核心区别

┌────────────────────────────┬─────────────────────────────┬───────────────┐
│ 特性                       │ 分散对象                    │ 层级          │
├────────────────────────────┼─────────────────────────────┼───────────────┤
│ tx-scatter-gather          │ 一个 skb 内的多个 page frag │ 页/缓冲区级别 │
├────────────────────────────┼─────────────────────────────┼───────────────┤
│ tx-scatter-gather-fraglist │ 多个 skb 组成的 frag_list   │ skb/包级别    │
└────────────────────────────┴─────────────────────────────┴───────────────┘

简单说：sg 解决一个包的数据可以不连续；fraglist 解决多个包可以串成链一起发。


物理机上:
```txt
🧀  /home/martins3/data/vn/docs/kernel/topics/check.sh
=== br-717fe5390dab ===
[sudo] password for martins3:
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== br-86f1222c7dcb ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== br-in ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== docker0 ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== enp5s0 ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: off [fixed]
=== enp6s0 ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: off [fixed]
=== lo ===
scatter-gather: on
	tx-scatter-gather: on [fixed]
	tx-scatter-gather-fraglist: on [fixed]
=== Meta ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== ovs-system ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== tailscale0 ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== veth04430e0 ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== vif_s_63_0 ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
=== wlan0 ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: off [fixed]
```

虚拟机中:
```txt
/home/martins3/data/vn/docs/kernel/topics/check.sh

=== ens5 ===
[sudo] password for martins3:
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: off [fixed]
=== ens6 ===
scatter-gather: off
	tx-scatter-gather: off [fixed]
	tx-scatter-gather-fraglist: off [fixed]
=== lo ===
scatter-gather: on
	tx-scatter-gather: on [fixed]
	tx-scatter-gather-fraglist: on [fixed]
=== sit0 ===
scatter-gather: on
	tx-scatter-gather: on
	tx-scatter-gather-fraglist: on
```

## iouring

IORING_OP_SEND_ZC 是 io_uring 版的 MSG_ZEROCOPY，把零拷贝发送和异步 io_uring 事件循环整合在一起。

它解决了 MSG_ZEROCOPY 的什么痛点

MSG_ZEROCOPY 最大的麻烦是完成通知机制丑陋：

- 要通过 poll(POLLERR) 感知
- 再通过 recvmsg(MSG_ERRQUEUE) 读通知
- 这套逻辑跟正常收发路径割裂，写起来很别扭

IORING_OP_SEND_ZC 把这个通知直接收进 io_uring 的 CQE 里，不用再去搞 socket error queue。

核心机制

提交一个 IORING_OP_SEND_ZC SQE 后，通常会收到两个 CQE：

第一个 CQE

• 表示这个 send 请求已经被内核处理（数据已经交给网络栈）
• 和普通 send 一样

第二个 CQE

- 才是真正的 zero-copy completion notification
- 表示内核已经释放对 buffer page 的引用，应用可以安全重用这块内存
- 会带 IORING_CQE_F_MORE 标志，提示"还有后续通知"
- 如果返回 IORING_CQE_F_NOTIF 相关标记，说明这是通知类 CQE

这样应用只需要在一个 io_uring 事件循环里处理所有事情，不需要额外维护 error queue。

和 MSG_ZEROCOPY 的对比

┌──────────┬─────────────────────┬───────────────────────────┐
│          │ MSG_ZEROCOPY        │ IORING_OP_SEND_ZC         │
├──────────┼─────────────────────┼───────────────────────────┤
│ API      │ send/sendmsg + flag │ io_uring SQE              │
├──────────┼─────────────────────┼───────────────────────────┤
│ 完成通知 │ socket error queue  │ 额外的 CQE                │
├──────────┼─────────────────────┼───────────────────────────┤
│ 通知方式 │ poll + recvmsg      │ io_uring 事件循环统一处理 │
├──────────┼─────────────────────┼───────────────────────────┤
│ 编程模型 │ 复杂、割裂          │ 统一、简洁                │
├──────────┼─────────────────────┼───────────────────────────┤
│ 底层实现 │ page pinning        │ 同样是 page pinning       │
└──────────┴─────────────────────┴───────────────────────────┘

使用注意:

1. 同样需要 pin page
    • 所以一样受 RLIMIT_MEMLOCK 限制
    • 大包才划算
2. buffer 不能立即复用
    • 第一个 CQE 返回不代表可以重写 buffer
    • 必须等到第二个 notification CQE
3. 可能 fallback 到拷贝
    • 和 MSG_ZEROCOPY 一样，设备不支持 scatter-gather 时会拷贝
    • 可以通过 CQE flags 判断
4. 需要内核版本支持
    - 较新的内核才支持 IORING_OP_SEND_ZC
    - 还有对应的 **fixed buffer / registered buffer** 配合起来用更稳

## [ ] loopback 为什么 MSG_ZEROCOPY 没有意义
(这个说法我感觉很奇怪，按道理，如果 MSG_ZEROCOPY 的话，
即便是 loopback ，可以 ping sender 的 page ，然后直接拷贝到
receiver 的用户态 buffer 中去的，所以我认为这次 codex 在扯淡
还需要继续看看内核文档)

MSG_ZEROCOPY + loopback 大致变成：

sender userspace
  -> send(MSG_ZEROCOPY)
     不立即 copy，skb frag 引用发送进程用户页

loopback/RX/local delivery
  -> skb_orphan_frags_rx()
     第 1 次：发送进程用户页 -> 内核私有页
     这是 deferred copy，不是省掉

receiver recv()
  -> copy_to_iter()
     第 2 次：内核页 -> 接收进程用户 buffer

所以端到端还是两次：

普通 loopback：
  sender user -> kernel
  kernel -> receiver user

MSG_ZEROCOPY loopback：
  sender user -> kernel   # 只是推迟到 RX/local delivery
  kernel -> receiver user

区别是第一拷贝的位置变了：从发送 syscall 里移到了 RX/local delivery 侧。loopback 设备本身不是主要复制点，它的
loopback_xmit() 基本是 skb_orphan()、设置协议、然后 __netif_rx(skb) 重新进入接收路径：drivers/net/
loopback.c:70。

所以你的理解是对的：默认 loopback 端到端是两次 copy；MSG_ZEROCOPY 在 loopback 上通常不是把两次变一次，而是把
第一次 copy 延后，最后仍然两次，还多了 zerocopy 管理成本。

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
