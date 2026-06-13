# CMA

CMA 划定一个区域，该区域只能分配 MOVABLE 的内存，如果 device 或者 hugetlb 不用，那么就给 buddy，如果用，那么就要 move 走。

我觉得这两个 blog 已经说的很好了:
- [【原创】（十六）Linux 内存管理之 CMA](https://www.cnblogs.com/LoyenWang/p/12182594.html)
- [Linux 中的 Memory Compaction [二] - CMA](https://zhuanlan.zhihu.com/p/105745299)

## 基本使用
节选自: https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html
```txt
        cma=nn[MG]@[start[MG][-end[MG]]]
                        [KNL,CMA]
                        Sets the size of kernel global memory area for
                        contiguous memory allocations and optionally the
                        placement constraint by the physical address range of
                        memory allocations. A value of 0 disables CMA
                        altogether. For more information, see
                        kernel/dma/contiguous.c

        cma_pernuma=nn[MG]
                        [ARM64,KNL,CMA]
                        Sets the size of kernel per-numa memory area for
                        contiguous memory allocations. A value of 0 disables
                        per-numa CMA altogether. And If this option is not
                        specified, the default value is 0.
                        With per-numa CMA enabled, DMA users on node nid will
                        first try to allocate buffer from the pernuma area
                        which is located in node nid, if the allocation fails,
                        they will fallback to the global default memory area.

        hugetlb_cma=    [HW,CMA] The size of a CMA area used for allocation
                        of gigantic hugepages. Or using node format, the size
                        of a CMA area per node can be specified.
                        Format: nn[KMGTPE] or (node format)
                                <node>:nn[KMGTPE][,<node>:nn[KMGTPE]]

                        Reserve a CMA area of given size and allocate gigantic
                        hugepages using the CMA allocator. If enabled, the
                        boot-time allocation of gigantic hugepages is skipped.
```

- [ ] 有必要分析下 kernel/dma/contiguous.c
- [ ] 不知道为什么 cma=100M 最后似乎没有任何效果

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
