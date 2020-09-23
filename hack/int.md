# 中断

不要害怕开始：
1. 总结 从 ics 的中断 和 ucore 的中断的实现，然后再去分析
2. file:///home/shen/Core/hack-linux-kernel/Documentation/output/teaching/lectures/interrupts.html


这篇文章
A Hardware Architecture for Implementing Protection Rings
让我怀疑人生:
1. 这里描述的 gates 和 syscall 是什么关系 ?
2. syscall 可以使用 int 模拟实现吗 ?
3. interupt 和 exception 在架构实现上存在什么区别吗 ?

## workqueue
wowotech 中间的东西可以看看:
https://zhuanlan.zhihu.com/p/91106844

## ipi
https://stackoverflow.com/questions/62068750/kinds-of-ipi-for-x86-architecture-in-linux
