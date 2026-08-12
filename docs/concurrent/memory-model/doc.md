# memory model

## 使用场景

### acquire and release

<!-- c3d3782f-e682-4b81-8e24-2dafd92d2b04 -->

这是最经典的，如果你什么都记住不是，

```txt
前面的 memory operations
          |
          v
       RELEASE
```

```txt
       ACQUIRE
          |
          v
后面的 memory operations
```

### 其他

(如果只有这个场景，那么为什么还有这么多的 CPU 定义的 model ?) (或者说，cpp
为什么定义出来这么多的 model )

release consume 暂时不用吧
https://stackoverflow.com/questions/65336409/what-does-memory-order-consume-really-do

## 回归到原理

### cache coherence

1. 如果 cacheline 的大小那么大，lock 为什么可以只是 lock 一个字节 ? 在同一个
   cacheline 上操作，会出现更加严重的 false share

### store buffer 和 load buffer 在那里?


## 相关资料

- https://stackoverflow.com/questions/38447226/atomicity-on-x86
- https://stackoverflow.com/questions/39393850/can-num-be-atomic-for-int-num

- https://news.ycombinator.com/item?id=32520365
- [ ] sys/mccc 的 A Primer on Memory Consistency and Cache Coherence
      可以重新看看

- 简短有力的分析: https://zhuanlan.zhihu.com/p/41872203
- https://mp.weixin.qq.com/s/s6AvLiVVkoMX4dIGpqmXYA

- Memory Barriers: a Hardware View for Software Hackers
- What every systems programmer should know about concurrency

- 现在才知道在 C++ 11 才开始定义的 memory model 的
- https://github.com/LearningOS/aos-lectures/blob/master/lec12/slide-12-01.tex
  - 但是不知道这个 slides 如何编译

- [ ] 尝试了解一下 RISCV 的模型
  - https://zhuanlan.zhihu.com/p/191660613
  - https://riscv.org/wp-content/uploads/2018/05/14.25-15.00-RISCVMemoryModelTutorial.pdf
- https://zhuanlan.zhihu.com/p/151425608

- https://www.cs.utexas.edu/~bornholt/post/memory-models.html

- https://bitbashing.io/papers.html : 其中有一篇是关于 memory concurrency 的

- https://paulcavallaro.com/blog/x86-tso-a-programmers-model-for-x86-multiprocessors/

- [ ] https://randomascii.wordpress.com/2020/11/29/arm-and-lock-free-programming/
- [ ] https://research.swtch.com/mm : Rust 的 contributor ? 写的
- https://www.cl.cam.ac.uk/~pes20/weakmemory/cacm.pdf
- https://www.cl.cam.ac.uk/~pes20/weakmemory/x86tso-paper.tphols.pdf

## ARM 的文档

- https://developer.arm.com/documentation/den0024/a/Memory-Ordering

