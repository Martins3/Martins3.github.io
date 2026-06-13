## Paul 维护的 RCU 文档

- https://docs.google.com/document/d/1X0lThx8OK0ZgLMqVoXiR4ZrGURHrXK6NyLRbeXe3Xac/edit#heading=h.dw5xxrskdsu2
- https://docs.google.com/document/d/1GCdQC8SDbb54W1shjEXqGZ0Rq8a6kIeYutdSIajfpLA/edit#heading=h.ytgz5i5df43s
- https://docs.google.com/document/d/104c8ewoCTw6_4OJYSuiZWkFufOVZ_ntxMxZQjG11Tq4/edit#heading=h.c11tdyhlpog1

https://liujunming.top/2023/08/06/Linux-kernel-SRCU-usage/ : 高度重合啊


## 参考资料

- [What is RCU, Fundamentally?](https://lwn.net/Articles/262464/)
- [What is RCU? Part 2: Usage](https://lwn.net/Articles/263130/)
- [RCU part 3: the RCU API](https://lwn.net/Articles/264090/)


## Documentation/RCU/Design/Requirements/Requirements.rst
<!-- 865c9d8e-5935-48fe-bb4a-e988d25ca03d -->

https://www.kernel.org/doc/Documentation/RCU/Design/Requirements/Requirements.html

这份文档主要包含以下几个核心部分：

1. 基本需求 (Fundamental Requirements)
这是 RCU 的灵魂，包含以下几个硬性保证：
 * 宽限期保证 (Grace-Period Guarantee)：synchronize_rcu() 必须等待所有在它开始之前就已经存在的 RCU 读侧临界区（read-side
   critical sections）结束。这是 RCU 能够安全释放内存的基础。
 * 发布/订阅保证 (Publish/Subscribe Guarantee)：这是 RCU 处理插入操作的核心。rcu_assign_pointer()
   保证数据在被指针引用之前完全初始化（发布），而 rcu_dereference()
   保证读者读取指针时能获取正确的数据依赖（订阅），防止编译器和 CPU 的乱序执行破坏数据一致性。
 * 内存屏障保证 (Memory-Barrier Guarantees)：详细规定了 RCU 原语在不同 CPU
   之间必须隐含的内存屏障语义，确保宽限期和临界区的顺序关系在多核环境下是严格且正确的。
 * 无条件执行保证：常用的 RCU 原语（如 rcu_read_lock）必须是无条件的，不能失败，不需要重试。
 * 读转写升级保证：允许在读侧临界区内进行更新操作（虽然需要额外的锁），这是为了代码复用。 (😦)

2. 基本非需求 (Fundamental Non-Requirements)
列出了 RCU 不 保证的事情，防止用户产生误解：
 * 读者几乎不强加排序：除了与宽限期的交互外，读侧原语本身几乎不提供任何内存顺序保证。
 * 读者不排斥更新者：rcu_read_lock() 不会阻塞更新者，也不会被更新者阻塞。
 * 更新者只等待“老”读者：synchronize_rcu() 启动后新进入的读者，它是不等的。
 * 宽限期不分割临界区：一个宽限期并不像一把刀一样把所有读侧临界区切成两半，它们可以重叠。
 * 临界区不分割宽限期：同理，一个长临界区可以跨越多个宽限期（只要不是覆盖整个宽限期导致其无法结束）。

3. 并行计算的现实 (Parallelism Facts of Life)
列出了 RCU 设计必须面对的物理现实：任何 CPU 都可能被延迟、CPU
和编译器会乱序执行、缓存行竞争代价高昂、计数器会溢出、系统可能拥有成千上万个 CPU。RCU 的设计必须能够处理这些极端情况。

4. 实现质量需求 (Quality-of-Implementation Requirements)
 * 专门化 (Specialization)：RCU 针对“读多写少”场景优化，读侧极快，写侧可能较慢。
 * 性能与可扩展性：必须节能（避免不必要的唤醒）、省内存（Tiny RCU）、在抢占式内核中保持低开销。
 * 前向进度 (Forward Progress)：必须有机制防止宽限期无限期拖延（如检测 CPU Stall，进行优先级提升等）。
 * 可组合性 (Composability)：读侧临界区可以嵌套。
 * 极端情况 (Corner Cases)：必须能处理极其密集的读操作流或极高的更新率。

5. 软件工程需求 (Software-Engineering Requirements)
为了防止人类犯错，RCU 必须提供调试辅助工具：
 * CONFIG_PROVE_RCU 和 lockdep 用于检查锁和 RCU 的正确使用。
 * sparse 的 __rcu 标记用于静态分析指针使用。
 * 死锁检测、双重释放检测、CPU Stall 警告等。

6. Linux 内核的复杂性 (Linux Kernel Complications)
RCU 是在 Linux 内核这个复杂的环境中生存的，必须处理：
 * 启动阶段 (Early Boot)：在调度器甚至没完全初始化前就要能工作。
 * 中断与 NMI：必须能在中断和不可屏蔽中断（NMI）中安全使用。
 * 模块加载/卸载：必须提供 rcu_barrier() 这样的机制来处理模块卸载时的回调清理。
 * CPU 热插拔：必须能应对 CPU 的动态上线和下线。

## Documentation/RCU/rcubarrier.rst
<!-- 5a3bf88a-a86a-4c15-b3d4-2c948d5957b3 -->

Documentation/RCU/rcubarrier.rst 文档专门介绍了 rcu_barrier() 原语，主要解决的是不可卸载模块（Unloadable Modules）在使用异步
  RCU 回调时遇到的问题。

  以下是该文档的核心分析：

  1. 问题背景：卸载模块时的回调残留
   * 当内核模块使用 call_rcu()
     注册异步回调函数（例如在中断上下文中删除链表元素并释放内存）时，回调函数会在未来的某个宽限期（grace period）后执行。
   * 如果在回调函数执行之前卸载该模块，回调函数代码所在的内存段将被释放。当 RCU 机制尝试执行这些回调时，CPU
     会跳转到已释放的内存地址，导致系统崩溃。
   * 仅仅使用 synchronize_rcu() 是不够的，因为它只等待宽限期结束，而不保证所有已排队的回调函数都已经执行完毕。

  2. 解决方案：`rcu_barrier()`
   * rcu_barrier() 的作用是：等待所有当前已排队的 RCU 回调函数执行完成。
   * 它不一定等待宽限期（如果没有排队的回调，它可以立即返回）。
   * 正确的使用流程：
       1. 停止产生新的 RCU 回调（例如停止相关线程、删除定时器）。
       2. 调用 rcu_barrier() 等待现有回调处理完毕。
       3. 安全卸载模块。

  3. 多种 RCU 变体
   * 如果模块使用了 SRCU (call_srcu)，则必须使用对应的 srcu_barrier()。
   * 如果混合使用了普通 RCU 和 SRCU，卸载时需要同时调用 rcu_barrier() 和 srcu_barrier()。

  4. 实现原理（简化版）
   * 文档简要描述了 rcu_barrier() 的早期实现逻辑：它在每个 CPU 上都注册一个新的 RCU 回调。
   * 由于 RCU 回调在单个 CPU 上是按顺序执行的，当所有 CPU
     上新注册的这个“屏障回调”都执行完毕时，就意味着在此之前注册的所有回调也都肯定执行完毕了。
   * 实现中使用了原子计数器 (atomic_inc/atomic_dec_and_test) 和完成量 (completion) 来等待所有 CPU 完成任务。

  5. 其他应用场景
   * 除了模块卸载，文件系统卸载（unmount）时也可能需要用到它（这是 rcu_barrier 最初诞生的原因）。

  简而言之，rcu_barrier() 是模块卸载时的安全守门员，确保“人走茶凉”之前，所有的“后事”（pending
  callbacks）都已经处理干净，防止内核因执行已卸载的代码而崩溃。

## Documentation/RCU/rcu_dereference.rst
<!-- d08aae5e-0178-4813-a0b0-4628704fe721 -->

https://www.kernel.org/doc/html/latest/RCU/rcu_dereference.html

 1. `rcu_dereference()` 的必要性
   * 依赖性保护：rcu_dereference() 不仅仅是一个指针加载操作，它还能防止编译器和 CPU（特别是在 Alpha、ARM、PowerPC
     等弱内存序架构上）进行可能破坏代码正确性的重排序或优化。它确保了从获取指针到后续使用该指针访问数据之间的“地址依赖”和“数据依
     赖”不被破坏。
   * 防止编译器过度优化：如果没有它，编译器可能会推断出指针的值（例如通过比较操作），从而进行错误的优化，导致读取到初始化之前的
     数据或不一致的状态。

  2. 使用规则与陷阱
   * 仅限指针：只能对指针值使用 rcu_dereference()，不能用于整数（因为编译器对整数运算的优化太激进）。
   * 避免算术抵消：不要对返回的指针进行像 p - (uintptr_t)p 这样的运算，编译器可能会将其优化为 0，从而丢失依赖链。
   * 禁止关系比较：不要对 rcu_dereference() 返回的指针使用 >、<
     等关系运算符，因为编译器可能会将其编译为分支指令，而在弱内存序机器上，分支预测可能导致在此之前的加载操作被推测执行，从而产
     生乱序。
   * 谨慎进行非 NULL 比较：将 RCU
     指针与某个具体对象的地址进行相等性比较（==）时要小心。如果编译器发现两个指针相等，它可能会直接使用该对象的地址来访问成员，
     从而绕过 rcu_dereference() 提供的内存屏障保护。

  3. 如何选择正确的原语
  文档详细列出了在不同场景下应该使用的变体：
   * rcu_dereference()：在标准的 RCU 读侧临界区（read-side critical section）中使用。
   * rcu_dereference_check()：当访问可能由 RCU 保护，或者由某个锁保护时使用（用于调试和 lockdep 检查）。
   * rcu_dereference_protected()：在更新侧（update-side）使用，确保代码持有相应的锁。
   * rcu_dereference_raw()：仅在极少数保护机制对调用者不可见或极其复杂的情况下使用。

  4. 静态分析支持 (Sparse)
   * 介绍 __rcu 标记：用于标记 RCU 保护的指针。配合 sparse 工具，可以自动检测直接访问受保护指针（未使用
     rcu_dereference）的错误，或者在非 RCU 指针上错误使用 rcu_dereference 的情况。


## 读读 LoyenWang 的 blog

### https://www.cnblogs.com/LoyenWang/p/12681494.html

### https://www.cnblogs.com/LoyenWang/p/12770878.html


## 更多的文档

- https://docs.google.com/document/d/1GCdQC8SDbb54W1shjEXqGZ0Rq8a6kIeYutdSIajfpLA/edit#heading=h.ytgz5i5df43s
- https://www.kernel.org/doc/Documentation/RCU/Design/Requirements/Requirements.html

## oe 的文档
- https://www.openeuler.org/en/blog/luoyuzhe/RCU/Read-Copy%20Update.html

- [ ] https://liburcu.org/ : 提供了三个很好的资源
- https://mp.weixin.qq.com/s/SZqmxMGMyruYUH5n_kobYQ
- https://hackmd.io/@sysprog/linux-rcu?type=view
- `__d_lookup_rcu`


## [What is RCU? -- "Read, Copy, Update"](https://www.kernel.org/doc/html/next/RCU/whatisRCU.html)

## [The RCU API, 2019 edition](https://lwn.net/Articles/777036/)

- synchronize_rcu
- synchronize_rcu_expedited
- call_rcu
- rcu_barrier
- get_state_synchronize_rcu / cond_synchronize_rcu

## [Kernel configuration parameters for RCU](https://lwn.net/Articles/777214/)

### kernel/rcu/Kconfig

```txt
# RCU Subsystem
CONFIG_TREE_RCU=y
CONFIG_PREEMPT_RCU=y
# CONFIG_RCU_EXPERT is not set
CONFIG_TREE_SRCU=y
CONFIG_TASKS_RCU_GENERIC=y
CONFIG_TASKS_RCU=y
CONFIG_TASKS_RUDE_RCU=y
CONFIG_TASKS_TRACE_RCU=y
CONFIG_RCU_STALL_COMMON=y
CONFIG_RCU_NEED_SEGCBLIST=y
CONFIG_RCU_NOCB_CPU=y
# CONFIG_RCU_NOCB_CPU_DEFAULT_ALL is not set
CONFIG_RCU_LAZY=y
# end of RCU Subsystem
CONFIG_MMU_GATHER_RCU_TABLE_FREE=y
# RCU Debugging
# CONFIG_RCU_SCALE_TEST is not set
# CONFIG_RCU_TORTURE_TEST is not set
CONFIG_RCU_REF_SCALE_TEST=m
CONFIG_RCU_CPU_STALL_TIMEOUT=21
CONFIG_RCU_EXP_CPU_STALL_TIMEOUT=0
# CONFIG_RCU_CPU_STALL_CPUTIME is not set
CONFIG_RCU_TRACE=y
# CONFIG_RCU_EQS_DEBUG is not set
# end of RCU Debugging
```

## 很好的入门
- https://www.zhihu.com/question/27943222/answer/2174857178

## 将 stackoverflow 搜索下 RCU 的问题

例如 https://stackoverflow.com/questions/tagged/rcu?tab=Votes

- https://stackoverflow.com/questions/21287932/is-it-necessary-invoke-rcu-read-lock-in-softirq-context
  - 似乎，有道理

- https://stackoverflow.com/questions/17437165/rcu-as-an-alternative-to-conventional-garbage-collection
  - 的确是一个很好的思考，可惜对于垃圾回收不是很清楚

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
