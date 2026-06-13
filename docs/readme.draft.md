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

<!-- BEGIN AUTO DOCS INDEX -->
## 自动文档索引

以下只包含当前 README 手工区还没有引用的 Markdown 文档。

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
  - `RuinGuard/`
    - [AI 下如何加速学习](./ai/RuinGuard/basic.md)
  - `rust-teach-infer/`
    - `vendor/`
      - `candle-kernels/`
        - [candle-kernels](./ai/rust-teach-infer/vendor/candle-kernels/README.md)
    - [rust-teach-infer](./ai/rust-teach-infer/README.md)
  - `tools/`
    - [各种 browser 的自动化也了解下](./ai/tools/brower.md)
    - [这机器可以看看](./ai/tools/mcp.md)
    - [misc](./ai/tools/misc.md)
    - [pdf](./ai/tools/pdf.md)
    - [似乎现在有了 ai ，前端工具就是做 slides 最好的方法](./ai/tools/slides.md)
  - [ACP](./ai/acp.md)
  - [Chrome DevTools MCP 配置指南](./ai/chrome-devtools-mcp-setup.md)
  - [claude 核心教程](./ai/claude.md)
  - [AI 带来的启发](./ai/inspiration.md)
  - [AI 编程辅助工具调研](./ai/lsp.md)
  - [AI 相关工具与项目备忘](./ai/misc.md)
  - [AI Skills 资源收集](./ai/skills.md)
  - [值得尝试的 AI 应用方向](./ai/try.md)
  - [AI 学习与应用笔记](./ai/why.md)
  - [解决封锁问题](./ai/workaround.md)
- `benchmark/`
  - [性能基准测试工具](./benchmark/benchmarks.md)
  - [个人性能测试记录](./benchmark/my-result.md)
