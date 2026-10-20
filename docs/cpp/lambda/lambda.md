# cpp lambda
<!-- 8d3d456a-2f10-4853-92ad-e77fe9798d6f -->

## 为什么 cpp 要设计出来 lambda 来

闭包 = 函数 + 它捕获的词法环境

之所以叫 closure（闭包），是因为它把函数和它需要的变量"封闭"在了一起，形成一个自包含的整体。

由于 Lambda 可以捕获变量，所以相对比普通函数	

每个实例有不同状态	不可以	可以
实际上，Lambda 在 C++11 以前就能通过手写 Functor 实现。

## 基本语法
基本格式:
```cpp
[](argument1,argument2,.....)-> return_type {//code}
```
在 () 中传入参数，在 {} 中编写代码，[] 是一个捕获列表，可以指定外部作用域中，可以使用的局部变量：
1. [] — 捕获列表为空，表示在 lambda 表达式中不能使用任何外部作用域的局部变量，只能使用传进去的参数。
2. [=] — 表示按照值传递的方法捕获父作用域的所有变量。
3. [&] — 表示按照引用传递的方法捕获父作用域的所有变量。
4. [this] — 在成员函数中，按照值传递的方法捕获 this 指针。
5. [a, &b] — 不建议直接使用 [=] 或 [&] 捕获所有变量，可以按需显式捕获，就是在 [] 指定变量名，[a] 表示值传递，[&b] 表示引用传递。
6. [x = expr]	初始化捕获（C++14，可移动捕获）

6. 在捕获中，没有 move 语义的问题

- 可以忽略参数和返回值, 比如
```cpp
  auto f = [] { return 42; };
```

- 一个 lambda 表达式只能使用那些明确指明的局部变量，为什么这么设计?

- lambda 表达式在创建的时候，其实是在创建一个隐藏的对象，并且该对象定义了一个 () 操作符
- 之所以 lambda 需要对于局部变量进行 explicit 的捕获，是因为 lambda 再次被使用的时候，这些变量可能都不存在了
  - 当然捕获局部变量的 reference 需要自己注意
  - https://stackoverflow.com/questions/24474957/why-lambda-captures-only-automatic-storage-variables

## lambda  : 2026-10-16

1. 解决了什么问题?
2. 各种捕获问题了
	- move 语义
	- 和 template 会不会有关系?
3. 导致悬空引用 感觉很容易出现啊


### 对比

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
