# cpuset

```txt
-rw-r--r--.  1 root root 0 Nov  3 15:47 cpuset.cpus
-r--r--r--.  1 root root 0 Nov  3 15:47 cpuset.cpus.effective
-rw-r--r--.  1 root root 0 Nov  3 15:47 cpuset.cpus.partition
-rw-r--r--.  1 root root 0 Nov  3 15:47 cpuset.mems
-r--r--r--.  1 root root 0 Nov  3 15:47 cpuset.mems.effective
```

## 可以分析的函数
- rebuild_sched_domains
- cpuset_hotplug_update_tasks

```c
	{
		.name = "cpus.partition",
		.seq_show = sched_partition_show,
		.write = sched_partition_write,
		.private = FILE_PARTITION_ROOT,
		.flags = CFTYPE_NOT_ON_ROOT,
		.file_offset = offsetof(struct cpuset, partition_file),
	},

	{
		.name = "cpus.subpartitions",
		.seq_show = cpuset_common_seq_show,
		.private = FILE_SUBPARTS_CPULIST,
		.flags = CFTYPE_DEBUG,
	},
```

## 问题
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

此时 echo 100000 > /proc/sys/vm/nr_hugepages，发现最多只能占据 node=1 的大页

## v2 有吗? v1 中如何实现的
https://stackoverflow.com/questions/55507022/how-to-make-cpuset-cpu-exclusive-function-of-cpuset-work-correctly

## 如何形成限制

### cpu
1. cpuset::cpus_allowed : 直接是在这里的

看上去应该就是这两个位置，但是不知道为什么，但是实际上并不是：
- cpuset_cpus_allowed
- cpuset_cpus_allowed_fallback

实际上，这个才是形成的位置: is_cpu_allowed ，其调用位置非常合理:
```txt
#0  is_cpu_allowed (cpu=4, p=0xffff8880047cd3c0) at kernel/sched/core.c:2279
#1  select_task_rq (wake_flags=8, cpu=4, p=0xffff8880047cd3c0) at kernel/sched/core.c:3518
#2  try_to_wake_up (p=0xffff8880047cd3c0, state=state@entry=3, wake_flags=wake_flags@entry=0) at kernel/sched/core.c:4201
#3  0xffffffff8116c0a0 in wake_up_process (p=<optimized out>) at kernel/sched/core.c:4350
#4  0xffffffff811d926d in hrtimer_wakeup (timer=<optimized out>) at kernel/time/hrtimer.c:1939
#5  0xffffffff811d993a in __run_hrtimer (flags=2, now=0xffffc900001c8f48, timer=0xffffc90040b37dc0, base=0xffff88823bc1f300, cpu_base=0xffff88823bc1f2c0) at kernel/time/hrtimer.c:1685
#6  __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff88823bc1f2c0, now=636813046568, flags=flags@entry=2, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#7  0xffffffff811da675 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#8  0xffffffff810fa50b in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1096
#9  __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1113
#10 0xffffffff8218bfb1 in sysvec_apic_timer_interrupt (regs=0xffffc900000ffe38) at arch/x86/kernel/apic/apic.c:1107
```

### memory
- cpuset_mems_allowed

## [ ] 不知道为什么 QEMU 中，说着是限制一个，但是 htop 中实际上可以看到两个 CPU 繁忙
