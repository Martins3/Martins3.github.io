# 内核学习经验
最重要的问题, 你为什么要去学习内核 ? 可以参考[陈硕](https://www.zhihu.com/question/20541014/answer/93312920)的回答。

不同的人基础不同，需求不同，知识范围不同，看内核的方式也是不尽相同的。
其实闻道有先后，术业有专攻，要想成为内核工程师，首先要成为一个工程师，
更多时候应该思考的事情是如何写 bug-free 的代码，如何深入理解计算机的本质，
如何使用计算机解决问题，如何发现问题，如果和他人高效的合作，而不是像孔乙己那样，
炫耀内核里面的“回”有多少种类的写法。没有必要无脑鼓吹内核。

总体来说，内核是系统软件的基础和核心，无论是做系统软件的研究还是从事相关的工作，学习内核还是有必要的。

内核相关的工作我感觉主要集中在下面部分:
1. 存储 : 需要理解 IO 栈, 但是内核之外的很多事情需要懂，比如 ceph 之类的分布式数据库；
2. 网络 : 内核实现 TCP / IP 等网络栈协议；
3. 驱动 : 各种硬件厂商的硬件需要对应驱动；
4. 虚拟化 : 云计算厂商需要 kvm QEMU 等虚拟化技术，cgroup 和 namespace 之类的容器技术。

但是还存在一些方向:
1. 现在芯片行业很火(2021), 高性能芯片需要软硬协同，操作系统是系统软件的关键一环。
2. 云计算厂商对于异构计算来加速很有兴趣，这些都是软硬件结合的东西。

所以，一般来说，可以根据自己的需求有侧重的阅读，
至于内存管理，调度器之类的核心模块相关的工作，应该是少的，
但是这些内核最基础的模块，也是需要被好好掌握的, 而且面试的时候，这些都是被最频繁的提问的东西。

## 理论基础
- [深入理解计算机体系结构](https://book.douban.com/subject/26912767/)
- [陈海波教授的操作系统课程](https://ipads.se.sjtu.edu.cn/mospi/)
- [谭志虎教授的计算机组成原理](https://www.ryjiaoyu.com/book/details/42720)
- 一个上万行的小型操作系统，比如清华的[ucore](https://github.com/chyyuu/ucore_os_lab) [^1] 或者 [xv6](https://github.com/mit-pdos/xv6-riscv)

<!-- ## 心理准备 -->

## 环境准备
1. 阅读环境准备
    - 首先你要保证你的操作系统是 Linux 内核，让自己对于 kernel 支撑起来的用户态环境有一个感性的认识。
2. 保证你的编辑器可以正确跳转，如果你恰好使用的是 neovim, 可以参考[我的 neovim 配置](https://github.com/Martins3/My-Linux-Config)
    - 或者 VSCode 的配置: [DKernel-Plus](https://github.com/ShaoxunZeng/DKernel-Plus)
3. [QEMU, dataframe 和 bpftrace](./tips-reading-kernel.md) 等工具也是可以大大加快分析的速度的。

## 内核学习
我个人认为需要将**理解用户态**, **读书**, **分析源代码**, **写代码**。
- 理解用户态是基础，好在用户态的资料一般都丰富，一般问题不大。
- 读书可以快速掌握设计思路，但是有时候感觉像是背字典一样，作者往往只会将代码中间的骨架挑出来讲解，一些连接这些骨架的血肉被忽略，阅读的时候总会感觉哪里缺一点什么。
- 分析源代码可以让你掌握各种细节，但是如果连整体概念都没有，这会让自己陷入到细节的汪洋大海之中。
- 写代码我感觉是最好的方式，主要是相关资源较少，只是覆盖了内核较小的一部分。

### 用户层需要理解
内核的核心模块(mm, process, fs 等)和用户态的联系非常的紧密。
虽然很多人推荐 Richard Stevens 的 Advanced Unix Programming，但是我更加推荐 [The Linux Programming Interface](https://book.douban.com/subject/4292217/)。
Richard Stevens 英年早逝，这导致 Advanced Unix Programming 这本书接近 20 年没有更新了，书中的很多例子实际上运行起来有一些小毛病。

### 参考书
[奔跑吧 Linux 内核](https://book.douban.com/subject/35283154/) 我阅读了其中部分章节，感觉还行，比较推荐。

[Understand Linux Kernel](https://book.douban.com/subject/1767120/), 这是大家公认的经典教材，唯一的问题和现在和内核有些出入。

如果你恰好有一台龙芯电脑，并且想要了解 MIPS 内核的实现，[用"芯"探核 基于龙芯的 Linux 内核探索解析](https://book.douban.com/subject/35166926/) 可以说相当的不错。

### 写代码
[Linux Device Driver](https://lwn.net/Kernel/LDD3/)(ldd3)是公认的 Linux 设备驱动最佳教材之一，该书着重介绍了设备驱动，但是同时也介绍了大量内核的其他方面的知识，例如中断，时间，锁等等。
可惜其中的代码非常的老，有人对于 ldd3 的代码向高版本内核进行了移植，[代码](https://github.com/martinezjavier/ldd3)在 Github 上。

[jserv](https://github.com/jserv) 写了更加现代版本的内核驱动教程, [The Linux Kernel Module Programming Guide](https://github.com/sysprog21/lkmpg).

我个人更加推荐 [Linux kernel labs](https://linux-kernel-labs.github.io)，以试验的形式学习，各种环境配置齐全，文档详细，更新活跃。

国内的有人做的项目 [tinyclub linux lab](https://github.com/tinyclub/linux-lab) 虽然没有阅读过，但是感觉还不错。

### 看 blog
[lwn](https://lwn.net/Kernel/Index/) 是最权威，最丰富的资料来源，

### 其他的学习资源
我在自己的学习过程中间收集了一些资源，放到了[这里](https://github.com/Martins3/Martins3.github.io/blob/master/os/os-route.md), 可以作为参考。

### 个人想法
我个人感觉，如果想要真正深入理解内核，其实是要向上和向下扩展才可以的:
- 深入理解硬件的原理，至少要搞搞 FPGA，写写 CPU
- 深入理解应用的需求

当然从简单的来说，QEMU 应该是每个搞 Linux 最后都会掌握的吧。

[^1]: https://github.com/rcore-os 以及 https://github.com/LearningOS 中利用 rust 来构建操作系统，也许更加现代化。

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
