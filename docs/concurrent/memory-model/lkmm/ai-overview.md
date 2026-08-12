## 3. 内核源码中的 LKMM

### 3.1 源码位置

```
linux/tools/memory-model/
|-- linux-kernel.cat      # 核心模型定义（cat 语言）
|-- linux-kernel.bell     # 事件类型分类
|-- linux-kernel.def      # C 语法到 herd7 指令的映射
|-- linux-kernel.cfg      # herd7 配置文件
|-- lock.cat              # 锁语义定义
|-- README                # 使用说明
|-- Documentation/        # 详细文档
|   |-- explanation.txt   # 26 章详细解释（2810 行）
|   |-- litmus-tests.txt  # Litmus test 格式教程
|   |-- simple.txt        # 简明指南
|-- litmus-tests/         # 官方 litmus tests
|-- scripts/              # 测试脚本
```

### 3.2 linux-kernel.cat 详解

这是 LKMM 的核心，用 cat 语言（由 Luc Maranget 设计）编写。主要部分：

**Coherence（一致性）**

```cat
let co0 = loc & (W * W)
let co = ...  (* 计算 coherence order *)
let com = co | rf | fr
```

`co` 是同一内存位置上所有写操作的全序。`com` (communication) 是 `co`、`rf`、`fr` 的并集。

**Happens-Before（发生先于）**

```cat
let hb = po | ... | rf | ...
```

`hb` 由 program order、各种屏障关系、reads-from 等组合而成。

**Propagation（传播）**

```cat
let pb = ...
```

`pb` 确保写操作能够传播到所有 CPU，是处理弱内存模型的关键。

**RCU**

```cat
let rb = ...
irreflexive rb as rcu
```

`rb` (RCU-before) 约束 RCU 读临界区和 grace period 的关系。

### 3.3 linux-kernel.bell 详解

定义了 herd7 需要识别的事件类型：

```bell
enum Accesses = 'once | 'acquire | 'release | 'noreturn | 'mb
enum Barriers = 'wmb | 'rmb | 'mb | 'release | 'acquire | ...
```

### 3.4 linux-kernel.def 详解

将 C 语言语法映射到 herd7 内部指令：

```def
READ_ONCE(x)          -> R[once] x
WRITE_ONCE(x, v)      -> W[once] x v
smp_load_acquire(&x)   -> R[acquire] x
smp_store_release(&x,v) -> W[release] x v
smp_mb()              -> F[mb]
xchg(&x, v)           -> Rmw[mb] x v
atomic_inc(&x)        -> Rmw[once] x (Add 1)
rcu_read_lock()       -> F[rcu_lock]
rcu_read_unlock()     -> F[rcu_unlock]
synchronize_rcu()     -> F[sync]
```

### 3.5 lock.cat 详解

处理锁的获取和释放语义：

```cat
let lk-rf = ...  (* 锁的 reads-from 关系 *)
let lk-hb = ...  (* 锁的 happens-before *)
```

锁操作引入的 happens-before 关系是 LKMM 的重要组成部分。

---

## 4. Documentation/ 下的文档详解

内核 `tools/memory-model/Documentation/` 目录下包含 13 个文档文件，总计约 7000 行。这些文档按照从入门到精通的顺序组织：

### 4.1 README — 文档导航

这是整个 Documentation 目录的入口。它不做技术讲解，而是根据读者的背景知识水平，推荐不同的阅读路径：

- **完全新手** -> `simple.txt`
- **了解并发基础，想看内核提供的原语概览** -> `ordering.txt`
- **熟悉内核并发原语，想写 litmus tests** -> `litmus-tests.txt`
- **需要在锁保护外访问共享变量** -> `locking.txt`
- **熟悉内核并发，想看常见模式** -> `recipes.txt`
- **想了解编译器对控制依赖的破坏** -> `control-dependencies.txt`
- **需要标记有意的并发访问（应对 KCSAN）** -> `access-marking.txt`
- **需要快速参考** -> `cheatsheet.txt`
- **想了解 LKMM 的设计原理和实现** -> `explanation.txt` + `herd-representation.txt`
- **查找相关论文和资料** -> `references.txt`

