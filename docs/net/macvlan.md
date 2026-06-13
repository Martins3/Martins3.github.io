# macvlan
<!-- b81f68e6-5b65-465f-9ec2-654fa51ea4d8 -->

关联话题，./ipvlan.md

## 设计哲学与演进背景

### 为什么需要 macvlan？

macvlan 于 2007 年引入内核（Patrick McHardy），早于容器时代，最初设计目的是**替代 Linux Bridge 提供更高性能的虚拟化网络**。

**核心矛盾: Bridge 的 "万能" 带来的性能损耗**

```
Linux Bridge 设计目标:
- 学习 MAC 地址，构建转发表
- 支持 STP（生成树协议）防止环路
- 支持 VLAN 划分
- 支持 multicast snooping
-  netfilter 集成（iptables）

问题: 对于简单场景（如容器直连物理网络），这些功能都是 overhead
```

**macvlan 的设计哲学: 做减法**

```
┌─────────────────────────────────────────────────────────────┐
│                    Linux Bridge (万能)                       │
│  ┌─────────┐                                               │
│  │  学习   │── MAC 表学习/老化                              │
│  │  转发   │── 查表、洪泛、STP                              │
│  │  过滤   │── iptables、ebtables                           │
│  │  隔离   │── VLAN、广播域                                 │
│  └─────────┘                                               │
│  特点: 功能全面，适合复杂拓扑                                 │
│  代价: 软件转发，CPU 消耗，延迟增加                           │
└─────────────────────────────────────────────────────────────┘
                              ↓ 简化
┌─────────────────────────────────────────────────────────────┐
│                    macvlan (专用)                            │
│  ┌─────────┐                                               │
│  │  直通   │── 直接绑定物理网卡，无软件转发                  │
│  │  哈希   │── 简单 MAC 哈希表定位目标设备                   │
│  │  模式   │── 5 种通信模式应对不同场景                      │
│  └─────────┘                                               │
│  特点: 极简设计，性能优先                                     │
│  代价: 功能单一，需要外部配合                                 │
└─────────────────────────────────────────────────────────────┘
```

### 解决了什么问题

**1. Bridge 的性能瓶颈**
```
场景: 10G/25G/100G 网卡，高性能容器/VM
Bridge 问题:
  - 软中断瓶颈（单核处理）
  - 内存拷贝（skb 操作）
  - netfilter 钩子遍历

macvlan 优化:
  - 绕过 bridge，直接调用 dev_queue_xmit
  - 可选的硬件 L2 转发卸载（NETIF_F_HW_L2FW_DOFFLOAD）
  - 无 iptables 遍历（除非指定）

实测性能提升: 10-50%（取决于包大小和流量模式）
```

**2. 网络拓扑简化**
```
传统方案（Bridge）:
容器 ──► veth ──► bridge ──► eth0
        3 个设备，2 次转发

macvlan 方案:
容器 ──► macvlan ──► eth0
        2 个设备，1 次转发，代码路径更短
```

**3. 真正的 L2 隔离**
```
与 Bridge 对比:
- Bridge: 所有端口在同一广播域，依赖 VLAN 隔离
- macvlan: slave 设备对物理网络"可见"为独立网卡，广播域由物理交换机控制

优势: 与现有物理网络架构无缝集成，无需重新设计 VLAN
```

### 带来了什么问题

**1. MAC 地址管理困境**
```
问题: 每个 slave 需要独立 MAC 地址

限制:
- 交换机端口安全: 默认限制 1/4/32/64 个 MAC，超出会触发保护（shutdown/alert）
- MAC 表容量: 交换机 CAM 表有限，大量 MAC 可能导致洪泛
- 云厂商限制: AWS EC2 限制 ENI 数量，间接限制 macvlan 数量

实际案例:
某公司在 Cisco 交换机上部署 macvlan 容器，
默认 port-security maximum 1，导致频繁端口关闭，
需要手动调整或关闭 port-security，增加运维负担
```

**2. 通信模式复杂度高**
```
5 种模式选择困难:
- VEPA: 需要交换机 hairpin 支持（多数默认关闭）
- Bridge: 同主机通信ok，跨主机依赖外部
- Private: 完全隔离，调试困难
- Passthru: 只能有一个 slave，使用场景狭窄
- Source: 需要手动维护源 MAC 列表

对比: Bridge 模式简单，plug-and-play
```

