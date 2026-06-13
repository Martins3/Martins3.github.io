# 其他 Memory Model 测试工具

除了 herdtools7（herd7 + klitmus7 + diy7 + litmus7）之外，Linux 内核生态和学术界还有多种工具用于验证、测试和分析内存模型。这些工具从不同角度（形式化验证、动态检测、压力测试、硬件测试）来保障并发代码的正确性。

---

## 目录

1. [herdtools7 套件中的其他工具](#1-herdtools7-套件中的其他工具)
2. [内核动态检测工具](#2-内核动态检测工具)
3. [内核压力测试框架](#3-内核压力测试框架)
4. [用户态内存模型工具](#4-用户态内存模型工具)
5. [学术界的其他工具](#5-学术界的其他工具)
6. [工具对比总结](#6-工具对比总结)

---

## 1. herdtools7 套件中的其他工具

herdtools7 是一个完整的工具链，除了 herd7 之外，还包括以下工具：

### 1.1 klitmus7 — 内核模块生成器

**功能**: 将 litmus test 编译成可加载的内核模块（.ko），在真实硬件上运行测试。

**与 herd7 的区别**:
- herd7: 在模拟器中运行，基于 LKMM 的公理验证所有可能的执行路径
- klitmus7: 在真实 CPU 上运行，通过大量重复执行来观察实际硬件行为

**使用方法**:
```bash
# 生成内核模块源代码
klitmus7 -o klitmus_output/ test.litmus

# 指定运行参数
klitmus7 -o klitmus_output/ -n 100000 -a 2 test.litmus
# -n: 每个测试运行次数
# -a: 可用 CPU 数

# 编译并加载（在生成的目录中）
cd klitmus_output/
make
insmod test.ko
```

**兼容性**: klitmus7 生成的内核模块与内核版本和 herdtools7 版本都有绑定关系。详见 `tools/memory-model/README` 中的兼容性表格。

### 1.2 litmus7 — 用户态硬件测试

**功能**: 在用户态直接运行 litmus test，测试真实 CPU 的内存序行为，无需编译内核模块。

**与 klitmus7 的区别**:
- litmus7: 用户态程序，更容易运行，但受限于用户态内存访问
- klitmus7: 内核模块，可以测试内核特有的原语（如 RCU、spinlock）

**使用方法**:
```bash
# 直接运行 litmus test
litmus7 test.litmus

# 指定运行参数
litmus7 -n 1000000 -s 2 test.litmus
# -n: 运行次数
# -s: 可用 CPU 数
```

### 1.3 diy7 — Litmus Test 生成器

**功能**: 根据指定的模式自动生成 litmus tests。

**使用方法**:
```bash
# 生成 Message Passing 模式的测试
diy7 -arch Linux MP

# 生成 Store Buffering 模式的测试
diy7 -arch Linux SB

# 生成所有经典模式
diy7 -arch Linux -mode uni
```

diy7 可以生成大量测试用例，配合 herd7 批量验证，是发现内存模型边界情况的有力工具。

---

## 2. 内核动态检测工具

### 2.1 KCSAN (Kernel Concurrency Sanitizer)

**位置**: `kernel/kcsan/` + `Documentation/dev-tools/kcsan.rst`

**功能**: 动态数据竞争检测器。在运行时检测内核中的数据竞争，帮助开发者发现潜在的并发 bug。

**原理**: KCSAN 使用编译时插桩（instrumentation），在内存访问点插入检查代码。通过延迟机制（delay-based）来检测并发访问：
- 当一个线程访问某个内存位置时，KCSAN 会设置一个"观察点"（watchpoint）
- 如果在观察期间有其他线程访问同一位置，且至少有一个是写操作，则报告数据竞争

**配置选项**:
```
CONFIG_KCSAN=y              # 启用 KCSAN
CONFIG_KCSAN_EARLY_ENABLE=y # 早期启用
CONFIG_KCSAN_UDELAY_TASK=80 # 任务延迟（微秒）
CONFIG_KCSAN_SKIP_WATCH=4000 # 跳过观察次数
```

**使用方式**:
```bash
# 标记有意数据竞争以避免误报
data_race(READ_ONCE(x));  # 告诉 KCSAN 这是有意的

# 运行时参数
kcsan.early_enable=1      # 启动时启用
kcsan.skip_watch=1000     # 调整跳过次数
```

**与 LKMM 的关系**: KCSAN 是 LKMM 的互补工具。LKMM 是形式化模型，用于验证设计正确性；KCSAN 是动态检测器，用于在运行时捕获实际发生的数据竞争。

**局限性**:
- 依赖实际执行路径，可能遗漏未触发的情况
- 有性能开销（通常 5-20 倍 slowdown）
- 可能产生误报（需要 `data_race()` 标记来抑制）

### 2.2 lockdep — 锁依赖验证器

**位置**: `kernel/locking/lockdep.c`

**功能**: 检测死锁、锁顺序违规、irq-unsafe -> irq-safe 锁嵌套等问题。

**原理**: lockdep 维护一个锁的依赖图（lock dependency graph）。当代码获取锁 A 后再获取锁 B，lockdep 记录 A -> B 的依赖关系。如果发现循环依赖（A -> B -> C -> A），则报告潜在死锁。

**检测的问题类型**:
- 死锁（deadlock）
- 锁顺序反转（lock inversion）
- 不安全的中断上下文锁嵌套
- 递归锁获取（对于非递归锁）

**使用方式**:
```bash
# 启用 lockdep
CONFIG_LOCKDEP=y
CONFIG_DEBUG_LOCK_ALLOC=y

# 查看 lockdep 报告
cat /proc/lockdep
```

**与 LKMM 的关系**: lockdep 处理的是锁的正确使用，而 LKMM 处理的是无锁代码的内存排序。两者互补：
- lockdep: "你是否正确地使用了锁？"
- LKMM: "如果你不用锁，你的无锁代码是否正确？"

---

## 3. 内核压力测试框架

### 3.1 rcutorture — RCU 压力测试

**位置**: `kernel/rcu/rcutorture.c` + `tools/testing/selftests/rcutorture/`

**功能**: 对 RCU 子系统进行全面的压力测试，包括：
- grace period 处理
- 回调执行
- CPU 热插拔
- 回调洪水（callback flooding）
- SRCU、Tasks RCU 等变体

**使用方法**:
```bash
# 加载 rcutorture 模块
modprobe rcutorture

# 查看测试状态
cat /sys/kernel/debug/rcutorture/rcu/rcudata

# 使用 kvm.sh 脚本进行自动化测试
cd tools/testing/selftests/rcutorture/
./bin/kvm.sh --cpus 8 --duration 30

# 检查结果
./bin/kvm-find-errors.sh res/2024.01.01-00.00.00/
```

**与 LKMM 的关系**: rcutorture 验证 RCU 的实现是否正确，而 LKMM 中的 `rcu` 公理形式化地定义了 RCU 的语义。两者结合：LKMM 证明设计正确，rcutorture 验证实现正确。

### 3.2 locktorture — 锁原语压力测试

**位置**: `kernel/locking/locktorture.c`

**功能**: 对内核各种锁原语进行压力测试：
- spinlock_t
- rwlock_t
- mutex
- rwsem
- percpu_lock

**使用方法**:
```bash
# 加载 locktorture 模块
modprobe locktorture

# 查看统计信息
cat /proc/locktorture/stat
```

### 3.3 membarrier selftests

**位置**: `tools/testing/selftests/membarrier/`

**功能**: 测试 `membarrier` 系统调用的正确性。membarrier 是用户空间的内存屏障机制，与内核的 `smp_mb()` 等原语相关。

**测试内容**:
- `MEMBARRIER_CMD_GLOBAL`: 全局内存屏障
- `MEMBARRIER_CMD_PRIVATE_EXPEDITED`: 私有加速屏障
- `MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED`: 注册私有加速屏障
- `MEMBARRIER_CMD_SYNC_CORE`: 核心同步

**使用方法**:
```bash
cd tools/testing/selftests/membarrier/
make
./membarrier_test_single_thread
./membarrier_test_multi_thread
```

---

## 4. 用户态内存模型工具

### 4.1 ThreadSanitizer (TSan)

**功能**: 用户态动态数据竞争检测器，集成在 Clang/GCC 中。

**使用方法**:
```bash
# 编译时启用 TSan
clang -fsanitize=thread -g -O1 program.c -o program.tsan

# 运行
./program.tsan
```

**与 KCSAN 的区别**:
- TSan: 用户态程序
- KCSAN: 内核态

### 4.2 CDSChecker

**功能**: 针对 C11/C++11 内存模型的模型检测工具。

**特点**:
- 专门验证 C11 atomic 操作的正确性
- 支持 `memory_order_relaxed`, `memory_order_acquire`, `memory_order_release` 等
- 可以检测 C11 程序中的数据竞争

**局限性**: 主要针对 C11/C++11 标准，不直接支持内核特有的原语。

### 4.3 Nidhugg

**功能**: 针对 C/C++ 和 LLVM IR 的状态空间探索工具。

**特点**:
- 支持多种内存模型（SC, TSO, PSO, POWER, ARM）
- 使用 LLVM IR 作为输入，可以分析编译后的代码
- 支持部分顺序缩减（Partial Order Reduction）来减少状态空间

**使用方法**:
```bash
# 编译为 LLVM IR
clang -S -emit-llvm program.c -o program.ll

# 使用 Nidhugg 分析
nidhugg -sc program.ll    # Sequential Consistency
nidhugg -tso program.ll   # Total Store Order
```

---

## 5. 学术界的其他工具

### 5.1 Dartagnan

**功能**: 使用 SMT solver（如 Z3）验证内存模型的工具。

**特点**:
- 基于 SMT 求解器，可以处理复杂的内存模型
- 支持多种架构（x86, ARM, POWER, RISC-V）
- 可以验证程序在特定内存模型下的正确性

**原理**: 将程序执行和内存模型约束编码为 SMT 公式，然后使用求解器检查是否存在违反规范的执行路径。

### 5.2 GenMC

**功能**: 针对 C/C++11 内存模型的模型检测工具。

**特点**:
- 支持 C11/C++11 内存模型
- 使用基于事件的模型检测方法
- 可以处理无锁数据结构和算法

**与 herd7 的区别**:
- GenMC: 专门针对 C11/C++11，支持更复杂的程序结构
- herd7: 更通用，支持多种内存模型（包括 LKMM）

### 5.3 MemAlloy

**功能**: 基于 Alloy 的内存模型比较工具。

**特点**:
- 使用 Alloy 建模语言描述内存模型
- 可以比较不同内存模型的强弱关系
- 自动生成反例（counterexamples）

**应用场景**: 用于验证新提出的内存模型是否与现有模型兼容，或者找出两个模型之间的差异。

### 5.4 rmem

**功能**: ARM 架构的内存模型探索工具。

**特点**:
- 专门针对 ARMv8 内存模型
- 支持可交互的内存模型探索
- 可以可视化内存操作的执行流程

**应用场景**: 主要用于 ARM 架构的内存模型研究和教学。

### 5.5 Nemos

**功能**: 用于验证和比较内存模型的工具。

**特点**:
- 支持多种内存模型的形式化验证
- 可以自动推导内存模型之间的关系
- 支持 litmus test 的自动生成和验证

---

## 6. 工具对比总结

| 工具 | 类型 | 目标 | 内核/用户态 | 形式化/动态 | 主要用途 |
|------|------|------|-------------|-------------|----------|
| herd7 | 模拟器 | LKMM 验证 | 通用 | 形式化 | 验证 litmus test 在 LKMM 下是否允许 |
| klitmus7 | 代码生成器 | 硬件测试 | 内核 | 动态 | 在真实硬件上运行 litmus test |
| litmus7 | 测试运行器 | 硬件测试 | 用户态 | 动态 | 在用户态运行 litmus test |
| diy7 | 生成器 | 测试生成 | 通用 | - | 自动生成 litmus tests |
| KCSAN | 检测器 | 数据竞争 | 内核 | 动态 | 运行时检测内核数据竞争 |
| lockdep | 检测器 | 锁顺序 | 内核 | 动态 | 检测死锁和锁顺序违规 |
| rcutorture | 压力测试 | RCU | 内核 | 动态 | 压力测试 RCU 实现 |
| locktorture | 压力测试 | 锁 | 内核 | 动态 | 压力测试锁原语 |
| membarrier selftests | 单元测试 | membarrier | 用户态 | 动态 | 测试 membarrier 系统调用 |
| TSan | 检测器 | 数据竞争 | 用户态 | 动态 | 运行时检测用户态数据竞争 |
| CDSChecker | 模型检测 | C11 MM | 用户态 | 形式化 | 验证 C11 程序正确性 |
| Nidhugg | 模型检测 | 多种 MM | 用户态 | 形式化 | 状态空间探索 |
| Dartagnan | SMT 验证 | 多种 MM | 通用 | 形式化 | SMT-based 验证 |
| GenMC | 模型检测 | C11 MM | 用户态 | 形式化 | C11 模型检测 |
| MemAlloy | 比较工具 | 多种 MM | 通用 | 形式化 | 内存模型比较 |
| rmem | 探索工具 | ARM MM | 通用 | 形式化 | ARM 内存模型可视化 |
| Nemos | 验证工具 | 多种 MM | 通用 | 形式化 | 内存模型验证和比较 |

---

## 7. 如何选择工具

### 场景 1: 验证内核无锁代码的内存序
- **首选**: herd7（形式化验证 LKMM）
- **补充**: klitmus7（在真实硬件上验证）
- **辅助**: KCSAN（运行时检测数据竞争）

### 场景 2: 调试内核并发 bug
- **数据竞争**: KCSAN
- **死锁/锁顺序**: lockdep
- **RCU 问题**: rcutorture

### 场景 3: 验证用户态并发代码
- **动态检测**: TSan
- **形式化验证**: CDSChecker, Nidhugg, GenMC

### 场景 4: 研究新的内存模型
- **模型比较**: MemAlloy
- **SMT 验证**: Dartagnan
- **ARM 研究**: rmem

### 场景 5: 批量生成和验证测试
- **生成测试**: diy7
- **批量验证**: herd7 + shell 脚本
- **硬件验证**: litmus7 / klitmus7

---

## 8. 参考资源

### herdtools7
- [herdtools7 GitHub](https://github.com/herd/herdtools7)
- [diy7 文档](https://diy.inria.fr/doc/index.html)
- [ASPLOS 2018 Paper](http://diy.inria.fr/linux/)

### 内核工具
- [KCSAN 文档](https://www.kernel.org/doc/html/latest/dev-tools/kcsan.html)
- [lockdep 文档](https://www.kernel.org/doc/html/latest/locking/lockdep-design.html)
- [rcutorture 文档](https://www.kernel.org/doc/html/latest/RCU/torture.html)

### 学术工具
- [Dartagnan](https://github.com/herdspy/dartagnan)
- [GenMC](https://github.com/MPI-SWS/genmc)
- [Nidhugg](https://github.com/nidhugg/nidhugg)
- [CDSChecker](https://github.com/parasol-aser/c11tester)
- [MemAlloy](https://github.com/anishathalye/memalloy)
- [rmem](https://github.com/rems-project/rmem)

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
