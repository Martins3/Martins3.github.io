## 简单问题

1. access once / data_race ? 完全不懂
2. memory barrier 需要斟酌一下
3. 观察各个系统中的 spin lock 和 mutex 才是重点



# 当这个事情结束之后，重新思考一下这个事情
- [ ] 回答在 volatile 中的问题
- [ ] 总结一个如何使用 memory bariier 的基本方法，太 TM 的恐惧了
- [ ] 如何正确的使用 RCU ，以及 Linux 内核中的 ART
- [ ] 总结 memory model 形成的原因，CPU 如何保证，对于编程者的影响
- [ ] cache coherence 和 memory model 的关系 ?
- [ ] DMA 的 cache 一致性是谁处理的

## 阅读的 blog
https://www.datadoghq.com/blog/engineering/introducing-scipio/
https://preshing.com/20120612/an-introduction-to-lock-free-programming/ : notes of `the art of concurrent programming`

## 测试库
https://github.com/awslabs/shuttle

## notifier chain
想不到一个小小的 notifier chain include/linux/notifier.h

居然就有这么复杂的考虑

```txt
	Atomic notifier chains: Chain callbacks run in interrupt/atomic
		context. Callouts are not allowed to block.
	Blocking notifier chains: Chain callbacks run in process context.
		Callouts are allowed to block.
	Raw notifier chains: There are no restrictions on callbacks,
		registration, or unregistration.  All locking and protection
		must be provided by the caller.
	SRCU notifier chains: A variant of blocking notifier chains, with
		the same restrictions.
```

## 了解下这个项目
- https://github.com/sysprog21/concurrent-programs

## mvcc 处理多核
https://en.wikipedia.org/wiki/Multiversion_concurrency_control

## 虽然 archive 了，但是看看为什么会如此
https://github.com/microsoft/vcc?tab=readme-ov-file

## 看看有趣的 race 问题
https://lore.kernel.org/netdev/5a2caf2e.4ce61c0a.5017a.575f@mx.google.com/

https://github.com/cameron314/concurrentqueue

## 为什么语言的判读是
- https://github.com/BurtonQin/lockbud

## 为什么这里非要 cpmexchg128 啊

- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=e52d58d54a321d4fe9d0ecdabe4f8774449f0d6e

- 不可以用 spinlock 吗?

## hpx
https://github.com/STEllAR-GROUP/hpx

## 看看这个文件开头的地方的 lock 规则
mm/rmap.c

## 看看这个东西
pstl/ : https://libcxx.llvm.org/Status/PSTL.html
https://github.com/oneapi-src/oneTBB

## 计划
1. 将 kernel api 都背下来
2. 将 ifso 读完
3. 将 aarch64 的 atomic 指令和 x86 都测试下，最后可以写一个测试框架，自动对比各种性能

## 用用这个接口
atomic_inc_not_zero

## 看看，这个人的积累很强
解释 write once 问题
https://www.zhihu.com/question/404513670/answer/3323448915

