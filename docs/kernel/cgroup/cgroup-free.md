# cgroup 释放问题
<!-- ffe003e8-4892-4c0e-a385-673a9f1ab033 -->

https://lore.kernel.org/linux-mm/fee37e75-e818-46b0-8494-684ef3eb5cd4@lucifer.local/T/#m21e331d32ce035b5d68d77421929f5b4eef527ca

问题 1 : 为什么 page cache 文件无法释放，那么 cgroup 就无法释放?

> 当 vipservice 重启时，systemd 会删除/创建 vipservice mem cgroup。由于这些 mem cgroup 指向 events.log 的不同 page，导致 cgroup 虽然被删除，但无法真正被释放。


哦，是这样的啊:

> mem_cgroup_css_offline
> 首先触发该阶段，重置 mem cgroup id，清空 writeback 等。此时，mem cgroup 无法通过文件系统访问，但仍能通过 snapshot_refaults 等函数从内核中访问其状态。
>
> mem_cgroup_css_free
> 当 mem cgroup 关联的 page list 为空时，触发该阶段，彻底释放 mem cgroup。

## 也许有关的
https://lore.kernel.org/all/20191107205334.158354-3-hannes@cmpxchg.org/T/#med6d09203b3b17ef93484744f84298fbd6ee73d2

## TODO
1. share memory 也是需要测试下
  1. 之前的测试进行，需要一个更高的覆盖率才可以
  2. 真的如此吗? 如果这种，还有操作空间吗? 这个问题还可以修复吗?
2. cgroup 为什么不去释放?
3. 其他的 cgroup 有什么遍历方法
4. memcg->memory.usage 和 memcg.css.refcnt 做一下对比
5. /proc/cgroups 的输出看看

2. https://lore.kernel.org/all/20191107205334.158354-3-hannes@cmpxchg.org/T/#med6d09203b3b17ef93484744f84298fbd6ee73d2
3. snapshot_refaults 的作用到底是什么?

- cat /proc/cgroups 中 v1 和 v2 的结果不同
    - 合并架构和非合并架构

## 难道真的是只有在 page free 的时候 css_put 才会释放 css 吗?

这个问题已经就绪了，使用
docs/kernel/cgroup/cgroup-free.test.sh 来测试就可以了。

```txt
@[
        charge_memcg+1
        __mem_cgroup_charge+44
        alloc_anon_folio+511
        do_anonymous_page+299
        __handle_mm_fault+1370
        handle_mm_fault+279
        do_user_addr_fault+707
        exc_page_fault+116
        asm_exc_page_fault+38
]: 45550
```

css_get 不是在和 page 关联的 refcount 相关的:
```c
static int charge_memcg(struct folio *folio, struct mem_cgroup *memcg,
			gfp_t gfp)
{
	int ret;

	ret = try_charge(memcg, gfp, folio_nr_pages(folio));
	if (ret)
		goto out;

	css_get(&memcg->css);
	commit_charge(folio, memcg);
	memcg1_commit_charge(folio, memcg);
out:
	return ret;
}

int __mem_cgroup_charge(struct folio *folio, struct mm_struct *mm, gfp_t gfp)
{
	struct mem_cgroup *memcg;
	int ret;

	memcg = get_mem_cgroup_from_mm(mm);
	ret = charge_memcg(folio, memcg, gfp);
	css_put(&memcg->css);

	return ret;
}
```

```txt

#subsys_name    hierarchy       num_cgroups     enabled
cpu     0       273     1
cpuacct 0       273     1
blkio   0       273     1
memory  0       273     1
devices 0       273     1
freezer 0       273     1
net_cls 0       273     1
perf_event      0       273     1
net_prio        0       273     1
hugetlb 0       273     1
pids    0       273     1
rdma    0       273     1
misc    0       273     1
dmem    0       273     1

/usr/share/bcc/tools/funccount snapshot_refaults -i 1
/usr/share/bcc/tools/funccount mem_cgroup_iter -i 1
/usr/share/bcc/tools/funclatency mem_cgroup_iter -i 1


但是如果切换为 4.19 内核之后，的确会有不释放的情况，但是数量有限
#subsys_name    hierarchy       num_cgroups     enabled
cpuset  10      1       1
cpu     4       85      1
cpuacct 4       85      1
blkio   9       85      1
memory  13      164     1
devices 12      85      1
freezer 11      1       1
net_cls 5       1       1
perf_event      3       1       1
net_prio        5       1       1
hugetlb 2       1       1
pids    6       106     1
rdma    7       1       1
files   8       1       1
```

