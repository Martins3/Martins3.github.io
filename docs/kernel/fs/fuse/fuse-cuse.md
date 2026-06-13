# cuse
## Character devices in user space
<!-- ac46c0db-669f-4b23-bb1a-c01d0431bf26 -->

https://lwn.net/Articles/308445/
这两个再仔细看看了。

> CUSE 是构建在 FUSE 之上的，因为两者在功能上有大量重叠。字符设备和文件系统都实现了许多相同的文件操作——比如 open()、close()、read() 和 write()——这使得它们天然适配。Heo 还提交了一组独立的 FUSE 补丁，为文件系统增加了额外的操作，其中一些将被 CUSE 所使用。
>
> 这些新增的 FUSE 操作中，就包括一个实现 ioctl() 的机制，而这个机制“必然相当丑陋”。因为 ioctl() 的实现可能以不可预测的方式访问内存，而这些数据结构可能嵌套任意深，因此必须提供一种机制，让用户空间的 CUSE 设备能够读写调用者的内存。由于 CUSE 服务器无法直接访问调用者的内存，因此必须实现一个多步骤、可重试的 ioctl() 流程。这种“丑陋”的机制仅限内核内部使用，以便 CUSE（或其他类似框架）能够支持“无限制”的 ioctl() 实现。而所有普通的 FUSE 文件系统仍被要求使用“受限”的 ioctl()，即内核必须能明确知道数据传输的方向和大小。
>
> CUSE 的核心思想是：将一个由用户空间程序实现的“设备”通过 FUSE 挂载的虚拟文件系统，与内核导出的字符设备节点绑定在一起。FUSE 负责与用户空间程序通信——方式与它处理文件系统完全相同。

SoftHSM
https://github.com/softhsm/SoftHSMv2

tty0tty
https://github.com/freemed/tty0tty (这个没使用 cuse 吧)

所以，也似乎没有什么必须使用 cuse 的有用的项目

此外，真的 cuse 依赖 fuse 吗?

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
