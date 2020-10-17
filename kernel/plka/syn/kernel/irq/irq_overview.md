# kernel/irq
> 缺乏相关的背景内容，根本没有办法动笔!

| file           | blank | comment | code | explanation |
|----------------|-------|---------|------|-------------|
| manage.c       | 309   | 678     | 1271 |             |
| irqdomain.c    | 242   | 412     | 1107 |             |
| chip.c         | 217   | 390     | 843  |             |
| irqdesc.c      | 157   | 127     | 661  |             |
| generic-chip.c | 92    | 144     | 412  |             |
| proc.c         | 92    | 52      | 385  |             |
| msi.c          | 76    | 92      | 357  |             |
| internals.h    | 73    | 60      | 348  |             |
| matrix.c       | 55    | 121     | 280  |             |
| spurious.c     | 58    | 160     | 247  |             |
| debugfs.c      | 45    | 8       | 220  |             |
| ipi.c          | 45    | 112     | 182  |             |
| affinity.c     | 44    | 44      | 182  |             |
| devres.c       | 39    | 98      | 150  |             |
| settings.h     | 27    | 5       | 137  |             |
| pm.c           | 32    | 56      | 124  |             |
| timings.c      | 41    | 197     | 124  |             |
| cpuhotplug.c   | 24    | 86      | 106  |             |
| handle.c       | 35    | 86      | 101  |             |
| irq_sim.c      | 25    | 43      | 95   |             |
| autoprobe.c    | 21    | 71      | 92   |             |
| migration.c    | 18    | 45      | 56   |             |
| resend.c       | 11    | 41      | 48   |             |
| debug.h        | 8     | 5       | 36   |             |
| dummychip.c    | 7     | 21      | 36   |             |
| Makefile       | 1     | 1       | 15   |             |


## todo
1. 以 x86 为例，想知道从 architecture 的 entry.S 触发 到指向对应的 handler 的过程是怎样的 ?
2. 中断控制器很简单，其大概是怎么实现的 ?
    1. CPU 提供给 interrupt controller 的接口是什么 ?
        1. 我(CPU) TM 怎么知道这一个信号是来自于 interrupt controller 的 ?
        2. 是不是 CPU 专门为其提供了引脚，各种CPU 的引脚的数量是什么 ?
        3. IC 的输入输出的引脚的数量是多少 ?
    2. 当 IC 可以进行层次架构之后，

3. 为了多核，是不是也是需要进一步修改 IC 来支持。
    1. affinity 提供硬件支持


## wowotech 相关的内容

1. 中断描述符中应该会包括底层irq chip相关的数据结构，linux kernel中把这些数据组织在一起，形成`struct irq_data`
2. 中断有两种形态，一种就是直接通过signal相连，用电平或者边缘触发。另外一种是基于消息的，被称为MSI (Message Signaled Interrupts)。
3. Interrupt controller描述符（struct irq_chip）包括了若干和具体Interrupt controller相关的`callback`函数
