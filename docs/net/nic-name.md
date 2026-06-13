## 网卡名称

- [可预测的网络接口名](https://zhuanlan.zhihu.com/p/38369902)
- [Predictable Network Interface Names](https://www.freedesktop.org/wiki/Software/systemd/PredictableNetworkInterfaceNames/)
- https://systemd.io/PREDICTABLE_INTERFACE_NAMES/
- https://www.freedesktop.org/software/systemd/man/latest/systemd.net-naming-scheme.html : 具体的细节

简单来说:

> 固件、BIOS 提供的不可插拔的板载设备，命名 eno1，eno2……
> 固件、BIOS 提供的 PCI-E 可热插拔的设备，命名 ens1，ens2……
> physical/geographical location of the connector of the hardware，命名为 enp2s0……
> MAC 地址，命名为 enx78e7d1ea46da
> 传统的，命名为 eth0……

## 对照图
https://unix.stackexchange.com/questions/134483/why-is-my-ethernet-interface-called-enp0s10-instead-of-eth0

## 实验1 : 在虚拟机中， 是否 enable iommu 也会导致网卡名称变化

enable 之后:

```txt
3: enp0s4: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:00:00:15 brd ff:ff:ff:ff:ff:ff
    inet6 fe80::d257:27da:d80b:64be/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```

没有

```txt
3: ens5: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:00:00:15 brd ff:ff:ff:ff:ff:ff
    altname enp0s5
    inet 10.0.0.100/16 brd 10.0.255.255 scope global noprefixroute ens5
       valid_lft forever preferred_lft forever
    inet6 fe80::fe45:9ed6:a64e:4d19/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```

看到没，当存在 iommu 之后，网卡名称增加了 domain 。


在 https://www.freedesktop.org/software/systemd/man/latest/systemd.net-naming-scheme.html 中:

> The PCI domain is only prepended when it is not 0

## 实验 2 : 测试这个问题

- [ ] 这个东西可以制作一个假的看看。看看有没有变化:

```c
/**
 *	dev_get_phys_port_name - Get device physical port name
 *	@dev: device
 *	@name: port name
 *	@len: limit of bytes to copy to name
 *
 *	Get device physical port name
 */
int dev_get_phys_port_name(struct net_device *dev,
			   char *name, size_t len)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	int err;

	if (ops->ndo_get_phys_port_name) {
		err = ops->ndo_get_phys_port_name(dev, name, len);
		if (err != -EOPNOTSUPP)
			return err;
	}
	return devlink_compat_phys_port_name_get(dev, name, len);
}
```
## 实验 3 : 在 guest OS 中，当

-machine pc 变为
-machine q35 之后，可以发现虚拟机中网卡名称全部变为了 enp0s5 这种



## 其他的测试 13900k

```txt
2: enp5s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel master ovs-system state UP group default qlen 1000
    link/ether 00:e0:4c:68:0c:0c brd ff:ff:ff:ff:ff:ff
3: enp6s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether a0:36:bc:ad:c2:ce brd ff:ff:ff:ff:ff:ff
```

```txt
05:00.0 Ethernet controller: Realtek Semiconductor Co., Ltd. RTL8125 2.5GbE Controller (rev 05)
06:00.0 Ethernet controller: Intel Corporation Ethernet Controller I225-V (rev 03)
```

n100

```txt
2: enp1s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether 58:47:ca:76:2d:9f brd ff:ff:ff:ff:ff:ff
    inet 10.0.0.5/16 brd 10.0.255.255 scope global noprefixroute enp1s0
       valid_lft forever preferred_lft forever
    inet6 fe80::1f6f:f5a4:4fa6:1f2d/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
3: enp3s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether 58:47:ca:76:2d:a0 brd ff:ff:ff:ff:ff:ff
```

```txt
🧀  lspci | grep Eth
01:00.0 Ethernet controller: Intel Corporation Ethernet Controller I226-V (rev 04)
03:00.0 Ethernet controller: Intel Corporation Ethernet Controller I226-V (rev 04)
```

## 发生修改的地方
dev_change_name

## 可以影响 rename 的地方还有

具体的，详细的，在这里。
- https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/7/html/networking_guide/sec-understanding_the_device_renaming_procedure#sec-Understanding_the_Device_Renaming_Procedure

可以看到，甚至 /etc/sysconfig/network-scripts/ 都会控制。

例如，如果有 nic 匹配了 HWADDR =
```txt
[root@node74 11:12:43 network-scripts]$ cat ifcfg-ens1f0
DEVICE=ens1f0abcfafa
HWADDR=52:54:00:00:02:58
TYPE=Ethernet
ONBOOT=yes
BOOTPROTO=no
```
那么，网卡的名称将会变为 `ens1f0abcfafa` 。

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
