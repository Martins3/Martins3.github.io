## 切分测试

### 物理机中配置
```txt
{8:59}~ ➭ lspci  | grep Eth
81:00.0 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
81:00.1 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
82:00.0 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
82:00.1 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]

{8:59}~ ➭ cat /sys/devices/pci0000:80/0000:80:00.0/0000:81:00.1/sriov_numvfs
0

{9:00}~ ➭ echo 8 >  /sys/devices/pci0000:80/0000:80:00.0/0000:81:00.1/sriov_numvfs

{9:00}~ ➭ lspci | grep Eth

81:00.0 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
81:00.1 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
81:01.2 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
81:01.3 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
81:01.4 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
81:01.5 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
81:01.6 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
81:01.7 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
81:02.0 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
81:02.1 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]

82:00.0 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
82:00.1 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
```

```txt
{9:03}~ ➭ ls -la /sys/class/net
total 0
drwxr-xr-x  2 root root 0 Jun 13 13:43 .
drwxr-xr-x 72 root root 0 Jun 13 13:43 ..
lrwxrwxrwx  1 root root 0 Jun 13 13:44 enp129s0f0np0 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:00.0/net/enp129s0f0np0
lrwxrwxrwx  1 root root 0 Jun 13 13:44 enp129s0f1np1 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:00.1/net/enp129s0f1np1
lrwxrwxrwx  1 root root 0 Jun 14 09:00 enp129s0f1v0 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:01.2/net/enp129s0f1v0
lrwxrwxrwx  1 root root 0 Jun 14 09:00 enp129s0f1v1 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:01.3/net/enp129s0f1v1
lrwxrwxrwx  1 root root 0 Jun 14 09:00 enp129s0f1v2 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:01.4/net/enp129s0f1v2
lrwxrwxrwx  1 root root 0 Jun 14 09:00 enp129s0f1v3 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:01.5/net/enp129s0f1v3
lrwxrwxrwx  1 root root 0 Jun 14 09:00 enp129s0f1v4 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:01.6/net/enp129s0f1v4
lrwxrwxrwx  1 root root 0 Jun 14 09:00 enp129s0f1v5 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:01.7/net/enp129s0f1v5
lrwxrwxrwx  1 root root 0 Jun 14 09:00 enp129s0f1v6 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:02.0/net/enp129s0f1v6
lrwxrwxrwx  1 root root 0 Jun 14 09:00 enp129s0f1v7 -> ../../devices/pci0000:80/0000:80:00.0/0000:81:02.1/net/enp129s0f1v7
lrwxrwxrwx  1 root root 0 Jun 13 13:44 enp130s0f0np0 -> ../../devices/pci0000:80/0000:80:04.0/0000:82:00.0/net/enp130s0f0np0
lrwxrwxrwx  1 root root 0 Jun 13 13:44 enp130s0f1np1 -> ../../devices/pci0000:80/0000:80:04.0/0000:82:00.1/net/enp130s0f1np1
lrwxrwxrwx  1 root root 0 Jun 13 13:43 lo -> ../../devices/virtual/net/lo
```

当切分出来新的 vfio 之后， 会创建新的 iommu group 出来

```txt
/sys/kernel/iommu_groups/43/devices:
total 0
drwxr-xr-x 2 root root 0 Jun 14 09:19 .
drwxr-xr-x 3 root root 0 Jun 14 09:19 ..
lrwxrwxrwx 1 root root 0 Jun 14 09:19 0000:81:01.4 -> ../../../../devices/pci0000:80/0000:80:00.0/0000:81:01.4

/sys/kernel/iommu_groups/44/devices:
total 0
drwxr-xr-x 2 root root 0 Jun 14 09:19 .
drwxr-xr-x 3 root root 0 Jun 14 09:19 ..
lrwxrwxrwx 1 root root 0 Jun 14 09:19 0000:81:01.5 -> ../../../../devices/pci0000:80/0000:80:00.0/0000:81:01.5
```

