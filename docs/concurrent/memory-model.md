# memory model

## 问题
- [ ] 能不能稳定的复现，或者制作出来这些 memory model 的效果
- [ ] 各种使用案例
- [ ] 和 volatile 中的问题

- https://www.cs.utexas.edu/~bornholt/post/memory-models.html

- [Shared Memory Consistency Models: A Tutorial](https://www.hpl.hp.com/techreports/Compaq-DEC/WRL-95-7.pdf)
  - 这是最经典的项目了

字节团队写的，应该是相当清楚了:
https://mp.weixin.qq.com/s/wt5b5e1Y1yG1kDIf0QPsvg

https://lotabout.me/2019/QQA-What-is-Sequential-Consistency/
- 介绍什么是顺序一致性

- https://bitbashing.io/papers.html : 其中有一篇是关于 memory concurrency 的

- https://paulcavallaro.com/blog/x86-tso-a-programmers-model-for-x86-multiprocessors/

在 go 语言中，因为没有考虑到 arm 的弱内存序导致的问题:
https://mzh.io/how-go-core-team-debug-1-memory-model/

Each memory model defines `pfn_to_page()` and page_to_pfn() helpers that allow the conversion from PFN to struct page and vice versa. [^12]

- [ ] https://randomascii.wordpress.com/2020/11/29/arm-and-lock-free-programming/
- [ ] [^12]
- [ ] https://kernelgo.org/memory-model.html : really nice blog with cpp perspective
- [ ] https://research.swtch.com/mm : Rust 的 contributor ? 写的

[^12]: [kernel doc : memory model](https://www.kernel.org/doc/html/latest/vm/memory-model.html)
