## bysyscall
https://github.com/oracle-samples/bysyscall

只能支持这些 syscall :
```txt
getpid()
gettid()
getuid()
getgid()
getrusage()
```

实现方法我理解是，先让内核在 fork 的时候，自动将
tid gid 写入到共享内存，之后，替换动态库中的 syscall ，然后去
直接去读共享空间就可以了。

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
