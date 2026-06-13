# Memory Tiering

- Documentation/ABI/testing/sysfs-kernel-mm-numa

- https://stevescargall.com/blog/2022/06/using-linux-kernel-memory-tiering/
  - [ ] 最基本的使用了

说了很多，但是和 tired 有什么关系，只是解决了之前的 numa balancing 的 page migration 问题而已啊!
```txt
commit c574bbe917036c8968b984c82c7b13194fe5ce98
Author: Huang Ying <ying.huang@intel.com>
Date:   Tue Mar 22 14:46:23 2022 -0700

    NUMA balancing: optimize page placement for memory tiering system

    With the advent of various new memory types, some machines will have
    multiple types of memory, e.g.  DRAM and PMEM (persistent memory).  The
    memory subsystem of these machines can be called memory tiering system,
    because the performance of the different types of memory are usually
    different.

    In such system, because of the memory accessing pattern changing etc,
    some pages in the slow memory may become hot globally.  So in this
    patch, the NUMA balancing mechanism is enhanced to optimize the page
    placement among the different memory types according to hot/cold
    dynamically.

    In a typical memory tiering system, there are CPUs, fast memory and slow
    memory in each physical NUMA node.  The CPUs and the fast memory will be
    put in one logical node (called fast memory node), while the slow memory
    will be put in another (faked) logical node (called slow memory node).
    That is, the fast memory is regarded as local while the slow memory is
    regarded as remote.  So it's possible for the recently accessed pages in
    the slow memory node to be promoted to the fast memory node via the
    existing NUMA balancing mechanism.

    The original NUMA balancing mechanism will stop to migrate pages if the
    free memory of the target node becomes below the high watermark.  This
    is a reasonable policy if there's only one memory type.  But this makes
    the original NUMA balancing mechanism almost do not work to optimize
    page placement among different memory types.  Details are as follows.

    It's the common cases that the working-set size of the workload is
    larger than the size of the fast memory nodes.  Otherwise, it's
    unnecessary to use the slow memory at all.  So, there are almost always
    no enough free pages in the fast memory nodes, so that the globally hot
    pages in the slow memory node cannot be promoted to the fast memory
    node.  To solve the issue, we have 2 choices as follows,

    a. Ignore the free pages watermark checking when promoting hot pages
       from the slow memory node to the fast memory node.  This will
       create some memory pressure in the fast memory node, thus trigger
       the memory reclaiming.  So that, the cold pages in the fast memory
       node will be demoted to the slow memory node.

    b. Define a new watermark called wmark_promo which is higher than
       wmark_high, and have kswapd reclaiming pages until free pages reach
       such watermark.  The scenario is as follows: when we want to promote
       hot-pages from a slow memory to a fast memory, but fast memory's free
       pages would go lower than high watermark with such promotion, we wake
       up kswapd with wmark_promo watermark in order to demote cold pages and
       free us up some space.  So, next time we want to promote hot-pages we
       might have a chance of doing so.

    The choice "a" may create high memory pressure in the fast memory node.
    If the memory pressure of the workload is high, the memory pressure
    may become so high that the memory allocation latency of the workload
    is influenced, e.g.  the direct reclaiming may be triggered.

    The choice "b" works much better at this aspect.  If the memory
    pressure of the workload is high, the hot pages promotion will stop
    earlier because its allocation watermark is higher than that of the
    normal memory allocation.  So in this patch, choice "b" is implemented.
    A new zone watermark (WMARK_PROMO) is added.  Which is larger than the
    high watermark and can be controlled via watermark_scale_factor.

    In addition to the original page placement optimization among sockets,
    the NUMA balancing mechanism is extended to be used to optimize page
    placement according to hot/cold among different memory types.  So the
    sysctl user space interface (numa_balancing) is extended in a backward
    compatible way as follow, so that the users can enable/disable these
    functionality individually.

    The sysctl is converted from a Boolean value to a bits field.  The
    definition of the flags is,

    - 0: NUMA_BALANCING_DISABLED
    - 1: NUMA_BALANCING_NORMAL
    - 2: NUMA_BALANCING_MEMORY_TIERING

    We have tested the patch with the pmbench memory accessing benchmark
    with the 80:20 read/write ratio and the Gauss access address
    distribution on a 2 socket Intel server with Optane DC Persistent
    Memory Model.  The test results shows that the pmbench score can
    improve up to 95.9%.

    Thanks Andrew Morton to help fix the document format error.

    Link: https://lkml.kernel.org/r/20220221084529.1052339-3-ying.huang@intel.com
    Signed-off-by: "Huang, Ying" <ying.huang@intel.com>
    Tested-by: Baolin Wang <baolin.wang@linux.alibaba.com>
    Reviewed-by: Baolin Wang <baolin.wang@linux.alibaba.com>
    Acked-by: Johannes Weiner <hannes@cmpxchg.org>
    Reviewed-by: Oscar Salvador <osalvador@suse.de>
    Reviewed-by: Yang Shi <shy828301@gmail.com>
    Cc: Michal Hocko <mhocko@suse.com>
    Cc: Rik van Riel <riel@surriel.com>
    Cc: Mel Gorman <mgorman@techsingularity.net>
    Cc: Peter Zijlstra <peterz@infradead.org>
    Cc: Dave Hansen <dave.hansen@linux.intel.com>
    Cc: Zi Yan <ziy@nvidia.com>
    Cc: Wei Xu <weixugc@google.com>
    Cc: Shakeel Butt <shakeelb@google.com>
    Cc: zhongjiang-ali <zhongjiang-ali@linux.alibaba.com>
    Cc: Randy Dunlap <rdunlap@infradead.org>
    Cc: Feng Tang <feng.tang@intel.com>
    Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
    Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

看看和 dax 直接的关系是什么
https://www.kernel.org/doc/html/latest/admin-guide/abi-testing.html#file-srv-docbuild-lib-git-linux-testing-sysfs-kernel-mm-numa

https://lwn.net/Articles/974126/

https://lwn.net/Articles/931421/

## 这里其实不太理解的为什么总是和 CXL 有关的

## 如何理解这个文件
mm/migrate_device.c

## 有趣的
- https://mp.weixin.qq.com/s/8MTd1-vg7KSdHq-RBbfsWg

https://lwn.net/Articles/948037/

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
