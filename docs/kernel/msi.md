## MSI
Generic MSIs : https://en.wikipedia.org/wiki/Message_Signaled_Interrupts

linux/drivers/pci/msi.c

use nvme as example:
- nvme_pci_enable
  - pci_alloc_irq_vectors
    - pci_alloc_irq_vectors_affinity
      - `__pci_enable_msix_range`
        - `__pci_enable_msix`
          - **msix_capability_init** : [^3] page 130

[ARM](https://elinux.org/images/8/8c/Zyngier.pdf)

# kernel/irq
> 缺乏相关的背景内容，根本没有办法动笔!

## todo
1. 以 x86 为例，想知道从 architecture 的 entry.S 触发 到指向对应的 handler 的过程是怎样的 ?
2. 中断控制器很简单，其大概是怎么实现的 ?
    1. CPU 提供给 interrupt controller 的接口是什么 ?
        1. 我(CPU) TM 怎么知道这一个信号是来自于 interrupt controller 的 ?
        2. 是不是 CPU 专门为其提供了引脚，各种 CPU 的引脚的数量是什么 ?
        3. IC 的输入输出的引脚的数量是多少 ?
    2. 当 IC 可以进行层次架构之后，

3. 为了多核，是不是也是需要进一步修改 IC 来支持。
    1. affinity 提供硬件支持

## wowotech 相关的内容

1. 中断描述符中应该会包括底层 irq chip 相关的数据结构，linux kernel 中把这些数据组织在一起，形成`struct irq_data`
2. 中断有两种形态，一种就是直接通过 signal 相连，用电平或者边缘触发。另外一种是基于消息的，被称为 MSI (Message Signaled Interrupts)。
3. Interrupt controller 描述符（struct irq_chip）包括了若干和具体 Interrupt controller 相关的`callback`函数
