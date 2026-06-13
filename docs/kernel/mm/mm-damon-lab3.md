# DAMON Lab 3
```txt
/sys/kernel/mm/damon/admin
└── kdamonds
    ├── 0
    │   ├── contexts
    │   │   ├── 0
    │   │   │   ├── avail_operations
    │   │   │   ├── monitoring_attrs
    │   │   │   │   ├── intervals
    │   │   │   │   │   ├── sample_us # 5ms 采样一次
    │   │   │   │   │   ├── aggr_us   #
    │   │   │   │   │   └── update_us # Dynamic Target Space Updates Handling ，多个环境的修改
    │   │   │   │   └── nr_regions
    │   │   │   │       ├── max
    │   │   │   │       └── min
    │   │   │   ├── operations
    │   │   │   ├── schemes
    │   │   │   │   └── nr_schemes
    │   │   │   └── targets
    │   │   │       ├── 0
    │   │   │       │   ├── pid_target
    │   │   │       │   └── regions
    │   │   │       │       └── nr_regions
    │   │   │       └── nr_targets
    │   │   └── nr_contexts
    │   ├── pid
    │   └── state
    └── nr_kdamonds
```
- [ ] kdamonds : 是一个 kthread 吗? 是，每一个 kdamonds 都有一个 pid 的
- [ ] contexts : 每一个 contexts 的内存类型可以不同，paddr vaddr
  - 但是实际上没有支持: 	/* TODO: Support multiple contexts per kdamond */
- [ ] schemes : 一个 contexts 可以有多个，例如，一部分处理 hugetlb ，一部分 swapout

	damon_for_each_scheme(scheme, ctx);


- aggregate interval : 如何聚合 region

apply_interval 还是看不懂

schemes 下有什么东西
> In each scheme directory, five directories (access_pattern, quotas, watermarks, filters, stats, and tried_regions) and three files (action, target_nid and apply_interval) exist.

```txt
└── kdamonds
    ├── 0
    │   ├── contexts
    │   │   ├── 0
    │   │   │   ├── avail_operations
    │   │   │   ├── monitoring_attrs
    │   │   │   │   ├── intervals
    │   │   │   │   │   ├── aggr_us
    │   │   │   │   │   ├── sample_us
    │   │   │   │   │   └── update_us
    │   │   │   │   └── nr_regions
    │   │   │   │       ├── max
    │   │   │   │       └── min
    │   │   │   ├── operations
    │   │   │   ├── schemes
    │   │   │   │   ├── 0
    │   │   │   │   │   ├── access_pattern
    │   │   │   │   │   │   ├── age
    │   │   │   │   │   │   │   ├── max
    │   │   │   │   │   │   │   └── min
    │   │   │   │   │   │   ├── nr_accesses
    │   │   │   │   │   │   │   ├── max
    │   │   │   │   │   │   │   └── min
    │   │   │   │   │   │   └── sz
    │   │   │   │   │   │       ├── max
    │   │   │   │   │   │       └── min
    │   │   │   │   │   ├── action
    │   │   │   │   │   ├── apply_interval_us
    │   │   │   │   │   ├── filters
    │   │   │   │   │   │   └── nr_filters
    │   │   │   │   │   ├── quotas
    │   │   │   │   │   │   ├── bytes
    │   │   │   │   │   │   ├── effective_bytes
    │   │   │   │   │   │   ├── goals
    │   │   │   │   │   │   │   └── nr_goals
    │   │   │   │   │   │   ├── ms
    │   │   │   │   │   │   ├── reset_interval_ms
    │   │   │   │   │   │   └── weights
    │   │   │   │   │   │       ├── age_permil
    │   │   │   │   │   │       ├── nr_accesses_permil
    │   │   │   │   │   │       └── sz_permil
    │   │   │   │   │   ├── stats
    │   │   │   │   │   │   ├── nr_applied
    │   │   │   │   │   │   ├── nr_tried
    │   │   │   │   │   │   ├── qt_exceeds
    │   │   │   │   │   │   ├── sz_applied
    │   │   │   │   │   │   └── sz_tried
    │   │   │   │   │   ├── target_nid
    │   │   │   │   │   ├── tried_regions
    │   │   │   │   │   │   └── total_bytes
    │   │   │   │   │   └── watermarks
    │   │   │   │   │       ├── high
    │   │   │   │   │       ├── interval_us
    │   │   │   │   │       ├── low
    │   │   │   │   │       ├── metric
    │   │   │   │   │       └── mid
    │   │   │   │   └── nr_schemes
    │   │   │   └── targets
    │   │   │       └── nr_targets
    │   │   └── nr_contexts
    │   ├── pid
    │   └── state
    └── nr_kdamonds
```

