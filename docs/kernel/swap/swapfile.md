# swapfile.c

The cornerstone of swap
area management is the `swap_info` array defined in `mm/swapfile.c`; its entries store information on the
individual swap areas in the system

## 问题
2. swap_count_continued : 为什么 refcount 搞得这么复杂 ?
3. extent 和 cluster 机制如何融合起来 ?

4. 既然存在多个 swap ，当 pageout() 的时候，如何选择的哪一个 swapfile 的

```c
#define SWAP_CLUSTER_MAX 32UL
#define COMPACT_CLUSTER_MAX SWAP_CLUSTER_MAX

#define SWAP_MAP_MAX	0x3e	/* Max duplication count, in first swap_map */
#define SWAP_MAP_BAD	0x3f	/* Note pageblock is bad, in first swap_map */
#define SWAP_HAS_CACHE	0x40	/* Flag page is cached, in first swap_map */
#define SWAP_CONT_MAX	0x7f	/* Max count, in each swap_map continuation */
#define COUNT_CONTINUED	0x80	/* See swap_map continuation for full count */
#define SWAP_MAP_SHMEM	0xbf	/* Owned by shmem/tmpfs, in first swap_map */
```
2. 18.3 实现似乎处理

> 有待分析的封装函数

```c
/*
 * out-of-line __page_file_ methods to avoid include hell.
 */
struct address_space *__page_file_mapping(struct page *page)
{
	return page_swap_info(page)->swap_file->f_mapping;
}
```

## try_to_free_swap

```c
/*
 * If swap is getting full, or if there are no more mappings of this page,
 * then try_to_free_swap is called to free its swap space.
 */
int try_to_free_swap(struct page *page)
{
	VM_BUG_ON_PAGE(!PageLocked(page), page);

	if (!PageSwapCache(page))
		return 0;
	if (PageWriteback(page))
		return 0;
	if (page_swapped(page))
		return 0;

	/*
	 * Once hibernation has begun to create its image of memory,
	 * there's a danger that one of the calls to try_to_free_swap()
	 * - most probably a call from __try_to_reclaim_swap() while
	 * hibernation is allocating its own swap pages for the image,
	 * but conceivably even a call from memory reclaim - will free
	 * the swap from a page which has already been recorded in the
	 * image as a clean swapcache page, and then reuse its swap for
	 * another page of the image.  On waking from hibernation, the
	 * original page might be freed under memory pressure, then
	 * later read back in from swap, now with the wrong data.
	 *
	 * Hibernation suspends storage while it is writing the image
	 * to disk so check that here.
	 */
	if (pm_suspended_storage())
		return 0;

	page = compound_head(page);
	delete_from_swap_cache(page); // 将用于 swap 的 page 删除掉
	SetPageDirty(page); // todo 为什么需要设置为 dirty ?
	return 1;
}

static bool page_swapped(struct page *page) // 判断是否还有该 page 还有人依赖，我是怀疑，随时都是有人需要写会的
{
	swp_entry_t entry;
	struct swap_info_struct *si;

  // THP_SWAP 居然是 TransParentHugePage swap 的意思
	if (!IS_ENABLED(CONFIG_THP_SWAP) || likely(!PageTransCompound(page))) // 一般是非 huge
		return page_swapcount(page) != 0;

	page = compound_head(page);
	entry.val = page_private(page);
	si = _swap_info_get(entry);
	if (si)
		return swap_page_trans_huge_swapped(si, entry);
	return false;
}

/*
 * How many references to page are currently swapped out?
 * This does not give an exact answer when swap count is continued,
 * but does include the high COUNT_CONTINUED flag to allow for that.
 */
int page_swapcount(struct page *page) // 通过 swap_map 获取数值而已
{
	int count = 0;
	struct swap_info_struct *p;
	struct swap_cluster_info *ci;
	swp_entry_t entry;
	unsigned long offset;

	entry.val = page_private(page);
	p = _swap_info_get(entry);
	if (p) {
		offset = swp_offset(entry);
		ci = lock_cluster_or_swap_info(p, offset);
		count = swap_count(p->swap_map[offset]);
		unlock_cluster_or_swap_info(p, ci);
	}
	return count;
}

static inline unsigned char swap_count(unsigned char ent)
{
	return ent & ~SWAP_HAS_CACHE;	/* may include COUNT_CONTINUED flag */
}
```



```c
#ifdef CONFIG_SWAP
static __always_inline int PageSwapCache(struct page *page)
{
#ifdef CONFIG_THP_SWAP
	page = compound_head(page);
#endif
	return PageSwapBacked(page) && test_bit(PG_swapcache, &page->flags); // PG_swapcache 和 PG_swapbacked 的关系是什么 ?

}
```



## swapoff : 对于 swapfile 终极理解(1)
1. 这个东西为什么最终会调用 page_add_new_anon_rmap
2. try_to_unuse 了解一下

