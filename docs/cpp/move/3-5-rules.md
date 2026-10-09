# cpp 三/五法则
<!-- 5fcddcc2-3d50-4734-aa44-c89d23efa926 -->

看这个 PDF 基本上就清晰了:

https://smartkeyerror.oss-cn-shenzhen.aliyuncs.com/Psyduck/C%2B%2B/copy-control/4.%20%E4%B8%89%E4%BA%94%E6%B3%95%E5%88%99.pdf
析构函数，拷⻉构造函数，拷⻉赋值函数，移动构造函数以及移动赋值函数

https://stackoverflow.com/questions/4172722/what-is-the-rule-of-three

真正的还是得看这句话了:
As long as you stay away from raw pointer members, the rule of three is unlikely to concern your own code.
大多数情况下，你都没有必要自己手写一个管理资源的类，因为 std:: 基本上都给你实现好了。
只要避免使用原始指针，那么“三/五法则”你基本上也用不到。
（译注：这点可以从智能指针就可以看出来)

## 继续

为了控制类对象的拷贝、移动、赋值和销毁，五法则关注以下五种特殊成员函数：

1. copy constructor：拷贝构造函数，`A(const A &other)`。
2. copy assignment operator：拷贝赋值运算符，`A &operator=(const A &other)`。
3. move constructor：移动构造函数，`A(A &&other)`。
4. move assignment operator：移动赋值运算符，`A &operator=(A &&other)`。
5. destructor：析构函数，`~A()`。

这里只有拷贝构造函数和移动构造函数是构造函数：它们创建新对象；赋值运算符修改已经存在的对象。

```cpp
A b = a;             // 拷贝构造：创建 b
b = a;               // 拷贝赋值：b 已经存在
A c = std::move(a);   // 移动构造：创建 c
c = std::move(b);     // 移动赋值：c 已经存在
```

C++ 的特殊成员函数还包括默认构造函数（例如 `A()`），共六类。五法则关注其中与资源管理相关的上述五类，不包含默认构造函数。

- 直接初始化 和 拷贝初始化，拷贝赋值
  - 拷贝赋值最容易区分，区别在于这个对象是否已经存在, 而不是在于是否使用等于号 [^1]
  - 拷贝 和 直接 : 拷贝函数声明方法是直接的一个子类啊
  - 永远不存在绕一个大圈子，首先直接初始化，然后赋值或者拷贝
  - 由于拷贝初始化使用 =, 所以总是
  - 到目前唯一非常诡异的地方在于，string s = "abc" 为什么是拷贝初始化
    - https://stackoverflow.com/questions/32413700/assigning-a-string-literal-to-stdstring
    - 不在于语法层次，而是在于 "abc" 将会成为 s 的可修改变量

A copy constructor of class T is a non-template constructor whose first parameter is T&‍, const T&‍, volatile T&‍,
or const volatile T&‍, and either there are no other parameters, or the rest of the parameters all have default values.[^2]

需要析构函数的类也需要拷贝和赋值操作(3/5法则)

需要拷贝操作的类也需要赋值操作，反之亦然。

- [ ] 既然编译器会帮助我们合成各种函数，为什么需要 =default

析构函数如果被删除, 那么该对象的自动删除功能就消失了，不能定义这种类型的变量或成员，但是可以动态分配这种类型的对象(可以 new 但是无法 delete)。

如果一个类有数据成员不能默认构造，拷贝，复制或者销毁，则对应的成员函数将被定义为删除。

一般来说， 赋值运算符组合了析构函数和拷贝构造函数。

> 注意，是拷贝构造函数，和赋值运算符
> 拷贝构造函数创建了一个新的对象出来
> 赋值运算符是本身就存在一个对象的，需要将其中原来的部分删除掉

> 在赋值运算符中使用 swap 的方法是更加好的，但是实现的前提是正确的实现了拷贝构造函数。因为其中自动发生了一次拷贝构造。

除了性能因素，使用移动的另一个原因是 IO 和 unique_ptr 这种类不能共享资源。

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
