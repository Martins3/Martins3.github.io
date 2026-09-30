# cpp template : concept
<!-- 5d0649f3-e644-46f2-8365-b97fcbc2cbd9 -->

C++20 的 **concept 是给模板参数加上编译期约束**：明确告诉编译器，这个模板接受什么类型，以及这些类型必须支持哪些操作。

`typename T` 表示“这里需要一个类型”；`SomeConcept T` 表示“这里需要一个满足指定条件的类型”。

## 为什么需要 concept

普通模板可以这样写：

```cpp
template<typename T>
T add(T a, T b) {
    return a + b;
}
```

这里隐含着一个要求：`T` 必须支持 `a + b`，而且结果能够转换为返回类型 `T`。

但是模板声明没有表达这个要求。如果传进来的类型不支持加法，编译器通常要等到实例化函数体时才发现错误。复杂模板中，这容易产生很长的报错。

concept 可以把这个要求放到模板的接口上：

```cpp
#include <concepts>

template<typename T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};

template<Addable T>
T add(T a, T b) {
    return a + b;
}
```

这里分两步：

1. 定义 `Addable`：对于类型 `T`，表达式 `a + b` 必须合法，而且结果能够转换为 `T`。
2. 用 `Addable` 约束模板：只有满足这个条件的类型，才能使用这个 `add`。

此处的 `-> std::convertible_to<T>` 是对表达式结果的类型约束，不是函数的尾置返回类型声明。

## concept 是一个命名的编译期条件

concept 本质上是一个带名字的、结果为 `bool` 的编译期条件。它不一定要通过 `requires` 表达式定义：

```cpp
template<typename T>
concept SmallType = sizeof(T) <= 8;

static_assert(SmallType<char>);
static_assert(!SmallType<char[16]>);
```

`SmallType<char>` 就是在问：“`char` 是否满足 `SmallType` 这个条件？”

条件可以使用类型特征、常量表达式，也可以使用 `&&`、`||` 等运算符组合。例如：

```cpp
template<typename T>
concept SmallIntegral = std::integral<T> && SmallType<T>;
```

`std::integral` 是标准库 `<concepts>` 中提供的 concept，用来约束整数类型。

## requires 的两种用途

### requires 子句：给模板加约束

下面两种写法在这个例子中表达相同的约束，实际编写时选择一种即可：

```cpp
template<Addable T>
T add(T a, T b);
```

```cpp
template<typename T>
requires Addable<T>
T add(T a, T b);
```

约束也可以写在函数声明的参数列表之后：

```cpp
template<typename T>
T add(T a, T b) requires Addable<T>;
```

### requires 表达式：检查某些代码能不能成立

```cpp
template<typename T>
concept HasSize = requires(const T& x) {
    x.size();
};
```

这里检查 `x.size()` 是否是合法表达式。**不会创建对象 `x`，也不会真的执行 `size()`。**

它还可以检查嵌套类型以及表达式的结果类型：

```cpp
#include <concepts>
#include <cstddef>

template<typename T>
concept ContainerLike = requires(const T& x) {
    typename T::value_type;  // 必须存在这个嵌套类型
    { x.size() } -> std::convertible_to<std::size_t>;
};
```

三者的关系是：

- `requires` 表达式描述要检查的条件。
- `concept` 给条件命名，以便复用。
- `requires` 子句或者 `template<Concept T>` 把条件用到模板上。

也可以不定义 concept，直接使用 `requires` 表达式作为模板的约束。这时会出现两个连续的 `requires`：

```cpp
template<typename T>
requires requires(const T& x) { x.size(); }
void use_size(const T& x) {
    (void)x.size();
}
```

第一个 `requires` 引入约束子句，第二个 `requires` 开始一个结果为 `bool` 的表达式。

## concept 参与重载选择

```cpp
#include <concepts>
#include <iostream>

template<std::integral T>
void print(T) {
    std::cout << "整数\n";
}

template<std::floating_point T>
void print(T) {
    std::cout << "浮点数\n";
}

int main() {
    print(42);    // 选择整数版本
    print(3.14);  // 选择浮点数版本
    // print("hello");  // 两个版本的约束都不满足
}
```

编译器选择模板时会检查约束，不满足约束的候选不会被选中。如果没有其他可用的重载，调用就会报错。

编译器还可以按照约束的包含关系规则（subsumption）选择更具体的重载。但这不等于对任意布尔表达式进行数学推理，不能认为编译器总能识别两个条件之间的逻辑蕴含关系。

这也是 concept 与在函数体中写 `static_assert` 的区别：约束影响候选是否可用，而函数体中的 `static_assert` 是在选中模板并实例化函数体之后进行检查。

## 与面向对象接口的区别

| 对比项 | concept | 抽象基类 / 虚函数 |
| --- | --- | --- |
| 检查或选择的时机 | 编译期 | 类型关系在编译期检查，虚调用可在运行期分派 |
| 类型如何满足要求 | 满足指定条件即可，无须声明继承 | 通常需要显式继承并实现接口 |
| 是否引入运行时分派 | 不会 | 虚调用可能涉及运行时分派 |

一个自定义类型只要支持合适的 `operator+`，就能满足前面的 `Addable`，无需写任何“实现 Addable”的声明。

concept 本身不会引入运行时检查，但模板函数体中的操作仍然可以在运行时执行。

## 能检查合法性，不能自动证明行为正确

检查出 `a + b` 可以编译，并不代表它满足交换律，也不代表它实现了算法期望的“加法”。

同样，检查出 `x.size()` 可以调用，也不代表它一定返回正确的元素个数。涉及操作含义、复杂度或数学性质的要求，仍然需要通过文档约定和实现来保证。

已有语法示例见 [concepts.cpp](concepts.cpp)。阅读其中的 `requires` 时，要理解成“检查表达式是否合法”，而不是“执行这里的函数”。

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
