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
