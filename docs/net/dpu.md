## BlueField-3
<!-- 7de90152-8183-40c7-8e3e-88e7a8251446 -->
https://www.nvidia.com/content/dam/en-zz/Solutions/Data-Center/documents/datasheet-nvidia-bluefield-3-dpu.pdf

问题:
1. 和之前的 Mellanox 网卡的关系是什么?
2. 如何理解 : Programmability through DOCA
3. 找到


> [!NOTE]
> 参考神奇海螺的意见，有待验证

以下是 Mellanox（2020年被 NVIDIA 收购后现为 NVIDIA Networking）主要网卡产品线的发布时间整理：

ConnectX 系列智能网卡

| 产品型号 | 发布/上市时间 | 主要特性 |
|---------|--------------|---------|
| **ConnectX-3** | 2011年6月正式发布 [[18]] | 首款支持 FDR 56Gb/s InfiniBand 和 10/40GbE 以太网的多协议适配器 |
| **ConnectX-4** | 2014年11月宣布，2015年Q1开始向客户供货 [[50]] | 支持 100Gb/s 以太网和 EDR 100Gb/s InfiniBand |
| **ConnectX-4 Lx** | 2015年6月18日发布 [[55]] | 面向主流数据中心的可编程网卡，支持 25/50Gb/s |
| **ConnectX-5** | 2016年6月宣布，2017年Q3开始出货 [[61]] | 100Gb/s 智能网卡，支持 PCIe 3.0 |
| **ConnectX-5 Ex** | 2017年发布 [[58]] | 增强版，支持更多虚拟化功能 |
| **ConnectX-6** | 2019年7月发布初始固件 [[71]] | 200Gb/s 带宽，PCIe 4.0 支持 |
| **ConnectX-6 Dx** | 2020年3月全面上市 [[28]] | 增强安全引擎，支持 TLS/SSL/IPsec 硬件加速 |
| **ConnectX-6 Lx** | 2020年5月宣布，2020年Q3全面上市 [[3]] | 25/50Gb/s 安全智能网卡 |
| **ConnectX-7** | 2022年正式发布 [[78]] | 400Gb/s 带宽，相比 ConnectX-6 带宽翻倍、延迟降低 |
| **ConnectX-8 SuperNIC** | 2024年10月宣布，2025年10月GA发布 [[89]][[90]] | 800Gb/s 带宽，PCIe Gen6 支持，专为 AI 集群设计 |

### BlueField DPU 系列

| 产品型号 | 发布/上市时间 | 主要特性 |
|---------|--------------|---------|
| **BlueField-1** | 约2018-2019年 | 初代 DPU，基于 ARM 处理器的可编程网卡 |
| **BlueField-2** | 2020年10月5日 GTC 2020 宣布，2021年开始出货 [[107]] | 8核 ARM A72，200Gb/s 带宽，集成 DOCA 软件框架 |
| **BlueField-2X** | 2020年10月同期发布 [[107]] | 在 BlueField-2 基础上集成 Ampere GPU 核心 |
| **BlueField-3** | 2021年4月12日 GTC 2021 宣布，2022年Q1提供样品，2022年底正式发布 [[114]][[117]] | 16核 ARM Neoverse N2，400Gb/s 带宽，220亿晶体管 |
| **BlueField-4** | 2021年GTC宣布，原计划2024年发布，实际推迟至2026年 [[104]] | 预计支持 800Gb/s 带宽，面向 AI 工厂场景 |

重要时间节点
- **2019年3月11日**：NVIDIA 宣布以 69 亿美元收购 Mellanox [[126]]
- **2020年4月27日**：NVIDIA 完成对 Mellanox 的收购 [[124]]
- **2020年后**：Mellanox 品牌逐步过渡为 "NVIDIA Networking"，但产品仍沿用 ConnectX/BlueField 命名体系

> **注**：ConnectX-8 是目前最新一代产品（截至2026年2月），专为大规模 AI 训练和推理集群设计，支持 800Gb/s 甚至 1.6Tb/s 光学连接。BlueField-4 预计将于 2026 年随 NVIDIA Vera Rubin 平台推出

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