```c
SYSCALL_DEFINE1(swapoff, const char __user *, specialfile)
{
	struct swap_info_struct *p = NULL;
	unsigned char *swap_map;
	struct swap_cluster_info *cluster_info;
	unsigned long *frontswap_map;
	struct file *swap_file, *victim;
	struct address_space *mapping;
	struct inode *inode;
	struct filename *pathname;
	int err, found = 0;
	unsigned int old_block_size;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	BUG_ON(!current->mm);

	pathname = getname(specialfile);
	if (IS_ERR(pathname))
		return PTR_ERR(pathname);

	victim = file_open_name(pathname, O_RDWR|O_LARGEFILE, 0);
	err = PTR_ERR(victim);
	if (IS_ERR(victim))
		goto out;

	mapping = victim->f_mapping;
	spin_lock(&swap_lock);
	plist_for_each_entry(p, &swap_active_head, list) {
		if (p->flags & SWP_WRITEOK) {
			if (p->swap_file->f_mapping == mapping) {
				found = 1;
				break;
			}
		}
	}
	if (!found) {
		err = -EINVAL;
		spin_unlock(&swap_lock);
		goto out_dput;
	}
	if (!security_vm_enough_memory_mm(current->mm, p->pages))
		vm_unacct_memory(p->pages);
	else {
		err = -ENOMEM;
		spin_unlock(&swap_lock);
		goto out_dput;
	}
	del_from_avail_list(p);
	spin_lock(&p->lock);
	if (p->prio < 0) {
		struct swap_info_struct *si = p;
		int nid;

		plist_for_each_entry_continue(si, &swap_active_head, list) {
			si->prio++;
			si->list.prio--;
			for_each_node(nid) {
				if (si->avail_lists[nid].prio != 1)
					si->avail_lists[nid].prio--;
			}
		}
		least_priority++;
	}
	plist_del(&p->list, &swap_active_head);
	atomic_long_sub(p->pages, &nr_swap_pages);
	total_swap_pages -= p->pages;
	p->flags &= ~SWP_WRITEOK;
	spin_unlock(&p->lock);
	spin_unlock(&swap_lock);

	disable_swap_slots_cache_lock();

	set_current_oom_origin();
	err = try_to_unuse(p->type, false, 0); /* force unuse all pages */
	clear_current_oom_origin();

	if (err) {
		/* re-insert swap space back into swap_list */
		reinsert_swap_info(p);
		reenable_swap_slots_cache_unlock();
		goto out_dput;
	}

	reenable_swap_slots_cache_unlock();

	spin_lock(&swap_lock);
	spin_lock(&p->lock);
	p->flags &= ~SWP_VALID;		/* mark swap device as invalid */
	spin_unlock(&p->lock);
	spin_unlock(&swap_lock);
	/*
	 * wait for swap operations protected by get/put_swap_device()
	 * to complete
	 */
	synchronize_rcu();

	flush_work(&p->discard_work);

	destroy_swap_extents(p);
	if (p->flags & SWP_CONTINUED)
		free_swap_count_continuations(p);

	if (!p->bdev || !blk_queue_nonrot(bdev_get_queue(p->bdev)))
		atomic_dec(&nr_rotate_swap);

	mutex_lock(&swapon_mutex);
	spin_lock(&swap_lock);
	spin_lock(&p->lock);
	drain_mmlist();

	/* wait for anyone still in scan_swap_map */
	p->highest_bit = 0;		/* cuts scans short */
	while (p->flags >= SWP_SCANNING) {
		spin_unlock(&p->lock);
		spin_unlock(&swap_lock);
		schedule_timeout_uninterruptible(1);
		spin_lock(&swap_lock);
		spin_lock(&p->lock);
	}

	swap_file = p->swap_file;
	old_block_size = p->old_block_size;
	p->swap_file = NULL;
	p->max = 0;
	swap_map = p->swap_map;
	p->swap_map = NULL;
	cluster_info = p->cluster_info;
	p->cluster_info = NULL;
	frontswap_map = frontswap_map_get(p);
	spin_unlock(&p->lock);
	spin_unlock(&swap_lock);
	frontswap_invalidate_area(p->type);
	frontswap_map_set(p, NULL);
	mutex_unlock(&swapon_mutex);
	free_percpu(p->percpu_cluster);
	p->percpu_cluster = NULL;
	vfree(swap_map);
	kvfree(cluster_info);
	kvfree(frontswap_map);
	/* Destroy swap account information */
	swap_cgroup_swapoff(p->type);
	exit_swap_address_space(p->type);

	inode = mapping->host;
	if (S_ISBLK(inode->i_mode)) {
		struct block_device *bdev = I_BDEV(inode);

		set_blocksize(bdev, old_block_size);
		blkdev_put(bdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
	}

	inode_lock(inode);
	inode->i_flags &= ~S_SWAPFILE;
	inode_unlock(inode);
	filp_close(swap_file, NULL);

	/*
	 * Clear the SWP_USED flag after all resources are freed so that swapon
	 * can reuse this swap_info in alloc_swap_info() safely.  It is ok to
	 * not hold p->lock after we cleared its SWP_WRITEOK.
	 */
	spin_lock(&swap_lock);
	p->flags = 0;
	spin_unlock(&swap_lock);

	err = 0;
	atomic_inc(&proc_poll_event);
	wake_up_interruptible(&proc_poll_wait);

out_dput:
	filp_close(victim, NULL);
out:
	putname(pathname);
	return err;
}

```


## swapon : 对于 swapfile 终极理解(2)
1. specialfile 是一个字符串，这里是文件还是 dev 有没有区分处理
2. priority 体现在什么地方 ?