[Concurrency management in BPF](https://lwn.net/Articles/779120/)

https://stackoverflow.com/questions/78742654/how-does-mmaped-ebpf-map-shared-between-processes-synchronizes-operations

## harzard pointer
<!-- 0c1f14c4-cfe9-4e76-ab72-16f4278a3fdd -->

https://en.cppreference.com/w/cpp/thread 这里介绍的，还没有实现
https://melodiessim.netlify.app/intro-hazard-ptrs/
https://ckf104.github.io/posts/Thread-in-UE/ harzard pointer 是实现 lockfree 的基础
游戏引擎这么复杂的吗?


## 看看
https://lore.kernel.org/all/20240112122626.4181044-1-ming.lei@redhat.com/#r

https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=e2c27b803bb664748e090d99042ac128b3f88d92

## pony 一个新语言尝试解决并发的问题
- https://lwn.net/Articles/1001224/

## 看看
如何理解 C++11 的六种 memory order？ - 群青的回答 - 知乎
https://www.zhihu.com/question/24301047/answer/2980958431

但是 C++11 的

## 这个东西好啊
https://github.com/tokio-rs/console

## 一些思考，为什么感觉锁的问题没有一个清晰的体系

使用和分析 userfaultfd 的时候发现，似乎上锁的没有什么很强的规律的

原子性和锁不是一个事情

引用计数也是一个锁 （）

思考问题的本质是，**这个资源**不可以被同时做**什么操作**然后**操作有什么特点**。

## dpdk 还是需要看看的
- https://doc.dpdk.org/guides-22.03/prog_guide/rcu_lib.html

## atomic 的封装也是值得一看
- https://en.cppreference.com/w/c/atomic.html
- Documentation/atomic_bitops.txt
- Documentation/atomic_t.txt

## 整理一下这些东西
2. memory 和 cache 一致性 和　顺序一致性的关系
https://zh.wikipedia.org/wiki/%E7%BA%BF%E6%80%A7%E4%B8%80%E8%87%B4%E6%80%A7

https://juejin.im/post/5d328b94e51d45554877a698

线性一致性给我们这样一种错觉：并发进程的每一个操作是在调用和响应之间的某个点瞬时生效的。这也意味着并发对象的操作可以使用“先决条件”（pre-conditions）和“后置条件”（post-conditions）来定义

#### 各种数据结构的wait free 转化的基本方法是什么 ?
1. 读取，compareAndSet 失败，重复尝试 (linkedlist queue 的push 计数器)
2. help : atomic snapshot 和 通用 wait-free 构造 的方法


# 总结时间


## wait-free lock-free && obstruction free
1. https://stackoverflow.com/questions/46489805/what-is-an-example-of-obstruction-free-algorithm-that-is-not-lock-free
> The core difference between lock-freedom and obstruction-freedom is that the latter doesn't guarantee progress if two or more threads are running (but does if only one is running).

2. https://stackoverflow.com/questions/4432527/what-is-the-difference-between-lock-free-and-obstruction-free


1. 定义问题

https://stackoverflow.com/questions/37742306/examples-of-wait-lock-obstruction-freedom-and-ooc-methods
http://www.cs.swan.ac.uk/~csdavec/HPC/11LockFreedom.pdf --> ref:A
> 给出来 lock free 的定义 : 其中任意时间总有线程运行
> 1. 到底是不是 lock-free 是 obstruction free 的子集 ? (lock free 的条件下，加上其他线程都停止，显然该线程可以完成，不然该线程不能)
> 2. https://stackoverflow.com/questions/46489805/what-is-an-example-of-obstruction-free-algorithm-that-is-not-lock-free 举出了是 obstruction-free 但是不是 wait-free 的例子。

2. 真的可以带来性能提高吗 ?
ref:A 中间说了各种锁的问题，但是直觉上，从wait-free的snapshot 上，实现根本不可能效率高(比lock)

https://stackoverflow.com/questions/5680869/do-lock-free-algorithms-really-perform-better-than-their-lock-full-counterparts
> 可以确认的想法 : lock free 不停的失败的确像是新的lock
> lock free 的核心是，不在性能，取决于实现，而是防止防止出现一个thread 持有了lock 之后，其他人就没办法了


## java 语言
3. [reentrant lock](https://stackoverflow.com/questions/11821801/why-use-a-reentrantlock-if-one-can-use-synchronizedthis)
2. [atomicreference](https://stackoverflow.com/questions/3964211/when-to-use-atomicreference-in-java)

## https://zhuanlan.zhihu.com/p/20832611?from=singlemessage

## 扩展
https://book.douban.com/subject/4130141/



## 整理一下这个笔记 https://research.swtch.com/mm

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




## 什么时候应该使用 seqlock 而不是 rcu ，既然都是写多读少 ?

## slab 中的 system_has_freelist_aba 是做什么的?

## 这个必看的
A primer on memory consistency and cache coherence

## 关于 lock 的部分可以看看
https://news.ycombinator.com/item?id=45402820

```txt
Benchmarks
Four Horsemen of Performance
  Locks and Mutexes
  Memory Allocations
  Atomics and CAS
  Alignment
Comparing APIs
  Fork Union
  Rayon
  Taskflow
Conclusions & Observations
```

## rseq 机制
https://stackoverflow.com/questions/76102375/what-are-rseqs-restartable-sequences-and-how-to-use-them

```txt
commit d7822b1e24f2df5df98c76f0e94a5416349ff759
Author: Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
Date:   Sat Jun 2 08:43:54 2018 -0400

    rseq: Introduce restartable sequences system call

    Expose a new system call allowing each thread to register one userspace
    memory area to be used as an ABI between kernel and user-space for two
    purposes: user-space restartable sequences and quick access to read the
    current CPU number value from user-space.

    * Restartable sequences (per-cpu atomics)

    Restartables sequences allow user-space to perform update operations on
    per-cpu data without requiring heavy-weight atomic operations.

    The restartable critical sections (percpu atomics) work has been started
    by Paul Turner and Andrew Hunter. It lets the kernel handle restart of
    critical sections. [1] [2] The re-implementation proposed here brings a
    few simplifications to the ABI which facilitates porting to other
    architectures and speeds up the user-space fast path.

    Here are benchmarks of various rseq use-cases.

    Test hardware:

    arm32: ARMv7 Processor rev 4 (v7l) "Cubietruck", 2-core
    x86-64: Intel E5-2630 v3@2.40GHz, 16-core, hyperthreading

    The following benchmarks were all performed on a single thread.

    * Per-CPU statistic counter increment

                    getcpu+atomic (ns/op)    rseq (ns/op)    speedup
    arm32:                344.0                 31.4          11.0
    x86-64:                15.3                  2.0           7.7

    * LTTng-UST: write event 32-bit header, 32-bit payload into tracer
                 per-cpu buffer

                    getcpu+atomic (ns/op)    rseq (ns/op)    speedup
    arm32:               2502.0                 2250.0         1.1
    x86-64:               117.4                   98.0         1.2

    * liburcu percpu: lock-unlock pair, dereference, read/compare word

                    getcpu+atomic (ns/op)    rseq (ns/op)    speedup
    arm32:                751.0                 128.5          5.8
    x86-64:                53.4                  28.6          1.9

    * jemalloc memory allocator adapted to use rseq

    Using rseq with per-cpu memory pools in jemalloc at Facebook (based on
    rseq 2016 implementation):

    The production workload response-time has 1-2% gain avg. latency, and
    the P99 overall latency drops by 2-3%.

    * Reading the current CPU number

    Speeding up reading the current CPU number on which the caller thread is
    running is done by keeping the current CPU number up do date within the
    cpu_id field of the memory area registered by the thread. This is done
    by making scheduler preemption set the TIF_NOTIFY_RESUME flag on the
    current thread. Upon return to user-space, a notify-resume handler
    updates the current CPU value within the registered user-space memory
    area. User-space can then read the current CPU number directly from
    memory.

    Keeping the current cpu id in a memory area shared between kernel and
    user-space is an improvement over current mechanisms available to read
    the current CPU number, which has the following benefits over
    alternative approaches:

    - 35x speedup on ARM vs system call through glibc
    - 20x speedup on x86 compared to calling glibc, which calls vdso
      executing a "lsl" instruction,
    - 14x speedup on x86 compared to inlined "lsl" instruction,
    - Unlike vdso approaches, this cpu_id value can be read from an inline
      assembly, which makes it a useful building block for restartable
      sequences.
    - The approach of reading the cpu id through memory mapping shared
      between kernel and user-space is portable (e.g. ARM), which is not the
      case for the lsl-based x86 vdso.

    On x86, yet another possible approach would be to use the gs segment
    selector to point to user-space per-cpu data. This approach performs
    similarly to the cpu id cache, but it has two disadvantages: it is
    not portable, and it is incompatible with existing applications already
    using the gs segment selector for other purposes.

    Benchmarking various approaches for reading the current CPU number:

    ARMv7 Processor rev 4 (v7l)
    Machine model: Cubietruck
    - Baseline (empty loop):                                    8.4 ns
    - Read CPU from rseq cpu_id:                               16.7 ns
    - Read CPU from rseq cpu_id (lazy register):               19.8 ns
    - glibc 2.19-0ubuntu6.6 getcpu:                           301.8 ns
    - getcpu system call:                                     234.9 ns

    x86-64 Intel(R) Xeon(R) CPU E5-2630 v3 @ 2.40GHz:
    - Baseline (empty loop):                                    0.8 ns
    - Read CPU from rseq cpu_id:                                0.8 ns
    - Read CPU from rseq cpu_id (lazy register):                0.8 ns
    - Read using gs segment selector:                           0.8 ns
    - "lsl" inline assembly:                                   13.0 ns
    - glibc 2.19-0ubuntu6 getcpu:                              16.6 ns
    - getcpu system call:                                      53.9 ns

    - Speed (benchmark taken on v8 of patchset)

    Running 10 runs of hackbench -l 100000 seems to indicate, contrary to
    expectations, that enabling CONFIG_RSEQ slightly accelerates the
    scheduler:

    Configuration: 2 sockets * 8-core Intel(R) Xeon(R) CPU E5-2630 v3 @
    2.40GHz (directly on hardware, hyperthreading disabled in BIOS, energy
    saving disabled in BIOS, turboboost disabled in BIOS, cpuidle.off=1
    kernel parameter), with a Linux v4.6 defconfig+localyesconfig,
    restartable sequences series applied.

    * CONFIG_RSEQ=n

    avg.:      41.37 s
    std.dev.:   0.36 s

    * CONFIG_RSEQ=y

    avg.:      40.46 s
    std.dev.:   0.33 s

    - Size

    On x86-64, between CONFIG_RSEQ=n/y, the text size increase of vmlinux is
    567 bytes, and the data size increase of vmlinux is 5696 bytes.

    [1] https://lwn.net/Articles/650333/
    [2] http://www.linuxplumbersconf.org/2013/ocw/system/presentations/1695/original/LPC%20-%20PerCpu%20Atomics.pdf

    Signed-off-by: Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
    Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
    Acked-by: Peter Zijlstra (Intel) <peterz@infradead.org>
    Cc: Joel Fernandes <joelaf@google.com>
    Cc: Catalin Marinas <catalin.marinas@arm.com>
    Cc: Dave Watson <davejwatson@fb.com>
    Cc: Will Deacon <will.deacon@arm.com>
    Cc: Andi Kleen <andi@firstfloor.org>
    Cc: "H . Peter Anvin" <hpa@zytor.com>
    Cc: Chris Lameter <cl@linux.com>
    Cc: Russell King <linux@arm.linux.org.uk>
    Cc: Andrew Hunter <ahh@google.com>
    Cc: Michael Kerrisk <mtk.manpages@gmail.com>
    Cc: "Paul E . McKenney" <paulmck@linux.vnet.ibm.com>
    Cc: Paul Turner <pjt@google.com>
    Cc: Boqun Feng <boqun.feng@gmail.com>
    Cc: Josh Triplett <josh@joshtriplett.org>
    Cc: Steven Rostedt <rostedt@goodmis.org>
    Cc: Ben Maurer <bmaurer@fb.com>
    Cc: Alexander Viro <viro@zeniv.linux.org.uk>
    Cc: linux-api@vger.kernel.org
    Cc: Andy Lutomirski <luto@amacapital.net>
    Cc: Andrew Morton <akpm@linux-foundation.org>
    Cc: Linus Torvalds <torvalds@linux-foundation.org>
    Link: http://lkml.kernel.org/r/20151027235635.16059.11630.stgit@pjt-glaptop.roam.corp.google.com
    Link: http://lkml.kernel.org/r/20150624222609.6116.86035.stgit@kitami.mtv.corp.google.com
    Link: https://lkml.kernel.org/r/20180602124408.8430-3-mathieu.desnoyers@efficios.com
```

## 无锁队列
https://github.com/asynchronics/tachyonix

## spin lock
https://mp.weixin.qq.com/s/imPaORfnt--iVVJdRAQUDQ

## 这个就真的很有趣了
__try_cmpxchg_user : uaccess 加上 cmpxchg 的做法

## bitmap 和 concurrent 问题

这些东西和
```txt
builtin	等价 CPU 指令	用途
__builtin_ctz(x)	TZCNT/BSF	找最低位 1 的 index
__builtin_clz(x)	LZCNT/BSR	找最高位 1 的 index
__builtin_ffs(x)	BSF		找最低位 1（1-based）
```

这个可以同时使用吗?
```txt
static __always_inline bool bitlock_test_and_set(int lock_bit,
						 volatile uint8_t *addr)
{
	assert(lock_bit < BYTE_BIT);
	uint8_t mask = (uint8_t)(1U << lock_bit);
	uint8_t old = __atomic_fetch_or(addr, mask, __ATOMIC_ACQUIRE);
	return !!(old & mask);
}

static __always_inline void bitlock_clear(int lock_bit, volatile uint8_t *addr)
{
	assert(lock_bit < BYTE_BIT);
	uint8_t mask = (uint8_t) ~(1U << lock_bit);
	__atomic_fetch_and(addr, mask, __ATOMIC_RELEASE);
}
```

## 质量未知，可以读读
https://news.ycombinator.com/item?id=46186997

## 无论是否进入主线，这个机制都是需要看看的
https://lwn.net/Articles/1021761/

## 理解一下为什么 cgroup 还有自己的 refcnt 的?
include/linux/cgroup_refcnt.h

## 很好
https://www.kernel.org/doc/html/latest/locking/preempt-locking.html


## 好几好哦
/home/martins3/data/vn/code/src/concurrent/README.md

## page table 的 lock 和 fs 目录的 lock

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
