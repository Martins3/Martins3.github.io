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

## 概念辨析

2026-08-17 codex 整理分析的

### “几层负载均衡”表示什么

这个说法主要表示设备用哪一层的信息做分流决策，不要把它当成严格的 OSI 分类。

| 类型 | 分流对象 | 决策依据 | 关键区别 |
| --- | --- | --- | --- |
| 链路聚合 | 多条物理链路 | MAC/IP/端口的哈希 | 只是选链路，通常不叫服务负载均衡 |
| ECMP | 多个等价下一跳 | IP，某些实现也纳入端口 | 选路径，不一定感知后端服务是否健康 |
| L4 负载均衡 | TCP/UDP 流 | IP、端口和协议 | 不理解 HTTP Host、URL 等应用内容 |
| L7 负载均衡 | 应用请求 | Host、URL、Header、Cookie 等 | 必须解析应用协议，通常会终止客户端连接 |

L5/L6 负载均衡并不是常用的工程分类。例如 TLS 卸载通常直接按 L7 代理的能力讨论，没必要硬套成 OSI L6。

### 反向代理和负载均衡

两者描述的不是同一件事：

- 反向代理描述连接方式：客户端访问代理，代理代表后端提供服务。
- 负载均衡描述选择策略：从多个后端中选一个。

Nginx 只代理一个后端时是反向代理，但没有负载均衡；从一组 upstream 中选后端时才同时具备两种属性。

IPVS 是 L4 负载均衡器，但不是常说的 HTTP 反向代理。
它转发数据包，不终止 TCP 后再向后端新建一条 TCP 连接。
这一点与 Nginx stream 或 HAProxy TCP 模式这类 full proxy 也不同。

### LVS 和 IPVS

LVS（Linux Virtual Server）是整体方案的名称，IPVS 是 Linux 内核中实现 LVS 数据面的子系统，
`ipvsadm` 是其配置工具。

IPVS 的核心对象是：

```text
virtual service（协议 + VIP + 端口，或 fwmark）
    -> scheduler（rr/wrr/lc/wlc/sh/mh ...）
        -> real server 1
        -> real server 2
```

新流量的第一个包到达时，调度器选择一个 real server，然后在 IPVS 连接表中记住这个结果。该流后续的包沿用原结果，而不是每个包重新轮询后端。

IPVS 自身不主动做 HTTP 健康检查。用户空间的管理程序负责检查后端，并增删 IPVS destination。

### NAT、DR 和 TUN

| 模式 | 请求方向 | 回包方向 | 主要限制 |
| --- | --- | --- | --- |
| NAT | director 改写目的 IP/端口 | 必须回到 director 做反向改写 | director 同时承担双向流量 |
| DR（Direct Routing） | director 只改写二层目的 MAC，IP 包中的 VIP 不变 | real server 直接回客户端 | director 和 real server 通常要在同一二层网络；real server 要配置 VIP 并避免代替 director 回应 ARP |
| TUN | director 用 IP-in-IP 封装请求 | real server 直接回客户端 | real server 要支持解封装，但不必与 director 处于同一二层网络 |

DR/TUN 中回包不经过 director，适合响应流量远大于请求流量的服务。

### kube-proxy 的 IPVS 模式

kube-proxy 把 Kubernetes 对象翻译成 IPVS 配置：

```text
Service ClusterIP:port  -> IPVS virtual service
EndpointSlice endpoints -> IPVS real servers
```

kube-proxy 根据 Service 和 EndpointSlice 的变化增删后端；IPVS 负责新连接的调度和数据包转发。即使使用 IPVS 模式，kube-proxy 仍会配置一些 iptables/ipset 规则处理过滤、伪装等辅助工作，不是完全绕开 netfilter。

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
