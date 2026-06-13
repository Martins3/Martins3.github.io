# 杂谈
为什么搞 Linux?
  - 几乎所有的内容都是开源的
  - 可以养活自己

- tokei 和每一个文件中包含那些内容
  - 构建出来一个项目，列举出来基本包含什么内容
  - 参考 https://github.com/oldwinter/knowledge-garden 的做法
- 熟悉基本使用 /proc /sys
    - systeroid --tui
    - sysctl
      - https://gitlab.com/procps-ng/procps/
    - 熟悉使用 man
    - 对于文件系统，熟悉其 mount options 来熟悉
- 使用 ebpf 来学习内核
- 使用 flamegraph 来学习内核
- tracepoint 就是一个模块的重点，例如 block 模块中
- 使用 perf 来学习内核
- 各种 tlpi 学习内核，搭建一个更加清楚的 tlpi 环境

- make menuconfig 中，分析 kenrel hacking 那个目录

- 包括 KSAN 之类的调试技术总结
The Kernel Address Sanitizer (KASAN)

- 也许整个项目写成一个 vuepress 吧

感觉内核是个无底洞啊，如何才可以搞定其核心
似乎我现在连其中的重点都没有时间解决了。

- 动手测试，可以快速解决自己的疑问

打开 htop ，有很多字段看不懂，没关系，这个就是你接下来看内核的动力。

## 一个模块的基本过程
- 代码统计
- tracepoint
- 文档加 ai
- 找如何使用
- perf 找高速路径
- 找 lock 看数据流向

需要到最后再去分析代码的调用路线。

## 其他资源
- https://github.com/R3x/How2Kernel : 从 security 的角度

## 还可以写的总结

内核如何做到最大的释放硬件性能
  1. 做一个内核是如何支持多核, unma, cache 的整理


## nvim 的基本要求
1. git blame

## 深入细节
- 同步技术
- 各个模块的锁设计
- 如何构建超大型的程序?

## 理解一个 CPU 的指令手册

## 基本知识
- 虚拟化
- 存储
- 网络
- 驱动

## 如何定位死锁问题?

## crash 使用案例，到时候把这个补充上
https://stackoverflow.com/questions/69624499/generate-a-cpu-soft-lockup-with-a-user-space-process

## 看看这个选项
- CONFIG_PCI_DEBUG

## 一个想法
看看都存在那些 tracepoint ，那一般是重点

## 不是没有时间掌握不了，而是效率不够，方法不对导致的，基础不够掌握不了

## 利用 crash 看看全局变量中的数值
- remap_ops

## 搞一个项目，叫做常规路径查询，方便找核心的函数

## 这个也加入到体系中
https://github.com/sindrets/diffview.nvim

## 这个图画还不错
1. 也许可以补充补充
2. 也许可以搞一个网络的版本的
2. 也许可以作为更好的数字花园的补充
  - 数字花园作为两个形式，一个基于文件夹的，一个基于图形的

https://www.thomas-krenn.com/en/wiki/Linux_Storage_Stack_Diagram

## 太酷了
https://github.com/lkl/linux


## 这个目录中的内容大多数是需要看下的
- /home/martins3/core/linux/tools/

## 牛逼啊
https://askubuntu.com/questions/575651/what-is-the-difference-between-grub-cmdline-linux-and-grub-cmdline-linux-default

## RTLA
https://bristot.me/and-now-linux-has-a-real-time-linux-analysis-rtla-tool/
https://lwn.net/Articles/869563/

https://mp.weixin.qq.com/s/28MPKE5Er77hSf-HsyEu6g


## 这个总结写的很好
https://jvns.ca/blog/2022/08/30/a-way-to-categorize-debugging-skills/

其实我们从

https://blog.richliu.com/2022/12/08/5005/build-centos-8-kernel-from-source-on-arm64-platform/


## 如果内核构建不是 reproducible 的，那么内核模块的构建是 reproducible 的吗?

## 类似于有的线程 hang 住了，这种一般怎么用 crash 分析

## 似乎已经有人这么过了

https://github.com/cirosantilli/linux-kernel-module-cheat

搭建环境
1. python 设置源
https://mirror.tuna.tsinghua.edu.cn/help/pypi/
2. 安装 zsh
https://gist.github.com/MichalZalecki/4a87880bbe7a3a5428b5aebebaa5cd97
然后改一下 : run-docker
3. 代理:
虽然不知道为什么，gitproxy 和 setproxy 就可以解决问题网络问题。

