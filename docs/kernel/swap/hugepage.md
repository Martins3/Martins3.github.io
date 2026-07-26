## Q : Linux 现在支持大页 swap out 吗?
当前 Linux 内核对大页的 swap 支持可以分三类来看：

1. Transparent HugePage（THP）— 支持整体 swap

内核通过 CONFIG_THP_SWAP 开启（默认 y），依赖：
• TRANSPARENT_HUGEPAGE
• ARCH_WANTS_THP_SWAP（目前 x86、arm64、s390、riscv、loongarch 为 y）
• SWAP && 64BIT

swap out：THP 可以以整片方式写出到 swap，不用先 split。回收路径在 mm/vmscan.c 里先调用 folio_alloc_swap(folio) 尝试分配一段连续的
swap entry；分配成功就整体 swap out，对应计数器 thp_swpout。

fallback：如果 swap 设备上找不到连续的大块空间，就会把 folio split 成普通页再分别 swap out，对应 thp_swpout_fallback /
MTHP_STAT_SWPOUT_FALLBACK。folio_alloc_swap() 里也会显式拒绝 order 超过 SWAPFILE_CLUSTER（即 HPAGE_PMD_SIZE）的分配。

swap in：也可以整片读回（swpin），但如果分配/charge 大页失败，会 fallback 到小页或更低 order（swpin_fallback）。

2. shmem / tmpfs 大页 — 支持，但可能 split

tmpfs/shmem 的 large folio 同样走 folio_alloc_swap()。代码注释写得很清楚：

│ The large shmem folio can be split if CONFIG_THP_SWAP is not enabled or we failed to allocate contiguous swap entries...

所以能整体 swap 就整体 swap；条件不满足时 split 成普通页再 swap。

3. hugetlbfs — 不支持 swap

mm/hugetlb.c 里明确写了：

│ ...this may not be extremely important for hugetlbfs for now since swapping is not supported...

hugetlbfs 的大页目前不能被换出。这是设计上的限制，和 THP/shmem 走的是不同机制。

4. Multi-Size THP（MTHP）

现在内核支持多种 order 的 THP，swap 分配也按 order 进行（SWAP_NR_ORDERS）。mm/swapfile.c 里的 percpu_swap_cluster 按 order 缓存
swap cluster，所以不同大小的大页都可以尝试整体 swap，失败再按 order fallback。

简单总结

┌──────────────────┬───────────────┬─────────────────────────────────────────────┐
│ 大页类型         │ 是否支持 swap │ 说明                                        │
├──────────────────┼───────────────┼─────────────────────────────────────────────┤
│ THP / MTHP       │ ✅            │ 整体 swap out/in，空间不够时 split fallback │
├──────────────────┼───────────────┼─────────────────────────────────────────────┤
│ shmem/tmpfs 大页 │ ✅            │ 同 THP，可能 split                          │
├──────────────────┼───────────────┼─────────────────────────────────────────────┤
│ hugetlbfs        │ ❌            │ 不支持换出                                  │
└──────────────────┴───────────────┴─────────────────────────────────────────────┘

如果你是在 arm64 上（当前这个仓库就是 linux-aarch64），ARCH_WANTS_THP_SWAP 是启用的，所以 THP 大页可以整体 swap。


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
