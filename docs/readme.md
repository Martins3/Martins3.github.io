<p align="center">
  <p align="center">
      <img src="https://github-readme-stats.vercel.app/api?username=Martins3&count_private=true" />
      <img src="https://repobeats.axiom.co/api/embed/204d4f971425aa6d3eac4ea0bff2787d28d999a2.svg" />
  </p>
  <p align="center">
    <a href="https://996.icu"><img src="https://img.shields.io/badge/link-996.icu-red.svg" alt="996.icu" /></a>
    <a href="https://wakatime.com/@7be5bddf-f650-4cd0-a1d5-02c16f6a74f4"><img src="https://wakatime.com/badge/user/21daab89-a694-4970-88ed-a7d264a380e4.svg" alt="Total time coded since Feb 8 2020" /></a>
    <a href="https://github.com/Martins3/Martins3.github.io/commits/master"><img src="https://img.shields.io/github/commit-activity/w/martins3/martins3.github.io"></a>
  </p>
  <p align="center">
    <a href="https://martins3.substack.com">订阅</a>
  </p>
</p>

## Collections
- [slides](https://martins3.github.io/slides/)

## Virtualization && Binary Translation
#### Dune
- [Loongson Dune : A Process Level Virtualization framework Base on KVM](https://github.com/Martins3/loongson-dune)

#### 裸金属二进制翻译器的设计和实现
设计思想可以直接参考[硕士毕业论文以及答辩 PPT](https://github.com/Martins3/Bare-Metal-Binary-Translator)，以下是技术细节
- 🚧 [裸金属二进制翻译器的软件架构](./bmbt/2-arch.md)
- 🚧 [裸金属二进制翻译器的技术细节](./bmbt/3-tech.md)
- [淦，写一个裸金属二进制翻译器不可能这么难](./bmbt/4-emotion.md)

#### QEMU 源码分析
- [QEMU 的基本使用方法](./qemu/manual.md)
- [QEMU 源码概叙](./qemu/introduction.md)
- [QEMU 初始化过程分析](./qemu/init.md)
- [QEMU 的 memory model 设计](./qemu/memory.md)
- [QEMU 的 softmmu 设计](./qemu/softmmu.md)
- [QEMU 中的 map 和 set](./qemu/map.md)
- [QEMU softmmu 访存函数集整理](./qemu/softmmu-functions.md)
- [QEMU 中的 seabios : 地址空间](./qemu/bios-memory.md)
- [QEMU 和 seabios 的数据传输协议:`fw_cfg`](./qemu/fw_cfg.md)
- [QEMU 如何加载 Linux kernel image](./qemu/load-kernel-image.md)
- [QEMU 的参数解析](./qemu/options.md)
- [QEMU 中的线程和事件循环](./qemu/threads.md)
- [QEMU 中的锁](./qemu/cpus.md)
- [QEMU 如何模拟中断](./qemu/interrupt.md)
- [QEMU 中的面向对象 : QOM](./qemu/qom.md)
- [QEMU 中的时钟](./qemu/timer.md)
- [QEMU 字符设备模拟](./qemu/char.md)
<!-- - 🚧 [QEMU 二进制翻译基础](./qemu/tcg.md) -->
<!-- - 🚧 [QEMU 时钟模拟](./qemu/timer.md) -->
<!-- - 🚧 [QEMU 如何模拟 PCI 设备](./qemu/pci.md) -->
<!-- - 🚧 [seabios 源码分析](./qemu/seabios.md) -->
<!-- - 🚧 [QEMU Hash Table 设计](./qemu/qht.md) -->
<!-- - 🚧 [QEMU Hotplug 和 Reset](./qemu/reset.md) -->
<!-- 介绍 libvirt -->

## 淦，打一把英雄联盟不可能这么难
这里介绍三种方法在 Linux 上打英雄联盟的方法和背后的原理。

- 🚧[双系统（一）: 块设备](./lol/blockdev.md)
- [双系统（二）: bootloader](./lol/grub.md)
- [wine : 利用系统调用虚拟化来在 Linux 上模拟运行 Window 程序](./lol/wine.md)
- 🚧[VFIO : 利用设备直通将 GPU 提供给 Guest 使用](./lol/vfio.md)

## UEFI
- [Linux 下 UEFI 学习环境搭建](./uefi/uefi-linux.md)
- [UEFI 入门](./uefi/uefi-beginner.md)

## Micro Architecture

- [如何设计一个成功的指令集架构](./cpu/arch-design.md)
<!-- - 如何设计一个 Hypervisor，通过对比 HyperV, Xen 和 ESXi -->
<!-- - 如何设计一个虚拟化指令 -->

<!-- #### BOOM 源码阅读: 从静态 5 级 MIPS 流水线到乱序多发射 RISC-V CPU -->
<!-- - [准备工作]() -->

## Compiler

<!-- ### Lua 解释器源码分析 -->

## Tips
- [计算机学习的一点经验之谈](./learn-cs.md)
- [虚拟化学习的一点经验之谈](./learn-virtualization.md)
- [使用 Github 记录笔记和搭建 blog](./setup-github-pages.md)

## PCIe
<!-- - 🚧 [PCIe 的基本原理](.) -->
<!-- - 🚧 [Seabios 如何探测 PCIe](.) -->
<!-- - 🚧 [Linux Kernel 如何管理 PCIe 设备](./pci/kernel.md) -->
<!-- - 🚧 [QEMU 如何模拟 PCIe 设备](.) -->

## Tools
- [My Linux Config](https://martins3.github.io/My-Linux-Config/)
- [Mac，将就着用吧](./mac.md)
- [年轻人的第一次攒机](./hw/machine.md)

## Linux Environment Programming
- 🚧 [musl 阅读笔记](./linux/musl.md)
- 🚧 [认识 ELF](./linux/elf.md)
- 🚧 [Debugger 的理念，原理和使用](./linux/gdb.md)

## Linux Kernel
- 内核学习
  - [内核学习的一点经验之谈](./kernel/learn-linux-kernel.md)
  <!-- - 熟悉基本使用 /proc /sys -->
  <!-- - 使用 QEMU 来学习内核 -->
  <!-- - 使用 ebpf 来学习内核 -->
  <!-- - 使用 crash 来学习内核 -->
  <!-- - 使用 drgn 来学习内核 -->
  <!-- - 使用 flamegraph 来学习内核 -->
  <!-- - 使用 kcov 来学习内核 -->
  <!-- - 使用 perf scirpts 来学习内核 -->
  <!-- - 各种 fuzzer -->
  <!-- - 各种 tlpi 学习内核，搭建一个更加清楚的 tlpi 环境 -->
  <!-- - [使用 QEMU, FlameGraph 和 bpftrace 阅读内核](./kernel/tips-reading-kernel.md) -->
- [tty 到底是什么](./kernel/tty.md)
- [mknod](./kernel/mknod.md)
- 内存管理
  - [oom](./kernel/mm-oom.md)
  - [memblock](./kernel/mm-memblock.md)
  - [cma](./kernel/mm-cma.md)
  - [sparse vmemmap](./kernel/mm-vmemmap.md)
  - [watermark](./kernel/mm-watermark.md)
  - [rmap](./kernel/mm-rmap.md)
  <!-- - [singal 和 syscall restart](./kernel/signal-pending.md) -->
<!-- -  🚧 [folio](./kernel/mm-folio.md) -->
<!-- - [swap](./kernel/swap.md) -->
<!-- - [为什么 Linux 6.0 相比于 Linux 0.1 复杂那么多](./kernel/why-so-complex.md) -->
<!-- - 🚧 [syscall](./kernel/syscall.md) -->
<!-- -  🚧 [Linux 设备模型](./kernel/device.md) -->
<!-- - [irq domain](./kernel/irq-domain.md) -->
<!-- -  🚧 [LWN 阅读笔记](./lwn.md) -->
<!-- -  🚧 [softirq](./kernel/softirq.md) -->
<!-- - [iommu 基本原理介绍](.) -->
<!-- - /proc/cpuinfo -->
<!-- - kvm -->
<!--   - shadow page table -->
<!--   - nested virtualization -->
<!-- - page fault 总结 -->

<!-- ## Database -->
<!-- - [leveldb 源码分析](./database/leveldb.md) -->

## Loongson
- [X86 上阅读 Loongarch 内核](./loongarch/hacking-ccls.md)
- [使用 3A5000 作为我的主力机](./loongarch/neovim.md)

## Multiprocessor Programming
- [Quiescent consistency，Sequential consistency 和 Linearizability](./concurrent/linearizability.md)
- [wait free，lockfree 和 obstruction free 区分](./concurrent/lock-free.md)
<!-- - 🚧 [memory model](./concurrent/memory-model.md) -->
<!-- - 🚧 [volatile 关键字说明](./concurrent/volatile.md) -->

## Potpourri
- [what is x86 IA-32 IA-64 x86-64 and amd64 ?](./x86-names.md)
- [言论](./words.md)
- [Martins3 的 Check Sheet](./sheet.md)

## Guff
- [About](./abaaba/about.md)
- [2021 秋招总结](./abaaba/job.md)
- [有缘再见，龙芯](./abaaba/loongson.md)

## For Girlfriend
- [How long will I love you](https://martins3.github.io/theday/)
- [Garden](http://martins3.gitee.io/garden/)

## Friends
- [niugenen](https://niugenen.github.io/)
- [limaomao821](https://limaomao821.github.io/)
- [foxsen](https://foxsen.github.io)
- [SPC 的自由天空](https://blog.spcsky.com/)
- [utopianfuture](https://utopianfuture.github.io/)
- [xieby1](https://xieby1.github.io/)
- [qaqcxh](https://qaqcxh.github.io/Blogs/)


<!-- @todo 将 blog 再完善一下，然后投稿到 https://github.com/timqian/chinese-independent-blogs -->

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
