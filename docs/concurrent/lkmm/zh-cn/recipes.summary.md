# recipes.txt 内容总结

源文件：`tools/memory-model/Documentation/recipes.txt`

## 核心结论

这是 LKMM 的模式手册。它把常见并发形状映射到常见同步写法。

## 最重要的模式

- MP：`store_release` 配 `load_acquire`
- RCU 指针发布：`rcu_assign_pointer` 配 `rcu_dereference`
- 锁外观察锁内数据：可能要额外 barrier
- SB/LB：用来提醒你“直觉顺序”并不可靠

## 适合什么时候看

当你已经知道 barrier 名字，但不知道该怎样组合时，先看这篇。

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
