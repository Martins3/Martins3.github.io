## ipvlan
<!-- a2617ba8-32f4-4fc9-bac7-6f31f81166d2 -->

关联话题 ./macvlan.md

## 设计哲学与演进背景

### 为什么需要 ipvlan？

```

macvlan (解决 bridge 性能问题)
        ┌─────────┐
        │ 容器 A  │──┐ 独立 MAC
        └─────────┘  │
                     ├────► eth0 (物理网卡)
        ┌─────────┐  │
        │ 容器 B  │──┘ 独立 MAC
        └─────────┘
        问题: 交换机可能限制单端口 MAC 数量，promiscuous 模式

ipvlan (解决 macvlan 的交换机兼容性问题)
        ┌─────────┐
        │ 容器 A  │──┐ 共享 MAC
        └─────────┘  │      通过 IP 区分
                     ├────► eth0 (物理网卡)
        ┌─────────┐  │
        │ 容器 B  │──┘ 共享 MAC
        └─────────┘
```

**ipvlan 的诞生动机**（Google 内部需求驱动，2014 年由 Mahesh Bandewar 开发）：

1. **数据中心交换机限制**: 企业级交换机通常限制单端口学习 MAC 地址数量（如 32/64/128 个），大规模容器部署时 macvlan 会触发端口安全策略
2. **Promiscuous 模式性能问题**: 大量 macvlan 设备可能迫使网卡进入 promiscuous 模式，导致所有流量走软件路径
3. **L3 路由简化**: 在云环境中，网络通常基于 L3 路由而非 L2 交换，ipvlan L3 模式更符合云网络模型

### 解决了什么问题

| 问题 | 传统方案 | ipvlan 方案 | 效果 |
|------|---------|------------|------|
| **MAC 地址耗尽** | macvlan 每个容器独立 MAC | 共享物理网卡 MAC | 单端口 MAC 数量从 N 降为 1 |
| **交换机端口安全告警** | 需要配置 port-security | 天然规避 | 减少运维干预 |
| **Bridge 性能开销** | veth+bridge 引入软交换 | 直接绑定物理网卡 | 延迟降低 10-30%，吞吐提升 |
| **Netfilter 开销** | bridge 走 iptables | L3 模式绕过 | CPU 使用率降低 |
| **云网络兼容性** | 需要 Overlay 封装 | 原生 IP 直连 | 与 VPC 网络无缝集成 |

### 带来了什么问题（ Trade-offs ）

**1. 网络可见性降低**
```
问题: 所有容器共享同一 MAC，传统网络工具（tcpdump 在交换机上）无法区分流量来源
影响: 网络排障困难，安全审计复杂
```

**2. 与现有网络的兼容性**
```
问题:
- DHCP 服务器通常基于 MAC 分配 IP，共享 MAC 导致 DHCP 冲突
- ARP 协议在 L3 模式被禁用，某些依赖 ARP 的应用异常
- 需要静态 IP 或专门的路由配置

影响: 无法即插即用，需要重新设计网络架构
```

**3. 调试复杂度增加**
```
问题: 跨 namespace 的流量转发在内核中完成，难以追踪
对比: bridge 可以通过 brctl showmacs 查看 MAC 表，ipvlan 需要查看内核哈希表
```

**4. 生态支持有限**
```
现状:
- Docker 默认不支持 ipvlan（需要 --network ipvlan 手动指定）
- Kubernetes CNI 生态中主流是 Calico/Cilium/Flannel，原生 ipvlan 支持有限
- 多数云厂商的托管 K8s 不提供 ipvlan 网络选项
```

### 生产环境使用情况

**使用频率: 相对小众，特定场景**