cat "$(./getvar run_dir)/run.sh"

让 ai 分析一下这个项目:

## 开始之前，需要熟悉掌握 ebpf 的书写吧
话说，ebpf 是如何处理同步问题的？

## 参考这个
https://events.linuxfoundation.org/wp-content/uploads/2023/09/Joels-Linux-Kernel-Debugging-Webinar-2023.pdf

https://d3s.mff.cuni.cz/files/teaching/nswi161/vlastimil-babka-kernel-debugging.pdf

也许看看这个东西:
file:///home/martins3/Downloads/debugging-linux-kernel.pdf

## 这本书找出来看下

- Linux Kernel Debugging: Leverage proven tools and advanced techniques to effectively debug Linux kernels and kernel modules
  - https://github.com/PacktPublishing/Linux-Kernel-Debugging
    - https://www.kernel.org/doc/html/next/process/kernel-docs.html : 居然是 kernel 文档有介绍的方法

## 这个文档阅读一下
Documentation/virt/

## 可以参考这个项目来将两个项目分开
https://github.com/plantree/ruanyf-weekly/blob/main/scripts/main.sh

## 才意识到使用 ftrace 可以轻松获取到 printf 到底搞了什么

## 错误的 Linux 教材
错误的 Linux 教材 : Understanding Linux kernel

正确的 Linux 教材:
- gcc 手册
- nvme scsi specification
- tcp/ip rcf
- uefi 手册
- x86 arm 手册
- intel amd arm iommu 手册
- lwn / kvm forum
- sosp
- ovs / dpdk / k8s / qemu
- lpc

就像是学习了 C 语言之后发现自己几乎什么应用都写不出来，其实看了
Understanding Linux kernel 也几乎是做不了任何事情，这只是入门而已，如果想要解决问题，
还是需要继续深入的。

其实 Meta 和 Intel 对于 Linux Kernel 的了解都非常深刻，因为他们大量的硬件和 CPU 打交道。
真正纯粹的 Linux 厂商其实很少了，redhat / suse ?

## 多写 > 多读

从具体到抽象，而非从抽象到具体


## 研究下 是如何构建内核的，里面有很多有趣的命令，包括构建工具的方法
- https://git.rockylinux.org/staging/rpms/kernel/-/tree/r10s?ref_type=heads

## 有趣的
https://github.com/shadps4-emu/shadPS4


## 内核裁剪工程师必看
https://github.com/a13xp0p0v/kernel-hardening-checker

## 不要过多的阅读那些枯燥无比的教程
- https://github.com/foxsen/archbase

## 看看 wireguard 作者的生活
https://www.reddit.com/r/linux/comments/hzyu8j/im_jason_a_donenfeld_security_researcher_kernel/

## bpftrace maintainer
- https://github.com/danobi/
- https://dxuuu.xyz/ : 写过一系列的小 blog ，有趣的

## network 专家
http://borkmann.ch/

## wireless : 80 多岁的人依旧战斗一线
https://arstechnica.com/gadgets/2024/06/larry-finger-linux-wireless-hero-was-a-persistent-patient-coder-and-mentor/

> The deepest of trenches: Linux Wi-Fi in the 2000s

## 将
https://cacm.acm.org/research/10-things-software-developers-should-learn-about-learning/

## 这个人的回答似乎是错误的，可以重新回答下，这里关于 kernel rpm 的构建
https://stackoverflow.com/questions/58549382/fastest-to-rebuild-the-linux-kernel-using-rpmbuild

## mptcp 的开发的环境
https://github.com/multipath-tcp/mptcp-upstream-virtme-docker

## 如何理解 linux
买一个 2T 内存，最好是 500 core 的，开启
200G 的网卡，开启一个 10 个 200 core， 100G 的虚拟机。

开启几千个 pod 的 k8s。

## 似乎
也可以将 kernle bzImage 直接放到 /boot 下，其他的东西拷贝到 /usr 下之类

从而绕过 rpm 的构建。

## 这个是极好的
https://github.com/tzussman/kmodleak

插桩和 ebpf ，来实现 kernel 的 memleak 定位

## 总结一下经典的 call path ，用 backtrace 和 function graph 测试
1. open 函数 : vfs,  page cache , block
2. sleep 函数 : 时钟调度器
3. kill : 信号机制 process 生命周期

经典的 syscall 函数将整体串联起来

