#ifndef SIMPLEFS_BITMAP_H
#define SIMPLEFS_BITMAP_H

#include <linux/bitmap.h>

#include "simplefs.h"
#include "simplefs_debug.h"

#ifdef __KERNEL__
#include "simplefs_journal.h"

/*
 * 把包含指定位的位图块并入当前 journal 事务（若存在）。
 * 位图约定：bfree/ifree 的位图块紧跟 inode store 之后（ifree 在前）。
 * 没有运行中的事务时（writeback/无 handle 路径）自动跳过。
 */
static inline void simplefs_journal_bitmap(struct simplefs_sb_info *sbi,
					   bool inode_map, uint32_t bit,
					   uint32_t len)
{
	struct simplefs_journal *journal = sbi->s_journal;
	uint32_t index, last, block;
	char *base;

	if (!journal || !len)
		return;

	index = bit / (SIMPLEFS_BLOCK_SIZE * 8);
	last = (bit + len - 1) / (SIMPLEFS_BLOCK_SIZE * 8);
	for (; index <= last; index++) {
		if (inode_map) {
			block = 1 + sbi->nr_istore_blocks + index;
			base = (char *)sbi->ifree_bitmap;
		} else {
			block = 1 + sbi->nr_istore_blocks +
				sbi->nr_ifree_blocks + index;
			base = (char *)sbi->bfree_bitmap;
		}
		simplefs_journal_block(journal->j_sb, block,
				       base + index * SIMPLEFS_BLOCK_SIZE);
	}
}

static inline int simplefs_journal_free_counts(struct simplefs_sb_info *sbi)
{
	struct simplefs_sb_info *disk_sb;
	struct folio *folio;
	int ret;

	if (!sbi->s_journal)
		return 0;

	disk_sb = simplefs_get_folio(sbi->sb, SIMPLEFS_SB_BLOCK_NR, &folio);
	if (IS_ERR(disk_sb))
		return PTR_ERR(disk_sb);
	disk_sb->nr_free_inodes = sbi->nr_free_inodes;
	disk_sb->nr_free_blocks = sbi->nr_free_blocks;
	ret = simplefs_journal_dirty_folio(sbi->sb, SIMPLEFS_SB_BLOCK_NR,
					   disk_sb, folio);
	folio_release_kmap(folio, disk_sb);
	return ret;
}
#endif /* __KERNEL__ */

/* Returns the first bit found and clears the following 'len' consecutive
 * free bits (sets them to 1) in a given in-memory bitmap spanning multiple
 * blocks. Returns 0 if an adequate number of free bits were not found.
 * Assumes the first bit is never free (reserved for the superblock and the
 * root inode), allowing the use of 0 as an error value.
 */
static inline uint32_t get_first_free_bits_from(unsigned long *freemap,
						unsigned long size, uint32_t len,
						uint32_t start)
{
	uint32_t bit, prev = 0, count = 0;

	bit = start;
	for_each_set_bit_from(bit, freemap, size) {
		if (prev != bit - 1)
			count = 0;
		prev = bit;
		if (++count == len) {
			bitmap_clear(freemap, bit - len + 1, len);
			return bit - len + 1;
		}
	}
	return 0;
}

static inline uint32_t get_first_free_bits(unsigned long *freemap,
					   unsigned long size, uint32_t len)
{
	return get_first_free_bits_from(freemap, size, len, 0);
}

/* Return an unused inode number and mark it used.
 * Return 0 if no free inode was found.
 */
static inline uint32_t get_free_inode(struct simplefs_sb_info *sbi)
{
	struct simplefs_handle *handle;
	uint32_t ret;

	handle = simplefs_journal_start_sb(sbi->sb, 1);
	if (IS_ERR(handle))
		return 0;

	mutex_lock(&sbi->bitmap_lock);
	ret = get_first_free_bits(sbi->ifree_bitmap, sbi->nr_inodes, 1);
	if (ret) {
		sbi->nr_free_inodes--;
		sfs_stat_inc(inode_alloc_count);
		simplefs_journal_bitmap(sbi, true, ret, 1);
		if (simplefs_journal_free_counts(sbi))
			ret = 0;
	}
	mutex_unlock(&sbi->bitmap_lock);
	if (simplefs_journal_stop(handle))
		return 0;
	return ret;
}

/* Return 'len' unused block(s) number and mark it used.
 * Return 0 if no enough free block(s) were found.
 */
