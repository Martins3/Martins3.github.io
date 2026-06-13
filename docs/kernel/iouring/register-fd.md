# iouring register fds

## io_uring_register_fd(3)

完全相同的道理，

不难找到 IOSQE_FIXED_FILE 中的 flgs 在内核中表示为
REQ_F_FIXED_FILE

他们的一个经典区别在于 io_assign_file 中
```c
	if (req->flags & REQ_F_FIXED_FILE)
		req->file = io_file_get_fixed(req, req->cqe.fd, issue_flags);
	else
		req->file = io_file_get_normal(req, req->cqe.fd);
```

io_file_get_normal 就是一个很简单的访问数组，而 io_file_get_fixed 会进入到一个痛苦的 vfs 的 fget 中，
如果每次 io 都进行这样的操作，这个开销的确存在优化的空间。

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
