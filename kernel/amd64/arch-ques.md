> 关于需要硬件的问题:
> 1. context switch 的硬件支持，以前使用TSS 为什么现在不使用了
> 2. exception interrupt syscall 需要的支持
> 3. io 映射， inb outb 的实现 ?
> 4. 用户和内核，用户和用户之间如何实现的保护的
> 5. 原子操作指令(应该很简单)
> 6. 内存屏障指令


# 关于amd64 架构的疑问
1. cs gs fs 三个segment 寄存器的作用是什么 ?
2. gdt  tss  idt 各自运行机制是什么
3. msr 寄存器等作用


4. swapfs 等各种特权指令
> 直接参考v3 中的特权指令




> 需要最终可以看的懂 entry_64.S(32 到 64 的切换) 和 head_64.S 中间的所有的内容即可。
```
 * entry.S contains the system-call and fault low-level handling routines.
 *
 * Some of this is documented in Documentation/x86/entry_64.txt
```




# 要不要学一波汇编语言
https://software.intel.com/en-us/articles/introduction-to-x64-assembly intel 入学手册，感觉没有必要。



