# Ease of Use

## 一、章节概述与核心观点

"Ease of Use" 是 Paul E. McKenney 《Is Parallel Programming Hard, And, If So, What Can You Do About It?》中看似最"软"却极其关键的一章。作者在开篇就用一个严肃的事实定下了基调：Linux 内核 RCU 的一个易用性缺陷（ease-of-use bug）直接导致了一个可被利用的内核安全漏洞。这证明即便是内核级别的底层 API，如果设计得难以正确使用，也会造成真实的安全事故。因此，API 的易用性不是一个可有可无的"锦上添花"特性，而是与正确性和性能同等重要的工程属性。

作者紧接着指出了一个根本困难："容易"是一个相对概念。对一个饥饿的人来说，食物是改善生活的关键；但对一个严重肥胖的人来说，额外的食物可能致命。同理，API 设计者必须深刻理解目标用户群体的背景、习惯和认知模型，才能判断什么对他们来说是"容易"的。如果你在设计一个帮助陌生人的软件，却从不与他们交流，那么他们对软件的不满就不应该让你感到惊讶。

## 二、Rusty Scale for API Design

本章的核心框架来自 Rusty Russell 在 2003 年 Ottawa Linux Symposium 的主题演讲。Rusty 的核心论点是：API 设计的目标不应该是"易于使用"（easy to use），而应该是"难以误用"（hard to misuse）。他据此提出了一个从"不可能出错"到"不可能做对"的 20 级尺度，本章将其推广到了通用软件工程领域。

### 2.1 顶级：不可能出错（It is impossible to get wrong）

这是所有 API 设计者应该追求的理想标准，但几乎只有传说中的 `dwim()`（"Do What I Mean"）函数接近这一水平。现实中没有任何通用 API 能达到这一级别。

### 2.2 编译器/链接器阻止错误（Level 2-3）

第二级是"编译器或链接器不让你出错"，第三级是"编译器或链接器会在你出错时发出警告"。Linux 内核中 `BUILD_BUG_ON()` 宏是这一级别的典型代表。

`BUILD_BUG_ON()` 定义在 `include/linux/build_bug.h` 中：

```c
#define BUILD_BUG_ON(condition) \
	BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)

#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)
```

其底层通常依赖 `_Static_assert`（C11 标准）或类似的编译时断言机制。当条件为真时，编译直接失败并输出错误信息。这被广泛应用于内核中验证结构体大小、字段偏移量、常量关系等场景。例如，如果某个驱动依赖 `struct my_struct` 中 `field_b` 必须在偏移 8 的位置，就可以写：

```c
BUILD_BUG_ON(offsetof(struct my_struct, field_b) != 8);
```

一旦有人重构结构体导致字段偏移变化，编译立即失败，而不是在运行时产生难以调试的 ABI 破坏问题。这完美体现了"将错误检测提前到编译期"的工程哲学。

### 2.3 最简单的用法就是正确的（Level 4）

这一级别要求 API 的默认、最直观的使用方式就是正确的方式。内核中的引用计数 API（`refcount_t`）正在向这一方向演进。老式的 `atomic_t` 引用计数容易被误用（例如直接用 `atomic_inc` 而不是 `atomic_inc_return` 来判断是否首次引用），而 `refcount_t` 通过限制可用的操作子集，使得"显而易见"的使用路径更难出错。

### 2.4 名字告诉你如何使用（Level 5）

`rcu_read_lock()` 是一个好名字，对于熟悉读写锁的开发者来说，它清晰地表达了"进入 RCU 读端临界区"的语义。但名字也可能是双刃剑：如果一个从引用计数背景转过来的开发者看到 `rcu_read_lock()`，他可能会困惑——"加锁"和"引用计数获取"的心理模型差异可能导致误用。

在内核源码 `kernel/rcu/tree_plugin.h` 中，`__rcu_read_lock()` 的实现如下：

