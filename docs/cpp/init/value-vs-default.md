# 值初始化为什么存在，以及它和默认初始化的关系

本文以 C++20 为基准。除特别说明外，示例中的变量都是函数内的普通局部变量。
文中说某个 `int` 没有初始化数值时，不要通过打印它来验证结果。
使用 `std::string` 的例子需要包含 `<string>`。

**value-initialization（值初始化）是一套特定的规则，不是“给变量一个初始值”的统称。**
default-initialization（默认初始化）和它也不是覆盖所有初始化情况的两个大类。

## 0. c++ 的高性能默认要求

首先，只要没有，就不会初始化的

利用 value initialization ，就可以显示的说明，哪些成员是需要默认清零的。

其实 value initialization 反而奇怪

## 1. 为什么需要值初始化

先看没有写初始化器时的行为：

```cpp
void demo() {
    int n;          // 默认初始化：不初始化这个 int 的数值
    std::string s;  // 默认初始化：调用默认构造函数，得到空字符串
}
```

两句都采用默认初始化，但是结果依赖类型：类可以通过构造函数建立初始状态，
普通局部 `int` 则没有这样的动作。

这给泛型代码提出一个需求：**不知道 T 是整数、指针还是类时，能否用同一种写法，
让整数得到零、指针得到空指针，同时让类执行自己的默认构造逻辑？**

`T value;` 不能满足这个需求；`T value = 0;` 也不通用，因为类不一定接受 `0`。
值初始化提供了相应规则，典型入口是表达式 `T()`：

```cpp
template<class T>
T make_initial_value() {
    return T();  // 对这里的目标类型执行值初始化
}

void demo() {
    auto n = make_initial_value<int>();          // 0
    auto p = make_initial_value<int*>();         // 空指针
    auto s = make_initial_value<std::string>();  // 空字符串
}
```

