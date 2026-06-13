# control-dependencies.txt 内容总结

源文件：`tools/memory-model/Documentation/control-dependencies.txt`

## 核心结论

控制依赖只适合“先读后写”，而且很脆弱，极易被编译器优化破坏。

## 必须记住的三点

- 它不能单独保证先读后读。
- `if` 两边写同一个值时，控制依赖通常会失效。
- 控制依赖只覆盖分支内部，不覆盖 `if` 之后的代码。

## 工程建议

默认优先 acquire/release；只有在明确需要并能证明分支不会被优化掉时才考虑控制依赖。

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
