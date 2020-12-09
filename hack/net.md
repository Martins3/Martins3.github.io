# network

1. audit 从内核中间传输数据到用户层使用的网络的什么机制
2. 首先完成 linux kernel lab 然后再说吧

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
- https://tools.ietf.org/html/rfc1180 : rfc 提供的TCP/IP 教程
- https://robertovitillo.com/what-every-developer-should-know-about-tcp/ : 这就是 TCP 的全部内容，结束了。
- https://news.ycombinator.com/item?id=23241934 : ssh-agent 的工作原理是什么 ?
- https://www.jianshu.com/p/4ba0d706ee7c : TCP/IP 快速复习一遍

交换机(switch) 集线器(hub) 网桥(bridge) 中继器(repeater) 的关系[^1]

[^1]: https://www.zhihu.com/topic/19717539/top-answers
[^2]: 用芯探核:基于龙芯的 Linux 内核探索解析
