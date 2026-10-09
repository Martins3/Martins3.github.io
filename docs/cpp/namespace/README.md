# C++ namespace

| 文件                                                                                         | 重点                                                    |
| -------------------------------------------------------------------------------------------- | ------------------------------------------------------- |
| [basic.cpp](basic.cpp)                                                                       | 同名函数、全局限定、嵌套、别名、using、inline namespace |
| [adl.cpp](adl.cpp)                                                                           | 根据实参类型查找函数、hidden friend、通用 swap          |
| [counter.hpp](counter.hpp)、[counter.cpp](counter.cpp)、[counter-main.cpp](counter-main.cpp) | 跨文件声明与定义、匿名命名空间、链接                    |

## 3. 几种容易混淆的 using

| 写法                              | 含义                                        |
| --------------------------------- | ------------------------------------------- |
| `using std::cout;`                | using-declaration，将指定名字引入当前作用域 |
| `using namespace std;`            | using-directive，使该命名空间的名字参与查找 |
| `using Count = int;`              | 类型别名，Count 是 int 的另一种名字         |
| `namespace fs = app::filesystem;` | 命名空间别名，使用 namespace 关键字         |

```cpp
void print() {
    using std::cout;
    cout << "hello\n";
}
```

`using std::cout` 的影响限制在这里的函数块内。它不会复制一个 cout 对象。
如果引入的是函数名，会引入声明处可见的同名重载，而不是只选一个函数。
类型别名的更多用法见 [template/alias.md](../template/alias.md)。

`using namespace` 不会把成员搬到当前命名空间；原来的限定名仍然可以使用。
不同命名空间中有相同名字时，非限定查找可能产生歧义：

```cpp
namespace left  { int value = 1; }
namespace right { int value = 2; }

void ambiguous() {
    using namespace left;
    using namespace right;
    // int n = value;          // 编译错误：无法决定用哪个 value
    int n = left::value;       // 明确指定即可
    (void)n;
}
```

头文件的全局作用域应避免 `using namespace std;`，因为每个包含它的文件都会
受到影响，后续增加 include 也可能让原来正常的非限定名字产生歧义。
一般优先写限定名，或者在较小作用域内用 `using std::某个名字`。

## 4. namespace 和头文件、链接的关系

[counter.hpp](counter.hpp) 在 `namespace tutorial` 中声明 `next()`；
[counter.cpp](counter.cpp) 再打开它并提供定义；
[counter-main.cpp](counter-main.cpp) 包含头文件并调用 `tutorial::next()`。
编译器分别处理两个 `.cpp`，链接器再把函数调用与定义连接起来。

namespace 不会改变“非 inline 普通函数通常只能定义一次”的规则。
下面的头文件如果被多个 `.cpp` 包含，通常会在链接时报重复定义：

```cpp
// bad.hpp
namespace tutorial {
    int answer() { return 42; } // namespace 并不能消除重复定义
}
```

可以在头文件只写声明，把定义放到一个 `.cpp`；也可以在确实需要头文件定义时
写 `inline int answer() { return 42; }`，并满足 ODR 对多份定义的要求。
`#pragma once` 或 include guard 只避免同一个翻译单元内重复包含，不能解决跨
翻译单元的重复定义。翻译单元可以先理解为一个 `.cpp` 加上它包含的头文件。

命名空间作用域变量也要注意：普通可变变量可用头文件 `extern` 声明加单个
`.cpp` 定义；C++17 起，也可以用 inline 变量在头文件定义共享实体。

## 5. 匿名命名空间：只属于这个翻译单元

```cpp
namespace {
    int count = 0;
    int increment() { return ++count; }
}
```