### 虚拟机中的观察
```txt
 ethtool -i enp0s7

driver: mlx5_core
version: 6.16.3
firmware-version: 16.31.1014 (HUA0000000024)
expansion-rom-version:
bus-info: 0000:00:07.0
supports-statistics: yes
supports-test: yes
supports-eeprom-access: no
supports-register-dump: no
supports-priv-flags: yes
```

虚拟机中也是可以识别到这个网卡是一个虚拟网卡:
```txt
 lspci | grep Eth
00:03.0 Ethernet controller: Red Hat, Inc. Virtio network device
00:04.0 Ethernet controller: Red Hat, Inc. Virtio network device
00:05.0 Ethernet controller: Red Hat, Inc. Virtio network device
00:07.0 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
```

### 给 pf 配置 ip

sudo ip addr add 10.1.2.0/16 dev enp130s0f1np1

在虚拟机中 vf 和 pf 可以互相 ping 通


### 通过 pf 直接配置 vf 的各种属性
<!-- 8672844a-9b6a-480c-ac78-811ff1255168 -->

sudo ip link set enp130s0f1np1 vf 0 min_tx_rate 1000
sudo ip link set enp130s0f1np1 vf 0 max_tx_rate 5000

sudo ip link set enp130s0f1np1 vf 0 vlan 100
sudo ip link set enp130s0f0np0 vf 0 vlan 100

```txt
ip link show enp130s0f1np1
3: enp130s0f1np1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP mode DEFAULT group default qlen 1000
    link/ether 8c:2a:8e:40:a3:7d brd ff:ff:ff:ff:ff:ff
    vf 0     link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff, tx rate 5000 (Mbps), max_tx_rate 5000Mbps, min_tx_rate 1000Mbps, spoof checking off, link-state auto, trust off, query_rss off
    vf 1     link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff, spoof checking off, link-state auto, trust off, query_rss off
    vf 2     link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff, spoof checking off, link-state auto, trust off, query_rss off
    vf 3     link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff, spoof checking off, link-state auto, trust off, query_rss off
    vf 4     link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff, spoof checking off, link-state auto, trust off, query_rss off
    vf 5     link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff, spoof checking off, link-state auto, trust off, query_rss off
    vf 6     link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff, spoof checking off, link-state auto, trust off, query_rss off
    vf 7     link/ether 00:00:00:00:00:00 brd ff:ff:ff:ff:ff:ff, spoof checking off, link-state auto, trust off, query_rss off
```

ip link help 中可以看到这些内容，和上面展示的东西基本是一致的
```txt
                [ alias NAME ]
                [ vf NUM [ mac LLADDR ]
                         [ vlan VLANID [ qos VLAN-QOS ] [ proto VLAN-PROTO ] ]
                         [ rate TXRATE ]
                         [ max_tx_rate TXRATE ]
                         [ min_tx_rate TXRATE ]
                         [ spoofchk { on | off} ]
                         [ query_rss { on | off} ]
                         [ state { auto | enable | disable} ]
                         [ trust { on | off} ]
                         [ node_guid EUI64 ]
                         [ port_guid EUI64 ] ]
```

```txt
@[
        mlx5e_set_vf_vlan+0
        do_setlink.isra.0+1128
        __rtnl_newlink+600
        rtnl_newlink+664
        rtnetlink_rcv_msg+496
        netlink_rcv_skb+104
        rtnetlink_rcv+32
        netlink_unicast+784
        netlink_sendmsg+432
        __sock_sendmsg+100
        ____sys_sendmsg+576
        ___sys_sendmsg+140
        __sys_sendmsg+140
        __arm64_sys_sendmsg+44
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 1
```

### 虚拟机中的基本配置
sudo nmcli c add type ethernet ifname enp0s7 con-name 7 ip4 10.1.2.19/16
sudo nmcli c add type ethernet ifname enp0s7 con-name 7 ip4 10.1.2.30/16
sudo nmcli c add type ethernet ifname enp125s0f0 con-name X ip4 192.168.19.60/20

### 使用同一个 pf 的两个 vf 做 iperf3 测试

