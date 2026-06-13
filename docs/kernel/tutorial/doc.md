# 阅读文档

> "Bad programmers worry about the code. Good programmers worry about data structures and their relationships."
> https://lwn.net/Articles/193244/

首先掌握基本设计思想，只有必要的时候，才去分析细节。

写代码的时候，关注数据流动，而非调用关系

我以前从来不看文档，原因很简单:
- 很多模块的设计，是没有文档的，直接默认大家都清楚，例如 raid / block layer / iouring
	- 这些内容散布在邮件列表，lpc , lwn 的回忆中
- 很多文档写的像是手册，从第一段开始就是工程细节，还没看完脑瓜子就是嗡嗡的
就算是看完了，里面不是所有的东西都是我感兴趣的
	- Documentation/trace/ftrace.rst
	- ext4
	- rcu
- 内核中的很多文档已经过时了
	- Documentation/admin-guide/ldm.rst
```txt
:Last Updated: Anton Altaparmakov on 30 March 2007 for Windows Vista.
```

不看文档，直接看代码，问题也很大

代码准确，但是代码中夹带了大量的细节和其他模块的耦合，很容易跑偏

解决办法:
1. ai 总结文档
2. ai 总结代码，然后不断的询问


基本经验:
1. 本地运行的效果非常好，只要提供足够多的资料

现在，哪些偏向代码流程分析的书已经没有意义了

## 工作流

这个的确好哇
https://pypi.org/project/marker-pdf/
https://github.com/opendatalab/MinerU
https://github.com/PaddlePaddle/PaddleOCR : 这个存在页数限制

1. 先检查 PDF 是否可搜索（无需 OCR）
pdffonts intel_manual.pdf  # 有字体列表 = 可搜索文本

2. 优先尝试直接提取（无 OCR 损耗）
pip install pymupdf4llm
python -c "import pymupdf4llm; print(pymupdf4llm.to_markdown('intel_manual.pdf'))"

3. 若效果不佳，再用 Marker 增强处理
pip install marker-pdf
marker_pdf intel_manual.pdf ./output --max_pages 100  # 分批处理

```txt
uv pip install marker-pdf
marker_single /path/to/file.pdf
```

## 所以，大致的计划

1. uefi
2. nvme
3. scsi ata 文档
4. fc
5. arm / intel / amd / iommu / loongarch 手册
6. nfs rfc
7. raft

一些教材:
1. 量化
2. sys/yao.md 的内容
3. eac 编译

让同时阅读代码和手册，然后让他生成对照的东西
解决几个长期困扰的问题
1. apic 寄存器的含义


## 至少 perfbook 是不太需要
git clone git://git.kernel.org/pub/scm/linux/kernel/git/paulmck/perfbook.git

## 用这里的方法吗?
https://context7.com/skills/anthropics/skills/pdf

## 多好的基本调试思路，但是也许在 ai 面前就是一个弟弟

```txt
D agents/skills/kernel/references/coding-tools.md
D agents/skills/kernel/references/crash-analysis.md
D agents/skills/kernel/references/drgn-guide.md
D agents/skills/kernel/references/ebpf-guide.md
D agents/skills/kernel/references/files.md
D agents/skills/kernel/references/ftrace-guide.md
D agents/skills/kernel/references/git.md
D agents/skills/kernel/references/kbuild.md
D agents/skills/kernel/references/lwn-analysis.md
D agents/skills/kernel/references/perf-guide.md
D agents/skills/kernel/references/qemu-testing.md
D agents/skills/kernel/references/skill-lessons.md
R docs/kernel/tutorial/kcov.md
```

1. 阅读文档
3. 分析相关的源码
	- 该代码对应的 kconfig 是哪些
2. 如何验证问题
	- 如果可以，写 bash 脚本，例如 cgroup namespace 网络特性
	- 如果可能，使用 c 语言写用户态代码来测试，典型的是 syscall 相关的
	- 通过 sysfs / proc
	- 如果存在，分析其 debugfs
	- 基本工具
		- lsp
		- git
	- 利用 trace 工具
		- bcc
		- ebpf
	- 领域相关的 trace 工具
		- blktrace
		- kvm_stat
	- 使用 qemu 搭建调试环境，内核验证
		- -kernel
		- initramfs
	- 快速编译内核来验证
	- 写内核模块来验证
3. 当输出文档的时候，需要注意一下问题，分析问题的顺序如下:
	- 首先，需要有一个高屋建瓴的总结，也就是这个模块在整个体系的位置，解决了什么问题
	- 核心对象是什么，数据是如何流动的
	- 然后关注数据如何同步的
	- 不要过分关注 那一个函数调用了哪一个函数。
	- 最后，为关键点构建 anki 复习点

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
