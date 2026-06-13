# Process Creation

- vfork 和 fork 的区别 ? 一般上来说，vfork 和 exec 配合使用

Except where speed is absolutely critical, new programs should avoid the use of vfork() in favor of fork().

SUSv3 marks vfork() as obsolete, and SUSv4 goes further, removing the specification of vfork().
> 所以，道理很简单，弃用 vfork


fork 机制对于 parent 还是 chilren 谁先运行，并没有任何保证，但是书上利用 signal 机制实现

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
