# Network tools internals
使用 [Julia Evans](https://wizardzines.com/networking-tools-poster/) 作为基准

- [ ] 在我的印象中，一直都是 ip 和 ifconfig 是不是存在一些冲突啊


## TODO
- [ ] 测试延迟

## nmap
- https://security.stackexchange.com/questions/36198/how-to-find-live-hosts-on-my-network

检查局域网中所有的 host:
nmap -sP 192.168.11.3/8


## iperf
- 测试带宽

## hping3
https://linux.die.net/man/8/hping3

## wrk
测试 http 的性能
https://github.com/wg/wrk

## tcpdump
- [tcpdump](https://jvns.ca/tcpdump-zine.pdf)
- [ ] https://blog.cloudflare.com/bpf-the-forgotten-bytecode/ : bpf, the kernel counterpart of tcpdump

- loopback interface
  - `sudo tcpdump -i lo` : print out many message
- [ ] tcpdump 如何工作的 ?

## nc

https://www.kawabangga.com/posts/4515 中间提到 nc -l  9999 的操作可以了解一下 nc 的含义

## traceroute
- [ ] traceroute

## mtr

主要参考 [使用 mtr 检查网络问题，以及注意事项](https://www.kawabangga.com/posts/4275)

## nslookup

## wireshark
主要参考[这里](https://gaia.cs.umass.edu/kurose_ross/wireshark.php)

- [ ] https://www.kawabangga.com/posts/4794 : 写的真好啊  [ddf]
  - 之前是没有想到 wireshark 还可以宏观的处理网络问题

## ip

1. 创建 bridge
```sh
ip link add name virbr0 type bridge
ip link set dev virbr0 up
```

https://wiki.archlinux.org/title/network_bridge

## ifconfig

## arp

- 为什么感觉 arp 和 dhcp 存在一些冲突啊?
  - 如果一个网络中，加入一个新的机器的流程
    - 为了获取一个 ip addr, 使用 udp broadcast，此时 destination mac address 是 ff.ff.ff.ff.ff.ff
    - 和 dhcp server 可以获取 ip addr，但是无法知道局域网中每一台机器的 mac addr

## iproute2
https://github.com/shemminger/iproute2

## 网络测试工具
- https://github.com/google/packetdrill : 甚至还有相关的资源

## smap
https://github.com/s0md3v/Smap

## netstat
> 使用 netstat -lntp 或 ss -plat 检查哪些进程在监听端口（默认是检查 TCP 端口; 添加参数 -u 则检查 UDP 端口）或者 lsof -iTCP -sTCP:LISTEN -P -n (这也可以在 OS X 上运行)。

> This program is obsolete. Replacement for netstat is ss. Replacement for netstat -r is ip route. Replacement for netstat -i is ip -s link. Replacement for netstat -g is ip maddr.

## [ ] dropwatch

## [ ] ip

## [ ] ethtool

## netdata
- https://github.com/netdata/netdata

## tuned-adm
- https://linux.die.net/man/1/tuned-adm

## ICMP IP 和 Traceroute
https://news.ycombinator.com/item?id=32257852

## packetdrill

## 检查 gateway
route -n
ip r

setup gateway:
```txt
🧀  netstat -rn
Kernel IP routing table
Destination     Gateway         Genmask         Flags   MSS Window  irtt Iface
0.0.0.0         192.168.8.1     0.0.0.0         UG        0 0          0 wlan0
10.0.0.0        0.0.0.0         255.255.255.240 U         0 0          0 eth0
172.17.0.0      0.0.0.0         255.255.0.0     U         0 0          0 docker0
192.168.8.0     0.0.0.0         255.255.252.0   U         0 0          0 wlan0
```

## iptable
算是讲解的很好的啦!

- https://www.zsythink.net/archives/category/%e8%bf%90%e7%bb%b4%e7%9b%b8%e5%85%b3/iptables

## iptables详解（1）：iptables概念
- https://www.zsythink.net/archives/1199

iptables 操作 netfilter 来实现

完成封包过滤、封包重定向和网络地址转换（NAT）等功能。

Netfilter是Linux操作系统核心层内部的一个数据包处理模块，它具有如下功能：
- 网络地址转换(Network Address Translate)
- 数据包内容修改
- 以及数据包过滤的防火墙功能


iptables为我们提供了如下规则的分类，或者说，iptables为我们提供了如下”表”
- filter表：负责过滤功能，防火墙；内核模块：iptables_filter
- nat表：network address translation，网络地址转换功能；内核模块：iptable_nat
- mangle表：拆解报文，做出修改，并重新封装 的功能；iptable_mangle
- raw表：关闭nat表上启用的连接追踪机制；iptable_raw
也就是说，我们自定义的所有规则，都是这四种分类中的规则，或者说，所有规则都存在于这4张”表”中。


## nslookup
> nslookup www.baidu.com 114.114.114.114

- 顺便可以看看:
  - https://www.zhihu.com/question/20100901

## [ ] dig

## [ ] mtr

## tshark

## ngrep