```c
void __rcu_read_lock(void)
{
	rcu_preempt_read_enter();
	if (IS_ENABLED(CONFIG_PROVE_LOCKING))
		WARN_ON_ONCE(rcu_preempt_depth() > RCU_NEST_PMAX);
	if (IS_ENABLED(CONFIG_RCU_STRICT_GRACE_PERIOD) && rcu_state.gp_kthread)
		WRITE_ONCE(current->rcu_read_unlock_special.b.need_qs, true);
	barrier();  /* critical section after entry code. */
}
```

这里的实现比名字本身要复杂得多。名字告诉了你"做什么"，但没有告诉你"RCU 读端临界区内不能阻塞"、"必须与 `rcu_read_unlock()` 配对"等关键约束。这正是 Level 5 的局限性：名字再好，也无法传达全部契约。

### 2.5 做对了就不会在运行时崩溃，否则总会崩溃（Level 6）

这一级别的 API 在误用时会以可预测、可检测的方式失败。内核中的 `WARN_ON_ONCE()` 是这一级别的利器。

在 `include/asm-generic/bug.h` 中，`WARN_ON_ONCE` 的定义（简化后）为：

```c
#define WARN_ON_ONCE(condition) ({                                    \
	int __ret_warn_on = !!(condition);                            \
	if (unlikely(__ret_warn_on) && !__warned) {                   \
		__warned = true;                                      \
		__WARN();                                             \
	}                                                             \
	unlikely(__ret_warn_on);                                      \
})
```

它的关键价值在于"ONCE"：同一类警告只会输出一次，避免了日志风暴（log spam），同时又能让开发者注意到内核代码路径中存在的异常假设。例如，RCU 实现中用 `WARN_ON_ONCE` 检测嵌套深度超过安全阈值的情况，既保护了系统不被日志淹没，又能在开发阶段暴露问题。

### 2.6 遵循常见约定就能做对（Level 7）

`malloc()` 是这一级别的经典例子。虽然 C 语言的内存管理很容易出错，但大量项目仍然能正确使用 `malloc()`，因为社区已经形成了牢固的约定：检查返回值、配对使用 `free()`、避免 use-after-free 等。如果再配合 Valgrind 等工具，可以将 `malloc()` 的安全性提升到接近 Level 6。

在 Linux 内核中，虽然没有用户空间的 `malloc()`，但 `kmalloc()` 和 `vmalloc()` 家族遵循类似的约定。`kmalloc()` 在分配小对象时高效，`vmalloc()` 用于大块虚拟连续内存。开发者只要遵循"用 `kfree()` 释放 `kmalloc()` 的内存，用 `vfree()` 释放 `vmalloc()` 的内存"这一约定，通常就不会出错。

### 2.7 读文档才能做对（Level 8）与读实现才能做对（Level 9）

这两个级别标志着 API 安全性的明显下降。当 API 的语义无法从名字和直觉推断，而必须依赖文档或阅读源码时，出错概率大幅上升。内核文档目录 `Documentation/` 和内存模型工具 `tools/memory-model/` 存在的重要价值，就在于将许多内核同步原语从 Level 9 提升到 Level 8。

### 2.8 读实现会让你出错（Level 12）

这是最危险的级别之一。内核中 infamous 的例子是早期非 `CONFIG_PREEMPT` 配置下的 `rcu_read_lock()` 实现。某些版本的实现几乎是空操作，阅读源码的开发者可能误以为"RCU 读端什么都不做，所以非常轻量"，却没有意识到在可抢占配置下它的语义完全不同。这种"实现看似简单，实则误导"的情况，是内核演进中需要不断修补的陷阱。

### 2.9 读文档会让你出错（Level 13）

DEC Alpha 的 `wmb` 指令文档曾让许多开发者误以为它提供了比实际更强的内存序保证。后来文档被澄清，才将其提升到 Level 8。这说明文档本身也可能成为 bug 的来源——尤其是当硬件厂商的文档存在歧义时。

### 2.10 遵循常见约定会让你出错（Level 14）

