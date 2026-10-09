# C++ 初始化梳理

C++ 初始化容易混乱，是因为三件事叠在了一起：

1. 写法不同：`()`、`{}`、`=`。
2. 类型不同：标量、类、聚合体、数组各有规则。
3. 同一种写法可能调用普通构造、拷贝构造、移动构造，或者执行聚合初始化。

## 1. 常见写法总表

| 写法            | 名称           | 常见效果                                                     |
| --------------- | -------------- | ------------------------------------------------------------ |
| `T x;`          | 默认初始化     | 类调用默认构造；函数内的内置标量通常没有初始化               |
| `T x(args);`    | 直接初始化     | 为类选择构造函数；C++20 也支持聚合体圆括号初始化             |
| `T x = value;`  | 拷贝初始化     | 初始化新对象，不是先构造再赋值；不能使用 `explicit` 构造函数 |
| `T x{args};`    | 直接列表初始化 | 类选择构造函数，聚合体按成员初始化；禁止窄化转换             |
| `T x = {args};` | 拷贝列表初始化 | 同样是初始化；若最终选中 `explicit` 构造函数则程序不合法     |
| `T x{};`        | 空列表初始化   | 标量得到零值，聚合体补齐成员，类通常调用默认构造函数         |

这里的“直接”“拷贝”描述的是初始化语法，不保证真的发生一次
copy。编译器可能直接调用匹配的构造函数，也可能进行拷贝省略。

不要写：

```cpp
Widget widget();
```

这通常被解析为名叫 `widget`、没有参数、返回 `Widget` 的函数声明，即 most vexing
parse（最令人烦恼的解析）。 创建默认对象应写：

```cpp
Widget widget;
Widget another{};
```

## 2. 标量类型

局部自动变量没有初始化时，不要读取其值：

```cpp
int a; // 值不确定，读取它是错误
int b = 0;
int c(0);
int d{0};
int e{}; // 0
```

全局变量和 `static`
变量会先进行零初始化；这里需要警惕的是没有初始化器的局部自动变量。

花括号会拒绝窄化转换：

```cpp
int a(3.14); // 可以编译，但丢失小数部分
// int b{3.14}; // 错误：列表初始化禁止 double 到 int 的窄化
```

因此，对于标量，默认优先写显式值或 `{}`，不要留下未初始化变量。

## 3. 普通类对象

```cpp
class Widget {
public:
    Widget();
    explicit Widget(int size);
    Widget(int width, int height);
};

Widget a; // 默认初始化，调用 Widget()
Widget b{}; // 调用 Widget()
Widget c(10); // 调用 Widget(int)
Widget d{10}; // 调用 Widget(int)
Widget e(10, 20); // 调用 Widget(int, int)
Widget f{10, 20}; // 调用 Widget(int, int)

// Widget g = 10; // 错误：拷贝初始化不能使用 explicit 构造函数
```

对类来说，`{}` 不等于“直接填写成员”。如果类型有构造函数，它会参与重载决议。

### initializer_list 的优先级

如果类提供 `std::initializer_list` 构造函数，花括号会优先考虑它。最典型的例子是
`std::vector`：

```cpp
std::vector<int> a(3, 9); // 三个元素：9, 9, 9
std::vector<int> b{3, 9}; // 两个元素：3, 9
```

所以不能机械地把所有 `()` 都替换成
`{}`。花括号通常更安全，但必须先理解类接口对花括号的定义。

另外，`{1, 2, 3}` 本身没有普通表达式类型；
只有在具体上下文中，它才会触发列表初始化。 它也不总是 `std::initializer_list`。

## 4. 结构体与聚合体

`struct` 和 `class` 在初始化机制上没有本质区别，默认访问权限不同。
通常把没有用户声明构造函数、没有虚函数、数据成员可供聚合初始化的一类简单数据类型称为
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

聚合初始化按成员的声明顺序进行。C++20 指定初始化器也必须保持声明顺序，不能像 C
那样任意调整：

```cpp
// Point bad{.y = 4, .x = 3}; // C++ 中顺序错误
```

如果后来给 `Point` 增加用户声明的构造函数，`Point{...}`
就会改为选择构造函数，不再是按位置直接初始化成员。这是判断 `struct`
初始化行为时最需要先确认的事情。

通常优先使用聚合体的 `{}` 写法。相比 C++20 的
`()`，它更直观、支持指定初始化器，并拒绝窄化转换。

## 5. 数组

### 原生数组

```cpp
int a[3]; // 函数内的三个元素值不确定
int b[3]{}; // 0, 0, 0
int c[]{1, 2, 3}; // 编译器推导长度为 3
int d[5]{1, 2}; // 1, 2, 0, 0, 0
std::string names[3]; // 每个元素都调用 std::string 默认构造函数
```

对象数组会逐个初始化元素。没有为所有元素提供初始化器时，剩余元素必须能够默认初始化：

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

## 6. 拷贝、移动与赋值

设 `source` 已经存在：

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

从函数按值返回时还可能发生 RVO/NRVO：对象直接在最终位置构造，连 move
都不会调用。详细示例见 [`../move/rvo.md`](../move/rvo.md)。

## 7. 成员初始化

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
会先完成默认初始化，然后再执行赋值。`const`
成员、引用成员和没有默认构造函数的成员必须通过类内默认值或成员初始化列表完成初始化。

成员真正的初始化顺序永远是声明顺序，不是成员初始化列表的书写顺序。因此初始化列表也应按声明顺序写。

## 8. auto 的花括号规则

`auto` 有额外规则：

```cpp
auto a{1}; // int
auto b = {1}; // std::initializer_list<int>
auto c = {1, 2, 3}; // std::initializer_list<int>
// auto d{1, 2}; // 错误：直接列表初始化 auto 只能有一个元素
```

如果推导出的类型很重要，不要依赖容易误读的写法，直接写明类型。

## 9. 实用选择顺序

1. 先看对象是否正在声明：正在声明就是初始化；已经存在才是赋值。
2. 标量和简单聚合数据优先使用 `{}`，避免未初始化和窄化转换。
3. 类对象先理解构造函数接口；存在 `initializer_list` 时特别比较 `()` 与 `{}`
   的语义。
4. 聚合结构体优先写 `Point{...}`，C++20 可以使用指定初始化器增强可读性。
5. 固定长度数组优先 `std::array`，动态长度优先 `std::vector`。
6. 构造函数使用成员初始化列表，不要在函数体中用赋值模拟初始化。
7. 不要用等号判断 copy：`T x = source` 是初始化，`x = source` 才是赋值。

参考：

- [Initialization](https://en.cppreference.com/w/cpp/language/initialization)
- [List initialization](https://en.cppreference.com/w/cpp/language/list_initialization)
- [Aggregate initialization](https://en.cppreference.com/w/cpp/language/aggregate_initialization)
- [Default initialization](https://en.cppreference.com/w/cpp/language/default_initialization)
- [Value initialization](https://en.cppreference.com/w/cpp/language/value_initialization)

## 比想象的还复杂

1. std::initializer_list
2. {} 和 ()
3. 不要忘记了，当多出来了 new 关键字之后，这些初始化方法都是如何做的?

4. 有时候真的有点无语:

D() 可能是初始化，也可能
	C d(A());

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
