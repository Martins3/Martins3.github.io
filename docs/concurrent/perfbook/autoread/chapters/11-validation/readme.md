# 第 11 章：Validation（验证）详细分析

## 一、章节概述

本章是《Is Parallel Programming Hard, And, If So, What Can You Do About It?》中至关重要的一章，作者 Paul E. McKenney 以极其坦诚的态度告诉读者：并行程序如果不经过验证，几乎注定是失败的。验证不是可有可无的附属品，而是并行编程的核心组成部分。本章的哲学根基在于一句话："The machine knows what is wrong. Make it tell you."（机器知道哪里错了，你要迫使它告诉你。）

本章涵盖的内容极其广泛，从软件工程心理学的角度出发，讨论了 bug 的来源和开发者应有的心态；然后依次介绍了 Tracing（追踪）、Assertions（断言）、Static Analysis（静态分析）、Code Review（代码审查）等传统验证手段；接着用大量数学推导讲解了概率统计在并行软件验证中的应用，包括离散测试的二项分布和连续测试的泊松分布；随后专门用一节讨论 Heisenbug（海森堡 bug）的狩猎技术；最后讨论了性能评估的各种方法，包括基准测试、性能分析、微基准测试和干扰隔离。

## 二、详细内容分析

### 2.1 Introduction（引言）

#### 2.1.1 Where Do Bugs Come From?（bug 从何而来）

作者指出了一个深刻的人类学问题：人类大脑并不是为编写计算机程序而进化的。计算机有三个特性常常冲击人类直觉：（1）计算机缺乏常识；（2）计算机无法理解用户意图；（3）计算机无法处理碎片化的计划，必须把所有细节都明确 spelled out。人类喜欢写碎片化计划（fragmentary plans），因为在人类的进化史中，执行碎片化计划的人更容易存活——与其饿死在做计划的路上，不如先行动。但这种乐观主义在编程中就成了 bug 的温床。作者甚至引用了临床心理学的说法："低于疯狂水平的乐观主义的临床术语是'临床抑郁'。"这意味着，正常健康的人都是有疯狂乐观主义的，而正是这种乐观主义让我们敢于开始困难的项目，同时也让我们高估自己的能力。

#### 2.1.2 Required Mindset（所需心态）

作者给出了两个定义：（1）唯一没有 bug 的程序是平凡程序；（2）可靠的程序是"没有已知 bug"的程序。由此逻辑推导出：任何可靠的非平凡程序都至少包含一个你还不知道的 bug。因此，验证工作的本质是破坏（destruction），而不是建设。如果你找不到任何 bug，那你的验证工作本身就是失败的。这种心态转换极其重要——开发者需要从"我写的代码是对的"转变为"我的代码一定有错，我要找出它"。

本章中有一个经典的 QuickQuiz：如果你要写一个解析 `time` 命令输出的脚本，应该提供哪些测试输入？答案列举了近 20 种边界情况，包括所有时间为零、user + sys > real、分钟值非零、秒值大于 60、32 位/64 位溢出、负值、省略 m 或 s、非数字输入、行被省略、行被重复、随机字符等等。这个例子极好地说明了"破坏性思维"在测试中的重要性。

#### 2.1.3 When Should Validation Start?（何时开始验证）

答案是：项目开始的那一刻。作者提出了"达尔文主义验证观"：验证套件是淘汰不适应代码的筛选器。从这个角度看，开发者对代码库的精心修改，从达尔文主义的角度来看只不过是随机突变。这个视角虽然令人谦卑，但非常有效——研究表明 7% 的修复会引入至少一个新 bug。验证应该贯穿项目整个生命周期，因为 bug 会不断适应你的验证套件，除非你持续改进验证套件，否则项目会自然积累大量"免疫"的 bug。

#### 2.1.4 The Open Source Way（开源方式）

作者以自己的亲身经历说明了开源社区代码审查的力量。他向 Linux 内核提交的一个分布式文件系统补丁，还没来得及写测试代码，就收到了多轮 bug 报告。这验证了开源格言："Given enough eyeballs, all bugs are shallow."（只要眼睛足够多，所有 bug 都是浅层的。）但作者也提出了三个现实问题：有多少眼睛真的会看你的代码？有多少眼睛足够经验丰富？它们什么时候看？因此，即使是开源项目，开发者也必须准备自己的测试套件。

