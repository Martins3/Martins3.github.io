# cpp operator
<!-- ec277e12-6547-4368-be7d-84cdc522dc2a -->

`operator` 可以让自定义类型支持 `a + b`、`a[i]`、`a()` 等表达式，也可以定义
`operator bool()` 这样的类型转换函数。本文重点总结运算符重载；示例默认使用
C++17，比较运算的简化写法单独标注 C++20。

## 速查表

下表中的 `T` 表示自定义类型，`E`
表示元素类型。返回类型是通常的设计惯例，不全是语法强制要求。

| 表达式        | 常见声明                                                 | 通常放在哪里                | 语义与返回值                        |
| ------------- | -------------------------------------------------------- | --------------------------- | ----------------------------------- |
| `a = b`       | `T& operator=(const T& rhs)`                             | 成员，语法要求              | 修改自己，返回 `*this`              |
| `a += b`      | `T& operator+=(const T& rhs)`                            | 成员，设计惯例              | 修改自己，返回 `*this`              |
| `a + b`       | `T operator+(T lhs, const T& rhs)`                       | 非成员，可为 `friend`       | 不修改原操作数，返回新值            |
| `-a`          | `T operator-() const`                                    | 成员或非成员                | 返回取负后的新值                    |
| `a == b`      | `bool operator==(const T& a, const T& b)`                | 通常非成员，也可成员        | 返回是否相等                        |
| `a < b`       | `bool operator<(const T& a, const T& b)`                 | 通常非成员，也可成员        | 定义排序关系                        |
| `++a`         | `T& operator++()`                                        | 通常成员                    | 修改自己，返回修改后的自身引用      |
| `a++`         | `T operator++(int)`                                      | 通常成员                    | 修改自己，返回修改前的副本          |
| `a[i]`        | `E& operator[](std::size_t i)`                           | 成员，语法要求              | 通常返回元素引用，另提供 const 版本 |
| `a(x)`        | `R operator()(Arg x) const`                              | 成员，语法要求              | 对象像函数一样被调用                |
| `*a` / `a->x` | `E& operator*() const` / `E* operator->() const`         | `*` 可非成员，`->` 必须成员 | 指针、迭代器风格访问                |
| `os << a`     | `std::ostream& operator<<(std::ostream& os, const T& a)` | 非成员                      | 输出对象，返回流以支持连续输出      |
| `is >> a`     | `std::istream& operator>>(std::istream& is, T& a)`       | 非成员                      | 读入对象，返回流以支持连续输入      |
| `if (a)`      | `explicit operator bool() const`                         | 成员，语法要求              | 判断对象是否有效                    |

`- * / %` 通常参考 `+`，`-= *= /= %=` 通常参考 `+=`，`--` 参考 `++`。

## 成员函数还是 friend？

对普通二元运算，可以这样理解参数对应关系：

```cpp
// 成员版本：左操作数是 *this，只写右操作数参数。
T T::operator+(const T& rhs) const;
// a + b 对应 a.operator+(b)

// 非成员版本：左右操作数都是显式参数。
T operator+(const T& lhs, const T& rhs);
// a + b 对应 operator+(a, b)
```

这是两种候选形式，不建议给同一组操作数同时提供两种等价重载，以免歧义。

- 修改左操作数的 `+=` 等通常写成成员。
- 左右地位对等的 `+`、`==`
  等通常写成非成员，使两边都能参与普通参数的隐式转换。传统成员形式不能先把左边转换成
  `T`，再调用 `T` 的成员。
- 非成员需要访问私有数据时，可以声明为 `friend`；只用公开接口时不必加。
- **写在类里面的 `friend operator+` 仍然是非成员，没有 `this`。**

这些是设计惯例，特别是 `+=` 并非语法上必须写成成员。参见
[C++ Core Guidelines 的运算符设计建议](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c160-define-operators-primarily-to-mimic-conventional-usage)。

## 算术与复合赋值：先写 +=，再复用它实现 +

```cpp
struct Vec2 {
    int x = 0;
    int y = 0;

    Vec2& operator+=(const Vec2& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    friend Vec2 operator+(Vec2 lhs, const Vec2& rhs) {
        lhs += rhs;
        return lhs;
    }

    Vec2 operator-() const {
        return {-x, -y};
    }
};

// Vec2 a{1, 2}, b{3, 4};
// auto c = a + b;  // c 为 {4, 6}，a 和 b 不变
// a += b;         // a 变为 {4, 6}
```

`+=` 返回 `Vec2&`，可以继续操作原对象，例如 `(a += b) += c`。`+`
返回新对象，不能返回局部变量的引用。

