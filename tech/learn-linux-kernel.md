---
title: "内核学习经验"
date: 2019-11-03T15:19:47+08:00
draft: false
categories: [kernel]
---

## 为什么要学习Linux内核

我反对无差别的鼓励学习Linux内核。虽然学习内核的好处多不胜数，但是投入的时间学习其他的东西也许收获更大。
本科的学习大学物理，教育部的想法是大学物理这么基础的东西，学了肯定没有什么坏处，可以提升一个人的底蕴，
但是大学物理和大学物理试验(大学物理实验的报告非常的官僚主义)几乎吃掉整个大一的时间，而操作系统，计算机网络，组成原理等专业核心课 却被全部塞到一个学期上。大学物理由于在平时的工作学习中间没有使用的余地，被忘得一干二净，而专业核心课学的不牢固，让进一步的工作学习都受阻。
学习内核就像是学习物理，如果你是物理专业的，投入再多时间也不为过，如果你是其他专业的，应该依据专业相关程度进行学习，毕竟人的精力是有限的。

不同的人的基础不同，从事的方向不同，对于是否学习，学习什么，学习到什么样的程度，其目标应该是不同的。
我希望大家不要在公众号看了一篇类似于，"为什么程序员都必须懂点Linux内核" 之类的文章就非常的紧张，觉得自己不懂内核就怎么样了。
知乎上有一个[回答](https://www.zhihu.com/question/20541014/answer/93312920)，陈硕的回答非常好，大家可以参考一下。

## 阅读代码工具
我自己是使用vim阅读代码，具体如何配置，我之前写过一个相关的[文章](https://www.jianshu.com/p/249850f2cc64)，其实使用什么编辑器无所谓，但是一定要有**定义**和**引用**查找的功能。
图形化的工具，我使用过[sourcetrail](https://www.sourcetrail.com/)，效果很不错，就是吃配置。

## 学习的路线
github 有一个叫[low level programming university](https://github.com/gurugio/lowlevelprogramming-university/blob/master/README_cn.md)的项目中有讲到学习Linux内核，也可以参考一下，现在我谈谈自己的想法。

本科操作系统课程[Operating Systems: Three Easy Pieces](http://pages.cs.wisc.edu/~remzi/OSTEP/) 是基础，有了理论基础，不要着急，可以开始掌握一个简化版的内核，比如linux0.1.1，ucore 或者xv6。
清华的[ucore](https://github.com/chyyuu/ucore_os_lab) 试验几乎是Linux内核的简化版，完成ucore 可以获取大致的脉络，个人认为写的很好的。如果这种只有两三万行的小工程不看就直接看一个成熟的高版本内核，会非常容易迷失。

[Linux Kernel Development](https://book.douban.com/subject/3291901/)(以下简称lkd) 的作用是让你对于Linux内核有一个大致的了解，可以简单翻翻。
然后就可以阅读[Understand Linux Kernel](https://book.douban.com/subject/1767120/)(一下简称为ULK), 这是大家公认的经典教材，
这本书的问题在于其中的内容有点老，但是我个人认为写的比 
[Professional Linux Kernel Architecture](https://book.douban.com/subject/3244090/)(一下简称为PLKA)好，plka为了避免偏向于某一个架构，对于架构相关的内容几乎都是一笔带过，所以有的内容显得不是非常的通透，
我个人感觉ULK更加强调设计思想，PLKA有时候有点像是对于代码的罗列。

当ULK看个大概之后，我相信你已经可以靠 LWN 和kernel documentation 独立自助解决你的问题了。

## 学习经验和体会

#### 用户层需要理解
内核中间几乎所有的章节都是有对应的用户态的知识点。推荐[The Linux Programming Interface](https://book.douban.com/subject/4292217/)(一下简称tlpi)，有人评价此书比APUE强。不建议首先通读本书再去学习内核，这一本书是一个手册一样的东西，内容详尽，也非常厚，看一遍不是一件容易的事情，搞不好后面看完前面忘，所以我建议，内核学到哪里，在本书对应的章节看到哪里即可。

#### 及早确定内核文件树布局
刚刚接触内核，对于内核的理解就是代码量巨大，非常的复杂，感觉自己被扔到一个无边无尽的大海中间一样，学起来非常的压抑。更加的糟糕的是，对于新学习的内容，不知道其在内核中间的位置，无法确定和已经学习的内容的关系。
所以，我建议及早确定内核文件树布局。

方法很简单，对于 mm/ kernel/ fs/ 这几个文件夹中间的文件逐个分析，看看里面都是在做什么的，找到其中的系统调用，搞清楚这些系统调用的使用方法。

我这里举一个例子: https://gitee.com/martins3/Notes/blob/master/kernel/plka/syn/mm/mem_overview.md

#### 理解汇编
如果不理解汇编语言，最后对于操作系统的理解用于只会是夹生的状态，这一部分的学习我推荐[这个项目](https://github.com/cirosantilli/x86-assembly-cheat)

#### 写代码才是学习方法
[Linux Device Driver](https://lwn.net/Kernel/LDD3/)(ldd3)是公认的Linux设备驱动最佳教材之一，该书着重介绍了设备驱动，但是同时也介绍了大量内核的其他方面的知识，例如中断，时间，锁等等。
可惜其中的代码非常的老，有人对于 ldd3 的代码向高版本内核进行了移植，[代码](https://github.com/duxing2007/ldd3-examples-3.x.git)在github上。

我个人更加推荐[这个项目](https://linux-kernel-labs.github.io)，以试验的形式学习，各种环境配置齐全，文档详细，更新活跃。

国内的有人做的项目 [tinyclub linux lab](https://github.com/tinyclub/linux-lab) 虽然没有阅读过，但是感觉还不错。

#### 获取最新的文档
学习内核，你会发现很多书的内容都非常老，Documentation/下很多文档也非常老，最新的文档都在 https://lwn.net/Kernel/Index/ 

## 其他的一些学习资料
- [https://sysctl-explorer.net/](https://sysctl-explorer.net/)
- [linux-kernel-labs](https://linux-kernel-labs.github.io/master/index.html)
- [linux-insides](https://github.com/0xAX/linux-insides) PLKA为了避免陷入架构的细节中间，众多可以辅助理解底层信息要么不说，要么一笔带过，linux-insides详细分析了基于amd64架构的中断异常，syscall和锁机制。其他关键内容也有提及
- [奔跑吧 Linux内核 入门篇](https://book.douban.com/subject/30645390/) 和 [奔跑吧 Linux内核](https://book.douban.com/subject/27108677/) 并没有阅读，看目录感觉内容还不错，入门篇中每章后面还有习题。
- [操作系统真象还原](https://book.douban.com/subject/26745156/) ，目前没有看，豆瓣评价还可以，有点类似于ucore文档的详细版本
- [osdev](https://wiki.osdev.org/Main_Page) 操作系统重要的参考资料来源。

- [Think OS](http://greenteapress.com/thinkos/html/index.html) 操作系统教材
- [Operating System: three easy pieces](http://pages.cs.wisc.edu/~remzi/OSTEP/) 操作系统教材
- [what cs major should know](http://matt.might.net/articles/what-cs-majors-should-know/)
- [How to Make a Computer Operating System](https://github.com/SamyPesse/How-to-Make-a-Computer-Operating-System) 非常详细和基础教程，写ucore 之前可以首先阅读此项目。

- [linux insides](https://0xax.gitbooks.io/linux-insides/content/) 
- [free programming books](https://github.com/EbookFoundation/free-programming-books/blob/master/free-programming-books.md#operating-systems) 操作系统相关的免费书籍
- [build you own OS](https://github.com/danistefanovic/build-your-own-x#build-your-own-operating-system) 写自己的操作系统

- [Linux 上的一些工具教程](http://linuxtools-rst.readthedocs.io/zh_CN/latest/index.html)
- [内核地图](http://www.makelinux.net/kernel_map/)
- [Kernel newbies](https://kernelnewbies.org)

- [linux frome scratch](http://www.linuxfromscratch.org/)
- [write your own os](http://mikeos.sourceforge.net/write-your-own-os.html)

转发 **CSDN** 按侵权追究法律责任，其它情况随意。