[lock would enforce a memory barrier, like an mfence](https://stackoverflow.com/questions/42820121/why-cannot-the-load-part-of-the-atomic-rmw-instruction-pass-the-earlier-store-to)

> 1. Loads may be reordered with older stores to different locations but not
>    with older stores to the same location
> 2. Locked instructions have a total order

https://www.arangodb.com/2021/02/cpp-memory-model-migrating-from-x86-to-arm/

## 只有 store-load 存在乱序

https://preshing.com/20120930/weak-vs-strong-memory-models/
https://preshing.com/20120710/memory-barriers-are-like-source-control-operations/

## 这个

- https://www.cl.cam.ac.uk/~pes20/ppc-supplemental/test7.pdf
- https://acg.cis.upenn.edu/rg_papers/memo-493.pdf

这个讨论有意思的啊
https://github.com/ziglang/zig/issues/6396

有趣的问题
https://bartoszmilewski.com/2008/11/05/who-ordered-memory-fences-on-an-x86/

## 这种文摘需要看看

https://www.scylladb.com/2018/02/15/memory-barriers-seastar-linux/

## [ ] 分析下 ARM 的 memory model，尤其是对比普通 arm 和 Mac arm 的区别


Memory Barriers in the Linux Kernel
https://elinux.org/images/a/ab/Bueso.pdf

## 其他的阅读材料

- https://lwn.net/Articles/846700/
- https://lwn.net/Articles/576486/
- https://stackoverflow.com/questions/61749435/pairing-acquire-release-operations-between-user-and-kernel-space

- https://stackoverflow.com/questions/59626494/understanding-memory-order-acquire-and-memory-order-release-in-c11
  - 答案中推荐的做法: https://www.youtube.com/watch?v=A8eCGOqgvH4

- https://stackoverflow.com/questions/36824811/memory-fences-acquire-load-and-release-store

- https://stackoverflow.com/questions/15491751/real-life-use-cases-of-barriers-dsb-dmb-isb-in-arm

- https://stackoverflow.com/questions/23105052/is-there-a-need-for-dmb-if-we-are-using-dsb

- [ ] https://zhuanlan.zhihu.com/cpu-cache

当分析那么多窒息的例子，都是由于同时访问相同位置的内存，但是访问相同位置的内存的时候，难道不是采用
lock 保护的吗 ? smp_mb 的使用位置和实现方式是什么 ?

// 教程，也许可以阅读一下 :

- https://www.cs.utexas.edu/~bornholt/post/memory-models.html
- https://www.linuxjournal.com/article/8211
- https://www.linuxjournal.com/article/8212

### [Acquire and Release Fences](https://preshing.com/20130922/acquire-and-release-fences/)

### [Learn the architecture - Memory Systems, Ordering, and Barriers](https://developer.arm.com/documentation/102336/0100/Load-Acquire-and-Store-Release-instructions)

可以直接下载 PDF 的

- DMB : Data Memory Barrier
- DSB : Data Synchronization Barrier

[Barrier Litmus Tests and Cookbook](https://developer.arm.com/documentation/genc007826/latest)

2. 到底在什么地方使用这个 ?


【计算机体系结构】内存一致性 - 天外飞仙的文章 - 知乎
https://zhuanlan.zhihu.com/p/694673551

https://mp.weixin.qq.com/s/s6AvLiVVkoMX4dIGpqmXYA

https://mp.weixin.qq.com/s/JxyMBHc4qPdozGK32TBTbQ
这个修改真的有这么大的性能提升吗?

https://mp.weixin.qq.com/s/wt5b5e1Y1yG1kDIf0QPsvg
字节团队写的，应该是相当清楚了:

https://mp.weixin.qq.com/s/RCpZK0pD62B6UnMP736L_w 排版很差，讲究看看

## 似乎视角需要继续抬高: 认识 rcpc rcsc 之前首先认识 release consistency

https://en.wikipedia.org/wiki/Consistency_model

## arm 获取时钟的时候，需要使用 isb 来控制 order

```c
static __always_inline u64 __arch_counter_get_cntvct(void)
{
	u64 cnt;

	asm volatile(ALTERNATIVE("isb\n mrs %0, cntvct_el0",
				 "nop\n" __mrs_s("%0", SYS_CNTVCTSS_EL0),
				 ARM64_HAS_ECV)
		     : "=r" (cnt));
	arch_counter_enforce_ordering(cnt);
	return cnt;
}
```

## 很好的整理

https://ops101.org/archives/000328.html

https://www.cis.upenn.edu/~devietti/classes/cis601-spring2016/sc_tso.pdf

https://www.zhihu.com/question/583090138/answer/2887780955

https://preshing.com/20120515/memory-reordering-caught-in-the-act/
http://www.rdrop.com/users/paulmck/scalability/paper/whymb.2010.06.07c.pdf

https://mes0903.github.io/memory/memory_model/#programming-language-memory-models

2. https://people.cs.pitt.edu/~xianeizhang/notes/cpp11_mem.html
3. wowotech
4. perfbook : chapter 14.2
- http://www.rdrop.com/users/paulmck/scalability/paper/whymb.2010.07.23a.pdf
  - 这个 ifso 写过了吗?



## 既然存在 smp_load_acquire，为什么还需要 smp_mb

核心一句话：`smp_load_acquire`/`smp_store_release` 是单向屏障，管不住 Store→Load
的重排； `smp_mb` 是全屏障，四种重排（LL、LS、SL、SS）全部禁止

### 只能用 smp_mb 的场景：先写后读（Store→Load）

最经典的就是调度器的睡眠/唤醒配对，`include/linux/sched.h` 中
`set_current_state()` 的定义：

```c
/* 等待方 */
for (;;) {
        set_current_state(TASK_UNINTERRUPTIBLE);   /* smp_store_mb(): 先 STORE __state */
        if (CONDITION)                              /* 再 LOAD CONDITION */
                break;
        schedule();
}

/* 唤醒方 */
CONDITION = 1;                    /* 先 STORE */
wake_up_state(p, ...);            /* try_to_wake_up() 里 smp_mb__after_spinlock() 后再 LOAD p->__state */
```

两侧都是「先写一个变量，再读另一个变量」。如果写操作还停在 store buffer
里就读了对方， 会出现「等待方没睡下、唤醒方也没看到它要睡」→
丢唤醒。acquire/release 在这里**结构上就不够用**： `smp_load_acquire`
只保证它之后的访问不提前到它之前，并不阻止它之前的 store 漂到它之后 （反之
release 只管之前的访问不后移）。所以内核专门造了 `smp_store_mb()`（STORE + full
barrier） 放在 `set_current_state()` 里，并与 `kernel/sched/core.c` 中
`try_to_wake_up()` 的全屏障注释配对：

> we need to ensure that CONDITION=1 done by the caller can not be reordered
> with p->state check below. This pairs with smp_store_mb() in
> set_current_state()

这类 pattern（Dekker 式双标志、等待队列、membarrier
私有屏障等）在内核里到处都是， 它们的共同点就是需要 SL 顺序。

### 只能用 smp_mb 的另一个场景：跨 CPU 的全局顺序一致性

`Documentation/memory-barriers.txt` 的 MULTICOPY ATOMICITY
一节有个专门论证：release-acquire 链**不能**补偿 非 multicopy-atomic 系统，只有
general barrier 能保证「所有 CPU 对所有操作的顺序达成一致」。 文档里 4 个 CPU
的例子中，cpu0/1/2 用 acquire/release 链、cpu3 用 `smp_mb()`，结论是链外的 cpu3
完全可能观察到与链上 CPU 矛盾的顺序——release-acquire
的排序是「局部的」，`smp_mb()` 的排序是全局的。RCU、refcount、percpu-ref 里很多
`smp_mb()` 就是靠这个性质。

### 只需要 acquire/release（用 smp_mb 属于浪费）的场景

消息传递（flag + data）模式：

```c
/* 生产者 */                          /* 消费者 */
WRITE_ONCE(buf, data);               if (smp_load_acquire(&flag)) {
smp_store_release(&flag, 1);             /* 保证看到 buf */
                                     }
```

这里需求只是「读方看到 flag=1 时必须看到 buf 的写」——Load→Load / Store→Store
的单向顺序， acquire/release 恰好够。换成 `smp_mb()`
也能对，但代价完全不同：ARM64 上 acquire/release 是
`ldar/stlr`（接近普通访存），`smp_mb` 是 `dmb sy` 全系统栅栏；x86 上 acquire
编译成普通 `mov`， `smp_mb` 是 `lock` 前缀指令（几十到上百
cycle）。锁实现（qspinlock、mutex 的 slowpath）刻意用 acquire/release 而非
mb，就是为了让临界区「吸收」相邻访存、减少流水线冲刷。

### 小结

| 需求                                   | acquire/release  | smp_mb             |
| -------------------------------------- | ---------------- | ------------------ |
| Load→Load、Store→Store（消息传递、锁） | 够，且便宜       | 能替代但浪费       |
| Store→Load（睡眠/唤醒、双标志互斥）    | **做不到**       | 唯一选择           |
| 全局顺序传播（RCU 等）                 | 链式排序是局部的 | 有累积性，唯一选择 |

简记：acquire/release
是性能优化版本的单向屏障，能用就用；凡是「写完必须立刻看到别人的写」
的协议，acquire 结构上给不了保证，只能上 `smp_mb`。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
