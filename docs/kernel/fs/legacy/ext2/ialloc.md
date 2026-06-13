## ialloc.c
1. 多个group
2. 每一个group : a bitmap for blocks(说明block 是否被使用), a bitmap for inode(说明inode 号码是否被使用), inode table(存储inode信息) and data

```c
/*
 * Read the inode allocation bitmap for a given block_group, reading
 * into the specified slot in the superblock's bitmap cache.
 *
 * Return buffer_head of bitmap on success or NULL.
 */
static struct buffer_head *
read_inode_bitmap(struct super_block * sb, unsigned long block_group)

// 1. super_block 的 bitmap cache ?
// 2. 如何确定 block_group 的编号 ? 通过ino 计算 / 每一个 block_group 的容量

// by this !
struct ext2_group_desc * ext2_get_group_desc(struct super_block * sb,
					     unsigned int block_group,
					     struct buffer_head ** bh)
// we get :
/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
	__le32	bg_block_bitmap;		/* Blocks bitmap block */
	__le32	bg_inode_bitmap;		/* Inodes bitmap block */
	__le32	bg_inode_table;		/* Inodes table block */
	__le16	bg_free_blocks_count;	/* Free blocks count */
	__le16	bg_free_inodes_count;	/* Free inodes count */
	__le16	bg_used_dirs_count;	/* Directories count */
	__le16	bg_pad;
	__le32	bg_reserved[3];
};
```

```c
// 处理一些inode ref 加减的事情
static void ext2_release_inode(struct super_block *sb, int group, int dir)

// 真正的释放
void ext2_free_inode (struct inode * inode)

// TODO
// 1. 偶遇到一个新的子系统 quota , man quotactl
// 2. void mark_buffer_dirty(struct buffer_head *bh)
// 3. int sync_dirty_buffer(struct buffer_head *bh) 又是两个buffer 内容的支持，其实不是非常的理解，为什么总是使用bitmap 而不是page cache 处理

// 原子操作，防止出现释放的过程中间，inode 就被人使用了 !
#define ext2_clear_bit_atomic(l, nr, addr)	test_and_clear_bit_le(nr, addr)

static inline void brelse(struct buffer_head *bh)
{
	if (bh)
		__brelse(bh);
}

/*
 * Decrement a buffer_head's reference count.  If all buffers against a page
 * have zero reference count, are clean and unlocked, and if the page is clean
 * and unlocked then try_to_free_buffers() may strip the buffers from the page
 * in preparation for freeing it (sometimes, rarely, buffers are removed from
 * a page but it ends up not being freed, and buffers may later be reattached).
 */
void __brelse(struct buffer_head * buf)
{
	if (atomic_read(&buf->b_count)) {
		put_bh(buf);
		return;
	}
	WARN(1, KERN_ERR "VFS: brelse: Trying to free free buffer\n");
}
```


```c
/*
 * We perform asynchronous prereading of the new inode's inode block when
 * we create the inode, in the expectation that the inode will be written
 * back soon.  There are two reasons:
 *
 * - When creating a large number of files, the async prereads will be
 *   nicely merged into large reads
 * - When writing out a large number of inodes, we don't need to keep on
 *   stalling the writes while we read the inode block.
 *
 * FIXME: ext2_get_group_desc() needs to be simplified.
 */
static void ext2_preread_inode(struct inode *inode)

// TODO 说实话，永远都是不知道到底一个 vfs_inode 的上面套上什么东西!
struct bdev_inode {
	struct block_device bdev;
	struct inode vfs_inode;
};

struct block_device *I_BDEV(struct inode *inode)
{
	return &BDEV_I(inode)->bdev;
}

static inline struct bdev_inode *BDEV_I(struct inode *inode)
{
	return container_of(inode, struct bdev_inode, vfs_inode);
}

// 最后进入到buffer.c 中间实行真正的读取

/*
 * Do async read-ahead on a buffer..
 */
void __breadahead(struct block_device *bdev, sector_t block, unsigned size)
{
	struct buffer_head *bh = __getblk(bdev, block, size);
	if (likely(bh)) {
		ll_rw_block(REQ_OP_READ, REQ_RAHEAD, 1, &bh);
		brelse(bh);
	}
}
```

```c
/*
 * There are two policies for allocating an inode.  If the new inode is
 * a directory, then a forward search is made for a block group with both
 * free space and a low directory-to-inode ratio; if that fails, then of
 * the groups with above-average free space, that group with the fewest
 * directories already is chosen.
 *
 * For other inodes, search forward from the parent directory\'s block
 * group to find a free inode.
 */
static int find_group_dir(struct super_block *sb, struct inode *parent)
// TODO 所以为什么需要让目录inode 均匀分布呢 ?
```


```c
/*
 * Orlov's allocator for directories.
 *
 * We always try to spread first-level directories.
 *
 * If there are blockgroups with both free inodes and free blocks counts
 * not worse than average we return one with smallest directory count.
 * Otherwise we simply return a random group.
 *
 * For the rest rules look so:
 *
 * It's OK to put directory into a group unless
 * it has too many directories already (max_dirs) or
 * it has too few free inodes left (min_inodes) or
 * it has too few free blocks left (min_blocks) or
 * it's already running too large debt (max_debt).
 * Parent's group is preferred, if it doesn't satisfy these
 * conditions we search cyclically through the rest. If none
 * of the groups look good we just look for a group with more
 * free inodes than average (starting at parent's group).
 *
 * Debt is incremented each time we allocate a directory and decremented
 * when we allocate an inode, within 0--255.
 */

#define INODE_COST 64
#define BLOCK_COST 256

static int find_group_orlov(struct super_block *sb, struct inode *parent)
// Orlov
// 感觉就是上一个函数的复杂版!
```

```c
// 第三个查找group的策略
static int find_group_other(struct super_block *sb, struct inode *parent)

// 而且最后，回顾ialloc 的 ext2_new_inode 函数，就是通过ino读取disk 然后初始化各种mm 特有的内容
// 在ext2_new_inode中间，只有，其中如下规则确定。
	if (S_ISDIR(mode)) {
		if (test_opt(sb, OLDALLOC))
			group = find_group_dir(sb, dir);
		else
			group = find_group_orlov(sb, dir);
	} else
		group = find_group_other(sb, dir);
```

> 此外，还提供了两个外部统计interrface

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
