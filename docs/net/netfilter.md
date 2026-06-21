# netfilter

http://www.netfilter.org/projects/

# [wiki](https://en.wikipedia.org/wiki/Netfilter)
Userspace utility programs
- iptables(8)
- ip6tables(8)
- ebtables(8)
- arptables(8)
- ipset(8)
- nftables(8)
- conntrack(8)

nftables is the new packet-filtering portion of Netfilter. nft is the new userspace utility that replaces `iptables`, `ip6tables`, `arptables` and `ebtables`.

One of the important features built on top of the Netfilter framework is connection tracking. Connection tracking allows the kernel to keep track of all logical network connections or sessions, and thereby relate all of the packets which may make up that connection. NAT relies on this information to translate all related packets in the same way, and iptables can use this information to act as a stateful firewall.


# 简单的代码分析
-   net/ipv4/netfilter/iptable_nat.c
-   net/ipv4/netfilter/iptable_raw.c
-   net/ipv4/netfilter/iptable_filter.c
-   net/ipv4/netfilter/iptable_mangle.c
-   net/ipv4/netfilter/iptable_security.c
-   net/ipv4/netfilter/ip_tables.c

- [ ] /home/maritns3/core/linux/net/bridge/netfilter : 网桥是一个很神奇的东西啊, 很复杂
  - [ ] 实现我们所说的 vlan 的功能 ?

- netfilter 和 bpf 是什么关系?

- [ ] 好吧，并不能找到 routing table 相关的代码 ! (netfilter ?)
- [ ] Where is ebpf hooks for packet filter ?
- [ ] 使用 ip 来分析一下内核是如何实现路由的

## firewall

nixos 默认是打开防火墙的：
- https://nixos.org/manual/nixos/unstable/options.html#opt-networking.firewall.enable

这导致了机器可以 ping，但是 iperf 或者 python -m http.server 无法链接。

- [ ] 在 fedora 或者 mac 将 firewalld 关闭之后，连 ping 都无法搞定了

## netfilter
- https://randorisec.fr/crack-linux-firewall/

## filter
- [ ] https://devarea.com/introduction-to-network-filters-linux/
  - 似乎还行，但是给出来的用户态和内核态中的代码都不可以运行

参考这个来分析下吧:
- https://switch-router.gitee.io/categories/

## 简单使用下吧
https://www.redhat.com/sysadmin/iptables

## 朱双印
### [iptables详解（1）：iptables概念](https://www.zsythink.net/archives/1199)
<!-- 10d4d3f2-4eb1-4e8d-b44b-7eab6b2edc8b -->

### [iptables详解（2）：iptables实际操作之规则查询](https://www.zsythink.net/archives/1493)
<!-- d312ec10-a210-4577-a420-171cd2fe862d -->
```sh
sudo iptables -t filter -L
sudo iptables -t raw -L
sudo iptables -t mangle -L
sudo iptables -t nat -L
```

展示 chain
```sh
iptables -vL INPUT
```
```txt
Chain INPUT (policy ACCEPT)
target     prot opt source               destination
LIBVIRT_INP  all  --  anywhere             anywhere
```

```sh
sudo iptables -nvL INPUT
```
```txt
Chain INPUT (policy ACCEPT 7212K packets, 377G bytes)
 pkts bytes target     prot opt in     out     source               destination
7212K  377G LIBVIRT_INP  0    --  *      *       0.0.0.0/0            0.0.0.0/0
```

```sh
sudo iptables -t raw -L
```

```txt
iptables v1.8.9 (legacy): can't initialize iptables table `raw': Table does not exist (do you need to insmod?)
Perhaps iptables or your kernel needs to be upgraded.
```

```sh
sudo iptables -t nat -L
```

```txt
iptables v1.8.9 (legacy): can't initialize iptables table `nat': Table does not exist (do you need to insmod?)
Perhaps iptables or your kernel needs to be upgraded.
```

### [iptables详解（3）：iptables 规则管理](https://www.zsythink.net/archives/1517)
<!-- cfd1c0fa-74fd-4df6-85f3-77f4677bd974 -->

```sh
sudo iptables -t filter -I INPUT -s 10.0.0.2 -j DROP
```
之后无法从 host ping 通 guest

```sh
sudo iptables -nvL INPUT
iptables -t filter -D INPUT 3
```

### [iptables详解（4）：iptables 匹配条件总结之一](https://www.zsythink.net/archives/1544)
<!-- 3a44be1b-d9da-4e6e-b95c-f610a9e5ad08 -->
- [ ] 是如何转换为 ebpf 的

guest 中:
```sh
 python3 -m http.server
