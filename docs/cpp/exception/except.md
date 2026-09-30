# cpp exception 机制
<!-- 4b2e42a3-2b86-4de3-a5dc-59bd1efd447e -->

https://en.cppreference.com/w/cpp/error/exception
https://stackoverflow.com/questions/8480640/how-to-throw-a-c-exception

简单来说中将 error class 都列举出来了
https://cplusplus.com/reference/exception/exception/

https://learn.microsoft.com/en-us/cpp/cpp/errors-and-exception-handling-modern-cpp
提供了一堆基本使用规则，只能说，感受不深
	- https://learn.microsoft.com/en-us/cpp/cpp/how-to-design-for-exception-safety?view=msvc-170

- 使用上: 先区分 exception 机制和普通的概念的 exception ，不是为了捕获
	1. assert(false);
	2. segv
而是为了捕获 C++ 语言级别的。
- 为了将

## 异常机制基本流程

执行 `throw` 时大致发生以下步骤：

1. 创建异常对象。
2. 从当前函数开始，沿调用栈寻找类型匹配的 `catch`。
3. 每退出一层函数，就销毁这一层已经构造的局部对象，这叫 stack unwinding（栈展开）。
4. 找到匹配的 `catch` 后执行处理逻辑。
5. 如果完全找不到匹配的处理器，调用 `std::terminate()`。

异常可以跨越多层调用，因此中间函数不需要为每个错误都显式返回错误码：

```text
main -> service -> database -> parse -> throw
```

现代 C++ 实现通常采用 zero-cost exception 模型：
正常路径通常只需要少量额外支持，真正抛异常时才付出较大的查找和栈展开成本。
因此异常适合表示异常情况，不适合代替高频的普通分支控制。

## 触发异常后如何执行

下面以 Linux 上 GCC/Clang 常用的 Itanium C++ ABI 为例。MSVC 的具体函数名
不同，但整体过程类似。假设调用链如下：

```cpp
void parse()
{
    Token token;
    throw ConfigError("invalid port");
}

void load()
{
    Buffer buffer;
    parse();
}

void start()
{
    Connection connection;
    load();
}

int main()
{
    try {
        start();
    } catch (const ConfigError &error) {
        std::cout << error.what() << '\n';
    }
}
```

运行时的调用栈是：

```text
main -> start -> load -> parse -> throw
```

异常处理可以概括为：

```text
构造异常对象
    -> 第一阶段：寻找匹配的 catch
    -> 第二阶段：展开调用栈并析构局部对象
    -> 进入 catch
    -> 销毁异常对象
```

### 编译器预先生成异常元数据

编译器不会在每个可能抛异常的函数调用后生成类似下面的隐藏检查：

```cpp
parse();
if (exception_occurred)
    goto error;
```

它会在可执行文件中生成异常表和栈展开信息，记录：

- 当前机器指令属于哪个函数和调用点。
- 如何恢复上一层函数的栈指针和寄存器。
- 离开当前作用域时需要销毁哪些对象。
- 当前函数有哪些 `catch`，分别匹配什么类型。
- 发生异常时应该进入哪个清理代码块。

GCC 生成的相关结构通常包括 `.eh_frame`、`.gcc_except_table` 和 landing pad。
正常执行时，这些表通常不会被读取；函数调用正常返回后直接执行下一条指令。
这就是 zero-cost exception 中“正常路径成本接近零”的来源。它并不表示完全
没有成本：异常元数据会增大二进制，可能抛异常的调用也可能限制某些优化。

### 构造异常对象

执行下面的语句时：

```cpp
throw ConfigError("invalid port");
```

异常对象不能只放在 `parse()` 的普通栈帧中，因为 `parse()` 马上就要退出。
运行时会申请独立的异常存储空间，在其中构造对象，并记录其类型和析构函数。
概念上类似：

```cpp
void *storage = __cxa_allocate_exception(sizeof(ConfigError));
new (storage) ConfigError("invalid port");
__cxa_throw(storage, &typeid(ConfigError), destructor);
```

因此退出 `parse()`、`load()` 和 `start()` 后，异常对象仍然存在。

### 第一阶段：寻找处理器

`__cxa_throw()` 会进入底层 unwinder，从抛出点沿调用栈向上搜索：

```text
parse：没有 catch
load：没有 catch
start：没有 catch
main：catch (const ConfigError &) 匹配
```

这一阶段只查找处理器，不销毁局部对象。每经过一层，运行时都会查询异常表，
根据异常对象的 RTTI 信息判断 `catch` 是否匹配。派生类异常可以匹配相应的
基类引用，`catch (...)` 可以匹配任意异常。

运行时先完整搜索，是为了确认确实存在处理器，再进行不可逆的栈展开。

### 第二阶段：展开调用栈

找到 `main()` 中的处理器后，运行时重新从抛出点开始逐层退出函数。前面例子
中的析构顺序是：

```text
Token::~Token()
Buffer::~Buffer()
Connection::~Connection()
```

编译器为可能需要清理的调用点生成 landing pad，其逻辑概念上类似：

```cpp
cleanup:
    connection.~Connection();
    resume_current_exception();
```

landing pad 不在正常控制流上，只在栈展开时执行。这也是 RAII 与异常能够良好
配合的原因：智能指针、锁和文件对象等局部资源会在传播异常时自动释放。

只有已经完整构造成功的对象才会调用自身的析构函数。如果对象的构造函数
抛异常，对象本身尚未构造完成，只会逆序销毁已经构造成功的成员和基类。

### 进入 catch 并销毁异常对象

栈展开到 `main()` 后，运行时将控制权转移到匹配的 `catch`。底层通常通过
`__cxa_begin_catch()` 标记异常已经进入处理状态。这里使用引用捕获：