### 4.2 simple.txt — 简单并发入门（270 行）

目标读者：对内核并发完全陌生的开发者。

核心内容：

**单线程代码**
- 最简单的方式：用全局锁包裹代码（code locking）
- 警告：Linux 花了很大力气移除了 "Big Kernel Lock"，不要轻易添加新的全局锁
- 锁的优势：几乎所有内核开发者都熟悉，且 lockdep (CONFIG_PROVE_LOCKING=y) 能检测死锁

**使用库函数**
- 内核的 `lib/` 目录、`include/linux/` 目录提供了大量封装好的并发 API
- 例如：list 宏、workqueues、smp_call_function()、各种哈希表和搜索树
- 把并发问题交给库维护者

**数据锁（Data Locking）**
- 与 code locking 相对，将锁与特定数据结构实例关联
- 典型例子：哈希表每个 bucket 一个锁，不同 bucket 的操作可以并发
- 随着 bucket 数量增加，自然扩展

**Per-CPU 处理**
- 将处理和数据分区到各个 CPU，每个 CPU 单线程处理
- 优点：性能和扩展性极佳
- 缺点：内存占用大幅增加
- RCU 大量使用 per-CPU 数据结构

**封装好的无锁原语**

1. **Sequence Locking（顺序锁）**
   - 简单规则："不要在读端代码中写"
   - 复杂用法需要 LKMM 指导（LKMM 目前还不直接支持 seqlock，需要在 litmus tests 中手动展开）

2. **RCU**
   - 简单规则："读端不写入"、"不更新读者可见的数据"、"更新用锁保护"

3. **Atomic 操作**
   - 三类：初始化/读取（atomic_set/atomic_read）、无返回值无序操作（atomic_inc）、有返回值全序操作（atomic_add_return）
   - 新内核增加了 `_relaxed()`、`_acquire()`、`_release()` 后缀的变体

**无锁但全序访问**
- 当必须在无锁情况下访问变量时，只使用全序操作
- 这样保证：对同一变量的访问，所有 CPU 看到的顺序一致

**统计和启发式数据**
- `READ_ONCE()`、`WRITE_ONCE()` 等无序原语可用于统计计数器、启发式参数
- 但 "无序" 真的就是 "无序"，不要假设有顺序

**不要让编译器坑了你**
- 不要用普通 C 语言访问共享变量（plain C-language accesses）
- 现代优化编译器会 profoundly rewrite 你的代码
- 参考 LWN 文章："Who's afraid of a big bad optimizing compiler?"

### 4.3 ordering.txt — 内存排序原语分类（556 行）

目标读者：了解内核并发基础，想看所有低级别排序原语的分类和说明。

核心内容按强度递减组织：

**1. Barriers（屏障/栅栏）**

- **Full Memory Barriers**: `smp_mb()`、有返回值且非 `_acquire/_release/_relaxed` 后缀的 RMW 操作、`synchronize_rcu()` 等 grace period 原语
  - `smp_mb()` 将 CPU 的所有先前访问与所有后续访问排序
  - RMW 操作将访问分为三部分：之前、操作本身、之后
  - RCU grace period 原语开销极大，且只能在可睡眠上下文调用

- **RMW 排序增强屏障**: `smp_mb__before_atomic()`、`smp_mb__after_atomic()`、`smp_mb__after_spinlock()`、`smp_mb__after_srcu_read_unlock()`
  - 解决非返回值 RMW（如 atomic_inc）不提供排序的问题
  - 在 x86 上 atomic_inc 本身就有全序，这些屏障不生成代码；在弱序架构上生成必要指令

- **Write Memory Barrier**: `smp_wmb()`
  - 只排序先前的 store 和后续的 store
  - 通常不如 `smp_store_release()` 好用

- **Read Memory Barrier**: `smp_rmb()`
  - 只排序先前的 load 和后续的 load
  - 通常不如 `smp_load_acquire()` 好用

