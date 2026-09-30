# 用 Clang 和 GCC 观察 C++ 的隐式行为与实现

> [!NOTE]
> 参考神奇海螺的意见，有待验证

沿着 C++
源码、AST、中间表示、汇编逐层观察，可以分别回答语言语义、对象布局和执行成本的问题。
学习 C++ 的隐式行为，最常用的入口是 Clang AST、GCC GIMPLE 和 C++ Insights。

| 想弄明白的问题                 | 优先看                         |
| ------------------------------ | ------------------------------ |
| `auto`、引用折叠、重载选择     | Clang AST                      |
| lambda 捕获、范围 `for` 的展开 | C++ Insights                   |
| 构造析构、RAII、异常清理       | GCC GIMPLE                     |
| 多重继承、虚继承、对象大小     | record layout 和 vtable layout |
| 虚调用、RTTI、异常运行库       | LLVM IR                        |
| 抽象是否有额外成本             | 优化报告和 `-O2` 汇编          |

下面的命令假设当前目录中有 `actress.cpp`，可以使用本仓库的
[vtable/actress.cpp](vtable/actress.cpp)。它不依赖头文件，适合观察继承和虚表；
研究 lambda、模板或局部 `static` 时，需要换成包含相应特性的最小示例。 命令中的
`actress.out` 是通过 `-c` 生成的目标文件，不是可执行程序。

Clang 通常使用 `clang++ -Xclang ...`，让驱动正常处理头文件路径等配置。 `-Xclang`
将紧随其后的一个参数传递给前端；直接调用 `clang -cc1` 则绕过驱动，
更适合不依赖头文件的例子或前端调试。

## 1. Clang AST：类型推导、重载选择和隐式转换

```bash
clang++ -std=c++20 -Xclang -ast-dump -fsyntax-only actress.cpp

# 只看限定名称包含 Derived 的声明，避免标准库输出淹没结果
clang++ -std=c++20 -Xclang -ast-dump \
    -Xclang -ast-dump-filter -Xclang Derived \
    -fsyntax-only actress.cpp

# JSON 输出，方便进一步处理
clang++ -std=c++20 -Xclang -ast-dump=json \
    -fsyntax-only actress.cpp > actress.ast.json
```

特别适合回答：

- `auto`、模板参数最终推导成什么类型？
- 表达式是 `lvalue`、`xvalue` 还是 `prvalue`？
- 调用选择了哪个重载、哪个构造函数？
- 哪里发生了隐式类型转换、临时对象实质化？

常见节点有 `ImplicitCastExpr`、`CXXConstructExpr`、`MaterializeTemporaryExpr`、
`LambdaExpr`。Clang AST 保留了很多源语言结构，并不是完全展开后的实现。
模板通常需要实际实例化，才能观察到具体实例的语义信息。

