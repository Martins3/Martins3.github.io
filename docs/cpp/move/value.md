## 分类

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
- **可被移动（can be moved from）**：它的资源（堆内存、文件句柄等）可以被“偷走”，因为该对象马上就要销毁或没人再用了。

|          | 有身份                                                 | 无身份                                                           |
| -------- | ------------------------------------------------------ | ---------------------------------------------------------------- |
| 不可移动 | lvalue：变量名、`*p`、前置 `++i`、字符串字面量 `"abc"` | （不存在）                                                       |
| 可移动   | xvalue：`std::move(x)`、返回 `T&&` 的函数调用          | prvalue：`42`、`a+b`、返回 `T`（按值）的函数调用、`T{}` 临时对象 |

- glvalue = “有身份”那一列：lvalue + xvalue
- rvalue = “可移动”那一行：xvalue + prvalue
- xvalue = 行列交点，唯一同时属于两边的类别（glvalue ∩ rvalue）
- lvalue = glvalue − rvalue，prvalue = rvalue − glvalue

“无身份且不可移动”一格不存在——连身份都没有，意味着再没有任何表达式能访问它指向的对象，
它马上就要销毁，资源天然可以安全搬走。所以叶子类别恰好是三个，不是两个也不是四个。

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

## 内置 `&` 的操作数必须是 lvalue

内置取地址符 `&` 的操作数要求是 lvalue，xvalue 虽有身份也不能直接取地址。

此外，引用也必须要求是 lvalue

```cpp
void ordinary_reference(int &s)
{
}

// 这个结果不可以
// ordinary_reference(12);
```

## std::move 其实什么都不移动

它只是一个类型转换，运行时是零开销的：

```cpp
// 简化版实现
template <typename T>
constexpr std::remove_reference_t<T>&& my_move(T&& t) noexcept {
    return static_cast<std::remove_reference_t<T>&&>(t);
}
```

这是程序员来告诉编译器，这里需要做一个移动了。

## `std::move` 之后源对象处于“有效但未指定”状态——能析构、能赋新值，但不能假设内容还在。

`std::move(x)` 的全部作用是：把表达式 x 从 lvalue 转换成
xvalue，从而“有资格”调用移动构造/移动赋值。
真正的移动发生在移动构造函数里（把源对象的指针拿过来，源置空）

对象的资源被移动走之后，源对象仍然是一个正常的对象，但它的具体内容通常没有保证。

先纠正一个容易混淆的点：std::move 本身不会移动任何东西，它只是让表达式可以被当作右值，从而可能调用移动构造或移动赋值。

std::string a = "hello";
std::string b = std::move(a);  // 这里的构造操作才真正执行移动

此时：

• b 的内容是 "hello"。
• a 仍然存活，可以析构，也可以重新赋值。
• 不能依赖 a 的内容仍是 "hello"，也不能通用地假设它一定为空。

“有效”意味着对象仍满足自身的基本约束，可以执行不要求额外前提的操作。例如：

a.empty();       // 可以检查是否为空
a.size();        // 可以查询当前长度
a = "world";    // 可以重新赋值

“未指定”意味着标准没有保证它具体是什么内容。它不是“内存损坏”，也不是“访问它就会产生未定义行为”。

但某些操作有前提，仍然必须先检查：

// a.front();   // 如果 a 为空，调用就违反前提

if (!a.empty()) {
    char c = a.front();  // 检查后可以调用
}

所以，更准确的理解是：移动后源对象还能用，但要把它当成“当前内容未知”的对象；重新赋值或检查状态之后，再做依赖内容的操作。

另外，这个保证主要适用于标准库类型，且具体类型可能提供更强的保证。例如，std::unique_ptr 移动构造后，源指针保证为空。自己
写的类则取决于移动操作如何实现。

## 什么时候回产生 rvalue

https://en.cppreference.com/cpp/language/value_category 中的 xvalue 提到了:

归纳成三类本质

类别 A：显式说"这个值可以搬走"

```cpp
  std::move(x)               // 函数返回 T&&
  static_cast<int&&>(x)      // cast 成右值引用
  (int&&)x                   // C 风格 cast 同理
  g()                        // g 的返回类型是 int&&
```

std::move 就是这类里的一个具体函数，别把它当成单独一种。

类别 B：从一个右值里"抠"子对象

一个整体都不要了，它的成员/元素自然也"要弃"，所以子对象也是 xvalue：

```cpp
  std::move(s).m        // 成员访问：对象是右值 → 成员是 xvalue
  S{}.m                 // prvalue 取成员（先 materialize 再取）
  std::move(arr)[0]     // 下标：数组是右值 → 元素是 xvalue
  std::move(s).*p       // 成员指针访问
```

这条最容易被忽略，但很实用：std::move(s).m 可以把结构体的单个成员搬走，而不动其他成员。

类别 C：组合传播

```cpp
  true ? std::move(x) : std::move(y)   // 结果跟着操作数走
```

条件表达式会继承操作数的值类别（粗略说：同为 glvalue 且有一个是 xvalue，结果就是 xvalue）。



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
