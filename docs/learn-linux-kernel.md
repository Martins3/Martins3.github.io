# 内核学习经验
最重要的问题, 你为什么要去学习内核 ? 可以参考[陈硕](https://www.zhihu.com/question/20541014/answer/93312920)的回答。

不同的人基础不同，需求不同，知识范围不同，看内核的方式也是不尽相同的，下面谈谈我的理解。

## 环境准备
1. 阅读环境准备

首先你要保证你的操作系统是 linux 内核，让自己对于 kernel 支撑起来的用户态环境有一个感性的认识。

2. 阅读工具

具体什么类型的工具关系不大，但是一定要保证可以高效查找**定义**和**引用**, 如果靠 grep 查找，一般用不了多久就会放弃吧。

我自己是使用 vim 阅读代码，配置在[这里](https://github.com/Martins3/My-Linux-config)

图形化的工具，我使用过[sourcetrail](https://www.sourcetrail.com/)，视觉效果很不错，就是吃配置。

## 理论基础
- [深入理解计算机体系结构](https://book.douban.com/subject/26912767/) 让你对于计算机体系结构有一个大致了解, 否则在内核中间的 cache, TLB, page walk 之类很难理解。
- 本科操作系统课程, 比如[Operating Systems: Three Easy Pieces](http://pages.cs.wisc.edu/~remzi/OSTEP/) 可以让知道 kernel 在计算机体系结构的角色，kernel 的主要功能是什么。
- 一个上万行的小型操作系统，比如清华的[ucore](https://github.com/chyyuu/ucore_os_lab)，可以让你对于 os 的基本知识融汇贯通。

## 内核学习
我个人认为需要将**理解用户态**, **读书**, **分析源代码**, **写代码**。
- 理解用户态是基础，好在用户态的资料一般都丰富，一般问题不大。
- 读书可以快速掌握设计思路，但是有时候感觉像是背字典一样，作者往往只会将代码中间的骨架挑出来讲解，一些连接这些骨架的血肉被忽略，阅读的时候总会感觉哪里缺一点什么。
- 分析源代码可以让你掌握各种细节，但是如果连整体概念都没有，这会让自己陷入到细节的汪洋大海之中。
- 写代码我感觉是最好的方式，主要是相关资源较少，只是覆盖了内核较小的一部分。

### 用户层需要理解
内核的核心模块(mm, process, fs 等)都是或多或少有用户态的知识点。推荐[The Linux Programming Interface](https://book.douban.com/subject/4292217/) 作为查询手册。

### 读书
Robert Love 的[Linux Kernel Development](https://book.douban.com/subject/3291901/) 可以让你对于 Linux 内核有一个大致的了解。
然后就可以阅读[Understand Linux Kernel](https://book.douban.com/subject/1767120/), 这是大家公认的经典教材，唯一的问题和现在和内核有些出入。

[奔跑吧Linux内核](https://book.douban.com/subject/35283154/) 我阅读了其中部分章节，感觉还行。

如果你恰好有一台龙芯电脑，并且想要了解 MIPS 内核的实现，[用"芯"探核 基于龙芯的 Linux 内核探索解析](https://book.douban.com/subject/35166926/) 可以说相当的不错。

### 代码阅读
选择哪一个版本其实不是太大的问题，因为内核的核心设计思路基本不变，最好保证你的版本和阅读材料一致。

### 写代码
[Linux Device Driver](https://lwn.net/Kernel/LDD3/)(ldd3)是公认的Linux设备驱动最佳教材之一，该书着重介绍了设备驱动，但是同时也介绍了大量内核的其他方面的知识，例如中断，时间，锁等等。
可惜其中的代码非常的老，有人对于 ldd3 的代码向高版本内核进行了移植，[代码](https://github.com/martinezjavier/ldd3)在github上。

我个人更加推荐[linux kernel labs](https://linux-kernel-labs.github.io)，以试验的形式学习，各种环境配置齐全，文档详细，更新活跃。

国内的有人做的项目 [tinyclub linux lab](https://github.com/tinyclub/linux-lab) 虽然没有阅读过，但是感觉还不错。

## blog
[lwn](https://lwn.net/Kernel/Index/) 是最权威，最丰富的资料来源，下面的一些 blog 也不错:

- [LoyenWang](https://www.cnblogs.com/LoyenWang/) : 强烈推荐
- [wowotech](http://www.wowotech.net/)
- [linux inside](https://0xax.gitbooks.io/linux-insides/content/)
- [low level programming university](https://github.com/gurugio/lowlevelprogramming-university/blob/master/README_cn.md)
- [知乎专栏:术道经纬](https://zhuanlan.zhihu.com/p/93289632)
- [gatieme 的笔记](https://github.com/gatieme/LDD-LinuxDeviceDrivers)
- [dsahern's blog](https://people.kernel.org/dsahern/)
- [terenceli](https://terenceli.github.io/archive.html)
- [kernelgo](https://kernelgo.org/)

#### 学习资源
我在自己的学习过程中间收集了一些资源，放到了[这里](https://github.com/kernelrookie/doc/blob/master/awesome-kernel.md), 可以作为参考。

转发 **CSDN** 按侵权追究法律责任，其它情况随意。