```c
SYSCALL_DEFINE2(swapon, const char __user *, specialfile, int, swap_flags)
{
	struct swap_info_struct *p;
	struct filename *name;
	struct file *swap_file = NULL;
	struct address_space *mapping;
	int prio;
	int error;
	union swap_header *swap_header;
	int nr_extents;
	sector_t span;
	unsigned long maxpages;
	unsigned char *swap_map = NULL;
	struct swap_cluster_info *cluster_info = NULL;
	unsigned long *frontswap_map = NULL;
	struct page *page = NULL;
	struct inode *inode = NULL;
	bool inced_nr_rotate_swap = false;

	if (swap_flags & ~SWAP_FLAGS_VALID)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (!swap_avail_heads)
		return -ENOMEM;

  // swap_info_struct 持有 swap cache 的基本信息
	p = alloc_swap_info();
	if (IS_ERR(p))
		return PTR_ERR(p);

	INIT_WORK(&p->discard_work, swap_discard_work);

  // getname 将用户态的字符串"拷贝"到转化为内核态
	name = getname(specialfile);
	if (IS_ERR(name)) {
		error = PTR_ERR(name);
		name = NULL;
		goto bad_swap;
	}

  // 可能是 name 表示可能是文件，但是也可能是设备
	swap_file = file_open_name(name, O_RDWR|O_LARGEFILE, 0);
	if (IS_ERR(swap_file)) {
		error = PTR_ERR(swap_file);
		swap_file = NULL;
		goto bad_swap;
	}

	p->swap_file = swap_file;
	mapping = swap_file->f_mapping;
	inode = mapping->host;

  // 设置 p->bdev，对于文件和 block dev 区别处理
	/* If S_ISREG(inode->i_mode) will do inode_lock(inode); */
	error = claim_swapfile(p, inode);
	if (unlikely(error))
		goto bad_swap;

	/*
	 * Read the swap header.
	 */
	if (!mapping->a_ops->readpage) {
		error = -EINVAL;
		goto bad_swap;
	}
  // 读取 sawp_header
	page = read_mapping_page(mapping, 0, swap_file);
	if (IS_ERR(page)) {
		error = PTR_ERR(page);
		goto bad_swap;
	}
	swap_header = kmap(page);

  // 解析 swap_header
	maxpages = read_swap_header(p, swap_header, inode);
	if (unlikely(!maxpages)) {
		error = -EINVAL;
		goto bad_swap;
	}

  // 这就是那个用于存储每一个 page 的 counter 地方
	/* OK, set up the swap map and apply the bad block list */
	swap_map = vzalloc(maxpages);
	if (!swap_map) {
		error = -ENOMEM;
		goto bad_swap;
	}

  // 权限检查
	if (bdi_cap_stable_pages_required(inode_to_bdi(inode)))
		p->flags |= SWP_STABLE_WRITES;

  // 权限检查
	if (bdi_cap_synchronous_io(inode_to_bdi(inode)))
		p->flags |= SWP_SYNCHRONOUS_IO;

  // 初始化 cluster 相关的
  // 一个 swapfile 会被拆分为 cluster
	if (p->bdev && blk_queue_nonrot(bdev_get_queue(p->bdev))) {
		int cpu;
		unsigned long ci, nr_cluster;

		p->flags |= SWP_SOLIDSTATE;
		/*
		 * select a random position to start with to help wear leveling
		 * SSD
		 */
		p->cluster_next = 1 + (prandom_u32() % p->highest_bit);
		nr_cluster = DIV_ROUND_UP(maxpages, SWAPFILE_CLUSTER);

		cluster_info = kvcalloc(nr_cluster, sizeof(*cluster_info),
					GFP_KERNEL);
		if (!cluster_info) {
			error = -ENOMEM;
			goto bad_swap;
		}

		for (ci = 0; ci < nr_cluster; ci++)
			spin_lock_init(&((cluster_info + ci)->lock));

		p->percpu_cluster = alloc_percpu(struct percpu_cluster);
		if (!p->percpu_cluster) {
			error = -ENOMEM;
			goto bad_swap;
		}
		for_each_possible_cpu(cpu) {
			struct percpu_cluster *cluster;
			cluster = per_cpu_ptr(p->percpu_cluster, cpu);
			cluster_set_null(&cluster->index);
		}
	} else {
		atomic_inc(&nr_rotate_swap);
		inced_nr_rotate_swap = true;
	}

  // cgroup 跳过
	error = swap_cgroup_swapon(p->type, maxpages);
	if (error)
		goto bad_swap;

  // 初始化 cluster 和 extents
	nr_extents = setup_swap_map_and_extents(p, swap_header, swap_map,
		cluster_info, maxpages, &span);
	if (unlikely(nr_extents < 0)) {
		error = nr_extents;
		goto bad_swap;
	}
	/* frontswap enabled? set up bit-per-page map for frontswap */
	if (IS_ENABLED(CONFIG_FRONTSWAP))
		frontswap_map = kvcalloc(BITS_TO_LONGS(maxpages),
					 sizeof(long),
					 GFP_KERNEL);

	if (p->bdev &&(swap_flags & SWAP_FLAG_DISCARD) && swap_discardable(p)) { // todo 所以 request queue 的 discard 是什么意思 ?
		/*
		 * When discard is enabled for swap with no particular
		 * policy flagged, we set all swap discard flags here in
		 * order to sustain backward compatibility with older
		 * swapon(8) releases.
		 */
		p->flags |= (SWP_DISCARDABLE | SWP_AREA_DISCARD |
			     SWP_PAGE_DISCARD);

		/*
		 * By flagging sys_swapon, a sysadmin can tell us to
		 * either do single-time area discards only, or to just
		 * perform discards for released swap page-clusters.
		 * Now it's time to adjust the p->flags accordingly.
		 */
		if (swap_flags & SWAP_FLAG_DISCARD_ONCE)
			p->flags &= ~SWP_PAGE_DISCARD;
		else if (swap_flags & SWAP_FLAG_DISCARD_PAGES)
			p->flags &= ~SWP_AREA_DISCARD;

		/* issue a swapon-time discard if it's still required */
		if (p->flags & SWP_AREA_DISCARD) {
			int err = discard_swap(p);
			if (unlikely(err))
				pr_err("swapon: discard_swap(%p): %d\n",
					p, err);
		}
	}

  // 初始化 address_space
  // p->type 就是编号
	error = init_swap_address_space(p->type, maxpages);
	if (error)
		goto bad_swap;

	/*
	 * Flush any pending IO and dirty mappings before we start using this
	 * swap device.
	 */
	inode->i_flags |= S_SWAPFILE;
	error = inode_drain_writes(inode);
	if (error) {
		inode->i_flags &= ~S_SWAPFILE;
		goto bad_swap;
	}

	mutex_lock(&swapon_mutex);
	prio = -1;
	if (swap_flags & SWAP_FLAG_PREFER)
		prio =
		  (swap_flags & SWAP_FLAG_PRIO_MASK) >> SWAP_FLAG_PRIO_SHIFT;

　// 设置权限以及加入到 global list 中间
  // 分析其中的  _enable_swap_info 和 setup_swap_info
	enable_swap_info(p, prio, swap_map, cluster_info, frontswap_map);

	pr_info("Adding %uk swap on %s.  Priority:%d extents:%d across:%lluk %s%s%s%s%s\n",
		p->pages<<(PAGE_SHIFT-10), name->name, p->prio,
		nr_extents, (unsigned long long)span<<(PAGE_SHIFT-10),
		(p->flags & SWP_SOLIDSTATE) ? "SS" : "",
		(p->flags & SWP_DISCARDABLE) ? "D" : "",
		(p->flags & SWP_AREA_DISCARD) ? "s" : "",
		(p->flags & SWP_PAGE_DISCARD) ? "c" : "",
		(frontswap_map) ? "FS" : "");

	mutex_unlock(&swapon_mutex);
	atomic_inc(&proc_poll_event);
	wake_up_interruptible(&proc_poll_wait);

	error = 0;
	goto out;
bad_swap:
	free_percpu(p->percpu_cluster);
	p->percpu_cluster = NULL;
	if (inode && S_ISBLK(inode->i_mode) && p->bdev) {
		set_blocksize(p->bdev, p->old_block_size);
		blkdev_put(p->bdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
	}
	destroy_swap_extents(p);
	swap_cgroup_swapoff(p->type);
	spin_lock(&swap_lock);
	p->swap_file = NULL;
	p->flags = 0;
	spin_unlock(&swap_lock);
	vfree(swap_map);
	kvfree(cluster_info);
	kvfree(frontswap_map);
	if (inced_nr_rotate_swap)
		atomic_dec(&nr_rotate_swap);
	if (swap_file) {
		if (inode) {
			inode_unlock(inode);
			inode = NULL;
		}
		filp_close(swap_file, NULL);
	}
out:
	if (page && !IS_ERR(page)) {
		kunmap(page);
		put_page(page);
	}
	if (name)
		putname(name);
	if (inode)
		inode_unlock(inode);
	if (!error)
		enable_swap_slots_cache();
	return error;
}
```


