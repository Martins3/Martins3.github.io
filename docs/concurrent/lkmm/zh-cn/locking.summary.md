# locking.txt 内容总结

源文件：`tools/memory-model/Documentation/locking.txt`

## 核心结论

锁能提供很强的排序，但这种排序主要服务于“也参与这把锁”的 CPU。对不持锁的访问者，很多你以为自然成立的结论并不成立。

## 关键场景

- 双重检查锁定需要 acquire/release。
- 第三个不持锁 CPU 可能看不到锁内顺序，需要额外 barrier。
- 不能随意把访问“挪进临界区”来偷懒证明正确性。

## 阅读建议

如果你在写“快路径无锁读，慢路径持锁改”的代码，这篇必须读。

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