```cpp
catch (const ConfigError &error)
```

所以 `error` 直接引用最初保存在异常存储区中的对象，不需要再复制一份。
`catch` 结束后会调用类似 `__cxa_end_catch()` 的运行时接口；如果异常没有
再次抛出，最后销毁异常对象并释放异常存储。

在 `catch` 中单独使用：

```cpp
throw;
```

会继续传播当前异常，不会创建新的异常对象。`throw error;` 则是重新以表达式
抛出，可能复制异常并造成对象切片，因此传播当前异常通常使用 `throw;`。

### 会调用 std::terminate() 的边界

以下情况不能继续正常传播异常：

1. 整条调用链都没有匹配的 `catch`。不要依赖这种情况下所有局部对象都会被
   析构；调用 `std::terminate()` 前是否展开栈属于实现相关行为。
2. 异常试图逃出 `noexcept` 函数。它不会被更外层的 `catch` 正常接住，而是
   调用 `std::terminate()`。
3. 已经有异常正在展开栈时，某个析构函数又抛出第二个异常。运行时无法同时
   处理两套展开过程，因此调用 `std::terminate()`。这也是析构函数通常必须
   保持 `noexcept` 的重要原因。

所以真正抛出一次异常，需要创建异常对象、按异常表搜索调用栈、执行第二遍
栈展开、调用沿途析构函数，最后跳转到 `catch`。正常路径不执行这些工作，
而抛出路径的代价很高。

3. 异常安全保证
设计可能抛异常的函数时，通常讨论三种保证：
	- **基本保证**：不会泄漏资源，对象仍处于有效状态。
	- **强保证**：操作失败时状态保持不变，类似事务回滚。
	- **不抛保证**：函数不会把异常传播出去，即 `noexcept`。
容器扩容时会尽量维护强保证，但这取决于元素的复制和移动操作是否可能抛异常。

## noexcept

```cpp
void f() noexcept;
```

`noexcept` 是一个契约：`f()` 不应该让异常传播到函数外。
如果异常从 `f()` 逸出，程序会直接调用 `std::terminate()`，而不是继续寻找外层 `catch`。

可以用条件表达式传播这个属性：

```cpp
template<class T>
void swap_wrapper(T& a, T& b)
    noexcept(noexcept(a.swap(b)))
{
    a.swap(b);
}
```

也可以在编译期检查：

```cpp
static_assert(std::is_nothrow_move_constructible_v<T>);
```

### move 和 noexcept

最典型的场景是 `std::vector` 扩容。容量不足时，`vector` 需要把旧内存中的元素搬到新内存：

1. 分配更大的内存。
2. 构造新元素。
3. 销毁旧元素。

假设移动构造函数可能抛异常：

```cpp
T(T&& other);  // 可能抛异常
```

假设旧内存中有 `[A][B][C]`。如果使用 copy，复制 `B` 时抛出异常，
`vector` 只需要销毁新内存中已经复制出来的 `A'`。copy 没有修改旧元素，
因此旧内存仍然是 `[A][B][C]`，操作失败后可以保持原状。

如果使用 move，成功移动 `A` 后，状态可能变成：

```text
旧内存：[moved-from A][B][C]
新内存：[A'          ][ ][ ]
```

接下来移动 `B` 时抛出异常，`B` 甚至也可能已经被部分修改。
销毁新内存中的 `A'` 并不能恢复旧的 `A`。`vector` 也不能简单地将 `A'`
move 回去，因为反向 move 同样可能抛异常，而且任意类型 `T` 都没有通用的
“恢复到 move 前状态”操作。因此原容器虽然仍可析构，但元素值未必保持不变，
无法提供强异常安全保证。

如果移动构造是 `noexcept`，一旦开始搬迁，所有元素就一定可以全部移动完成，
自然不需要处理中途回滚。

所以标准库通常倾向于以下策略：

```cpp
if constexpr (std::is_nothrow_move_constructible_v<T>) {
    // move 不会抛：安全地使用 move
} else if constexpr (std::is_copy_constructible_v<T>) {
    // copy 不会修改旧对象：优先使用 copy
} else {
    // 不能 copy：只能使用可能抛异常的 move
}
```

这就是 `std::move_if_noexcept` 背后的思想：

> 对容器而言，`noexcept move` 比“可能抛异常的 move”更可靠。

最后一种情况并不安全，只是没有其他选择。如果 move 中途抛异常，容器的
强异常安全保证可能不再成立。

一个最小可运行例子见 [vector-move-if-noexcept.cpp](vector-move-if-noexcept.cpp)：
该类型的 move 会先修改源对象再抛异常，但它同时支持 copy，所以 `vector`
扩容时选择 copy，避免破坏旧元素。

[vector-copy-exception.cpp](vector-copy-exception.cpp) 展示了 copy 本身也可能
抛异常的情况：第二次 copy 抛出异常后，`vector` 销毁新内存中已经构造成功的
对象并释放新内存，旧容器的底层存储、大小和元素值都保持不变。

### move 并不天然是 noexcept

一个类型的默认移动构造是否为 `noexcept`，取决于所有成员的移动构造：

```cpp
struct S {
    std::string name;
    std::vector<int> values;

    S(S&&) = default;
};
```

如果某个成员的移动操作可能抛异常，`S` 的默认移动操作也可能不是 `noexcept`。
手写移动构造时也不能盲目加上 `noexcept`，因为一旦实际抛异常，程序会终止。

资源管理类通常希望提供如下接口，但前提是实现确实不抛异常：

```cpp
T(T&&) noexcept;
T& operator=(T&&) noexcept;
void swap(T&, T&) noexcept;
```

## 参考
https://learn.microsoft.com/en-us/cpp/cpp/errors-and-exception-handling-modern-cpp 写的好

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
