#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/statfs.h>
#include <linux/parser.h>
#include <linux/blkdev.h>
#include <linux/fs_context.h>
#include <linux/pagemap.h>
#include <linux/kdev_t.h>
#include <linux/writeback.h>

#include <linux/exportfs.h>
#include "simplefs.h"
#include "simplefs_trace.h"
#include "simplefs_bitmap.h"
#include "simplefs_journal.h"

static struct kmem_cache *simplefs_inode_cache;

/* Forward declaration */
static int simplefs_sync_fs(struct super_block *sb, int wait);

enum simplefs_block_owner_kind {
	SIMPLEFS_OWNER_META = 1,
	SIMPLEFS_OWNER_EXTENT_ROOT,
	SIMPLEFS_OWNER_EXTENT_LEAF,
	SIMPLEFS_OWNER_FILE_DATA,
	SIMPLEFS_OWNER_DIR_INDEX,
	SIMPLEFS_OWNER_DIR_DATA,
	SIMPLEFS_OWNER_SYMLINK,
	SIMPLEFS_OWNER_XATTR,
};

static const char *simplefs_owner_kind_name(u8 kind)
{
	switch (kind) {
	case SIMPLEFS_OWNER_META:
		return "meta";
	case SIMPLEFS_OWNER_EXTENT_ROOT:
		return "extent-root";
	case SIMPLEFS_OWNER_EXTENT_LEAF:
		return "extent-leaf";
	case SIMPLEFS_OWNER_FILE_DATA:
		return "file-data";
	case SIMPLEFS_OWNER_DIR_INDEX:
		return "dir-index";
	case SIMPLEFS_OWNER_DIR_DATA:
		return "dir-data";
	case SIMPLEFS_OWNER_SYMLINK:
		return "symlink-data";
	case SIMPLEFS_OWNER_XATTR:
		return "xattr";
	default:
		return "unknown";
	}
}

static int simplefs_rebuild_mark_block(unsigned long *used_map,
				       u32 *owner_ino, u8 *owner_kind,
				       unsigned long nr_blocks, uint32_t block,
				       u8 kind, uint32_t ino)
{
	if (!block || block >= nr_blocks) {
		pr_err("rebuild: invalid %s block=%u ino=%u\n",
		       simplefs_owner_kind_name(kind), block, ino);
		return -EUCLEAN;
	}

	if (test_bit(block, used_map)) {
		pr_err("rebuild: overlap on block=%u first=%s/%u second=%s/%u\n",
		       block, simplefs_owner_kind_name(owner_kind[block]),
		       owner_ino[block], simplefs_owner_kind_name(kind), ino);
		return -EUCLEAN;
	}

	__set_bit(block, used_map);
	owner_ino[block] = ino;
	owner_kind[block] = kind;
	return 0;
}

static int simplefs_rebuild_mark_range(unsigned long *used_map,
				       u32 *owner_ino, u8 *owner_kind,
				       unsigned long nr_blocks, uint32_t start,
				       uint32_t len, u8 kind, uint32_t ino)
{
	uint32_t i;

	if (!len)
		return 0;
	if (!start || start >= nr_blocks || len > nr_blocks - start) {
		pr_err("rebuild: invalid %s range start=%u len=%u ino=%u\n",
		       simplefs_owner_kind_name(kind), start, len, ino);
		return -EUCLEAN;
	}

	for (i = 0; i < len; i++) {
		int ret = simplefs_rebuild_mark_block(used_map, owner_ino,
						      owner_kind, nr_blocks,
						      start + i, kind, ino);

		if (ret)
			return ret;
	}

	return 0;
}

static int simplefs_rebuild_scan_file(struct super_block *sb,
				      unsigned long *used_map, u32 *owner_ino,
				      u8 *owner_kind,
				      uint32_t ino, uint32_t root_block)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_extent_buffer buf;
	uint32_t i;
	int ret;

	ret = simplefs_rebuild_mark_block(used_map, owner_ino, owner_kind,
					  sbi->nr_blocks, root_block,
					  SIMPLEFS_OWNER_EXTENT_ROOT, ino);
	if (ret)
		return ret;

	ret = simplefs_file_load_extents(sb, root_block, &buf);
	if (ret)
		return ret;

	for (i = 0; i < buf.nr_leaf_blocks; i++) {
		ret = simplefs_rebuild_mark_block(used_map, owner_ino,
						  owner_kind, sbi->nr_blocks,
						  buf.leaf_blocks[i],
						  SIMPLEFS_OWNER_EXTENT_LEAF,
						  ino);
		if (ret)
			goto out;
	}

	for (i = 0; i < buf.nr_extents; i++) {
		struct simplefs_extent *ext = &buf.extents[i];

		if (simplefs_extent_is_empty(ext))
			continue;
		ret = simplefs_rebuild_mark_range(used_map, owner_ino,
						  owner_kind, sbi->nr_blocks,
						  ext->ee_start,
						  simplefs_ext_len(ext),
						  SIMPLEFS_OWNER_FILE_DATA,
						  ino);
		if (ret)
			goto out;
	}

out:
	simplefs_file_destroy_extents(&buf);
	return ret;
}