```

tcp 扩展，可以 pint 通，但是 wget
```sh
iptables -t filter -I INPUT -s 10.0.0.2 -p tcp -m tcp --dport 8000:9000 -j REJECT
```

- [iptables详解（5）：iptables匹配条件总结之二（常用扩展模块）](https://www.zsythink.net/archives/1564)
- [iptables详解（6）：iptables扩展匹配条件之’–tcp-flags’](https://www.zsythink.net/archives/1578)
- [iptables详解（7）：iptables扩展之udp扩展与icmp扩展](https://www.zsythink.net/archives/1588)
- [iptables详解（8）：iptables扩展模块之state扩展](https://www.zsythink.net/archives/1597)

### [iptables详解（10）：iptables自定义链](https://www.zsythink.net/archives/1625)
<!-- 98c20034-fb24-4bca-bafa-0a3d805c975e -->

```sh
iptables -t filter -N IN_WEB

iptables -nvL  IN_WEB

iptables -t filter -I INPUT -p tcp --dport 8000 -j IN_WE
```

### [iptables详解（12）：iptables动作总结之一](https://www.zsythink.net/archives/1684)
<!-- 3d25c1a5-c83c-4bdd-ab32-076a20241e3e -->

介绍 ACCEPT、DROP、REJECT、LOG


### [iptables详解（13）：iptables动作总结之二](https://www.zsythink.net/archives/1764)
<!-- 5b59b4e8-701d-4365-ab29-c5a529396cc4 -->

SNAT :

sudo iptables -nvL -t nat --line

sudo iptables -t nat -A POSTROUTING  -s 10.0.0.0/16 -j SNAT --to-source 192.168.11.3

sudo iptables -t nat -D POSTROUTING 6


并没有效果，

1. 现象 ， guest 中 ping 192.168.11.3 ，host 中 sudo tcpdump -i br-in -nn icmp

tcpdump 没有任何消息。

2. 难道 ovs 有问题吗？
- 回答这个问题: https://unix.stackexchange.com/questions/449654/iptable-snat-with-ovs-bridge

- https://docs.openstack.org/developer/dragonflow/specs/distributed_snat.html

### 更多的参考
- https://gist.github.com/tomasinouk/eec152019311b09905cd

- https://superuser.com/questions/1410865/what-is-the-difference-between-nat-and-snat-dnat
  - SNAT : connecting from a private to a public IP address
  - DNAT : connecting from a public IP to a private IP


### 原来是这样配置 gateway 的
https://unix.stackexchange.com/questions/222054/how-can-i-use-linux-as-a-gateway


https://www.karlrupp.net/en/computer/nat_tutorial

物理机中执行:
```sh
sudo iptables -A FORWARD -i wlo1 -j ACCEPT
sudo iptables -t nat -A POSTROUTING -o br-in -j MASQUERADE
```

虚拟机中执行:
```sh
ip route add default via 10.0.0.2 dev enp1s0
```

这个时候虚拟机可以 ping 通 100.100.100.100 ，但是，无法 dns 解析。


虚拟机中 wget baidu.com 的时候，可以看到:
```txt
🧀  sudo tcpdump -i br-in
tcpdump: verbose output suppressed, use -v[v]... for full protocol decode
listening on br-in, link-type EN10MB (Ethernet), snapshot length 262144 bytes
21:25:16.137164 ARP, Request who-has bogon tell bogon, length 28
21:25:17.161378 ARP, Request who-has bogon tell bogon, length 28
21:25:18.185122 ARP, Request who-has bogon tell bogon, length 28
21:25:19.209096 ARP, Request who-has bogon tell bogon, length 28
21:25:20.233181 ARP, Request who-has bogon tell bogon, length 28
21:25:21.257082 ARP, Request who-has bogon tell bogon, length 28
21:25:22.281096 ARP, Request who-has bogon tell bogon, length 28
```
用虚拟机测试吧，此外，瞎鸡巴尝试已经没有意义了，先这样吧，需要彻底搞清楚这个问题。

## 其他
将这个从模块变为 built-in
```txt
 Symbol: IP_NF_NAT [=m]                                                                                                                                                                                                                                │
 Type  : tristate                                                                                                                                                                                                                                      │
 Defined at net/ipv4/netfilter/Kconfig:214                                                                                                                                                                                                             │
   Prompt: iptables NAT support                                                                                                                                                                                                                        │
   Depends on: NET [=y] && INET [=y] && NETFILTER [=y] && IP_NF_IPTABLES [=y] && NF_CONNTRACK [=y]                                                                                                                                                     │
   Location:                                                                                                                                                                                                                                           │
     -> Networking support (NET [=y])                                                                                                                                                                                                                  │
       -> Networking options                                                                                                                                                                                                                           │
         -> Network packet filtering framework (Netfilter) (NETFILTER [=y])                                                                                                                                                                            │
           -> IP: Netfilter Configuration                                                                                                                                                                                                              │
             -> IP tables support (required for filtering/masq/NAT) (IP_NF_IPTABLES [=y])                                                                                                                                                              │
 (1)           -> iptables NAT support (IP_NF_NAT [=m])                                                                                                                                                                                                │
 Selects: NF_NAT [=y] && NETFILTER_XT_NAT [=m] && IP_NF_IPTABLES_LEGACY [=y]                                                                                                                                                                           │
