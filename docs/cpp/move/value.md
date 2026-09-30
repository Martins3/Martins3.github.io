## 1. 直观理解（C 时代的粗糙定义）

最早的说法是：能放在赋值号左边的是 lvalue，只能放在右边的是 rvalue。

```c
int a = 42;   // a 是 lvalue（有名字、有地址），42 是 rvalue（临时值）
a = a + 1;    // a+1 的结果存在寄存器里，没有地址，是 rvalue
```

这个定义在 C 里够用，但在 C++ 里会失效：

```cpp
const int x = 10;   // x 不能放在赋值号左边，但它显然是 lvalue（能取地址 &x）
std::string s = f(); // 函数返回的临时对象会被构造进 s，它“马上要消失”，资源可以被偷走
```

所以 C++11 引入了更精确的分类。

## 2. C++11 的精确分类：不是树，是二维表

每个表达式（注意：值类别是**表达式**的属性，不是对象的属性，也不是类型的属性）恰好属于三个基本类别之一：lvalue、xvalue、prvalue。glvalue 和 rvalue 不是并列的第四、五个类别，而是两个总称，而且这两个总称**相交**——xvalue 同时属于两边。所以分类图画不成一棵树，xvalue 有两个父节点：

```
            expression
           /          \
      glvalue          rvalue
      /      \         /     \
  lvalue      \       /     prvalue
               \     /
                xvalue
```

两个维度定义：

- **有身份（has identity）**：能判断两个表达式指的是不是同一个对象，通俗近似是“有名字”。
注意：内置取地址符 `&` 的操作数要求是 lvalue，xvalue 虽有身份也不能直接取地址（demo 里有验证）。
- **可被移动（can be moved from）**：它的资源（堆内存、文件句柄等）可以被“偷走”，因为该对象马上就要销毁或没人再用了。

|          | 有身份                                                 | 无身份                                                           |
| -------- | ------------------------------------------------------ | ---------------------------------------------------------------- |
| 不可移动 | lvalue：变量名、`*p`、前置 `++i`、字符串字面量 `"abc"` | （不存在）                                                       |
| 可移动   | xvalue：`std::move(x)`、返回 `T&&` 的函数调用          | prvalue：`42`、`a+b`、返回 `T`（按值）的函数调用、`T{}` 临时对象 |

- glvalue = “有身份”那一列：lvalue + xvalue
- rvalue = “可移动”那一行：xvalue + prvalue
- xvalue = 行列交点，唯一同时属于两边的类别（glvalue ∩ rvalue）
- lvalue = glvalue − rvalue，prvalue = rvalue − glvalue

“无身份且不可移动”一格不存在——连身份都没有，意味着再没有任何表达式能访问它指向的对象，它马上就要销毁，资源天然可以安全搬走。所以叶子类别恰好是三个，不是两个也不是四个。

名字的来历（全是历史包袱，一层层叠出来的）：

- lvalue / rvalue 这两个词来自 C：赋值号左边 / 右边的值。C++03 之前只有这个二分法。
- C++11 需要二维分类，但 lvalue/rvalue 已遍布标准、教材和编译器诊断，不能推翻重来，于是：
  - **rvalue** 被重新定义为“可被移动”一侧的总称：xvalue + prvalue。词形不变，含义扩容；
  - 老分类里“纯粹的临时值”那部分起名 **prvalue**（pure rvalue）——它不直接叫 rvalue，是因为 rvalue 这个名字已经被征用为总称了；
  - “有身份但允许被掏空”的新物种起名 **xvalue**（expiring value）；
  - **lvalue** 名字和含义都保留，收窄为“有身份且不可移动”。
- **glvalue**（generalized lvalue，泛化左值）是为“有身份”一侧新造的总称：lvalue + xvalue。

所以 lvalue 和 xvalue 的关系：两者都是 glvalue，都指着一个真实的对象（下面 demo 里 `std::move(s)` 和 `s` 的 `.data()` 指针相同），
区别只在资源允不允许被搬走。`std::move(x)` 不创建新对象，它和 x 指的是同一个对象，只是把访问这个对象的表达式从 lvalue 换成了 xvalue。xvalue 是唯一同时落在两组里的类别（既是 glvalue 又是 rvalue）。

## 3. 用代码逐个确认

```cpp
#include <string>
#include <utility>

std::string make();

void demo() {
    std::string a = "hello";
    std::string b = a;              // a 是 lvalue：拷贝构造
    std::string c = make();         // make() 是 prvalue：C++17 起直接在 c 里构造，零拷贝
    std::string d = std::move(b);   // std::move(b) 是 xvalue：移动构造，b 的内容被偷走

    int&& r = 42;                   // 关键陷阱，见下文
}
```

