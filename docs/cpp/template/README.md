只有 17 的部分了
https://www.tmplbook.com/

有点老:
https://github.com/wuye9036/CppTemplateTutorial

这个参考
https://github.com/Mq-b/Modern-Cpp-templates-tutorial

- [CTAD：类模板实参推导](ctad.md)（[测试代码](ctad.cpp)）
- [如何理解 C++ template 的 tuple 遍历](tuple-traversal.md)
- [理解 std::function：可调用对象、类型擦除与可复制性](std-function-copyable.md)


## 还是用这个作为一个阅读的入口吧
https://cppreference.com/cpp/language 中指导:

Template parameters and arguments
Class template – Function template
Variable template (C++14)
Class member template
Template argument deduction
    function – class (CTAD) (C++17)
Explicit specialization – Partial specialization
Packs (C++11)
sizeof... (C++11)
Fold expressions (C++17)
Pack indexing (C++26)
Dependent names – SFINAE
Constraints and concepts (C++20)
Requires expression (C++20)

## 可以开始回想了，不看，仅仅用编译器来想

C 中解决 template 是怎么做的?

max() # 利用宏
sort() # 利用函数指针传递来做比较
```c
qsort(void *base, size_t nmemb, size_t size, __compar_fn_t compar)
```
container of # 很强的技巧，各种 container 容器都可以这么解决

有什么必须用 tempalte 来解决吗，直接解决了 macro ，但是生成了多份代码啊!

## 导致 template 复杂的原因是什么?

1. pairtial initilization 的时候到底选择谁?

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