如果是 drop cache 之后，数量也是的确变少了:
```txt

#subsys_name    hierarchy       num_cgroups     enabled
cpuset  10      1       1
cpu     4       85      1
cpuacct 4       85      1
blkio   9       85      1
memory  13      122     1
devices 12      85      1
freezer 11      1       1
net_cls 5       1       1
perf_event      3       1       1
net_prio        5       1       1
hugetlb 2       1       1
pids    6       106     1
rdma    7       1       1
files   8       1       1
```

但是无法复现出来有几千个的场景

如果是采用独立文件的方法，即便是在 6.17.7 的内核中，结果如下:
```txt

 cat /proc/cgroups
subsys_name    hierarchy       num_cgroups     enabled
cpu     0       2096    1
cpuacct 0       2096    1
blkio   0       2096    1
devices 0       2096    1
freezer 0       2096    1
net_cls 0       2096    1
perf_event      0       2096    1
net_prio        0       2096    1
hugetlb 0       2096    1
pids    0       2096    1
rdma    0       2096    1
misc    0       2096    1
debug   0       2096    1

drop cache 之后，结果为:
#subsys_name    hierarchy       num_cgroups     enabled
cpu     0       70      1
cpuacct 0       70      1
blkio   0       70      1
devices 0       70      1
freezer 0       70      1
net_cls 0       70      1
perf_event      0       70      1
net_prio        0       70      1
hugetlb 0       70      1
pids    0       70      1
rdma    0       70      1
misc    0       70      1
debug   0       70      1
```

如果是 4.19 内核中，其中的结果是: cat /proc/cgroups
```txt
#subsys_name    hierarchy       num_cgroups     enabled
cpuset  3       1       1
cpu     4       75      1
cpuacct 4       75      1
blkio   10      75      1
memory  9       2123    1
devices 8       75      1
freezer 2       1       1
net_cls 5       1       1
perf_event      6       1       1
net_prio        5       1       1
hugetlb 13      1       1
pids    12      75      1
rdma    7       1       1
files   11      1       1
```

## 案发现场

```txt
-   99.99%     0.00%  kswapd7  [kernel.kallsyms]  [k] ret_from_fork
     ret_from_fork
     kthread
   - kswapd
      - 99.97% balance_pgdat
         - 74.98% shrink_node
            - 28.21% shrink_node_memcg
                 5.59% lruvec_lru_size
               - 5.38% shrink_list
                  - 4.02% shrink_inactive_list
                     - 2.82% shrink_page_list
                        - 1.06% __remove_mapping
                             0.73% __delete_from_page_cache
                        - 1.01% free_unref_page_list
                           - 0.89% free_unref_page_commit
                                0.81% free_pcppages_bulk
                     - 1.05% isolate_lru_pages
                          0.75% __list_del_entry_valid
                    1.04% inactive_list_is_low
               - 0.92% blk_finish_plug
                    0.54% blk_flush_plug_list
              23.74% mem_cgroup_iter
            - 19.89% shrink_slab
               - 18.50% do_shrink_slab
                  - 17.02% count_shadow_nodes
                       1.09% find_next_bit
              1.17% mem_cgroup_protected
              0.64% find_first_bit
         - 24.67% snapshot_refaults
              0.77% mem_cgroup_iter
-   99.99%     0.00%  kswapd7  [kernel.kallsyms]  [k] kthread
```

此内核为 5.4 内核:
```txt
$ cat /proc/cgroups
#subsys_name    hierarchy       num_cgroups     enabled
cpuset  10      63      1
cpu     2       211     1
cpuacct 2       211     1
blkio   11      199     1
memory  5       4611    1
devices 8       199     1
freezer 4       25      1
net_cls 3       25      1
perf_event      7       25      1
net_prio        3       25      1
hugetlb 9       20      1
pids    6       222     1
```

## 最终疑问，为什么我现在需要多个文件才可以制作出来

而案发现场，只是需要一个文件就可以制作出来?

## 似乎就是这个问题了
https://lore.kernel.org/linux-mm/fee37e75-e818-46b0-8494-684ef3eb5cd4@lucifer.local/T/#m21e331d32ce035b5d68d77421929f5b4eef527ca

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
