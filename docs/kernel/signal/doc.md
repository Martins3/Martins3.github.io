## man signal(7)

**WHEN COMING BACK** : : 实际上，job control 比想想的更加简单，将 tlpi 34 的内容好好看看, 总结总结，顺便再去搞定 setsid 的问题

首先阅读一下 man signal(7)

 A  process can change the disposition of a signal using sigaction(2) or signal(2).

- [x] By default, a signal handler is invoked on the normal process stack.
It is possible to arrange that the signal handler uses an alternate stack;
see sigaltstack(2) for a discussion of how to do this and when it might be useful.
  - 只是修改一下执行的时候使用的 stack


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