static int simplefs_rebuild_scan_dir(struct super_block *sb,
				     unsigned long *used_map, u32 *owner_ino,
				     u8 *owner_kind,
				     uint32_t ino, uint32_t dir_block)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct folio *folio;
	struct simplefs_file_ei_block *eblock;
	uint32_t i;
	int ret = 0;

	ret = simplefs_rebuild_mark_block(used_map, owner_ino, owner_kind,
					  sbi->nr_blocks, dir_block,
					  SIMPLEFS_OWNER_DIR_INDEX, ino);
	if (ret)
		return ret;

	eblock = simplefs_get_folio(sb, dir_block, &folio);
	if (IS_ERR(eblock))
		return PTR_ERR(eblock);

	for (i = 0; i < SIMPLEFS_MAX_EXTENTS; i++) {
		struct simplefs_extent *ext = &eblock->extents[i];

		if (simplefs_extent_is_empty(ext))
			continue;
		ret = simplefs_rebuild_mark_range(used_map, owner_ino,
						  owner_kind, sbi->nr_blocks,
						  ext->ee_start,
						  simplefs_ext_len(ext),
						  SIMPLEFS_OWNER_DIR_DATA, ino);
		if (ret)
			break;
	}

	folio_release_kmap(folio, eblock);
	return ret;
}

int simplefs_rebuild_block_bitmap(struct super_block *sb)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	unsigned long *used_map;
	unsigned long *new_bfree;
	u32 *owner_ino;
	u8 *owner_kind;
	uint32_t meta_end, free_blocks;
	uint32_t ino;
	int ret = 0;

	used_map = bitmap_zalloc(sbi->nr_blocks, GFP_NOFS);
	if (!used_map)
		return -ENOMEM;
	owner_ino = kvcalloc(sbi->nr_blocks, sizeof(*owner_ino), GFP_NOFS);
	if (!owner_ino) {
		ret = -ENOMEM;
		goto out_used;
	}
	owner_kind = kvcalloc(sbi->nr_blocks, sizeof(*owner_kind), GFP_NOFS);
	if (!owner_kind) {
		ret = -ENOMEM;
		goto out_owner_ino;
	}

	meta_end = 1 + sbi->nr_istore_blocks + sbi->nr_ifree_blocks +
		   sbi->nr_bfree_blocks;
	bitmap_set(used_map, 0, meta_end);
	for (ino = 0; ino < meta_end; ino++) {
		owner_ino[ino] = 0;
		owner_kind[ino] = SIMPLEFS_OWNER_META;
	}
	if (sbi->s_journal_present && sbi->s_journal_start < sbi->nr_blocks) {
		uint32_t journal_blocks = sbi->nr_blocks - sbi->s_journal_start;

		bitmap_set(used_map, sbi->s_journal_start, journal_blocks);
		for (ino = sbi->s_journal_start; ino < sbi->nr_blocks; ino++) {
			owner_ino[ino] = 0;
			owner_kind[ino] = SIMPLEFS_OWNER_META;
		}
	}

	for (ino = 1; ino < sbi->nr_inodes; ino++) {
		struct simplefs_inode *disk_inode;
		struct folio *folio;
		uint32_t inode_block;
		uint32_t inode_shift;
		uint32_t mode;
		uint32_t ei_block;
		uint32_t xattr_block;

		if (test_bit(ino, sbi->ifree_bitmap))
			continue;

		inode_block = (ino / SIMPLEFS_INODES_PER_BLOCK) + 1;
		inode_shift = ino % SIMPLEFS_INODES_PER_BLOCK;
		disk_inode = simplefs_get_folio(sb, inode_block, &folio);
		if (IS_ERR(disk_inode)) {
			ret = PTR_ERR(disk_inode);
			break;
		}

		disk_inode += inode_shift;
		mode = le32_to_cpu(disk_inode->i_mode);
		ei_block = le32_to_cpu(disk_inode->ei_block);
		xattr_block = le32_to_cpu(disk_inode->i_xattr_block);

		if (S_ISREG(mode) && ei_block)
			ret = simplefs_rebuild_scan_file(sb, used_map,
							 owner_ino, owner_kind,
							 ino,
							 ei_block);
		else if (S_ISDIR(mode) && ei_block)
			ret = simplefs_rebuild_scan_dir(sb, used_map,
							owner_ino, owner_kind,
							ino,
							ei_block);
		else if (S_ISLNK(mode) && ei_block)
			ret = simplefs_rebuild_mark_block(used_map, owner_ino,
							  owner_kind,
							  sbi->nr_blocks,
							  ei_block,
							  SIMPLEFS_OWNER_SYMLINK,
							  ino);

		if (!ret && xattr_block)
			ret = simplefs_rebuild_mark_block(used_map, owner_ino,
							  owner_kind,
							  sbi->nr_blocks,
							  xattr_block,
							  SIMPLEFS_OWNER_XATTR,
							  ino);

		folio_release_kmap(folio, disk_inode - inode_shift);
		if (ret)
			break;
	}

	if (!ret) {
		for (ino = 0; ino < sbi->nr_blocks; ino++) {
			if (!test_bit(ino, used_map))
				continue;
			if (!test_bit(ino, sbi->bfree_bitmap))
				continue;
			pr_warn("rebuild: mapped-but-free block=%u owner=%s/%u\n",
				ino, simplefs_owner_kind_name(owner_kind[ino]),
				owner_ino[ino]);
			break;
		}

		new_bfree = bitmap_zalloc(sbi->nr_blocks, GFP_NOFS);
		if (!new_bfree) {
			ret = -ENOMEM;
			goto out;
		}
		bitmap_fill(new_bfree, sbi->nr_blocks);
		bitmap_complement(new_bfree, used_map, sbi->nr_blocks);
		free_blocks = bitmap_weight(new_bfree, sbi->nr_blocks);

		mutex_lock(&sbi->bitmap_lock);
		bitmap_copy(sbi->bfree_bitmap, new_bfree, sbi->nr_blocks);
		sbi->nr_free_blocks = free_blocks;
		sbi->nr_free_inodes = bitmap_weight(sbi->ifree_bitmap,
						    sbi->nr_inodes);
		mutex_unlock(&sbi->bitmap_lock);
		pr_debug("rebuild: nr_free_blocks=%u nr_free_inodes=%u\n",
			 sbi->nr_free_blocks, sbi->nr_free_inodes);
		bitmap_free(new_bfree);
	}

	kvfree(owner_kind);
	kvfree(owner_ino);
