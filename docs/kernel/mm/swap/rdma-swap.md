# RDMA Swap
<!-- aa3e0928-819c-451e-a8db-faaffb6c937f -->

添加 Swap 后端为 RDMA

https://github.com/clusterfarmem/fastswap

## 项目的基本构成
看代码实现是相当简单了，没有考虑太多复杂的内容:
```txt
fastswap_rdma.c  C                       676         25          152         853         72.47 %
fastswap.c       C                       94          4           22          120         10.20 %
fastswap_dram.c  C                       62          0           18          80          6.80 %
fastswap_rdma.h  C Header                63          0           15          78          6.63 %
Makefile         Makefile                23          2           7           32          2.72 %
fastswap_dram.h  C Header                10          0           4           14          1.19 %
```

```txt
这是一个 Fastswap 远端内存 swap 原型，大概来自 EuroSys 2020 相关研究代码。它的目标是：当本机内存不够、Linux 需要 swap 时，不把页写到本地磁盘，而是通过
RDMA 写到另一台机器的内存里，从而把远端内存当作高速 swap 扩展。

主要结构：

- README.md:1
  说明依赖 Linux 4.11、Mellanox OFED、RDMA 网卡，推荐 ConnectX-3。客户端要打内核补丁并加载 fastswap 驱动，远端机器跑 memory server。

- kernel/kernel.patch:1
  修改 Linux 4.11 的 swap/frontswap 路径，给 frontswap 增加 load_async、poll_load，让换入页可以走异步 RDMA 读；还改了 swap_readpage 等路径。

- drivers/fastswap.c:1
  核心 frontswap 驱动。注册 frontswap_ops，Linux swap 页的 store/load 会转发到后端：
    - store -> 写远端页
    - load_async / load -> 从远端读页
    - poll_load -> 轮询 RDMA 完成队列

- drivers/fastswap_rdma.c:1
  真正的 RDMA 后端。客户端连接远端 server，拿到远端内存区域的 baseaddr + rkey，之后用 RDMA WRITE/READ 直接把页写入或读出远端内存。每个 CPU 建三类队列：
  同步读、异步读、同步写。

- farmemserver/rmserver.c:1
  远端内存服务器。启动后申请并注册一块默认 32GB 内存，通过 RDMA CM 等待客户端连接，然后把这块内存的地址和 rkey 发给客户端。之后数据面不再经 server 主动
  处理，client 直接 RDMA 访问这块内存。

- drivers/fastswap_dram.c:1
  本地 DRAM 模拟后端。用于实验，不走 RDMA，而是在本机 vzalloc 一块 32GB 缓冲区模拟远端内存。

运行模型可以理解为：

应用内存压力
  -> Linux swap/frontswap
  -> fastswap.ko
  -> RDMA 后端 fastswap_rdma.ko
  -> 远端机器 rmserver 注册的 32GB 内存

注意点：

- 这是研究原型，不是现代内核可直接使用的生产项目。
- 强绑定 Linux 4.11 和 Mellanox OFED。
- 需要一个 swap 设备“激活”内核 swap 路径，但实际数据不会写到这个 swap 设备。
- 远端内存大小默认 32GB，README 里也要求 swap 设备大小和可用远端内存匹配。
```

## 是否具有可行性

```txt
短结论：有研究和架构参考价值，但作为 2026 年可直接用的工程项目，意义很有限。

这个 repo 今天最有价值的地方是它展示了一个清晰的系统设计问题：怎么把远端内存接进 Linux swap/page fault 路径，并把 page fault 的 RDMA
延迟压到足够低。EuroSys 2020 论文里 Fastswap 的重点就是避免 RDMA swap 的 head-of-line blocking、减少中断和 reclaim 在 fault critical path
上的开销，并结合集群调度器判断远端内存是否能提升整体吞吐。论文结论也很克制：far memory 不是万能的，但对 memory-intensive workloads
有机会提升吞吐。来源：CFM 论文 https://amyousterhout.com/papers/cfm_eurosys20.pdf

但如果你问“今天能不能拿来用”，答案基本是 不建议：

- 它绑定 Linux 4.11，这个仓库 README 也明确要求 kernel patch 打在 4.11.0 上；现代 Linux MM、swap、folio、memcg、zswap 路径都已经变了。
- 它依赖老 Mellanox OFED/RDMA 环境，README 推荐 ConnectX-3，这在 2026 年已经很老。
- 本地仓库最后一次提交是 2022-07-05 Update README.md，看起来不是活跃维护项目。
- 它是研究原型，不是生产系统：缺少现代内核适配、容错、安全隔离、多租户管理、动态容量管理、观测和运维能力。
- 远端 server 很简单，注册一块 32GB 内存后把 rkey/baseaddr 给客户端，本质上是实验用 memory server。

2026 年更主流的方向已经变成几类：

- 本机 swap 优化：zswap/zram/高速 NVMe swap。现代 kernel 文档里 zswap 已经是标准压缩 swap cache，支持 sysfs 配置、writeback、debugfs 统计等。来源：
  https://docs.kernel.org/admin-guide/mm/zswap.html

- CXL 内存扩展/池化：Linux CXL 子系统已经支持 Type-3 memory expander，可通过 DAX 或 memory hotplug
  暴露给用户态或页分配器。来源：https://docs.kernel.org/driver-api/cxl/index.html 和 https://docs.kernel.org/driver-api/cxl/linux/dax-driver.html

- NUMA / memory tiering：远端或慢速内存更像“另一层 NUMA memory tier”，而不是非要伪装成 swap block device。
- 应用/运行时级内存管理：数据库、cache、ML serving 往往自己做冷热分层、对象迁移、spill、KV offload，比透明 swap 更可控。

所以我的判断是：

 用途                                                             是否有意义
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━
 学 Linux swap/frontswap、RDMA page fault、far memory 论文原型    有意义
───────────────────────────────────────────────────────────────  ────────────────────
 复现实验、读系统论文代码、做课程/研究 baseline                   有意义
───────────────────────────────────────────────────────────────  ────────────────────
 直接部署到现代生产环境                                           基本没意义
───────────────────────────────────────────────────────────────  ────────────────────
 直接移植到 Linux 6.x/7.x                                         成本高，收益不确定
───────────────────────────────────────────────────────────────  ────────────────────
 借鉴思想重新设计一个现代远端内存系统                             有意义

如果你是为了研究，建议把它当作 历史上很好的 design artifact 来读：看它怎么改 fault path、怎么分 RDMA queue、怎么避免关键页被预取页阻塞。

如果你是为了解决真实机器内存不够，今天更现实的路线通常是：先看 zswap/zram + NVMe swap + memcg 调参，再看硬件上是否有 CXL memory expander；只有在明确需
要跨机器共享内存池、并且能接受大量内核/RDMA工程投入时，才考虑复活 Fastswap 这类设计。
```
似乎是合理的，也就是

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
