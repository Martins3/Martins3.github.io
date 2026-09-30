## 7. 一个反直觉规则：有名字的右值引用是左值

void consume(std::string&& value)
{
    use(value);
}

虽然 value 的声明类型是 std::string&&，但表达式 value 是左值。

因为它有名字，可以被反复访问：

value;
value;
value;

因此：

void consume(std::string&& value)
{
    target(value);            // 作为左值传递
    target(std::move(value)); // 作为右值传递
}

一定要区分：

- 变量的类型可能是 T&&。
- 使用变量名形成的表达式仍然是左值。

这个规则也是为什么完美转发需要 std::forward。

## 8. T&& 不一定是普通右值引用

下面是普通右值引用：

void f(std::string&& value);

这里类型已经确定为 std::string，只接受右值。

下面这种特殊形式叫 forwarding reference，过去常称 universal reference：

template<class T>
void wrapper(T&& value);

它成为转发引用需要同时满足：

- 形状正好是 T&&；
- T 由当前调用推导出来；
- 没有 const 等修饰。

例如下面不是转发引用：

```cpp
template<class T>
void f(const T&&); // 普通右值引用

template<class T>
struct Box {
    void set(T&&); // T 由 Box 决定，也不是转发引用
};
```

## 9. 转发引用为什么既能接左值又能接右值

template<class T>
void wrapper(T&& value);

传入左值：

std::string s;
wrapper(s);

模板推导得到：

T = std::string&

参数类型经过引用折叠：

T&&
= std::string& &&
= std::string&

传入右值：

wrapper(std::string{});

推导得到：

T = std::string
T&& = std::string&&

引用折叠规则可以简化记成：

&  + &  -> &
&  + && -> &
&& + &  -> &
&& + && -> &&

只要出现一个 &，结果就是 &。

因此 T&& 在模板推导的配合下，可以记住调用者传来的是左值还是右值。

———

## 10. 为什么完美转发需要 std::forward

假设有两个重载：

void target(const std::string&)
{
    std::cout << "lvalue\n";
}

void target(std::string&&)
{
    std::cout << "rvalue\n";
}

错误的包装器：

template<class T>
void wrapper(T&& value)
{
    target(value);
}

无论调用者传什么，value 都是一个有名字的变量，所以 target(value) 总是按左值处理：

std::string s;

wrapper(s);             // lvalue
wrapper(std::string{}); // 仍然是 lvalue

正确写法：

template<class T>
void wrapper(T&& value)
{
    target(std::forward<T>(value));
}

std::forward<T> 会根据模板推导出来的 T 恢复调用者原来的值类别：

- 原来传左值，继续转发为左值。
- 原来传右值，继续转发为右值。

可以近似理解为：

static_cast<T&&>(value)

当传入左值时：

T = std::string&
T&& = std::string& && = std::string&

所以结果是左值。

当传入右值时：

T = std::string
T&& = std::string&&

所以结果是右值。

所谓“完美转发”，就是尽量原样保留调用者传入参数的：

- 类型；
- const 属性；
- 左值或右值属性。

———

## 11. std::move 和 std::forward 的本质区别

std::move(value)

无条件地表示：

> 从现在开始，把 value 当成可以被消费的右值。

std::forward<T>(value)

有条件地表示：

> 调用者原来传右值，我就传右值；调用者原来传左值，我就传左值。

因此一般规则是：

- 普通代码中确定不再需要某对象：使用 std::move。
- 转发引用参数继续传给其他函数：使用 std::forward<T>。

void set_name(std::string name)
{
    name_ = std::move(name);
}

template<class T>
void construct(T&& value)
{
    object_.set(std::forward<T>(value));
}

———

## 12. 为什么容器喜欢完美转发

考虑两种插入：

vector.push_back(Widget(args...));
vector.emplace_back(args...);

emplace_back 大致是：

template<class... Args>
void emplace_back(Args&&... args)
{
    new (storage) Widget(std::forward<Args>(args)...);
}

它接受任意数量的左值或右值，并把每个参数原来的值类别转发给 Widget 的构造函数。

这使得对象可以直接在容器存储区中构造。

———

## 13. 几条最实用的规则

3. 不要随便 move 一个 const 对象。

const std::string s = "hello";
std::string t = std::move(s);

这里通常发生拷贝，因为移动构造一般需要修改源对象，参数是 std::string&&，无法绑定 const std::string。

4. 通常不要这样返回局部变量：

Widget make_widget()
{
    Widget result;
    return std::move(result);
}

应写成：

Widget make_widget()
{
    Widget result;
    return result;
}

这样编译器可以执行 NRVO；显式 std::move 反而可能阻止它。

5. 右值引用参数本身不会自动移动：

void f(Widget&& value)
{
    Widget copy = value;            // copy
    Widget moved = std::move(value); // move
}

6. 移动构造函数通常应标记 noexcept：

Widget(Widget&& other) noexcept;

例如 std::vector 扩容时，为了异常安全，可能只在移动构造是 noexcept 时选择移动，否则退回拷贝。

———

最后把整个模型压缩成四句话：

右值：这个表达式所代表的对象可以被消费。
右值引用：让函数能够识别并接收这种表达式。
std::move：无条件把表达式标记为可以被消费。
std::forward：按照调用者原来的左值/右值属性继续传递。

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