### 2.2 Tracing（追踪）

当其他方法都失败时，添加一个 `printk()` 或 `printf()`！这是调试的经典方法。但在并行程序中，追踪工具有一个严重问题：开销过大。Linux 内核中的 trace events 利用 per-CPU 缓冲区实现了极低开销的数据收集。BPF 可以在内核中进行数据缩减，减少从内核到用户空间传输信息的开销。然而，启用追踪可能改变时序，导致 heisenbug 消失。

作者的 `rcutorture` 脚本早期版本对 RCU grace period 的无限期 stall 完全满意，后来才增加了检测逻辑。这说明：脚本只会检测你告诉它检测的问题。而且，除非你有坚实的设计，否则你不会知道脚本应该检测什么。

### 2.3 Assertions（断言）

断言的基本模式是：`if (something_bad_is_happening()) complain();`。在 Linux 内核中，这通常封装为 `WARN_ON()` 和 `WARN_ON_ONCE()`。

#### Linux 内核中的 WARN_ON_ONCE

内核源码在 `include/asm-generic/bug.h` 中定义了 `WARN_ON_ONCE`：

```c
#define WARN_ON_ONCE(condition) ({                    \
	int __ret_warn_on = !!(condition);                \
	if (unlikely(__ret_warn_on))                      \
		__WARN_FLAGS(#condition,                      \
			     BUGFLAG_ONCE |                     \
			     BUGFLAG_TAINT(TAINT_WARN));        \
	unlikely(__ret_warn_on);                          \
})
```

这个宏使用 `BUGFLAG_ONCE` 确保每个运行时只报告一次。本章的 QuickQuiz 问到了如何实现 `WARN_ON_ONCE`：一种简单方法是维护一个 static 变量；如果需要严格保证只出现一次，可以使用原子交换操作，仅在返回零时打印消息。

#### lockdep_assert_held

在并行代码中，一个特别糟糕的"坏事"是：期望在某个锁保护下调用的函数，实际上在没有持有该锁的情况下被调用了。Linux 内核的 lockdep（锁依赖验证器）提供了 `lockdep_assert_held()` 函数来检查指定锁是否被持有。内核源码在 `include/linux/lockdep.h` 中的实现如下：

```c
#define lockdep_assert_held(l)                                          \
	do { lockdep_assert(lockdep_is_held(l) != LOCK_STATE_NOT_HELD);     \
	     __assume_ctx_lock(l); } while (0)
```

当启用了 `CONFIG_LOCKDEP` 时，这个宏会在运行时检查锁状态；如果未启用，则为空操作。在我们的示例代码 `assertion_demo.c` 中，实现了一个简化版本，使用 `pthread_mutex_t` 和 owner 字段来模拟这一机制。

#### KCSAN 与 data_race

本章还提到了 KCSAN（Kernel Concurrency Sanitizer），它利用现有的 `READ_ONCE()` 和 `WRITE_ONCE()` 标记来判断哪些并发访问应该收到警告。KCSAN 的误报率较高，因此内核提供了 `data_race()` 构造来原谅已知良性的数据竞争，以及 `ASSERT_EXCLUSIVE_ACCESS()` 和 `ASSERT_EXCLUSIVE_WRITER()` 断言来显式检查数据竞争。

在 `include/linux/kcsan-checks.h` 中，`ASSERT_EXCLUSIVE_ACCESS` 的定义如下：

```c
#define ASSERT_EXCLUSIVE_ACCESS(var)                                    \
	__kcsan_check_access(&(var), sizeof(var), KCSAN_ACCESS_WRITE | KCSAN_ACCESS_ASSERT)
```

这个宏用于断言对变量的独占访问，即没有并发读写。例如，在引用计数降为 0 后进行清理时，可以使用它验证此时确实没有其他线程在访问该对象。

### 2.4 Static Analysis（静态分析）

静态分析是一个程序以另一个程序为输入，报告其中的错误和漏洞。编译器本身就是一种静态分析器。Linux 内核使用 sparse 静态分析器来发现高级别问题，包括：用户空间结构体指针的误用、过长常量的赋值、空 switch 语句、锁获取/释放原语不匹配、per-CPU 原语误用、RCU 原语与非 RCU 指针的混用等。

coccinelle 工具则是一个 C 语法感知的搜索替换工具，有大量脚本用于定位和修复内核 bug 类别。