- **Compiler Barrier**: `barrier()`
  - 只阻止编译器重排，不影响硬件排序
  - 例如防止编译器将代码移到无限循环之后

**2. Ordered Memory Accesses（有序内存访问）**

- **Release Operations**: `smp_store_release()`、`atomic_set_release()`、`rcu_assign_pointer()`
  - 将 store 与 CPU 的所有先前访问排序
  - 通常比显式屏障更易读、性能更好
  - 在 x86 上可能只编译为简单 store 指令

- **Acquire Operations**: `smp_load_acquire()`、`atomic_read_acquire()`
  - 将 load 与 CPU 的所有后续访问排序
  - 通常与 release 操作配对使用

- **RCU Read-Side Ordering**: `rcu_read_lock()`、`rcu_read_unlock()`、`rcu_dereference()`
  - `rcu_dereference()` 通过 address dependency 排序
  - 与 `rcu_assign_pointer()` 配对

- **Control Dependencies**: 通过 `if` 条件从 load 到 store 的依赖
  - 极易被编译器优化破坏，需要特别小心
  - 详见 `control-dependencies.txt`

**3. Unordered Accesses（无序访问）**

- **Unordered Marked Operations**: `READ_ONCE()`、`WRITE_ONCE()`、无返回值 RMW、`_relaxed()` 后缀 RMW
  - 只阻止编译器优化，不提供硬件排序
  - 对同一变量的访问，所有 CPU 会达成一致顺序

- **Unmarked C-Language Accesses**: 普通 C 变量访问
  - 不提供任何排序保证
  - 需要非常深入理解 C 标准和编译器行为

### 4.4 litmus-tests.txt — Litmus Test 格式与技巧（1083 行）

目标读者：熟悉内核并发原语，想写和运行 litmus tests。

核心内容：

**Litmus Test 格式**
- 以 `C` 开头，后跟测试名称
- 初始化部分用 `{ }` 包裹
- 进程用 `P0(int *x, int *y)` 形式定义
- 局部变量用 `r0`、`r1` 等命名
- 断言用 `exists` 或 `forall`
- 断言语法：`/\` 表示 AND，`\/ ` 表示 OR，`~` 表示 NOT，`=` 表示相等

**高级特性**
- `locations [x; y]` 语句让 herd7 显示额外变量
- `filter` 子句过滤执行路径（用于模拟 spin loop）
- 支持 `if` 语句（但不支持 `while`）
- 链表操作：初始化时 `x=y` 表示 x 初始化为 y 的地址

**性能优化技巧**
- 减少进程数量和代码量
- 使用 `filter` 减少状态空间
- 避免不必要的变量

**限制**
- 不支持 `while` 循环（状态空间爆炸）
- 不支持函数调用
- 不支持指针算术（除简单的链表遍历外）

### 4.5 locking.txt — 锁与无锁访问（303 行）

目标读者：需要在不持有锁的情况下访问锁保护的共享变量。

核心内容：

**锁的基本规则**
- 任何 CPU 持有某锁时，能看到之前持有该锁的 CPU 释放前所做的所有更改
- 这个规则比 "看到之前持锁期间的更改" 更强

**Double-Checked Locking（DCL）**
- 错误版本：只用 `READ_ONCE()` 检查 flag，然后加锁
- 修复版本：使用 `smp_load_acquire()` 检查 flag，`smp_store_release()` 设置 flag
- 原因：需要额外的排序保证 flag 和 data 的可见性

**锁对未持锁 CPU 的排序**
- 锁的排序不一定对未持锁的 CPU 可见
- 例子：CPU0 和 CPU1 通过锁操作 x 和 y，CPU2 不持锁但用 `smp_mb()` 观察，可能看到不一致的结果
- 解决：`smp_mb__after_spinlock()` 可以增强锁获取的排序效果

**No Roach-Motel Locking**
- 不允许将临界区外的代码拉入临界区（roach motel：只能进不能出）
- 编译器不能将 spin loop 的代码优化到临界区内部
- 锁原语必须阻止代码进入临界区，而不仅仅是阻止代码离开

### 4.6 recipes.txt — 常见内存排序模式（574 行）

目标读者：熟悉内核并发，想看实际代码中的常见模式。

核心内容：

**Message Passing (MP)**
- release/acquire 配对：`smp_store_release()` + `smp_load_acquire()`
- assign/dereference 配对：`rcu_assign_pointer()` + `rcu_dereference()`
- write/read barrier 配对：`smp_wmb()` + `smp_rmb()`
- 实际代码示例：`init_stack_slab()`、`expand_to_next_prime()`、`xlog_state_switch_iclogs()`、`perf_output_put_handle()`

**Load Buffering (LB)**
- 控制依赖 + 全屏障配对
- 实际代码示例：ring buffer 的 producer/consumer 同步

**Release-Acquire Chains**
- 多 CPU 通过 release/acquire 传递顺序
- 限制：链外的 CPU 不一定能看到全局顺序

**Store Buffering (SB)**
- 需要两个全屏障才能阻止反直觉结果

**规则总结**
- 文档末尾提供了几条实用的规则

### 4.7 control-dependencies.txt — 控制依赖（258 行）

目标读者：需要使用控制依赖进行排序的开发者。

核心内容：

**控制依赖的基本形式**
```c
q = READ_ONCE(a);
if (q)
    WRITE_ONCE(b, 1);
