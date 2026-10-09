# forwarding reference


## 为什么需要 forwarding reference
### 1. 本来直接调用，一切都很自然

假设有两个重载：

void process(const std::string& s); // 读取，不消耗传入的字符串
void process(std::string&& s);      // 可以消费传入的字符串

std::string name = "hello";

process(name);            // 调用 const std::string& 版本
process(std::move(name)); // 调用 std::string&& 版本

这里调用者表达了两种不同的意图：

• name：这个对象我还要用。
• std::move(name)：允许你使用它的资源。

注意，std::move 本身没有移动资源；具体是否移动，由 process() 的实现决定。

现在你想增加一层包装，例如记录日志：

// 希望实现这样的调用：
logged_process(name);
logged_process(std::move(name));

你的要求是：加一层日志，不应该改变最终选中的 process() 重载。

Forwarding reference 就是为了支持这样的需求。

### 2. 为什么普通的传参方式不够？

尝试按值接收：

template <typename T>
void logged_process(T s)
{
    log();
    process(s);
}

问题有两个：

1. s 是一个新对象，接收参数可能发生额外的拷贝或移动。
2. s 这个表达式是左值，下一层看不到调用者原来传的是左值还是右值。

改成引用呢？

 包装函数的参数          遇到的问题
━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 T&                      不能接收普通右值
──────────────────────  ─────────────────────────────────────────────────────────────────────
 const T&                可以接收左值和右值，但下一层看到的都是 const 左值，失去了原来的区别
──────────────────────  ─────────────────────────────────────────────────────────────────────
 分别写左值、右值重载    可以实现，但多个参数会产生大量组合

例如包装函数有三个参数，每个参数都可能是左值或右值，仅这两种类别就有 8 种组合。你不希望每写一个工厂函数，都手写一堆这样的重
载。

所以需要一种机制：

> 用一个函数模板接住不同类型、不同值类别的参数，并保留足够的信息，让它们能按原来的方式继续传递。

### 3. 因此，语言给了 T&& 一条特殊推导规则

template <typename T>
void logged_process(T&& s);

这里的 T&& 是 forwarding reference。

它不是一个“什么都能装的特殊运行时对象”，而是让编译器根据实参生成不同的函数实例。

传左值：

logged_process(name);

// T 被推导成 std::string&
// T&& 折叠成 std::string&
//
// 实例化后的参数相当于：
// void logged_process(std::string& s);

传右值：

logged_process(std::move(name));

// T 被推导成 std::string
// T&& 是 std::string&&
//
// 实例化后的参数相当于：
// void logged_process(std::string&& s);

这里最关键的设计是：

传入左值时，把“这是左值”这条信息编码进 T：让 T 成为引用类型。

这样，即使进入函数之后，s 这个具名表达式都是左值，我们仍然可以通过 T 知道它原来是怎样传进来的。

### 4. std::forward 负责用这条信息继续传参

最终实现：

template <typename T>
void logged_process(T&& s)
{
    log();
    process(std::forward<T>(s));
}

可以把 std::forward<T>(s) 理解为：

static_cast<T&&>(s)

于是：

 调用者传入         保存的信息 T    std::forward<T>(s) 的结果
━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━
 name               std::string&    左值
─────────────────  ──────────────  ───────────────────────────
 std::move(name)    std::string     右值

为什么不能直接用 s？

process(s); // 无论原来怎么传，s 都是左值

为什么不能统一用 std::move(s)？

process(std::move(s)); // 无论原来怎么传，都转成右值

这两种方式都丢掉了调用者原来的区别。

所以，这套机制有明确的分工：

• Forwarding reference：接收参数，并通过类型推导保留信息。
• std::forward：利用这些信息，恢复传递给下一层时的值类别。

### 5. 为什么标准库特别需要它？

考虑：

auto p = std::make_unique<Person>(name);

真正需要 name 的是 Person 的构造函数。make_unique() 只是中间人：分配内存，再调用构造函数。

它不应擅自决定“所有参数都复制”或“所有参数都允许被移动”，而应让调用者决定：

std::make_unique<Person>(name);            // 按左值交给构造函数
std::make_unique<Person>(std::move(name)); // 按右值交给构造函数

核心实现就是：

template <typename T, typename... Args>
std::unique_ptr<T> make_unique_like(Args&&... args)
{
    return std::unique_ptr<T>(
        new T(std::forward<Args>(args)...)
    );
}

每个参数独立保留自己的传参方式，最终由 T 的构造函数决定如何处理。 这就是工厂函数、emplace()、泛型包装函数需要它的原因。

最后，你文档末尾“转发引用与引用折叠”一节里的“左值就是拷贝，右值就是移动”“所有参数都会被掏空”过于绝对，容易干扰理解。准确地说，
转发控制的是左值、右值如何传到下一层；是否发生拷贝或移动，要看下一层选中的函数以及它的实现。