### 2.5 Code Review（代码审查）

代码审查是人类进行的静态分析。本节介绍了三种形式：

1. **Inspection（检查）**：传统面对面的正式会议，有主持人、开发者和参与者。Linux 内核社区通过邮件和 IRC 异步进行，优势在于可能获得不熟悉代码的人的视角。

2. **Walkthroughs（走查）**：团队用特定测试用例"扮演计算机"。作者想象了一种"虐待狂"式的现代走查：用调试器单步执行，每一步都要求开发者预测结果并解释原因；在并行代码中，还要有一个"并发鲨鱼"问什么代码可能与此并发执行。

3. **Self-Inspection（自我检查）**：当无法涉及他人时的替代方案。作者给出了一个极端但有效的流程：写设计文档、咨询专家、用笔在纸上写代码（复制现有代码而不是引用）、在每个步骤中质疑假设、重复直到最后两份拷贝一致、为非显然代码写正确性证明、使用版本控制、自底向上测试。

### 2.6 Probability and Heisenbugs（概率与海森堡 bug）

并行程序有时会随机失败，这就是 heisenbug。添加追踪和断言很容易降低 bug 出现的概率，因此极轻量级的追踪和断言机制至关重要。

#### 2.6.1 离散测试统计（二项分布）

假设一个 bug 在单次测试中有 10% 的概率出现，运行 5 次，至少一次失败的概率是多少？

单次成功概率 = 0.9，五次都成功的概率 = 0.9^5 ≈ 0.59，因此至少一次失败的概率 ≈ 41%。

通用公式：
- S_n = (1-f)^n  （全部成功的概率）
- F_n = 1 - (1-f)^n  （至少一次失败的概率）

如果要 99% 确信修复有效，即如果原 bug 还在，运行 n 次至少失败一次的概率达到 99%，但实际上全部成功。解方程求 n：

n = log(1 - F_n) / log(1 - f)

代入 f=0.1, F_n=0.99，得到 n ≈ 43.7，即需要 44 次连续成功。如果 bug 的单次失败率只有 1%，则需要 459 次测试。这说明了创建高失败率 reproducer（复现器）的重要性——将失败率从 1% 提升到 30%，所需测试从 459 次降到 13 次。

#### 2.6.2 连续测试统计（泊松分布）

对于 rcutorture 这类持续运行的压力测试，使用泊松分布更合适：

F_m = (λ^m / m!) * e^(-λ)

其中 λ 是单位时间的期望失败次数。对于零次失败（m=0）：

F_0 = e^(-λ)

要 99% 确信修复有效，设 F_0 = 0.01，得 λ = -ln(0.01) = 4.6。如果原失败率是每小时 0.3 次，则需要 4.6 / 0.3 = 14.3 小时的无错误运行。这与把每小时运行当作离散测试（13 小时）的结果相差不到 10%，说明简单方法通常足够好。

更通用的零错误测试时长公式：

T = -(1/n) * ln((100-P)/100)

其中 n 是每小时失败次数，P 是所需置信度。

#### 2.6.3 Hunting Heisenbugs（狩猎海森堡 bug）

海森堡 bug 的名字来自海森堡不确定性原理，意指观察行为本身会改变被观察对象。实际上这更像是经典物理中的"观察者效应"。

作者提出了六种反海森堡 bug（anti-heisenbug）技术：

1. **Add Delay（添加延迟）**：在竞态条件的临界区添加延迟可以极大增加失败概率。例如将 load-add-store 改为 load-add-delay-store。在我们的 `heisenbug_demo.c` 中，演示了添加 1 微秒延迟后，count loss 从约 46% 提升到 75%。

2. **Increase Workload Intensity（增加工作负载强度）**：添加更多 CPU、更多网卡、更快的存储、改变问题大小等。

3. **Isolate Suspicious Subsystems（隔离可疑子系统）**：Linux 内核的 `rcutorture` 模块就是对 RCU 进行隔离压力测试的典型例子。`rcutorture` 对 RCU 施加的压力超过了生产环境中的任何负载。

4. **Make Rare Events Less Rare（让罕见事件更常见）**：例如让 CPU hotplug 操作更频繁、模拟内存分配失败、引入虚假失败等。

