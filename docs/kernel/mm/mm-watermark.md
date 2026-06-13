# Watermarks
<!-- e4ffd750-f442-4e35-a757-06b1a39e3c90 -->

- `WMARK_MIN` : 内存不足的最低点，如果计算出的可用页面低于该值，则无法进行页面计数；
- `WMARK_LOW` : 默认情况下，该值为 WMARK_MIN 的 125%，此时 kswapd 将被唤醒，可以通过修改 watermark_scale_factor 来改变比例值；
- `WMARK_HIGH` : 默认情况下，该值为 WMARK_MAX 的 150%，此时 kswapd 将睡眠，可以通过修改 watermark_scale_factor 来改变比例值；

- [ ] `WMARK_MIN`

## [ ] 操作的接口总结一下

cat /proc/sys/vm/lowmem_reserve_ratio


## [ ] 这个问题需要理解下
https://lore.kernel.org/all/20230817035155.84230-1-liusong@linux.alibaba.com/

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

calculate_totalreserve_pages() 实现 totalreserve_pages 的计算方法
- 将每一个 zone 中的 ( maximum 的 lowmem_reserve ) 以及 ( watermark_high ) 加起来 ，但是不能超过该 zone 的总大小


## watermark 的计算的两个复杂点

https://www.kernel.org/doc/Documentation/sysctl/vm.txt

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


### watermark_scale_factor
watermark_scale_factor `__setup_per_zone_wmarks` 在配置，

实际上，这个东西可以控制 low high 距离 min ，从文档中没有看出来

当 watermark_scale_factor = 100 的时候:
```txt
[root@localhost vm]# cat /proc/zoneinfo | grep -E "min |low |high "
        min      5
        low      43
        high     81
        min      1001
        low      7939
        high     14877
        min      1744
        low      13833
        high     25922
        min      32
        low      32
        high     32
```

当 watermark_scale_factor = 10 的时候:

```txt
[root@localhost vm]# cat /proc/zoneinfo | grep -E "min |low |high "
        min      5
        low      8
        high     11
        min      1001
        low      1694
        high     2387
        min      1744
        low      2952
        high     4160
        min      32
        low      32
        high     32
```

### watermark_boost_factor
这个 commit 添加了两个字段
```diff
commit a6ea8b5b9f1ce3403a1c8516035d653006741e80
Author: Liangcai Fan <liangcaifan19@gmail.com>
Date:   Fri Nov 5 13:40:37 2021 -0700

    mm/page_alloc.c: show watermark_boost of zone in zoneinfo

    min/low/high_wmark_pages(z) is defined as

      (z->_watermark[WMARK_MIN/LOW/HIGH] + z->watermark_boost)

    If kswapd is frequently woken up due to the increase of
    min/low/high_wmark_pages, printing watermark_boost can quickly locate
    whether watermark_boost or _watermark[WMARK_MIN/LOW/HIGH] caused
    min/low/high_wmark_pages to increase.

    Link: https://lkml.kernel.org/r/1632472566-12246-1-git-send-email-liangcaifan19@gmail.com
    Signed-off-by: Liangcai Fan <liangcaifan19@gmail.com>
    Cc: Chunyan Zhang <zhang.lyra@gmail.com>
    Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
    Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

添加两个字段:
1. show_free_areas ，也就是 oom 中展示 boost KB 的
2. zoneinfo_show_print 中的 boost

现在 min low high 的定义是这个样子的:
```txt
z->_watermark[WMARK_MIN/LOW/HIGH] + z->watermark_boost
```

`z->_watermark[WMARK_MIN/LOW/HIGH]` 的调整不是自动的，需要通过 sys proc 来调整才可以，
所以如果出现了问题，可以首先怀疑 boost 。

什么时候会调整 watermark_boost_factor ，这个调用路径是唯一的:

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

## 确认几个事情
1. 只要 free 高于 high，就不会产生进行收缩
2. 只要 available > 0 ，就不会产生 oom
- [x] 只要 available > 0, 也不会产生 swap ?
  - 不至于，因为 swapiness 的控制，anon 也是会消耗很多的内存的

## 从 Available 说起
MemAvailable:   16317644 kB
创建一个 1024G 的虚拟机，然后
```txt
echo 508000 > /proc/sys/vm/nr_hugepages
```

```txt
               total        used        free      shared  buff/cache   available
Mem:         1031694     1018787        7694           1        5212        6590
Swap:           9999           0        9999
```

```txt
$echo 3 | sudo tee /proc/sys/vm/drop_caches
```

```txt
              total        used        free      shared  buff/cache   available
Mem:         1031694     1018824       12742           0         128        7667
Swap:           9999           0        9999
```
具体实现在 si_mem_available()

1. 问题 1: 清理完 page cache 之后，发现 free 比 available 大 5G 了
  - 1T 的机器中，`totalreserve_pages` 占用的内存为:
```txt
In [1]: 1311796 * 4 / 1024
Out[1]: 5124.203125
```

2. 问题 2: 为什么 page cache 减少了 5 个 G ，对应 free 增加了 5G 的内存，而 available 增加了 1G 啊。

使用 fio 来制造 cache ，发现 free 和 available 都会下降，之后 available 不会下降，保持稳定，
最终是 available 下降 1G 左右的。

好吧，因为 io engine 使用的是，


3. 问题 3 : 系统到底可以使用多少内存

可以看到 available 逐渐变为 0 ，然后瞬间和 free 相等，并且出现 available 大于 free 的现象:

如果持续的消耗内存，本来 5G 的差值会消失的
```txt
               total        used        free      shared  buff/cache   available
