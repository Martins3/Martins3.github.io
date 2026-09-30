# C++20 为什么允许用圆括号初始化聚合体

C++20 不是第一次引入 `()` 初始化。
类对象早就可以通过圆括号选择构造函数：

```cpp
std::string text(10, 'a');
```

C++20 新增的是：允许使用圆括号中的表达式列表，按成员初始化 aggregate（聚合体）。

```cpp
struct Point {
    int x;
    int y;
};

Point a{1, 2}; // C++20 之前就可以
Point b(1, 2); // C++20 开始可以
```

## 1. 要解决的核心问题

直接写 `Point{1, 2}` 已经很方便，
这个改动为了让聚合体能够通过泛型构造接口原地构造。

考虑下面的代码：

```cpp
auto point = std::make_shared<Point>(1, 2);
```

`std::make_shared` 内部需要完美转发参数，其构造过程可以近似理解为：

```cpp
::new (address) Point(std::forward<Args>(args)...);
```

最终形成的是 `Point(1, 2)`。在 C++17 中，`Point` 没有接受两个整数的构造函数，
聚合初始化又只能使用 `{}`，所以这段代码不能编译。

类似问题还会出现在：

- `std::make_unique<T>(args...)`
- `container.emplace_back(args...)`
- `optional.emplace(args...)`
- `allocator_traits::construct(...)`
- 其他基于完美转发的工厂和原地构造接口

C++20 允许 `Point(1, 2)` 按顺序初始化成员后，这些接口不需要特殊处理聚合体：

```cpp
auto point = std::make_shared<Point>(1, 2);

std::vector<Point> points;
points.emplace_back(3, 4);
```

这正是 [P0960R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html) 的主要设计动机。

## 2. C++17 的绕过办法有什么问题

一种办法是先构造临时聚合体：

```cpp
auto point = std::make_shared<Point>(Point{1, 2});
```

这不能获得真正的原地成员初始化：它先产生一个 `Point` 临时对象，再把临时对象移动到 `make_shared` 管理的存储区。
它可能多出一次移动，而且要求目标类型能够移动或拷贝。

`emplace` 一类接口的重要目标之一，恰恰是让不能拷贝、不能移动的对象也能在最终位置直接构造。

另一种办法是给聚合体添加构造函数：

```cpp
struct Point {
    Point(int x, int y) : x(x), y(y) {}

    int x;
    int y;
};
```

但这会产生重复代码，并且在 C++20 中，
存在用户声明的构造函数会使 `Point` 不再是聚合体。
为了配合泛型库而被迫放弃聚合体性质，并不合理。

## 3. 为什么不让泛型库统一使用花括号

看起来也可以把泛型库改成：

```cpp
::new (address) T{std::forward<Args>(args)...};
```

问题是 `T(args...)` 和 `T{args...}` 对普通类可能表示不同操作：

```cpp
std::vector<int> a(3, 9); // 三个元素：9, 9, 9
std::vector<int> b{3, 9}; // 两个元素：3, 9
```

花括号会优先考虑 `std::initializer_list` 构造函数。若 `make_shared`、`emplace` 等现有接口从 `T(args...)` 改成 `T{args...}`，一些原有程序会悄悄改变行为。

而且 `{1, 2}` 这种 braced-init-list（花括号初始化器列表）本身不是普通表达式，没有可供函数模板直接推导和完美转发的类型。

所以正确的兼容方向不是修改所有泛型库，而是让聚合体也接受这些接口一直使用的 `T(args...)` 形式。早期提案 [P0960R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0960r0.html) 对这个问题给出了详细说明。

## 4. 圆括号和花括号并不完全等价

C++20 没有规定把圆括号机械替换成花括号。二者刻意保留了一些不同语义。

### 4.1 窄化转换

花括号禁止窄化转换，圆括号允许：

```cpp
struct Value {
    int number;
};

// Value a{3.14}; // 错误：double 到 int 是窄化转换
Value b(3.14); // 可以，number 得到 3
```

因此直接初始化聚合体时，`{}` 通常更安全。

### 4.2 指定成员初始化

C++20 designated initializer（指定初始化器）只能使用花括号：

```cpp
Point point{.x = 1, .y = 2};
```

不存在对应的 `Point(.x = 1, .y = 2)` 写法。

### 4.3 引用成员与临时对象生命周期

对于聚合体的引用成员，花括号和圆括号的临时对象生命周期不同：

```cpp
struct Reference {
    const int& value;
};

Reference safe{42}; // 临时 int 的生命周期延长到 safe 的生命周期
Reference dangling(42); // 临时 int 在本条语句结束时销毁
```

第二个对象中的引用随后会悬空，因此引用成员尤其应该优先使用花括号。

### 4.4 数组

数组也是聚合体，所以 C++20 也允许：

```cpp
int values[](1, 2, 3);
```

不过这种写法并不常见。直接写数组时，花括号通常更清晰：

```cpp
int values[]{1, 2, 3};
```

## 5. 不会改变原有的拷贝构造含义

C++20 的设计原则之一是尽量不改变已有 `A(value)` 代码的意义。

```cpp
struct A {
    int value;
};

A source{10};
A result(source);
```

`A result(source)` 在 C++20 中仍然调用 `A` 的隐式拷贝构造函数，
不会尝试用整个 `source` 初始化第一个 `int` 成员。

而下面的代码才使用 C++20 新增的逐成员圆括号初始化：

```cpp
A result(10);
```

## 6. 应该选择哪一种

自己直接创建聚合体时，通常优先使用花括号：

```cpp
Point point{1, 2};
```

理由是它：

- 禁止窄化转换；
- 支持指定成员初始化；
- 对引用成员有更合适的临时对象生命周期规则；
- 能直观表达“这是聚合初始化”。

圆括号初始化的主要价值，是让聚合体无缝进入 `make_shared`、`make_unique`、
`emplace`、allocator 和其他使用 `T(args...)` 的泛型构造体系：

```cpp
auto point = std::make_unique<Point>(1, 2);
```

可以把 C++20 的改动概括成：

> 不是为了替代聚合体的 `{}`，而是为了消除聚合体与泛型原地构造接口之间的不兼容。

## 参考资料

- [P0960R3: Allow initializing aggregates from a parenthesized list of values](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html)
- [P0960R0: 初始提案与库接口动机](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0960r0.html)
- [P1975R0: Fixing the wording of parenthesized aggregate-initialization](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1975r0.html)

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
