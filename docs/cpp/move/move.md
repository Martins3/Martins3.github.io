## stackoverflow top question
2. 右值引用 : http://thbecker.net/articles/rvalue_references/section_01.html
    1. https://stackoverflow.com/questions/3413470/what-is-stdmove-and-when-should-it-be-used

## question
- [ ] initlize_list
- [ ] move 的含义
- [ ] 左值右值引用
- [ ] copy constructor 和 copy assignment 的区别 ?
- [ ] copy 和 move 的使用情况举例


## cpp 为什么需要 move 语义
<!-- 6dd0969a-adad-44ed-8057-542d56bb14a4 -->

move 语义解决的是两个核心问题：**性能优化** 和 **所有权转移**。

1. 性能：避免不必要的深拷贝

当源对象是一个临时值（prvalue）或者你确定它之后不再被使用时，deep copy 是浪费的。

以 `std::string` 为例，其内部通常持有指向堆内存的指针。move 只需要复制这个指针（常数时间），而 copy 需要分配新内存并复制全部字符（线性时间）。对于 `std::vector` 等大对象，性能差距更明显。

2. 所有权转移：有些资源根本不能拷贝

- `std::unique_ptr` — 独占所有权，不能拷贝，只能通过 move 转移
- `std::thread` / `std::future` / `std::promise` — 线程资源不可复制，只能 move
- 文件句柄、socket — 拷贝会导致重复关闭等问题

没有 move 语义，这些类型几乎无法实用：不能放入 `std::vector`、不能从函数按值返回、不能按值传递。

## 相关总结
1. 为什么拷贝赋值函数需要返回 `*this`
https://stackoverflow.com/questions/34562865/why-does-operator-return-this/34562890

2. 这些 constructor 的参数是 : takes exactly one parameter of type T, T&, const T&, volatile T&, or const volatile T&.
https://en.cppreference.com/w/cpp/language/copy_assignment

## move 的其他资料
[如何评价 C++11 的右值引用（Rvalue reference）特性？](https://www.zhihu.com/question/22111546/answer/30801982)

> std::thread的传递
> thread也是一种典型的不可复制的资源，但可以通过移动来传递所有权。同样std::future std::promise std::packaged_task等等这一票多线程类都是不可复制的，也都可以用移动的方式传递。

- [ ] unique_ptr and shared_ptr 's relation ?

- [深入浅出 C++ 11 右值引用](https://zhuanlan.zhihu.com/p/107445960)


- https://stackoverflow.com/questions/1051379/is-there-a-difference-between-copy-initialization-and-direct-initialization
- https://en.cppreference.com/w/cpp/language/copy_constructor

## move 仔细分析
### 被 move 之后，对象还能用吗

对象仍然存在，也仍然会正常析构。

对于标准库的大多数可移动对象，移动后的状态通常是：

> valid but unspecified：有效，但具体值未指定。

例如：

std::string a = "hello";
std::string b = std::move(a);

a.clear();        // 可以
a = "new value";  // 可以

但不要依赖：

assert(a.empty()); // 不应这样假设

常见的安全做法是：

- 销毁它；
- 给它重新赋值；
- 调用不依赖旧内容的操作；
- 如果类的文档明确规定移动后状态，则按文档使用。

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
