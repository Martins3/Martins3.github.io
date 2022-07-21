# hugetlbfs
如何分配 1G 的 page
- https://github.com/lagopus/lagopus/blob/master/docs/how-to-allocate-1gb-hugepages.md

1. 每一个 pagesize 都是添加一个大小
2. `gather_bootmem_prealloc` : 似乎的确对于 bootmem 存在特殊处理
3. 阅读 linhugetlbfs ，其大致的功能是 : 在该文件系统中间的 mmap 出来的都是 hugepage，但是现在不知道如何测试！
4. subpool 和 reserved map 都是为了 alloc huge page 处理的。
5. `alloc_huge_page` 还会处理 numa 之类的各种业务
6. `enqueue_huge_page` : huge page 的关系需要单独的分析。
7. page cache 相关的

```c
/*
 * node_hstate/s - associate per node hstate attributes, via their kobjects,
 * with node devices in node_devices[] using a parallel array.  The array
 * index of a node device or _hstate == node id.
 * This is here to avoid any static dependency of the node device driver, in
 * the base kernel, on the hugetlb module.
 */
struct node_hstate {
	struct kobject		*hugepages_kobj;
	struct kobject		*hstate_kobjs[HUGE_MAX_HSTATE];
};
static struct node_hstate node_hstates[MAX_NUMNODES];

struct hugepage_subpool {
	spinlock_t lock;
	long count;
	long max_hpages;	/* Maximum huge pages or -1 if no maximum. */
	long used_hpages;	/* Used count against maximum, includes */
				/* both alloced and reserved pages. */
	struct hstate *hstate;
	long min_hpages;	/* Minimum huge pages or -1 if no minimum. */
	long rsv_hpages;	/* Pages reserved against global pool to */
				/* sasitfy minimum size. */
};

struct resv_map {
	struct kref refs;
	spinlock_t lock;
	struct list_head regions;
	long adds_in_progress;
	struct list_head region_cache;
	long region_cache_count;
};
```

```c
#define HSTATE_NAME_LEN 32
/* Defines one hugetlb page size */
struct hstate {
	int next_nid_to_alloc;
	int next_nid_to_free;
	unsigned int order;
	unsigned long mask;
	unsigned long max_huge_pages;
	unsigned long nr_huge_pages;
	unsigned long free_huge_pages;
	unsigned long resv_huge_pages;
	unsigned long surplus_huge_pages;
	unsigned long nr_overcommit_huge_pages;
	struct list_head hugepage_activelist;
	struct list_head hugepage_freelists[MAX_NUMNODES];
	unsigned int nr_huge_pages_node[MAX_NUMNODES];
	unsigned int free_huge_pages_node[MAX_NUMNODES];
	unsigned int surplus_huge_pages_node[MAX_NUMNODES];
#ifdef CONFIG_CGROUP_HUGETLB
	/* cgroup control files */
	struct cftype cgroup_files_dfl[5];
	struct cftype cgroup_files_legacy[5];
#endif
	char name[HSTATE_NAME_LEN];
};
```

## hugepage_subpool
```c
struct hugepage_subpool *hugepage_new_subpool(struct hstate *h, long max_hpages,
						long min_hpages);
void hugepage_put_subpool(struct hugepage_subpool *spool);
```

被 hugetlbfs_fill_super 调用，也就是一个 mount point 对应一个.

pool 就会用于预留 hugepage


1. get pages
```c
/*
 * Subpool accounting for allocating and reserving pages.
 * Return -ENOMEM if there are not enough resources to satisfy the
 * the request.  Otherwise, return the number of pages by which the
 * global pools must be adjusted (upward).  The returned value may
 * only be different than the passed value (delta) in the case where
 * a subpool minimum size must be manitained.
 */
static long hugepage_subpool_get_pages(struct hugepage_subpool *spool,
				      long delta)
{
	long ret = delta;

	if (!spool)
		return ret;

	spin_lock(&spool->lock);

	if (spool->max_hpages != -1) {		/* maximum size accounting */
		if ((spool->used_hpages + delta) <= spool->max_hpages)
			spool->used_hpages += delta;
		else {
			ret = -ENOMEM;
			goto unlock_ret;
		}
	}

	/* minimum size accounting */
	if (spool->min_hpages != -1 && spool->rsv_hpages) {
		if (delta > spool->rsv_hpages) {
			/*
			 * Asking for more reserves than those already taken on
			 * behalf of subpool.  Return difference.
			 */
			ret = delta - spool->rsv_hpages;
			spool->rsv_hpages = 0;
		} else {
			ret = 0;	/* reserves already accounted for */
			spool->rsv_hpages -= delta;
		}
	}

unlock_ret:
	spin_unlock(&spool->lock);
	return ret;
}
```