```

实现的 nat table 的位置: net/ipv4/netfilter/iptable_nat.c



## 一个花里胡哨的程序
https://safing.io/


## 看来很清楚，就是设置 netfilter 而已
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/security_guide/sec-using_firewalls#sec-Getting_started_with_firewalld

## 看看这个漏洞吧
https://github.com/Notselwyn/CVE-2024-1086

## nf contrack
https://blog.cloudflare.com/conntrack-tales-one-thousand-and-one-flows

## 看看这个案例吧，非常好的
https://access.redhat.com/solutions/401273

## 似乎可以使用 pwru 这个工具来调试 nat 的问题
- https://github.com/cilium/pwru
- https://cilium.io/blog/2023/03/22/packet-where-are-you/

语法规则参考: https://linux.die.net/man/7/pcap-filter

使用
```sh
sudo pwru "src host 10.0.58.0"
```

当从 guest os 中 ping 10.0.0.2
```txt
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]        netif_receive_skb
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]      __netif_receive_skb
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978] __netif_receive_skb_one_core
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]                 skb_push
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]           __skb_get_hash
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]           eth_type_trans
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]                 netif_rx
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]        netif_rx_internal
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]       enqueue_to_backlog
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]      __netif_receive_skb
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978] __netif_receive_skb_one_core
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]                   ip_rcv
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]              ip_rcv_core
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]             nf_hook_slow
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]           nf_ip_checksum
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]  __skb_checksum_complete
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]     ip_route_input_noref
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]      ip_route_input_slow
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]      fib_validate_source
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]    __fib_validate_source
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]         ip_local_deliver
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]             nf_hook_slow
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]  ip_local_deliver_finish
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]  ip_protocol_deliver_rcu
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]        raw_local_deliver
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]                 icmp_rcv
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]                icmp_echo
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]               icmp_reply
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]        __ip_options_echo
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]     fib_compute_spec_dst
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]              consume_skb
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]   skb_release_head_state
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]         skb_release_data
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]            skb_free_head
0xffffa159e41f1200      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]             kfree_skbmem
0xffffa159e41f0000      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]                 skb_push
0xffffa159e41f0000      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]           __skb_get_hash
0xffffa159e41f0000      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]              consume_skb
0xffffa159e41f0000      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]   skb_release_head_state
0xffffa159e41f0000      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]         skb_release_data
0xffffa159e41f0000      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]            skb_free_head
0xffffa159e41f0000      8 [/home/martins3/core/qemu/build/qemu-system-x86_64:1004978]             kfree_skbmem
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]           eth_type_trans
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]                 netif_rx
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]        netif_rx_internal
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]       enqueue_to_backlog
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]      __netif_receive_skb
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765] __netif_receive_skb_one_core
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]                  arp_rcv
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]              arp_process
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]     ip_route_input_noref
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]      ip_route_input_slow
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]      fib_validate_source
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]    __fib_validate_source
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]              consume_skb
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]   skb_release_head_state
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]         skb_release_data
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]            skb_free_head
0xffffa15b95ec6000      9 [/nix/store/g73nc7vfabpg6v0vq3pxlhis8pj1dza1-openvswitch-2.17.9/bin/ovs-vswitchd:1765]             kfree_skbmem
```
实在是好奇，这个到底可以有如何帮助排查问题。

而且每次关闭的时候，非常慢。

## 看看这个
- https://news.ycombinator.com/item?id=41396206
  - https://www.haskellforall.com/2024/08/firewall-rules-not-as-secure-as-you.html

## 展示当前的 nat 规则
```txt
🤒  sudo iptables -t nat -L
[sudo] password for martins3:
Chain PREROUTING (policy ACCEPT)
target     prot opt source               destination
DOCKER     all  --  anywhere             anywhere             ADDRTYPE match dst-type LOCAL