```c
/*
 * Flush file data before changing attributes.  Caller must hold any locks
 * required to prevent further writes to this file until we're done setting
 * flags.
 */
static inline int inode_drain_writes(struct inode *inode)
{
	inode_dio_wait(inode);
	return filemap_write_and_wait(inode->i_mapping);
}
```


## try_to_unuse : 非常的长，也不知道在说什么 @todo
1. 两次 while 循环

```c
/*
 * If the boolean frontswap is true, only unuse pages_to_unuse pages;
 * pages_to_unuse==0 means all pages; ignored if frontswap is false
 */
int try_to_unuse(unsigned int type, bool frontswap,
		 unsigned long pages_to_unuse)
{
	struct mm_struct *prev_mm;
	struct mm_struct *mm;
	struct list_head *p;
	int retval = 0;
	struct swap_info_struct *si = swap_info[type];
	struct page *page;
	swp_entry_t entry;
	unsigned int i;

	if (!si->inuse_pages)
		return 0;

	if (!frontswap)
		pages_to_unuse = 0;

retry:
	retval = shmem_unuse(type, frontswap, &pages_to_unuse); // todo 为什么这么关键 ?
	if (retval)
		goto out;

	prev_mm = &init_mm;
	mmget(prev_mm);

	spin_lock(&mmlist_lock);
	p = &init_mm.mmlist;
	while (si->inuse_pages &&
	       !signal_pending(current) &&
	       (p = p->next) != &init_mm.mmlist) {

		mm = list_entry(p, struct mm_struct, mmlist);
		if (!mmget_not_zero(mm))
			continue;
		spin_unlock(&mmlist_lock);
		mmput(prev_mm);
		prev_mm = mm;
		retval = unuse_mm(mm, type, frontswap, &pages_to_unuse);

		if (retval) {
			mmput(prev_mm);
			goto out;
		}

		/*
		 * Make sure that we aren't completely killing
		 * interactive performance.
		 */
		cond_resched();
		spin_lock(&mmlist_lock);
	}
	spin_unlock(&mmlist_lock);

	mmput(prev_mm);

	i = 0;
	while (si->inuse_pages &&
	       !signal_pending(current) &&
	       (i = find_next_to_unuse(si, i, frontswap)) != 0) {

		entry = swp_entry(type, i);
		page = find_get_page(swap_address_space(entry), i);
		if (!page)
			continue;

		/*
		 * It is conceivable that a racing task removed this page from
		 * swap cache just before we acquired the page lock. The page
		 * might even be back in swap cache on another swap area. But
		 * that is okay, try_to_free_swap() only removes stale pages.
		 */
		lock_page(page);
		wait_on_page_writeback(page);
		try_to_free_swap(page);
		unlock_page(page);
		put_page(page);

		/*
		 * For frontswap, we just need to unuse pages_to_unuse, if
		 * it was specified. Need not check frontswap again here as
		 * we already zeroed out pages_to_unuse if not frontswap.
		 */
		if (pages_to_unuse && --pages_to_unuse == 0)
			goto out;
	}

	/*
	 * Lets check again to see if there are still swap entries in the map.
	 * If yes, we would need to do retry the unuse logic again.
	 * Under global memory pressure, swap entries can be reinserted back
	 * into process space after the mmlist loop above passes over them.
	 *
	 * Limit the number of retries? No: when mmget_not_zero() above fails,
	 * that mm is likely to be freeing swap from exit_mmap(), which proceeds
	 * at its own independent pace; and even shmem_writepage() could have
	 * been preempted after get_swap_page(), temporarily hiding that swap.
	 * It's easy and robust (though cpu-intensive) just to keep retrying.
	 */
	if (si->inuse_pages) {
		if (!signal_pending(current))
			goto retry;
		retval = -EINTR;
	}
out:
	return (retval == FRONTSWAP_PAGES_UNUSED) ? 0 : retval;
}
```


```c
static int unuse_mm(struct mm_struct *mm, unsigned int type,
		    bool frontswap, unsigned long *fs_pages_to_unuse) // 为什么参数会有 mm_struct
{
	struct vm_area_struct *vma;
	int ret = 0;

	down_read(&mm->mmap_sem);
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (vma->anon_vma) { // 看到没有，只有 anon，
			ret = unuse_vma(vma, type, frontswap,
					fs_pages_to_unuse);
			if (ret)
				break;
		}
		cond_resched();
	}
	up_read(&mm->mmap_sem);
	return ret;
}
```


```c
static int unuse_vma(struct vm_area_struct *vma, unsigned int type,
		     bool frontswap, unsigned long *fs_pages_to_unuse)
{
	pgd_t *pgd;
	unsigned long addr, end, next;
	int ret;

	addr = vma->vm_start;
	end = vma->vm_end;

	pgd = pgd_offset(vma->vm_mm, addr);
	do {
		next = pgd_addr_end(addr, end);
		if (pgd_none_or_clear_bad(pgd))
			continue;
		ret = unuse_p4d_range(vma, pgd, addr, next, type, // 然后递归向上
				      frontswap, fs_pages_to_unuse);
		if (ret)
			return ret;
	} while (pgd++, addr = next, addr != end);
	return 0;
}
```