**3. 与容器编排的集成问题**
```
Docker:
- 支持 macvlan network: docker network create -d macvlan
- 但默认 bridge 网络仍是主流
- macvlan 容器无法与主机通信（设计限制，除非使用 host 模式）

Kubernetes:
- 无原生 macvlan CNI（需第三方如 multus + macvlan CNI）
- Service 流量需要额外处理（kube-proxy 与 macvlan 兼容性）
- Pod 无法访问 ClusterIP（因为 bypass 了主机网络栈）
```

**4. 可观测性降低**
```
问题: 流量直接走物理网卡，主机上难以监控

对比 Bridge:
- bridge: tcpdump -i br0 可看到所有容器流量
- macvlan: 只能在每个 slave 上分别抓包，或依赖交换机端口镜像

影响: 安全审计、流量分析、故障排查困难
```

### 生产环境使用情况

**使用频率: 中等，特定场景较常见**

| 场景 | 普及度 | 说明 |
|------|-------|------|
| **Docker 独立部署** | 中等 | 需要直连物理网络的容器化应用 |
| **Kubernetes** | 较低 | 需 Multus 等多网卡方案配合 |
| **虚拟机+容器混合** | 中等 | VM 内运行 macvlan 容器 |
| **传统 IT 虚拟化** | 较低 | 已被 OVS/Linux Bridge 主导 |
| **网络设备模拟** | 较高 | 模拟多网卡设备，测试场景 |

**生产使用的典型限制**:
1. **规模限制**: 单节点通常不超过 50-100 个 macvlan 设备（MAC 地址管理）
2. **网络团队阻力**: 需要与网络团队协调 MAC 地址分配和交换机配置
3. **云环境不友好**: 公有云不支持自定义 MAC，macvlan 无法使用

### 主要应用场景

**场景 1: 传统企业容器化（裸金属）**
```
环境: 自有数据中心，物理服务器，传统三层网络
需求: 容器需要与遗留系统在同一 L2 网络互通
方案: macvlan bridge 模式
优势:
  - 容器直接获取物理网段子网 IP
  - 遗留系统无需改造即可访问容器
  - 性能优于 NAT

典型案例:
- 传统金融企业核心系统容器化
- 制造业工控系统与容器混合部署
- 运营商内部系统上云（私有云）
```

**场景 2: 网络功能虚拟化（NFV）**
```
环境: 电信云、运营商边缘计算
需求: VNF（虚拟网络功能）需要直接处理物理流量
方案: macvlan passthru 或 bridge 模式
优势:
  - 虚拟防火墙、LB、路由器可直接处理 L2 帧
  - 支持 promiscuous 模式（抓包、IDS）

典型案例:
- OpenStack Neutron 部分部署使用 macvlan
- 5G UPF（用户面功能）容器化部署
```

**场景 3: 开发测试环境**
```
环境: 开发者的物理机或测试服务器
需求: 模拟多主机网络，测试分布式应用
方案: macvlan + 多个容器模拟集群
优势:
  - 每个容器有独立 IP，可直接从外部访问
  - 避免端口映射（-p 8080:80）的繁琐
  - 接近生产网络拓扑

典型案例:
- 微服务本地开发，直接访问服务 IP
- Kubernetes 多节点模拟（单物理机）
- 网络协议栈学习和测试
```

**场景 4: 特定网络设备模拟**
```
需求: 一个容器需要多个网络接口，模拟路由器/防火墙
方案: macvlan + multus（K8s 多网卡 CNI）
优势: 每个接口独立 MAC，可被外部识别为独立设备

典型案例:
- 虚拟路由器（如 VyOS、FRR）容器化运行
- 安全设备（IDS/IPS）流量镜像分析
- 网络实验室教学环境
```

### macvlan vs ipvlan: 如何选择

```
决策树:
                    ┌─────────────────┐
                    │ 需要直连物理网络? │
                    └────────┬────────┘
                             │
              ┌──────────────┴──────────────┐
             否                              是
              │                               │
    ┌─────────▼──────────┐         ┌──────────▼──────────┐
    │ 使用 Bridge/VXLAN  │         │ 交换机限制 MAC 数量? │
    │ (Docker/K8s 默认)  │         └──────────┬──────────┘
    └────────────────────┘                    │
                              ┌───────────────┴───────────────┐
                             是                               否
                              │                               │
                    ┌─────────▼──────────┐         ┌──────────▼──────────┐
                    │   使用 ipvlan       │         │ 需要独立 MAC 可见性? │
                    │   (共享 MAC)        │         └──────────┬──────────┘
                    └────────────────────┘                    │
                                                  ┌───────────┴───────────┐
                                                 是                       否
                                                  │                       │
                                        ┌─────────▼──────────┐  ┌─────────▼──────────┐
                                        │   使用 macvlan      │  │  使用 ipvlan        │
                                        │   (独立 MAC)        │  │  (更简单，性能更好)  │
                                        └────────────────────┘  └────────────────────┘
```

