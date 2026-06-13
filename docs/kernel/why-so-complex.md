# 为什么 Linux Kernel 这么复杂

## 为什么我要写这个

说实话，我在大四在龙芯实习，做的项目是将 Android 移植到 MIPS 。
对于这个项目，我是完全摸不着头脑，那个时候也很迷茫，不知道做什么， 就决定先看看
Linux 内核，也许对于项目是有推动的。

刚开始学习 Linux 内核的时候，完全无从入手。

- 打开一个文件，里面的每一个函数都不认识，每一个结构体都不认识。
- 打开一个文件夹，里面有一大堆文件夹，随便打开一个文件夹，我靠，结果里面又是一堆文件夹。

- 从那个文件开始看起?
- 那些文件是重点，那些不是重点？

我上网搜索了一下如何开始 Linux 内核编程，大多数内容都是相当简单。

1. https://gist.github.com/vegard/22200a9f91af138a99ae22a9b814a9a4
2. https://www.kernel.org/doc/html/latest/process/howto.html
3. https://www.zhihu.com/question/439569498/answer/2242340127

显然，我们是存在一些经典的书籍是可以看的， 例如 Understanding Linux kernel 或者
Procesfessional Linux kernel, Linux Device Driver
但是我不喜欢看过时的东西，里面大多数内容都对不上，
就算可以对上，一本书动辄六七百页，直接看相当乏味。

看 Linux 内核的一个很大的挑战在于没有大块的时间， 另一个挑战在于，Linux
内核有一个漫长的，痛苦的入门过程，搞不好就放弃了。 还有，Linux
内核遇到问题，基本找不到人讨论。

## Linux Kernel 具体一定的理论复杂性

1. 虚拟地址空间: 用户态为什么不能访问内核的地址空间，为什么内核可以通过 gup 访问用户态的空间
2. DMA
3. 中断和异常的区别是什么?
4. 锁机制 : 信号量 互斥锁
5. 内存页面分配: 为什么内核使用伙伴系统
6. 调度算法: eevdf 相对于 cfs 有什么好处?

不过这些你完成了 ucore 之后，你就大概了解了，接下在才是真正的折磨

## Linux kernel 的容量极大

之前我提到 QEMU 非常复杂
QEMU 的复杂性在内核面前不值一提

QEMU 的邮件 channel 实际上只有一个 https://www.qemu.org/contribute/ 而 Linux
kernel 有上百个 https://subspace.kernel.org/vger.kernel.org.html

QEMU 是为了解决一个问题，如何模拟一台机器。 而 Linux kernel
由于是宏内核设计，任何需要在内核态执行的代码都需要放到内核中。

如果你仔细观察内核，你会发现，存储，计算，网络，这些项目在在用户态有一座高山，
在内核中亦有一座地基，而在地基之下，还有固件和硬件。
- kvm : QEMU libvirt 之类的
- block layer 上有ceph
- cgroup : k8s
- CPU : 熟悉所在平台的硬件，流水线的设计仿真

对于 QEMU 我勉强可以知道其能力范围，对于 Kernel ，我一直计划搞一个 module 依赖可视化。

Linux kernel 实现了大量各类协议:
scsi /  acpi / nvme / tcp / ip / RDMA / fc / nfs
这里随便拿出来一个，其 spec 或者 rfc 就是上千页。


> There are so many features in the Linux kernel it sometimes blows my mind. eventfd, signalfd, timerfd, memfd, pidfd. The whole fricking tc/qdisc featureset (OMG). netlink. io_uring. criu. SO_REUSEPORT. Teaming. Namespaces. veths. vsocks. Dpdk/netmap/af_packet. XDP ! Seccomp.
> I mean look at that https://developers.redhat.com/blog/2018/10/22/introduction-t...
>
> Amazing.
>
> https://news.ycombinator.com/item?id=27328285

即便是从事相关工作很久了， 还是时不时可以发现一个目录我从来没见过，
每当这个时候，我就说到， "你还有多少惊喜是朕不知道的。"
## 对于性能的不断的提升

- io 接口从 read readv preadv aio io_uring 的演化
- 存储中 single queue 到 multiqueue 的演化
- 网络中，单纯的 tcp / ip 网络协议支持，到各种 tunnel , overlay , offload , ebpf/xdp 和 rdma
- 计算中，多核的支持
- 虚拟化，kvm iommu vfio sriov 到 siov
- 总线: ide pcie bus cxl
- GPU / DRM : 暂不展开
- 音频: 暂不展开

1. 从简单的 spinlock/mutex 到各种面向特定场景优化的
2. smp / numa / socket 居然让时钟的同步也变的很复杂了

