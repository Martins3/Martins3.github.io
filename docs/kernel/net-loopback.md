# loopback 网卡

分析一下:
![](./img/iperf.svg)

## [ ] 没有看到 `raise_softirq` 的调用
- `raise_softirq` 和 `do_softirq` 的调用数量是对等的吗？
- 物理网卡中，`raise_softirq` 是发生在的中断处理函数中，但是在 loopback 网卡中，是发生在哪里的？

## [ ] `net_tx_action` 会调用到 `napi_poll` 上，既然是没有中断的，为什么还是需要中断聚合的

## [ ] CPU A 和 CPU B 使用 softirq 通信，不使用 RPS 的时候，如果 CPU C 使用网络栈的内容，会导致 softirq 迁移吗

## [ ] What is the main difference between RSS, RPS and RFS
- https://stackoverflow.com/questions/44958511/what-is-the-main-difference-between-rss-rps-and-rfs

- 调用 `raise_softirq` 100% 是时钟中断触发的。
- `__raise_softirq_irqoff` 是真正插入 flag 的位置:

从 `ip6_finish_output2` 的位置开始，关闭和打开 softirq 的:

```txt
__raise_softirq_irqoff+1
enqueue_to_backlog+642
netif_rx_internal+58
__netif_rx+20
loopback_xmit+201
dev_hard_start_xmit+217
__dev_queue_xmit+2199
ip6_finish_output2+705
ip6_xmit+1048
inet6_csk_xmit+215
__tcp_transmit_skb+1333
tcp_write_xmit+830
tcp_sendmsg_locked+1283
tcp_sendmsg+40
sock_sendmsg+65
sock_write_iter+151
new_sync_write+371
vfs_write+521
ksys_write+167
do_syscall_64+59
entry_SYSCALL_64_after_hwframe+68
```

```txt
sk_filter_trim_cap+270
tcp_v6_rcv+3100
ip6_protocol_deliver_rcu+205
ip6_input_finish+64
__netif_receive_skb_one_core+99
process_backlog+137
__napi_poll+44
net_rx_action+571
__softirqentry_text_start+238
do_softirq.part.0+152
__local_bh_enable_ip+115
ip6_finish_output2+499
ip6_xmit+1048
inet6_csk_xmit+215
__tcp_transmit_skb+1333
tcp_write_xmit+830
tcp_sendmsg_locked+1283
tcp_sendmsg+40
sock_sendmsg+65
sock_write_iter+151
new_sync_write+371
vfs_write+521
ksys_write+167
do_syscall_64+59
entry_SYSCALL_64_after_hwframe+68
```

任何一次 softirq


```c
static const struct net_device_ops loopback_ops = {
	.ndo_init        = loopback_dev_init,
	.ndo_start_xmit  = loopback_xmit,
	.ndo_get_stats64 = loopback_get_stats64,
	.ndo_set_mac_address = eth_mac_addr,
};

/* Registered in net/core/dev.c */
struct pernet_operations __net_initdata loopback_net_ops = {
	.init = loopback_net_init,
};

static const struct net_device_ops blackhole_netdev_ops = {
	.ndo_start_xmit = blackhole_netdev_xmit,
};

static const struct ethtool_ops loopback_ethtool_ops = {
	.get_link		= always_on,
	.get_ts_info		= ethtool_op_get_ts_info,
};
```

- `loopback_xmit`
	- `__netif_rx`
		- `enqueue_to_backlog`


> However this three-way handshake takes some time. And during that time the connection is queued and this is the backlog.
>
> https://stackoverflow.com/questions/36594400/what-is-backlog-in-tcp-connections

> XDP
>
> https://www.iovisor.org/technology/xdp

> NAPI
>
>

似乎这就是 percpu 的 backlog ?
```c
/*
 *	Device drivers call our routines to queue packets here. We empty the
 *	queue in the local softnet handler.
 */

DEFINE_PER_CPU_ALIGNED(struct softnet_data, softnet_data);
EXPORT_PER_CPU_SYMBOL(softnet_data);
```

处理 backlog 的地方:
- `process_backlog`
- `napi_busy_loop`

## [ ] softirq 的工作流程

## [ ] 统计和 loopback 网卡有关的 softirq 的频率，和每次发生的数量
虽然，两个版本的网络栈变化很大，但是从 loopback 的这一个函数而已，其调用过程是非常清晰明了的吧，不存在其他的位置。

## iperf
https://load-balancer.inlab.net/manual/performance/measuring-internal-bandwidth-with-iperf/


## copy 是不是都是采用 SIMD 的


```c
void irq_exit(void)
{
#ifndef __ARCH_IRQ_EXIT_IRQS_DISABLED
	local_irq_disable();
#else
	lockdep_assert_irqs_disabled();
#endif
	account_irq_exit_time(current);
	preempt_count_sub(HARDIRQ_OFFSET);
	if (!in_interrupt() && local_softirq_pending())
		invoke_softirq(); ==================================》 __do_softirq

	tick_irq_exit();
	rcu_irq_exit();
	trace_hardirq_exit(); /* must be last! */
}
```
