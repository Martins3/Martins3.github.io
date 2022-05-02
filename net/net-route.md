# kernel network stack
1. audit 从内核中间传输数据到用户层使用的网络的什么机制
2. 首先完成 linux kernel lab 然后再说吧

## Unix domain socket
[Introduction](https://stackoverflow.com/questions/21032562/example-to-explain-unix-domain-socket-af-inet-vs-af-unix)

## question
- [ ] Where is ebpf hooks for packet filter ?
- loopback interface
  - `sudo tcpdump -i lo` : print out many message
- 为什么 QEMU 可以让 Guest 可以有某一个 ip 到 host 的网络中
- [ ] 将 net.diff 中的内容整理一下

## dpdk
https://github.com/F-Stack/f-stack : 似乎是对于 dpdk 的封装

## TODO
忽然间对于网络没有那么恐惧了，所以

- [ ] net/socket.c : all kinds of syscall, check it one by one with tlpi
  - `send()` and `write()`, if everything is a file, why a special syscall is needed for network


- [ ] direcory under linux/net to read
  - core : 各种 proc dev sys 之类的东西，整个网络的公共部分吧 !
  - ceph
  - netfilter
  - bridge :
  - tipc : Transparent Inter Process Communication (TIPC) is an Inter-process communication (IPC) service in Linux designed for cluster wide operation. It is sometimes presented as Cluster Domain Sockets, in contrast to the well-known Unix Domain Socket service; the latter working only on a single kernel. [^9]
  - mac80211
  - sched
  - [openvswitch](https://www.zhihu.com/column/software-defined-network)

- Red Book
- [ ] 好吧，并不能找到 routing table 相关的代码 ! (netfilter ?)
- [ ] https://github.com/CyC2018/CS-Notes : network and distributed system
- TUN/TAP 驱动分析一下吧
- [ ] epoll 机制
- [ ] https://man7.org/linux/man-pages/man7/vsock.7.html
- [ ] vsock
- [ ] vlan/[802.1q](https://en.wikipedia.org/wiki/IEEE_802.1Q)
- [ ] 9p
- [ ] bpf / bpfilter
- [ ] ceph
- [ ] /home/maritns3/core/linux/net/ethernet/eth.c
- [ ] openvswitch
- [ ] packet socket
- [ ] sctp : 流量控制协议
- [ ] /home/maritns3/core/linux/net/unix

https://www.kawabangga.com/posts/4515 中间提到 nc -l  9999 的操作可以了解一下 nc 的含义

给下面写一个一句话总结好吧
```txt
__do_sys_bind
__do_sys_send
__do_sys_recv
__do_sys_socket
__do_sys_listen
__do_sys_accept
__do_sys_sendto
__do_sys_accept4
__do_sys_connect
__do_sys_sendmsg
__do_sys_recvmsg
__do_sys_recvfrom
__do_sys_shutdown
__do_sys_sendmmsg
__do_sys_recvmmsg
__do_sys_socketpair
__do_sys_setsockopt
__do_sys_getsockopt
__do_sys_socketcall
__do_sys_getsockname
__do_sys_getpeername
__do_sys_recvmmsg_time32
```



## IBM Read Book
[TCP/IP--ICMP和IGMP](https://www.jianshu.com/p/4bd8758f9fbd)

## e1000e[^2]
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
- file : ip_output.c tcp_output.c tcp.c route.c

- [ ] 应该将 recv 的路径也跟着看一遍的

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
- 现在的想法 : 整体框架其实清楚的，利用 level-ip 来分析，其次就是解决 TCP 中间具体的细节问题, tc, retransmission,

- https://github.com/chenshuo/muduo
- http://www.mattkeeter.com/projects/pont/ : 了解一下前端后端的整个流程是什么
- https://zhuanlan.zhihu.com/p/38548737　: http 图解, 应该是没有什么用的
- https://www.learncloudnative.com/blog/2020-04-25-beginners-guide-to-gateways-proxies/ : 讲解网关
- https://news.ycombinator.com/item?id=23241934 : ssh-agent 的工作原理是什么 ?
- https://www.jianshu.com/p/4ba0d706ee7c : TCP/IP 快速复习一遍

https://www2.tkn.tu-berlin.de/teaching/rn/animations/gbn_sr/ : 拥塞网络图形化演示

- [sdn book](https://tonydeng.github.io/sdn-handbook/) : 中文的
- [网络编程懒人入门](https://zhuanlan.zhihu.com/p/335137284) : 一共十几篇，值得分析

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
- https://github.com/smoltcp-rs/smoltcp : 似乎这个用户态网络栈的文档就可以了

## 工具
- https://rfc.fyi/ : rfc 搜索

## 有趣
- [ping localhost 不会和网卡打交道，那是 loopback devices](https://superuser.com/questions/565742/localhost-pinging-when-there-is-no-network-card)

使用这个代码[^1] 可以用于测试网卡的 ip
注意: eth0 -> enxd43a650739d8

[^1]: https://stackoverflow.com/questions/2283494/get-ip-address-of-an-interface-on-linux

用这个代码可以来测试获取所有的网卡:
https://www.cyberithub.com/list-network-interfaces/

[^2]: 用芯探核:基于龙芯的 Linux 内核探索解析
[^4]: http://yuba.stanford.edu/rcp/
[^6]: [An Introduction to Computer Networks](http://intronetworks.cs.luc.edu/current2/html/)
[^7]: [The TCP/IP Guide](http://www.tcpipguide.com/index.htm)
[^8]: [TUN/TAP设备浅析(一) -- 原理浅析](https://www.jianshu.com/p/09f9375b7fa7)
[^9]: https://en.wikipedia.org/wiki/Transparent_Inter-process_Communication
