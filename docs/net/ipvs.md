# ipvs

kube-proxy 需要使用，基于 netfilter

细节可以分析这个:
https://kubernetes.io/docs/reference/networking/virtual-ips/

## osi 模型
https://en.wikipedia.org/wiki/OSI_model

7.  Application layer
6.  Presentation layer
5.  Session layer
4.  Transport layer
3.  Network layer
2.  Data link layer
1.  Physical layer


## 4 层负载
[四层负载均衡漫谈](https://www.kawabangga.com/posts/5301) : 相当详细了

- https://www.learncloudnative.com/blog/2020-04-25-beginners-guide-to-gateways-proxies/ : 讲解网关

- https://github.com/facebookincubator/katran : 项目

### SLB
- SLB : Server Load Balancing ： https://answers.uillinois.edu/illinois/page.php?id=49949
    - 可以基于OpenResty构建

## LVS

话说，反向代理和 ld 有区别吗?

- https://www.alibabacloud.com/blog/load-balancing---linux-virtual-server-lvs-and-its-forwarding-modes_595724

- https://github.com/liexusong/linux-source-code-analyze/blob/master/lvs-principle-and-source-analysis-part1.md
- https://github.com/liexusong/linux-source-code-analyze/blob/master/lvs-principle-and-source-analysis-part2.md
  - 看这个代码分析，原来是 ipvs 的实现

- https://www.yuque.com/abser/kubernetes/tpg92n

## Deepseeek

> [!NOTE]
> 参考 Deepseeek ，有待验证

| 层级 | 名称 | 负载均衡技术 | 依据 | 典型设备/软件 |
|------|------|----------------|--------|----------------|
| L2 | 数据链路层 | 链路聚合（LAG）、MAC 地址哈希 | MAC 地址 | 交换机、NIC Teaming |
| L3 | 网络层 | 基于 IP 的哈希、ECMP | 源/目的 IP | 路由器、三层交换机 |
| L4 | 传输层 | 基于 TCP/UDP 五元组 | IP + 端口 | F5、LVS、Nginx (TCP) |
| L7 | 应用层 | 内容感知、URL/HTTP 头路由 | HTTP 请求内容 | Nginx、HAProxy、ALB |

> ⚠️ 注意：L5-L6 一般不单独用于负载均衡，通常与 L7 结合使用。我们重点讲 L2、L3、L4、L7。


## 一、第2层（数据链路层）负载均衡

### ✅ 主要技术：**链路聚合（Link Aggregation） / LACP / NIC Teaming**

- **原理**：
  - 将多个物理链路捆绑成一个逻辑链路（如 EtherChannel、802.3ad/LACP）。
  - 使用 **MAC 地址哈希**（源 MAC、目的 MAC 或两者）决定数据帧走哪条物理链路。
- **目的**：
  - 提高带宽、冗余、负载分担。
- **特点**：
  - 不改变 IP 或端口，纯二层转发。
  - 适用于交换机与服务器、交换机之间。
- **典型协议**：
  - IEEE 802.1AX（原 802.3ad）LACP（链路聚合控制协议）
- **设备支持**：
  - 企业级交换机（Cisco、Huawei）、服务器网卡绑定（Linux bonding）

🔹 示例：服务器双网卡绑定到交换机，流量在两条链路间分担。

## 二、第3层（网络层）负载均衡

### ✅ 主要技术：**基于 IP 的负载均衡、ECMP（等价多路径路由）**

- **原理**：
  - 根据 **源 IP、目的 IP 或两者组合** 的哈希值选择路由路径。
  - 在多个等价路径之间分发流量。
- **技术实现**：
  - **ECMP（Equal-Cost Multi-Path Routing）**：
    - 路由器发现多条代价相同的路径时，可将流量分摊到这些路径上。
    - 常用于数据中心 spine-leaf 架构。
- **特点**：
  - 不关心端口或应用内容，只看 IP 地址。
  - 适合大规模网络流量分发。
- **设备支持**：
  - 三层交换机、路由器（Cisco、Juniper、华为）、BGP/OSPF 路由协议支持 ECMP。

🔹 示例：数据中心中，从客户端到服务器群的流量通过多个 spine 交换机分发。

## 三、第4层（传输层）负载均衡

### ✅ 主要技术：**基于 TCP/UDP 五元组的负载均衡**

- **五元组**：源IP、目的IP、源端口、目的端口、协议（TCP/UDP）
- **原理**：
  - 负载均衡器监听某个 VIP（虚拟 IP）和端口（如 80、443）。
  - 根据连接的五元组哈希或轮询算法，将连接转发到后端真实服务器。
  - 维护连接状态（如 NAT 映射），属于 **会话级负载均衡**。
- **调度算法**：
  - 轮询（Round Robin）
  - 加权轮询（Weighted RR）
  - 最小连接数（Least Connections）
  - 源地址哈希（Source IP Hash）——保持会话粘性
- **特点**：
  - 快速、高效，不解析应用层内容。
  - 适合 TCP/UDP 服务（如数据库、邮件、非 HTTP 应用）。
- **典型产品**：
  - Linux Virtual Server（LVS）
  - F5 BIG-IP L4 模式
  - Nginx（TCP/UDP 流模块）
  - Citrix ADC

🔹 示例：LVS 使用 DR（Direct Routing）或 NAT 模式分发 TCP 流量到多个 Web 服务器。

## 四、第7层（应用层）负载均衡

### ✅ 主要技术：**基于 HTTP/HTTPS 内容的负载均衡**

- **原理**：
  - 负载均衡器终止或代理客户端的 HTTP(S) 请求。
  - 解析 **URL、Host 头、Cookie、User-Agent、请求方法** 等应用层信息。
  - 根据内容决定转发到哪个后端服务。
- **功能强大**：
  - 支持 **内容路由**（如 `/api` → 后端 API 服务器，`/static` → 静态资源服务器）
  - 支持 **SSL 卸载**（HTTPS 解密）
  - 支持 **会话保持**（基于 Cookie）
  - 支持 **WAF、限流、重写** 等高级功能
- **典型产品**：
  - Nginx、Apache HTTP Server（反向代理）
  - HAProxy（应用层模式）
  - F5 BIG-IP L7 策略
  - AWS ALB（Application Load Balancer）
  - Azure Application Gateway
  - Kong、Traefik（云原生 API 网关）

🔹 示例：访问 `https://example.com/admin` 转发到管理后台，`/shop` 转发到电商服务。

## ❌ 第5层（会话层）和第6层（表示层）的负载均衡？

- 实际上，在现代网络中，**几乎没有专门基于 L5 或 L6 的负载均衡技术**。
- 这两层主要负责：
  - L5：会话建立与管理（如 RPC、NetBIOS）
  - L6：数据格式转换、加密（如 SSL/TLS 的表示层部分）
- 但注意：
  - **SSL/TLS 加密属于表示层（L6）**，但其处理通常被集成在 **L7 负载均衡器** 中（如 Nginx 做 HTTPS 解密）。
  - 所以“SSL 卸载”虽然是 L6 功能，但由 L7 设备实现。

> ✅ 结论：L5/L6 不独立用于负载均衡，功能被 L4 或 L7 设备吸收。

### ✅ 总结：各层负载均衡技术一览表

| 层级 | 技术名称 | 依据 | 特点 | 典型应用 |
|------|----------|------|--------|------------|
| **L2** | 链路聚合（LACP/NIC Teaming） | MAC 地址 | 物理链路冗余与带宽聚合 | 服务器双网卡、交换机互联 |
| **L3** | ECMP、IP 哈希 | 源/目的 IP | 路由级负载分担 | 数据中心 spine-leaf 架构 |
| **L4** | TCP/UDP 五元组负载均衡 | IP + 端口 | 连接级分发，高性能 | Web 服务器集群、数据库 |
| **L7** | 内容感知负载均衡 | URL、Header、Cookie | 应用级智能路由 | Web 应用、微服务、API 网关 |

### 💡 如何选择？

| 需求 | 推荐层级 |
|------|----------|
| 提高链路带宽和冗余 | L2（链路聚合） |
| 多路径路由分发 | L3（ECMP） |
| 高性能 TCP/UDP 负载 | L4（如 LVS） |
| Web 应用、微服务路由 | L7（如 Nginx、ALB） |
| HTTPS 卸载、安全策略 | L7 |

✅ **现代趋势**：
越来越多采用 **L4 + L7 混合架构**，例如：
- L4 负载均衡器（如 IPVS）做第一层分发，
- 再由 L7 负载均衡器（如 Nginx）做精细化路由。

在云原生环境中，**Ingress Controller（如 Nginx Ingress、Traefik）** 已成为 L7 负载均衡的事实标准。

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
