# explanation.txt 内容总结

源文件：`tools/memory-model/Documentation/explanation.txt`

## 核心结论

`explanation.txt` 是 LKMM 的主文档。它把内核并发代码抽象成事件和关系，再用“不允许成环”的公理判断某个结果是否可能发生。

## 你真正要抓住的主线

- 共享访问先被抽象成 `read`、`write`、`fence` 事件。
- 再建立 `po`、依赖、`rf`、`co`、`fr` 等基础关系。
- 在这些基础上构造 `ppo`、`hb`、`pb`、RCU 相关关系。
- 如果这些必须成立的关系形成环，则对应结果被禁止。

## 最重要的工程含义

- 源码顺序不等于跨 CPU 可见顺序。
- 编译器和硬件都会破坏直觉。
- `READ_ONCE()` / `WRITE_ONCE()`、acquire/release、barrier、RCU、锁，都只是往事件图里添加不同类型的约束边。

## 适合什么时候读

当你已经会写基本 barrier 代码，但想真正理解“为什么这个结果被禁止/允许”时，这篇必须读。

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
