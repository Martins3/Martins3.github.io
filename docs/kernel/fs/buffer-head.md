# buffer.c

## 什么时候时候需要使用 buffer head

在一个使用 ext4 的系统中，buffer head :
```txt
3010086 1755653  58%    0.22K  83614       36    668912K dentry
2683902 1631823  60%    0.10K  68818       39    275272K buffer_head
1188864 1178796  99%    0.03K   9288      128     37152K kmalloc-32
1099320 1083166  98%    0.13K  18322       60    146576K kernfs_node_cache
826112 294509  35%    0.06K  12908       64     51632K kmalloc-rcl-64
619650 583479  94%    0.70K  13770       45    440640K proc_inode_cache
556500 511105  91%    0.63K  11130       50    356160K inode_cache
539021 249230  46%    0.57K   9642       56    308544K radix_tree_node
381824 211601  55%    0.50K   5966       64    190912K kmalloc-512
354688  81444  22%    1.13K  12671       28    405472K ext4_inode_cache
305536 260047  85%    0.06K   4774       64     19096K lsm_inode_cache
301504 288972  95%    0.06K   4711       64     18844K kmalloc-64
```
当然，如果当前的文件系统使用的是 xfs ，就只能观察到很少的东西了。

首先，第一个问题，就是我们会在哪里来分配:
1. 文件读写时（最常见）
```c
int block_read_full_folio(struct folio *folio, get_block_t *get_block)
{
    // 为整个 folio 创建 buffer head 链
    head = folio_create_buffers(folio, inode, 0);  // ← 这里分配
    // ...
}
struct buffer_head *folio_buffers(struct folio *folio)
{
    bh = folio_buffers(folio);
    if (!bh)
        bh = create_empty_buffers(folio, blocksize, 0);  // ← 这里分配
    return bh;
}
```

2. 直接操作块设备时：getblk/bdev_getblk

```c
struct buffer_head *bdev_getblk(struct block_device *bdev, sector_t block,
        unsigned size, gfp_t gfp)
{
    // 1. 先在缓存中查找
    bh = __find_get_block(bdev, block, size);
    if (bh)
        return bh;

    // 2. 没找到，分配新的
    return __getblk_slow(bdev, block, size, gfp);  // ← 这里分配
}
```

## 最基本的总结
buffer head 需要解决两个问题:
1. page size 比一个 block 的 size 要大，所以一个 struct buffer_head 来记录一个 page 中的内容和 block 的映射关系
2. metadata 不会加载到 page cache 中，有的文件系统利用 bh 机制来加载 metadata

从 plka 中记录下来的东西，应为 buffer_head 很多年没有变化，其中的内容基本正确:

### dependent cache

参考 struct buffer_head 的注释，如下三个功能:
1. extracting block mappings (via a get_block_t call),
  - 构建 内存和 disk 的映射关系，记录到 buffer_head:b_page 和 buffer_head:b_blocknr
  - 一个 page 有多个 buffer_head ，buffer_head 之间通过 b_assoc_buffers 联系起来
2. for tracking state within a page (via a page_mapping)
3. and for wrapping bio submission for backward compatibility reasons


```c
/*
 * Historically, a buffer_head was used to map a single block
 * within a page, and of course as the unit of I/O through the
 * filesystem and block layers.  Nowadays the basic I/O unit
 * is the bio, and buffer_heads are used for extracting block
 * mappings (via a get_block_t call), for tracking state within
 * a page (via a page_mapping) and for wrapping bio submission
 * for backward compatibility reasons (e.g. submit_bh).
 */
struct buffer_head
```

Buffers are kept for small I/O transfers with block size granularity.
This is often required by filesystems to handle their metadata.
Transfer of raw data is done in a page-centric fashion, and **the implementation
of buffers is also on top of the page cache**.

The buffer cache consists of two structural units:
1. A `buffer head` holds all management data relating to the state of the buffer
including information on block number, block size, access counter, and so on, discussed below. These data are
not stored directly after the buffer head but in a separate area of RAM memory indicated by
a corresponding pointer in the buffer head structure.
2. The useful data are held in specially reserved pages that **may also reside in the page cache**.

**The buffer cache operates independently of the page cache, not in addition to it**.

In this case, `private`(struct page) points to the first buffer head used to split the page into smaller units.
The various buffer heads are linked in a cyclic list by means of `b_this_page`.

