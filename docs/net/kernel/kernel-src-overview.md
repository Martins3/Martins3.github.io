# 网络栈的一些源码分析

## 摘抄从 gregg 的 BCC book
> Sockets are defined by a sock struct embedded at the start of protocol
variants such as tcp_sock.
Network protocols are attached to socket using a struct proto,
such that there a tcp_prot,udp_prot,etc;
this struct defines callback functions for operating
the protocol,including for connect,sendmsg,and recvmsg.

> These include the new API(NAPI) interface
Receive Side Scaling(RSS), Receive Packet Steering(RPS)
Receive Flow Steering(RFS),
Accelerated RFS, and Transmit Packet Steering(XPS).

SO_REUSEPORT 可以让多个 process 使用相同的 port

使用 SYN Backlog 和 listen Backlog 来两个队列来构建
方式 SYN flooding 攻击。

> 最后，这里还有有一些高级的 perfermance optimizations 话题一带而过，在 P395
> 1. Nagle
> 2. Byte Queue Limits(BQL)
> 3. Pacing
> 4. TCP Small Queues(TSQ)
> 5. Early Departure Time(EDT)


## dsa : Distributed Switch Architecture
https://www.kernel.org/doc/Documentation/networking/dsa/dsa.txt

分布式 switch

## can : SocketCAN - Controller Area Network
https://www.kernel.org/doc/Documentation/networking/can.rst

