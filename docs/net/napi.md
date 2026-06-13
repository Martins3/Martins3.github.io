
## backlog
收发包的核心逻辑:
1. enqueue_to_backlog  : 加入
2. process_backlog : 处理

这里有一个基本的代码分析:
https://blog.packagecloud.io/monitoring-tuning-linux-networking-stack-receiving-data/

- [什么是 NAPI](https://stackoverflow.com/questions/28090086/what-are-the-advantages-napi-before-the-irq-coalesce)
  - interrupt 聚合
  - napi 的 loop 时间是多少
  - 存储体系下会使用 napi

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
