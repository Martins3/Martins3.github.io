/* 
 * Mainly copied from fs/erofs/data.c
 */
#include <linux/sched/mm.h>
#include <linux/pagemap.h>

#include <linux/fs.h>
#include <linux/blkdev.h>

#include "simplefs.h"
#include "simplefs_bitmap.h"
#include "simplefs_journal.h"

/*
 * 配合 folio_release_kmap(folio, de); 来使用
 *
 * TODO 没有 folio_release_kmap 会导致内存无法被回收，但是 folio_release_kmap
 * 不会导致内存立刻被释放 对吧
 */
void *simplefs_get_folio(struct super_block *sb, unsigned long n,
			    struct folio **foliop)
{
	struct address_space *mapping = sb->s_bdev->bd_mapping;
	struct folio *folio;
	void *kaddr;

	/* 从地址空间获取folio */
	folio = read_mapping_folio(mapping, n, NULL);
	if (IS_ERR(folio))
		return ERR_CAST(folio);

	/* 映射folio到内核空间，处理 large folio 中的偏移 */
	kaddr = kmap_local_folio(folio, offset_in_folio(folio, n * PAGE_SIZE));
	if (simplefs_journal_prepare_current(sb, n)) {
		kunmap_local(kaddr);
		folio_put(folio);
		return ERR_PTR(-EIO);
	}
	*foliop = folio;
	return kaddr;
}

/*
 * 元数据通过块设备 mapping 缓存，而普通文件数据通过 inode mapping。
 * 元数据块放回全局位图前必须完成旧 bdev folio 的写回；否则该块被复用
 * 为文件数据后，迟到的 leaf/root/xattr 写回会覆盖新数据。
 */
int simplefs_retire_metadata_blocks(struct super_block *sb, uint32_t bno,
				    uint32_t len)
{
	struct address_space *mapping = sb->s_bdev->bd_mapping;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_journal *journal = sbi->s_journal;
	struct simplefs_handle *handle = NULL;
	loff_t start = (loff_t)bno << sb->s_blocksize_bits;
	loff_t end = ((loff_t)bno + len) << sb->s_blocksize_bits;
	struct inode *root;
	uint32_t i;
	int ret;
	bool own_handle = false;

	if (journal && journal->j_jbd2) {
		if (!simplefs_journal_has_current_handle(sb)) {
			root = sb->s_root ? d_inode(sb->s_root) : NULL;
			if (!root)
				return -EIO;
			handle = simplefs_journal_start(root, len);
			if (IS_ERR(handle))
				return PTR_ERR(handle);
			own_handle = true;
		}

		for (i = 0; i < len; i++) {
			ret = simplefs_journal_forget_current(sb, bno + i);
			if (ret)
				goto out_abort;
		}
		put_blocks(sbi, bno, len);

		if (!own_handle)
			return 0;

		ret = simplefs_journal_stop(handle);
		if (ret)
			return ret;

		/* Revoke/forget is the authoritative JBD2 lifetime operation.
		 * Dropping the now-clean folio is only a cache optimisation. */
		filemap_invalidate_lock(mapping);
		invalidate_inode_pages2_range(mapping, start >> PAGE_SHIFT,
					      (end - 1) >> PAGE_SHIFT);
		filemap_invalidate_unlock(mapping);
		return 0;
	}

	ret = filemap_write_and_wait_range(mapping, start, end - 1);
	if (ret)
		return ret;

	/*
	 * A clean bdev folio still contains the retired metadata image.  Drop it
	 * before making the blocks available for a different owner; otherwise a
	 * later metadata allocation can observe that stale in-memory image even
	 * after the physical block has been overwritten through another mapping.
	 */
	filemap_invalidate_lock(mapping);
	ret = invalidate_inode_pages2_range(mapping, start >> PAGE_SHIFT,
					    (end - 1) >> PAGE_SHIFT);
	filemap_invalidate_unlock(mapping);
	if (ret)
		return ret;

	put_blocks(SIMPLEFS_SB(sb), bno, len);
	return 0;

out_abort:
	if (own_handle) {
		simplefs_journal_abort(handle, ret);
		simplefs_journal_stop(handle);
	}
	return ret;
}