`operator+` 按值接收
`lhs`，直接在这份值上运算。左边是左值时通常需要复制；临时对象则可能利用移动或复制消除。`return lhs`
可以隐式移动，但不能保证必定调用移动构造，也不必写 `std::move(lhs)`。

这种类内定义的友元常称为 **hidden
friend**：没有额外的命名空间声明时，普通名字查找找不到它，但 `a + b` 可以通过
ADL（根据实参类型关联查找）找到。在普通头文件场景中，类内定义隐含
`inline`，不代表编译器必须把调用展开。

## 比较：==、< 与 C++20 的 <=>

只需要相等判断时，比较能代表对象含义的字段即可：

```cpp
#include <string>

struct User {
    std::string name;

    friend bool operator==(const User& lhs, const User& rhs) {
        return lhs.name == rhs.name;
    }

    friend bool operator!=(const User& lhs, const User& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const User& lhs, const User& rhs) {
        return lhs.name < rhs.name;
    }
};
```

成员版本 `bool operator==(const User& rhs) const` 同样合法；末尾的 `const`
表示比较不修改左操作数，也允许 const 对象调用。

在 C++17 中，定义 `==` 不会自动提供 `!=`，定义 `<`
也不会自动提供其他关系运算。用于 `std::sort`、`std::set`
等的排序关系要满足严格弱序，例如不能拿 `<=` 代替 `<`。

C++20 中，如果逐成员比较正好符合业务含义，可以简化为：

```cpp
#include <compare>

struct Point {
    int x;
    int y;

    auto operator<=>(const Point&) const = default;
};

// Point a{1, 9}, b{2, 0};
// a < b 为 true：先比较 x，相等时才比较 y。
// a == b、a != b、a <= b 等也可以使用。
```