`printf()` 的错误返回值检查是一个教科书级例子。几乎所有开发者都习惯性地忽略 `printf()` 的返回值，因为"向标准输出写数据怎么会失败？"但在某些场景下（例如输出被重定向到已满的管道），`printf()` 确实可能失败，而忽略返回值可能导致错误未被传播。

### 2.11 做对反而会在运行时崩溃（Level 15）

这是最反直觉的级别。API 的设计如此糟糕，以至于正确的使用方式会触发 bug。

### 2.12 名字告诉你如何不用（Level 16）与显而易见的用法是错误的（Level 17）

`smp_mb()` 是 Level 17 的绝佳例子。它的名字"SMP Memory Barrier"让许多开发者误以为它提供了"全面、全局的内存屏障"——仿佛它能将所有 CPU 上的所有操作序列化。但事实远非如此。

在 `include/asm-generic/barrier.h` 中，`smp_mb()` 的定义为：

```c
#ifdef CONFIG_SMP
#define smp_mb()	do { kcsan_mb(); __smp_mb(); } while (0)
#else
#define smp_mb()	barrier()
#endif
```

其中 `__smp_mb()` 在 x86 上通常是 `mfence` 或 `lock; addl $0,0(%%rsp)`，在 ARM64 上是 `dmb ish`。这些指令提供的是**成对的**（pairwise）顺序保证：确保 `smp_mb()` 之前的加载和存储不会重排到它之后，反之亦然。但它们并不提供跨所有 CPU 的全局总序（total order），也不自动保证与其他无关变量的顺序。

作者明确指出，第 15 章 "Advanced Synchronization: Memory Ordering" 以及内核源码树中的 `Documentation/memory-barriers.txt` 和 `tools/memory-model/` 包含了避免这一误用所需的信息。这再次说明，对于内存序 API，仅靠名字是远远不够的。

### 2.13 不可能做对（Level 20）

`gets()` 函数是这一级别的终极代表。它从标准输入读取一行，但没有缓冲区大小参数，因此永远无法防止缓冲区溢出。C11 标准已经正式移除了 `gets()`。内核代码中从来不会使用如此危险的 API，而是使用带长度限制的 `strscpy()`、`strncpy()` 等函数。

## 三、Shaving the Mandelbrot Set
FIXME

本章第三节用 Mandelbrot 集合作为隐喻，说明了一个深刻的工程道理：有用程序的集合像 Mandelbrot 集合一样，没有清晰光滑的边界。但我们需要的是"真实人类能够使用"的 API，而不是需要为每个用例都撰写博士论文才能正确使用的 API。因此，我们必须"修剪 Mandelbrot 集合"（shave the Mandelbrot set）——将 API 的使用限制在一个易于描述的子集内。

### 3.1 循环链表插入的死锁避免：一个需要被"剃掉"的算法

作者设计了一个精妙的反例来说明这一点。假设有一个循环双向链表，每个线程对应一个元素，还有一个头节点。当新线程被创建时，需要向链表中插入一个新元素。为了保护链表，可以对每个节点加细粒度锁，插入时需要同时持有前驱和后继的锁。

避免死锁的常规方法是按地址顺序或按列表顺序加锁。但作者提出了一个"更巧妙"的方案：无条件地按列表顺序加锁（先前驱、后后继）。然后证明：在这种情况下，死锁**不可能**发生。

证明思路：将头节点编号为 0，后续节点编号为 1 到 N；线程编号为 0 到 N-1。如果每个线程试图锁定一对连续节点，那么线程 0 锁节点 0，线程 1 锁节点 1，依此类推。当线程 N-1 试图锁节点 N 时，已经没有其他线程了，因此它不可能被阻塞。所以至少有一个线程能拿到两个锁。

这个证明在数学上是正确的。但问题在于：这个算法极度脆弱（fragile）。它要求：

1. 列表必须是循环的。
2. 必须有恰好 N 个线程和 N+1 个节点（包括头节点）。
3. 每个线程必须恰好锁定一对**连续**节点。
4. 不能有任何遗漏的节点插入。