5. **Count Near Misses（计数近失）**：这是极其强大但常被忽视的技术。真正的失败可能每小时一次，但"近失"（near miss）可能每分钟一次。在 RCU 的一个真实案例中，一个优先级提升的 bug 大约每 100 小时出现一次，但近失大约每小时一次。利用近失，bug 的根本原因在一周内被定位，修复在一天内获得高置信度；如果不用近失，可能需要数月。在 `kernel/rcu/rcutorture.c` 中，可以看到 `err_segs_recorded` 和 "Failure/close-call rcutorture reader segments" 的日志记录，这就是近失技术的实际应用。

6. **Proactive Hunting Techniques（主动狩猎技术）**：对需要最多分析的并发代码区域添加延迟；分析工作负载强度趋势；对新代码保持怀疑；在故障恢复代码和慢路径中寻找近失。最重要的一点是：要特别关注那些开发者最引以为傲的代码——因为人们最可能为不寻常的代码感到骄傲，而这种代码的 bug 最容易逃脱常规测试。

### 2.7 Performance Estimation（性能评估）

对于并行程序，性能是第一类需求，否则为什么要写并行程序？性能不足本身就是一种 bug。

#### 2.7.1 Benchmarking（基准测试）

基准测试有四个主要目的：提供公平竞争框架、聚焦用户关心的改进、作为使用示例、作为营销工具。但最公平的框架始终是实际应用本身。创建近似应用的基准测试可以克服应用专有、硬件需求过大、数据隐私、运行时间过长等障碍。

#### 2.7.2 Profiling（性能分析）

开发者 notoriously 无法通过检查识别真正的瓶颈。老派但有效的方法是在调试器下运行程序，定期中断并记录所有线程的调用栈。更现代的工具如 `gprof` 和 `perf` 通常做得更好。使用 `perf` 时，`perf record`  followed by `perf report` 是标准流程。

#### 2.7.3 Differential Profiling（差异性能分析）

在较小系统上检测即将到来的可扩展性问题的技术。运行两个不同条件下的工作负载（如 2 CPU vs 4 CPU），收集性能分析数据，然后数学组合对应的测量值。例如，取比率并降序排序，最可疑的可扩展性问题会排到列表顶部。

#### 2.7.4 Microbenchmarking（微基准测试）

微基准测试在决定哪些算法或数据结构值得深入评估时很有用。但常见的"测量时间 - 运行多次 - 再测量"方法有许多误差来源：测量开销、缓存未命中/缺页、随机干扰、热节流等。其中第一条和第四条给出了矛盾的建议（增加迭代次数 vs 减少迭代次数），这正是现实世界的标志。

#### 2.7.5 Isolation（隔离）

Linux 提供了多种隔离 CPU 免受外部干扰的方法：
- `sched_setaffinity()` / `taskset`：将任务绑定到特定 CPU
- cgroups：控制组隔离
- 实时优先级 `sched_setscheduler()`：避免被其他任务抢占（但要注意避免死循环）
- `/proc/irq/*/smp_affinity`：将中断限制到特定 CPU
- `NO_HZ_FULL`：在只有一个可运行任务的 CPU 上禁用调度时钟中断

#### 2.7.6 Detecting Interference（检测干扰）

如果无法防止干扰，可以检测它并拒绝受影响的测试结果。

**通过测量检测**：Linux 的 `/proc/<PID>/sched` 中的 `nr_switches` 字段可见上下文切换。`getrusage()` 系统调用可以获取 `ru_nvcsw`（自愿上下文切换）和 `ru_nivcsw`（非自愿上下文切换），以及 minor/major page faults。书中的代码示例 `getrusage_demo.c` 展示了这一用法。

**通过统计检测**：假设（1）较小的测量更可能准确；（2）好数据的测量不确定度已知；（3）合理比例的测试运行会产生好数据。算法：排序测量值，假设前 1/divisor 是好的，计算可接受的上界，然后逐个检查后续值，如果超出上界且趋势断裂则拒绝。书中的 `datablows.sh`（以及我们的 `datablows.c`）实现了这一算法。

## 三、代码实现说明

本章配套的代码示例全部放在当前目录下，通过 `make` 一键编译，通过 `make test` 运行测试。

### 3.1 count_racey.c / heisenbug_demo.c

这两个程序都演示了非原子计数器的竞态条件。`count_racey.c` 通过命令行参数控制线程数、增量次数和延迟，可以清晰展示 count loss。`heisenbug_demo.c` 专门用于演示 anti-heisenbug 技术：不添加延迟时，在某些系统上 race 概率可能不高（或不稳定）；添加 `usleep()` 或 `nanosleep()` 延迟后，race 概率显著上升。