**总结建议**:
- **默认选 ipvlan**: 除非明确需要独立 MAC（如 DHCP、某些安全策略）
- **macvlan 作为备选**: 当 ipvlan 因某种原因不工作时（如需要 ARP）
- **都无效时**: 回归 Bridge + veth，兼容性和灵活性最优

## 核心概念

macvlan 是 Linux 内核提供的虚拟网络设备，允许在**单个物理网络接口上创建多个具有独立 MAC 地址的虚拟接口**。每个虚拟接口都可以像独立的物理网卡一样工作。

```
┌─────────────────────────────────────────────────────────────┐
│                         Host                                │
│  ┌─────────────┐      ┌─────────────┐      ┌─────────────┐ │
│  │   NS: ns1   │      │   NS: ns2   │      │    Host     │ │
│  │  ┌───────┐  │      │  ┌───────┐  │      │  ┌───────┐  │ │
│  │  │macv1  │  │      │  │macv2  │  │      │  │macv0  │  │ │
│  │  │MAC:A  │  │      │  │MAC:B  │  │      │  │MAC:C  │  │ │
│  │  └───┬───┘  │      │  └───┬───┘  │      │  └───┬───┘  │ │
│  └──────┼──────┘      └──────┼──────┘      └──────┼──────┘ │
│         │                    │                    │         │
│         └────────────────────┼────────────────────┘         │
│                              │                              │
│                    ┌─────────┴──────────┐                   │
│                    │   eth0 (master)    │                   │
│                    │   MAC: Master      │                   │
│                    └─────────┬──────────┘                   │
│                              │                              │
└──────────────────────────────┼──────────────────────────────┘
                               │
                         ┌─────┴─────┐
                         │  Switch   │
                         └───────────┘
```

## 与 Bridge + veth 对比

| 特性 | macvlan | bridge + veth |
|------|---------|---------------|
| 架构 | 直接绑定物理网卡 | 需要虚拟交换机 |
| 性能 | 更高（无桥接开销） | 有桥接处理开销 |
| 隔离性 | L2 隔离 | L2 隔离 |
| 灵活性 | slave 间通信需特殊模式 | 天然支持同主机通信 |
| 外部可见性 | 每个 slave 独立 MAC | 通过 NAT/路由隐藏 |
| 适用场景 | 需要直接暴露容器到物理网络 | 内部网络隔离 |

## 内核数据结构

```c
// drivers/net/macvlan.c
struct macvlan_port {
    struct net_device   *dev;                    // master 设备
    struct hlist_head   vlan_hash[MACVLAN_HASH_SIZE];   // MAC 地址哈希表
    struct list_head    vlans;                   // 所有 macvlan slave 链表
    struct sk_buff_head bc_queue;                // 广播队列
    struct work_struct  bc_work;                 // 广播处理工作队列
    u32                 flags;                   // PASSTHRU 等标志
    int                 count;                   // slave 数量
    DECLARE_BITMAP(bc_filter, MACVLAN_MC_FILTER_SZ);
    DECLARE_BITMAP(mc_filter, MACVLAN_MC_FILTER_SZ);
    unsigned char       perm_addr[ETH_ALEN];     // 原始 MAC
};

struct macvlan_dev {
    struct net_device   *dev;           // 虚拟设备
    struct macvlan_port *port;          // 所属端口
    struct net_device   *lowerdev;      // 底层设备
    struct hlist_node   hlist;          // 哈希表节点
    struct list_head    list;           // 链表节点
    int                 mode;           // 工作模式
    int                 flags;
    struct vlan_pcpu_stats __percpu *pcpu_stats;
    void                *accel_priv;    // 硬件加速私有数据
    struct netpoll      *netpoll;
    u32                 set_features;
    int                 macaddr_count;
    u32                 bc_queue_len_req;
};

// MAC 地址哈希计算
static u32 macvlan_eth_hash(const unsigned char *addr)
{
    u64 value = get_unaligned((u64 *)addr);
    // 只取 6 字节 MAC 地址
    #ifdef __BIG_ENDIAN
        value >>= 16;
    #else
        value <<= 16;
    #endif
    return hash_64(value, MACVLAN_HASH_BITS);
}
```