如何理解 access_pattern ，描述了什么样的 region 是我们需要的:
- age
- nr_accesses
- sz

quotas : 不要使用太多 CPU
- goals : 按照这样的目标
- weights : 侧重于什么

access_pattern/nr_accesses/min 也就是可以统计一个 access region 的访问次数，如何实现的?

### age
如何理解 age 的含义:

`aggregate interval in [10, 20]` 和 access_pattern/age/

Age Tracking
By analyzing the monitoring results, users can also find how long the current access pattern of a region has maintained.
That could be used for good understanding of the access pattern.
For example, page placement algorithm utilizing both the frequency and the recency could be implemented using that.
To make such access pattern maintained period analysis easier, DAMON maintains yet another counter called age in each region.
For each aggregation interval, DAMON checks if the region’s size and access frequency (nr_accesses) has significantly changed.
If so, the counter is reset to zero. Otherwise, the counter is increased.

所谓的 age ，也就是

多长时间做一次 aggregate 啊?

## [x] 原来 `state` 这么复杂
Users can write below commands for the kdamond to the state file.

> on: Start running.
>
> off: Stop running.
>
> commit: Read the user inputs in the sysfs files except state file again.
>
> commit_schemes_quota_goals : Read the DAMON-based operation schemes’ quota goals.
>
> update_schemes_stats: Update the contents of stats files for each DAMON-based operation scheme of the kdamond. For details of the stats, please refer to stats section.
>
> update_schemes_tried_regions: Update the DAMON-based operation scheme action tried regions directory for each DAMON-based operation scheme of the kdamond. For details of the DAMON-based operation scheme action tried regions directory, please refer to tried_regions section.
>
> update_schemes_tried_bytes: Update only .../tried_regions/total_bytes files.
>
> clear_schemes_tried_regions: Clear the DAMON-based operating scheme action tried regions directory for each DAMON-based operation scheme of the kdamond.
>
> update_schemes_effective_quotas: Update the contents of effective_bytes files for each DAMON-based operation scheme of the kdamond. For more details, refer to quotas directory.

- 所有的 write 需要等待 commit 操作一下


其实各个位置都是有使用的:
https://docs.kernel.org/admin-guide/mm/damon/usage.html#sysfs-interface


这个是和 user input 有关吧
update_schemes_tried_bytes


## [ ] va 是如何访问的

damon_va_check_accesses

```txt
	if (pte_young(ptent) || !folio_test_idle(folio) ||
			mmu_notifier_test_young(walk->mm, addr))
		priv->young = true;
```

为什么只有 mmu_notifier_clear_young 需要 notifier 啊 !

到底是如何实现

## apply_interval_us
就是多长时间使用一次方案

## [ ] /sys/kernel/mm/damon 下的 admin 是做什么的

是不是有和 admin 评级的目录

## 有办法测试一下 damon 的 kernel api ?


## 什么东西
https://docs.kernel.org/mm/damon/design.html#aim-oriented-feedback-driven-auto-tuning

## 代码中的细节 :

- damon_sysfs_update_schemes_tried_regions

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
