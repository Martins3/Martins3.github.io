# Awesome Linux kernel
Linux 内核相关的有趣资源，欢迎补充。

<!-- ## TODO Merge this to kernel rookie -->
<!-- microvm: -->
<!-- - https://github.com/qemu/qemu/blob/master/docs/microvm.rst -->
<!-- - https://github.com/Solo5/solo5 -->

<!-- unikernel: -->
<!-- - https://ssrg-vt.github.io/hermitux/ -->
<!-- - https://github.com/solo-io/unik -->
<!-- - includeOs -->
<!-- - https://unikraft.org/getting-started/ : 2021 的一个 unikernel，所以这些 unikernel 都有什么不同吗? -->
<!-- - https://github.com/hermitcore/libhermit-rs -->
<!--  无论如何，我是知道的，有一个人已经整理好了 -->
<!--  那就是 http://unikernel.org/projects/ -->

<!-- https://github.com/ikwzm/udmabuf : user dma driver -->

<!-- 两个 CPU perf 工具，都是仅仅针对于 intel 的那一侧的: -->
<!-- https://github.com/cyring/CoreFreq -->
<!-- https://github.com/intel/msr-tools -->
<!-- https://github.com/tycho/cpuid -->

<!-- 有一个性能计数器: -->
<!-- https://github.com/opcm/pcm -->

