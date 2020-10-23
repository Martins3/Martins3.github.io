- [ ] clear this document and pick useful info from plka/syn

# ptrace 的代价是什么 ?
1. 为了支持ptrace syscall的努力是什么 
2. ptrace 是不是实现debug 的基础
3. fork 等机制做出了何种支持


# 内核和用户态之间拷贝数据
1. 用户读写文件　文件内容　是如何传递到达用户的 ?　显然不可能是通过copy_to_user 之类的函数
2. copy_to_user 的两个参数，都是虚拟地址，但是实际上，但是这两个地址是位于不同的pgdir 中间的
3. copy_to_user 检查内容是看该addr 是不是在对应用户的地址空间的 segment 而且保证没有segment fault，这一个函数从哪里获取的 mm_struct 


# 到底如何实现context switch

1. http://www.maizure.org/projects/evolution_x86_context_switch_linux/
> 绝对清晰的讲解
Many of these tasks float between the `switch_to()` and the scheduler across kernel versions. All I can guarantee is that we'll always see stack swaps and FPU switching in every version

2. https://eli.thegreenplace.net/2018/measuring-context-switching-and-memory-overheads-for-linux-threads/　
> 分析context switch 的代价是什么 ?

3. https://stackoverflow.com/questions/2711044/why-doesnt-linux-use-the-hardware-context-switch-via-the-tss
> 再次印证TSS 在 context switch 中间并没有什么作用，但是 @todo TSS 中间存储了ESP0 和 SS0 用于实现interrupt
