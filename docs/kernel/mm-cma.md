# cma

CMA 划定一个区域，该区域只能分配 MOVABLE 的内存，如果 device 或者 hugetlb 不用，那么就给 buddy，如果用，那么就要 move 走。

我觉得这两个 blog 已经说的很好了:
- [【原创】（十六）Linux 内存管理之 CMA](https://www.cnblogs.com/LoyenWang/p/12182594.html)
- [Linux 中的 Memory Compaction [二] - CMA](https://zhuanlan.zhihu.com/p/105745299)

## 和 hugetlb 的关系
相关的 patch : https://lkml.org/lkml/2020/3/9/1135

相关函数 : `hugetlb_cma_reserve`

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
