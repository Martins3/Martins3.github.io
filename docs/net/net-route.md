# Network Route

## KeyNotes

| OSI        | TCP/IP | 作用                     | 主要协议                 |
| ---------- | ------ | ------------------------ | ------------------------ |
| 应用层     | 应用层 | 文件服务、电子邮件等应用 | DNS、HTTP、FTP 等        |
| 表示层     | 应用层 | 格式、压缩、加密         | 无对应                   |
| 会话层     | 应用层 | 建立或解除通信           | 无对应                   |
| 传输层     | 传输层 | 进程到进程的通信         | TCP、UDP                 |
| 网络层     | 网络层 | 主机到主机的通信         | IP、ICMP、IGMP、BGP 等   |
| 数据链路层 | 链路层 | 构建数据帧传输的链路     | ARP、PPP、MTU、SLIP 等   |
| 物理层     | 物理层 | 提供信息传输的物理介质   | IEEE802.2、IEEE802.11 等 |

- https://switch-router.gitee.io/categories/ ：阅读其中的每一篇文章；
  - 记录 tcp 参数:
    - https://segmentfault.com/a/1190000020473127
- https://zorrozou.github.io/docs/tcp/handshake/tcp_three_way_handshake.html : 关于 TCP 的文章可以分析一下。

## 总结


- 为什么 ip 需要 netmask ，或者说，为什么 ip 需要划分网段?
  - 对于一个 ip ，例如 10.0.0.1 想和另一个 ip 沟通，例如是 10.0.1.1 ，如果 netmask 是 255.255.255.0 ，那么时候就要走网关了，如果是 255.255.0.0 ，那么就查无此人。