out:
	bitmap_free(used_map);
	return ret;

out_owner_ino:
	kvfree(owner_ino);
out_used:
	bitmap_free(used_map);
	return ret;
}

int simplefs_init_inode_cache(void)
{
	simplefs_inode_cache = kmem_cache_create_usercopy(
		"simplefs_cache", sizeof(struct simplefs_inode_info), 0, 0, 0,
		sizeof(struct simplefs_inode_info), NULL);
	if (!simplefs_inode_cache)
		return -ENOMEM;
	return 0;
}

void simplefs_destroy_inode_cache(void)
{
	kmem_cache_destroy(simplefs_inode_cache);
}

static struct inode *simplefs_alloc_inode(struct super_block *sb)
{
	struct simplefs_inode_info *ci =
		kmem_cache_alloc(simplefs_inode_cache, GFP_KERNEL);
	if (!ci)
		return NULL;

	inode_init_once(&ci->vfs_inode);
	mutex_init(&ci->extent_lock);
	atomic_set(&ci->writeback_ioends, 0);
	init_waitqueue_head(&ci->writeback_wait);
	return &ci->vfs_inode;
}

static void simplefs_free_inode(struct inode *inode)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	kmem_cache_free(simplefs_inode_cache, ci);
}

static void simplefs_evict_inode(struct inode *inode)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(inode->i_sb);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct simplefs_handle *retire_handle = NULL;
	int retire_ret;

	trace_simplefs_evict_inode(inode, inode->i_ino);

	truncate_inode_pages_final(&inode->i_data);

	/* Only free blocks and inode number when the file was actually deleted
	 * (nlink == 0). On unmount, inodes with nlink > 0 are evicted but
	 * must NOT have their resources freed - they still exist on disk.
	 * This follows the ext2 pattern: unlink sets nlink=0, evict_inode
	 * does the actual cleanup.
	 */
	if (!inode->i_nlink && !is_bad_inode(inode) &&
	    !simplefs_is_shutdown(inode->i_sb)) {
		retire_handle = simplefs_journal_start_sb(inode->i_sb, 64);
		if (IS_ERR(retire_handle)) {
			if (!simplefs_is_shutdown(inode->i_sb))
				pr_err("failed to start inode retirement ino=%llu ret=%ld\n",
				       (unsigned long long)inode->i_ino,
				       PTR_ERR(retire_handle));
			retire_handle = NULL;
			goto out_clear;
		}

		/* Free data blocks if this is a regular file or directory */
		if ((S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)) && ci->ei_block) {
			struct folio *folio;
			struct simplefs_file_ei_block *eblock;
			int i;

			if (S_ISREG(inode->i_mode)) {
				struct simplefs_extent_buffer buf;

				if (!simplefs_file_load_extents(inode->i_sb,
								ci->ei_block,
								&buf)) {
					for (i = 0; i < buf.nr_extents; i++) {
						trace_simplefs_free_blocks(
							inode->i_sb,
							buf.extents[i].ee_start,
							simplefs_ext_len(&buf.extents[i]));
						put_blocks(
							sbi, buf.extents[i].ee_start,
							simplefs_ext_len(&buf.extents[i]));
					}
					for (i = 0; i < buf.nr_leaf_blocks; i++) {
						trace_simplefs_free_blocks(
							inode->i_sb,
							buf.leaf_blocks[i], 1);
						simplefs_retire_metadata_blocks(
							inode->i_sb,
							buf.leaf_blocks[i], 1);
					}
					simplefs_file_destroy_extents(&buf);
				}
				trace_simplefs_free_blocks(inode->i_sb,
							   ci->ei_block, 1);
				simplefs_retire_metadata_blocks(inode->i_sb,
							ci->ei_block, 1);
			} else {
				eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
					inode->i_sb, ci->ei_block, &folio);
				if (!IS_ERR(eblock)) {
					for (i = 0; i < SIMPLEFS_MAX_EXTENTS; i++) {
						if (eblock->extents[i].ee_start) {
							trace_simplefs_free_blocks(inode->i_sb,
								eblock->extents[i].ee_start,
								simplefs_ext_len(&eblock->extents[i]));
							simplefs_retire_metadata_blocks(
								inode->i_sb,
								eblock->extents[i].ee_start,
								simplefs_ext_len(&eblock->extents[i]));
						}
					}
					folio_release_kmap(folio, eblock);
				}

				trace_simplefs_free_blocks(inode->i_sb, ci->ei_block, 1);
				simplefs_retire_metadata_blocks(inode->i_sb,
							ci->ei_block, 1);
			}
		}

		/* Long symlink targets use a single external block. */
		if (S_ISLNK(inode->i_mode) && ci->ei_block) {
			trace_simplefs_free_blocks(inode->i_sb, ci->ei_block, 1);
			simplefs_retire_metadata_blocks(inode->i_sb,
						ci->ei_block, 1);
		}

		/* Free xattr block */
		simplefs_xattr_delete_inode(inode);

		retire_ret = simplefs_clear_disk_inode(inode);
		if (retire_ret)
			pr_err("failed to clear retired inode ino=%llu ret=%d\n",
			       (unsigned long long)inode->i_ino, retire_ret);

		/* Free the inode number */
		if (inode->i_ino)
			put_inode(sbi, inode->i_ino);

		retire_ret = simplefs_journal_stop(retire_handle);
		if (retire_ret && !simplefs_is_shutdown(inode->i_sb))
			pr_err("failed to commit inode retirement ino=%llu ret=%d\n",
			       (unsigned long long)inode->i_ino, retire_ret);
	}