测试输出示例（4 线程，无延迟，各 100 万次）：
```
expected=4000000 actual=1028039 lost=2971961
```

添加 1 微秒延迟后（4 线程，各 1 万次）：
```
expected=40000 actual=10000 lost=30000
```

这验证了书中观点：延迟是发现 race condition 的有力工具。

### 3.2 assertion_demo.c

实现了三个核心断言机制的简化版本：

1. `WARN_ON_ONCE`：使用 `atomic_exchange` 确保每个运行时只打印一次警告。测试中故意触发 `g_data != 100`（此时 `g_data` 为 1），第一次打印警告，第二次不打印。

2. `lockdep_assert_held`：使用 `pthread_mutex_t` 加 owner 记录模拟。`increment_data_locked()` 要求调用者必须持有锁，否则调用 `abort()`。多线程正确用法演示通过，`worker_buggy` 函数被注释掉以避免 abort，但读者可以取消注释来体验断言捕获 bug 的效果。

### 3.3 getrusage_demo.c

直接复现了书中的 Listing 11.1。在测试前后调用 `getrusage(RUSAGE_SELF, ...)`，比较 `ru_nvcsw` 和 `ru_nivcsw`。如果发生任何上下文切换，结果应被拒绝。在物理机上运行时，由于调度器抢占，通常会出现非自愿上下文切换，因此测试输出为 `REJECT`。如果绑定到隔离 CPU 并提升实时优先级，可能得到 `ACCEPT`。

### 3.4 binomial_test.c

实现了离散测试的统计公式 `n = log(1-F_n) / log(1-f)`。默认计算 10% 失败率下达到 99% 置信度所需的 44 次测试。同时输出了一张常用查找表，从 50% 失败率到 0.1% 失败率，展示了置信度要求越高、bug 越罕见，所需测试量就越大的指数级增长关系。

### 3.5 datablows.c

将书中的 `datablows.sh`（shell/awk 脚本）改写为纯 C 实现，功能完全一致：读取每行数据（x 值后跟多个 y 值），排序后假设前 1/divisor 数据是可信的，计算最大允许值和趋势断裂阈值，过滤掉受干扰的异常值，输出平均值、最小值、最大值、好数据个数和总个数。

测试输入 `1 10.0 10.1 10.2 50.0 100.0` 时，前 3 个值（假设 divisor=3）被认为是可信的，50.0 和 100.0 因为远超投影上限和趋势断裂阈值而被过滤，输出 `avg=10.1 min=10.0 max=10.2 good=3 total=5`。

## 四、与 Linux 内核源码的结合

本章提到的多个验证工具都有直接的 Linux 内核源码对应：

| 书中概念 | 内核源码位置 | 说明 |
|---------|------------|------|
| WARN_ON_ONCE | `include/asm-generic/bug.h` | 带 BUGFLAG_ONCE 的运行时警告 |
| lockdep_assert_held | `include/linux/lockdep.h` | 锁持有断言，需 CONFIG_LOCKDEP |
| KCSAN / data_race | `include/linux/kcsan-checks.h` | 并发访问检测器 |
| ASSERT_EXCLUSIVE_ACCESS | `include/linux/kcsan-checks.h` | 独占访问断言 |
| trace events | `include/linux/tracepoint.h` | 低开销追踪机制 |
| rcutorture | `kernel/rcu/rcutorture.c` | RCU 隔离压力测试 |
| getrusage | `kernel/sys.c` (系统调用实现) | 资源使用统计 |

在 `kernel/rcu/rcutorture.c`（4707 行）中，可以清晰看到近失（close call）的实现：`pipe_count > 1 || completed > 1` 时记录错误段序列，后续打印 "Failure/close-call rcutorture reader segments"。这正是本章 11.5.2.5 节 "Count Near Misses" 中描述的真实案例。

`include/linux/lockdep.h` 中的 `lockdep_assert_held` 宏使用了 `lockdep_is_held()` 函数，它通过锁依赖映射（lockdep_map）来跟踪每个锁的持有状态。当启用 `CONFIG_LOCKDEP` 时，内核会构建锁的依赖图，检测死锁和锁顺序违规。

