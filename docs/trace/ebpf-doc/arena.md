## arena
kernel/bpf/arena.c

https://mp.weixin.qq.com/s/IOW8UkA1kDw6vanmqxL8KQ

> BPF 程序已经有几种与用户空间通信的方式，包括环形缓冲区、哈希映射和数组映射。然而，每种方法都存在一些问题。
> 1. 环形缓冲区（ring buffer）可以用于将性能测量或跟踪事件发送到用户空间进程，但不能从用户空间接收数据。
> 2. 哈希映射（hash map）可以用于此目的，但从用户空间访问它们需要进行 bpf()系统调用。
> 3. 数组映射（array map）可以使用 mmap()将它们映射到用户空间进程的地址空间中，但 Starovoitov 指出它们的“缺点是数组的整个内存在开始时就被预留下来”。数组映射（以及新的 arena）存储在不可分页的内核内存中，所以未使用的页面会产生明显的资源利用效率上的浪费。
>
> 他的补丁系列允许 BPF 程序创建最多 4GB 大小的 arena。与数组映射不同，这些 arena 不会预先分配页面。BPF 程序可以使用 bpf_arena_alloc_pages() 向 arena 添加页面，并且当用户空间程序在 arena 内触发页面错误（page fault）时，页面将被自动添加进来。

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
