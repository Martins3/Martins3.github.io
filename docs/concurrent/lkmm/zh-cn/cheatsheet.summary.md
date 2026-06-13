# cheatsheet.txt 内容总结

源文件：`tools/memory-model/Documentation/cheatsheet.txt`

## 核心结论

这是一张排序能力矩阵，适合快速确认某个原语到底能约束哪些前后访问。

## 最实用的几行

- `READ_ONCE()` / `WRITE_ONCE()`：受控，但基本不排序。
- `*_acquire()`：约束后续访问。
- `*_release()`：约束先前访问。
- `smp_mb()`：最强。

## 用法建议

先设计同步模式，再用这张表核对，不要反过来只靠查表拼代码。

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