## 将 kernel 中大致的目录解释一下
1. samples 文件夹的代码　作用是什么
2. tools 中间　主要做什么的
3. lib 和 tools 有什么区别吗 ?

## 哈哈，错误的入门资料
- https://makelinux.net/kernel_map/
- https://linux-mm.org/ : kernel newbie 的内容

## 想要解决问题吗?
- https://github.com/KSPP/linux/issues
  - https://kspp.github.io/

## 很多学习的顺序都是反过来的

例如 cpp ，如果没有超大型的项目，cpp 的很多设计会让人感觉莫名其妙。

但是如果用 c 写过项目，就会发现，存在大量重复的代码。

linux kernel 也是如此，开始的时候，很多模块都是用不到的，
这些模块都是互相耦合的，所以

## 感觉
看这种标准，例如 https://isocpp.org/std/the-standard
其实，描述都是细节，其实相当无聊，因为不知道背后的想法是什么，想要从细节中反推出来设计目的，非常痛苦。

Understanding Linux Kernel 有类似的感觉。

## 这个人好强

https://diy.inria.fr/doc/index.html

## 掌握基本原理，熟练使用基本工具
解决任何问题，深入到细节中，做一个长期的项目。

- kernel 的基本工具 : QEMU + ftrace
- llvm 的基本工具 : ...
- chip 的基本工具 :

## 原来有这么多的工具啊
https://github.com/google/sanitizers

## 这里有一个
https://lore.kernel.org/lkml/172816780614.3194359.10913571563159868953.pr-tracker-bot@kernel.org/T/#me8b6f9b0a5e612f956354a9a73121e5dfe054248

- https://github.com/tytso/xfstests-bld
- https://github.com/linux-kdevops/kdevops
  - 很详细，很复杂，很有必要深入了解

## DAMON maintainer 整理的一些 kernel patches
https://sjp38.github.io/post/lkml_news_v6.12-rc3/

## kernel soruce tree 下的 tools/ 是你的好朋友

## 编辑工具，需要用这种形式吗?
https://typst.app/docs/tutorial/writing-in-typst/

## 有用
https://github.com/pimlie/ubuntu-mainline-kernel.sh

大约 1000 行的，也许可以

## 这个目录是你的好朋友
linux/samples/

## 似乎的确有一个问题
之前 kernel/plka/0/plka-chapter-07.md 中记录的内容，几乎完全不记得了。
所以，其实如果没有掌握，没有理解，一旦理解，就很难忘记，然后将其变为
blog 输出。

其实还是那个问题，只有阅读，没有操作，很快就忘记了。

所以:
1. 概论
2. 操作
3. 细节

## 写一个内核依赖图
> 先收集起来

提前准备: C，深入理解计算机系统，组成原理

- memory
  - folio
- 虚拟化


可以实现一个项目:
- https://stackoverflow.com/questions/63510943/get-dependencies-of-a-given-feature-in-the-linux-kernel

1. 将各种 kconfig 的依赖用图形展示出来
  - nix-tree 的形式?
2. 测试下，能不能如果直接打开一个最底层的 config ，其依赖的 config 是否有效。

其实就是，首先有一个大概的印象:

## 感触
1. lld 那个书不好，引入了太多复杂的东西，其实我完全对于 USB 完全没有兴趣。
可以解决问题的，自然就有兴趣了，反复无法解决，最后就没意思了。

2. 有的东西等到要用的时候才去学习已经晚了，因为不是一下午可以搞定的。
但是没有带着什么目的学什么东西，真是纯纯的折磨。

## 就是这个样子的，我靠
https://www.zhihu.com/question/553813879/answer/3623526087

## 当然，到了一定的时间，你开始关注的数据的流动，而不是他们直接的调用关系了

https://github.com/Mic92/vmsh

## 其实类似的每一个 blog 都是长篇大论，浅尝辄止
- https://wangzhou.github.io/

## 这里的测试看看
https://people.kernel.org/read

## 这个也是我的心愿
https://github.com/alex/what-happens-when

## 如果是入门，最好是使用原生的环境
当然如果你很强，可以随便切换，那无所谓，例如:
https://blog.hackret.com/2023/07/564/