没有任何特殊操作:
```txt
[  5] 358.00-359.00 sec   811 MBytes  6.80 Gbits/sec   25    757 KBytes
[  5] 359.00-360.00 sec   902 MBytes  7.57 Gbits/sec   56    758 KBytes
[  5] 360.00-361.00 sec   954 MBytes  8.00 Gbits/sec   43    834 KBytes
[  5] 361.00-362.00 sec   954 MBytes  8.01 Gbits/sec   56    860 KBytes
[  5] 362.00-363.00 sec   914 MBytes  7.67 Gbits/sec   53    831 KBytes
[  5] 363.00-364.00 sec   897 MBytes  7.52 Gbits/sec   21    660 KBytes
[  5] 364.00-365.00 sec   858 MBytes  7.20 Gbits/sec   38    782 KBytes
```

作为对比 virtio-net 的性能:
```txt
iperf3 -c 10.0.30.0 -t 0
Connecting to host 10.0.30.0, port 5201
[  5] local 10.0.19.0 port 35206 connected to 10.0.30.0 port 5201
[ ID] Interval           Transfer     Bitrate         Retr  Cwnd
[  5]   0.00-1.00   sec  1.31 GBytes  11.3 Gbits/sec    0   3.57 MBytes
[  5]   1.00-2.00   sec  1.28 GBytes  11.0 Gbits/sec    0   3.77 MBytes
[  5]   2.00-3.00   sec  1.39 GBytes  12.0 Gbits/sec    0   3.77 MBytes
[  5]   3.00-4.00   sec  1.05 GBytes  9.04 Gbits/sec    0   3.96 MBytes
[  5]   4.00-5.00   sec  1.76 GBytes  15.1 Gbits/sec    0   3.96 MBytes
[  5]   5.00-6.00   sec  1.55 GBytes  13.3 Gbits/sec    0   4.17 MBytes
[  5]   6.00-7.00   sec  1.38 GBytes  11.9 Gbits/sec    0   4.17 MBytes
[  5]   7.00-8.00   sec  1.23 GBytes  10.5 Gbits/sec    0   4.17 MBytes
```
好像性能没有任何优势


#### 物理机中无法观察到流量

sudo tcpdump -i enp130s0f1np1

#### 配置 vlan 不会导致不通

sudo ip link set enp130s0f1np1 vf 1 vlan 200
sudo ip link set enp130s0f1np1 vf 0 vlan 200

当然，这两个 vlan id 还是需要相同的


### 使用不同 pf 的两个 vf 来启动

如果是一个 pf 的两个 vf，
使用 8 个 core 可以达到这个性能:

```txt
[SUM]   0.00-2.75   sec  9.69 GBytes  30.3 Gbits/sec  2887             sender
```

如果一个两个 pf 的两个 vf ，那么使用 8 个 core ，受限于网线的速度:
```txt
[SUM]   2.00-2.60   sec   674 MBytes  9.40 Gbits/sec    1
```

由于交换机的原因，如果配置 vlan 之后，他们将无法 ping 通，除非去配置交换机的 vlan

## 其他实验

### 未解之谜

当使用 sfc 的网卡，vf 通信，结果在物理机中 tcpdump 观察到流量。

### 将 iommu 切分其实是可以不去直通 sriov 的，可以继续将 vf 当普通的设备使用

### nvme 也是可以实现 sriov 的切分的

在 13900k 的环境中:
```txt
🧀  sudo lspci -v | grep SR-IOV
[sudo] password for martins3:
        Capabilities: [320] Single Root I/O Virtualization (SR-IOV)
        Capabilities: [1b4] Single Root I/O Virtualization (SR-IOV)
        Capabilities: [1d4] Single Root I/O Virtualization (SR-IOV)
```
两个 zhitai 的盘，一个 intel 的集显

但是会切分失败
```txt
🧀  cat /sys/devices/pci0000:00/0000:00:1a.0/0000:03:00.0/sriov_totalvfs
3
```

不成功，报错为:
```txt
[857170.483268] nvme 0000:03:00.0: not enough MMIO resources for SR-IOV
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
