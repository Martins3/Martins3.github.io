# numa balancing 工作原理
<!-- 01f1368f-fb04-44d0-95ea-20a115795904 -->

1. 基本使用

/proc/sys/kernel/numa_balancing
```c
#define NUMA_BALANCING_DISABLED		0x0
#define NUMA_BALANCING_NORMAL		0x1
#define NUMA_BALANCING_MEMORY_TIERING	0x2
```

2. 基本文档
- https://www.kernel.org/doc/html/latest/admin-guide/sysctl/kernel.html#numa-balancing
- https://www.kernel.org/doc/html/latest/admin-guide/sysctl/kernel.html#numa-balancing-promote-rate-limit-mbps

## 基本实现原理
2. numa balancing 无需特殊之处，只是需要清理掉 pte 的 access bit ，
然后就可以自动

制作过程:
```txt
- entry_SYSCALL_64
  - do_syscall_64
    - syscall_exit_to_user_mode
      - syscall_exit_to_user_mode_work
        - exit_to_user_mode_prepare
          - exit_to_user_mode_loop
            - resume_user_mode_work
              - task_work_run
                - task_numa_work
                  - change_prot_numa
```

触发的过程:
```c
	if (pte_protnone(vmf->orig_pte) && vma_is_accessible(vmf->vma))
		return do_numa_page(vmf);
```
关键在于:
```c
static inline int pte_protnone(pte_t pte)
{
	return (pte_flags(pte) & (_PAGE_PROTNONE | _PAGE_PRESENT))
		== _PAGE_PROTNONE;
}
```

numa balance page fault 的的实现就是靠周期性的拆掉 page table ，然后
来构建。

## mbind 行为是什么?


## 需要考虑什么问题
1. 到底是迁移的 cpu 还是迁移内存


## 基本结构体
```c
struct task_struct {
  // ...
#ifdef CONFIG_NUMA_BALANCING
	int				numa_scan_seq;
	unsigned int			numa_scan_period;
	unsigned int			numa_scan_period_max;
	int				numa_preferred_nid;
	unsigned long			numa_migrate_retry;
	/* Migration stamp: */
	u64				node_stamp;
	u64				last_task_numa_placement;
	u64				last_sum_exec_runtime;
	struct callback_head		numa_work;

	/*
	 * This pointer is only modified for current in syscall and
	 * pagefault context (and for tasks being destroyed), so it can be read
	 * from any of the following contexts:
	 *  - RCU read-side critical section
	 *  - current->numa_group from everywhere
	 *  - task's runqueue locked, task not running
	 */
	struct numa_group __rcu		*numa_group;

	/*
	 * numa_faults is an array split into four regions:
	 * faults_memory, faults_cpu, faults_memory_buffer, faults_cpu_buffer
	 * in this precise order.
	 *
	 * faults_memory: Exponential decaying average of faults on a per-node
	 * basis. Scheduling placement decisions are made based on these
	 * counts. The values remain static for the duration of a PTE scan.
	 * faults_cpu: Track the nodes the process was running on when a NUMA
	 * hinting fault was incurred.
	 * faults_memory_buffer and faults_cpu_buffer: Record faults per node
	 * during the current scan window. When the scan completes, the counts
	 * in faults_memory and faults_cpu decay and these values are copied.
	 */
	unsigned long			*numa_faults;
	unsigned long			total_numa_faults;

	/*
	 * numa_faults_locality tracks if faults recorded during the last
	 * scan window were remote/local or failed to migrate. The task scan
	 * period is adapted based on the locality of the faults with different
	 * weights depending on whether they were shared or private faults
	 */
	unsigned long			numa_faults_locality[3];

	unsigned long			numa_pages_migrated;
#endif /* CONFIG_NUMA_BALANCING */
  // ...
}
```

```c
struct numa_group {
	refcount_t refcount;

	spinlock_t lock; /* nr_tasks, tasks */
	int nr_tasks;
	pid_t gid;
	int active_nodes;

	struct rcu_head rcu;
	unsigned long total_faults;
	unsigned long max_faults_cpu;
	/*
	 * faults[] array is split into two regions: faults_mem and faults_cpu.
	 *
	 * Faults_cpu is used to decide whether memory should move
	 * towards the CPU. As a consequence, these stats are weighted
	 * more by CPU use than by memory faults.
	 */
	unsigned long faults[];
};
```
基本上都是被 fair.c 使用

```c
/* The regions in numa_faults array from task_struct */
enum numa_faults_stats {
	NUMA_MEM = 0,
	NUMA_CPU,
	NUMA_MEMBUF,
	NUMA_CPUBUF
};
extern void sched_setnuma(struct task_struct *p, int node);
extern int migrate_task_to(struct task_struct *p, int cpu);
extern int migrate_swap(struct task_struct *p, struct task_struct *t,
			int cpu, int scpu);
extern void init_numa_balancing(unsigned long clone_flags, struct task_struct *p);
```