out_clear:
	clear_inode(inode);
}

static int simplefs_write_inode(struct inode *inode,
				struct writeback_control *wbc)
{
	struct simplefs_handle *handle;
	struct simplefs_inode *disk_inode;
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct folio *folio;
	uint32_t ino = inode->i_ino;
	uint32_t inode_block = (ino / SIMPLEFS_INODES_PER_BLOCK) + 1;
	uint32_t inode_shift = ino % SIMPLEFS_INODES_PER_BLOCK;
	int ret, stop_ret;

	if (simplefs_is_shutdown(sb))
		return -EIO;
	
	pr_debug("[simplefs write_inode] ino=%llu, mode=0%o\n",
		  (unsigned long long)inode->i_ino, inode->i_mode);

	if (ino >= sbi->nr_inodes)
		return 0;

	/* writeback may run outside a namespace transaction.  Start one before
	 * mapping the inode-table folio so simplefs_get_folio() can obtain JBD2
	 * write access before the first byte is changed. */
	handle = simplefs_journal_start_sb(sb, 1);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	disk_inode = (struct simplefs_inode *)simplefs_get_folio(
		sb, inode_block, &folio);
	if (IS_ERR(disk_inode)) {
		ret = PTR_ERR(disk_inode);
		goto out_stop;
	}

	disk_inode += inode_shift;

	/* update the mode using what the generic inode has */
	disk_inode->i_mode = inode->i_mode;
	disk_inode->i_uid = i_uid_read(inode);
	disk_inode->i_gid = i_gid_read(inode);
	disk_inode->i_size = inode->i_size;

	struct timespec64 ctime = inode_get_ctime(inode);
	disk_inode->i_ctime = cpu_to_le32((uint32_t)(int32_t)ctime.tv_sec);
	disk_inode->i_ctime_nsec = cpu_to_le32(ctime.tv_nsec);

	disk_inode->i_atime = cpu_to_le32((uint32_t)(int32_t)inode->i_atime_sec);
	disk_inode->i_atime_nsec = cpu_to_le32(inode->i_atime_nsec);
	disk_inode->i_mtime = cpu_to_le32((uint32_t)(int32_t)inode->i_mtime_sec);
	disk_inode->i_mtime_nsec = cpu_to_le32(inode->i_mtime_nsec);
	disk_inode->i_blocks = cpu_to_le32(
		simplefs_sectors_to_blocks(inode->i_blocks));
	disk_inode->i_nlink = inode->i_nlink;
	disk_inode->ei_block = ci->ei_block;
	disk_inode->i_xattr_block = ci->i_xattr_block;
	disk_inode->i_generation = cpu_to_le32(inode->i_generation);
	disk_inode->i_flags = cpu_to_le32(ci->i_flags);
	disk_inode->i_crtime = cpu_to_le32((uint32_t)(int32_t)ci->i_crtime.tv_sec);
	disk_inode->i_crtime_nsec = cpu_to_le32(ci->i_crtime.tv_nsec);
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
		*((__le32 *)disk_inode->i_data) = cpu_to_le32(old_encode_dev(inode->i_rdev));
	else
		memcpy(disk_inode->i_data, ci->i_data, sizeof(ci->i_data));

	ret = simplefs_journal_dirty_folio(sb, inode_block,
					   disk_inode - inode_shift, folio);
	if (ret && !simplefs_is_shutdown(sb))
		pr_warn("journal inode-table dev=%s block=%u failed: %d shutdown=%d rdonly=%d\n",
			sb->s_id, inode_block, ret, simplefs_is_shutdown(sb),
			sb_rdonly(sb));

	folio_release_kmap(folio, disk_inode - inode_shift);


out_stop:
	stop_ret = simplefs_journal_stop(handle);
	if (!ret)
		ret = stop_ret;
	return ret;
}

int simplefs_persist_inode(struct inode *inode)
{
	return simplefs_write_inode(inode, NULL);
}

int simplefs_clear_disk_inode(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct simplefs_handle *handle;
	uint32_t ino = inode->i_ino;
	uint32_t block = (ino / SIMPLEFS_INODES_PER_BLOCK) + 1;
	uint32_t shift = ino % SIMPLEFS_INODES_PER_BLOCK;
	struct simplefs_inode *base;
	struct folio *folio;
	int ret, stop_ret;

	handle = simplefs_journal_start_sb(sb, 1);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	base = simplefs_get_folio(sb, block, &folio);
	if (IS_ERR(base)) {
		ret = PTR_ERR(base);
		goto out_stop;
	}
	memset(&base[shift], 0, sizeof(base[shift]));
	ret = simplefs_journal_dirty_folio(sb, block, base, folio);
	folio_release_kmap(folio, base);

out_stop:
	stop_ret = simplefs_journal_stop(handle);
	if (!ret)
		ret = stop_ret;
	return ret;
}

