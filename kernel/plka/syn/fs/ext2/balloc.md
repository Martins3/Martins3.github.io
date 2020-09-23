# balloc.c 分析

```c
struct ext2_group_desc * ext2_get_group_desc(struct super_block * sb,
					     unsigned int block_group,
					     struct buffer_head ** bh)

// 利用 ext2_test_bit 检查 block_group 的 valid
static int ext2_valid_block_bitmap(struct super_block *sb,
					struct ext2_group_desc *desc,
					unsigned int block_group,
					struct buffer_head *bh)



// 这一个函数读入，读入之后, ext2_valid_block_bitmap 检查，然后外部 interface  ext2_get_group_desc 被获取来分析
// 感觉这个东西仅仅读取一次就可以了
/*
 * Read the bitmap for a given block_group,and validate the
 * bits for block/inode/inode tables are set in the bitmaps
 *
 * Return buffer_head on success or NULL in case of failure.
 */
static struct buffer_head *
read_block_bitmap(struct super_block *sb, unsigned int block_group)


// 上各种锁，然后将 free_blocks 的数目改掉
static void group_adjust_blocks(struct super_block *sb, int group_no,
	struct ext2_group_desc *desc, struct buffer_head *bh, int count)

/*
 * The reservation window structure operations
 * --------------------------------------------
 * Operations include:
 * dump, find, add, remove, is_empty, find_next_reservable_window, etc.
 *
 * We use a red-black tree to represent per-filesystem reservation
 * windows.
 *
 */

 // 然后下面分别介绍其中的各种函数。
```

```c
/**
 * ext2_init_block_alloc_info()
 * @inode:		file inode structure
 *
 * Allocate and initialize the  reservation window structure, and
 * link the window to the ext2 inode structure at last
 *
 * The reservation window structure is only dynamically allocated
 * and linked to ext2 inode the first time the open file
 * needs a new block. So, before every ext2_new_block(s) call, for
 * regular files, we should check whether the reservation window
 * structure exists or not. In the latter case, this function is called.
 * Fail to do so will result in block reservation being turned off for that
 * open file.
 *
 * This function is called from ext2_get_blocks_handle(), also called
 * when setting the reservation window size through ioctl before the file
 * is open for write (needs block allocation).
 *
 * Needs truncate_mutex protection prior to calling this function.
 */
 // 其实就是kmalloc 一个结构体，然后之。
 // ioctl 和 ext2_get_blocks() 调用
void ext2_init_block_alloc_info(struct inode *inode)


/**
 * ext2_discard_reservation()
 * @inode:		inode
 *
 * Discard(free) block reservation window on last file close, or truncate
 * or at last iput().
 *
 * It is being called in three cases:
 * 	ext2_release_file(): last writer closes the file
 * 	ext2_clear_inode(): last iput(), when nobody links to this file.
 * 	ext2_truncate(): when the block indirect map is about to change.
 */
// TODO kfree 在哪里啊 ? 为什么不是和 ext2_init_block_alloc_info 对应的 ?
void ext2_discard_reservation(struct inode *inode)
```

就是读取block 然后进行分配，理解reseravation 的工作原理，书上讲过，可以看书了!

http://ext2.sourceforge.net/2005-ols/paper-html/node7.html