## 计划
g. [Essential Linux Device Drivers](https://book.douban.com/subject/3088263/)

http://www.linuxfromscratch.org/lfs/view/10.0-rc1/prologue/foreword.html

## 文章合集
- [LoyenWang](https://www.cnblogs.com/LoyenWang/) :star: :star: :star: :star: :star:
- [wowotech](http://www.wowotech.net/) :star: :star: :star: :star: :star:
- [linux inside](https://0xax.gitbooks.io/linux-insides/content/) :star: :star: :star: :star: :star:
- [low level programming university](https://github.com/gurugio/lowlevelprogramming-university/blob/master/README_cn.md) :star: :star: :star: :star:
- [知乎专栏:术道经纬](https://zhuanlan.zhihu.com/p/93289632) :star: :star: :star:
- [liexusong](https://github.com/liexusong/linux-source-code-analyze) :star: :star: :star:
- [gatieme 的笔记](https://github.com/gatieme/LDD-LinuxDeviceDrivers) :star: :star:
- [泰晓科技](http://tinylab.org/categories/) :star: :star:
- [dsahern's blog](https://people.kernel.org/dsahern/) :star:
  - [The CPU Cost of Networking on a Linux Host](https://news.ycombinator.com/item?id=23189372)
- https://devarea.com/labs/#.Xm3c_XUzYUE
- https://github.com/novelinux/linux-4.x.y
- https://terenceli.github.io/archive.html
- https://unixism.net/2020/04/io-uring-by-example-article-series/


## virtualization
- [LC-3 虚拟机](https://justinmeiners.github.io/) : 只有几百行
- [dockerpi](https://github.com/lukechilds/dockerpi) : 其实是一百行左右的 Dockerfile，在其中运行 qemu 模拟 raspberrypi 的硬件环境法
- [OSX-KVM](https://github.com/kholia/OSX-KVM) : 利用 kvm 实现运行 OSX 的虚拟机
- [Docker-OSX](https://github.com/sickcodes/Docker-OSX) : 类似 dockerpi, 提供安装 [OSX-KVM](https://github.com/kholia/OSX-KVM) 的自动安装
- [macos virtualbox](https://github.com/myspaghetti/macos-virtualbox) : 提供一个脚本，在 virtualbox 中间运行 macos
- [v86](https://github.com/copy/v86/) : 使用 js 写的 x86 硬件虚拟化，可以在网页上运行机器
- [windows95 in electron](https://github.com/felixrieseberg/windows95) : 利用 v86 实现运行 windows95 在 electron 中间

## 进程
- [Kernel Thread Sanitizer](https://github.com/google/ktsan/wiki)
- [cfs](https://mp.weixin.qq.com/s?src=11&timestamp=1591080226&ver=2375&signature=K2jSqjhp-0l6XyTMq2HpeRjHcThcAXKYScK2X3SIdSh5K01c1tF723WBf3y834RS7KoK18OlSRROZe3uA8JRTwsp3p8VkIei7tRK5ibDllsoWytRhLxkolG*79iX9Jc9&new=1)

## 网络
- [epoll 原理](https://zhuanlan.zhihu.com/p/63179839)

## sysadmin
- [Linux sysadmin chanllenge](https://github.com/snori74/linuxupskillchallenge) : 一共二十个教程
- [Linux Tools Quick Tutorial](https://linuxtools-rst.readthedocs.io/zh_CN/latest/base/03_text_processing.html) : 文本处理的部分强烈推荐
- [Linux Productivity Tools](https://news.ycombinator.com/item?id=23229241)
- [Test your sysadmin skills](https://github.com/trimstray/test-your-sysadmin-skills)
- [Linux sysadmin interview questions](https://github.com/chassing/linux-sysadmin-interview-questions)
- [Java 程序员眼中的 Linux](https://github.com/judasn/Linux-Tutorial)
- [常用工具的 checksheet](https://github.com/guodongxiaren/LinuxTool/blob/master/gcc.md)
- [Linux journey](https://linuxjourney.com/) : 界面精美

## 安全
- [make linux fast again](https://make-linux-fast-again.com/)
- [a13xp0p0v](https://github.com/a13xp0p0v) : linux 安全的工程师，多个项目可以作为参考，linux-kernel-defence-map 可重点关注
- [Andrey Konovalov](https://github.com/xairy) : 很厉害
- https://github.com/r0hi7/BinExp : binary exploitation
- https://github.com/milabs/awesome-linux-rootkits
- https://github.com/xairy/linux-kernel-exploitation

## distribution
- [snakeware](https://news.ycombinator.com/item?id=23391380)
- https://github.com/linuxkit/linuxkit : 制作自己的发行版
- https://github.com/ivandavidov/minimal : mininal 发行版

## 教程
- [eudyptula](https://github.com/agelastic/eudyptula) : 划分为 20 任务，项目已停止，不过还是有价值
- [Linux kernel labs](https://linux-kernel-labs.github.io/refs/heads/master/labs/introduction.html) : 强烈推荐
- [raspberry pi os](https://github.com/s-matyukevich/raspberry-pi-os) : 利用 raspberrypi 学习内核
- https://gitee.com/tinylab
- https://devarea.com/labs
  - https://devarea.com/introduction-to-network-filters-linux/#.Xm3bn3UzYUE
  - https://devarea.com/linux-kernel-development-creating-a-proc-file-and-interfacing-with-user-space/#.Xm3biXUzYUE
- https://github.com/figozhang/runninglinuxkernel_4.0 : 奔跑吧linux内核 @todo 似乎讲解过总线

## trace
- [eBPF 介绍](https://news.ycombinator.com/item?id=22953730)
- https://github.com/zoidbergwill/awesome-ebpf
- [Debug Hacks](https://book.douban.com/subject/6799412/) 内核调试的老技术

## tiny os
- [Os tutorial](https://github.com/cfenollosa/os-tutorial) : 讲解清晰，但是部分完工
- [OS in Rust](https://github.com/phil-opp/blog_os)
- [biscuit](https://github.com/mit-pdos/biscuit) : 使用 go 写的 POSIX-subset OS
- [清华的 rcore](https://github.com/chyyuu/rCore_tutorial)
- [zcore](https://github.com/rcore-os/zCore)
    - [相关文档](https://zhuanlan.zhihu.com/p/137733625)
- [南京大学的os lab](https://github.com/sslab-gatech/cs3210-rustos-public)
    - [相关文档](https://tc.gts3.org/cs3210/2020/spring/info.html)
- [rust kernel](https://blog.stephenmarz.com/)
    - https://github.com/sgmarz/osblog
- https://github.com/MRNIU/SimpleKernel
- [How to Make a Computer Operating System](https://github.com/SamyPesse/How-to-Make-a-Computer-Operating-System) 非常详细和基础教程，写ucore 之前可以首先阅读此项目。
- [linux frome scratch](http://www.linuxfromscratch.org/)
- [write your own os](http://mikeos.sourceforge.net/write-your-own-os.html)
- [bb-kernel](https://github.com/RobertCNelson/bb-kernel/tree/am33x-rt-v5.4) : This is just a set of scripts to rebuild a known working kernel for ARM devices.
- [Write Your Own 64-bit Operating System Kernel From Scratch](https://github.com/davidcallanan/os-series)

## 文摘
- [Fuchsia Overview](https://news.ycombinator.com/item?id=23364172) : hn 关于 Fuchaia 的评价
- [知乎 : 如何学习内核 ?](https://www.zhihu.com/question/304179651/answer/561395663)
- https://embeddedbits.org/how-is-the-linux-kernel-tested/
- https://news.ycombinator.com/item?id=22987747 : telefork() 将进程发送到另一个计算机上
- [meltdown 和 spectrum 相关](https://mp.weixin.qq.com/s?__biz=Mzg2MjE0NDE5OA==&mid=2247484455&idx=1&sn=3ce685da00fb31579c08ce585bfda135&chksm=ce0d178ef97a9e98fc8e77cc57a9efe91cba607ee235d5c1e28bce23453b72298f7372b04f18&scene=0&xtrack=1#rd)
- [what cs major should know](http://matt.might.net/articles/what-cs-majors-should-know/)
- [制作一个启动到 bash 的最小内核](https://weeraman.com/building-a-tiny-linux-kernel-8c07579ae79d)
- [品读 Linux 0.11 核心代码](https://github.com/sunym1993/flash-linux0.11-talk)

## project
- [syzkaller](https://github.com/google/syzkaller/blob/master/docs/setup.md) : @todo 暂时不知道如何实现 fuzzer 的
- [kernelci](https://kernelci.org/)
- https://github.com/linux-test-project/ltp
- https://systemd.io/ 了解一下其中的内容
- https://github.com/jarun/keysniffer
- https://github.com/orhun/kmon : 内核包管理器，
- [idea4good](https://gitee.com/idea4good) : 和内核没有什么关系，只是利用 fb 和 shmem，绕过 X 来实现显示让人觉得很有意思
- [build you own OS](https://github.com/danistefanovic/build-your-own-x#build-your-own-operating-system) 写自己的操作系统

## another os
- [popcorn os](https://news.ycombinator.com/item?id=23060695) linux kernel 的基础上为异构体系统提供支持
- [minos](https://github.com/minosproject/minos) : 国人开发的 RTOS
- https://github.com/bottlerocket-os/bottlerocket : 基于linux 为容器而生的操作系统, 类似还有很多, 可以在[awesome linux containers](https://github.com/Friz-zy/awesome-linux-containers) 中间找
- https://www.freebsd.org/
- https://www.minix3.org/
- https://github.com/swimos/swim
- https://github.com/dwelch67/raspberrypi : 要啥树莓派，qemu 学习 arm 指令集
- https://news.ycombinator.com/item?id=22564665 : good fellow helps, all kinds of resources
- [太素](https://github.com/belowthetree/TisuOS) : RISCV kernel

## reference
- [https://sysctl-explorer.net/](https://sysctl-explorer.net/)
- [osdev](https://wiki.osdev.org/Main_Page) 操作系统重要的参考资料来源。
- [内核地图](http://www.makelinux.net/kernel_map/)
- [Kernel newbies](https://kernelnewbies.org)

## Book
- [奔跑吧 Linux内核 入门篇](https://book.douban.com/subject/30645390/) 和 [奔跑吧 Linux内核](https://book.douban.com/subject/27108677/) 并没有阅读，看目录感觉内容还不错，入门篇中每章后面还有习题。
- [操作系统真象还原](https://book.douban.com/subject/26745156/) ，目前没有看，豆瓣评价还可以，有点类似于ucore文档的详细版本
- [Think OS](http://greenteapress.com/thinkos/html/index.html) 操作系统教材
- [Operating System: three easy pieces](http://pages.cs.wisc.edu/~remzi/OSTEP/) 操作系统教材
- [free programming books](https://github.com/EbookFoundation/free-programming-books/blob/master/free-programming-books.md#operating-systems) 操作系统相关的免费书籍

## draft
- https://github.com/riscv/riscv-pk : It is designed to support tethered RISC-V implementations with limited I/O capability and thus handles I/O-related system calls by proxying them to a host computer.
- https://github.com/jdah/tetris-os : 一个只能玩俄罗斯方块的操作系统, 只可惜是基于 i386 架构的
- https://github.com/rust-embedded/rust-raspberrypi-OS-tutorials : 使用 rust 在树莓派上构建 os

- [一个64位操作系统的设计与实现》学习笔记](https://github.com/yifengyou/The-design-and-implementation-of-a-64-bit-os)
- https://github.com/belowthetree/TisuOS : 国人 Rust OS
- https://makelinux.github.io/kernel/map/ : kernel map
