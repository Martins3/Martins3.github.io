# VXLAN

## VXLAN 核心概念

VXLAN (Virtual Extensible LAN) 是一种 Overlay 网络技术，将二层以太网帧封装在 UDP 包中传输。

```
封装结构:
+-------------------------------+
|        Outer Ethernet         |
+-------------------------------+
|         Outer IP              |  <- Underlay (物理网络)
|      UDP Port 4789            |
+-------------------------------+
|      VXLAN Header (VNI)       |  <- 24位 VNI 标识
+-------------------------------+
|       Inner Ethernet          |  <- Overlay (虚拟网络)
|       Inner IP/Payload        |
+-------------------------------+
```

| 概念 | 说明 |
|------|------|
| VTEP | VXLAN Tunnel End Point，隧道端点，负责封装/解封装 |
| VNI | VXLAN Network Identifier，24位，支持1600万个隔离网络 |
| Underlay | 底层物理网络，VXLAN 包在此传输 |
| Overlay | 虚拟二层网络，用户看到的网络 |

## 核心问题：Bridge 如何选择转发路径？

**关键理解：Bridge 是二层交换机，它只看 MAC 地址，不看 IP！**

当容器数据到达 bridge 时，转发决策完全基于 **FDB (Forwarding Database)** 中的 MAC 地址表：

```
┌─────────────────────────────────────────────────────────────┐
│  ns-host1 的 FDB 表 (bridge fdb show)                       │
├─────────────────────────────────────────────────────────────┤
│  MAC                  │ 端口      │ 转发动作               │
├───────────────────────┼───────────┼────────────────────────┤
│  f2:77:e8:53:85:57    │ veth4     │ 本地容器，直接转发     │
│  (ns-container1)      │           │                        │
├───────────────────────┼───────────┼────────────────────────┤
│  6a:f7:21:16:6d:fc    │ vxlan0    │ 走 VXLAN 隧道          │
│  (ns-container2)      │           │ dst=172.16.0.2         │
├───────────────────────┼───────────┼────────────────────────┤
│  其他/未知 MAC        │ -         │ 泛洪到所有端口         │
└───────────────────────┴───────────┴────────────────────────┘
```

### 转发流程详解

```
ns-container1 → ns-container2 的数据流程:

1. 容器发帧
   Src MAC: f2:77:e8:53:85:57 (ns-container1)
   Dst MAC: 6a:f7:21:16:6d:fc (ns-container2)  <- 关键！

2. 到达 br0 (通过 veth4)
   br0 学习: "f2:77:e8:53:85:57 在 veth4 端口"

3. br0 查 FDB 找目的 MAC
   "6a:f7:21:16:6d:fc 在 vxlan0 端口，dst=172.16.0.2"

4. 转发到 vxlan0 -> 封装
   Outer IP: 172.16.0.1 -> 172.16.0.2
   Outer UDP: 4789
   VNI: 100
   Inner: 原始帧不变

5. 到达 ns-host2 -> 解封装 -> 查 FDB -> 转发到 veth6
```

### 与普通网络的区别

| 场景 | FDB 中的端口 | 行为 |
|------|-------------|------|
| 同一主机内容器通信 | veth4/veth6 | 本地 bridge 转发，不走 VXLAN |
| 跨主机容器通信 | vxlan0 | 封装后通过 VXLAN 隧道传输 |
| 容器访问外部 | br0/网关 | 走主机路由表，可能出 ens5 |

详细机制说明见 [bridge-forwarding.md](bridge-forwarding.md)

## 实验1: 观察现有 K8s Flannel VXLAN

你的环境已有 K8s + Flannel，可以直接观察真实的 VXLAN：

```bash
# 查看 flannel VXLAN 设备
ip -d link show flannel.1

# 查看 VTEP 信息
ip neighbor show dev flannel.1

# 查看 VXLAN 转发表(FDB)
bridge fdb show dev flannel.1

# 查看路由（Pod 网段通过 VXLAN）
ip route | grep flannel
```

## 实验2: 手动创建 VXLAN 隧道

如果你想从零搭建 VXLAN，有两个选择:

### 方案A: 在现有 namespace 中创建（推荐）

由于系统已有大量 CNI namespace，我们在当前 root namespace 创建:

```bash
# 创建两个 VXLAN 设备模拟跨主机通信
# 使用 loopback 作为 underlay
sudo ip link add vxlan-demo type vxlan id 100 local 127.0.0.1 remote 127.0.0.1 dstport 4789 dev lo
sudo ip addr add 10.99.1.1/24 dev vxlan-demo
sudo ip link set vxlan-demo up

# 测试自环
ping -c 3 10.99.1.1

# 查看详细信息
ip -d link show vxlan-demo
```

### 方案B: 完整多 namespace 实验（清理后运行）

```bash
# 先清理所有实验 namespace
sudo ./cleanup.sh

# 创建环境
sudo ./setup.sh

# 运行测试
sudo ./test.sh

# 抓包分析
sudo ./capture.sh

# 清理
sudo ./cleanup.sh
```

## 关键命令速查

```bash
# 创建 VXLAN 设备
ip link add vxlan0 type vxlan id 100 local 172.16.0.1 remote 172.16.0.2 dstport 4789 dev eth0

# 创建 VXLAN + Bridge
ip link add br0 type bridge
ip link set vxlan0 master br0

# 查看 FDB（MAC 地址转发表）
bridge fdb show

# 添加静态 FDB
bridge fdb add 00:11:22:33:44:55 dev vxlan0 dst 172.16.0.2

# 多播模式（一对多）
ip link add vxlan0 type vxlan id 100 group 239.1.1.1 dstport 4789 dev eth0
```

## 进阶：VXLAN vs VLAN

| 特性 | VLAN | VXLAN |
|------|------|-------|
| 隔离数量 | 4094 (12-bit) | 1600万 (24-bit) |
| 传输方式 | 二层帧直接传输 | 封装在 UDP 中 |
| 底层依赖 | 需要二层连通 | 只需要三层连通 |
| 应用场景 | 数据中心内部 | 跨机架/跨机房 |

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
