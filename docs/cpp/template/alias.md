# cpp template : using 与 typedef
<!-- 84b7d5b5-047c-4ed3-8119-81a95dc94851 -->

`using` 在 C++ 中有三种互不相关的用法，谈 `using` 和 `typedef` 的区别时，只有第三种才是同类项:

1. using-directive: `using namespace std;`，把整个命名空间引入当前作用域
2. using-declaration: `using std::cout;` / `using Base::f;` / `using Base::Base;` / `using enum Color;`，把某个名字引入当前作用域
3. alias-declaration: `using T = int;`，类型别名，也就是本文的主角

前两种 `typedef` 完全没有对应物，详见文末。第三种里，非模板场景两者等价，真正的分水岭是**别名模板(alias template)**，`typedef` 做不到。

## 语法形式

```cpp
typedef int              Int;
typedef int             *IntPtr;
typedef int              Arr[10];
typedef void             (*Func)(int, double);   // 名字埋在声明符中间
typedef void             (Screen::*GetFn)();
typedef std::vector<int> IntVec;

using Int    = int;
using IntPtr = int *;
using Arr    = int[10];
using Func   = void (*)(int, double);            // 名字在最左，右边是完整 type-id
using GetFn  = void (Screen::*)();
using IntVec = std::vector<int>;
```

区别只在可读性: `using` 把别名放在最左边，右边可以照抄类型本身的写法；`typedef` 的名字被塞进声明符里，遇到函数指针、数组、成员指针，基本需要手工拆解。这也是各种规范推荐 `using` 的首要理由。

## 非模板场景: 完全等价

对于不涉及模板参数的情形，两者语义完全相同，都是给类型起名字，不创建新类型:

```cpp
typedef int I;
using J = int;
static_assert(std::is_same<I, J>::value, "");
static_assert(std::is_same<I, int>::value, "");  // 别名是透明的
```

由此推出的共同性质:

- 别名不产生新类型，所以不能靠别名重载、不能靠别名特化、别名不参与 ADL
- 别名可以互相定义: `typedef I I2;` 合法
- 同一作用域可以**重复声明**同一个别名（别名指的是同一个类型时不算重定义）:
  `typedef int K; typedef int K;` 与 `using J = int; using J = int;` 在 g++ 15.2 / clang++ 21.1 下均通过；但换成不同类型就是错误
- 别名不能被前向声明，目标类型必须先可见
- 访问控制取决于别名自身的可访问性，以及**别名声明处**目标类型是否可访问:
  `class A { struct Impl {}; public: using I = Impl; };` 之后 `A::I` 在类外可用；
  而 `class A { public: typedef Impl I; struct Impl {}; };` 会因为声明处 `Impl` 还不可见而失败

C 兼容场景下 `typedef` 仍是唯一选择:

```c
typedef struct S { int a; } S;  // C 中 struct tag 和类型名分属两个命名空间
typedef struct T T;             // C++ 中也常这么写
```

## 别名模板

`typedef` 无法带模板参数。想给 `std::vector<T>` 起个短名字，C++98 只能绕道 class template + 嵌套 `::type`:

```cpp
// C++98 的老办法
template <typename T> struct MyVec { typedef std::vector<T> type; };

template <template <typename> class C> struct X { typename C<int>::type c; };
X<MyVec> x;   // 使用侧还要写 typename ... ::type
```

`using` 直接支持:

```cpp
template <typename T> using MyVec = std::vector<T>;   // 别名模板
MyVec<int> v;

template <typename T> using Ptr = MyVec<T> *;
```

标准库 `_t` 后缀的别名家族（C++14 起）就是靠这个把 trait 的 `::type` 擦掉的:

```cpp
// 以前
typename std::enable_if<std::is_integral<T>::value, T>::type
// 现在
std::enable_if_t<std::is_integral<T>::value, T>

std::remove_reference_t<T>   std::conditional_t<B, T, F>   std::void_t<Ts...>
```

本仓库中的实例:

- `template/alias.cpp`: `template <typename T> using Coord = Pair<T>;`，以及 C++20 alias template 的 CTAD
- `template/SFINAE-3.cpp`: `template <typename... Ts> using void_t = void;`
- `template/SFINAE.cpp`: `template <typename T> using TypeSinkT = typename TypeSink<T>::Type;`
- `code/basic/modern/using.cpp`: `using abc = A::type;` 等基础用法

别名模板和依赖名消歧经常一起出现:

```cpp
using T1 = typename X<T>::type;                                  // 目标类型是依赖名
using T2 = std::pointer_traits<int *>::template rebind<int *>;   // 依赖名里的模板成员
```

`code/basic/modern/using.cpp` 里这两个例子都有。