## 这个我不推荐
[小的操作系统代码教程](https://littleosbook.github.io/)

似乎很多教程都有这个错误，大量的时间浪费到无聊的 seabios 上了

https://news.ycombinator.com/item?id=43440473 这个评论我太同意了

## 也许我们的目的是提供一个 https://roadmap.sh/roadmaps

但是可能无法达到这样的效果，如果太简单，死路一条，如果太难，
认为是装逼

## 2019 的感触，还是如此正确
The kernel feels like an ever-growing mountain that keeps rising higher, no matter how tirelessly you strive to climb it.

## 内核不是难
1. 而是搞的人少而已
2. 接触的少

编程模型和普通的环境有所区别，但是熟悉就好了。

## 用这个描述 kconfig 的依赖?
https://github.com/terrastruct/d2

## 逐个看看
- https://github.com/jubalh/awesome-os

## 这个搞的项目都可以看看
https://github.com/netoptimizer

https://netoptimizer.blogspot.com/

https://github.com/xdp-project/xdp-project

## 观望一下
https://github.com/chanhx/crabviz

看上去代码量很小，所以其实可以很容易

## 仔细看看
- https://github.com/icsnju/visualinux
  - 看看论文，然后观望一下，如果没有更新就算了

- https://www.usenix.org/system/files/atc23-jiang-yanyan.pdf
  - 这个也可以仔细看看

## 似乎是一个专业人士
https://walac.github.io/archiv/

https://github.com/kernel-cyrus/kernel-tour

http://lastweek.io/notes/linux/linux-function-trace/

## 和本仓库类似的项目
- https://github.com/gatieme/LDD-LinuxDeviceDrivers/tree/master/study/kernel/01-process

## 使用 AI 的方法
https://www.bilibili.com/list/ml85366172

## modules 的大整理
https://blog.csdn.net/weixin_41028621/article/details/113919558

## 常看常新
https://www.zhihu.com/question/304179651/answer/561395663

## 说实话，的确不好用的

https://lore.kernel.org/rust-for-linux/208e1fc3-cfc3-4a26-98c3-a48ab35bb9db@marcan.st/


I'm tired of having to review patches in an email client, where I can't
even tell which patches are for me to merge and not without writing
complex filtering rules to correlate email bodies with kernel subsystem
paths, which I don't have the time to write and maintain.

I'm tired of having to type a half dozen `b4` commands just to send a
change.

## 他的 blog 看看
https://github.com/ryncsn

https://blog.hackret.com/2021/10/531/

他的 hacking 能力很强


## 不要期待问问题有人回复
https://lore.kernel.org/lkml/2149597.8uJZFlvqrj@xrated/T/#m10d125da313244d9b40b67dbec9f488e186bc06f


## 常用网站
- https://bugzilla.kernel.org

## xen 的 kernel
https://github.com/zen-kernel/zen-kernel

## 类似的项目是很多的哇
https://github.com/0xef53/kvmrun

## 基本同意这个想法

Linux内核打怪升级指南 - 寻找北极星的企鹅的文章 - 知乎
https://zhuanlan.zhihu.com/p/715102006

https://www.zhihu.com/people/haox-57/posts

## 不错
主要是关于 ARM 的
http://raverstern.site/en/posts/slab-merging/

## 果然，马上这个项目就有了
https://deepwiki.com/torvalds/linux
虽然效果一般

## 内核的很多功能很复杂，是因为他的用户很多，需求很多
你需要知道

这些复杂是你所需要的吗?

## 这个是写的好的
https://www.linkedin.com/pulse/kernel-mind-moon-hee-lee-miwze

## 使用这个统计代码
https://github.com/zdyxry/tokui
这两个要是可以结合一下就好了
https://github.com/brendangregg/FlameGraph

## 使用 gdb 可以获取执行路径，有什么办法可以知道数据路径吗?

数据路径才是理解问题的最佳方式，
1. 写代码都是围绕数据如何运行的，先有数据如何维护，运转，然后才去写代码
2. 可以自动理解 lock 关系

但是有什么办法获取到吗，类似 gdb 的 backtrace 这种，
利用 llvm 分析结构体的联系吗?

如果实在是没有什么办法，那么就把这个事情记好把。

或者有办法通过 lock 分析出来数据流吗?

所以，虽然很难，但是尽量早的观察锁的持有状态，这是思考问题极好

## 是我不会用吗?
https://deepwiki.com/torvalds/linux
效果这么差

## 由于学习内核产生的一些思考

很多内核问题都无法立刻理解，而是过一段时间才能来看，这个时候
之前搞过的事情已经忘记了。

- 解决办法 : 定时回忆，但是这个会导致主线偏移
- 不要写文档，写测试代码，让测试代码下次开始的时候，可以迅速的开始
- 当要结束的时候，写一个总结，让下一次知道从哪里开始

## 记录一个调试的重大失误

既然都发现了问题，为什么不去按照这个方向
而是不断的用 bcc 来检查，要知道我们什么技术都有的:

一个其他的类似的效果
```txt
606272  98792  16%    0.12K   9473       64     75784K scsi_sense_cache
 34752  32446  93%    2.00K   2172       16     69504K kmalloc-2k
116416  98256  84%    0.50K   1819       64     58208K kmalloc-512
 74284  72003  96%    0.64K   1516       49     48512K inode_cache
```

```c
static unsigned char *scsi_alloc_sense_buffer(bool unchecked_isa_dma,
	gfp_t gfp_mask, int numa_node)
{
	return kmem_cache_alloc_node(scsi_select_sense_cache(unchecked_isa_dma),
				     gfp_mask, numa_node);
}
```

似乎并没有什么特殊的地方，只是 alloc node 就有问题，这个时候为什么还继续用 bcc 来看。


## 这个人的确是内存相关的 maintainer
https://nostarch.com/linux-memory-manager

相比必然分析了锁相关的东西

## 除了使用 kconfig 获取，当然可以知道内核的结构

但是仅仅 MAINTAINERS 这个文件，也是可以把这个结果划分的，划分的会更加粗略一点，
还是很有趣的。


不知道是不是真的，但是我现在就是这个感觉的，这是正确的:

> linus 的代码观：先把结构想清楚，他的经典审查风格大家也都熟悉：
>
> 不做没必要的抽象
> 不为了“可能将来会有的功能”写胶水代码
> 先把数据流、状态、不变量想清楚
> 用设计消灭特殊分支，而不是到处 if else


## 各种 linux 资料点评

- Professional Linux Kernel Architecture : Device Drivers
  - 对于存储栈的讲解太简单了
- **Linux Device Drivers** by Corbet et al. [CRKH05].
- [Essential Linux Device Drivers](https://book.douban.com/subject/3088263/)
- https://www.wiki.kernel.org/ : 太老了，不过也算是一个列表了


## 内核工程师
就是这样的，有老板的工作要做，同时搞社区
https://mp.weixin.qq.com/s/-o8Uerckha2YtGhAPePNAQ

## 首先，你需要会编译内核
一个方法就买很好的机器:
https://openbenchmarking.org/test/pts/build-linux-kernel&eval=8342131a895da626e7bf0bf5555c783bac8059fa#metrics

## 有趣的东西
https://news.ycombinator.com/item?id=46066280


## 有必要使用 flamegraph 吗?
perf 可能需要安装:
```sh
sudo apt install linux-tools-common linux-tools-generic linux-tools-`uname -r`
```

最终效果如下，可以在新的窗口中打开从而可以动态交互。
![](./img/dd.svg)

这个工具我使用的非常频繁，所以构建了简单的[一个脚本](https://github.com/Martins3/Martins3.github.io/blob/master/docs/kernel/code/flamegraph.sh)，例如:

```sh
./flamegraph.sh -c 'iperf -c localhost' -g iperf
```

## 这里的东西也需要查漏补缺一下
https://origin.kernel.org/doc/html/latest/dev-tools/propeller.html

modprobe 和 centos 的区别 : https://askubuntu.com/questions/20070/whats-the-difference-between-insmod-and-modprobe

man kmod(8)

https://man7.org/linux/man-pages/man7/dracut.cmdline.7.html

rpm 包

https://blog.csdn.net/tiantao2012/article/details/54426549

看 /sys/modules 的 srcversion

看 pcie 的匹配。

pcie 可以动态加载，如果不是 pcie 设备，都是怎么办的？

weak-modules 的功能

## 小工具
lspci lsscsi

## pstack

## 基本的操作
make hardening.config

## 其他的解决方案
https://github.com/ottomatica/slim

如今，将容器的东西打包过来

## Greg Kroah-Hartman 的 blog
http://www.kroah.com/log/blog/2020/09/18/fast-kernel-builds/

## nvim 的基本
此外补充一下
1. nvim 中的集成 git blame
2. nvim 中的 git-messager

问题:
1. git grep 和 rg 有什么差别吗?
2. git ls-files 和 fd 有区别？


https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux.git
```txt
  master
  remotes/origin/HEAD -> origin/master
  remotes/origin/linux-2.6.11.y
  remotes/origin/linux-2.6.12.y
  remotes/origin/linux-2.6.13.y
  remotes/origin/linux-2.6.14.y
  remotes/origin/linux-2.6.15.y
  remotes/origin/linux-2.6.16.y
  remotes/origin/linux-2.6.17.y
  remotes/origin/linux-2.6.18.y
  remotes/origin/linux-2.6.19.y
  remotes/origin/linux-2.6.20.y
  remotes/origin/linux-2.6.21.y
  remotes/origin/linux-2.6.22.y
  remotes/origin/linux-2.6.23.y
  remotes/origin/linux-2.6.24.y
  remotes/origin/linux-2.6.25.y
  remotes/origin/linux-2.6.26.y
  remotes/origin/linux-2.6.27.y
  remotes/origin/linux-2.6.28.y
  remotes/origin/linux-2.6.29.y
  remotes/origin/linux-2.6.30.y
  remotes/origin/linux-2.6.31.y
  remotes/origin/linux-2.6.32.y
  remotes/origin/linux-2.6.33.y
```

## kcbench
https://gitlab.com/knurd42/kcbench


# 从新手到专业

- [ ] 不知道有什么方法订阅这里的 merge 信息 : https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/log/
- [ ] https://brennan.io/2021/05/05/kernel-mailing-lists-thunderbird-nntp/
  - http://pan.rebelbase.com/
- [ ] 应该将 nvim 中继承的 git blame 技术好好重新分析一下，例如，已经进入到一个文件之后，如何获取没一行的 commit message

## 内核关系

- linux 的分支:
  - 主线 rc-1
  - release

- 各个 subsystem 的

## 参与社区
- http://vger.kernel.org/lkml/

## 关于其他的 distribution 的关系

### redhat
好像，他的内核是没有逐个 commit 的，只是存在版本的发布和对应的包:

- https://stackoverflow.com/questions/41314978/can-we-git-clone-the-redhat-kernel-source-code-and-see-the-changes-made-by-them
- https://github.com/linux-audit/audit-userspace : audit 模块的测试，那么是否存在所有的测试整理一下，免得很尴尬。

## 测试，ci 和分析

- [ ] https://github.com/google/syzkaller
- [ ] https://github.com/kernelci/kernelci-core
- [ ] https://www.kernel.org/doc/html/latest/dev-tools/index.html
- https://www.kernel.org/doc/Documentation/admin-guide/tainted-kernels.rst

## kernel patch

### 配置
在 .gitconf 上的设置:
```plain
[sendemail]
  smtpencryption = tls
  smtpserver = smtp.gmail.com
  smtpuser = hubachelor@gmail.com
  smtppass = ***********
  smtpserverport = 587
```
在 gmail 上的设置 https://myaccount.google.com/lesssecureapps, 否则 git sendemail 无法使用

### 使用

```sh
git commit --amend
proxychains4 git send-email 0001-change-mmap-flags-from-PROT_EXEC-to-PROT_READ.patch --to ltp@lists.linux.it
```

[^1]: http://houjingyi233.com/2019/07/15/%E7%BB%99linux%E5%86%85%E6%A0%B8%E6%8F%90%E4%BA%A4%E4%BB%A3%E7%A0%81/
[^2]: https://zhuanlan.zhihu.com/p/138315470

- https://josefbacik.github.io/kernel/scheduler/bcc/bpf/2017/08/03/sched-time.html
    - 终于看到了一个写 blog 的内核 maintainer

- https://kernel-tour.org/kvm/vhe.html

- https://blogs.oracle.com/authors/dongli-zhang

UI 界面很好
https://kernel.guide/

有趣，但是内容不多
https://nskernel.gitbook.io/kernel-play-guide/kvm/amd-v-and-sev

收集一个 kenrel blog
https://blog.ffwll.ch/archive/ : intel 的工程师


这个有趣
https://dg-docs.ole.dev/


## 建立一个整体概念

### 代码主要分布在哪里？
- https://cregit.linuxsources.org/code/6.15/


## 继续这个东西
https://www.zhihu.com/question/588396308/answer/1888993882483709117
https://www.zhihu.com/question/553813879/answer/3440732601
https://stackoverflow.com/questions/19628393/how-to-begin-with-windows-kernel-programming

# 内核的一些技巧和方法
<!-- ec629b33-76e5-41a8-be88-c57d44418c2f -->

搞内核不是玩英雄联盟:
1. 打完一盘之后，必须复盘
2. 教学视频都是要看的
3. 刻意训练 (做自己最恐惧的，对自己影响最大的)

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
