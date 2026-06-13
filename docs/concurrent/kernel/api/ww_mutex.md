# ww_mutex
看 Documentation/locking/locktypes.rst 才知道 ww_mutex 的存在

再 include/linux/ww_mutex.h 中函数定义上，写的非常详细。

一些函数具体实现也是放到 kernel/locking/mutex.c 中

目前只是看到了一个用户 include/linux/dma-resv.h

具体介绍在 Documentation/locking/ww-mutex-design.rst

[Linux 中的 mutex 机制[二] - 解锁和 ww-mutex](https://zhuanlan.zhihu.com/p/95068855)


暂时用不上，先撤了

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
