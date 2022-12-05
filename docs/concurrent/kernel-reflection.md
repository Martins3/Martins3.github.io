# 内核的想法


## 为什么内核如此的复杂
因为内核锁需要描述的事情太复杂了。

- trace / kdump / bpf
- 进程
- cgroup
- fs
- 内存管理
- 外设
  - 中断
- RDMA
- vfio
- iommu

## 内核中核心

我认为是，当理解了这些内容，之后阅读任何内容，都不会进入到递归学习的状态，
阅读内核代码和阅读普通代码一样，只是在乎其中的业务逻辑，而无需额外的掌握新的技术。

这些代码不读，会让人感觉不断的刺痛的位置。

- 地址空间映射
- 中断，thread 如何上下文切换，fork 的原理，preemption
- 锁
- syscall

> 代码流程分析的头头是道，但是如果我问题，现在虚拟地址是什么，物理地址是什么，stack 在哪里，
当前代码的位置能够发生中断，当前的位置能够发生抢占，当前位置持有了什么锁，如果发生了中断或者抢占，
接下来的执行流程是什么，如果答不出来，我只能给你零分。

反而没有想象的那么重要的部分：
- 内存页面分配：
- 调度算法：

一些，看似没关系，实际上，无处不在：
- cgroup

## kernel lock questions
在 64bit 中间 kmap 和 `kmap_atomic` 的区别:

1. 为什么这保证了 atomic ?
2. 什么情况需要 atomic ?
```c
static inline void *kmap_atomic(struct page *page)
{
    preempt_disable();
    pagefault_disable();
    return page_address(page);
}
```

Code that is safe from concurrent access from an interrupt handler is said to be
**interrupt-safe**. Code that is safe from concurrency on symmetrical multiprocessing
machines is **SMP-safe**. Code that is safe from concurrency with kernel preemption is **preempt-safe**.
