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
