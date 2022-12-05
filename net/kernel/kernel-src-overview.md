# 记录一下网络栈的一些源码分析

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

### net/core/neighbour.c
- `neigh_direct_output`

### `net/ipv6/ip6_output.c`

- `ip6_finish_output2`

### net/ipv6/mcast.c
- `mld_ifc_start_timer`

### `net/sched/sch_generic.c`
- `sch_direct_xmit`

## 关键的结构体
- `struct socket`
  - `struct proto_ops` : 每一个协议都会对应的注册，例如 inet
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

[^1]: https://switch-router.gitee.io/blog/linux-net-stack/
