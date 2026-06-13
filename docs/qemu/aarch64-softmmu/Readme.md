# 一些记录

按照 https://zhuanlan.zhihu.com/p/77564702, 但是根据 根据[这个](https://bbs.archlinux.org/viewtopic.php?id=252864)的分析,
我们应该采用更加新的版本的 busybox 出来。

还有的就是 tty2/3/4 找不到的问题:
https://busybox.busybox.narkive.com/KfWYHoQf/no-such-device-tty2-tty3-tty4-on-embedded-system

使用了一个解决方法:
```plain
ln -sf null tty2
ln -sf null tty3
ln -sf null tty4
```
但是不知道到底是否正确。

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
