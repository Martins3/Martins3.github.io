# mini NFSv3 server
<!-- deed2f58-e92b-4d22-b820-f9d5a5567689 -->

我之前一直好奇这种用户态的 nfs server 如何实现，现在有了 codex ，
帮构建了一个，发现也不难:

之前简单看了一眼 https://github.com/nfs-ganesha/nfs-ganesha ，
发现非常的复杂，放弃了。

其次，我需要一个环境来验证一个问题
- nfs server 使用的内存如果需要被 swap out ，那么可能会死锁吗?

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
