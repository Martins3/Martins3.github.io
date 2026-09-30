<p align="center">
	<picture>
		<img alt="Star History Chart"
			src="https://star-history.dera.page/svg?repos=Martins3/Martins3.github.io" />
	</picture>
	<img src="https://repobeats.axiom.co/api/embed/204d4f971425aa6d3eac4ea0bff2787d28d999a2.svg" />

</p>
<p align="center">
	<a href="https://996.icu"><img src="https://img.shields.io/badge/link-996.icu-red.svg" alt="996.icu" /></a>
	<a href="https://wakatime.com/@7be5bddf-f650-4cd0-a1d5-02c16f6a74f4"><img
			src="https://wakatime.com/badge/user/21daab89-a694-4970-88ed-a7d264a380e4.svg"
			alt="Total time coded since Feb 8 2020" /></a>
	<a href="https://github.com/Martins3/Martins3.github.io/commits/master"><img
			src="https://img.shields.io/github/commit-activity/w/martins3/martins3.github.io"></a>
</p>

<div align="center">
<pre style="display: inline-block; text-align: left;">
<code>
☁️☁️🌞       ☁
     ☁  ✈     ☁    🚁
  🏬🏨🏫🏢🏤🏥🏦🏪
👬🌲 /  🚶 |🚍   \🌳👫👫
  🌳/  🚘  |🏃    \🌴🐈
🌴 /       |🚔     \🌲👯👯
🌲/🚖      |   🚘   \🌳👭
</code>
</pre>
</div>

<p align="center">
	<a href="https://martins3.substack.com">订阅</a>
</p>

## Collections

