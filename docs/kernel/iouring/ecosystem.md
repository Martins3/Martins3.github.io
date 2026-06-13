# iouring 的生态

## 一个问题考虑的角度

- C++ 的支持
- rust 的支持
  - https://github.com/tokio-rs/io-uring

- samba 的支持 (没太看懂)
  - https://sambaxp.org/fileadmin/user_upload/sambaxp2023-Slides/Metzmacher_sXP23_io_uring.pdf
  - https://www.youtube.com/watch?v=h9-0MLPtyQQ

- https://github.com/libuv/libuv
  - 全部用的都是 io uring 了


看上去，nvim 使用的是 libuv 来实现的
```txt
  3536444.257 nvim/10618 io_uring:io_uring_submit_req(ctx: 0xffff88811baaa800, req: 0xffff888146b6bc00, user_data: 184683593937, opcode: 29, op_str: "EPOLL")
```

居然真的有人使用:
```txt
	[IORING_OP_EPOLL_CTL] = {
		.name			= "EPOLL",
	},
```

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
