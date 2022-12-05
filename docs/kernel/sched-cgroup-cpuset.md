# cpuset

```txt
-rw-r--r--.  1 root root 0 Nov  3 15:47 cpuset.cpus
-r--r--r--.  1 root root 0 Nov  3 15:47 cpuset.cpus.effective
-rw-r--r--.  1 root root 0 Nov  3 15:47 cpuset.cpus.partition
-rw-r--r--.  1 root root 0 Nov  3 15:47 cpuset.mems
-r--r--r--.  1 root root 0 Nov  3 15:47 cpuset.mems.effective
```

## 问题
- cpuset 如何影响 hugepage 的
- cpuset_init_current_mems_allowed

## cpuset
```c
struct cpuset {
  struct cgroup_subsys_state css;
```

- [ ] https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v1/cpusets.html

## 检查一下 cpuset 形成限制的各个位置

例如 page cache 的位置

```c
static inline struct page *page_cache_alloc(struct address_space *x)
{
	return __page_cache_alloc(mapping_gfp_mask(x));
}

static inline struct page *page_cache_alloc_cold(struct address_space *x)
{
	return __page_cache_alloc(mapping_gfp_mask(x)|__GFP_COLD);
}

static inline struct page *page_cache_alloc_readahead(struct address_space *x)
{
	return __page_cache_alloc(mapping_gfp_mask(x) |
				  __GFP_COLD | __GFP_NORETRY | __GFP_NOWARN);
}


#ifdef CONFIG_NUMA
extern struct page *__page_cache_alloc(gfp_t gfp);
#else
static inline struct page *__page_cache_alloc(gfp_t gfp)
{
	return alloc_pages(gfp, 0);
}
#endif


#ifdef CONFIG_NUMA
struct page *__page_cache_alloc(gfp_t gfp)
{
	int n;
	struct page *page;

	if (cpuset_do_page_mem_spread()) {
		unsigned int cpuset_mems_cookie;
		do {
			cpuset_mems_cookie = read_mems_allowed_begin();
			n = cpuset_mem_spread_node();
			page = __alloc_pages_node(n, gfp, 0);
		} while (!page && read_mems_allowed_retry(cpuset_mems_cookie));

		return page;
	}
	return alloc_pages(gfp, 0);
}
EXPORT_SYMBOL(__page_cache_alloc);
#endif


static inline gfp_t mapping_gfp_mask(struct address_space * mapping)
{
	return mapping->gfp_mask;
}
```
> 如果不是 NUMA, 那么就很简单了，和普通的 alloc_pages 唯一的区别在于，mapping_gfp_mask

## cpuset.mems.effective 和 cpuset.mems 是什么关系

## 如果一个 NUMA 上没有内存，设置 cpuset 为什么会出错

实际上测试，并不会，和 v1 和 v2 也是没有关系的
```sh
echo 1,2 > cpuset.mems
```

这个问题只是发生在 cgroup v1 中，
- 在初始化的时候，设置 root 的 cpuset.mems 为可用的
- 而 v1 中必须遵守层次结构，所以会失败

- cpuset_write_resmask -> validate_change -> is_cpuset_subset

```c
	/* On legacy hiearchy, we must be a subset of our parent cpuset. */
	ret = -EACCES;
	if (!is_in_v2_mode() && !is_cpuset_subset(trial, par))
		goto out;
```

- 初始化的过程中，如何探测内存为 0 的情况，暂时没有看了

## cpuset 下分配大页

```sh
cgcreate -g cpuset:g
cgset -r cpuset.mems=1 g
cgexec -g cpuset:g bash
```

此时 echo 100000 > /proc/sys/vm/nr_hugepages，发现最多只能占据 huge page 的大页:

## v2 有吗? v1 中如何实现的
https://stackoverflow.com/questions/55507022/how-to-make-cpuset-cpu-exclusive-function-of-cpuset-work-correctly
