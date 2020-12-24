# network
1. audit 从内核中间传输数据到用户层使用的网络的什么机制
2. 首先完成 linux kernel lab 然后再说吧

## Unix domain socket
[Introduction](https://stackoverflow.com/questions/21032562/example-to-explain-unix-domain-socket-af-inet-vs-af-unix)

## question
- [ ] Why we need the mac address in the link layer
- [ ] which layer will ethernet reside ?
  - [ ] substitute for ethernet 

- [ ] DHCP
- [ ] NAT
- [ ] ICMP

## TODO
- [ ] net/socket.c : all kinds of syscall, check it one by one with tlpi
  - `send()` and `write()`, if everything is a file, why a special syscall is needed for network

- [ ] direcory under linux/net to read
  - core
  - ceph
  - netfilter : http://www.netfilter.org/projects/
  - bridge
  - tipc : Transparent Inter-Process Communication
  - mac80211
  - sched : https://www.cnblogs.com/charlieroro/p/13993695.html

## TCP

### Congestion Control
rtt 标准算法（Jacobson / Karels 算法）
（Linux 的源代码在：tcp_rtt_estimator）。[^5]

tcp_ack_update_rtt
  - tcp_rtt_estimator

## e1000e[^2]
- [ ] watch dog

```
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
- file : ip_output.c tcp_output.c tcp.c route.c

## A TCP/IP Tutorial[^3]
The generic term "TCP/IP" usually means anything and everything
related to the specific protocols of TCP and IP. It can include
other protocols, applications, and even the network medium. A sample
of these protocols are: UDP, ARP, and ICMP. A sample of these
applications are: TELNET, FTP, and rcp. 

- [ ] What's rcp[^4]?

An Ethernet frame contains the destination address, source address,
type field, and data.

An Ethernet address is 6 bytes. Every device has its own Ethernet
address and listens for Ethernet frames with that destination
address. All devices also listen for Ethernet frames with a wild-
card destination address of "FF-FF-FF-FF-FF-FF" (in hexadecimal),
called a "broadcast" address.

Ethernet uses CSMA/CD (Carrier Sense and Multiple Access with
Collision Detection). CSMA/CD means that all devices communicate on
a single medium, that only one can transmit at a time, and that they
can all receive simultaneously. If 2 devices try to transmit at the
same instant, the transmit collision is detected, and both devices
wait a random (but short) period before trying to transmit again.

- [x] ARP

When sending out an IP packet, how is the destination Ethernet
address determined?

ARP (Address Resolution Protocol) is used to translate IP addresses
to Ethernet addresses. The translation is done only for outgoing IP
packets, because this is when the IP header and the Ethernet header
are created.

The translation is performed with a table look-up. The table, called
the ARP table, is stored in memory and contains a row for each
computer. 

Two things happen when the ARP table can not be used to translate an
address:
1. An ARP request packet with a broadcast Ethernet address is sent
out on the network to every computer.
2. The outgoing IP packet is queued.

          A      B      C      ----D----      E      F      G
          |      |      |      |   |   |      |      |      |
        --o------o------o------o-  |  -o------o------o------o--
        Ethernet 1                 |  Ethernet 2
        IP network "development"   |  IP network "accounting"
                                   |
                                   |
                                   |     H      I      J
                                   |     |      |      |
                                 --o-----o------o------o--
                                  Ethernet 3
                                  IP network "factory"

               Figure 7.  Three IP Networks; One internet


                ----------------------------
                |    network applications  |
                |                          |
                |...  \ | /  ..  \ | /  ...|
                |     -----      -----     |
                |     |TCP|      |UDP|     |
                |     -----      -----     |
                |         \      /         |
                |         --------         |
                |         |  IP  |         |
                |  -----  -*----*-  -----  |
                |  |ARP|   |    |   |ARP|  |
                |  -----   |    |   -----  |
                |      \   |    |   /      |
                |      ------  ------      |
                |      |ENET|  |ENET|      |
                |      ---@--  ---@--      |
                ----------|-------|---------
                          |       |
                          |    ---o---------------------------
                          |             Ethernet Cable 2
           ---------------o----------
             Ethernet Cable 1

             Figure 3.  TCP/IP Network Node on 2 Ethernets

Computer D is the IP-router; it is connected to
all 3 networks and therefore has 3 IP addresses and 3 Ethernet
addresses. Computer D has a TCP/IP protocol stack similar to that in
Figure 3, except that it has 3 ARP modules and 3 Ethernet drivers
instead of 2. Please note that computer D has only one IP module.

If A sends an IP packet to E, the source IP address and the source
Ethernet address are A’s. The destination IP address is E’s, but
because A’s IP module sends the IP packet to D for forwarding, the
destination Ethernet address is D’s.

One part of a 4-byte IP address is the IP network number, the other part is the IP
computer number (or host number). For the computer in table 1, with
an IP address of 223.1.2.1, the network number is 223.1.2 and the
host number is number 1.

The portion of the address that is used for network number and for
host number is defined by the upper bits in the 4-byte address. All
example IP addresses in this tutorial are of type class C, meaning
that the upper 3 bits indicate that 21 bits are the network number
and 8 bits are the host number. This allows 2,097,152 class C
networks up to 254 hosts on each network.

People refer to computers by names, not numbers. A computer called
alpha might have the IP address of 223.1.2.1. For small networks,
this name-to-address translation data is often kept on each computer
in the "hosts" file. For larger networks, this translation data file
is stored on a server and accessed across the network when needed.

The route table contains one row for each route. The primary columns
in the route table are: IP network number, direct/indirect flag,
router IP address, and interface number. This table is referred to
by IP for each outgoing IP packet.

On most computers the route table can be modified with the "route"
command. The content of the route table is defined by the network
manager, because the network manager assigns the IP addresses to the
computers.

Help is also available from certain protocols and network
applications. ICMP (Internet Control Message Protocol) can report
some routing problems. 

TCP is a sliding window protocol with time-out and retransmits.
Outgoing data must be acknowledged by the far-end TCP.
Acknowledgements can be piggybacked on data.

- [ ] I don't know I have finished it or not.

## TAP/TUN


## 可以参考
https://github.com/saminiir/level-ip : 学习一下
dpdk : 了解一下工作原理
  - https://blog.selectel.com/introduction-dpdk-architecture-principles/

http://beej.us/guide/bgnet/html/#socket : 主要是使用 posix 接口吧!

https://www.cs.dartmouth.edu/~sergey/me/netreads/path-of-packet/Lab9_modified.pdf

https://blog.packagecloud.io/eng/2016/06/22/monitoring-tuning-linux-netw  orking-stack-receiving-data/ : 

https://github.com/sdnds-tw/awesome-sdn

## Project
https://github.com/coolsnowwolf/lede
https://github.com/redcode-labs/Neurax

## 可以参考的资源
https://github.com/google/packetdrill : 甚至还有相关的资源
https://github.com/mtcp-stack/mtcp

## 网络基础知识
- https://github.com/chenshuo/muduo
- http://www.mattkeeter.com/projects/pont/ : 了解一下前端后端的整个流程是什么
- https://zhuanlan.zhihu.com/p/38548737　: http 图解, 应该是没有什么用的
- https://www.learncloudnative.com/blog/2020-04-25-beginners-guide-to-gateways-proxies/ : 讲解网关
- https://news.ycombinator.com/item?id=23241934 : ssh-agent 的工作原理是什么 ?
- https://www.jianshu.com/p/4ba0d706ee7c : TCP/IP 快速复习一遍

- [ ] https://github.com/CyC2018/CS-Notes : network and distributed system

交换机(switch) 集线器(hub) 网桥(bridge) 中继器(repeater) 的关系[^1]

[^1]: https://www.zhihu.com/topic/19717539/top-answers
[^2]: 用芯探核:基于龙芯的 Linux 内核探索解析
[^3]: https://tools.ietf.org/html/rfc1180
[^4]: http://yuba.stanford.edu/rcp/
[^5]: [万字详文：TCP 拥塞控制详解](https://zhuanlan.zhihu.com/p/144273871)
[^6]: [An Introduction to Computer Networks](http://intronetworks.cs.luc.edu/current2/html/)
[^7]: [The TCP/IP Guide](http://www.tcpipguide.com/index.htm)
[^8]: [TUN/TAP设备浅析(一) -- 原理浅析](https://www.jianshu.com/p/09f9375b7fa7)