### 限制一: 别名模板不可特化

全特化和偏特化都不允许（clang 报 `partial specialization of alias templates is not permitted`）:

```cpp
template <class T> using Ptr = T *;
template <class T> using Ptr<T *> = T **;   // 错误
```

要按类型分支，只能回到 class template + `::type`，或者用 `std::conditional_t` 一类的工具组合。

### 限制二: 作为模板模板参数的实参

历史上把别名模板传给模板模板参数是非法的，CWG 1286 之后被允许，且作为 DR 追溯生效，所以现在 `-std=c++11` 也放行:

```cpp
template <typename T> using MyVec = std::vector<T>;
template <template <typename> class C> struct X { C<int> c; };
X<MyVec> x;   // g++ 15.2 / clang++ 21.1 实测通过
```

## 别名的透明性

别名不创造新类型，下面这些都不行:

```cpp
using Meters = double;
using Seconds = double;
// 无法区分两者，也无法为 Meters 单独重载 operator+
// 想要强类型必须写 struct / enum class
```

也不要把别名当作特化的锚点:

```cpp
template <class T> struct is_mine : std::false_type {};
template <> struct is_mine<Meters> : std::true_type {};  // 特化的其实是 double
```

## using-declaration

这部分和别名无关，但 `using` 经常一起出现，容易混淆。`typedef` 没有任何对应能力:

```cpp
// 1. 引入单个名字，替代 using namespace
using std::cout;

// 2. 恢复被隐藏的基类 overload
struct Base { void f(int); void f(double); };
struct Derived : Base {
    void f(int) override;   // 只 override 一个，Base::f(double) 被隐藏
    using Base::f;          // 把 Base::f(double) 带回来
};

// 3. 修改基类成员的可见性（protected -> public）
struct D2 : Base { public: using Base::f; };

// 4. 继承构造函数（拷贝、移动、默认构造函数不会被继承）
struct D3 : Base { using Base::Base; };

// 5. 在模板中引入依赖基类的成员
template <class T> struct D4 : Base<T> {
    using typename Base<T>::type;   // 类型成员需要 typename
    using Base<T>::data;            // 非类型成员不需要，见 template/dependent-name-2.cpp
};

// 6. C++20
enum class Color { red, green };
using enum Color;   // 之后可以直接写 red，不用 Color::red
```

## 对比表

| 场景 | typedef | using |
| --- | --- | --- |
| 普通类型别名 | `typedef int I;` | `using I = int;` |
| 函数指针 | `typedef void (*F)(int);` | `using F = void (*)(int);` |
| 数组成员 | `typedef int A[10];` | `using A = int[10];` |
| 成员函数指针 | `typedef void (C::*M)();` | `using M = void (C::*)();` |
| 别名模板 | 不支持，需 class template + `::type` | `template <class T> using V = ...;` |
| 别名模板特化 | 不适用 | 不支持 |
| 重复声明同一别名 | 允许 | 允许 |
| 引入命名空间/基类名字 | 不支持 | using-declaration |
| 继承构造函数 | 不支持 | `using Base::Base;` |
| C 兼容 | 是，唯一选择 | 否 |
| 创建新类型 | 否 | 否 |

## 风格结论

- Core Guidelines T.43: Prefer `using` over `typedef` for defining aliases
- Core Guidelines T.42: Use template aliases to simplify notation and hide implementation details
- Effective Modern C++ Item 9: 同一结论

理由三条: 可读性（名字在前）、别名模板（`typedef` 做不到）、语义一致。仍需 `typedef` 的场合基本只剩 C / C++98 兼容、C 头文件，以及团队编码规范强制。

参考:

- https://stackoverflow.com/questions/10747810/what-is-the-difference-between-typedef-and-using-in-c11
- https://en.cppreference.com/w/cpp/language/type_alias
- https://en.cppreference.com/w/cpp/language/using_declaration

## human
2026-09-12 : 这里对吗?

- using 和 typedef 在非模板场景下是相同的，真正的差异是别名模板(alias template)，完整整理见 [template/alias.md](template/alias.md):
  - https://stackoverflow.com/questions/10747810/what-is-the-difference-between-typedef-and-using-in-c11

- using
  - 使用 using 除了可以 typedef，和 namespace
  - 而且 using 可以修改 parent 的 field / method 对外的可见性
    - override 一个 overload function 的时候，所有的全部都会被 override, 但是靠 using 可以把 parent 的重新带回来
    - https://stackoverflow.com/questions/888235/overriding-a-bases-overloaded-function-in-c
  - using-declaration、别名模板、和 typedef 的完整对比: [template/alias.md](template/alias.md)


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
