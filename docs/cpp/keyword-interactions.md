# C++ 关键字全览与组合关系：类、函数、继承及类外用法

第 1～12 节以 C++17 为基础，讨论类、函数与继承中的关键字组合，并单独标明后续版本扩展。
第 13 节补齐 C++98～C++23 的全部标准关键字，并单列 C++26 工作草案新增项；
第 14 节整理函数声明中关键字的位置，第 15 节比较容易混淆的常量相关关键字。

快速入口：[全部关键字](#13-c-全部关键字按用途查阅)、
[特殊标识符与非关键字](#1310-特殊标识符以及看起来像关键字的名字)、
[函数声明中的位置](#14-函数左边和右边分别能放什么)、
[constconstexprconstevalconstinit](#15-constconstexprconstevalconstinit-各管什么)。

## 1. 总览表

| 概念                                    | 典型位置                              | 控制的问题                                             | 不代表什么                               |
| --------------------------------------- | ------------------------------------- | ------------------------------------------------------ | ---------------------------------------- |
| `static` 数据成员                       | `inline static int count = 0;`        | 数据属于类，不随每个对象各存一份                       | 不表示只读，也不保证并发安全             |
| `static` 成员函数                       | `static void reset();`                | 调用没有隐式对象参数，没有 `this`                      | 不支持运行时虚派发                       |
| `virtual`                               | `virtual void run();`                 | 通过对象进行虚调用时，选择最终重写函数                 | 不表示必须公开，也不表示必须重写         |
| `override`                              | `void run() override;`                | 要求编译器验证此函数确实重写基类虚函数                 | 不负责第一次建立虚函数接口               |
| `final`                                 | `void run() final;` / `class D final` | 禁止继续重写该虚函数 / 继续派生该类                    | 不改变访问权限                           |
| `public` / `protected` / `private` 成员 | 类内访问区段                          | 谁可以通过名称访问成员                                 | 不决定是否构成重写                       |
| `public` / `protected` / `private` 继承 | `class D : private B`                 | 基类成员的继承访问权限，以及基类转换的可访问性         | 不决定虚函数是否继续有效                 |
| `inline`                                | 函数、C++17 起的变量                  | 在满足 ODR 等要求时，支持跨翻译单元定义                | 不保证机器码中发生内联展开               |
| `const` 数据成员                        | `const int id;`                       | 成员初始化后不能被普通赋值修改                         | 不代表全类共享                           |
| 尾部 `const`                            | `int size() const;`                   | 限制通过 `this` 修改当前对象，并参与成员函数匹配       | 不保证无副作用或线程安全                 |
| 参数/返回类型中的 `const` ?               | `const T& get() const;`               | 限定对应参数或返回结果                                 | 与函数尾部 `const` 是不同位置的约束      |
| `friend`                                | `friend class Inspector;`             | 让指定函数或类访问本类非公开成员                       | 不会让普通友元函数成为本类成员           |
| `explicit`                 ?             | 构造函数、转换函数                    | 限制隐式转换与部分初始化方式                           | 不等于 `private`，也不是“禁止复制”       |
| 默认参数                                | `void run(int n = 1);`                | 调用时省略实参，由可见声明补足                         | 不参与虚派发，也不是函数类型的一部分     |
| `= default`                             | 特殊成员函数，C++20 起的特定比较函数  | 请求编译器按语言规则生成实现                           | 不保证该函数可用，也不等于空函数体       |
| `= delete`                              | 函数声明                              | 禁止调用被选中的函数                                   | 不只是“隐藏实现”                         |
| `= 0`                                   | `virtual void run() = 0;`             | 声明纯虚函数；类是否抽象取决于最终重写函数是否仍为纯虚 | 不等于 `= default`，也不绝对禁止提供定义 |

static 其他的作用:
1. 命名空间作用域的 `static` 控制内部链接；
2. 函数内局部 `static` 控制静态存储期。

关键字不限于类内使用；命名空间、普通函数、局部变量、模板、控制流等场景见第 13 节。

## 2. 组合速查表

| 组合                                       | 是否可行       | 结果或注意事项                                             |
| ------------------------------------------ | -------------- | ---------------------------------------------------------- |
| `static` 成员函数 + `virtual` / `override` | 不可行         | 没有用于虚派发的隐式对象                                   |
| `static` 成员函数 + 尾部 `const`           | 不可行         | 没有 `this` 可供限定；参数、返回值仍可以带 `const`         |
| `static` 数据成员 + `const` + `inline`     | 可行           | `inline static const int limit = 8;`，C++17 起可在类内定义 |
| `virtual` + 尾部 `const`                   | 可行           | 派生类重写必须匹配 `const` 限定                            |
| `virtual` + `private`                      | 可行           | 不允许无权代码直接访问，但仍可重写                         |
| `override` + `private`                     | 可行           | 重写函数可以比基类声明更不公开                             |
| `virtual` + `inline`                       | 可行           | 虚派发语义与是否内联展开是两个问题                         |
| `virtual` + 默认参数                       | 可行，但易误用 | 函数实现按动态类型选，默认实参按调用处声明选               |
| 私有继承 + `override`                      | 可行           | 对外隐藏基类转换，内部仍有虚派发                           |
| 构造函数 + `virtual` / 尾部 `const`        | 不可行         | 构造函数不支持这两种修饰                                   |
| 析构函数 + `virtual` + `= default`         | 可行           | 常见于允许通过基类指针销毁对象的接口                       |
| 普通业务函数 + `= default`                 | 不可行         | 编译器不知道 `run()` 应该如何实现                          |
| 非成员友元函数 + `virtual` / 尾部 `const`  | 不可行         | 它不是成员函数；但另一个类的虚成员函数可以成为本类友元     |
| `friend` + `const T&` 参数                 | 可行           | 获得私有访问权，仍受对象的 `const` 限制                    |
| `explicit` 构造函数 + 默认参数             | 可行           | `explicit X(int n = 0);` 也可以是默认构造函数              |
| `explicit` 转换函数 + 尾部 `const`         | 可行           | 例如 `explicit operator bool() const;`                     |
| `explicit` 转换函数 + `virtual`            | 可行           | 转换函数也可以参与虚派发，不常用                           |
| `explicit` + `= default`                   | 可行           | 例如 `explicit X() = default;`；不意味着两者作用相同       |
| `const` 标量成员 + 默认赋值运算符          | 通常导致不可用 | `const int` 不能被赋值，使默认复制/移动赋值被定义为删除    |

## 3. 一次调用中，哪些事情静态决定，哪些动态决定

可用下面的顺序辅助阅读代码；这是分析方法，不是编译器必须采用的实现流水线。

| 需要判断的事               | 主要依据                               |
| -------------------------- | -------------------------------------- |
| 从哪里查找成员、有哪些重载 | 表达式的静态类型及当前作用域           |
| 选中的声明能否访问         | 该声明的访问权限、调用上下文、继承路径 |
| 省略的参数取什么值         | 调用处可见且适用的默认参数声明         |
| 普通非虚函数执行哪个实现   | 静态选中的函数                         |
| 虚调用执行哪个实现         | 对象的最终重写函数，通常由动态类型决定 |

`Base& b = derived;` 中，`b` 的静态类型是 `Base`，所引用对象的动态类型可以是
`Derived`。以下两节展示：**访问权限和默认参数不会跟着运行时选中的函数一起重新选择**。

### 3.1 基类 public，重写函数 private

```cpp
#include <cassert>

struct Interface {
    virtual int value() const { return 1; }
    virtual ~Interface() = default;
};

class Impl : public Interface {
private:
    int value() const override { return 2; }
};

int main() {
    Impl object;
    Interface& interface = object;
    assert(interface.value() == 2); // 合法，最终执行 private 的 Impl::value()
    // object.value();              // 错误：此处访问的是 Impl::value()
    assert(object.Interface::value() == 1); // 显式限定，关闭这次调用的虚派发
}
```

这里 `interface.value()` 检查的是 `Interface::value()`
的访问权限。通过检查后，派发到 `Impl::value()`，不会再因后者是 `private`
而拒绝。规则见 [class.access.virt](https://eel.is/c++draft/class.access.virt)。

因此，**把重写函数改成 private，并不能撤销基类已经公开的调用入口**。

### 3.2 默认参数静态选择，虚函数实现动态选择

```cpp
#include <cassert>

struct Base {
    virtual int run(int n = 1) { return 100 + n; }
    virtual ~Base() = default;
};

struct Derived : Base {
    int run(int n = 2) override { return 200 + n; }
};

int main() {
    Derived object;
    Base& base = object;
    assert(object.run() == 202); // Derived::run(2)
    assert(base.run() == 201);   // Derived::run(1)
}
```

派生类的重写声明不继承基类默认参数。若把 `Derived::run()` 的声明改为
`int run(int n) override`，`base.run()` 仍合法，但 `object.run()`
会因缺少参数而报错。

默认参数表达式在每次需要它的调用中求值，不是“一律在编译期算出的常量”；静态确定的是使用哪处声明。规则见
[dcl.fct.default](https://eel.is/c++draft/dcl.fct.default)。

设计上可把默认参数放在单一非虚入口，再把显式实参传给虚函数，见下一节。

## 4. “virtual 函数最好 private”到底适用于什么

这句话对应 **NVI（Non-Virtual
Interface，非虚接口）**：把给调用者的入口与给派生类的扩展点分开。

```cpp
#include <cassert>
#include <stdexcept>

class Processor {
public:
    int run(int count = 1) const {
        if (count < 0) {
            throw std::invalid_argument("negative count");
        }
        return do_run(count);
    }

    virtual ~Processor() = default;

private:
    virtual int do_run(int count) const = 0;
};

class Doubler final : public Processor {
private:
    int do_run(int count) const override { return count * 2; }
};

int main() {
    Doubler object;
    const Processor& processor = object;
    assert(processor.run() == 2);
    bool rejected = false;
    try {
        processor.run(-1);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
}
```

| 角色                         | 推荐权限与形式             | 原因                                                    |
| ---------------------------- | -------------------------- | ------------------------------------------------------- |
| 对外统一入口 `run()`         | `public`、非虚             | 集中默认参数、前置条件，以及必要的日志、锁等处理        |
| 派生类扩展点 `do_run()`      | `private virtual`          | 派生类可以替换实现，但不能直接调用基类这个私有入口      |
| 允许派生类调用的基类默认实现 | 可考虑 `protected virtual` | 派生类需要写 `Base::do_run(...)` 时，应明确授予调用权限 |
| 单纯的多态接口方法           | 可以 `public virtual`      | 没有统一包装逻辑时，公开纯虚接口也很自然                |
| 支持基类指针删除的析构函数   | `public virtual`           | 调用者必须能访问析构函数，并正确销毁派生部分            |

NVI 的价值来自统一入口，不来自 `private` 本身。派生类仍可声明自己的同名 `run()`
隐藏非虚入口；通过派生类类型调用时要留意这一点。`private virtual`
也不是一种禁止派生类扩展行为的安全边界。什么时候必须退到 `protected virtual`、以及 `protected` 作为访问说明符 / 继承说明符 / 构造析构控制的完整讨论，见
[protected：存在理由与经典用法](protected.md)。

析构函数需要单独考虑：常见设计选择是 **public virtual** 或 **protected
non-virtual**。后一种禁止普通外部代码通过基类指针执行
`delete`，但允许派生类析构时销毁基类子对象。仅仅“可能被继承”不足以要求虚析构函数。参见
[C++ Core Guidelines C.35](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-virtual)。

若把析构函数声明为纯虚，例如
`virtual ~Base() = 0;`，当需要销毁该类或派生类对象时，仍必须提供析构函数定义，可以在类外写
`Base::~Base() = default;`。纯虚与拥有定义并不冲突。参见
[class.dtor](https://eel.is/c++draft/class.dtor)。

补充：构造/析构期间的虚调用只派发到当前正在构造/析构的类层次，不会调用尚未构造好或已析构的更派生实现。不要在基类构造函数中调用上面的
`run()` 来期望执行派生类钩子；这里会间接涉及纯虚调用。规则见
[class.cdtor](https://eel.is/c++draft/class.cdtor)。

## 5. private 成员与 private 继承是两条不同的轴

`private: virtual void hook();` 控制某个成员的访问。 `class D : private B`
控制一条继承关系的访问。 可以独立选择它们。

### 5.1 继承访问权限表

以下假定无额外友元授权、无 `using`
调整访问权限，并且只有一条无歧义的直接继承路径。

| 基类中的成员                           | public 继承后         | protected 继承后      | private 继承后       |
| -------------------------------------- | --------------------- | --------------------- | -------------------- |
| `public`                               | 作为 public 可访问    | 作为 protected 可访问 | 作为 private 可访问  |
| `protected`                            | 作为 protected 可访问 | 作为 protected 可访问 | 作为 private 可访问  |
| `private`                              | 派生类仍不能直接访问  | 派生类仍不能直接访问  | 派生类仍不能直接访问 |
| 普通外部代码的 `D* → B*` 隐式转换      | 允许                  | 不允许                | 不允许               |
| `D` 自己的成员/友元中的 `D* → B*` 转换 | 允许                  | 允许                  | 允许                 |
| 重写 `B` 的虚函数                      | 允许                  | 允许                  | 允许                 |

“不能访问基类 private 成员”不意味着那些成员不在基类子对象里，也不影响重写基类
private 虚函数。`class` 默认成员访问和默认继承都是 private；`struct` 都是
public。继承访问规则见
[class.access.base](https://eel.is/c++draft/class.access.base)。

### 5.2 私有继承仍然支持虚派发

```cpp
#include <cassert>
#include <type_traits>

class Engine {
public:
    int execute() const { return compute(); }

protected:
    ~Engine() = default; // 不允许外部通过 Engine* 删除

private:
    virtual int compute() const = 0;
};

class Adapter : private Engine {
public:
    int run() const {
        const Engine& engine = *this; // 内部允许转换
        return engine.execute();     // 仍会派发到 Adapter::compute()
    }

private:
    int compute() const override { return 42; }
};

int main() {
    Adapter object;
    assert(object.run() == 42);
    static_assert(std::is_base_of_v<Engine, Adapter>);
    static_assert(!std::is_convertible_v<Adapter*, Engine*>);
    // Engine& engine = object; // 错误：普通外部代码不能转换
    // object.execute();       // 错误：经私有继承，该入口不公开
}
```

**私有继承保留继承机制，用访问控制限制对外接口。** `is_base_of`
为真并不保证外部可转换，歧义继承也会使二者产生区别。

| 需求                                                  | 通常选择               | 主要代价                       |
| ----------------------------------------------------- | ---------------------- | ------------------------------ |
| 调用者应能把 D 当 B 使用，且行为符合 B 的约定         | public 继承            | D 必须维护 B 的行为契约        |
| 只想复用 B 的普通功能                                 | 组合：`B member_;`     | 必要时需要转发成员调用         |
| 复用基类框架，同时需要重写其钩子或访问 protected 成员 | 可考虑 private 继承    | 与基类实现约定耦合更深         |
| 需要通过成员对象参与回调，但不希望外层类继承          | 组合一个内部派生适配器 | 增加一个适配类型，但依赖更集中 |

上表是设计建议：private 继承不是 NVI 的必要条件，NVI 也不要求使用 private
继承。空基类优化可能是私有继承的另一种动机，但 C++20 还有
`[[no_unique_address]]` 成员方案，布局是否缩小应实际验证。

## 6. const：修饰成员、函数和类型时的区别

| 写法                                 | 限制的对象           | 关键区别                                                 |
| ------------------------------------ | -------------------- | -------------------------------------------------------- |
| `const int id;`                      | 每个对象中的 `id`    | 必须在初始化阶段确定，不能在构造函数体里普通赋值         |
| `inline static const int limit = 8;` | 类共享的数据成员     | 不占每个对象的一份存储                                   |
| `int size() const;`                  | 当前隐式对象         | 可通过 `const T&` 调用，不能通过 `this` 修改普通数据成员 |
| `const T& get() const;`              | 返回的对象与当前对象 | 前面的 const 限制返回引用，尾部 const 限制当前对象       |
| `void use(const T& value);`          | 引用所指对象         | 不能通过此引用修改普通状态                               |
| `void use(const T value);`           | 函数内部的参数副本   | 不限制调用者的原对象，不能与 `use(T)` 构成重载           |
| `const T* p`                         | 指针所指对象         | 可改变 `p`，不能通过 `p` 修改对象                        |
| `T* const p`                         | 指针本身             | 不能改变 `p`，可修改所指对象                             |

尾部 `const`
针对本文使用的传统隐式对象成员函数。它参与重载及重写匹配；参数的顶层 `const`
不用于区分函数类型。成员函数规则见
[class.mfct.non.static](https://eel.is/c++draft/class.mfct.non.static)。

```cpp
#include <cassert>

struct Query {
    virtual int read() const = 0;
    virtual ~Query() = default;
};

struct CachedQuery : Query {
    int read() const override {
        ++hits;
        return 7;
    }

    mutable int hits = 0;
    // int read() override; // 错误：缺少 const，没有重写 Query::read()
};

int main() {
    const CachedQuery query;
    assert(query.read() == 7);
    assert(query.hits == 1);
}
```

`const` 成员函数仍能修改 `mutable`
成员、静态数据、全局状态，也可能通过指针成员修改外部对象。它表达的是受限的对象访问方式，不是“纯函数”。上面的计数在多线程并发调用时还需要同步。

`const int`
或引用成员会让默认复制/移动赋值运算符被定义为删除，但不一定影响复制/移动构造：构造是在初始化新对象，赋值是在修改已有对象。对于
`const` 类类型成员，需要进一步看其赋值操作是否可用，不能把标量结论机械套用。见
[class.copy.assign](https://eel.is/c++draft/class.copy.assign)。

## 7. static、隐藏、重载和 override

| 情形                               | 名称                 | 是否运行时派发                                 |
| ---------------------------------- | -------------------- | ---------------------------------------------- |
| 同一作用域有 `f(int)`、`f(double)` | 重载 overload        | 重载选择本身在编译期完成；选中虚函数后可再派发 |
| 派生类用匹配签名实现基类虚函数     | 重写 override        | 是                                             |
| 派生类声明同名函数，挡住基类名字   | 名字隐藏 name hiding | 隐藏本身不产生虚派发                           |
| 派生类声明与基类同名的 static 函数 | 静态成员的名字隐藏   | 否                                             |

静态成员函数可以重载，只是不能重写虚函数。现有
[static-override.cpp](static-override.cpp) 展示的静态同名函数，应按隐藏来理解。

重写时通常只写 `override`，不用再重复
`virtual`。返回类型通常相同，但特定类指针/引用允许协变；不能仅凭“函数名相同”判断重写。引用限定符也需要匹配。显式写
`override` 能让签名错误及时报出。

派生类的 `f(int)` 可能隐藏基类其他 `f` 重载，需要时用 `using Base::f;`
把基类重载引入。`using` 不会把非虚函数变成虚函数，也不能让无权访问的基类 private
函数变得可访问。

## 8. inline：定义规则与机器码优化分开看

`inline` 不强制编译器把函数体展开到调用点，也不阻止编译器展开未标记 `inline`
的函数。

| 场景                                   | 定义规则或优化结论                                                    |
| -------------------------------------- | --------------------------------------------------------------------- |
| 普通头文件中，在类体内定义成员函数     | 隐式 inline；C++20 附属于命名模块的定义有例外                         |
| 头文件中，在类体外定义普通成员函数     | 不会仅因“是成员函数”自动 inline；通常需要显式 inline 或放入单个源文件 |
| `inline virtual int f() { return 1; }` | 合法，不改变虚调用语义                                                |
| 通过基类引用调用虚函数                 | 若编译器能证明目标，可去虚化并继续内联；不能保证，也非绝对不能        |
| 显式限定调用 `obj.Base::f()`           | 抑制此次虚派发，但依然不保证内联展开                                  |
| 类内 `inline static int count = 0;`    | C++17 起可直接提供静态成员定义，跨翻译单元共享该成员                  |

跨翻译单元的 inline 定义需要满足
ODR（一处定义规则）的要求，包括一致的定义及相应名字查找约束；不是“只要写
inline，就能任意重复定义”。同一个翻译单元仍不能随意重复定义同一实体。

类内首次声明就写 `= default` 的函数也是隐式 inline；`constexpr` 函数同样隐式
inline，C++17 起 `constexpr` 静态数据成员也隐式 inline。参见
[dcl.inline](https://eel.is/c++draft/dcl.inline) 和
[dcl.fct.def.default](https://eel.is/c++draft/dcl.fct.def.default)。

## 9. friend：访问授权不改变函数归属

```cpp
#include <cassert>

class Reading {
public:
    explicit Reading(int value) : value_(value) {}

private:
    int value_;

    friend bool operator==(const Reading& a, const Reading& b) {
        return a.value_ == b.value_;
    }
};

int main() {
    const Reading a{3};
    const Reading b{3};
    assert(a == b);
}
```

这里 `operator==` 是非成员函数，无 `this`，不写尾部 `const`。它的只读约束来自
`const Reading&` 参数。它定义在类里并获得私有访问权，但并未因此变成
`Reading::operator==`；这种隐藏友元可由 ADL（实参依赖查找）发现。

| friend 与其他机制          | 关系                                                                     |
| -------------------------- | ------------------------------------------------------------------------ |
| 放在 public / private 区段 | 授予的友元权限相同，区段位置不决定友元函数的“公私有”                     |
| `friend class Inspector;`  | 把访问权授予指定类，而非所有派生类                                       |
| 友元的继承与传递           | 友元关系不继承、不传递，也不自动双向                                     |
| `const` 对象               | 友元仍不能通过 const 引用普通修改非 mutable 状态                         |
| private 继承               | 派生类的友元可在允许的上下文做私有基类转换；不自动取得基类私有成员访问权 |
| 另一个类的成员函数         | 可以只把那个成员函数声明为友元；即便它是虚函数也允许                     |

友元是一种明确、局部的封装授权。它适合需要对称操作数的运算符和紧密协作实现，但会让受授权代码依赖私有表示。规则见
[class.friend](https://eel.is/c++draft/class.friend)。

## 10. explicit：控制转换，不控制访问

```cpp
#include <cassert>

class Count {
public:
    explicit Count(int value = 0) : value_(value) {}
    explicit operator bool() const { return value_ != 0; }

private:
    int value_;
};

int main() {
    Count a(3);             // 直接初始化
    Count b{3};             // 直接列表初始化
    Count c;                // 默认初始化，使用默认实参 0
    Count d{};              // 直接列表初始化，合法
    Count copy = a;         // 合法：调用隐式复制构造函数
    // Count e = 3;         // 错误：不能用 explicit 构造函数隐式转换
    // Count f = {3};       // 错误：复制列表初始化选中 explicit 构造函数
    // Count g = {};        // 同样错误，explicit 默认构造函数也受此限制
    // bool ready = a;      // 错误：不是允许 explicit bool 转换的语境
    if (a) {                // 合法：按语境转换为 bool
        assert(static_cast<bool>(b));
    }
    assert(!c && !d && static_cast<bool>(copy));
}
```

`explicit Count(int)` 不会把另一个复制构造函数自动变为
explicit，也不禁止复制。同样，函数形参若为 `Count`，不能直接传 `3`
来隐式构造，应写 `consume(Count{3})`。

`explicit` 构造函数仍需要可访问：把它放在 private 后，外部即使用 `Count{3}`
也不能调用。反过来，public 构造函数加 explicit 仍可直接初始化。规则见
[class.conv.ctor](https://eel.is/c++draft/class.conv.ctor)。

C++20 支持 `explicit(condition)`，常用于模板中按条件控制转换；此外 explicit
也可用于推导指引。普通业务函数不能通过加 explicit 来“要求显式调用”。

## 11. default 的几种含义

| 表达              | 实际含义                         | 示例                       |
| ----------------- | -------------------------------- | -------------------------- |
| 默认参数          | 省略实参时使用某表达式           | `void f(int n = 1);`       |
| 默认构造函数      | 能够无实参调用的构造函数类别     | `X();` 或 `X(int n = 1);`  |
| 默认成员初始化器  | 构造函数未另行初始化该成员时使用 | `int n = 1;`               |
| 显式默认化        | 请求语言规定的函数实现           | `X() = default;`           |
| 默认初始化        | 一种初始化形式                   | `X x;`；具体效果取决于类型 |
| switch 的 default | 没有 case 匹配时的分支标签       | `default: break;`          |

### 11.1 = default 可以生成什么

常见对象是默认构造、复制/移动构造、复制/移动赋值和析构函数。C++20
还支持满足约束的比较函数，例如 `bool operator==(const X&) const = default;`。

显式默认化不能附带默认实参，也不能用于普通
`void run() = default;`。生成函数若无法完成成员或基类操作，可能被定义为删除。见
[dcl.fct.def.default](https://eel.is/c++draft/dcl.fct.def.default)。

```cpp
#include <type_traits>

struct Record {
    const int id = 0;
    Record() = default;
    Record(const Record&) = default;
    // Record& operator=(const Record&) = default;
    // 即使显式请求默认赋值，也会因 const int 成员而被定义为删除。
};

static_assert(std::is_default_constructible_v<Record>);
static_assert(std::is_copy_constructible_v<Record>);
static_assert(!std::is_copy_assignable_v<Record>);

int main() {}
```

### 11.2 = default 与 {} 并不相同

```cpp
struct A {
    int value;
    A() = default;
};

struct B {
    int value;
    B() {} // 用户提供的构造函数
};

void example() {
    [[maybe_unused]] A a{}; // 值初始化，value 为 0
    [[maybe_unused]] B b{}; // value 未初始化，不能拿来读取
    [[maybe_unused]] A c;   // 默认初始化，value 同样未初始化
}
```

`A() = default` 在首次声明处默认化，不是
user-provided（用户提供的）构造函数；`B() {}`
是。这会影响值初始化是否先零初始化，以及 trivial 等性质。若先声明
`A();`，再在类外写
`A::A() = default;`，则属于用户提供的构造函数，不能照搬上面的零初始化结论。初始化规则见
[dcl.init](https://eel.is/c++draft/dcl.init)。

### 11.3 virtual 析构函数与移动生成的耦合

即使只写 `virtual ~Base() = default;`，它也已经是
user-declared（用户声明的）析构函数，会抑制该类移动构造和移动赋值的隐式声明。

这不等于所有右值构造都会报错：复制构造函数可能接受右值，使调用退回复制；`is_move_constructible`
为真也不一定证明存在移动构造函数。多态基类是否应允许复制还需考虑对象切片，不要为了恢复移动而无条件公开全部复制操作。见
[class.copy.ctor](https://eel.is/c++draft/class.copy.ctor)。

## 12. 按需求选择，而不是按关键字背口诀

| 需求                                   | 首先考虑                                                     |
| -------------------------------------- | ------------------------------------------------------------ |
| 函数不依赖具体对象，但属于类的职责     | static 成员函数；不需要私有访问时也可考虑非成员函数          |
| 需要通过基类接口替换实现               | virtual，派生类明确写 override                               |
| 需要统一校验、日志或锁，再定制少量步骤 | public 非虚入口 + private virtual 钩子                       |
| 派生类需要直接调用基类的钩子实现       | protected virtual，并明确允许的调用方式                      |
| 只需要复用已有类的能力                 | 优先组合；需要继承钩子时再评估 private 继承                  |
| 接口有默认参数且实现是多态的           | 默认值集中到非虚入口，传显式参数给虚钩子                     |
| 函数逻辑上只读取对象                   | 尾部 const；缓存和外部状态仍要独立评估                       |
| 单参数构造不应悄悄发生                 | explicit；默认参数可能使多参数构造也能单实参调用             |
| 少数外部操作确实需要私有表示           | 精确授予 friend，不必为此公开全部数据                        |
| 头文件需要承载定义                     | 按 inline / 模板 / 链接规则安排，不把 inline 当性能保证      |
| 特殊成员函数采用逐成员语义即可         | 考虑省略声明或 = default，并检查 const、引用、资源成员的影响 |

回看本文两组最关键的区别：**private 成员控制直接访问，private
继承控制继承路径的访问；virtual 控制动态派发，override 验证重写关系。**
把这几件事分开后，大多数组合都能按各自规则推导。

已有示例可对照
[visibility.cpp](visibility.cpp)、[virtual-default-parameter.cpp](virtual-default-parameter.cpp)、[explicit.cpp](../operator/explicit.cpp)。本文标有
`main()` 的第 1～12 节代码块可各自作为 C++17 程序编译；标为错误的语句保留为注释。

第 1～12 节原有验证记录：使用 GCC 和 Clang，以 C++17、`-Wall -Wextra -pedantic-errors` 编译全部
9 个代码块，8 个完整程序运行通过；另验证了 13
种预期编译失败的写法，覆盖访问权限、默认参数、const 重写匹配、explicit
初始化及不合法的关键字组合。

## 13. C++ 全部关键字：按用途查阅

这里的“全部”以标准 C++ 为范围，不把编译器扩展、标准库名字、属性名和预处理指令混在一起计数。
C++23 的 [lex.key](https://timsong-cpp.github.io/cppwp/n4950/lex.key)
列出 **81 个普通关键字拼写和 11 个运算符替代拼写，共 92 个**。
另外，`module`、`import` 仅在指定语境中形成关键字，`override`、`final` 是具有特殊含义的标识符。

以下表格覆盖这 92 个拼写。版本列写“98”表示 C++98 已有；“11”“20”分别表示 C++11、C++20 引入。
同一关键字可以承担多种语法角色，归类只是方便查找，不限制使用位置。
C++26 项按核对时的[工作草案 lex.key](https://eel.is/c++draft/lex.key) 单列，不混入 C++23 的计数。

### 13.1 基本类型、类型推导与字面量

| 关键字 | 版本 | 用途与典型写法 | 注意点 |
| --- | --- | --- | --- |
| `bool` | 98 | 布尔类型：`bool ready = true;` | 与整数类型有转换规则，但不是宏 |
| `true`、`false` | 98 | `bool` 字面量 | 不是变量名，也不需要头文件 |
| `char` | 98 | 普通字符类型：`char ch = 'A';` | 是独立类型，其有无符号由实现决定 |
| `signed`、`unsigned` | 98 | `signed char`、`unsigned int` 等整数类型组合 | 单写时隐含 `int`；不能修饰任意类型 |
| `short`、`int`、`long` | 98 | `short int`、`int`、`long int`、`long long int` | `long long` 自 C++11 标准化；不要假定 `long` 固定为 64 位 |
| `float`、`double` | 98 | 浮点类型；另有组合 `long double` | 精度和表示范围取决于实现，不保证都是特定 IEEE 格式 |
| `void` | 98 | 无返回值 `void f();`；无类型对象指针 `void*` | 不能定义 `void` 类型对象，`void*` 不能直接解引用 |
| `wchar_t` | 98 | 宽字符类型：`wchar_t ch = L'A';` | 宽度和编码依实现而定 |
| `char16_t`、`char32_t` | 11 | UTF-16 / UTF-32 代码单元类型：`u'A'`、`U'A'` | 一个 Unicode 字符不一定只占一个 UTF-16 代码单元 |
| `char8_t` | 20 | UTF-8 代码单元类型：`char8_t ch = u8'A';` | C++20 起 `u8"abc"` 的元素类型是 `const char8_t` |
| `auto` | 98；11 起改义 | 从初始化器推导类型：`auto n = 1;`；C++14 起可推导函数返回类型 | C++98/03 的自动存储期用法已移除；`auto` 按值推导通常去掉引用和顶层 `const` |
| `decltype` | 11 | 从表达式取得类型：`decltype(x)`、`decltype((x))`；C++14 起有 `decltype(auto)` | 对未加括号的名字与一般表达式适用不同规则，可能得到引用类型 |
| `nullptr` | 11 | 空指针字面量：`int* p = nullptr;` | 类型为 `std::nullptr_t`，不是整数 `0`；后者是库类型名 |

类型与推导规则见 [dcl.spec](https://eel.is/c++draft/dcl.spec)，字面量见
[lex.literal](https://eel.is/c++draft/lex.literal)。

### 13.2 类型限定、存储期、链接与常量求值

| 关键字 | 版本 | 类内与类外用法 | 注意点 |
| --- | --- | --- | --- |
| `const` | 98 | `const int n = 1;`、`const T*`、成员函数尾部 `const` | 只读访问约束不等于一定能用于常量表达式，见第 6 节 |
| `volatile` | 98 | `volatile int* reg;`；也可限定成员函数的隐式对象 | 约束相关访问的优化，不提供原子性或线程同步；不能代替 `std::atomic` |
| `mutable` | 98 | 可在 const 对象上修改的非静态数据成员；C++11 起可写 `[n]() mutable { return ++n; }` | lambda 中允许修改按值捕获的副本，不会因此修改外部原变量 |
| `static` | 98 | 类的静态成员；局部静态变量；命名空间作用域的内部链接变量/函数 | 三种位置含义不同，静态存储期不代表线程安全 |
| `extern` | 98 | `extern int count;`；`extern "C" void f();`；C++11 起 `extern template` | 分别涉及声明/链接、语言链接、显式实例化声明；带初始化器的变量声明可以是定义 |
| `thread_local` | 11 | `thread_local int count = 0;` | 每个线程有自己的对象；可与 `static` 或 `extern` 组合，不可用于普通非静态数据成员 |
| `register` | 98；17 起无用途 | 旧式局部变量/参数存储类说明符 | C++11 弃用、C++17 移除该用途，但仍是保留关键字；旧时也不保证放进寄存器 |
| `inline` | 98 | 函数；C++11 起内联命名空间；C++17 起内联变量 | `inline namespace v1 {}` 和“允许多翻译单元定义”是不同语法用途，见第 8 节 |
| `constexpr` | 11 | 常量表达式变量、可参与常量求值的函数；C++17 起 `if constexpr` | 函数调用仍可能发生在运行时；变量必须满足常量初始化等要求 |
| `consteval` | 20 | 立即函数：`consteval int answer() { return 42; }`；C++23 起 `if consteval` | 普通潜在求值语境中的立即调用必须产生常量表达式 |
| `constinit` | 20 | 静态/线程存储期变量：`constinit int count = 0;` | 要求静态初始化，变量之后可以修改；不能修饰函数 |

声明规则见 [dcl.stc](https://eel.is/c++draft/dcl.stc)、
[dcl.constexpr](https://eel.is/c++draft/dcl.constexpr) 和
[dcl.constinit](https://eel.is/c++draft/dcl.constinit)。四个 `const` 家族关键字的对照示例见第 15 节。

### 13.3 类、枚举、成员与访问控制

| 关键字 | 版本 | 用途与典型写法 | 注意点 |
| --- | --- | --- | --- |
| `class`、`struct` | 98 | 声明/定义类；C++11 起可写 `enum class` / `enum struct` | 类的默认成员权限和默认继承权限分别为 private / public；`class` 还可声明类型模板参数 |
| `union` | 98 | 多个成员共享存储的联合体 | 必须遵守成员生命周期和活跃成员规则，不能任意读取其他成员来转换类型 |
| `enum` | 98 | `enum Color { red, blue };`；C++11 起支持作用域枚举及指定底层类型 | `enum class Color : unsigned { red, blue };` 不隐式转为整数 |
| `public`、`protected`、`private` | 98 | 成员访问区段、基类访问说明符 | `private` 另用于 C++20 模块的 `module :private;`，见第 5 节 |
| `virtual` | 98 | 虚函数、虚继承：`struct D : virtual B {};` | 虚继承用于共享虚基类子对象，与虚函数派发是两个机制 |
| `friend` | 98 | 授予函数或类型私有/受保护成员访问权 | 不是继承关系，不表示自动成为成员函数，见第 9 节 |
| `explicit` | 98 | 构造函数；C++11 起转换函数；C++17 起推导指引 | C++20 支持 `explicit(condition)`；只影响指定转换规则，见第 10 节 |
| `this` | 98 | 隐式对象成员函数中的当前对象指针；lambda 捕获 `this`，C++17 起可捕获 `*this` | C++23 还引入显式对象形参，如 `void f(this X& self);`，这种函数没有隐式 `this` |
| `operator` | 98 | 运算符函数 `operator+`、转换函数 `operator bool`、C++11 起字面量运算符 | 不能发明新的运算符符号，也不能改变运算符优先级 |

`override`、`final` 见第 13.10 节。相关规则见
[class](https://eel.is/c++draft/class) 和 [dcl.enum](https://eel.is/c++draft/dcl.enum)。

### 13.4 控制流与返回

| 关键字 | 版本 | 用途与典型写法 | 注意点 |
| --- | --- | --- | --- |
| `if`、`else` | 98 | 条件分支；C++17 起 `if constexpr`；C++23 起 `if consteval` | 普通 `if` 不会因为条件恒假就普遍免除另一分支的编译要求 |
| `switch`、`case` | 98 | 按整数/枚举等允许的条件值分支 | `case` 标签要求转换后的常量表达式；没有 `break` 等跳转会继续执行后续代码 |
| `default` | 98 | `switch` 的兜底标签；C++11 起显式默认化函数 `= default` | 默认参数 `int n = 1` 本身没有使用这个关键字，见第 11 节 |
| `for` | 98 | 计数循环；C++11 起范围 for：`for (auto& x : xs)` | 范围 for 不是标准库函数 |
| `while`、`do` | 98 | `while (condition) {}`、`do {} while (condition);` | 后者至少进入循环体一次 |
| `break` | 98 | 跳出最近一层循环或 `switch` | 不能一次跳出多层循环 |
| `continue` | 98 | 进入最近一层循环的下一轮处理 | 普通 for 中会先执行迭代表达式，再检查条件 |
| `goto` | 98 | 跳到当前函数的标签：`goto done;` | 不能跨函数跳转，也不能任意跳过变量初始化进入作用域 |
| `return` | 98 | 返回调用者：`return value;`、`return;` | 返回局部对象的指针或引用会留下悬空结果 |

规则见 [stmt](https://eel.is/c++draft/stmt)。`if constexpr` 的丢弃分支在模板实例化中有特殊规则，
不等同于预处理器的 `#if`：模板外的丢弃分支仍要通过语义检查。

### 13.5 对象创建、类型转换与类型查询

| 关键字 | 版本 | 用途与典型写法 | 注意点 |
| --- | --- | --- | --- |
| `new` | 98 | 创建对象：`new T(args)`、`new T[n]`；也有 placement new | new 表达式与只负责分配存储的 `operator new` 不等价 |
| `delete` | 98 | 销毁对象并释放对应存储：`delete p;`、`delete[] p;`；C++11 起可写 `= delete` | 数组/非数组形式必须匹配；删除函数仍参与重载决议 |
| `sizeof` | 98 | 对象/类型的字节数：`sizeof(T)`、`sizeof x`；C++11 起 `sizeof...(Ts)` | 操作数不求值；数组退化为指针后查询的是指针大小 |
| `alignof` | 11 | 查询类型对齐要求：`alignof(T)` | 不等于对象大小；标准写法的操作数是类型 |
| `alignas` | 11 | 指定对齐：`alignas(64) int counter;`、`struct alignas(16) X {};` | 不能降低类型本来要求的对齐；不是 `[[...]]` 属性 |
| `typeid` | 98 | 查询类型信息：`typeid(T)`、`typeid(expr)`，需包含 `<typeinfo>` | 多态类型的 glvalue 可得到动态类型；这种表达式操作数可能被求值 |
| `static_cast` | 98 | 明确执行允许的静态转换：`static_cast<double>(n)` | 不做运行时派生类型检查；向下转换需要调用者保证条件 |
| `dynamic_cast` | 98 | 类层次中的检查转换：`dynamic_cast<Derived*>(base)` | 运行时向下/交叉转换要求源类型多态；失败的指针转换返回空，引用转换抛 `std::bad_cast` |
| `const_cast` | 98 | 调整指针/引用涉及的 cv 限定 | 去掉 `const` 后修改原本就是 const 的对象是未定义行为 |
| `reinterpret_cast` | 98 | 低层指针/整数等转换 | 转换成功不等于可合法解引用，不自动满足别名、对齐或生命周期规则 |

规则见 [expr](https://eel.is/c++draft/expr) 和 [dcl.align](https://eel.is/c++draft/dcl.align)。
`std::move`、`std::forward`、`std::bit_cast` 都是库设施，不是这张表之外的关键字。

### 13.6 异常与断言

| 关键字 | 版本 | 用途与典型写法 | 注意点 |
| --- | --- | --- | --- |
| `try`、`catch` | 98 | `try { work(); } catch (const Error& e) { ... }`；还有函数 try 块 | `catch (...)` 可匹配任意异常；处理器按出现顺序匹配 |
| `throw` | 98 | `throw Error{};`；处理异常期间用 `throw;` 重新抛出 | 旧动态异常说明 `throw(T)` 在 C++17 移除；原 `throw()` 的空异常说明写法在 C++20 移除 |
| `noexcept` | 11 | 函数异常说明 `void f() noexcept;`、`noexcept(condition)`；查询表达式 `noexcept(expr)` | 查询不会执行 expr；异常逃出不抛出函数时调用 `std::terminate`，不是编译器保证函数体没有 throw |
| `static_assert` | 11 | 编译期断言：`static_assert(sizeof(int) >= 2, "int too small");` | C++17 起可省略消息；它是声明，可放在函数内或外；运行期 `assert` 是宏 |
| `contract_assert` | C++26 草案 | 契约断言语句：`contract_assert(x > 0);` | 不计入上面的 92 个；检查和违约处理取决于契约求值语义，不能当成 `static_assert` |

规则见 [except](https://eel.is/c++draft/except)、
[except.spec](https://eel.is/c++draft/except.spec)、[dcl.pre](https://eel.is/c++draft/dcl.pre) 和
[stmt.contract.assert](https://eel.is/c++draft/stmt.contract.assert)。

### 13.7 模板、命名空间、别名与模块导出

| 关键字 | 版本 | 用途与典型写法 | 注意点 |
| --- | --- | --- | --- |
| `template` | 98 | 声明模板；显式特化 `template<>`；显式实例化；依赖名消歧 `obj.template f<int>()` | 最后一种用于告诉解析器后面是模板名 |
| `typename` | 98 | 类型模板参数 `template<typename T>`；依赖类型名 `typename T::value_type` | 在类型模板参数位置可与 `class` 互换；不是所有依赖名位置都必须写 |
| `concept` | 20 | 定义命名约束：`template<class T> concept Small = sizeof(T) <= 8;` | 约束在编译期处理，不是运行时接口类 |
| `requires` | 20 | requires 子句约束声明；requires 表达式检查语法/类型等要求 | `requires requires(T x) { x + x; }` 中前后两个词承担不同角色 |
| `namespace` | 98 | 命名空间定义、别名、匿名命名空间 | 命名空间不是类型，不能实例化成对象 |
| `using` | 98 | using 声明、using 指令；C++11 起类型别名 `using Size = unsigned;`；C++20 起 `using enum` | `using Base::f;` 引入名字，`using namespace N;` 影响名字查找；不是复制声明的实现 |
| `typedef` | 98 | 类型别名：`typedef unsigned Size;` | 不创建新的独立类型；别名模板需用 `using` |
| `export` | 98；20 起新用途 | 模块接口导出：`export module math;`、`export int add(int, int);` | C++98 的模板导出用途在 C++11 移除，C++11～17 保留，C++20 用于模块 |

规则见 [temp](https://eel.is/c++draft/temp)、[namespace](https://eel.is/c++draft/namespace) 和
[module](https://eel.is/c++draft/module)。`module`、`import` 的特殊地位见第 13.10 节。

### 13.8 协程与汇编

| 关键字 | 版本 | 用途与典型写法 | 注意点 |
| --- | --- | --- | --- |
| `co_await` | 20 | `co_await awaitable`，按 awaiter 协议进行等待 | 不保证一定挂起，也不自动创建线程 |
| `co_yield` | 20 | `co_yield value`，经 promise 的 `yield_value` 提供值 | 常用于生成器，具体行为取决于协程协议 |
| `co_return` | 20 | `co_return value;` 或 `co_return;`，完成协程 | 使用 promise 的返回协议，不能简单当成普通函数 return |
| `asm` | 98 | 内联汇编声明：`asm("...");` | 条件支持；字符串含义由实现定义，GCC 的扩展 asm 语法不等于可移植标准语法 |

协程代码还需要返回类型、promise 和 awaiter 等配套定义；表中是语法片段，不是独立程序。
规则见 [dcl.fct.def.coroutine](https://eel.is/c++draft/dcl.fct.def.coroutine) 和
[dcl.asm](https://eel.is/c++draft/dcl.asm)。

### 13.9 11 个运算符替代拼写

这些拼写从 C++98 起就是保留词，不需要包含 `<iso646.h>` 或 `<ciso646>`，不能拿来做变量名。
它们保留原运算符的优先级和语义，例如内置逻辑 `and` / `or` 仍会短路。

| 拼写 | 等价符号 | 拼写 | 等价符号 |
| --- | --- | --- | --- |
| `and` | `&&` | `and_eq` | `&=` |
| `or` | `\|\|` | `or_eq` | `\|=` |
| `xor` | `^` | `xor_eq` | `^=` |
| `not` | `!` | `not_eq` | `!=` |
| `bitand` | `&` | `bitor` | `\|` |
| `compl` | `~` | — | — |

例如 `ready and not stopped` 等价于 `ready && !stopped`。
`<%`、`%>`、`<:`、`:>`、`%:`、`%:%:` 也是替代 token，但属于符号拼写，不额外算作单词关键字。
映射见 [lex.digraph](https://eel.is/c++draft/lex.digraph)。

### 13.10 特殊标识符，以及看起来像关键字的名字

| 名字 | 类别与版本 | 含义 |
| --- | --- | --- |
| `override` | C++11 特殊标识符 | 虚函数重写检查，只在指定成员声明位置具有该含义 |
| `final` | C++11 特殊标识符 | 禁止继续派生类或继续重写虚函数 |
| `module`、`import` | C++20 特殊标识符，在指定语境形成关键字 | 模块声明与导入，如 `export module math;`、`import math;`；不是一般的头文件文本替换 |
| `pre`、`post` | C++26 草案特殊标识符 | 函数契约前置/后置条件，例如 `int f(int x) pre(x > 0) post(r: r > 0);` |
| `include`、`define`、`ifdef`、`endif`、`pragma` 等 | 预处理指令名 | 出现在 `#include` 等指令里；没有 `#` 时不是因此就成为 C++ 关键字 |
| `defined`、`__has_include`、`__has_cpp_attribute` | 预处理条件中的特殊操作 | 分别查询宏是否定义、头文件可用性、属性支持情况；后两者分别自 C++17、C++20 标准化 |
| `nodiscard`、`maybe_unused`、`fallthrough`、`deprecated`、`likely`、`unlikely`、`no_unique_address`、`assume` | 标准属性名 | 用于 `[[...]]`，不因是标准属性就成为普通关键字；`assume` 自 C++23 引入 |
| `std`、`string`、`vector`、`size_t`、`move`、`forward` | 标准库名字 | 命名空间、类型、模板或函数名，使用时遵循头文件和名字查找规则 |
| `assert`、`NULL`、`offsetof` | 标准库宏 | 需要相应头文件；`assert` 与 `static_assert` 是不同机制 |
| `atomic_cancel`、`atomic_commit`、`atomic_noexcept`、`synchronized` | 事务内存 TS 的词 | 不属于这里统计的 ISO C++98～C++23 主标准关键字 |
| `reflexpr` | 反射 TS 的词 | 不在核对时的 C++26 工作草案关键字表中，不能照搬 TS 列表 |
| `__attribute__`、`__declspec`、`__int128`、`__restrict__` | 实现扩展的例子 | 支持和语义取决于编译器；C 的 `restrict` 也不是标准 C++ 关键字 |

因此 `int override = 1;` 可以合法声明普通变量，`int virtual = 1;` 则不行。
这不意味着所有含下划线的名字都可自用：包含双下划线、以下划线加大写字母开头等名字另有实现保留规则。

上述分类依据 [lex.name](https://eel.is/c++draft/lex.name)、
[cpp.pre](https://eel.is/c++draft/cpp.pre)、[dcl.attr.grammar](https://eel.is/c++draft/dcl.attr.grammar) 和
[dcl.contract](https://eel.is/c++draft/dcl.contract)。属性和预处理指令表仅用于区分类别，不是它们各自的完整清单。

### 13.11 按版本核对是否遗漏

| 版本 | 新增普通关键字或重要状态变化 |
| --- | --- |
| C++98/03 | 63 个普通关键字 + 11 个运算符替代拼写，共 74 个；C++03 未新增关键字 |
| C++11 | 新增 `alignas`、`alignof`、`char16_t`、`char32_t`、`constexpr`、`decltype`、`noexcept`、`nullptr`、`static_assert`、`thread_local`，共 84 个；另有 `override`、`final` 特殊标识符 |
| C++14 | 未新增关键字；扩展 `auto`、`constexpr` 等已有关键字的用法 |
| C++17 | 未新增关键字；`register` 用途移除但保留拼写；加入 `if constexpr`、内联变量等用法 |
| C++20 | 新增 `char8_t`、`concept`、`consteval`、`constinit`、`co_await`、`co_return`、`co_yield`、`requires`，共 92 个；另有模块语境的 `module`、`import`，并重新使用 `export` |
| C++23 | 未新增普通关键字；加入 `if consteval`、显式对象形参等用法 |
| C++26 工作草案 | 在上述 92 个之外新增 `contract_assert`；另有特殊标识符 `pre`、`post` |

这里所有总数都按“普通关键字 + 11 个替代拼写”计数，不计特殊标识符。
查资料时若数字不同，先检查版本，以及是否把 `module`、`import`、`override`、`final` 或 TS 扩展算了进去。

## 14. 函数左边和右边分别能放什么

不要把函数声明理解为“所有修饰词都能任意排列”。返回类型前后的声明说明符有一定排列自由，
但参数列表之后的 cv 限定、引用限定、异常说明、尾置返回类型等有自己的语法位置。

| 位置 | 常见内容 | 示例与限制 |
| --- | --- | --- |
| 声明开头/返回类型附近 | `static`、`extern`、`inline`、`virtual`、`constexpr`、`consteval`、`friend` | 按适用语境选择，不是任意组合；`static virtual` 不合法 |
| 构造函数/转换函数的声明前部 | `explicit`、`explicit(condition)` | 这两类函数本来就没有普通返回类型；不要把 explicit 加到一般业务函数上 |
| 返回类型内部 | `const`、`volatile`、`auto`、`decltype` 等 | `const T& f();` 限定返回引用所指对象，与成员函数尾部 const 不同 |
| 参数列表内部 | 参数类型限定、默认参数；C++23 的显式对象形参 `this X& self` | 显式对象形参必须是首个参数；这种成员函数不能再带尾部 cv/ref 限定，也不能是 static 或 virtual |
| 参数列表之后 | `const` / `volatile`，然后 `&` / `&&`，然后 `noexcept` | `int f() const & noexcept;`；引用限定符不是关键字 |
| 异常说明之后 | 尾置返回类型 `-> T` | `auto f() const noexcept -> int;`；前面使用 auto |
| 声明的约束位置 | `requires` 子句 | 可跟在模板参数列表后，也可作为函数的尾部约束；使用尾置返回类型时尾部 requires 在它之后 |
| 成员声明后部 | `override`、`final`，再到 `= 0` / `= default` / `= delete` 等结尾 | 只在各自允许的函数种类中使用，不是把所有结尾叠加；`= 0` 不是关键字 |
| lambda 参数列表之后 | `mutable` 等 lambda 说明符 | 这里 mutable 修饰闭包调用行为；普通成员函数不能写 `void f() mutable;` |

下面是可独立编译的 C++17 程序，用一组声明展示这些位置：

```cpp
#include <cassert>

struct Source {
    virtual auto read() const & noexcept -> int = 0;
    virtual ~Source() = default;
};

class Fixed final : public Source {
public:
    explicit Fixed(int value) noexcept : value_(value) {}

    auto read() const & noexcept -> int override final {
        return value_;
    }

    static constexpr int capacity() noexcept { return 8; }

private:
    int value_;
};

int main() {
    const Fixed object{3};
    const Source& source = object;
    assert(source.read() == 3);
    static_assert(Fixed::capacity() == 8);
    static_assert(noexcept(source.read()));
}
```

这里 `class Fixed final` 已禁止进一步派生，`read()` 上的 final 仅用于演示语法位置，实际可以省略。
尾部的 `const &` 约束调用对象，`noexcept` 约束异常是否允许逃出，`-> int` 指定返回类型，
`override final` 约束继承中的重写；它们各管不同的事情。
语法见 [dcl.fct](https://eel.is/c++draft/dcl.fct) 和 [class.mem.general](https://eel.is/c++draft/class.mem.general)。

## 15. const、constexpr、consteval、constinit 各管什么

| 写法 | 约束什么 | 是否允许之后修改 / 运行期执行 |
| --- | --- | --- |
| `const int x = runtime();` | 对象初始化后不可经普通赋值修改 | 初始化可以在运行期发生 |
| `constexpr int x = 3;` | 变量可用于常量表达式，需要满足常量初始化等要求 | 变量本身是 const，不能之后赋值 |
| `constexpr int f(int x)` | 函数允许参与常量求值 | 合适的调用可在常量求值时执行，也可在运行期执行 |
| `consteval int f(int x)` | 声明立即函数 | 普通语境中的立即调用必须得到常量表达式，不能靠运行期参数计算 |
| `constinit int x = 3;` | 静态/线程存储期变量必须静态初始化 | x 可修改；如果还需要只读，可以另加 const |

可独立编译的 C++20 示例：

```cpp
#include <cassert>

constexpr int twice(int value) { return value * 2; }
consteval int initial_limit() { return 8; }

constinit int limit = initial_limit(); // 静态初始化，但 limit 不是 const
constexpr int fixed_limit = initial_limit();

int main() {
    int runtime_value = limit;
    const int snapshot = twice(runtime_value); // 合法：constexpr 函数也能运行期调用
    static_assert(twice(fixed_limit) == 16);
    ++limit;                                  // 合法：constinit 没有禁止修改
    assert(snapshot == 16 && limit == 9);

    // constexpr int bad = runtime_value;      // 错误：不是常量表达式
    // constinit int local = 0;                // 错误：普通自动存储期局部变量
}
```

同一个声明不能同时写 `constexpr`、`consteval`、`constinit` 中的两个。
`constinit const` 合法；类的 `constexpr` 静态数据成员自 C++17 起隐式 inline，
普通命名空间作用域的 constexpr 变量则不能据此一概认为 inline。

补充验证记录：已逐项对照 C++23 N4950 和当前工作草案的 `lex.key` 表，分别覆盖全部
92 / 93 个拼写。第 14、15 节的完整程序分别以 C++17、C++20，使用 GCC 和 Clang 加
`-Wall -Wextra -pedantic-errors` 编译并运行通过；Clang 显式使用本机 GCC 的标准库头文件和库路径。
表格中的语法片段及 C++26 草案示意不属于这两项编译验证。

## human
2. 显然，构造函数是可以不用是 virtual 的，因为在创建一个对象的时候，需要明确知道当前的对象类型到底是什么
是 virtual 没有什么意义。

3. 那些是放到函数的左边的，那些是放到函数右边的
3. 成员函数和 inline 的关系

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
