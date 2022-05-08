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
- [An introduction to lockless algorithms](https://lwn.net/Articles/844224/) : 本来以为 lockless 实际上没有什么作用，但是实际上在 Linux 中是存在很多使用的

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