| 场景 | 使用 ipvlan | 主流方案 |
|------|------------|---------|
| 公有云容器 | 极少 | VPC CNI (AWS/Azure/GCP) |
| 私有云/裸金属 K8s | 部分使用 | Calico BGP / Cilium |
| 超大规模容器 (10k+/node) | 考虑使用 | ipvlan / SR-IOV |
| NFV/VNF | 较多使用 | ipvlan / DPDK |
| 边缘计算 | 部分使用 | Flannel / Cilium |

**主要原因**:
1. **overlay 网络的胜利**: VXLAN/Geneve overlay 方案（Flannel/Calico VXLAN/Cilium）成为 Kubernetes 主流，它们不依赖底层网络配置，部署更简单
2. **云厂商锁定**: 主流云厂商提供自研 CNI（如 AWS VPC CNI、Azure CNI），直接对接 VPC，ipvlan 优势不明显
3. **CNI 生态**: 原生支持 ipvlan 的 CNI 插件较少，且社区活跃度不如 Calico/Cilium

### 主要应用场景

**场景 1: 裸金属大规模容器部署**
```
环境: 自建 IDC，物理机直接跑容器，无虚拟化层
需求: 单机 1000+ 容器，需要直连物理网络
优势: 避免 bridge 开销，避免 MAC 地址耗尽
案例: 早期 Docker Swarm 大规模部署，部分头条/字节跳动内部业务
```

**场景 2: NFV (Network Function Virtualization)**
```
环境: 电信云，运行 VNF（虚拟化网络功能）
需求: 网络功能（防火墙、LB）需要直接处理物理流量
优势: L3 模式性能接近物理网卡，适合包处理
案例: OpenStack Tacker，某些 5G core 网元部署
```

**场景 3: 需要直连 VPC 的容器**
```
环境: 混合云，容器需要与 VM 处于同一 L2/L3 网络
需求: 容器 IP 可被外部直接访问，无需 NAT
优势: 容器作为"一等公民"接入 VPC
案例: AWS ECS 早期支持 ipvlan，某些金融企业合规要求
```

**场景 4: 高性能网络测试/基准测试**
```
环境: 网络性能测试工具开发
需求: 最小化虚拟化开销，测试物理网卡极限
优势: 代码路径最短，接近裸机性能
```

### 与主流 CNI 的关系

```
┌─────────────────────────────────────────────────────────────┐
│                    Kubernetes CNI 生态                       │
├─────────────────────────────────────────────────────────────┤
│  Overlay 方案 (主流)                                         │
│  ├── Flannel (VXLAN)        - 简单易用，性能一般              │
│  ├── Calico (VXLAN/IPIP)    - 功能丰富，企业常用              │
│  └── Cilium (VXLAN/路由)    - eBPF 加速，云原生首选           │
│                                                             │
│  Underlay/直连方案 (特定场景)                                 │
│  ├── Calico (BGP)           - 需要交换机支持 BGP              │
│  ├── macvlan                - 需要独立 MAC                    │
│  ├── ipvlan                 - 本文主角，共享 MAC              │
│  └── SR-IOV                 - 硬件虚拟化，性能最优             │
│                                                             │
│  云厂商方案                                                  │
│  ├── AWS VPC CNI            - 直接分配 ENI 弹性网卡           │
│  ├── Azure CNI              - 对接 Azure VPC                  │
│  └── GCP GKE Networking     - 对接 GCP VPC                    │
└─────────────────────────────────────────────────────────────┘
```

**ipvlan 的定位**: Underlay 方案中的轻量级选择，介于 macvlan 和 SR-IOV 之间，在不需要硬件虚拟化但需要直连物理网络时使用。

## 核心概念

ipvlan 是 Linux 内核提供的一种虚拟网络设备，与 macvlan 类似，但关键区别在于：**所有 slave 设备共享 master 设备的 MAC 地址**，通过 **L3 (IP 层) 进行多路复用/解复用**。

