# iouring register fds

- [Descriptorless files for io_uring](https://lwn.net/Articles/863071/)

> 一次读或写操作必须同时指定要操作的文件描述符以及用于存放数据的缓冲区。
> 然而，在内核中，该操作得以进行之前，还需完成相当数量的准备工作。
> 这包括对打开的文件引用（以防止在操作进行期间该文件被移除）以及对缓冲区内存进行锁定。
> 在许多情况下，这一开销可能构成操作总成本的显著部分；由于程序往往会对相同的文件描述符和缓冲区执行多次操作，因此这份开销可能会为同一资源反复支付，
> 并逐渐累积起来。

## io_uring_register_fd(3)

完全相同的道理，

不难找到 IOSQE_FIXED_FILE 中的 flgs 在内核中表示为 REQ_F_FIXED_FILE

他们的一个经典区别在于 io_assign_file 中

```c
if (req->flags & REQ_F_FIXED_FILE)
	req->file = io_file_get_fixed(req, req->cqe.fd, issue_flags);
else
	req->file = io_file_get_normal(req, req->cqe.fd);
```

io_file_get_normal 就是一个很简单的访问数组，而 io_file_get_fixed
会进入到一个痛苦的 vfs 的 fget 中， 如果每次 io
都进行这样的操作，这个开销的确存在优化的空间。

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