## 五种工作模式

### 1. VEPA 模式 (`MACVLAN_MODE_VEPA`)

Virtual Ethernet Port Aggregator，默认模式。

```
        ┌─────────┐         ┌─────────┐
        │  macv1  │         │  macv2  │
        └───┬─────┘         └───┬─────┘
            │                   │
            └─────────┬─────────┘
                      │
                ┌─────┴─────┐
                │   eth0    │
                └─────┬─────┘
                      │
                ┌─────┴─────┐
                │  Switch   │ ←  Hairpin 模式必须支持
                └─────┬─────┘
                      │
            ┌─────────┼─────────┐
            ▼         ▼         ▼
        其他主机  其他主机  其他主机
```

**特点**:
- 所有流量（包括 slave 间通信）都发送到外部交换机
- 需要交换机支持 **hairpin 模式** (将流量反射回源端口)
- 适用于外部交换机做流量监控/策略的场景

```c
// macvlan_queue_xmit() 中 VEPA 模式处理
static int macvlan_queue_xmit(struct sk_buff *skb, struct net_device *dev)
{
    const struct macvlan_dev *vlan = netdev_priv(dev);
    const struct macvlan_port *port = vlan->port;

    // VEPA 模式下，即使是同一 port 内的目标，也发送到外部
    // 由外部交换机决定是否 hairpin 回来
    skb->dev = vlan->lowerdev;
    return dev_queue_xmit_accel(skb, netdev_get_sb_channel(dev) ? dev : NULL);
}
```

### 2. Bridge 模式 (`MACVLAN_MODE_BRIDGE`)

```
        ┌─────────┐         ┌─────────┐
        │  macv1  │◄───────►│  macv2  │
        └───┬─────┘  直接通信 └───┬─────┘
            │                   │
            └─────────┬─────────┘
                      │
                ┌─────┴─────┐
                │   eth0    │ ← 外部流量才经过这里
                └───────────┘
```

**特点**:
- slave 间直接通信，不经过物理网卡
- 类似软件交换机，但更高效
- 适用于同一主机内容器/VM 间高频通信

```c
// bridge 模式发送处理
if (vlan->mode == MACVLAN_MODE_BRIDGE) {
    const struct ethhdr *eth = skb_eth_hdr(skb);

    // 组播/广播：向所有 bridge 模式的 slave 发送
    if (is_multicast_ether_addr(eth->h_dest)) {
        skb_reset_mac_header(skb);
        macvlan_broadcast(skb, port, dev, MACVLAN_MODE_BRIDGE);
        goto xmit_world;  // 同时发送到外部
    }

    // 单播：查找目标 MAC
    dest = macvlan_hash_lookup(port, eth->h_dest);
    if (dest && dest->mode == MACVLAN_MODE_BRIDGE) {
        // 直接转发到目标 slave，走内部路径
        dev_forward_skb(vlan->lowerdev, skb);
        return NET_XMIT_SUCCESS;
    }
}
xmit_world:
// 发送到外部网络
skb->dev = vlan->lowerdev;
return dev_queue_xmit_accel(skb, ...);
```

### 3. Private 模式 (`MACVLAN_MODE_PRIVATE`)

**特点**:
- 禁止 slave 间任何通信
- 每个 slave 只能与外部通信
- 强隔离场景

### 4. Passthru 模式 (`MACVLAN_MODE_PASSTHRU`)

**特点**:
- 只有一个 slave，直接与 master 共享 MAC
- slave 完全接管 master 的网络功能
- 用于 SR-IOV 等硬件虚拟化场景

```c
static inline bool macvlan_passthru(const struct macvlan_port *port)
{
    return port->flags & MACVLAN_F_PASSTHRU;
}

// passthru 模式下直接获取第一个（也是唯一一个）slave
if (macvlan_passthru(port))
    vlan = list_first_or_null_rcu(&port->vlans, struct macvlan_dev, list);
else
    vlan = macvlan_hash_lookup(port, eth->h_dest);
```

### 5. Source 模式 (`MACVLAN_MODE_SOURCE`)

**特点**:
- 基于源 MAC 地址过滤
- 只允许特定源 MAC 的流量进入
- 可配合 `macvlan_source` 配置允许的源地址列表

