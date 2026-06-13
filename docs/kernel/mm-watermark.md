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

## 具体分析一个例子
如果 free 跌倒 min 之后:

crash 的时候:
```txt
[  156.398333] Node 0 active_anon:1244kB inactive_anon:11860356kB active_file:252kB inactive_file:0kB unevictable:11492kB isolated(anon):0kB isolated(file):0kB mapped:11288kB dirty:0kB writeback:0kB shmem:8852kB shmem_thp: 0kB shmem_pmdmapped: 0kB anon_thp: 11327488kB writeback_tmp:0kB kernel_stack:6320kB pagetables:25432kB sec_pagetables:0kB all_unreclaimable? no
[  156.399813] Node 0 DMA free:15360kB boost:0kB min:140kB low:172kB high:204kB reserved_highatomic:0KB active_anon:0kB inactive_anon:0kB active_file:0kB inactive_file:0kB unevictable:0kB writepending:0kB present:15992kB managed:15360kB mlocked:0kB bounce:0kB free_pcp:0kB local_pcp:0kB free_cma:0kB
[  156.401070] lowmem_reserve[]: 0 2923 11915 11915 11915
[  156.401340] Node 0 DMA32 free:63724kB boost:0kB min:27784kB low:34728kB high:41672kB reserved_highatomic:0KB active_anon:0kB inactive_anon:2956284kB active_file:0kB inactive_file:20kB unevictable:0kB writepending:0kB present:3129148kB managed:3020404kB mlocked:0kB bounce:0kB free_pcp:248kB local_pcp:0kB free_cma:0kB
[  156.402656] lowmem_reserve[]: 0 0 8992 8992 8992
[  156.402874] Node 0 Normal free:84476kB boost:0kB min:84708kB low:105884kB high:127060kB reserved_highatomic:0KB active_anon:1244kB inactive_anon:8904144kB active_file:348kB inactive_file:48kB unevictable:11492kB writepending:0kB present:9437184kB managed:9207936kB mlocked:11492kB bounce:0kB free_pcp:68kB local_pcp:0kB free_cma:0kB
[  156.404321] lowmem_reserve[]: 0 0 0 0 0
```
发现其实 DMA 和 DMA32 中还存在很多内存:

当程序被 kill 的时候，/proc/zoneinfo 的内容为:
```txt
 pages free     3840
        boost    0
        min      35
        low      43
        high     51
        spanned  4095
        present  3998
        managed  3840
        cma      0
        protection: (0, 2923, 11915, 11915, 11915)

Node 0, zone    DMA32
  pages free     754824 # 为3019296kB 而 oom 的时候为 63724kB，因为 DMA32 就是描述 4G 下面的，所以这个不科学
        boost    0
        min      6946
        low      8682
        high     10418
        spanned  1044480
        present  782287
        managed  755101
        cma      0
        protection: (0, 0, 8992, 8992, 8992)

Node 0, zone   Normal
  pages free     2206771
        boost    0
        min      21177
        low      26471
        high     31765
        spanned  2359296
        present  2359296
        managed  2301984
        cma      0
        protection: (0, 0, 0, 0, 0)
```

其中 DMA32 :
```txt
[  156.402656] lowmem_reserve[]: 0 0 8992 8992 8992
```
35968kB + 27784kB (DMA32 的 min) = 63752kB  此时的 free 为 : 63724kB
可见 free 已经小于阈值，所以 crash 。

<!-- 但是 (63752 - 63724) = 7 而不是 1 -->

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
