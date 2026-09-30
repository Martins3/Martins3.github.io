# cpp template :  typename class
<!-- 4d66cfee-775b-4fbd-a66a-01d8175f276f -->

`typename` 和 `class` 要分场景看：**声明模板类型参数时可以互换；显式标明依赖名称是类型时用 `typename`。**

## 声明模板类型参数：两者等价

下面两种写法含义完全相同，选择其中一种即可：

```cpp
template <typename T>
void foo(T value) {}
```

```cpp
template <class T>
void foo(T value) {}
```

这里的 `class` **不代表 T 必须是类**，它也可以是 `int`、指针等类型：

```cpp
foo<int>(42);
foo<double>(3.14);
```

两种写法也都能用于类模板，下面同样选择其中一种即可：

```cpp
template <typename T>
class Box {};
```

```cpp
template <class T>
class Box {};
```

实际代码里，选择一种风格并保持一致即可。不要把上述两种等价定义同时放进同一作用域，否则会造成重定义。

## 标明依赖于模板参数的类型：用 typename

考虑下面的代码：

```cpp
template <typename T>
void foo()
{
    typename T::value_type x{};
}
```

`T::value_type` 的含义取决于模板参数 `T`，因此称为**依赖名称**。在模板定义阶段，编译器无法直接确定它是类型，还是静态数据成员等其他东西。

这里的 `typename` 告诉编译器：把 `T::value_type` 当作类型来解析。

例如：

```cpp
struct MyType {
    using value_type = int;
};

// 在函数中调用：
foo<MyType>();  // x 的类型是 int
```

这种用途不能换成 `class`：

```cpp
typename T::value_type x{};  // 正确
class T::value_type x{};     // 不能作为等价替换
```

常见例子是容器的迭代器类型：

```cpp
template <typename Container>
void traverse(const Container& c)
{
    typename Container::const_iterator it = c.begin();
}
```

如果使用 `auto`，就不用写出该类型：

```cpp
auto it = c.begin();
```

注意，并非所有依赖类型出现的位置都必须写 `typename`；部分语法位置本来就被视为类型，C++20 也进一步放宽了规则。

## 声明普通类：用 class

```cpp
class Foo;      // 类的前向声明
class Bar {};   // 类的定义
```

这里不能用 `typename` 替换。

## 对照表

| 场景 | `typename` | `class` |
| --- | --- | --- |
| 模板类型参数：`template <... T>` | 可以 | 可以，含义相同 |
| 显式标明依赖名称是类型：`... T::value_type` | 使用它 | 不能等价替换 |
| 普通类的声明或定义 | 不可以 | 使用它 |

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
