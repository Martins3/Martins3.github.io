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

```txt
☁️☁️🌞       ☁
     ☁  ✈     ☁    🚁
  🏬🏨🏫🏢🏤🏥🏦🏪
👬🌲 /  🚶 |🚍   \🌳👫👫
  🌳/  🚘  |🏃    \🌴🐈
🌴 /       |🚔     \🌲👯👯
🌲/🚖      |   🚘   \🌳👭
```

## Collections

- [slides](https://martins3.github.io/slides/)

## 我的工作流
- [使用 Github Pages 来搭建 Blog](./blog/setup-github-pages.md)
- [用写作来重新思考问题](./blog/use-write-to-think.md)
- [使用 Anki 持续思考](./blog/why-anki.md)

## Virtualization && Binary Translation

#### Dune

- [Loongson Dune : A Process Level Virtualization framework Base on KVM](https://github.com/Martins3/loongson-dune)

#### 裸金属二进制翻译器的设计和实现

设计思想可以直接参考[硕士毕业论文以及答辩 PPT](https://github.com/Martins3/Bare-Metal-Binary-Translator)，以下是技术细节

- 🚧 [裸金属二进制翻译器的软件架构](./bmbt/2-arch.md)
- 🚧 [裸金属二进制翻译器的技术细节](./bmbt/3-tech.md)
- [淦，写一个裸金属二进制翻译器不可能这么难](./bmbt/4-emotion.md)

#### QEMU 源码分析

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
- [QEMU 中的面向对象 : QOM](./qemu/qom.md)
- [QEMU 字符设备模拟](./qemu/char.md)

<!-- - 🚧 [QEMU 二进制翻译基础](./qemu/tcg.md) -->
<!-- - 🚧 [QEMU 如何模拟 PCI 设备](./qemu/pci.md) -->
<!-- - 🚧 [seabios 源码分析](./qemu/seabios.md) -->
<!-- - 🚧 [QEMU Hash Table 设计](./qemu/qht.md) -->
<!-- - 🚧 [QEMU Hotplug 和 Reset](./qemu/reset.md) -->
<!-- 介绍 libvirt -->

<!-- ## 开机助手的自我修养 -->
<!-- - 固件 -->
<!-- - bootloader -->
<!-- - kernel -->

## 如何组装一台电脑

### 如何选购固态

- PCIe 基础
- DMA
- 中断
	- [QEMU KVM 如何中断注入](./kernel/irq/virt-int-inject.md)

### 如何选购内存

### 案例分析
- [年轻人的第一次攒机](./hw/1-13900k.md)

## 淦，打一把英雄联盟不可能这么难

这里介绍三种方法在 Linux 上打英雄联盟的方法和背后的原理。

- 双系统
	- seabios 和 UEFI 的启动分区
	- [bootloader](./lol/grub.md)
- 图形虚拟化
	- cirrus-vga
	- vga
	- virtio-gpu
- 设备直通
	- vfio
	- [一盘两用](./kernel/vfio/fun.md)
	- [QEMU tcg 模式设备直通](./kernel/vfio/tcg.md)
	- [vfio 如何管理中断](./kernel/vfio/int-vfio.md)
	- [remapped interrupt](./kernel/vfio/int-remapping.md)
	- [posted interrupt](./kernel/vfio/int-posted.md)
- Wine
	- [wine 基本介绍](./lol/wine.md)
	- Proton

## 调试内核的几种方法
- 从外部观察
	- crash

	- drgn
	- kvm-dmesg
	- gdb kernel
	- perf kvm

## 学习
- [重新学会如何学习](./learn.md)

## UEFI

- [Linux 下 UEFI 学习环境搭建](./uefi/uefi-linux.md)
- [UEFI 入门](./uefi/uefi-beginner.md)

## 重新思考计算机系统结构

- [如何设计一个成功的指令集架构](./cpu/arch-design.md)
- [如何设计一个成功的文件系统](./kernel/fs-design.md)
- [如何设计 Hotplug 机制](./kernel/hotplug.md)

<!-- - 如何设计一个 Hypervisor，通过对比 HyperV, Xen 和 ESXi -->
<!-- - 如何设计一个虚拟化指令 -->

## 生活技能
- [应急救护 : 深圳市直机关党员应急能力培训](./chores/emergency-medical-care.md)

## Compiler

<!-- ### Lua 解释器源码分析 -->

## Tips

- [计算机学习的一点经验之谈](./learn-cs.md)
- [虚拟化学习的一点经验之谈](./learn-virtualization.md)


## Tools

- [My Linux Config](https://martins3.github.io/My-Linux-Config/)
<!-- - [Mac，将就着用吧](./mac.md) -->

## Linux Environment Programming

- 🚧 [musl 阅读笔记](./linux/musl.md)
- 🚧 [认识 ELF](./linux/elf.md)
- 🚧 [Debugger 的理念，原理和使用](./linux/gdb.md)

<!-- ## 深入敌营 18 年 -->
<!-- - [Windows 环境配置](./kernel/windows-route.md) -->
<!-- - [Windows 驱动开发](./kernel/windows-route.md) -->
<!-- - [Windows Hyper-V](./kernel/windows-route.md) -->

## Linux Kernel

- 内核学习
  - [内核学习的一点经验之谈](./kernel/learn-linux-kernel.md)
- [tty 到底是什么](./kernel/tty.md)
- [mknod](./kernel/mknod.md)

### 综合话题

- [why kernel bypass](./kernel/why-by-pass.md)
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

<!-- ## Perfbook 阅读笔记 -->

## Loongson

- [X86 上阅读 Loongarch 内核](./loongarch/hacking-ccls.md)
- [使用 3A5000 作为我的主力机](./loongarch/neovim.md)

## 并行，并发，多核，一致性

- [Quiescent consistency，Sequential consistency 和 Linearizability](./concurrent/linearizability.md)
- [wait free，lockfree 和 obstruction free 区分](./concurrent/lock-free.md)

<!-- - 🚧 [memory model](./concurrent/memory-model.md) -->
<!-- - 🚧 [volatile 关键字说明](./concurrent/volatile.md) -->

## Potpourri

- [what is x86 IA-32 IA-64 x86-64 and amd64 ?](./x86-names.md)
- [言论](./words.md)

<!-- ## TODO 还是应该搭建一个 ai workflow -->
<!-- 1. 静态检查 -->
<!-- 2. ai 润色 -->
<!-- 3. 翻译 -->

## Guff

- [About](./abaaba/about.md)
  - https://bento.me/martins3
- [2021 秋招总结](./abaaba/job.md)
- [有缘再见，龙芯](./abaaba/loongson.md)
- [Linux 内核的本质就是英雄联盟](./abaaba/lol-vs-linux.md)
- [Linux 内核的本质就是原神](./abaaba/genshin-vs-linux.md)

<!-- - [为什么我如此讨厌 CSDN](./abaaba/csdn.md) -->
<!-- - [为什么我不旅游](./abaaba/travel.md) -->
<!-- - [为什么我不用小红书](./abaaba/xiaohongshu.md) -->

## Kernel Contribution

- https://github.com/search?q=repo%3Atorvalds%2Flinux+Xueshi&type=commits

## Friends

- [niugenen](https://niugenen.github.io/)
- [limaomao821](https://limaomao821.github.io/)
- [foxsen](https://foxsen.github.io)
- [SPC 的自由天空](https://blog.spcsky.com/)
- [utopianfuture](https://utopianfuture.github.io/)
- [xieby1](https://xieby1.github.io/)
- [qaqcxh](https://qaqcxh.github.io/Blogs/)
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
