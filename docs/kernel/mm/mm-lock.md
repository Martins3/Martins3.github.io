# Memory Management Locking

- Documentation/mm/split_page_table_lock.rst
- https://mp.weixin.qq.com/s/yLM5FlYxT06axDoyTeECKg : TODO 后面的各种例子还没有看
  - 后面讨论的例子，没有想错的话，每一个问题的解决都是 lwn 的文章的
- mm/rmap.c 注释介绍了锁的层级结构
- mm/swapfile.c 的注释
```txt
/*
 * all available (active, not full) swap_info_structs
 * protected with swap_avail_lock, ordered by priority.
 * This is used by folio_alloc_swap() instead of swap_active_head
 * because swap_active_head includes all swap_info_structs,
 * but folio_alloc_swap() doesn't need to look at full ones.
 * This uses its own lock instead of swap_lock because when a
 * swap_info_struct changes between not-full/full, it needs to
 * add/remove itself to/from this list, but the swap_info_struct->lock
 * is held and the locking order requires swap_lock to be taken
 * before any swap_info_struct->lock.
 */
```

## 硬件实现的考虑
- [ ] https://blog.stuffedcow.net/2015/08/pagewalk-coherence/ 还存在一些蛇皮的 TLB pagewalk 的 coherence 问题啊

果然是存在的

## 锁类型

### PG_locked

典型的例子是获取一个页面
```txt
- filemap_fault
  - __filemap_get_folio
    - filemap_get_pages
      - filemap_add_folio
        - __folio_set_locked(设置page的PG_locked)

- filemap_read_folio
  - folio_wait_locked_killable
    - folio_wait_bit_killable(folio, PG_locked)
      - folio_wait_bit_common(等IO完成PG_locked被清除)

- mpage_read_end_io
  - folio_mark_uptodate
    - folio_unlock(IO完成时会标记page的PG_update，同时清除PG_locked)。
```

例如:
```txt
[  360.786408] "echo 0 > /proc/sys/kernel/hung_task_timeout_secs" disables this message.
[  360.786447] auditd          D ffff92ae1f41acc0     0  1274      1 0x00000000
[  360.786448] Call Trace:
[  360.786455]  [<ffffffffc0527f3c>] ? __es_remove_extent+0x5c/0x2e0 [ext4]
[  360.786456]  [<ffffffffa4585f20>] ? bit_wait+0x50/0x50
[  360.786457]  [<ffffffffa4587df9>] schedule+0x29/0x70
[  360.786458]  [<ffffffffa45858e1>] schedule_timeout+0x221/0x2d0
[  360.786460]  [<ffffffffc03b9df7>] ? do_get_write_access+0x357/0x4f0 [jbd2]
[  360.786461]  [<ffffffffa4086e8d>] ? __getblk+0x2d/0x2e0
[  360.786463]  [<ffffffffa3e6d3fe>] ? kvm_clock_get_cycles+0x1e/0x20
[  360.786464]  [<ffffffffa4585f20>] ? bit_wait+0x50/0x50
[  360.786465]  [<ffffffffa45874cd>] io_schedule_timeout+0xad/0x130
[  360.786466]  [<ffffffffa4587568>] io_schedule+0x18/0x20
[  360.786467]  [<ffffffffa4585f31>] bit_wait_io+0x11/0x50
[  360.786468]  [<ffffffffa4585ae1>] __wait_on_bit_lock+0x61/0xc0
[  360.786469]  [<ffffffffa3fbd5b4>] __lock_page+0x74/0x90
[  360.786470]  [<ffffffffa3ec70a0>] ? wake_bit_function+0x40/0x40
[  360.786471]  [<ffffffffa3fbe324>] __find_lock_page+0x54/0x70
[  360.786472]  [<ffffffffa3fbf085>] grab_cache_page_write_begin+0x55/0xc0
[  360.786476]  [<ffffffffc04e85cd>] ext4_da_write_begin+0xad/0x360 [ext4]
[  360.786478]  [<ffffffffa3fbda8f>] generic_file_buffered_write+0x10f/0x270
[  360.786479]  [<ffffffffa3fc0712>] __generic_file_aio_write+0x1e2/0x400
[  360.786480]  [<ffffffffa3fc0989>] generic_file_aio_write+0x59/0xa0
[  360.786484]  [<ffffffffc04dd5c8>] ext4_file_write+0x348/0x600 [ext4]
[  360.786485]  [<ffffffffa3ec9d8f>] ? __remove_hrtimer+0x3f/0xb0
[  360.786486]  [<ffffffffa404d2e3>] do_sync_write+0x93/0xe0
[  360.786488]  [<ffffffffa404ddd0>] vfs_write+0xc0/0x1f0
[  360.786489]  [<ffffffffa404ebaf>] SyS_write+0x7f/0xf0
[  360.786490]  [<ffffffffa4594f92>] system_call_fastpath+0x25/0x2a
```