Mem:         1031694     1027499        3690           3         503        3888
Swap:              0           0           0
```
靠，被骗了，是 free(1) 的 bug !

结论:
1. totalreserve_pages 将会对于 free 内存存在一个巨大的差别
2. free -m 中 available 在 /proc/meminfo 的 Available 为 0 的时候显示是错误的


## [ ] 1T 的机器上，high 是好几百兆是怎么计算出来的

似乎任何机器上，总是存在 1.8G 的大小的空闲内存

经过了进一步的压缩，大约是:
```txt
very 0.1s: free -m                                                      nixos: Mon Aug 21 15:04:37 2023

               total        used        free      shared  buff/cache   available
Mem:           64049       19838         566       23887       43645       19589
Swap:          47282        1713       45569

```

```txt
🤒  cat /proc/zoneinfo | grep "high "
        high     9
        high     1313
        high     59612
        high     0
        high     0
```
但是 high (大约 230m) 的数值加起来和 free 的数量还是存在很大的差距啊,
大约存在两倍的差距。

## [ ] totalreserve_pages 是如何计算出来的

```txt
➜  ~ cat /proc/zoneinfo | grep protect
        protection: (0, 2925, 1029604, 1029604, 1029604)
        protection: (0, 0, 1026679, 1026679, 1026679)
        protection: (0, 0, 0, 0, 0)
        protection: (0, 0, 0, 0, 0)
        protection: (0, 0, 0, 0, 0)
```


## [ ] /proc/sys/vm/min_free_kbytes 和 /proc/zone 中的 min 啥关系

set_recommended_min_free_kbytes

## [ ] 在 thp 中增加 min low high 这好吗?

## 测试环境搭建
1T 的虚拟机的启动

echo 510000 > /proc/sys/vm/nr_hugepages

## 和 thp 没有关系，总是 5G ，那么 thp increase lsp 是怎么回事

## 问题
- [ ] 为什么 transparent hugepage 需要通过 set_recommended_min_free_kbytes 来影响 min_free_kbytes

## 当 kernelcore=50% 的时候

结果存在一个更大的 fallback 了
```txt
➜  ~ free -g
               total        used        free      shared  buff/cache   available
Mem:            1007           2        1003           0           0         983
Swap:              9           0           9
```

运行 stress 前:
```txt
MemFree:        23377876 kB
MemAvailable:    2160068 kB
```

运行 stress 后，在 19G 的位置稍作停留之后就会触发 stress-ng 来导致错误:
```txt
MemFree:        19292276 kB
MemAvailable:          0 kB
```
当 MemAvailable = 0 的时候没有 crash，而是等待 2G 的时候 crash 的。
这正好是 high 的大小。

```txt
➜  ~ cat /proc/zoneinfo | grep low
        low      3
        low      835
        low      147658
        low      143773 # 561 M
        low      0

➜  ~ cat /proc/zoneinfo | grep min
        min      0
        min      80
        min      14226
        min      13852 # 54M
        min      0

➜  ~ cat /proc/zoneinfo | grep "high "
        high     6
        high     1590
        high     281090
        high     273694 # 1069M
        high     0

➜  ~ cat /proc/zoneinfo | grep protect
        protection: (0, 2925, 524145, 1029604, 1029604) # 4021M
        protection: (0, 0, 521220, 1026679, 1026679) # 4010M
        protection: (0, 0, 0, 4043676, 4043676) # 15795M
        protection: (0, 0, 0, 0, 0)
        protection: (0, 0, 0, 0, 0)
```

## 分析没有 kernelcore 的 1T 虚拟机

```txt
➜  ~ cat /proc/zoneinfo | grep "high "
        high     6
        high     1590
        high     554791 # 2167M
        high     0
        high     0

➜  ~ cat /proc/zoneinfo | grep manage
        managed  3840
        managed  755668
        managed  263356020 # 2951M
        managed  0
        managed  0

➜  ~ cat /proc/zoneinfo | grep protect
        protection: (0, 2925, 1029611, 1029611, 1029611) # 4010M (已经大于 16M，没有意义)
        protection: (0, 0, 1026686, 1026686, 1026686) # 4010M
        protection: (0, 0, 0, 0, 0)
        protection: (0, 0, 0, 0, 0)
        protection: (0, 0, 0, 0, 0)

➜  ~ free -m
               total        used        free      shared  buff/cache   available
Mem:         1031701     1023220        8317           0         163        3260
Swap:           9999           0        9999
```

好像是，使用 MemAvailable 也不太科学，容易估算的偏大。

其实是取决于 0 ~ 3G 的内存到底使用了多少。这部分的 free 无法被使用。

```txt
➜  ~  cat /proc/zoneinfo | grep "free "

  pages free     3824
  pages free     755666
  pages free     262388887
  pages free     0
  pages free     0
```
DMA32 中 managed 一共 3G，NORMAL 中一共 2G 的 highmem
所以，正好差距是 5G

## 关键参考
1. [【原创】（八）Linux 内存管理 - zoned page frame allocator - 3](https://www.cnblogs.com/LoyenWang/p/11708255.html)
2. [内存管理参数 lowmem_reserve_ratio 分析](http://linux.laoqinren.net/kernel/vm-sysctl-lowmem_reserve_ratio/)

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
