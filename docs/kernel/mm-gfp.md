# 总结 gfp flags

- [GFP flags and the end of`__GFP_ATOMIC`](https://lwn.net/Articles/920891/)

## overview
基本上 gfp_flags.h 的注释还不错的，按照其分类处理
1. DOC: Page mobility and placement hints
  - `__GFP_MOVABLE` : 可以移动
  - `__GFP_RECLAIMABLE` : 可以写回去的
2. DOC: Reclaim modifiers
3. DOC: Useful GFP flag combinations

### FS
```c
#define __GFP_FS	((__force gfp_t)___GFP_FS)
#define GFP_NOFS	(__GFP_RECLAIM | __GFP_IO)

#define GFP_KERNEL	(__GFP_RECLAIM | __GFP_IO | __GFP_FS)
```

和 GFP_KERNEL 的区别只是 `__GFP_FS`，GFP_KERNEL 的使用位置大约有 2000 个。

```c
/*
 * %__GFP_FS can call down to the low-level FS. Clearing the flag avoids the
 * allocator recursing into the filesystem which might already be holding
 * locks.
 */

/*
 * %GFP_NOFS will use direct reclaim but will not use any filesystem interfaces.
 * Please try to avoid using this flag directly and instead use
 * memalloc_nofs_{save,restore} to mark the whole scope which cannot/shouldn't
 * recurse into the FS layer with a short explanation why. All allocation
 * requests will inherit GFP_NOFS implicitly.
 */
```

GFP_NOFS 使用的位置大多数都是具体的 fs 中，但是在两个 generic 的位置:
1. folio_alloc_buffers
2. `__mpage_writepage` : 分配 bio 结构体

带上这个 flags 表示我们在文件系统中申请内存

```txt
#0  folio_alloc_buffers (folio=folio@entry=0xffffea0004279700, size=4096, retry=retry@entry=true) at fs/buffer.c:855
#1  0xffffffff8147dafd in folio_create_empty_buffers (folio=0xffffea0004279700, blocksize=<optimized out>, b_state=b_state@entry=0) at fs/buffer.c:1606
#2  0xffffffff8147f0b8 in folio_create_buffers (b_state=0, inode=0xffff88810787b380, folio=0xffffea0004279700) at fs/buffer.c:1732
#3  block_read_full_folio (folio=0xffffea0004279700, get_block=0xffffffff81740bd0 <blkdev_get_block>) at fs/buffer.c:2329
#4  0xffffffff81338536 in filemap_read_folio (file=0xffffea0004279700, file@entry=0x0 <fixed_percpu_data>, filler=filler@entry=0xffffffff81740c50 <blkdev_read_folio>, folio=0x1
<fixed_percpu_data+1>, folio@entry=0xffffea0004279700) at mm/filemap.c:2421
#5  0xffffffff8133b56a in do_read_cache_folio (mapping=0xffff88810787b4f8, index=index@entry=0, filler=0xffffffff81740c50 <blkdev_read_folio>, filler@entry=0x0 <fixed_percpu_dat
a>, file=file@entry=0x0 <fixed_percpu_data>, gfp=1051840) at mm/filemap.c:3680
#6  0xffffffff8133b642 in read_cache_folio (mapping=<optimized out>, index=index@entry=0, filler=filler@entry=0x0 <fixed_percpu_data>, file=file@entry=0x0 <fixed_percpu_data>) a
t ./include/linux/pagemap.h:274
#7  0xffffffff81764f46 in read_mapping_folio (file=0x0 <fixed_percpu_data>, index=0, mapping=<optimized out>) at ./include/linux/pagemap.h:778


#0  folio_alloc_buffers (folio=folio@entry=0xffffea000525d040, size=4096, retry=retry@entry=true) at fs/buffer.c:855
#1  0xffffffff8147dafd in folio_create_empty_buffers (folio=0xffffea000525d040, blocksize=<optimized out>, b_state=b_state@entry=0) at fs/buffer.c:1606
#2  0xffffffff814810c9 in folio_create_buffers (b_state=0, inode=<optimized out>, folio=0xffffea000525d040) at fs/buffer.c:1732
#3  __block_write_begin_int (folio=0xffffea000525d040, pos=<optimized out>, len=<optimized out>, get_block=0xffffffff8150ec20 <ext4_da_get_block_prep>, iomap=0x0 <fixed_percpu_d
ata>) at fs/buffer.c:2043
#4  0xffffffff81513598 in ext4_da_write_begin (file=<optimized out>, mapping=0xffff88813e24f4b0, pos=7524352, len=4096, pagep=0xffffc900017dfc28, fsdata=<optimized out>) at fs/e
xt4/inode.c:2918
#5  0xffffffff813369b6 in generic_perform_write (iocb=iocb@entry=0xffff88810b9d1500, i=i@entry=0xffffc900017dfcb8) at mm/filemap.c:3923
#6  0xffffffff814fe313 in ext4_buffered_write_iter (iocb=0xffff88810b9d1500, from=0xffffc900017dfcb8) at fs/ext4/file.c:289
#7  0xffffffff8179f88c in call_write_iter (iter=0xffffc900017dfcb8, kio=0xffff88810b9d1500, file=0xffff888108c1fe00) at ./include/linux/fs.h:1868
#8  io_write (req=0xffff88810b9d1500, issue_flags=10) at io_uring/rw.c:920
#9  0xffffffff8178d690 in io_issue_sqe (req=req@entry=0xffff88810b9d1500, issue_flags=issue_flags@entry=10) at io_uring/io_uring.c:1956
#10 0xffffffff8178dba7 in io_wq_submit_work (work=0xffff88810b9d15d0) at io_uring/io_uring.c:2032
#11 0xffffffff817a186c in io_worker_handle_work (worker=worker@entry=0xffff88810ba96c00) at io_uring/io-wq.c:569
#12 0xffffffff817a1f71 in io_wq_worker (data=0xffff88810ba96c00) at io_uring/io-wq.c:613
```

```c
/**
 * memalloc_nofs_save - Marks implicit GFP_NOFS allocation scope.
 *
 * This functions marks the beginning of the GFP_NOFS allocation scope.
 * All further allocations will implicitly drop __GFP_FS flag and so
 * they are safe for the FS critical section from the allocation recursion
 * point of view. Use memalloc_nofs_restore to end the scope with flags
 * returned by this function.
 *
 * This function is safe to be used from any context.
 */
static inline unsigned int memalloc_nofs_save(void)
{
	unsigned int flags = current->flags & PF_MEMALLOC_NOFS;
	current->flags |= PF_MEMALLOC_NOFS;
	return flags;
}
```

的确是，最后是体现在这里了:

```c
/*
 * Applies per-task gfp context to the given allocation flags.
 * PF_MEMALLOC_NOIO implies GFP_NOIO
 * PF_MEMALLOC_NOFS implies GFP_NOFS
 * PF_MEMALLOC_PIN  implies !GFP_MOVABLE
 */
static inline gfp_t current_gfp_context(gfp_t flags)
{
	unsigned int pflags = READ_ONCE(current->flags);

	if (unlikely(pflags & (PF_MEMALLOC_NOIO | PF_MEMALLOC_NOFS | PF_MEMALLOC_PIN))) {
		/*
		 * NOIO implies both NOIO and NOFS and it is a weaker context
		 * so always make sure it makes precedence
		 */
		if (pflags & PF_MEMALLOC_NOIO)
			flags &= ~(__GFP_IO | __GFP_FS);
		else if (pflags & PF_MEMALLOC_NOFS)
			flags &= ~__GFP_FS;

		if (pflags & PF_MEMALLOC_PIN)
			flags &= ~__GFP_MOVABLE;
	}
	return flags;
}
```

#### 如何体现出来不能 call down to fs interface ?

在 vmscan.c 中

```c
/*
 * This is the main entry point to direct page reclaim.
 *
 * If a full scan of the inactive list fails to free enough memory then we
 * are "out of memory" and something needs to be killed.
 *
 * If the caller is !__GFP_FS then the probability of a failure is reasonably
 * high - the zone may be full of dirty or under-writeback pages, which this
 * caller can't do much about.  We kick the writeback threads and take explicit
 * naps in the hope that some of these pages can be written.  But if the
 * allocating task holds filesystem locks which prevent writeout this might not
 * work, and the allocation attempt will fail.
 *
 * returns:	0, if no pages reclaimed
 * 		else, the number of pages reclaimed
 */
static unsigned long do_try_to_free_pages(struct zonelist *zonelist,
					  struct scan_control *sc)
{
```

应该是体现在此处的 : `shrink_folio_list`

```c
			if (!may_enter_fs(folio, sc->gfp_mask))
				goto keep_locked;
```

#### dead lock 体现在什么位置?

如果文件系统的活动中需要内存，例如此时正在构造 inode 之类的，但是此时绝大多数的内存都被 swap 占据了，
导致需要释放内存，为了释放内存，需要将一些文件刷下去，这导致使用一些内存来实现分配访问文件系统，这回导致死锁的。

- `__perform_reclaim`
  - fs_reclaim_acquire
  - try_to_free_pages
    - do_try_to_free_pages
  - fs_reclaim_release


## 如何理解 ATOMIC


```c
/*
 * %GFP_ATOMIC users can not sleep and need the allocation to succeed. A lower
 * watermark is applied to allow access to "atomic reserves".
 * The current implementation doesn't support NMI and few other strict
 * non-preemptive contexts (e.g. raw_spin_lock). The same applies to %GFP_NOWAIT.
 */
```

```c
#define GFP_ATOMIC	(__GFP_HIGH|__GFP_KSWAPD_RECLAIM)

#define GFP_NOWAIT	(__GFP_KSWAPD_RECLAIM)
```

为什么单走一个 `__GFP_KSWAPD_RECLAIM` 就是 NOWAIT 的呀
