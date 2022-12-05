# 并行程序设计

> 从量化分析方法的中关于并行的总结全部放到这里来。

- [ ] 如果 Guest 和 Host 如何同步？
  - 如果 Guest 对于一个物理内存原子操作，那么 Host 也是无法修改的吗?
    - 会不会在一些边界上制造问题

## 这个 kernel locking 的内容非常老了，应该整理一个类似的内容出来
https://www.kernel.org/doc/htmldocs/kernel-locking/index.html

## 深入分析 tls 和 percpu 机制

## 收集的资源
- https://www.1024cores.net/
  - 其中有分析过 wait free 和 lock free 的内容

## 硬件支持上也可以分析一下
- [CPU 多核指令 —— WFE 原理](http://www.wowotech.net/armv8a_arch/499.html)
  - http://www.wowotech.net/sort/armv8a_arch : 其实 wowotech 关于 ARM 的 atomic 分析了不少内容

- https://news.ycombinator.com/item?id=20096946
  - The design and implementation of a lock-free ring-buffer with contiguous reservations
  - 好像 dpdk 中也是提到过这个事情的

- umwait 指令: https://lwn.net/Articles/790920/

- 也许，我们应该增加一个项目，叫做不同的语言是如何设计锁的:
  - https://course.rs/advance/concurrency-with-threads/thread.html
- https://www.kawabangga.com/posts/4777