理解它时，始终抓住这个场景：调用者 → 通用中间函数 → 真正处理参数的函数。Forwarding reference 的目的，是让这层中间函数能够保留
调用者的传参意图。




**转发引用**（forwarding reference)：
模板实参推导和引用折叠共同让同一个参数既能接收左值，也能接收右值，并记住实参原来的值类别。

## 1. 先判断它是不是转发引用

最常见的形式是：

```cpp
template <typename T>
void relay(T&& value);
```

这里的 `T&&` 是转发引用，因为：

1. `T` 是当前函数模板需要推导的模板参数；
2. 参数恰好写成 cv-unqualified `T&&`，即没有 `const`、`volatile`，也没有套上其他类型。

下面虽然都出现了 `&&`，但不是转发引用：

```cpp
void consume(std::string&& value);       // 类型已经确定，是普通右值引用

template <typename T>
void consume(const T&& value);           // 有 const，是普通右值引用

template <typename T>
void consume(std::vector<T>&& value);    // 不是裸 T&&

template <typename T>
class Box {
public:
    void set(T&& value);                  // T 由 Box<T> 决定，不由 set() 推导

    template <typename U>
    void emplace(U&& value);              // U 由 emplace() 推导，是转发引用
};
```

所以不能看到 `&&` 就称为 forwarding reference。关键不是名字叫 `T` 还是 `U`，而是这个模板参数是否
在当前位置被推导，以及形式是否恰好为 `T&&`。

## 3. 为什么还需要 `std::forward`

函数参数一旦有了名字，这个名字形成的表达式就是左值，即使它的声明类型是 `Widget&&`：

```cpp
template <typename T>
void relay_badly(T&& value)
{
    sink(value); // value 是具名表达式，永远按左值传递
}
```

如果改成 `std::move(value)`，问题会走向另一个极端：调用者传来的左值也会被无条件转成
右值，可能意外消耗调用者仍要使用的对象。

正确方式是：

```cpp
template <typename T>
void relay(T&& value)
{
    sink(std::forward<T>(value));
}
```

`std::forward<T>(value)` 可以近似理解为：

```cpp
static_cast<T&&>(value)
```

- 调用者传左值时，`T` 是 `Widget&`，`T&&` 折叠成 `Widget&`，结果仍是左值；
- 调用者传右值时，`T` 是 `Widget`，`T&&` 是 `Widget&&`，结果恢复为右值。

这就是“完美转发”：中间包装函数尽量保留实参的类型、cv 限定和值类别，让下一层函数
看到的参数性质与调用者直接调用时一致。

## 5. `auto&&` 也是同一套规则

当 `auto&&` 会根据初始化表达式推导类型时，它也能充当转发引用：

```cpp
Widget value;
const Widget const_value;

auto&& a = value;         // Widget&
auto&& b = const_value;   // const Widget&
auto&& c = Widget{};      // Widget&&，临时对象的生命周期延长到 c 的作用域结束
```

常见场景有泛型 lambda 和范围 `for`：

```cpp
auto call = [](auto&& value) {
    target(std::forward<decltype(value)>(value));
};

for (auto&& element : range) {
    // 适配返回普通引用或代理引用的 range
}
```

泛型 lambda 没有可写出来的 `T`，因此用 `decltype(value)` 作为 `std::forward` 的模板参数。

一个特殊例外是花括号初始化列表：

```cpp
auto&& values = {1, 2, 3}; // 推导为 std::initializer_list<int>&&，但不称为转发引用

template <typename T>
void relay(T&&);

// relay({1, 2, 3});       // 错误：无法从花括号列表推导 T
```

需要显式构造目标类型，或单独提供 `std::initializer_list<T>` 重载。

## 7. 常见陷阱

### 转发引用重载可能抢走其他重载

```cpp
void log(const std::string& text);

template <typename T>
void log(T&& value);

std::string text;
log(text); // 模板得到精确的 std::string&，可能优先于 const std::string&
```

转发引用可以生成非常精确的匹配，因此容易劫持本来期待的重载。公共接口中可优先考虑
按值传递、不同函数名，或用 concepts/SFINAE 约束模板的可接受类型。

### “完美”不代表任何语法都能转发

以下情况缺少可供模板直接推导的单一类型，或者不能绑定转发引用：

- `{1, 2, 3}`：花括号列表本身没有普通类型；
- 重载函数名：在选定具体重载前没有唯一类型；
- 位域：不能绑定到普通非 `const` 引用；
- 用 `0` 或 `NULL` 表示空指针：转发后保留的是整数类型，应使用 `nullptr`。

解决思路通常是先明确类型，再传给转发接口：

```cpp
relay(std::vector<int>{1, 2, 3});
relay(static_cast<void (*)(int)>(overloaded_function));
relay(nullptr);
```

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