Chain INPUT (policy ACCEPT)
target     prot opt source               destination

Chain OUTPUT (policy ACCEPT)
target     prot opt source               destination
DOCKER     all  --  anywhere            !127.0.0.0/8          ADDRTYPE match dst-type LOCAL

Chain POSTROUTING (policy ACCEPT)
target     prot opt source               destination
ts-postrouting  all  --  anywhere             anywhere
MASQUERADE  all  --  bogon/16             anywhere
MASQUERADE  all  --  bogon/24             anywhere
MASQUERADE  all  --  bogon/16             anywhere
MASQUERADE  all  --  bogon/16             anywhere
MASQUERADE  all  --  bogon/24             anywhere

Chain DOCKER (2 references)
target     prot opt source               destination
RETURN     all  --  anywhere             anywhere
RETURN     all  --  anywhere             anywhere
RETURN     all  --  anywhere             anywhere
RETURN     all  --  anywhere             anywhere
RETURN     all  --  anywhere             anywhere

Chain ts-postrouting (1 references)
target     prot opt source               destination
MASQUERADE  all  --  anywhere             anywhere             mark match 0x40000/0xff0000
```

添加如下规则前后:
```sh
	sudo iptables -t nat -A POSTROUTING -s 10.0.0.0/16 -o $wifi -j MASQUERADE
	# 添加转发规则
	sudo iptables -A FORWARD -i $nic -o $wifi -j ACCEPT
	sudo iptables -A FORWARD -i $wifi -o $nic -m state --state RELATED,ESTABLISHED -j ACCEPT
```

```txt
🧀  sudo iptables -t nat -L
Chain PREROUTING (policy ACCEPT)
target     prot opt source               destination

Chain INPUT (policy ACCEPT)
target     prot opt source               destination

Chain OUTPUT (policy ACCEPT)
target     prot opt source               destination

Chain POSTROUTING (policy ACCEPT)
target     prot opt source               destination
MASQUERADE  all  --  bogon/16             anywhere

Chain DOCKER (0 references)
target     prot opt source               destination

Chain ts-postrouting (0 references)
target     prot opt source               destination


🧀  sudo iptables -t nat -L
Chain PREROUTING (policy ACCEPT)
target     prot opt source               destination

Chain INPUT (policy ACCEPT)
target     prot opt source               destination

Chain OUTPUT (policy ACCEPT)
target     prot opt source               destination

Chain POSTROUTING (policy ACCEPT)
target     prot opt source               destination

Chain DOCKER (0 references)
target     prot opt source               destination

Chain ts-postrouting (0 references)
target     prot opt source               destination
```

## netfilter 配置经典案例 : wifi 转发
<!-- 165b8df6-3f46-494a-8c36-53bdae69ac11 -->

使用如下提示器即可:
```txt
我现在有一个机器的网络配置如下