2. put pages

```c
/*
 * Subpool accounting for freeing and unreserving pages.
 * Return the number of global page reservations that must be dropped.
 * The return value may only be different than the passed value (delta)
 * in the case where a subpool minimum size must be maintained.
 */
static long hugepage_subpool_put_pages(struct hugepage_subpool *spool,
				       long delta)
{
	long ret = delta;

	if (!spool)
		return delta;

	spin_lock(&spool->lock);

	if (spool->max_hpages != -1)		/* maximum size accounting */
		spool->used_hpages -= delta;

	 /* minimum size accounting */
	if (spool->min_hpages != -1 && spool->used_hpages < spool->min_hpages) {
		if (spool->rsv_hpages + delta <= spool->min_hpages)
			ret = 0;
		else
			ret = spool->rsv_hpages + delta - spool->min_hpages;

		spool->rsv_hpages += delta;
		if (spool->rsv_hpages > spool->min_hpages)
			spool->rsv_hpages = spool->min_hpages;
	}

	/*
	 * If hugetlbfs_put_super couldn't free spool due to an outstanding
	 * quota reference, free it now.
	 */
	unlock_or_release_subpool(spool);

	return ret;
}
```

## alloc_huge_page && hugetlb_reserve_pages
都是 hugepage_subpool_get_pages 打交道，只是一个在 page fault 的时候处理，一个是在 mmap 的时候

alloc_huge_page 调用位置 : hugetlb_cow(被hugetlb_no_page调用) hugetlb_no_page(被hugetlb_fault调用)

hugetlb_reserve_pages :  hugetlb_file_setup 和 hugetlbfs_file_mmap


## resv_map

```c
/*
 * Add the huge page range represented by [f, t) to the reserve
 * map.  Existing regions will be expanded to accommodate the specified
 * range, or a region will be taken from the cache.  Sufficient regions
 * must exist in the cache due to the previous call to region_chg with
 * the same range.
 *
 * Return the number of new huge pages added to the map.  This
 * number is greater than or equal to zero.
 */
static long region_add(struct resv_map *resv, long f, long t) //

/*
 * vma_needs_reservation, vma_commit_reservation and vma_end_reservation
 * are used by the huge page allocation routines to manage reservations.
 *
 * vma_needs_reservation is called to determine if the huge page at addr
 * within the vma has an associated reservation.  If a reservation is
 * needed, the value 1 is returned.  The caller is then responsible for
 * managing the global reservation and subpool usage counts.  After
 * the huge page has been allocated, vma_commit_reservation is called
 * to add the page to the reservation map.  If the page allocation fails,
 * the reservation must be ended instead of committed.  vma_end_reservation
 * is called in such cases.
 *
 * In the normal case, vma_commit_reservation returns the same value
 * as the preceding vma_needs_reservation call.  The only time this
 * is not the case is if a reserve map was changed between calls.  It
 * is the responsibility of the caller to notice the difference and
 * take appropriate action.
 *
 * vma_add_reservation is used in error paths where a reservation must
 * be restored when a newly allocated huge page must be freed.  It is
 * to be called after calling vma_needs_reservation to determine if a
 * reservation exists.
 */
enum vma_resv_mode {
	VMA_NEEDS_RESV,
	VMA_COMMIT_RESV,
	VMA_END_RESV,
	VMA_ADD_RESV,
};
```

