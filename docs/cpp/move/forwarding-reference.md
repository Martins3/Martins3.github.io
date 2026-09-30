# 万能引用（转发引用）

“万能引用”（universal reference）是 Scott Meyers 推广的旧称；C++ 标准使用的名称是
**转发引用**（forwarding reference）。它不是一种新的引用类型，底层仍然是左值引用或
右值引用。特殊之处在于：模板实参推导和引用折叠共同让同一个参数既能接收左值，也能
接收右值，并记住实参原来的值类别。

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

所以不能看到 `&&` 就称为万能引用。关键不是名字叫 `T` 还是 `U`，而是这个模板参数是否
在当前位置被推导，以及形式是否恰好为 `T&&`。

## 2. 为什么它能接收所有值类别

普通模板推导有一条只对转发引用生效的规则：如果实参是左值，推导出的 `T` 本身会是
左值引用类型。

```cpp
template <typename T>
void inspect(T&& value);

Widget value;
const Widget const_value;

inspect(value);                  // T = Widget&
inspect(const_value);            // T = const Widget&
inspect(Widget{});               // T = Widget
inspect(std::move(const_value)); // T = const Widget
```

然后编译器应用引用折叠：

```text
&  + &  -> &
&  + && -> &
&& + &  -> &
&& + && -> &&
```

可以只记一句：只要组合中出现 `&`，结果就是 `&`；只有两个都是 `&&`，结果才是 `&&`。

完整推导结果如下：

| 调用实参 | 推导出的 `T` | 折叠后的参数类型 `T&&` |
| --- | --- | --- |
| `value` | `Widget&` | `Widget&` |
| `const_value` | `const Widget&` | `const Widget&` |
| `Widget{}` | `Widget` | `Widget&&` |
| `std::move(const_value)` | `const Widget` | `const Widget&&` |

转发引用不仅记住左值或右值，也保留 `const`。它并不意味着所有实参最终都能被移动；
`const Widget&&` 通常仍然只能调用接受 `const Widget&` 的拷贝操作。

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

## 4. `std::move` 和 `std::forward` 怎么选

```cpp
void set_name(std::string name)
{
    name_ = std::move(name); // name 是本函数拥有的按值参数，可以消费
}

template <typename T>
void wrapper(T&& value)
{
    target(std::forward<T>(value)); // value 代表调用者的实参，恢复原值类别
}
```

可以用所有权意图区分：

- `std::move(x)`：我决定不再依赖 `x` 当前的值，允许下一层消费它；
- `std::forward<T>(x)`：我不替调用者做决定，只恢复调用者传入时的值类别。

普通右值引用参数通常使用 `std::move`，转发引用参数通常使用 `std::forward`：

```cpp
void consume(Widget&& value)
{
    store(std::move(value));
}

template <typename T>
void pass_through(T&& value)
{
    store(std::forward<T>(value));
}
```

转发同一个右值参数多次要格外小心。第一次调用可能已经移动了对象，后续调用看到的将是
移动后的状态。

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

## 6. 实际用途

标准库的 `std::make_unique`、`std::make_shared` 和容器的 `emplace` 系列接口都依赖完美
转发。其核心结构可以简化为：

```cpp
template <typename T, typename... Args>
std::unique_ptr<T> make_unique_like(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
```

参数包中的每一个 `Args` 都独立推导和折叠，因此同一次调用可以同时正确转发左值、
`const` 左值和右值。

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

## 8. 一套判断步骤

看到函数参数 `T&&` 时，依次问：

1. `T` 是否由当前函数调用推导？
2. 是否恰好是无 cv 修饰的 `T&&`？
3. 实参是左值还是右值，从而 `T` 被推导成什么？
4. 引用折叠后，参数的声明类型是什么？
5. 函数体中是否用 `std::forward<T>(value)` 恢复值类别？

最短总结：转发引用负责**接住并记录**调用者的值类别，`std::forward` 负责在下一次调用时
**恢复**这个值类别。

运行 [forward.cpp](forward.cpp) 可以同时观察模板推导、错误转发、无条件移动和完美转发
的区别：

```sh
make run-forward
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
