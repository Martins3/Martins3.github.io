# C++ 初始化梳理

C++ 初始化规则多，
1. 可以从它兼容 C、
2. C++ 引入构造函数
3. 再推广 list-initialization

## 2. C++ 有了构造函数：direct-initialization 与 copy-initialization

类可以通过构造函数建立对象的初始状态，参数不必与成员一一对应：

```cpp
class Rectangle {
public:
    Rectangle(int width, int height) : area_(width * height) {}

private:
    int area_;
};

Rectangle r(3, 4); // 两个构造参数，最后初始化的是一个面积成员
```

`T x(args);` 表达“直接使用这些参数初始化 T”，称为 direct-initialization。
C 风格的 `T x = value;` 则保留下来，在 C++ 中称为 copy-initialization。
copy 是规则名称的一部分，不承诺调用拷贝构造函数，也不意味着先构造再赋值。


为什么要保留这一区分？类的作者可能允许调用者明确构造对象，却不希望其他类型被隐式接受。
`explicit` 就用来表达这样的接口约束：

```cpp
struct Size {
    explicit Size(int n) : value(n) {}
    int value;
};

void resize(Size size);

Size a(10);        // OK：direct-initialization 允许使用 explicit 构造函数
// Size b = 10;    // 错误：copy-initialization 不能使用这个 explicit 构造函数
// resize(10);     // 错误：不能隐式地把 int 转换为 Size
resize(Size(10));  // OK：明确构造 Size 后传参
```

这里 `resize(10)` 才有明确的“源类型 int → 目标类型 Size”。
direct-initialization 与 copy-initialization 的区别也适用于函数调用等语境，不只是变量定义处有没有 `=`。

## 3. C++11 推广花括号：list-initialization

原有语法对不同类型并不统一：数组、聚合体可以使用花括号，而普通类通过构造函数接收参数。
C++11 将花括号初始化推广到更多类型，并加入禁止窄化转换等规则：

```cpp
int n{10};
int values[]{1, 2, 3};
Rectangle r{3, 4};
std::vector<int> v{1, 2, 3}; // 需要包含 <vector>

int truncated(3.14);       // 可以编译，得到 3
// int rejected{3.14};     // 错误：list-initialization 禁止这种窄化转换
```

不过，统一写法不代表所有类型执行相同操作。聚合体可以按成员初始化，普通类要选择构造函数，
容器还可以通过 `std::initializer_list` 构造函数接收一组元素：

```cpp
std::vector<int> a(3, 9); // 三个元素：9, 9, 9
std::vector<int> b{3, 9}; // 两个元素：3, 9
```

这两种需求都存在，因此花括号不能机械地替换圆括号。

list-initialization 还延续了 direct-initialization 与 copy-initialization 的区别：

```cpp
Size a{10};       // direct-list-initialization：允许 explicit 构造函数
// Size b = {10}; // copy-list-initialization：选中 explicit 构造函数会报错
```

