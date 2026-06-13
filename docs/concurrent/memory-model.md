## acquire and release
<!-- c3d3782f-e682-4b81-8e24-2dafd92d2b04 -->

> 参考 Deepseeek ，这个解释仔细检查过，写的相当好
>
> CPU 0 (写者/Producer):
>
> 在“数据区”写下 “Hello World”。
>
> 执行 RELEASE 操作：在“锁标志”区擦掉“占用”标志。
>
> RELEASE 语义就像一道向上的栅栏。它保证：在它之前的所有内存写入操作（比如写 "Hello World"），都必须在这条 RELEASE 指令完成之前，对所有其他CPU可见。
>
> CPU 1 (读者/Consumer):
>
> 执行 ACQUIRE 操作：不断检查“锁标志”，直到发现“占用”标志被擦掉。
>
> ACQUIRE 语义就像一道向下的栅栏。它保证：在它之后的所有内存读取操作（比如读 "Hello World"），都必须在这条 ACQUIRE 指令完成之后才能执行。

这么想，release 和 acquire 就像是锁的语义。当 writer 做完事情了之后，就 release 。reader 需要开始读，那么就先 acquire 。
而且 memory model 似乎就是需要处理这个场景，write 需要保证 reader 可以正确的看到。

(如果只有这个场景，那么为什么还有这么多的 CPU 定义的 model ?)
(或者说，cpp 为什么定义出来这么多的 model )

## release consume 暂时不用吧
https://stackoverflow.com/questions/65336409/what-does-memory-order-consume-really-do

## memory model
- 简短有力的分析: https://zhuanlan.zhihu.com/p/41872203
- https://mp.weixin.qq.com/s/s6AvLiVVkoMX4dIGpqmXYA

- Memory Barriers: a Hardware View for Software Hackers
- What every systems programmer should know about concurrency

- 现在才知道在 C++ 11 才开始定义的 memory model 的
- https://github.com/LearningOS/aos-lectures/blob/master/lec12/slide-12-01.tex
   - 但是不知道这个 slides 如何编译



## 相关资料
- https://stackoverflow.com/questions/38447226/atomicity-on-x86
- https://stackoverflow.com/questions/39393850/can-num-be-atomic-for-int-num

- https://news.ycombinator.com/item?id=32520365
- [ ] sys/mccc 的 A Primer on Memory Consistency and Cache Coherence 可以重新看看

## 问题
- [ ] 能不能稳定的复现，或者制作出来这些 memory model 的效果
- [ ] 各种使用案例

- [ ] GPU 中的 memory model 是做什么的?
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


## leveldb 中的 skiplist 中的 cpp 11 的 memory model

## memory consistency
- [ ] https://zhuanlan.zhihu.com/cpu-cache

当分析那么多窒息的例子，都是由于同时访问相同位置的内存，但是访问相同位置的内存的时候，难道不是采用 lock 保护的吗 ? smp_mb 的使用位置和实现方式是什么 ?

// 教程，也许可以阅读一下 :
- https://www.cs.utexas.edu/~bornholt/post/memory-models.html
- https://www.linuxjournal.com/article/8211
- https://www.linuxjournal.com/article/8212

## ARM 的文档
- https://developer.arm.com/documentation/den0024/a/Memory-Ordering