static inline uint32_t __simplefs_get_free_blocks(struct simplefs_sb_info *sbi,
						  uint32_t len,
						  const char *caller)
{
	struct simplefs_handle *handle;
	uint32_t ret;
	uint32_t search = 0;
	uint32_t root_dir_block = 1 + sbi->nr_istore_blocks +
				 sbi->nr_ifree_blocks + sbi->nr_bfree_blocks;

	handle = simplefs_journal_start_sb(sbi->sb, 1);
	if (IS_ERR(handle))
		return 0;

	mutex_lock(&sbi->bitmap_lock);
	for (;;) {
		bool still_free;
		int prepare_ret;

		ret = get_first_free_bits_from(sbi->bfree_bitmap,
					       sbi->nr_blocks, len, search);
		if (!ret)
			break;
		still_free = test_bit(ret, sbi->bfree_bitmap);

		prepare_ret = simplefs_journal_prepare_new_blocks(
			sbi->sb, ret, len);
		if (prepare_ret) {
			bitmap_set(sbi->bfree_bitmap, ret, len);
			if (prepare_ret == -EAGAIN) {
				search = ret + 1;
				continue;
			}
			ret = 0;
			break;
		}

		sbi->nr_free_blocks -= len;
		/* A block's pending discard belongs to its previous free
		 * lifetime.  Cancel it as part of allocation while bitmap_lock
		 * still excludes the discard scanner; a later discard must never
		 * cross into the newly allocated lifetime. */
		if (sbi->discard_pending)
			bitmap_clear(sbi->discard_pending, ret, len);
		sfs_stat_inc(block_alloc_count);
		simplefs_journal_bitmap(sbi, false, ret, len);
		if (simplefs_journal_free_counts(sbi))
			ret = 0;
		pr_debug("bitmap alloc caller=%s start=%u len=%u still_free=%d nr_free=%u\n",
			caller, ret, len, still_free, sbi->nr_free_blocks);
		if (ret <= root_dir_block && root_dir_block < ret + len) {
			pr_warn("bitmap alloc touched root_dir_block=%u start=%u len=%u\n",
				root_dir_block, ret, len);
			dump_stack();
		}
		break;
	}
	mutex_unlock(&sbi->bitmap_lock);
	if (simplefs_journal_stop(handle))
		return 0;
	return ret;
}

#define get_free_blocks(sbi, len) \
	__simplefs_get_free_blocks((sbi), (len), __func__)

static inline bool simplefs_blocks_are_free(struct simplefs_sb_info *sbi,
					    uint32_t bno, uint32_t len)
{
	uint32_t i;

	if (!bno || !len || bno >= sbi->nr_blocks ||
	    len > sbi->nr_blocks - bno)
		return false;

	mutex_lock(&sbi->bitmap_lock);
	for (i = 0; i < len; i++) {
		if (!test_bit(bno + i, sbi->bfree_bitmap))
			goto out_false;
	}

	mutex_unlock(&sbi->bitmap_lock);
	return true;

out_false:
	mutex_unlock(&sbi->bitmap_lock);
	return false;
}

static inline bool simplefs_take_exact_blocks(struct simplefs_sb_info *sbi,
					      uint32_t bno, uint32_t len)
{
	struct simplefs_handle *handle;
	uint32_t i;

	if (!bno || !len || bno >= sbi->nr_blocks ||
	    len > sbi->nr_blocks - bno)
		return false;

	handle = simplefs_journal_start_sb(sbi->sb, 1);
	if (IS_ERR(handle))
		return false;

	mutex_lock(&sbi->bitmap_lock);
	for (i = 0; i < len; i++) {
		if (!test_bit(bno + i, sbi->bfree_bitmap))
			goto out_false;
	}

	bitmap_clear(sbi->bfree_bitmap, bno, len);
	if (simplefs_journal_prepare_new_blocks(sbi->sb, bno, len)) {
		bitmap_set(sbi->bfree_bitmap, bno, len);
		goto out_false;
	}
	if (sbi->discard_pending)
		bitmap_clear(sbi->discard_pending, bno, len);
	sbi->nr_free_blocks -= len;
	sfs_stat_inc(block_alloc_count);
	simplefs_journal_bitmap(sbi, false, bno, len);
	if (simplefs_journal_free_counts(sbi))
		goto out_false;
	mutex_unlock(&sbi->bitmap_lock);
	if (simplefs_journal_stop(handle))
		return false;
	return true;

out_false:
	mutex_unlock(&sbi->bitmap_lock);
	simplefs_journal_stop(handle);
	return false;
}