## mmget

```c
/**
 * mmget() - Pin the address space associated with a &struct mm_struct.
 * @mm: The address space to pin.
 *
 * Make sure that the address space of the given &struct mm_struct doesn't
 * go away. This does not protect against parts of the address space being
 * modified or freed, however.
 *
 * Never use this function to pin this address space for an
 * unbounded/indefinite amount of time.
 *
 * Use mmput() to release the reference acquired by mmget().
 *
 * See also <Documentation/vm/active_mm.rst> for an in-depth explanation
 * of &mm_struct.mm_count vs &mm_struct.mm_users.
 */
static inline void mmget(struct mm_struct *mm)
{
	atomic_inc(&mm->mm_users);
}

static inline bool mmget_not_zero(struct mm_struct *mm)
{
	return atomic_inc_not_zero(&mm->mm_users);
}
```

## 各种 swap page count
1. 四个函数没有实质区别，都是返回 swap_map 数组上的数值

```c
/*
 * How many references to page are currently swapped out?
 * This does not give an exact answer when swap count is continued,
 * but does include the high COUNT_CONTINUED flag to allow for that.
 */
int page_swapcount(struct page *page)
{
	int count = 0;
	struct swap_info_struct *p;
	struct swap_cluster_info *ci;
	swp_entry_t entry;
	unsigned long offset;

	entry.val = page_private(page);
	p = _swap_info_get(entry);
	if (p) {
		offset = swp_offset(entry);
		ci = lock_cluster_or_swap_info(p, offset);
		count = swap_count(p->swap_map[offset]);
		unlock_cluster_or_swap_info(p, ci);
	}
	return count;
}

int __swap_count(swp_entry_t entry)
{
	struct swap_info_struct *si;
	pgoff_t offset = swp_offset(entry);
	int count = 0;

	si = get_swap_device(entry);
	if (si) {
		count = swap_count(si->swap_map[offset]);
		put_swap_device(si);
	}
	return count;
}

static int swap_swapcount(struct swap_info_struct *si, swp_entry_t entry)
{
	int count = 0;
	pgoff_t offset = swp_offset(entry);
	struct swap_cluster_info *ci;

	ci = lock_cluster_or_swap_info(si, offset);
	count = swap_count(si->swap_map[offset]);
	unlock_cluster_or_swap_info(si, ci);
	return count;
}

/*
 * How many references to @entry are currently swapped out?
 * This does not give an exact answer when swap count is continued,
 * but does include the high COUNT_CONTINUED flag to allow for that.
 */
int __swp_swapcount(swp_entry_t entry)
{
	int count = 0;
	struct swap_info_struct *si;

	si = get_swap_device(entry);
	if (si) {
		count = swap_swapcount(si, entry);
		put_swap_device(si);
	}
	return count;
}

/*
 * How many references to @entry are currently swapped out?
 * This considers COUNT_CONTINUED so it returns exact answer.
 */
int swp_swapcount(swp_entry_t entry)
{
	int count, tmp_count, n;
	struct swap_info_struct *p;
	struct swap_cluster_info *ci;
	struct page *page;
	pgoff_t offset;
	unsigned char *map;

	p = _swap_info_get(entry);
	if (!p)
		return 0;

	offset = swp_offset(entry);

	ci = lock_cluster_or_swap_info(p, offset);

	count = swap_count(p->swap_map[offset]);
	if (!(count & COUNT_CONTINUED))
		goto out;

	count &= ~COUNT_CONTINUED;
	n = SWAP_MAP_MAX + 1;

	page = vmalloc_to_page(p->swap_map + offset);
	offset &= ~PAGE_MASK;
	VM_BUG_ON(page_private(page) != SWP_CONTINUED);

	do {
		page = list_next_entry(page, lru);
		map = kmap_atomic(page);
		tmp_count = map[offset];
		kunmap_atomic(map);

		count += (tmp_count & ~COUNT_CONTINUED) * n;
		n *= (SWAP_CONT_MAX + 1);
	} while (tmp_count & COUNT_CONTINUED);
out:
	unlock_cluster_or_swap_info(p, ci);
	return count;
```



## swap_free

```c
/*
 * Caller has made sure that the swap device corresponding to entry
 * is still around or has not been recycled.
 */
void swap_free(swp_entry_t entry)
{
	struct swap_info_struct *p;

	p = _swap_info_get(entry);
	if (p)　// 当 usage count 为 0 的时候释放，每次 pgfault 的时候处理一下
		__swap_entry_free(p, entry, 1);
}
```

1. 进行一些释放工作
```c
static unsigned char __swap_entry_free(struct swap_info_struct *p,
				       swp_entry_t entry, unsigned char usage)
{
	struct swap_cluster_info *ci;
	unsigned long offset = swp_offset(entry);

	ci = lock_cluster_or_swap_info(p, offset);
	usage = __swap_entry_free_locked(p, offset, usage);
	unlock_cluster_or_swap_info(p, ci);
	if (!usage)
		free_swap_slot(entry); // todo swap_slots.c 中间的内容，不会太复杂吧!

	return usage;
}

static unsigned char __swap_entry_free_locked(struct swap_info_struct *p,
					      unsigned long offset,
					      unsigned char usage)
{
	unsigned char count;
	unsigned char has_cache;

	count = p->swap_map[offset];

	has_cache = count & SWAP_HAS_CACHE;
	count &= ~SWAP_HAS_CACHE;

	if (usage == SWAP_HAS_CACHE) {
		VM_BUG_ON(!has_cache);
		has_cache = 0;
	} else if (count == SWAP_MAP_SHMEM) {
		/*
		 * Or we could insist on shmem.c using a special
		 * swap_shmem_free() and free_shmem_swap_and_cache()...
		 */
		count = 0;
	} else if ((count & ~COUNT_CONTINUED) <= SWAP_MAP_MAX) {
		if (count == COUNT_CONTINUED) {
			if (swap_count_continued(p, offset, count)) // todo 这个东西有点复杂
				count = SWAP_MAP_MAX | COUNT_CONTINUED;
			else
				count = SWAP_MAP_MAX;
		} else
			count--;
	}

	usage = count | has_cache;
	p->swap_map[offset] = usage ? : SWAP_HAS_CACHE;

	return usage;
}


```