[IEEE_802](https://en.wikipedia.org/wiki/IEEE_802) 主要是处理这两个层次的规划:

- Data link layer
  - LLC sublayer
  - MAC sublayer
- Physical Layer
  其提供了一大堆协议，但是我们主要用的就是 802.11 (wifi) 和 802.3 (eth)


- [A framework that aids in creation of self-spreading software](https://github.com/redcode-labs/Neurax) : 有趣的项目

- 0 结尾的 ip address 是正常的
  - https://stackoverflow.com/questions/14915188/ip-address-ending-with-zero
  - 参考 https://www.iusmentis.com/technology/tcpip/ipaddress ，原来 .1 和 .255 结尾的都是存在特殊含意啊
    - As a convention, an IP address that ends in ".1" usually refers to a gateway or router on a particular network. An address that ends in ".255" is a so-called broadcast address: all devices in the same network should handle packets addressed to the broadcast address.
  - 测试一下，如果 13900k 和 9745hx 的两个机器两个 ip 都没有 .1 结尾，那么如何?
    - 问题并不是很大，两个机器照样是可以正常运行的。

- 内网中可以使用的 ip 段都是什么?
  - https://datatracker.ietf.org/doc/html/rfc1918

一共可以用的是这三个段:
```txt
     10.0.0.0        -   10.255.255.255  (10/8 prefix)
     172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
     192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
```
172.16. 一直到 172.31. 都是 private network 的，注意 netmask

- 居然出现了可以 ssh ，但是无法 ping 通的场景，可以用 tcpdump 在接受端找到 icmp 的。防火墙导致的

- 所谓 ip alias 就是一个 nic 可以有多个 ip
  - https://en.wikipedia.org/wiki/IP_aliasing
  - https://serverfault.com/questions/12285/when-ip-aliasing-how-does-the-os-determine-which-ip-address-will-be-used-as-sour

- 如果可以 ping 通网关，但是无法 wget 网络
  - sudo route add default gw 192.168.1.254 eth0

## 有趣

- [ping localhost 不会和网卡打交道，那是 loopback devices](https://superuser.com/questions/565742/localhost-pinging-when-there-is-no-network-card)

使用这个代码可以用于测试网卡的 ip
https://stackoverflow.com/questions/2283494/get-ip-address-of-an-interface-on-linux

> 注意: eth0 -> enxd43a650739d8

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
- [flow control vs congestion control](https://stackoverflow.com/questions/16473038/whats-the-difference-between-flow-control-and-congestion-control-in-tcp)
  - Congestion control is a global issue – involves every router and host within the subnet
  - Flow control, that is scoped from point to point, involves just sender and receiver.
- [What's ndisc_cache?](https://unix.stackexchange.com/questions/228469/whats-ndisc-cache)
  - arp_cache 给 ipv4 用的，ndisc_cache 给 ipv6 ，对应 ndp 协议


## ipoe

- [ ] [IPOE 到底是什么？](https://www.zhihu.com/question/35749997/answer/83459499)


## 9p

- [Plan 9 的历史](https://www.zhihu.com/question/19706063/answer/763200196)

share dir 用的是 9p 吗？

9p 是在哪一层的，如何使用的?

## 关键参考


## [ ] Understanding Host Network Stack Overheads

> 去 zotero 上看看，其引用

## netdev 的 watch dog

### 不错的 blog 合集
- [一文理解 K8s 容器网络虚拟化](https://www.0xffffff.org/2022/03/20/43-k8s/)
  - 写的太好了
  - [ ] 为什么两个机器上创建了 bridge 下的不能互相 ping 通，而是需要借助 vxlan
  - [ ] vxlan 居然是隧道协议
  - [ ] 最后关于 k8s 的部分不太了解


- [coder-kung-fu](https://github.com/yanfeizhang/coder-kung-fu)
  - [ ] tcpdump 那个可以看看
  - 网络相关内容深入浅出，很好


- [Ivan Velichko](https://iximiuz.com/en/posts/)
  - 写的很不错啊

- [TCP Retransmission May Be Misleading (2023)](https://arthurchiao.art/blog/tcp-retransmission-may-be-misleading/)

- [史上最全 SSH 暗黑技巧详解](https://plantegg.github.io/2019/06/02/%E5%8F%B2%E4%B8%8A%E6%9C%80%E5%85%A8_SSH_%E6%9A%97%E9%BB%91%E6%8A%80%E5%B7%A7%E8%AF%A6%E8%A7%A3--%E6%94%B6%E8%97%8F%E4%BF%9D%E5%B9%B3%E5%AE%89/)

- [Kernel Documentation](https://docs.kernel.org/networking/index.html)

- [liexusong](https://github.com/liexusong/linux-source-code-analyze)

- [SDN 手册](https://tonydeng.gitbooks.io/sdn/content/)
  - 最后的部分已经看不懂了

- https://blog.cloudflare.com/author/marek-majkowski
  - 这个人所有的 blog 都可以看看

- [程序员如何构建网络知识体系](https://plantegg.github.io/2020/05/24/%E7%A8%8B%E5%BA%8F%E5%91%98%E5%A6%82%E4%BD%95%E5%AD%A6%E4%B9%A0%E5%92%8C%E6%9E%84%E5%BB%BA%E7%BD%91%E7%BB%9C%E7%9F%A5%E8%AF%86%E4%BD%93%E7%B3%BB/)
  - 常看常新

- https://plantegg.github.io/

### 从业务的角度看看

- [高并发的哲学原理](https://github.com/johnlui/PPHC)
- [深入架构原理与实践](https://www.thebyte.com.cn/)
- [System Design](https://github.com/karanpratapsingh/system-design)

### 资源

- [An Introduction to Computer Networks](http://intronetworks.cs.luc.edu/current2/html/)
- [Beej's Guide to Network Concepts](https://beej.us/guide/bgnet0/html/split/) : 教科书
- [Computer Networks: A Systems Approach](https://book.systemsapproach.org/index.html) : 教科书
- [The TCP/IP Guide](http://www.tcpipguide.com/index.htm)
- [cloudflare](https://www.cloudflare.com/learning/) : cloudflare 提供的都是基础概念
- https://rfc.fyi/ : rfc 搜索
- [CyC2018 CS-Notes](https://github.com/CyC2018/CS-Notes/blob/master/notes/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%BD%91%E7%BB%9C%20-%20%E4%BC%A0%E8%BE%93%E5%B1%82.md) : TCP 总结的真的到位
- [互联网体系结构/庖丁解牛Linux网络协议栈](https://github.com/mengning/net) : 一些简单的 slides
- [High Performance Browser Networking](https://hpbn.co/)


### 网络安全
- https://github.com/krisnova/boopkit
- [Ethical Hacking Labs](https://github.com/Samsar4/Ethical-Hacking-Labs/blob/master/11-Bonus/TCPDump-Tutorial.md)
  - 部分内容值得一看

### 高级
- fwmask
  - https://utcc.utoronto.ca/~cks/space/blog/linux/LinuxIpFwmarkMasks
  - https://unix.stackexchange.com/questions/436799/how-the-fwmark-works-together-with-mask-in-ip-rule-command

- dccp
  - https://www.reddit.com/r/programming/comments/5i3anm/dccp_the_socket_type_you_probably_never_heard_of/
  - https://www.anmolsarma.in/post/dccp/
  - https://wiki.wireshark.org/DCCP
  - dccp 似乎是一个是 TCP UDP 的例子


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
