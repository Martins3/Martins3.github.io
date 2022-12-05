# memory model
- 简短有力的分析: https://zhuanlan.zhihu.com/p/41872203
- https://mp.weixin.qq.com/s/s6AvLiVVkoMX4dIGpqmXYA

- https://github.com/mrkline/concurrency-primer

- Memory Barriers: a Hardware View for Software Hackers
- What every systems programmer should know about concurrency

## dma
- dma_alloc_coherent

## 相关资料
- https://stackoverflow.com/questions/38447226/atomicity-on-x86
- https://stackoverflow.com/questions/39393850/can-num-be-atomic-for-int-num

- https://news.ycombinator.com/item?id=32520365
- [ ] sys/mccc 的 A Primer on Memory Consistency and Cache Coherence 可以重新看看

## 问题
- [ ] 能不能稳定的复现，或者制作出来这些 memory model 的效果
- [ ] 各种使用案例
- [ ] 和 volatile 中的问题
- [ ] https://blog.stuffedcow.net/2015/08/pagewalk-coherence/ 还存在一些蛇皮的 TLB pagewalk 的 coherence 问题啊

- [ ] GPU 中的 memory model 是做什么的?
- [ ] 尝试了解一下 RISCV 的模型
  - https://zhuanlan.zhihu.com/p/191660613
  - https://riscv.org/wp-content/uploads/2018/05/14.25-15.00-RISCVMemoryModelTutorial.pdf
- https://zhuanlan.zhihu.com/p/151425608

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

- [ ] `READ_ONCE` and `WRITE_ONCE`

## perf book 中的 B.3 的表中使用 ref counter 实现 RCU

## 和 cache coherence 的关系是什么
- [ ] 可以吧周音困的总结搞过来看看

- [ ] cache 中的三级 cache 同步的时候，需要每一个层级都同步吗?
- [ ] 可不可以将 load queue / store queue 也是作为 3 级 cache 中的一种方法

- [ ] 似乎 [^1] 中说明了 **全局序** ，那么什么样子的设计会导致观测顺序的不一致啊

- 浅谈多核系统的缓存一致性协议与非均一缓存访问 : https://zhuanlan.zhihu.com/p/162099300

## 具体案例
### `__setup_APIC_LVTT`
```c
		/*
		 * See Intel SDM: TSC-Deadline Mode chapter. In xAPIC mode,
		 * writing to the APIC LVTT and TSC_DEADLINE MSR isn't serialized.
		 * According to Intel, MFENCE can do the serialization here.
		 */
		asm volatile("mfence" : : : "memory");
		return;
```
###  wait_on_bit
kernel 8238b4579866b7c1bb99883cfe102a43db5506ff

### virtio_wmb

## io uring 的使用似乎是需要实现用户态和系统态的同步，使用 memory barrier 的

---
title: 'Shared Memory Consistency Models: A Tutorial'
date: 2017-05-04 15:59:00
tags: papre
---

> Warning: Published in 1995. Older than me !

# Abstraction
parallel systems that support the shared memory abstraction are becoming widely accepted in many areas of computing
> shared memory abstraction : different from OS ? by hardware ?

We focus on consistency models proposed for hardware-based sharedmemory systems

The shared memory or single address space abstraction provides several advantages over the message passing (or
private memory) abstraction by presenting a more natural transition from uniprocessors and by simplifying difficult
programming tasks such as **data partitioning** and **dynamic load distribution**

> why should use unimemory ?
> what are the two terminology

# 1 Introduction


We begin with a short note on who should be concerned with the memory consistency model of a system.
We next describe the programming model offered by **sequential consistency**, and the **implications of sequential consistency on hardware and compiler implementations**.
We then describe several relaxed memory consistency models using a simple and uniform terminology.
The last part of the article describes the programmer-centric view of relaxed memory consistency models

# 2 Memory Consistency Models - Who Should Care

# 3 Memory Semantics in Uniprocessor Systems

# 4 Understanding Sequential Consistency

# 5 Implementing Sequential Consistency

> Is Sequential and Consistency are controversial words ?

---

[^1]: https://zhuanlan.zhihu.com/p/191660613