```

**关键限制**
- 控制依赖只排序后续的 store，不排序后续的 load（需要 `smp_rmb()`）
- `READ_ONCE()` 和 `WRITE_ONCE()` 都不能省略
- 两个分支的 store 必须是不同值，否则编译器可能将 store 提出 `if`
- 不能用 `barrier()` 替代不同值的 store
- 不能用编译时已知的条件（如 `if (q || 1 > 0)`）
- 控制依赖不排序 `if` 语句之后的代码

**编译器优化破坏的例子**
- 相同值的 store 被提出 `if`
- 编译器证明条件总是真/假，消除 `if`
- 条件表达式被优化掉（如 `q % 1` 总是 0）
- 布尔短路求值被优化

**解决方案**
- 使用 `smp_store_release()` 替代控制依赖
- 确保条件在运行时才能确定
- 使用 `BUILD_BUG_ON()` 检查编译时常量

### 4.8 access-marking.txt — 访问标记指南（624 行）

目标读者：需要标记有意的并发访问，特别是应对 KCSAN 报告的开发者。

核心内容：

**访问标记选项**
1. Plain C-language accesses（`a = b`）
2. `data_race(a = b)` — 标记有意数据竞争
3. `READ_ONCE()` / `WRITE_ONCE()` — 阻止编译器优化
4. `__data_racy` — 变量属性
5. `ASSERT_EXCLUSIVE_ACCESS()` / `ASSERT_EXCLUSIVE_WRITER()` — KCSAN 断言

**何时使用 data_race()**
- 诊断目的的读（lockdep 报告、/proc 输出、统计）
- 读值后续会与标记重载检查对比
- 容错启发式的读/写

**何时使用 Plain C Accesses**
- 严格锁保护或顺序锁保护的访问
- 初始化/清理阶段的访问
- 不跨 CPU 访问的 per-CPU 变量
- 任务私有变量
- 确定无并发访问的变量

**KCSAN 的特殊处理**
- 默认假设 plain writes 是原子的（可配置）
- 使用 `data_race()` 或 `READ_ONCE()`/`WRITE_ONCE()` 禁用 KCSAN 诊断
- 但不要盲目添加标记，需要 thoughtful approach

### 4.9 cheatsheet.txt — 快速参考（35 行）

一个表格，总结了所有内存排序原语对不同类型访问的排序效果。

表格列：Prior Operation（Self, R, W, RMW）和 Subsequent Operation（Self, R, W, DR, DW, RMW, SV）

行：Relaxed store/load/RMW、rcu_dereference()、*_acquire()、*_release()、smp_rmb()、smp_wmb()、smp_mb()、full RMW、smp_mb__before_atomic()、smp_mb__after_atomic()

标记：C（累积排序）、P（传播）、R（读）、W（写）、Y（提供排序）、a（需要 intervening RMW）

### 4.10 glossary.txt — 术语表（178 行）

定义了 LKMM 相关术语：

- **Address Dependency**: 后续内存访问的地址基于前面 load 的值计算
- **Acquire**: 锁获取或特殊 load 操作，排序该 load 与后续访问
- **Coherence (co)**: 一个 CPU 的 store 覆盖另一个 CPU 的 store
- **Control Dependency**: 后续 store 的执行依赖于前面 load 的值的测试
- **Cycle**: 内存屏障配对扩展到多个 CPU
- **Data Dependency**: 后续 store 的数据基于前面 load 的值计算
- **From-Reads (fr)**: load 读取的值来自某个 store 之前的 store
- **Fully Ordered**: 如 `smp_mb()` 排序所有先前和后续访问
- **Happens-Before (hb)**: LKMM 保证第一个访问先于第二个的关系
- **Marked Access**: 使用特殊函数/宏的访问（如 `READ_ONCE()`）
- **Pairing**: 两个 CPU 的内存屏障配对
- **Reads-From (rf)**: load 返回了某个 store 写入的值
- **Relaxed**: 不隐含排序的标记访问
- **Release**: 锁释放或特殊 store 操作，排序该 store 与先前访问
- **Unmarked Access**: 使用普通 C 语法的访问

### 4.11 explanation.txt — LKMM 详细解释（2810 行）

目标读者：熟悉内核并发和 LKMM，想了解模型的设计原理和实现。

这是 Documentation 下最长的文档，共 26 章：

1. **INTRODUCTION**: LKMM 简介，目标读者
2. **BACKGROUND**: 什么是内存一致性模型，多处理器系统的复杂性
3. **A SIMPLE EXAMPLE**: 用设备驱动例子（中断处理程序和 read() 系统调用）引入 MP 模式
4. **A SELECTION OF MEMORY MODELS**: Sequential Consistency、TSO、PSO、Weak Ordering 等模型对比
5. **ORDERING AND CYCLES**: 排序和循环的概念
6. **EVENTS**: LKMM 中的事件类型
7. **THE PROGRAM ORDER RELATION**: po 和 po-loc（同一位置的 program order）
8. **A WARNING**: 关于编译器优化的警告
9. **DEPENDENCY RELATIONS**: data、addr、ctrl 依赖
10. **THE READS-FROM RELATION**: rf、rfi（internal）、rfe（external）
11. **CACHE COHERENCE AND THE COHERENCE ORDER RELATION**: co、coi、coe
12. **THE FROM-READS RELATION**: fr、fri、fre
13. **AN OPERATIONAL MODEL**: 操作模型视角
14. **PROPAGATION ORDER RELATION**: cumul-fence 和传播
15. **DERIVATION OF THE LKMM FROM THE OPERATIONAL MODEL**: 从操作模型推导 LKMM
16. **SEQUENTIAL CONSISTENCY PER VARIABLE**: 每个变量的顺序一致性
17. **ATOMIC UPDATES**: RMW 操作的原子性
18. **THE PRESERVED PROGRAM ORDER RELATION**: ppo（保留的 program order）
19. **AND THEN THERE WAS ALPHA**: Alpha 架构的特殊处理
20. **THE HAPPENS-BEFORE RELATION**: hb 的详细定义
21. **THE PROPAGATES-BEFORE RELATION**: pb 的详细定义
22. **RCU RELATIONS**: rcu-link、rcu-gp、rcu-rscsi、rcu-order、rcu-fence、rb
23. **SRCU READ-SIDE CRITICAL SECTIONS**: SRCU 的处理
24. **LOCKING**: 锁的语义
25. **PLAIN ACCESSES AND DATA RACES**: 普通访问和数据竞争
26. **ODDS AND ENDS**: 杂项

### 4.12 herd-representation.txt — Herd7 表示（113 行）

目标读者：想了解内核并发原语在 herd7 中的抽象表示。

解释了 `linux-kernel.def` 如何将 C 代码映射到 herd7 内部的事件表示。例如：
- `READ_ONCE(x)` -> `R[once] x`
- `smp_mb()` -> `F[mb]`
- `spin_lock(&lck)` -> 一系列 LKR、LKW 事件

### 4.13 references.txt — 参考资料（130 行）

列出了 LKMM 相关的论文、手册、标准委员会工作文件和 LWN 文章。

包括：
- 硬件手册（ARM、PowerPC、x86、RISC-V）
- 学术论文（Alglave、Maranget、McKenney 等人的工作）
- C/C++ 标准委员会文件
- LWN 文章系列

---

## 5. Litmus Test 详解

### 5.1 什么是 Litmus Test

Litmus Test 是一种小型并发程序，用于测试内存模型的某个特定属性。它通常包含：

- 2-4 个线程
- 少量共享变量
- 一个断言，描述 "不可能" 出现的结果

### 5.2 Litmus Test 格式

```
C MP+pooncerelease+poacquireonce