这里的变量和函数具有内部链接。另一个 `.cpp` 写相同代码，得到的是自己那份
count 和 increment，不会与这里的实体合并。
同一个作用域内的匿名命名空间在同一翻译单元中重新打开，仍然是同一个。
规则见 [Unnamed namespaces](https://eel.is/c++draft/namespace.unnamed)。

通常把 `.cpp` 内部的辅助函数、变量、类型放进去。它也适用于类型，而命名空间
作用域的 `static` 常用于让函数或变量具有内部链接。
匿名命名空间不是权限隔离：本翻译单元中后续代码仍然可以通过名字查找使用它。

不要把需要跨文件共享的状态或类型放进头文件的匿名命名空间：每个包含它的
翻译单元都会得到独立实体。[counter.cpp](counter.cpp) 中的 count 和
[counter-main.cpp](counter-main.cpp) 中的 count 就是独立变量；运行时前者递增，
后者始终为 100。

## 6. inline namespace：默认版本的名字可以省一层

```cpp
namespace library {
    namespace v1 {
        int version() { return 1; }
    }
    inline namespace v2 {       // C++11 起
        int version() { return 2; }
    }
}
// library::version()     -> 2
// library::v2::version() -> 2
// library::v1::version() -> 1
```

它常用于库接口版本组织：默认暴露 v2，保留显式访问 v1 的路径。
这里的 inline 不要求函数调用被展开；实体仍属于 v2。
它还对 ADL 和模板特化有专门规则，并不完全等同于手写 `using namespace v2`。
详见标准草案 [Namespace definition](https://eel.is/c++draft/namespace.def)。
选择一个默认版本本身不能保证任意两个库版本的 ABI 兼容。

## 7. ADL：没有 using，为什么也能调用？

```cpp
namespace geometry {
    struct Point { int x; int y; };
    int sum(Point p) { return p.x + p.y; }
}

// 放在函数内：
geometry::Point p{3, 4};
int result = sum(p);  // ADL 找到 geometry::sum，结果为 7
```

ADL 是 Argument-Dependent Lookup，实参相关查找。对于符合条件的非限定函数
调用，除了普通名字查找，还会根据实参类型到关联的命名空间等位置找函数。
这里 p 的类型是 geometry::Point，所以 geometry 中的 sum 也成为候选。
详见标准草案 [Argument-dependent name lookup](https://eel.is/c++draft/basic.lookup.argdep)。

几个边界值得记住：

- `geometry::sum(p)` 是限定调用，不触发 ADL。
- `int` 这样的基本类型没有关联命名空间，不能靠 `sum(1)` 找到 geometry 的函数。
- `using P = geometry::Point` 不创建新类型，别名所在命名空间不会因此成为关联命名空间。
- 普通查找若找到同名非函数、类成员或块作用域函数声明，会阻止 ADL。
- 类内定义的 hidden friend 是非成员函数；没有额外命名空间声明时，普通查找
  找不到它，ADL 仍可以找到。见 [adl.cpp](adl.cpp) 中 Point 内的 `operator==()`
  友元定义，以及 [operator/README.md](../operator/README.md) 的相关说明。

通用代码经常利用 ADL 做定制：

```cpp
using std::swap;
swap(a, b);
```

这样 std::swap 提供默认候选，实参类型对应命名空间的自定义 swap 也可以参与
重载选择；直接写 `std::swap(a, b)` 则不会通过 ADL 找自定义版本。
[adl.cpp](adl.cpp) 中的 `exchange()` 演示了这一区别。

## 8. 实际组织代码时怎么选

- 项目公开接口放到具名命名空间中，例如 `project::net`。
- `.cpp` 内部辅助实体放到匿名命名空间。
- 长命名空间路径可以用 namespace 别名缩短。
- 在头文件中优先保留限定名，避免全局 using-directive。
- `detail` 只是“实现细节”的命名惯例，没有语言层面的访问限制。
- 自己的普通类型、函数放进自己的命名空间，不要随意往 std 增加声明。
  标准允许的少数定制（例如满足要求的某些 std::hash 特化）需要另查对应规则。

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