```c
// Source 模式：检查源 MAC 是否在允许列表中
static bool macvlan_forward_source(struct sk_buff *skb,
                                   struct macvlan_port *port,
                                   const unsigned char *addr)
{
    struct macvlan_source_entry *entry;
    u32 idx = macvlan_eth_hash(addr);
    struct hlist_head *h = &port->vlan_source_hash[idx];
    bool consume = false;

    hlist_for_each_entry_rcu(entry, h, hlist) {
        if (ether_addr_equal_64bits(entry->addr, addr)) {
            struct macvlan_dev *vlan = rcu_dereference(entry->vlan);
            if (!vlan)
                continue;
            if (vlan->flags & MACVLAN_FLAG_NODST)
                consume = true;
            macvlan_forward_source_one(skb, vlan);
        }
    }
    return consume;
}
```

## 接收路径 (RX)

```c
// drivers/net/macvlan.c:macvlan_handle_frame()
static rx_handler_result_t macvlan_handle_frame(struct sk_buff **pskb)
{
    struct macvlan_port *port;
    struct sk_buff *skb = *pskb;
    const struct ethhdr *eth = eth_hdr(skb);
    const struct macvlan_dev *vlan;
    const struct macvlan_dev *src;

    // 跳过 loopback 包
    if (unlikely(skb->pkt_type == PACKET_LOOPBACK))
        return RX_HANDLER_PASS;

    port = macvlan_port_get_rcu(skb->dev);

    // ========== 组播/广播处理 ==========
    if (is_multicast_ether_addr(eth->h_dest)) {
        // IP 分片重组检查
        skb = ip_check_defrag(dev_net(skb->dev), skb, IP_DEFRAG_MACVLAN);
        if (!skb)
            return RX_HANDLER_CONSUMED;

        // Source 模式：根据源 MAC 转发
        if (macvlan_forward_source(skb, port, eth->h_source)) {
            kfree_skb(skb);
            return RX_HANDLER_CONSUMED;
        }

        src = macvlan_hash_lookup(port, eth->h_source);
        if (src && src->mode != MACVLAN_MODE_VEPA &&
            src->mode != MACVLAN_MODE_BRIDGE) {
            // 源是 private/passthru/source 模式，只发给源
            vlan = src;
            ret = macvlan_broadcast_one(skb, vlan, eth, 0) ?: __netif_rx(skb);
            return RX_HANDLER_CONSUMED;
        }

        // 广播到所有感兴趣的 slave
        macvlan_broadcast_enqueue(port, src, skb);
        return RX_HANDLER_PASS;  // 同时让 master 也能收到
    }

    // ========== 单播处理 ==========
    // Source 模式检查
    if (macvlan_forward_source(skb, port, eth->h_source)) {
        kfree_skb(skb);
        return RX_HANDLER_CONSUMED;
    }

    // Passthru 模式直接取第一个 slave
    if (macvlan_passthru(port))
        vlan = list_first_or_null_rcu(&port->vlans, struct macvlan_dev, list);
    else
        // 根据目的 MAC 查找 slave
        vlan = macvlan_hash_lookup(port, eth->h_dest);

    if (!vlan || vlan->mode == MACVLAN_MODE_SOURCE)
        return RX_HANDLER_PASS;  // 没有匹配，给 master

    // 转发到目标 slave
    dev = vlan->dev;
    if (unlikely(!(dev->flags & IFF_UP))) {
        kfree_skb(skb);
        return RX_HANDLER_CONSUMED;
    }

    skb->dev = dev;
    skb->pkt_type = PACKET_HOST;
    return RX_HANDLER_ANOTHER;  // 继续协议栈处理
}
```

## 广播/组播处理机制

```c
// 广播处理使用工作队列异步处理
static void macvlan_process_broadcast(struct work_struct *w)
{
    struct macvlan_port *port = container_of(w, struct macvlan_port, bc_work);
    struct sk_buff *skb;
    struct sk_buff_head list;

    __skb_queue_head_init(&list);

    // 从队列取出待处理的广播包
    spin_lock_bh(&port->bc_queue.lock);
    skb_queue_splice_tail_init(&port->bc_queue, &list);
    spin_unlock_bh(&port->bc_queue.lock);

    while ((skb = __skb_dequeue(&list))) {
        const struct macvlan_dev *src = MACVLAN_SKB_CB(skb)->src;

        rcu_read_lock();
        macvlan_multicast_rx(port, src, skb);
        rcu_read_unlock();

        if (src)
            dev_put(src->dev);
        consume_skb(skb);
    }
}

// 根据源设备和模式决定广播范围
static void macvlan_multicast_rx(const struct macvlan_port *port,
                                 const struct macvlan_dev *src,
                                 struct sk_buff *skb)
{
    if (!src)
        // 外部帧：发给所有 slave
        macvlan_broadcast(skb, port, NULL,
                  MACVLAN_MODE_PRIVATE | MACVLAN_MODE_VEPA |
                  MACVLAN_MODE_PASSTHRU | MACVLAN_MODE_BRIDGE);
    else if (src->mode == MACVLAN_MODE_VEPA)
        // VEPA 源：发给其他 VEPA 和 BRIDGE
        macvlan_broadcast(skb, port, src->dev,
                  MACVLAN_MODE_VEPA | MACVLAN_MODE_BRIDGE);
    else
        // BRIDGE 源：发给 VEPA (BRIDGE 已经直接收到了)
        macvlan_broadcast(skb, port, src->dev, MACVLAN_MODE_VEPA);
}
```