(*
 * Result: Never
 *
 * Test that release/acquire prevents reordering.
 *)

{
  int x = 0;
  int y = 0;
}

P0(int *x, int *y)
{
  WRITE_ONCE(*x, 1);
  smp_store_release(y, 1);
}

P1(int *x, int *y)
{
  int r0;
  int r1;

  r0 = smp_load_acquire(y);
  r1 = READ_ONCE(*x);
}

exists (1:r0=1 /\ 1:r1=0)
```

**格式说明**

| 部分 | 含义 |
|------|------|
| `C MP+...` | 测试名称 |
| `{ ... }` | 初始化代码 |
| `P0(...)` / `P1(...)` | 线程 0 / 线程 1 |
| `exists (...)` | 断言：描述 "坏结果" |

**Result 含义**

| 结果 | 含义 |
|------|------|
| `Never` | 断言的结果永远不会出现（模型保证） |
| `Sometimes` | 断言的结果可能出现（允许的行为） |
| `Always` | 断言的结果总是出现 |

### 5.3 断言语法

```
exists (1:r0=1 /\ 1:r1=0)   (* P1 读到 y=1 但 x=0 *)
forall (1:r0=0 \/ 1:r1=1)   (* 总是满足某个条件 *)
```

- `r0`, `r1` ... 线程局部寄存器
- `/\` 逻辑与
- `\/ ` 逻辑或
- `~` 逻辑非

---

## 6. 工具链使用指南

### 6.1 安装 herdtools7

**从源码编译安装**

```bash
# 依赖
git clone https://github.com/herd/herdtools7.git
cd herdtools7

