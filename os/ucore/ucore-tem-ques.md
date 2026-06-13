## TSS segment 的划分是x86 特有的吗 ?
tss 处理了 ring 的切换

这时CPU会从当前程序的TSS信息（该信息在内存中的起始地址存在TR寄存器中）里取得该程序的内核栈地址，即包括内核态的ss和esp的值，并立即将系统当前使用的栈切换成新的内核栈。这个栈就是即将运行的中断服务程序要使用的栈。紧接着就将当前程序使用的用户态的ss和esp压到新的内核栈中保存起来；
> 这是hardware 处理的，是那一条指令(应该是int, 但是PA中间实现的没有这么复杂)，找到amd64的文档或者arm64的文档
> 更多的中断可以在 0ax0 的blog 中间分析


## GDT 是x86 特有的吗 ?


## x86 汇编参考
http://www.cs.princeton.edu/courses/archive/fall09/cos318/precepts/precept0 
https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/esqaq/index.html forward label
https://docs.oracle.com/cd/E19620-01/805-4693/instructionset-73/index.html　ljmp
https://stackoverflow.com/questions/36617718/difference-between-dpl-and-rpl-in-x86 rpl

## gdt and segment selector
https://stackoverflow.com/questions/33198282/why-have-the-first-segment-descriptor-of-the-global-descriptor-table-contain-onl/33198311#33198311 
> @todo 

https://en.wikipedia.org/wiki/Segment_descriptor
> gdt 中间到底包含的是什么东西

https://stackoverflow.com/questions/9113310/segment-selector-in-ia-32
> segment selector 中间不仅仅包含的数值为 index，这里的确对

https://en.wikipedia.org/wiki/X86_memory_segmentation


https://stackoverflow.com/questions/6468896/why-is-orig-eax-provided-in-addition-to-eax
> orig_eax 是什么?

https://wiki.osdev.org/CPU_Registers_x86-64
> 关于x86 的寄存器使用的完整清晰的资料:

1. 通用寄存器16 个，包括SP BP
2. Segments of CS, DS, ES, and SS are treated as if their base was 0 no matter what the segment descriptors in the GDT say. Exceptions are FS and GS which have MSRs to change their base. 实现平坦模型的定义


## 整理资料
而IDT本身的起始地址保存在idtr寄存器中



## 代办
1. 测试样例的内容是什么 ?
2. 分析kern/debug 在各个试验中间的变化是什么? 至少需要解释为什么 kernel.ld 中间的 debug 作用的 stab
    1. 分析一下assert fail 之后输出的backtrace的内容，
3. 居然还有swap.img 和 sys.img 注意一下nemu 是如何使用这些东西的


4. getchar() 的实现很有意思，其实可以不使用 trap 来处理