很长一段时间，Linux 内核只能使用 gcc 编译。

## 支持不断演化的硬件

- CPU 架构支持: alpha x86 arm long
- 存储 : scsi (hba fc) -> nvme
- 网络 : rdma

存储，计算，网络不断的融合
1. nvme over fc / nvme over rdma
2. 智能网卡

如果你不去理解硬件，软件也是往往理解不了的:
1. 你需要大致理解超标量 CPU
2. 你需要了解 pcie ，不然整个设备相关的，从驱动到虚拟化基本上都没办法理解

## 硬件，编译和操作系统从来都是互相耦合的
操作系统厂商，例如微软或者 Apple 都是有自己编译器:
微软就不说了，Apple 有自己的 swift ，而且常年资助 LLVM 社区。

## 功能需求

1. cgroup : 由于容器隔离，scheduler 和 memory management 都复杂了特别多
2. ebpf

## 内核的调试和开发都是新的一套规则
1. 普通的程序的 double free 之类的会导致踩内存。
内核中，很多踩内存是由于 QEMU KVM 或者固件的 BUG 。

2. 很多场景，机器一旦开机，往往不可以重启，所以内核社区总是在捣鼓 live patch 功能。


## 无可避免的耦合
1. 一个模块就很复杂，例如 memory management (mm) ，而且 mm 和 fs 和 storage
系统本来就是耦合的。我在写文件系统的时候，我会发现的文件系统的 page cache 很难正确管理，

2. memory policy, cgroup cpuset, cgroup memory 三个位置同时可以限制内存使用 memory 中 Transparent Huge Page

### 不使用、不关心的代码还是会干扰你来分析

各种内存管理都看到了 cgroup，这个时候才发现原来存在一个机制叫作 cgroup，
结果发现怎么用都不会，直接看原理不就是上天。

为了理解 softirq ，看一个具体的例子，结果发现 nvme 的代码更加多
这也是非常经典的问题，就是

这个例子其实还好，因为 softirq 的用户就是 nvme

1. 在生产中 kvm 嵌套虚拟化是不用的， kvm 的嵌套虚拟化

内核不是教程，而是终于知道原来内核中使用 task_struct 描述进程
但是打开 task_struct 一看，发现里面存在几百个字段。

## accidental complexity

- incomplete transitions
  - cgroup 中同时存在 v1 和 v2 机制
  - x86 kvm : 如果 CPU 不支持这个功能，就用低效一点的方法，如果 CPU 支持，那么就用硬件。最经典的就是影子页表。
  - x86 各种模式的兼容
- 重复逻辑以及缺乏抽象: raid 模块已经大量的驱动，这都是 Linus 认为可以用 Rust 的部分。

## 意想不到的复杂

### 向上向下，都需要熟悉

只是掌握内核是解决不了问题的，
内核的复杂绝对不是因为他本身的代码量巨大，而是因为他只是解决问题的一部分。

用户态程序，总体可控，毕竟很多也是开源的但是出现了问题，如何判断是用户态程序的问题还是内核的问题?
而且很多时候，整个项目大部分都是在用户态，怎么办?

和硬件相关的大多数都是驱动，但是有些驱动不得不掌握，例如
1. 驱动的 DMA 和 中断都和 iommu 相关 : 做虚拟化必须看，但是很痛苦
2. ARM 的 GIC / x86 的 IO-APIC APIC : 不看，中断虚拟化没法做
3. mlnx 网卡驱动/ megaraid 驱动 : 不管看不看，都很痛苦

如果固件有 bug ，如何判断是驱动的问题还是固件的问题?


## 解决办法
1. proc sysfs 导出信息
2. fuse , qemu + kvm , tun tap 等，尽量让业务在用户态
3. 尽量模块化
4. 反复的重构，例如 Christoph Hellwig
1. kdump / kexec : 对于内核宕机不再束手无策
2. perf ftrace : 解决性能问题

由于内核中驱动成千上万，导致内核需要一个通用框架:
1. 中断的 irqdomain
2. sysfs 对于驱动
3. 驱动的 bind 支持
4. lock 需要兼容各种 memory model 和同步原语

## 下一步
1. https://github.com/vvaltchev/tilck : 和其他各种建议操作系统的对比
1. 可以对比一下 QEMU 的 rcu 和 kernel 的 rcu 的差别
   - https://www.kernel.org/doc/html/latest/RCU/Design/Requirements/Requirements.html#linux-kernel-complications
2. 开发用户态驱动和内核驱动
   - 直接写内核的文件系统和 fuse ?

## 参考
- https://www.zhihu.com/question/35484429/answer/62964898

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
