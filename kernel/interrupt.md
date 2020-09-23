# Exception and Interupt in Linux


## 问题
1. 中断的分类 从中文到英， 一个都没有定义清楚的东西
2. 中断和多核的关系是什么 ?

## 异常和终端的分类

[某人的分类](https://zhuanlan.zhihu.com/p/26568566)， 但是不是非常的认同
>  1. 异常（excepption）：由CPU内部引起，也叫同步中断，不能被CPU屏蔽，细分分为：
>    1. Fault（出错）：可恢复，恢复后回到出发Fault的那条指令继续执行，比如 Page Fault，在page被load进内存后继续执行刚才执行的命令
>    2. Trap（陷入）：可恢复，恢复后执行触发Trap指令的后一条指令。比如除0错误，或者Invalid Memory Address Reference。也称为硬陷入（Hardware Trap），对比软陷入（Software Trap）
>    3. Abort（中止）：无法恢复，比如发生了硬件问题。
>  2. 中断：也叫异步中断，由外部设备引起，细分为可屏蔽中断（INTR）和非可屏蔽中断（NMI）

| -             | synchronous                                           | asynchronous                |
|---------------|-------------------------------------------------------|-----------------------------|
| 产生部位      | CPU控制单元                                           | 外部设备                    |
| intel标准名称 | exception(maskable exception & nonmaskable exception) | interrupt(fault trap abort) |
| mips标准名称  |                                                       |                             |

fault : 可纠正的
trap : 主要用于调试
abort : 严重错误
programming exception(software interrupt) : 编程者发出



## 几个重要的定义
1. interrupt handler

2. interrupt service routine

3. softirq

4. tasklet

## 中断的流程


## 中断实现的硬件支持


## 中断的软件支持


## 源码级解释

### [参考一](https://kernelnewbies.org/KernelHacking-HOWTO/Overview_of_the_Kernel_Source_Code/Internals_of_Interrupt_Handling/Details_of_do_IRQ%28%29_function)
> 忽然发现newbie上面的内容不只是一点点啊


### [参考二](http://www.stillhq.com/pdfdb/000447/data.pdf)


### 参考三
> Linux kernel development 3e

和硬件沟通的方法: poll 或者 interrupt

An interrupt is physically produced by electronic signals originating from hardware
devices and directed into input pins on an interrupt controller, a simple chip that multi
plexes multiple interrupt lines into a single line to the processor
> a simple chip

In OS texts, exceptions are often discussed at the same time as interrupts. Unlike interrupts, exceptions occur synchronously with respect to the processor clock. Indeed, they are
often called synchronous interrupts

Different devices can be associated with different interrupts by means of a unique
value associated with each interrupt
Interrupts associated with devices on the PCI bus, for example, generally are dynamically assigned
> 常用的是固定的，但是PCI 上面插入是动态分配的

Because many processor architectures handle exceptions in a similar manner to interrupts,
the kernel infrastructure for handling the two is similar
> 那为什么，　x86定义了那么多的中断出来。

The function the kernel runs in response to a specific interrupt is called an interrupt handler
or interrupt service routine (ISR)

The interrupt handler for a device is part of the device’s driver—the kernel code that manages the device
> driver 到底是什么东西，和linux module有什么关系，如果驱动装入到内核态，驱动又是充满bug的，　岂不是降低了内核的安全

What differentiates interrupt handlers from other kernel functions is that the kernel
invokes them in response to interrupts and that they run in a special context 
called interrupt context

The interrupt handler is the top half.
The top half is run immediately upon receipt of the interrupt and performs only the
work that is time-critical, such as acknowledging receipt of the interrupt or resetting the
hardware.Work that can be performed later is deferred until the bottom half.The bottom
half runs in the future, at a more convenient time, with all interrupts enabled. Linux provides various mechanisms for implementing bottom halves.
> 为了解决中断要求尽快返回和　中断中间含有大量的事情处理的矛盾

**interrupt line** : these interrupt values are often called interrupt request (IRQ) lines

An interrupt is physically produced by electronic signals originating from hardware
devices and directed into input pins on an interrupt controller, plexes multiple interrupt lines into a single line to the processor


registering an interrupt handler:
```
int request_irq(unsigned int irq,
  irq_handler_t handler,
  unsigned long flags,
  const char *name,
  void *dev)
```
handler is a function pointer：
```
typedef irqreturn_t (*irq_handler_t)(int, void *);
```

> 117 最后一段关于sleep 安全的没有看懂

A call to `free_irq()` must be made from process context
> 什么叫做process context, 一共都是含有什么process context 呢


Writing a interrupt handler
```
/** irq : interrupt line number
    dev : dev name
  */
static irqreturn_t intr_handler(int irq, void *dev)
```
irq was used to differentiate between multiple devices using the same driver and therefore the same interrupt handler

> 既然参数没有办法发生变化，dev和irq 都是采用的`request_irq` 中间的变量，为什么还要设置这两个参数

> 中断的过程是不是，interrupt line接受信号，然后注册在该line 上面所有的线全部的函数都被调用，然后所有的函数检查是不是调用自己，如果不是，　直接返回

When executing an interrupt handler, the kernel is in interrupt context

Recall that **process context** is the mode of operation the kernel is in while it is executing on behalf of a process

The setup of an interrupt handler’s stacks is a configuration option

```
unsigned int do_IRQ(struct pt_regs regs) /* 检查当前环境是否可以开始中断 */
irqreturn_t handle_IRQ_event(unsigned int irq, struct irqaction *action) /*　执行中断 */
```

To disable interrupts locally for the current processor (and only the current processor) and
then later reenable them, do the following:
```
local_irq_disable();
/* interrupts are disabled .. */
local_irq_enable();
```

The `local_irq_disable()` routine is dangerous if interrupts were already disabled prior to its invocation
> 解决的办法是添加参数flags, 但是细节有点迷糊


masking out an interrupt line
```
void disable_irq(unsigned int irq);
void disable_irq_nosync(unsigned int irq);
void enable_irq(unsigned int irq);
void synchronize_irq(unsigned int irq);
```

The kernel provides interfaces for registering and unregistering interrupt handlers,
disabling interrupts, masking out interrupt lines, and checking the status of the interrupt system
