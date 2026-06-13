# 谈谈超线程

我发现，为了支持操作系统看上去是有两个 CPU 的，其实还是很难的。

两个 CPU 可以独立的接受中断和触发异常

比较下 AMD 的打开关闭，内核和 nullblk 的性能

## 忽然感受，Mac 的 SMT 很强啊
也许是错觉，
测试方法，让两个 pthread 在两个 SMT 循环，看看和一个 SMT 上循环，需要的时间的多少。

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
