## veth

从其中可以注意到:
- `dev_hard_start_xmit`
  - `veth_xmit`
    - `veth_forward_skb`

然后可以找到这个:
drivers/net/veth.c
```c
static const struct net_device_ops veth_netdev_ops = {
```

```txt
[root@centos ~]$ iptables -t nat -A POSTROUTING -s 172.16.0.0/24 ! -o br0 -j MASQUERADE
# -t nat：选择 nat 表，不指定，默认是 filter 表。
# -A：Append 新增一条规则，-D Delete，-L List。
# POSTROUTING：选择 POSTROUTING chain，POSTROUTING代表数据包出口阶段。
# -s 172.16.0.0/24：匹配条件，source address 是网段 172.16.0.102/24 中的 IP。
# -j MASQUERADE：匹配后执行命令，执行 MASQUERADE，也就是源 IP 地址伪装为出口网卡 IP。对于 nat 表来说，可选项有 DNAT/MASQUERADE/REDIRECT/SNAT。
```
https://www.finclip.com/blog/container-virtualization-network-1/

其实这里有个奇怪的地方: 不是用 vxlan 实现跨主机吗？怎么又变成了 netfilter 了?

## 更多阅读
https://docs.docker.com/network/
https://docs.docker.com/network/drivers/


## [x] 容器中为什么不用 bridge ，而是非要使用 veth
两个是配合起来的，veth 相当于是一个管道，

这里描述的的是
https://developers.redhat.com/blog/2018/10/22/introduction-to-linux-interfaces-for-virtual-networking#veth

但是 veth.sh 中，目前仅仅可以

```sh
sudo ip link set dev C.1 master br0
```

但是无法将 C.1 连接到

```sh
sudo ip link set dev C.1 master br0
```

正确的操作办法是:
```sh
sudo ovs-vsctl add-port br-in C.1
sudo ovs-vsctl add-port br-in D.1
```

所以，其实，我们的问题是，为什么虚拟机中用的 tun ，而 container 用 veth 的。

## 使用 net/veth.sh 测试

看看这个日志的都是从那里来的
```txt
[252084.506801] br0: port 1(C.1) entered blocking state
[252084.506806] br0: port 1(C.1) entered disabled state
[252084.506826] C.1: entered allmulticast mode
[252084.506893] C.1: entered promiscuous mode
[252084.506962] br0: port 1(C.1) entered blocking state
[252084.506964] br0: port 1(C.1) entered forwarding state
[252084.526494] br0: port 2(D.1) entered blocking state
[252084.526498] br0: port 2(D.1) entered disabled state
[252084.526528] D.1: entered allmulticast mode
[252084.526580] D.1: entered promiscuous mode
[252084.526608] br0: port 2(D.1) entered blocking state
[252084.526610] br0: port 2(D.1) entered forwarding state
```

## /sys/class/net 的观察

当 veth 和 bridge 联系起来的时候，/sys/class/net/C.1 下会增加如下内容
```txt
Name
 brport
 master -> ../br0
 upper_br0 -> ../br0
```

但是如果是增加
```txt
master -> ../ovs-system
upper_ovs-system -> ../ovs-system
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