```
┌─────────────────────────────────────────────────────────────┐
│                         Host                                │
│  ┌─────────────┐      ┌─────────────┐                       │
│  │   NS: ns1   │      │   NS: ns2   │                       │
│  │  ┌───────┐  │      │  ┌───────┐  │                       │
│  │  │ ipvl0 │  │      │  │ ipvl1 │  │                       │
│  │  └───┬───┘  │      │  └───┬───┘  │                       │
│  └──────┼──────┘      └──────┼──────┘                       │
│         │                    │                              │
│         └────────────────────┘                              │
│                     │                                       │
│              ┌──────┴──────┐                                │
│              │  eth0 (master)│  ← 共享 MAC, L3 分发          │
│              └─────────────┘                                │
└─────────────────────────────────────────────────────────────┘
```

## 与 macvlan 的关键区别

| 特性 | macvlan | ipvlan |
|------|---------|--------|
| MAC 地址 | 每个 slave 独立 MAC | 共享 master MAC |
| 分发层级 | L2 (MAC 层) | L3 (IP 层) |
| 交换机兼容性 | 可能触发端口安全策略 | 单 MAC 无此问题 |
| promiscuous 模式 | 大量 slave 可能触发 | 不会触发 |
| 适用场景 | 需要独立 MAC | 交换机限制 MAC 数量 |

## 内核数据结构

```c
// drivers/net/ipvlan/ipvlan.h
struct ipvl_dev {
    struct net_device   *dev;           // 虚拟设备本身
    struct list_head    pnode;          // 链表节点
    struct ipvl_port    *port;          // 端口信息
    struct net_device   *phy_dev;       // 物理设备 (master)
    struct list_head    addrs;          // IP 地址列表
    struct ipvl_pcpu_stats __percpu *pcpu_stats;
    DECLARE_BITMAP(mac_filters, IPVLAN_MAC_FILTER_SIZE);
    netdev_features_t   sfeatures;
    u32                 msg_enable;
};

struct ipvl_port {
    struct net_device   *dev;           // master 设备
    struct hlist_head   hlhead[IPVLAN_HASH_SIZE];  // IP 地址哈希表
    spinlock_t          addrs_lock;
    struct list_head    ipvlans;        // 所有 slave 链表
    u16                 mode;           // L2/L3/L3S 模式
    u16                 flags;          // bridge/private/vepa
    // ...
};

struct ipvl_addr {
    struct ipvl_dev     *master;
    union {
        struct in6_addr ip6;
        struct in_addr  ip4;
    } ipu;
    struct hlist_node   hlnode;         // 哈希表节点
    struct list_head    anode;
    ipvl_hdr_type       atype;          // IPVL_IPV4/IPVL_IPV6
};
```

## 三种工作模式

### L2 模式 (`IPVLAN_MODE_L2`)

```c
// drivers/net/ipvlan/ipvlan_core.c:ipvlan_xmit_mode_l2()
static int ipvlan_xmit_mode_l2(struct sk_buff *skb, struct net_device *dev)
{
    const struct ipvl_dev *ipvlan = netdev_priv(dev);
    struct ethhdr *eth = skb_eth_hdr(skb);
    struct ipvl_addr *addr;
    void *lyr3h;
    int addr_type;

    // 1. 检查是否是同一主机的 slave 间通信
    if (!ipvlan_is_vepa(ipvlan->port) &&
        ether_addr_equal(eth->h_dest, eth->h_source)) {
        lyr3h = ipvlan_get_L3_hdr(ipvlan->port, skb, &addr_type);
        if (lyr3h) {
            // 根据目的 IP 查找对应的 slave
            addr = ipvlan_addr_lookup(ipvlan->port, lyr3h, addr_type, true);
            if (addr) {
                if (ipvlan_is_private(ipvlan->port)) {
                    consume_skb(skb);
                    return NET_XMIT_DROP;
                }
                // 直接转发到目标 slave
                ipvlan_rcv_frame(addr, &skb, true);
                return NET_XMIT_SUCCESS;
            }
        }
        // 转发到 master 设备
        dev_forward_skb(ipvlan->phy_dev, skb);
        return NET_XMIT_SUCCESS;

    } else if (is_multicast_ether_addr(eth->h_dest)) {
        // 组播/广播入队处理
        skb_reset_mac_header(skb);
        ipvlan_skb_crossing_ns(skb, NULL);
        ipvlan_multicast_enqueue(ipvlan->port, skb, true);
        return NET_XMIT_SUCCESS;
    }

    // 2. 普通单播，直接通过 master 发出
    skb->dev = ipvlan->phy_dev;
    return dev_queue_xmit(skb);
}
```

