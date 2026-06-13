# A TCP/IP Tutorial 阅读笔记
[A TCP/IP Tutorial](https://tools.ietf.org/html/rfc1180)

TCP/IP 一般不仅仅指的是 TCP IP 两个协议，还包括 UDP, ARP 和 ICMP

Ethernet frame 的地址为 MAC 地址

Ethernet uses CSMA/CD (Carrier Sense and Multiple Access with
Collision Detection).

ARP (Address Resolution Protocol) : 根据 IP 查询 MAC 地址

TCP is a sliding window protocol with time-out and retransmits.


## 基本路径

```txt
- send
  - `__sys_sendto`
    - sock_sendmsg
      - sock_sendmsg_nosec
        - `INDIRECT_CALL_INET(sock->ops->sendmsg, inet6_sendmsg, inet_sendmsg, sock, msg, msg_data_left(msg));`
          - inet_sendmsg
            - `INDIRECT_CALL_2(sk->sk_prot->sendmsg, tcp_sendmsg, udp_sendmsg, sk, msg, size);`
              - tcp_sendmsg
                - tcp_sendmsg_locked
                  - `__tcp_push_pending_frames`
                  - tcp_push_one
                    - tcp_write_xmit
                      - tcp_transmit_skb
                        - `__tcp_transmit_skb`
                          - `INDIRECT_CALL_INET(icsk->icsk_af_ops->queue_xmit, inet6_csk_xmit, ip_queue_xmit, sk, skb, &inet->cork.fl);`=
                            - inet_connection_sock::inet_connection_sock_af_ops::queue_xmit
                              - ip_queue_xmit
                                - `__ip_queue_xmit`
                                  - ip_local_out
                                    - `__ip_local_out`
                                      - dst_output
                                        - `skb_dst(skb)->output(net, sk, skb);`
                                          - ip_output
                                            - ip_finish_output
                                              - `__ip_finish_output`
                                                - ip_finish_output2
                                                  - neigh_output
                                                    - neigh_hh_output
                                                    - neighbour::output
                                                      - dev_queue_xmit
                                                        - `__dev_queue_xmit`
                                                          - `__dev_xmit_skb`
                                                            - `__qdisc_run`
                                                              - qdisc_restart
                                                                - sch_direct_xmit
                                                                  - dev_hard_start_xmit
                                                                    - xmit_one
                                                                      - netdev_start_xmit
                                                                        - `__netdev_start_xmit`
                                                                          - net_device_ops::ndo_start_xmit
                                                                            - e1000_xmit_frame
```
- dir : net/tcp4
- file : `ip_output.c` `tcp_output.c` tcp.c route.c

- [ ] 应该将 recv 的路径也跟着看一遍的

- 物理设备接受一个 package 的过程，参考  https://www.cs.dartmouth.edu/~sergey/me/netreads/path-of-packet/Lab9_modified.pdf
  - core/dev.c:`netif_rx`: 将 skb 放到 CPU 的队列中 : receives a packet from a device driver and queues it for the upper (protocol)
  - core/dev.c:`net_rx_action`: 将 skb 从 CPU 队列中移除: 数据包申请sk_buff缓冲区对象，同时将数据从接收队列拷贝至sk_buff对象
- ip 层接受
  - `ip_input.c:ip_rcv`
    - `ip_rcv_finish`
      - `route.c:ip_route_input`
        - `ip_input.c:ip_local_deliver()` : 如果 ip 等于就是自己，将 packet 向上提交
        - `ipv4/route.c:ip_route_input_slow()`
          - 如果不 forward，或者不知道如何 forward ，那么 send ICMP
          - `ipv4/ip_forward.c:ip_forward()`
            - `core/dev.c:dev_queue_xmit()`
              - `sched/sch_generic.c:pfifo_fast_enqueue()`

<p align="center">
  <img src="./img/kernel-receive.png" alt="drawing" height="800" align="center"/>
</p>
<p align="center">
from https://ipads.se.sjtu.edu.cn/mospi/
</p>

## 9293
[最官方的文档](https://www.rfc-editor.org/rfc/rfc9293#name-introduction)

## 三次握手
- client 端的过程 : https://github.com/liexusong/linux-source-code-analyze/blob/master/tcp-three-way-handshake-connect.md

## Moving past TCP in the data center, part 1
- https://lwn.net/Articles/913260/

## congestion control

似乎默认是 cubic
```txt
cat /proc/sys/net/ipv4/tcp_congestion_control
cubic
```

```c
static struct tcp_congestion_ops cubictcp __read_mostly = {
	.init		= cubictcp_init,
	.ssthresh	= cubictcp_recalc_ssthresh,
	.cong_avoid	= cubictcp_cong_avoid,
	.set_state	= cubictcp_state,
	.undo_cwnd	= tcp_reno_undo_cwnd,
	.cwnd_event	= cubictcp_cwnd_event,
	.pkts_acked     = cubictcp_acked,
	.owner		= THIS_MODULE,
	.name		= "cubic",
};
```

搜索了下，感觉 tcp congestion 的参数真不少啊:
```txt
tools/testing/selftests/bpf/progs/tcp_ca_incompl_cong_ops.c:	.ssthresh = (void *)incompl_cong_ops_ssthresh,
tools/testing/selftests/bpf/progs/bpf_cubic.c:	.ssthresh	= (void *)bpf_cubic_recalc_ssthresh,
tools/testing/selftests/bpf/progs/bpf_dctcp.c:	.ssthresh	= (void *)dctcp_ssthresh,
tools/testing/selftests/bpf/progs/tcp_ca_update.c:	.ssthresh = (void *)ca_update_ssthresh,
tools/testing/selftests/bpf/progs/tcp_ca_update.c:	.ssthresh = (void *)ca_update_ssthresh,
tools/testing/selftests/bpf/progs/tcp_ca_update.c:	.ssthresh = (void *)ca_update_ssthresh,
tools/testing/selftests/bpf/progs/tcp_ca_update.c:	.ssthresh = (void *)ca_update_ssthresh,
tools/testing/selftests/bpf/progs/tcp_ca_write_sk_pacing.c:	.ssthresh = (void *)write_sk_pacing_ssthresh,
include/trace/events/rxrpc.h:		      __entry->sum.ssthresh,
net/ipv4/tcp_cubic.c:	.ssthresh	= cubictcp_recalc_ssthresh,
net/ipv4/tcp_cdg.c:	.ssthresh = tcp_cdg_ssthresh,
net/ipv4/tcp_cong.c:	.ssthresh	= tcp_reno_ssthresh,
net/ipv4/tcp_htcp.c:	.ssthresh	= htcp_recalc_ssthresh,
net/ipv4/tcp_hybla.c:	.ssthresh	= tcp_reno_ssthresh,
net/ipv4/tcp_lp.c:	.ssthresh = tcp_reno_ssthresh,
net/ipv4/tcp_vegas.c:	.ssthresh	= tcp_reno_ssthresh,
net/ipv4/tcp_yeah.c:	.ssthresh	= tcp_yeah_ssthresh,
net/ipv4/tcp_highspeed.c:	.ssthresh	= hstcp_ssthresh,
net/ipv4/tcp_illinois.c:	.ssthresh	= tcp_illinois_ssthresh,
net/ipv4/tcp_veno.c:	.ssthresh	= tcp_veno_ssthresh,
net/ipv4/tcp_westwood.c:	.ssthresh	= tcp_reno_ssthresh,
net/ipv4/tcp_scalable.c:	.ssthresh	= tcp_scalable_ssthresh,
net/ipv4/tcp_bbr.c:	.ssthresh	= bbr_ssthresh,
net/ipv4/tcp_bic.c:	.ssthresh	= bictcp_recalc_ssthresh,
net/ipv4/tcp_nv.c:	.ssthresh	= tcpnv_recalc_ssthresh,
net/ipv4/tcp_dctcp.c:	.ssthresh	= dctcp_ssthresh,
net/ipv4/tcp_dctcp.c:	.ssthresh	= tcp_reno_ssthresh,
```

## 资源
### cs 144
- https://cs144.github.io/ : tcp 实验
  - https://csdiy.wiki/en/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%BD%91%E7%BB%9C/CS144/
  - https://gwzlchn.github.io/202205/cs144-lab4/


## 数据中心的网络移除掉 TCP
- [Moving past TCP in the data center, part 1](https://mp.weixin.qq.com/s/yz47AT-ySvzZYCgd-AsQww)
- [Moving past TCP in the data center, part 2](https://lwn.net/Articles/914030/)

## [ ] big tcp
- [Going big with TCP packets](https://lwn.net/Articles/884104/)
  - https://netdevconf.info/0x15/session.html?BIG-TCP

##  重传
通过 /proc/sys/net/ipv4/tcp_retries2 可以实现
- https://stackoverflow.com/questions/5907527/application-control-of-tcp-retransmission-on-linux

## TCP 拥塞控制对数据延迟的影响
https://www.kawabangga.com/posts/5181

## 重传实验
- https://www2.tkn.tu-berlin.de/teaching/rn/animations/gbn_sr/ : 拥塞网络图形化演示


## possible tcp sync

https://www.cloudflare.com/zh-cn/learning/ddos/syn-flood-ddos-attack/

[^1]: [TCP : rfc793](https://tools.ietf.org/html/rfc793)
[^3]:
[^4]: http://yuba.stanford.edu/rcp/

## TODO
- tcp keep alive
  - https://tldp.org/HOWTO/TCP-Keepalive-HOWTO/overview.html
  - https://webhostinggeeks.com/howto/tcp-keepalive-recommended-settings-and-best-practices/

## It’s always TCP_NODELAY. Every damn time.
- https://news.ycombinator.com/item?id=40310896

## 看看，不过可能意义不大
https://jeclark.net/articles/tcp-initcwnd/?tag=performance

https://cefboud.com/posts/tcp-deep-dive-internals/


https://brooker.co.za/blog/2024/05/09/nagle.html
https://news.ycombinator.com/item?id=46359120

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