- `blog/`
  - [这个相当不错的](./blog/ai.md)
  - [https://borretti.me/article/hashcards-plain-text-spaced-repetition](./blog/anki.md)
- `bmbt/`
  - [BMBT 常见问题解答](./bmbt/1-why.md)
  - [BMBT newbie 必读](./bmbt/5-newbie.md)
  - [二进制翻译介绍](./bmbt/bt-introduction.md)
  - [QEMU 如何模拟 pcspker](./bmbt/pcspk.md)
- `concurrent/`
  - `arch/`
    - [简单看看 aarch64 的指令支持](./concurrent/arch/aarch64.md)
    - [反汇编常见指令](./concurrent/arch/disassmble.md)
    - [CPU 微架构](./concurrent/arch/instructions.md)
    - [riscv](./concurrent/arch/riscv.md)
    - [x86](./concurrent/arch/x86.md)
  - `code/`
    - `tools/`
      - [tsan](./concurrent/code/tools/README.md)
    - [store-buffer](./concurrent/code/README.md)
  - `kernel/`
    - `api/`
      - [阅读下 Documentation/atomic_t.txt](./concurrent/kernel/api/atomic.md)
      - [gcc atomic](./concurrent/kernel/api/gcc-atomic.md)
      - [mutex](./concurrent/kernel/api/mutex.md)
      - [为什么 percpu 还需要 rwsem 啊?](./concurrent/kernel/api/percpu-rwsem.md)
      - [PER_CPU](./concurrent/kernel/api/percpu.md)
      - [rcuwait](./concurrent/kernel/api/rcuwait.md)
      - [refcount](./concurrent/kernel/api/refcount.md)
      - [kernel/locking/rt_mutex.md](./concurrent/kernel/api/rt_mutex.md)
      - [rwlock](./concurrent/kernel/api/rwlock.md)
      - [R/W semaphore](./concurrent/kernel/api/rwsem.md)
      - [Semaphores](./concurrent/kernel/api/semaphore.md)
      - [seqlock](./concurrent/kernel/api/seqlock.md)
      - [introduction to spinlock](./concurrent/kernel/api/spinlock.md)
      - [swait](./concurrent/kernel/api/swait.md)
      - [wait](./concurrent/kernel/api/wait.md)
      - [waitbit](./concurrent/kernel/api/waitbit.md)
      - [ww_mutex](./concurrent/kernel/api/ww_mutex.md)
    - `qa/`
      - [什么时候需要使用 atomic 的版本](./concurrent/kernel/qa/qa-1.md)
      - [folio_test_lru 提前判断来加速](./concurrent/kernel/qa/qa-10.md)
      - [qa-11](./concurrent/kernel/qa/qa-11.md)
      - [[ ] 看个 smp_load_acquire  smp_store_release 的例子](./concurrent/kernel/qa/qa-12.md)
      - [smp_load_acquire 和 smp_cond_load_acquire 的关系是什么?](./concurrent/kernel/qa/qa-2.md)
      - [smp_store_release 为什么不可以满足需求，而必须使用 smp_wmb](./concurrent/kernel/qa/qa-3.md)
      - [经典案例，更新数组指针](./concurrent/kernel/qa/qa-4.md)
      - [fdtable ，超级经典的 rcu 使用，将两个问题(代办)](./concurrent/kernel/qa/qa-5.md)
      - [为什么 backing-dev 中的这个 look up 需要 bh 的屏蔽](./concurrent/kernel/qa/qa-6.md)
      - [为什么 napi enable 的时候需要屏蔽掉 softirq ?](./concurrent/kernel/qa/qa-7.md)
      - [blk_mq_raise_softirq 中操作 percpu 变量需要 preempt_disable](./concurrent/kernel/qa/qa-8.md)
      - [kmap 和 kmap_atomic](./concurrent/kernel/qa/qa-9.md)
    - [data_race](./concurrent/kernel/kcsan.md)
    - [内核中可以阅读材料](./concurrent/kernel/kernel-doc.md)
    - [存放 kernel 各个模块的锁的设计](./concurrent/kernel/kernel-lock-design.md)
    - [源码](./concurrent/kernel/lockdep-internal.md)
    - [配套测试代码](./concurrent/kernel/lockdep-usage.md)
  - `lan/`
    - [最后，总结对比一下吧](./concurrent/lan/README.md)
  - `lkmm/`
    - `tests/`
      - [LKMM Litmus Tests](./concurrent/lkmm/tests/README.md)
    - `zh-cn/`
      - [LKMM 文档中文辅助材料](./concurrent/lkmm/zh-cn/README.md)
    - [3. 内核源码中的 LKMM](./concurrent/lkmm/ai-overview.md)
    - [Linux Kernel Memory Model (LKMM) 完全指南](./concurrent/lkmm/ai-read.md)
    - [rcu 的邮件，每一个都需要阅读下](./concurrent/lkmm/human.md)
    - [其他 Memory Model 测试工具](./concurrent/lkmm/other-tools.md)
    - [8. LKMM vs C/C++ Memory Model](./concurrent/lkmm/vs-cpp.md)
  - `multiprocessor/`
    - [Introduction](./concurrent/multiprocessor/1.md)
    - [Concurrent Queues and the ABA Problem](./concurrent/multiprocessor/10.md)
    - [Concurrent Stacks and Elimination](./concurrent/multiprocessor/11.md)
    - [Counting, sorting and distributed Coordination](./concurrent/multiprocessor/12.md)
    - [13 Concurrent Hashing and Natural Parallelism](./concurrent/multiprocessor/13.md)
    - [Futures, Scheduling, and Work Distribution](./concurrent/multiprocessor/16.md)
    - [Mutual Exclusion](./concurrent/multiprocessor/2.md)
    - [3 Concurrent Objects](./concurrent/multiprocessor/3.md)
    - [Foundations of shared Memory](./concurrent/multiprocessor/4.md)
    - [5 The Relative Power of Primitive Synchronization Operations](./concurrent/multiprocessor/5.md)
    - [Universality of Consensus](./concurrent/multiprocessor/6.md)
    - [Spin Locks and Contention](./concurrent/multiprocessor/7.md)
    - [Monitors and Blocking Synchronization](./concurrent/multiprocessor/8.md)
    - [Linked Lists: The Role of Locking](./concurrent/multiprocessor/9.md)
    - [homework](./concurrent/multiprocessor/homework.md)
    - [Introduction](./concurrent/multiprocessor/notes.md)
    - [用于列车售票的可线性化并发数据结构](./concurrent/multiprocessor/report.md)
    - [讲义总结](./concurrent/multiprocessor/syn.md)
  - `perfbook/`
    - `autoread/`
      - `chapters/`
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
  - `rcu/`
    - [rcu 基本使用](./concurrent/rcu/1-usage.md)
    - [api](./concurrent/rcu/2-api.md)
    - [总结下其中的 workqueue 中间的 rcu](./concurrent/rcu/3-usage.md)
    - [rcu_read_lock_bh](./concurrent/rcu/bh.md)
    - [rcu boost](./concurrent/rcu/boost.md)
    - [context_tracking](./concurrent/rcu/context_tracking.md)
    - [Paul 维护的 RCU 文档](./concurrent/rcu/doc.md)
    - [如何理解什么 extended quiescent state](./concurrent/rcu/eqs.md)
    - [RCU](./concurrent/rcu/overview.md)
    - [一共存在那些种类的 RCU](./concurrent/rcu/qs.md)
    - [rcu_nocbs](./concurrent/rcu/rcu_nocbs.md)
    - [rcu by kimi](./concurrent/rcu/relearn-with-ai.md)
    - [srcu](./concurrent/rcu/srcu.md)
    - [文档](./concurrent/rcu/stall.md)
    - [CONFIG_TASKS_RCU](./concurrent/rcu/tasks_rcu.md)
    - [用户态 rcu](./concurrent/rcu/userspace-rcu.md)
    - [rcu 杂记](./concurrent/rcu/yes-we-know.md)
  - [Linux 内核 Litmus Tests 介绍](./concurrent/1-litmus.md)
  - [并发锁分析工具 lslocks](./concurrent/2-tools.md)
  - [Cache Coherence 机制](./concurrent/cache-coherency.md)
  - [并发编程中违反直觉的例子](./concurrent/counter-intuitive.md)
  - [并发数据结构 readerwriterqueue 分析](./concurrent/data-structure.md)
  - [梳理一下多核的基本生存法则](./concurrent/engineerings-perspective.md)
  - [并发编程趣味资源与工具](./concurrent/fun.md)
  - [memory model  IRIW](./concurrent/iriw.md)
  - [Lockless 无锁设计收集](./concurrent/lockless.md)
  - [Shared Memory Consistency Models: A Tutorial](./concurrent/m.md)
  - [Host 与 Guest 同步机制](./concurrent/misc.md)
  - [并发编程中 lockless 是什么意思](./concurrent/solutions.md)
  - [并发调试工具 TSan](./concurrent/tools.md)
  - [事务内存初识](./concurrent/transctiona-memory.md)
  - [并行编程实践记录](./concurrent/usage.md)
  - [为什么并行编程如此困难](./concurrent/why-parallel-is-hard.md)
  - [kernel/sched/membarrier.c syscall](./concurrent/yes.md)
- `container/`
  - [Podman Rootless 问题记录](./container/container.md)
  - [Docker](./container/docker.md)
  - [有一个感觉 nsenter 之类的工具类组成 docker 的](./container/minitools.md)
  - [Podman 使用指南](./container/podman.md)
  - [docker 代理的方法](./container/proxy.md)
- `cpu/`
  - `boom/`
    - [BOOM 微架构学习(1)——取指单元与分支预测](./cpu/boom/doc.md)
    - [源码阅读](./cpu/boom/source-code.md)
    - [BOOM 源码阅读](./cpu/boom/why-boom.md)
  - `rocket-chip/`
    - [总体来说，rocket-chip 中的人是无法实现](./cpu/rocket-chip/rocket-chip.md)
  - [chipyard 环境搭建](./cpu/chipyard.md)
  - [Chisel 学习资源汇总](./cpu/chisel.md)
  - [MIPS R10000 的设计](./cpu/mipsR10000.md)
  - [芯片设计相关资料汇总](./cpu/overview.md)
  - [Scala 基础语法学习](./cpu/scala.md)
  - [芯片设计中两个关键设备](./cpu/smart-dev.md)
  - [访存子系统](./cpu/xiangshan.md)
- `cxl/`
  - [CXL 技术资料](./cxl/readme.md)
- `dataplain/`
  - [DPDK 基础介绍](./dataplain/dpdk.md)
  - [SPDK 基础介绍](./dataplain/spdk.md)
- `games/`
  - [在 Linux kernel 上如何玩游戏](./games/kernel.md)
  - [SteamOS](./games/steam.md)
  - [wine : 如何实现系统调用虚拟化](./games/wine.md)
- `grub/`
  - [GRUB 配置详解](./grub/basic.md)
  - [grub](./grub/grub.md)
  - [systemd-boot](./grub/new.md)
- `hw/`
  - [2018 年款的小米笔记本](./hw/10-xiaomi.md)
  - [拯救者 R9000P 2023](./hw/2-7950hx.md)
  - [n100 https://bret.dk/intel-n100-radxa-x4-first-thoughts/](./hw/3-n100.md)
  - [13900K 的结果](./hw/debug-machine1.md)
  - [我靠，原来 widndows 也是可以修改，但是也没用](./hw/debug-machine2.md)
  - [Mac](./hw/mac.md)
  - [mac2](./hw/mac2.md)
  - [nano kvm](./hw/nano-kvm-pcie.md)
  - [REDMI K90PRO 2025](./hw/redmi.md)
- `kernel/`
  - `blk/`
    - `blktrace/`
      - [关键原料](./kernel/blk/blktrace/internal.md)
      - [blktrace 基本使用](./kernel/blk/blktrace/usage.md)
    - `ds/`
      - [其他的各种收集](./kernel/blk/ds/README.md)
    - `fio/`
      - [HDD 已死](./kernel/blk/fio/fio-result.md)
    - `mq/`
      - [flush](./kernel/blk/mq/bakctrace.md)
      - [bio-based 和 request-based 的区分](./kernel/blk/mq/bio-based-device.md)
      - [bio request request_queue 三者的关系](./kernel/blk/mq/bio-request.md)
      - [drivers/md/dm-rq.c](./kernel/blk/mq/dm-rq.md)
      - [Multi-Queue Block IO Queueing Mechanism (blk-mq)](./kernel/blk/mq/doc.md)
      - [Block Layer IO 请求路径详解 - Bypass 机制全梳理](./kernel/blk/mq/io_paths_analysis.md)
      - [Linux Device Driver : Block Drivers](./kernel/blk/mq/ldd-chapter-16.md)
      - [blk-mq-debugfs.c](./kernel/blk/mq/mq-debugfs.md)
      - [mq 核心结构体](./kernel/blk/mq/mq.md)
      - [storage blk plug 机制](./kernel/blk/mq/plug.md)
      - [sbitmap](./kernel/blk/mq/sbitmap.md)
      - [看看文档](./kernel/blk/mq/scheduler-deadline.md)
      - [什么情况下， block_rq_insert 但是无法 block_rq_issue](./kernel/blk/mq/scheduler.md)
      - [在 blk-mq 里，shared tags 的意思](./kernel/blk/mq/shared-tags.md)
      - [一个 request 创建的时候就需要 tag ，那么意味着创建的时候就需要知道发送给哪一个盘吗？](./kernel/blk/mq/tag.md)
      - [选项 BLK_WBT](./kernel/blk/mq/wbt.md)
      - [mq 的 queue 的数量就是 hctx 的数量](./kernel/blk/mq/yes.md)
    - `nvme/`
      - [nvme-cli](./kernel/blk/nvme/nvme-cli.md)
      - [深入浅出 SSD](./kernel/blk/nvme/nvme-hardware.md)
      - [先将 fabrics 也放到这里吧](./kernel/blk/nvme/nvme-tcp.md)
      - [nvme](./kernel/blk/nvme/nvme.md)
    - `raid/`
      - [资料](./kernel/blk/raid/general.md)
      - [同步模型](./kernel/blk/raid/lock.md)
      - [多个 raid 共享相同的物理盘会带来什么问题？](./kernel/blk/raid/not.md)
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
    - [block layer integrity](./kernel/blk/integrity.md)
    - [分析下 disk events](./kernel/blk/later.md)
    - [blk layer lock](./kernel/blk/lock.md)
    - [loop device](./kernel/blk/loop-device.md)
    - [device mapper](./kernel/blk/lvm.md)
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
    - [Cgroup V1](./kernel/cgroup/v1.md)
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
    - [分析 arm 的 cpuid 的代码](./kernel/cpuinfo/06-arm.md)
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
    - `epoll/`
      - [epoll 断联之后，可以继续重连](./kernel/fs/epoll/epoll-reconnect.md)
      - [epoll](./kernel/fs/epoll/epoll.md)
    - `ext4/`
      - [内部文档](./kernel/fs/ext4/doc.md)
      - [fs jbd2](./kernel/fs/ext4/jbd2.md)
      - [dumpe2fs](./kernel/fs/ext4/lab.md)
      - [先看看怎么用吧](./kernel/fs/ext4/overview.md)
    - `fuse/`
      - [Character devices in user space](./kernel/fs/fuse/fuse-cuse.md)
      - [如果后端死掉了，那么会出现一直卡死在这里](./kernel/fs/fuse/fuse-virtiofs.md)
      - [FUSE 机制梳理](./kernel/fs/fuse/fuse.codex.md)
      - [背景知识](./kernel/fs/fuse/fuse.md)
    - `nfs/`
      - [nfs rfc](./kernel/fs/nfs/doc.md)
      - [fscache](./kernel/fs/nfs/fscache.md)
      - [为什么 nfs 需要对于 swap 特殊支持](./kernel/fs/nfs/nfs-deadlock.md)
      - [收集一点 nfs 的常识](./kernel/fs/nfs/overview.md)
      - [windows 作为 samba server](./kernel/fs/nfs/samba-windows.md)
      - [linux 作为 samba server](./kernel/fs/nfs/samba.md)
    - `simplefs/`
      - [我用 ai 写了一个文件系统](./kernel/fs/simplefs/README.md)
    - [autofs 的作用](./kernel/fs/autofs.md)
    - [bcachefs](./kernel/fs/bcachefs.md)
    - [binfmt_elf.c](./kernel/fs/binfmt_elf.md)
    - [Readings](./kernel/fs/btr.md)
    - [buffer.c](./kernel/fs/buffer-head.md)
    - [dax](./kernel/fs/dax.md)
    - [VFS 文档](./kernel/fs/doc.md)
    - [cp vs mv 替换运行中二进制](./kernel/fs/etxtbsy.md)
    - [fsfreeze](./kernel/fs/freeze.md)
    - [fsnotify](./kernel/fs/fsnotify.md)
    - [文件系统功能分类](./kernel/fs/function-checklist.md)
    - [inode](./kernel/fs/inode.md)
    - [fs/iomap](./kernel/fs/iomap.md)
    - [iops](./kernel/fs/iops.md)
    - [fs 的 lock 设计](./kernel/fs/lock.md)
    - [mount](./kernel/fs/mount.md)
    - [关键源码位置](./kernel/fs/namei.md)
    - [基本 io 流程](./kernel/fs/overlay.md)
    - [pipe](./kernel/fs/pipe.md)
    - [relay fs](./kernel/fs/relay.md)
    - [configfs](./kernel/fs/virtual-configfs.md)
    - [kernfs](./kernel/fs/virtual-kernfs.md)
    - [总结一下各种虚拟文件系统](./kernel/fs/virtual.md)
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
  - `hp/`
    - [CPU hotplug](./kernel/hp/cpu.md)
    - [hotplug 概述](./kernel/hp/hotplug.md)
    - [memory hotplug](./kernel/hp/memory.md)
    - [qemu 的热插内存居然可以是不同的后端类型的](./kernel/hp/qemu.md)
    - [存储的热插拔](./kernel/hp/storage-hotplug.md)
  - `iouring/`
    - `async/`
      - [buffer io](./kernel/iouring/async/aio-buffer-io.md)
      - [基础](./kernel/iouring/async/basic.md)
      - [buffered write](./kernel/iouring/async/buffer-write.md)
      - [epoll](./kernel/iouring/async/epoll.md)
    - [aio](./kernel/iouring/aio.md)
    - [bpf](./kernel/iouring/bpf.md)
    - [iouring 的 cancel 设计](./kernel/iouring/cancel.md)
    - [文档](./kernel/iouring/doc.md)
    - [一个问题考虑的角度](./kernel/iouring/ecosystem.md)
    - [实现分析](./kernel/iouring/internal.md)
    - [这个文档差不多成熟了，已经很接近了](./kernel/iouring/iopoll.md)
    - [仔细 review 其中的同步机制](./kernel/iouring/lock.md)
    - [iouring msg ring](./kernel/iouring/msg-ring.md)
    - [iouring multishot](./kernel/iouring/multishot.md)
    - [net](./kernel/iouring/net.md)
    - [iouring 的 queue 同步](./kernel/iouring/queue.md)
    - [iouring register buffers](./kernel/iouring/register-buf.md)
    - [iouring register fds](./kernel/iouring/register-fd.md)
    - [安全](./kernel/iouring/security.md)
    - [iouring 内核和用户态如何共享内存](./kernel/iouring/share.md)
    - [iouring 对于 signal 的改造](./kernel/iouring/signal.md)
    - [基本的使用](./kernel/iouring/ublk.md)
    - [io uring 版本迭代](./kernel/iouring/version.md)
    - [workqueue](./kernel/iouring/wq.md)
    - [iouring 杂记](./kernel/iouring/yes.md)
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
      - [触发的 hi 和 tasklet 的](./kernel/irq/softirq/1-tools.md)
      - [原来 irq work 的调用来自于这里](./kernel/irq/softirq/irqwork.md)
      - [softirq](./kernel/irq/softirq/softirq.md)
      - [tasklet](./kernel/irq/softirq/tasklet.md)
      - [workqueue](./kernel/irq/softirq/workqueue.md)
    - [不看代码，先操作一下](./kernel/irq/1-tools.md)
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
    - [nmi](./kernel/irq/nmi.md)
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
    - `slub/`
      - [slub 调试记录](./kernel/mm/slub/debug.md)
      - [slub](./kernel/mm/slub/overview.md)
      - [Slub TID](./kernel/mm/slub/tid.md)
      - [Slub Tools](./kernel/mm/slub/tools.md)
    - [Idle Page Tracking](./kernel/mm/idle-page-tracking.md)
    - [madvise](./kernel/mm/mm-advise.md)
    - [Backing Device](./kernel/mm/mm-backing-dev.md)
    - [Buddy System](./kernel/mm/mm-buddy.md)
    - [CMA](./kernel/mm/mm-cma.md)
    - [Memory Compaction](./kernel/mm/mm-compaction.md)
    - [Copy-On-Write](./kernel/mm/mm-cow.md)
    - [DAMO](./kernel/mm/mm-damon-damo.md)
    - [DAMON Lab 3](./kernel/mm/mm-damon-lab3.md)
    - [DAMON](./kernel/mm/mm-damon.md)
    - [Page Poisoning](./kernel/mm/mm-debug.md)
    - [Memory Failure](./kernel/mm/mm-failure.md)
    - [Page Cache](./kernel/mm/mm-filemap.md)
    - [Fixmap](./kernel/mm/mm-fixmap.md)
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
    - [Page Table Flags](./kernel/mm/mm-pgtable.md)
    - [Page Poisoning](./kernel/mm/mm-poison.md)
    - [Readahead](./kernel/mm/mm-readahead.md)
    - [Page Refcount](./kernel/mm/mm-refcount.md)
    - [Reverse Mapping](./kernel/mm/mm-rmap.md)
    - [shmem](./kernel/mm/mm-shmem.md)
    - [Kernel Stack](./kernel/mm/mm-stack.md)
    - [Transparent Huge Pages](./kernel/mm/mm-thp.md)
    - [Memory Tiering](./kernel/mm/mm-tier.md)
    - [Thread-Local Storage](./kernel/mm/mm-tls.md)
    - [tmpfs](./kernel/mm/mm-tmpfs.md)
    - [Memory Tracepoints](./kernel/mm/mm-tracepoint.md)
    - [Userfaultfd](./kernel/mm/mm-userfault.md)
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
    - [Virtio Balloon Debug](./kernel/mm/virtio-balloon-debug.md)
    - [Virtio Balloon](./kernel/mm/virtio-balloon-kernel.md)
    - [Virtio Balloon (QEMU)](./kernel/mm/virtio-balloon-qemu.md)
    - [folio_clear_swapbacked 和 __folio_clear_swapbacked 区别是什么?](./kernel/mm/yes.md)
  - `module/`
    - [内核模块](./kernel/module/README.md)
  - `ospm/`
    - [2025](./kernel/ospm/2025.md)
  - `perf/`
    - [经典 flamegraph 流程](./kernel/perf/io.md)
    - [分析 x86_emulate_instruction 大致路径](./kernel/perf/kvm.md)
    - [rsync](./kernel/perf/net.md)
    - [在 QEMU 中测试 nvme : 观察 nvme 的写入和中断到了哪里](./kernel/perf/qemu.md)
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
    - [selinux 到底是什么个原理](./kernel/security/security.md)
    - [偶尔发现自己构建的内核在 qemu 中启动存在如下报错](./kernel/security/selinux.md)
    - [spectre](./kernel/security/spectre.md)
    - [一个小的 fix](./kernel/security/yama.md)
  - `signal/`
    - [man signal(7)](./kernel/signal/doc.md)
    - [gdb 和 signal fd 的诡异故事](./kernel/signal/gdb-signal.md)
    - [jobctl](./kernel/signal/jobctl.md)
    - [调试方法和有趣的实验](./kernel/signal/lab.md)
    - [kernel/signal.md](./kernel/signal/overview.md)
    - [signal pending](./kernel/signal/signal-pending.md)
    - [基本使用](./kernel/signal/signalfd.md)
    - [为什么有 sigpipe ?](./kernel/signal/sigpipe.md)
    - [syscall restart 机制解析与笔记勘误](./kernel/signal/syscall-restart-analysis.md)
    - [syscall restart](./kernel/signal/syscall-restart.md)
  - `sriov/`
    - [切分测试](./kernel/sriov/sriov.lab.md)
    - [https://learn.microsoft.com/en-us/windows-hardware/drivers/network/overview-of-single-root-i-o-virtualization--sr-iov-](./kernel/sriov/sriov.md)
    - [支持 SR-IOV 的 NVMe 设备概述（2025 年 11 月数据）](./kernel/sriov/sriov.nvme.md)
  - `swap/`
    - [4](./kernel/swap/fj.md)
    - [简单分析一下 folio 在 lru 中移动](./kernel/swap/folio_add_lru.md)
    - [备忘](./kernel/swap/lruvec.md)
    - [page-io.c](./kernel/swap/page-io.md)
    - [iouring swap](./kernel/swap/rswap.md)
    - [swap 模块基本分析](./kernel/swap/swap-overview.md)
    - [swap.c 分析](./kernel/swap/swap.md)
    - [理解下这个变化](./kernel/swap/swapcache.md)
    - [swapfile.c](./kernel/swap/swapfile.md)
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
  - `tty/`
    - [bmc](./kernel/tty/bmc.md)
    - [console](./kernel/tty/console.md)
    - [getty](./kernel/tty/getty.md)
    - [安装虚拟机的时候，ovmf stdio 也有安装界面](./kernel/tty/lab.md)
    - [Linux Device Driver : TTY Drivers](./kernel/tty/ldd-chapter-18.md)
    - [Linux TTY 子系统深入分析](./kernel/tty/linux-tty-analysis.md)
    - [login](./kernel/tty/login.md)
    - [pts](./kernel/tty/pts.md)
    - [pty driver](./kernel/tty/pty-driver.md)
    - [putty](./kernel/tty/putty.md)
    - [qemu](./kernel/tty/qemu.md)
    - [drivers/input/serio/i8042.c 这个是做什么的?](./kernel/tty/serial.md)
    - [ssh 的操作会过 tty 机制吗?](./kernel/tty/ssh.md)
    - [sysfs](./kernel/tty/sysfs.md)
    - [sysrq](./kernel/tty/sysrq.md)
    - [termios](./kernel/tty/termios.md)
    - [tty driver](./kernel/tty/tty-driver.md)
    - [tty 到底是什么](./kernel/tty/tty.md)
    - [vt](./kernel/tty/vt.md)
    - [tty0](./kernel/tty/yes.md)
  - `tutorial/`
    - `crash/`
      - [实战材料](./kernel/tutorial/crash/case.md)
      - [crash utility](./kernel/tutorial/crash/crash.md)
      - [crash 内核实现](./kernel/tutorial/crash/internal.md)
      - [kdump](./kernel/tutorial/crash/kdump.md)
      - [kexec](./kernel/tutorial/crash/kexec.md)
      - [panic 的参数配置](./kernel/tutorial/crash/options.md)
      - [pstore 如何使用](./kernel/tutorial/crash/pstore.md)
    - `drgn/`
      - [使用 drgn 来学习内核](./kernel/tutorial/drgn/drgn.md)
    - `initramfs/`
      - [buildroot](./kernel/tutorial/initramfs/builtroot.md)
      - [dracut](./kernel/tutorial/initramfs/dracut.md)
      - [initfs](./kernel/tutorial/initramfs/initramfs.md)
      - [initramfs : iso](./kernel/tutorial/initramfs/iso.md)
      - [linuxfromscratch](./kernel/tutorial/initramfs/minimal.md)
      - [bootc](./kernel/tutorial/initramfs/yes.md)
    - [和社区沟通](./kernel/tutorial/community.md)
    - [使用 kernel 中调试工具](./kernel/tutorial/debug.md)
    - [DKMS](./kernel/tutorial/dkms.md)
    - [dmesg 的基本使用](./kernel/tutorial/dmesg.md)
    - [阅读文档](./kernel/tutorial/doc.md)
    - [先收集起来](./kernel/tutorial/format-verification.md)
    - [fuzz](./kernel/tutorial/fuzz.md)
    - [gdb kernel 的常用命令](./kernel/tutorial/gdb-kernel.md)
    - [git](./kernel/tutorial/git.md)
    - [kcov](./kernel/tutorial/kcov.md)
    - [kgdb](./kernel/tutorial/kgdb.md)
    - [Linux kernel Labs 笔记](./kernel/tutorial/linux-kernel-labs.md)
    - [杂谈](./kernel/tutorial/misc.md)
    - [prepare](./kernel/tutorial/prepare.md)
    - [proxmox 基本使用](./kernel/tutorial/proxmox.md)
    - [sparse && smatch](./kernel/tutorial/sparse.md)
    - [Linux 测试](./kernel/tutorial/test.md)
    - [ubsan](./kernel/tutorial/ubsan.md)
    - [uml](./kernel/tutorial/uml.md)
    - [记录一些极其奇怪的问题](./kernel/tutorial/wired.md)
  - `usb/`
    - [qemu 模型](./kernel/usb/usb-qemu.md)
    - [usb 分析记录记录](./kernel/usb/usb.md)
  - `vfio/`
    - [内核文档](./kernel/vfio/doc.md)
    - [看看这个 ACS override 是什么鬼?](./kernel/vfio/group.md)
    - [PWN : Posted MSI notification event](./kernel/vfio/int-posted-msi.md)
    - [vfio 对外提供的三个 fops](./kernel/vfio/internal-kernel.md)
    - [qemu](./kernel/vfio/internal-qemu.md)
    - [mdev-no-iommu](./kernel/vfio/mdev-no-iommu.md)
    - [vfio misc](./kernel/vfio/misc.md)
    - [noiommu](./kernel/vfio/noiommu.md)
    - [uio](./kernel/vfio/uio.md)
    - [nvgrace-gpu](./kernel/vfio/vGPU.md)
    - [vfio 基础](./kernel/vfio/vfio.md)
    - [drivers/vfio/pci/virtio 是做什么的](./kernel/vfio/virtio.md)
  - `vhost/`
    - [vhost-user Inflight I/O Tracking 详解](./kernel/vhost/1-inflight-io.md)
    - [vhost 热迁移](./kernel/vhost/2-migration.md)
    - [vhost gpu 的实现](./kernel/vhost/gpu.md)
    - [vhost iotlb](./kernel/vhost/iommu.md)
    - [vhost 协议的定义](./kernel/vhost/qemu.md)
    - [redhat blog for vhost](./kernel/vhost/redhat-blog.md)
    - [vdpa](./kernel/vhost/vdpa.md)
    - [vduse](./kernel/vhost/vduse.md)
    - [vhost-scsi 以及 vhost-user-scsi](./kernel/vhost/vhost-scsi.md)
    - [vhost 协议基本分析](./kernel/vhost/vhost.md)
  - `xdc/`
    - [xdc](./kernel/xdc/2025.md)
  - [分析一些基本问题](./kernel/aarch64-basic.md)
  - [原来 qemu 可以指定 gic 版本](./kernel/aarch64-gic.md)
  - [收集经典 backtrace](./kernel/backtrace.md)
  - [rfkill](./kernel/bluetooth.md)
  - [AMBA](./kernel/bus-axi.md)
  - [nvlink](./kernel/bus-nvlink.md)
  - [psi](./kernel/bus-spi.md)
  - [tilelink](./kernel/bus-tilelink.md)
  - [bus](./kernel/bus.md)
  - [CFI in kernel](./kernel/cfi.md)
  - [中国 Linux 大会记录](./kernel/clk.md)
  - [谈谈 kernel 构建的基本技术](./kernel/compiler.md)
  - [对 swap device 错误注入](./kernel/fault-inject-swap.md)
  - [错误注入](./kernel/fault-inject.md)
  - [futex](./kernel/futex.md)
  - [hid](./kernel/hid.md)
  - [基本启动流程](./kernel/init.md)
  - [总结常用的 kernel cmdline](./kernel/kernel-parameters.md)
  - [内核学习经验 : 进阶版](./kernel/learn-linux-kernel-v2.md)
  - [livepatch](./kernel/livepatch.md)
  - [dracut 这个警告有意思](./kernel/microcode.md)
  - [杂记](./kernel/misc.md)
  - [namespace](./kernel/namespace.md)
  - [notifier](./kernel/notifier.md)
  - [resource](./kernel/resource.md)
  - [rlimit](./kernel/rlimit.md)
  - [rng](./kernel/rng.md)
  - [sel4](./kernel/sel4.md)
  - [内核屎山评选](./kernel/shit-kernel.md)
  - [QEMU 屎山评选](./kernel/shit-qemu.md)
  - [sound](./kernel/sound.md)
  - [sysv ipc](./kernel/sysv-ipc.md)
  - [命令缩写](./kernel/tools-abbr.md)
  - [为什么 Linux Kernel 的代码质量比 QEMU 更高](./kernel/why-better-than-qemu.md)
  - [guest_memfd](./kernel/why-so-many-fd.md)
- `kr/`
  - [2024](./kr/2024.md)
  - [kernel-recipes 2025](./kr/2025.md)
- `kvm/`
  - `aarch64/`
    - `sys_regs/`
      - [似乎关联的源码](./kvm/aarch64/sys_regs/encode.md)
      - [sys_reg_descs 的 filter 功能](./kvm/aarch64/sys_regs/feature.md)
      - [code overview](./kvm/aarch64/sys_regs/lab.md)
      - [qemu 如何管理 cpreg](./kvm/aarch64/sys_regs/qemu.md)
    - [ARM KVM 的大致代码流程](./kvm/aarch64/README.md)
  - `features/`
    - [kvm feautres](./kvm/features/kvm-features.md)
    - [pv eoi](./kvm/features/pv-eoi.md)
    - [PV_SCHED_YIELD](./kvm/features/pv-sched-yield.md)
    - [虚拟化下的 spin lock](./kvm/features/pv-spinlock.md)
    - [PV_TLB_FLUSH](./kvm/features/pv-tlb-flush.md)
    - [kvm_emulate_hypercall -> __kvm_emulate_hypercall](./kvm/features/readme.md)
    - [KVM_FEATURE_STEAL_TIME](./kvm/features/steal-time.md)
    - [代码](./kvm/features/vcpu-stall.md)
  - `fun/`
    - [记录调试 kvm 的一个有趣问题](./kvm/fun/host-freq.md)
  - `hyperv/`
    - [Hyperv Enlightment](./kvm/hyperv/hyperv-pv.md)
    - [HyperV](./kvm/hyperv/hyperv.md)
    - [hyperv 中运行 Linux](./kvm/hyperv/in-hyperv-manager.md)
    - [hypev 基本使用](./kvm/hyperv/usage.md)
  - `hypervisor/`
    - [Cloud Hypervisor](./kvm/hypervisor/cloud-hypervisor.md)
    - [stratovirt](./kvm/hypervisor/stratovirt.md)
  - `kvm-forum/`
    - [2016](./kvm/kvm-forum/2016.md)
    - [2017](./kvm/kvm-forum/2017.md)
    - [2018](./kvm/kvm-forum/2018.md)
    - [2020](./kvm/kvm-forum/2020.md)
    - [2021](./kvm/kvm-forum/2021.md)
    - [2022](./kvm/kvm-forum/2022.md)
    - [2023](./kvm/kvm-forum/2023.md)
    - [2024](./kvm/kvm-forum/2024.md)
    - [2025](./kvm/kvm-forum/2025.md)
    - [misc](./kvm/kvm-forum/misc.md)
  - `mmu/`
    - [L1 中观测到 kvm_set_pfn_dirty](./kvm/mmu/ad.md)
    - [`async_pf`](./kvm/mmu/async-pf.md)
    - [EXIT_REASON_EPT_VIOLATION vs EXIT_REASON_EPT_MISCONFIG](./kvm/mmu/basic.md)
    - [ept 格式的定义在哪里呢?](./kvm/mmu/ept.md)
    - [分析这个](./kvm/mmu/guest-memfd.md)
    - [kvm 为什么需要特殊处理 hugepage](./kvm/mmu/hugepage.md)
    - [TODO](./kvm/mmu/mmu.md)
    - [Documentation/virt/kvm/x86/mmu.rst](./kvm/mmu/mmu.rst.md)
    - [kvm_vcpu_arch 中的 5 个 MMU 的含义](./kvm/mmu/nested.md)
    - [kvm mmu notifier](./kvm/mmu/notifier.md)
    - [kvm track mode](./kvm/mmu/page-track.md)
    - [为什么需要 arch/x86/kvm/mmu/paging_tmpl.h 来处理各种情况](./kvm/mmu/paging_tmpl.md)
    - [PDPTR 是什么？](./kvm/mmu/pdptr.md)
    - [参考资料](./kvm/mmu/rmap.md)
    - [for_each_shadow_entry](./kvm/mmu/shadow-page.md)
    - [tdp_mmu](./kvm/mmu/tdp_mmu.md)
    - [KVM TLB Flush 机制分析](./kvm/mmu/tlb-flush-draft.md)
    - [kvm_mmu_invalidate_addr 是唯一的入口](./kvm/mmu/tlb-flush-virt.md)
    - [tlb flush 的基本原理](./kvm/mmu/tlb-flush.md)
  - `nested/`
    - [aarch64](./kvm/nested/aarch64.md)
    - [如何实现无穷级嵌套](./kvm/nested/nested-l3.md)
    - [kvm 嵌套虚拟化](./kvm/nested/nested.md)
    - [svm](./kvm/nested/svm.md)
    - [vmx](./kvm/nested/vmx.md)
  - `svm/`
    - [其中部分内容分析到](./kvm/svm/avic.md)
    - [sev](./kvm/svm/sev.md)
    - [简单浏览下 svm.c 的代码](./kvm/svm/svm.md)
  - [lab](./kvm/8254.md)
  - [复杂啊 : 看看 reference 的位置!](./kvm/cache-regs.md)
  - [cr0](./kvm/cr.md)
  - [基本内容](./kvm/debugfs.md)
  - [到底什么时候需要 emulate](./kvm/emulate.md)
  - [SGX](./kvm/enclave.md)
  - [event injection](./kvm/event-delivery.md)
  - [将所有的 exit reason 都整理下](./kvm/exit-reason.md)
  - [什么是 FRED ?](./kvm/fred.md)
  - [kvm interrupt window 到底在说什么](./kvm/interrupt-window.md)
  - [KVM](./kvm/kvm.md)
  - [通过 kvm_vfio_ops 来理解 kvm_device_ops](./kvm/kvm_device_ops.md)
  - [问题](./kvm/kvm_x86_ops.md)
  - [KVM Lock Overview](./kvm/lock.md)
  - [使用 tracepoint 来跟踪 kvm_check_request](./kvm/make_request.md)
  - [pfncache.c 以及其他的辅助函数](./kvm/map-cache.md)
  - [KVM_CAP_COALESCED_MMIO](./kvm/mmio.md)
  - [如何理解 msr exit 的优化?](./kvm/msr.md)
  - [mtrr](./kvm/mtrr.md)
  - [ple windo](./kvm/ple.md)
  - [qemu 如何支持 kvm 的](./kvm/qemu.md)
  - [secure](./kvm/secure.md)
  - [kvm selftests](./kvm/selftests.md)
  - [kvm 如何支持 smm](./kvm/smm.md)
  - [记录几个相见恨晚的 tracepoint 点](./kvm/tracepoint.md)
  - [Intel VMCS 字段表](./kvm/vmcs-fields.md)
  - [记录下 x86.c 的内容](./kvm/x86.md)
  - [SVM_EXIT_TASK_SWITCH 和 EXIT_REASON_TASK_SWITCH](./kvm/yes-we-know.md)
- `linux/`
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
      - [Pseudoterminals](./linux/tlpi/6/tlpi-chapter-64.md)
    - [为 tlpi 的可执行文件添加.out 扩展名](./linux/tlpi/change-extension.md)
  - [Makefile](./linux/Makefile.md)
  - [有趣的](./linux/android.md)
  - [ansible 记录](./linux/ansible.md)
  - [基本记录](./linux/fedora.md)
  - [omarchy](./linux/omarchy.md)
  - [如何给 OpenEuler 提交打包](./linux/openeuler.md)
  - [ubuntu 使用的问题合集](./linux/ubuntu.md)
- `math/`
  - [auto.dr.can](./math/auto.dr.can.md)
  - [自动化控制理论](./math/auto.md)
  - [这两个都都是够了的](./math/linear-algebra.md)
  - [maxwell 方程的解释](./math/maxwell.md)
  - [为什么我决定重新学校数学了](./math/route.md)
- `net/`
  - `demo/`
    - [macvlan & ipvlan 网络虚拟化 Demo](./net/demo/README.md)
  - `geneve/`
    - [GENEVE 隧道实验](./net/geneve/README.md)
  - `kernel/`
    - `core/`
      - [overview](./net/kernel/core/overview.md)
    - `ipv4/`
      - [af_net.c](./net/kernel/ipv4/overview.md)
      - [IP Fragmentation and Reassembly](./net/kernel/ipv4/reassemb.md)
    - `mac80211/`
      - [wiki](./net/kernel/mac80211/mac80211-overview.md)
    - `sched/`
      - [sched](./net/kernel/sched/sched-overview.md)
    - [记录一下网络栈的一些源码分析](./net/kernel/kernel-src-overview.md)
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
    - [cm](./net/rdma/cm.md)
    - [mmap 的观测](./net/rdma/dma-and-int.md)
    - [doc](./net/rdma/doc.md)
    - [rdma 环境准备](./net/rdma/lab.md)
    - [qemu](./net/rdma/misc.md)
    - [MLX4 和 MLX5 的区别](./net/rdma/mlnx.md)
    - [RDMA](./net/rdma/overview.md)
    - [RDMA Core 详解](./net/rdma/rdma-core.md)
    - [RDMA 杂谈](./net/rdma/rdma-insights.md)
    - [RDMA 网卡配置指南](./net/rdma/rdma-setup.md)
    - [smc](./net/rdma/smc-r.md)
    - [rdma 常用工具](./net/rdma/tools.md)
  - `sfc/`
    - [sfc](./net/sfc/basic.md)
  - `vxlan-demo/`
    - [VXLAN](./net/vxlan-demo/README.md)
  - [linux/net/9p 和 linux/fs/9p 这两个文件夹是什么关系](./net/9p.md)
  - [bgp](./net/bgp.md)
  - [bonding](./net/bonding.md)
  - [bridge](./net/bridge.md)
  - [赛博活佛](./net/cloudflare.md)
  - [dhcp client 的基本操作](./net/dhcp.md)
  - [如何理解这个这几个配置?](./net/diag.md)
  - [DNS](./net/dns.md)
  - [BlueField-3](./net/dpu.md)
  - [背景介绍](./net/erspan.md)
  - [geneve](./net/geneve.md)
  - [gro](./net/gro.md)
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
  - [net zero copy](./net/msg_zerocopy.md)
  - [backlog](./net/napi.md)
  - [neighbour](./net/neighbour.md)
  - [网络基本配置](./net/net-config.md)
  - [网络问题常见排查思路](./net/net-debug.md)
  - [Linux kernel network stack lock](./net/net-lock.md)
  - [loopback 网卡](./net/net-loopback.md)
  - [mlx5](./net/net-mlx5.md)
  - [网络的 namespace](./net/net-namespace.md)
  - [net-phy](./net/net-phy.md)
  - [Network Route](./net/net-route.md)
  - [rpc](./net/net-rpc.md)
  - [net-sendfile](./net/net-sendfile.md)
  - [network timestamping](./net/net-timestamping.md)
  - [Network tools internals](./net/net-tools.md)
  - [vsock](./net/net-vsock.md)
  - [linux 网络基础查漏补缺](./net/net.md)
  - [netconsole](./net/netconsole.md)
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
  - [关于 clash 代理的两个基本问题](./net/proxy.md)
  - [qdisc](./net/qdisc.md)
  - [quic](./net/quic.md)
  - [raw socket](./net/raw-socket.md)
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
  - [客户端部署](./net/tailscale.md)
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
  - [qemu 中就是支持 udmabuf 的](./pci/udmabuf.md)
- `qemu/`
  - `aarch64-softmmu/`
    - [一些记录](./qemu/aarch64-softmmu/Readme.md)
  - `aarch64-user/`
    - [一些细节的说明](./qemu/aarch64-user/Readme.md)
  - `bios/`
    - [QEMU 中的 seabios : 地址空间](./qemu/bios/bios-memory.md)
    - [如何调试 seabios](./qemu/bios/debug.md)
    - [QEMU 中的 seabios : fw_cfg](./qemu/bios/fw_cfg.md)
    - [QEMU 如何加载 Linux kernel image](./qemu/bios/load-kernel-image.md)
    - [qboot](./qemu/bios/qboot.md)
    - [seabios](./qemu/bios/seabios.md)
    - [smbios](./qemu/bios/smbios.md)
  - `block/`
    - [block](./qemu/block/block.md)
    - [https://www.linux-kvm.org/images/b/b5/2012-fourm-block-overview.pdf](./qemu/block/doc.md)
    - [libblkio](./qemu/block/libblkio.md)
    - [qemu-storage-daemon](./qemu/block/qsd.md)
  - `dev/`
    - [e1000 的工作原理](./qemu/dev/e1000-2.md)
    - [观测 e1000 驱动在 qemu 如何被模拟的](./qemu/dev/e1000.md)
    - [i8042 : 键盘](./qemu/dev/i8042.md)
    - [None pci device](./qemu/dev/misc.md)
  - `memory/`
    - [qemu memory backend](./qemu/memory/memory.backend.md)
    - [MemoryListener](./qemu/memory/memory.listener.md)
    - [QEMU 的 memory model](./qemu/memory/memory.md)
    - [RAMBlock ，从热迁移的角度分析](./qemu/memory/ram.md)
  - `migration/`
    - [migration 基本测试](./qemu/migration/0-lab.md)
    - [Level 1 : stop continue](./qemu/migration/1-stop-continue.md)
    - [Level 2 : savevm loadvm](./qemu/migration/1.2-savevm-loadvm.md)
    - [Level 3: local migration](./qemu/migration/2-cpr.md)
    - [问题](./qemu/migration/balloon.md)
    - [colo](./qemu/migration/colo.md)
    - [dirty rate](./qemu/migration/dirty.rate.md)
    - [先仔细阅读文档吧](./qemu/migration/doc.md)
    - [how to migrate with latest QEMU](./qemu/migration/libvirt.md)
    - [migration](./qemu/migration/migration.md)
    - [multifd](./qemu/migration/multifd.md)
    - [先把基本理念搞清楚吧](./qemu/migration/object.block.md)
    - [内存拷贝相关](./qemu/migration/object.ram.md)
    - [分析 rom 的热迁移行为](./qemu/migration/object.rom.md)
    - [vfio migration](./qemu/migration/object.vfio.md)
    - [virtio 有特殊的封装](./qemu/migration/object.virtio.md)
    - [这个是经典例子了吧](./qemu/migration/object.vmstate.md)
    - [post copy](./qemu/migration/postcopy.md)
    - [vhost](./qemu/migration/vhost.md)
    - [xbzrle](./qemu/migration/xbzrle.md)
    - [qemu 中 yank 的含义](./qemu/migration/yank.md)
    - [为什么热迁移后，使用 memfd 的后端会自动的 touch 所有的内存](./qemu/migration/zero-page.md)
  - `qom/`
    - [QEMU 的参数解析](./qemu/qom/options.md)
    - [qdev](./qemu/qom/qdev.md)
    - [QEMU 中的面向对象 : QOM](./qemu/qom/qom.md)
  - `tcg/`
    - [TCG](./qemu/tcg/core-loop.md)
    - [QEMU 中的 map 和 set](./qemu/tcg/map.md)
    - [mttcg](./qemu/tcg/mttcg.md)
    - [record / replay](./qemu/tcg/record-reply.md)
    - [QEMU softmmu 访存 helper 整理](./qemu/tcg/softmmu-functions.md)
    - [QEMU 的 softmmu 设计](./qemu/tcg/softmmu.md)
    - [TCGContext : 如何工作的，如何维护的，作用是什么](./qemu/tcg/tb.md)
    - [QEMU 二进制翻译基础](./qemu/tcg/tcg.md)
  - `thread/`
    - `glib/`
      - [glib](./qemu/thread/glib/readme.md)
    - [AioContext](./qemu/thread/aiocontext.md)
    - [qemu 中的 atomic 使用](./qemu/thread/atomic.md)
    - [aio_bh_poll 的作用](./qemu/thread/bh.md)
    - [Big QEMU Lock](./qemu/thread/bql.md)
    - [从 setjmp 到 coroutine](./qemu/thread/coroutine-baisc.md)
    - [coroutine](./qemu/thread/coroutine-qemu.md)
    - [util/defer-call.c](./qemu/thread/defer.md)
    - [doc](./qemu/thread/doc.md)
    - [FDMonOps](./qemu/thread/fdmon.md)
    - [Event Loop in glib](./qemu/thread/glib.md)
    - [block/graph-lock.c](./qemu/thread/graph-lock.md)
    - [QEMU 中的锁](./qemu/thread/lock.md)
    - [qemu lockcounters](./qemu/thread/lockcnt.md)
    - [QEMU Event Loop](./qemu/thread/main-loop.md)
    - [qemu rcu](./qemu/thread/rcu.md)
    - [qemu 的 thread pool 的作用](./qemu/thread/thread-pool.md)
    - [qemu 到底有那些 thread](./qemu/thread/threads.md)
    - [QEMU AIO 事件循环架构分析](./qemu/thread/why-glib.md)
    - [QEMU `AioContext` 与 GLib 连接机制](./qemu/thread/why-glib2.md)
    - [qemu 中的 aio 的工作机制](./qemu/thread/yes.md)
  - [alpine iso 可以直接启动使用](./qemu/basic.md)
  - [如何给 qemu 配置 cdrom](./qemu/cdrom.md)
  - [QEMU 的挑战者](./qemu/challenger.md)
  - [分析 QEMU 的每一个模块和演化过程](./qemu/changelog.md)
  - [qemu io/ 目录中的功能](./qemu/channel.md)
  - [如何正确的配置 qemu 的 memory 和 cpu](./qemu/cpu-topo.md)
  - [经典例子](./qemu/error.md)
  - [QEMU 启动代码](./qemu/init-2.md)
  - [设置环境变量方便编译示例](./qemu/libkrun-analysis.md)
  - [libkrun](./qemu/libkrun.md)
  - [microvm](./qemu/microvm.md)
  - [qemu 中关于 page size 问题的合集](./qemu/page-size.md)
  - [Official Docs](./qemu/qemu-docs.md)
  - [qemu 如何做测试的](./qemu/qemu-test.md)
  - [qmp 和 hmp](./qemu/qmp-hmp.md)
  - [multi-process qemu](./qemu/remote.md)
  - [slirp](./qemu/slirp.md)
  - [QEMU 中的 trace 机制](./qemu/trace.md)
  - [为什么 qemu 这么复杂](./qemu/why-so-complex.md)
  - [代码分析](./qemu/wolf-book.md)
- `rust/`
  - `demo/`
    - [基本执行操作](./rust/demo/README.md)
  - `rust/`
    - [Learning Rust With Entirely Too Many Linked Lists](./rust/rust/linked-list.md)
    - [rust 中的 macro](./rust/rust/macro.md)
    - [Ownership and lifetime](./rust/rust/ownership-lifetime.md)
    - [Resource](./rust/rust/rust-route.md)
    - [The Rust Book](./rust/rust/the-Rust-book.md)
  - [rust async](./rust/async.md)
  - [收集一些和 rust 有关的项目](./rust/kernel.md)
  - [low-priority](./rust/low-priority.md)
  - [Rust os 实现](./rust/os.md)
  - [教程](./rust/rust.md)
  - [收集一些必须 rust 来解决的](./rust/rust.target.md)
  - [libuv 和两个 tokio](./rust/tokio.md)
  - [写 rust 的工具](./rust/tools.md)
  - [先搞清楚基本问题](./rust/unsafe.md)
- `shell/`
  - [awk](./shell/awk.md)
  - [如何彻底征服 bash script](./shell/bash.md)
  - [shell 常用命令](./shell/basic.md)
  - [find 命令](./shell/find.md)
  - [grep 基本使用](./shell/grep.md)
  - [问题](./shell/nu.md)
  - [Regex](./shell/regex.md)
  - [ripgrep](./shell/rg.md)
  - [sed](./shell/sed.md)
  - [如何战胜 bash](./shell/systhesis.md)
  - [unix 文本处理](./shell/text.md)
- `tools/`
  - [构建系统](./tools/build.md)
  - [Gerrit](./tools/gerrit.md)
  - [git](./tools/git.md)
  - [github](./tools/github.md)
  - [jinkens](./tools/jenkins.md)
  - [各种小工具](./tools/misc-tools.md)
  - [mutt](./tools/mutt.md)
  - [RPM](./tools/rpm.md)
  - [typst 工具](./tools/typst.md)
  - [how to debug neovim](./tools/vimrc.md)
  - [vscode 的调试环境](./tools/vscode.md)
- `trace/`
  - `bpftime/`
    - [bpftime](./trace/bpftime/basic.md)
  - `ebpf/`
    - `bcc/`
      - [如果使用 bpf 来调试，那么就是为这个目录](./trace/ebpf/bcc/readme.md)
    - `cilium/`
      - [做做这里的教程](./trace/ebpf/cilium/README.md)
    - `ra/`
      - [readme](./trace/ebpf/ra/readme.md)
    - [基本使用方法](./trace/ebpf/README.md)
  - `ebpf-doc/`
    - [arena](./trace/ebpf-doc/arena.md)
    - [写一个 bcc 和 bpftrace 使用对比](./trace/ebpf-doc/bcc-vs-bpftrace.md)
    - [bcc 的打包](./trace/ebpf-doc/bcc.md)
    - [bloom filter](./trace/ebpf-doc/bloom-filter.md)
    - [基本使用](./trace/ebpf-doc/bpftool.md)
    - [btf](./trace/ebpf-doc/btf.md)
    - [CO:RE](./trace/ebpf-doc/core.md)
    - [基本的代码分析](./trace/ebpf-doc/internal.md)
    - [bpf iterators](./trace/ebpf-doc/iter.md)
    - [这个居然意外的好懂](./trace/ebpf-doc/libbpf.md)
    - [需要搞的事情](./trace/ebpf-doc/overview.md)
    - [bysyscall](./trace/ebpf-doc/projects.md)
    - [STRUCT_OPS](./trace/ebpf-doc/struct_ops.md)
    - [bpf syscall 的基本观察](./trace/ebpf-doc/syscall.md)
    - [有趣，看来 verifier 还是很厉害的](./trace/ebpf-doc/verifier.md)
  - `ftrace/`
    - [原来 trace_pipe 会自动的清理掉 trace 中内容](./trace/ftrace/basic.md)
    - [eprobe - Event-based Probe Tracing](./trace/ftrace/eprobe.md)
    - [fprobe 机制](./trace/ftrace/fprobe.md)
    - [ftrace 实现](./trace/ftrace/ftrace-internals.md)
    - [ftrace 输出的格式](./trace/ftrace/ftrace.md)
    - [kernel-trace-files](./trace/ftrace/kernel-trace-files.md)
    - [latency-collector](./trace/ftrace/latency-collector.md)
    - [https://lwn.net/Articles/410200/](./trace/ftrace/trace-cmd.md)
    - [hwlat](./trace/ftrace/tracer-hwlat.md)
    - [osnoise](./trace/ftrace/tracer-osnoise.md)
  - `perf/`
    - [关于 perf 我知道的一切](./trace/perf/README.md)
  - `tools/`
    - `bpftrace/`
      - [./ip-change.bt (qwen)](./trace/tools/bpftrace/readme.md)
    - [计划和代办](./trace/tools/README.md)
  - [Linux Trace 技术整理报告](./trace/TRACE_INVENTORY.md)
  - [bpftrace](./trace/bpftrace.md)
  - [trace 相关的文档](./trace/doc.md)
  - [kallsyms_lookup_name](./trace/kallsyms.md)
  - [kprobe](./trace/kprobe.md)
  - [libtraceevent](./trace/libtraceevent.md)
  - [mce 的工作原理](./trace/mce.md)
  - [观测](./trace/monitor.md)
  - [valgrind](./trace/others.md)
  - [先不搞那些虚的东西，分析清楚下面这个问题](./trace/overview.md)
  - [pcm](./trace/pcm.md)
  - [strace 基本使用](./trace/strace.md)
  - [systemtap](./trace/systemtap.md)
  - [trace 传统工具](./trace/tools.md)
  - [arm 环境的确容易出现 backtrace 没有的情况](./trace/tracepoint-arm.md)
  - [tracepoint 的积累已经很多了](./trace/tracepoint.md)
  - [用户态符号基础](./trace/user.md)
  - [noinstr code](./trace/yes.md)
- `uefi/`
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
  - [acrn](./virt/acrn.md)
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
  - `driver/`
    - [windows 驱动开发](./windows/driver/windows-driver.md)
  - [windows 性能测试工具](./windows/benchmarks.md)
  - [dotnet](./windows/dotnet.md)
  - [windows 杂谈](./windows/misc.md)
  - [windows 网络](./windows/net.md)
  - [windows powershell 基本命令](./windows/pwsh.md)
  - [rdp](./windows/rdp.md)
  - [windows 环境搭建](./windows/setup-env.md)
  - [如何将 windows 放到虚拟机中](./windows/virt.md)
  - [关于 windows 的各种碎碎念](./windows/why.md)
  - [windows](./windows/windows-route.md)
  - [windows 下的基本编程](./windows/windows-via-cpp.md)
- `xen/`
  - [xen](./xen/README.md)
- [配套代码](./assembly.md)
- [酷炫](./cool.md)
- [记录一些 Linux kernel 中好玩的东西](./kernel-fun.md)
- [Collections](./readme.draft.md)
- [system programming project reading](./source-code-reading-list.md)
- [系统软件工程师的技能树](./sys-software-developer.md)
- [为什么我喜欢计算机](./why-learn-cs.md)
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
