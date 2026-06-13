# Network tools internals

## Julia Evans networking tools poster
<!-- 1fcbe967-2efb-4acb-9856-b5d659abde9c -->

[Julia Evans](https://wizardzines.com/networking-tools-poster/) 作为基准

已经太老的:
- iptraf (http://iptraf.seul.org/) : 20 年前都不更新了
- nload (https://github.com/rolandriegel/nload) : 7 年不更新了
- zenmap (nmap 的 gui)
- ntop (ntopng) : 似乎是在一个商业公司的
- mitmproxy : https 的工具

还没搞懂的:
- hping3

## TODO
- [ ] 测试延迟的工具

## ntpperf
专门测试 ntp server 的延迟的
https://github.com/mlichvar/ntpperf

## nmap
Nmap (Network Mapper) 是一个强大的网络扫描和安全审计工具，常用于网络发现和安全检查

- https://security.stackexchange.com/questions/36198/how-to-find-live-hosts-on-my-network


检查局域网中所有的 host:
nmap -sP 192.168.11.3/8

## iperf3

- 测试带宽

## netperf
https://github.com/HewlettPackard/netperf

2021 将没有维护了

## hping3 : 网络安全
https://linux.die.net/man/8/hping3

## wrk

测试 http 的性能，没有维护了
https://github.com/wg/wrk

## pwru
参考 net/netfilter.md 中有介绍。

## tcpdump

- [tcpdump](https://jvns.ca/tcpdump-zine.pdf)
  - https://jvns.ca/blog/2016/03/16/tcpdump-is-amazing/
- [ ] https://blog.cloudflare.com/bpf-the-forgotten-bytecode/ : bpf, the kernel counterpart of tcpdump

- loopback interface
  - `sudo tcpdump -i lo` : print out many message
- [ ] tcpdump 如何工作的 ?


### 监控一个 port
```sh
sudo tcpdump -i wlp4s0 port 33985
```

client
```txt
nc -u 192.168.1.2 10001
```
server
```txt
nc -ul 10001
```
在 server 端检测到这个，但是结果如下，并没有什么办法连续上，无法理解
```txt
🧀  sudo tcpdump -i wlan0 port 10001 -v

[sudo] password for martins3:
tcpdump: listening on wlan0, link-type EN10MB (Ethernet), snapshot length 262144 bytes
20:20:09.389396 IP (tos 0x0, ttl 64, id 61679, offset 0, flags [DF], proto UDP (17), length 30)
    192.168.1.7.38449 > 192.168.1.2.scp-config: UDP, length 2
20:20:21.918820 IP (tos 0x0, ttl 64, id 1915, offset 0, flags [DF], proto UDP (17), length 30)
    192.168.1.7.39896 > 192.168.1.2.scp-config: UDP, length 2
```

### ngrep && tcpflow

https://my-tiny.net/L15-tcpflo.htm

> `tcpdump` allows us to watch traffic and save captured packets for future analysis
> `ngrep` allows us to specify a regular expression to match against data payloads of packets
> `tcpflow` reconstructs the data streams and shows or stores each flow separately

## sniffnet
https://github.com/GyulyVGC/sniffnet

侧重于 GUI 的 iftop 吧

## iftop
13 年前就没人维护了

```txt
sudo iftop -i  wlo1
```

## netsniff-ng
还是用会 tcpdump 吧

https://github.com/netsniff-ng/netsniff-ng
https://netsniff-ng.github.io/ : 这个仓库 6 年前就没更新了

## tc
主要用于流量管理

1. 延迟控制
```sh
tc qdisc add dev eth0 root netem delay 100ms
```
2. 丢包模拟
```sh
tc qdisc add dev eth0 root netem loss 5%
```
3. 通过 tc 对虚拟接口（如 veth）设置规则。例如，为某个容器的虚拟接口限制带宽：
```sh
tc qdisc add dev veth123 root tbf rate 2mbit burst 32kbit latency 400ms
```
4. 使用 tc 的令牌桶过滤器（TBF）或层次化令牌桶（HTB）来设置带宽上限或分配带宽。
```sh
tc qdisc add dev eth0 root tbf rate 1mbit burst 32kbit latency 400ms
```
这将限制 eth0 接口的上传带宽为 1Mbps

## nc
俗称 netcat ，和 socat 的关系是什么?

nc -l 9999 的操作可以了解一下 nc 的含义

nc 创建

### 给 udp 发送一个 byte
nc -v -u -z -w 3 127.0.0.1 8888

## traceroute

- [ ] traceroute

- ICMP IP 和 Traceroute

https://news.ycombinator.com/item?id=32257852

## nslookup

## wireshark

主要参考[这里](https://gaia.cs.umass.edu/kurose_ross/wireshark.php)

- [ ] https://www.kawabangga.com/posts/4794 : 写的真好啊 [ddf]
  - 之前是没有想到 wireshark 还可以宏观的处理网络问题

- [使用 wireshark 分析网络](https://gaia.cs.umass.edu/kurose_ross/wireshark.php)
  - [ ] tls 和 wireless 没有深入分析

wireshark tshark 和 tcpdump 的关系:
- https://networkengineering.stackexchange.com/questions/69250/what-is-the-difference-between-tshark-dumpcap-and-others-for-collecting-sniffin

- https://andreaskaris.github.io/blog/networking/bpf-and-tcpdump/

- https://www.kernel.org/doc/Documentation/networking/filter.txt

感觉 filter 工具和包过滤都是在一个体系下的。

### termshark

- https://news.ycombinator.com/item?id=38531181

## ip

ip 甚至可以创建网桥，具体使用放到 ./bridge.md 中。

### ip address
https://man7.org/linux/man-pages/man8/ip-address.8.html

>        valid_lft LFT
>               the valid lifetime of this address; see section 5.5.4 of
>               RFC 4862. When it expires, the address is removed by the
>               kernel.  Defaults to forever.
>
>        preferred_lft LFT
>               the preferred lifetime of this address; see section 5.5.4
>               of RFC 4862. When it expires, the address is no longer
>               used for new outgoing connections. Defaults to forever.
>


```txt
38: enp177s0f0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether 00:0f:53:a9:41:c0 brd ff:ff:ff:ff:ff:ff
    inet6 fe80::20f:53ff:fea9:41c0/64 scope link
       valid_lft forever preferred_lft forever
39: enp177s0f1: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc mq state DOWN group default qlen 1000
    link/ether 00:0f:53:a9:41:c1 brd ff:ff:ff:ff:ff:ff
```


- [ip Output format:](http://linux-ip.net/gl/ip-cref/ip-cref-node17.html)
- ⭐ https://unix.stackexchange.com/questions/465563/how-to-understand-the-output-of-ifconfig-or-ip-addr-show

- [Using ip, what does LOWER_UP mean?](https://stackoverflow.com/questions/36715664/using-ip-what-does-lower-up-mean)
  - 应该是这个意思
    - LOWER_UP : 网线插好了
    - IFF_UP : 这个网卡启用了

- [Interface groups](https://unix.stackexchange.com/questions/683048/interface-groups)

### route
- https://serverfault.com/questions/877880/how-can-i-add-a-default-gateway-with-the-ip-command-not-the-route-command

有时候，没有默认路由，表现为可以 ping 通网关，但是无法 ping www.baidu.com 之类的，从另外一个局域网

执行 ip route 的结果为:
```sh
192.168.16.0/20 dev enp125s0f0 proto kernel scope link src 192.168.19.60
```

```sh
ip route add default via 192.168.16.1 dev enp125s0f0
```

## ifconfig

### 为什么不用 ifconfig 了，为什么？
https://news.ycombinator.com/item?id=17151046

ip link set dev ens22f1 up 和 ifconfig ens22f1 up 区别吗?


## arp

- 为什么感觉 arp 和 dhcp 存在一些冲突啊?
  - 如果一个网络中，加入一个新的机器的流程
    - 为了获取一个 ip addr, 使用 udp broadcast，此时 destination mac address 是 ff.ff.ff.ff.ff.ff
    - 和 dhcp server 可以获取 ip addr，但是无法知道局域网中每一台机器的 mac addr

## iproute2

https://github.com/shemminger/iproute2

## route

- [How to understand the output of route](https://unix.stackexchange.com/questions/433734/how-to-understand-the-output-of-route)

```txt
🧀  route -n
Kernel IP routing table
Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
0.0.0.0         192.168.1.1     0.0.0.0         UG    600    0        0 wlp4s0
192.168.1.0     0.0.0.0         255.255.255.0   U     600    0        0 wlp4s0
```

假如发送到 192.168.1.11 ，首先和 Genmask and 一下，然后发现等于 192.168.1.0，所以就可以
直接发送到该设备，无需经过 Gateway ，如果不等于，那么发送和 0.0.0.0 and 一下，
等于 Destination 中的 0.0.0.0 ，所以发送到 Gateway ，也就是 192.168.1.1

所以，对于一个机器同时存在两个网卡，分别 ip 在两个网段，例如 192.168.1.3 和 10.0.0.3
想要连接到到另一个机器，其 ip 是 10.0.0.4 ，是可以自动选择哪一个网卡的。

```txt
🧀  route -n
Kernel IP routing table
Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
0.0.0.0         192.168.8.1     0.0.0.0         UG    600    0        0 wlo1
10.0.0.0        0.0.0.0         255.255.255.0   U     100    0        0 enp6s0
10.0.0.0        0.0.0.0         255.255.255.0   U     101    0        0 enp5s0
172.17.0.0      0.0.0.0         255.255.0.0     U     0      0        0 docker0
172.18.0.0      0.0.0.0         255.255.0.0     U     0      0        0 br-1ef4b93ecdc9
172.19.0.0      0.0.0.0         255.255.0.0     U     0      0        0 br-a7c25b8e6a73
172.20.0.0      0.0.0.0         255.255.0.0     U     0      0        0 br-6b3cdeb5b4ca
192.168.8.0     0.0.0.0         255.255.252.0   U     600    0        0 wlo1
```

- [Interpreting the 'metric' column in routing table](https://superuser.com/questions/1167244/interpreting-the-metric-column-in-routing-table)

Metric 的含义 : 到达 distance 的距离

## packetdrill
网络栈正确性测试工具

- https://github.com/google/packetdrill : 甚至还有相关的资源

## smap
https://github.com/s0md3v/Smap

## netstat (deprecated)
> 使用 netstat -lntp 或 ss -plat 检查哪些进程在监听端口（默认是检查 TCP 端口; 添加参数 -u 则检查 UDP 端口）或者 lsof -iTCP -sTCP:LISTEN -P -n (这也可以在 OS X 上运行)。

> This program is obsolete. Replacement for netstat is ss. Replacement for netstat -r is ip route. Replacement for netstat -i is ip -s link. Replacement for netstat -g is ip maddr.

netstat --inet -ap

https://www.redhat.com/sysadmin/netstat : 看上去还是相当好用的哇

## ss

ss 是 netstat 的替代，差别参考:

https://stackoverflow.com/questions/11763376/difference-between-netstat-and-ss-in-linux

### [An Introduction to the ss Command](https://www.linux.com/topic/networking/introduction-ss-command)

> The ss command-line utility can display stats for the likes of PACKET, TCP, UDP, DCCP, RAW, and Unix domain sockets.

ss -l

The above command will only output a list of current listening sockets.

ss -uap
ss -upnl

展示哪些使用的 udp 端口


## [ ] dropwatch

## [ ] ip

## ethtool

常用的几个:
```txt
       -i --driver
              Queries the specified network device for associated driver information.

       -k --show-features --show-offload
              Queries the specified network device for the state of pro‐
              tocol offload and other features.

       -S --statistics
              Queries  the  specified network device for standard (IEEE,
              IETF, etc.), or NIC- and driver-specific statistics.  NIC-
              and driver-specific statistics are requested when no group
              of statistics is specified.

              NIC-  and  driver-specific statistics and standard statis‐
              tics are independent, devices may implement  either,  both
              or  none.  There  is  little commonality between naming of
              NIC- and driver-specific statistics across vendors.
```

## netdata

- https://github.com/netdata/netdata

## packetdrill

## 检查路由

route -n
ip r

setup gateway

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

## iptables 详解（1）：iptables 概念

- https://www.zsythink.net/archives/1199

iptables 操作 netfilter 来实现

完成封包过滤、封包重定向和网络地址转换（NAT）等功能。

Netfilter 是 Linux 操作系统核心层内部的一个数据包处理模块，它具有如下功能：

- 网络地址转换(Network Address Translate)
- 数据包内容修改
- 以及数据包过滤的防火墙功能

iptables 为我们提供了如下规则的分类，或者说，iptables 为我们提供了如下”表”

- filter 表：负责过滤功能，防火墙；内核模块：iptables_filter
- nat 表：network address translation，网络地址转换功能；内核模块：iptable_nat
- mangle 表：拆解报文，做出修改，并重新封装 的功能；iptable_mangle
- raw 表：关闭 nat 表上启用的连接追踪机制；iptable_raw
  也就是说，我们自定义的所有规则，都是这四种分类中的规则，或者说，所有规则都存在于这 4 张”表”中。

## nslookup

> nslookup www.baidu.com 114.114.114.114

nslookup www.baidu.com

- 顺便可以看看:
  - https://www.zhihu.com/question/20100901

## dig
dns 工具

## mtr

主要参考 [使用 mtr 检查网络问题，以及注意事项](https://www.kawabangga.com/posts/4275)

> mtr：一个网络诊断工具，可执行 traceroute 和 ping 功能。它通过限制单个数据包可能经过的跃点数来探测路由路径上的路由器，并监听数据包过期的响应。

这不必 traceroute 好用多了

## arp-scan

https://www.kali.org/tools/arp-scan/

## 查看公有云的延迟

- https://www.cloudping.info/

## gping


## iw

## trippy
https://trippy.cli.rs/

## ethtool
https://man7.org/linux/man-pages/man8/ethtool.8.html


## 25 个网络模拟工具
- https://news.ycombinator.com/item?id=37842161
  - https://docs.gns3.com/docs/ : 这个似乎是不错的，多次看到

## ssh

## 保持当前进程运行退出
https://stackoverflow.com/questions/954302/how-to-make-a-programme-continue-to-run-after-log-out-from-ssh
ctrl+z
disown -h %1
bg 1
logout

### 密码
➜  vn git:(master) ✗ ssh-copy-id maritns3@192.168.12.34

- [ ] https://news.ycombinator.com/item?id=32486031 : ssh 技巧

- [ ] https://console.dev/articles/ssh-alternatives-for-mobile-high-latency-unreliable-connections/
- [ ] https://project-awesome.org/moul/awesome-ssh : 没有想到，原来 ssh 也是存在 awesome 项目的

> 对 ssh 设置做一些小优化可能是很有用的，例如这个 ~/.ssh/config 文件包含了防止特定网络环境下连接断开、压缩数据、多通道等选项：
>
>       TCPKeepAlive=yes
>       ServerAliveInterval=15
>       ServerAliveCountMax=6
>       Compression=yes
>       ControlMaster auto
>       ControlPath /tmp/%r@%h:%p
>       ControlPersist yes
> 一些其他的关于 ssh 的选项是与安全相关的，应当小心翼翼的使用。例如你应当只能在可信任的网络中启用 StrictHostKeyChecking=no，ForwardAgent=yes。

这个工具看看:
https://github.com/francoismichel/ssh3

## nmtui
之外的替代品: gnome 在界面上可以设置，实际上还可以 nm-connection-editor
https://askubuntu.com/questions/174381/openning-networkmanagers-edit-connections-window-from-terminal

实际上都是 NetworkManager 的工具而已
- https://github.com/NetworkManager/NetworkManager

https://wiki.archlinux.org/title/NetworkManager
> NetworkManager is a program for providing detection and configuration for systems to automatically connect to networks.


```txt
🧀  systemctl status NetworkManager
● NetworkManager.service - Network Manager
     Loaded: loaded (/etc/systemd/system/NetworkManager.service; enabled; preset: enabled)
    Drop-In: /nix/store/vhm653lgsndq8k6fgcp7q4y7jvirxy8h-system-units/NetworkManager.service.d
             └─NetworkManager-ovs.conf, overrides.conf
     Active: active (running) since Sun 2024-01-21 09:26:00 CST; 2h 41min ago
       Docs: man:NetworkManager(8)
   Main PID: 1509 (NetworkManager)
         IP: 68.0K in, 240B out
         IO: 7.2M read, 32.0K written
      Tasks: 4 (limit: 38117)
     Memory: 12.5M
        CPU: 754ms
     CGroup: /system.slice/NetworkManager.service
             └─1509 /nix/store/q5b4iwjcyvcvw1a3w8d20rbxwn7lcw1n-networkmanager-1.44.2/sbin/NetworkManager --no-daemon

1月 21 09:25:59 nixos systemd[1]: Starting Network Manager...
1月 21 09:26:00 nixos systemd[1]: Started Network Manager.
```

## speedbump
模拟 tcp 延迟的，很小的工具
- https://news.ycombinator.com/item?id=39012697
  - https://github.com/kffl/speedbump

## nethogs

## 看看这个
https://stackoverflow.com/questions/368002/network-usage-top-htop-on-linux
https://unix.stackexchange.com/questions/67807/is-there-a-top-like-command-that-shows-the-network-bandwidths-and-file-accesses


## fping

fping 127.0.0.1

## trippy
https://github.com/fujiapple852/trippy


## avahi

https://github.com/avahi/avahi


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