2. 判断是否真的存在
```c
static struct swap_info_struct *_swap_info_get(swp_entry_t entry)
{
	struct swap_info_struct *p;

	p = __swap_info_get(entry);
	if (!p)
		goto out;
	if (!p->swap_map[swp_offset(entry)])
		goto bad_free;
	return p;

bad_free:
	pr_err("swap_info_get: %s%08lx\n", Unused_offset, entry.val);
	goto out;
out:
	return NULL;
}

static struct swap_info_struct *__swap_info_get(swp_entry_t entry) // 获取一个 swap_info_struct 而已，搞得这么麻烦!
{
	struct swap_info_struct *p;
	unsigned long offset;

	if (!entry.val)
		goto out;
	p = swp_swap_info(entry);
	if (!p)
		goto bad_nofile;
	if (!(p->flags & SWP_USED))
		goto bad_device;
	offset = swp_offset(entry);
	if (offset >= p->max)
		goto bad_offset;
	return p;

bad_offset:
	pr_err("swap_info_get: %s%08lx\n", Bad_offset, entry.val);
	goto out;
bad_device:
	pr_err("swap_info_get: %s%08lx\n", Unused_file, entry.val);
	goto out;
bad_nofile:
	pr_err("swap_info_get: %s%08lx\n", Bad_file, entry.val);
out:
	return NULL;
}

struct swap_info_struct *swp_swap_info(swp_entry_t entry)
{
	return swap_type_to_swap_info(swp_type(entry));
}


static struct swap_info_struct *swap_type_to_swap_info(int type)
{
	if (type >= READ_ONCE(nr_swapfiles))
		return NULL;

	smp_rmb();	/* Pairs with smp_wmb in alloc_swap_info. */
	return READ_ONCE(swap_info[type]);
}

/*
 * Extract the `type' field from a swp_entry_t.  The swp_entry_t is in
 * arch-independent format
 */
static inline unsigned swp_type(swp_entry_t entry)
{
	return (entry.val >> SWP_TYPE_SHIFT);
}

struct swap_info_struct *swap_info[MAX_SWAPFILES];

/*
 * Extract the `offset' field from a swp_entry_t.  The swp_entry_t is in
 * arch-independent format
 */
static inline pgoff_t swp_offset(swp_entry_t entry)
{
	return entry.val & SWP_OFFSET_MASK;
}

/*
 * swapcache pages are stored in the swapper_space radix tree.  We want to
 * get good packing density in that tree, so the index should be dense in
 * the low-order bits.
 *
 * We arrange the `type' and `offset' fields so that `type' is at the seven
 * high-order bits of the swp_entry_t and `offset' is right-aligned in the
 * remaining bits.  Although `type' itself needs only five bits, we allow for
 * shmem/tmpfs to shift it all up a further two bits: see swp_to_radix_entry().
 *
 * swp_entry_t's are *never* stored anywhere in their arch-dependent format.
 */
#define SWP_TYPE_SHIFT	(BITS_PER_XA_VALUE - MAX_SWAPFILES_SHIFT)
#define SWP_OFFSET_MASK	((1UL << SWP_TYPE_SHIFT) - 1)
// 非常的人性化，前面存储 offset ，后面存储 type
```

## reuse_swap_page : gp fault 辅助函数，reuse 没有 references 的 page
```c
/*
 * We can write to an anon page without COW if there are no other references
 * to it.  And as a side-effect, free up its swap: because the old content
 * on disk will never be read, and seeking back there to write new content
 * later would only waste time away from clustering.
 *
 * NOTE: total_map_swapcount should not be relied upon by the caller if
 * reuse_swap_page() returns false, but it may be always overwritten
 * (see the other implementation for CONFIG_SWAP=n).
 */
