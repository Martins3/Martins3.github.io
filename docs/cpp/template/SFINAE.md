# cpp template SFINAE

SFINAE 的核心是：编译器尝试把某个函数模板加入重载候选时，
如果模板参数替换导致特定位置的类型或表达式不合法，就丢弃这个候选，而不是立即报错

全称是 **Substitution Failure Is Not An Error**，即“替换失败并不是错误”。

**2. `enable_if`：主动制造替换失败**

实际写代码时，经常用 `std::enable_if` 控制模板是否参与重载：

```cpp
#include <iostream>
#include <type_traits>

// C++14
template<class T>
std::enable_if_t<std::is_integral<T>::value, void>
print(T x)
{
    std::cout << "integer: " << x << '\n';
}

template<class T>
std::enable_if_t<std::is_floating_point<T>::value, void>
print(T x)
{
    std::cout << "floating: " << x << '\n';
}

int main()
{
    print(42);    // integer: 42
    print(3.14);  // floating: 3.14
}
```

你可以把 `enable_if_t<条件, 类型>` 理解成：

```cpp
条件为 true  → 得到指定类型
条件为 false → 无法得到类型，触发替换失败
```

于是调用 `print(42)` 时：

```cpp
// 第一个模板：T = int
enable_if_t<true, void>   // 返回类型为 void，保留候选

// 第二个模板：T = int
enable_if_t<false, void>  // 无法形成返回类型，丢弃候选
```

**这不是运行时的 `if`，而是在编译期决定“哪些函数可以参与这次重载选择”。**

**3. 为什么 `enable_if` 能做到？**

它的基本实现非常简单：

```cpp
template<bool Cond, class T = void>
struct enable_if {};           // 没有 type

template<class T>
struct enable_if<true, T> {
    using type = T;            // 只有 true 才提供 type
};

template<bool Cond, class T = void>
using enable_if_t = typename enable_if<Cond, T>::type;
```

所以：

```cpp
enable_if<true, void>::type   // 存在，是 void
enable_if<false, void>::type  // 不存在
```

**SFINAE 提供“替换失败就淘汰候选”的规则，`enable_if` 利用这个规则表达条件。**

**4. 最容易误解的边界：函数体报错不属于 SFINAE**

```cpp
template<class T>
void g(T x)
{
    typename T::value_type y;  // 在函数体里面
}

void g(...)
{
}

int main()
{
    g(42);  // 编译错误，不会退回 g(...)
}
```

原因是：

1. 替换 `T = int` 后，函数声明 `void g(int)` 完全合法。
2. 重载选择选中这个模板，因为它比 `g(...)` 更匹配。
3. 实例化函数体，发现 `int::value_type` 不合法。
4. **此时直接报错，不会重新选择其他重载。**

严格地说，SFINAE 适用于替换过程中发生在**直接上下文（immediate context）**里的失败，例如函数参数类型、返回类型以及模板参数声明中的相关类型或表达式；并不是模板中发生的所有错误都可以忽略。

另外，**如果所有候选都被淘汰，调用本身仍然会报“没有匹配的函数”**。

C++20 中，这类限制通常可以用 concepts 更直接地表达：

```cpp
#include <concepts>

void print(std::integral auto x)
{
    // 整数版本
}

void print(std::floating_point auto x)
{
    // 浮点数版本
}
```

读老代码时，看到 `enable_if`，首先问自己：**“这个条件想让哪个模板在什么情况下退出候选集合？”**


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
