# litmus-tests.txt 内容总结

源文件：`tools/memory-model/Documentation/litmus-tests.txt`

## 核心结论

litmus test 是把并发问题缩成一个可穷举的小模型，然后问 herd7：坏结果是否允许发生。

## 结构要点

- `C <name>`
- 初始化块
- `P0/P1/...`
- `exists (...)`

## 最重要的实践建议

- 优先改现成测试，不要从零写。
- 测试越小越好。
- 调试时看完整状态集，不要只看 `Never`/`Sometimes`。

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
