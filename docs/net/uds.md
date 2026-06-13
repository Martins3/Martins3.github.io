# unix domain 分析


## 基本问题
- [unix domain vs pipe in function](https://stackoverflow.com/questions/9475442/unix-domain-socket-vs-named-pipes)
- [Introduction](https://stackoverflow.com/questions/21032562/example-to-explain-unix-domain-socket-af-inet-vs-af-unix)
- [unix domain vs pipe in performance](https://stackoverflow.com/questions/1235958/ipc-performance-named-pipe-vs-socket) : shared memory 直接碾压

## 具体代码
net/unix/

### 问题 2: 三个 socket 类型啥关系
```c
static const struct proto_ops unix_stream_ops
static const struct proto_ops unix_dgram_ops
static const struct proto_ops unix_seqpacket_ops
```

### 问题 3 : unix_poll 和普通的 socket poll 有区别吗?


### 问题 4 : 这两个 proto 有什么区别?
```c
struct proto unix_dgram_proto
struct proto unix_stream_proto
```

### 问题 5 : 为什么需要 garbage 啊


### 问题 7: 将 uds socket 路径 共享给 guest os
当然，是无法 connect 的，但是具体做出检查的位置在那里?

由于用的是路径而不是 一般的 socket 的 port ，是不是会有很多 path 的处理的问题，

### 问题 8 : 为什么可以将 uds 共享给容器啊!

太厉害了

## SCM
通过 uds 将 fd 传递到另外一个 process 中去。




### 测试代码
- code/module/c/fd-passing.c
- code/module/c/uds-*.c

### 阅读材料
https://stackoverflow.com/questions/909064/portable-way-to-pass-file-descriptor-between-different-processes
https://blog.cloudflare.com/know-your-scm_rights
https://unix.stackexchange.com/questions/699186/what-does-scm-mean-in-unix-sockets-context-scm-rights-etc

### 具体实现
net/core/scm.c

- [ ] 似乎 scm_recv 的调用者不只是 uds

### SCM_RIGHTS 在 unix 上存在循环计数问题?

> https://lwn.net/Articles/973687/
>
> The sending of file descriptors over Unix-domain sockets with SCM_RIGHTS messages has long been prone to the creation of reference-count cycles; see this 2019 article for one description of the problem and attempts to resolve it. The associated garbage-collection code has been massively reworked for 6.10, leading to a simpler and more robust solution; see this merge message for some more information.

- https://lwn.net/Articles/779472/
- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=da493dbb1f2a

https://googleprojectzero.blogspot.com/2022/08/the-quantum-state-of-linux-kernel.html


## net/unix/garbage.c 这个文件是做啥的，为什么 uds 需要 garbage 啊

## 他到底有什么问题?
https://www.tenable.com/cve/CVE-2023-52656

怎么一下全部 drop 了啊


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