这里的规则可以理解为：新加入的列表写法，也保留“是否允许使用 explicit 构造函数”的接口约束。
标准明确规定了这一区别，见 [list-initialization 的构造函数选择规则](https://eel.is/c++draft/over.match.list)。

## 4. 四种 initializer 语法及其语义

[cppreference 的 Initializers 章节](https://cppreference.com/cpp/language/initialization#Initializers)
将声明中的 initializer 列为四种语法形式。下面按原页面的顺序列出，以 C++20 为基准；
花括号中的列表可以为空，也可以使用 C++20 的 designated initializer。

Initializers 一共有四种:

| 编号 | initializer 形式 | 变量定义示例 | 对应规则 |
| ---- | ---------------- | ------------ | -------- |
| (1) | `= expression` | `T x = value;` | copy-initialization |
| (2) | `= {...}` | `T x = {1, 2};`、`T x = {};` | list-initialization，具体为 copy-list-initialization |
| (3) | `(...)` | `T x(arg);`、`T x(arg1, arg2);` | direct-initialization；此处参数列表非空 |
| (4) | `{...}` | `T x{1, 2};`、`T x{};` | list-initialization，具体为 direct-list-initialization |

`T x;` 没有 initializer，所以不在这四种语法形式里；对于对象，这种情况执行
default-initialization。

  可以把表中的四种形式重新排成二维表：

```txt
                  不使用列表       使用列表
  ━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━
   Copy 语境      T x = expr;      T x = {args...};
  ─────────────  ───────────────  ──────────────────
   Direct 语境    T x(args...);    T x{args...};
```
很容易理解，那么 copy 就是有等于号，direct 就是没有等于号。



`T x{};` 则已经包含在第 (4) 种里，不需要再单列一类。

这四种是 initializer 的语法形式，不代表 C++ 只有四种 initialization 语义。
default-initialization、value-initialization、aggregate initialization 等描述的是如何初始化对象。
例如 `Point p = {1, 2};` 使用第 (2) 种语法，属于 copy-list-initialization；
如果 `Point` 是这里的双成员聚合体，实际执行 aggregate initialization。

direct-list-initialization 和 copy-list-initialization 是 list-initialization 在两种语境中的名称，
不是额外增加的两种 initializer 语法。前者允许 explicit 构造函数，后者若最终选中 explicit
构造函数则程序不合法。名称中的 copy 也不保证调用拷贝构造函数。

阅读代码时依次问：有没有初始化器？属于 direct-initialization 还是 copy-initialization 语境？是否使用花括号？
目标类型是标量、数组、聚合体还是其他类？这些答案共同决定结果。
术语总览可查 [cppreference：Initialization](https://cppreference.com/cpp/language/initialization)。


## 5. 标量类型

局部自动变量没有初始化时，不要读取其值：

```cpp
int a; // 值不确定，读取它是错误
int b = 0;
int c(0);
int d{0};
int e{}; // 0
```

没有显式初始化器的全局 `int` 和局部 `static int` 会得到零值；这里需要警惕的是没有初始化器的
局部自动变量。更一般的静态存储期对象可能执行 constant initialization，不能一概说成“先清零再执行初始化器”。

花括号会拒绝窄化转换：

```cpp
int a(3.14); // 可以编译，但丢失小数部分
// int b{3.14}; // 错误：list-initialization 禁止 double 到 int 的窄化
```

因此，对于标量，默认优先写显式值或 `{}`，不要留下未初始化变量。

## 6. 普通类对象

```cpp
class Widget {
public:
    Widget();
    explicit Widget(int size);
    Widget(int width, int height);
};

Widget a; // default-initialization，调用 Widget()
Widget b{}; // 调用 Widget()
Widget c(10); // 调用 Widget(int)
Widget d{10}; // 调用 Widget(int)
Widget e(10, 20); // 调用 Widget(int, int)
Widget f{10, 20}; // 调用 Widget(int, int)

// Widget g = 10; // 错误：copy-initialization 不能使用 explicit 构造函数
```

对类来说，`{}` 不等于“直接填写成员”。对这里的非聚合类 `Widget`，需要选择构造函数。

### initializer_list 的优先级

// TODO 这里需要一个 DEMO 来体现优先级的差别
// 似乎我可以理解这个问题，因为都是为 {}

如果非聚合类提供 `std::initializer_list` 构造函数，非空花括号列表会优先考虑它。
空列表且类有默认构造函数时，则执行 value-initialization。最典型的非空列表例子是
`std::vector`：

```cpp
std::vector<int> a(3, 9); // 三个元素：9, 9, 9
std::vector<int> b{3, 9}; // 两个元素：3, 9
```

所以不能机械地把所有 `()` 都替换成
`{}`。花括号通常更安全，但必须先理解类接口对花括号的定义。

另外，`{1, 2, 3}` 本身不是表达式，也没有类型；具体上下文决定如何使用它，
并不总会创建 `std::initializer_list` 对象。

## 7. 结构体与聚合体

`struct` 和 `class` 在初始化机制上没有本质区别，默认访问权限不同。
通常把没有用户声明构造函数、没有虚函数、数据成员可供 aggregate initialization 的一类简单数据类型称为
aggregate（聚合体）；精确条件随 C++ 标准版本有变化。

```cpp
struct Point {
    int x;
    int y = 7;
};

Point a{}; // x = 0，y 使用类内默认值 7
Point b{1}; // x = 1，y = 7
Point c{1, 2}; // x = 1，y = 2
Point d{.x = 3, .y = 4}; // C++20 指定成员初始化
Point e(5, 6); // C++20 聚合体圆括号初始化
```

`Point p;` 不保证 `p.x`
为零。即使编译器为类隐式生成了默认构造函数，也不代表其中的标量成员会自动清零；`Point p{};`
才能得到上面展示的确定结果。

aggregate initialization 按成员的声明顺序进行。
C++20 指定初始化器也必须保持声明顺序，不能像 C 那样任意调整：

```cpp
// Point bad{.y = 4, .x = 3}; // C++ 中顺序错误
```

如果后来给 `Point` 增加用户声明的构造函数，`Point{...}`
就会改为选择构造函数，不再是按位置 direct-initialization 成员。这是判断 `struct`
初始化行为时最需要先确认的事情。

通常优先使用聚合体的 `{}` 写法。相比 C++20 的
`()`，它更直观、支持指定初始化器，并拒绝窄化转换。

### b.cpp：花括号没有源类型，为什么 explicit 仍然有限制？

[`b.cpp`](b.cpp) 中，注释掉 `B(int, int)` 时，`B` 是聚合体：

```cpp
struct B {
    int a;
    int b;
};

B e = {1, 2};
B f = {1, 2};
```

两句都是 copy-list-initialization，实际执行 aggregate initialization：用 `1` 初始化 `a`，用 `2` 初始化 `b`。
`{1, 2}` 不是表达式，没有自己的类型，也不是一个 `std::initializer_list<int>` 对象。
因此这里不存在一个“源类型 X 的对象转换成 B”的过程。
标准在 [初始化的一般规则](https://eel.is/c++draft/dcl.init.general) 中也明确区分了有无源类型的情况。

对于 `void test_B_implicit(B b)`，调用 `test_B_implicit({1, 2})` 时，目标类型来自形参。
形参同样通过 copy-list-initialization 得到。在重载决议中，标准用专门的
[list-initialization sequence](https://eel.is/c++draft/over.ics.list) 规则处理这个实参，不能据此认为列表本身有类型。

如果改成下面的定义，就发生了两件事：`B` 不再是聚合体，初始化要选择构造函数；
而 copy-list-initialization 一旦选中 `explicit` 构造函数，就不合法。

```cpp
struct B {
    int a;
    int b;
    explicit B(int a, int b) : a(a), b(b) {}
};

B x(1, 2);                      // OK：direct-initialization
B y{1, 2};                      // OK：direct-list-initialization
// B e = {1, 2};                // 错误：copy-list-initialization 选中 explicit 构造函数
// test_B_implicit({1, 2});     // 错误：形参的 copy-list-initialization，同样受限
B z = B{1, 2};                  // OK：右侧 direct-list-initialization B；C++17 起这里不需要拷贝/移动
test_B_implicit(B{1, 2});        // OK：明确构造 B 后传参
```

注意，这不是因为花括号突然有了源类型，而是 copy-list-initialization 本身就受 `explicit` 规则约束。
更精确地说，这种初始化会考虑 `explicit` 构造函数，选中后再报错；普通 copy-initialization 则不把它作为可用的转换构造函数。

去掉 `explicit`、保留双参数构造函数后，`B e = {1, 2};` 可以通过，但仍然调用构造函数，
不会恢复为 aggregate initialization。原注释里的空函数体 `B(int a, int b) {}` 也不会自动初始化同名成员，
所以这里特意补了成员初始化列表。

声明双参数构造函数还会阻止编译器隐式声明默认构造函数，所以 `b.cpp` 中的
`std::make_shared<B>()` 会因缺少无参构造函数而报错。这和 `explicit` 的限制是两个问题。


## 8. default-initialization、value-initialization 与 new

default-initialization 不保证零值；value-initialization 对于标量会得到零值。`new` 负责取得存储并创建对象，
对象如何初始化仍然由初始化器决定：

```cpp
// TODO 我就说，仅仅四种初始化方法是不够的，为什么 cpp reference 没有提到这个操作?
int* a = new int;    // default-initialization：没有确定的初值
int* b = new int();  // value-initialization：0
int* c = new int{};  // list-initialization（空列表），最终 value-initialization：0
int* d = new int(7); // direct-initialization：7
int* e = new int{7}; // direct-list-initialization：7
```

不能因此推断“任何类加上 `{}` 都能把成员清零”：

```cpp
struct Defaulted {
    int n;
    Defaulted() = default; // 在首次声明处 default，不是用户提供的构造函数
};

struct Custom {
    int n;
    Custom() {}           // 用户提供的构造函数，没有初始化 n
};

Defaulted a;   // default-initialization，a.n 没有确定的初值
Defaulted b{}; // C++20 中此类不是聚合体；value-initialization 先 zero-initialization，b.n 为 0
Custom c{};    // value-initialization 调用用户提供的构造函数，c.n 仍没有确定的初值
```

类的 value-initialization 是否先做 zero-initialization，取决于选中的默认构造函数是否为用户提供的构造函数；
随后再执行 default-initialization。不要把“用户声明”和“用户提供”混为一谈。
相关规则见 [default-initialization 与 value-initialization](https://eel.is/c++draft/dcl.init.general)。

表达式 `T()` 可以用于 value-initialization，但变量声明 `T x();` 是函数声明。
因此创建局部对象时通常写 `T x{};`，再根据 `T` 是否为聚合体等条件判断最终行为。

## 9. 数组

### 原生数组

```cpp
int a[3]; // 函数内的三个元素值不确定
int b[3]{}; // 0, 0, 0
int c[]{1, 2, 3}; // 编译器推导长度为 3
int d[5]{1, 2}; // 1, 2, 0, 0, 0
std::string names[3]; // 每个元素都调用 std::string 默认构造函数
```

对象数组会逐个初始化元素。没有初始化器时，各元素 default-initialization；提供花括号列表但省略部分元素时，
剩余元素按空初始化列表进行初始化，并不等同于对它们执行 default-initialization：

```cpp
Widget widgets[3]; // 对三个元素分别调用 Widget()
Widget selected[]{Widget{1}, Widget{2}}; // 长度为 2，逐项初始化
```

原生数组不能整体拷贝或赋值：

```cpp
int source[3]{1, 2, 3};
int target[3]{};

// int copy[3] = source; // 错误
// target = source; // 错误
```

需要值语义时，优先使用 `std::array`：

```cpp
std::array<int, 3> source{1, 2, 3};
std::array<int, 3> copy = source; // 可以整体拷贝
copy = source; // 也可以整体赋值
```

长度在运行时确定时使用 `std::vector`。除非确实需要手动管理内存，不要优先选择
`new[]`：

```cpp
int* a = new int[3]; // 元素值不确定
int* b = new int[3]{}; // 0, 0, 0

delete[] a;
delete[] b;
```

## 10. 拷贝、移动与赋值

设 `source` 已经存在，`T` 是提供通常的、可访问且未删除的拷贝/移动操作的类，
`source` 是非 const 左值，且没有其他构造函数改变重载选择：

| 写法                       | 对象此前是否存在 | 调用           |
| -------------------------- | ---------------: | -------------- |
| `T a(source);`             |               否 | 拷贝构造函数   |
| `T a = source;`            |               否 | 拷贝构造函数   |
| `T a(std::move(source));`  |               否 | 移动构造函数   |
| `T a = std::move(source);` |               否 | 移动构造函数   |
| `a = source;`              |               是 | 拷贝赋值运算符 |
| `a = std::move(source);`   |               是 | 移动赋值运算符 |

`std::move`
本身不移动任何数据。它把表达式转换成允许选择移动操作的右值类别；真正转移资源的是移动构造函数或移动赋值运算符。
如果类型没有可用的移动重载，某些右值用法也可以由接受 `const T&` 的拷贝操作处理，
所以看到 `std::move` 并不保证发生移动。

从函数按值返回时还可能发生 RVO/NRVO：对象直接在最终位置构造，连 move
都不会调用。详细示例见 [`../move/rvo.md`](../move/rvo.md)。

## 11. 成员初始化

成员初始化列表发生在进入构造函数体之前：

```cpp
class User {
public:
    User(int id, std::string name)
        : id_(id), name_(std::move(name))
    {
    }

private:
    int id_;
    std::string name_;
};
```

如果在构造函数体内写 `name_ = name`，`name_`
会先完成 default-initialization，然后再执行赋值。`const`
成员、引用成员和没有默认构造函数的成员必须通过类内默认值或成员初始化列表完成初始化。

成员真正的初始化顺序永远是声明顺序，不是成员初始化列表的书写顺序。因此初始化列表也应按声明顺序写。

## 12. auto 的花括号规则

`auto` 有额外规则：

```cpp
auto a{1}; // int
auto b = {1}; // std::initializer_list<int>
auto c = {1, 2, 3}; // std::initializer_list<int>
// auto d{1, 2}; // 错误：direct-list-initialization auto 只能有一个元素
```
TODO 为什么存在这条补充规则啊?

如果推导出的类型很重要，不要依赖容易误读的写法，直接写明类型。

## 13. 实用选择顺序

2. 标量和简单聚合数据优先使用 `{}`，避免未初始化和窄化转换。
3. 类对象先理解构造函数接口；存在 `initializer_list` 时特别比较 `()` 与 `{}` 的语义。
4. 聚合结构体优先写 `Point{...}`，C++20 可以使用指定初始化器增强可读性。

- [Initialization](https://en.cppreference.com/w/cpp/language/initialization)
- [List initialization](https://en.cppreference.com/w/cpp/language/list_initialization)
- [Aggregate initialization](https://en.cppreference.com/w/cpp/language/aggregate_initialization)
- [Default initialization](https://en.cppreference.com/w/cpp/language/default_initialization)
- [Value initialization](https://en.cppreference.com/w/cpp/language/value_initialization)

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
