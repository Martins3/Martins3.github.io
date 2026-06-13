## wireguard

> Maybe the code isn't perfect, but I've skimmed it, and compared to the horrors that are OpenVPN and IPSec, it's a work of art.
>
> Linus

- [Linux Networking Shallow Dive: WireGuard, Routing, TCP/IP and NAT](https://news.ycombinator.com/item?id=36040803)
  - 看不懂

## 使用

- https://www.wireguard.com/quickstart/ : 非常详细的

### wg debug

- https://unix.stackexchange.com/questions/751066/wireguard-one-of-the-peers-can-t-ping-other-peers-but-handshake-works-and-othe

```txt
sudo tcpdump -tttnei wg0 icmp
sudo tcpdump -ttttni any 'udp port 51820'
```


## ip a 看到的 tailscale0 是什么:

是 tap 设备 : /sys/devices/virtual/net/tailscale0/tun_flags


## 基于 wireguard 的应用

- https://github.com/gravitl/netmaker
  - https://www.netmaker.io/resources/tailscale-vs-wireguard
- https://github.com/netbirdio/netbird
- https://github.com/firezone/firezone

## iperf3 && perf 的结果

```txt
-   10.48%     0.00%  kworker/3:1-wg-  [kernel.kallsyms]         [k] ret_from_fork_asm                                                                                                                                                                       ◆
     ret_from_fork_asm                                                                                                                                                                                                                                       ▒
     ret_from_fork                                                                                                                                                                                                                                           ▒
     kthread                                                                                                                                                                                                                                                 ▒
   - worker_thread                                                                                                                                                                                                                                           ▒
      - 10.47% process_scheduled_works                                                                                                                                                                                                                       ▒
         - 6.83% wg_packet_decrypt_worker                                                                                                                                                                                                                    ▒
            - 6.82% __local_bh_enable_ip                                                                                                                                                                                                                     ▒
               - 6.82% do_softirq                                                                                                                                                                                                                            ▒
                  - handle_softirqs                                                                                                                                                                                                                          ▒
                     - net_rx_action                                                                                                                                                                                                                         ▒
                        - 6.81% __napi_poll                                                                                                                                                                                                                  ▒
                           - 5.66% virtnet_poll                                                                                                                                                                                                              ▒
                              - 5.38% receive_buf                                                                                                                                                                                                            ▒
                                 - 4.61% napi_gro_receive                                                                                                                                                                                                    ▒
                                    - 4.48% netif_receive_skb_list_internal                                                                                                                                                                                  ▒
                                       - 4.44% __netif_receive_skb_list_core                                                                                                                                                                                 ▒
                                          - 4.38% ip_list_rcv                                                                                                                                                                                                ▒
                                             - 4.34% ip_sublist_rcv                                                                                                                                                                                          ▒
                                                - 4.09% ip_local_deliver                                                                                                                                                                                     ▒
                                                   - 3.87% ip_local_deliver_finish                                                                                                                                                                           ▒
                                                      - 3.86% ip_protocol_deliver_rcu                                                                                                                                                                        ▒
                                                         - 3.72% udp_unicast_rcv_skb                                                                                                                                                                         ▒
                                                            - 3.71% udp_queue_rcv_one_skb                                                                                                                                                                    ▒
                                                               - 3.68% wg_receive                                                                                                                                                                            ▒
                                                                  - wg_packet_receive                                                                                                                                                                        ▒
                                                                       2.59% queue_work_on                                                                                                                                                                   ▒
                             1.15% wg_packet_rx_poll                                                                                                                                                                                                         ▒
         - 3.63% wg_packet_encrypt_worker                                                                                                                                                                                                                    ▒
            - 3.62% __local_bh_enable_ip                                                                                                                                                                                                                     ▒
               - 3.61% do_softirq                                                                                                                                                                                                                            ▒
                  - handle_softirqs                                                                                                                                                                                                                          ▒
                     - net_rx_action                                                                                                                                                                                                                         ▒
                        - __napi_poll                                                                                                                                                                                                                        ▒
                           - 3.00% virtnet_poll                                                                                                                                                                                                              ▒
                              - 2.85% receive_buf                                                                                                                                                                                                            ▒
                                 - 2.45% napi_gro_receive                                                                                                                                                                                                    ▒
                                    - 2.38% netif_receive_skb_list_internal                                                                                                                                                                                  ▒
                                       - 2.36% __netif_receive_skb_list_core                                                                                                                                                                                 ▒
                                          - 2.33% ip_list_rcv                                                                                                                                                                                                ▒
                                             - 2.30% ip_sublist_rcv                                                                                                                                                                                          ▒
                                                - 2.18% ip_local_deliver                                                                                                                                                                                     ▒
                                                   - 2.06% ip_local_deliver_finish                                                                                                                                                                           ▒
                                                      - 2.06% ip_protocol_deliver_rcu                                                                                                                                                                        ▒
                                                         - 1.99% udp_unicast_rcv_skb                                                                                                                                                                         ▒
                                                            - 1.98% udp_queue_rcv_one_skb                                                                                                                                                                    ▒
                                                               - 1.97% wg_receive                                                                                                                                                                            ▒
                                                                  - wg_packet_receive                                                                                                                                                                        ▒
                                                                       1.38% queue_work_on                                                                                                                                                                   ▒
                             0.61% wg_packet_rx_poll
```

## 可以测试一下吗?

```c
static const struct net_device_ops netdev_ops = {
	.ndo_open		= wg_open,
	.ndo_stop		= wg_stop,
	.ndo_start_xmit		= wg_xmit,
	.ndo_get_stats64	= dev_get_tstats64
};
```

https://icloudnative.io/posts/wireguard-endpoint-discovery-nat-traversal/

- https://www.wireguard.com/papers/wireguard.pdf

### 有趣的

https://ubuntu.com/server/docs/introduction-to-wireguard-vpn

## 扩展阅读

- [A Cryptographic Analysis of the WireGuard Protocol](https://www.wireguard.com/papers/dowling-paterson-computational-2018.pdf)
- [Analysis of the WireGuard protoco]() 硕士论文

https://pboyd.io/posts/securing-a-linux-vm/

https://blog.scottlowe.org/2021/02/22/setting-up-wireguard-for-aws-vpc-access/

https://github.com/pufferffish/wireproxy

https://github.com/jodevsa/wireguard-operator

https://tailscale.com/blog/how-tailscale-works

## 应该思考下，如何使用一个 server 来，然后让所有的虚拟机都连到一起

为什么感觉 wireguard 只是可以 p2p 了 ?

就是这个了，到时候在说吧:
https://zdyxry.github.io/2019/03/23/Wireguard-%E4%BD%93%E9%AA%8C/

## 代码简单的分析

代码量很小:

```txt
 ./noise.c                     861
 ./netlink.c                   644
 ./receive.c                   586
 ./device.c                    472
 ./socket.c                    437
 ./send.c                      414
 ./allowedips.c                389
 ./timers.c                    243
 ./peer.c                      239
 ./cookie.c                    236
 ./peerlookup.c                226
 ./ratelimiter.c               223
 ./queueing.c                  109
 ./main.c                       78
```

## noise

似乎需要准备一些材料才可以看懂:

- https://noiseexplorer.com/patterns/IKpsk2/
- https://bblanche.gitlabpages.inria.fr/CryptoVerif/WireGuard/talk.pdf : 完全看不懂啊
- https://www.wireguard.com/papers/dowling-paterson-computational-2018.pdf
- https://www.wireguard.com/papers/kobeissi-bhargavan-noise-explorer-2018.pdf

- https://github.com/noisysockets/noisysockets

  - 似乎只要实现了 noise，那么就可以在用户态实现 wireguard ?

- https://news.ycombinator.com/item?id=39688545

## nixos

- https://nixos.wiki/wiki/WireGuard
- https://alberand.com/nixos-wireguard-vpn.html
- https://discourse.nixos.org/t/which-should-i-use-networking-wg-quick-or-networking-wireguard/13472/2

## 其他的 vpn 技术

- https://www.reddit.com/r/linuxquestions/comments/1cufbcd/what_is_the_best_vpn_for_linux/
- https://account.protonvpn.com/signup
- https://github.com/anderspitman/awesome-tunneling
- https://github.com/rapiz1/rathole
  - reverse proxy for NAT traversal ，类似于 frp and ngrok
- https://news.ycombinator.com/item?id=39928681

### [ ] reverse proxy ，NAT traversal 和 wireguard 是什么关系?

## geneve

linux/drivers/net/geneve.c 如何使用?

## 很有名的一个 blog

- https://pboyd.io/posts/securing-a-linux-vm/
- https://news.ycombinator.com/item?id=36934052
- https://pboyd.io/posts/follow-up-to-the-reluctant-sysadmin/
- https://github.com/anderspitman/SirTunnel

其他的似乎勉强可以理解，但是这里的将 ssh 放到 wireguard 上如何理解?

## 用户态的实现
https://github.com/cloudflare/boringtun

https://news.ycombinator.com/item?id=41219440 : 官方的实现


## 配置
https://news.ycombinator.com/item?id=42229299


## 看看
https://github.com/EasyTier/EasyTier


## 这个队列似乎技巧性很高，但是看不懂
```c
struct sk_buff *wg_prev_queue_dequeue(struct prev_queue *queue)
{
	struct sk_buff *tail = queue->tail, *next = smp_load_acquire(&NEXT(tail));

	if (tail == STUB(queue)) {
		if (!next)
			return NULL;
		queue->tail = next;
		tail = next;
		next = smp_load_acquire(&NEXT(next));
	}
	if (next) {
		queue->tail = next;
		atomic_dec(&queue->count);
		return tail;
	}
	if (tail != READ_ONCE(queue->head))
		return NULL;
	__wg_prev_queue_enqueue(queue, STUB(queue));
	next = smp_load_acquire(&NEXT(tail));
	if (next) {
		queue->tail = next;
		atomic_dec(&queue->count);
		return tail;
	}
	return NULL;
}
```

## 实在是有趣的工作
https://news.ycombinator.com/item?id=45559857

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
