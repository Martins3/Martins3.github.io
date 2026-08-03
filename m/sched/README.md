看看 cat /proc/4580/status 中是如何获取的:
```txt
voluntary_ctxt_switches:        21932
nonvoluntary_ctxt_switches:     52
```

不过思考下，这个意味着什么，如果数值不在增长?

如何理解 nonvoluntary_ctxt_switches 的含义

尝试构建自己的 scheduler 的参考:
1. https://stackoverflow.com/questions/54635617/what-scheduling-policy-does-each-return-int-value-from-sched-getschedulerpid-c
2. https://stackoverflow.com/questions/45692699/build-against-newer-linux-headers-than-libc-is-built-using

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