任何一个条件被破坏，死锁就可能发生。例如，如果某个线程因为 bug 晚插入了一点点，或者线程数量增加了一个，整个证明的前提就崩塌了。相比之下，按地址顺序加锁虽然需要比较地址（额外的分支），但它适用于任意大小的列表、任意数量的线程，而且代码简单、易于维护。

这就是"修剪 Mandelbrot 集合"的本质：我们不应该仅仅因为某个算法"能工作"就使用它。算法的通用性和鲁棒性必须与它的学习和维护成本相匹配。越复杂、越难理解的算法，就必须越通用、越有价值，才值得被纳入工程实践。

### 3.2 与 Dining Philosopher 问题的联系

这个算法的灵感来自 Dining Philosopher 问题。经典的解决方案是给五个哲学家配五把叉子，而 Paul's 的"解决方案"是"直接再给五把叉子"。然后他不恰当地将这个思路套用到循环链表上，创造出了上述脆弱的算法。这个轶事生动地说明了：一个在特定问题域中成立的洞察，如果不加批判地移植到其他领域，可能产生有害的结果。

## 四、结合 Linux 内核源码的具体分析

### 4.1 BUILD_BUG_ON 在内核中的广泛应用

`BUILD_BUG_ON` 及其变体（`BUILD_BUG_ON_ZERO`、`BUILD_BUG_ON_MSG`、`BUILD_BUG_ON_NOT_POWER_OF_2`）是内核编译时断言的基础设施。它们被用于：

- 验证页表级别的常量关系。
- 确保结构体大小符合硬件期望（如设备描述符）。
- 检查数组大小是否为 2 的幂（对位运算优化至关重要）。

例如，`BUILD_BUG_ON_NOT_POWER_OF_2` 的定义为：

```c
#define BUILD_BUG_ON_NOT_POWER_OF_2(n) \
	BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))
```

如果传入的 `n` 不是 2 的幂，编译立即失败。这在定义哈希表大小、环形缓冲区掩码时极其有用。

### 4.2 WARN_ON_ONCE 与内核的防御性编程

`WARN_ON_ONCE` 体现了内核的防御性编程哲学：不要默默容忍异常，但也不要让一次异常毁掉整个系统。它在 RCU、调度器、内存管理等子系统中广泛使用。例如，当检测到 RCU 读端嵌套深度异常时，输出一次警告并继续运行，而不是立即 panic。这让开发者有机会在测试环境中发现问题，同时保证生产环境的稳定性。

### 4.3 rcu_read_lock 的命名困境

如前文所述，`rcu_read_lock()` 对于从读写锁迁移过来的开发者很直观，但对于从引用计数迁移过来的开发者则可能产生困惑。RCU 的读端临界区本质上不是"锁"，而是一种与宽限期（grace period）机制协调的并发约定。命名上的"lock"一词强化了错误的直觉模型。内核社区为了缓解这一问题，提供了大量文档（如 `Documentation/RCU/`）和 lockdep 检查（`CONFIG_PROVE_LOCKING`），但命名本身的双刃剑效应依然存在。

### 4.4 smp_mb 的语义鸿沟

`smp_mb()` 是内核中最容易被误用的原语之一。其名字暗示了"SMP 层面的完全屏障"，但实际语义要弱得多。在 x86 上，由于 TSO（Total Store Order）模型，很多场景下不需要显式的 `smp_mb()`；而在 ARM64 等弱序架构上，`smp_mb()` 的成本很高，但即便如此，它也不提供开发者想象中的"全局序列化"效果。内核的 LKMM（Linux Kernel Memory Model）工具 `herd7` 和 litmus 测试正是为了帮助开发者理解这些细微差别而存在的。

## 五、与我的并发编程笔记的关联

在我的笔记 `~/data/vn/docs/concurrent/` 中，记录了以下主要关注点：