参考：[Clang AST 文档](https://clang.llvm.org/docs/IntroductionToTheClangAST.html)。

## 2. GCC GIMPLE：编译器补充了哪些操作

```bash
# 较早阶段，保留较多源语言结构
g++ -std=c++20 -O0 -fdump-tree-original=actress.original \
    -c actress.cpp -o actress.out

# 降低后的、类似 C 的中间表示
g++ -std=c++20 -O0 -fdump-tree-gimple=actress.gimple \
    -c actress.cpp -o actress.out

# 优化后的中间表示
g++ -std=c++20 -O2 -fdump-tree-optimized=actress.optimized \
    -c actress.cpp -o actress.out
```

适合研究
RAII、构造和析构、异常清理、返回值处理，以及循环如何转换成更简单的控制流。
例如局部对象的清理可能呈现为下面这样的中间表示：

```text
A::A (&a);
try
  {
    work (&a);
  }
finally
  {
    A::~A (&a);
  }
```

这是示意，不是可直接编译的 C++。`original` 也不是原始源码； GIMPLE
进一步把复杂表达式拆成简单操作。对照优化前后，可以观察拷贝、临时对象和调用是否被消除。

参考：[GIMPLE 文档](https://gcc.gnu.org/onlinedocs/gccint/GIMPLE.html)。

## 3. 对象布局与虚表

```bash
# 成员偏移、基类子对象、大小和对齐
clang++ -std=c++20 -Xclang -fdump-record-layouts \
    -c actress.cpp -o actress.out

# 虚表条目及 this 调整等信息
clang++ -std=c++20 -Xclang -fdump-vtable-layouts \
    -c actress.cpp -o actress.out

# GCC 的类布局、继承和虚表信息
g++ -std=c++20 -fdump-lang-class=actress.class \
    -c actress.cpp -o actress.out
```

适合看多重继承、虚继承、空基类优化、虚表，以及调整 `this` 的 thunk。 本仓库的
[C++ vtable 与 RTTI 布局](vtable/vtable.md) 配合 `actress.cpp` 解释了这些输出。

类或虚表没有输出时，先确保示例实际使用了它，并提供必要的虚函数定义。 Clang
的普通 record layout dump 按需产生；vtable dump 针对当前翻译单元实际生成的虚表。
布局属于目标 ABI 和编译器实现，不能把某个平台的偏移当作 C++ 标准保证。

前端选项可以通过本机帮助查找：

```bash
clang -cc1 --help | rg 'ast-dump|ast-print|fdump-record|fdump-vtable'
```

## 4. LLVM IR：虚调用、异常和运行库

```bash
clang++ -std=c++20 -O0 -S -emit-llvm actress.cpp -o actress.O0.ll
clang++ -std=c++20 -O2 -S -emit-llvm actress.cpp -o actress.O2.ll
```

这一层适合追踪：

- 虚函数调用如何读取虚表并间接调用。
- 构造函数如何初始化虚表指针。
- 局部 `static` 的动态初始化如何接入 guard 机制。
- 异常如何调用运行库、进入清理路径。
- 优化后哪些分配、拷贝和调用消失了。

具体符号与异常表示取决于目标 ABI。例如在常见 Linux C++ ABI 下，
包含对应操作的示例可能出现 `__cxa_guard_acquire`、`__cxa_throw`。 IR
中仍然存在抽象操作，最终指令需要继续查看汇编。

参考：[Clang 参数参考](https://clang.llvm.org/docs/ClangCommandLineReference.html)。

## 5. 优化报告与 pass dump

直接查看编译器为什么应用或放弃某种优化：

```bash
# GCC：内联和向量化的成功、失败及说明
g++ -std=c++20 -O2 -fopt-info-inline-all -fopt-info-vec-all \
    -c actress.cpp -o actress.out

# Clang：内联决策
clang++ -std=c++20 -O2 -Rpass=inline -Rpass-missed=inline \
    -Rpass-analysis=inline -c actress.cpp -o actress.out
```

如果想跟踪 GCC 某次变换，可以先查 pass，再选择对应的 dump：

```bash
# 查看当前编译配置下的 pass 列表
g++ -std=c++20 -O2 -fdump-passes -c actress.cpp -o actress.out

# 控制流图、SSA 和内联阶段输出
g++ -std=c++20 -O2 -fdump-tree-cfg -fdump-tree-ssa -fdump-ipa-inline \
    -c actress.cpp -o actress.out
```

适合分析为什么没内联、为什么循环不能向量化，以及某个分支在哪一步消失。 具体 pass
名称和默认 dump 文件编号可能随版本变化。 支持 `=文件名` 的 dump
选项可以显式指定输出路径，如前面的 `actress.gimple`， 避免依赖自动生成的文件名。

参考：[GCC 开发选项](https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html)、
[Clang 优化报告](https://clang.llvm.org/docs/UsersManual.html#options-to-emit-optimization-reports)。

## 6. 汇编：核实最终成本

```bash
# x86 上使用 Intel 语法；其他架构去掉 -masm=intel
g++ -std=c++20 -O2 -S -masm=intel -fverbose-asm \
    actress.cpp -o actress.s
```

用来确认引用传参、对象返回、虚调用、原子操作最终生成了什么指令。 建议对照 `-O0`
和 `-O2`；`-O0` 中额外的栈读写不能代表正常优化后的成本。 启用 LTO
时，编译阶段生成的汇编还不能代表链接后最终结果。

参考：[GCC 汇编注释选项](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html#index-fverbose-asm)。

## 7. C++ Insights：用更显式的 C++ 解释语法

[C++ Insights](https://cppinsights.io/) 基于 Clang， 把 lambda、范围
`for`、模板实例化等转换成更显式的 C++，通常比直接阅读 AST 更直观。

适合先观察闭包对象如何保存捕获、范围循环如何使用迭代器，以及模板实例化后的具体类型。
它是帮助理解的源码转换，具体 ABI 和最终执行成本仍要看布局、IR 和汇编。

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
