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
  - [ ] 实现我们所说的 vlan 的功能