- [ CAN 通信讲解](https://zhuanlan.zhihu.com/p/538834760)



## 调试 bmbt 的时候，打过断点的位置

### arch/x86/kernel/apic/msi.c: `irq_msi_compose_msg`

### drivers/net/ethernet/intel/e1000e/netdev.c

- `e1000_intr_msix_tx` => `e1000_clean_tx_irq` => `netdev_completed_queue` => `netdev_tx_completed_queue`

`e1000_clean_tx_ring` => `netdev_reset_queue` => `netdev_tx_reset_queue`

- `e1000_xmit_frame`
  - `netdev_sent_queue`
  - `e1000_tx_queue`

### net/core/dev.c
- `xmit_one`
  - `dev_queue_xmit`

### `net/ipv6/ip6_output.c`

- `ip6_finish_output2`

### net/ipv6/mcast.c
- `mld_ifc_start_timer`

### `net/sched/sch_generic.c`
- `sch_direct_xmit`

## 关键的结构体
### `struct socket`
```c
const struct proto_ops inet_stream_ops; // 对应 tcp ，而 SOCK_STREAM

const struct proto_ops inet_dgram_ops; // 对应 udp ，SOCK_DGRAM

static const struct proto_ops inet_sockraw_ops;
```

### `struct proto_ops`
每一个协议都会对应的注册，例如 ipv4

- `net_proto_family` : 只有一个 create hook，相比 `struct proto_ops` 是一个更大的分类，例如在 `inet6_family_ops` 下， 有 `inet6_dgram_ops` 和 `inet6_stream_ops`
- `net_device` ：基于 virtio_net 来看看吧
- `proto`
- `packet_type`
- `dst_entry`

```c
/* Networking protocol blocks we attach to sockets.
 * socket layer -> transport layer interface
 */
struct proto {
```
- `net_protocol`
  - ip_protocol_deliver_rcu : 接受的包中解析上层的协议是什么，如果是 TCP，那么使用 tcp 注册的协议

socket 的 ops 是一个指向 struct proto_ops 的指针，sock 的 ops 是一个指向 struct proto 的指针, 它们在结构被创建时确定[^1]
`socket->ops` 和 `sock->ops` 由前两个参数 socket_family 和 socket_type 共同确定。

以 INET 协议簇为例，注册接口是
```c
int inet_add_protocol(const struct net_protocol *prot, unsigned char protocol);
```

L2->L3 如出一辙。只不过注册接口变成了
```c
void dev_add_pack(struct packet_type *pt)
```

对于需要本机上送的报文
 rth->dst.input = ip_local_deliver;
对需要转发的报文
 rth->dst.input = ip_forward;
对本机发送的报文
 rth->dst.output = ip_output;

## 基本流程
- `sys_socket`
  - `sock_create`
  - `sock_map_fd` : 将 socket 和 fd 关联起来

## 关键目录
- net/ethernet/eth.c

eth_mac_addr : ethernet 控制 mac  2

- net/8021q

实现 vlan 的

### net/devlink

应该是驱动实现的合集，但是 virtio_net 是不需要这个

https://docs.kernel.org/networking/devlink/index.html

[^1]: https://switch-router.gitee.io/blog/linux-net-stack/

## net/core/ 下有什么?
linkwatch_event 搞不清楚是什么


| File                  | Lines | Code | Comments | Blanks | Comments |
|-----------------------|-------|------|----------|--------|----------|
| filter.c              | 11947 | 9545 | 792      | 1610   | bpf 相关 |
| dev.c                 | 11660 | 7760 | 2093     | 1807   |
| skbuff.c              | 6990  | 4535 | 1379     | 1076   |
| rtnetlink.c           | 6688  | 5316 | 315      | 1057   |
| sock.c                | 4215  | 3043 | 537      | 635    |
| pktgen.c              | 4086  | 3088 | 352      | 646    |
| neighbour.c           | 3894  | 3132 | 192      | 570    |
| net-sysfs.c           | 2108  | 1610 | 106      | 392    |
| flow_dissector.c      | 2053  | 1610 | 136      | 307    |
| drop_monitor.c        | 1785  | 1350 | 83       | 352    |
| sock_map.c            | 1715  | 1405 | 45       | 265    |
| net_namespace.c       | 1400  | 1014 | 170      | 216    |
| fib_rules.c           | 1319  | 1056 | 26       | 237    |
| skmsg.c               | 1253  | 1023 | 61       | 169    |
| dev_addr_lists.c      | 1050  | 611  | 307      | 132    |
| page_pool.c           | 954   | 612  | 176      | 166    |
| bpf_sk_storage.c      | 930   | 724  | 54       | 152    |
| datagram.c            | 923   | 597  | 206      | 120    |
| netpoll.c             | 867   | 670  | 51       | 146    |
| dev_ioctl.c           | 817   | 557  | 128      | 132    |
| xdp.c                 | 804   | 575  | 84       | 145    |
| gro.c                 | 767   | 552  | 84       | 131    |
| sysctl_net_core.c     | 753   | 660  | 19       | 74     |
| sock_reuseport.c      | 749   | 501  | 110      | 138    |
| lwt_bpf.c             | 657   | 514  | 29       | 114    |
| flow_offload.c        | 638   | 525  | 7        | 106    |
| utils.c               | 486   | 361  | 77       | 48     |
| gen_stats.c           | 485   | 297  | 137      | 51     |
| lwtunnel.c            | 427   | 332  | 14       | 81     |
| net-procfs.c          | 415   | 339  | 13       | 63     |
| selftests.c           | 410   | 327  | 12       | 71     |
| scm.c                 | 373   | 288  | 26       | 59     |
| sock_diag.c           | 343   | 279  | 3        | 61     |
| dst.c                 | 340   | 265  | 22       | 53     |
| failover.c            | 315   | 212  | 42       | 61     |
| netprio_cgroup.c      | 295   | 197  | 49       | 49     |
| link_watch.c          | 294   | 187  | 44       | 63     |
| gen_estimator.c       | 278   | 171  | 71       | 36     |
| gso_test.c            | 274   | 211  | 19       | 44     |
| gso.c                 | 273   | 131  | 106      | 36     |
| dev_addr_lists_test.c | 237   | 183  | 8        | 46     |
| ptp_classifier.c      | 228   | 116  | 96       | 16     |
| stream.c              | 220   | 141  | 52       | 27     |
| secure_seq.c          | 200   | 161  | 19       | 20     |
| fib_notifier.c        | 199   | 162  | 0        | 37     |
| dst_cache.c           | 183   | 141  | 7        | 35     |
| netdev-genl.c         | 175   | 135  | 1        | 39     |
| of_net.c              | 172   | 91   | 58       | 23     |
| netclassid_cgroup.c   | 152   | 110  | 14       | 28     |
| gro_cells.c           | 138   | 103  | 11       | 24     |
| request_sock.c        | 132   | 44   | 77       | 11     |
| tso.c                 | 89    | 70   | 5        | 14     |
| hwbm.c                | 85    | 62   | 9        | 14     |
| timestamping.c        | 71    | 51   | 6        | 14     |
| net-traces.c          | 68    | 52   | 6        | 10     |
| netevent.c            | 63    | 20   | 36       | 7      |
| netdev-genl-gen.c     | 48    | 35   | 6        | 7      |


> [!NOTE]
> 参考 Deepseeek ，有待验证

- consume_skb : 释放掉 skb ，主要场景为:
- 数据包成功交付到上层协议
- 数据包被明确丢弃
  - 防火墙规则丢弃数据包（如iptables规则匹配为DROP）。
  - 路由失败（如无可用路由或TTL过期）。
  - 协议栈处理错误（如校验和失败、无效包头）。
递减skb的引用计数，当计数归零时，释放其占用的内存及相关资源

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