取地址的验证——内置 `&` 的操作数必须是 lvalue（g++ 和 clang++ 都实测过）：

```cpp
&a;                    // OK：a 是 lvalue
&make();               // 编译错误：prvalue 没有地址
&std::move(b);         // 编译错误：xvalue 也不是 lvalue，一样不行
```

那 xvalue 的“有身份”怎么观察？用成员访问：`std::move(b).data()` 和 `b.data()` 返回同一个指针——move 之后访问的还是原来那个对象，`std::move` 没有创建任何新东西。

### demo：把值类别交给编译器判定

[lvalue-rvalue.cpp](lvalue-rvalue.cpp) 的原理：`decltype((expr))`（双括号，把括号里的东西当表达式求值类别）按值类别把表达式编码成三种类型——lvalue 编码为 `T&`，xvalue 编码为 `T&&`，prvalue 编码为 `T`——再用模板偏特化把名字和两个维度打印出来：

```text
== leaf categories ==
i                                      => lvalue   identity=1 movable=0
*p                                     => lvalue   identity=1 movable=0
++i                                    => lvalue   identity=1 movable=0
s                                      => lvalue   identity=1 movable=0
s[0]                                   => lvalue   identity=1 movable=0
"hello"                                => lvalue   identity=1 movable=0
i++                                    => prvalue  identity=0 movable=1
i + 1                                  => prvalue  identity=0 movable=1
42                                     => prvalue  identity=0 movable=1
std::string("temporary")               => prvalue  identity=0 movable=1
make()                                 => prvalue  identity=0 movable=1
std::move(s)                           => xvalue   identity=1 movable=1
static_cast<std::string &&>(s)         => xvalue   identity=1 movable=1
expiring()                             => xvalue   identity=1 movable=1

== xvalue denotes the same object as the lvalue ==
s.data()             = 0x7fffbd11d2a0
std::move(s).data()  = 0x7fffbd11d2a0

== a named rvalue reference is an lvalue ==
r                                      => lvalue   identity=1 movable=0
r = 42
s                                      => lvalue   identity=1 movable=0
bind(std::string&)
bind(std::string&&)

== overload resolution ==
bind(std::string&)
bind(std::string&&)
bind(std::string&&)
bind(const std::string&)
bind(const std::string&)
```

（地址每次运行不同，关键在于同一节内两个地址相同。）

值得盯住的几行：

- `"hello"` 是 lvalue：字符串字面量是仅有的“字面量却是 lvalue”的特例（能取地址）；其他字面量如 `42` 是 prvalue。
- `++i` 是 lvalue 而 `i++` 是 prvalue：前置自增返回引用，后置自增返回临时值。
- `std::move(s)` 是 xvalue，且 `std::move(s).data() == s.data()`：move 只是换了表达式类别，对象还是那个对象。
- 第三节的 `r`（声明类型 `int&&`）和 `take()` 内部的形参 `s`（声明类型 `std::string&&`）作为表达式都是 lvalue：`bind(s)` 落到左值引用重载，`bind(std::move(s))` 才落到右值引用重载。
- 最后一组 `bind(std::move(cs))`：`const std::string` 的 xvalue 类型是 `const std::string&&`，右值引用重载要求非 const 而不可行，最终落到 `const std::string&`——值类别和 const 是两个独立的维度。

## 4. 最重要的一个坑：具名的右值引用是 lvalue

这是理解移动语义的钥匙：

```cpp
void f(std::string&& s) {   // s 的类型是 std::string&&（右值引用）
    std::string t = s;      // 但 s 这个表达式是 lvalue！这里是拷贝
    std::string u = std::move(s); // 想移动必须再 move 一次
}
```

规则：**有名字的表达式就是 lvalue，跟它的类型是 `T&` 还是 `T&&` 无关**。类型（`int&&`）描述的是“它能绑定到谁”，值类别（lvalue/rvalue）描述的是“这个表达式本身是什么”。右值引用类型的具名变量，作为表达式是 lvalue——因为它有名字，后面还可能被人继续用，编译器不敢偷它的资源。

## 5. std::move 其实什么都不移动

它只是一个类型转换，运行时是零开销的：

```cpp
// 简化版实现
template <typename T>
constexpr std::remove_reference_t<T>&& my_move(T&& t) noexcept {
    return static_cast<std::remove_reference_t<T>&&>(t);
}
```

