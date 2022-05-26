# memory model

## 问题
- [ ] 能不能稳定的复现，或者制作出来这些 memory model 的效果
- [ ] 各种使用案例
- [ ] 和 volatile 中的问题
- [ ] https://blog.stuffedcow.net/2015/08/pagewalk-coherence/ 还存在一些蛇皮的 TLB pagewalk 的 coherence 问题啊

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
- https://www.cl.cam.ac.uk/~pes20/weakmemory/cacm.pdf
- https://www.cl.cam.ac.uk/~pes20/weakmemory/x86tso-paper.tphols.pdf

[^12]: [kernel doc : memory model](https://www.kernel.org/doc/html/latest/vm/memory-model.html)

## https://research.swtch.com/mm

```c
// Thread 1           // Thread 2
x = 1;                while(done == 0) { /* loop */ }
done = 1;             print(x);
```
It depends. It depends on the hardware, and it depends on the compiler. A direct line-for-line translation to assembly run on an x86 multiprocessor will always print 1. But a direct line-for-line translation to assembly run on an ARM or POWER multiprocessor can print 0.
- [ ] 为什么 x86 不会
- [ ] 如果从 high level 的语言的角度处理，为什么会存在更加高级的


<p align="center">
  <img src="https://research.swtch.com/mem-sc.png" alt="drawing" align="center"/>
</p>
<p align="center">
https://research.swtch.com/hwmm
</p>

<p align="center">
  <img src="https://research.swtch.com/mem-tso.png" alt="drawing" align="center"/>
</p>
<p align="center">
https://research.swtch.com/hwmm
</p>


Litmus Test: Write Queue (also called Store Buffer) Can this program see r1 = 0, r2 = 0?
```txt
// Thread 1           // Thread 2
x = 1                 y = 1
r1 = y                r2 = x
```

- On sequentially consistent hardware: no.
- On x86 (or other TSO): yes!

This example may seem artificial, but using two synchronization variables does happen in well-known synchronization algorithms, such as `Dekker's algorithm` or `Peterson's algorithm`, as well as ad hoc schemes. They break if one thread isn’t seeing all the writes from another.

Litmus Test: Independent Reads of Independent Writes (IRIW)
- there is a total order over all stores (writes) to main memory, and all processors agree on that order, subject to the wrinkle that each processor knows about its own writes before they reach main memory.

## leveldb 中的 skiplist 中的 cpp 11 的 memory model

## memory consistency
- [ ] https://zhuanlan.zhihu.com/cpu-cache : read posts writen by muchun

- [ ] barrier() Documentation/memory-barriers.txt : 彻底理解让人窒息的 memory-barriers.txt

当分析那么多窒息的例子，都是由于同时访问相同位置的内存，但是访问相同位置的内存的时候，难道不是采用 lock 保护的吗 ? smp_mb 的使用位置和实现方式是什么 ?

// TODO 是存在一个叫做 membarrier 的系统调用的哦!

// 教程，也许可以阅读一下 :
https://www.cs.utexas.edu/~bornholt/post/memory-models.html
https://www.linuxjournal.com/article/8211
https://www.linuxjournal.com/article/8212

- [ ] READ_ONCE and WRITE_ONCE