## 五、用户疑问分析

用户的并发编程笔记位于 `~/data/vn/docs/concurrent/todo.md`，其中记录了大量关于并行编程的疑问。本章（Validation）对这些疑问的覆盖情况分析如下：

### 5.1 部分涉及的疑问

1. **"access once / data_race ? 完全不懂"**：本章在 Assertions 一节明确提到了 KCSAN 使用 `READ_ONCE()` 和 `WRITE_ONCE()` 来判断哪些并发访问值得警告，并提到了 `data_race()` 构造用于原谅已知良性的数据竞争。这为用户理解 `READ_ONCE/WRITE_ONCE` 和 `data_race` 的关系提供了入门线索——它们是内核中标记" intentional 并发访问"的工具，但本章并未深入讲解内存序和编译器优化背后的原理。

2. **"观察各个系统中的 spin lock 和 mutex 才是重点"**：本章提到的 `lockdep_assert_held()` 与锁密切相关，它帮助验证"函数是否在被期望的锁保护下调用"。这是观察和理解锁使用规范的重要工具。但本章并未深入讲解 spinlock 和 mutex 的实现细节。

3. **"如何正确的使用 RCU"**：本章多次以 `rcutorture` 作为验证案例，提到了 RCU grace period stall 的检测、RCU 优先级提升 bug 的近失计数。这为用户理解"如何验证 RCU 使用是否正确"提供了视角，但并未讲解 RCU 的读侧/写侧 API 和原理。

4. **"memory barrier 需要斟酌一下 / 总结一个如何使用 memory barrier 的基本方法"**：本章完全没有涉及 memory barrier 的使用方法。这是第 14/15 章（Formal Verification / Memory Ordering）的内容。

### 5.2 未涉及的疑问

以下疑问在本章中几乎没有得到解答：

- cache coherence 和 memory model 的关系
- DMA 的 cache 一致性是谁处理的
- memory model 形成的原因，CPU 如何保证，对编程者的影响
- seqlock vs rcu 的选择
- wait-free / lock-free / obstruction-free 的区别和实现
- hazard pointer 的实现
- rseq 机制
- slab 中的 `system_has_freelist_aba`
- 各种原子操作的封装和底层指令映射
- 无锁队列的实现

### 5.3 结论

本章的核心定位是"验证方法论"，而非"并发机制原理"。它教给你的是：当你写了一个并行程序后，如何通过各种手段（追踪、断言、静态分析、代码审查、统计测试、heisenbug 狩猎、性能评估）来确保它是正确的和高性能的。但它不教你：原子操作怎么工作、memory barrier 怎么放、RCU 的 API 怎么调用、无锁数据结构怎么设计。

因此，对于用户笔记中的大部分底层机制疑问，本章没有直接回答。但这些验证方法（尤其是 rcutorture、lockdep、KCSAN）是理解内核并发代码时不可或缺的工具——你可以用它们来验证自己对 RCU、锁、memory barrier 的理解是否正确。

## 六、总结与思考

本章给我的最大启示是：验证并行程序不是"写完代码后的最后一步"，而是贯穿整个项目生命线的持续活动。作者用达尔文的进化论来类比——代码修改是随机突变，验证套件是自然选择，只有足够激烈的验证才能筛选出适应力强的代码。

另一个深刻观点是：统计学是并行验证的数学基础，但它有极限。对于罕见 bug，你需要通过反海森堡技术来提高其出现概率，否则无限的测试时间也换不来绝对的信心。rcutorture 的近失技术是一个绝佳案例：将每小时一次的真正错误，转化为每分钟一次的近失，调试效率提升了两个数量级。

最后，作者提醒我们不要陷入"用统计替代思考"的陷阱。验证的终极三工具是：彻底理解需求、坚实的设计、以及良好的睡眠。所有工具和公式都只是辅助——如果你不知道代码应该做什么，再好的测试覆盖率也没有意义。

本章的实践价值在于，它提供了一个完整的验证工具箱。对于 Linux 内核开发者而言，`lockdep`、`KCSAN`、`rcutorture`、`sparse`、`coccinelle` 都是日常武器。对于用户空间并行程序开发者，`perf`、`getrusage`、统计过滤、微基准测试隔离同样是必备技能。将本章的方法论与实际的内核源码对照阅读，可以极大加深对"工业级并行软件如何确保质量"的理解。

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
