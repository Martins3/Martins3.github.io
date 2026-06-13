# DAMON

- https://www.kernel.org/doc/html/latest/mm/damon/design.html
- https://github.com/awslabs/damo/blob/next/USAGE.md

- https://sjp38.github.io/post/damon/

- https://damonitor.github.io/doc/html/latest/admin-guide/mm/damon/start.html#
- https://docs.kernel.org/admin-guide/mm/damon/usage.html
- https://docs.kernel.org/admin-guide/mm/damon/reclaim.html
- https://docs.kernel.org/admin-guide/mm/damon/lru_sort.html
  - 似乎只是强化 lru 使用的一个机制而已

- https://sjp38.github.io/post/damon_profile_callstack_example/

从这个入手吧:

- https://www.cnblogs.com/skpupil/p/16380664.html
- https://www.cnblogs.com/skpupil/p/16380794.html

## 对比 multi generation

## damon 的询问
- What kind of memory is DAMON RECLAIM able to free?
  - https://lore.kernel.org/damon/20230504171749.89225-1-sj@kernel.org/T/#r133525f352ea3f93373f5c912f69a1aa6d3b618d

## 首先尝试使用
- https://github.com/awslabs/damo/blob/next/USAGE.md
- https://docs.kernel.org/admin-guide/mm/damon/usage.html


## sysfs
/sys/kernel/mm/damon/admin/kdamonds
```txt
.
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
- [ ] nr_kdamonds nr_targets 和 nr_contexts 有什么关系?

- [ ] 数据是如何被加载的?

观察: damon start 之后，就可以继续运行

## 问题

1. 和 madvise 有什么关系 ?
```c
static const char * const damon_sysfs_damos_action_strs[] = {
	"willneed",
	"cold",
	"pageout",
	"hugepage",
	"nohugepage",
	"lru_prio",
	"lru_deprio",
	"stat",
};
```

2. 这两个参数功能，看 c 语言的实现似乎过于简单了

mm/damon/lru_sort.c : 应该是手动指定一段区域，说其可以结束了
mm/damon/reclaim.c : 这个不太清楚发

## 扩展资料
- [LRU-list manipulation with DAMON](https://lwn.net/Articles/905370/)

## 可以，是可以实现的
https://lpc.events/event/16/contributions/1224/attachments/1107/2137/damon_status_plan_ksummit_2022.pdf

https://lwn.net/Articles/973702/
https://lwn.net/Articles/931769/

- https://github.com/damonitor/damo : 这里也有一个 usage

https://gitee.com/openeuler/etmem

## https://github.com/damonitor/damo/blob/next/USAGE.md#damo-args-damon
damo args damon 这一节是有笔误吧

有两个标题是 Partial DAMOS Parameters Update 的章节

## 问题
- damon 是如何处理的，不去干扰 idle page tracking 以及 kernel 自己的 LRU 的

## 又一个 lru 的技术和 multi gen lru 什么关系?
```txt
config DAMON_RECLAIM
	bool "Build DAMON-based reclaim (DAMON_RECLAIM)"
	depends on DAMON_PADDR
	help
	  This builds the DAMON-based reclamation subsystem.  It finds pages
	  that not accessed for a long time (cold) using DAMON and reclaim
	  those.

	  This is suggested to be used as a proactive and lightweight
	  reclamation under light memory pressure, while the traditional page
	  scanning-based reclamation is used for heavy pressure.

config DAMON_LRU_SORT
	bool "Build DAMON-based LRU-lists sorting (DAMON_LRU_SORT)"
	depends on DAMON_PADDR
	help
	  This builds the DAMON-based LRU-lists sorting subsystem.  It tries to
	  protect frequently accessed (hot) pages while rarely accessed (cold)
	  pages reclaimed first under memory pressure.
```
居然实现的这么简单 mm/damon/lru_sort.c

如何理解这个 mm/damon/reclaim.c 文件

## 看看这个 config 的含义
CONFIG_DAMON_PGIDLE=y

## 什么是 fvaddr 啊
```txt
➜  0 cat avail_operations
vaddr
fvaddr
paddr
```

## 原来 mm/damon/reclaim.c 的操作是自动使用这个的
```c
	return damon_new_scheme(
			&pattern,
			/* page out those, as soon as found */
			DAMOS_PAGEOUT,
			/* for each aggregation interval */
			0,
			/* under the quota. */
			&damon_reclaim_quota,
			/* (De)activate this according to the watermarks. */
			&damon_reclaim_wmarks,
			NUMA_NO_NODE);
```

那么 generic 的接口不是可以实现这个吗?

例如这个:
https://docs.kernel.org/admin-guide/mm/damon/usage.html#example

## 看看第一封邮件
- https://lore.kernel.org/all/20210716081449.22187-2-sj38.park@gmail.com/T/#mb54cefd0e293b9049c468da68321c8e8625d2a27

## auto tuning 体现在什么地方?
user_input 和 some_mem_psi_us 的方式

## https://docs.kernel.org/mm/damon/design.html

adaptive regions adjustment mechanism

This could disturb other kernel subsystems using the Accessed bits, namely Idle page tracking and the reclaim logic.
DAMON does nothing to avoid disturbing Idle page tracking, so handling the interference is the responsibility of sysadmins.
However, it solves the conflict with the reclaim logic using `PG_idle` and `PG_young` page flags, as Idle page tracking does.

> damon 如何 sovle 这个问题 ? PG_idle 和 PG_young 的问题，那个不是 idle page tracking 的问题吗?

> vaddr ，如果被其他的 thread 访问，还是任务没有访问吗?

如何理解 filter 中的
1. DAMON monitoring target ?
2. anonymous page / memory cgroup / young page 为什么都是仅仅适用于 kernel 的

Aim-oriented Feedback-driven Auto-tuning : 需要看看如何使用?

## 可以用 damon 获取到虚拟机的内存使用特点吗?

可以的，但是需要付出代价，其实还好，他喵的，简直就是为 kvm 而生的，没有 rmap 啊:
```c
void damon_ptep_mkold(pte_t *pte, struct vm_area_struct *vma, unsigned long addr)
{
	struct folio *folio = damon_get_folio(pte_pfn(ptep_get(pte)));

	if (!folio)
		return;

	if (ptep_clear_young_notify(vma, addr, pte))
		folio_set_young(folio);

	folio_set_idle(folio);
	folio_put(folio);
}

```

```txt
@[
    damon_ptep_mkold+5
    damon_mkold_pmd_entry+287
    walk_pgd_range+1349
    __walk_page_range+91
    walk_page_range+184
    damon_va_prepare_access_checks+251
    kdamond_fn+1288
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 179
```


## etmem 也是使用了 damon 的
etmem/src/etmemd_src/etmemd_damon_sysfs.c

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