[lock would enforce a memory barrier, like an mfence](https://stackoverflow.com/questions/42820121/why-cannot-the-load-part-of-the-atomic-rmw-instruction-pass-the-earlier-store-to)
> 1. Loads may be reordered with older stores to different locations but not with older stores to the same location
> 2. Locked instructions have a total order

https://www.arangodb.com/2021/02/cpp-memory-model-migrating-from-x86-to-arm/


## 只有 store-load 存在乱序
分别是 store-store，store-load，load-load 和 load-store。TSO 模型中，只存在 store-load 存在乱序，另外 3 种内存操作不存在乱序。

https://preshing.com/20120930/weak-vs-strong-memory-models/

https://preshing.com/20120710/memory-barriers-are-like-source-control-operations/

## 这个
- https://www.cl.cam.ac.uk/~pes20/ppc-supplemental/test7.pdf
- https://acg.cis.upenn.edu/rg_papers/memo-493.pdf

## 这个讨论有意思的啊
https://github.com/ziglang/zig/issues/6396

## 有趣的问题
https://bartoszmilewski.com/2008/11/05/who-ordered-memory-fences-on-an-x86/

## 如果 cacheline 的大小那么大，lock 为什么可以只是 lock 一个字节 ?

## 务必仔细理解其中的关于 cache 的 linearize 的含义
https://stackoverflow.com/questions/69925465/how-does-the-x86-tso-memory-consistency-model-work-when-some-of-the-stores-being

就是这个没有理解，导致假设一直有问题，是在是关键

## 这种文摘需要看看
https://www.scylladb.com/2018/02/15/memory-barriers-seastar-linux/


## try_to_wake_up 上有大量的注释来分析为什么
lock 相关的保证

从这里看，也不得不说，内核中才是处理各种 lock , memory model 最复杂的地方。
尤其是 scheduler 的作者。

## 参考一下
【计算机体系结构】内存一致性 - 天外飞仙的文章 - 知乎
https://zhuanlan.zhihu.com/p/694673551

https://mp.weixin.qq.com/s/s6AvLiVVkoMX4dIGpqmXYA

https://mp.weixin.qq.com/s/JxyMBHc4qPdozGK32TBTbQ 这个修改真的有这么大的性能提升吗?

https://mp.weixin.qq.com/s/wt5b5e1Y1yG1kDIf0QPsvg 字节团队写的，应该是相当清楚了:

https://mp.weixin.qq.com/s/RCpZK0pD62B6UnMP736L_w 排版很差，讲究看看

## qemu/kvm 中的 dirty ring 是处理 memory model 一个很好的话题

dirty_gfn_set_collected

## [ ] 分析下 ARM 的 memory model，尤其是对比普通 arm 和 Mac arm 的区别

## Documentation/atomic_bitops.txt 结尾中的

```txt
Like with atomic_t, the rule of thumb is:

 - non-RMW operations are unordered;

 - RMW operations that have no return value are unordered;

 - RMW operations that have a return value are fully ordered.

 - RMW operations that are conditional are fully ordered.
```

为什么是这个样子的

## 既然存在 smp_load_acquire ，那么为什么还需要 smp_mb


## 这个用这个改造 改造一下
docs/concurrent/code/cpp-std/memory-model/ordering/2-acquire-release.cpp

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

### [Acquire and Release Fences](https://preshing.com/20130922/acquire-and-release-fences/)

### [Learn the architecture - Memory Systems, Ordering, and Barriers](https://developer.arm.com/documentation/102336/0100/Load-Acquire-and-Store-Release-instructions)

可以直接下载 PDF 的

- DMB : Data Memory Barrier
- DSB : Data Synchronization Barrier

[Barrier Litmus Tests and Cookbook](https://developer.arm.com/documentation/genc007826/latest)

2. 到底在什么地方使用这个 ?

## doc
2. https://people.cs.pitt.edu/~xianeizhang/notes/cpp11_mem.html
3. wowotech
4. perfbook : chapter 14.2

## 对比 x86 arm 和 risc-v

arch/x86/include/asm/barrier.h


x86 和 arm 实现 barrier 都是如此, rsic-v 也是
```c
/* Optimization barrier */
#ifndef barrier
/* The "volatile" is due to gcc bugs */
# define barrier() __asm__ __volatile__("": : :"memory")
#endif
```

- [ ] 但是为什么 barrier 要如此实现，没有更好的办法吗?
  - 这样实现充分吗?

2. x86 中只有 smp_mb 的实现不同

1. smp_store_release 的实现是不是有点出乎意料了
```c
#define smp_store_release(p, v) do { kcsan_release(); __smp_store_release(p, v); } while (0)

#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	barrier();							\
	WRITE_ONCE(*p, v);						\
} while (0)
```

但是 arm 的实现中: 9 个 mb 和 smp_store_release 的实现都各自不同

```c
#define __smp_store_release(p, v)					\
do {									\
	typeof(p) __p = (p);						\
	union { __unqual_scalar_typeof(*p) __val; char __c[1]; } __u =	\
		{ .__val = (__force __unqual_scalar_typeof(*p)) (v) };	\
	compiletime_assert_atomic_type(*p);				\
	kasan_check_write(__p, sizeof(*p));				\
	switch (sizeof(*p)) {						\
	case 1:								\
		asm volatile ("stlrb %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u8 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 2:								\
		asm volatile ("stlrh %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u16 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 4:								\
		asm volatile ("stlr %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u32 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 8:								\
		asm volatile ("stlr %x1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u64 *)__u.__c)		\
				: "memory");				\
		break;							\
	}								\
} while (0)

#define __smp_load_acquire(p)						\
({									\
	union { __unqual_scalar_typeof(*p) __val; char __c[1]; } __u;	\
	typeof(p) __p = (p);						\
	compiletime_assert_atomic_type(*p);				\
	kasan_check_read(__p, sizeof(*p));				\
	switch (sizeof(*p)) {						\
	case 1:								\
		asm volatile ("ldarb %w0, %1"				\
			: "=r" (*(__u8 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 2:								\
		asm volatile ("ldarh %w0, %1"				\
			: "=r" (*(__u16 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 4:								\
		asm volatile ("ldar %w0, %1"				\
			: "=r" (*(__u32 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 8:								\
		asm volatile ("ldar %0, %1"				\
			: "=r" (*(__u64 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	}								\
	(typeof(*p))__u.__val;						\
})
```

https://stackoverflow.com/questions/65466840/arm-stlr-memory-ordering-semantics



### [ ] 一个对比

在 arm 中，dsb 和 dmb 是分别给 rmb 和 dma_rmb 使用的

```c
#define __mb()		dsb(sy)
#define __rmb()		dsb(ld)
#define __wmb()		dsb(st)

#define __dma_mb()	dmb(osh)
#define __dma_rmb()	dmb(oshld)
#define __dma_wmb()	dmb(oshst)
```

在 x86 中
```c
#define __dma_rmb()	barrier()
#define __dma_wmb()	barrier()

#define __smp_mb()	asm volatile("lock addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")

#define __smp_rmb()	dma_rmb()
#define __smp_wmb()	barrier()
#define __smp_store_mb(var, value) do { (void)xchg(&var, value); } while (0)
```

### 如何理解 __smp_store_mb

例如 arm64 中是:
```c
#define __smp_store_mb(var, value)  do { WRITE_ONCE(var, value); __smp_mb(); } while (0)
```

x86 中是
```c
#define __smp_store_mb(var, value) do { (void)xchg(&var, value); } while (0)
```

Documentation/memory-barriers.txt
```txt
There are some more advanced barrier functions:

 (*) smp_store_mb(var, value)

     This assigns the value to the variable and then inserts a full memory
     barrier after it.  It isn't guaranteed to insert anything more than a
     compiler barrier in a UP compilation.
```




## 其他的资料
- http://www.rdrop.com/users/paulmck/scalability/paper/whymb.2010.07.23a.pdf
  - 这个 ifso 写过了吗?


## [Random ASCII – tech blog of Bruce Dawson](https://randomascii.wordpress.com/2020/11/29/arm-and-lock-free-programming/)


## [The AArch64 processor (aka arm64), part 14: Barriers](https://devblogs.microsoft.com/oldnewthing/20220812-00/?p=106968)

```txt
    dmb     ish     ; data memory barrier
    dsb     ish     ; data synchronization barrier
    isb     sy      ; instruction synchronization barrier
```

```txt
The data memory barrier ensures that all preceding writes are issued before any subsequent memory operations (including speculative memory access). In acquire/release terms, it is a full barrier. The instruction does not stall execution; it just tells the memory controller to preserve externally-visible ordering. This is probably the only barrier you will ever seen in user-mode code.

The data synchronization barrier is a data memory barrier, but with the additional behavior of stalling until all outstanding writes have completed. This is typically used before changing memory mappings, such as during context switches, to ensure that any outstanding writes complete to the original memory before it gets unmapped.

The instruction synchronization barrier flushes instruction prefetch. This is typically used if you have generated new code, say by jitting it or paging it in from disk.
```
对于 dmb dsb isb 总结的简单清晰，但是:

1. 但是 x86 都是直接自动简化掉的，不敢现象 x86 的硬件如何实现的
例如 x86 的是:
```txt
#define __dma_wmb()	barrier()
```
AArch64 是
```txt
#define __dma_wmb()	dmb(oshst)
```

2. isb 的问题
  - 内核中动态生成代码的不少啊，但是 isb() 的调用位置全部都在 arm 架构相关的代码中. 例如内核模块加载之后，是需要 isb 的吧，或者 bpf 的执行

```txt
    ; sequential consistency interlocked increment and
    ; acquire-release interlocked increment
@@: ldaxr   w8, [x0]                ; load acquire from x0
    add     w8, w8, 1               ; increment
    stlxr   w9, w8, [x0]            ; store it back with release
    cbnz    @B                      ; if failed, try again

    ; acquire-only interlocked increment
@@: ldaxr   w8, [x0]                ; load acquire from x0
    add     w8, w8, 1               ; increment
    stxr   w9, w8, [x0]             ; store it back (no release)
    cbnz    @B                      ; if failed, try again

    ; release-only interlocked increment
@@: ldxr    w8, [x0]                ; load (no acquire) from x0
    add     w8, w8, 1               ; increment
    stlxr   w9, w8, [x0]            ; store it back with release
    cbnz    @B                      ; if failed, try again

    ; relaxed interlocked increment
@@: ldxr    w8, [x0]                ; load from x0
    add     w8, w8, 1               ; increment
    stxr    w9, w8, [x0]            ; store it back
    cbnz    @B                      ; if failed, try again
```
问题:
1. 这就是实现 lock 的方法吗?
2. 如何理解 x : exclusive
3. 如果实现 lock ，后面的 acquire-only interlocked increment
release-only interlocked increment
relaxed interlocked increment
应该是没有什么意义吧

## 如何理解 arm 的 synchronization domain

## [ ] 既然提交代码的时候需要说明为什么使用 memory-barriers ，
那么为什么不去说明 smp_load_acquire  smp_store_release 的使用
恐怕是有的吧


## 有趣的读物

https://ipads.se.sjtu.edu.cn/_media/publications/liuppopp20.pdf


## 有趣的讨论，QEMU
- https://github.com/utmapp/UTM/issues/5460
- https://github.com/utmapp/UTM/issues/2366

> Latest A-series chips and M1 support ARM “Release Consistency sequentially consistent“ and “Release Consistent processor consistent” instructions. By emitting them in JIT in place of standard STR instructions, we can enable error-free multicore emulation of x86_64 on ARM64. RCpc seems to be the better of the two.

使用 rcpc 和 rcsc 可以让 arm 来模拟 x86 ? 不太可能吧!

## 似乎视角需要继续抬高: 认识 rcpc rcsc 之前首先认识 release consistency
https://en.wikipedia.org/wiki/Consistency_model

## 有趣的讨论
https://lore.kernel.org/linux-kernel/Zf4cP6lx7LHmt3dz@boqun-archlinux/T/#r42e04760aa8e0abac493b2e69a8979339e20249a

## 有趣的 commit
76e079fefc8f62bd9b2cd2950814d1ee806e31a5

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

## try_to_wake_up 为什么大量使用 smp_mb__after_spinlock

## end_page_writeback 为什么使用 smp_mb__after_atomic


## 看看这个
https://www.cis.upenn.edu/~devietti/classes/cis601-spring2016/sc_tso.pdf

## 为什么 x86 需要三个 fence ?

既然只有 store load 的问题


## asahi linux 中的结果

```txt
Oct 13 08:00:00 fedora kernel: smp: Brought up 1 node, 8 CPUs
Oct 13 08:00:00 fedora kernel: SMP: Total of 8 processors activated.
Oct 13 08:00:00 fedora kernel: CPU: All CPU(s) started at EL2
Oct 13 08:00:00 fedora kernel: CPU features: detected: Branch Target Identification
Oct 13 08:00:00 fedora kernel: CPU features: detected: ARMv8.4 Translation Table Level
Oct 13 08:00:00 fedora kernel: CPU features: detected: Data cache clean to the PoU not required for I/D coherence
Oct 13 08:00:00 fedora kernel: CPU features: detected: Common not Private translations
Oct 13 08:00:00 fedora kernel: CPU features: detected: CRC32 instructions
Oct 13 08:00:00 fedora kernel: CPU features: detected: Data cache clean to Point of Deep Persistence
Oct 13 08:00:00 fedora kernel: CPU features: detected: Data cache clean to Point of Persistence
Oct 13 08:00:00 fedora kernel: CPU features: detected: Data independent timing control (DIT)
Oct 13 08:00:00 fedora kernel: CPU features: detected: E0PD
Oct 13 08:00:00 fedora kernel: CPU features: detected: Enhanced Counter Virtualization
Oct 13 08:00:00 fedora kernel: CPU features: detected: Enhanced Virtualization Traps
Oct 13 08:00:00 fedora kernel: CPU features: detected: Fine Grained Traps
Oct 13 08:00:00 fedora kernel: CPU features: detected: Generic authentication (IMP DEF algorithm)
Oct 13 08:00:00 fedora kernel: CPU features: detected: RCpc load-acquire (LDAPR)
Oct 13 08:00:00 fedora kernel: CPU features: detected: LSE atomic instructions
Oct 13 08:00:00 fedora kernel: CPU features: detected: Privileged Access Never
Oct 13 08:00:00 fedora kernel: CPU features: detected: RAS Extension Support
Oct 13 08:00:00 fedora kernel: CPU features: detected: Speculation barrier (SB)
Oct 13 08:00:00 fedora kernel: CPU features: detected: Stage-2 Force Write-Back
Oct 13 08:00:00 fedora kernel: CPU features: detected: TLB range maintenance instructions
Oct 13 08:00:00 fedora kernel: CPU features: detected: Speculative Store Bypassing Safe (SSBS)
Oct 13 08:00:00 fedora kernel: alternatives: applying system-wide alternatives
Oct 13 08:00:00 fedora kernel: CPU features: detected: ACTLR virtualization (architectural?)
Oct 13 08:00:00 fedora kernel: CPU features: detected: TSO memory model (Apple)
Oct 13 08:00:00 fedora kernel: Memory: 15877984K/16296480K available (22400K kernel code, 6098K rwdata, 18896K rodata, 14464K init, 10371K bss, 334832K reserved, 65536K cma-reserved)
```

## pual 的文章，memory model ，一篇足以
https://www.zhihu.com/question/583090138/answer/2887780955

经典的翻译:
https://mes0903.github.io/memory/memory_model/#programming-language-memory-models

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
