# macvlan & ipvlan 网络虚拟化 Demo

演示 Linux 网络虚拟化技术 macvlan 和 ipvlan 的使用和区别。

## 核心概念

### macvlan 解决什么问题？

**问题：在一台物理机上创建多个"独立网卡"**

传统 bridge 模式的问题：
- 所有容器共享物理网卡的 MAC 地址
- 外部网络无法区分流量来自哪个容器
- 部分网络设备/安全策略依赖 MAC 地址识别

**macvlan 方案：**
- 每个虚拟接口有**独立的 MAC 地址**
- 对外部网络完全透明，看起来像多个独立物理设备
- 性能略优于 bridge（少一层转发）

```
对比图：

bridge 模式:                    macvlan 模式:
┌─────────────────┐            ┌─────────────────┐
│   物理网卡 eth0  │            │   物理网卡 eth0  │
│   MAC: 52:54... │            │   MAC: 52:54... │
└────────┬────────┘            └─────────────────┘
         │                              │
    ┌────┴────┐                ┌────────┼────────┐
    │ Bridge  │                │        │        │
    └────┬────┘            ┌───┴───┐ ┌──┴───┐ ┌──┴───┐
         │                 │macvlan0│macvlan1│macvlan2│
    ┌────┼────┐            │独立MAC │独立MAC │独立MAC │
    │    │    │            └───┬───┘ └──┬───┘ └──┬───┘
  容器A 容器B 容器C           容器A    容器B    容器C
  
所有容器共享同一个 MAC       每个容器有独立 MAC
外部网络无法区分              外部网络可区分
```

### ipvlan 解决什么问题？

**问题：macvlan 需要多个 MAC 地址，但部分环境 MAC 受限**

macvlan 的限制：
- 需要交换机支持多 MAC 地址
- 云环境（AWS/Azure）往往限制 MAC 数量
- 硬件 MAC 地址表有上限

**ipvlan 方案：**
- 所有虚拟接口**共享同一个 MAC 地址**
- 仅通过 IP 地址区分流量
- 不受交换机 MAC 地址表限制

### 核心对比

| 特性 | macvlan | ipvlan |
|------|---------|--------|
| MAC 地址 | 每个设备独立 | 所有设备共享 |
| 交换机兼容性 | 需要支持多 MAC | 兼容所有交换机 |
| MAC 地址上限 | 受硬件限制 | 无限制 |
| 性能 | 正常 | 略优(无 MAC 学习) |
| 使用场景 | 需要独立 MAC | 云环境/MAC 受限 |

## 使用场景

### macvlan 适用场景
- 容器/虚拟机需要独立 MAC 地址
- 传统网络环境需要区分设备
- 网络策略依赖 MAC 地址

### ipvlan 适用场景
- 云环境（AWS/Azure 等 MAC 受限环境）
- 需要大量虚拟接口（超出 MAC 地址限制）
- 对交换机无特殊要求

## 快速开始

```bash
# 运行 macvlan 演示
sudo PHYSICAL_DEV=ens5 ./macvlan-demo.sh

# 运行 ipvlan 演示
sudo PHYSICAL_DEV=ens5 ./ipvlan-demo.sh

# 对比两种技术
./ipvlan-demo.sh compare
```

## macvlan 使用详解

### 运行 Demo

```bash
# 完整演示
sudo ./macvlan-demo.sh

# 查看帮助
./macvlan-demo.sh help

# 指定物理网卡
sudo PHYSICAL_DEV=ens33 ./macvlan-demo.sh

# 显示常用命令
./macvlan-demo.sh cmd

# 创建多个 macvlan 设备
sudo PHYSICAL_DEV=ens5 ./macvlan-demo.sh multi

# 仅清理环境
sudo ./macvlan-demo.sh cleanup
```

### 手动操作

```bash
# 创建 macvlan 设备
ip link add macvlan0 link eth0 type macvlan mode bridge

# 创建网络命名空间
ip netns add ns1
ip link set macvlan0 netns ns1

# 配置 IP
ip netns exec ns1 ip addr add 192.168.100.2/24 dev macvlan0
ip netns exec ns1 ip link set macvlan0 up
```

### 模式说明

- **bridge**: 允许同物理接口上的 macvlan 设备之间通信
- **vepa**: 强制流量通过外部交换机
- **private**: 禁止同物理接口上的设备间通信
- **passthru**: 将物理接口直接交给单个 macvlan 设备

## ipvlan 使用详解

### 运行 Demo

```bash
# 完整演示
sudo ./ipvlan-demo.sh

# 查看帮助
./ipvlan-demo.sh help

# 指定物理网卡
sudo PHYSICAL_DEV=ens33 ./ipvlan-demo.sh

# 显示常用命令
./ipvlan-demo.sh cmd

# 创建多个 ipvlan 设备
sudo PHYSICAL_DEV=ens5 ./ipvlan-demo.sh multi

# L3 模式演示
sudo PHYSICAL_DEV=ens5 ./ipvlan-demo.sh l3

# 仅清理环境
sudo ./ipvlan-demo.sh cleanup
```

### 手动操作

```bash
# 创建 ipvlan 设备 (L2 模式)
ip link add ipvlan0 link eth0 type ipvlan mode l2

# 创建网络命名空间
ip netns add ns1
ip link set ipvlan0 netns ns1

# 配置 IP
ip netns exec ns1 ip addr add 192.168.200.2/24 dev ipvlan0
ip netns exec ns1 ip link set ipvlan0 up
```

### 模式说明

- **l2**: 二层模式，行为类似 macvlan，共享 MAC 地址
- **l3**: 三层模式，不需要 ARP，通过路由转发
- **l3s**: 三层模式，带安全策略

## 环境变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `PHYSICAL_DEV` | eth0 | 物理网卡名称 |
| `IP_PREFIX` | 192.168.100/200 | IP 地址前缀 |

## 清理环境

```bash
# 清理 macvlan
sudo ./macvlan-demo.sh cleanup

# 清理 ipvlan
sudo ./ipvlan-demo.sh cleanup

# 或手动清理
ip netns del macvlan-ns 2>/dev/null || true
ip link del macvlan0 2>/dev/null || true
ip netns del ipvlan-ns 2>/dev/null || true
ip link del ipvlan0 2>/dev/null || true
```

## 预期测试结果

### macvlan
- [OK] 设备创建成功
- [OK] MAC 地址独立（与物理设备不同）
- [OK] 多个 macvlan 设备间通信正常
- [FAIL] 与主机通信失败（bridge 模式设计特性）

### ipvlan
- [OK] 设备创建成功
- [OK] MAC 地址共享（与物理设备相同）
- [OK] 多个 ipvlan 设备间通信正常
- [FAIL] 与主机通信失败（L2 模式设计特性）

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