4: wlo1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    altname wlp0s20f3
    inet 192.168.11.3/22 brd 192.168.11.255 scope global dynamic noprefixroute wlo1
       valid_lft 85729sec preferred_lft 85729sec
7: br-in: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UNKNOWN group default qlen
1000
    inet 10.0.0.2/16 scope global br-in
       valid_lft forever preferred_lft forever

br-in 是 ovs bridge ，qemu 虚拟机通过 tun 和 br-in 连接。

通过 wlo1 可以连接到互联网。

在 qemu 虚拟机中，可以配置虚拟机网卡的 ip 为 10.0.0.3/16 来和 host 沟通，但是无法通过该 ip 访问互联网。

给如何配置主机的网络。
```

collei/collei-global.sh
```sh
wifi=wlo1
vb=br-in # virtual bridge
# sudo iptables -t nat -F
# 添加 NAT 规则，将 10.0.0.0/16 的流量通过 wlo1 伪装
sudo iptables -t nat -A POSTROUTING -s 10.0.0.0/16 -o $wifi -j MASQUERADE
# 添加转发规则
sudo iptables -A FORWARD -i $vb -o $wifi -j ACCEPT
sudo iptables -A FORWARD -i $wifi -o $vb -m state --state RELATED,ESTABLISHED -j ACCEPT
```

## netfilter 配置经典案例 : hyperv 虚拟机转发网络
<!-- 7ba9064c-f8a3-4c10-9fcf-d430dfed9d7c -->

1. n100 和 windows 网线相连
2. hyperv 虚拟机有两个网卡，一个默认网卡，一个网卡和物理机绑定
3. n100 网络
```txt
5: br9527: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 58:47:ca:76:2d:9f brd ff:ff:ff:ff:ff:ff
    inet 10.0.0.5/16 brd 10.0.255.255 scope global noprefixroute br9527
       valid_lft forever preferred_lft forever
    inet6 fe80::95a:c9ba:9ab8:d281/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```
4. hyperv 虚拟机中
```txt
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether 00:15:5d:00:08:04 brd ff:ff:ff:ff:ff:ff
    altname enx00155d000804
    inet 172.18.146.93/20 brd 172.18.159.255 scope global dynamic noprefixroute eth0
       valid_lft 56951sec preferred_lft 56951sec
    inet6 fe80::215:5dff:fe00:804/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
3: eth1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether 00:15:5d:00:08:06 brd ff:ff:ff:ff:ff:ff
    altname enx00155d000806
    inet 10.0.0.80/16 brd 10.0.255.255 scope global noprefixroute eth1
       valid_lft forever preferred_lft forever
    inet6 fe80::639c:bac:626c:7c40/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```


n100
```txt
sudo ip route add default via 10.0.0.80
```

hyperv 虚拟机中配置:
```txt
# 1. 开启 IP 转发（已执行）
echo 1 > /proc/sys/net/ipv4/ip_forward

# 2. 配置 NAT（已执行）
iptables -t nat -A POSTROUTING -s 10.0.0.5/32 -j SNAT --to-source 10.0.0.80

# 3. 配置 FORWARD 链（重新执行，注意是一行）
iptables -A FORWARD -s 10.0.0.5/32 -j ACCEPT
iptables -A FORWARD -d 10.0.0.5/32 -m state --state ESTABLISHED,RELATED -j ACCEPT

查看当前规则是否生效：

iptables -t nat -L -n -v
iptables -L FORWARD -n -v
```
这让我意识到，之前的wifi 配置其实是过于麻烦了,

## 参考他的写的东西吧
https://github.com/hvhghv/se-script/blob/main/linux-firewall/firewall-cui.sh


## 支持一下 libvirt 的网络
```txt
zcat /proc/config.gz | grep CONFIG_NF_TABLES
CONFIG_NF_TABLES=m
# CONFIG_NF_TABLES_INET is not set
# CONFIG_NF_TABLES_NETDEV is not set
# CONFIG_NF_TABLES_IPV4 is not set
# CONFIG_NF_TABLES_ARP is not set
# CONFIG_NF_TABLES_IPV6 is not set
# CONFIG_NF_TABLES_BRIDGE is not set
```

一个又一个的错误

nft add chain ip libvirt_network guest_nat

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
