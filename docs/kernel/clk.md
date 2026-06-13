# 中国 Linux 大会记录
http://ckernel.org/

## clk 2025
<!-- a93e7435-e136-4d37-a5a4-4560e1bf1daf -->
https://mp.weixin.qq.com/s/KWrNZW_4Gw8_sUqr1SkbZA

### 主论坛
"AI Agent 时代 Linux 内核的现状和发展"

drivers/dma-buf/udmabuf.c

"EROFS支持Direct IO + udmabuf" 如何理解?

- https://lwn.net/Articles/997548/
- https://www.phoronix.com/news/Linux-RWF_UNCACHED-2024

- "字节大规模内核版本迁移实践"
(最后具体成果的链接都需要 check 一下)

"从内核视角出发，如何推进 RISC-V 架构走进云场景"
(看不懂，先跳过)

"灵衢UB总线操作系统支持和关键场景介绍"
(工作的宣传，等进入主线再看吧)

### 2 内存管理与优化分论坛

- 宋峻睿-Swap Table 与 Swap 子系统的革新.pdf
- 李文通-PMR：并行内存回收的设计与实践.pdf
- 汪劭文-zram 多压缩算法效率实践与评估.pdf
- 林芝驰-ZRAM 异构压缩技术：基于 GPU 加速的内存回收方案.pdf
- 李杨欣文-mmap_lock 和 anon_vma_lock contention 优化.pdf
- 林泽生-ZCACHE：异步文件压缩缓存管理方案.pdf
- 韩棋-Uncached buffer IO 探索与在 f2fs 上的支持.pdf
- 郭纯海-EROFS 压缩文件 Direct IO 的探索和支持.pdf

### 3 文件系统和存储
- F2FS Large Folios 新进展针对压缩文件和私有状态的原创设计与实现
- XMFS：跨节点池化共享文件系统
- Parallelizing filesystem writeback
- ZonedStorage 性能优化探索与实践
- mdraid 无锁位图优化机制
- FallocateWriteZeroes 垂直零化文件预分配 (看不懂，已经进入主线)
- EXT4 块分配可扩展性优化 (挺好的，讲了 ext4 的背景，已经进入到主线)

### 4 调度、性能与调试分论坛

- 路自谦-Defer throttle to when task exits to user.pdf
- 章雨宸-持续Profiling.pdf
- 臧春鑫-对 eevdf的一点优化及思考.pdf
- Lianbo Jiang-Improvements to Eppic and Rust support in the crash-utility.pdf
- Gavin Guo-Decoding Kernel Callstack with MCP Tools.pdf
- 苏峰-RTRadar.pdf
- 王建政-EEIO.pdf

### 5 硬件架构与异构计算分论坛
- 牛根 - 基于龙架构的系统级二进制翻译器.pdf
- 张春艳 - 基于RISC-V向量扩展的RAID6 PQ校验算法优化.pdf
- 郭任 - Device_shared_work_queue.pdf
- 陈佩余方玉 - RISC-V perf TopDown和IOMMU-based Trace工作.pdf
- 李彬 - 同机多内核 众核高密容器场景的应用前景和优化思路.pdf
- 黎红波 - TrIO 利用IO轨迹加速容器冷启动.pdf
- 王金超 - KStackWatch 栈破坏实时定位工具.pdf

### 6 AI 基础设施与 eBPF 应用分论坛
- 皮振伟 - GD2FS - 面向AI的新一代分布式文件系统.pdf
- 程书意 - 基于eBPF的大模型性能分析及慢节点检测.pdf
- 林义凯 - 基于eBPF支持自定义低功耗策略的cpuidle_ext框架.pdf
- 唐葛亮 - 使用MPTCP实现网络加速.pdf
- 李朝阳 - PORTGPT 基于大语言模型的自动化向后移植研究.pdf
- 尹瑞星 - GPU Profiling Extending eBPF to GPUs.pdf
- 徐浪 - 基于eBPF的多线程系统信息采集优化方法.pdf


### baohua 的分析
https://mp.weixin.qq.com/s/VONHxd4sfwlSIHnG25NFdA

## 2024

- [ ] 从内核的角度提高数据中心内存利用率
  - [ ] https://engineering.fb.com/2022/06/20/data-infrastructure/transparent-memory-offloading-more-memory-at-a-fraction-of-the-cost-and-power/

- [ ] SLS 单层存储:零拷贝共享内存设计与应用
- [ ] YALT:Yet Another Lockless Tree
- [ ] PVM:轻量级的 KVM 虚拟化软件实现方案
- [ ] DMA-BUF 支持 DirectI/0 功能的方案和收益


## 2023

- [ ] vDPA：wirespeed_virtual_networking
  - vDPA 的介绍
- New kernel tracer(osnoise, timerlat) to locate the latency spike
- [ ] 向 linux 内核社区引入 objtrace
  - https://lwn.net/Articles/899265/ : 看上去没有合并进去


## 中国云计算基础架构开发者大会
似乎两个会有重复的
- https://gitee.com/china-cid/cid_slides

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