## 硬件加速支持

```c
// macvlan_open() 中尝试硬件 L2 转发卸载
if (lowerdev->features & NETIF_F_HW_L2FW_DOFFLOAD) {
    vlan->accel_priv =
          lowerdev->netdev_ops->ndo_dfwd_add_station(lowerdev, dev);
}

// 如果硬件加速失败，回退到软件哈希
if (IS_ERR_OR_NULL(vlan->accel_priv)) {
    vlan->accel_priv = NULL;
    err = dev_uc_add(lowerdev, dev->dev_addr);  // 添加单播地址到下层
}
```

## 设备操作 ops

```c
static const struct net_device_ops macvlan_netdev_ops = {
    .ndo_init           = macvlan_init,
    .ndo_uninit         = macvlan_uninit,
    .ndo_open           = macvlan_open,
    .ndo_stop           = macvlan_stop,
    .ndo_start_xmit     = macvlan_start_xmit,
    .ndo_change_mtu     = macvlan_change_mtu,
    .ndo_fix_features   = macvlan_fix_features,
    .ndo_change_rx_flags= macvlan_change_rx_flags,
    .ndo_set_mac_address= macvlan_set_mac_address,
    .ndo_set_rx_mode    = macvlan_set_mac_lists,
    .ndo_get_stats64    = macvlan_dev_get_stats64,
    .ndo_validate_addr  = eth_validate_addr,
    .ndo_vlan_rx_add_vid= macvlan_vlan_rx_add_vid,
    .ndo_vlan_rx_kill_vid= macvlan_vlan_rx_kill_vid,
    .ndo_fdb_add        = macvlan_fdb_add,
    .ndo_fdb_del        = macvlan_fdb_del,
    .ndo_get_iflink     = macvlan_dev_get_iflink,
};
```

## 使用示例

使用 `net/macvlan.sh` 测试:

```bash
# 创建 macvlan bridge 模式
sudo ip link add macv1 link ens5 type macvlan mode bridge
sudo ip link add macv2 link ens5 type macvlan mode bridge

# 移动到 netns
sudo ip netns add ns1
sudo ip netns add ns2
sudo ip link set macv1 netns ns1
sudo ip link set macv2 netns ns2

# 配置网络
sudo ip netns exec ns1 ip addr add 10.0.29.101/16 dev macv1
sudo ip netns exec ns1 ip link set macv1 up

sudo ip netns exec ns2 ip addr add 10.0.29.102/16 dev macv2
sudo ip netns exec ns2 ip link set macv2 up

# 测试
sudo ip netns exec ns1 ping 10.0.29.102
```

## 模式选择指南

| 场景 | 推荐模式 |
|------|----------|
| 需要外部交换机做流量分析 | VEPA |
| 同一主机内高频容器通信 | Bridge |
| 强隔离要求 | Private |
| 硬件虚拟化/SR-IOV | Passthru |
| 源 MAC 过滤需求 | Source |

## 与 ipvlan 的对比选择

| 场景 | 推荐 |
|------|------|
| 交换机限制 MAC 数量 | ipvlan |
| 需要独立 MAC 可见性 | macvlan |
| 大量容器避免 promiscuous | ipvlan |
| 需要传统 L2 网络行为 | macvlan |
| 不可信容器环境 | ipvlan (共享 MAC 更安全) |

## 未来趋势与总结

### macvlan 的地位变迁

**从"bridge 杀手"到"niche 工具"**:

```
2007: 诞生
  └─ 目标: 替代 Linux Bridge，提供更高性能

2013-2016: Docker 早期
  └─ 容器网络方案之一，与 bridge 竞争
  └─ 因"无法与主机通信"问题被边缘化

2017-2020: Kubernetes 时代
  └─ CNI 生态成熟，overlay 网络（Flannel/Calico）成为主流
  └─ macvlan 仅在特定场景使用（需要直连物理网络）

2021-至今: eBPF 和智能网卡时代
  └─ Cilium + eBPF 提供更高性能
  └─ 智能网卡（AWS ENA/Azure Mellanox）提供类似 SR-IOV 能力
  └─ macvlan 使用场景进一步收缩
```

**macvlan 的长期价值**:

1. **简单性**: 无需复杂的控制平面，适合边缘/IoT 场景
2. **兼容性**: 所有 Linux 发行版支持，无额外依赖
3. **教学价值**: 理解 Linux 网络虚拟化的入门工具
4. **遗留系统**: 大量既有部署的维护需求

### macvlan 与新一代技术的对比

| 技术 | 性能 | 复杂度 | 适用场景 | 与 macvlan 的关系 |
|------|------|--------|----------|------------------|
| **macvlan** | 高 | 低 | 直连物理网络 | 基准方案 |
| **ipvlan** | 更高 | 低 | MAC 受限环境 | macvlan 的 MAC 优化版 |
| **Cilium/eBPF** | 极高 | 中 | 云原生网络 | 替代 macvlan 的性能优势 |
| **SR-IOV** | 最高 | 高 | NFV/高性能计算 | macvlan 的硬件加速版 |
| **DPU/IPU** | 最高 | 高 | 超大规模数据中心 | 彻底绕过内核网络栈 |

**结论**: 随着 eBPF 和智能网卡的普及，macvlan 的性能优势不再独特，但其简单性和兼容性确保它在轻量级场景中长期存在。

### 终极选型建议

