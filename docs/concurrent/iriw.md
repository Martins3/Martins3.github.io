## memory model: IRIW
<!-- 0a169bea-9011-4965-a35c-251eb1055396 -->

Litmus Test: Independent Reads of Independent Writes (IRIW)
Can this program see r1 = 1, r2 = 0, r3 = 1, r4 = 0?
(Can Threads 3 and 4 see x and y change in different orders?)

```txt
// Thread 1    // Thread 2    // Thread 3    // Thread 4
x = 1          y = 1          r1 = x         r3 = y
                              r2 = y         r4 = x
```

On sequentially consistent hardware: no.
On x86 (or other TSO): no.

这个问题也是 aarch64 手册中的提到的 Multi-copy atomicity

- [ ] https://stackoverflow.com/questions/73426425/multicopy-atomicity-vs-cache-coherence


这个问题是 memory model 中最后一个问题了，这里总是想不清楚如何控制 cache 的实现是如何做的，
但是先记住基本事情，就是 x86 的 cache 一致性让所有的 CPU 可以同时看到(也就是就像是到达 cache 后，是瞬间到达的所有的 CPU 的，没有传输时间)

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