/* Mark the 'len' bit(s) from i-th bit in freemap as free (i.e. 1) */
static inline int put_free_bits(unsigned long *freemap, unsigned long size,
				uint32_t i, uint32_t len)
{
	if (!len || i >= size || len > size - i)
		return -EINVAL;

	bitmap_set(freemap, i, len);

	return 0;
}

/* Mark an inode as unused */
static inline void put_inode(struct simplefs_sb_info *sbi, uint32_t ino)
{
	struct simplefs_handle *handle;

	handle = simplefs_journal_start_sb(sbi->sb, 1);
	if (IS_ERR(handle))
		return;

	mutex_lock(&sbi->bitmap_lock);
	if (!ino || ino >= sbi->nr_inodes) {
		pr_warn("bitmap invalid inode free ino=%u nr_inodes=%u\n",
			ino, sbi->nr_inodes);
		dump_stack();
		goto out;
	}
	if (test_bit(ino, sbi->ifree_bitmap)) {
		pr_warn("bitmap double inode free ino=%u\n", ino);
		dump_stack();
		goto out;
	}
	if (put_free_bits(sbi->ifree_bitmap, sbi->nr_inodes, ino, 1))
		goto out;

	sbi->nr_free_inodes++;
	sfs_stat_inc(inode_free_count);
	simplefs_journal_bitmap(sbi, true, ino, 1);
	if (simplefs_journal_free_counts(sbi))
		pr_err("failed to journal free counts for inode %u\n", ino);

out:
	mutex_unlock(&sbi->bitmap_lock);
	if (simplefs_journal_stop(handle))
		pr_err("failed to commit inode bitmap free ino=%u\n", ino);
}

/* Mark len block(s) as unused */
static inline void __simplefs_put_blocks(struct simplefs_sb_info *sbi,
					 uint32_t bno, uint32_t len,
					 const char *caller)
{
	struct simplefs_handle *handle;
	uint32_t root_dir_block = 1 + sbi->nr_istore_blocks +
				 sbi->nr_ifree_blocks + sbi->nr_bfree_blocks;
	uint32_t i;

	handle = simplefs_journal_start_sb(sbi->sb, 1);
	if (IS_ERR(handle))
		return;

	mutex_lock(&sbi->bitmap_lock);
	if (!bno || !len || bno >= sbi->nr_blocks ||
	    len > sbi->nr_blocks - bno) {
		pr_warn("bitmap invalid free caller=%s start=%u len=%u nr_blocks=%u\n",
			caller, bno, len, sbi->nr_blocks);
		dump_stack();
		goto out;
	}
	if (bno <= root_dir_block && root_dir_block < bno + len) {
		pr_warn("bitmap refused root_dir_block free root=%u start=%u len=%u\n",
			root_dir_block, bno, len);
		dump_stack();
		goto out;
	}
	for (i = 0; i < len; i++) {
		if (test_bit(bno + i, sbi->bfree_bitmap)) {
			pr_warn("bitmap double free caller=%s start=%u len=%u free_bit=%u\n",
				caller, bno, len, bno + i);
			dump_stack();
			goto out;
		}
	}
	if (put_free_bits(sbi->bfree_bitmap, sbi->nr_blocks, bno, len))
		goto out;

	sbi->nr_free_blocks += len;
	sfs_stat_inc(block_free_count);
	if (sbi->s_discard)
		bitmap_set(sbi->discard_pending, bno, len);
	simplefs_journal_bitmap(sbi, false, bno, len);
	if (simplefs_journal_free_counts(sbi))
		pr_err("failed to journal free counts for blocks %u+%u\n",
		       bno, len);
	pr_debug("bitmap free caller=%s start=%u len=%u nr_free=%u\n",
		 caller, bno, len, sbi->nr_free_blocks);
out:
	mutex_unlock(&sbi->bitmap_lock);
	if (simplefs_journal_stop(handle))
		pr_err("failed to commit block bitmap free start=%u len=%u\n",
		       bno, len);
	else if (sbi->s_discard &&
		 (!sbi->s_journal || !sbi->s_journal->j_jbd2))
		simplefs_issue_pending_discards(sbi->sb);
}

#define put_blocks(sbi, bno, len) \
	__simplefs_put_blocks((sbi), (bno), (len), __func__)

#endif
