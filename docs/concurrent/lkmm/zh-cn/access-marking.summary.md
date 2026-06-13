# access-marking.txt 内容总结

源文件：`tools/memory-model/Documentation/access-marking.txt`

## 核心结论

这篇文档在回答“什么时候该用 `READ_ONCE()`、`WRITE_ONCE()`、`data_race()` 或 plain C 访问”。

## 最重要的判断

- 想限制编译器并明确这是受控共享访问：`READ_ONCE()` / `WRITE_ONCE()`
- 想告诉 KCSAN 这是故意竞争：`data_race()`
- 本来就不该有并发冲突：保留 plain C，让工具能报真 bug

## 常见误区

不要把访问标注当成压制告警的手段。它首先是设计文档，其次才是工具注解。

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
