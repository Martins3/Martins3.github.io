# 为什么写一个这么复杂的脚本来启动 QEMU

我没有做过任何需求调研，因为这个项目就是为了临时的需求，

本来并不是，最开始是很简单的，
这个脚本越来越复杂，就变成这个样子了。
当其复杂度超过维护能力，我就会稍微将其简化一下，或者正规化一下
例如将

## 动机
总体来说，QEMU 直接使用还是非常复杂的，如果只是想要使用虚拟化功能，可以使用如下两个软件，其将 QEMU 进行了封装:

- https://github.com/quickemu-project/quickemu
- https://mac.getutm.app/
- libvirt

但是，如果想要学习 Linux 内核或者 QEMU 本身，熟悉 QEMU 的命令行使用还是必须的。
这就是的核心目的，快速调试 QEMU 和用 QEMU 调试内核。

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
