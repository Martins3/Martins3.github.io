# 内核文档

## memory-barriers.txt
终结一切的神:
https://www.kernel.org/doc/Documentation/memory-barriers.txt

```txt
> (6) RELEASE operations.
>
>      This also acts as a one-way permeable barrier.  It guarantees that all
>      memory operations before the RELEASE operation will appear to happen
>      before the RELEASE operation with respect to the other components of the
>      system. RELEASE operations include UNLOCK operations and
>      smp_store_release() operations.
>
>      Memory operations that occur after a RELEASE operation may appear to
>      happen before it completes.
>
>      The use of ACQUIRE and RELEASE operations generally precludes the need
>      for other sorts of memory barrier.  In addition, a RELEASE+ACQUIRE pair is
>      -not- guaranteed to act as a full memory barrier.  However, after an
>      ACQUIRE on a given variable, all memory accesses preceding any prior
>      RELEASE on that same variable are guaranteed to be visible.  In other
>      words, within a given variable's critical section, all accesses of all
>      previous critical sections for that variable are guaranteed to have
>      completed.
>
>      This means that ACQUIRE acts as a minimal "acquire" operation and
>      RELEASE acts as a minimal "release" operation.
```

```txt
It is tempting to argue that there in fact is ordering because the
compiler cannot reorder volatile accesses and also cannot reorder
the writes to 'b' with the condition.  Unfortunately for this line
of reasoning, the compiler might compile the two writes to 'b' as
conditional-move instructions, as in this fanciful pseudo-assembly
language:
```

READ_ONCE / WRITE_ONCE 和 barrier 是不同的 compiler 级别的告示 ，换言之:

这种会被 compiler order 吗?
```txt
	q = READ_ONCE(a);
	WRITE_ONCE(b, 1);
```
不会(compiler cannot reorder volatile accesses)，但是，CPU 会来 order

```txt
General barriers pair with each other, though they also pair with most
other types of barriers, albeit without multicopy atomicity.
```

```txt
A write barrier pairs
with an address-dependency barrier, a control dependency, an acquire barrier,
a release barrier, a read barrier, or a general barrier.

Similarly a
read barrier, control dependency, or an address-dependency barrier pairs
with a write barrier, an acquire barrier, a release barrier, or a
general barrier:
```

```txt
General barriers can compensate not only for non-multicopy atomicity,
but can also generate additional ordering that can ensure that -all-
CPUs will perceive the same order of -all- operations.  In contrast, a
chain of release-acquire pairs do not provide this additional ordering,
which means that only those CPUs on the chain are guaranteed to agree
on the combined order of the accesses.
```

> 1520 附近，连续六个等于，没有耐心了

```txt
 (*) The compiler is within its rights to reorder loads and stores
     to the same variable, and in some cases, the CPU is within its
     rights to reorder loads to the same variable.  This means that
     the following code:

	a[0] = x;
	a[1] = x;

     Might result in an older value of x stored in a[1] than in a[0].
     Prevent both the compiler and the CPU from doing this as follows:

	a[0] = READ_ONCE(x);
	a[1] = READ_ONCE(x);

     In short, READ_ONCE() and WRITE_ONCE() provide cache coherence for
     accesses from multiple CPUs to a single variable.
```
理解下，
1. 这里 READ_ONCE 其实是说提供了防止 compiler 的 order 的功能 ?
2. 这里为什么可以阻碍 CPU 的 order 吗? 不太可能吧，编译之后有区别 ?

```txt
     This effect could also be achieved using barrier(), but READ_ONCE()
     and WRITE_ONCE() are more selective:  With READ_ONCE() and
     WRITE_ONCE(), the compiler need only forget the contents of the
     indicated memory locations, while with barrier() the compiler must
     discard the value of all memory locations that it has currently
     cached in any machine registers.  Of course, the compiler must also
     respect the order in which the READ_ONCE()s and WRITE_ONCE()s occur,
     though the CPU of course need not do so.
```


没有太搞懂这里在说什么，到底导致了什么位置啊?
```txt
 (*) For aligned memory locations whose size allows them to be accessed
     with a single memory-reference instruction, prevents "load tearing"
     and "store tearing," in which a single large access is replaced by
     multiple smaller accesses.  For example, given an architecture having
     16-bit store instructions with 7-bit immediate fields, the compiler
     might be tempted to use two 16-bit store-immediate instructions to
     implement the following 32-bit store:

```

```txt
     Note that the dma_*() barriers do not provide any ordering guarantees for
     accesses to MMIO regions.  See the later "KERNEL I/O BARRIER EFFECTS"
     subsection for more information about I/O accessors and MMIO ordering.
```

