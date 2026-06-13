# copy_from_user

ubuf 用户提供的指针，在执行该函数的时候，当前的进程地址空间就是该用户的，所以使用 ubuf 并不需要什么奇怪的装换。

这个回答已经很清楚了:
https://stackoverflow.com/questions/8265657/how-does-copy-from-user-from-the-linux-kernel-work-internally
1. 拷贝之前设置当前环境是可以进行缺页中断的，如果真的该地址有问题，那么导致该系统调用失败。
2. 检查了下代码，是在 _copy_from_user 中的 should_fail_usercopy 来实现操作

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
