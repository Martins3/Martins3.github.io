# Monitor Filesystem Events

The inotify mechanism replaces an older mechanism, dnotify, which provided a
subset of the functionality of inotify. We describe dnotify briefly at the end of this
chapter, focusing on why inotify is better.

## 19.1 Overview
inotify_init : 初始化 inotify instance
inotify_add_watch/inotify_rm_watch : 添加监听
read : 读取事件
close : 关闭监听

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
