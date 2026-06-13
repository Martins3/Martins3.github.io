# GENEVE 隧道实验
<!-- ca7d984e-dde4-4ad8-918c-a8bcf9c72e14 -->

## 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                         物理主机                                 │
│  ┌─────────────────────────┐      ┌─────────────────────────┐   │
│  │      host1 (NS)         │      │      host2 (NS)         │   │
│  │  ┌─────────────────┐    │      │    ┌─────────────────┐  │   │
│  │  │   geneve0       │    │      │    │   geneve0       │  │   │
│  │  │   10.0.0.1/24   │◄───┼──────┼───►│   10.0.0.2/24   │  │   │
│  │  │   VNI=100       │    │      │    │   VNI=100       │  │   │
│  │  └────────┬────────┘    │      │    └────────┬────────┘  │   │
│  │           │             │      │             │           │   │
│  │  ┌────────▼────────┐    │      │    ┌────────▼────────┐  │   │
│  │  │   veth0         │    │      │    │   veth1         │  │   │
│  │  │ 192.168.100.1/24│◄───┼──────┼───►│ 192.168.100.2/24│  │   │
│  │  └─────────────────┘    │      │    └─────────────────┘  │   │
│  └─────────────────────────┘      └─────────────────────────┘   │
│                                                                 │
│  数据流向:                                                       │
│  10.0.0.1 ──► geneve0 ──► UDP/6081 ──► veth0 ──► veth1 ──► geneve0 ──► 10.0.0.2
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 使用方法

```bash
# 一键搭建环境并测试
make setup

# 查看网络状态
make status

# 测试连通性
make test

# 抓包查看 GENEVE 封装
make capture

# 清理环境
make clean
```

## GENEVE 协议说明

GENEVE (Generic Network Virtualization Encapsulation) 是一种灵活的网络虚拟化封装协议，特点：

- **基于 UDP**: 使用 UDP 端口 6081 (默认)
- **VNI 标识**: 24-bit VXLAN Network Identifier (0-16777215)
- **可扩展**: 支持可选的 TLV (Type-Length-Value) 选项
- **外层封装格式**:
  ```
  +------------------+
  |   Ethernet       |
  +------------------+
  |   IP (UDP)       |
  +------------------+
  |   GENEVE Header  |
  |   - Ver (2bits)  |
  |   - Opt Len      |
  |   - Flags        |
  |   - Protocol     |
  |   - VNI (24bits) |
  |   - Reserved     |
  +------------------+
  |   Options (opt)  |
  +------------------+
  |   Inner Ethernet |
  +------------------+
  |   Inner IP       |
  +------------------+
  |   Payload        |
  +------------------+
  ```

## 常用命令

```bash
# 查看 GENEVE 接口详细信息
sudo ip -d link show geneve0

# 进入命名空间执行命令
sudo ip netns exec host1 ip addr
sudo ip netns exec host1 ping 10.0.0.2

# 手动抓包分析
sudo ip netns exec host1 tcpdump -i veth0 -nn -X udp port 6081
```

## vxlan 和 geneve 非常类似

```txt
相似之处

 特性       VXLAN              GENEVE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 封装方式   UDP + 固定头       UDP + 可扩展头
 网络标识   24-bit VNI         24-bit VNI
 默认端口   4789               6081
 应用场景   数据中心 Overlay   数据中心 Overlay

关键区别：可扩展性

这是两者设计的核心差异：

VXLAN 头 (固定 8 字节):
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|R|R|R|R|I|R|R|R|            Reserved                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                VNI (24-bit)           |    Reserved           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

GENEVE 头 (最小 8 字节，可扩展):
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Ver|Opt Len|O|C|    Reserved     |          Protocol Type      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                VNI (24-bit)           |    Reserved           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|              Variable Length Options (0-252 bytes)            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

为什么需要 GENEVE？

VXLAN 的问题：头部固定，无法携带额外元数据。例如：

• 无法传递源容器的安全组信息
• 无法标记流量优先级
• 无法传递 Trace/Telemetry 数据

GENEVE 的解决方案：TLV (Type-Length-Value) 选项

# 实际例子：OpenStack 可能需要传递这样的元数据
- 源 VM 的 tenant ID
- 安全组规则标记
- 负载均衡的原始目的 IP

一句话总结

▌ VXLAN = 够用但僵化，GENEVE = 灵活但复杂

如果你的场景只需要基本的网络隔离，两者没区别。但如果需要传递额外的网络策略元数据（如 Kubernetes 网络策略、云厂商的 VPC 功能），GENEVE 的扩展
能力就体现出来了。

很多云厂商（AWS、Azure）内部其实用的是类似 GENEVE 的方案，只是对外的标准接口兼容 VXLAN。
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