这里默认化的 `<=>` 在没有显式声明 `==` 时还会隐式声明默认化的
`==`，其他比较表达式通过重写规则获得支持。手写 `<=>` 的函数体不会自动生成
`==`。只需要相等比较时，也可以单独写
`bool operator==(const Point&) const = default;`。参见
[标准草案的默认比较规则](https://eel.is/c++draft/class.compare.default)。

## 自增自减：前置返回自己，后置返回旧值

```cpp
struct Counter {
    int value = 0;

    Counter& operator++() {
        ++value;
        return *this;
    }

    Counter operator++(int) {
        Counter old = *this;
        ++(*this);
        return old;
    }
};

// Counter c{3};
// auto old = c++;  // old.value == 3，c.value == 4
// auto& now = ++c; // c.value == 5，now 引用 c
```

后置版本的 `int` 是区分重载的占位参数，`c++` 调用时传入
`0`，不是“增加多少”。不需要旧值时，通常优先写
`++it`，避免要求迭代器保存旧值；内置整数的两种写法通常能被优化成相同代码。

## 下标：提供可写与只读两个版本

```cpp
#include <cstddef>
#include <vector>

struct Buffer {
    std::vector<int> data;

    int& operator[](std::size_t i) {
        return data[i];
    }

    const int& operator[](std::size_t i) const {
        return data[i];
    }
};

// Buffer b{{10, 20}};
// b[0] = 42;             // 返回 int&，可以修改元素
// const Buffer& cb = b;
// int n = cb[0];          // 返回 const int&，只能读取
// cb[0] = 1;             // 编译错误
```

非 const 版本如果返回 `int`，就不能通过 `b[i] = value` 修改原元素。这里沿用
`vector::operator[]` 的有效索引前提；需要越界异常检查时，可以在实现里改用
`data.at(i)`。

## 函数调用：operator() 让对象携带状态并被调用

```cpp
struct Add {
    int base;

    int operator()(int x) const {
        return base + x;
    }
};

// Add add{10};
// int n = add(3);  // 13，相当于 add.operator()(3)
```

这种对象称为函数对象，常用作算法的比较器、回调或策略。可以按参数类型重载
`operator()`；如果调用需要修改对象内部状态，就不加末尾的 `const`。普通 lambda
的闭包类型也提供 `operator()`。

## 流输入输出：返回流引用，支持链式调用

```cpp
#include <istream>
#include <ostream>
#include <string>
#include <utility>

struct Kitty {
    std::string name;

    friend std::ostream& operator<<(std::ostream& os, const Kitty& k) {
        return os << k.name;
    }

    friend std::istream& operator>>(std::istream& is, Kitty& k) {
        std::string name;
        if (is >> name) {
            k.name = std::move(name);
        }
        return is;
    }
};

// std::cout << k1 << ' ' << k2 << '\n';
// if (std::cin >> k1) { /* 输入成功 */ }
```

`std::cout << k` 的左操作数是流，因此不能用 `Kitty`
的成员函数实现这种参数顺序。`Kitty::operator<<(std::ostream&)` 对应的是
`k << std::cout`。

输出一般不擅自追加换行或 `std::endl`，由调用方决定格式和刷新时机。输入参数不能是
`const Kitty&`；先解析到临时变量，成功后再更新对象。自定义格式验证失败时用
`is.setstate(std::ios::failbit)` 标记失败。

## 指针风格：operator* 与 operator->

```cpp
struct Item {
    int value;
};

struct ItemRef {
    Item* ptr;  // 仅借用，不拥有或释放对象

    Item& operator*() const { return *ptr; }
    Item* operator->() const { return ptr; }
};

// Item item{7};
// const ItemRef p{&item};
// (*p).value = 8;
// p->value = 9;  // 经 operator->() 得到 Item*，再访问 value
```

这里的 const 限制包装对象本身，不限制它指向的 `Item`，类似
`Item* const`。若需要只读对象，应使用指向 const
元素的设计。调用方需要保证指针非空且目标仍存活。

`operator->()` 还可以返回另一个提供 `operator->()`
的类对象，编译器会继续处理，直到得到原生指针；它不能任意返回一个整数来作为成员访问的目标。

## 类型转换：operator int() 与 explicit operator bool()

```cpp
struct Handle {
    int fd = -1;

    explicit operator bool() const noexcept {
        return fd >= 0;
    }
};

// Handle h{3};
// if (h) { /* 有效 */ }             // 允许：布尔上下文转换
// bool ok = static_cast<bool>(h);   // 允许：显式转换
// bool ok2 = h;                    // 编译错误：不允许这种隐式转换
// int n = h;                       // 编译错误
```

转换函数名已经包含目标类型，所以写
`operator bool()`，前面不再写返回类型。`explicit` 允许在
`if`、`while`、`!`、内置 `&&`/`||`
等布尔上下文里检查对象，又避免它随意参与整数运算。

`operator int() const` 则可以让对象隐式转成整数，例如
`int n = obj;`。如果只想允许主动转换，可以写成
`explicit operator int() const`，调用方使用
`static_cast<int>(obj)`。**`static_cast` 也能调用没有 explicit
的转换函数，并不只选择 explicit 版本。** 转换函数规则参见
[标准草案](https://eel.is/c++draft/class.conv.fct)。

构造函数 `T(int)` 解决“其他类型转成 `T`”，转换函数 `operator int()` 解决“`T`
转成其他类型”；两者的方向不同。

## 赋值：优先让成员自己管理资源

```cpp
#include <string>

struct Record {
    std::string name;
    // 编译器自动提供合适的复制/移动操作，无需手写 operator=。
};

// Record a{"alice"};
// Record b = a;  // 初始化：复制构造，不调用 operator=
// b = a;         // 已有对象赋值：调用复制赋值
```

需要阅读手写代码时，记住典型签名：复制赋值是
`T& operator=(const T&)`，移动赋值是 `T& operator=(T&&)`；常规实现返回
`*this`。移动赋值只有在确实不会抛异常时才标记 `noexcept`。

直接拥有裸资源时，还需要考虑自赋值、旧资源释放和异常安全；一般优先用
`std::string`、`std::vector`、智能指针管理资源，遵循 Rule of
Zero。不要仅为了展示 `operator=`
就添加特殊成员声明：用户声明复制操作等会影响隐式移动操作的生成。

## 常见限制与易错点

- 不能创造新符号，也不能改变运算符原有的优先级和结合方式。`a + b * c`
  的分组不会因重载改变。
- 普通运算符重载至少涉及一个类或枚举操作数，不能重新定义两个 `int`
  相加的含义。`operator new/delete` 有单独的规则。
- `.`、`.*`、`::`、`?:`、`sizeof` 等不能重载。
- `=`、`[]`、`()`、`->` 必须是成员。C++23 允许 `operator()` 和 `operator[]`
  是静态成员，但仍不是自由函数。
- 重载 `&&`、`||`
  不具有内置运算符的短路行为：正常求值时两个操作数都会被求值。有效性判断通常提供
  `explicit operator bool()` 即可。
- 定义 `+` 不会自动生成 `+=`，反过来也一样，需要显式编写复用关系。
- 用普通 `auto` 推导返回类型会丢掉引用；需要返回自身引用时明确写 `T&` 或
  `auto&`。不要返回局部对象的引用。

语法限制参见 [标准草案的运算符重载章节](https://eel.is/c++draft/over.oper)。

## human

1. 那么
`T& operator=(const T& rhs)` 这种，参数只能这样定义吗?
	- 还是说，其实我们可以搞一些其他的定义方法?
2. 如何归类一下这些东西?

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