这个接口仍要求 `T()` 合法，例如类必须有可用的默认构造函数。
它也不承诺任意类的所有成员都有确定值：用户提供的构造函数仍然要负责自己的成员。
规则定义见标准草案的 [初始化通则](https://eel.is/c++draft/dcl.init.general)。

### 历史上具体修复了什么问题

[CWG 35](https://cplusplus.github.io/CWG/issues/35.html) 记录了旧规则中的不一致：

```cpp
struct Numbers {
    int x;
    double y;
};

struct Record {
    int x;
    double y;
    std::string name;
};
```

当时 `Numbers()` 保证数值成员为零；但加入 `std::string` 成员、成为非 POD 类型后，
`Record()` 虽然会构造字符串，却不再保证旁边的 `x`、`y` 为零。
这里讲的是旧规则的问题，不是当前 C++ 的行为。

[CWG 178](https://cplusplus.github.io/CWG/issues/178.html) 通过独立的值初始化规则处理了这个问题，
并把规则用于 `T()`、`new T()`、构造函数成员初始化器中的 `member()` 等位置。
在当前规则下，上述两个类型的 `T()` 都会让数值成员得到零，字符串正常构造。

为什么没有直接让默认初始化总是清零？CWG 35 明确指出，局部对象写成 `T obj;`
时允许数值成员不被初始化，是有意保留的行为。从使用上看，这也保留了“随后直接覆盖数据，
无需先要求清零”的选择，例如函数内的 `int buffer[4096];`。

## 2. 它在初始化体系中的位置

cppreference 的 [Initializers 章节](https://en.cppreference.com/cpp/language/initialization#Initializers)
列出的四种形式，分类的是**声明中的初始化器语法**：

| 语境 | 不使用列表 | 使用列表 |
| --- | --- | --- |
| Copy | `T x = expr;` | `T x = {args...};` |
| Direct | `T x(args...);` | `T x{args...};` |

圆括号一格指参数列表非空的对象定义；`T x();` 是函数声明。
**这四格不属于 value-initialization 的四个子类。**
有没有初始化器、使用哪种语法、目标是什么类型，共同决定应用哪些规则。

```cpp
void demo() {
    int a;         // 默认初始化
    int b = 42;    // 拷贝初始化，不涉及值初始化
    int c(42);     // 直接初始化，不涉及值初始化
    int d = {42};  // 拷贝列表初始化，不涉及值初始化
    int e{42};     // 直接列表初始化，不涉及值初始化
    int f{};       // 直接列表初始化 → 值初始化 → 零初始化
    int g = {};    // 拷贝列表初始化 → 值初始化 → 零初始化
}
```

`b` 到 `e` 都有初始值 42，但它们都没有进行值初始化。
`f` 则可以同时从不同层次描述：使用直接列表初始化，空列表在这里触发值初始化，
而对 `int` 的值初始化要求零初始化。

箭头表示规则的衔接，不表示创建了多个对象，也不表示执行了多次赋值。
具体分支见标准草案的 [列表初始化](https://eel.is/c++draft/dcl.init.list)。

同一句代码的不同部分也可以涉及不同规则：

```cpp
int b = int();
```

右侧的 `int()` 执行值初始化，得到 0；变量 `b` 从这个表达式进行拷贝初始化。
不能只看整句出现了 `=` 或 `()`，就给所有部分贴同一个标签。

## 3. 默认初始化和值初始化分别做什么

下面概括本文涉及的类型；默认构造函数不可用等情况会使程序不合法。

| 目标类型 | 默认初始化 | 值初始化 |
| --- | --- | --- |
| `int`、`double`、指针等标量 | 不执行数值初始化 | 零初始化：整数为 0、浮点数为 0.0、指针为空指针 |
| 数组 | 各元素默认初始化 | 各元素值初始化 |
| 类 | 选择并调用默认构造函数 | 根据选中的默认构造函数是否为 user-provided，决定是否先零初始化；随后默认初始化 |

因此它们不是互斥的两条完整路径：**类的值初始化会进一步执行默认初始化。**
“默认初始化”也不代表一定获得一个可读取的默认数值。

零初始化是语言规定的操作，不等同于 `memset`：例如指针得到空指针值，
并不要求该值的存储表示一定是所有位为零。

## 4. 怎样判断 `()` 和 `{}`

先区分代码所处的位置，再判断目标类型。

| 代码 | 含义 |
| --- | --- |
| `int x;` | 对象定义，默认初始化 |
| `int x();` | 函数声明，没有创建 `int` 对象 |
| `int()` | 表达式，值初始化，得到 0 |
| `int x{};` | 对象定义，列表初始化进一步触发值初始化，得到 0 |
| `new int` | 创建一个默认初始化的 `int` |
| `new int()` | 创建一个值初始化的 `int`，得到 0 |
| `new int{}` | 列表初始化进一步触发值初始化，得到 0 |
| `new int(7)` | 直接初始化，得到 7 |
| `new int{7}` | 直接列表初始化，得到 7 |

表中的 `new` 形式用于对比规则；实际使用时需要管理返回的指针并释放对象。
`new` 改变对象存储的取得方式，不会让所有初始化都变成值初始化。

### 空花括号要继续看类型

```cpp
struct Point {
    int x;
    int y;
};

void demo() {
    int n{};            // 列表初始化 → 值初始化 → 零初始化
    std::string s{};    // 列表初始化 → 值初始化 → 默认初始化，调用默认构造函数
    Point p{};          // 列表初始化 → 聚合初始化
}
```

`p` 的两个成员也都得到 0，但 `p` 本身采用聚合初始化。
这里没有给出成员的初始化器，成员也没有类内默认值，因此按空初始化列表初始化各成员，
再对这两个 `int` 应用值初始化和零初始化。

对于空列表，判断常见类型时可以按这个顺序理解：

1. 聚合类型：应用聚合初始化。
2. 非聚合类且有默认构造函数：应用值初始化。
3. 非聚合类没有默认构造函数：继续检查列表构造函数等规则，也可能报错。
4. 标量：应用值初始化。

这份顺序只用于上面这些常见对象类型，完整规则还包括引用等情况。
**不能把“写了 `{}`”“值初始化”“最终数值为零”当成同一件事。**
规则见 [列表初始化](https://eel.is/c++draft/dcl.init.list) 和
[聚合初始化](https://eel.is/c++draft/dcl.init.aggr)。

## 5. 类的边界：为什么有 user-provided 这条规则

对类的值初始化，可以先用下面的思路理解：如果类作者提供了默认构造函数，
就由该构造函数负责建立初始状态；否则先进行零初始化，再执行默认构造逻辑。
但这只是帮助理解的说法，精确边界由 **user-provided** 这个标准术语决定。

```cpp
struct A {
    int x;
    A() = default;
};

struct B {
    int x;
    B() {}
};

void demo() {
    A a;    // 默认初始化：a.x 没有初始化数值
    A av{}; // C++20 中 A 不是聚合体：值初始化先零初始化，av.x == 0

    B b;    // 默认初始化：b.x 没有初始化数值
    B bv{}; // 值初始化调用 B()，不预先零初始化；bv.x 没有初始化数值
}
```

`A() = default` 写在首次声明处，不属于 user-provided；`B() {}` 属于 user-provided。
**`= default` 与空函数体不等价。** 空函数体只是函数体里没有语句，
不会替作者补上 `x` 的数值初始化。

还有一种必须单独记住的情况：

```cpp
struct C {
    int x;
    C();
};

C::C() = default;  // 首次声明之后才 default：属于 user-provided
```

函数内的 `C c{};` 不会因为值初始化而先把 `x` 置零。
判断依据不是肉眼看到有没有函数体，而是构造函数是否符合标准的定义。

| 默认构造函数的来源或写法 | 是否 user-provided |
| --- | --- |
| 编译器隐式声明 | 否 |
| 首次声明处 `A() = default;` | 否 |
| `B() {}` | 是 |
| 先声明 `C();`，再定义 `C::C() = default;` | 是 |

定义见标准草案的 [Explicitly-defaulted functions](https://eel.is/c++draft/dcl.fct.def.default)。

### 写业务类型时，可以减少对这条差异的依赖

如果成员应该有确定初值，直接把要求写在成员或构造函数中：

```cpp
struct Settings {
    int retries = 3;
    int timeout{};

    Settings() {}
};

void demo() {
    Settings a;    // retries == 3，timeout == 0
    Settings b{};  // retries == 3，timeout == 0
}
```

这样即使默认构造函数是 user-provided，成员的初始状态仍然明确。
值初始化是否额外进行零初始化，不再决定这两个成员的最终值。

## 6. 存储期是另一个维度

```cpp
int global;  // 静态存储期，零初始化后为 0

void demo() {
    static int s;  // 静态存储期，零初始化后为 0
    int local;     // 普通局部变量，没有初始化数值
}
```

三处都没有显式初始化器，但前两者还有静态初始化规则。
不能因为全局 `int` 默认为零，就认为默认初始化会清零；
也不能因为最终得到零，就反推发生了值初始化。
参见标准草案的 [Static initialization](https://eel.is/c++draft/basic.start.static)。

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