static void simplefs_put_super(struct super_block *sb)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	if (sbi) {
		bool discard_bdev_cache = sbi->s_shutdown;
		int sync_ret = 0;

		/* A read-only/norecovery mount must not update home metadata or
		 * consume the journal merely because it is being unmounted. */
		if (!sb_rdonly(sb) && !sbi->s_shutdown &&
		    sbi->s_journal_mode != 2) {
			/* Sync all data to disk before destroying */
			sync_ret = simplefs_sync_fs(sb, 1);

			/* Ensure all dirty folios are written */
			if (!sync_ret)
				sync_ret = filemap_write_and_wait(
					sb->s_bdev->bd_mapping);
			if (!sync_ret)
				sync_ret = simplefs_journal_checkpoint(
					sbi->s_journal);

			/* Clear the lifecycle marker only after both the journal and its
			 * home metadata are stable. */
			if (!sync_ret)
				sync_ret = simplefs_journal_set_needs_recovery(
					sb, false);
			if (sync_ret)
				pr_err("failed to mark filesystem clean on %s: %d\n",
				       sb->s_id, sync_ret);
		}
		
		/* Destroy journal before freeing sbi */
		simplefs_journal_destroy(sb);
		/* Forced shutdown intentionally skips sync.  Drop every residual
		 * bdev folio after JBD2 releases its private state so an old dirty
		 * metadata/data image cannot write back after the loop device is
		 * reused by the recovery mount. */
		if (discard_bdev_cache)
			truncate_inode_pages_range(sb->s_bdev->bd_mapping, 0, -1);
		kfree(sbi->ifree_bitmap);
		kfree(sbi->bfree_bitmap);
		bitmap_free(sbi->discard_pending);
		kfree(sbi);
	}
}

static int simplefs_sync_fs(struct super_block *sb, int wait)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_handle *handle;
	struct simplefs_sb_info *disk_sb;
	struct folio *folio;
	int credits = 1 + sbi->nr_ifree_blocks + sbi->nr_bfree_blocks;
	int i, ret = 0;

	pr_debug("[simplefs] sync_fs called (wait=%d)\n", wait);
	handle = simplefs_journal_start_sb(sb, credits);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	/* Keep counters and both bitmap arrays in one coherent snapshot. */
	mutex_lock(&sbi->bitmap_lock);

	/* Flush superblock */
	disk_sb = (struct simplefs_sb_info *)simplefs_get_folio(sb, 0, &folio);
	if (IS_ERR(disk_sb)) {
		ret = PTR_ERR(disk_sb);
		goto out_unlock_bitmap;
	}

	disk_sb->nr_blocks = sbi->nr_blocks;
	disk_sb->nr_inodes = sbi->nr_inodes;
	disk_sb->nr_istore_blocks = sbi->nr_istore_blocks;
	disk_sb->nr_ifree_blocks = sbi->nr_ifree_blocks;
	disk_sb->nr_bfree_blocks = sbi->nr_bfree_blocks;
	disk_sb->nr_free_inodes = sbi->nr_free_inodes;
	disk_sb->nr_free_blocks = sbi->nr_free_blocks;

	if (handle)
		ret = simplefs_journal_dirty_metadata(handle, sb, 0, disk_sb,
						      folio);
	else
		folio_mark_dirty(folio);
	folio_release_kmap(folio, disk_sb);
	if (ret)
		goto out_unlock_bitmap;

	pr_debug("[simplefs] sync_fs: nr_free_inodes=%u, nr_free_blocks=%u\n",
		sbi->nr_free_inodes, sbi->nr_free_blocks);

	/* Flush free inodes bitmask */
	for (i = 0; i < sbi->nr_ifree_blocks; i++) {
		int idx = sbi->nr_istore_blocks + i + 1;
		void *data;

		data = simplefs_get_folio(sb, idx, &folio);
		if (IS_ERR(data)) {
			ret = PTR_ERR(data);
			goto out_unlock_bitmap;
		}

		memcpy(data,
		       (void *)sbi->ifree_bitmap + i * SIMPLEFS_BLOCK_SIZE,
		       SIMPLEFS_BLOCK_SIZE);

		if (handle)
			ret = simplefs_journal_dirty_metadata(handle, sb, idx,
							      data, folio);
		else
			folio_mark_dirty(folio);
		folio_release_kmap(folio, data);
		if (ret)
			goto out_unlock_bitmap;
	}

	/* Flush free blocks bitmask */
	for (i = 0; i < sbi->nr_bfree_blocks; i++) {
		int idx = sbi->nr_istore_blocks + sbi->nr_ifree_blocks + i + 1;
		void *data;

		data = simplefs_get_folio(sb, idx, &folio);
		if (IS_ERR(data)) {
			ret = PTR_ERR(data);
			goto out_unlock_bitmap;
		}

		memcpy(data,
		       (void *)sbi->bfree_bitmap + i * SIMPLEFS_BLOCK_SIZE,
		       SIMPLEFS_BLOCK_SIZE);

		if (handle)
			ret = simplefs_journal_dirty_metadata(handle, sb, idx,
							      data, folio);
		else
			folio_mark_dirty(folio);
		folio_release_kmap(folio, data);
		if (ret)
			goto out_unlock_bitmap;
	}
	mutex_unlock(&sbi->bitmap_lock);

	if (handle) {
		ret = simplefs_journal_stop(handle);
		handle = NULL;
		if (ret)
			return ret;
	}

	/* If synchronous write is requested, actually perform writeback */
	if (wait) {
		/* Flush all dirty inodes */
		sync_inodes_sb(sb);

		/* With JBD2, durability is the commit record.  Do not write the
		 * metadata bdev mapping directly ahead of that record. */
		if (sbi->s_journal && sbi->s_journal->j_jbd2)
			return simplefs_journal_force_commit(sbi->s_journal);

		/* Sync block device */
		ret = sync_blockdev(sb->s_bdev);
		if (ret)
			return ret;
		
		/* Sync page cache */
		ret = filemap_write_and_wait_range(sb->s_bdev->bd_mapping, 0, LLONG_MAX);
		if (ret)
			return ret;
	}

	return 0;