```txt
LOCK ACQUISITION FUNCTIONS
```
这一节分析了内核中几个 locking 的基础设施为什么是自带 barrier 的。但是，他上来就用 acquire 和 release 分析，
只能说视角很高:
- [ ] 但是，为什么 locking 是含有 acquire 和 release 的语义的

```txt
 (1) ACQUIRE operation implication:

     Memory operations issued after the ACQUIRE will be completed after the
     ACQUIRE operation has completed.

     Memory operations issued before the ACQUIRE may be completed after
     the ACQUIRE operation has completed.
```
acquire - 上锁 - 后续的不可向前


- [ ] 是因为实现的过程中就携带了 barrier() 的调用 ，还是说因为实现原子语句自带了 barrier ？

```txt
It might appear that this reordering could introduce a deadlock.
However, this cannot happen because if such a deadlock threatened,
the RELEASE would simply complete, thereby avoiding the deadlock.
```


这一章到底在说什么?
```txt
INTER-CPU ACQUIRING BARRIER EFFECTS
```




```txt
VIRTUAL MACHINE GUESTS
```
这里明显和上下文不是统一的，过于简单，我猜是因为 host 和 guest 分别在两个 CPU 中，所以存在一些同步问题。


```txt
WHERE ARE MEMORY BARRIERS NEEDED?
```
中的 interrupts 那一个章节解释没看懂，中断屏蔽了，内部的还可以传递出来，还是说，只是针对于 device 才会 reorder。



multicopy
```txt
  (*) Control dependencies do -not- provide multicopy atomicity.  If you
      need all the CPUs to see a given store at the same time, use smp_mb().

General barriers pair with each other, though they also pair with most
other types of barriers, albeit without multicopy atomicity.
```

继续扩展
```txt
CIRCULAR BUFFERS
----------------

Memory barriers can be used to implement circular buffering without the need
of a lock to serialise the producer with the consumer.  See:

	Documentation/core-api/circular-buffers.rst

for details.
```

## 工具

### tools/memory-model/

### tools/testing/selftests/rseq/

## [Documentation/locking/](https://docs.kernel.org/locking/index.html)
<!-- ac2b0a4b-fc70-48c8-9bd8-444d0f12522a -->

### Documentation/locking/locktypes.rst

将锁分为三个类型:

- Sleeping locks
- CPU local locks
- Spinning locks

分析了 PREEMPT_RT 的影响。

### Documentation/locking/spinlocks.rst

一共三个 lession

1. 介绍 spinlock 是最简单的 lock
2. 引出 rwlock
3. spin lock 中的中断

### Documentation/locking/lockstat.rst


### Documentation/locking/lockdep-design.rst
https://docs.kernel.org/locking/lockdep-design.html

https://lwn.net/Articles/321663/

https://stackoverflow.com/questions/20892822/how-to-use-lockdep-feature-in-linux-kernel-for-deadlock-detection

## [Documentation/memory-barriers.txt](https://docs.kernel.org/core-api/wrappers/memory-barriers.html)

记录放到 docs/concurrent/kernel-api-memory_barrier.md 中

## [Documentation/RCU/](https://docs.kernel.org/RCU/)

### [What is RCU? -- "Read, Copy, Update"](https://docs.kernel.org/RCU/whatisRCU.html)

好家伙，直接指向了超过 lwn 链接，同时包含了两个 youtube 链接

### [A Tour Through RCU’s Requirements](https://docs.kernel.org/RCU/Design/Requirements/Requirements.html)

### [A Tour Through TREE_RCU's Data Structures](https://www.kernel.org/doc/html/latest/RCU/Design/Data-Structures/Data-Structures.html)

The purpose of this combining tree is to allow per-CPU events such as quiescent states, dyntick-idle transitions, and CPU hotplug operations to be processed efficiently and scalably.

Quiescent states are recorded by the per-CPU `rcu_data` structures, and other events are recorded by the leaf-level `rcu_node` structures.

All of these events are combined at each level of the tree until finally grace periods are completed at the tree’s root rcu_node structure. A grace period can be completed at the root once every CPU (or, in the case of CONFIG_PREEMPT_RCU, task) has passed through a quiescent state.

### Documentation/kernel-hacking/locking.rst

## 领域相关

- [KVM Lock Overview](https://docs.kernel.org/virt/kvm/locking.html)
  - 分析记录到 docs/kvm/lock.md
- Documentation/filesystems/locks.rst Documentation/filesystems/locking.rst
  - 分析记录到 docs/kernel/fs/fs-lock.md

## Kernel Hacking Guides
<!-- ff83637f-410e-4090-958a-d9ba3184f86f -->

https://www.kernel.org/doc/html/latest/kernel-hacking/index.html
- hacking 入门 : https://www.kernel.org/doc/html/latest/kernel-hacking/hacking.html
- 锁 : https://www.kernel.org/doc/html/latest/kernel-hacking/locking.html

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