```c
/*
 * Region tracking -- allows tracking of reservations and instantiated pages
 *                    across the pages in a mapping.
 *
 * The region data structures are embedded into a resv_map and protected
 * by a resv_map's lock.  The set of regions within the resv_map represent
 * reservations for huge pages, or huge pages that have already been
 * instantiated within the map.  The from and to elements are huge page
 * indicies into the associated mapping.  from indicates the starting index
 * of the region.  to represents the first index past the end of  the region.
 *
 * For example, a file region structure with from == 0 and to == 4 represents
 * four huge pages in a mapping.  It is important to note that the to element
 * represents the first element past the end of the region. This is used in
 * arithmetic as 4(to) - 0(from) = 4 huge pages in the region.
 *
 * Interval notation of the form [from, to) will be used to indicate that
 * the endpoint from is inclusive and to is exclusive.
 */
struct file_region {
	struct list_head link;
	long from;
	long to;
};
```

> resv_map 就是给一个具体的 vma 处理的: @todo 但是为什么 vma 需要持有这个东西

```c
static struct resv_map *vma_resv_map(struct vm_area_struct *vma)
{
	VM_BUG_ON_VMA(!is_vm_hugetlb_page(vma), vma); // 首先这个 vma 必须持有这个东西
	if (vma->vm_flags & VM_MAYSHARE) {
		struct address_space *mapping = vma->vm_file->f_mapping;
		struct inode *inode = mapping->host;

		return inode_resv_map(inode);

	} else {
		return (struct resv_map *)(get_vma_private_data(vma) &
							~HPAGE_RESV_MASK);
	}
}
```

```c
/*
 * These helpers are used to track how many pages are reserved for
 * faults in a MAP_PRIVATE mapping. Only the process that called mmap()
 * is guaranteed to have their future faults succeed.
 *
 * With the exception of reset_vma_resv_huge_pages() which is called at fork(),
 * the reserve counters are updated with the hugetlb_lock held. It is safe
 * to reset the VMA at fork() time as it is not in use yet and there is no
 * chance of the global counters getting corrupted as a result of the values.
 *
 * The private mapping reservation is represented in a subtly different
 * manner to a shared mapping.  A shared mapping has a region map associated
 * with the underlying file, this region map represents the backing file
 * pages which have ever had a reservation assigned which this persists even
 * after the page is instantiated.  A private mapping has a region map
 * associated with the original mmap which is attached to all VMAs which
 * reference it, this region map represents those offsets which have consumed
 * reservation ie. where pages have been instantiated.
 */
static unsigned long get_vma_private_data(struct vm_area_struct *vma)
{
	return (unsigned long)vma->vm_private_data;
}
```






## syscontrol
```c
int hugetlb_sysctl_handler(struct ctl_table *, int, void __user *, size_t *, loff_t *);
int hugetlb_overcommit_handler(struct ctl_table *, int, void __user *, size_t *, loff_t *);
int hugetlb_treat_movable_handler(struct ctl_table *, int, void __user *, size_t *, loff_t *);

#ifdef CONFIG_NUMA
int hugetlb_mempolicy_sysctl_handler(struct ctl_table *, int,
					void __user *, size_t *, loff_t *);
#endif

void hugetlb_report_meminfo(struct seq_file *);
int hugetlb_report_node_meminfo(int, char *);
void hugetlb_show_meminfo(void);
unsigned long hugetlb_total_pages(void);
```

## misc
```c
int hugetlb_reserve_pages(struct inode *inode, long from, long to,
						struct vm_area_struct *vma,
						vm_flags_t vm_flags);
long hugetlb_unreserve_pages(struct inode *inode, long start, long end,
						long freed);
bool isolate_huge_page(struct page *page, struct list_head *list);
void putback_active_hugepage(struct page *page);
void move_hugetlb_state(struct page *oldpage, struct page *newpage, int reason);
void free_huge_page(struct page *page);
void hugetlb_fix_reserve_counts(struct inode *inode);
```





## 从用户层角度分析

```c
static inline bool is_file_hugepages(struct file *file)
{
	if (file->f_op == &hugetlbfs_file_operations) // 首先阅读一下 hugetlbfs 的作用吧
		return true;

	return is_file_shm_hugepages(file);
}
```