**特点**:
- TX: 在 slave 协议栈处理到 L2，然后切换到 master 发送
- RX: 接收组播/广播流量
- 性能接近传统桥接

### L3 模式 (`IPVLAN_MODE_L3`)

```c
// drivers/net/ipvlan/ipvlan_core.c:ipvlan_xmit_mode_l3()
static int ipvlan_xmit_mode_l3(struct sk_buff *skb, struct net_device *dev)
{
    const struct ipvl_dev *ipvlan = netdev_priv(dev);
    void *lyr3h;
    struct ipvl_addr *addr;
    int addr_type;

    lyr3h = ipvlan_get_L3_hdr(ipvlan->port, skb, &addr_type);
    if (!lyr3h)
        goto out;

    if (!ipvlan_is_vepa(ipvlan->port)) {
        // 检查目的 IP 是否属于同一 master 的其他 slave
        addr = ipvlan_addr_lookup(ipvlan->port, lyr3h, addr_type, true);
        if (addr) {
            if (ipvlan_is_private(ipvlan->port)) {
                consume_skb(skb);
                return NET_XMIT_DROP;
            }
            // 同一主机内转发，不走物理网络
            ipvlan_rcv_frame(addr, &skb, true);
            return NET_XMIT_SUCCESS;
        }
    }
out:
    // 跨 namespace 发送，需要 L3 路由
    ipvlan_skb_crossing_ns(skb, ipvlan->phy_dev);
    return ipvlan_process_outbound(skb);
}
```

**特点**:
- TX: 在 slave 协议栈处理到 L3，然后通过 master 的协议栈进行 L2 处理和路由
- RX: **不接收组播/广播** (节省 CPU)
- 更适合高性能容器场景

### L3S 模式 (`IPVLAN_MODE_L3S`)

L3S = L3-Symmetric，与 L3 类似，但支持 iptables/netfilter conntrack。

```c
// drivers/net/ipvlan/ipvlan_l3s.c
// 注册 nf_hooks 在多个 netfilter 钩子点
static const struct nf_hook_ops ipvlan_l3s_nfops[] = {
    {
        .hook     = ipvlan_l3s_nf_hook,
        .pf       = NFPROTO_IPV4,
        .hooknum  = NF_INET_LOCAL_IN,
        .priority = INT_MIN,
    },
    {
        .hook     = ipvlan_l3s_nf_hook,
        .pf       = NFPROTO_IPV4,
        .hooknum  = NF_INET_LOCAL_OUT,
        .priority = INT_MAX,
    },
    // ... IPv6 类似
};
```

**特点**:
- 支持连接跟踪 (conntrack)
- 支持 iptables 规则
- 性能略低于 L3 模式

## 接收路径 (RX)

