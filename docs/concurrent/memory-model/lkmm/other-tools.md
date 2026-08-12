

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

**原理**: 将程序执行和内存模型约束编码为 SMT
公式，然后使用求解器检查是否存在违反规范的执行路径。

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

**应用场景**:
用于验证新提出的内存模型是否与现有模型兼容，或者找出两个模型之间的差异。

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

| 工具                 | 类型       | 目标       | 内核/用户态 | 形式化/动态 | 主要用途                            |
| -------------------- | ---------- | ---------- | ----------- | ----------- | ----------------------------------- |
| herd7                | 模拟器     | LKMM 验证  | 通用        | 形式化      | 验证 litmus test 在 LKMM 下是否允许 |
| klitmus7             | 代码生成器 | 硬件测试   | 内核        | 动态        | 在真实硬件上运行 litmus test        |
| litmus7              | 测试运行器 | 硬件测试   | 用户态      | 动态        | 在用户态运行 litmus test            |
| diy7                 | 生成器     | 测试生成   | 通用        | -           | 自动生成 litmus tests               |
| KCSAN                | 检测器     | 数据竞争   | 内核        | 动态        | 运行时检测内核数据竞争              |
| lockdep              | 检测器     | 锁顺序     | 内核        | 动态        | 检测死锁和锁顺序违规                |
| rcutorture           | 压力测试   | RCU        | 内核        | 动态        | 压力测试 RCU 实现                   |
| locktorture          | 压力测试   | 锁         | 内核        | 动态        | 压力测试锁原语                      |
| membarrier selftests | 单元测试   | membarrier | 用户态      | 动态        | 测试 membarrier 系统调用            |
| TSan                 | 检测器     | 数据竞争   | 用户态      | 动态        | 运行时检测用户态数据竞争            |
| CDSChecker           | 模型检测   | C11 MM     | 用户态      | 形式化      | 验证 C11 程序正确性                 |
| Nidhugg              | 模型检测   | 多种 MM    | 用户态      | 形式化      | 状态空间探索                        |
| Dartagnan            | SMT 验证   | 多种 MM    | 通用        | 形式化      | SMT-based 验证                      |
| GenMC                | 模型检测   | C11 MM     | 用户态      | 形式化      | C11 模型检测                        |
| MemAlloy             | 比较工具   | 多种 MM    | 通用        | 形式化      | 内存模型比较                        |
| rmem                 | 探索工具   | ARM MM     | 通用        | 形式化      | ARM 内存模型可视化                  |
| Nemos                | 验证工具   | 多种 MM    | 通用        | 形式化      | 内存模型验证和比较                  |

---

## 7. 如何选择工具

### 场景 1: 验证内核无锁代码的内存序

- **首选**: herd7（形式化验证 LKMM）
- **补充**: klitmus7（在真实硬件上验证）
- **辅助**: KCSAN（运行时检测数据竞争）

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


### herdtools7

- [herdtools7 GitHub](https://github.com/herd/herdtools7)
- [diy7 文档](https://diy.inria.fr/doc/index.html)
- [ASPLOS 2018 Paper](http://diy.inria.fr/linux/)

### 内核工具

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