bool reuse_swap_page(struct page *page, int *total_map_swapcount)
{
	int count, total_mapcount, total_swapcount;

	VM_BUG_ON_PAGE(!PageLocked(page), page);
	if (unlikely(PageKsm(page)))
		return false;
	count = page_trans_huge_map_swapcount(page, &total_mapcount,
					      &total_swapcount);
	if (total_map_swapcount)
		*total_map_swapcount = total_mapcount + total_swapcount;
	if (count == 1 && PageSwapCache(page) &&
	    (likely(!PageTransCompound(page)) ||
	     /* The remaining swap count will be freed soon */
	     total_swapcount == page_swapcount(page))) {
		if (!PageWriteback(page)) {
			page = compound_head(page);
			delete_from_swap_cache(page);
			SetPageDirty(page);
		} else {
			swp_entry_t entry;
			struct swap_info_struct *p;

			entry.val = page_private(page);
			p = swap_info_get(entry);
			if (p->flags & SWP_STABLE_WRITES) {
				spin_unlock(&p->lock);
				return false;
			}
			spin_unlock(&p->lock);
		}
	}

	return count <= 1;
}
```


## setup_swap_map_and_extents

```c
static int setup_swap_map_and_extents(struct swap_info_struct *p,
					union swap_header *swap_header, // swapfile 中间读入的基本信息
					unsigned char *swap_map,  // swap_map = vzalloc(maxpages)
					struct swap_cluster_info *cluster_info, // 上层分配的一组 swap_cluster_info，其中的 lock 被初始化，另外的两个变量需要被初始化
					unsigned long maxpages, // 从 swap_header 中间读取的，反映了容量的大小
					sector_t *span) // 返回值
{
	unsigned int j, k;
	unsigned int nr_good_pages;
	int nr_extents;
	unsigned long nr_clusters = DIV_ROUND_UP(maxpages, SWAPFILE_CLUSTER);
	unsigned long col = p->cluster_next / SWAPFILE_CLUSTER % SWAP_CLUSTER_COLS; // p->cluster_next 被初始化为随机数值
	unsigned long i, idx;

	nr_good_pages = maxpages - 1;	/* omit header page */

	cluster_list_init(&p->free_clusters);
	cluster_list_init(&p->discard_clusters);

  // 将 badpage 标记为被使用
  // cluster 持有了该 cluster 中间一共被使用的 page 数量
	for (i = 0; i < swap_header->info.nr_badpages; i++) {
		unsigned int page_nr = swap_header->info.badpages[i];
		if (page_nr == 0 || page_nr > swap_header->info.last_page)
			return -EINVAL;
		if (page_nr < maxpages) {
			swap_map[page_nr] = SWAP_MAP_BAD;
			nr_good_pages--;
			/*
			 * Haven't marked the cluster free yet, no list
			 * operation involved
			 */
			inc_cluster_info_page(p, cluster_info, page_nr);
		}
	}

  // 将由于多余的 page 清理掉
	/* Haven't marked the cluster free yet, no list operation involved */
	for (i = maxpages; i < round_up(maxpages, SWAPFILE_CLUSTER); i++)
		inc_cluster_info_page(p, cluster_info, i);

	if (nr_good_pages) {
		swap_map[0] = SWAP_MAP_BAD;
		/*
		 * Not mark the cluster free yet, no list
		 * operation involved
		 */
    // 第一个 page 应该是是用于存储
		inc_cluster_info_page(p, cluster_info, 0);
		p->max = maxpages;
		p->pages = nr_good_pages;
    // 初始化 extents
		nr_extents = setup_swap_extents(p, span);
		if (nr_extents < 0)
			return nr_extents;
		nr_good_pages = p->pages;
	}
	if (!nr_good_pages) {
		pr_warn("Empty swap-file\n");
		return -EINVAL;
	}

	if (!cluster_info)
		return nr_extents;

  // 其实方法是 : free_clusters 中间的 cluster_info 排布方式 :
  // 不同组(放到 cache line 的会放到一起的)将会连续排练，如果是顺序访问，
  // 那么就不会互相 cache line 冲突了
	/*
	 * Reduce false cache line sharing between cluster_info and
	 * sharing same address space.
	 */
	for (k = 0; k < SWAP_CLUSTER_COLS; k++) { // 单个
		j = (k + col) % SWAP_CLUSTER_COLS;
		for (i = 0; i < DIV_ROUND_UP(nr_clusters, SWAP_CLUSTER_COLS); i++) { // 所有的 cluster
			idx = i * SWAP_CLUSTER_COLS + j;
			if (idx >= nr_clusters) // idx 越界
				continue;
			if (cluster_count(&cluster_info[idx])) // 不是 free 的
				continue;
			cluster_set_flag(&cluster_info[idx], CLUSTER_FLAG_FREE); // 其余全部初始化为 free
			cluster_list_add_tail(&p->free_clusters, cluster_info, // 以及加入到链表后面
					      idx);
		}
	}
	return nr_extents;
}

// 一个 L1_CACHE_BYTES 存储的 swap_cluster_info 的个数
// 一个 address_space 存储的 cluster 的数量
#define SWAP_CLUSTER_INFO_COLS						\
	DIV_ROUND_UP(L1_CACHE_BYTES, sizeof(struct swap_cluster_info))
#define SWAP_CLUSTER_SPACE_COLS						\
	DIV_ROUND_UP(SWAP_ADDRESS_SPACE_PAGES, SWAPFILE_CLUSTER)
#define SWAP_CLUSTER_COLS						\
	max_t(unsigned int, SWAP_CLUSTER_INFO_COLS, SWAP_CLUSTER_SPACE_COLS)
```


```c
static void cluster_list_init(struct swap_cluster_list *list)
{
	cluster_set_null(&list->head);
	cluster_set_null(&list->tail);
}

static inline void cluster_set_null(struct swap_cluster_info *info)
{
	info->flags = CLUSTER_FLAG_NEXT_NULL;
	info->data = 0;
}

#define CLUSTER_FLAG_NEXT_NULL 2 /* This cluster has no next cluster */
```



```c
struct swap_cluster_list {
	struct swap_cluster_info head;
	struct swap_cluster_info tail;
};

/*
 * We use this to track usage of a cluster. A cluster is a block of swap disk
 * space with SWAPFILE_CLUSTER pages long and naturally aligns in disk. All
 * free clusters are organized into a list. We fetch an entry from the list to
 * get a free cluster.
 *
 * The data field stores next cluster if the cluster is free or cluster usage
 * counter otherwise. The flags field determines if a cluster is free. This is
 * protected by swap_info_struct.lock.
 */
struct swap_cluster_info {
	spinlock_t lock;	/*
				 * Protect swap_cluster_info fields
				 * and swap_info_struct->swap_map
				 * elements correspond to the swap
				 * cluster
				 */
	unsigned int data:24; // 当存储数据的时候，表示其中一共多少空余，否则可能当做next使用
	unsigned int flags:8;
};

/*
 * The cluster corresponding to page_nr will be used. The cluster will be
 * removed from free cluster list and its usage counter will be increased.
 */
static void inc_cluster_info_page(struct swap_info_struct *p,
	struct swap_cluster_info *cluster_info, unsigned long page_nr)
{
	unsigned long idx = page_nr / SWAPFILE_CLUSTER; // 获取 page_nr 所在的　cluster_info 所在的编号

	if (!cluster_info)
		return;
	if (cluster_is_free(&cluster_info[idx]))
		alloc_cluster(p, idx);

	VM_BUG_ON(cluster_count(&cluster_info[idx]) >= SWAPFILE_CLUSTER);
	cluster_set_count(&cluster_info[idx],
		cluster_count(&cluster_info[idx]) + 1); // 终于，完成其工作
}

static void alloc_cluster(struct swap_info_struct *si, unsigned long idx) // idx 是 cluster 的地址
{
	struct swap_cluster_info *ci = si->cluster_info;

  // si->free_clusters 应该是所有的完全的 free_clusters 吧!
	VM_BUG_ON(cluster_list_first(&si->free_clusters) != idx); // alloc 的内容总是需要从 si->free_clusters 这里
	cluster_list_del_first(&si->free_clusters, ci);
	cluster_set_count_flag(ci + idx, 0, 0);
}

static inline unsigned int cluster_list_first(struct swap_cluster_list *list)
{
	return cluster_next(&list->head);
}

static inline unsigned int cluster_next(struct swap_cluster_info *info)
{
	return info->data;
}
```


```c
static unsigned int cluster_list_del_first(struct swap_cluster_list *list,
					   struct swap_cluster_info *ci)
{
	unsigned int idx;