out_unlock_bitmap:
	mutex_unlock(&sbi->bitmap_lock);
	if (handle) {
		simplefs_journal_abort(handle, ret);
		simplefs_journal_stop(handle);
	}
	return ret;
}

static int simplefs_statfs(struct dentry *dentry, struct kstatfs *stat)
{
	struct super_block *sb = dentry->d_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);

	stat->f_type = SIMPLEFS_MAGIC;
	stat->f_bsize = SIMPLEFS_BLOCK_SIZE;
	stat->f_blocks = sbi->nr_blocks;
	stat->f_bfree = sbi->nr_free_blocks;
	stat->f_bavail = sbi->nr_free_blocks;
	stat->f_files = sbi->nr_inodes;
	stat->f_ffree = sbi->nr_free_inodes;
	stat->f_namelen = SIMPLEFS_FILENAME_LEN;

	return 0;
}

int simplefs_issue_pending_discards(struct super_block *sb)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	unsigned int granularity;
	unsigned long gran_blocks;
	unsigned long block = 0;
	int ret = 0;

	if (!sbi->s_discard || !sbi->discard_pending)
		return 0;
	granularity = max_t(unsigned int,
			    bdev_discard_granularity(sb->s_bdev),
			    sb->s_blocksize);
	gran_blocks = DIV_ROUND_UP(granularity, sb->s_blocksize);

	/* The JBD2 adapter calls this only after the freeing transaction's
	 * commit record is durable and while its serialized handle mutex still
	 * excludes the next metadata transaction. */
	mutex_lock(&sbi->bitmap_lock);
	while (block < sbi->nr_blocks) {
		unsigned long end;
		unsigned long discard_start;
		unsigned long discard_end;

		block = find_next_bit(sbi->discard_pending, sbi->nr_blocks,
				      block);
		if (block >= sbi->nr_blocks)
			break;
		if (!test_bit(block, sbi->bfree_bitmap)) {
			__clear_bit(block, sbi->discard_pending);
			block++;
			continue;
		}

		end = block + 1;
		while (end < sbi->nr_blocks &&
		       test_bit(end, sbi->discard_pending) &&
		       test_bit(end, sbi->bfree_bitmap))
			end++;

		/* Thin-provisioned devices may deallocate at their advertised
		 * cluster granularity.  Never send a partial edge unit: doing so can
		 * punch neighbouring live SimpleFS blocks on targets with a larger
		 * discard unit than the filesystem block size. */
		discard_start = round_up(block, gran_blocks);
		discard_end = round_down(end, gran_blocks);
		if (discard_start >= discard_end) {
			block = end;
			continue;
		}

		ret = sb_issue_discard(sb, discard_start,
				       discard_end - discard_start, GFP_NOFS, 0);
		if (ret)
			break;
		bitmap_clear(sbi->discard_pending, discard_start,
			     discard_end - discard_start);
		block = end;
	}
	mutex_unlock(&sbi->bitmap_lock);
	return ret;
}

static int simplefs_freeze(struct super_block *sb)
{
	simplefs_sync_fs(sb, 1);
	return 0;
}

static int simplefs_unfreeze(struct super_block *sb)
{
	return 0;
}

static const struct super_operations simplefs_super_ops = {
	.put_super = simplefs_put_super,
	.alloc_inode = simplefs_alloc_inode,
	.free_inode = simplefs_free_inode,
	.evict_inode = simplefs_evict_inode,
	.write_inode = simplefs_write_inode,
	.sync_fs = simplefs_sync_fs,
	.statfs = simplefs_statfs,
	.freeze_fs = simplefs_freeze,
	.unfreeze_fs = simplefs_unfreeze,
};

/* NFS export support */
static struct inode *simplefs_nfs_get_inode(struct super_block *sb, u64 ino,
					    u32 generation)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct inode *inode;
	bool free;

	if (ino < 1 || ino >= sbi->nr_inodes)
		return ERR_PTR(-ESTALE);

	mutex_lock(&sbi->bitmap_lock);
	free = test_bit(ino, sbi->ifree_bitmap);
	mutex_unlock(&sbi->bitmap_lock);
	if (free) {
		pr_debug("stale file handle references free inode ino=%llu requested_gen=%u\n",
			(unsigned long long)ino, generation);
		return ERR_PTR(-ESTALE);
	}

	inode = simplefs_iget(sb, ino);
	if (IS_ERR(inode))
		return inode;

	if (is_bad_inode(inode) ||
	    (generation && inode->i_generation != generation)) {
		pr_debug("stale file handle ino=%llu requested_gen=%u actual_gen=%u nlink=%u\n",
			(unsigned long long)ino, generation,
			inode->i_generation, inode->i_nlink);
		iput(inode);
		return ERR_PTR(-ESTALE);
	}

	return inode;
}

static struct dentry *simplefs_fh_to_dentry(struct super_block *sb,
					    struct fid *fid, int fh_len,
					    int fh_type)
{
	return generic_fh_to_dentry(sb, fid, fh_len, fh_type,
				    simplefs_nfs_get_inode);
}

static struct dentry *simplefs_fh_to_parent(struct super_block *sb,
					    struct fid *fid, int fh_len,
					    int fh_type)
{
	return generic_fh_to_parent(sb, fid, fh_len, fh_type,
				    simplefs_nfs_get_inode);
}

static struct dentry *simplefs_get_parent(struct dentry *child)
{
	struct inode *dir = d_inode(child);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(dir);
	ino_t parent_ino;

	if (!S_ISDIR(dir->i_mode))
		return ERR_PTR(-ENOTDIR);