`std::move(x)` 的全部作用是：把表达式 x 从 lvalue 转换成 xvalue，从而“有资格”调用移动构造/移动赋值。真正的移动发生在移动构造函数里（把源对象的指针拿过来，源置空）。所以：

- 对没有移动构造的类型（如 `int`）`std::move` 毫无效果，就是一次拷贝。
- `std::move` 之后源对象处于“有效但未指定”状态——能析构、能赋新值，但不能假设内容还在。

## 6. 引用的绑定规则

```cpp
void g(std::string& a);         // 左值引用：只绑定 lvalue
void h(std::string&& b);        // 右值引用：只绑定 rvalue（prvalue 和 xvalue）
void k(const std::string& c);   // const 左值引用：通吃，lvalue 和 rvalue 都能绑

std::string s;
g(s);      // OK
g(make()); // 错误
h(s);      // 错误
h(make()); // OK
h(std::move(s)); // OK：move 后变 xvalue
k(s); k(make()); // 都 OK
```

历史上 `const T&` 就是为了让临时对象能传参而设计的（并会把临时对象的生命周期延长到引用的作用域结束）。C++11 的右值引用解决了它的局限：const 引用绑定的临时对象你没法改，而右值引用可以放心地掏空它。

## 7. 转发引用与引用折叠

下面这个 `T&&` 看起来是右值引用，其实不是：

```cpp
template <typename T>
void f(T&& x);        // 转发引用（forwarding reference，旧称 universal reference）

template <typename T>
void g(std::vector<T>&& x);  // 这才是普通右值引用：不是裸的 T&&
```

区分标准：`T&&` 中的 `T` **处于被推导的位置**（裸模板参数、`auto&&`）才是转发引用。传 lvalue 时 `T` 被推导为 `std::string&`，经过引用折叠：

```
&  &   -> &
&  &&  -> &
&& &   -> &
&& &&  -> &&
```

于是 `f(x)` 传 lvalue 时 `T&&` 折叠成 `std::string&`，传 rvalue 时就是 `std::string&&`——同一个模板两种类型都能收。

配套的 `std::forward` 负责把值类别还原回去（有条件的 move）：

```cpp
template <typename T, typename... Args>
std::unique_ptr<T> make(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
    // 调用方传的是 lvalue 就继续按 lvalue 转发（拷贝），传的是 rvalue 就按 rvalue 转发（移动）
}
```

如果这里写成 `std::move(args)...`，所有参数都会被掏空，调用方原来还想用的对象就遭殃了；如果不 cast 直接传 `args...`，又会全部退化为拷贝。

更完整的判定规则、推导表、`auto&&` 和常见陷阱见 [forwarding-reference.md](forwarding-reference.md)。
行为演示见同目录的两个文件：[forward.cpp](forward.cpp) 展示模板推导以及直接传参、`std::move`、`std::forward` 的区别；
[move-rvalue.cpp](move-rvalue.cpp) 的 `forwarding_reference_demo` 展示转发引用的推导过程
（传 lvalue 时 `T` 被推导为引用类型），它的 `named_rvalue_reference_demo` 和 `copy_and_move_demo` 则演示第 4、5 节的内容。

## 8. 实战建议

1. **按值返回局部变量时不要写 `return std::move(local);`**。C++17 起 prvalue 直接就地构造（guaranteed copy elision）；返回局部变量也早有 NRVO 优化。写了 `std::move` 反而把返回语句变成 xvalue，编译器不再适用 NRVO，妥妥的负优化。规则：`return local;` 裸写。
2. **成员函数可以按值类别限定**：`void f() &&;` 表示只能对右值调用（如 `std::optional` 的 `value() &&` 返回 `T&&`）。读库代码时会遇到。
3. **`decltype` 的双括号规则**：`decltype(x)` 是变量声明时的类型；`decltype((x))` 对 lvalue 变量会给出引用（`int&`）——因为 `(x)` 是一个表达式，decltype 对 lvalue 表达式返回左值引用。泛型代码里容易踩。
4. 判断一个表达式是什么类别，最快的办法就是问两个问题：**有身份吗（identity）？允许掏空吗（movable）？**——两个都“是”就是 xvalue，只前者是 lvalue，只后者是 prvalue。（identity 不等于“能取地址”：内置 `&` 要求操作数是 lvalue，xvalue 取不了地址。）

一句话收尾：类型系统的 `&`/`&&` 管“能绑定谁”，值类别管“这个表达式是什么”；`std::move` 是从 lvalue 到 xvalue 的桥，`std::forward` 是在转发时恢复调用方的原始值类别。动手验证就用 [lvalue-rvalue.cpp](lvalue-rvalue.cpp)。

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
