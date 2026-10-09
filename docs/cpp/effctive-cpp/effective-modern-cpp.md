# 《Effective Modern C++》规则速览

> Scott Meyers 的《Effective Modern C++》总结了有效使用 C++11 和 C++14 的 42
> 个具体做法。本文统一使用标准术语“转发引用（forwarding reference）”。

## 一、类型推导

1. 理解模板的类型推导规则。
2. 理解 `auto` 的类型推导规则。
3. 理解 `decltype`。 // FIXME
4. 掌握查看类型推导结果的方法。

## 二、`auto`

5. 优先使用 `auto`，而不是显式类型声明。
6. 当 `auto` 推导出非预期类型时，使用显式类型初始化惯用法。

## 三、转向现代 C++

7. 创建对象时，区分 `()` 和 `{}`。
8. 优先使用 `nullptr`，而不是 `0` 或 `NULL`。
9. 优先使用别名声明 `using`，而不是 `typedef`。
10. 优先使用有作用域的 `enum class`，而不是无作用域枚举。
11. 优先使用删除函数 `= delete`，而不是只声明为 `private` 却不定义的函数。
12. 重写虚函数时使用 `override`。
13. 优先使用 `const_iterator`，而不是 `iterator`。
14. 不会抛出异常的函数应声明为 `noexcept`。
15. 尽可能使用 `constexpr`。
16. 让 `const` 成员函数具备线程安全性。
17. 理解特殊成员函数的自动生成规则。

## 四、智能指针

18. 使用 `std::unique_ptr` 管理独占所有权的资源。
19. 使用 `std::shared_ptr` 管理共享所有权的资源。
20. 使用 `std::weak_ptr` 表示可能失效、但不参与共享所有权的观察关系。
21. 优先使用 `std::make_unique` 和 `std::make_shared`，而不是直接使用 `new`。
22. 使用 Pimpl 惯用法时，在实现文件中定义特殊成员函数。

## 五、右值引用、移动语义与完美转发

23. 理解 `std::move` 和 `std::forward` 的作用。
24. 区分转发引用和右值引用。
25. 对右值引用使用 `std::move`，对转发引用使用 `std::forward`。
26. 避免对转发引用进行函数重载。
27. 熟悉替代转发引用重载的方法，如标签分派、约束模板和按值传递。
28. 理解引用折叠。
29. 假定移动操作可能不存在、成本不一定低，也不一定会被调用。
30. 熟悉完美转发失效的场景。

## 六、Lambda 表达式

31. 避免使用默认捕获模式。
32. 使用初始化捕获将对象移动到闭包中。
33. 对 `auto&&` 参数使用 `decltype` 和 `std::forward` 进行转发。
34. 优先使用 Lambda，而不是 `std::bind`。

## 七、并发 API

35. 优先采用基于任务的编程方式，而不是直接操作线程。
36. 必须异步执行时，为 `std::async` 指定 `std::launch::async`。
37. 确保 `std::thread` 在所有执行路径上都变为不可联结状态。
38. 注意不同线程句柄的析构行为并不相同。
39. 考虑使用 `void` 类型的 future 实现一次性事件通知。
40. 并发编程使用 `std::atomic`，访问特殊内存才使用 `volatile`。

## 八、细节调整

41. 对于总会被复制、可复制且移动成本低的参数，考虑按值传递。
42. 考虑使用 `emplace`，而不是 `insert` 或
    `push`；但应注意隐式转换和资源所有权问题。

## 阅读提示

- 本书针对 C++11 和 C++14；阅读时还应结合 C++17、C++20 及后续标准中的新设施。
- 这些条目是需要结合适用条件判断的经验法则，不是无条件执行的硬性规定。
- 目录顺序参考
  [O'Reilly 的《Effective Modern C++》目录](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/introduction01.html)。

## human

### chapter 5 : 右值引用，移动语义，完美转发

移动语义让创建 move only 类型的对象成为可能，包括 std::unique_ptr, std::future,
std::thread.

形参是 左值, 即使其类型是右值引用。

```cpp
void f(Widget && a);
```

- 如果想要移动一个对象，就不要将其声明为常量,
  常量对象将移动操作会一声不响的转换为复制操作
- std::move 只是保证转换之后的对象可以移动

- std::forward
  的操作，当且仅当其实参是使用右值完成初始化时，他才会将执行向右值类型的强制类型转换

- [ ] P155 论述 std::forward 为什么不可以替代 std::move

- 如果看到 `T &&`, 但是没有涉及该类型的类型推导，那么就是右值引用。

- forwarding reference 的常见场景是在 auto 和 template 上, 是不是 forwarding reference 的关键在于 类型推导

- 使用右值来初始化 forwarding reference，将会得到右值引用，采用左值来初始化 forwarding reference，将会得到一个左值引用

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
