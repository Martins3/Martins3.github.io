# 内核学习经验

最重要的问题, 你为什么要去学习内核 ? 可以参考[陈硕](https://www.zhihu.com/question/20541014/answer/93312920)的回答。

不同的人基础不同，需求不同，知识范围不同，看内核的方式也是不尽相同的，下面谈谈我的理解。

## 理论基础
- [深入理解计算机体系结构](https://book.douban.com/subject/26912767/)
- [陈海波的操作系统课程](https://ipads.se.sjtu.edu.cn/mospi/)
- [谭志虎的计算机组成原理](https://www.ryjiaoyu.com/book/details/42720)
- 一个上万行的小型操作系统，比如清华的[ucore](https://github.com/chyyuu/ucore_os_lab)

## 环境准备
1. 阅读环境准备

首先你要保证你的操作系统是 linux 内核，让自己对于 kernel 支撑起来的用户态环境有一个感性的认识。

2. 阅读工具
  1. 我的 neovim 配置在[这里](https://github.com/Martins3/My-Linux-config)
  2. 图形化的工具，我使用过[sourcetrail](https://www.sourcetrail.com/)，很不错，但是很卡。

3. qemu
  - 可以参考我的[一个脚本](https://github.com/Martins3/Martins3.github.io/blob/master/hack/qemu/x64-e1000/alpine.sh)

## 内核学习
我个人认为需要将**理解用户态**, **读书**, **分析源代码**, **写代码**。
- 理解用户态是基础，好在用户态的资料一般都丰富，一般问题不大。
- 读书可以快速掌握设计思路，但是有时候感觉像是背字典一样，作者往往只会将代码中间的骨架挑出来讲解，一些连接这些骨架的血肉被忽略，阅读的时候总会感觉哪里缺一点什么。
- 分析源代码可以让你掌握各种细节，但是如果连整体概念都没有，这会让自己陷入到细节的汪洋大海之中。
- 写代码我感觉是最好的方式，主要是相关资源较少，只是覆盖了内核较小的一部分。

### 用户层需要理解
内核的核心模块(mm, process, fs 等)都是或多或少有用户态的知识点。推荐[The Linux Programming Interface](https://book.douban.com/subject/4292217/) 作为查询手册。

### 参考书
然后就可以阅读[Understand Linux Kernel](https://book.douban.com/subject/1767120/), 这是大家公认的经典教材，唯一的问题和现在和内核有些出入。

[奔跑吧Linux内核](https://book.douban.com/subject/35283154/) 我阅读了其中部分章节，感觉还行。

如果你恰好有一台龙芯电脑，并且想要了解 MIPS 内核的实现，[用"芯"探核 基于龙芯的 Linux 内核探索解析](https://book.douban.com/subject/35166926/) 可以说相当的不错。

### 写代码
[Linux Device Driver](https://lwn.net/Kernel/LDD3/)(ldd3)是公认的Linux设备驱动最佳教材之一，该书着重介绍了设备驱动，但是同时也介绍了大量内核的其他方面的知识，例如中断，时间，锁等等。
可惜其中的代码非常的老，有人对于 ldd3 的代码向高版本内核进行了移植，[代码](https://github.com/martinezjavier/ldd3)在github上。

我个人更加推荐[linux kernel labs](https://linux-kernel-labs.github.io)，以试验的形式学习，各种环境配置齐全，文档详细，更新活跃。

国内的有人做的项目 [tinyclub linux lab](https://github.com/tinyclub/linux-lab) 虽然没有阅读过，但是感觉还不错。

### 看 blog
[lwn](https://lwn.net/Kernel/Index/) 是最权威，最丰富的资料来源，

### 其他的学习资源
我在自己的学习过程中间收集了一些资源，放到了[这里](https://github.com/Martins3/Martins3.github.io/blob/master/os/os-route.md), 可以作为参考。

转发 **CSDN** 按侵权追究法律责任，其它情况随意。
