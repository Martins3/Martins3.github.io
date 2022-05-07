# kernel network stack
1. audit 从内核中间传输数据到用户层使用的网络的什么机制
2. 首先完成 linux kernel lab 然后再说吧

## Unix domain socket
[Introduction](https://stackoverflow.com/questions/21032562/example-to-explain-unix-domain-socket-af-inet-vs-af-unix)

## dpdk
- [ ] https://github.com/F-Stack/f-stack : 似乎是对于 dpdk 的封装
- [ ] https://blog.selectel.com/introduction-dpdk-architecture-principles/

## [ ] openvswitch
- [openvswitch](https://www.zhihu.com/column/software-defined-network)

## todo
- [ ] openwrt 到底是什么?
  - 教别人编译的 : https://github.com/coolsnowwolf/lede
- Red Book
- [ ] 好吧，并不能找到 routing table 相关的代码 ! (netfilter ?)
- [ ] bpf / bpfilter
- [ ] ceph
- [ ] packet socket
- 无论如何，将 Linux kernel Labs 中的实验做一下
- [ ] Where is ebpf hooks for packet filter ?
- 为什么 QEMU 可以让 Guest 可以有某一个 ip 到 host 的网络中
- [ ] 测试一下 bpf filter 的功能

## IBM Read Book

## network flow[^2]
- [ ] watch dog

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

- 物理设备接受一个 package [^10]
  - core/dev.c:`netif_rx`: 将 skb 放到 CPU 的队列中
  - core/dev.c:`net_rx_action`: 将 skb 从 CPU 队列中移除
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


## 网络基础知识
- 现在的想法 : 整体框架其实清楚的，利用 level-ip 来分析，其次就是解决 TCP 中间具体的细节问题, tc, retransmission,

- http://www.mattkeeter.com/projects/pont/ : 一个小游戏，涉及到前端后端，websoctet
- https://www.learncloudnative.com/blog/2020-04-25-beginners-guide-to-gateways-proxies/ : 讲解网关
- https://news.ycombinator.com/item?id=23241934 : ssh-agent 的工作原理是什么 ?
- https://www2.tkn.tu-berlin.de/teaching/rn/animations/gbn_sr/ : 拥塞网络图形化演示

- 仅仅是看一看
```txt
       -nic [tap|bridge|user|l2tpv3|vde|netmap|vhost-user|socket][,...][,mac=macaddr][,model=mn]
           This option is a shortcut for configuring both the on-board (default) guest NIC hardware and the host network backend in one go. The host backend options are the same as with the corresponding -netdev options below.  The guest NIC model can be set with
           model=modelname.  Use model=help to list the available device types.  The hardware MAC address can be set with mac=macaddr.

           The following two example do exactly the same, to show how -nic can be used to shorten the command line length (note that the e1000 is the default on i386, so the model=e1000 parameter could even be omitted here, too):

                   qemu-system-i386 -netdev user,id=n1,ipv6=off -device e1000,netdev=n1,mac=52:54:98:76:54:32
                   qemu-system-i386 -nic user,ipv6=off,model=e1000,mac=52:54:98:76:54:32
```
- 所以 -nic 和 -netdev 都是做啥的?
- -nic 后面跟着的这么多的设备是做啥的


### cloudflare
- [bgp](https://www.cloudflare.com/learning/security/glossary/what-is-bgp/) : 这是当时 facebook 不可用的时候出现的
  - 更多的教程参考这里: https://www.cloudflare.com/learning/

## 教材
- [Computer Networks: A Systems Approach](https://book.systemsapproach.org/index.html)
  - 有 hackernews 用户推荐过

## 工具
- https://rfc.fyi/ : rfc 搜索

## 有趣
- [ping localhost 不会和网卡打交道，那是 loopback devices](https://superuser.com/questions/565742/localhost-pinging-when-there-is-no-network-card)

使用这个代码可以用于测试网卡的 ip
https://stackoverflow.com/questions/2283494/get-ip-address-of-an-interface-on-linux
> 注意: eth0 -> enxd43a650739d8

用这个代码可以来测试获取所有的网卡:
https://www.cyberithub.com/list-network-interfaces/

- [网卡的工作模式](https://zdyxry.github.io/2020/03/18/%E7%90%86%E8%A7%A3%E7%BD%91%E5%8D%A1%E6%B7%B7%E6%9D%82%E6%A8%A1%E5%BC%8F/)
- [什么是 NAPI](https://stackoverflow.com/questions/28090086/what-are-the-advantages-napi-before-the-irq-coalesce)
- [什么是 GRO](https://stackoverflow.com/questions/47332232/why-is-gro-more-efficient)
  - [ ] [gro 详解](https://abcdxyzk.github.io/blog/2015/04/18/kernel-net-gro/)
  - `napi_gro_receive` 在 NAPI 层次做 GRO
- [当 read/write 的 flags 为 0 的时候，其等价于 read / write](https://stackoverflow.com/questions/19971858/c-socket-send-recv-vs-write-read)
- [sendmsg vs send 和 writev vs write 有点类似](https://stackoverflow.com/questions/4258834/how-sendmsg-works)
- [socketpair](https://stackoverflow.com/questions/64214231/why-do-i-need-socketpair-when-i-have-socket-with-af-unix)
  - 相比于 pipe 可以是双向的
  - 相比于 unix domain 可以用于暴露出来路径
- getsockname / getpeername: 详细内容参考 tlpi 61.5 Retrieving Socket Addresses
  - 存在好几种情况，可以让内核分配 port ，例如在 bind 之前 connect 或者 listen，可以通过 getsockname 来获取
- [Why does DHCP use UDP and not TCP?](https://networkengineering.stackexchange.com/questions/64401/why-does-dhcp-use-udp-and-not-tcp)
  - 因为 TCP 是 connection-oriented 的，负责两个 Host 之间的联系，无法进行 broadcast 的操作

- [ICMP vs IGMP](https://www.jianshu.com/p/4bd8758f9fbd)
- [Wireshark 的工作原理](https://stackoverflow.com/questions/29620590/where-does-the-wireshark-capture-the-packets)
  - [How does the `AF_PACKET` socket work in Linux?](https://stackoverflow.com/questions/62866943/how-does-the-af-packet-socket-work-in-linux)

## [ ] `AF_PACKET`
- [ ] 如果 wireshark 使用 `AF_PACKET` ，那么 bpf filter 发生在什么位置啊?

## [ ] linux network interfaces
https://developers.redhat.com/blog/2018/10/22/introduction-to-linux-interfaces-for-virtual-networking

## [ ] linux network virtual interfaces
https://developers.redhat.com/blog/2019/05/17/an-introduction-to-linux-virtual-interfaces-tunnels#

## dhcp
```sh
sudo ip addr flush en0
sudo dhclient -r 
sudo dhclient en0
```

## [ ] tls

## LVS
- liexusong 和 sdn book 都分析过

## sdn
- https://github.com/sdnds-tw/awesome-sdn

## 关键参考
- [使用 wireshark 分析网络](https://gaia.cs.umass.edu/kurose_ross/wireshark.php)
  - [ ] tls 和 wireless 没有深入分析
- [liexusong](https://github.com/liexusong/linux-source-code-analyze)
- [sdn book](https://tonydeng.github.io/sdn-handbook/)
  - [ ] dpdk
  - [ ] vpn
  - [ ] ovs
  - [ ] 以及后面的 sdn 的所有内容

## Linux Virtual Network Device
- drivers/net/veth.c
- drivers/net/tun.c
- drivers/net/tap.c
- net/bridge

## filter
- [ ] https://devarea.com/introduction-to-network-filters-linux/
  - 似乎还行，但是给出来的用户态和内核态中的代码都不可以运行

## 802
[wiki](https://en.wikipedia.org/wiki/IEEE_802)
主要是处理这两个层次的规划:
- Data link layer
  - LLC sublayer
  - MAC sublayer
- Physical layer

## [ ] vsock
- [ ] https://man7.org/linux/man-pages/man7/vsock.7.html

## [ ] vlan
- [ ] vlan/[802.1q](https://en.wikipedia.org/wiki/IEEE_802.1Q)

## ipoe
- [ ] [IPOE到底是什么？](https://www.zhihu.com/question/35749997/answer/83459499)

## [ ] tun

## [ ] tap

## [ ] bridge
https://github.com/liexusong/linux-source-code-analyze/blob/master/net_bridge.md

最终想要挑战的内容:
- https://www.ibm.com/docs/en/linux-on-systems?topic=choices-using-open-vswitch

- [ ] 将 bridge 使用 docker 和 qemu 测试一下

## [ ] veth
- [ ]  https://tonydeng.github.io/sdn-handbook/linux/virtual-device.html

## vhost-net / virtio-net / vhost-sock

## arp
https://github.com/liexusong/linux-source-code-analyze/blob/master/arp-neighbour.md

## tc

- https://tldp.org/HOWTO/Traffic-Control-HOWTO/

### 源码分布位置
- `linux/net/sched/cls_*.c`
  - `struct tcf_proto_ops`
- `linux/net/sched/sch_*.c`
  - `struct Qdisc_ops`
  - `struct Qdisc_class_ops`
- `linux/net/sched/act_*.c`
  - `struct tc_action_ops`

## QUIC

> But it's still just a layer on top of UDP, and still implemented at the application, like in the past.
> from https://news.ycombinator.com/item?id=27310349

emmm, 只是 DUP 上的封装

## 9p
- [Plan 9 的历史](https://www.zhihu.com/question/19706063/answer/763200196)


## 奇怪的项目
- [A framework that aids in creation of self-spreading software](https://github.com/redcode-labs/Neurax)

[^2]: 用芯探核:基于龙芯的 Linux 内核探索解析
[^4]: http://yuba.stanford.edu/rcp/
[^6]: [An Introduction to Computer Networks](http://intronetworks.cs.luc.edu/current2/html/)
[^7]: [The TCP/IP Guide](http://www.tcpipguide.com/index.htm)
[^8]: [TUN/TAP设备浅析(一) -- 原理浅析](https://www.jianshu.com/p/09f9375b7fa7)
[^9]: https://en.wikipedia.org/wiki/Transparent_Inter-process_Communication
[^10]: https://www.cs.dartmouth.edu/~sergey/me/netreads/path-of-packet/Lab9_modified.pdf
