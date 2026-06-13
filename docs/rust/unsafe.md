# 先搞清楚基本问题

raw pointers
aliasing
stacked borrows

unsafe-cell
variance

(我当时是想要表达什么来着)

1. 常量与静态变量的另一个区别在于静态变量可以是可变的。访问和修改可变静态变量都是 不安全 的
2. 使用 unsafe 来进行这**四个操作（超级力量）**之一是没有问题的，甚至是不需要深思熟虑的
    * 解引用裸指针
    * 调用不安全的函数或方法
    * 访问或修改可变静态变量
    * 实现不安全 trait

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
