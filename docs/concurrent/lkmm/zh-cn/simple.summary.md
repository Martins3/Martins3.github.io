# simple.txt 内容总结

源文件：`tools/memory-model/Documentation/simple.txt`

## 核心结论

这篇文档是在教你“不要一上来就做复杂的 lockless 推理”。优先级大致是：

1. 单线程化或加锁。
2. 复用已有并发 API。
3. 采用数据分片或 per-CPU。
4. 只有在必要时才进入复杂的 lockless 设计。

## 关键提醒

- `atomic` 不等于自动正确排序。
- `READ_ONCE()` 和 `WRITE_ONCE()` 只解决一部分问题。
- 普通 C 访问一旦参与并发，就必须考虑编译器重写。

## 适合谁先读

适合刚接触 LKMM、想先建立工程直觉的人。

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
