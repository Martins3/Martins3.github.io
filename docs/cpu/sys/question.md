# 问题

1. SRAM 的结构是什么?
2. DRAM 的结构是什么?
3. DRAM 为什么不用做关键字优先，到底什么是关键字优先?

- 组合逻辑就是全部吗？什么时候不用组合逻辑?

## 问题
- [x] store buffer 和 load buffer 的位置在哪里，如果指令提交了，那么是不是一定需要 store buffer / load buffer 中的内容写入到 cache 中?
  - 可以不用 cache 的，store buffer 和 load buffer 中是否需要到 cache 中，取决于 memory model 的实现

## store queue 和 load queue 的平均深度是什么?

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
