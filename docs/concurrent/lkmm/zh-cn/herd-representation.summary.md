# herd-representation.txt 内容总结

源文件：`tools/memory-model/Documentation/herd-representation.txt`

## 核心结论

这份文件给出“内核原语 -> herd 抽象事件”的对照表，是读形式化模型时的桥梁。

## 最关键的映射

- `READ_ONCE` -> 读事件
- `WRITE_ONCE` -> 写事件
- `smp_mb/rmb/wmb` -> fence 事件
- acquire/release -> 带属性的读/写事件
- `cmpxchg` -> 成功时读写对，失败时只有读

## 适合谁看

适合已经开始读 litmus test 或 herd7 输出的人。

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
