## 背景介绍
- http://oldvger.kernel.org/lpc_net2018_talks/erspan-linux-presentation.pdf
- https://lpc.events/event/2/contributions/98/attachments/97/115/erspan-linux.pdf

net/ipv4/ip_gre.c 中有相关的代码:

```c
static const struct net_device_ops erspan_netdev_ops = {
	.ndo_init		= erspan_tunnel_init,
	.ndo_uninit		= ip_tunnel_uninit,
	.ndo_start_xmit		= erspan_xmit,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_change_mtu		= ip_tunnel_change_mtu,
	.ndo_get_stats64	= dev_get_tstats64,
	.ndo_get_iflink		= ip_tunnel_get_iflink,
	.ndo_fill_metadata_dst	= gre_fill_metadata_dst,
};
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

ERSPAN（Enhanced Remote Switched Port Analyzer）是一种网络流量监控协议，**主要用于在 Cisco 等厂商的交换机和路由器中远程镜像流量**，将指定端口或 VLAN 的流量封装后，通过 IP 网络（通常是 GRE 隧道）发送到远程监控设备（如抓包分析器、IDS 等）。

在 Linux 内核中，`erspan` 是一种 **基于 GRE（Generic Routing Encapsulation）的隧道设备类型**，用于支持接收或发送符合 ERSAN 协议格式的封包。它的主要特点包括：

- 使用 GRE 封装，并在 GRE 头部之后添加一个 **ERSPAN 专用的元数据头**（包含会话 ID、时间戳等信息）；
- 支持两种版本：**ERSPAN Type II**（较老）和 **ERSPAN Type III**（带时间戳、方向等扩展字段）；
- 在 Linux 中，`erspan` 设备通过 `ip_gre` 模块实现，表现为一种虚拟网络接口（如 `erspan0`），其 `net_device_ops` 正是你列出的 `erspan_netdev_ops`；
- 常用于构建 **Linux 作为 ERSAPAN 目标（接收镜像流量）或源（主动封装并发送镜像流量）** 的监控/分析系统。

典型使用场景：

- 在云环境中，将物理交换机镜像的流量通过 ERSAPAN 发给 Linux 虚拟机做深度包检测；
- 使用 `ip link add dev erspan0 type erspan ...` 创建 ERSAPAN 隧道；
- 抓包工具（如 tcpdump）可直接在 `erspan0` 接口上抓取解封装后的原始流量。

总结：**ERSPAN 是一种用于远程网络流量镜像的隧道协议，Linux 内核通过 erspan 虚拟网卡支持其收发功能，便于构建网络监控基础设施。**

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
