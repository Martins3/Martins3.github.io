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

## sched 目录下有这个文件，可以看看
- linux/kernel/sched/membarrier.c

# 同步，并发和锁

## 吐槽
为什么操作系统中的大多数内容都是讲锁?
- [ ] 使用 Modern OS 做一个验证

## 资源
- 并发编程艺术

## 用户态的常用技术
- 从 Java 并发编程中总结一下
  - 然后对应将 cpp 和 c 中总结一下

> 如果是将 perf book 作为参考，那么将会很快乐，如果只是为了阅读，将会很痛苦的

# 当这个事情结束之后，重新思考一下这个事情
- [ ] 回答在 volatile 中的问题
- [ ] 总结一个如何使用 memory bariier 的基本方法，太 TM 的恐惧了
- [ ] 如何正确的使用 RCU ，以及 Linux 内核中的 ART
- [ ] 总结 memory model 形成的原因，CPU 如何保证，对于编程者的影响
- [ ] C++ 并发编程 和 Rust 并发编程 : 写一个 blog 对比一下。
- [ ] cache coherence 和 memory model 的关系 ?
- [ ] DMA 的 cache 一致性是谁处理的

## TODO
- coroutine in c++

## 正经的教材
https://arxiv.org/pdf/1701.00854.pdf

## 阅读的 blog

https://begriffs.com/posts/2020-03-23-concurrent-programming.html?hn=1
https://www.datadoghq.com/blog/engineering/introducing-scipio/
https://preshing.com/20120612/an-introduction-to-lock-free-programming/ : notes of `the art of concurrent programming`

## 资源

https://github.com/cpp-taskflow/cpp-taskflow 其实，我一直想要知道如何对 GPU 进行编程，以及其他的内容，这个代码非常有意思的。

https://brennan.io/2020/05/24/userspace-cooperative-multitasking/ : c setjmp longjmp 实现 multitasking

## 项目
https://github.com/Tencent/libco : 腾讯的项目，似乎代码量不是很大，接口简单

https://github.com/libuv/libuv : aio 跨平台库

https://github.com/taskflow/taskflow

## 想法
其实关于 concurrent 可以好好总结一下:
1. 微架构
2. 锁
3. 操作系统的支持: 比如 aio
4. 各种设计模式 比如 Java 并发编程
5. 然后列举各种例子，内核 malloc 等等为多核做出的努力

## perf book
https://mirrors.edge.kernel.org/pub/linux/kernel/people/paulmck/perfbook/perfbook.html
