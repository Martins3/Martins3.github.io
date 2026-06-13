# ARM KVM 的大致代码流程


- [天高气爽阅码疾：一日看尽虚拟化](https://mp.weixin.qq.com/s/CWqUagksabj4kDFQhTlgUA)
- [天高气爽阅码疾：一日看尽虚拟化](https://mp.weixin.qq.com/s/1gA4vRntnWrJjADsMEa7tA)
讲的很细节，作为大致的参考吧

## 很难的哇
- https://systems.cs.columbia.edu/projects/kvm-arm/
- https://www.cnblogs.com/LoyenWang/tag/%E8%99%9A%E6%8B%9F%E5%8C%96/ : LoyenWang 讲解的虚拟化是基于 ARM 的
- https://lists.cs.columbia.edu/pipermail/kvmarm/2020-July/041733.html ：对于 ARM 最好的总结

## 资源
- [ ] https://www.usenix.org/system/files/conference/atc17/atc17-dall.pdf
- [ ] https://calinyara.github.io/technology/2019/11/03/armv8-virtualization.html
- [ ] https://openeuler.org/zh/blog/yorifang/2020-10-24-arm-virtualization-overview.html


## [内存虚拟化](https://www.cnblogs.com/LoyenWang/p/13943005.html)

## [中断虚拟化](https://www.cnblogs.com/LoyenWang/p/14017052.html)
整个 kvm 只有 17000 行，其中 kvm/vgic 下有 7000 行

- [ ] 分析的相当不错，但是只能理解其中部分内容

## 简单跟踪一下其中的源码

### 找到 exit 的流程

- kvm_arch_vcpu_ioctl_run
  - handle_exit ：
    - ARM_EXCEPTION_TRAP : handle_trap_exceptions
      - kvm_get_exit_handler
        - kvm_vcpu_get_esr
      - kvm_handle_guest_abort : guest page fault 的位置
        - handle_access_fault : 从这里开始是我们熟悉的内容

在 handle_exit 中引用的几个内容为，他们是什么含义，为什么在 handle_exit 中处于几乎无需处理的状态:
```c
#define ARM_EXCEPTION_IRQ     0
#define ARM_EXCEPTION_EL1_SERROR  1
#define ARM_EXCEPTION_TRAP    2
#define ARM_EXCEPTION_IL      3
```

- [ ] arm_exit_handlers ：这就是全部的 exit handler 吗?

### 将 arm_exit_handlers 中，找到对应的手册

D13.2.37 中

### 查看一下 kvm_vcpu_arch 中的内容

- kvm_vcpu_fault_info

## ARM 的 SERROR
- http://happyseeker.github.io/kernel/2016/03/03/about-system-error-in-AArach64.html
- https://developer.arm.com/documentation/102412/0102/Exception-types

## SMC : Secure Monitor Call
- https://stackoverflow.com/questions/59217906/how-to-use-an-arm-secure-monitor-call-smc


## ARM exception
- http://osnet.cs.nchu.edu.tw/powpoint/Embedded94_1/Chapter%207%20ARM%20Exceptions.pdf
- LoyenWang 是分析过 ARM 的中断的过程的哇

## ARM data abort
- https://developer.arm.com/documentation/ddi0406/b/System-Level-Architecture/The-System-Level-Programmers--Model/Exceptions/Data-Abort-exception

## ARM page walking

### TTBR 和 TTBCR 基础知识
- https://stackoverflow.com/questions/14460752/linux-kernel-arm-translation-table-base-ttb0-and-ttb1

TTBR0 用于存放用户空间的一级页表基址，TTBR1 存放内核空间的一级页表基址。

## ARM Address Space
- 官方文档，总结非常到位的: https://developer.arm.com/documentation/101811/0102/Address-spaces


## 分析一下 ARM 的 exception

首先在这里定义的:
```S
SYM_CODE_START(vectors)
    kernel_ventry   1, t, 64, sync      // Synchronous EL1t
    kernel_ventry   1, t, 64, irq       // IRQ EL1t
    kernel_ventry   1, t, 64, fiq       // FIQ EL1h
    kernel_ventry   1, t, 64, error     // Error EL1t

    kernel_ventry   1, h, 64, sync      // Synchronous EL1h
    kernel_ventry   1, h, 64, irq       // IRQ EL1h
    kernel_ventry   1, h, 64, fiq       // FIQ EL1h
    kernel_ventry   1, h, 64, error     // Error EL1h

    kernel_ventry   0, t, 64, sync      // Synchronous 64-bit EL0
    kernel_ventry   0, t, 64, irq       // IRQ 64-bit EL0
    kernel_ventry   0, t, 64, fiq       // FIQ 64-bit EL0
    kernel_ventry   0, t, 64, error     // Error 64-bit EL0

    kernel_ventry   0, t, 32, sync      // Synchronous 32-bit EL0
    kernel_ventry   0, t, 32, irq       // IRQ 32-bit EL0
    kernel_ventry   0, t, 32, fiq       // FIQ 32-bit EL0
    kernel_ventry   0, t, 32, error     // Error 32-bit EL0
```
- arch/arm64/kernel/entry-common.c

- 什么叫做 el1t 和 el1h 的区别是什么?
  - https://stackoverflow.com/questions/21586768/armv8-aarch64-vs-aarch32-stack-pointer-register
  - 目前是相关知识介绍的最清楚的了: http://www.wowotech.net/armv8a_arch/238.html

- el1h_64_sync_handler : 内核中出现 sync 的错误，很快就会到达 kdump 的位置
- el0t_64_sync_handler : 用户态出现错误
  - el0_da
    - exit_to_user_mode : 这里有我们熟悉的进程切换的环节
- el1h_64_irq_handler
  - el1_interrupt
    - `__el1_irq`
      - do_interrupt_handler ：最后调用到对应的 hook 应该一般就是 `gic_handle_irq` 上了。

- arch/arm64/mm/fault.c 和 page fault 相关的内容会跳转到这里


- [ ] 不知道为什么 el1t 是不存在的，而 el0h 是不存在的
```c
UNHANDLED(el1t, 64, sync)
UNHANDLED(el1t, 64, irq)
UNHANDLED(el1t, 64, fiq)
UNHANDLED(el1t, 64, error)
```

## [x] arm 需要额外分配很多空间吗？
- kvm_prepare_memory_region
  - kvm_arch_prepare_memory_region

## 如何理解 pkvm
https://source.android.com/docs/core/virtualization/architecture?hl=zh-cn

## 了解下 ARM 中的 virt machine type
最大支持的内存修改的好随意啊
- https://lists.gnu.org/archive/html/qemu-arm/2016-02/msg00453.html

## arm sve
https://alastairreid.github.io/papers/sve-ieee-micro-2017.pdf

## 访问时钟是如何确定的？
- https://developer.arm.com/documentation/ddi0595/2021-03/AArch64-Registers/CNTVCT-EL0--Counter-timer-Virtual-Count-register

## 看看
- https://www.openeuler.org/en/blog/yorifang/2020-10-24-arm-virtualization-overview.html : 虚拟化，可以看看

## 有趣的 TODO
1. 为什么 arm 有那么多的 vgic 版本?
  - 而且 arch/arm64/kvm/vgic/vgic-v5.c 的代码量这么小?
  - kvm 选择使用那一个 vgic 是在哪里决定的?
  - 虚拟机是如何知道 kvm 提供那一个 vgic ，是从哪里知道的。

2. 也许从 arm 的 vgic 中看 irqfd ，以及什么 gsi 之类的东西:
arch/arm64/kvm/vgic/vgic-irqfd.c

2. 看看 arm 对于变长指令的支持的情况


## arm 这个必须看看
https://www.cnblogs.com/LoyenWang/p/13584020.html

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
