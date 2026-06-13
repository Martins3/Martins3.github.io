# references.txt 中文译解

源文件：`tools/memory-model/Documentation/references.txt`

## 文档定位

这份文件是 LKMM 的参考书目索引，把相关背景材料分成四组：硬件手册与硬件模型、Linux 内核内存模型、建模工具、以及不同模型之间的比较。

## Hardware manuals and models

这一组收录了理解底层硬件行为所需的资料，包括：

- SPARC、Alpha、Itanium、Intel x86、Power、ARMv8、MIPS 等架构手册。
- x86-TSO、POWER、ARMv8 等正式或半正式内存模型论文。
- 多副本原子性、混合大小并发等更细节的体系结构研究。

这部分的作用是告诉你：LKMM 不是凭空创造出来的，它需要覆盖 Linux 支持的弱内存架构，因此必须认真面对硬件模型差异。

## Linux-kernel memory model

这一组是 LKMM 本体的主要背景材料，包括：

- 两篇 LWN 连载《A formal kernel memory-ordering model》
- 关于优化编译器风险的两篇 LWN 文章
- ASPLOS 2018 上的正式论文
- 配套备份材料与网页

如果你想了解 LKMM 的设计动机、验证方法和历史演化，这一组是最关键的。

## Memory-model tooling

这一组介绍用于描述和运行内存模型的工具与语言，包括：

- Alloy
- herd 工具相关论文
- `cat` 规范语言论文

如果你准备深入 `linux-kernel.cat`、`linux-kernel.bell` 或 herd7，这些文献是工具层背景。

## Memory-model comparisons

这一组提供 LKMM 与其他语言或标准模型进行对照的材料。例如与 ISO C/C++ 并发内存模型对照时，哪些地方更弱、哪些地方更贴近内核实践。

## 这份文档的使用方式

它不是入门文档，而是“继续深挖时该去哪找原始材料”的索引页。通常先读 `ordering.txt`、`recipes.txt`、`explanation.txt`，再回来按主题查这里的资料。

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