- [slides](https://martins3.github.io/slides/)

## 我的工作流

- [使用 Github Pages 来搭建 Blog](./blog/setup-github-pages.md)
- [日志压缩式的信息获取](./blog/stream.md)
- [用写作来重新思考问题](./blog/use-write-to-think.md)
- [使用 Anki 持续思考](./blog/why-anki.md)

## 综合总结

- [如何设计一个成功的指令集架构](./cpu/arch-design.md)
- [为什么 QEMU 这么复杂](./qemu/why-so-complex.md)
- [为什么 Linux Kernel 这么复杂](./kernel/why-so-complex.md)
- [为什么 Linux 内核中有如此多 fd](./kernel/why-so-many-fd.md)
- [Linux 内核的本质就是英雄联盟](./chatter/lol-vs-linux.md)
- [Linux 内核的本质就是原神](./chatter/genshin-vs-linux.md)
- [为什么要 kernel bypass](./kernel/why-by-pass.md)
- [interrupt, execption , softirq 和 nmi 谁可以打断谁](./kernel/nested-interrupt.md)
<!-- - [命运的织机: 各种 scheduler 杂谈](./kernel/scheduler.md) -->
<!-- - 如何设计一个 Hypervisor，通过对比 HyperV, Xen 和 ESXi -->
<!-- - 如何设计一个虚拟化指令 -->

## 技术细节

- [x86 nmi](./kernel/irq/nmi.md)
- [Linux Capabilities 简述](./kernel/security/kernel-capbility.md)

### signal

- [jobctl](./kernel/signal/jobctl.md)
- [signal 的简单测试](./kernel/signal/lab.md)
- [signal](./kernel/signal/overview.md)
- [什么地方需要检查 signal_pending](./kernel/signal/signal_pending.md)
- [signalfd](./kernel/signal/signalfd.md)
- [为什么有 sigpipe ?](./kernel/signal/sigpipe.md)
- [syscall restart](./kernel/signal/syscall-restart.md)

### fuse

- [fuse-bench](./kernel/fs/fuse/fuse-bench/README.md)
- [3fs](./kernel/fs/fuse/3fs.md)
- [fuse](./kernel/fs/fuse/fuse.md)
- [virtio-fs 简单尝试](./kernel/fs/fuse/virtiofs.md)

### nfs

- [常用命令](./kernel/fs/nfs/basic.md)
- [mini NFSv3 server](./kernel/fs/nfs/user-nfsd/README.md)
- [nfs rfc](./kernel/fs/nfs/doc.md)
- [netfs](./kernel/fs/nfs/fscache.md)
- [nfs swap 支持](./kernel/fs/nfs/nfs-swap.md)
- [samba 配置简单记录](./kernel/fs/nfs/samba.md)

### ext4

- [ext4](./kernel/fs/ext4/overview.md)
- [ext4 实现原理](./kernel/fs/ext4/doc.md)
- [ext4 的错误处理路径](./kernel/fs/ext4/error.md)
- [ext4 中的 iomap 使用](./kernel/fs/ext4/iomap.md)
- [jbd2](./kernel/fs/ext4/jbd2-orig.md)
- [ext4 基本使用](./kernel/fs/ext4/basic.md)

### aarch64 sysregs

- [aarch64 cpufeature](./kvm/aarch64/sys_regs/feature.md)
- [aarch64 sysregs 编解码](./kvm/aarch64/sys_regs/encode.md)
- [qemu 如何管理 sys_regs](./kvm/aarch64/sys_regs/qemu.md)
- [sys_regs 基础](./kvm/aarch64/sys_regs/sys_regs.md)
- [用户态访问 sys_reg 是如何模拟的](./kvm/aarch64/sys_regs/feature-user.md)

细节:

- [aarch64 ESR.ISS](./kvm/aarch64/sys_regs/id/esr.md)
- [aarch64 MPAM](./kvm/aarch64/sys_regs/id/mpam.md)
- [aarch64 ID_AA64PFR0_EL1](./kvm/aarch64/sys_regs/id/pfr0.md)
- [aarch64 PMMIR](./kvm/aarch64/sys_regs/id/pmmir.md)
- [aarch64 AA64MMFR1](./kvm/aarch64/sys_regs/id/aa64mmfr1.md)
- [aarch64 LOREGION](./kvm/aarch64/sys_regs/id/loregion.md)

## zero copy

- [概述](./kernel/zero-zopy/abstract.md)
- [存储栈 zero copy](./kernel/zero-zopy/dio.md)
- [拷贝的成本](./kernel/zero-zopy/page-copy.md)
- [pipe 和 splice](./kernel/fs/pipe/README.md)
- [network zero copy](./kernel/zero-zopy/msg_zerocopy.md)
- [devmem tcp](./kernel/zero-zopy/devmem.md)
- [vhost zero copy](./kernel/zero-zopy/vhost.md)
- [NVMe 与 PCI P2PDMA](./kernel/blk/nvme/p2pdma.md)

## memory management

- [madvise](./kernel/mm/madvise/mm-advise.md)
- [fixmap](./kernel/mm/mm-fixmap.md)


## 网络虚拟化

- [vsock](./net/net-vsock.md)

## 文件系统

- [simplefs : 一个自我教学的文件系统](https://github.com/Martins3/Martins3.github.io/tree/master/simplefs)

## Dune

- [Loongson Dune : A Process Level Virtualization framework Base on KVM](https://github.com/Martins3/loongson-dune)

## 裸金属二进制翻译器的设计和实现

[论文以及答辩 PPT](https://github.com/Martins3/Bare-Metal-Binary-Translator)

## 我的硬件

- [年轻人的第一次攒机](./hw/1-13900k.md)
- [拯救者 R9000P 2023](./hw/2-7950hx.md)
- [Asahi Linux](./hw/3-asahi.md)
- [小米笔记本 2018](./hw/10-xiaomi.md)
- [n100](./hw/3-n100.md)
- [Mac](./hw/mac.md)
- [nano kvm](./hw/nano-kvm-pcie.md)
- [REDMI K90PRO 2025](./hw/redmi.md)

## 虚拟化杂谈

- PCIe 基础
- DMA
- 中断
  - [QEMU KVM 中断注入](./kernel/irq/virt-int-inject.md)


## AI

### AI 杂谈

- [AI Infra 到底在做什么](./ai/vs-kernel.md)
  - [slides](./ai/ai-infra-vskernel.html)
- [Linux 内核如何随着 AI 来演进](./ai/with-ai.md)
- [还没结束呢！和 AI 的故事，现在才开始！](./ai/ai-is-amazing.md)

### AI 作品集

- [Linux 语音输入解决方案](https://github.com/Martins3/VibeCast)
- [nvim 翻译插件](https://github.com/Martins3/translator.nvim)
- [nvim rsync 插件](https://github.com/Martins3/rsync.nvim)
- threejs-demo
  - [Martins3](./blog/threejs-demo/index.html)
  - [刀刀](./blog/threejs-demo/daodao.html)

## 淦，打一把英雄联盟不可能这么难

本人从 2018 年开始研究如何在 Linux 上玩 LOL (手动狗头)，一下是技术背景调研:

### 双系统

- seabios 和 UEFI 的启动分区
- [grub](./grub/grub.md)

### Linux 图形栈

- cirrus-vga
- vga
- virtio-gpu

### 设备直通

- [一盘两用](./kernel/vfio/fun.md)
- [vfio 如何管理中断](./kernel/vfio/int-vfio.md)
- [remapped interrupt](./kernel/vfio/int-remapping.md)
- [posted interrupt](./kernel/vfio/int-posted.md)
- [为什么我的 GPU 直通失败了](./kernel/vfio/debug-gpu.md)

### Wine

- [wine : 如何实现系统调用虚拟化](./games/wine.md)
- Proton

- 为什么不来玩 Dota 2?

## 生活技能

- [应急救护 : 深圳市直机关党员应急能力培训](./chores/emergency-medical-care.md)

## 数学
- [为什么我决定重新学习数学了](./math/notes/math.md)

### ai
- [为什么 Decoder 使用最后一个位置预测下一个 Token](./math/ai/causal-lm-last-token.pdf)
- [GEMM Convolution](./math/ai/gemm-convolution.pdf)
- [大模型中的 PyTorch 数学大纲](./math/ai/llm-pytorch-math.pdf)
- [Attention Is All You Need](./math/ai/llm.pdf)
- [从 MLA 理论理解 vLLM 的 prefill context 实现](./math/ai/mla-vllm-prefill-context.pdf)
- [Seq2Seq Attention](./math/ai/seq2seq-attention.pdf)
- [softmax 的 max trick](./math/ai/softmax-max-trick.pdf)
- [投机解码：从接受—拒绝采样到并行验证](./math/ai/speculative-decoding.pdf)

### 应用
- [自动控制原理知识大纲](./math/applied/control-theory.pdf)
- [快速傅里叶变换：原理、GPU 加速与应用](./math/applied/fft.pdf)
- [Maxwell 方程入门教程](./math/applied/maxwell.pdf)

### 基础
- [代数知识大纲](./math/basic/algebra.pdf)
- [分析知识大纲](./math/basic/analysis.pdf)
- [微分几何知识大纲](./math/basic/differential-geometry.pdf)
- [线性代数学习笔记](./math/basic/linear-algebra.pdf)
- [概率论知识大纲](./math/basic/probability-theory.pdf)
- [复变函数](./math/basic/complex/complex-analysis.pdf)

### 有趣的问题
- [2026 AI 数学突破实录](./math/problems/ai-mathematics-breakthroughs-2026.pdf)
- [Hopf 猜想](./math/problems/hopf-conjecture.pdf)
- [三维 Jacobian 猜想反例](./math/problems/jacobian.pdf)
- [有趣的数学分析主题](./math/problems/math_analysis.pdf)
- [黎曼猜想分析](./math/problems/riemann-hypothesis.pdf)
- [孪生素数问题](./math/problems/twin-prime-conjecture.pdf)
- [Erdős 单位距离猜想](./math/problems/unit-distance-graph.pdf)

## Compiler

- [谈谈 kernel 中使用的编译技术](./kernel/compiler.md)

## 糟糕 糟糕 OH MY GOD 我的机器宕机了

- [dmesg 的基本使用](./kernel/tutorial/dmesg.md)
- [drgn](./kernel/tutorial/drgn/drgn.md)
- crash utility
  - [crash utility](./kernel/tutorial/crash/crash-utility.md)
  - [crash 内核实现](./kernel/tutorial/crash/internal.md)
  - [kdump](./kernel/tutorial/crash/kdump.md)
  - [kexec](./kernel/tutorial/crash/kexec.md)
  - [panic 的参数配置](./kernel/tutorial/crash/options.md)
  - [实战记录](./kernel/tutorial/crash/case.md)
- [pstore](./kernel/tutorial/crash/pstore.md)

## 学习

- [学习计算机经验之谈](./learn/cs-how.md)
- [为什么我喜欢计算机](./learn/cs-why.md)
- [虚拟化学习的一点经验之谈](./learn/virtualization.md)
- [内核学习经验](./learn/linux-kernel.md)
- [学习 LLM 的一点经验之谈](./learn/llm.md)
- [如何学习(draft)](./learn/how-to.md)

## Tools

- [My Linux Config](https://martins3.github.io/My-Linux-Config/)
- [X86 上阅读 Loongarch 内核](./loongarch/hacking-ccls.md)
- [使用 3A5000 作为我的主力机](./loongarch/neovim.md)
- [Fedora 使用记录 2026](./linux/fedora/doc.md)
- [tailscale 使用简单记录](./net/tailscale.md)
- [如何给 nixpkgs 添加一个新的包](./tools/nix.md)
- [clash meta 基础](./net/proxy.md)

## Potpourri

- [x86's acronyms](./x86-names.md)
- [言论](./words.md)

## 杂谈

- [关于](./chatter/about.md)
  - https://bento.me/martins3
  - [游戏](./games/games.md)
  - [影视](./chores/movies.md)
  - [可曾读过什么书](./chores/books.md)
  - [播客](./chatter/podcast.md)
- [2021 秋招总结](./chatter/job.md)
- [有缘再见，龙芯](./chatter/loongson.md)

## 固件

- [efibootmgr 和 efivar](./uefi/efibootmgr.md)

## iommu

- [IOMMU 杂谈](./kernel/iommu/overview.md)
- [iommu domain 基本概念](./kernel/iommu/iommu-domain.md)
- [blk-mq-dma](./kernel/iommu/blk-dma.md)
- [swiotlb](./kernel/iommu/swiotlb/swiotlb.md)
- [dev->dma_ops](./kernel/iommu/dma_ops.md)
- [pasid](./kernel/iommu/pasid.md)
- [dma coherence](./kernel/iommu/dma-coherence.md)
- [iommu debugfs](./kernel/iommu/debugfs/debugfs.md)
- 架构相关
  - [aarch64](./kernel/iommu/arch/aarch64.md)
  - [amd](./kernel/iommu/arch/amd.md)
  - [intel](./kernel/iommu/arch/intel.md)
- 高级话题:
  - [iommufd](./kernel/iommu/iommufd.md)
  - [SIOV](./kernel/iommu/siov.md)
  - [auxd](./kernel/iommu/auxd.md)
  - [vIOMMU](./kernel/iommu/viommu.md)

## rcu

- [doc](./concurrent/rcu/doc.md)
- [RCU 基本介绍](./concurrent/rcu/overview.md)
- [RCU 基本使用](./concurrent/rcu/usage.md)
- [rcu_read_lock_bh](./concurrent/rcu/bh.md)
- [rcu boost](./concurrent/rcu/boost.md)
- [context_tracking](./concurrent/rcu/context_tracking.md)
- [extended quiescent state](./concurrent/rcu/eqs.md)
- [harzard pointer](./concurrent/rcu/hazard-pointer.md)
- [preempt rcu](./concurrent/rcu/preempt_rcu.md)
- [rcu qs](./concurrent/rcu/qs.md)
- [rcu nocb](./concurrent/rcu/rcu_nocb.md)
- [rcu slab](./concurrent/rcu/slab.md)
- [srcu](./concurrent/rcu/srcu.md)
- [rcu stall](./concurrent/rcu/stall.md)
- [task rcu](./concurrent/rcu/tasks_rcu.md)
- [kthread && softirq](./concurrent/rcu/thread.md)
- [rcu wq](./concurrent/rcu/wq.md)
- [qemu rcu](./qemu/thread/rcu.md)
- [tree rcu](./concurrent/rcu/tree.md)

## 杂事

- [摄影摄像环境](./chores/photo-workflow.md)

## volatile

- [编译器乱序](./concurrent/volatile/doc.md)

## sanitizer

- [asan](./concurrent/san/asan.md)
  - [kasan](./concurrent/san/kasan.md)
- [kfence](./concurrent/san/kfence.md)
- [kmemleak](./concurrent/san/kmemleak.md)
- [tsan](./concurrent/san/tsan.md)
  - [kcsan](./concurrent/san/kcsan.md)
- [ubsan](./concurrent/san/ubsan.md)
- [lockdep 实现](./concurrent/san/lockdep-internal.md)
- [lockdep usage](./concurrent/san/lockdep-usage.md)
- [使用 kernel 中调试工具](./concurrent/san/misc-kernel-debug.md)

- [sanitizer 实验代码](./concurrent/san/code/README.md)

## [ ] userfaultfd

- [Userfaultfd 基础](./kernel/mm/userfaultfd/basic.md)
- [userfaultfd 高级话题探讨](./kernel/mm/userfaultfd/advance.md)
- [userfaultfd UAPI history](./kernel/mm/userfaultfd/uapi-history.md)

## 内核同步 API

- [mutex](./concurrent/kernel/api/mutex.md)
- [percpu rwsem](./concurrent/kernel/api/percpu-rwsem.md)
- [percpu](./concurrent/kernel/api/percpu.md)
- [rcuwait](./concurrent/kernel/api/rcuwait.md)
- [refcount](./concurrent/kernel/api/refcount.md)
- [rt_mutex.md](./concurrent/kernel/api/rt_mutex.md)
- [rwlock](./concurrent/kernel/api/rwlock.md)
- [R/W semaphore](./concurrent/kernel/api/rwsem.md)
- [Semaphores](./concurrent/kernel/api/semaphore.md)
- [seqlock](./concurrent/kernel/api/seqlock.md)
- [spinlock](./concurrent/kernel/api/spinlock.md)
- [swait](./concurrent/kernel/api/swait.md)
- [wait](./concurrent/kernel/api/wait.md)
- [waitbit](./concurrent/kernel/api/waitbit.md)
- [ww_mutex](./concurrent/kernel/api/ww_mutex.md)

## [ ] memory model

- [扉页](./concurrent/memory-model/memory-consistency-models-tutorial/mcm-chinese.md)
- [Front Matter](./concurrent/memory-model/memory-consistency-models-tutorial/mcm-original.md)
- [《Shared Memory Consistency Models: A Tutorial》中文总结](./concurrent/memory-model/memory-consistency-models-tutorial/mcm-summary.md)
- [实验](./concurrent/code/README.md)
- [memory model litmus 测试](./concurrent/memory-model/tests/README.md)
- [高级话题](./concurrent/memory-model/advance.md)
- [memory model](./concurrent/memory-model/doc.md)
- [Linux 内核 Litmus Tests 介绍](./concurrent/1-litmus.md)

## [ ] atomic

- [简单看看 aarch64 的指令支持](./concurrent/atomic/aarch64.md)
- [atomic 在 CPU 是如何实现的?](./concurrent/atomic/ai.md)
- [反汇编常见指令](./concurrent/atomic/cpu-arch-atomic.md)
- [记录下 x86.c 的内容](./kvm/x86.md)
- [atomic](./concurrent/atomic/atomic.md)
- [gcc atomic](./concurrent/atomic/gcc-atomic.md)
- [对比 x86 arm 和 risc-v](./concurrent/atomic/diff.md)

## [ ] 同步设计

- [c++ 的同步设计](./concurrent/lan/cpp/cpp.md)
- [glib 中也有很多](./concurrent/lan/glib.md)
- [glibc](./concurrent/lan/glibc.md)
- [golang 同步设计](./concurrent/lan/go.md)
- [pthread](./concurrent/pthread/pthread.md)
- [cpython 中锁设计](./concurrent/lan/python.md)

## [ ] 并行，并发，多核，一致性

- [Quiescent consistency，Sequential consistency 和 Linearizability](./concurrent/linearizability.md)
- [wait free，lockfree 和 obstruction free 区分](./concurrent/lock-free.md)

## QEMU 杂记

- [QEMU 概述](./qemu/introduction.md)
- [qemu 基础](./qemu/basic.md)
- [qemu 中关于 page size 问题总结](./qemu/page-size.md)
- [qemu 如何做测试的](./qemu/test.md)
- [qemu 错误处理](./qemu/error.md)
- [QEMU 中的 trace 机制](./qemu/trace.md)
- [qtest](./qemu/qtest.md)
- [qemu 文档](./qemu/docs.md)
- [qemu slirp](./qemu/slirp.md)
- [multi-process qemu](./qemu/remote.md)
- [reset](./qemu/reset.md)
- [如何给 qemu 配置 cdrom](./qemu/cdrom.md)
- [qemu channel 机制](./qemu/channel.md)
- [qht 移植](./qemu/qht.md)
- 杂谈中的杂谈
  - [e1000 的工作原理](./qemu/device/e1000-2.md)
  - [观测 e1000 驱动在 qemu 如何被模拟的](./qemu/device/e1000.md)
  - [i8042 : 键盘](./qemu/device/i8042.md)
  - [None pci device](./qemu/device/misc.md)
- [QEMU 启动代码](./qemu/init-2.md)
- [QEMU 初始化过程分析](./qemu/init.md)

### 启动配置

- [QEMU 字符设备模拟](./qemu/char.md)
- [如何正确的配置 qemu 的 memory 和 cpu](./qemu/cpu-topo.md)

### challenger

- [QEMU 的挑战者](./qemu/challenger.md)
- [libkrun](./qemu/libkrun.md)
- [Hyperlight](./qemu/hyperlight.md)

### bios

- [QEMU 中的 seabios : 地址空间](./qemu/bios/bios-memory.md)
- [如何调试 seabios](./qemu/bios/debug.md)
- [QEMU 中的 seabios : fw_cfg](./qemu/bios/fw_cfg.md)
- [QEMU 如何加载 Linux kernel image](./qemu/bios/load-kernel-image.md)
- [qboot](./qemu/bios/qboot.md)
- [seabios](./qemu/bios/seabios.md)
- [smbios](./qemu/bios/smbios.md)

### block

- [block](./qemu/block/block.md)
- [libblkio](./qemu/block/libblkio.md)
- [qemu-storage-daemon](./qemu/block/qsd.md)

### memory

- [qemu memory backend](./qemu/memory/memory.backend.md)
- [MemoryListener](./qemu/memory/memory.listener.md)
- [QEMU 的 memory model](./qemu/memory/memory.md)

### tcg

- [用户态二进制翻译](./qemu/aarch64-user/Readme.md)
- [TCG](./qemu/tcg/core-loop.md)
- [QEMU 中的 map 和 set](./qemu/tcg/map.md)
- [mttcg](./qemu/tcg/mttcg.md)
- [record / replay](./qemu/tcg/record-reply.md)
- [QEMU softmmu 访存 helper 整理](./qemu/tcg/softmmu-functions.md)
- [QEMU 的 softmmu 设计](./qemu/tcg/softmmu.md)
- [TCGContext : 如何工作的，如何维护的，作用是什么](./qemu/tcg/tb.md)
- [QEMU 二进制翻译基础](./qemu/tcg/tcg.md)

### migration

- [QEMU 热迁移基础](./qemu/migration/overview.md)
- [QEMU 热迁移文档](./qemu/migration/doc.md)
- [aarch64 cpufeature](./kvm/aarch64/sys_regs/feature.md)
- [migration status 转换](./qemu/migration/status.md)
- [background-snapshot](./qemu/migration/backgroup-snapshot.md)
- [io 后端](./qemu/migration/io.md)
- [qemu migration capability](./qemu/migration/capbility.md)
- [QEMU migration parameter](./qemu/migration/parameter.md)
- [postcopy](./qemu/migration/postcopy.md)
- [stop](./qemu/migration/stop-continue.md)
- [savevm](./qemu/migration/savevm.md)
- [CPR](./qemu/migration/cpr.md)
- [multifd](./qemu/migration/multifd.md)
- [xbzrle](./qemu/migration/xbzrle.md)
- [mapped-ram](./qemu/migration/mapped-ram.md)
- [colo](./qemu/migration/colo.md)
- [dirty rate](./qemu/migration/dirty.rate.md)

不同类型的 device 分别处理:

- [block](./qemu/migration/state/block.md)
- [vmstate](./qemu/migration/state/vmstate.md)
- [nvme](./qemu/migration/state/nvme.md)
- [virtio](./qemu/migration/state/virtio.md)
- [vfio](./qemu/migration/state/vfio.md)
- [ram](./qemu/migration/state/ram.md)
- [rom](./qemu/migration/state/rom.md)
- [vhost](./qemu/migration/state/vhost.md)
- [migration 为什么需要有优先级](./qemu/migration/state/priority.md)

其他话题:

- [migration 中一共存在那些 thread](./qemu/migration/thread.md)
- [qemu 中 yank 的含义](./qemu/migration/yank.md)
- [qemu 热插拔和热迁移](./qemu/migration/hotplug.md)
- [通过 libvirt 热迁移](./qemu/migration/libvirt.md)
- [热迁移中 share memory 会被自动 touch](./qemu/migration/zero-page/zero-page.md)
- [PCIDevice::net_failover 与热迁移](./qemu/migration/net-failover.md)

### 线程模型

- [AioContext](./qemu/thread/aiocontext.md)
- [qemu 中的 atomic 使用](./qemu/thread/atomic.md)
- [Big QEMU Lock](./qemu/thread/bql.md)
- [QEMU 中的锁](./qemu/thread/lock.md)
- [qemu rcu](./qemu/thread/rcu.md)
- [FDMonOps](./qemu/thread/fdmon.md)
- [block/graph-lock.c](./qemu/thread/graph-lock.md)
- [qemu lockcounters](./qemu/thread/lockcnt.md)
- [QEMU Event Loop](./qemu/thread/main-loop.md)
- [qemu 的 thread pool 的作用](./qemu/thread/thread-pool.md)
- [qemu thread io](./qemu/thread/overview.md)
- [qemu bh](./qemu/thread/bh.md)
- [qemu defer 机制](./qemu/thread/defer.md)
- [qemu nested aio_poll](./qemu/thread/nested-aio-poll.md)
- glib
  - [glib](./qemu/thread/glib/README.md)
  - [qemu 中的 glib event loop](./qemu/thread/glib.md)
- coroutine
  - [从 setjmp 到 coroutine](./qemu/thread/coroutine-baisc.md)
  - [coroutine](./qemu/thread/coroutine-qemu.md)

### qom

- [QEMU 中的面向对象 : QOM](./qemu/qom/qom.md)
- [qom property](./qemu/qom/qom-property.md)
- [qapi](./qemu/qom/qapi.md)
- [qmp 和 hmp](./qemu/qom/qmp-hmp.md)
- [QEMU 的参数解析](./qemu/qom/options.md)
- [qdev](./qemu/qom/qdev.md)
- 速查
	- [hmp](./qemu/qom/example/hmp.md)
	- [qemu 的 help](./qemu/qom/example/options.md)

## perfbook 阅读笔记

- AI 自动总结:
  - [Introduction](./concurrent/perfbook/autoread/chapters/02-introduction.md)
  - [Hardware and its Habits](./concurrent/perfbook/autoread/chapters/03-hardware-and-its-habits.md)
  - [Tools of the Trade](./concurrent/perfbook/autoread/chapters/04-tools-of-the-trade.md)
  - [Counting](./concurrent/perfbook/autoread/chapters/05-counting.md)
  - [Partitioning and Synchronization Design](./concurrent/perfbook/autoread/chapters/06-partitioning-and-synchronization-design.md)
  - [Locking](./concurrent/perfbook/autoread/chapters/07-locking.md)
  - [Data Ownership](./concurrent/perfbook/autoread/chapters/08-data-ownership.md)
  - [Deferred Processing](./concurrent/perfbook/autoread/chapters/09-deferred-processing.md)
  - [Data Structures](./concurrent/perfbook/autoread/chapters/10-data-structures.md)
  - [Validation](./concurrent/perfbook/autoread/chapters/11-validation.md)
  - [Formal Verification](./concurrent/perfbook/autoread/chapters/12-formal-verification.md)
  - [Putting It All Together](./concurrent/perfbook/autoread/chapters/13-putting-it-all-together.md)
  - [Advanced Synchronization](./concurrent/perfbook/autoread/chapters/14-advanced-synchronization.md)
  - [Advanced Synchronization: Memory Ordering](./concurrent/perfbook/autoread/chapters/15-advanced-synchronization-memory-ordering.md)
  - [Ease of Use](./concurrent/perfbook/autoread/chapters/16-ease-of-use.md)
  - [Conflicting Visions of the Future](./concurrent/perfbook/autoread/chapters/17-conflicting-visions-of-the-future.md)
  - [Looking Forward and Back](./concurrent/perfbook/autoread/chapters/18-looking-forward-and-back.md)
  - [Appendix](./concurrent/perfbook/autoread/chapters/A-appendix.md)
- [perfbook 阅读思考](./concurrent/perfbook/notes.md)
- [perf book](./concurrent/perfbook/overview.md)
- [perfbook 词汇表](./concurrent/perfbook/words.md)

## Yet another libvirt

- [为什么写一个这么复杂的脚本来启动 QEMU](./collei/why.md)
- [基本注意](./collei/usage.md)
- [使用 codex 重写 collei](./collei/rewrite.md)
- [利用 virtio balloon 来节省内存](./collei/balloond.md)
- 特殊功能
	- [firecracker 集成](./collei/firecracker.md)
	- [NixOS 集成](./collei/nixos.md)
	- [vmtest 集成](./collei/vmtest.md)
    - [virtme-ng 集成](./collei/virtme.md)
	- [Windows 支持](./collei/windows.md)

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

## trace

### ebpf

- [ebpf 基础](./trace/ebpf/overview.md)
- [ebpf demo](./trace/ebpf/code/libbpf/README.md)
- [bcc](./trace/ebpf/bcc.md)
- [bpftime](./trace/bpftime/basic.md)
- [cilium 初步尝试](./trace/ebpf/code/cilium/README.md)
- [bpf arena](./trace/ebpf/arena.md)
- [ebpf bloom filter](./trace/ebpf/bloom-filter.md)
- [bpftool](./trace/ebpf/bpftool.md)
- [btf](./trace/ebpf/btf.md)
- [ebpf CO:RE](./trace/ebpf/core.md)
- [ebpf 内部实现](./trace/ebpf/internal.md)
- [bpf iterators](./trace/ebpf/iter.md)
- [libbpf](./trace/ebpf/libbpf.md)
- [STRUCT_OPS](./trace/ebpf/struct_ops.md)
- [bpf syscall 的基本观察](./trace/ebpf/syscall.md)
- [bpftrace](./trace/bpftrace/readme.md)

### ftrace

- [eprobe - Event-based Probe Tracing](./trace/ftrace/eprobe.md)
- [fprobe 机制](./trace/ftrace/fprobe.md)
- [ftrace 实现](./trace/ftrace/ftrace-internals.md)
- [ftrace 输出的格式](./trace/ftrace/ftrace.md)
- [latency-collector](./trace/ftrace/latency-collector.md)
- [trace-cmd](./trace/ftrace/trace-cmd.md)
- [hwlat](./trace/ftrace/tracer-hwlat.md)
- [osnoise](./trace/ftrace/tracer-osnoise.md)

### perf

- [关于 perf 我知道的一切](./trace/perf/README.md)

### 杂项

- [trace 相关的文档](./trace/doc.md)
- [kallsyms_lookup_name](./trace/kallsyms.md)
- [kprobe](./trace/kprobe.md)
- [libtraceevent](./trace/libtraceevent/libtraceevent.md)
- [mce 的工作原理](./trace/hw/mce.md)
- [可观测简单调研](./trace/monitor.md)
- [drmemory](./trace/others.md)
- [trace](./trace/overview.md)
- [pcm](./trace/pcm.md)
- [strace 基本使用](./trace/strace.md)
- [SystemTap](./trace/systemtap.md)
- [trace 传统工具](./trace/tools.md)
- [问题调查](./trace/tracepoint-aarch64.md)
- [tracepoint](./trace/tracepoint.md)
- [用户态程序的 trace](./trace/user.md)
- [noinstr code](./trace/yes.md)

## iouring

- [iouring 基础](./kernel/iouring/overview.md)
- [io_uring 版本迭代](./kernel/iouring/version.md)
- [iouring 实现分析](./kernel/iouring/internal.md)
- [net](./kernel/iouring/net.md)
- 高级特性
  - [iouring register buffers](./kernel/iouring/register-buf.md)
  - [iouring register fds](./kernel/iouring/register-fd.md)
  - [iouring multishot](./kernel/iouring/multishot.md)
  - [iouring msg ring](./kernel/iouring/msg-ring.md)
  - [iopoll](./kernel/iouring/iopoll.md)
  - [iouring 的 cancel 设计](./kernel/iouring/cancel.md)
  - [iouring FEAT_SINGLE_MMAP](./kernel/iouring/single-mmap.md)
  - [iouring NO_SQARRAY](./kernel/iouring/no-sqarray.md)
- [iouring 对于 signal 的改造](./kernel/iouring/signal.md)
- [io wq](./kernel/iouring/wq.md)
- 周边
  - [iouring 的生态](./kernel/iouring/ecosystem.md)
  - [ublk](./kernel/iouring/ublk.md)
- 综合分析
  - [iouring 内核和用户态如何共享内存](./kernel/iouring/share.md)
  - [安全](./kernel/iouring/security.md)
  - [环形队列设计](./kernel/iouring/queue.md)
  - [同步设计](./kernel/iouring/lock.md)
  - [aio](./kernel/iouring/aio.md)
  - [bpf](./kernel/iouring/bpf.md)
- async
  - [基础](./kernel/iouring/async/basic.md)
  - [buffer io](./kernel/iouring/async/aio-buffer-io.md)
  - [buffered write](./kernel/iouring/async/buffer-write.md)
  - [epoll](./kernel/iouring/async/epoll.md)

## Linux 安全

- [selinux 到底是什么个原理](./kernel/security/security.md)

## shell

- [如何彻底征服 bash script](./shell/bash.md)
- [shell 常用命令](./shell/basic.md)
- [awk](./shell/awk.md)
- [find](./shell/find.md)
- [grep](./shell/grep.md)
- [nushell](./shell/nushell.md)
- [Regex](./shell/regex.md)
- [ripgrep](./shell/rg.md)
- [sed](./shell/sed.md)
- [unix 文本处理](./shell/text.md)

## 热插拔

- [hotplug 概述](./kernel/hp/hotplug.md)
- [qemu](./kernel/hp/qemu.md)
- [acpi 如何支持到 CPU 热插拔机制](./kernel/hp/acpi.md)
- [CPU hotplug](./kernel/hp/cpu.md)
- [memory hotplug](./kernel/hp/memory.md)
- [设备热插拔](./kernel/hp/storage.md)

## block layer
- [loop device](./kernel/blk/loop-device.md)
- [multipath](./kernel/blk/dm/multipath.md)


## vhost
- [vhost](./kernel/vhost/vhost.md)
- [vhost 重连](./kernel/vhost/reconnect.md)
- [vhost 的 feature 协商](./kernel/vhost/api-version.md)
- [vhost-user Inflight I/O Tracking 详解](./kernel/vhost/inflight-io.md)
- [vhost 协议的定义](./kernel/vhost/internal-qemu.md)
- [vhost iotlb](./kernel/vhost/iommu.md)
- [vdpa](./kernel/vhost/vdpa.md)
- [vdpa sim 实验](./kernel/vhost/vdpa-lab.md)
- [vduse](./kernel/vhost/vduse.md)
- backend
  - [DPDK 杂记](./kernel/vhost/backend/dpdk.md)
  - [vhost gpu](./kernel/vhost/backend/gpu.md)
  - [vhost-scsi 以及 vhost-user-scsi](./kernel/vhost/backend/scsi.md)
  - [SPDK 杂记](./kernel/vhost/backend/spdk.md)

## tty
- `color/`
  - [终端颜色是如何显示出来的](./kernel/tty/color/color.md)
- `hw/`
  - [hvc](./kernel/tty/hw/hvc.md)
  - [keyboard](./kernel/tty/hw/keyboard.md)
  - [pl011](./kernel/tty/hw/pl011.md)
  - [serio](./kernel/tty/hw/serio-2.md)
  - [tty driver](./kernel/tty/hw/serio.md)
  - [uart](./kernel/tty/hw/uart.md)
  - [asahi linux 如何调试](./kernel/tty/hw/usb-m1n1.md)
  - [usb serial](./kernel/tty/hw/usb.md)
  - [vcs](./kernel/tty/hw/vcs.md)
  - [vt](./kernel/tty/hw/vt.md)
- `pty/`
  - [uefi 是可以把 grub 显示到 stdio 的](./kernel/tty/fun/1.md)
  - [n_tty](./kernel/tty/pty/n_tty.md)
  - [alacritty 的 PTY 之路:pts 的 master 到底在谁手里](./kernel/tty/pty/pty-alacritty.md)
  - [pty driver](./kernel/tty/pty/pty-driver.md)
  - [pty](./kernel/tty/pty/pty.md)
- `readline/`
  - [GNU Readline 交互式 demo](./kernel/tty/readline/readme.md)
- `vim/`
  - [zellij 居然还支持 web 模式啊](./kernel/tty/vim/fun.md)
  - [什么东西](./kernel/tty/vim/libghostty.md)
  - [mini-vim：三个终端绘制后端](./kernel/tty/vim/mini-vimm.md)
  - [nvim 为什么必须借助 tmux 才可以拷贝，](./kernel/tty/vim/osc52.md)
  - [为什么内部的 ssh 有时候需要添加上这个](./kernel/tty/vim/terminfo.md)
  - [TODO 那么为什么我发现在 vim 使用的 terminal 和普通的有点不同的?](./kernel/tty/vim/vim.md)
- `virtio-port-demo/`
  - [普通 virtserialport：字节通信与 Bash shell demo](./kernel/tty/virtio-port-demo/README.md)
- `fun/`
  - [uefi 是可以把 grub 显示到 stdio 的](./kernel/tty/fun/1.md)
  - [我发现内核中存在日志](./kernel/tty/fun/3.md)
  - [同时所有的程序都可以接受消息 ?](./kernel/tty/fun/broadcast.md)
  - [ttyprintk](./kernel/tty/fun/ttyprintk.md)
  - [virsh console 和 virsh consoletty 什么关系](./kernel/tty/fun/virsh.md)
  - [tty in web](./kernel/tty/fun/web.md)
- [bmc](./kernel/tty/bmc.md)
- [console](./kernel/tty/console.md)
- [doc](./kernel/tty/doc.md)
- [wsl 的 magic](./kernel/tty/fun.md)
- [实验](./kernel/tty/lab.md)
- [Linux Device Driver : TTY Drivers](./kernel/tty/ldd-chapter-18.md)
- [2. 架构原理 - TTY 核心设计](./kernel/tty/linux-tty-analysis.md)
- [getty](./kernel/tty/login-getty.md)
- [netconsole](./kernel/tty/netconsole.md)
- [tty](./kernel/tty/overview.md)
- [putty](./kernel/tty/putty.md)
- [qemu](./kernel/tty/qemu.md)
- [uptime 展示的 user 数量](./kernel/tty/session.md)
- [ssh 的操作会过 tty 机制吗?](./kernel/tty/ssh.md)
- [sysrq](./kernel/tty/sysrq.md)
- [termios](./kernel/tty/termios.md)
- [如何停止另外的 tmux](./kernel/tty/tmux.md)
- [Linux TTY 子系统架构概览](./kernel/tty/tty-architecture-overview.md)
- [tty 到底是什么](./kernel/tty/tty.md)
- [为什么在 systemd 中，需要将日志设置为这个东西](./kernel/tty/win.md)
- [tty0](./kernel/tty/yes.md)

## kvm

- [记录调试 kvm 的一个有趣问题](./kvm/fun/host-freq.md)
- [QEMU 中 null_blk fio 性能与 HugeTLB 对比](./kvm/fun/qemu-nullblk-performance.md)

### aarch64
- [ARM KVM 简述](./kvm/aarch64/aarch64.md)
- [debugfs](./kvm/aarch64/2-tools.md)
- [大致分析下](./kvm/aarch64/code-overview.md)
- [基本流程](./kvm/aarch64/mmu.md)
- [aarch64](./kvm/aarch64/nested.md)
- [arm timer 模拟](./kvm/aarch64/timer.md)
- [aarch64 kvm_stat 观测](./kvm/aarch64/tracepoint.md)
- [VGIC : Interrupt Translation Service](./kvm/aarch64/vgic-its.md)
- [简单的代码分析](./kvm/aarch64/vgic.md)
- [vhe 和 non-vhe](./kvm/aarch64/vhe.md)
- [arm kvm 虚拟机的 exit reason](./kvm/aarch64/yes.md)

### kvm features
  - [kvm feautres](./kvm/features/kvm-features.md)
  - [pv eoi](./kvm/features/pv-eoi.md)
  - [pv sched yield](./kvm/features/pv-sched-yield.md)
  - [pv spinlock](./kvm/features/pv-spinlock.md)
  - [pv tlb flush](./kvm/features/pv-tlb-flush.md)
  - [steal time](./kvm/features/steal-time.md)

### mmu
  - [L1 中观测到 kvm_set_pfn_dirty](./kvm/mmu/ad.md)
  - [async pf](./kvm/mmu/async-pf.md)
  - [EXIT_REASON_EPT_VIOLATION vs EXIT_REASON_EPT_MISCONFIG](./kvm/mmu/basic.md)
  - [ept 格式的定义在哪里呢?](./kvm/mmu/ept.md)
  - [分析这个](./kvm/mmu/guest-memfd.md)
  - [为什么 kvm 需要特殊处理 hugepage](./kvm/mmu/hugepage.md)
  - [kvm mmu](./kvm/mmu/mmu.md)
  - [Documentation/virt/kvm/x86/mmu.rst](./kvm/mmu/mmu.rst.md)
  - [kvm_vcpu_arch 中的 5 个 MMU 的含义](./kvm/mmu/nested.md)
  - [kvm mmu notifier](./kvm/mmu/notifier.md)
  - [kvm track mode](./kvm/mmu/page-track.md)
  - [为什么需要 arch/x86/kvm/mmu/paging_tmpl.h 来处理各种情况](./kvm/mmu/paging_tmpl.md)
  - [PDPTR 是什么？](./kvm/mmu/pdptr.md)
  - [kvm rmap](./kvm/mmu/rmap.md)
  - [for_each_shadow_entry](./kvm/mmu/shadow-page.md)
  - [tdp_mmu](./kvm/mmu/tdp_mmu.md)
  - [KVM TLB Flush 机制分析](./kvm/mmu/tlb-flush-draft.md)
  - [tlb flush](./kvm/mmu/tlb-flush-virt.md)
  - [tlb flush 的基本原理](./kvm/mmu/tlb-flush.md)

### 嵌套虚拟化
  - [aarch64](./kvm/nested/aarch64.md)
  - [如何实现无穷级嵌套](./kvm/nested/nested-l3.md)
  - [kvm 嵌套虚拟化](./kvm/nested/nested.md)
  - [svm](./kvm/nested/svm.md)
  - [vmx](./kvm/nested/vmx.md)

### svm
  - [其中部分内容分析到](./kvm/svm/avic.md)
  - [sev](./kvm/svm/sev.md)
  - [简单浏览下 svm.c 的代码](./kvm/svm/svm.md)

- [kvm 如何处理 exception](./kvm/exception.md)

- [lab](./kvm/pit.md)
- [cache regs](./kvm/cache-regs.md)
- [cr0](./kvm/cr.md)
- [debugfs](./kvm/debugfs.md)
- [x86 emulate](./kvm/emulate/emulate.md)
- [SGX](./kvm/enclave.md)
- [event injection](./kvm/event-delivery.md)
- [exit reason](./kvm/exit-reason.md)
- [FRED](./kvm/fred.md)
- [interrupt window](./kvm/interrupt-window.md)
- [KVM](./kvm/kvm.md)
- [kvm_device_ops](./kvm/kvm_device_ops.md)
- [kvm lock 机制](./kvm/lock.md)
- [使用 tracepoint 来跟踪 kvm_check_request](./kvm/make_request.md)
- [mmio](./kvm/mmio.md)
- [msr](./kvm/msr.md)
- [mtrr](./kvm/mtrr.md)
- [ple window](./kvm/ple.md)
- [qemu 如何支持 kvm 的](./kvm/qemu.md)
- [secure](./kvm/secure.md)
- [kvm selftests](./kvm/selftests.md)
- [smm](./kvm/smm.md)
- [tracepoint](./kvm/tracepoint.md)
- [Intel VMCS 字段表](./kvm/vmcs-fields.md)
- [kvm](./kvm/yes-we-know.md)


## 整理中


### [ ] nvme

- [nvme 模块简述](./kernel/blk/nvme/nvme.md)
- [`nvme id-ns` 输出梳理](./kernel/blk/nvme/nvme-cli.md)
- [《深入浅出 SSD》阅读笔记](./kernel/blk/nvme/nvme-hardware.md)
- [nvmf](./kernel/blk/nvme/nvme-tcp.md)

### [ ] mq

- [bio-based 和 request-based 设备的区别](./kernel/blk/mq/bio-based-device.md)
- [storage blk plug 机制](./kernel/blk/mq/plug.md)
- [bio request request_queue 三者的关系](./kernel/blk/mq/bio-request.md)
- [drivers/md/dm-rq.c](./kernel/blk/mq/dm-rq.md)
- [Multi-Queue Block IO Queueing Mechanism (blk-mq)](./kernel/blk/mq/doc.md)
- [Block Layer IO 请求路径详解 - Bypass 机制全梳理](./kernel/blk/mq/ai-io_paths_analysis.md)
- [Linux Device Driver : Block Drivers](./kernel/blk/mq/ldd-chapter-16.md)
- [blk-mq-debugfs.c](./kernel/blk/mq/mq-debugfs.md)
- [mq 核心结构体](./kernel/blk/mq/mq.md)
- [sbitmap](./kernel/blk/mq/sbitmap.md)
- [deadline scheduler](./kernel/blk/mq/scheduler-deadline.md)
- [scheduler](./kernel/blk/mq/scheduler.md)
- [shared tags](./kernel/blk/mq/shared-tags.md)
- [tag](./kernel/blk/mq/tag.md)
- [选项 BLK_WBT](./kernel/blk/mq/wbt.md)
- [mq 基础](./kernel/blk/mq/yes.md)

### [ ] cgroup

### [ ] vmscan lru

### [ ] initramfs

- [buildroot](./kernel/tutorial/initramfs/builtroot.md)
- [dracut](./kernel/tutorial/initramfs/dracut.md)
- [initfs](./kernel/tutorial/initramfs/initramfs.md)
- [initramfs : iso](./kernel/tutorial/initramfs/iso.md)
- [linuxfromscratch](./kernel/tutorial/initramfs/minimal.md)
- [bootc](./kernel/tutorial/initramfs/yes.md)

### [ ] Rust

- [demo 合集](./rust/demo/README.md)

- [rust unsafe](./rust/unsafe.md)
- [Learning Rust With Entirely Too Many Linked Lists](./rust/linked-list.md)

- [Resource](./rust/links.md)
- [工具](./rust/tools.md)
- [tokio](./rust/tokio.md)

- [Linux 和 QEMU 中对于 Rust 的支持](./rust/kernel.md)
- [Rust 实现 Hobby OS](./rust/os.md)
- [QEMU rust 支持现状](./rust/qemu.md)

- [macro](./rust/macro.md)
- [Ownership and lifetime](./rust/ownership-lifetime.md)

### [ ] 并发编程

- [工具](./concurrent/2-tools.md)
- [并发编程中违反直觉的例子](./concurrent/counter-intuitive.md)
- [并发数据结构](./concurrent/data-structure.md)
- [并发编程基本生存法则](./concurrent/engineerings-perspective.md)
- [Lockless](./concurrent/lockless.md)
- [特殊并发问题分析](./concurrent/misc.md)
- [事务内存初识](./concurrent/transctiona-memory.md)
- [为什么并行编程如此困难](./concurrent/why-parallel-is-hard.md)
- [kernel/sched/membarrier.c syscall](./concurrent/yes.md)

<!-- BEGIN AUTO DOCS INDEX -->
## 自动文档索引

以下只包含当前 README 手工区还没有引用的发布文档。

- `aarch64/`
  - `sdm/`
    - [aarch64 sdm](./aarch64/sdm/README.md)
  - [aarch64 和 x86 的简单对比](./aarch64/arch-diff.md)
  - [aarch64 基础](./aarch64/basic.md)
  - [原来 qemu 可以指定 gic 版本](./aarch64/gic.md)
  - [工具](./aarch64/overview.md)
- `acpi/`
  - [ACPI 概述](./acpi/acpi.md)
  - [acpi_power_meter](./acpi/acpi_meter.md)
  - [acpica](./acpi/acpica.md)
  - [使用 acpidump 来观察](./acpi/lab.md)
  - [pnp](./acpi/pnp.md)
  - [poweroff 内核的触发过程](./acpi/poweroff.md)
  - [hack with qemu](./acpi/qemu.md)
  - [UACPI](./acpi/uacpi.md)
- `ai/`
  - [AI 时代的英语解决方案](./ai/english.md)
  - [pi](./ai/pi.md)
- `algorithm/`
  - [Summary](./algorithm/advance.md)
  - [快排](./algorithm/basic.md)
  - [sort](./algorithm/c_algorithm.md)
  - [dp](./algorithm/dp.md)
  - [TODO](./algorithm/graph.md)
  - [Huffman Code](./algorithm/greedy.md)
  - [吃葡萄](./algorithm/misc.md)
  - [String](./algorithm/oj_with_go.md)
  - [emplace](./algorithm/stl.md)
  - [String alrotithm described by cpp](./algorithm/stl2.md)
  - [禁用](./algorithm/superstition_of_oj.md)
  - [不使用 stack](./algorithm/tree.md)
- `asm/`
  - `aarch64/`
    - [https://mariokartwii.com/armv8/](./asm/aarch64/README.md)
  - `x86_64/`
    - [checksheet](./asm/x86_64/README.md)
  - [Nasm](./asm/README.md)
- `benchmark/`
  - `gpu-microbench/`
    - [GPU microbench](./benchmark/gpu-microbench/README.md)
  - `microbench/`
    - [本机 C++ microbench](./benchmark/microbench/README.md)
  - [性能基准测试工具](./benchmark/benchmarks.md)
  - [内存带宽](./benchmark/memory-bandwithd.md)
  - [microbench 简单的测试](./benchmark/my-result.md)
  - [峰值算力怎么算，FLOP 怎么数](./benchmark/peak-flops.md)
- `blog/`
  - [Next AI Draw.io 本地部署记录](./blog/next-ai-draw-io.md)
- `bmbt/`
  - `papers/`
    - [Efficient Memory Virtualization for Cross-ISA System Mode Emulation](./bmbt/papers/espt.md)
    - [问题](./bmbt/papers/hstp.md)
  - [BMBT 常见问题解答](./bmbt/1-why.md)
  - [裸金属二进制翻译器的技术细节](./bmbt/3-tech.md)
  - [淦，设计一个裸金属二进制翻译器不可能这么难](./bmbt/4-emotion.md)
- `chatter/`
  - [2026-09-24](./chatter/2026-9-24-ai-survey.md)
  - [不要辜负这个伟大的时代](./chatter/great-era.md)
  - [为什么你不应该考公务员](./chatter/gwy.md)
  - [Kimi k3 并不好用](./chatter/k3.md)
  - [乱七八糟的想法](./chatter/life-dev.md)
  - [恋爱](./chatter/love.md)
  - [关于读博的思考](./chatter/phd.md)
  - [blog 阅读](./chatter/readings.md)
  - [深圳](./chatter/shenzhen.md)
  - [社交媒体](./chatter/social-media.md)
  - [看似有关系，实际上没关系](./chatter/unrelated-but-similar.md)
  - [远程工作体验](./chatter/wfh.md)
- `chores/`
  - [汽车](./chores/car.md)
  - [打扮](./chores/dress-up.md)
  - [反向开票](./chores/financial.md)
  - [电吉他](./chores/guitar.md)
  - [手工](./chores/handicrafts.md)
  - [医疗](./chores/medical.md)
  - [音乐](./chores/music.md)
  - [基本](./chores/photo.md)
  - [山水小品](./chores/shan-shui-xiao-pin.md)
  - [运动](./chores/sports.md)
- `collei/`
  - [kgdb 为什么最好是用 ttyS0 来](./collei/kgdb-2.md)
  - [kgdb 支持](./collei/kgdb.md)
- `concurrent/`
  - `atomic/`
    - [x86](./concurrent/atomic/x86.md)
  - `kernel/`
    - [内核文档](./concurrent/kernel/kernel-doc.md)
    - [kernel 各个模块的锁的设计](./concurrent/kernel/kernel-lock-design.md)
  - `memory-model/`
    - `litmus/`
      - [memory model : litmus 测试工具工作原理](./concurrent/memory-model/litmus/README.md)
    - `lkmm/`
      - `zh-cn/`
        - [access-marking.txt 中文译解](./concurrent/memory-model/lkmm/zh-cn/access-marking.translation.md)
        - [control-dependencies.txt 内容总结](./concurrent/memory-model/lkmm/zh-cn/control-dependencies.summary.md)
        - [control-dependencies.txt 中文译解](./concurrent/memory-model/lkmm/zh-cn/control-dependencies.translation.md)
        - [explanation.txt 中文译解](./concurrent/memory-model/lkmm/zh-cn/explanation.translation.md)
        - [locking.txt 中文译解](./concurrent/memory-model/lkmm/zh-cn/locking.translation.md)
      - [3. 内核源码中的 LKMM](./concurrent/memory-model/lkmm/ai-overview.md)
      - [Linux Kernel Memory Model (LKMM) 完全指南](./concurrent/memory-model/lkmm/ai-read.md)
      - [rcu 的邮件，每一个都需要阅读下](./concurrent/memory-model/lkmm/human.md)
      - [4.2 CDSChecker](./concurrent/memory-model/lkmm/other-tools.md)
      - [8. LKMM vs C/C++ Memory Model](./concurrent/memory-model/lkmm/vs-cpp.md)
  - `san/`
    - [nvidia compute sanitizer](./concurrent/san/nv-cs.md)
  - [并行编程实践记录](./concurrent/usage.md)
- `container/`
  - [Podman Rootless 问题记录](./container/container.md)
  - [Docker 基本使用](./container/docker.md)
  - [有一个感觉 nsenter 之类的工具类组成 docker 的](./container/minitools.md)
  - [Podman 使用指南](./container/podman.md)
  - [docker 代理的方法](./container/proxy.md)
- `cpp/`
  - `coroutine/`
    - [coroutines](./cpp/coroutine/coroutine.md)
  - `cpp-primer/`
    - [动态内存](./cpp/cpp-primer/12.md)
    - [19](./cpp/cpp-primer/19.md)
    - [类](./cpp/cpp-primer/7.md)
  - `effctive-cpp/`
    - [C++ Core Guidelines 完整规则标题索引](./cpp/effctive-cpp/core-guidelines-index.md)
    - [C++ Core Guidelines 中文整理](./cpp/effctive-cpp/core-guidelines.md)
    - [《Effective Modern C++》规则速览](./cpp/effctive-cpp/effective-modern-cpp.md)
    - [《Effective C++》规则速览](./cpp/effctive-cpp/effective.md)
    - [cpp rules](./cpp/effctive-cpp/rules.md)
  - `exception/`
    - [cpp exception 机制](./cpp/exception/except.md)
  - `init/`
    - [C++20 为什么允许用圆括号初始化聚合体](./cpp/init/aggregate.md)
    - [emplace_back](./cpp/init/emplace-back.md)
    - [C++ 初始化梳理](./cpp/init/init.md)
  - `lambda/`
    - [lambda](./cpp/lambda/lambda.md)
  - `misc/`
    - [杂记](./cpp/misc/misc.md)
  - `module/`
    - [C++20 Modules](./cpp/module/module.md)
  - `move/`
    - [cpp 三/五法则](./cpp/move/3-5-rules.md)
    - [万能引用（转发引用）](./cpp/move/forwarding-reference.md)
    - [7. 一个反直觉规则：有名字的右值引用是左值](./cpp/move/move-2.md)
    - [stackoverflow top question](./cpp/move/move.md)
    - [rvo](./cpp/move/rvo.md)
    - [1. 直观理解（C 时代的粗糙定义）](./cpp/move/value.md)
  - `oop/`
    - [方便的工具](./cpp/oop/README.md)
  - `operator/`
    - [cpp operator](./cpp/operator/README.md)
  - `template/`
    - [cpp template SFINAE](./cpp/template/SFINAE.md)
    - [cpp template : using 与 typedef](./cpp/template/alias.md)
    - [cpp template : concept](./cpp/template/concepts.md)
    - [cpp template : fold](./cpp/template/fold-expression.md)
    - [CppTemplateTutorial' Notes](./cpp/template/template.md)
    - [cpp template :  typename class](./cpp/template/typename-class.md)
  - `trait/`
    - [trait](./cpp/trait/trait.md)
  - `types-cast/`
    - [RTTI](./cpp/types-cast/cast.md)
    - [C++ 类型转换总结](./cpp/types-cast/casts.md)
  - `vtable/`
    - [C++ vtable 与 RTTI 布局](./cpp/vtable/vtable.md)
  - [用 Clang 和 GCC 观察 C++ 的隐式行为与实现](./cpp/compiler-inspection.md)
  - [cpp RAII](./cpp/raii.md)
  - [C++ 资源](./cpp/resource.md)
  - [version](./cpp/version.md)
- `cpu/`
  - `boom/`
    - [BOOM 微架构学习(1)——取指单元与分支预测](./cpu/boom/doc.md)
    - [源码阅读](./cpu/boom/source-code.md)
    - [BOOM 源码阅读](./cpu/boom/why-boom.md)
  - `rocket-chip/`
    - [总体来说，rocket-chip 中的人是无法实现](./cpu/rocket-chip/rocket-chip.md)
  - `sys/`
    - `MCCC/`
      - [Memory Consistency Motivation and Sequential Consistency](./cpu/sys/MCCC/README.md)
    - `PCA/`
      - [1.1 Introduction](./cpu/sys/PCA/01-introduction.md)
      - [02-parallel program](./cpu/sys/PCA/02-parallel program.md)
      - [5.2 Cache Coherence](./cpu/sys/PCA/05_shared_memory_multiprocessor.md)
      - [06_Snoop-based_Multiprocessor_Design](./cpu/sys/PCA/06_Snoop-based_Multiprocessor_Design.md)
      - [11](./cpu/sys/PCA/11.md)
    - `Quantitative/`
      - `synthesis/`
        - [Instruction-Level Parallelism and it's Exploitation](./cpu/sys/Quantitative/synthesis/3.md)
        - [Pipelinng: Basic and Intermediate Conceptes](./cpu/sys/Quantitative/synthesis/C.md)
        - [Storage System](./cpu/sys/Quantitative/synthesis/D.md)
        - [Embedded Systems](./cpu/sys/Quantitative/synthesis/E.md)
        - [Interconnection Networks](./cpu/sys/Quantitative/synthesis/F.md)
      - [Instruction-Level Parallelism and it's Exploitation](./cpu/sys/Quantitative/3.md)
      - [Data-Level Parallelism in Vector, SIMD, and GPU Architectures](./cpu/sys/Quantitative/4.md)
      - [Models of Memory Consistency: An Introduction](./cpu/sys/Quantitative/5.md)
      - [Warehouse-Scale Computers to Exploit Request-Level and Data-Level Parallelis](./cpu/sys/Quantitative/6.md)
      - [Instruction Set Principles](./cpu/sys/Quantitative/A.md)
      - [Review of Memory Hierarchy](./cpu/sys/Quantitative/B.md)
      - [Pipelining: Basic and Intermediate Concepts](./cpu/sys/Quantitative/C.md)
      - [Storage System](./cpu/sys/Quantitative/D.md)
      - [Embedded Systems](./cpu/sys/Quantitative/E.md)
      - [Interconnection Networks](./cpu/sys/Quantitative/F.md)
      - [Survey of Instruction Set Architectures](./cpu/sys/Quantitative/K.md)
      - [Historical Perspective and Reference](./cpu/sys/Quantitative/M.md)
      - [Chapter 2](./cpu/sys/Quantitative/Quantitative.md)
      - [plans](./cpu/sys/Quantitative/plans.md)
    - `SLCA/`
      - [入门](./cpu/sys/SLCA/1.md)
      - [2 Cache](./cpu/sys/SLCA/2.md)
      - [指令读取](./cpu/sys/SLCA/3.md)
      - [解码](./cpu/sys/SLCA/4.md)
      - [Allocation](./cpu/sys/SLCA/5.md)
      - [第六章　发射](./cpu/sys/SLCA/6.md)
      - [第7章 执行](./cpu/sys/SLCA/7.md)
      - [第8章　提交](./cpu/sys/SLCA/8.md)
      - [readme](./cpu/sys/SLCA/readme.md)
    - `cs252/`
      - [1 intruction](./cpu/sys/cs252/lecture.md)
    - `fpga/`
      - [hdlbits](./cpu/sys/fpga/setup.md)
    - [dram](./cpu/sys/dram.md)
    - [gem5](./cpu/sys/gem5.md)
    - [问题](./cpu/sys/question.md)
    - [计划就是，首先理解软件层的指令集的含义，然后再去处理怎么写](./cpu/sys/sys-route.md)
    - [verilator](./cpu/sys/verilator.md)
    - [超标量处理器设计 : 姚永斌](./cpu/sys/yao.md)
  - [chipyard 环境搭建](./cpu/chipyard.md)
  - [Chisel 学习资源汇总](./cpu/chisel.md)
  - [hotchip](./cpu/hotchip.md)
  - [MIPS R10000 的设计](./cpu/mipsR10000.md)
  - [芯片设计相关资料汇总](./cpu/overview.md)
  - [Reorder Buffer (ROB)](./cpu/rob.md)
  - [Scala 基础语法学习](./cpu/scala.md)
  - [芯片设计中两个关键设备](./cpu/smart-dev.md)
  - [CPU ARCH : 进入到 store buffer 意味着已经 commit 了吗?](./cpu/store-buffer.md)
  - [访存子系统](./cpu/xiangshan.md)
- `cxl/`
  - [CXL 技术资料](./cxl/readme.md)
- `games/`
  - [在 Linux kernel 上如何玩游戏](./games/kernel.md)
  - [SteamOS](./games/steam.md)
- `grub/`
  - [GRUB 配置详解](./grub/basic.md)
  - [systemd-boot](./grub/new.md)
- `kernel/`
  - `api/`
    - [经典参考资料](./kernel/api/README.md)
  - `binder/`
    - [binder](./kernel/binder/README.md)
  - `blk/`
    - `blktrace/`
      - [关键原料](./kernel/blk/blktrace/internal.md)
      - [blktrace 基本使用](./kernel/blk/blktrace/usage.md)
    - `dm/`
      - [device mapper](./kernel/blk/dm/device-mapper.md)
    - `ds/`
      - [其他的各种收集](./kernel/blk/ds/README.md)
    - `fio/`
      - [HDD 已死](./kernel/blk/fio/fio-result.md)
    - `mq/`
      - [bio-based、request-based 与 `request_queue` 的关系](./kernel/blk/mq/ai-bio-based-vs-request-based.md)
    - `raid/`
      - [资料](./kernel/blk/raid/general.md)
      - [同步模型](./kernel/blk/raid/lock.md)
      - [raid1 的开机过程](./kernel/blk/raid/raid1-boot.md)
      - [raid1](./kernel/blk/raid/raid1.md)
      - [sync](./kernel/blk/raid/sync.md)
      - [syfs md](./kernel/blk/raid/sysfs.md)
    - `scsi/`
      - [fc](./kernel/blk/scsi/fc.md)
      - [megaraid](./kernel/blk/scsi/megaraid.md)
      - [mpt3sas](./kernel/blk/scsi/mpt3sas.md)
      - [以 scsi 为例子分析 blk 层超时机制](./kernel/blk/scsi/scsi-error-timeout.md)
      - [iscsi](./kernel/blk/scsi/scsi-iscsi.md)
      - [实现原理](./kernel/blk/scsi/scsi-log.md)
      - [qemu 是如何支持 scsi 的](./kernel/blk/scsi/scsi-qemu.md)
      - [scsi](./kernel/blk/scsi/scsi.md)
      - [scsi_debug](./kernel/blk/scsi/scsi_debug.md)
    - [Linux block layer 用户态工具](./kernel/blk/1-usage.md)
    - [iostat 的每一项的含义是什么](./kernel/blk/2-iostat.md)
    - [hdpara](./kernel/blk/3-hdpram.md)
    - [fio 使用](./kernel/blk/4-fio.md)
    - [iowait](./kernel/blk/5-iowait.md)
    - [smartctl](./kernel/blk/7-smartctl.md)
    - [bcache](./kernel/blk/bcache.md)
    - [Blockdev 源码分析](./kernel/blk/blockdev.md)
    - [discard / TRIM](./kernel/blk/discard.md)
    - [block layer integrity](./kernel/blk/integrity.md)
    - [分析下 disk events](./kernel/blk/later.md)
    - [blk layer lock](./kernel/blk/lock.md)
    - [nbd](./kernel/blk/nbd.md)
    - [null blk](./kernel/blk/null-blk.md)
    - [sata](./kernel/blk/sata.md)
    - [Zoned block devices](./kernel/blk/zone-device.md)
  - `cgroup/`
    - `sched.code/`
      - [cgroup sched 在多核上的分布](./kernel/cgroup/sched.code/readme.md)
    - [cgroup](./kernel/cgroup/cgroup-blk.md)
    - [cpuset](./kernel/cgroup/cgroup-cpuset.md)
    - [cgroup ebpf](./kernel/cgroup/cgroup-ebpf.md)
    - [cgroup 释放问题](./kernel/cgroup/cgroup-free.md)
    - [分析下 hugetlb cgroup 的实现](./kernel/cgroup/cgroup-hugetlb.md)
    - [memcontrol.c](./kernel/cgroup/cgroup-mm.md)
    - [net_cls](./kernel/cgroup/cgroup-net.md)
    - [cgroup sched](./kernel/cgroup/cgroup-sched.md)
    - [cgroup](./kernel/cgroup/cgroup.md)
    - [cpu-controller-files](./kernel/cgroup/cpu-controller-files.md)
    - [cgroup 的基础设施](./kernel/cgroup/tmp.md)
    - [cgroup 的操作手册](./kernel/cgroup/usage.md)
    - [cgroup V1](./kernel/cgroup/v1.md)
  - `cpuinfo/`
    - `feature/`
      - [aperfmperf](./kernel/cpuinfo/feature/aperfmperf.md)
      - [nofsgsbase](./kernel/cpuinfo/feature/fsgsbase.md)
      - [基本观察](./kernel/cpuinfo/feature/hfi.md)
      - [cpuid leaf : CPUID_LEAF_MWAIT](./kernel/cpuinfo/feature/mwait.md)
      - [rtm](./kernel/cpuinfo/feature/rtm.md)
    - [cpuid 的 leaf 和 subleaf 是做什么的](./kernel/cpuinfo/00-tools.md)
    - [如何快速知道 CPUID 的含义](./kernel/cpuinfo/01-extract-cpuid-result.md)
    - [分析下 QEMU cpu model 的](./kernel/cpuinfo/02-qemu-kvm.md)
    - [分析 Linux 内核如何处理](./kernel/cpuinfo/03-kernel.md)
    - [各种 CPU features 总结](./kernel/cpuinfo/04-features.md)
    - [分析 arm 的 cpuid 的代码](./kernel/cpuinfo/06-aarch64.md)
    - [migration 的 cpu flags](./kernel/cpuinfo/08-migraion.md)
    - [msr](./kernel/cpuinfo/09-msr.md)
    - [cpuinfo](./kernel/cpuinfo/10-proc-cpuinfo.md)
    - [libvirt 处理 cpuinfo](./kernel/cpuinfo/11-libvirt.md)
    - [vmware](./kernel/cpuinfo/12-vendor-specific.md)
    - [趣事](./kernel/cpuinfo/fun.md)
  - `dts/`
    - [设备树](./kernel/dts/device-tree.md)
  - `fpu/`
    - [测试](./kernel/fpu/lab.md)
    - [Enable dune (mips) with fpu](./kernel/fpu/mips-fpu.md)
    - [fpu 引入的 context switch](./kernel/fpu/overview.md)
    - [pkru](./kernel/fpu/pkru.md)
    - [qemu 处理 fpu](./kernel/fpu/qemu.md)
  - `fs/`
    - `code/`
      - [unlink() 和 remove()](./kernel/fs/code/unlink-remove.md)
    - `nfs/`
      - `sunrpc/`
        - [README](./kernel/fs/nfs/sunrpc/README.md)
    - `virtual/`
      - [configfs](./kernel/fs/virtual/configfs.md)
      - [kernfs](./kernel/fs/virtual/kernfs.md)
      - [总结一下各种虚拟文件系统](./kernel/fs/virtual/virtual.md)
    - [acl](./kernel/fs/acl.md)
    - [autofs 的作用](./kernel/fs/autofs.md)
    - [bcachefs](./kernel/fs/bcachefs.md)
    - [binfmt_elf.c](./kernel/fs/binfmt_elf.md)
    - [btrfs](./kernel/fs/btrfs.md)
    - [buffer.c](./kernel/fs/buffer-head.md)
    - [dax](./kernel/fs/dax.md)
    - [VFS 文档](./kernel/fs/doc.md)
    - [cp vs mv 替换运行中二进制](./kernel/fs/etxtbsy.md)
    - [fsfreeze](./kernel/fs/freeze.md)
    - [fsnotify](./kernel/fs/fsnotify.md)
    - [inode](./kernel/fs/inode.md)
    - [fs/iomap](./kernel/fs/iomap.md)
    - [iops](./kernel/fs/iops.md)
    - [fs 的 lock 设计](./kernel/fs/lock.md)
    - [mount](./kernel/fs/mount.md)
    - [关键源码位置](./kernel/fs/namei.md)
    - [基本 io 流程](./kernel/fs/overlay.md)
    - [relay fs](./kernel/fs/relay.md)
    - [为什么设计一个文件系统是很难的](./kernel/fs/why-so-complex.md)
    - [writeback](./kernel/fs/writeback.md)
    - [attr && xattr](./kernel/fs/xattr.md)
    - [xfs](./kernel/fs/xfs.md)
    - [fs 基础](./kernel/fs/yes.md)
    - [zfs](./kernel/fs/zfs.md)
    - [zonefs](./kernel/fs/zonefs.md)
  - `gcc/`
    - `inline-asm-tutorial/`
      - [GCC 内联汇编教程](./kernel/gcc/inline-asm-tutorial/README.md)
    - [gcc 内联汇编](./kernel/gcc/gcc-inline-asm.md)
  - `hotos/`
    - [hotos](./kernel/hotos/README.md)
  - `iommu/`
    - [iommu=pt 到底意味着什么?](./kernel/iommu/iommu-pt.md)
  - `iouring/`
    - `async/`
      - `epoll/`
        - [EPOLLOUT 什么时候是"必须"监听的?](./kernel/iouring/async/epoll/epoll-out.md)
        - [epoll 断联之后，可以继续重连](./kernel/iouring/async/epoll/epoll-reconnect.md)
        - [epoll](./kernel/iouring/async/epoll/epoll.md)
  - `ipmi/`
    - [pikvm](./kernel/ipmi/pikvm.md)
  - `irq/`
    - `kvm/`
      - [intel 中断虚拟化，基于 狮子书](./kernel/irq/kvm/overview.md)
    - `legacy/`
      - [ioapic](./kernel/irq/legacy/ioapic.md)
      - [kvmvapic](./kernel/irq/legacy/kvmvapic.md)
      - [noapic](./kernel/irq/legacy/pic.md)
    - `qemu/`
      - [内核中模拟 intc](./kernel/irq/qemu/intc-kvm.md)
      - [中断是如何产生的](./kernel/irq/qemu/intc.md)
      - [tcg pic](./kernel/irq/qemu/tcg.md)
    - `softirq/`
      - [中断分析的工具](./kernel/irq/softirq/1-tools.md)
      - [irq work](./kernel/irq/softirq/irqwork.md)
      - [softirq](./kernel/irq/softirq/softirq.md)
      - [tasklet](./kernel/irq/softirq/tasklet.md)
      - [workqueue](./kernel/irq/softirq/workqueue.md)
    - [中断相关实验](./kernel/irq/1-tools.md)
    - [先从 debugfs 仔细看看内核的东西](./kernel/irq/2-debugfs.md)
    - [对比整理一下 arm 和 x86_64 的 config 的差别是什么](./kernel/irq/aarch64.md)
    - [APIC 学习资料整理](./kernel/irq/apic.md)
    - [x86 中断资料](./kernel/irq/doc.md)
    - [irq domain 的结构](./kernel/irq/domain.md)
    - [idt](./kernel/irq/idt.md)
    - [文档](./kernel/irq/int-x2apic.md)
    - [VECTOR 总是从 32 开始的](./kernel/irq/int-yes.md)
    - [似乎，中断经过 IOMMU 似乎是不受内核参数 iommu=off 控制的](./kernel/irq/iommu.md)
    - [ipi](./kernel/irq/ipi.md)
    - [trigger](./kernel/irq/trigger.md)
    - [x86 的 arch/x86/kernel/apic/vector.c](./kernel/irq/vector.md)
  - `lpc/`
    - [ftrace with args](./kernel/lpc/2021.md)
    - [LoongArch: What we will do next](./kernel/lpc/2022.md)
    - [Speeding up Kernel Testing and Debugging with virtme-ng](./kernel/lpc/2023.md)
    - [2024](./kernel/lpc/2024.md)
    - [2025](./kernel/lpc/2025.md)
    - [LPC](./kernel/lpc/Readme.md)
  - `lsfmmbpf/`
    - [2023](./kernel/lsfmmbpf/2023.md)
    - [2024](./kernel/lsfmmbpf/2024.md)
    - [lsfmmbpf 2026](./kernel/lsfmmbpf/2026.md)
  - `mm/`
    - `code/`
      - [测试内容记录](./kernel/mm/code/README.md)
    - `damon/`
      - [DAMO](./kernel/mm/damon/damo.md)
      - [DAMON](./kernel/mm/damon/damon.md)
      - [DAMON Lab 3](./kernel/mm/damon/lab3.md)
      - [yyds-fs DAMON 局部性实验](./kernel/mm/damon/locality-yyds-fs.md)
    - `pgtable/`
      - [msharefs](./kernel/mm/pgtable/msharefs.md)
    - `slub/`
      - [slub 调试记录](./kernel/mm/slub/debug.md)
      - [slub](./kernel/mm/slub/overview.md)
      - [Slub TID](./kernel/mm/slub/tid.md)
      - [Slub Tools](./kernel/mm/slub/tools.md)
    - `userfaultfd/`
      - [GDB 读取未填充的 userfaultfd missing 页为何失败](./kernel/mm/userfaultfd/gdb-missing.md)
    - [Idle Page Tracking](./kernel/mm/idle-page-tracking.md)
    - [kaslr](./kernel/mm/kaslr.md)
    - [Backing Device](./kernel/mm/mm-backing-dev.md)
    - [Buddy System](./kernel/mm/mm-buddy.md)
    - [CMA](./kernel/mm/mm-cma.md)
    - [Memory Compaction](./kernel/mm/mm-compaction.md)
    - [Copy-On-Write](./kernel/mm/mm-cow.md)
    - [Memory Failure](./kernel/mm/mm-failure.md)
    - [Page Cache](./kernel/mm/mm-filemap.md)
    - [Folio](./kernel/mm/mm-folio.md)
    - [GFP Flags](./kernel/mm/mm-gfp.md)
    - [Get User Pages](./kernel/mm/mm-gup.md)
    - [High Memory](./kernel/mm/mm-highmem.md)
    - [HMM](./kernel/mm/mm-hmm.md)
    - [HugeTLB Bug List](./kernel/mm/mm-hugetlb.bug.md)
    - [HugeTLB Filesystem](./kernel/mm/mm-hugetlb.fs.md)
    - [HugeTLB](./kernel/mm/mm-hugetlb.md)
    - [HugeTLB Surplus Bug](./kernel/mm/mm-hugetlb.patch.md)
    - [ioremap 和 resource 机制](./kernel/mm/mm-ioremap.md)
    - [Memory Isolation](./kernel/mm/mm-isolation.md)
    - [khugepaged](./kernel/mm/mm-khugepaged.md)
    - [KSM](./kernel/mm/mm-ksm.md)
    - [Memory Management Locking](./kernel/mm/mm-lock.md)
    - [malloc](./kernel/mm/mm-malloc.md)
    - [解释 folio->mapping 的含义](./kernel/mm/mm-mapping.md)
    - [Memblock](./kernel/mm/mm-memblock.md)
    - [memfd](./kernel/mm/mm-memfd.md)
    - [Memory Policy](./kernel/mm/mm-mempolicy.md)
    - [Memory Pool](./kernel/mm/mm-mempool.md)
    - [Page Migration](./kernel/mm/mm-migration.md)
    - [copy_from_user](./kernel/mm/mm-misc.md)
    - [mlock](./kernel/mm/mm-mlock.md)
    - [`vm_area_struct::vm_operations_struct` 结构体的作用](./kernel/mm/mm-mmap.md)
    - [MMU Notifier](./kernel/mm/mm-mmu-notifier.md)
    - [OOM](./kernel/mm/mm-oom.md)
    - [page::private](./kernel/mm/mm-page-private.md)
    - [Page Reporting](./kernel/mm/mm-page-reporting.md)
    - [Page Writeback](./kernel/mm/mm-page-writeback.md)
    - [Page Fault](./kernel/mm/mm-pagefault.md)
    - [Page Flags](./kernel/mm/mm-pageflags.md)
    - [Page Owner](./kernel/mm/mm-pageowner.md)
    - [Page Poisoning](./kernel/mm/mm-poison.md)
    - [Readahead](./kernel/mm/mm-readahead.md)
    - [Page Refcount](./kernel/mm/mm-refcount.md)
    - [Reverse Mapping](./kernel/mm/mm-rmap.md)
    - [shmem](./kernel/mm/mm-shmem.md)
    - [kernel stack](./kernel/mm/mm-stack.md)
    - [Transparent Huge Pages](./kernel/mm/mm-thp.md)
    - [Memory Tiering](./kernel/mm/mm-tier.md)
    - [Thread-Local Storage](./kernel/mm/mm-tls.md)
    - [tmpfs](./kernel/mm/mm-tmpfs.md)
    - [Memory Tracepoints](./kernel/mm/mm-tracepoint.md)
    - [Virtio-mem (QEMU)](./kernel/mm/mm-virtio-mem-qemu.md)
    - [Virtio-mem](./kernel/mm/mm-virtio-mem.md)
    - [Virtio-pmem](./kernel/mm/mm-virtio-pmem.md)
    - [vmalloc](./kernel/mm/mm-vmalloc.md)
    - [Sparse Vmemmap](./kernel/mm/mm-vmemmap.md)
    - [vmflags](./kernel/mm/mm-vmflags.md)
    - [mm shrinker](./kernel/mm/mm-vmscan-shrinker.md)
    - [VM Pressure](./kernel/mm/mm-vmscan-vmpressure.md)
    - [Multi-Gen LRU](./kernel/mm/mm-vmscan.gen.md)
    - [Page Reclaim](./kernel/mm/mm-vmscan.md)
    - [mm vmscan](./kernel/mm/mm-vmscan.yes.md)
    - [Watermarks](./kernel/mm/mm-watermark.md)
    - [Workingset](./kernel/mm/mm-workingset.md)
    - [z3fold / zbud](./kernel/mm/mm-z3fold-zbud.md)
    - [Memory Zones](./kernel/mm/mm-zone.md)
    - [numa balancing 工作原理](./kernel/mm/numa-balancing.md)
    - [NUMA](./kernel/mm/numa.md)
    - [RSS](./kernel/mm/rss.md)
    - [内核虚拟机地址空间](./kernel/mm/va.md)
    - [Virtio Balloon Debug](./kernel/mm/virtio-balloon-debug.md)
    - [Virtio Balloon Kernel 实现](./kernel/mm/virtio-balloon-kernel.md)
    - [Virtio Balloon QEMU 实现](./kernel/mm/virtio-balloon-qemu.md)
  - `module/`
    - [内核模块](./kernel/module/README.md)
  - `ospm/`
    - [2025](./kernel/ospm/2025.md)
  - `release/`
    - [kernel release](./kernel/release/README.md)
  - `sched/`
    - `code/`
      - [EEVDF 用户态实验](./kernel/sched/code/eevdf-demo.md)
      - [sched PELT](./kernel/sched/code/pelt-demo.md)
      - [混合 RT / 普通线程测试](./kernel/sched/code/rt-mixed-demo.md)
    - [https://lwn.net/Articles/639543/](./kernel/sched/autogroup.md)
    - [balance](./kernel/sched/balance.md)
    - [主要参考资料](./kernel/sched/cfs.md)
    - [context switch](./kernel/sched/context-switch.md)
    - [sched debugfs](./kernel/sched/debug.md)
    - [scheduler 内核文档](./kernel/sched/doc.md)
    - [EEVDF 把 SCHED_BATCH 给干没了](./kernel/sched/eevdf.md)
    - [exec](./kernel/sched/exec.md)
    - [https://github.com/sched-ext/scx](./kernel/sched/ext.md)
    - [fork](./kernel/sched/fork-exit.md)
    - [CPU freq](./kernel/sched/freq.md)
    - [CONFIG_CFS_BANDWIDTH](./kernel/sched/group-wip.md)
    - [hung task 机制](./kernel/sched/hung-task.md)
    - [idle 子系统](./kernel/sched/idle.md)
    - [IRQ_TIME_ACCOUNTING](./kernel/sched/irq-time-accounting.md)
    - [cpu isolation](./kernel/sched/isolation.md)
    - [load avg](./kernel/sched/load.md)
    - [martins3 scheduler](./kernel/sched/martins3-scheduler.md)
    - [pelt](./kernel/sched/pelt.md)
    - [pid](./kernel/sched/pid-basic.md)
    - [pid 高级话题](./kernel/sched/pid.md)
    - [pidfd](./kernel/sched/pidfd.md)
    - [preempt rt](./kernel/sched/preempt-rt.md)
    - [preempt](./kernel/sched/preempt.md)
    - [process 的状态 procstat](./kernel/sched/process-state.md)
    - [wait syscall](./kernel/sched/process-wait.md)
    - [psi](./kernel/sched/psi.md)
    - [ptrace](./kernel/sched/ptrace.md)
    - [rt.c 分析](./kernel/sched/rt.md)
    - [sched/stop_task.c](./kernel/sched/stop.md)
    - [TIF](./kernel/sched/tif.md)
    - [先把玩一下 sched 中各个 trace 点](./kernel/sched/tracepoint.md)
    - [ttwu](./kernel/sched/ttwu.md)
    - [uclamp](./kernel/sched/uclamp.md)
    - [CLONE_THREAD 和 CLONE_VM](./kernel/sched/yes.md)
  - `security/`
    - [libkcapi : 为什么需要从内核中获取加密功能](./kernel/security/af_alg.md)
    - [drivers/crypto/ccp/ 源码中包含了什么东西](./kernel/security/amd-ccp-psp.md)
    - [kernel 中的 apparmor 是做啥的?](./kernel/security/apparmor.md)
    - [监控 /etc/passwd 的读写属性变更](./kernel/security/audit-basic.md)
    - [audit 机制](./kernel/security/audit.md)
    - [记录一些内核的 CVE 的 blog](./kernel/security/cve.md)
    - [hw-vuln](./kernel/security/hw-vuln.md)
    - [CONFIG_MODULE_SIG 的作用](./kernel/security/module-sig.md)
    - [seccomp.c](./kernel/security/seccomp.md)
    - [selinux](./kernel/security/selinux.md)
    - [spectre](./kernel/security/spectre.md)
    - [一个小的 fix](./kernel/security/yama.md)
  - `signal/`
    - [QEMU 中的信号机制](./kernel/signal/qemu.md)
  - `sriov/`
    - [切分测试](./kernel/sriov/sriov.lab.md)
    - [https://learn.microsoft.com/en-us/windows-hardware/drivers/network/overview-of-single-root-i-o-virtualization--sr-iov-](./kernel/sriov/sriov.md)
    - [支持 SR-IOV 的 NVMe 设备概述（2025 年 11 月数据）](./kernel/sriov/sriov.nvme.md)
  - `swap/`
    - [4](./kernel/swap/fj.md)
    - [简单分析一下 folio 在 lru 中移动](./kernel/swap/folio_add_lru.md)
    - [Q : Linux 现在支持大页 swap out 吗?](./kernel/swap/hugepage.md)
    - [kvm](./kernel/swap/kvm.md)
    - [备忘](./kernel/swap/lruvec.md)
    - [page-io.c](./kernel/swap/page-io.md)
    - [RDMA Swap](./kernel/swap/rdma-swap.md)
    - [iouring swap](./kernel/swap/rswap.md)
    - [swap 模块基本分析](./kernel/swap/swap-overview.md)
    - [swap.c 分析](./kernel/swap/swap.md)
    - [理解下这个变化](./kernel/swap/swapcache.md)
    - [swapfile.c](./kernel/swap/swapfile.md)
    - [分析](./kernel/swap/user.md)
    - [zram 基本使用](./kernel/swap/zram.md)
    - [zswap](./kernel/swap/zswap.md)
  - `sysfs/`
    - [sysfs block](./kernel/sysfs/sysfs-blk.md)
    - [sysfs bus](./kernel/sysfs/sysfs-bus.md)
    - [sysfs cpu](./kernel/sysfs/sysfs-cpu.md)
    - [sysfs dev](./kernel/sysfs/sysfs-dev.md)
    - [sysfs fs](./kernel/sysfs/sysfs-fs.md)
    - [sysfs iommu](./kernel/sysfs/sysfs-iommu.md)
    - [sysfs irq](./kernel/sysfs/sysfs-irq.md)
    - [sysfs memory](./kernel/sysfs/sysfs-mm.md)
    - [sysfs module](./kernel/sysfs/sysfs-module.md)
    - [sysfs net](./kernel/sysfs/sysfs-net.md)
    - [sysfs kobject](./kernel/sysfs/sysfs-obj.md)
    - [bus](./kernel/sysfs/sysfs-pci.md)
    - [proc fs](./kernel/sysfs/sysfs-proc.md)
    - [sysfs sched](./kernel/sysfs/sysfs-sched.md)
    - [sysfs scsi](./kernel/sysfs/sysfs-scsi.md)
    - [sysfs](./kernel/sysfs/sysfs.md)
  - `time/`
    - `code/`
      - [timer 相关测试](./kernel/time/code/README.md)
    - `kvmclock/`
      - [kvmclock ioctl](./kernel/time/kvmclock/ioctl.md)
      - [khz](./kernel/time/kvmclock/khz.md)
      - [常用 backtrace](./kernel/time/kvmclock/kvmclock.md)
      - [这个问题难道不会影响到虚拟机的热迁移吗?](./kernel/time/kvmclock/kvmclock3.md)
      - [大部分 kvmclock3 的地方](./kernel/time/kvmclock/master-clock.md)
      - [QEMU/KVM 的 kvmclock 热迁移：保存时间值，在目标重新建立映射](./kernel/time/kvmclock/migration-codex.md)
      - [问题是，为什么同步 host 的 CLOCK_MONOTONIC_RAW 到 guest 中会导致问题](./kernel/time/kvmclock/mono-raw.md)
      - [x86 KVM 时间相关寄存器](./kernel/time/kvmclock/msr.md)
      - [基本逻辑](./kernel/time/kvmclock/nested.md)
      - [从 Linux timekeeping 到 KVM clock：沿着一次读时间理解](./kernel/time/kvmclock/overview-codex.md)
      - [kvmclock 基础](./kernel/time/kvmclock/pvti.md)
      - [qemu patch 代码](./kernel/time/kvmclock/qemu-patch.md)
      - [利用 gdb 暂停的确存在本质的不同:](./kernel/time/kvmclock/stall.md)
      - [为什么不去直接使用 tsc 来解决问题](./kernel/time/kvmclock/tsc.md)
      - [不如直接用 tsc](./kernel/time/kvmclock/use-tsc.md)
      - [hypervisor 的工作](./kernel/time/kvmclock/wallclock.md)
      - [kvmclock windows](./kernel/time/kvmclock/windows.md)
    - `lab/`
      - [Linux time system 基本观察](./kernel/time/lab/lab1-cmd.md)
      - [描述从 从 clockid 到 hrtimer_bases 的变化:](./kernel/time/lab/lab3-kmod.md)
      - [lab5-kvmclock](./kernel/time/lab/lab5-kvmclock.md)
      - [sysfs](./kernel/time/lab/lab6-jump.md)
      - [这个不错](./kernel/time/lab/lab8-crond.md)
    - `namespace/`
      - [time namespace](./kernel/time/namespace/namespace.md)
    - `ntp/`
      - [ntp](./kernel/time/ntp/basic.md)
      - [hwclock](./kernel/time/ntp/hwclock.md)
    - `others/`
      - [acpi_pm](./kernel/time/others/acpi_pm.md)
    - `syscall/`
      - [clock 类型](./kernel/time/syscall/clock-type.md)
      - [使用 syscall](./kernel/time/syscall/monotonic-coarse.md)
      - [time syscall](./kernel/time/syscall/syscall.md)
    - `tsc/`
      - [tsc calibraction](./kernel/time/tsc/calibraction.md)
      - [seabios 中也会校准时间](./kernel/time/tsc/seabios.md)
      - [tsc 之前的同步](./kernel/time/tsc/sync.md)
      - [tsc reorder](./kernel/time/tsc/tsc.md)
    - `vdso/`
      - [vdso](./kernel/time/vdso/vdso.md)
    - [time keeping 机制中为什么忽视 85ns 的延迟](./kernel/time/85ns.md)
    - [基本问题](./kernel/time/basic.md)
    - [Clock Event Devices](./kernel/time/clockevent.md)
    - [Overview](./kernel/time/clocksource.md)
    - [Linux 内核时间子系统文档汇编](./kernel/time/doc.md)
    - [更新 jiffies_64 的路径](./kernel/time/jiffies.md)
    - [soft hard mode](./kernel/time/kmod-timer.md)
    - [还是两个数值](./kernel/time/kvm-nested.md)
    - [preemempt timer exit](./kernel/time/kvm-preemption-timer.md)
    - [时钟中断可以做 interrupt posting 吗?](./kernel/time/kvm-timer.md)
    - [arch/x86/kvm/i8254.c](./kernel/time/legacy.md)
    - [太抽象了](./kernel/time/migration.md)
    - [网卡模块中](./kernel/time/ptp.md)
    - [sched_clock](./kernel/time/sched-clock.md)
    - [struct tick_device](./kernel/time/tick_device.md)
    - [clocksource watchdog](./kernel/time/time-keeping-watchdog.md)
    - [感觉，实际上，arm 也是没问题的](./kernel/time/timer.aarch64.md)
    - [Documentation/timers/](./kernel/time/timer.doc.md)
    - [tick 模式](./kernel/time/timer.dynamic.md)
    - [QEMU 中的时钟](./kernel/time/timer.qemu.md)
    - [tk_read_base](./kernel/time/timer.timekeeping.md)
    - [soft lockup](./kernel/time/watchdog.md)
    - [CLOCK_MONOTONIC 和 CLOCK_MONOTONIC_RAW 的区别在于](./kernel/time/yes-we-know.md)
  - `tutorial/`
    - `crash/`
      - [基于 kcore 的几种内核调试办法](./kernel/tutorial/crash/kcore.md)
    - [和社区沟通](./kernel/tutorial/community.md)
    - [DKMS](./kernel/tutorial/dkms.md)
    - [阅读文档](./kernel/tutorial/doc.md)
    - [形式化验证](./kernel/tutorial/format-verification.md)
    - [fuzz](./kernel/tutorial/fuzz.md)
    - [gdb kernel 的常用命令](./kernel/tutorial/gdb-kernel.md)
    - [git](./kernel/tutorial/git.md)
    - [kcov](./kernel/tutorial/kcov.md)
    - [Linux kernel Labs 笔记](./kernel/tutorial/linux-kernel-labs.md)
    - [杂谈](./kernel/tutorial/misc.md)
    - [prepare](./kernel/tutorial/prepare.md)
    - [proxmox 基本使用](./kernel/tutorial/proxmox.md)
    - [sparse && smatch](./kernel/tutorial/sparse.md)
    - [Linux 测试](./kernel/tutorial/test.md)
    - [uml](./kernel/tutorial/uml.md)
    - [记录一些极其奇怪的问题](./kernel/tutorial/wired.md)
  - `usb/`
    - [qemu 模型](./kernel/usb/usb-qemu.md)
    - [usb 分析记录记录](./kernel/usb/usb.md)
  - `vfio/`
    - [内核文档](./kernel/vfio/doc.md)
    - [看看这个 ACS override 是什么鬼?](./kernel/vfio/group.md)
    - [PWN : Posted MSI notification event](./kernel/vfio/int-posted-msi.md)
    - [vfio 内核实现](./kernel/vfio/internal-kernel.md)
    - [qemu](./kernel/vfio/internal-qemu.md)
    - [mdev-no-iommu](./kernel/vfio/mdev-no-iommu.md)
    - [vfio misc](./kernel/vfio/misc.md)
    - [noiommu](./kernel/vfio/noiommu.md)
    - [uio](./kernel/vfio/uio.md)
    - [nvgrace-gpu](./kernel/vfio/vGPU.md)
    - [vfio](./kernel/vfio/vfio.md)
    - [drivers/vfio/pci/virtio 是做什么的](./kernel/vfio/virtio.md)
  - `xdc/`
    - [xdc](./kernel/xdc/2025.md)
  - [收集经典 backtrace](./kernel/backtrace.md)
  - [rfkill](./kernel/bluetooth.md)
  - [AMBA](./kernel/bus-axi.md)
  - [nvlink](./kernel/bus-nvlink.md)
  - [psi](./kernel/bus-spi.md)
  - [tilelink](./kernel/bus-tilelink.md)
  - [bus](./kernel/bus.md)
  - [CFI in kernel](./kernel/cfi.md)
  - [中国 Linux 大会记录](./kernel/clk.md)
  - [linux 设备驱动](./kernel/device.md)
  - [对 swap device 错误注入](./kernel/fault-inject-swap.md)
  - [错误注入](./kernel/fault-inject.md)
  - [记录一些 Linux kernel 中好玩的东西](./kernel/fun.md)
  - [futex](./kernel/futex.md)
  - [hid](./kernel/hid.md)
  - [基本启动流程](./kernel/init.md)
  - [总结常用的 kernel cmdline](./kernel/kernel-parameters.md)
  - [内核学习经验 : 进阶版](./kernel/learn-linux-kernel-v2.md)
  - [livepatch](./kernel/livepatch.md)
  - [dracut 这个警告有意思](./kernel/microcode.md)
  - [杂记](./kernel/misc.md)
  - [mknod](./kernel/mknod.md)
  - [namespace](./kernel/namespace.md)
  - [notifier](./kernel/notifier.md)
  - [resource](./kernel/resource.md)
  - [rlimit](./kernel/rlimit.md)
  - [rng](./kernel/rng.md)
  - [sel4](./kernel/sel4.md)
  - [内核屎山评选](./kernel/shit-kernel.md)
  - [QEMU 屎山评选](./kernel/shit-qemu.md)
  - [sound](./kernel/sound.md)
  - [syscall](./kernel/syscall.md)
  - [sysv ipc](./kernel/sysv-ipc.md)
  - [命令缩写](./kernel/tools-abbr.md)
  - [为什么 Linux Kernel 的代码质量比 QEMU 更高](./kernel/why-better-than-qemu.md)
- `kr/`
  - [2024](./kr/2024.md)
  - [kernel-recipes 2025](./kr/2025.md)
- `kvm/`
  - `emulate/`
    - [opcode_table 的使用位置](./kvm/emulate/details.md)
    - [x86 KVM 为什么需要指令模拟](./kvm/emulate/emulate-2.md)
    - [到底是什么在触发](./kvm/emulate/type.md)
  - `hyperv/`
    - [Hyperv Enlightment](./kvm/hyperv/hyperv-pv.md)
    - [HyperV](./kvm/hyperv/hyperv.md)
    - [hyper-v 基本使用](./kvm/hyperv/in-hyperv-manager.md)
  - `kvm-forum/`
    - [kvm forum](./kvm/kvm-forum/README.md)
  - `mini-kvm/`
    - [Rust VMM + C guest](./kvm/mini-kvm/README.md)
  - [KVM 机制演进与源码阅读索引](./kvm/mechanism-evolution.md)
  - [pfncache.c](./kvm/pfncache.md)
- `language/`
  - `c/`
    - [TODO](./language/c/c.md)
  - `python/`
    - [静态函数和静态成员](./language/python/basic.md)
    - [uv 的基本使用](./language/python/uv.md)
  - [My cmake Notes](./language/cmake.md)
  - [SML](./language/functional_programming.md)
  - [Go](./language/go.md)
  - [haskell](./language/haskell.md)
  - [structure](./language/java-containers.md)
  - [重学Java](./language/java.md)
  - [Where to learn](./language/js.md)
  - [资源](./language/lua.md)
  - [Nim](./language/nim.md)
  - [教程](./language/tex.md)
- `linux/`
  - `fedora/`
    - `wecom-notify-bridge/`
      - [企业微信消息通知桥](./linux/fedora/wecom-notify-bridge/README.md)
  - `tlpi/`
    - `0/`
      - [File I/O: Further Details](./linux/tlpi/0/tlpi-chapter-05.md)
    - `1/`
      - [Time](./linux/tlpi/1/tlpi-chapter-10.md)
      - [File Systems](./linux/tlpi/1/tlpi-chapter-14.md)
      - [File Attributes](./linux/tlpi/1/tlpi-chapter-15.md)
      - [Extended Attributes](./linux/tlpi/1/tlpi-chapter-16.md)
      - [Access Control Lists](./linux/tlpi/1/tlpi-chapter-17.md)
      - [Monitor Filesystem Events](./linux/tlpi/1/tlpi-chapter-19.md)
    - `2/`
      - [Signals: Fundamental Concepts](./linux/tlpi/2/tlpi-chapter-20.md)
      - [Timers and Sleeping](./linux/tlpi/2/tlpi-chapter-23.md)
      - [Process Creation](./linux/tlpi/2/tlpi-chapter-24.md)
      - [Process Termination](./linux/tlpi/2/tlpi-chapter-25.md)
      - [Monitoring Child Processes](./linux/tlpi/2/tlpi-chapter-26.md)
      - [Program Execution](./linux/tlpi/2/tlpi-chapter-27.md)
      - [Process Creation and Program Execution in More Detail](./linux/tlpi/2/tlpi-chapter-28.md)
      - [Threads: Introduction](./linux/tlpi/2/tlpi-chapter-29.md)
    - `3/`
      - [Threads: Thread Synchronization](./linux/tlpi/3/tlpi-chapter-30.md)
      - [Threads: Further Details](./linux/tlpi/3/tlpi-chapter-33.md)
      - [Process Group, Sessions, and Job Control](./linux/tlpi/3/tlpi-chapter-34.md)
    - `4/`
      - [System V Message Queues](./linux/tlpi/4/tlpi-chapter-46.md)
      - [SYSTEM V Semaphores](./linux/tlpi/4/tlpi-chapter-47.md)
      - [System V Shared Memory](./linux/tlpi/4/tlpi-chapter-48.md)
    - `5/`
      - [Introduction to POSIX IPC](./linux/tlpi/5/tlpi-chapter-51.md)
      - [POSIX Message Queues](./linux/tlpi/5/tlpi-chapter-52.md)
      - [POSIX Semaphores](./linux/tlpi/5/tlpi-chapter-53.md)
      - [POSIX Shared Memory](./linux/tlpi/5/tlpi-chapter-54.md)
      - [File Locking](./linux/tlpi/5/tlpi-chapter-55.md)
      - [Sockets: Introduction](./linux/tlpi/5/tlpi-chapter-56.md)
    - `6/`
      - [Sockets: Advanced Topics](./linux/tlpi/6/tlpi-chapter-61.md)
      - [Terminals](./linux/tlpi/6/tlpi-chapter-62.md)
      - [Alternative I/O Models](./linux/tlpi/6/tlpi-chapter-63.md)
    - [为 tlpi 的可执行文件添加.out 扩展名](./linux/tlpi/change-extension.md)
  - [Makefile](./linux/Makefile.md)
  - [有趣的](./linux/android.md)
  - [ansible 记录](./linux/ansible.md)
  - [cuda gdb](./linux/cuda-gdb.md)
  - [Debugger 的理念，原理和使用](./linux/gdb.md)
  - [阅读 musl 学到的一些东西](./linux/musl.md)
  - [omarchy](./linux/omarchy.md)
  - [如何给 OpenEuler 提交打包openeul](./linux/openeuler.md)
  - [ubuntu 使用的问题合集](./linux/ubuntu.md)
- `math/`
  - `applied/`
    - [自动化控制理论](./math/applied/auto.md)
  - `basic/`
    - `complex/`
      - [首次使用](./math/basic/complex/env.md)
- `net/`
  - `dccp/`
    - `dccp/`
      - [dccp](./net/dccp/dccp/README.md)
  - `demo/`
    - [macvlan & ipvlan 网络虚拟化 Demo](./net/demo/README.md)
  - `geneve/`
    - [GENEVE 隧道实验](./net/geneve/README.md)
  - `kernel/`
    - `ipv4/`
      - [af_net.c](./net/kernel/ipv4/overview.md)
      - [IP Fragmentation and Reassembly](./net/kernel/ipv4/reassemb.md)
    - `mac80211/`
      - [wiki](./net/kernel/mac80211/mac80211-overview.md)
    - `sched/`
      - [sched](./net/kernel/sched/sched-overview.md)
    - [网络栈的一些源码分析](./net/kernel/kernel-src-overview.md)
  - `misc/`
    - [impala : wifi 图形管理工具](./net/misc/wifi-80211.md)
  - `pxe/`
    - [pxe](./net/pxe/pxe.md)
  - `rdma/`
    - `rdma-demo/`
      - `docs/`
        - [RDMA Atomic 操作详解](./net/rdma/rdma-demo/docs/RDMA_ATOMIC.md)
        - [RDMA操作类型](./net/rdma/rdma-demo/docs/RDMA操作类型.md)
        - [为什么 RDMA 程序需要 TCP？](./net/rdma/rdma-demo/docs/WHY_TCP.md)
      - [RDMA Programming Demo](./net/rdma/rdma-demo/README.md)
    - `rdma-demo2/`
      - [基本环境搭建](./net/rdma/rdma-demo2/README.md)
    - [rdma cm](./net/rdma/cm.md)
    - [mmap 的观测](./net/rdma/dma-and-int.md)
    - [doc](./net/rdma/doc.md)
    - [rdma 环境准备](./net/rdma/lab.md)
    - [qemu](./net/rdma/misc.md)
    - [mlnx](./net/rdma/ofed.md)
    - [RDMA](./net/rdma/overview.md)
    - [RDMA Core 详解](./net/rdma/rdma-core.md)
    - [RDMA 杂谈](./net/rdma/rdma-insights.md)
    - [RDMA 网卡配置指南](./net/rdma/rdma-setup.md)
    - [smc](./net/rdma/smc-r.md)
    - [rdma 常用工具](./net/rdma/tools.md)
  - `sfc/`
    - [sfc](./net/sfc/basic.md)
  - `skbuff/`
    - [skbuff](./net/skbuff/skbuff.md)
  - `vxlan-demo/`
    - [VXLAN](./net/vxlan-demo/README.md)
  - [9p](./net/9p.md)
  - [bgp](./net/bgp.md)
  - [bonding](./net/bonding.md)
  - [bridge](./net/bridge.md)
  - [cloudflare](./net/cloudflare.md)
  - [dhcp](./net/dhcp.md)
  - [diag](./net/diag.md)
  - [DNS](./net/dns.md)
  - [BlueField-3](./net/dpu.md)
  - [背景介绍](./net/erspan.md)
  - [geneve](./net/geneve.md)
  - [gro](./net/gro.md)
  - [公共的 wifi 意味着什么?](./net/hotel.md)
  - [icmp](./net/icmp.md)
  - [igmp](./net/igmp.md)
  - [ipsec 是什么](./net/ipsec.md)
  - [ipvlan](./net/ipvlan.md)
  - [ipvs](./net/ipvs.md)
  - [网络杂谈](./net/kernel-hacking.md)
  - [ipvs](./net/lb.md)
  - [lldp](./net/lldp.md)
  - [LWT（Light Weight Tunnel）是什么?](./net/lwt.md)
  - [macvlan](./net/macvlan.md)
  - [mptcp](./net/mptcp.md)
  - [backlog](./net/napi.md)
  - [neighbour](./net/neighbour.md)
  - [网络基本配置](./net/net-config.md)
  - [网络问题常见排查思路](./net/net-debug.md)
  - [Linux kernel network stack lock](./net/net-lock.md)
  - [loopback](./net/net-loopback.md)
  - [mlx5](./net/net-mlx5.md)
  - [网络的 namespace](./net/net-namespace.md)
  - [net-phy](./net/net-phy.md)
  - [Network Route](./net/net-route.md)
  - [rpc](./net/net-rpc.md)
  - [net-sendfile](./net/net-sendfile.md)
  - [network timestamping](./net/net-timestamping.md)
  - [Network tools internals](./net/net-tools.md)
  - [linux 网络基础查漏补缺](./net/net.md)
  - [这应该就是 kernel network 的会议吧](./net/netdev.md)
  - [netfilter](./net/netfilter.md)
  - [netlink](./net/netlink.md)
  - [nettrace](./net/nettrace.md)
  - [nic driver](./net/nic-driver.md)
  - [网卡名称](./net/nic-name.md)
  - [nmcli 基本使用](./net/nmcli.md)
  - [network offload](./net/offload.md)
  - [ovn](./net/ovn.md)
  - [openvswitch](./net/ovs.md)
  - [网络性能](./net/perfermance.md)
  - [从这里切入的确不错](./net/pingora.md)
  - [mac 会不断的产生这个日志](./net/promiscuous.md)
  - [qdisc](./net/qdisc.md)
  - [quic](./net/quic.md)
  - [net: raw socket](./net/raw-socket.md)
  - [rds](./net/rds.md)
  - [RxRPC](./net/rxrpc.md)
  - [unix domain 分析](./net/scm.md)
  - [sctp](./net/sctp.md)
  - [smart-nic](./net/smart-nic.md)
  - [snmp](./net/snmp.md)
  - [socat](./net/socat.md)
  - [https://linux.die.net/man/7/socket](./net/socket.md)
  - [sockmap](./net/sockmap.md)
  - [ssh](./net/ssh.md)
  - [stp](./net/stp.md)
  - [rpc](./net/sunrpc.md)
  - [switch](./net/switch.md)
  - [tc 和 tcp congestion control](./net/tc.md)
  - [tcp ip syn](./net/tcp-ip-syn.md)
  - [A TCP/IP Tutorial 阅读笔记](./net/tcp-ip.md)
  - [tcptrace 如何使用](./net/tcptrace.md)
  - [tipc](./net/tipc.md)
  - [tls](./net/tls.md)
  - [tun tap](./net/tun-tap.md)
  - [linux 的 tunnel 技术](./net/tunnel.md)
  - [unix domain 分析](./net/uds.md)
  - [quic](./net/usermod-app.md)
  - [netmap](./net/usermod-network.md)
  - [level-ip](./net/usermod-stack-level-ip.md)
  - [veth](./net/veth.md)
  - [virtio-net](./net/virtio-net.md)
  - [tracking](./net/vlan.md)
  - [GVE](./net/vnic.md)
  - [wid crash 报告](./net/wid.md)
  - [wireguard](./net/wireguard.md)
  - [xdp](./net/xdp.md)
  - [enum netdev_priv_flags](./net/yes.md)
- `pci/`
  - `option-rom/`
    - [从 qemu 的角度分析 Option ROM](./pci/option-rom/doc.md)
  - [虚拟机中为什么会有 rescan 的操作](./pci/debug.md)
  - [内核中关于 dma 的几个目录做什么的](./pci/dma.md)
  - [pci](./pci/doc.md)
  - [Linux Kernel 如何管理 PCIe 设备](./pci/kernel.md)
  - [问题](./pci/lab-2.md)
  - [question](./pci/lab-3.md)
  - [pcie bridge 和 pcie port 到底是什么鬼](./pci/lab.md)
  - [求求了，彻底搞清楚这个问题](./pci/msix.md)
  - [p2pdma](./pci/p2pdma.md)
  - [dma](./pci/qemu.md)
- `qemu/`
  - [qemu 封装](./qemu/ease-of-use.md)
  - [设置环境变量方便编译示例](./qemu/libkrun-analysis.md)
- `rust/`
  - `too-many-linked-lists/`
    - `ai/`
      - [Introduction（引言）](./rust/too-many-linked-lists/ai/00-introduction.md)
      - [第一章 A Bad Stack（一个糟糕的单链表栈）](./rust/too-many-linked-lists/ai/01-first-bad-stack.md)
      - [第二章 An Ok Stack（一个还行的单链表栈）](./rust/too-many-linked-lists/ai/02-second-ok-stack.md)
      - [第三章 A Persistent Stack（持久化栈）](./rust/too-many-linked-lists/ai/03-third-persistent-stack.md)
      - [第四章 A Bad but Safe Doubly-Linked Deque（糟糕但安全的双向双端队列）](./rust/too-many-linked-lists/ai/04-fourth-bad-safe-deque.md)
      - [第五章 An Ok Unsafe Queue（还行的 unsafe 队列）](./rust/too-many-linked-lists/ai/05-fifth-ok-unsafe-queue.md)
      - [第六章 A Production Unsafe Deque（生产级 unsafe 双端队列）](./rust/too-many-linked-lists/ai/06-sixth-production-unsafe-deque.md)
      - [第七章 A Bunch of Silly Lists（一堆整活链表）](./rust/too-many-linked-lists/ai/07-infinity-silly-lists.md)
  - [rust 基础](./rust/basic.md)
  - [Rust 中 Move、Copy、Clone 和 Drop 的关系](./rust/clone-copy-drop.md)
  - [rust gdb 基本使用方法](./rust/debug.md)
  - [Rust 学习记录](./rust/overview.md)
  - [rust 的 smart pointers](./rust/pointers.md)
  - [对比 Rust 和 Cpp](./rust/rust-vs-cpp.md)
- `shell/`
  - [bash 核心](./shell/internal.md)
- `systemd/`
  - [man user@.service](./systemd/systemd-internal.md)
  - [我所知道 systemd 的全部](./systemd/systemd.md)
  - [udev 机制深度解析](./systemd/udev.basic.md)
  - [udev 机制](./systemd/udev.md)
- `tools/`
  - `rpm/`
    - [RPM](./tools/rpm/basic.md)
  - [构建系统](./tools/build.md)
  - [Gerrit](./tools/gerrit.md)
  - [git](./tools/git.md)
  - [github](./tools/github.md)
  - [jinkens](./tools/jenkins.md)
  - [各种小工具](./tools/misc-tools.md)
  - [mutt](./tools/mutt.md)
  - [typst 工具](./tools/typst.md)
  - [how to debug neovim](./tools/vimrc.md)
  - [vscode 的调试环境](./tools/vscode.md)
- `trace/`
  - `ebpf/`
    - `code/`
      - `libbpf-rs-demo/`
        - [libbpf-rs demo](./trace/ebpf/code/libbpf-rs-demo/README.md)
    - [libbpf-tools](./trace/ebpf/libbpf-tools.md)
  - `gpu/`
    - [Nsight Systems / Nsight Compute 实验](./trace/gpu/README.md)
  - `tools/`
    - [计划和代办](./trace/tools/README.md)
  - [Linux Trace 技术整理报告](./trace/TRACE_INVENTORY.md)
  - [trace 机制实现](./trace/internal.md)
  - [rtla](./trace/rlta.md)
- `uefi/`
  - `BootLoaderPkg/`
    - [第一个 UEFI 程序](./uefi/BootLoaderPkg/README.md)
  - `edk2/`
    - [UEFI 入门](./uefi/edk2/1.md)
    - [编译 edk2](./uefi/edk2/build.md)
    - [具体的源码分析](./uefi/edk2/code-detail.md)
    - [Linux UEFI 学习环境搭建](./uefi/edk2/setup.md)
  - `firmware/`
    - [drivers/base/firmware_loader 通用框架加载驱动](./uefi/firmware/firmware_loader.md)
    - [TODO](./uefi/firmware/overview.md)
    - [其他](./uefi/firmware/version.md)
  - [rust-hypervisor-firmware](./uefi/boot.md)
  - [core n100 ，启动!](./uefi/coreboot.md)
  - [dmi](./uefi/dmi.md)
  - [为什么内核中存在这么多 efi 相关的东西](./uefi/kernel.md)
  - [使用 qemu 来理解 boot 机器](./uefi/qemu-boot.md)
  - [uboot](./uefi/uboot.md)
  - [UEFI 实战 : 将 QEMU 转换为 UEFI Application](./uefi/uefi-in-action.md)
- `virt/`
  - `libvirt/`
    - [flint](./virt/libvirt/flint.md)
    - [libvirt](./virt/libvirt/overview.md)
    - [简要分析下 source code 的位置](./virt/libvirt/source-code.md)
    - [virsh 基本使用](./virt/libvirt/virsh.md)
    - [virt-manager 可以尝试一下](./virt/libvirt/virt-manager.md)
  - [ACRN](./virt/acrn.md)
  - [gvisor](./virt/gvisor.md)
  - [jailhouse](./virt/jailhouse.md)
  - [misc](./virt/misc.md)
  - [应该测试一下 wsl 的东西](./virt/wsl.md)
- `virtio/`
  - [virtio packed ring](./virtio/packed-ring.md)
  - [virtio-iommu](./virtio/virtio-iommu.md)
  - [virtio split ring 结构](./virtio/virtio.md)
- `vmware/`
  - [vmware 简单记录](./vmware/vmware.md)
- `windows/`
  - `code/`
    - `docs/`
      - [Linux / Windows 系统编程对照](./windows/code/docs/linux_windows_mapping.md)
    - [Windows 系统编程 Demo](./windows/code/README.md)
  - `driver/`
    - `code/`
      - `kmdf-hello/`
        - [KMDF Hello](./windows/driver/code/kmdf-hello/README.md)
    - [virtio-win 驱动手动构建与安装记录（BUILD NOTES）](./windows/driver/BUILD-NOTES.md)
    - [windows 驱动开发](./windows/driver/windows-driver.md)
  - [Windows 蓝屏 dump 分析](./windows/crash.md)
  - [dotnet 简述](./windows/dotnet.md)
  - [Windows 内存管理](./windows/memory.md)
  - [mingw](./windows/mingw.md)
  - [windows 杂谈](./windows/misc.md)
  - [windows 网络](./windows/net.md)
  - [windows 性能测试工具](./windows/tools.md)
  - [如何将 windows 放到虚拟机中](./windows/virt.md)
- `xen/`
  - [xen](./xen/README.md)
<!-- END AUTO DOCS INDEX -->

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