(问题 : 如果这个时候，去删掉他的 page table ，会在哪里阻塞，也许会阻塞，也许不会阻塞)

### lruvec::lru_lock

(这个问题我一直非常奇怪的点在于，一个链表就用一个 lru_lock ，
这个不会成为一个很大的 contention ，说话的 lockless linked list 在哪里)

更多细节看这里吧
- [memcg lru lock 血泪史](https://mp.weixin.qq.com/s/7eDqHR06TIBh6hqUMTrZKg)

### mm_struct::mmap_lock

https://lore.kernel.org/all/20230227173632.3292573-9-surenb@google.com/T/#m4c560a0cf3839ad6d72bd450f6f38db1e544c20d

- [LWN：采用 per-VMA 锁进行并发 page-fault 处理！](https://mp.weixin.qq.com/s/R8BVIrps5UPXVncvbGkkug)

- https://lwn.net/ml/linux-kernel/20220901173516.702122-1-surenb@google.com/
- [LWN：修正per-VMA locking的问题！](https://mp.weixin.qq.com/s/t6VCVyUnDdtcgPyZvIZ5EA)

一个前置的准备 patch
https://lore.kernel.org/all/20230126193752.297968-5-surenb@google.com/T/#m2b9c92d69b356fb8ae1501a9eff4ec62f12f83e3

https://zhuanlan.zhihu.com/p/14709946806

### rmap

page_get_anon_vma 获取上含有锁的 tricky 的机制

#### anon
anon_vma->rwsem

anon_vma_lock_write
anon_vma_lock_read

(问题 : 那么，内核中为什么无法实现，)

#### file
mapping->i_mmap_rwsem

### shrinker_rwsem
( 这是一个小问题了，但是也有优化的空间，看看对应的 patch 吧)

### page table

访问 page table 在任何时候都可以发生，在修改 page table 的时候，
1. 修改 page table
2. invliad tlb
(有可能，有 core 在两个步骤之间使用错误的 tlb ，这当然可以，不过这个靠用户态的保护)
(操作系统只能保证当系统调用返回之后，状态正确，不能保证系统调用中间，不会观察到中间状态)

#### [ ] 多个 cpu 可以对于同一个位置来 page fault

首先我猜测:
mmap 一个位置，两个 thread 同时写这个页面，可能同时触发 #pf ，然后只有一个可以成功
另外一个是 mutex wait 、spin lock 还是什么?

但是，测试结果让人大吃一惊，
对于 mm/shmem.c:shmem_fault 测试

使用这个:
```c
	const long MAP_SIZE = get_size(4, 'G');
	int fd = get_file("/home/martins3/qemu.ram", MAP_SIZE);
	void *ptr = mmap_region(MAP_SIZE, fd, MAP_SHARED);
```

```txt
[ 2120.479034] 1 lock held by a.out/5446:
[ 2120.479652]  #0: ffff888149425588 (vm_lock){++++}-{0:0}, at: do_user_addr_fault+0x1c8/0x680
```

该 lock 的位置为: arch/x86/mm/fault.c:1329 调用 lock_vma_under_rcu
(lock_vma_under_rcu 也是有趣的，居然是 rcu 下只有锁，持有锁了，还害怕会回收吗)

所以，我的猜测是，的确会有 page fault 在同一个位置，但是最后只是在 ptdesc::ptl 的出现 contention ，
此外的处理都是一样的。


启动 qemu 测试，memory backend 是 memfd ，结果为:
```txt
[52966.163244] 3 locks held by qemu-system-x86/5090:
[52966.163796]  #0: ffff888127c000b0 (&vcpu->mutex){+.+.}-{4:4}, at: kvm_vcpu_ioctl+0x96/0xa70 [kvm]
[52966.164979]  #1: ffffc900023e6ef0 (&kvm->srcu){.+.+}-{0:0}, at: kvm_arch_vcpu_ioctl_run+0x1379/0x2450 [kvm]
[52966.166577]  #2: ffff88810864ea50 (&mm->mmap_lock){++++}-{4:4}, at: get_user_pages_unlocked+0x88/0x360
```
这个我们去 kvm/lock.c 中分析

#### ptdesc::ptl

```diff
diff --git a/mm/memory.c b/mm/memory.c
index 49199410805c..27a02bf8cc8d 100644
--- a/mm/memory.c
+++ b/mm/memory.c
@@ -1570,6 +1570,10 @@ static inline int zap_present_ptes(struct mmu_gather *tlb,
 		return 1;
 	}

+	if (trace_rss_stat_enabled()) {
+		if(!strcmp(current->comm, "a.out")){
+			debug_show_held_locks(current);
+		}
+
+	}
+
 	/*
 	 * Make sure that the common "small folio" case is as fast as possible
 	 * by keeping the batching logic separate.
```

然后可以找到这个东西(也不容易)

```txt
3 locks held by a.out/3826:
 #0: ffff888109f881f0 (&mm->mmap_lock){++++}-{4:4}, at: madvise_lock+0x30/0xc0
 #1: ffffffff8255abc0 (rcu_read_lock){....}-{1:3}, at: ___pte_offset_map+0x2a/0x160
 #2: ffff888138125648 (ptlock_ptr(ptdesc)#2){+.+.}-{3:3}, at: __pte_offset_map_lock+0x73/0x160
```

rculock 在 ___pte_offset_map 中

spinlock zap_pte_range 中
```txt
	start_pte = pte = pte_offset_map_lock(mm, pmd, addr, &ptl);
```

这个 spinlock_t 来自于
```c
static inline spinlock_t *pte_lockptr(struct mm_struct *mm, pmd_t *pmd)
{
	return ptlock_ptr(page_ptdesc(pmd_page(*pmd)));
}
```
从意识到，通过 struct ptdesc 可以用来管理内核中的

为什么这里有一个 rcu lock ，即便是已经有一个 spin lock ，答案在这里，但是答案看不懂:
https://lore.kernel.org/all/7cd843a9-aa80-14f-5eb2-33427363c20@google.com/T/#re80cdabb3389028199125d4c1b2d1db4499652f1

(似乎 page table 的每一个层级都是有 pmd_lock )


猜测:
- 当两个 thread 同时触发 page fault ，pud 存在，pmd 不存在，构建一个 page 用于当做之后的 pmd 。
当要修改 pmd 的时候，会有一个 spin lock 来保护 pud 的修改。

#### mm::page_table_lock

居然有一个巨大的 lock 来保护

为什么 p4d 需要 page_table_lock 的保护:
```c
int __p4d_alloc(struct mm_struct *mm, pgd_t *pgd, unsigned long address)
{
	p4d_t *new = p4d_alloc_one(mm, address);
	if (!new)
		return -ENOMEM;

	spin_lock(&mm->page_table_lock);
	if (pgd_present(*pgd)) {	/* Another has populated it */
		p4d_free(mm, new);
	} else {
		smp_wmb(); /* See comment in pmd_install() */
		pgd_populate(mm, pgd, new);
	}
	spin_unlock(&mm->page_table_lock);
	return 0;
}
```

而 pte 是不需要的:
```c
int __pte_alloc(struct mm_struct *mm, pmd_t *pmd)
{
	pgtable_t new = pte_alloc_one(mm);
	if (!new)
		return -ENOMEM;

	pmd_install(mm, pmd, &new);
	if (new)
		pte_free(mm, new);
	return 0;
}
```

原来在 pmd_install 中，就有了 pmd 的保护

### swap

- swap_lock

### slub

mm/slub.c 上的注释写了一个很长锁注释

### 其他

- mmlist_lock : 保护 所有的 mm 放到一起链表

## 问题
如何理解这里的 PageLocked ?

```c
bool PageMovable(struct page *page)
{
	const struct movable_operations *mops;

	VM_BUG_ON_PAGE(!PageLocked(page), page);
	if (!__PageMovable(page))
		return false;

	mops = page_movable_ops(page);
	if (mops)
		return true;

	return false;
}
```

## 打开这个选项看看 CONFIG_DEBUG_PAGE_REF

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
