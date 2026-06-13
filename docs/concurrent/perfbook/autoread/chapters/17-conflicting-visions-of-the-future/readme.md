# 第17章：Conflicting Visions of the Future - 详细分析
FIXME

## 目录

1. [CPU技术未来愿景的回顾](#1-cpu技术未来愿景的回顾)
2. [事务内存（Transactional Memory）](#2-事务内存transactional-memory)
3. [硬件事务内存（HTM）](#3-硬件事务内存htm)
4. [形式化回归测试](#4-形式化回归测试)
5. [函数式编程与并行](#5-函数式编程与并行)
6. [Linux内核源码关联](#6-linux内核源码关联)
7. [代码示例与测试](#7-代码示例与测试)
8. [与个人笔记疑问的对照](#8-与个人笔记疑问的对照)
9. [总结](#9-总结)

---

## 1. CPU技术未来愿景的回顾

本章开篇以Niels Bohr的名言"预测未来非常困难，尤其是关于未来"点题，随后回顾了2004年前后对CPU技术未来的五种预测场景。这些预测在今天看来具有深刻的启示意义。

FIXME 这个翻译就是一坨狗屎，到时候直接看看吧
1.1 Uniprocessor Uber Alles（单核至上）
1.2 Multithreaded Mania（多线程狂热）
1.3 More of the Same（维持现状）
1.4 Crash Dummies Slamming into the Memory Wall（撞向内存墙）
1.5 Astounding Accelerators（惊人的加速器）


## 2. 事务内存（Transactional Memory）

事务内存（TM）是本章的核心内容之一。其基本思想是将一段代码原子执行，使得其他线程看不到中间状态。TM的实现（无论是硬件还是软件）的复杂性主要在于高效检测并发事务何时可以安全并行运行。由于这种检测是动态的，冲突的事务可以被中止并回滚。

### 2.1 与外部世界的交互（Outside World）

TM面临的最大挑战之一是不可撤销操作（irrevocable operations）。

#### I/O操作

事务可能因冲突而回滚，这要求事务内的所有操作必须是可撤销的。然而I/O通常是典型的不可撤销操作。可选方案包括：
- 限制事务内I/O为带内存缓冲的缓冲I/O
- 禁止事务内I/O，使任何I/O尝试中止事务
-  enlist编译器协助执行此禁止
- 允许一次特殊的"不可撤销事务"（irrevocable transaction），但这严重限制了I/O的可扩展性和性能
- 创建新硬件和协议将I/O拉入事务基板

**关键洞察**：I/O是TM的已知弱点，目前尚不清楚是否存在一个既保持可用性能又可扩展的通用解决方案。

#### RPC操作

如果RPC请求和响应都要包含在事务内，且事务的某部分依赖响应结果，那么无法使用缓冲技巧。因为事务的内存足迹无法在收到RPC响应前确定，而事务的内存足迹不确定就无法判断事务是否可以提交。唯一符合事务语义的操作是无条件中止事务。

#### 时间延迟

事务内的时间延迟与TM的原子性属性相悖。选项包括忽略、中止事务、让编译器禁止、或让时间延迟正常执行。但这些方案都有问题，尤其是当TM实现仅在提交时才发布修改时，时间延迟的目的可能被挫败。

#### 持久性（Persistence）

持久锁（如flock、lockf、System V信号量）可以独立于使用锁的进程地址空间存在。TM作为"事务内存"，其名称本身就与持久事务的概念冲突。提供持久事务功能的选项包括限制到SQL等特殊环境、使用存储设备的快照功能、或完全避免在事务内使用持久设施。

### 2.2 进程修改（Process Modification）

#### 多线程事务

在锁保护下创建线程是完全合法且简单的。但在TM中，如果一个事务内调用pthread_create()，面临的选择包括：
- 声明pthread_create()在事务内非法
- 只允许父线程属于事务
- 将pthread_create()转换为函数调用
- 扩展事务覆盖父线程和所有子线程

数据库世界中并行执行事务很常见，但当前的TM提案大多未提供此功能。

#### exec()系统调用

在锁保护下执行exec()有明确的语义：非持久锁随地址空间消失，持久锁则保留。但在TM中，exec()的处理方式尚不明确。C++ TM草案规范采用transaction_safe和transaction_unsafe属性来装饰函数，但这需要装饰大量库函数。

#### 动态链接和加载

动态加载的函数代码在编译时是不可知的。TM面临的问题包括：如何在事务内动态链接加载函数，以及如何处理函数内可能存在的TM不友好操作。选项包括：像处理页面错误一样处理动态加载（可能中止事务后重试）、或从事务内禁止动态链接和加载。

#### 内存映射操作

在事务内执行mmap/munmap的复杂性在于，如果重映射的区域包含参与当前或其他线程事务的变量，结果难以定义。

#### 调试

在初始HTM硬件实现中，事务内的异常会中止事务，这意味着断点会中止所有封闭事务。调试选项包括：在事务内使用软件仿真、使用能处理断点异常的硬件TM、或仅使用软件TM。

### 2.3 同步机制交互

#### 与锁的交互

理论上，从事务内获取锁只需将锁数据结构作为事务的一部分操作。但实际上，根据TM系统的实现细节，会产生大量非显而易见的复杂性。研究表明，解决这些问题会使锁获取开销增加45%（事务外）到300%（事务内）。

#### 与读写锁的交互

传统基于计数器的读写锁在TM中的问题是：两个事务并发读获取同一个读写锁时，读获取涉及修改锁的数据结构，导致冲突并回滚其中一个事务。这完全违背了读写锁允许并发读者的目标。解决方案包括使用per-CPU/per-thread读写锁（但写获取开销极高）。

#### 与延迟回收（RCU等）的交互

RCU允许读者和更新者并发运行，这直接违背了TM的原子性属性。可能的交互方式包括：
- RCU读者中止冲突的TM更新（TxLinux的做法）
- RCU读者获得旧值（pre-transaction）
- 延迟RCU访问直到冲突事务提交或中止
- 将RCU读者转换为事务
- 禁止在RCU更新中使用TM

有趣的是，许多高性能STM实现内部使用了RCU类似的技术。

### 2.4 其他事务问题

#### 争用管理（Contention Management）

当多个事务访问同一变量时，需要选择中止哪些事务。Scherer和Scott的论文描述了多种策略：指数退避、优先中止工作量最少的事务、跟踪事务被中止次数、使用时间戳等。2005年的这篇论文就预示了争用管理的内在复杂性。

#### Seqlock作为受限的STM

本章特别指出，**seqlock可以被视为一种受限的STM形式**。seqlock的读端临界区通常仅限于load操作，在冲突时重试。关键的是，seqlock读者之间不会冲突。seqlock处理外部世界交互的方式很简单：在每次读端临界区遍历时执行TM不友好的操作。如果重试三次，I/O写操作就会执行四次。

seqlock将STM的许多挑战推回给用户，这是合理的，因为seqlock没有要成为"统治一切的单一同步机制"的野心。**或许阻碍STM广泛采用的最大障碍，正是其支持者对完全通用性的过度渴望**。

---

## 3. 硬件事务内存（HTM）

截至2021年，HTM已在多种商用系统上可用多年，包括Sun Rock、Azul Vega、IBM Blue Gene/Q、Intel Haswell TSX和IBM System z。

### 3.1 HTM的核心概念

HTM使用处理器缓存和推测执行来使一组语句从事务的角度原子执行。事务由begin-transaction机器指令发起，由commit-transaction指令完成。通常还有abort-transaction指令，用于清除推测并跳转到失败处理器。

### 3.2 HTM的优势

1. **避免同步缓存未命中**：HTM通过CPU缓存同步，不需要单独的同步数据结构
2. **数据结构动态分区**：对于难以静态分区的树和图结构，HTM可以自动动态分区
3. **实际应用价值**：已有大量实际应用，包括锁消除（TLE）

### 3.3 HTM的弱点

#### 事务大小限制

当前HTM实现使用处理器缓存保存事务影响的数据。但缓存的关联性（associativity）是关键限制因素。例如，如果事务需要触及9个缓存行且都映射到同一个缓存集（set），则无论缓存有多少MB空间，该事务都无法完成。

UTM（Unbounded Transactional Memory）方案使用DRAM作为极大的victim cache，但将其集成到生产级缓存一致性机制中仍是未解决的问题。

#### 冲突处理

当两个并发事务访问同一变量且至少一个是store时，发生"冲突"。系统必须中止一个或两个事务。选择哪个事务中止是一个复杂问题，已产生大量研究。

#### 中止和回滚

任何事务都可能随时被中止，因此事务不能包含无法回滚的语句。这意味着事务不能做I/O、系统调用或调试断点。此外，在某些系统上，中断、异常、陷阱、TLB未命中也会中止事务。

当前HTM实现可能确定性中止给定事务，因此软件必须提供fallback代码。如果fallback使用锁，那么死锁等所有锁的限制都会重新出现。这引发了"旅鼠效应"（lemming effect）的问题：系统可能陷入fallback模式而无法恢复。

#### 缺乏前进进度保证

除了低时钟频率的学术研究原型外，当前可用的HTM实现拒绝提供任何前进进度保证。因此HTM不能用于避免死锁。**IBM大型机是个例外**，它提供"受限事务"（constrained transactions），但约束极为严格：最多4个内存块（每块32字节）、最大代码足迹256字节、最多32条汇编指令、禁止向后分支等。

#### 不可撤销操作

HTM事务不能容纳不可撤销操作。当前实现通常通过要求所有访问都针对可缓存内存（禁止MMIO）并在中断/陷阱/异常时中止事务（禁止系统调用）来强制执行此限制。

#### 语义差异

HTM在语义上与锁存在微妙差异。一个特别恶劣的例子是**空临界段**（empty critical section）：在基于锁的程序中，空临界段保证所有先前持有该锁的进程已释放它。但如果将空临界段翻译为事务，结果是一个无操作（no-op），失去了时间同步的消息语义。

另一个例子是**优先级提升**（priority boosting）。在基于锁的实时程序中，空临界段结合优先级提升可确保低优先级线程定期运行。但使用TLE时，空临界段变成空事务（无效），优先级提升机制完全失效。

### 3.4 增强锁与HTM的对比

当锁与RCU或Hazard Pointers结合时，对比更加明显：
- 非阻塞读端机制缓解了死锁问题
- 读端机制可高效处理不可分区数据
- Hazard Pointers和RCU不相互争用，也不与更新者争用
- Hazard Pointers和RCU提供前进进度保证
- 私有化操作直观简单

**Table 17.3: Comparison of Locking (Plain and Augmented) and HTM ([+] Advantage, [-] Disadvantage, [--] Strong Disadvantage)**

| Category                              | Locking                                                                                                         | Locking with RCU/Hazard Pointers                                                                                                                                                                                                 | HTM                                                                                                                                                                            |
|---------------------------------------|-----------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Basic Idea**                        | Allow only one thread at a time to access a given set of objects.                                               | Allow only one thread at a time to access a given set of objects.                                                                                                                                                                | Cause a given operation over a set of objects to execute atomically.                                                                                                           |
| **Scope**                             | [+] Handles all operations.                                                                                     | [+] Handles all operations.                                                                                                                                                                                                      | [+] Handles revocable operations.<br>[-] Irrevocable operations force fallback (typically to locking).                                                                         |
| **Composability**                     | [--] Limited by deadlock.                                                                                       | [+] Readers limited only by grace-period-wait operations.<br>[-] Updaters limited by deadlock. Readers reduce deadlock.                                                                                                          | [--] Limited by irrevocable operations, transaction size, and deadlock (assuming lock-based fallback code).                                                                    |
| **Scalability & Performance**         | [-] Data must be partitionable to avoid lock contention.                                                        | [-] Data must be partitionable to avoid lock contention among updaters.<br>[+] Partitioning not needed for readers.                                                                                                              | [-] Data must be partitionable to avoid conflicts.                                                                                                                             |
| **Scalability & Performance (cont.)** | [--] Partitioning must typically be fixed at design time.                                                       | [--] Partitioning for updaters must typically be fixed at design time.<br>[+] Partitioning not needed for readers.                                                                                                               | [+] Dynamic adjustment of partitioning carried out automatically down to cacheline boundaries.<br>[-] Partitioning required for fallbacks (less important for rare fallbacks). |
| **Scalability & Performance (cont.)** | [--] Locking primitives typically result in expensive cache misses and memory-barrier instructions.             | [--] Updater locking primitives typically result in expensive cache misses and memory-barrier instructions.                                                                                                                      | [-] Transactions begin/end instructions typically do not result in cache misses, but do have memory-ordering and overhead consequences.                                        |
| **Scalability & Performance (cont.)** | [+] Contention effects are focused on acquisition and release, so that the critical section runs at full speed. | [+] Update-side contention effects are focused on acquisition and release, so that the critical section runs at full speed.<br>[+] Readers do not contend with updaters or with each other.                                      | [-] Contention aborts conflicting transactions, even if they have been running for a long time.                                                                                |
| **Scalability & Performance (cont.)** |                                                                                                                 | [+] Read-side primitives are typically bounded wait-free with low overhead (lock-free with low overhead for hazard pointers).                                                                                                    | [-] Read-only transactions subject to conflicts and rollbacks. No forward-progress guarantees other than those supplied by fallback code.                                      |
| **Scalability & Performance (cont.)** | [+] Privatization operations are simple, intuitive, performant, and scalable.                                   | [+] Privatization operations are simple, intuitive, performant, and scalable when data is visible only to updaters.<br>[-] Privatization operations are expensive (though still intuitive and scalable) for reader-visible data. | [-] Privatized data contributes to transaction size.                                                                                                                           |
| **Hardware Support**                  | [+] Commodity hardware suffices.<br>[+] Performance is insensitive to cache-geometry details.                   | [+] Commodity hardware suffices.<br>[+] Performance is insensitive to cache-geometry details.                                                                                                                                    | [-] New hardware required (and is starting to become available).<br>[-] Performance depends critically on cache geometry.                                                      |
| **Software Support**                  | [+] APIs exist, large body of code and experience, debuggers operate naturally.                                 | [+] APIs exist, large body of code and experience, debuggers operate naturally.                                                                                                                                                  | [-] APIs emerging, little experience outside of DBMS, breakpoints mid-transaction can be problematic.                                                                          |
| **Interaction With Other Mechanisms** | [+] Long experience of successful interaction.                                                                  | [+] Long experience of successful interaction.                                                                                                                                                                                   | [--] Just beginning investigation of interaction.                                                                                                                              |
| **Practical Apps**                    | [+] Yes.                                                                                                        | [+] Yes.                                                                                                                                                                                                                         | [+] Yes.                                                                                                                                                                       |
| **Wide Applicability**                | [+] Yes.                                                                                                        | [+] Yes.                                                                                                                                                                                                                         | [-] Jury still out.                                                                                                                                                            |

### 3.5 HTM最适合的位置

HTM似乎最适合：更新密集型工作负载、对相对大型内存数据结构的相对较小更改、在大型多处理器上运行。这满足当前HTM实现的大小限制，同时最小化冲突概率。

与RCU或Hazard Pointers结合使用可以缓解HTM的事务大小限制，用于遍历数据结构大部分内容的只读操作。

---

## 4. 形式化回归测试

形式化验证在生产环境中已有长期应用历史，但硬核形式化验证能否纳入复杂并发代码库（如Linux内核）的自动化回归测试套件仍是一个开放问题。

### 4.1 纳入回归测试的要求

1. **自动翻译**：手动将C代码翻译为Promela等形式化语言不可持续
2. **环境建模**：必须正确处理内存模型
3. **开销可接受**：形式验证工具通常是指数级的
4. **定位bug**：需要提供bug位置信息
5. **最小化脚手架**：不需要完整的数学规范
6. **相关bug**：应优先定位最麻烦的bug

### 4.2 工具对比

| 工具 | 自动翻译 | 环境/内存模型 | 开销 | 定位bug | 最小脚手架 |
|------|---------|--------------|------|--------|-----------|
| Promela/spin | 手动 | 仅顺序一致性 | 中等 | 有traceback | 好 |
| PPCMEM | 手动（litmus） | 多种模型 | 高 | 图形显示 | 一般 |
| herd | 手动（litmus） | 多种（含LKMM） | 中等 | 图形显示 | 一般 |
| cbmc | 直接输入C | 少量模型 | 中等 | traceback | 好 |
| Nidhugg | 直接输入C | 少量模型 | 低 | traceback | 好 |

**关键发现**：使用API边界进行"分而治之"可以产生组合爆炸的逆过程——组合内爆（combinatorial implosion）。例如，先验证锁实现正确，再验证锁API的正确使用。

本章还展示了用`cmpxchg_acquire()`模拟`spin_lock()`与直接使用模型内置锁的性能对比：在4个进程时，模型比模拟快两个数量级以上（0.374秒 vs 59.565秒）。这证明了分层验证的重要性。

### 4.3 相关bug的悖论

一个反直觉的发现是：修复极罕见（每百万年出现一次）的bug实际上可能降低软件可靠性。因为约7%的修复会引入新bug，而修复后的新bug通常比原来的罕见bug更频繁地出现。

---

## 5. 函数式编程与并行

函数式编程曾被认为特别适合并行化，但三十多年后，主流生产环境仍使用C、C++、Java、Fortran等过程式语言。

### 5.1 将过程式代码转换为函数式代码的障碍

1. **全局变量**：过程式语言大量使用可被多线程独立更新的全局变量
2. **同步原语**：锁、原子操作、事务等对函数式模型构成冲击
3. **别名（Aliasing）**：函数参数可能通过不同指针别名指向同一结构

### 5.2 单静态赋值（SSA）形式验证

一个有趣的方向是将并行原语转换为SSA形式，然后结合断言进行可满足性（SAT）检查。这种方法已被应用于验证Linux内核的Tree RCU实现（使用Nidhugg工具）。

---

## 6. Linux内核源码关联

### 6.1 TSX控制：arch/x86/kernel/cpu/tsx.c

Linux内核对Intel TSX的处理集中在`arch/x86/kernel/cpu/tsx.c`。由于TAA（TSX Async Abort）等侧信道攻击，现代Intel处理器在微码更新后默认禁用TSX。内核提供以下控制：

- `tsx=on|off|auto` 内核命令行参数
- 通过`MSR_IA32_TSX_CTRL` MSR禁用/启用TSX
- 清除CPUID中的RTM和HLE标志位，避免用户空间浪费资源尝试注定失败的事务

当前测试的CPU（13th Gen Intel i9-13900K）已通过微码禁用TSX，因此任何`_xbegin()`调用都会立即中止。

### 6.2 Seqlock实现：include/linux/seqlock.h

内核seqlock是本章重点讨论的"受限STM"的生产级实现。关键设计：
- `read_seqcount_begin()`：获取sequence计数（等待writer完成）
- `read_seqcount_retry()`：检查sequence是否变化（含`smp_rmb()`）
- `write_seqcount_begin/end()`：递增sequence（需外部锁保护写者串行化）
- 支持多种关联锁：raw_spinlock、spinlock、rwlock、mutex

特别值得注意`raw_seqcount_try_begin()`的注释："当计数为奇数时，可以完全跳过临界区，而不是明知会失败还做推测。"

### 6.3 内存一致性模型：tools/memory-model/

Linux内核包含完整的内存一致性模型（LKMM），使用"cat"语言编写，可由`herd7`工具执行。这直接对应本章讨论的形式化验证工具：
- `linux-kernel.cat`：定义memory order、barrier、atomic、RCU的约束
- `linux-kernel.bell`：分类相关指令
- `litmus-tests/`：大量litmus测试用例
- `klitmus7`：可将litmus测试转换为内核模块

内核v4.17正式接受了LKMM，这是形式化验证融入内核开发的重要里程碑。

---

## 7. 代码示例与测试

本章配套三个C代码示例，均在本地编译测试通过。

### 7.1 seqlock_demo.c - Seqlock作为受限STM

实现了用户空间seqlock，展示：
- 读者通过sequence计数检测写者活动
- 读者之间无冲突（与STM读者不同）
- 读者可包含I/O、时间延迟等"TM不友好"操作
- 写者通过mutex串行化（seqlock本身不提供写者互斥）

测试结果：4读者2写者各执行100万次迭代，reader retry次数在2-8万次之间，无一致性错误。

### 7.2 tsx_demo.c - TSX/HTM使用与Fallback

演示：
- `_xbegin()`/`_xend()`事务API的使用
- 事务中止原因分析（冲突、容量、显式中止等）
- Fallback到pthread_mutex的"事务锁消除"模式
- 运行时通过CPUID检查RTM支持
- 空临界段的语义差异演示

由于测试CPU（i9-13900K）已禁用TSX，demo正确检测到不支持并跳过事务测试，仅展示语义差异部分。

### 7.3 priority_boost_demo.c - 优先级提升与HTM语义差异

直接演示本章Listing 17.3中的概念：
- `boostee()`始终持有两个锁之一
- `booster()`通过空临界段确保`boostee()`定期运行
- 测试显示在1秒内`boostee`运行超过6600次
- 如果HTM消除了空临界段，`boostee`将永远无法运行

### 编译与运行

```bash
make all    # 编译所有示例
make test   # 运行所有测试
make clean  # 清理
```

所有二进制文件以`.out`结尾，可被`.gitignore`自动忽略。

---

## 8. 与个人笔记疑问的对照

对照`~/data/vn/docs/concurrent/todo.md`中的笔记和疑问：

### 已部分解答的疑问

1. **"atomic 的封装也是值得一看"**（笔记第129行）：本章深入讨论了原子操作与事务内存的交互，尤其是HTM使用缓存一致性实现原子性，以及原子操作如何在TM中造成冲突。

2. **"为什么语言的判断是"**（笔记第56行，关于lockbud）：本章解释了TM与锁的交互复杂性，包括C++ TM草案中的`transaction_safe`/`transaction_unsafe`属性。这解释了为什么静态分析工具需要特殊处理并发原语。

3. **"初识事务内存"**（transctiona-memory.md）：本章提供了远比该知乎链接更深入的TM分析，包括I/O、RPC、exec、动态加载等实际挑战。

4. **"hazard pointer"**（笔记第91行）：本章多处讨论Hazard Pointers与TM/HTM的交互，以及如何用HP增强锁来避免死锁。

5. **"spin lock"**（笔记第430行）：本章详细对比了锁与HTM，包括锁消除（TLE）的开销和语义差异。

### 尚未直接解答的疑问

1. **"memory barrier 需要斟酌一下"**（todo.md第2行）：本章主要关注TM而非memory barrier，但LKMM部分涉及了内存模型。

2. **"如何正确的使用 RCU"**（todo.md第12行）：本章讨论了RCU与TM的交互，但没有深入RCU的使用方法本身。

3. **"cache coherence 和 memory model 的关系"**（todo.md第14行）：本章没有深入讨论这个基础问题。

4. **"什么时候应该使用 seqlock 而不是 rcu"**（todo.md第230行）：本章将seqlock定位为"受限的STM"，但没有直接对比seqlock和RCU的选择标准。

5. **"DMA 的 cache 一致性是谁处理的"**（todo.md第15行）：未涉及。

6. **"bitmap 和 concurrent 问题"**（笔记第436行）：未涉及。

---

## 9. 总结

第17章的核心信息是：**对未来技术的预测往往失败，但对这些技术影响力的理解仍然重要**。

关于事务内存，本章传达了几个关键教训：

1. **TM不是银弹**：尽管TM承诺简化并行编程，但它引入了失败、回滚、容量限制、语义差异等新复杂性。

2. **HTM是工具箱的一员**：当前HTM实现有严重限制（大小、冲突、回滚、无前进进度保证），但仍是并行编程工具箱中受欢迎且有用的补充。

3. **seqlock是生产中的受限STM**：seqlock处理了许多STM挑战，且在生产中大量使用。它的成功部分源于没有"统治一切的野心"。

4. **形式化验证在回归测试中的潜力**：虽然当前工具大多不适合自动化回归测试，但cbmc、Nidhugg等工具正在向这个目标前进。Linux内核的LKMM是形式化验证融入实际开发的重要范例。

5. **函数式编程尚未统治并行领域**：尽管有形式化验证优势，过程式语言仍是主流。SSA转换和SAT求解为验证过程式并行代码提供了新途径。

正如本章结语所言：未来可能比我们想象的更加奇特。重要的不是准确预测哪种技术会成功，而是理解每种技术的优缺点，以便在技术生态演化的过程中做出明智的选择。

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
