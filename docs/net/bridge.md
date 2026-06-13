# bridge

## 内核代码
https://github.com/liexusong/linux-source-code-analyze/blob/master/net_bridge.md

## 基本操作

https://wiki.archlinux.org/title/network_bridge

```sh
	# sudo ip link set enp7s0 master br0
	sudo ip link add br0 type bridge
	sudo ip address add 10.0.0.10/24 dev br0
	sudo ip link set dev br0 up

  sudo ip link delete br0 type bridge
```

## [ ]  网络不通的场景

1. 在物理机 A 上操作， 将 bridge attach 到一个网卡上之后，其他的机器就无法 ping 通了 A 。使用 ovs 进行类似的操作结果也是如此的。

```sh
	sudo ip link add br0 type bridge
	sudo ip link set dev br0 up

  sudo ip link set eth0 master br0
```

2. 在物理机 A 上操作，执行如下，操作
```sh
	sudo ip link add br0 type bridge
	sudo ip link set dev br0 up
	sudo ip address add 10.0.0.10/24 dev br0
```

3. 但是如果 A 将这两个都执行之后，网络将会重新联通起来。B 可以 ping 通 bridge 使用的 ip
```sh
	sudo ip link add br0 type bridge
	sudo ip link set dev br0 up

	sudo ip address add 10.0.0.10/24 dev br0
  sudo ip link set enu1u2 master br0
```

解释第一种场景:
使用 route -n 检查，其输出总是:
```txt
Kernel IP routing table
Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
0.0.0.0         192.168.1.1     0.0.0.0         UG    600    0        0 wlan0
10.0.0.0        0.0.0.0         255.255.255.0   U     0      0        0 br0
10.0.0.0        0.0.0.0         255.255.255.0   U     100    0        0 enu1u2
172.17.0.0      0.0.0.0         255.255.0.0     U     0      0        0 docker0
192.168.1.0     0.0.0.0         255.255.255.0   U     600    0        0 wlan0
```
因为 br0 的 Metric 是 0 ，所以只是赋了 ip，但是没有让 br0 和 enu1u2 连接起来，
那么所有数据会发送给 br0 的。

第二种场景不知道理解?



## [ ] 原来 vlan 也是 bridege 中需要处理的
或者说，vlan 是 L2 处理的内容。

##
https://segmentfault.com/a/1190000009491002

## 配置一个网卡的内容
- https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/6/html/deployment_guide/s2-networkscripts-interfaces_network-bridge#s2-networkscripts-interfaces_network-bridge_with_bond

举个例子:
```txt
cat /etc/sysconfig/network-scripts/ifcfg-br0

DEVICE=br0
TYPE=Bridge
ONBOOT=yes
BOOTPROTO=static
IPADDR=10.10.0.2
NETMASK=255.255.240.0
NM_CONTROLLED=no
DELAY=0
GATEWAY=10.10.0.3

cat /etc/sysconfig/network-scripts/ifcfg-enp1s02

DEVICE=eno3
TYPE=Ethernet
ONBOOT=yes
BOOTPROTO=none
NM_CONTROLLED=no
BRIDGE=br0
DELAY=5
```
## 这个 STP 概念是 bridge 特有的吗？

ovs 有这个问题么?


```txt
🧀  sudo brctl show
bridge name     bridge id               STP enabled     interfaces
br9527          8000.5a15a1b6c37f       no              br_vif_s_24_1
                                                        br_vif_s_24_2
                                                        enp125s0f0
docker0         8000.0242c19ea57c       no
```

🧀  sudo brctl stp br9527 off

(kunepng 机器) 开机启动之后，总是有这个问题:
```txt
[   66.538044][   C23] br9527: port 1(enp125s0f0) entered learning state
[   81.641864][    C0] br9527: port 1(enp125s0f0) entered forwarding state
```

(kunepng) 现在是启动一次 qemu 就会有这么多日志吗?
```txt
[347183.023220] br9527: port 3(vif_s_19_2) entered forwarding state
[347183.030245] br9527: topology change detected, sending tcn bpdu
[347183.037063] br9527: port 2(vif_s_19_1) entered forwarding state
[347183.043953] br9527: topology change detected, sending tcn bpdu
[347276.106402] br9527: port 2(vif_s_19_1) entered disabled state
[347277.134231] br9527: port 3(vif_s_19_2) entered disabled state
[347329.384341] br9527: port 2(vif_s_19_1) entered blocking state
[347329.391320] br9527: port 2(vif_s_19_1) entered listening state
[347329.398942] br9527: port 3(vif_s_19_2) entered blocking state
[347329.405953] br9527: port 3(vif_s_19_2) entered listening state
[347344.557324] br9527: port 2(vif_s_19_1) entered learning state
[347344.564122] br9527: port 3(vif_s_19_2) entered learning state
[347359.661139] br9527: port 3(vif_s_19_2) entered forwarding state
[347359.668195] br9527: topology change detected, sending tcn bpdu
[347359.675025] br9527: port 2(vif_s_19_1) entered forwarding state
[347359.682115] br9527: topology change detected, sending tcn bpdu
[347654.429911] br9527: port 2(vif_s_19_1) entered disabled state
[347655.433926] br9527: port 3(vif_s_19_2) entered disabled state
[347657.779332] br9527: port 2(vif_s_19_1) entered blocking state
[347657.786260] br9527: port 2(vif_s_19_1) entered listening state
[347657.793767] br9527: port 3(vif_s_19_2) entered blocking state
[347657.800550] br9527: port 3(vif_s_19_2) entered listening state
[347673.001447] br9527: port 2(vif_s_19_1) entered learning state
[347673.008242] br9527: port 3(vif_s_19_2) entered learning state
[347688.105275] br9527: port 3(vif_s_19_2) entered forwarding state
[347688.112293] br9527: topology change detected, sending tcn bpdu
[347688.119111] br9527: port 2(vif_s_19_1) entered forwarding state
[347688.125964] br9527: topology change detected, sending tcn bpdu
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
