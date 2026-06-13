# ordering.txt 内容总结

源文件：`tools/memory-model/Documentation/ordering.txt`

## 核心结论

LKMM 原语分三层：barrier、带顺序的访问、无顺序访问。最常用的判断方法是先问自己需要哪种顺序：

- 全序：`smp_mb()`
- 只写侧发布：`smp_store_release()`
- 只读侧获取：`smp_load_acquire()`
- 只防编译器：`barrier()`
- 只做受控访问：`READ_ONCE()` / `WRITE_ONCE()`

## 关键提醒

- acquire/release 通常比显式 `rmb/wmb` 更语义化。
- `READ_ONCE()` 和 `WRITE_ONCE()` 不等于跨 CPU 排序。
- RCU 与控制依赖有自己特有的适用边界。

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
