# cpp std::initializer_list

<!-- 54d09739-fc15-48c9-b658-093f5c7f652f -->

`std::initializer_list<T>`
解决的核心问题是：**让函数和构造函数能够用一个参数，接收 `{...}`
中列出的、数量不固定的一组元素。** 最典型的用途就是让 `vector`、`set`
以及你自己定义的类，也能像数组一样方便地初始化。
[stroustrup.com](https://www.stroustrup.com/C%2B%2B11FAQ.html)

## 1. 原来有什么不方便？

数组很早就支持这样的写法：

```cpp
int a[] = {10, 20, 30};
```

但是在 C++11 之前，不能直接这样初始化 `vector`：

```cpp
std::vector<int> v = {10, 20, 30};  // C++11 之前不支持
```

往往需要先准备数组，再利用迭代器区间构造 `vector`：

```cpp
int a[] = {10, 20, 30};
std::vector<int> v(a, a + 3);
```

**同样是“用这几个元素创建一个集合”，数组可以直接写，类类型却需要绕一下。**
`initializer_list`
配合语言层面的列表初始化规则，补上了这个缺口。[stroustrup.com](https://www.stroustrup.com/C%2B%2B11FAQ.html)

现在可以直接写：

```cpp
std::vector<int> v{10, 20, 30};
```

`vector` 提供了这样的构造函数，省略分配器等细节后是：

```cpp
vector(std::initializer_list<T> values);
```

所以，**这里不是调用“接收三个 `int`
参数的构造函数”，而是调用“接收一个列表参数的构造函数”。**
[Eel](https://eel.is/c%2B%2Bdraft/vector.overview)

## 2. 自己的类和普通函数，也能使用这个能力

这不是 `vector` 独享的特殊语法。你可以给自己的类添加同样的接口：

```cpp
#include <initializer_list>
#include <vector>

class Numbers {
    std::vector<int> data_;

public:
    Numbers(std::initializer_list<int> values)
        : data_(values)
    {}
};

Numbers a{1, 2};
Numbers b{1, 2, 3, 4, 5};
```

无论列出两个还是五个元素，调用的都是同一个构造函数，不需要针对每种元素数量写一个重载。[stroustrup.com](https://www.stroustrup.com/C%2B%2B11FAQ.html)

虽然名字里有 _initializer_，它也可以用于普通函数：

```cpp
#include <initializer_list>

int sum(std::initializer_list<int> values)
{
    int result = 0;
    for (int value : values) {
        result += value;
    }
    return result;
}

// 调用示例：
int a = sum({1, 2, 3});        // 6
int b = sum({10, 20, 30, 40}); // 100
int c = sum({});              // 0
```

注意，这里的调用是 `sum({1, 2, 3})`，不是
`sum(1, 2, 3)`：**传入的是一个列表参数，而不是三个独立参数。**
[stroustrup.com](https://www.stroustrup.com/C%2B%2B11FAQ.html)

## 3. 它本质上是什么？不是一个负责存储数据的 `vector`

可以把 `std::initializer_list<T>` 理解为：**指向一组只读元素的轻量视图。**

概念上类似下面这样，具体布局不由标准强制规定：

```cpp
template<class T>
struct InitializerListModel {
    const T* elements;
    std::size_t count;
};
```

实际的 `initializer_list` 提供 `begin()`、`end()` 和 `size()`
等接口。复制它只复制这个“视图”，**不会复制底层元素**，因此函数通常直接按值接收它。[Eel](https://eel.is/c%2B%2Bdraft/support.initlist)

对于：

```cpp
sum({10, 20, 30});
```

可以把执行过程理解成：

```cpp
// 示意：由编译器准备底层数组
const int backing_array[] = {10, 20, 30};

// 再让一个 initializer_list<int> 指向它，传给 sum。
```

编译器在语言规则的支持下完成这个过程；并不是用户自己调用某个公开的“指针＋长度”构造函数。[Eel](https://eel.is/c%2B%2Bdraft/dcl.init.list)

因此要区分：

| 对象                    | 作用                                           |
| ----------------------- | ---------------------------------------------- |
| `initializer_list<int>` | 临时访问传进来的一组只读元素，不拥有它们       |
| `vector<int>`           | 保存并管理自己的元素，之后可以修改、增加、删除 |

在前面的 `Numbers` 中，`data_(values)` 会把列表里的元素复制进 `vector`
自己的存储空间，而不是让 `vector`
一直引用这份列表。[Eel](https://eel.is/c%2B%2Bdraft/support.initlist)

## 4. `{...}` 不等于 `std::initializer_list`

这是一个很容易混淆的地方：

```cpp
int x{10};                   // 初始化一个 int

int a[]{1, 2, 3};            // 数组的聚合初始化

struct Point {
    int x;
    int y;
};
Point p{1, 2};               // 结构体的聚合初始化

std::vector<int> v{1, 2, 3}; // 调用 initializer_list 构造函数
```

这些写法都用了花括号，但只有最后一个例子涉及 `vector` 的 `std::initializer_list`
构造函数。

**`{...}` 是语言语法；`std::initializer_list<T>`
是用于接收其中一组元素的标准库类型，二者不是同一个概念。**
[Eel](https://eel.is/c%2B%2Bdraft/dcl.init.list)

## 5. 两个必须知道的注意事项

### 花括号可能改变构造函数的选择

```cpp
std::vector<int> a(3, 100); // 三个元素：100, 100, 100
std::vector<int> b{3, 100}; // 两个元素：3, 100
```

对于 `vector` 这样的类，使用非空花括号列表初始化时，会先尝试匹配
`initializer_list` 构造函数。因此，**不能简单地认为 `()` 和 `{}`
可以随便互换。** [Eel](https://eel.is/c%2B%2Bdraft/over.match.list)

### 不要把接收到的列表当成长期存储

下面这种写法存在生命周期问题：

```cpp
class Bad {
    std::initializer_list<int> data_;

public:
    Bad(std::initializer_list<int> values)
        : data_(values)  // 只复制视图，不复制元素
    {}
};

Bad b{1, 2, 3};
// 这个分号之后，底层临时数组的生命周期已经结束。
// b.data_ 还在，但它引用的元素已经失效。
```

需要长期保存时，应像前面的 `Numbers` 一样，把元素复制进 `vector`
等拥有存储的对象。另一方面，直接声明局部列表变量是有效的：

```cpp
std::initializer_list<int> values = {1, 2, 3};
// 底层数组的生命周期会延长到 values 的生命周期结束。
```

关键是：**从 `{...}`
直接初始化列表可以触发生命周期延长，但再复制这个列表不会继续延长底层数组的生命周期。**
[Eel](https://eel.is/c%2B%2Bdraft/dcl.init.list)

---

可以把它记成一句话：

**`std::initializer_list`
是“接收一组花括号元素”的接口工具，不是“长期保存一组元素”的容器。**
[Eel](https://eel.is/c%2B%2Bdraft/support.initlist)

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
