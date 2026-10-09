## trait
cpp 的这种 trait ，值
只能说，这个是新的库 ？

https://www.reddit.com/r/cpp_questions/comments/bwevbh/why_do_we_need_stdallocator_traitst/

1. allocator 也完全理解清楚，和 allocator_trais 啥关系

1. char_traits
https://en.cppreference.com/w/cpp/string/char_traits
    - https://stackoverflow.com/questions/5319770/what-is-the-point-of-stl-character-traits


## 从零理解 traits：给类型配一份说明书

先看可以直接运行的 [basic_traits.cpp](basic_traits.cpp)。它没有使用标准库的
traits，而是自己定义了一套，所以可以看到 traits 到底是怎么实现的。

假设要打印下面两种对象中的数值：

```cpp
int count = 42;

struct Temperature {
    double celsius;
};
Temperature room{23.5};
```

对于 `count`，数值就是它自己；对于 `room`，数值在 `room.celsius` 中。
我们希望写一个 `print_value()` 模板，同时处理这两种情况。
但直接写 `object.celsius` 不行，因为 `int` 没有这个成员。

### 第一步：为每种类型写一份说明书

这里把说明书命名为 `value_traits<T>`，约定提供三个成员：

| 成员 | 回答的问题 | `int` 对应的答案 | `Temperature` 对应的答案 |
| --- | --- | --- | --- |
| `value_type` | 取出的数值是什么类型？ | `int` | `double` |
| `name` | 打印时叫什么？ | `"integer"` | `"temperature (C)"` |
| `get(object)` | 怎样从对象中取出数值？ | 返回对象本身 | 返回对象的 `celsius` |

用普通的类模板和显式特化实现这份约定：

```cpp
template <typename T> struct value_traits;

template <> struct value_traits<int> {
    using value_type = int;
    static constexpr const char *name = "integer";

    static value_type get(const int &value) {
        return value;
    }
};

template <> struct value_traits<Temperature> {
    using value_type = double;
    static constexpr const char *name = "temperature (C)";

    static value_type get(const Temperature &value) {
        return value.celsius;
    }
};
```

`template <>` 表示显式特化，也就是专门为指定类型提供定义。
`value_traits<int>` 和 `value_traits<Temperature>` 是两个不同的类类型。
本例只声明主模板，不定义默认实现，因此尚未适配的类型不能使用这套接口。

**“以类型为输入”指的是把 `int`、`Temperature` 写进模板的 `<...>` 中。**
例如 `value_traits<Temperature>::value_type` 就是 `double`。
这里没有运行时传入“类型对象”的过程。

`get()` 的参数则是实际对象：`value_traits<Temperature>::get(room)` 返回 `23.5`。
traits 的选择发生在编译期，读取对象数值的操作可以发生在运行时。

### 第二步：算法通过说明书操作对象

```cpp
template <typename T> void print_value(const T &object) {
    using traits = value_traits<T>;
    typename traits::value_type value = traits::get(object);
    std::cout << traits::name << ": " << value << '\n';
}
```

这里 `traits` 只是一个局部类型别名，用来少写几次 `value_traits<T>`。
`typename` 告诉编译器：依赖模板参数的 `traits::value_type` 是一个类型名。
这一行也可以写成 `auto value = traits::get(object);`，示例刻意展开以展示关联类型。

调用 `print_value(room)` 时，可以把编译器选择后的代码理解为：

```cpp
double value = value_traits<Temperature>::get(room);
std::cout << value_traits<Temperature>::name << ": " << value << '\n';
```

调用 `print_value(count)` 时，则使用 `value_traits<int>`，得到 `int` 数值。
`get()` 和 `name` 都是静态成员，因此不需要先创建一个 traits 对象。

### 编译运行

在本目录执行：

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic basic_traits.cpp -o basic_traits.out
./basic_traits.out
```

输出：

```text
direct: 42, 23.5
integer: 42
temperature (C): 23.5
```

### 为什么要多写这一层？

对于这么小的程序，直接重载两个 `print_value()` 也很合理。
traits 的价值在于复用：如果以后还有保存数值、比较数值等模板函数，
它们都可以使用同一套 `value_traits<T>::get()`，不用分别了解 `celsius` 等成员。
新增类型时，提供对应的 traits 特化，使用这套接口的算法就可以继续复用。

因此，这里的 traits 是一种组织模板代码的惯用法。`value_traits` 是我们自己起的名字，
并非 C++ 关键字，也不需要安装一个新库。原类型也不需要继承 traits。

再看标准库就容易对应了：`std::numeric_limits<T>::max()` 提供数值上限，
`std::iterator_traits<It>::value_type` 提供迭代器的元素类型。
它们与本例一样，都让泛型代码通过统一名字获取与某种类型有关的信息。

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