	/* Parent inode number is stored in i_data[0..3] for directories */
	parent_ino = *((uint32_t *)ci->i_data);
	pr_debug("get_parent child=%llu parent=%llu mode=0%o generation=%u\n",
		(unsigned long long)dir->i_ino,
		(unsigned long long)parent_ino, dir->i_mode,
		dir->i_generation);
	if (!parent_ino)
		return ERR_PTR(-ENOENT);

	return d_obtain_alias(simplefs_iget(dir->i_sb, parent_ino));
}

static int simplefs_export_get_name(struct dentry *parent, char *name,
				    struct dentry *child)
{
	struct inode *dir = d_inode(parent);
	struct inode *inode = d_inode(child);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(dir);
	struct simplefs_file_ei_block *eblock;
	struct folio *efolio;
	int ei, bi, fi;
	int ret = -ENOENT;

	if (!S_ISDIR(dir->i_mode))
		return -ENOTDIR;

	eblock = simplefs_get_folio(dir->i_sb, ci->ei_block, &efolio);
	if (IS_ERR(eblock))
		return PTR_ERR(eblock);

	for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS; ei++) {
		if (!eblock->extents[ei].ee_start)
			break;
		for (bi = 0; bi < simplefs_ext_len(&eblock->extents[ei]); bi++) {
			struct simplefs_dir_block *dblock;
			struct folio *dfolio;

			dblock = simplefs_get_folio(
				dir->i_sb, eblock->extents[ei].ee_start + bi,
				&dfolio);
			if (IS_ERR(dblock)) {
				ret = PTR_ERR(dblock);
				goto out;
			}

			for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
				size_t len;

				if (dblock->files[fi].inode != inode->i_ino)
					continue;
				len = strnlen(dblock->files[fi].filename,
					      SIMPLEFS_FILENAME_LEN);
				memcpy(name, dblock->files[fi].filename, len);
				name[len] = '\0';
				ret = 0;
				break;
			}
			folio_release_kmap(dfolio, dblock);
			if (!ret)
				goto out;
		}
	}

out:
	folio_release_kmap(efolio, eblock);
	return ret;
}

static const struct export_operations simplefs_export_ops = {
	.encode_fh = generic_encode_ino32_fh,
	.fh_to_dentry = simplefs_fh_to_dentry,
	.fh_to_parent = simplefs_fh_to_parent,
	.get_parent = simplefs_get_parent,
	.get_name = simplefs_export_get_name,
};

