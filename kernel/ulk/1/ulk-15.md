# Understand Linux Kernel : The Page Cache

## KeyNote
1. 关键的问题 : radix tree 居然是可以 device 的，
  1. 是不是 file 的，总是不连续的
  2. 从 buffer page cache 和 非 buffered 之间的切换
2. try_to_release_page 的作用是什么 ?
3. 所以 vfs 的各种蛇皮 cache 都是怎么回事 ?
4. buffer_mapped BH_Mapped 这个 flags 的作用是什么 ?
    1. 岂不是说 : 可以存在 page 中间的部分 buffer 是和 disk 存在关联，部分是没有的
    2. 岂不是说 : mapped 的含义其实是 valid 的意思 ? 

5. @todo radix tree 的 tag 上的内容

6. 如今的 flusher 机制中间的各种内容 创建和销毁的过程是什么
7. 对于对于 mmap 的 file 文件，利用 shared 的模式，那么，靠什么通知 inode 哪一个文件被写过 ?

8. 考虑到 wowotech 的流程图，可以得出一下的结论
    1. 几乎所有都是从 wb_workfn 进入的
    2. 

9. 2019 的 修改 pdflush 的 patch : 
```diff
tree e5a6513b411de16a46199530ec98ef9b7f1efc50
parent 66f3b8e2e103a0b93b945764d98e9ba46cb926dd
author Jens Axboe <jens.axboe@oracle.com> Wed Sep 9 09:08:54 2009 +0200
committer Jens Axboe <jens.axboe@oracle.com> Fri Sep 11 09:20:25 2009 +0200

writeback: switch to per-bdi threads for flushing data

This gets rid of pdflush for bdi writeout and kupdated style cleaning.
pdflush writeout suffers from lack of locality and also requires more
threads to handle the same workload, since it has to work in a
non-blocking fashion against each queue. This also introduces lumpy
behaviour and potential request starvation, since pdflush can be starved
for queue access if others are accessing it. A sample ffsb workload that
does random writes to files is about 8% faster here on a simple SATA drive
during the benchmark phase. 
-	wakeup_pdflush(1024);
+	wakeup_flusher_threads(1024);

-static void generic_sync_sb_inodes(struct super_block *sb,
-				   struct writeback_control *wbc)
+static long wb_writeback(struct bdi_writeback *wb, long nr_pages,
+			 struct super_block *sb,
+			 enum writeback_sync_modes sync_mode, int for_kupdate)


-void
-writeback_inodes(struct writeback_control *wbc)
+void __mark_inode_dirty(struct inode *inode, int flags)
```

```c
static inline void mark_inode_dirty(struct inode *inode)
{
	__mark_inode_dirty(inode, I_DIRTY);
}

static inline void mark_inode_dirty_sync(struct inode *inode)
{
	__mark_inode_dirty(inode, I_DIRTY_SYNC);
}
```
> 不仅仅是插入一个 flags 

## 1 The Page Cache
> skip

#### 1.4 The Tags of the Radix Tree
Instead, to allow a quick search of dirty pages, each intermediate node in the radix
tree contains a dirty tag for each child node (or leaf); this flag is set if and only if at
least one of the dirty tags of the child node is set.

The dirty tags of the nodes at the
bottom level are usually copies of the `PG_dirty` flags of the page descriptors. In this
way, when the kernel traverses a radix tree looking for dirty pages, it can skip each
subtree rooted at an intermediate node whose dirty tag is clear: it knows for sure that 
all page descriptors stored in the subtree are not dirty.

The same idea applies to the `PG_writeback` flag, which denotes that a page is currently being written back to disk.
Thus, each node of the radix tree propagates two flags of the page descriptor:
`PG_dirty` and `PG_writeback` (see the section “Page Descriptors” in Chapter 8).

> 描述似乎非常有道理，但是似乎被 xarray 取代掉其功能了
> 下面讲解的几个函数仅仅被 idr.h 使用

## 2 Storing Blocks in the Page Cache

## 3 Writing Dirty Pages to Disk

Buffer pages introduce a further complication. The buffer heads associated with each
buffer page allow the kernel to keep track of the status of each individual block
buffer. The `PG_dirty` flag of the buffer page should be set if at least one of the associated buffer heads has the `BH_Dirty` flag set.
When the kernel selects a dirty buffer
page for flushing, it scans the associated buffer heads and effectively writes to disk
only the contents of the dirty blocks. As soon as the kernel flushes all dirty blocks in
a buffer page to disk, it clears the `PG_dirty` flag of the page.


#### 3.1 The pdflush Kernel Threads
> 将 pdflusher 的替换 https://lwn.net/Articles/326552/

#### 3.2 Looking for Dirty Pages To Be Flushed

The `wakeup_bdflush()` function is executed when 
either memory is scarce or a user makes an explicit request for a flush operation.
In particular, the function is invoked when:
- The User Mode process issues a sync() system call (see the section “The sync(), fsync(), and fdatasync() System Calls” later in this chapter).
- The grow_buffers() function fails to allocate a new buffer page (see the earlier section “Allocating Block Device Buffer Pages”).
- The page frame reclaiming algorithm invokes free_more_memory() or `try_to_free_pages()` (see Chapter 17).
- The mempool_alloc() function fails to allocate a new memory pool element (see the section “Memory Pools” in Chapter 8).

#### 3.3 Retrieving Old Dirty Pages

## 4 The sync(), fsync(), and fdatasync() System Calls
