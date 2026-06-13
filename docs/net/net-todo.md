# 这些都可以稍稍，RDMA 才是下一个关键
## 看看
[Chapter 6: Congestion Control](https://book.systemsapproach.org/congestion.html)

## 想要自己 ping 过网卡，看来没那么容易
https://stackoverflow.com/questions/2734144/linux-disable-using-loopback-and-send-data-via-wire-between-2-eth-cards-of-one/4490220#4490220

## 默认模式下，QEMU 是如何保证给一个分配的 10.0.2.15 的

```txt
2: ens5: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:12:34:56 brd ff:ff:ff:ff:ff:ff
    inet 10.0.2.15/24 brd 10.0.2.255 scope global dynamic ens5
       valid_lft 86447sec preferred_lft 86447sec
    inet6 fec0::5054:ff:fe12:3456/64 scope site dynamic mngtmpaddr
       valid_lft 86388sec preferred_lft 14388sec
    inet6 fe80::5054:ff:fe12:3456/64 scope link
       valid_lft forever preferred_lft forever
```

slirp 中自己需要实现一个 nat ，将从 10.0.2.15 的请求进行装换为另外一个.

在 qemu 中 net/slirp.c 可以看到:
```c
    struct in_addr dhcp = { .s_addr = htonl(0x0a00020f) }; /* 10.0.2.15 */
```

## 理解下这个输出

```txt
🧀  sudo ethtool enp5s0

Settings for enp5s0:
        Supported ports: [ TP ]
        Supported link modes:   10baseT/Half 10baseT/Full
                                100baseT/Half 100baseT/Full
                                1000baseT/Full
                                2500baseT/Full
        Supported pause frame use: Symmetric
        Supports auto-negotiation: Yes
        Supported FEC modes: Not reported
        Advertised link modes:  10baseT/Half 10baseT/Full
                                100baseT/Half 100baseT/Full
                                1000baseT/Full
                                2500baseT/Full
        Advertised pause frame use: Symmetric
        Advertised auto-negotiation: Yes
        Advertised FEC modes: Not reported
        Speed: 1000Mb/s
        Duplex: Full
        Auto-negotiation: on
        Port: Twisted Pair
        PHYAD: 0
        Transceiver: internal
        MDI-X: off (auto)
        Supports Wake-on: pumbg
        Wake-on: g
        Current message level: 0x00000007 (7)
                               drv probe link
        Link detected: yes
```

## 将 ovs 剩下的实验都搞完吧
最后实现热迁移了。

搭建一个虚拟机迁移的网络

虚拟机可以正常迁移的另一个机器上，因为是 vxlan ，所以可以依旧可以保持网络不变的。

## IP address that is the equivalent of /dev/null
https://superuser.com/questions/698244/ip-address-that-is-the-equivalent-of-dev-null

## 理解下 wireshark 的原理，为什么 wireshark 可以将所有的包都截获一次
感觉和 netfilter

## 尝试一下使用 .255 结尾
什么时候使用 broadcast ，那么 multicast 如何使用的

## [ ] linux network interfaces
- https://developers.redhat.com/blog/2018/10/22/introduction-to-linux-interfaces-for-virtual-networking
- https://developers.redhat.com/blog/2019/05/17/an-introduction-to-linux-virtual-interfaces-tunnels#

## [ ] openwrt 到底是什么?
- 教别人编译的 : https://github.com/coolsnowwolf/lede
  - https://github.com/newsnowlabs/runcvm 在虚拟机中运行 wrt 的东西

## 这个是如何影响 sudo iptables -L 的结果的
```txt
  networking.firewall = {
    # enable the firewall
    enable = true;

    # always allow traffic from your Tailscale network
    trustedInterfaces = [ "tailscale0" ];

    # allow the Tailscale UDP port through the firewall
    allowedUDPPorts = [ config.services.tailscale.port ];

    # allow you to SSH in over the public internet
    allowedTCPPorts = [
      22 # ssh
      21 # ftp
      5201 # iperf
      8889 # clash
      5900 # qemu vnc
      445 # samba
      /* 8384 # syncthing */
      /* 22000 # syncthing */
    ];
  };

```

## 尝试下 qemu 中中  -netdev socket 中的两个选项分析下

## macvtap 是做啥的

https://www.ibm.com/docs/en/linux-on-systems?topic=choices-using-macvtap-driver

## 可以做的测试
3. iscsi

## virtio 网卡需要特殊的代码才能支持 rss ?

## 也许可以用
https://www.isi.edu/nsnam/ns/

## 实验
https://github.com/srl-labs/containerlab

## 多网卡问题

1. 难道存在多个网卡配置一个子网的 ip ，第一个网卡 down 掉之后，其他的不能用了?


配置两个虚拟机，每一个虚拟机的配置大致为:
```txt
4: ens4: <BROADCAST,MULTICAST> mtu 1500 qdisc fq_codel state DOWN group default qlen 1000
    link/ether 52:54:00:12:34:56 brd ff:ff:ff:ff:ff:ff
    inet 10.0.2.15/24 brd 10.0.2.255 scope global dynamic ens4
       valid_lft 86268sec preferred_lft 86268sec
5: ens5: <BROADCAST,MULTICAST> mtu 1500 qdisc fq_codel state DOWN group default qlen 1000
    link/ether 52:54:00:00:02:2d brd ff:ff:ff:ff:ff:ff
    inet 10.0.0.109/16 brd 10.0.255.255 scope global ens5
       valid_lft forever preferred_lft forever
6: ens6: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:00:03:2d brd ff:ff:ff:ff:ff:ff
    inet 10.0.0.110/16 brd 10.0.255.255 scope global ens6
       valid_lft forever preferred_lft forever
    inet6 fe80::6258:d729:b2d8:c75a/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```

因为 ens5 和 ens6 配置两个网卡是在同一个网段中，两个虚拟机中任何一个网卡，只要不是全部都 down 掉，
那么宛如所有的卡都是可以正常使用的。

2. 还是不对，直通之后为什么不能用 ?

所以很奇怪，这个是如何实现的?

给一个机器配置了两个网卡，一个网卡一个 ip ，网卡 a 可以连接到网络上，网卡 b 不可以，
但是可以 ping 通 b

## 再次尝试配置网卡

原来 virbr0 是 libvirt 创建出来用于 nat 的:

https://askubuntu.com/questions/246343/what-is-the-virbr0-interface-used-for

https://wiki.qemu.org/Documentation/Networking/NAT

```txt
12: virbr0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 52:54:00:d2:41:1e brd ff:ff:ff:ff:ff:ff
    inet 192.168.122.1/24 brd 192.168.122.255 scope global virbr0
       valid_lft forever preferred_lft forever
13: vnet0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue master virbr0 state UNKNOWN group default qlen 1000
    link/ether fe:54:00:b6:44:74 brd ff:ff:ff:ff:ff:ff
    inet6 fe80::fc54:ff:feb6:4474/64 scope link proto kernel_ll
       valid_lft forever preferred_lft forever
```


## 到底为什么需要三次握手来着 ？

无法同时确定?

A 发送任何消息，都是期待一个 ack 的:

A ---> B  // A : can I do it
A <--- B  // B , do it (发送之前，A 不知道 B 是否接受到，发送之后，B 还是不知道 A 是否完成了)

A ---> B  // B 现在知道 A did it 了。


## 分析下 clash-meta 中的技术

- gvisor 的 tun mode 和 systemd tun mode 和一般的模式的区别?
- 如何不使用 tun mode ，那么数据流如何？
- 系统代理模式如何实现的?

## 分析下 netdev 的 feature
```c
typedef u64 netdev_features_t;
```
猜测如果网卡没有这个功能，那个

原来 ovs 也是需要创建一个 dev 的:
net/openvswitch/vport-internal_dev.c

## 这个 commit 啥意思，看来 vritio 的实现还可以影响
7c6d2ecbda83150b2036a2b36b21381ad4667762

cf9acc90c80ecbee00334aa85d92f4e74014bcff


## net todo
https://blog.sofiane.cc/setup_ssh_honeypot/

## 又多了一些奇怪的设备

```txt
➜  ~ ls -la /sys/class/net
total 0
drwxr-xr-x  2 root root    0 Jul 25 18:28 .
drwxr-xr-x 79 root root    0 Jul 25 18:27 ..
lrwxrwxrwx  1 root root    0 Jul 25 18:27 bond0 -> ../../devices/virtual/net/bond0
-rw-r--r--  1 root root 4096 Jul 25 18:27 bonding_masters
lrwxrwxrwx  1 root root    0 Jul 25 18:28 cni-podman0 -> ../../devices/virtual/net/cni-podman0
lrwxrwxrwx  1 root root    0 Jul 25 18:27 docker0 -> ../../devices/virtual/net/docker0
lrwxrwxrwx  1 root root    0 Jul 25 18:27 dummy0 -> ../../devices/virtual/net/dummy0
lrwxrwxrwx  1 root root    0 Jul 25 18:27 ens4 -> ../../devices/pci0000:00/0000:00:04.0/virtio1/net/ens4
lrwxrwxrwx  1 root root    0 Jul 25 18:27 ens5 -> ../../devices/pci0000:00/0000:00:05.0/virtio2/net/ens5
lrwxrwxrwx  1 root root    0 Jul 25 18:27 ens6 -> ../../devices/pci0000:00/0000:00:06.0/virtio3/net/ens6
lrwxrwxrwx  1 root root    0 Jul 25 18:27 lo -> ../../devices/virtual/net/lo
lrwxrwxrwx  1 root root    0 Jul 25 18:27 sit0 -> ../../devices/virtual/net/sit0
```
sit0 是啥 ?


## 理解这个命令

```txt
➜  ~ iptables -t nat -A CNI-db45cc4ae5b9b8e9610845b5 -d 10.88.0.4/16 -j ACCEPT -m comment --comment name: "podman" id: "720ff332496649827ae1e727ec77e730a09847fab97a6cec0f15ba70d8322cd3" --wait
Warning: Extension comment revision 0 not supported, missing kernel module?
Bad argument `podman'
Try `iptables -h' or 'iptables --help' for more information.
```

为什么这个用了这几个模块，只是启动而已
- "xt_mark"
- "xt_comment"
- "xt_multiport"

net/netfilter/xt_comment.c 一个超级 mini 的模块

顺便，这个也是 podman 需要的
```txt
  ┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── User namespace ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
  │ CONFIG_USER_NS:                                                                                                                                                                                                                                                                     │
  │                                                                                                                                                                                                                                                                                     │
  │ This allows containers, i.e. vservers, to use user namespaces                                                                                                                                                                                                                       │
  │ to provide different user info for different servers.                                                                                                                                                                                                                               │
  │                                                                                                                                                                                                                                                                                     │
  │ When user namespaces are enabled in the kernel it is                                                                                                                                                                                                                                │
  │ recommended that the MEMCG option also be enabled and that                                                                                                                                                                                                                          │
  │ user-space use the memory control groups to limit the amount                                                                                                                                                                                                                        │
  │ of memory a memory unprivileged users can use.                                                                                                                                                                                                                                      │
  │                                                                                                                                                                                                                                                                                     │
  │ If unsure, say N.                                                                                                                                                                                                                                                                   │
  │                                                                                                                                                                                                                                                                                     │
  │ Symbol: USER_NS [=y]                                                                                                                                                                                                                                                                │
  │ Type  : bool                                                                                                                                                                                                                                                                        │
  │ Defined at init/Kconfig:1236                                                                                                                                                                                                                                                        │
  │   Prompt: User namespace                                                                                                                                                                                                                                                            │
  │   Depends on: NAMESPACES [=y]                                                                                                                                                                                                                                                       │
  │   Location:                                                                                                                                                                                                                                                                         │
  │     -> General setup                                                                                                                                                                                                                                                                │
  │       -> Namespaces support (NAMESPACES [=y])                                                                                                                                                                                                                                       │
  │         -> User namespace (USER_NS [=y])
```

不然这个错误:
```txt
➜  ~ podman run hello-world
WARN[0000] Failed to read current user namespace mappings
Error: crun: error opening file `/proc/self/setgroups`: No such file or directory: OCI runtime attempted to invoke a command that was not found
```

## sit0
https://unix.stackexchange.com/questions/643890/what-is-this-sit0-device

在 podman 的容器中可以看到 sit0
```txt
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host proto kernel_lo
       valid_lft forever preferred_lft forever
2: sit0@NONE: <NOARP> mtu 1480 qdisc noop state DOWN group default qlen 1000
    link/sit 0.0.0.0 brd 0.0.0.0
3: eth0@if10: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default
    link/ether ca:d5:95:80:3e:1d brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 10.88.0.2/16 brd 10.88.255.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::c8d5:95ff:fe80:3e1d/64 scope link proto kernel_ll
       valid_lft forever preferred_lft forever
```

## 才发现 sysfs 也可以 namespace ，在虚拟机中

```txt
➜  martins3 ls -la /sys/class/net
total 0
drwxr-xr-x  2 root root    0 Jul 26 02:00 .
drwxr-xr-x 79 root root    0 Jul 26 01:56 ..
-rw-r--r--  1 root root 4096 Jul 26 02:01 bonding_masters
lrwxrwxrwx  1 root root    0 Jul 26 02:01 eth0 -> ../../devices/virtual/net/eth0
lrwxrwxrwx  1 root root    0 Jul 26 02:01 lo -> ../../devices/virtual/net/lo
lrwxrwxrwx  1 root root    0 Jul 26 02:01 sit0 -> ../../devices/virtual/net/sit0
```

在物理机中可以看到:
```txt
➜  net ls -la /sys/class/net
total 0
drwxr-xr-x  2 root root    0 Jul 26 10:02 .
drwxr-xr-x 79 root root    0 Jul 26 09:56 ..
lrwxrwxrwx  1 root root    0 Jul 26 09:29 bond0 -> ../../devices/virtual/net/bond0
-rw-r--r--  1 root root 4096 Jul 26 09:56 bonding_masters
lrwxrwxrwx  1 root root    0 Jul 26 09:31 cni-podman0 -> ../../devices/virtual/net/cni-podman0
lrwxrwxrwx  1 root root    0 Jul 26 09:29 docker0 -> ../../devices/virtual/net/docker0
lrwxrwxrwx  1 root root    0 Jul 26 09:29 dummy0 -> ../../devices/virtual/net/dummy0
lrwxrwxrwx  1 root root    0 Jul 26 09:29 ens4 -> ../../devices/pci0000:00/0000:00:04.0/virtio1/net/ens4
lrwxrwxrwx  1 root root    0 Jul 26 09:29 ens5 -> ../../devices/pci0000:00/0000:00:05.0/virtio2/net/ens5
lrwxrwxrwx  1 root root    0 Jul 26 09:29 ens6 -> ../../devices/pci0000:00/0000:00:06.0/virtio3/net/ens6
lrwxrwxrwx  1 root root    0 Jul 26 09:29 lo -> ../../devices/virtual/net/lo
lrwxrwxrwx  1 root root    0 Jul 26 09:29 sit0 -> ../../devices/virtual/net/sit0
lrwxrwxrwx  1 root root    0 Jul 26 10:00 veth108f1c2c -> ../../devices/virtual/net/veth108f1c2c
lrwxrwxrwx  1 root root    0 Jul 26 10:01 veth72ec7e3b -> ../../devices/virtual/net/veth72ec7e3b
```

## podman 网络简单分析

这个 ip 默认不会复用，新
```txt
3: eth0@if17: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default
    link/ether ce:39:7b:e9:9b:1e brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 10.88.0.9/16 brd 10.88.255.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::cc39:7bff:fee9:9b1e/64 scope link proto kernel_ll
       valid_lft forever preferred_lft forever
```

docker 之间可以互相 ping 通，而且可以 ping 通 10.0.0.0/16 上的机器。

可以 ping 通 baidu.com 的。


## 这三个东西是做什么的，为什么 ovs 需要这个东西
CONFIG_NET_IPGRE_DEMUX is not set
CONFIG_VXLAN is not set
CONFIG_GENEVE is not set

docker 中增加了这几个东西:
```txt
2: gre0@NONE: <NOARP> mtu 1476 qdisc noop state DOWN group default qlen 1000
    link/gre 0.0.0.0 brd 0.0.0.0
3: gretap0@NONE: <BROADCAST,MULTICAST> mtu 1462 qdisc noop state DOWN group default qlen 1000
    link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff
4: erspan0@NONE: <BROADCAST,MULTICAST> mtu 1450 qdisc noop state DOWN group default qlen 1000
    link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff
```

## 给 veth 和 tap 设备配置 ip 地址，有意义吗?

## docker 如何实现只是 map 一个 port 的 ？

```txt
podman run --name aaa -dt -p 8081:80/tcp docker.io/nginx
```

## docker 如何实现修改hostname 的

```txt
🧀  podman run --hostname good -it fedora
[root@good /]# ls
afs  bin  boot  dev  etc  home  lib  lib64  lost+found  media  mnt  opt  proc  root  run  sbin  srv  sys  tmp  usr  var
[root@good /]# cat /etc/hostname
good[root@good /]#
```

## 这个人是一个高手，他技术文章风格很好
https://blog.gmem.cc/network-faq

## 透明代理是什么?
https://blog.gmem.cc/istio-tproxy

## netcap
- https://github.com/bytedance/netcap

- https://mp.weixin.qq.com/s/0SHGaMxA2KrjozygK3ZR3w

## 高级
https://news.ycombinator.com/item?id=41162664

## 原来配置网络就是靠这个

/etc/sysconfig/network-scripts/ 中定义的配置文件是
可以定义 ETHTOOL_OPTS ，其被
/etc/sysconfig/network-scripts/network-functions 解析
最后被 ifdown 和 ifup 解析

- https://access.redhat.com/solutions/54229

这个的 ETHTOOL_OPTS 的工具最后是可以通过


## 如果两个网卡在 ovs 下，那么 ovs 是不是可以自动做负载均衡，
让这个机器可以可以拥有两个网卡的性能。

## 为什么 CONFIG_BRIDGE 会依赖 CONFIG_LLC 啊

## https://jia.je/networking/2023/06/20/spanning-tree-protocol/#virtual-port-channel-vpc

## 有必要理解 network manager , ifup ifdown ，以及 ubunut 的工具都是做啥的
https://serverfault.com/questions/463111/how-to-persist-ethtool-settings-through-reboot

## 这些网络文章都写不错，可以都会看看
https://thiscute.world/posts/iptables-and-container-networks/

## 用这个再次尝试一下 nat 的配置
https://thiscute.world/posts/about-nat/

https://www.kawabangga.com/posts/5324

## 看看这个仓库都做的啥
https://github.com/Mellanox

## 将 Mellanox 的网卡的每一个包都搞出来

## 这个看看
https://biriukov.dev/docs/resolver-dual-stack-application/6-dual-stack-applications/


## 这个放到哪里去
https://news.ycombinator.com/item?id=41461850

## 看看这个启动系统
https://github.com/mas-bandwidth

## 可以细品的东西，一个新的思考问题的方向
https://github.com/leandromoreira/linux-network-performance-parameters

## vhost 和 ovs 不是耦合的，可以用这样的一个矩阵才对

(vhost non-vhost) - (bridge, ovs)

没有

## 如何理解这种流程

iperf server
```txt
@[
    dev_hard_start_xmit+0
    __dev_xmit_skb+588
    __dev_queue_xmit+1060
    dev_queue_xmit+20
    ovs_vport_send+172
    do_output+108
    do_execute_actions+2568
    ovs_execute_actions+100
    ovs_dp_process_packet+164
    ovs_vport_receive+148
    internal_dev_xmit+60
    dev_hard_start_xmit+164
    __dev_queue_xmit+592
    neigh_hh_output+156
    ip_finish_output2+856
    __ip_finish_output+172
    ip_finish_output+60
    ip_output+116
    __ip_queue_xmit+372
    ip_queue_xmit+28
    __tcp_transmit_skb+1152
    __tcp_send_ack.part.0+212
    tcp_send_ack+44
    __tcp_cleanup_rbuf+140
    tcp_cleanup_rbuf+80
    tcp_recvmsg_locked+540
    tcp_recvmsg+140
    inet6_recvmsg+84
    sock_recvmsg+120
    sock_read_iter+164
    vfs_read+728
    ksys_read+248
    __arm64_sys_read+36
    invoke_syscall+116
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 37211
@[
    dev_hard_start_xmit+0
    neigh_hh_output+156
    ip_finish_output2+856
    __ip_finish_output+172
    ip_finish_output+60
    ip_output+116
    __ip_queue_xmit+372
    ip_queue_xmit+28
    __tcp_transmit_skb+1152
    __tcp_send_ack.part.0+212
    tcp_send_ack+44
    __tcp_cleanup_rbuf+140
    tcp_cleanup_rbuf+80
    tcp_recvmsg_locked+540
    tcp_recvmsg+140
    inet6_recvmsg+84
    sock_recvmsg+120
    sock_read_iter+164
    vfs_read+728
    ksys_read+248
    __arm64_sys_read+36
    invoke_syscall+116
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 37221
```
在 read 的系统调用立刻发送，是不是恢复一个信息而已。


iperf client 调用 dev_hard_start_xmit 的主要路径:
```txt
@[
    dev_hard_start_xmit+5
    sch_direct_xmit+164
    __qdisc_run+323
    net_tx_action+479
    handle_softirqs+228
    __irq_exit_rcu+152
    common_interrupt+135
    asm_common_interrupt+38
    cpuidle_enter_state+205
    cpuidle_enter+45
    do_idle+436
    cpu_startup_entry+41
    start_secondary+284
    common_startup_64+318
]: 61171
```
居然发送是在软中断中发送的，这么有趣，需要看看了

## 如何理解 softirq 中的 RX 和 TX

两个物理机测试:

iperf client:
```txt
SOFTIRQ          TOTAL_usecs
rcu                       54
timer                    118
sched                    403
hi                      1168
tasklet                12697
net_tx                 18729
net_rx                 28815
```

iperf server
```txt
SOFTIRQ          TOTAL_usecs
hrtimer                    1
block                      8
rcu                      189
timer                    493
net_tx                  1029
sched                   4414
tasklet               224351
net_rx                406594
```

似乎在

## 分析下常用的 net 下的 tracepoing

例如
netif_receive_skb
net_dev_start_xmit

如果虚拟机中测试，可以发现这样的:
```txt
@[
    netif_receive_skb+5
    tun_sendmsg+1055
    vhost_tx_batch.isra.0+111
    handle_tx_copy+402
    handle_tx+182
    vhost_run_work_list+69
    vhost_task_fn+75
    ret_from_fork+49
    ret_from_fork_asm+26
]: 74284
```
如果是两个物理机直接，那么没有

## bcc 中 netqtop 测试下

## 如何利用 iperf 制作超级多的中断，就是看看如果网络的 package 的大小不同的效果


## 确认一下这个，似乎 ovs 拷贝，可以让地址一样

如果拷贝 alpine 中的虚拟机:

如果没有 ovs ，就是随便拷贝了:

如果有 ovs ，会有这个问题:
```txt
[    2.609623] systemd-journald[799]: Received client request to flush runtime journal.
[    2.729945] EXT4-fs (sda2): mounted filesystem 2ae2509f-9cdb-4643-8860-335df89b59cf r/w with ordered data mode. Quota mode: none.
[    2.841368] EXT4-fs (dm-2): mounted filesystem 34fdfef6-702b-4f09-ad5a-52e8e1378b0f r/w with ordered data mode. Quota mode: none.
[    3.948671] IPv6: ens5: IPv6 duplicate address fe80::8bf0:7cbe:1fd5:793b used by 52:54:00:00:02:25 detected!
```

## 将 ping io 路径分析一下

## kernel 是如何直接统计一个 vnet 这种 ovs 的 nic 的信息的

## 测试下 nf conection contrck 是如何使用的

## 这个好啊
https://dbwu.tech/posts/network/what-is-tcp-fast-open/

## 有趣的
- https://news.ycombinator.com/item?id=41685533

- https://blog.cloudflare.com/zh-cn/tcp-resets-timeouts/

## 看看这个
https://github.com/Shopify/toxiproxy

https://github.com/jpillora/chisel

https://mp.weixin.qq.com/s/w4w3FRJgiJ5ArNjSX2hroA


## 查漏补缺一下
https://wiki.gentoo.org/wiki/QEMU_with_Open_vSwitch_network

## arping 和 ping 有什么区别

https://www.linux.com/news/ping-icmp-vs-arp/

没有太多好资料，但是我的确遇到了一次，无法 ping 通，但是 arping 可以检测到

```txt
$ arping 10.10.128.92
ARPING 10.10.128.92 from 172.21.128.92 port-mgt
Unicast reply from 10.10.128.92 [2E:AF:19:DB:5C:7E]  0.638ms
Unicast reply from 10.10.128.92 [2E:AF:19:DB:5C:7E]  0.594ms
^CSent 2 probes (1 broadcast(s))
Received 2 response(s)

$ ping 10.10.128.92
PING 10.10.128.92 (10.10.128.92) 56(84) bytes of data.
```

但是，如果使用 arping 10.11.128.92 ，发现的确是没有
最后用这个部署也是没问题的。

https://github.com/webrtc-rs/webrtc

## webrtc 是做什么的
- https://github.com/webrtc-rs/webrtc

## 这个还不错
https://notes.shichao.io/tcpv1/ch7/

## 整理一下
https://liujunming.top/2024/08/18/network-USO-vs-UFO/

https://news.ycombinator.com/item?id=42316091

https://news.ycombinator.com/item?id=41596818

## udp client 连 server , client 端口到底是谁分配的

client 可以绑定 ip ，也可以不绑定，port 也是如此哦

https://stackoverflow.com/questions/4118241/what-client-side-situations-need-bind


## 这个设备是那个模块，为什么被默认加载了
```txt
2: sit0@NONE: <NOARP> mtu 1480 qdisc noop state DOWN group default qlen 1000
    link/sit 0.0.0.0 brd 0.0.0.0
```

## 看看这个
https://github.com/retis-org/retis

https://info.support.huawei.com/info-finder/encyclopedia/zh/LACP.html

https://github.com/hengyoush/kyanos

## dmabuf
https://lwn.net/Articles/979549/

## nat
- https://news.ycombinator.com/item?id=42600846

https://docs.kernel.org/networking/statistics.html

## 如果两个 bridge 用一个网段会如何?

## bbr 拥塞算法
https://www.zhihu.com/question/53559433/answer/2375321369

## net todo
网络的 bonding 模式测试下，例如如下的命令:
```txt
ovs-ofctl show ovsbr-frl1l182k
ovs-appctl lacp/show
ovs-appctl fdb/show ovsbr-frl1l182k
```

## virtio 网卡有这个吗?
```txt
ethtool -K <网卡名称> rxvlan off
ethtool -K <网卡名称> txvlan off
```

## 测试一下 bridge 性能和 perf 的结果
## 这个是做什么的
drivers/net/net_failover.c

和

https://qemu.readthedocs.io/en/v9.1.0/system/virtio-net-failover.html

什么关系?

## 看看
https://www.ebpf.top/post/ebpf_network_kpatch_ipvs/

## bridge 的模式
```txt
🧀  bridge
Usage: bridge [ OPTIONS ] OBJECT { COMMAND | help }
       bridge [ -force ] -batch filename
where  OBJECT := { link | fdb | mdb | vlan | monitor }
       OPTIONS := { -V[ersion] | -s[tatistics] | -d[etails] |
                    -o[neline] | -t[imestamp] | -n[etns] name |
                    -c[ompressvlans] -color -p[retty] -j[son] }
```

## 为什么 kernel 下面有这么多

packetdrill 的测试，packetdrill 不是一个外部项目吗?

tools/testing/selftests/net/packetdrill/tcp_tcp_info_tcp-info-rwnd-limited.pkt

## device memory TCP
https://docs.kernel.org/networking/devmem.html

## 这里的几个模块都是做什么的
```txt
kernel/net/ipv4/ip_tunnel.ko
kernel/net/ipv4/tunnel4.ko
kernel/net/ipv4/tcp_cubic.ko
kernel/net/ipv4/tcp_sigpool.ko
kernel/net/xfrm/xfrm_algo.ko
kernel/net/xfrm/xfrm_user.ko
kernel/net/ipv6/ipv6.ko
kernel/net/ipv6/ah6.ko
kernel/net/ipv6/esp6.ko
kernel/net/ipv6/sit.ko
```

## 为什么 dns 在内核还有一个模块
CONFIG_DNS_RESOLVER

## af_packet 的作用是什么?

如果没有，slirp 网卡为什么没有 ip 啊

## 谁在加载这个?
```txt
[    4.346892] failed to validate module [iptable_filter] BTF: -22
[    4.358729] BPF: [115033] Invalid name_offset:2131947
[    4.359224] failed to validate module [iptable_filter] BTF: -22
[    4.372991] BPF: [115033] Invalid name_offset:2131955
[    4.373413] failed to validate module [ip6table_filter] BTF: -22
[    4.382646] BPF: [115033] Invalid name_offset:2131955
[    4.383207] failed to validate module [ip6table_filter] BTF: -22
[    4.455786] BPF: Invalid name
[    4.456380] failed to validate module [rfkill] BTF: -22
[    4.469356] BPF: Invalid name
[    4.469796] failed to validate module [rfkill] BTF: -22
[    4.616394] BPF: Invalid name
[    4.616797] failed to validate module [iptable_mangle] BTF: -22
[    4.629463] BPF: Invalid name
[    4.629875] failed to validate module [iptable_mangle] BTF: -22
[    4.644580] BPF: [115033] Invalid name_offset:2131947
[    4.645098] failed to validate module [iptable_filter] BTF: -22
[    4.655674] BPF: [115033] Invalid name_offset:2131947
[    4.656180] failed to validate module [iptable_filter] BTF: -22
[    4.674064] BPF: [115032] Invalid name_offset:2131937
[    4.674486] failed to validate module [ip6table_mangle] BTF: -22
[    4.685661] BPF: [115032] Invalid name_offset:2131937
[    4.686182] failed to validate module [ip6table_mangle] BTF: -22
[    4.700854] BPF: [115033] Invalid name_offset:2131955
[    4.701349] failed to validate module [ip6table_filter] BTF: -22
[    4.711642] BPF: [115033] Invalid name_offset:2131955
[    4.712163] failed to validate module [ip6table_filter] BTF: -22
[    4.784337] BPF: Invalid name
[    4.784751] failed to validate module [iptable_mangle] BTF: -22
[    4.797556] BPF: Invalid name
[    4.798050] failed to validate module [iptable_mangle] BTF: -22
[    4.810552] BPF: [115033] Invalid name_offset:2131947
[    4.811032] failed to validate module [iptable_filter] BTF: -22
[    4.821642] BPF: [115033] Invalid name_offset:2131947
[    4.822142] failed to validate module [iptable_filter] BTF: -22
[    4.839628] BPF: [115032] Invalid name_offset:2131937
[    4.840137] failed to validate module [ip6table_mangle] BTF: -22
[    4.850643] BPF: [115032] Invalid name_offset:2131937
[    4.851130] failed to validate module [ip6table_mangle] BTF: -22
[    4.866496] BPF: [115033] Invalid name_offset:2131955
[    4.866942] failed to validate module [ip6table_filter] BTF: -22
[    4.877633] BPF: [115033] Invalid name_offset:2131955
[    4.878147] failed to validate module [ip6table_filter] BTF: -22
[    5.427338] BPF: Invalid name
[    5.427745] failed to validate module [rpcsec_gss_krb5] BTF: -22
```

准确来说，是这里的:
```txt
iptable_filter
ip6table_filter
iptable_mangle
ip6table_mangle
rpcsec_gss_krb5
rfkill
xt_addrtype
xt_mark
llc
stp
bridge
xt_MASQUERADE
```

## 这两个是做什么的
```txt
CONFIG_FAILOVER=m
CONFIG_NET_FAILOVER=m
```
## 网卡的队列

可以给 alpine 虚拟机网卡多配置几个队列
现在都是一个队列:
```txt
🧀   ls /sys/class/net/*/queues
/sys/class/net/enp0s2/queues:
 rx-0   tx-0

/sys/class/net/enp0s3/queues:
 rx-0   tx-0

/sys/class/net/lo/queues:
 rx-0   tx-0

/sys/class/net/vhost/queues:
 rx-0   tx-0
```

这是物理机的的队列的数量的:

```txt
/sys/class/net/enp125s0f2/queues:
 rx-0   tx-0

/sys/class/net/enp125s0f3/queues:
 rx-0   tx-0

/sys/class/net/enp130s0f0np0/queues:
 rx-0    rx-14   rx-28   rx-42   rx-56   rx-70   rx-84   rx-98    rx-112   tx-0    tx-14   tx-28   tx-42   tx-56
 rx-1    rx-15   rx-29   rx-43   rx-57   rx-71   rx-85   rx-99    rx-113   tx-1    tx-15   tx-29   tx-43   tx-57
 rx-2    rx-16   rx-30   rx-44   rx-58   rx-72   rx-86   rx-100   rx-114   tx-2    tx-16   tx-30   tx-44   tx-58
 rx-3    rx-17   rx-31   rx-45   rx-59   rx-73   rx-87   rx-101   rx-115   tx-3    tx-17   tx-31   tx-45   tx-59
 rx-4    rx-18   rx-32   rx-46   rx-60   rx-74   rx-88   rx-102   rx-116   tx-4    tx-18   tx-32   tx-46   tx-60
 rx-5    rx-19   rx-33   rx-47   rx-61   rx-75   rx-89   rx-103   rx-117   tx-5    tx-19   tx-33   tx-47   tx-61
 rx-6    rx-20   rx-34   rx-48   rx-62   rx-76   rx-90   rx-104   rx-118   tx-6    tx-20   tx-34   tx-48   tx-62
 rx-7    rx-21   rx-35   rx-49   rx-63   rx-77   rx-91   rx-105   rx-119   tx-7    tx-21   tx-35   tx-49
 rx-8    rx-22   rx-36   rx-50   rx-64   rx-78   rx-92   rx-106   rx-120   tx-8    tx-22   tx-36   tx-50
 rx-9    rx-23   rx-37   rx-51   rx-65   rx-79   rx-93   rx-107   rx-121   tx-9    tx-23   tx-37   tx-51
 rx-10   rx-24   rx-38   rx-52   rx-66   rx-80   rx-94   rx-108   rx-122   tx-10   tx-24   tx-38   tx-52
 rx-11   rx-25   rx-39   rx-53   rx-67   rx-81   rx-95   rx-109   rx-123   tx-11   tx-25   tx-39   tx-53
 rx-12   rx-26   rx-40   rx-54   rx-68   rx-82   rx-96   rx-110   rx-124   tx-12   tx-26   tx-40   tx-54
 rx-13   rx-27   rx-41   rx-55   rx-69   rx-83   rx-97   rx-111   rx-125   tx-13   tx-27   tx-41   tx-55

/sys/class/net/enp130s0f1np1/queues:
 rx-0    rx-14   rx-28   rx-42   rx-56   rx-70   rx-84   rx-98    rx-112   tx-0    tx-14   tx-28   tx-42   tx-56
 rx-1    rx-15   rx-29   rx-43   rx-57   rx-71   rx-85   rx-99    rx-113   tx-1    tx-15   tx-29   tx-43   tx-57
 rx-2    rx-16   rx-30   rx-44   rx-58   rx-72   rx-86   rx-100   rx-114   tx-2    tx-16   tx-30   tx-44   tx-58
 rx-3    rx-17   rx-31   rx-45   rx-59   rx-73   rx-87   rx-101   rx-115   tx-3    tx-17   tx-31   tx-45   tx-59
 rx-4    rx-18   rx-32   rx-46   rx-60   rx-74   rx-88   rx-102   rx-116   tx-4    tx-18   tx-32   tx-46   tx-60
 rx-5    rx-19   rx-33   rx-47   rx-61   rx-75   rx-89   rx-103   rx-117   tx-5    tx-19   tx-33   tx-47   tx-61
 rx-6    rx-20   rx-34   rx-48   rx-62   rx-76   rx-90   rx-104   rx-118   tx-6    tx-20   tx-34   tx-48   tx-62
 rx-7    rx-21   rx-35   rx-49   rx-63   rx-77   rx-91   rx-105   rx-119   tx-7    tx-21   tx-35   tx-49
 rx-8    rx-22   rx-36   rx-50   rx-64   rx-78   rx-92   rx-106   rx-120   tx-8    tx-22   tx-36   tx-50
 rx-9    rx-23   rx-37   rx-51   rx-65   rx-79   rx-93   rx-107   rx-121   tx-9    tx-23   tx-37   tx-51
 rx-10   rx-24   rx-38   rx-52   rx-66   rx-80   rx-94   rx-108   rx-122   tx-10   tx-24   tx-38   tx-52
 rx-11   rx-25   rx-39   rx-53   rx-67   rx-81   rx-95   rx-109   rx-123   tx-11   tx-25   tx-39   tx-53
 rx-12   rx-26   rx-40   rx-54   rx-68   rx-82   rx-96   rx-110   rx-124   tx-12   tx-26   tx-40   tx-54
 rx-13   rx-27   rx-41   rx-55   rx-69   rx-83   rx-97   rx-111   rx-125   tx-13   tx-27   tx-41   tx-55
```

### 对比一下 网卡的队列和磁盘的队列，似乎有一些相似性，但是两个东西也是差别很大



## BQL 是做什么的
```txt
config BQL
	bool
	prompt "Enable Byte Queue Limits"
	depends on SYSFS
	select DQL
	default y
```
这里的 select 的 CONFIG_DQL 其实就是控制 lib/dynamic_queue_limits.c 是否变异进去。

这里的一段介绍:
- https://github.com/ffainelli/bqlmon

> Accurate reporting of how many buffers are in-flight allows for a better sizing of all software queueing.
>
> Using BQL also allows highlighting potential transmit queueing and flow control issues. For instance, if the transmitted buffers are reclaimed in an interrupt context, but that interrupt never fires, we will quickly queue up many buffers that are not freed.
>
> BQL exposes a few sysfs attributes in /sys/class/net/interface/queues/tx-N/byte_queue_limits/ which are per-queue information about:
>
> in-flight packets limit threshold until the queue is declared congested hold time

简单来说，就是提供一个报告一个当前有多少 packets 正在发送的。

- https://lwn.net/Articles/454378/

进阶内容:
https://lore.kernel.org/lkml/Zcz45KZj9JxwjGtR@gmail.com/t/#m93c03b27ccdf3c13598fc6c3ef3bbb6612e614a4


virtio-net 会包含这个吗? lib/dynamic_queue_limits.c

## 测试一下 offload 的效果
1. 例如网卡中的 offload 中的内容，打开之后和不打开之后，一些代码的路径都是如何走的?
2. 如下内容是一个 virtio-net 网卡的，那么如何理解其中的网卡的使用结果?

```txt
🧀  ethtool -k vhost
Features for vhost:
rx-checksumming: on [fixed]
tx-checksumming: on
        tx-checksum-ipv4: off [fixed]
        tx-checksum-ip-generic: on
        tx-checksum-ipv6: off [fixed]
        tx-checksum-fcoe-crc: off [fixed]
        tx-checksum-sctp: off [fixed]
scatter-gather: on
        tx-scatter-gather: on
        tx-scatter-gather-fraglist: off [fixed]
tcp-segmentation-offload: on
        tx-tcp-segmentation: on
        tx-tcp-ecn-segmentation: on
        tx-tcp-mangleid-segmentation: off
        tx-tcp6-segmentation: on
generic-segmentation-offload: on
generic-receive-offload: on
large-receive-offload: off [fixed]
rx-vlan-offload: off [fixed]
tx-vlan-offload: off [fixed]
ntuple-filters: off [fixed]
receive-hashing: off [fixed]
highdma: on [fixed]
rx-vlan-filter: on [fixed]
vlan-challenged: off [fixed]
tx-lockless: off [fixed]
netns-local: off [fixed]
tx-gso-robust: on [fixed]
tx-fcoe-segmentation: off [fixed]
tx-gre-segmentation: off [fixed]
tx-gre-csum-segmentation: off [fixed]
tx-ipxip4-segmentation: off [fixed]
tx-ipxip6-segmentation: off [fixed]
tx-udp_tnl-segmentation: off [fixed]
tx-udp_tnl-csum-segmentation: off [fixed]
tx-gso-partial: off [fixed]
tx-tunnel-remcsum-segmentation: off [fixed]
tx-sctp-segmentation: off [fixed]
tx-esp-segmentation: off [fixed]
tx-udp-segmentation: off [fixed]
tx-gso-list: off [fixed]
fcoe-mtu: off [fixed]
tx-nocache-copy: off
loopback: off [fixed]
rx-fcs: off [fixed]
rx-all: off [fixed]
tx-vlan-stag-hw-insert: off [fixed]
rx-vlan-stag-hw-parse: off [fixed]
rx-vlan-stag-filter: off [fixed]
l2-fwd-offload: off [fixed]
hw-tc-offload: off [fixed]
esp-hw-offload: off [fixed]
esp-tx-csum-hw-offload: off [fixed]
rx-udp_tunnel-port-offload: off [fixed]
tls-hw-tx-offload: off [fixed]
tls-hw-rx-offload: off [fixed]
rx-gro-hw: on
tls-hw-record: off [fixed]
rx-gro-list: off
macsec-hw-offload: off [fixed]
rx-udp-gro-forwarding: off
hsr-tag-ins-offload: off [fixed]
hsr-tag-rm-offload: off [fixed]
hsr-fwd-offload: off [fixed]
hsr-dup-offload: off [fixed]
```

## [ ] net/core/rtnetlink.c 是做什么的?
似乎就是 netlink 的实现

但是存在 net/netlink/ 的文件夹

## 这两个文件是做什么的?

### [ ] net/core/flow_dissector

### net/core/link_watch.c

## 整理一下用户态工具的使用
https://mp.weixin.qq.com/s/uGkeKByzB3_xpV2RLR5pvA

## 有用吗?
https://github.com/heiher/natmap

## ipv6
- https://news.ycombinator.com/item?id=43069533
## https://news.ycombinator.com/item?id=43096477


## 其中的 ssh 需要使用下
net/ssh.md

## 这个和 pxe 有什么关系?
CONFIG_IP_PNP=y
CONFIG_IP_PNP_DHCP=y

n100 上还有希望用网络启动吗?



## 有趣的问题
https://github.com/aceberg/WatchYourLAN


## firecracker 中的 docs/getting-started.md

这里没有使用 ovs 或者 linux bridge ，但是一样可以上网，如何理解?

## 网络的调用 stack 非常的有趣
- ret_from_fork
  - kthread
    - smpboot_thread_fn
      - run_ksoftirqd
        - run_ksoftirqd
          - __do_softirq
            - net_rx_action
              - napi_poll
                - __napi_poll
                  - e1000e_poll
                    - napi_complete_done
                      - gro_normal_list
                        - gro_normal_list
                          - netif_receive_skb_list_internal
                            - __netif_receive_skb_list
                              - __netif_receive_skb_list_core
                                - __netif_receive_skb_core
                                  - deliver_ptype_list_skb
                                    - deliver_skb
                                      - ip_local_deliver_finish
                                        - ip_protocol_deliver_rcu
                                          - tcp_v4_rcv
                                            - tcp_v4_do_rcv
                                              - tcp_rcv_established
                                                - tcp_data_queue
                                                  - sock_def_readable


就像是协程一样，从 irq 到 napi poll ，revice 之后，立刻发送，最后wakeup

## 学习
- https://github.com/everoute/troubleshooting-tools

## 可以从 ovs 的 vport 理解一些基本问题

这些正好就是 ovs 可以接入的后端:

- ./vport.c
- ./vport-internal_dev.c
- ./vport-netdev.c
- ./vport-vxlan.c
- ./vport-geneve.c
- ./vport-gre.c

正好是对应这个结构体:
```c
enum ovs_vport_type {
	OVS_VPORT_TYPE_UNSPEC,
	OVS_VPORT_TYPE_NETDEV,   /* network device */
	OVS_VPORT_TYPE_INTERNAL, /* network device implemented by datapath */
	OVS_VPORT_TYPE_GRE,      /* GRE tunnel. */
	OVS_VPORT_TYPE_VXLAN,	 /* VXLAN tunnel. */
	OVS_VPORT_TYPE_GENEVE,	 /* Geneve tunnel. */
	__OVS_VPORT_TYPE_MAX
};
```

```txt
@[
    ovs_vport_send+5
    do_execute_actions+6566
    ovs_execute_actions+76
    ovs_dp_process_packet+172
    ovs_vport_receive+132
    internal_dev_xmit+45
    dev_hard_start_xmit+97
    __dev_queue_xmit+577
    ip_finish_output2+636
    __ip_queue_xmit+392
    __tcp_transmit_skb+2766
    tcp_write_xmit+1609
    __tcp_push_pending_frames+54
    tcp_sendmsg_locked+2973
    tcp_sendmsg+43
    sock_write_iter+381
    vfs_write+988
    ksys_write+210
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 145
```

```txt
@[
    internal_dev_recv+5
    do_execute_actions+6566
    ovs_execute_actions+76
    ovs_dp_process_packet+172
    ovs_vport_receive+132
    netdev_frame_hook+222
    __netif_receive_skb_core.constprop.0+1219
    __netif_receive_skb_list_core+313
    netif_receive_skb_list_internal+514
    napi_complete_done+110
    rtl8169_poll+1183
    __napi_poll.constprop.0+40
    net_rx_action+873
    handle_softirqs+272
    __irq_exit_rcu+245
    irq_exit_rcu+14
    common_interrupt+135
    asm_common_interrupt+38
    cpuidle_enter_state+220
    cpuidle_enter+45
    do_idle+459
    cpu_startup_entry+41
    start_secondary+291
    common_startup_64+318
]: 235
```

## 调查一下 openfortivpn

openfortivpn 被开启之后，可以看到:

可以看到:
```txt
38: ppp0: <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP> mtu 1354 qdisc fq_codel state UNKNOWN group default qlen 3
    link/ppp
    inet 111.222.333.444 peer 121.xx.18.xxx/32 scope global ppp0
       valid_lft forever preferred_lft forever
```

类似的，可以观察到内核日志
```txt
[ 7633.044896] PPP generic driver version 2.4.2
[ 7633.072687] PPP BSD Compression module registered
[ 7633.073732] PPP Deflate Compression module registered
```

## 看看其中的 tracepoint
https://docs.google.com/document/d/1mrIcOqiXOd3zKxlpSo25ex-rw5TqGB_Z/edit#heading=h.qh9cphafwqca

## 看看这个东西
https://github.com/hengyoush/kyanos

## 这个做什么的
https://news.ycombinator.com/item?id=43933683

## 看看
https://github.com/mozillazg/ptcpdump

## udp tunnel
```c
struct udp_tunnel_sock_cfg {
	void *sk_user_data;     /* user data used by encap_rcv call back */
	/* Used for setting up udp_sock fields, see udp.h for details */
	__u8  encap_type;
	udp_tunnel_encap_rcv_t encap_rcv;
	udp_tunnel_encap_err_lookup_t encap_err_lookup;
	udp_tunnel_encap_err_rcv_t encap_err_rcv;
	udp_tunnel_encap_destroy_t encap_destroy;
	udp_tunnel_gro_receive_t gro_receive;
	udp_tunnel_gro_complete_t gro_complete;
};
```

rg "\.encap_rcv =" 可以看到好多，这个是发一个很通用的机制了:

```txt
net/tipc/udp_media.c
775:    tuncfg.encap_rcv = tipc_udp_recv;

net/ipv4/fou_core.c
607:            tunnel_cfg.encap_rcv = fou_udp_recv;
613:            tunnel_cfg.encap_rcv = gue_udp_recv;

net/sctp/protocol.c
899:    tuncfg.encap_rcv = sctp_udp_rcv;
921:    tuncfg.encap_rcv = sctp_udp_rcv;

net/l2tp/l2tp_core.c
1674:                   .encap_rcv = l2tp_udp_encap_recv,

net/rxrpc/local_object.c
194:    tuncfg.encap_rcv = rxrpc_encap_rcv;

drivers/infiniband/sw/rxe/rxe_net.c
197:    tnl_cfg.encap_rcv = rxe_udp_encap_recv;

drivers/net/bareudp.c
268:    tunnel_cfg.encap_rcv = bareudp_udp_encap_recv;

drivers/net/amt.c
2974:   tunnel_cfg.encap_rcv = amt_rcv;

drivers/net/pfcp.c
172:    tuncfg.encap_rcv = pfcp_encap_recv;

drivers/net/gtp.c
1431:   tuncfg.encap_rcv = gtp_encap_recv;
1684:   tuncfg.encap_rcv = gtp_encap_recv;

drivers/net/geneve.c
617:    tunnel_cfg.encap_rcv = geneve_udp_encap_recv;

drivers/net/wireguard/socket.c
356:            .encap_rcv = wg_receive

drivers/net/vxlan/vxlan_core.c
3607:   tunnel_cfg.encap_rcv = vxlan_rcv;
```


## 有趣，也算是 everything is a file 了
https://www.anmolsarma.in/post/bash-net-redirections/


## vpc
https://www.ducktyped.org/p/why-is-it-called-a-cloud-if-its-not

## 过两年再来看看这个工具又没用?
https://github.com/theopfr/somo

## 这个工具看看
https://github.com/EHfive/einat-ebpf


## socket 的基本编程理解

1. 如果 client 连续调用两个 write ，server 哪端一定需要两个 read 吗?


## 什么叫做 http proxy ?
https://github.com/johan-steffens/foxy

## 真的 nb 啊
https://github.com/EasyTier/EasyTier
https://easytier.cn/guide/gui/easytier-game.html
相当易用了

## 打开 sfc 之后，发现其依赖的两个内容
CONFIG_NET_DEVLINK=y
CONFIG_MDIO=m

## websocket 是做什么的?
https://github.com/law-chain-hot/websocket-devtools


## net todo : 这个人的都可以看看了
Linux Kernel TCP 终于移除了 RFC6675 - anonymous的文章 - 知乎
https://zhuanlan.zhihu.com/p/1937411424373682610

TCP RTO 与丢包检测 - anonymous的文章 - 知乎
https://zhuanlan.zhihu.com/p/1934744674821715021

epoll 陷阱：隧道中的高级负担 - anonymous的文章 - 知乎
https://zhuanlan.zhihu.com/p/1942356872591054402

https://www.ducktyped.org/p/an-illustrated-guide-to-oauth


## 有趣的 eswitch

```txt
CONFIG_MLX5_CORE=m
CONFIG_MLX5_CORE_EN=y
# 用于支持相同 pf 切分出来的 vf 不经过交换机直接通信
CONFIG_NET_SWITCHDEV=y
CONFIG_MLX5_ESWITCH=y
```

## alpine.sh 为什么有时候 clone 虚拟机的时候，会出现 ipv6 地址的冲突

```txt
localhost login: IPv6: ens4: IPv6 duplicate address fe80::3ee6:afc3:abb:da9a used by 52:54:00:00:02:0d detected!
IPv6: ens4: IPv6 duplicate address fe80::3ee6:afc3:abb:da9a used by 52:54:00:00:02:0d detected!
IPv6: ens4: IPv6 duplicate address fe80::3ee6:afc3:abb:da9a used by 52:54:00:00:02:0d detected!
IPv6: ens4: IPv6 duplicate address fe80::3ee6:afc3:abb:da9a used by 52:54:00:00:02:0d detected!
IPv6: ens4: IPv6 duplicate address fe80::3ee6:afc3:abb:da9a used by 52:54:00:00:02:0d detected!
IPv6: ens4: IPv6 duplicate address fe80::3ee6:afc3:abb:da9a used by 52:54:00:00:02:0d detected!
```
复现方法是，给一个虚拟机先通过 nmcli 配置一个静态 ip ，
然后就继续执行了

不清楚 v4 的原理是什么，为什么会导致产生一样的 v6 的地址。

## 什么叫做分享本机的 TCP 端口?

一个命令行工具，将本机的 TCP 端口分享出去。
https://malai.sh/hello-tcp/

## 这个该如何使用
https://github.com/ntop/ntopng

## 有时候，可以看到 169.254 的 ip
一般是 ovs 中的 port-internal

这是做什么的

## 网络问题解决
https://github.com/adrienverge/openfortivpn
https://github.com/EasyTier/EasyTier/blob/main/README_CN.md

## ruanyf 的周刊中偶尔遇到的，可以继续观察下
https://github.com/domcyrus/rustnet
https://github.com/karam-ajaj/atlas

## 工具
https://github.com/rathole-org/rathole

## 理解一下这个命令的含义

sudo iptables --wait -t nat -A PREROUTING -m addrtype --dst-type LOCAL -j DOCKER

## 很高级了
https://blog.cloudflare.com/so-long-and-thanks-for-all-the-fish-how-to-escape-the-linux-networking-stack/


## 这两个区别是什么?

为什么最后都是使用的 unix_stream_ops
```c
static const struct proto_ops unix_stream_ops = {
	.family =	PF_UNIX,
	.owner =	THIS_MODULE,
	.release =	unix_release,
	.bind =		unix_bind,
	.connect =	unix_stream_connect,
	.socketpair =	unix_socketpair,
	.accept =	unix_accept,
	.getname =	unix_getname,
	.poll =		unix_poll,
	.ioctl =	unix_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =	unix_compat_ioctl,
#endif
	.listen =	unix_listen,
	.shutdown =	unix_shutdown,
	.setsockopt =	unix_setsockopt,
	.sendmsg =	unix_stream_sendmsg,
	.recvmsg =	unix_stream_recvmsg,
	.read_skb =	unix_stream_read_skb,
	.mmap =		sock_no_mmap,
	.splice_read =	unix_stream_splice_read,
	.set_peek_off =	sk_set_peek_off,
	.show_fdinfo =	unix_show_fdinfo,
};

static const struct proto_ops unix_dgram_ops = {
	.family =	PF_UNIX,
	.owner =	THIS_MODULE,
	.release =	unix_release,
	.bind =		unix_bind,
	.connect =	unix_dgram_connect,
	.socketpair =	unix_socketpair,
	.accept =	sock_no_accept,
	.getname =	unix_getname,
	.poll =		unix_dgram_poll,
	.ioctl =	unix_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =	unix_compat_ioctl,
#endif
	.listen =	sock_no_listen,
	.shutdown =	unix_shutdown,
	.sendmsg =	unix_dgram_sendmsg,
	.read_skb =	unix_read_skb,
	.recvmsg =	unix_dgram_recvmsg,
	.mmap =		sock_no_mmap,
	.set_peek_off =	sk_set_peek_off,
	.show_fdinfo =	unix_show_fdinfo,
};
```

## 这个工具好哇
https://github.com/lance0/ttl

## 这个计数实在是太强了吧
```txt
function tcp_probe() {
	local ip=$1
	local port=$2

	timeout 5 bash -c "cat < /dev/null > /dev/tcp/${ip}/${port}"
}
```

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
