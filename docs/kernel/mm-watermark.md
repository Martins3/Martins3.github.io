# watermark

- `WMARK_MIN` : 内存不足的最低点，如果计算出的可用页面低于该值，则无法进行页面计数；
- `WMARK_LOW` : 默认情况下，该值为 WMARK_MIN 的 125%，此时 kswapd 将被唤醒，可以通过修改 watermark_scale_factor 来改变比例值；
- `WMARK_HIGH` : 默认情况下，该值为 WMARK_MAX 的 150%，此时 kswapd 将睡眠，可以通过修改 watermark_scale_factor 来改变比例值；

## 初始化的路线图

- init_per_zone_wmark_min
  - calculate_min_free_kbytes: 计算出来 `min_free_kbytes`，等于 int_sqrt(nr_free_buffer_pages() * (PAGE_SIZE >> 10) * 16);
  - setup_per_zone_wmarks
    - `__setup_per_zone_wmarks`
      - 计算每一个 zone 的 min low high，按照该 zone 的大小比例分担 min_free_kbytes
      - calculate_totalreserve_pages
  - setup_per_zone_lowmem_reserve
  - khugepaged_min_free_kbytes_update

- 从 get_page_from_freelist 中，一旦 zone_watermark_fast 检测到内存不足，那么将会开始调用 node_reclaim 来回收内存，并不是等到所有的 zone 低于 watermark 才会开始 direct reclaim 的
如果一个 zone 中无法 reclaim 到 page 之类的，才会进入到下一个 page

## lowmem_reserve
为了防止 "highmem" zone 内存分配过多的 fallback 到 "lowmem" zone 上。

## watermark 的计算的两个复杂点

> watermark_boost_factor:
>
> This factor controls the level of reclaim when memory is being fragmented.
> It defines the percentage of the high watermark of a zone that will be
> reclaimed if pages of different mobility are being mixed within pageblocks.
> The intent is that compaction has less work to do in the future and to
> increase the success rate of future high-order allocations such as SLUB
> allocations, THP and hugetlbfs pages.
>
> To make it sensible with respect to the watermark_scale_factor
> parameter, the unit is in fractions of 10,000. The default value of
> 15,000 on !DISCONTIGMEM configurations means that up to 150% of the high
> watermark will be reclaimed in the event of a pageblock being mixed due
> to fragmentation. The level of reclaim is determined by the number of
> fragmentation events that occurred in the recent past. If this value is
> smaller than a pageblock then a pageblocks worth of pages will be reclaimed
> (e.g.  2MB on 64-bit x86). A boost factor of 0 will disable the feature.
>
> =============================================================
>
> watermark_scale_factor:
>
> This factor controls the aggressiveness of kswapd. It defines the
> amount of memory left in a node/system before kswapd is woken up and
> how much memory needs to be free before kswapd goes back to sleep.
>
> The unit is in fractions of 10,000. The default value of 10 means the
> distances between watermarks are 0.1% of the available memory in the
> node/system. The maximum value is 1000, or 10% of memory.
>
> A high rate of threads entering direct reclaim (allocstall) or kswapd
> going to sleep prematurely (kswapd_low_wmark_hit_quickly) can indicate
> that the number of free pages kswapd maintains for latency reasons is
> too small for the allocation bursts occurring in the system. This knob
> can then be used to tune kswapd aggressiveness accordingly.
>
> https://www.kernel.org/doc/Documentation/sysctl/vm.txt


1. watermark_scale_factor `__setup_per_zone_wmarks` 在中，让 min low high 的距离更大
2. watermark_boost_factor: 观察这个调用路径

- `__rmqueue`
  - `__rmqueue_smallest`
  - `__rmqueue_fallback` : 如果 `__rmqueue_smallest` 分配失败，那么此处将 fallback migratetype 的页转换为目标类型
    - get_page_from_free_area : 获取页面
    - steal_suitable_fallback : 将获取的页面的所在的 pageblock 的类型进行转换
      - boost_watermark : 如果设置了 watermark_boost_factor，那么可以因此增大 watermark，回收的页面有助于 compaction

## watermark 检测位置
```txt
#0  __zone_watermark_ok (z=0xffff88823fff9d00, order=1, mark=0, highest_zoneidx=2, alloc_flags=257, free_pages=1281590) at mm/page_alloc.c:3977
#1  0xffffffff812fe8fb in zone_watermark_fast (gfp_mask=335872, alloc_flags=257, highest_zoneidx=2, mark=0, order=1, z=0xffff88823fff9d00) at mm/page_alloc.c:4069
#2  get_page_from_freelist (gfp_mask=335872, order=order@entry=1, alloc_flags=257, ac=ac@entry=0xffffffff82a03c70) at mm/page_alloc.c:4242
#3  0xffffffff8130032d in __alloc_pages (gfp=335872, order=order@entry=1, preferred_nid=preferred_nid@entry=0, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/page_alloc.c:5555
#4  0xffffffff8132a90a in __alloc_pages_node (order=<optimized out>, gfp_mask=<optimized out>, nid=0) at include/linux/gfp.h:223
```
其实是需综合检测 watermark 和 lowmem_reserve 的

## 关键参考
1. [【原创】（八）Linux 内存管理 - zoned page frame allocator - 3](https://www.cnblogs.com/LoyenWang/p/11708255.html)
2. [内存管理参数 lowmem_reserve_ratio 分析](http://linux.laoqinren.net/kernel/vm-sysctl-lowmem_reserve_ratio/)

## 问题
- [ ] 为什么 transparent hugepage 需要通过 set_recommended_min_free_kbytes 来影响 min_free_kbytes

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