/* Fill the struct superblock from partition superblock */
int simplefs_fill_super(struct super_block *sb, struct fs_context *fc)
{
	struct simplefs_sb_info *csb = NULL;
	struct simplefs_sb_info *sbi = NULL;
	struct inode *root_inode = NULL;
	uint32_t actual_free, meta_end;
	int ret = 0, i;
	struct folio *folio;

	pr_debug("[martins3:%s:%d] filling super\n", __func__, __LINE__);

	/* Init sb */
	sb->s_magic = SIMPLEFS_MAGIC;
	sb_set_blocksize(sb, SIMPLEFS_BLOCK_SIZE);
	sb->s_maxbytes = SIMPLEFS_MAX_FILESIZE;
	sb->s_time_gran = 1;
	/* 磁盘时间戳是有符号 32 位秒（读写路径做 int32 符号扩展），
	 * 与 ext2 相同。让 VFS 把 utimens 等请求钳制到格式范围内
	 * （generic/402 的 timestamp bounds 测试）。
	 */
	sb->s_time_min = S32_MIN;
	sb->s_time_max = S32_MAX;
	sb->s_op = &simplefs_super_ops;
	sb->s_export_op = &simplefs_export_ops;
	sb->s_xattr = simplefs_xattr_handlers;
	sb->s_iflags |= SB_I_CGROUPWB;
#ifdef CONFIG_FS_POSIX_ACL
	sb->s_flags |= SB_POSIXACL;
#endif

	/* Read sb from disk */
	csb = (struct simplefs_sb_info *)simplefs_get_folio(
		sb, SIMPLEFS_SB_BLOCK_NR, &folio);
	if (IS_ERR(csb))
		return -EIO;

	/* Check magic number */
	if (csb->magic != sb->s_magic) {
		pr_err("Wrong magic number\n");
		ret = -EINVAL;
		goto release;
	}

	/* Alloc sb_info */
	sbi = kzalloc(sizeof(struct simplefs_sb_info), GFP_KERNEL);
	if (!sbi) {
		ret = -ENOMEM;
		goto release;
	}
	memcpy(sbi, csb, sizeof(struct simplefs_sb_info));
	sb->s_fs_info = sbi;
	folio_release_kmap(folio, csb);
	sbi->sb = sb;
	mutex_init(&sbi->bitmap_lock);

	/* journal 默认启用（s_journal_mode=0）；仅在显式传 nojournal 时
	 * 禁用（fill_super 末尾的 ctx->nojournal 分支会置 1）。xfstests 的
	 * 挂载脚本统一传 nojournal，常规测试仍走 nojournal 路径。
	 */

	/* Alloc and copy ifree_bitmap */
	sbi->ifree_bitmap =
		kzalloc(sbi->nr_ifree_blocks * SIMPLEFS_BLOCK_SIZE, GFP_KERNEL);
	if (!sbi->ifree_bitmap) {
		ret = -ENOMEM;
		goto free_sbi;
	}

	for (i = 0; i < sbi->nr_ifree_blocks; i++) {
		int idx = sbi->nr_istore_blocks + i + 1;
		void *data = simplefs_get_folio(sb, idx, &folio);
		if (IS_ERR(data)) {
			ret = -EIO;
			goto free_ifree;
		}

		memcpy((void *)sbi->ifree_bitmap + i * SIMPLEFS_BLOCK_SIZE,
		       data, SIMPLEFS_BLOCK_SIZE);
		folio_release_kmap(folio, data);
	}
	/* Alloc and copy bfree_bitmap */
	sbi->bfree_bitmap =
		kzalloc(sbi->nr_bfree_blocks * SIMPLEFS_BLOCK_SIZE, GFP_KERNEL);
	if (!sbi->bfree_bitmap) {
		ret = -ENOMEM;
		goto free_ifree;
	}
	sbi->discard_pending = bitmap_zalloc(sbi->nr_blocks, GFP_KERNEL);
	if (!sbi->discard_pending) {
		ret = -ENOMEM;
		goto free_bfree;
	}

	for (i = 0; i < sbi->nr_bfree_blocks; i++) {
		int idx = sbi->nr_istore_blocks + sbi->nr_ifree_blocks + i + 1;
		void *data = simplefs_get_folio(sb, idx, &folio);
		if (IS_ERR(data)) {
			ret = -EIO;
			goto free_discard;
		}

		memcpy((void *)sbi->bfree_bitmap + i * SIMPLEFS_BLOCK_SIZE,
		       data, SIMPLEFS_BLOCK_SIZE);

		folio_release_kmap(folio, data);
	}

	meta_end = 1 + sbi->nr_istore_blocks + sbi->nr_ifree_blocks +
		   sbi->nr_bfree_blocks;
	if (sbi->s_journal_present) {
		if (sbi->s_journal_start <= meta_end ||
		    sbi->s_journal_start >= sbi->nr_blocks) {
			pr_err("Invalid journal range start=%u blocks=%u\n",
			       sbi->s_journal_start, sbi->nr_blocks);
			ret = -EFSCORRUPTED;
			goto free_discard;
		}
		bitmap_clear(sbi->bfree_bitmap, sbi->s_journal_start,
			     sbi->nr_blocks - sbi->s_journal_start);
	}
	/* Apply mount options from fs_context */
	if (fc->fs_private) {
		struct simplefs_fs_context *ctx = fc->fs_private;

		if (ctx->norecovery && !sb_rdonly(sb)) {
			pr_err("norecovery requires a read-only mount\n");
			ret = -EINVAL;
			goto free_discard;
		}
		if (ctx->nojournal)
			sbi->s_journal_mode = 1;
		else if (ctx->norecovery)
			sbi->s_journal_mode = 2;
		sbi->s_discard = ctx->discard;
	}

	/* Load journal and perform recovery if needed */
	ret = simplefs_journal_load(sb);
	if (ret) {
		pr_err("Failed to load journal: %d\n", ret);
		goto free_discard;
	}

	/* JBD2 recovery updates the on-disk bitmap blocks, but the first copies
	 * above were read before replay.  Reload them before any inode allocation
	 * or eviction can observe stale free state. */
	for (i = 0; i < sbi->nr_ifree_blocks; i++) {
		int idx = sbi->nr_istore_blocks + i + 1;
		void *data = simplefs_get_folio(sb, idx, &folio);

		if (IS_ERR(data)) {
			ret = PTR_ERR(data);
			goto destroy_journal;
		}
		memcpy((void *)sbi->ifree_bitmap + i * SIMPLEFS_BLOCK_SIZE,
		       data, SIMPLEFS_BLOCK_SIZE);
		folio_release_kmap(folio, data);
	}
	for (i = 0; i < sbi->nr_bfree_blocks; i++) {
		int idx = sbi->nr_istore_blocks + sbi->nr_ifree_blocks + i + 1;
		void *data = simplefs_get_folio(sb, idx, &folio);

		if (IS_ERR(data)) {
			ret = PTR_ERR(data);
			goto destroy_journal;
		}
		memcpy((void *)sbi->bfree_bitmap + i * SIMPLEFS_BLOCK_SIZE,
		       data, SIMPLEFS_BLOCK_SIZE);
		folio_release_kmap(folio, data);
	}
	if (sbi->s_journal_present)
		bitmap_clear(sbi->bfree_bitmap, sbi->s_journal_start,
			     sbi->nr_blocks - sbi->s_journal_start);

	actual_free = bitmap_weight(sbi->bfree_bitmap, sbi->nr_blocks);
	if (actual_free != sbi->nr_free_blocks)
		pr_debug("recovered free-block count disk=%u bitmap=%u\n",
			 sbi->nr_free_blocks, actual_free);
	sbi->nr_free_blocks = actual_free;
	sbi->nr_free_inodes = bitmap_weight(sbi->ifree_bitmap,
					      sbi->nr_inodes);

	/* Root must be instantiated from the post-replay inode-table image. */
	root_inode = simplefs_iget(sb, 1);
	if (IS_ERR(root_inode)) {
		ret = PTR_ERR(root_inode);
		goto destroy_journal;
	}
	inode_init_owner(&nop_mnt_idmap, root_inode, NULL, root_inode->i_mode);

	sb->s_root = d_make_root(root_inode);
	if (!sb->s_root) {
		ret = -ENOMEM;
		goto destroy_journal;
	}

	return 0;

destroy_journal:
	simplefs_journal_destroy(sb);
free_discard:
	bitmap_free(sbi->discard_pending);
free_bfree:
	kfree(sbi->bfree_bitmap);
free_ifree:
	kfree(sbi->ifree_bitmap);
free_sbi:
	sb->s_fs_info = NULL;
	kfree(sbi);
	return ret;
release:
	folio_release_kmap(folio, csb);

	return ret;
}