```
┌──────────────────────────────────────────────────────────────────────┐
│                    容器网络技术选型决策树                               │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  1. 是否在公有云（AWS/Azure/GCP/阿里云）?                             │
│     ├─ 是 → 使用云厂商 CNI（AWS VPC CNI/Azure CNI 等）               │
│     └─ 否 → 继续                                                      │
│                                                                      │
│  2. 是否需要容器与外部物理网络直连（无 NAT）?                         │
│     ├─ 否 → 使用 Calico/Cilium/Flannel（overlay 或 BGP）             │
│     └─ 是 → 继续                                                      │
│                                                                      │
│  3. 交换机是否限制单端口 MAC 数量?                                    │
│     ├─ 是 → 使用 ipvlan                                               │
│     └─ 否 → 继续                                                      │
│                                                                      │
│  4. 是否需要独立 MAC 地址（DHCP/特定安全策略）?                       │
│     ├─ 是 → 使用 macvlan                                              │
│     └─ 否 → 优先 ipvlan，备用 macvlan                                │
│                                                                      │
│  5. 是否需要极致性能（10M+ PPS）?                                     │
│     ├─ 是 → 考虑 SR-IOV 或 DPU                                       │
│     └─ 否 → macvlan/ipvlan 足够                                       │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

### 一句话总结

> **macvlan 是 Linux 网络虚拟化的经典设计** —— 它以极简的架构实现了容器直连物理网络的需求，但在云原生时代，面对 overlay 网络的灵活性和 eBPF 的高性能双重挑战，其应用范围已收缩至特定场景；理解 macvlan 有助于深入掌握 Linux 网络命名空间和虚拟化原理，是网络工程师的必备知识。

## 好的，我的问题是?

vlan 岂不是，物理网卡中可以直接支持实现?

bridge 中也可以实现 vlan ?

## 基本调查

所以，无论哪个，都是为了模拟出来各种新的设备:
```c
static const struct net_device_ops macvlan_netdev_ops = {
	.ndo_init		= macvlan_init,
	.ndo_uninit		= macvlan_uninit,
	.ndo_open		= macvlan_open,
	.ndo_stop		= macvlan_stop,
	.ndo_start_xmit		= macvlan_start_xmit,
	.ndo_change_mtu		= macvlan_change_mtu,
	.ndo_fix_features	= macvlan_fix_features,
	.ndo_change_rx_flags	= macvlan_change_rx_flags,
	.ndo_set_mac_address	= macvlan_set_mac_address,
	.ndo_set_rx_mode	= macvlan_set_mac_lists,
	.ndo_get_stats64	= macvlan_dev_get_stats64,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_vlan_rx_add_vid	= macvlan_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= macvlan_vlan_rx_kill_vid,
	.ndo_fdb_add		= macvlan_fdb_add,
	.ndo_fdb_del		= macvlan_fdb_del,
	.ndo_fdb_dump		= ndo_dflt_fdb_dump,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= macvlan_dev_poll_controller,
	.ndo_netpoll_setup	= macvlan_dev_netpoll_setup,
	.ndo_netpoll_cleanup	= macvlan_dev_netpoll_cleanup,
#endif
	.ndo_get_iflink		= macvlan_dev_get_iflink,
	.ndo_features_check	= passthru_features_check,
	.ndo_hwtstamp_get	= macvlan_hwtstamp_get,
	.ndo_hwtstamp_set	= macvlan_hwtstamp_set,
};
```

```c
static const struct net_device_ops ipvlan_netdev_ops = {
	.ndo_init		= ipvlan_init,
	.ndo_uninit		= ipvlan_uninit,
	.ndo_open		= ipvlan_open,
	.ndo_stop		= ipvlan_stop,
	.ndo_start_xmit		= ipvlan_start_xmit,
	.ndo_fix_features	= ipvlan_fix_features,
	.ndo_change_rx_flags	= ipvlan_change_rx_flags,
	.ndo_set_rx_mode	= ipvlan_set_multicast_mac_filter,
	.ndo_get_stats64	= ipvlan_get_stats64,
	.ndo_vlan_rx_add_vid	= ipvlan_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= ipvlan_vlan_rx_kill_vid,
	.ndo_get_iflink		= ipvlan_get_iflink,
};
```

## macvtap 做什么的?
<!-- e7a827ba-8982-47d0-b8b7-9ecc09dbd86c -->

相对于 macvlan 而言:
```txt
  Macvlan（用于容器/命名空间）

  ┌────────────────────────────────────────┐
  │              物理网卡 eth0              │
  └────────────────────────────────────────┘
             │
      ┌──────┴──────┐
      ▼             ▼
  ┌─────────┐  ┌─────────┐
  │macvlan0 │  │macvlan1 │  ← 网络命名空间用
  │(netns)  │  │(netns)  │
  └────┬────┘  └────┬────┘
       │            │
    容器/NS1      容器/NS2

  Macvtap（用于虚拟机）

  ┌────────────────────────────────────────┐
  │              物理网卡 eth0              │
  └────────────────────────────────────────┘
             │
      ┌──────┴──────┐
      ▼             ▼
  ┌─────────┐  ┌─────────┐
  │macvtap0 │  │macvtap1 │  ← 字符设备 /dev/tapX
  │(/dev/   │  │(/dev/   │
  │ tap100) │  │ tap101) │
  └────┬────┘  └────┬────┘
       │            │
       ▼            ▼
    ┌─────┐      ┌─────┐
    │ VM1 │      │ VM2 │      ← 虚拟机通过 /dev/tapX 直接访问
    │virtio│      │virtio│
    └─────┘      └─────┘
```
的确，使用 net/demo/macvlan-demo.sh 测试，
可以发现这个结果就是

sudo ip netns exec macvlan-ns1 bash
进去之后，可以看到的结果是:
```txt
root@yyds:/home/martins3/data/vn/.worktrees/vlan/net/demo# ls -la /sys/class/net
total 0
drwxr-xr-x  2 root root 0 Mar 21 21:05 .
drwxr-xr-x 66 root root 0 Mar 21 21:05 ..
lrwxrwxrwx  1 root root 0 Mar 21 21:05 lo -> ../../devices/virtual/net/lo
lrwxrwxrwx  1 root root 0 Mar 21 21:05 macvlan0 -> ../../devices/virtual/net/macvlan0
```
但是在物理机中是没有这个的:

## 为什么 Macvtap 不能和宿主机通信？

// 简化原理：macvtap 的数据包处理路径
1. 虚拟机发送数据包
2. 到达 macvtap 设备
3. macvtap 直接转发到物理网卡（绕过宿主机的网络协议栈）
4. 宿主机的 IP 协议栈根本看不到这个数据包！

// OVS + tap 的数据包处理路径
1. 虚拟机发送数据包
2. 到达 tap 设备
3. 进入 OVS 网桥
4. OVS 根据流表决定：
   - 转发到另一台 VM
   - 转发到物理网卡
   - 转发到宿主机协议栈

## 类似的，也是存在 ipvtap 的
drivers/net/ipvlan/ipvtap.c

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
