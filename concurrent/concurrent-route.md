## Plans
1. 底层架构上，cache 一致性, memory fence !(量化，小册子)
2. C++ 并发编程 和 Rust 并发编程 : 写一个 blog 对比一下。
3. perf
4. 将之前阅读 the art of concurrent programming 整理一下 !

## TODO
- even loop : https://questions.wizardzines.com/event-loops.html : I read part of it, it seems I haven't learn event loop at all.
- coroutine in c++

## 正经的教材
https://arxiv.org/pdf/1701.00854.pdf

## 阅读的blog

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