- init_numa_balancing

## 触发过程
do_numa_page


> [!NOTE]
> 参考神奇海螺的意见，有待验证

# Linux 内核 NUMA Balancing 与 Interleave 策略分析
<!-- 8d64d13e-2c35-4c71-90c0-cfaf629545ed -->

## 概述

本文档分析了Linux内核中NUMA balancing机制为何不会扫描采用interleave内存策略的VMA（虚拟内存区域）的代码实现。

## 核心发现

### 1. NUMA Balancing扫描VMA的条件检查

在`kernel/sched/fair.c`文件中，NUMA balancing扫描VMA时有以下条件检查：

```c
for (; vma; vma = vma_next(&vmi)) {
    if (!vma_migratable(vma) || !vma_policy_mof(vma) ||
        is_vm_hugetlb_page(vma) || (vma->vm_flags & VM_MIXEDMAP)) {
        trace_sched_skip_vma_numa(mm, vma, NUMAB_SKIP_UNSUITABLE);
        continue;
    }
    // ...
}
```

关键在于`!vma_policy_mof(vma)`条件，如果该函数返回false，则跳过该VMA。

### 2. `vma_policy_mof`函数实现

在`mm/mempolicy.c`文件中的`vma_policy_mof`函数决定了是否对特定VMA进行NUMA平衡：

```c
bool vma_policy_mof(struct vm_area_struct *vma)
{
    struct mempolicy *pol;

    if (vma->vm_ops && vma->vm_ops->get_policy) {
        bool ret = false;
        pgoff_t ilx;        /* ignored here */

        pol = vma->vm_ops->get_policy(vma, vma->vm_start, &ilx);
        if (pol && (pol->flags & MPOL_F_MOF))
            ret = true;
        mpol_cond_put(pol);

        return ret;
    }

    pol = vma->vm_policy;
    if (!pol)
        pol = get_task_policy(current);

    return pol->flags & MPOL_F_MOF;
}
```

该函数检查内存策略是否设置了`MPOL_F_MOF`（Migrate On Fault）标志。

### 3. 关键证据：interleave策略不支持NUMA balancing

在`mm/mempolicy.c`文件的`sanitize_mpol_flags`函数中，有明确的限制：

```c
if (*flags & MPOL_F_NUMA_BALANCING) {
    if (*mode == MPOL_BIND || *mode == MPOL_PREFERRED_MANY)
        *flags |= (MPOL_F_MOF | MPOL_F_MORON);
    else
        return -EINVAL;
}
```

这段代码表明：
- 只有`MPOL_BIND`和`MPOL_PREFERRED_MANY`内存策略模式才支持`MPOL_F_NUMA_BALANCING`标志
- `MPOL_INTERLEAVE`（interleave策略）不在允许列表中
- 如果尝试为interleave策略设置NUMA balancing标志，将返回`-EINVAL`错误

## 结论

NUMA balancing默认不会扫描interleave策略的VMA，因为：

1. interleave策略（MPOL_INTERLEAVE）在内核设计上不支持`MPOL_F_MOF`（Migrate On Fault）标志
2. 当NUMA balancing扫描VMA时，会调用`vma_policy_mof()`函数检查该VMA是否支持迁移
3. 对于使用interleave策略的VMA，`vma_policy_mof()`返回false
4. 因此这些VMA会被跳过，不会参与NUMA balancing过程

这是内核有意的设计选择，因为interleave策略本身就旨在跨多个节点均匀分布内存页面，与NUMA balancing的目标（将内存页移到访问最频繁的CPU节点）存在冲突。

# numa_scan_seq
<!-- 90fdcfae-0d20-4025-90a4-d2da83f27ebd -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

should_numa_migrate_memory 中的这个东西如何理解:
```c
	/*
	 * Allow first faults or private faults to migrate immediately early in
	 * the lifetime of a task. The magic number 4 is based on waiting for
	 * two full passes of the "multi-stage node selection" test that is
	 * executed below.
	 */
	if ((p->numa_preferred_nid == NUMA_NO_NODE || p->numa_scan_seq <= 4) &&
	    (cpupid_pid_unset(last_cpupid) || cpupid_match_pid(p, last_cpupid)))
		return true;
```

# cgroup v2 numa_balancing ?
<!-- 376f9b5f-c3c5-404d-ac8a-c27f29794045 -->

https://patchew.org/linux/20250625102337.3128193-1-yu.c.chen@intel.com/


## numa miss

```txt
$ numastat
                           node0           node1
numa_hit             11990392635     90136309435
numa_miss             5927456823        71773283
numa_foreign            71773283      5927456823
interleave_hit             10966           10345
local_node            7888653833     88512682289
other_node           10029195625      1695400429
```
Documentation/translations/zh_CN/admin-guide/numastat.rst

其实是在 zone_statistics 中获取的

这些统计是纯软件的统计，只是分配内存上的而已。

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
