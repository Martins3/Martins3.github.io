# VM Pressure

- vmpressure.c 是做什么的
  - 似乎是 memorycontrol 的一个子功能，但是和 psi 是什么关系啊？

- mem_cgroup_css_free
  - vmpressure_cleanup

通过这个来实现通知机制:

- mem_cgroup::vmpressure

都是调用到 vmpressure() 中:

- do_try_to_free_pages
  - vmpressure_prio : 通过 prio 可以知道当前的 vmpressure 的程度，具体参考
    - vmpressure
- shrink_node
  - shrink_node_memcgs
    - vmpressure : 记录每一层的
  - vmpressure : 记录整个 cgroup 的


vmpressure_calc_level() 中说明这个计算的方法的核心逻辑:
```c
	/*
	 * We calculate the ratio (in percents) of how many pages were
	 * scanned vs. reclaimed in a given time frame (window). Note that
	 * time is in VM reclaimer's "ticks", i.e. number of pages
	 * scanned. This makes it possible to set desired reaction time
	 * and serves as a ratelimit.
	 */
```
如果在一个扫描中，95% 页都无法回收，那么认为现在是出现大问题，这些数值都是经验数值:
```c
static const unsigned int vmpressure_level_med = 60;
static const unsigned int vmpressure_level_critical = 95;
```


## 为什么要忽略掉 memory.reclaim 的影响 ?

在 v2 的 cgroup 中，例如可以向 /sys/fs/cgroup/mem/memory.reclaim
向其中写入数值来控制回收多少内存，会调用到 memory_reclaim() 中，这是唯一一个会设置上 `MEMCG_RECLAIM_PROACTIVE` 的地方，
此时会忽略掉 vmpressure() 的调用，其他的人不会。

因为这是唯一一个可以主动触发 vmscan 的接口，主动触发的时候可能导致不必要的 notification 了。

## 使用方法
如果将要开始使用 swap 了，能否更快的通知我。

1. 在 v1 的版本中，可以尝试使用 memory.pressure_level 来检测是否压力过大
2. 在 v2 中，使用 psi 机制，psi 也是可以提供 notification 的功能的



## 确认一个事情

但是 v1 是无法设置 low min high 的呀，岂不是不到内存使用完，根本不会开始 vmscan ?

- 在 v1 中，如果一个 cgroup 的 memory 都是 anon，只要出现换出，就会导致
事件被警告?
  - 还是，其实我们对于无法换出的 page 有问题 ?

使用这个工具 :  https://unix.stackexchange.com/questions/217090/how-do-i-read-from-memory-pressure-level-in-a-cgroup

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
