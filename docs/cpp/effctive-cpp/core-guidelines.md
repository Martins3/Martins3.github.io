# C++ Core Guidelines 中文整理

> 原文：[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)。
> 编辑：Bjarne Stroustrup、Herb Sutter。整理日期：2026-09-05；原文页首日期：2026-06-14。
> 本文按原文章节提炼重点，不是逐条全文翻译。所有编号均保留原文含义，点击编号可查看理由、示例、例外和检查建议。
> 全部带编号的规则标题另见 [规则索引](core-guidelines-index.md)，来源和授权见文末。

## 阅读方式与总览

Core Guidelines 关注接口、资源、生命周期、类型安全和并发，适合作为设计与代码审查的参考。
它不是 ISO C++ 标准，也不是语言教程；合法的 C++ 写法也可能违反这些建议。
原文以 C++20 为当前基线，多数建议也适用于 C++11/14/17，但部分内容仍引用 GSL、提案或未完成的设计。

| 章节                                      | 主要问题                                   |
| ----------------------------------------- | ------------------------------------------ |
| [P：设计理念](#p设计理念)                 | 怎样让意图、约束和资源关系直接体现在代码中 |
| [I：接口](#i接口)                         | 调用者需要知道什么，类型能约束什么         |
| [F：函数](#f函数)                         | 参数、返回值和捕获方式如何表达语义         |
| [C：类与继承](#c类与继承)                 | 不变量、特殊成员函数与多态生命周期         |
| [Enum：枚举](#enum枚举)                   | 用类型表达有限取值集合                     |
| [R：资源管理](#r资源管理)                 | 谁拥有资源，谁负责释放                     |
| [ES：表达式与语句](#es表达式与语句)       | 初始化、求值顺序、转换和整数运算           |
| [Per：性能](#per性能)                     | 如何根据测量优化数据与执行路径             |
| [CP：并发与并行](#cp并发与并行)           | 共享状态、锁、线程与协程的生命周期         |
| [E：错误处理](#e错误处理)                 | 失败后如何维持不变量并释放资源             |
| [Con：常量与不可变性](#con常量与不可变性) | 哪些状态允许改变                           |
| [T：模板与泛型](#t模板与泛型)             | 如何表达类型要求并控制模板复杂度           |
| [CPL / SF / SL](#cplc-风格编程)           | C 接口、文件组织和标准库                   |
| [支撑章节](#支撑章节)                     | 架构、误区、安全配置、GSL、工具和迁移      |

建议先读 I、F、R、C、E，再按工作需要阅读 CP、T、Per。
与本目录的 [Effective C++](effective.md) 和 [Effective Modern C++](effective-modern-cpp.md) 对照阅读时，应保留各自的标准版本与适用条件。

## P：设计理念

- [P.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-direct)、[P.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-what)：直接表达意图。使用有意义的类型、函数和库抽象，减少靠注释解释的隐含约定。
- [P.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-cplusplus)：优先使用 ISO 标准 C++；平台扩展集中封装，避免扩散到业务代码。
- [P.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-typesafe)、[P.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-compile-time)：优先通过类型系统和编译期检查排除错误。
- [P.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-run-time)、[P.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-early)：编译期无法验证的条件，应能够在运行时检查，并尽早报告失败。
- [P.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-leak)、[P.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-waste)：不泄漏资源，不浪费时间和空间；资源不限于堆内存，也包括锁、文件和线程。
- [P.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-mutable)、[P.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-library)：优先不可变数据，把必要的复杂操作封装在明确的接口后。
- [P.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-tools)、[P.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-lib)：结合工具与支持库落实规则，不能只依靠人工记忆。

## I：接口

- [I.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-explicit)、[I.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-global)、[I.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-singleton)：显式声明依赖，避免可变全局变量和单例带来的隐式状态。
- [I.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-typed)、[I.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-unrelated)：用强类型区分不同含义的值，避免相邻的同类型参数被交换后仍能编译。例如用不同类型表达长度和超时时间。
- [I.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-pre)—[I.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-ensures)：写明前置条件和后置条件；原文推荐 `Expects` / `Ensures`，它们属于支持库约定，不是 C++20 关键字。
- [I.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-concepts)：模板接口用 concepts 表达要求，同时说明编译器无法验证的语义约束。
- [I.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-except)：无法完成约定任务时用异常报告失败；受限环境的替代策略见 E 章节。
- [I.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-raw)、[I.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-nullptr)、[I.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-array)：不靠裸指针隐式转移所有权；表达非空要求；数组接口同时携带范围信息。
- [I.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-global-init)、[I.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-nargs)：避免复杂的全局初始化，控制参数数量。
- [I.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-abstract)—[I.27](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-pimpl)：多态接口可用纯抽象类；跨编译器 ABI 考虑 C 风格子集，稳定库 ABI 可考虑 Pimpl。
- [I.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-encapsulate)：底层不得不违反某条规则时，将其封装，并对外提供满足约束的接口。

## F：函数

### 职责与声明

- [F.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-package)—[F.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-single)：函数命名表达操作，一个函数完成一个逻辑任务，保持短小清晰。
- [F.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-constexpr)：可能需要编译期求值的函数声明为 `constexpr`；这不意味着每次调用都会在编译期执行。
- [F.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-inline)：很小且性能关键的函数可考虑 `inline`；它不保证编译器执行内联优化。
- [F.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-noexcept)、[F.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-pure)：不能抛异常的函数表达为 `noexcept`；优先输入决定输出的纯函数。
- [F.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-smart)：只使用对象时接受引用或裸指针；需要表达所有权行为时才接受智能指针。

### 参数与返回值速查

| 语义 | 常见形式 | 对应规则及注意事项 |
| --- | --- | --- |
| 只读、小且复制便宜 | `T` | [F.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-in)，不要机械地把小整数改为 `const T&` |
| 只读、复制成本较高 | `const T&` | [F.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-in)，需要保存副本时另外考虑复制/移动成本 |
| 修改调用者对象 | `T&` | [F.17](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-inout)，明确输入输出语义 |
| 消耗实参的值 | `X&&` | [F.18](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-consume)，在函数内对具名参数使用 `std::move` |
| 泛型转发 | 推导得到的 `T&&` | [F.19](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-forward)，使用 `std::forward<T>` 保留值类别 |
| 输出一个结果 | 返回值 | [F.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-out)，优先于输出参数 |
| 输出多个相关结果 | 返回具名 `struct` | [F.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-out-multi)，字段名说明含义 |
| 可为空的借用 | `T*` | [F.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-ptr-ref)，调用前判断是否存在 |
| 必须非空的指针 | `not_null<T*>` | [F.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-nullptr)，仍需保证对象存活 |
| 连续序列的借用 | `span<T>` | [F.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-range)，携带长度，不拥有元素 |
| 转移独占所有权 | `unique_ptr<T>` | [F.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-unique_ptr)，按值接收便宜且语义清楚 |
| 共享所有权 | `shared_ptr<T>` | [F.27](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-shared_ptr)，仅在确有共享生命周期需求时使用 |

### 返回与 Lambda 生命周期

- [F.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-dangle)：不能返回指向局部对象的指针或引用；包含这种引用的视图也会悬空。
- [F.44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-ref)、[F.45](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-ref-ref)：借用既有对象可返回 `T&`，但要保证对象存活；通常不要返回 `T&&`。
- [F.47](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-assignment-op)：赋值运算符返回 `T&`，保持常规赋值语义。
- [F.48](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-move-local)：返回同类型局部结果时写 `return result;`，不要加 `std::move` 阻碍 NRVO。
- [F.49](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-const)：按值返回时通常不要加顶层 `const`，否则可能阻碍后续移动。
- [F.52](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-reference-capture)：立即、局部执行的 Lambda 可优先引用捕获。
- [F.53](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-value-capture)：会返回、存储或跨线程执行的 Lambda 避免引用捕获局部变量。
- [F.54](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-this-capture)：访问成员时不要用 `[=]` 隐藏对 `this` 的捕获；复制指针不会延长对象生命周期。
- [F.55](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f-varargs)：用类型安全的可变参数模板等机制替代 C 风格变参。

## C：类与继承

### 类型与不变量

- [C.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-org)、[C.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-struct)：相关数据组织为类型；需要维护不变量时使用 `class`，成员可独立变化的数据集合使用 `struct`。
- [C.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-member)、[C.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-private)：仅需访问内部表示的操作才必须成为成员，尽量缩小表示的暴露范围。
- [C.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-concrete)、[C.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-regular)：优先具体值类型；让复制、比较等行为符合正常值的直觉。
- [C.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-constref)：可复制或可移动的类型慎用 `const` / 引用数据成员，它们会限制赋值等操作。
- [C.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-lifetime)：成员 B 依赖 A 的生命周期时，先声明 A，再声明 B。

### 特殊成员函数

- [C.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-zero)：优先 Rule of Zero。用标准容器、字符串和资源句柄管理成员，让编译器生成特殊成员函数。
- [C.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-five)、[C.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-matched)：一旦自定义或删除析构、复制、移动中的某项，应明确处理整组操作，使其语义一致；需要默认行为时用 `= default`。
- [C.31](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-release)—[C.33](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-ptr2)：类拥有的资源应由析构释放；裸指针成员是否拥有资源必须明确。
- [C.35](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-virtual)：基类析构采用“public 且 virtual”或“protected 且 non-virtual”，分别表达允许或禁止经基类接口销毁。
- [C.36](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-fail)、[C.37](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-noexcept)：析构不能失败，不让异常逃出析构函数。
- [C.40](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-ctor)—[C.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-throw)：构造完成即建立不变量；无法构造有效对象时抛异常，避免要求调用者再调用 `init()` 才能使用。
- [C.46](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-explicit)：单参数构造函数默认加 `explicit`，除非有意支持隐式转换。
- [C.47](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-order)—[C.49](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-initialize)：初始化列表按成员声明顺序书写；优先类内默认初始化和直接初始化，避免先默认构造再赋值。
- [C.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-copy-assignment)—[C.65](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-move-self)：复制与移动符合各自语义，处理自赋值；移动后源对象应保持有效状态，但不承诺其值不变。
- [C.66](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-move-noexcept)：把移动操作设计为不抛异常；不能为了触发容器移动而给可能抛异常的实现虚标 `noexcept`。
- [C.80](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-eqdefault)、[C.81](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-delete)：明确使用默认行为或禁用行为，分别使用 `= default`、`= delete`。
- [C.83](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-swap)—[C.85](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-swap-noexcept)：值类型可提供不失败的 `swap`，并声明 `noexcept`。
- [C.90](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-memset)：对象状态由构造和赋值维护，不能用 `memset` / `memcpy` 替代一般类对象的这些操作。

### 多态、容器与运算符

- [C.67](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-copy-virtual)：多态类避免公开复制/移动造成切片；需要多态复制时提供表达该语义的接口。
- [C.82](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-ctor-virtual)：避免在构造和析构中调用虚函数；此时不能期待分派到尚未构造或已经析构的派生部分。
- [C.100](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcon-stl)—[C.104](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcon-empty)：自定义容器参考 STL 的值语义、移动行为与空状态设计。
- [C.120](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-domain)、[C.121](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-abstract)：只有领域本身存在层次关系时才使用继承；接口基类优先纯抽象。
- [C.128](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-override)：虚函数声明按角色使用 `virtual`、`override` 或 `final`，不堆叠冗余说明符。
- [C.160](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-conventional)、[C.161](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-symmetric)：运算符遵守惯常语义；对称运算符优先非成员函数。
- [C.180](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ru-union)：用 `union` 节省空间时必须管理活动成员；通常优先使用带类型信息的替代物，如 `std::variant`。

## Enum：枚举

- [Enum.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-macro)—[Enum.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-class)：相关命名常量用枚举表达，优先 `enum class`，减少名字污染和意外整数转换。
- [Enum.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-oper)：需要枚举运算时定义类型安全的操作，而非在调用处散布强制转换。
- [Enum.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-caps)、[Enum.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-unnamed)：枚举项不使用为宏保留的全大写风格，避免无名枚举。
- [Enum.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-underlying)、[Enum.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-value)：仅在 ABI、存储布局、协议等有要求时显式指定底层类型与枚举值。

## R：资源管理

- [R.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-raii)、[R.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-scoped)：用 RAII 让资源随句柄生命周期自动释放；优先作用域内对象，避免不必要的堆分配。
- [R.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-use-ptr)—[R.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-ref)：普通裸指针、引用在接口约定中表示借用；这是设计约定，语言本身不会阻止对裸指针执行 `delete`。
- [R.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-mallocfree)—[R.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-immediate-alloc)：避免直接调用分配/释放函数；必须调用底层 API 时，成功取得资源后立即交给管理对象。
- [R.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-single-alloc)、[R.15](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-pair)：避免在一个表达式中混杂多次显式资源分配，分配和释放机制必须匹配。
- [R.14](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-ap)：数组形参不携带长度，优先 `span`。
- [R.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-owner)、[R.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-unique)：所有权需要指针表示时，默认 `unique_ptr`，确实共享时才使用 `shared_ptr`。
- [R.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-make_shared)、[R.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-make_unique)：优先 `make_shared` / `make_unique`；特殊释放方式使用匹配的删除器。
- [R.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-weak_ptr)：共享所有权形成环时，可把观察关系改为 `weak_ptr`。
- [R.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-smartptrparam)、[R.32](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-uniqueptrparam)—[R.35](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-sharedptrparam)：智能指针参数分别表达接管所有权、重新绑定或共享所有权；仅访问对象时传对象引用即可。
- [R.37](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-smartptrget)：从可能被下层调用重置的智能指针取得借用时，先保证拥有者在整个调用链中存活；对 `shared_ptr` 可用本地副本保活。

借用的两个检查点：对象是否仍然存在，以及容器修改是否使地址/迭代器失效。
`span`、`string_view`、裸指针和引用都不会自动延长被引用对象的生命周期。

## ES：表达式与语句

- [ES.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-lib)—[ES.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-dry)：优先库与合适抽象，消除重复代码。
- [ES.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-scope)、[ES.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-name-one)、[ES.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-auto)：缩小作用域，一条声明一个名字，使用 `auto` 减少重复类型信息。
- [ES.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-always)—[ES.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-list)：使用前初始化，尽量等到有初值时再声明；优先 `{}`，但保留容器“大小构造”与“元素列表”的区别。
- [ES.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-const)、[ES.28](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-lambda-init)：不再修改的值使用 `const`；复杂初始化可用立即调用的 Lambda 返回最终值。
- [ES.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-macros)—[ES.33](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-macros3)：用函数、模板、枚举、常量替代宏；必须用宏时使用独特的全大写名字。
- [ES.40](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-complicated)—[ES.44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-order-fct)：表达式保持简单，不依赖函数参数的求值顺序；C++17 也未规定普通函数实参按从左到右求值。
- [ES.45](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-magic)—[ES.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-casts-const)：命名重要常量，避免窄化和强转，空指针使用 `nullptr`，不要随意去掉 `const`。
- [ES.56](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-move)：只在确实要转移值时使用 `std::move`；它本身是转换，不执行资源搬运。
- [ES.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-new)—[ES.65](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-deref)：限制手动资源操作，匹配 `delete` / `delete[]`，避免切片和无效指针解引用。
- [ES.71](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-for-range)—[ES.74](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-for-init)：遍历范围优先 range-for；显式循环根据控制变量选择 `for` 或 `while`。
- [ES.78](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-break)：不依赖隐式 `switch` 穿透，需要时明确表达意图。
- [ES.100](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-mix)—[ES.107](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-subscripts)：避免混合有符号/无符号运算，检查溢出与除零；无符号类型适合位操作，不能替代“非负数”的输入校验。

特别注意 [ES.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-list) 的例外：`std::vector<int>(8)` 是八个零，`std::vector<int>{8}` 是一个值为八的元素。
不要把所有圆括号初始化机械替换为花括号。

## Per：性能

- [Per.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-reason)—[Per.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-measure)：先确认性能目标和瓶颈，再测量优化；复杂、底层的代码不天然更快。
- [Per.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-efficiency)、[Per.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-type)、[Per.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-comp)：从设计上允许优化，使用静态类型信息，将合适的计算前移到编译期。
- [Per.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-alias)—[Per.15](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-alloc0)：减少冗余别名、间接访问与分配，避免关键路径上分配。
- [Per.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-compact)—[Per.19](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-access)：紧凑的数据和可预测的访存有利于性能；根据实际访问模式考虑布局与缓存。
- [Per.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-context)：关键路径减少上下文切换。

实践时同时记录输入规模、构建选项和运行环境。只优化已经测得的热点，同时验证结果仍然正确。

## CP：并发与并行

### 共享状态与线程

- [CP.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-multi)—[CP.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-task)：考虑代码被多线程使用的可能，避免数据竞争，减少共享可写数据，以任务组织并发。
- [CP.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-volatile)、[CP.200](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-volatile2)：`volatile` 不是线程同步机制；共享状态使用适合的原子操作或锁。
- [CP.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-raii)、[CP.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-lock)：锁用 RAII 管理；需要同时取得多把锁时用 `std::lock` / `std::scoped_lock`。
- [CP.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-unknown)：持锁期间不调用未知回调，避免重入、锁顺序反转和不可控阻塞。
- [CP.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-join)—[CP.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-detached_thread)：明确线程结束与数据生命周期的关系，优先自动 join 的线程句柄，避免 `detach()`。
- [CP.31](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-data-by-value)、[CP.32](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-shared)：少量数据优先按值跨线程传递；共享所有权用 `shared_ptr`，但其计数安全不代表所指对象访问安全。
- [CP.40](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-switch)、[CP.41](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-create)、[CP.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-time)：减少线程创建、上下文切换和临界区耗时。
- [CP.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-wait)：条件变量等待必须检查状态谓词；通知不是可积累的状态，还要处理虚假唤醒。
- [CP.44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-name)、[CP.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-mutex)：锁对象必须有名字，互斥量与受保护的数据放在一起。
- [CP.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-future)、[CP.61](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-async)：用 future 传回任务结果；使用 `async` 时理解执行策略，不把默认策略等同于必定创建异步线程。
- [CP.100](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-lockfree)、[CP.110](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-double)：无充分理由不写无锁算法，不手写双重检查初始化；优先成熟的同步与一次性初始化机制。

### 协程

- [CP.51](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcoro-capture)：避免带捕获的协程 Lambda；协程挂起后，闭包可能已销毁，捕获的 `shared_ptr` 也无法补救闭包本身的失效。
- [CP.52](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcoro-locks)：不持有锁跨越挂起点，防止阻塞其他任务或在错误线程解锁。
- [CP.53](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcoro-reference-parameters)：协程参数优先按值放入协程帧；引用形参不会让调用者对象自动存活到恢复执行时。

按值传入视图或裸指针仍然只是复制借用。跨挂起点还需检查其底层对象，以及成员协程的 `this` 所指对象是否存活。

## E：错误处理

- [E.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-design)、[E.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-design-invariants)：设计之初明确失败如何传播，以及失败后哪些不变量必须成立。
- [E.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-throw)、[E.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-errors)、[E.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-invariant)：无法完成约定任务或建立对象不变量时抛异常；不把异常用于正常分支控制。
- [E.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-raii)、[E.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-never-throw)：可能抛异常之前，资源应已由 RAII 句柄托管；不把手动释放留在可能跳过的后续语句中。
- [E.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-precondition)、[E.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-postcondition)：声明前置与后置条件，让失败边界清晰。
- [E.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-noexcept)：不允许异常逃出的接口使用 `noexcept`；异常逃出该边界会终止程序，不是被自动忽略。
- [E.14](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-exception-types)、[E.15](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-exception-ref)：使用有意义的异常类型，按值抛出，异常层次按引用捕获，通常是 `const&`。
- [E.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-never-fail)：析构、释放、`swap` 以及异常类型的复制/移动构造不能失败。
- [E.17](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-not-always)、[E.18](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-catch)：不在每层函数都捕获异常；在能够恢复、转换或报告失败的边界处理。
- [E.19](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-finally)：没有合适资源句柄时用作用域清理对象，避免重复手写清理路径。
- [E.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-no-throw-raii)—[E.28](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-no-throw)：不能使用异常时，仍系统管理资源，统一检查错误码，必要时快速失败；避免依赖全局错误状态。
- [E.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-specifications)：不用旧式动态异常规格 `throw(Type...)`；这条不禁止 `noexcept`。
- [E.31](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re_catch)：捕获顺序从派生异常到基类异常，避免前面的处理器遮蔽更具体的情况。

审查异常路径时，除了“会不会泄漏”，还应检查“是否留下半更新的业务状态”。
RAII 负责释放资源，不会自动回滚所有业务修改。

## Con：常量与不可变性

- [Con.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-immutable)：对象默认不可变，需要修改时再开放修改能力。
- [Con.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-fct)：不改变可观察状态的成员函数声明为 `const`。
- [Con.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-ref)：借用参数默认指向 `const`，除非函数确实需要修改对象。
- [Con.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-const)：构造后不变的对象使用 `const`。
- [Con.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-constexpr)：需要编译期常量时使用 `constexpr`。

`const` 不等于深度不可变，也不自动保证线程安全；仍需分析间接引用和共享的可变状态。

## T：模板与泛型

- [T.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-raise)—[T.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-generic-oo)：模板用于提升抽象、复用算法和容器，必要时与面向对象技术组合。
- [T.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-concepts)—[T.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-shorthand)：用 concepts 表达模板要求，优先标准 concepts；简单约束使用简洁声明形式。
- [T.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-low)—[T.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-use)：concept 应描述有意义的操作集合与语义；“某个表达式能编译”不等于满足算法所需的数学性质。
- [T.40](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-fo)、[T.41](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-essential)：用函数对象传递操作，约束只要求实现必需的能力。
- [T.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-alias)、[T.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-using)：使用别名简化类型，优先 `using`。
- [T.47](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-visible)、[T.48](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-concept-def)：避免常见名字的无约束模板污染重载集合；不支持 concepts 的环境可用 `enable_if` 表达部分要求。
- [T.49](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-erasure)、[T.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-depend)—[T.62](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-nondependent)：按需使用类型擦除，减少模板对上下文的依赖和不必要的实例化。
- [T.64](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-specialization)、[T.65](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-tag-dispatch)、[T.69](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-customization)：用类模板特化或标签分派选择实现；模板中的非限定调用应明确是否作为定制点使用。
- [T.80](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-hier)—[T.84](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-abi)：不机械地模板化继承层次；成员函数模板不能为虚函数，稳定 ABI 可使用非模板核心。
- [T.100](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-variadic)、[T.103](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-variadic-not)：异构参数使用可变参数模板；同类型序列优先容器、范围等接口。
- [T.120](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-metameta)—[T.125](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-lib)：只在必要时进行模板元编程；编译期值计算优先 `constexpr`，类型计算优先标准库设施。
- [T.144](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-specialize-function)、[T.150](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-check-class)：避免函数模板显式特化带来的重载困惑；用 `static_assert` 检查具体类型是否符合 concept。

原文中仍有历史过渡建议和未完成条目，例如 [T.101](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-variadic-pass)；应结合所用标准和工具链判断，不把草稿视为成熟约定。

## CPL：C 风格编程

- [CPL.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcpl-c)：新 C++ 代码优先使用 C++ 的类型和资源管理能力。
- [CPL.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcpl-subset)：原文建议必须使用 C 时采用 C/C++ 公共子集，并用 C++ 编译器检查；这是代码选择建议，不意味着任意 C 程序都是有效 C++。
- [CPL.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcpl-interface)：必须提供 C 接口时，调用方仍可用 C++ 封装类型、资源和错误处理。

## SF：源文件

- [SF.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-file-suffix)：没有既有约定时使用 `.cpp` / `.h`；已有工程保持一致。
- [SF.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-inline)、[SF.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-declaration-header)：共享接口集中声明在头文件中，避免普通对象定义和非 inline 函数定义导致重复定义；注意规则正文对模板等情况的说明。
- [SF.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-consistency)：实现文件包含自己的接口头文件，以检查声明与定义的一致性。
- [SF.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-using-directive)、[SF.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-guards)：头文件不在全局写 `using namespace`，并使用 include guard。
- [SF.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-cycles)—[SF.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-contained)：避免循环依赖，不依靠间接包含，头文件应可独立包含。
- [SF.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-namespace)—[SF.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-unnamed2)：命名空间表达逻辑结构；实现文件内部实体可用匿名命名空间，头文件避免使用匿名命名空间。

## SL：标准库

- [SL.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-lib)—[SL.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#sl-safe)：优先成熟库和标准库，按类型安全的方式使用；不向 `std` 随意添加实体。
- [SL.con.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-arrays)、[SL.con.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-vector)：固定长度用 `array`，动态序列默认考虑 `vector`，有实际理由再选其他容器。
- [SL.con.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-bounds)、[SL.con.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-copy)：防止越界，不对非 trivially-copyable 对象进行字节级复制。
- [SL.str.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-string)、[SL.str.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-view)：`string` 拥有字符，`string_view` 借用字符；借用的生命周期由调用者和接口共同保证。
- [SL.str.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-zstring)、[SL.str.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-byte)：区分零结尾字符串与任意字节数据，后者可使用 `std::byte`。
- [SL.str.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-span)：要修改字符序列时使用可写字符范围，不用只读 `string_view`。
- [SL.io.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rio-validate)：输入必须考虑格式错误。
- [SL.io.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rio-sync)、[SL.io.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rio-endl)：不混用 C 标准 I/O 时可考虑关闭同步；普通换行用 `\n`，不需要刷新时避免 `endl`。
- [SL.C.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rclib-jmp)：避免 `setjmp` / `longjmp` 破坏正常的对象生命周期管理。

原文的正则、时间等子章节仍较简略，不能由章节存在推断其已经提供完整实践指南。

## 支撑章节

| 原文章节 | 整理要点 |
| --- | --- |
| [A：架构](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-a) | 稳定与易变部分分离，可复用部分形成库，库依赖避免成环 |
| [NR：误区](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-not) | 不强制变量集中声明、单一 return、每类一个文件；避免两阶段初始化和末尾集中手动清理 |
| [RF：参考资料](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-references) | 书籍、网站、视频和其他规范入口；部分资料年代较早 |
| [Pro：安全配置](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-profile) | 类型、边界、生命周期三个方向相互配合；属于规则与检查要求，不是编译器自动提供的完整安全保证 |
| [GSL：支持库](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-gsl) | 通过视图、所有权标注、非空类型与断言表达约束；具体实现支持情况需另外核对 |
| [NL：命名与布局](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-naming) | 注释解释意图、保持风格一致、避免难辨名字；服从项目已有风格 |
| [FAQ](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-faq) | 解释指南定位、维护方式及 GSL 与标准库的关系 |
| [附录 A：库](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-libraries) | 支持库相关补充，原文尚不完整 |
| [附录 B：现代化](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-modernizing) | 渐进改造旧代码，结合工具处理易识别的旧式写法 |
| [附录 C：讨论](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-discussion) | 初始化顺序、工厂、析构、noexcept 与资源安全等深入解释 |
| [附录 D：工具](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-tools) | clang-tidy、CppCoreCheck 等检查入口 |
| [术语表](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-glossary) | 查阅术语定义 |
| [待整理规则](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-unclassified) | 未分类和待完善内容，不能当作已定稿规则 |

### 标准库与 GSL 不要混淆

| 名称 | 阅读时需要区分的含义 |
| --- | --- |
| `std::span` | C++20 连续范围视图；携带长度，不拥有元素；不能假定 C++20 的下标操作自动检查越界 |
| `std::string_view` | C++17 只读字符串视图，不保证零结尾，不延长底层字符串生命周期 |
| `gsl::owner<T*>` | 所有权标注，不等同于有析构管理能力的 `unique_ptr` |
| `gsl::not_null<T*>` | 表达非空要求，不能证明对象仍然存活 |
| `Expects` / `Ensures` | 原文 GSL 风格的前置/后置条件设施，不是 C++20 标准语言契约 |
| `gsl::joining_thread` | 原文的自动 join 线程抽象；C++20 项目也可结合 `std::jthread` 及其停止请求语义设计 |

上表是整理时的版本与语义提示；原文中的 `span_p`、`synchronized_value` 等名称也不能直接视为标准库现成组件。

## 代码审查清单

- 接口是否明确输入、输出、可空性、范围与所有权？是否存在易交换的参数？
- 每项资源是否及时进入 RAII 句柄？异常、提前返回和部分构造失败时是否安全？
- 指针、引用、视图、迭代器是否可能因释放、容器修改、线程退出或协程挂起失效？
- 类是否建立不变量？复制、移动、析构与多态销毁是否一致？
- 是否存在未初始化、窄化、符号混算、越界和依赖求值顺序的表达式？
- `noexcept` 是否真实？错误是否在能够处理的边界处理？
- 共享可写数据是否有同步？是否持锁调用未知代码或跨协程挂起？
- 模板是否表达所需约束？实现是否增加了无收益的复杂度？
- 性能结论是否有可重复的测量？头文件和依赖是否自洽？

## 来源与维护

- 在线原文：[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)。
- 整理依据的 Markdown 修订：[33bcd015997f0d8e0fa0202eb66254a16f59ad8f](https://github.com/isocpp/CppCoreGuidelines/blob/33bcd015997f0d8e0fa0202eb66254a16f59ad8f/CppCoreGuidelines.md)。
- [完整规则索引](core-guidelines-index.md) 保留原文标题与锚点；本文按主题合并重点，没有逐条覆盖全部正文。
- 原文仍在更新，带 `???` 或 removed 的条目保留其草稿/移除状态；新增内容应以具体规则正文和适用标准为准。
- Copyright (c) Standard C++ Foundation and its contributors。
  原文授权文本随附于 [core-guidelines-LICENSE.txt](core-guidelines-LICENSE.txt)，适用于所引用和整理的原文内容。

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
