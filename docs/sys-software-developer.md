# 系统软件工程师的技能树

最近看来这个[招聘要求](https://github.com/ruanyf/weekly/issues/1315#issuecomment-651569435)，其主要内容如下:

> ## 要求
> - 精通 C++，熟练掌握至少一种其它编程语言。C++是高性能计算的首选，同时我们相信问题的解决方案不止一种，需要灵活的选择合适的技术来构建系统。
> - 精通网络协议及其实现，能够分析网络协 议，熟悉 socket 和 TCP/IP 开发；
> - 高性能方向（上海 /北京）
>
> ## 工作职责
> - 参与回测系统和交易平台的设计开发，负责量化交易实现系统稳定运行；
> - 参与高频交易系统的研发、运行和优化；
> - 持续研究高性能技术，不断优化改进核心高性能组件；
> - 研究优化大规模计算，存储等技术；
> - 软硬件优化与维护。
>
> ## 任职要求
> 现代 C++/C/汇编等混合编程，对 C++ ( >= C++11) 有很深的认识，熟练掌握 Linux 的操作系统优化配置、Linux 服务器常见命令行工具以及 Linux 应用程序性能分析方法；
> 掌握以下技能将会大大加分：
> - 熟练掌握 x86\_64 汇编编程，精通 i386/x86\_64 ELF/ABI ；
> - 有 GCC 、Clang/LLVM 、ICC 相关开发经验，有编译器插件开发或优化开发经验；
> - 有 x86 等平台固件，Linux 内核开发经验；
> - 深入理解现代微处理器乱序多发射动态执行的原理，以及现代多核处理器 Cache Coherence Protocol ；
> - 精通 AVX/NEON 等向量化编程方法，有视频编解码、linpack 核心循环等算法向量化开发优化经验；
> - 精通高性能低延迟 FPGA 开发，时序收敛、分析及优化，熟悉 PCIE，Ethernet 相关协议及软件栈；
> - 精通网络协议及其实现，分析网络协议，熟悉 socket 和 TCP/IP 开发。

我个人觉得是指导我自己的技能树很好的指导。

## 长期计划
- 分布式
- 编译
- CPU 微结构
- 操作系统，虚拟化

## 短期计划
- QEMU
- Linux
  - virtio vhost OVS
  - folio
- DPDK
- Rust
  - CloudHppervisor
- 找一些 Data Center 相关的文章看看。
  - 量化中相关的章节
- CPU
  - [ ] 虽然，但是很难找到和多核相关的 CPU
- 数据密集型应用

## pending blog
- user mode Linux
- library OS
- loopback block / loopback network devices
- Dive into Folio
- 内核中的编译知识
  - MaskRay yyds
- kdump / kexec 以及 initramfs 的 gsG