The kernel provides the `create_empty_buffers` and `link_dev_buffers` functions for this purpose, both of which
are implemented in fs/buffer.c. The latter serves to associate an existing set of buffer heads with a
page, whereas `create_empty_buffers` generates a completely new set of buffers for association with the
page. For example, create_empty_buffers is invoked when reading and writing complete pages with
`block_read_full_page` and `__block_write_full_page`.

As already noted, some transfer operations to and from block devices may need to be
performed in units whose size depends on the block size of the underlying devices, whereas many
parts of the kernel prefer to carry out I/O operations with page granularity as this makes things much
easier — especially in terms of memory management.

In this scenario, buffers act as intermediaries between the two worlds

引用 [The future of the page cache](https://lwn.net/Articles/712467/) 中内容
Initially, the page and buffer caches were entirely separate, but Ingo Molnar unified them in 1999.
Now, the buffer cache still exists, but its entries point into the page cache.

The buffer cache is used not only as an add-on to the page cache but also as an **independent** cache for
objects that are not handled in pages but in blocks.

### independent cache
处理 page cache 之外的东西

Buffers are used not only in the context of pages. However, there are still
situations in which access to block device data is performed on the block level and not on the page level
in the view of higher-level code. **To help speed up such operations**, the kernel provides yet another cache
known as an ***LRU buffer*** cache discussed below.

This cache for independent buffers is not totally divorced from the page cache. Since RAM memory
is always managed in pages, buffered blocks must also be held in pages, with the result that there are
some points of contact with the page cache. These cannot and should not be ignored — after all,
**access to individual blocks is still possible via the buffer cache without having to worry about the
organization of the blocks into pages.**


When is it necessary to read individual blocks? There are not too many points in the kernel where this
must be done, but these are nevertheless of great importance. Filesystems in particular make use of the
routines described above when reading superblocks or management blocks.

The kernel defines two functions to simplify the work of filesystems with individual blocks:
```c
static inline struct buffer_head *
sb_bread(struct super_block *sb, sector_t block)
{
	return __bread_gfp(sb->s_bdev, block, sb->s_blocksize, __GFP_MOVABLE);
}
```

## 细节代码分析

### dependent cache 的 io 过程

主要是 block_read_full_folio 和 block_write_full_folio
他们的一个关键参数是 get_block_t

`block_read_full_page` reads a full page in three steps:
1. The buffers are set up and their state is checked.
2. The buffers are locked to rule out interference by other kernel threads in the next step.
3. The data are transferred to the buffers.
> 创建，上锁，读取。
> 创建包括的内容 : buffer 和 get_block

- ext4_block_write_begin : 从 page cache 层提供了 folio 的，然后在
  - create_empty_buffers
    - folio_alloc_buffers

- try_to_free_buffers : 释放 page 及其关联的 buffer head 等
  - free_buffer_head

释放 buffer_head 的位置:
```txt
@[
    try_to_free_buffers+5
    shrink_folio_list+1776
    evict_folios+600
    try_to_shrink_lruvec+420
    shrink_one+253
    shrink_node+2749
    balance_pgdat+1226
    kswapd+492
    kthread+220
    ret_from_fork+49
    ret_from_fork_asm+26
]: 3378
```
在 pageout 中，这里是很经典的，folio 中的 private 关联了 buffer head
```c
	if (!mapping) {
		/*
		 * Some data journaling orphaned folios can have
		 * folio->mapping == NULL while being dirty with clean buffers.
		 */
		if (folio_test_private(folio)) {
			if (try_to_free_buffers(folio)) {
				folio_clear_dirty(folio);
				pr_info("%s: orphaned folio\n", __func__);
				return PAGE_CLEAN;
			}
		}
		return PAGE_KEEP;
	}
```


### independent cache 的 io 过程

#### sb_bread
- sb_bread
  - __bread_gfp
    - bdev_getblk : Get a buffer_head in a block device's buffer cache ，folio 是这个时候释放的
      - __find_get_block
        - lookup_bh_lru : 先在一个小的 lru cache 中查询
        - __find_get_block_slow : 然后**在 radix 中查询，和 page cache 的效果很像**
      - __getblk_slow
    - __bread_slow : 使用 submit_bio 来提交 io

```txt
@[
    __bread_gfp+5
    ext2_get_inode+231
    __ext2_write_inode+118
    __writeback_single_inode+668
    writeback_single_inode+175
    sync_inode_metadata+71
    ext2_add_link+1096
    ext2_create+113
    path_openat+2214
    do_filp_open+196
    do_sys_openat2+171
    __x64_sys_openat+87
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

#### brelse

brelse 不是真的释放，而是一个
```c
/**
 * brelse - Release a buffer.
 * @bh: The buffer to release.
 *
 * Decrement a buffer_head's reference count.  If @bh is NULL, this
 * function is a no-op.
 *
 * If all buffers on a folio have zero reference count, are clean
 * and unlocked, and if the folio is unlocked and not under writeback
 * then try_to_free_buffers() may strip the buffers from the folio in
 * preparation for freeing it (sometimes, rarely, buffers are removed
 * from a folio but it ends up not being freed, and buffers may later
 * be reattached).
 *
 * Context: Any context.
 */
static inline void brelse(struct buffer_head *bh)
```

try_to_free_buffers


## 其他细节
### BH_Mapped

`BH_Mapped` means that there is a mapping of the buffer contents on a secondary storage device,
as is the case with all buffers that originate from filesystems or from direct accesses to block devices

告诉 buffer_head 他里面的内容指向到那里:
```c
static inline void
map_bh(struct buffer_head *bh, struct super_block *sb, sector_t block)
{
	set_buffer_mapped(bh);
	bh->b_bdev = sb->s_bdev;
	bh->b_blocknr = block;
	bh->b_size = sb->s_blocksize;
}
```

理解这个问题的典型位置，这里的 buffer head 都是从上面传递过来的，
通过 ext2_get_block 获取的 bno ，然后让 buffer head 指向到这个获取的 bno
```c
int ext2_get_block(struct inode *inode, sector_t iblock,
		struct buffer_head *bh_result, int create)
{
	unsigned max_blocks = bh_result->b_size >> inode->i_blkbits;
	bool new = false, boundary = false;
	u32 bno;
	int ret;

	ret = ext2_get_blocks(inode, iblock, max_blocks, &bno, &new, &boundary,
			create);
	if (ret <= 0)
		return ret;

	map_bh(bh_result, inode->i_sb, bno);
	bh_result->b_size = (ret << inode->i_blkbits);
	if (new)
		set_buffer_new(bh_result);
	if (boundary)
		set_buffer_boundary(bh_result);
	return 0;

}
```

### mark_buffer_dirty

BH_Dirty 和 page cache 中间 page 的 dirty 的功能不是重复，
而是具有相关性:
```txt
@[
    mark_buffer_dirty+151
    ext2_new_blocks+1636
    ext2_get_blocks+746
    ext2_get_block+94
    __block_write_begin_int+350
    block_write_begin+81
    ext2_write_begin+48
    generic_perform_write+220
    generic_file_write_iter+98
    vfs_write+673
    ksys_write+110
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```
mark_buffer_dirty 的时候会标记到其关联的 folio 上



blockdev 的 writeback 还是 buffer head 的:
```txt
@[
        __block_write_full_folio+5
        blkdev_writepages+110
        do_writepages+199
        __writeback_single_inode+65
        writeback_sb_inodes+534
        __writeback_inodes_wb+76
        wb_writeback+427
        wb_workfn+795
        process_one_work+394
        worker_thread+598
        kthread+249
        ret_from_fork+242
        ret_from_fork_asm+26
]: 5
```


## 为什么 nfs 和 fuse 可以不依赖 buffer head
<!-- 58407fad-e9df-4d02-9131-41805ba9d860 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

**NFS 和 FUSE 不依赖 `buffer_head` 机制，根本原因在于它们不是面向本地块设备（block device）的文件系统，而是面向“远程”或“用户态抽象后端”的文件系统。**

`buffer_head` 最初是 Linux **块设备缓存（buffer cache）** 的核心数据结构，用于：

- 表达 **“文件偏移 ↔ 块设备扇区”** 的映射（`b_blocknr`）
- 管理 **每个 fs-block 的状态**（`BH_Uptodate`, `BH_Dirty`, `BH_Lock` 等）
- 协调 **页缓存（page cache）与块设备 I/O** 之间的细粒度状态（尤其当 `fs block size < page size` 时）

这个说的看上去显然有点扯了，但是，既然 buffer_head 是管理文件偏移到 block 之间的关系，
为什么 fuse 还有 iomap 的支持，或者说，buffer_head 不是和 iomap 是相同的生态位的吗?

那么 nfs 也有 iomap 的支持吗?

## buffer_head 中到底存储了什么东西
<!-- a13b7078-d402-40a6-83ad-a97a3f856d68 -->

简而言之，描述内核映射到磁盘的哪一个块上:
```c
struct buffer_head {
    unsigned long b_state;        // 状态标志（BH_Uptodate, BH_Dirty, BH_Lock 等）
    struct block_device *b_bdev;  // 所属块设备
    sector_t b_blocknr;           // 在设备上的块号（sector_t，按 b_size 对齐）
    size_t b_size;                // 块大小（如 1KB, 4KB）
    char *b_data;                 // 指向页内偏移的数据指针
    struct folio *b_folio;        // 所属 folio
    struct buffer_head *b_this_page; // 同一页内 buffer_head 链表
    struct list_head b_assoc_buffers; // 关联到 inode 的私有链表（用于 fsync）
    struct address_space *b_assoc_map; // 所属 inode 的 mapping
    bh_end_io_t *b_end_io;        // I/O 完成回调
    void *b_private;              // 私有数据
    atomic_t b_count;             // 引用计数
    // ...
};
```

## 为什么 buffer head 需要一个 lru
<!-- f5cd216a-1eb1-4f0e-b79a-30e010e8cfae -->

### 系统中到底有多少个 buffer head

```txt
3015450 1760850  58%    0.22K  83763       36    670104K dentry
2684136 1634245  60%    0.10K  68824       39    275296K buffer_head
1188864 1178901  99%    0.03K   9288      128     37152K kmalloc-32
1099260 1084157  98%    0.13K  18321       60    146568K kernfs_node_cache
828160 296498  35%    0.06K  12940       64     51760K kmalloc-rcl-64
618480 582007  94%    0.70K  13744       45    439808K proc_inode_cache
559050 513208  91%    0.63K  11181       50    357792K inode_cache
539413 250020  46%    0.57K   9649       56    30876 radix_tree_node
385280 212026  55%    0.50K   6020       64    192640K kmalloc-512
354940  81852  23%    1.13K  12680       28    405760K ext4_inode_cache
305536 260221  85%    0.06K   4774       64     19096K lsm_inode_cache
301312 294252  97%    0.02K   1177      256      4708K lsm_file_cache
300672 287872  95%    0.06K   4698       64     18792K kmalloc-64
290368 258285  88%    0.25K   4537       64     72592K filp
288256 275888  95%    0.01K    563      512      2252K kmalloc-8
247488 222758  90%    0.06K   3867       64     15468K kmem_cache_node
209728 189431  90%    0.25K   3277       64     52432K skbuff_head_cache
209100 205815  98%    0.04K   2050      102      8200K ext4_extent_status
200270 182174  90%    0.23K   2861       70     45776K vm_area_struct
188412 162335  86%    0.09K   4486       42     17944K kmalloc-96
181566 154183  84%    0.09K   4323       42     17292K kmalloc-rcl-96
171904 154567  89%    0.06K   2686       64     10744K anon_vma_chain
156672 146521  93%    0.02K    612      256      2448K kmalloc-16
132864 132624  99%    0.12K   2076       64     16608K scsi_sense_cache
127488 125440  98%    0.12K   1992       64     15936K nfs_page
120768 118592  98%    0.12K   1887       64     15096K seq_file
120512 118108  98%    0.12K   1883       64     15064K eventpoll_epi
112768  97455  86%    1.00K   3524       32    112768K kmalloc-1k
 95706  86140  90%    0.10K   2454       39      9816K anon_vma
 91520  90705  99%    0.06K   1430       64      5720K jbd2_inode
 66822  57132  85%    0.74K   1554       43     49728K shmem_inode_cache
 59552  47855  80%    2.00K   3722       16    119104K kmalloc-2k
 52736  49235  93%    0.12K    824       64      6592K pid
 52224  52224 100%    0.04K    512      102      2048K pde_opener
```

在 13900k 上，就完全观察不到了，因为没有挂载 ext4
```txt
4096596 4095866  99%    0.19K  97538       42    780304K dentry
3887934 3887330  99%    0.08K  76234       51    304936K lsm_inode_cache
3826496 3826488  99%    1.00K 119578       32   3826496K xfs_inode
3189760 3189074  99%    0.02K  12460      256     49840K kmalloc-rnd-09-16
1515640 1407521  92%    0.57K  54130       28    866080K radix_tree_node
1465800 1440803  98%    0.20K  36645       40    293160K xfs_ili
595840 595840 100%    0.06K   9310       64     37240K kmalloc-rnd-04-64
567035 566572  99%    0.05K   6671       85     26684K xfs_ifork
522400 469399  89%    1.00K  16325       32    522400K kmalloc-rnd-01-1k
496768 496768 100%    0.03K   3881      128     15524K kmalloc-rnd-08-32
245700 245700 100%    0.38K   5850       42     93600K xfs_buf
174528 174518  99%    0.25K   5454       32     43632K kmalloc-rnd-03-256
154112 154112 100%    0.03K   1204      128      4816K kmalloc-rnd-04-32
105856 105699  99%    0.50K   3308       32     52928K kmalloc-rnd-03-512
 82944  20755  25%    0.50K   2592       32     41472K kmalloc-rnd-01-512
 81396  81396 100%    0.09K   1938       42      7752K kmalloc-rcl-96
 77580  77365  99%    0.13K   2586       30     10344K kernfs_node_cache
 71424  34615  48%    0.06K   1116       64      4464K kmalloc-rnd-08-64
 66048  66045  99%    0.01K    129      512       516K zs_handle-zram0
 60928  60928 100%    0.03K    476      128      1904K kmalloc-rnd-09-32
 55062  53978  98%    0.09K   1311       42      5244K kmalloc-rnd-04-96
 38148  26808  70%    0.62K    748       51     23936K inode_cache
 33264  33264 100%    0.19K    792       42      6336K kmalloc-rnd-04-192
 32046  31618  98%    0.09K    763       42      3052K kmalloc-rnd-08-96
 31936  31936 100%    0.25K    998       32      7984K kmalloc-rnd-09-256
 30240  26881  88%    0.07K    540       56      2160K vmap_area
```

### buffer head 的作用到底是什么?

因为一般的流程都是这样的:
```txt
┌─────────────────────────────────────────────┐
│  1. 先在当前 CPU 的 bh_lru 中查找           │
│     (O(16) = O(1)，无锁或轻量级锁)          │
│         ↓                                   │
│  2. 命中 → 直接返回，移到队首 (MRU)         │
│         ↓ 未命中                            │
│  3. 在 page cache 中查找 (慢路径)           │
│     需要锁，遍历 radix tree                 │
│         ↓                                   │
│  4. 找到后加入 bh_lru，淘汰最久未用的       │
└─────────────────────────────────────────────┘
```

这个过程也就是:
```c
/*
 * Perform a pagecache lookup for the matching buffer.  If it's there, refresh
 * it in the LRU and mark it as accessed.  If it is not present then return
 * NULL. Atomic context callers may also return NULL if the buffer is being
 * migrated; similarly the page is not marked accessed either.
 */
static struct buffer_head *
find_get_block_common(struct block_device *bdev, sector_t block,
			unsigned size, bool atomic)
{
	struct buffer_head *bh = lookup_bh_lru(bdev, block, size);

	if (bh == NULL) {
		/* __find_get_block_slow will mark the page accessed */
		bh = __find_get_block_slow(bdev, block, atomic);
		if (bh)
			bh_lru_install(bh);
	} else
		touch_buffer(bh);

	return bh;
}
```
find_get_block_common 就在那个经典的 get_block_t 的路径上。

慢速路径具体是，在 radix tree 中查询:
```txt
	folio = __filemap_get_folio(bd_mapping, index, FGP_ACCESSED, 0);
```


```c
  #define BH_LRU_SIZE 16
  struct bh_lru {
      struct buffer_head *bhs[BH_LRU_SIZE];
  };
  static DEFINE_PER_CPU(struct bh_lru, bh_lrus);
```

- **每 CPU 一份**，无需全局锁。
- **容量小（仅 16 个）**，适合缓存最近频繁访问的 bh（如 superblock、inode table 块）。
- **命中时直接返回**，避免 folio 查找。

```txt
sb_bread()
  → __bread_gfp()
    → bdev_getblk()
      → __find_get_block_nonatomic()  // 先查 LRU
        → find_get_block_common()
          → lookup_bh_lru()           // LRU 命中？→ 返回
          → __find_get_block_slow()   // 未命中 → 慢路径
            → __filemap_get_folio()   // 获取 folio
            → 遍历/创建 buffer_head
      → 若 bh 未 uptodate，调用 __bread_slow() 提交 bio 读数据
```

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
