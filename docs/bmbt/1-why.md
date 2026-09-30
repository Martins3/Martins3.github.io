# BMBT 常见问题解答
3. 为什么不使用 unikernel + QEMU 的方法?
    - 现在 unikernel 的解决方案都是试图将 unikernel 放到虚拟机的 guest 态中运行，无法避免 host 的软件栈
      - 那为什么不采用的 [A Linux in unikernel clothing](https://dl.acm.org/doi/10.1145/3342195.3387526) 的方式 ?
          - 这种方法无法实现设备直通, 而且这种方案依赖于 KLM[^3] 的支持
4. 为什么不基于 captive 开发?
    - captive 自身的代码量非常大, 大约在 10 万行左右
    - captive 需要对于 x86 写对应的 asl 硬件描述语言从而生成自动翻译
    - captive 是通过构建一个 unikernel 在 kvm 中运行，其 unikernel 实现和硬件打交道的方法都是 virtio 。这和我们希望直接在虚拟上运行需求不符合。

## 一些参考的 hypervisor
- https://github.com/quic/gunyah-hypervisor : 是构建了一个 micro kernel + kernel virtual machine 比较初级的项目
- https://github.com/udosteinberg/NOVA : NOVA 可以运行多个 unmodified 的 guest kernel
- https://github.com/tandasat/MiniVisorPkg : 使用 UEFI driver 来构建 hypervisor

- https://github.com/usbarmory/tamago : baremetal golang

[^1]: [Efficient Cross-architecture Hardware Virtualisation](https://era.ed.ac.uk/handle/1842/25377)
[^3]: [Kernel Mode Linux](http://web.yl.is.s.u-tokyo.ac.jp/~tosh/kml/)
[^4]: [High Velocity Kernel File Systems with Bento](https://www.usenix.org/conference/fast21/presentation/miller)


- https://github.com/airbus-seclab/ramooflax : 这个项目目前是和我们的需求最为相似的，从 bios 启动，然后运行 hypervisor 然后将磁盘上的操作系统在 guest 中间运行, 其唯一的区别在于使用了 vt-x 的虚拟化技术
- https://github.com/matsud224/raspvisor : 在树莓派上运行的 type 1 hypervisor
- https://github.com/wbenny/hvpp/blob/master/README.md :  使用 Window 驱动, 用 C++ 写的一个 hypervisor, 可以理解为一个简单的 kvm
- [Theseus](https://github.com/theseus-os/Theseus)



## 裸金属二进制翻译器的架构

显然，一个人在一年的时间内直接写一个系统级二进制翻译器，而且还需要直接运行在裸机上，
这种事情是不可能的。 MagiXen [^1] 是基于 IA-32 [^2] 开发的，所以我们在想基于什么进行二次开发。

一种想法是基于 Captive 进行开发，但是最后还是选择的是 QEMU ，原因主要是因为:
- Captive 不存在需要写 ArchC 来描述 x86 架构，这部分可能需要写超过 10000 行的代码。 [^5]
- 组里和龙芯公司，以及产业界有很多人都理解 QEMU，如果到时候出现了问题，可以更加容易的找人请教
- QEMU 的代码质量更加可靠，如果出现问题，那么一定是自己的问题

## 如何让 QEMU 运行在裸金属上
- QEMU 依赖了外部的库
  - 主要是 glibc 和 glib
  - 实际上，
- QEMU 调用了很多系统调用，这些系统调用需要操作系统的支持

## QEMU 被重构的部分
按照 interface.md 梳理一遍吧
- memory model
- thread
- option
- qom

## 技能清单
- QEMU
  - 二进制翻译器基本原理
  - memory model
  - PCI 模拟
- Linux Kernel
  - irq domain
  - memory
    - page fault 的过程
    - 进程地址空间
- LoongArch
  - TLB
- [BTMMU](https://liuty10.github.io/TianyiLiu_files/download/btmmu.pdf)
- HSPT
- ESPT
- Dune
- Linux Programming Interface
- 深入理解计算机体系结构
- [gdb bash makefile and ...](https://missing-semester-cn.github.io/)
- busybox

### 使用网络
[gnu grub doc](https://www.gnu.org/software/grub/manual/grub/html_node/Network.html)

## minicom 的使用注意点
如果在 grub 中在 Linux Kernel 的启动项中添加上 `console=ttyS0,115200`，
那么可以就可以使用串口来获取这个 Kernel 的 dmesg 输出了，调试 3A5000 的时候发现
无法输出 "密码" 两个中文字，使用下面的参数就可以了:
```sh
minicom -D /dev/ttyUSB0 -R UTF-8
```

minicom 无法发送字符，解决方法参考 [^2]，因为 uart 不需要中断的时候同样可以
正常工作（采用 poll ） 模式，所以如果 Guest 的 shell 不能交互，那么应该是 minicom 的配置有问题。

## 还可以继续开发的事情

- [ ] 从论文中抄过来

[^1]: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=872051
[^2]: https://stackoverflow.com/questions/3913246/cannot-send-character-with-minicom

From hack to elaborate technique - A survey on binary rewriting

> https://www.usenix.org/system/files/conference/usenixsecurity16/sec16_paper_andriesse.pdf

# 二进制翻译介绍

- [FX!32](http://www-leland.stanford.edu/class/cs343/resources/fx32.pdf)
- [ ] 了解一下 Binary translation using peephole superoptimization
- [Hardware-Accelerated Dynamic Binary Translation](https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=7927147)
- https://news.ycombinator.com/item?id=33533132 : 介绍 Rosetta 2

想要普及一个新的架构是很难的，看看 ARM 的操作:
- https://www.linaro.org/

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