1. **atomic API 的复杂性**：笔记中详细记录了 `atomic_add_return` 与 `atomic_fetch_add` 的区别、`acquire/release` 语义的选择困惑、以及 `smp_mb__before_atomic()` 等 barrier 的用法。第 16 章虽然没有直接回答这些技术细节，但提供了一个高层次的解释框架：**atomic API 之所以如此复杂，正是因为它在 Rusty Scale 上追求更高的安全性**。如果 `atomic_t` 的 API 设计得更简单（例如所有操作都统一为 strongest semantics），性能会受损；如果设计得更简单但语义更弱，则容易误用。当前的复杂度是一种在"难以误用"和"性能"之间的工程权衡。

2. **RCU 在网络栈中的大量使用**：笔记中观察到 `__dev_queue_xmit` 中嵌套了多层 `rcu_read_lock`，并质疑这是否是"滥用"。第 16 章提醒我们从 API 设计的角度看这个问题：`rcu_read_lock()` 的名字（Level 5）让开发者觉得"加锁"是轻量且安全的，因此倾向于多用。但 RCU 的真正约束（不能睡眠、必须与宽限期协调）并没有通过名字传达。这解释了为什么开发者容易过度使用 RCU——API 的命名让"使用"显得比"正确使用"更简单。

3. **Cache coherence 与 memory ordering**：笔记中探讨了 cache coherence 的保证、write buffer flush、以及中断与 memory ordering 的关系。第 16 章提到的 `smp_mb()` 命名陷阱直接相关：很多开发者（包括笔记中的我自己）可能会误以为 `smp_mb()` 能解决所有跨 CPU 的可见性问题，但实际上它只是 pairwise ordering 工具。真正理解 cache coherence 和 memory model 需要深入到 LKMM 和具体架构文档，而不是依赖一个屏障函数的名字。

4. **各种锁的设计与使用**：笔记中记录了 `irq` 锁的设计、`synchronize_irq` 的用法、virtio queue 的锁策略等。第 16 章的"Shaving the Mandelbrot Set"对这些记录有间接的指导意义：在内核驱动开发中，我们不应该追求最巧妙的锁策略（如那个脆弱的循环链表算法），而应该优先选择简单、可靠、经过广泛验证的模式（如按地址顺序加锁、使用现成的内核同步原语）。

**结论**：第 16 章没有直接解答我笔记中的具体技术问题（如 `atomic_fetch_add` 的精确语义、ARM64 上 `test_and_set_bit_lock` 的实现细节），但它提供了一个评价和选择 API 的元框架（meta-framework）。当我在笔记中困惑"为什么这个 API 这么复杂"时，Rusty Scale 告诉我：复杂性往往是"难以误用"的代价。当我在笔记中质疑"RCU 是否被滥用"时，"Shaving the Mandelbrot Set"提醒我：简单和通用比巧妙更重要。

## 六、代码说明

本章配套了两个演示程序：

### 6.1 `lock_order_demo.c`

演示循环链表并发插入的两种锁策略：

- **Robust（地址顺序）**：始终按内存地址排序加锁，避免死锁。适用于任意线程数、任意列表大小。
- **Fragile（列表顺序）**：按链表中的逻辑顺序加锁。虽然在特定数学条件下可以证明无死锁，但极度依赖线程数和节点数的精确匹配。

程序通过多线程并发插入测试两种策略，验证它们在高并发下的行为差异。这直观展示了为什么内核和通用库应该选择 robust 方案。

### 6.2 `api_safety_levels.c`

用用户空间可编译的代码模拟 Rusty Scale 中的多个级别：

- `_Static_assert` 模拟 `BUILD_BUG_ON`（Level 2/3）。
- `USER_WARN_ON_ONCE` 模拟内核的 `WARN_ON_ONCE`（Level 6）。
- `safe_malloc` 演示遵循约定即可安全使用的 API（Level 7）。
- `misleading_full_memory_fence` 演示命名误导（Level 15/16，`smp_mb()` 问题）。
- `safe_read_line` 演示 `fgets` 替代 `gets`（Level 20）。

### 6.3 编译与运行

```bash
make clean && make all && make run
```

Makefile 中二进制后缀为 `.out`，可被 `.gitignore` 自动忽略。

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