	idx = cluster_next(&list->head);
	if (cluster_next(&list->tail) == idx) { // 这个已释放，
		cluster_set_null(&list->head);
		cluster_set_null(&list->tail);
	} else
		cluster_set_next_flag(&list->head,
				      cluster_next(&ci[idx]), 0); // 将 head 指向 ci 的 next

	return idx;
}
```


## setup_swap_extents : 彻底理解 extents 机制
1. 所以，extents 是如何被使用的 ?

```c
/*
 * A `swap extent' is a simple thing which maps a contiguous range of pages
 * onto a contiguous range of disk blocks.  An ordered list of swap extents
 * is built at swapon time and is then used at swap_writepage/swap_readpage
 * time for locating where on disk a page belongs.
 *
 * If the swapfile is an S_ISBLK block device, a single extent is installed.
 * This is done so that the main operating code can treat S_ISBLK and S_ISREG
 * swap files identically.
 *
 * Whether the swapdev is an S_ISREG file or an S_ISBLK blockdev, the swap
 * extent list operates in PAGE_SIZE disk blocks.  Both S_ISREG and S_ISBLK
 * swapfiles are handled *identically* after swapon time.
 *
 * For S_ISREG swapfiles, setup_swap_extents() will walk all the file's blocks
 * and will parse them into an ordered extent list, in PAGE_SIZE chunks.  If
 * some stray blocks are found which do not fall within the PAGE_SIZE alignment
 * requirements, they are simply tossed out - we will never use those blocks
 * for swapping.
 *
 * For all swap devices we set S_SWAPFILE across the life of the swapon.  This
 * prevents users from writing to the swap device, which will corrupt memory.
 *
 * The amount of disk space which a single swap extent represents varies.
 * Typically it is in the 1-4 megabyte range.  So we can have hundreds of
 * extents in the list.  To avoid much list walking, we cache the previous
 * search location in `curr_swap_extent', and start new searches from there.
 * This is extremely effective.  The average number of iterations in
 * map_swap_page() has been measured at about 0.3 per page.  - akpm.
 */
static int setup_swap_extents(struct swap_info_struct *sis, sector_t *span) // 这个注释的最后一段有点问题啊 ! 根本没有 curr_swap_extent 这个东西，而且什么叫做
{
	struct file *swap_file = sis->swap_file;
	struct address_space *mapping = swap_file->f_mapping;
	struct inode *inode = mapping->host;
	int ret;

	if (S_ISBLK(inode->i_mode)) {
		ret = add_swap_extent(sis, 0, sis->max, 0); // 对于 block 而言，初始化的时候插入一个节点即可
		*span = sis->pages; // 所有的全部可以利用起来
		return ret;
	}

	if (mapping->a_ops->swap_activate) {
		ret = mapping->a_ops->swap_activate(sis, swap_file, span);
		if (ret >= 0)
			sis->flags |= SWP_ACTIVATED;
		if (!ret) {
			sis->flags |= SWP_FS;
			ret = add_swap_extent(sis, 0, sis->max, 0);
			*span = sis->pages;
		}
		return ret;
	}

	return generic_swapfile_activate(sis, swap_file, span);
}
```

#### swapfile
// 管理其中的结构，关键机制在 cluster 和 extents 即可，基本的 IO 交给下层的 file，所以 swapfile 的功能和 ext2 的功能一致，负责下层的磁盘的布局

- [ ] delete_from_swap_cache => put_swap_page => ??
- [ ] try_to_free_swap

// 机制
1. cluster
2. extents
3. 全局的 swap_active_head
4. avail_lists : 为什么需要给每一个 node 提供孤儿
```c
/*
 * all active swap_info_structs
 * protected with swap_lock, and ordered by priority.
 */
PLIST_HEAD(swap_active_head); // TODO 是不是首先按照 swap_info_struct，然后按照 cluster


static void __del_from_avail_list(struct swap_info_struct *p)
{
  int nid;

  for_each_node(nid)
    plist_del(&p->avail_lists[nid], &swap_avail_heads[nid]);
}

static void del_from_avail_list(struct swap_info_struct *p)
{
  spin_lock(&swap_avail_lock);
  __del_from_avail_list(p);
  spin_unlock(&swap_avail_lock);
}
```



结构:
1. 一个 swapfile 对应 swap_info_struct
2. 一个 swapfile 对应多个 cluster，并且使用 cluster_info 描述


回答问题:

0. 当一个文件被设置为 swapfile 的时候，如何阻止被访问。
1. 这几个函数看似都是 free，各自的作用是什么 ?
```c
/*
 * Caller has made sure that the swap device corresponding to entry
 * is still around or has not been recycled.
 */
void swap_free(swp_entry_t entry)


/*
 * If swap is getting full, or if there are no more mappings of this page,
 * then try_to_free_swap is called to free its swap space.
 */
int try_to_free_swap(struct page *page)

// TODO 很奇怪，swap 机制为什么和 vma 联系到一起了，这不是曾经的反向映射
/*
 * We completely avoid races by reading each swap page in advance,
 * and then search for the process using it.  All the necessary
 * page table adjustments can then be made atomically.
 *
 * if the boolean frontswap is true, only unuse pages_to_unuse pages;
 * pages_to_unuse==0 means all pages; ignored if frontswap is false
 */
int try_to_unuse(unsigned int type, bool frontswap,
     unsigned long pages_to_unuse)

static int claim_swapfile(struct swap_info_struct *p, struct inode *inode)
```
从 shmem_swapin_page 的内容看，
首先调用 delete_from_swap_cache，然后调用 swap_free，前者应该是处理 swap cache 的 radix tree 维护，后者处理 swap slot 的问题。


关键函数分析，这两个函数到时候看书(ULK) 进行补充一下
1. swapon : 似乎不难，处理各种机制的建立过程
2. swapoff : 如果彻底理解 swapon，那么不难，关键 : try_to_unuse 调用两个函数
    1. shmem_unuse
    2. unuse_mm : 逐个清理

## si_swapinfo
- [ ] si_swapinfo 比想象的复杂，是因为 swapfile 导致的
