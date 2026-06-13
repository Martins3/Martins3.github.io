# dax

https://lwn.net/Articles/717953/

这三个 config 看看，其管理的内容是什么
```txt
CONFIG_FS_DAX=y
CONFIG_FS_DAX_PMD=y
CONFIG_FUSE_DAX=y
```

https://github.com/zhangjaycee/real_tech/wiki/linux_005

> 对于 read/write 调用，一般有 存储外设--page cache--用户缓冲区 两次拷贝；但对于用 O_DIRECT 的情况，只有存储外设--用户缓冲区这一次拷贝。
> 对于 mmap 调用，只有存储外设--page cache一次拷贝，因为用户的访问直接映射了page cache中相应的page。
> 对于 dax文件系统的mmap， 只有存储外设(一般为pmem)，所以没有拷贝。

前两个都是说的对的，但是最后一个我有点怀疑，可以用 qemu 做测试看看是不是这么回事，
这么一想，dax 和 cxl 好像，cxl 可以复用之前的 dax 的支持吗?


观望:
https://www.phoronix.com/news/DAXFS-Linux-File-System

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
