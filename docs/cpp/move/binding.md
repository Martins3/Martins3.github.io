## 绑定
1. 完整绑定表（18 格，全部实测）

┌──────────────────────┬────────────┬────────────┬─────────────────────┐
│ 声明/操作 <- expr    │ lvalue     │ xvalue     │ prvalue             │
├──────────────────────┼────────────┼────────────┼─────────────────────┤
│ T（按值）            │ OK（拷贝） │ OK（移动） │ OK（拷贝消除/移动） │
├──────────────────────┼────────────┼────────────┼─────────────────────┤
│ const T&             │ OK         │ OK         │ OK                  │
├──────────────────────┼────────────┼────────────┼─────────────────────┤
│ T&                   │ OK         │ NO         │ NO                  │
├──────────────────────┼────────────┼────────────┼─────────────────────┤
│ T&& / const T&&      │ NO         │ OK         │ OK                  │
├──────────────────────┼────────────┼────────────┼─────────────────────┤

记忆口诀：

- 按值 T：来者不拒（左值拷贝、右值移动）
- T&：只要左值
- const T&：通吃（万能引用绑定，最宽松）
- T&&：只要右值（xvalue + prvalue）
- 取地址 &：只要左值（右值没有稳定地址，是临时的）

这里可以观察到，总是把 prvalue 和 xvalue 都是放到一起的归类的，
为什么是这样发

## 取地址

┌──────────────────────┬────────────┬────────────┬─────────────────────┐
│ 声明/操作 <- expr    │ lvalue     │ xvalue     │ prvalue             │
├──────────────────────┼────────────┼────────────┼─────────────────────┤
│ T* = &expr（取地址） │ OK         │ NO         │ NO                  │
└──────────────────────┴────────────┴────────────┴─────────────────────┘

2. 取地址相关的补充

```cpp
  T  lv{};
  T* p1 = &lv;              // OK
  T* p2 = &std::move(lv);   // NO: taking address of rvalue
  T* p3 = &make_pr();       // NO: taking address of rvalue
```

std::addressof(lv) 也一样——它的签名是 addressof(T&)，形参是左值引用，所以只能传左值。

> 注意：volatile T&、volatile T&& 也各有一套，规则与 const 版本相同，只是加了个"不可优化"限定，日常几乎不用，这里不展开。


## 进一步的推理


拷贝和移动靠引用类型的不同来分流：

 ```cpp
   T(const T& other);   // 拷贝构造：other 还要用，只读
   T(T&& other);        // 移动构造：other 不要了，抢资源
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