# 需要 OCaml 环境
sudo dnf install opam
eval $(opam env)

# 编译安装
make all
make install
```

**使用 opam 安装（推荐）**

```bash
opam install herdtools7
```

**验证安装**

```bash
herd7 --version
klitmus7 --version
```

### 6.2 使用 herd7 运行 Litmus Test

**基本用法**

```bash
cd /path/to/kernel/tools/memory-model

# 运行单个测试
herd7 -conf linux-kernel.cfg litmus-tests/MP+pooncerelease+poacquireonce.litmus

# 运行多个测试
herd7 -conf linux-kernel.cfg litmus-tests/*.litmus
```

**输出解读**

```
Test MP+pooncerelease+poacquireonce Allowed
States 3
1:r0=0; 1:r1=0;
1:r0=0; 1:r1=1;
1:r0=1; 1:r1=1;
No
Witnesses
Positive: 0 Negative: 3
Condition exists (1:r0=1 /\ 1:r1=0)
Observation MP+pooncerelease+poacquireonce Never 0 3
Time MP+pooncerelease+poacquireonce 0.01
Hash=...
```

- `Never` — 断言的结果从未出现（符合预期）
- `States 3` — 发现了 3 种合法状态
- `Condition exists (...)` — 断言条件

### 6.3 使用 klitmus7 生成内核模块

`klitmus7` 将 litmus test 转换为可在真实硬件上运行的内核模块：

```bash
# 生成内核模块
klitmus7 -o /tmp/litmus-test litmus-tests/SB+poonceonces.litmus

# 编译并加载
cd /tmp/litmus-test
make
sudo insmod litmus.ko

# 查看结果
dmesg | tail
```

### 6.4 批量测试脚本

内核提供了丰富的测试脚本：

```bash
cd tools/memory-model

# 运行所有 litmus tests，检查与预期结果
./scripts/checkalllitmus.sh

# 检查与 GitHub litmus archive 的兼容性
./scripts/checkghlitmus.sh

# 初始化/更新 litmus test 历史记录
./scripts/initlitmushist.sh --timeout 10m --procs 10
./scripts/newlitmushist.sh --timeout 10m --procs 10

# 比较两次运行结果
./scripts/checklitmushist.sh --timeout 10m --procs 10
```

**测试 LKMM 修改的工作流程**

```bash
# 1. 记录修改前的预期结果（约1小时）
scripts/initlitmushist.sh --timeout 10m --procs 10

# 2. 应用修改
git am -s -3 /path/to/patch

# 3. 快速冒烟测试（秒级）
scripts/checkalllitmus.sh

# 4. 与历史记录对比（约1小时）
scripts/checklitmushist.sh --timeout 10m --procs 10

# 5. 与 GitHub archive 对比（分钟级）
scripts/checkghlitmus.sh --timeout 10m --procs 10
```

---

## 7. 经典 Litmus Test 模式

### 7.1 MP (Message Passing)

```
P0:                    P1:
  WRITE_ONCE(x, 1);      r0 = READ_ONCE(y);
  WRITE_ONCE(y, 1);      r1 = READ_ONCE(x);

exists (1:r0=1 /\ 1:r1=0)
```

**问题**：P1 看到 y=1 但 x=0（消息未传递）。

**无屏障**：`Sometimes`（允许）
**加 `smp_mb()`**：`Never`（阻止）
**用 release/acquire**：`Never`（阻止）

### 7.2 SB (Store Buffering)

```
P0:                    P1:
  WRITE_ONCE(x, 1);      WRITE_ONCE(y, 1);
  r0 = READ_ONCE(y);     r1 = READ_ONCE(x);

exists (0:r0=0 /\ 1:r1=0)
```

**问题**：两个 CPU 都先写后读，但都没看到对方的写。

**无屏障**：`Sometimes`（x86 TSO 允许 store buffer）
**加 `smp_mb()`**：`Never`（阻止）

### 7.3 LB (Load Buffering)

```
P0:                    P1:
  r0 = READ_ONCE(x);     r1 = READ_ONCE(y);
  WRITE_ONCE(y, 1);      WRITE_ONCE(x, 1);

exists (0:r0=1 /\ 1:r1=1)
```

**问题**：两个 CPU 都先读后写，形成循环依赖。

### 7.4 IRIW (Independent Reads of Independent Writes)

```
P0:                    P1:                    P2:                    P3:
  WRITE_ONCE(x, 1);      WRITE_ONCE(y, 1);      r0 = READ_ONCE(x);     r2 = READ_ONCE(x);
                                                  r1 = READ_ONCE(y);     r3 = READ_ONCE(y);

exists (0:r0=1 /\ 0:r1=0 /\ 1:r2=0 /\ 1:r3=1)
```

**问题**：P2 和 P3 对两个写的观察顺序不一致（非全局一致性）。

**x86 TSO**：`Never`（TSO 保证全局一致性）
**ARM/Power**：`Sometimes`（允许）

### 7.5 WRC (Write-to-Read Causality)

```
P0:                    P1:                    P2:
  WRITE_ONCE(x, 1);      r0 = READ_ONCE(x);      r1 = READ_ONCE(y);
                         WRITE_ONCE(y, 1);       r2 = READ_ONCE(x);

exists (1:r0=1 /\ 2:r1=1 /\ 2:r2=0)
```

**问题**：P2 通过 P1 间接知道 P0 的写，但没看到 x=1。


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