```c
// drivers/net/ipvlan/ipvlan_core.c:ipvlan_handle_frame()
rx_handler_result_t ipvlan_handle_frame(struct sk_buff **pskb)
{
    struct sk_buff *skb = *pskb;
    struct ipvl_port *port = ipvlan_port_get_rcu(skb->dev);

    if (!port)
        return RX_HANDLER_PASS;

    switch (port->mode) {
    case IPVLAN_MODE_L2:
        return ipvlan_handle_mode_l2(pskb, port);
    case IPVLAN_MODE_L3:
        return ipvlan_handle_mode_l3(pskb, port);
    case IPVLAN_MODE_L3S:
        return RX_HANDLER_PASS;  // L3S 使用 netfilter hooks
    }
    // ...
}

// L3 模式接收处理
static rx_handler_result_t ipvlan_handle_mode_l3(struct sk_buff **pskb,
                                                  struct ipvl_port *port)
{
    void *lyr3h;
    int addr_type;
    struct ipvl_addr *addr;
    struct sk_buff *skb = *pskb;
    rx_handler_result_t ret = RX_HANDLER_PASS;

    // 提取 L3 头部 (IP/ARP/ICMPv6)
    lyr3h = ipvlan_get_L3_hdr(port, skb, &addr_type);
    if (!lyr3h)
        goto out;

    // 根据目的 IP 查找目标 slave
    addr = ipvlan_addr_lookup(port, lyr3h, addr_type, true);
    if (addr)
        ret = ipvlan_rcv_frame(addr, pskb, false);

out:
    return ret;
}
```

**关键机制**:
- 使用 `rx_handler` 机制在 master 设备收包时介入
- 通过 **IP 地址哈希表** 快速查找目标 slave
- 支持 IPv4/IPv6/ARP/NDISC 协议

## IP 地址管理

```c
// drivers/net/ipvlan/ipvlan_core.c:ipvlan_addr_lookup()
struct ipvl_addr *ipvlan_addr_lookup(struct ipvl_port *port, void *lyr3h,
                                     int addr_type, bool use_dest)
{
    struct ipvl_addr *addr = NULL;

    switch (addr_type) {
    case IPVL_IPV6: {
        struct ipv6hdr *ip6h = (struct ipv6hdr *)lyr3h;
        struct in6_addr *i6addr = use_dest ? &ip6h->daddr : &ip6h->saddr;
        addr = ipvlan_ht_addr_lookup(port, i6addr, true);
        break;
    }
    case IPVL_IPV4: {
        struct iphdr *ip4h = (struct iphdr *)lyr3h;
        __be32 *i4addr = use_dest ? &ip4h->daddr : &ip4h->saddr;
        addr = ipvlan_ht_addr_lookup(port, i4addr, false);
        break;
    }
    case IPVL_ARP: {
        // ARP 处理：根据 target IP 查找
        struct arphdr *arph = (struct arphdr *)lyr3h;
        // ...
        addr = ipvlan_ht_addr_lookup(port, &dip, false);
        break;
    }
    // ...
    }
    return addr;
}
```

## 通信模式标志

| 标志 | 说明 |
|------|------|
| `bridge` (默认) | slave 间可以直接通信 |
| `private` | slave 间禁止通信 |
| `vepa` | 所有流量强制发送到外部交换机 (802.1Qbg) |

```c
// bridge 模式：slave 间直接转发
if (addr) {
    if (ipvlan_is_private(ipvlan->port)) {
        consume_skb(skb);  // private 模式直接丢弃
        return NET_XMIT_DROP;
    }
    ipvlan_rcv_frame(addr, &skb, true);  // bridge 模式直接转发
    return NET_XMIT_SUCCESS;
}
```

## 使用场景选择

根据内核文档和实践经验：

| 场景 | 推荐方案 |
|------|----------|
| 交换机限制单端口 MAC 数量 | ipvlan |
| 大量容器导致 promiscuous 模式性能下降 | ipvlan |
| 不可信容器可能滥用/伪造 L2 | ipvlan (共享 MAC 更安全) |
| 需要独立 MAC 地址 | macvlan |
| 需要组播/广播支持 | macvlan 或 ipvlan L2 模式 |

## 测试验证

使用 `net/ipvlan.sh` 进行测试:

```bash
# 创建两个网络命名空间
sudo ip netns add ns1
sudo ip netns add ns2

# 在物理接口上创建 ipvlan L2 模式
sudo ip link add link ens5 name ipvl1 type ipvlan mode l2
sudo ip link add link ens5 name ipvl2 type ipvlan mode l2

# 移动到对应 namespace
sudo ip link set ipvl1 netns ns1
sudo ip link set ipvl2 netns ns2

# 配置 IP 并启用
sudo ip netns exec ns1 ip addr add 10.0.29.1/16 dev ipvl1
sudo ip netns exec ns1 ip link set ipvl1 up

sudo ip netns exec ns2 ip addr add 10.0.29.2/16 dev ipvl2
sudo ip netns exec ns2 ip link set ipvl2 up

# 测试连通性
sudo ip netns exec ns1 ping 10.0.29.2
```

### 内核函数调用链观察

使用 bpftrace/perf 可以观察到 ipvlan 的 TX 路径:

```
ipvlan_start_xmit+5
xmit_one.constprop.0+93
dev_hard_start_xmit+85
__dev_queue_xmit+1753
ip_finish_output2+587
ip_output+99
__ip_queue_xmit+873
__tcp_transmit_skb+2725
tcp_write_xmit+705
tcp_sendmsg_locked+1113
```

## 未来趋势与总结

### 技术演进方向

**ipvlan 的困境与机遇**:

```
2014-2018: 诞生期
  - Google 开源，解决内部大规模容器部署问题
  - Docker/K8s 社区开始关注

2018-2022: 被 overlay 网络压制
  - Calico/Flannel/Cilium 等 overlay 方案成为 K8s 主流
  - 云厂商 CNI 直接对接 VPC，ipvlan 优势不明显
  - 使用场景局限于裸金属+超大规模部署

2022-至今: eBPF 时代的边缘化
  - Cilium 使用 eBPF 实现更高效的数据平面
  - ipvlan 的 L3 模式优势被 eBPF 替代
  - 但在无 eBPF 支持的老内核中仍有价值
```

**ipvlan 不会消失，但会 niche 化**:
1. **老系统维护**: 大量 4.x 内核的生产环境无法使用 eBPF
2. **简单场景**: 不需要完整的 K8s CNI，单节点 Docker 部署
3. **NFV 领域**: 电信设备商的传统方案延续

### 选型决策建议

```
┌─────────────────────────────────────────────────────────────────┐
│                     ipvlan 适用性评估                            │
├─────────────────────────────────────────────────────────────────┤
│  ✅ 使用 ipvlan                                                  │
│     - 裸金属部署，单机 > 500 容器                                 │
│     - 交换机严格限制 MAC 数量（<32）                              │
│     - 需要直连 VPC，不需要 NAT                                    │
│     - 追求极致网络性能，无 overlay 预算                           │
│                                                                 │
│  ❌ 不使用 ipvlan                                                │
│     - 公有云环境（AWS/Azure/GCP）                                 │
│     - 已使用 Calico/Cilium 等 overlay 方案                        │
│     - 需要容器间复杂网络策略（NetworkPolicy）                     │
│     - 运维团队不熟悉底层网络调试                                  │
│     - 需要 DHCP 自动分配 IP                                       │
└─────────────────────────────────────────────────────────────────┘
```

### 一句话总结

> **ipvlan 是容器网络演进中的"中间态"技术** —— 它比 bridge 快，比 macvlan 省 MAC，比 SR-IOV 简单；但在 overlay 网络的便利性和 eBPF 的高性能双重挤压下，其使用场景日益收缩，仅在特定裸金属+大规模+直连物理网络的场景中保留价值。

## 参考资料

- 内核文档: `Documentation/networking/ipvlan.rst`
- 源码: `drivers/net/ipvlan/`
- Red Hat 文档: https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/8/html/configuring_and_managing_networking/getting-started-with-ipvlan
- Google 内部容器网络演进: https://www.youtube.com/watch?v=0kW8YKT9kVM

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