## core one : hugetlb_file_setup
1. 三个神奇的调用位置 : memfd.c mmap.c(mmap 系统调用必然经过) shmem.c(shmem 获取内存)
得出来的结论，这就是 hugetlb 提供的功能，也就是使用 hugetlb 都是需要创建对应的文件的


// 主要处理文件系统
```c
/*
* Note that size should be aligned to proper hugepage size in caller side,
 * otherwise hugetlb_reserve_pages reserves one less hugepages than intended.
 */
struct file *hugetlb_file_setup(const char *name, size_t size,
				vm_flags_t acctflag, struct user_struct **user,
				int creat_flags, int page_size_log)
```

2. hugetlb 并不一定总是需要 reserve_pages 的
```c
int hugetlb_reserve_pages(struct inode *inode,
					long from, long to,
					struct vm_area_struct *vma,
					vm_flags_t vm_flags)
```

## core two : hugetlb_fault


```c
/*
 * Hugetlb_cow() should be called with page lock of the original hugepage held.
 * Called with hugetlb_instantiation_mutex held and pte_page locked so we
 * cannot race with other handlers or page migration.
 * Keep the pte_same checks anyway to make transition from the mutex easier.
 */
static vm_fault_t hugetlb_cow(struct mm_struct *mm, struct vm_area_struct *vma,
		       unsigned long address, pte_t *ptep,
		       struct page *pagecache_page, spinlock_t *ptl)
```

## config

mm/Makefile
```c
obj-$(CONFIG_HUGETLBFS)	+= hugetlb.o
obj-$(CONFIG_CGROUP_HUGETLB) += hugetlb_cgroup.o
obj-$(CONFIG_TRANSPARENT_HUGEPAGE) += huge_memory.o khugepaged.o
```


```Kconfig
config HUGETLBFS
	bool "HugeTLB file system support"
	depends on X86 || IA64 || SPARC64 || (S390 && 64BIT) || \
		   SYS_SUPPORTS_HUGETLBFS || BROKEN
	help
	  hugetlbfs is a filesystem backing for HugeTLB pages, based on
	  ramfs. For architectures that support it, say Y here and read
	  <file:Documentation/admin-guide/mm/hugetlbpage.rst> for details.

	  If unsure, say N.

config TRANSPARENT_HUGEPAGE
	bool "Transparent Hugepage Support"
	depends on HAVE_ARCH_TRANSPARENT_HUGEPAGE
	select COMPACTION
	select XARRAY_MULTI
	help
	  Transparent Hugepages allows the kernel to use huge pages and
	  huge tlb transparently to the applications whenever possible.
	  This feature can improve computing performance to certain
	  applications by speeding up page faults during memory
	  allocation, by reducing the number of tlb misses and by speeding
	  up the pagetable walking.

	  If memory constrained on embedded, you may want to say N.
```

## hugetlb_vm_ops

```c
/*
 * When a new function is introduced to vm_operations_struct and added
 * to hugetlb_vm_ops, please consider adding the function to shm_vm_ops.
 * This is because under System V memory model, mappings created via
 * shmget/shmat with "huge page" specified are backed by hugetlbfs files,
 * their original vm_ops are overwritten with shm_vm_ops.
 */
const struct vm_operations_struct hugetlb_vm_ops = {
	.fault = hugetlb_vm_op_fault, // 绝对不会被调用
	.open = hugetlb_vm_op_open,
	.close = hugetlb_vm_op_close,
	.split = hugetlb_vm_op_split,
	.pagesize = hugetlb_vm_op_pagesize,
};
```


```c
static void hugetlb_vm_op_open(struct vm_area_struct *vma)
{
	struct resv_map *resv = vma_resv_map(vma);

	/*
	 * This new VMA should share its siblings reservation map if present.
	 * The VMA will only ever have a valid reservation map pointer where
	 * it is being copied for another still existing VMA.  As that VMA
	 * has a reference to the reservation map it cannot disappear until
	 * after this open call completes.  It is therefore safe to take a
	 * new reference here without additional locking.
	 */
	if (resv && is_vma_resv_set(vma, HPAGE_RESV_OWNER))
		kref_get(&resv->refs);
}
```
