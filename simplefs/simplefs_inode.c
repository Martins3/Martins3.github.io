#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/writeback.h>
#include <linux/random.h>
#include <linux/pagemap.h>
#include <linux/iomap.h>
#include <linux/kdev_t.h>
#include <linux/security.h>
#include <linux/posix_acl.h>
#include <linux/fileattr.h>

#include "simplefs_bitmap.h"
#include "simplefs.h"
#include "simplefs_trace.h"
#include "simplefs_journal.h"

static const struct inode_operations simplefs_inode_ops;
static void simplefs_set_inode_flags(struct inode *inode);
static int simplefs_fileattr_get(struct dentry *dentry,
				 struct file_kattr *fa);
static int simplefs_fileattr_set(struct mnt_idmap *idmap,
				 struct dentry *dentry, struct file_kattr *fa);

/* Helper: Check if a directory is empty */
static int simplefs_dir_is_empty(struct inode *inode)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct simplefs_file_ei_block *eblock;
	struct folio *dfolio;
	int ei, bi, fi;

	eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(sb, ci->ei_block, &dfolio);
	if (IS_ERR(eblock))
		return PTR_ERR(eblock);

	/*
	 * nr_files is only a cached count. Under rename/unlink stress it is safer
	 * to scan the actual directory entries before allowing rmdir().
	 */
	for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS; ei++) {
		if (!eblock->extents[ei].ee_start)
			break;

		for (bi = 0; bi < simplefs_ext_len(&eblock->extents[ei]); bi++) {
			struct folio *dblock_folio;
			struct simplefs_dir_block *dblock;

			dblock = (struct simplefs_dir_block *)simplefs_get_folio(
				sb, eblock->extents[ei].ee_start + bi, &dblock_folio);
			if (IS_ERR(dblock)) {
				int ret = PTR_ERR(dblock);
				folio_release_kmap(dfolio, eblock);
				return ret;
			}

			for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
				if (dblock->files[fi].inode != 0) {
					folio_release_kmap(dblock_folio, dblock);
					folio_release_kmap(dfolio, eblock);
					return 0;
				}
			}

			folio_release_kmap(dblock_folio, dblock);
			cond_resched();
		}
	}

	folio_release_kmap(dfolio, eblock);
	return 1;
}

/* Remove a file from parent directory */
static int simplefs_unlink_internal(struct inode *dir, struct dentry *dentry,
				    struct simplefs_handle *handle);

static int simplefs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct simplefs_handle *handle;
	int ret;
	
	/* Start transaction for unlink
	 * Credits estimate:
	 * - parent inode: 1
	 * - parent dir data block: 1
	 * - file inode: 1
	 * - bfree bitmap (if freeing blocks): 1
	 * - ifree bitmap: 1
	 * Total: 5 credits
	 */
	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return PTR_ERR(handle);
	
	ret = simplefs_unlink_internal(dir, dentry, handle);
	if (ret) {
		simplefs_journal_abort(handle, ret);
		simplefs_journal_stop(handle);
		return ret;
	}
	
	return simplefs_journal_stop(handle);
}

/* Helper: Mark folio dirty with journal support (pure iomap version)
 * 
 * 使用新的 journal API，直接传递 block_nr 和 data 指针
 */
static int simplefs_mark_folio_dirty(struct simplefs_handle *handle,
				      struct super_block *sb,
				      unsigned long block_nr,
				      void *folio_data_ptr,
				      struct folio *folio)
{
	int ret;

	/* JBD2 must be the only owner of home-metadata writeback. */
	if (!handle) {
		folio_mark_dirty(folio);
		return 0;
	}
	if (folio->index != block_nr) {
		pr_err("metadata block mismatch requested=%lu folio=%lu\n",
		       block_nr, folio->index);
		return -EFSCORRUPTED;
	}

	/* Mark metadata dirty in journal */
	ret = simplefs_journal_dirty_metadata(handle, sb, block_nr,
					      folio_data_ptr, folio);
	if (ret)
		pr_err("Failed to mark block %lu in journal: %d\n", block_nr, ret);

	return ret;
}

static int simplefs_alloc_dir_extent(struct inode *dir,
				     unsigned long index_block_nr,
				     struct simplefs_file_ei_block *eblock,
				     struct folio *dfolio, int ei,
				     struct simplefs_handle *handle)
{
	struct super_block *sb = dir->i_sb;
	int b;
	int bno;
	int ret;

	if (ei >= SIMPLEFS_MAX_EXTENTS)
		return -ENOSPC;

	bno = get_free_blocks(SIMPLEFS_SB(sb), SIMPLEFS_DIR_BLOCKS_PER_EXTENT);
	if (!bno)
		return -ENOSPC;

	simplefs_ext_init(&eblock->extents[ei],
			  ei ? eblock->extents[ei - 1].ee_block +
				       simplefs_ext_len(&eblock->extents[ei - 1])
			     : 0,
			  SIMPLEFS_DIR_BLOCKS_PER_EXTENT, bno, false);

	for (b = 0; b < SIMPLEFS_DIR_BLOCKS_PER_EXTENT; b++) {
		struct folio *zfolio;
		char *zblock;

		zblock = (char *)simplefs_get_folio(sb, bno + b, &zfolio);
		if (IS_ERR(zblock)) {
			ret = PTR_ERR(zblock);
			goto err_clear_extent;
		}

		memset(zblock, 0, SIMPLEFS_BLOCK_SIZE);
		ret = simplefs_mark_folio_dirty(handle, sb, bno + b, zblock,
						zfolio);
		if (ret) {
			folio_release_kmap(zfolio, zblock);
			goto err_clear_extent;
		}
		folio_release_kmap(zfolio, zblock);
	}

	ret = simplefs_mark_folio_dirty(handle, sb, index_block_nr, eblock,
					dfolio);
	if (ret)
		goto err_clear_extent;

	trace_simplefs_alloc_blocks(sb, bno, SIMPLEFS_DIR_BLOCKS_PER_EXTENT,
				    "simplefs_alloc_dir_extent");
	dir->i_blocks += simplefs_blocks_to_sectors(
		SIMPLEFS_DIR_BLOCKS_PER_EXTENT);
	mark_inode_dirty(dir);
	return 0;

err_clear_extent:
	simplefs_retire_metadata_blocks(sb, bno,
				 SIMPLEFS_DIR_BLOCKS_PER_EXTENT);
	memset(&eblock->extents[ei], 0, sizeof(eblock->extents[ei]));
	return ret;
}

struct simplefs_dir_entry_pos {
	int ei;
	int bi;
	int fi;
};

static int simplefs_initxattrs(struct inode *inode,
			       const struct xattr *xattr_array,
			       void *fs_data)
{
	const struct xattr *xattr;
	int err = 0;

	(void)fs_data;

	for (xattr = xattr_array; xattr->name != NULL; xattr++) {
		err = simplefs_xattr_set(inode, xattr->name, xattr->value,
					 xattr->value_len, 0);
		if (err < 0)
			break;
	}

	return err;
}

static int simplefs_init_security(struct inode *inode, struct inode *dir,
				  const struct qstr *qstr)
{
	return security_inode_init_security(inode, dir, qstr,
					    simplefs_initxattrs, NULL);
}

static void simplefs_release_new_inode(struct inode *inode)
{
	if (!inode)
		return;

	clear_nlink(inode);
	iput(inode);
}

static void simplefs_set_parent_ino(struct inode *inode, struct inode *dir)
{
	if (!S_ISDIR(inode->i_mode))
		return;

	*((uint32_t *)SIMPLEFS_INODE(inode)->i_data) = dir->i_ino;
}

static uint32_t simplefs_dir_pos_block_nr(struct simplefs_file_ei_block *eblock,
					  const struct simplefs_dir_entry_pos *pos)
{
	return eblock->extents[pos->ei].ee_start + pos->bi;
}

static int simplefs_find_dir_entry(struct super_block *sb,
				   struct simplefs_file_ei_block *eblock,
				   const struct qstr *name, uint32_t ino,
				   struct simplefs_dir_entry_pos *pos)
{
	int ei, bi, fi;

	for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS; ei++) {
		if (!eblock->extents[ei].ee_start)
			break;

		for (bi = 0; bi < simplefs_ext_len(&eblock->extents[ei]); bi++) {
			struct folio *dblock_folio;
			struct simplefs_dir_block *dblock;

			dblock = (struct simplefs_dir_block *)simplefs_get_folio(
				sb, eblock->extents[ei].ee_start + bi,
				&dblock_folio);
			if (IS_ERR(dblock))
				return PTR_ERR(dblock);

			for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
				struct simplefs_file *file = &dblock->files[fi];

				if (!file->inode)
					continue;
				if (ino && file->inode != ino)
					continue;
				if (strnlen(file->filename,
					    SIMPLEFS_FILENAME_LEN) != name->len)
					continue;
				if (!strncmp(file->filename, name->name, name->len)) {
					pos->ei = ei;
					pos->bi = bi;
					pos->fi = fi;
					folio_release_kmap(dblock_folio, dblock);
					return 0;
				}
			}

			folio_release_kmap(dblock_folio, dblock);
			cond_resched();
		}
	}

	return -ENOENT;
}

static int simplefs_find_free_dir_entry(struct inode *dir,
					struct simplefs_file_ei_block *eblock,
					struct folio *dfolio,
					struct simplefs_dir_entry_pos *pos,
					struct simplefs_handle *handle)
{
	struct super_block *sb = dir->i_sb;
	struct simplefs_inode_info *ci_dir = SIMPLEFS_INODE(dir);
	int ei, bi, fi;

	if (eblock->nr_files >= SIMPLEFS_MAX_SUBFILES)
		return -EMLINK;

	for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS; ei++) {
		if (!eblock->extents[ei].ee_start)
			break;

		for (bi = 0; bi < simplefs_ext_len(&eblock->extents[ei]); bi++) {
			struct folio *dblock_folio;
			struct simplefs_dir_block *dblock;

			dblock = (struct simplefs_dir_block *)simplefs_get_folio(
				sb, eblock->extents[ei].ee_start + bi,
				&dblock_folio);
			if (IS_ERR(dblock))
				return PTR_ERR(dblock);

			for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
				if (!dblock->files[fi].inode) {
					pos->ei = ei;
					pos->bi = bi;
					pos->fi = fi;
					folio_release_kmap(dblock_folio, dblock);
					return 0;
				}
			}

			folio_release_kmap(dblock_folio, dblock);
			cond_resched();
		}
	}

	if (ei >= SIMPLEFS_MAX_EXTENTS)
		return -ENOSPC;

	if (!eblock->extents[ei].ee_start) {
		int ret = simplefs_alloc_dir_extent(dir, ci_dir->ei_block, eblock,
						    dfolio, ei, handle);
		if (ret)
			return ret;
	}

	pos->ei = ei;
	pos->bi = 0;
	pos->fi = 0;
	return 0;
}

static int simplefs_update_dir_entry(struct super_block *sb,
				     struct simplefs_file_ei_block *eblock,
				     const struct simplefs_dir_entry_pos *pos,
				     struct simplefs_handle *handle,
				     uint32_t ino, const struct qstr *name,
				     bool clear)
{
	struct folio *dblock_folio;
	struct simplefs_dir_block *dblock;
	uint32_t block_nr;
	int ret;

	if (!clear && name && name->len > SIMPLEFS_FILENAME_LEN)
		return -ENAMETOOLONG;

	block_nr = simplefs_dir_pos_block_nr(eblock, pos);
	dblock = (struct simplefs_dir_block *)simplefs_get_folio(sb, block_nr,
						&dblock_folio);
	if (IS_ERR(dblock))
		return PTR_ERR(dblock);

	if (clear) {
		memset(&dblock->files[pos->fi], 0, sizeof(struct simplefs_file));
	} else {
		dblock->files[pos->fi].inode = ino;
		if (name) {
			memset(dblock->files[pos->fi].filename, 0,
			       SIMPLEFS_FILENAME_LEN);
			memcpy(dblock->files[pos->fi].filename, name->name,
			       name->len);
		}
	}

	ret = simplefs_mark_folio_dirty(handle, sb, block_nr, dblock,
					dblock_folio);
	folio_release_kmap(dblock_folio, dblock);
	return ret;
}

static int simplefs_mark_dir_index_dirty(struct inode *dir,
					 struct simplefs_file_ei_block *eblock,
					 struct folio *dfolio,
					 struct simplefs_handle *handle)
{
	return simplefs_mark_folio_dirty(handle, dir->i_sb,
					 SIMPLEFS_INODE(dir)->ei_block,
					 eblock, dfolio);
}

static int simplefs_rename_check_subdir(struct inode *old_inode,
					struct dentry *new_dentry)
{
	struct dentry *p;

	if (!S_ISDIR(old_inode->i_mode))
		return 0;

	for (p = new_dentry->d_parent; !IS_ROOT(p); p = p->d_parent) {
		if (d_inode(p) == old_inode)
			return -EINVAL;
	}

	return 0;
}

static int simplefs_prepare_rename_target(struct inode *old_inode,
					  struct inode *new_inode)
{
	int ret;

	if (!new_inode)
		return 0;

	if (S_ISDIR(new_inode->i_mode)) {
		if (!S_ISDIR(old_inode->i_mode))
			return -EISDIR;

		ret = simplefs_dir_is_empty(new_inode);
		if (ret < 0)
			return ret;
		if (!ret)
			return -ENOTEMPTY;
		return 0;
	}

	if (S_ISDIR(old_inode->i_mode))
		return -ENOTDIR;

	return 0;
}

static int simplefs_iomap_truncate_page_compat(struct inode *inode, loff_t pos)
{
#ifdef IOMAP_IOEND_UNWRITTEN
	return iomap_truncate_page(inode, pos, NULL,
				   &simplefs_write_iomap_ops, NULL, NULL);
#else
	return iomap_truncate_page(inode, pos, NULL,
				   &simplefs_write_iomap_ops);
#endif
}

static void simplefs_touch_rename_dirs(struct inode *old_dir,
				       struct inode *new_dir)
{
	inode_set_mtime_to_ts(old_dir, inode_set_ctime_current(old_dir));
	mark_inode_dirty(old_dir);
	if (old_dir != new_dir) {
		inode_set_mtime_to_ts(new_dir, inode_set_ctime_current(new_dir));
		mark_inode_dirty(new_dir);
	}
}


static int simplefs_unlink_internal(struct inode *dir, struct dentry *dentry,
				    struct simplefs_handle *handle)
{
	struct inode *inode = d_inode(dentry);
	struct super_block *sb = dir->i_sb;
	struct simplefs_inode_info *ci_dir = SIMPLEFS_INODE(dir);
	struct simplefs_file_ei_block *eblock;
	struct folio *dfolio;
	int ei = 0, bi = 0, fi = 0;
	int found = 0;
	int ret = 0;

	pr_debug("unlink_internal: looking for %pd (ino=%llu) in dir ino=%llu\n",
		dentry, (unsigned long long)inode->i_ino,
		(unsigned long long)dir->i_ino);

	/* Read parent directory index */
	eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(sb, ci_dir->ei_block, &dfolio);
	if (IS_ERR(eblock))
		return PTR_ERR(eblock);

	/* Find the file in directory */
	for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS && !found; ei++) {
		if (!eblock->extents[ei].ee_start)
			break;

		for (bi = 0; bi < simplefs_ext_len(&eblock->extents[ei]) && !found; bi++) {
			struct folio *dblock_folio;
			struct simplefs_dir_block *dblock;
			uint32_t block_nr = eblock->extents[ei].ee_start + bi;

			dblock = (struct simplefs_dir_block *)simplefs_get_folio(sb, block_nr, &dblock_folio);
			if (IS_ERR(dblock)) {
				ret = PTR_ERR(dblock);
				goto out;
			}

			for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
				if (dblock->files[fi].inode != 0) {
					pr_debug("  [%d][%d][%d]: ino=%u name='%.*s'\n",
						ei, bi, fi, dblock->files[fi].inode,
						(int)strnlen(dblock->files[fi].filename,
							     SIMPLEFS_FILENAME_LEN),
						dblock->files[fi].filename);
				}
				if (dblock->files[fi].inode == inode->i_ino &&
				    strncmp(dblock->files[fi].filename, dentry->d_name.name,
					    dentry->d_name.len) == 0 &&
				    strnlen(dblock->files[fi].filename,
					    SIMPLEFS_FILENAME_LEN) ==
					    dentry->d_name.len) {
					found = 1;
					break;
				}
			}

			if (found) {
				/* Remove entry by zeroing it out (no shifting) */
				memset(&dblock->files[fi], 0, sizeof(struct simplefs_file));

				/* Mark parent directory data block dirty with journal */
				ret = simplefs_mark_folio_dirty(handle, sb, block_nr,
								dblock, dblock_folio);
				folio_release_kmap(dblock_folio, dblock);
				if (ret)
					goto out;

				eblock->nr_files--;

				/* Mark parent directory index block dirty with journal */
				ret = simplefs_mark_folio_dirty(handle, sb,
								ci_dir->ei_block,
								eblock, dfolio);
				if (ret)
					goto out;
			} else {
				folio_release_kmap(dblock_folio, dblock);
			}
			cond_resched();
		}
	}

	if (!found) {
		ret = -ENOENT;
		goto out;
	}

	/* Update directory times */
	inode_set_mtime_to_ts(dir, inode_set_ctime_current(dir));
	mark_inode_dirty(dir);

	/* Update inode nlink - actual block/inode freeing happens in evict_inode
	 * when all VFS references are dropped (like ext2).
	 * Do NOT call put_inode/truncate_blocks here - the inode is still
	 * in the VFS cache and its number could be prematurely reused.
	 */
	if (S_ISDIR(inode->i_mode)) {
		clear_nlink(inode);
	} else {
		if (inode->i_nlink > 0)
			drop_nlink(inode);
	}
	inode_set_mtime_to_ts(inode, inode_set_ctime_current(inode));
	mark_inode_dirty(inode);

out:
	folio_release_kmap(dfolio, eblock);
	return ret;
}

/* Remove an empty directory */
static int simplefs_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);
	struct simplefs_handle *handle;
	int ret = 0;

	pr_debug("[martins3:%s:%d] rmdir: %pd\n", __func__, __LINE__, dentry);

	/* Check if it's a directory */
	if (!S_ISDIR(inode->i_mode))
		return -ENOTDIR;

	/* Check if directory is empty */
	ret = simplefs_dir_is_empty(inode);
	if (ret < 0)
		return ret;
	if (!ret)
		return -ENOTEMPTY;

	/* Start transaction for rmdir
	 * Credits: similar to unlink but may need extra for parent link update
	 */
	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	/* Use internal unlink to remove the directory entry */
	ret = simplefs_unlink_internal(dir, dentry, handle);
	if (ret)
		goto out_abort;

	/* Update parent link count (for ..) */
	if (dir->i_nlink > 0)
		drop_nlink(dir);
	mark_inode_dirty(dir);

	ret = simplefs_journal_stop(handle);
	return ret;

out_abort:
	simplefs_journal_abort(handle, ret);
	simplefs_journal_stop(handle);
	return ret;
}

/* Get symlink target */
static const char *simplefs_get_link(struct dentry *dentry,
				     struct inode *inode,
				     struct delayed_call *done)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	char *target;
	struct folio *folio;
	char *block;

	/* RCU-walk cannot sleep in simplefs_get_folio()/kmemdup_nul(). */
	if (!dentry)
		return ERR_PTR(-ECHILD);

	if (inode->i_size < sizeof(ci->i_data)) {
		if (!ci->i_data[0])
			return ERR_PTR(-EINVAL);
		return ci->i_data;
	}

	if (!ci->ei_block)
		return ERR_PTR(-EINVAL);

	block = (char *)simplefs_get_folio(inode->i_sb, ci->ei_block, &folio);
	if (IS_ERR(block))
		return ERR_CAST(block);

	target = kmemdup_nul(block, inode->i_size, GFP_KERNEL);
	folio_release_kmap(folio, block);
	if (!target)
		return ERR_PTR(-ENOMEM);

	set_delayed_call(done, kfree_link, target);
	return target;
}

const struct address_space_operations *simplefs_aops = &simplefs_iomap_aops;
const struct file_operations *simplefs_file_ops = &simple_fs_iomap_fops;

/* Symlink inode operations */
static const struct inode_operations symlink_inode_ops = {
	.get_link = simplefs_get_link,
	.listxattr = simplefs_listxattr,
	.fileattr_get = simplefs_fileattr_get,
	.fileattr_set = simplefs_fileattr_set,
};

/* Get inode ino from disk */
struct inode *simplefs_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode = NULL;
	struct simplefs_inode *cinode = NULL;
	struct simplefs_inode_info *ci = NULL;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct folio *efolio;
	uint32_t inode_block = (ino / SIMPLEFS_INODES_PER_BLOCK) + 1;
	uint32_t inode_shift = ino % SIMPLEFS_INODES_PER_BLOCK;
	int ret;

	/* Fail if ino is out of range */
	if (ino >= sbi->nr_inodes)
		return ERR_PTR(-EINVAL);

	/* Get a locked inode from Linux */
	inode = iget_locked(sb, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	/* If inode is in cache, return it */
	spin_lock(&inode->i_lock);
#ifdef IOMAP_IOEND_UNWRITTEN
	if (!(inode->i_state.__state & I_NEW)) {
#else
	if (!(inode->i_state & I_NEW)) {
#endif
		spin_unlock(&inode->i_lock);
		return inode;
	}
	spin_unlock(&inode->i_lock);

	ci = SIMPLEFS_INODE(inode);
	/* Read inode from disk and initialize */
	cinode = (struct simplefs_inode *)simplefs_get_folio(sb, inode_block,
							     &efolio);
	if (IS_ERR(cinode)) {
		ret = PTR_ERR(cinode);
		goto failed;
	}
	cinode += inode_shift;

	inode->i_ino = ino;
	inode->i_sb = sb;
	inode->i_op = &simplefs_inode_ops;

	inode->i_mode = le32_to_cpu(cinode->i_mode);
	i_uid_write(inode, le32_to_cpu(cinode->i_uid));
	i_gid_write(inode, le32_to_cpu(cinode->i_gid));
	i_size_write(inode, le32_to_cpu(cinode->i_size));

	inode_set_ctime(inode,
			(time64_t)(int32_t)le32_to_cpu(cinode->i_ctime),
			le32_to_cpu(cinode->i_ctime_nsec));

	inode->i_atime_sec = (time64_t)(int32_t)le32_to_cpu(cinode->i_atime);
	inode->i_atime_nsec = le32_to_cpu(cinode->i_atime_nsec);
	inode->i_mtime_sec = (time64_t)(int32_t)le32_to_cpu(cinode->i_mtime);
	inode->i_mtime_nsec = le32_to_cpu(cinode->i_mtime_nsec);
	inode->i_blocks = simplefs_blocks_to_sectors(
		le32_to_cpu(cinode->i_blocks));
	set_nlink(inode, le32_to_cpu(cinode->i_nlink));
	inode->i_generation = le32_to_cpu(cinode->i_generation);
	inode->i_rdev = 0;
	ci->i_xattr_block = le32_to_cpu(cinode->i_xattr_block);
	ci->i_flags = le32_to_cpu(cinode->i_flags);
	ci->i_crtime.tv_sec = (time64_t)(int32_t)le32_to_cpu(cinode->i_crtime);
	ci->i_crtime.tv_nsec = le32_to_cpu(cinode->i_crtime_nsec);
	ci->prealloc_block = 0;
	ci->prealloc_len = 0;
	simplefs_set_inode_flags(inode);

	if (S_ISDIR(inode->i_mode)) {
		ci->ei_block = le32_to_cpu(cinode->ei_block);
		memcpy(ci->i_data, cinode->i_data, sizeof(ci->i_data));
		inode->i_fop = &simplefs_dir_ops;
		/* Directory inodes need mapping a_ops for page cache operations */
		inode->i_mapping->a_ops = simplefs_aops;
	} else if (S_ISREG(inode->i_mode)) {
		ci->ei_block = le32_to_cpu(cinode->ei_block);
		inode->i_fop = simplefs_file_ops;
		inode->i_mapping->a_ops = simplefs_aops;
	} else if (S_ISLNK(inode->i_mode)) {
		ci->ei_block = le32_to_cpu(cinode->ei_block);
		memcpy(ci->i_data, cinode->i_data, sizeof(ci->i_data));
		if (inode->i_size < sizeof(ci->i_data)) {
			ci->i_data[inode->i_size] = '\0';
			inode->i_link = ci->i_data;
		} else {
			inode->i_link = NULL;
		}
		inode->i_op = &symlink_inode_ops;
	} else if (inode->i_mode != 0) {
		/* Special files: char/block devices, fifos, sockets */
		dev_t rdev = 0;
		if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode))
			rdev = old_decode_dev(le32_to_cpu(*((__le32 *)cinode->i_data)));
		init_special_inode(inode, inode->i_mode, rdev);
	}
	/* mode == 0: uninitialized inode (newly allocated), caller will set mode */

	folio_release_kmap(efolio, cinode - inode_shift);

	/* Unlock the inode to make it usable */
	unlock_new_inode(inode);

	return inode;

failed:
	if (!IS_ERR(cinode))
		folio_release_kmap(efolio, cinode - inode_shift);
	iget_failed(inode);
	return ERR_PTR(ret);
}

/* Searches for a dentry in dir.
 * Fills dentry with NULL if not found in dir, or with the corresponding inode
 * if found.
 * Returns NULL on success, indicating the dentry was successfully filled or
 * confirmed absent.
 */
static struct dentry *simplefs_lookup(struct inode *dir, struct dentry *dentry,
				      unsigned int flags)
{
	struct super_block *sb = dir->i_sb;
	struct simplefs_inode_info *ci_dir = SIMPLEFS_INODE(dir);
	struct inode *inode = NULL;
	struct folio *dfolio, *dfolio2;
	struct simplefs_file_ei_block *eblock = NULL;
	struct simplefs_dir_block *dblock = NULL;
	struct simplefs_file *f = NULL;
	int ei, bi, fi;

	pr_debug("[martins3:%s:%d] dentry name: %pd\n", __func__, __LINE__,
		dentry);

	/* Check filename length */
	if (dentry->d_name.len > SIMPLEFS_FILENAME_LEN)
		return ERR_PTR(-ENAMETOOLONG);

	/* Read the directory block on disk */
	eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
		sb, ci_dir->ei_block, &dfolio);
	if (IS_ERR(eblock))
		return ERR_CAST(eblock);

	/* Search for the file in directory */
	for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS; ei++) {
		if (!eblock->extents[ei].ee_start)
			break;

		/* Iterate blocks in extent */
		for (bi = 0; bi < simplefs_ext_len(&eblock->extents[ei]); bi++) {
			dblock =
				(struct simplefs_dir_block *)simplefs_get_folio(
					sb, eblock->extents[ei].ee_start + bi,
					&dfolio2);
			if (IS_ERR(dblock)) {
				folio_release_kmap(dfolio, eblock);
				return ERR_CAST(dblock);
			}

			/* Search file in ei_block (skip holes from deleted entries) */
			for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
				f = &dblock->files[fi];
				if (!f->inode)
					continue;
				trace_simplefs_lookup(dir, dentry->d_name.name, inode);
				if (strnlen(f->filename, SIMPLEFS_FILENAME_LEN) ==
					    dentry->d_name.len &&
				    !strncmp(f->filename, dentry->d_name.name,
					     dentry->d_name.len)) {
					inode = simplefs_iget(sb, f->inode);
					folio_release_kmap(dfolio2, dblock);
					if (IS_ERR(inode)) {
						folio_release_kmap(dfolio, eblock);
						return ERR_CAST(inode);
					}
					goto search_end;
				}
			}
			folio_release_kmap(dfolio2, dblock);
			cond_resched();
		}
	}

search_end:
	folio_release_kmap(dfolio, eblock);

	/* Reconnect an existing disconnected directory alias for exportfs. */
	return d_splice_alias(inode, dentry);
}

/* Create a new inode in dir */
static struct inode *simplefs_new_inode(struct mnt_idmap *idmap,
					struct inode *dir, mode_t mode)
{
	struct inode *inode;
	struct simplefs_inode_info *ci;
	struct super_block *sb;
	struct simplefs_sb_info *sbi;
	struct folio *metadata_folio;
	void *metadata;
	uint32_t ino, bno;
	int ret;

	struct timespec64 cur_time;

	/* Check mode before doing anything to avoid undoing everything */
	if (!S_ISDIR(mode) && !S_ISREG(mode) && !S_ISLNK(mode) &&
	    !S_ISCHR(mode) && !S_ISBLK(mode) && !S_ISFIFO(mode) &&
	    !S_ISSOCK(mode)) {
		pr_err("File type not supported\n");
		return ERR_PTR(-EINVAL);
	}

	/* Check if inodes are available */
	sb = dir->i_sb;
	sbi = SIMPLEFS_SB(sb);
	if (sbi->nr_free_inodes == 0)
		return ERR_PTR(-ENOSPC);

	/* Get a new free inode */
	ino = get_free_inode(sbi);
	if (!ino)
		return ERR_PTR(-ENOSPC);

	/*
	 * Allocate a fresh VFS inode instead of reading the free on-disk slot
	 * through simplefs_iget().  The disk slot deliberately retains the old
	 * inode contents until this allocation is committed.  Interpreting those
	 * contents first can therefore initialize VFS-only union members such as
	 * i_link/i_cdev for the previous file type.
	 */
	inode = new_inode(sb);
	if (!inode) {
		ret = -ENOMEM;
		goto put_ino;
	}
	inode->i_ino = ino;
	if (insert_inode_locked(inode)) {
		ret = -EIO;
		make_bad_inode(inode);
		iput(inode);
		goto put_ino;
	}
	/*
	 * Make it a safe unlinked object before any allocation that can fail.
	 * discard_new_inode() will then run eviction exactly once without
	 * following uninitialized private block pointers.
	 */
	ci = SIMPLEFS_INODE(inode);
	inode->i_mode = 0;
	inode->i_op = &simplefs_inode_ops;
	clear_nlink(inode);
	i_size_write(inode, 0);
	inode->i_blocks = 0;
	inode->i_rdev = 0;
	inode->i_generation = get_random_u32();
	if (!inode->i_generation)
		inode->i_generation = 1;
	ci->ei_block = 0;
	ci->i_xattr_block = 0;
	ci->i_flags = 0;
	ci->prealloc_block = 0;
	ci->prealloc_len = 0;
	memset(ci->i_data, 0, sizeof(ci->i_data));

	if (S_ISLNK(mode)) {
		inode_init_owner(idmap, inode, dir, mode);
		set_nlink(inode, 1);
		inode->i_blocks = 0;
		cur_time = current_time(inode);
		inode->i_atime_sec = inode->i_mtime_sec = cur_time.tv_sec;
		inode->i_atime_nsec = inode->i_mtime_nsec = cur_time.tv_nsec;
		inode_set_ctime_to_ts(inode, cur_time);
		ci->i_crtime = cur_time;
		inode->i_op = &symlink_inode_ops;
		ret = simplefs_init_acl(inode, dir);
		if (ret)
			goto discard_inode;
		unlock_new_inode(inode);
		return inode;
	}

	/* Special files (char/block devices, fifos, sockets) don't need data blocks */
	if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)) {
		inode_init_owner(idmap, inode, dir, mode);
		set_nlink(inode, 1);
		inode->i_blocks = 0;
		ci->ei_block = 0;
		memset(ci->i_data, 0, sizeof(ci->i_data));

		cur_time = current_time(inode);
		inode->i_atime_sec = inode->i_mtime_sec = cur_time.tv_sec;
		inode->i_atime_nsec = inode->i_mtime_nsec = cur_time.tv_nsec;
		inode_set_ctime_to_ts(inode, cur_time);
		ci->i_crtime = cur_time;
		ret = simplefs_init_acl(inode, dir);
		if (ret)
			goto discard_inode;
		unlock_new_inode(inode);
		return inode;
	}

	/* Get a free block for this new inode's index */
	bno = get_free_blocks(sbi, 1);
	if (!bno) {
		ret = -ENOSPC;
		goto discard_inode;
	}

	/* 新分配的 ei_block 必须通过块设备页缓存立即初始化。
	 * 不能用 blkdev_issue_zeroout() 绕过页缓存：inode/block 复用时，
	 * simplefs_get_folio() 可能继续命中上一任 inode 留下的旧 extent
	 * root。O_TMPFILE 或创建失败后的 evict 随后会按旧树重复释放块。
	 */
	metadata = simplefs_get_folio(sb, bno, &metadata_folio);
	if (IS_ERR(metadata)) {
		ret = PTR_ERR(metadata);
		put_blocks(sbi, bno, 1);
		goto discard_inode;
	}
	if (S_ISREG(mode))
		simplefs_file_init_extent_root(metadata);
	else
		memset(metadata, 0, SIMPLEFS_BLOCK_SIZE);
	ret = simplefs_journal_dirty_folio(sb, bno, metadata,
					   metadata_folio);
	if (ret) {
		folio_release_kmap(metadata_folio, metadata);
		put_blocks(sbi, bno, 1);
		goto discard_inode;
	}
	folio_release_kmap(metadata_folio, metadata);

	/* Initialize inode */
	inode_init_owner(idmap, inode, dir, mode);
	inode->i_blocks = simplefs_blocks_to_sectors(1);
	if (S_ISDIR(mode)) {
		ci->ei_block = bno;
		inode->i_size = SIMPLEFS_BLOCK_SIZE;
		inode->i_fop = &simplefs_dir_ops;
		set_nlink(inode, 2); /* . and .. */
	} else if (S_ISREG(mode)) {
		ci->ei_block = bno;
		inode->i_size = 0;
		inode->i_fop = simplefs_file_ops;
		inode->i_mapping->a_ops = simplefs_aops;
		set_nlink(inode, 1);
	}
	pr_debug("new inode ino=%llu mode=0%o root=%u\n",
		 (unsigned long long)inode->i_ino, inode->i_mode, ci->ei_block);

	cur_time = current_time(inode);
	inode->i_atime_sec = inode->i_mtime_sec = cur_time.tv_sec;
	inode->i_atime_nsec = inode->i_mtime_nsec = cur_time.tv_nsec;
	inode_set_ctime_to_ts(inode, cur_time);
	ci->i_crtime = cur_time;

	trace_simplefs_create_inode(inode, mode);

	ret = simplefs_init_acl(inode, dir);
	if (ret)
		goto discard_inode;

	unlock_new_inode(inode);
	return inode;

discard_inode:
	clear_nlink(inode);
	discard_new_inode(inode);
	return ERR_PTR(ret);

put_ino:
	put_inode(sbi, ino);

	return ERR_PTR(ret);
}

/* Create a file or directory in this way:
 *   - check filename length and if the parent directory is not full
 *   - create the new inode (allocate inode and blocks)
 *   - cleanup index block of the new inode
 *   - add new file/directory in parent index
 */
static struct dentry *simplefs_create_internal(struct mnt_idmap *id,
					       struct inode *dir,
					       struct dentry *dentry,
					       umode_t mode, bool excl,
					       struct simplefs_handle *handle)
{
	struct super_block *sb;
	struct inode *inode;
	struct simplefs_inode_info *ci_dir;
	struct simplefs_file_ei_block *eblock;
	struct simplefs_dir_block *dblock;
	char *fblock;
	struct folio *dfolio, *dfolio2, *dblock_folio;
	struct timespec64 cur_time;
	uint32_t dir_block_nr = 0;

	int ret = 0, alloc = false;
	int ei = 0, bi = 0, fi = 0;

	/* Check filename length */
	if (dentry->d_name.len > SIMPLEFS_FILENAME_LEN) {
		ret = -ENAMETOOLONG;
		goto out;
	}

	/* Read parent directory index */
	ci_dir = SIMPLEFS_INODE(dir);
	sb = dir->i_sb;
	eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
		sb, ci_dir->ei_block, &dfolio);
	if (IS_ERR(eblock)) {
		ret = PTR_ERR(eblock);
		goto out;
	}
	/* Check if parent directory is full */
	if (eblock->nr_files >= SIMPLEFS_MAX_SUBFILES) {
		folio_release_kmap(dfolio, eblock);
		ret = -EMLINK;
		goto out;
	}

	/* Get a new free inode */
	inode = simplefs_new_inode(id, dir, mode);
	if (IS_ERR(inode)) {
		ret = PTR_ERR(inode);
		goto end;
	}

	/* TODO: Mark inode block in journal if handle provided */
	
	/* 新分配的 metadata block 需要按 inode 类型初始化。
	 * 目录仍然使用旧的单块目录索引；regular file 改成 extent tree root。
	 */
	fblock = (char *)simplefs_get_folio(sb, SIMPLEFS_INODE(inode)->ei_block,
					    &dfolio2);
	if (IS_ERR(fblock)) {
		ret = PTR_ERR(fblock);
		goto iput;
	}
	if (S_ISREG(inode->i_mode))
		simplefs_file_init_extent_root(fblock);
	else
		memset(fblock, 0, SIMPLEFS_BLOCK_SIZE);
	
	/* Mark new inode's ei_block dirty with journal */
	ret = simplefs_mark_folio_dirty(handle, sb, SIMPLEFS_INODE(inode)->ei_block, fblock, dfolio2);
	if (ret) {
		folio_release_kmap(dfolio2, fblock);
		goto iput;
	}
	
	folio_release_kmap(dfolio2, fblock);

	ret = simplefs_init_security(inode, dir, &dentry->d_name);
	if (ret)
		goto iput;

	/* Find first free slot by scanning all existing blocks for holes */
	{
		int slot_found = 0;

		for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS && !slot_found; ei++) {
			if (!eblock->extents[ei].ee_start)
				break;
			for (bi = 0; bi < simplefs_ext_len(&eblock->extents[ei]) && !slot_found; bi++) {
				dir_block_nr = eblock->extents[ei].ee_start + bi;
				dblock = (struct simplefs_dir_block *)simplefs_get_folio(
					sb, dir_block_nr, &dblock_folio);
				if (IS_ERR(dblock)) {
					ret = PTR_ERR(dblock);
					goto iput;
				}
				for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
					if (dblock->files[fi].inode == 0) {
						slot_found = 1;
						break;
					}
				}
				if (!slot_found)
					folio_release_kmap(dblock_folio, dblock);
			}
		}

		if (!slot_found) {
			/* All existing blocks are full, allocate new extent.
			 * ei already points to the first free extent slot
			 * from the scanning loop above.
			 */
			bi = 0;
			fi = 0;

			if (!eblock->extents[ei].ee_start) {
				ret = simplefs_alloc_dir_extent(dir, ci_dir->ei_block,
								eblock, dfolio, ei,
								handle);
				if (ret)
					goto iput;
				alloc = true;
			}
			dir_block_nr = eblock->extents[ei].ee_start + bi;
			dblock = (struct simplefs_dir_block *)simplefs_get_folio(
				sb, dir_block_nr, &dblock_folio);
			if (IS_ERR(dblock)) {
				ret = PTR_ERR(dblock);
				goto put_block;
			}
		}
	}

	dblock->files[fi].inode = inode->i_ino;
	memset(dblock->files[fi].filename, 0, SIMPLEFS_FILENAME_LEN);
	memcpy(dblock->files[fi].filename, dentry->d_name.name,
	       dentry->d_name.len);

	/* Mark parent directory data block dirty with journal */
	ret = simplefs_mark_folio_dirty(handle, sb, dir_block_nr, dblock,
					dblock_folio);
	folio_release_kmap(dblock_folio, dblock);
	if (ret) {
		goto put_block;
	}

	eblock->nr_files++;

	/* Mark parent directory index block (eblock) dirty with journal */
	ret = simplefs_mark_folio_dirty(handle, sb, ci_dir->ei_block, eblock, dfolio);
	if (ret)
		goto put_block;

	// dfolio is released at the function's end

	if (S_ISDIR(mode)) {
		/* Store parent inode number for NFS export get_parent */
		struct simplefs_inode_info *ci_new = SIMPLEFS_INODE(inode);
		*((uint32_t *)ci_new->i_data) = dir->i_ino;
		inc_nlink(dir);
	}

	/* Persist i_data only after a new directory's parent is recorded. */
	mark_inode_dirty(inode);
	simplefs_persist_inode(inode);

	cur_time = current_time(dir);
	dir->i_mtime_sec = cur_time.tv_sec;
	dir->i_mtime_nsec = cur_time.tv_nsec;
	inode_set_ctime_to_ts(dir, cur_time);

	mark_inode_dirty(dir);
	simplefs_persist_inode(dir);

	d_instantiate(dentry, inode);

	/* Release parent directory index block before returning */
	folio_release_kmap(dfolio, eblock);
	/* Return NULL on success for mkdir/create operations */
	return NULL;

put_block:
	if (alloc && eblock->extents[ei].ee_start) {
		simplefs_retire_metadata_blocks(
			sb, eblock->extents[ei].ee_start,
			simplefs_ext_len(&eblock->extents[ei]));
		memset(&eblock->extents[ei], 0, sizeof(struct simplefs_extent));
		dir->i_blocks -= simplefs_blocks_to_sectors(
			SIMPLEFS_DIR_BLOCKS_PER_EXTENT);
	}
iput:
	simplefs_release_new_inode(inode);
end:
	if (!IS_ERR(eblock))
		folio_release_kmap(dfolio, eblock);
out:
	return ERR_PTR(ret);
}

// commit 88d5baf69082 ("Change inode_operations.mkdir to return struct dentry *")
static int simplefs_create(struct mnt_idmap *id, struct inode *dir,
			   struct dentry *dentry, umode_t mode, bool excl)
{
	struct simplefs_handle *handle;
	struct dentry *de;
	int error;
	
	/* Start transaction: credits for parent dir, new inode, bitmaps
	 * Credits estimate:
	 * - parent inode: 1
	 * - parent dir data block: 1
	 * - new inode: 1
	 * - ifree bitmap: 1
	 * - new inode ei_block: 1
	 * - bfree bitmap (if new extent): 1
	 * Total: 5-6 credits
	 */
	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return PTR_ERR(handle);
	
	de = simplefs_create_internal(id, dir, dentry, mode, excl, handle);
	error = PTR_ERR(de);
	if (IS_ERR(de)) {
		simplefs_journal_abort(handle, error);
		simplefs_journal_stop(handle);
		return error;
	}
	
	/* Commit transaction */
	error = simplefs_journal_stop(handle);
	return error;
}

#ifdef IOMAP_IOEND_UNWRITTEN
static struct dentry *simplefs_mkdir(struct mnt_idmap *id, struct inode *dir,
				     struct dentry *dentry, umode_t mode)
{
	struct simplefs_handle *handle;
	struct dentry *de;

	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return ERR_CAST(handle);

	de = simplefs_create_internal(id, dir, dentry, mode | S_IFDIR, 0, handle);
	if (IS_ERR(de)) {
		simplefs_journal_abort(handle, PTR_ERR(de));
		simplefs_journal_stop(handle);
		return de;
	}

	if (simplefs_journal_stop(handle))
		return ERR_PTR(-EIO);

	return de;
}
#else
static int simplefs_mkdir(struct mnt_idmap *id, struct inode *dir,
			  struct dentry *dentry, umode_t mode)
{
	struct simplefs_handle *handle;
	struct dentry *de;
	int error;

	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	de = simplefs_create_internal(id, dir, dentry, mode | S_IFDIR, 0, handle);
	if (IS_ERR(de)) {
		error = PTR_ERR(de);
		simplefs_journal_abort(handle, error);
		simplefs_journal_stop(handle);
		return error;
	}

	error = simplefs_journal_stop(handle);
	if (error)
		return error;

	return 0;
}
#endif


static int simplefs_rename_normal(struct inode *old_dir,
				  struct dentry *old_dentry,
				  struct inode *new_dir,
				  struct dentry *new_dentry,
				  unsigned int flags,
				  struct simplefs_handle *handle)
{
	struct super_block *sb = old_dir->i_sb;
	struct inode *old_inode = d_inode(old_dentry);
	struct inode *new_inode = d_inode(new_dentry);
	struct simplefs_file_ei_block *old_eblock, *new_eblock;
	struct folio *old_dfolio, *new_dfolio;
	struct simplefs_dir_entry_pos old_pos, new_pos;
	int ret;

	if ((flags & RENAME_NOREPLACE) && new_inode)
		return -EEXIST;

	old_eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
		sb, SIMPLEFS_INODE(old_dir)->ei_block, &old_dfolio);
	if (IS_ERR(old_eblock))
		return PTR_ERR(old_eblock);

	ret = simplefs_find_dir_entry(sb, old_eblock, &old_dentry->d_name,
				      old_inode->i_ino, &old_pos);
	if (ret)
		goto out_old;

	if (old_dir == new_dir && !new_inode) {
		ret = simplefs_update_dir_entry(sb, old_eblock, &old_pos, handle,
						old_inode->i_ino,
						&new_dentry->d_name, false);
		if (ret)
			goto out_old;

		inode_set_ctime_current(old_inode);
		mark_inode_dirty(old_inode);
		simplefs_touch_rename_dirs(old_dir, new_dir);
		goto out_old;
	}

	ret = simplefs_rename_check_subdir(old_inode, new_dentry);
	if (ret)
		goto out_old;

	ret = simplefs_prepare_rename_target(old_inode, new_inode);
	if (ret)
		goto out_old;

	if (new_inode) {
		ret = simplefs_unlink_internal(new_dir, new_dentry, handle);
		if (ret)
			goto out_old;
		if (S_ISDIR(new_inode->i_mode) && new_dir->i_nlink > 0)
			drop_nlink(new_dir);
	}

	if (old_dir == new_dir) {
		new_eblock = old_eblock;
		new_dfolio = old_dfolio;
	} else {
		new_eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
			sb, SIMPLEFS_INODE(new_dir)->ei_block, &new_dfolio);
		if (IS_ERR(new_eblock)) {
			ret = PTR_ERR(new_eblock);
			goto out_old;
		}
	}

	ret = simplefs_find_free_dir_entry(new_dir, new_eblock, new_dfolio,
					   &new_pos, handle);
	if (ret)
		goto out_new;

	ret = simplefs_update_dir_entry(sb, new_eblock, &new_pos, handle,
					old_inode->i_ino, &new_dentry->d_name,
					false);
	if (ret)
		goto out_new;

	new_eblock->nr_files++;
	ret = simplefs_mark_dir_index_dirty(new_dir, new_eblock, new_dfolio,
					    handle);
	if (ret)
		goto out_new;

	ret = simplefs_update_dir_entry(sb, old_eblock, &old_pos, handle, 0,
					NULL, true);
	if (ret)
		goto out_new;

	old_eblock->nr_files--;
	ret = simplefs_mark_dir_index_dirty(old_dir, old_eblock, old_dfolio,
					    handle);
	if (ret)
		goto out_new;

	if (S_ISDIR(old_inode->i_mode) && old_dir != new_dir) {
		simplefs_set_parent_ino(old_inode, new_dir);
		if (old_dir->i_nlink > 0)
			drop_nlink(old_dir);
		inc_nlink(new_dir);
	}

	inode_set_ctime_current(old_inode);
	mark_inode_dirty(old_inode);
	simplefs_touch_rename_dirs(old_dir, new_dir);

out_new:
	if (old_dir != new_dir)
		folio_release_kmap(new_dfolio, new_eblock);
out_old:
	folio_release_kmap(old_dfolio, old_eblock);
	return ret;
}

static int simplefs_rename_exchange(struct inode *old_dir,
				    struct dentry *old_dentry,
				    struct inode *new_dir,
				    struct dentry *new_dentry,
				    struct simplefs_handle *handle)
{
	struct super_block *sb = old_dir->i_sb;
	struct inode *old_inode = d_inode(old_dentry);
	struct inode *new_inode = d_inode(new_dentry);
	struct simplefs_file_ei_block *old_eblock, *new_eblock;
	struct folio *old_dfolio, *new_dfolio;
	struct simplefs_dir_entry_pos old_pos, new_pos;
	uint32_t old_block_nr, new_block_nr;
	int ret;

	if (!new_inode)
		return -ENOENT;

	old_eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
		sb, SIMPLEFS_INODE(old_dir)->ei_block, &old_dfolio);
	if (IS_ERR(old_eblock))
		return PTR_ERR(old_eblock);

	ret = simplefs_find_dir_entry(sb, old_eblock, &old_dentry->d_name,
				      old_inode->i_ino, &old_pos);
	if (ret)
		goto out_old;

	if (old_dir == new_dir) {
		new_eblock = old_eblock;
		new_dfolio = old_dfolio;
	} else {
		new_eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
			sb, SIMPLEFS_INODE(new_dir)->ei_block, &new_dfolio);
		if (IS_ERR(new_eblock)) {
			ret = PTR_ERR(new_eblock);
			goto out_old;
		}
	}

	ret = simplefs_find_dir_entry(sb, new_eblock, &new_dentry->d_name,
				      new_inode->i_ino, &new_pos);
	if (ret)
		goto out_new;

	old_block_nr = simplefs_dir_pos_block_nr(old_eblock, &old_pos);
	new_block_nr = simplefs_dir_pos_block_nr(new_eblock, &new_pos);
	if (old_dir == new_dir && old_block_nr == new_block_nr) {
		struct folio *dblock_folio;
		struct simplefs_dir_block *dblock;

		dblock = (struct simplefs_dir_block *)simplefs_get_folio(
			sb, old_block_nr, &dblock_folio);
		if (IS_ERR(dblock)) {
			ret = PTR_ERR(dblock);
			goto out_new;
		}

		dblock->files[old_pos.fi].inode = new_inode->i_ino;
		dblock->files[new_pos.fi].inode = old_inode->i_ino;
		ret = simplefs_mark_folio_dirty(handle, sb, old_block_nr, dblock,
						dblock_folio);
		folio_release_kmap(dblock_folio, dblock);
		if (ret)
			goto out_new;
	} else {
		ret = simplefs_update_dir_entry(sb, old_eblock, &old_pos, handle,
						new_inode->i_ino, NULL, false);
		if (ret)
			goto out_new;

		ret = simplefs_update_dir_entry(sb, new_eblock, &new_pos, handle,
						old_inode->i_ino, NULL, false);
		if (ret)
			goto out_new;
	}

	if (old_dir != new_dir && S_ISDIR(old_inode->i_mode) != S_ISDIR(new_inode->i_mode)) {
		if (S_ISDIR(old_inode->i_mode)) {
			if (old_dir->i_nlink > 0)
				drop_nlink(old_dir);
			inc_nlink(new_dir);
		} else {
			inc_nlink(old_dir);
			if (new_dir->i_nlink > 0)
				drop_nlink(new_dir);
		}
	}

	if (old_dir != new_dir && S_ISDIR(old_inode->i_mode))
		simplefs_set_parent_ino(old_inode, new_dir);
	if (old_dir != new_dir && S_ISDIR(new_inode->i_mode))
		simplefs_set_parent_ino(new_inode, old_dir);

	inode_set_ctime_current(old_inode);
	mark_inode_dirty(old_inode);
	inode_set_ctime_current(new_inode);
	mark_inode_dirty(new_inode);
	simplefs_touch_rename_dirs(old_dir, new_dir);

out_new:
	if (old_dir != new_dir)
		folio_release_kmap(new_dfolio, new_eblock);
out_old:
	folio_release_kmap(old_dfolio, old_eblock);
	return ret;
}

static void simplefs_rollback_whiteout_target(struct inode *new_dir,
					      struct simplefs_file_ei_block *new_eblock,
					      struct folio *new_dfolio,
					      const struct simplefs_dir_entry_pos *new_pos,
					      bool replaced_target,
					      uint32_t replaced_ino,
					      struct simplefs_handle *handle)
{
	if (replaced_target) {
		simplefs_update_dir_entry(new_dir->i_sb, new_eblock, new_pos,
					  handle, replaced_ino, NULL, false);
		return;
	}

	simplefs_update_dir_entry(new_dir->i_sb, new_eblock, new_pos, handle, 0,
				  NULL, true);
	if (new_eblock->nr_files > 0) {
		new_eblock->nr_files--;
		simplefs_mark_dir_index_dirty(new_dir, new_eblock, new_dfolio,
					      handle);
	}
}

static struct inode *simplefs_new_whiteout(struct mnt_idmap *idmap,
					   struct inode *dir,
					   struct dentry *old_dentry)
{
	struct inode *inode;
	struct simplefs_inode_info *ci;
	int ret;

	inode = simplefs_new_inode(idmap, dir, S_IFCHR | WHITEOUT_MODE);
	if (IS_ERR(inode))
		return inode;

	init_special_inode(inode, inode->i_mode, WHITEOUT_DEV);
	ci = SIMPLEFS_INODE(inode);
	*((__le32 *)ci->i_data) = cpu_to_le32(old_encode_dev(WHITEOUT_DEV));

	ret = simplefs_init_security(inode, dir, &old_dentry->d_name);
	if (ret) {
		simplefs_release_new_inode(inode);
		return ERR_PTR(ret);
	}

	return inode;
}

static int simplefs_rename_whiteout(struct mnt_idmap *id,
				    struct inode *old_dir,
				    struct dentry *old_dentry,
				    struct inode *new_dir,
				    struct dentry *new_dentry,
				    struct simplefs_handle *handle)
{
	struct super_block *sb = old_dir->i_sb;
	struct inode *old_inode = d_inode(old_dentry);
	struct inode *new_inode = d_inode(new_dentry);
	struct inode *whiteout = NULL;
	struct simplefs_file_ei_block *old_eblock, *new_eblock;
	struct folio *old_dfolio, *new_dfolio;
	struct simplefs_dir_entry_pos old_pos, new_pos;
	bool replaced_target = false;
	uint32_t replaced_ino = 0;
	int ret;

	whiteout = simplefs_new_whiteout(id, old_dir, old_dentry);
	if (IS_ERR(whiteout))
		return PTR_ERR(whiteout);

	old_eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
		sb, SIMPLEFS_INODE(old_dir)->ei_block, &old_dfolio);
	if (IS_ERR(old_eblock)) {
		ret = PTR_ERR(old_eblock);
		goto out_whiteout;
	}

	ret = simplefs_find_dir_entry(sb, old_eblock, &old_dentry->d_name,
				      old_inode->i_ino, &old_pos);
	if (ret)
		goto out_old;

	ret = simplefs_rename_check_subdir(old_inode, new_dentry);
	if (ret)
		goto out_old;

	ret = simplefs_prepare_rename_target(old_inode, new_inode);
	if (ret)
		goto out_old;

	if (old_dir == new_dir) {
		new_eblock = old_eblock;
		new_dfolio = old_dfolio;
	} else {
		new_eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
			sb, SIMPLEFS_INODE(new_dir)->ei_block, &new_dfolio);
		if (IS_ERR(new_eblock)) {
			ret = PTR_ERR(new_eblock);
			goto out_old;
		}
	}

	if (new_inode) {
		ret = simplefs_find_dir_entry(sb, new_eblock, &new_dentry->d_name,
					      new_inode->i_ino, &new_pos);
		if (ret)
			goto out_new;
		replaced_target = true;
		replaced_ino = new_inode->i_ino;
		ret = simplefs_update_dir_entry(sb, new_eblock, &new_pos, handle,
						old_inode->i_ino, NULL, false);
		if (ret)
			goto out_new;
	} else {
		ret = simplefs_find_free_dir_entry(new_dir, new_eblock, new_dfolio,
						   &new_pos, handle);
		if (ret)
			goto out_new;
		ret = simplefs_update_dir_entry(sb, new_eblock, &new_pos, handle,
						old_inode->i_ino,
						&new_dentry->d_name, false);
		if (ret)
			goto out_new;
		new_eblock->nr_files++;
		ret = simplefs_mark_dir_index_dirty(new_dir, new_eblock,
						    new_dfolio, handle);
		if (ret) {
			new_eblock->nr_files--;
			simplefs_update_dir_entry(sb, new_eblock, &new_pos,
						  handle, 0, NULL, true);
			goto out_new;
		}
	}

	ret = simplefs_update_dir_entry(sb, old_eblock, &old_pos, handle,
					whiteout->i_ino, NULL, false);
	if (ret) {
		simplefs_rollback_whiteout_target(new_dir, new_eblock, new_dfolio,
						  &new_pos, replaced_target,
						  replaced_ino, handle);
		goto out_new;
	}

	mark_inode_dirty(whiteout);
	inode_set_ctime_current(old_inode);
	mark_inode_dirty(old_inode);

	if (new_inode) {
		if (S_ISDIR(new_inode->i_mode))
			clear_nlink(new_inode);
		else if (new_inode->i_nlink > 0)
			drop_nlink(new_inode);
		inode_set_ctime_current(new_inode);
		mark_inode_dirty(new_inode);
	}

	if (S_ISDIR(old_inode->i_mode) && old_dir != new_dir) {
		simplefs_set_parent_ino(old_inode, new_dir);
		if (old_dir->i_nlink > 0)
			drop_nlink(old_dir);
		if (!new_inode)
			inc_nlink(new_dir);
	}

	simplefs_touch_rename_dirs(old_dir, new_dir);

out_new:
	if (old_dir != new_dir)
		folio_release_kmap(new_dfolio, new_eblock);
out_old:
	folio_release_kmap(old_dfolio, old_eblock);
out_whiteout:
	if (ret)
		simplefs_release_new_inode(whiteout);
	else if (whiteout)
		iput(whiteout);
	return ret;
}

static int simplefs_rename_internal(struct mnt_idmap *id,
			   struct inode *old_dir, struct dentry *old_dentry,
			   struct inode *new_dir, struct dentry *new_dentry,
			   unsigned int flags,
			    struct simplefs_handle *handle)
{
	if (flags & ~(RENAME_NOREPLACE | RENAME_EXCHANGE | RENAME_WHITEOUT))
		return -EINVAL;

	if (flags & RENAME_EXCHANGE)
		return simplefs_rename_exchange(old_dir, old_dentry, new_dir,
						new_dentry, handle);
	if (flags & RENAME_WHITEOUT)
		return simplefs_rename_whiteout(id, old_dir, old_dentry, new_dir,
						new_dentry, handle);
	return simplefs_rename_normal(old_dir, old_dentry, new_dir, new_dentry,
				      flags, handle);
}

/*
 * Wrapper for rename with journal transaction support
 */
static int simplefs_rename(struct mnt_idmap *id,
			   struct inode *old_dir, struct dentry *old_dentry,
			   struct inode *new_dir, struct dentry *new_dentry,
			   unsigned int flags)
{
	struct simplefs_handle *handle;
	int credits;
	int ret;
	
	/*
	 * renameat2 的 exchange/whiteout 会同时触碰两个目录项、多个 inode，
	 * whiteout 还会新建一个特殊 inode，这里保守多留一些 credit。
	 */
	credits = 8;
	if (old_dir != new_dir)
		credits += 2;
	if (d_inode(new_dentry))
		credits += 2;
	if (flags & RENAME_EXCHANGE)
		credits += 2;
	if (flags & RENAME_WHITEOUT)
		credits += 4;
	
	handle = simplefs_journal_start(old_dir, credits);
	if (IS_ERR(handle))
		return PTR_ERR(handle);
	
	ret = simplefs_rename_internal(id, old_dir, old_dentry,
				       new_dir, new_dentry, flags, handle);
	if (ret) {
		simplefs_journal_abort(handle, ret);
		simplefs_journal_stop(handle);
		return ret;
	}
	
	return simplefs_journal_stop(handle);
}

/* Forward declaration */
static int simplefs_add_to_dir(struct inode *dir, struct dentry *dentry,
			       struct inode *inode);

/* Helper: add inode to directory */
static int simplefs_add_to_dir(struct inode *dir, struct dentry *dentry,
			       struct inode *inode)
{
	struct super_block *sb = dir->i_sb;
	struct simplefs_inode_info *ci_dir = SIMPLEFS_INODE(dir);
	struct simplefs_file_ei_block *eblock;
	struct simplefs_dir_block *dblock;
	struct folio *dfolio, *dblock_folio;
	uint32_t dir_block_nr = 0;
	int ei, bi, fi;
	int bno = 0;
	int ret = 0;

	if (dentry->d_name.len > SIMPLEFS_FILENAME_LEN)
		return -ENAMETOOLONG;

	/* Read parent directory index */
	eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
		sb, ci_dir->ei_block, &dfolio);
	if (IS_ERR(eblock))
		return PTR_ERR(eblock);

	/* Check if parent directory is full */
	if (eblock->nr_files >= SIMPLEFS_MAX_SUBFILES) {
		folio_release_kmap(dfolio, eblock);
		return -EMLINK;
	}

	/* Find first free slot by scanning for holes */
	{
		int slot_found = 0;

		for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS && !slot_found; ei++) {
			if (!eblock->extents[ei].ee_start)
				break;
			for (bi = 0; bi < simplefs_ext_len(&eblock->extents[ei]) && !slot_found; bi++) {
				dir_block_nr = eblock->extents[ei].ee_start + bi;
				dblock = (struct simplefs_dir_block *)simplefs_get_folio(
					sb, dir_block_nr, &dblock_folio);
				if (IS_ERR(dblock)) {
					ret = PTR_ERR(dblock);
					goto out;
				}
				for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
					if (dblock->files[fi].inode == 0) {
						slot_found = 1;
						break;
					}
				}
				if (!slot_found)
					folio_release_kmap(dblock_folio, dblock);
			}
		}

		if (!slot_found) {
			/* All existing blocks full, allocate new extent.
			 * ei already points to the first free extent slot
			 * from the scanning loop above.
			 */
			bi = 0;
			fi = 0;

			if (!eblock->extents[ei].ee_start) {
				int b;

				bno = get_free_blocks(SIMPLEFS_SB(sb),
						      SIMPLEFS_DIR_BLOCKS_PER_EXTENT);
				if (!bno) {
					folio_release_kmap(dfolio, eblock);
					return -ENOSPC;
				}
				simplefs_ext_init(
					&eblock->extents[ei],
					ei ? eblock->extents[ei - 1].ee_block +
						     simplefs_ext_len(&eblock->extents[ei - 1])
					     : 0,
					SIMPLEFS_DIR_BLOCKS_PER_EXTENT, bno, false);

				/* Zero out all newly allocated dir blocks */
				for (b = 0; b < SIMPLEFS_DIR_BLOCKS_PER_EXTENT;
				     b++) {
					struct folio *zfolio;
					char *zblock;

					zblock = (char *)simplefs_get_folio(
						sb, bno + b, &zfolio);
					if (IS_ERR(zblock)) {
						ret = PTR_ERR(zblock);
						goto out;
					}
					memset(zblock, 0, SIMPLEFS_BLOCK_SIZE);
					ret = simplefs_journal_dirty_folio(
						sb, bno + b, zblock, zfolio);
					folio_release_kmap(zfolio, zblock);
					if (ret)
						goto out;
				}
			}
			dir_block_nr = eblock->extents[ei].ee_start + bi;
			dblock = (struct simplefs_dir_block *)simplefs_get_folio(
				sb, dir_block_nr, &dblock_folio);
			if (IS_ERR(dblock)) {
				ret = PTR_ERR(dblock);
				goto out;
			}
		}
	}

	dblock->files[fi].inode = inode->i_ino;
	memset(dblock->files[fi].filename, 0, SIMPLEFS_FILENAME_LEN);
	memcpy(dblock->files[fi].filename, dentry->d_name.name,
	       dentry->d_name.len);

	ret = simplefs_journal_dirty_folio(sb, dir_block_nr, dblock,
					   dblock_folio);
	folio_release_kmap(dblock_folio, dblock);
	if (ret)
		goto out;

	eblock->nr_files++;
	ret = simplefs_journal_dirty_folio(sb, ci_dir->ei_block, eblock,
					   dfolio);
	if (ret)
		goto out;

	if (bno) {
		dir->i_blocks += simplefs_blocks_to_sectors(
			SIMPLEFS_DIR_BLOCKS_PER_EXTENT);
		mark_inode_dirty(dir);
	}

out:
	folio_release_kmap(dfolio, eblock);
	return ret;
}


/* Create a symbolic link */
static int simplefs_symlink(struct mnt_idmap *id, struct inode *dir,
			    struct dentry *dentry, const char *symname)
{
	struct simplefs_handle *handle;
	struct inode *inode;
	struct simplefs_inode_info *ci;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(dir->i_sb);
	struct folio *folio = NULL;
	char *block = NULL;
	size_t len = strlen(symname);
	uint32_t bno = 0;
	int err = 0, stop_err;

	pr_debug("[martins3:%s:%d] creating symlink %pd -> %s\n",
		__func__, __LINE__, dentry, symname);

	/* Check symlink target length */
	if (len >= SIMPLEFS_BLOCK_SIZE)
		return -ENAMETOOLONG;

	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	/* Create the inode */
	inode = simplefs_new_inode(id, dir, S_IFLNK | 0777);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_stop;
	}

	ci = SIMPLEFS_INODE(inode);

	/* Store short symlinks inline; spill long targets to one data block. */
	if (len < sizeof(ci->i_data)) {
		memcpy(ci->i_data, symname, len);
		ci->i_data[len] = '\0';
		ci->ei_block = 0;
		inode->i_blocks = 0;
	} else {
		bno = get_free_blocks(sbi, 1);
		if (!bno) {
			simplefs_release_new_inode(inode);
			err = -ENOSPC;
			goto out_stop;
		}

		block = (char *)simplefs_get_folio(dir->i_sb, bno, &folio);
		if (IS_ERR(block)) {
			err = PTR_ERR(block);
			put_blocks(sbi, bno, 1);
			simplefs_release_new_inode(inode);
			goto out_stop;
		}

		memset(block, 0, SIMPLEFS_BLOCK_SIZE);
		memcpy(block, symname, len);
		err = simplefs_journal_dirty_folio(dir->i_sb, bno, block,
						   folio);
		folio_release_kmap(folio, block);
		if (err) {
			put_blocks(sbi, bno, 1);
			simplefs_release_new_inode(inode);
			goto out_stop;
		}

		ci->ei_block = bno;
		memset(ci->i_data, 0, sizeof(ci->i_data));
		inode->i_blocks = simplefs_blocks_to_sectors(1);
	}

	/* Set up symlink */
	inode->i_link = len < sizeof(ci->i_data) ? ci->i_data : NULL;
	inode->i_op = &symlink_inode_ops;
	inode->i_size = len;

	err = simplefs_init_security(inode, dir, &dentry->d_name);
	if (err) {
		simplefs_release_new_inode(inode);
		goto out_stop;
	}

	/* Add to directory - use the same logic as create_internal */
	err = simplefs_add_to_dir(dir, dentry, inode);
	if (err) {
		simplefs_release_new_inode(inode);
		goto out_stop;
	}

	mark_inode_dirty(inode);
	simplefs_persist_inode(inode);
	inode_set_mtime_to_ts(dir, inode_set_ctime_current(dir));
	mark_inode_dirty(dir);
	simplefs_persist_inode(dir);
	d_instantiate(dentry, inode);

out_stop:
	stop_err = simplefs_journal_stop(handle);
	if (!err)
		err = stop_err;
	return err;
}

/* Setattr - handle various attribute changes including truncate */
/* Forward declaration for truncate */
int simplefs_truncate(struct inode *inode, loff_t size);

static int simplefs_setattr(struct mnt_idmap *id, struct dentry *dentry,
			    struct iattr *attr)
{
	struct inode *inode = d_inode(dentry);
	int error;

	pr_debug("[martins3:%s:%d] setattr ino=%llu ia_valid=0x%x\n",
		__func__, __LINE__, (unsigned long long)inode->i_ino,
		attr->ia_valid);

	/* Check validity of attributes */
	error = setattr_prepare(id, dentry, attr);
	if (error)
		return error;

	/* Handle truncate */
	if (attr->ia_valid & ATTR_SIZE) {
		error = simplefs_truncate(inode, attr->ia_size);
		if (error)
			return error;
		inode_set_mtime_to_ts(inode, inode_set_ctime_current(inode));
	}

	/* Apply other attribute changes */
	setattr_copy(id, inode, attr);
	mark_inode_dirty(inode);

	if (attr->ia_valid & ATTR_MODE)
		error = posix_acl_chmod(id, dentry, inode->i_mode);

	return error;
}

/* Truncate file to specified size
 * Release blocks that are no longer needed
 */
int simplefs_truncate(struct inode *inode, loff_t size)
{
	struct simplefs_truncate_range {
		uint32_t start;
		uint32_t len;
	};
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_extent_buffer buf;
	struct simplefs_truncate_range *freed_ranges = NULL;
	loff_t old_size = i_size_read(inode);
	unsigned int blkbits = inode->i_blkbits;
	uint32_t old_blocks = (old_size + (1 << blkbits) - 1) >> blkbits;
	uint32_t new_blocks = (size + (1 << blkbits) - 1) >> blkbits;
	bool trim_prealloc_tail = false;
	uint32_t nr_freed_ranges = 0;
	int ret = 0;
	int i;

	pr_debug("[martins3:%s:%d] truncate ino=%llu old_size=%lld new_size=%lld\n",
		__func__, __LINE__, (unsigned long long)inode->i_ino,
		old_size, size);

	/* Extent removal and i_size changes must not race pending direct I/O. */
	inode_dio_wait(inode);

	/* Extending truncate */
	if (size > old_size) {
		truncate_setsize(inode, size);
		ret = simplefs_zero_partial_gap(inode, old_size,
						 size - old_size);
		if (ret) {
			truncate_setsize(inode, old_size);
			return ret;
		}
		mark_inode_dirty(inode);
		return 0;
	}

	if (ci->prealloc_len) {
		uint32_t new_end = (size + (1 << blkbits) - 1) >> blkbits;
		uint32_t pre_end = ci->prealloc_block + ci->prealloc_len;

		if (new_end < pre_end)
			trim_prealloc_tail = true;

		if (new_end <= ci->prealloc_block) {
			ci->prealloc_block = 0;
			ci->prealloc_len = 0;
		} else if (new_end < pre_end) {
			ci->prealloc_len = new_end - ci->prealloc_block;
		}
	}

	/* simplefs 先求收缩语义正确：在调整 extent 前把当前 inode 的脏页
	 * 全部写回并等待完成，避免旧脏页在 extent 已释放后才异步落盘。
	 */
	ret = filemap_write_and_wait(inode->i_mapping);
	simplefs_wait_ioend_conversions(inode);
	if (ret)
		return ret;

	filemap_invalidate_lock(inode->i_mapping);

	/* Zero the partial block tail before truncating page cache */
	ret = simplefs_iomap_truncate_page_compat(inode, size);
	if (ret)
		goto out_unlock;

	/* 先按新的 EOF 收缩页缓存，避免后台 writeback 继续落到即将释放的
	 * 旧块上。looptest/xfstests 会频繁 write+truncate(0)；如果先 free
	 * extent 再缩页缓存，随后到达的 writeback 会直接打到无效映射并报错。
	 */
	truncate_setsize(inode, size);

	/* Shrinking within the same last data block can skip extent surgery,
	 * but truncating at the current i_size still needs to drop any
	 * KEEP_SIZE/unwritten tail that lives beyond EOF.
	 */
	if (size != old_size && new_blocks == old_blocks && !trim_prealloc_tail) {
		mark_inode_dirty(inode);
		goto out_unlock;
	}

	mutex_lock(&ci->extent_lock);
	ret = simplefs_file_load_extents(sb, ci->ei_block, &buf);
	if (ret)
		goto out_extent_unlock;
	if (buf.nr_extents) {
		freed_ranges = kvmalloc_array(buf.nr_extents,
					       sizeof(*freed_ranges), GFP_NOFS);
		if (!freed_ranges) {
			ret = -ENOMEM;
			goto out_destroy_buf;
		}
	}

	/* Find and free extents/blocks beyond the new block count */
	uint32_t blocks_freed = 0;
	for (i = 0; i < buf.nr_extents; i++) {
		uint32_t ext_start, ext_end, ext_len;

		ext_start = buf.extents[i].ee_block;
		ext_len = simplefs_ext_len(&buf.extents[i]);
		ext_end = ext_start + ext_len;

		/* If this extent is completely after new size, free it */
		if (ext_start >= new_blocks) {
			freed_ranges[nr_freed_ranges++] =
				(struct simplefs_truncate_range) {
					.start = buf.extents[i].ee_start,
					.len = ext_len,
				};
			blocks_freed += ext_len;
			simplefs_extent_clear(&buf.extents[i]);
		} else if (ext_end > new_blocks) {
			/* Partial overlap: free the tail of this extent */
			uint32_t keep = new_blocks - ext_start;
			uint32_t freed = ext_len - keep;

			freed_ranges[nr_freed_ranges++] =
				(struct simplefs_truncate_range) {
					.start = buf.extents[i].ee_start + keep,
					.len = freed,
				};
			blocks_freed += freed;
			simplefs_ext_set_len(&buf.extents[i], keep);
		}
	}

	if (blocks_freed > 0) {
		ret = simplefs_file_sync_extents(inode, &buf);
		if (ret)
			goto out_destroy_buf;

		for (i = 0; i < nr_freed_ranges; i++) {
			put_blocks(sbi, freed_ranges[i].start, freed_ranges[i].len);
			trace_simplefs_free_blocks(sb, freed_ranges[i].start,
						   freed_ranges[i].len);
		}
		inode->i_blocks -= simplefs_blocks_to_sectors(blocks_freed);
	}

	out_destroy_buf:
	kvfree(freed_ranges);
	simplefs_file_destroy_extents(&buf);
	if (ret)
		goto out_extent_unlock;
	mutex_unlock(&ci->extent_lock);

	/* 这里已经通过 put_blocks() 增量释放了物理块。
	 * 在线全盘重建 bfree_bitmap 会和并发 create/rename/write 的元数据
	 * 更新交叉，最终把全局位图覆盖成一个不一致快照，fsstress 下会导致
	 * 活跃的目录/extent 元数据块被重新分配。
	 */

	/* truncate_setsize() 已在前面完成，这里只需要持久化元数据变化。 */
	mark_inode_dirty(inode);

out_unlock:
	filemap_invalidate_unlock(inode->i_mapping);
	return ret;

out_extent_unlock:
	mutex_unlock(&ci->extent_lock);
	goto out_unlock;
}

static int simplefs_link(struct dentry *old_dentry, struct inode *dir,
			 struct dentry *dentry)
{
	struct inode *inode = d_inode(old_dentry);
	struct simplefs_handle *handle;
	int err;

	/* Start journal transaction: parent dir data + inode update
	 * Credits: parent inode (1) + parent dir data block (1) + file inode (1)
	 */
	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	inode_set_ctime_current(inode);
	inode_inc_link_count(inode);
	ihold(inode);

	err = simplefs_add_to_dir(dir, dentry, inode);
	if (err) {
		inode_dec_link_count(inode);
		iput(inode);
		simplefs_journal_abort(handle, err);
		simplefs_journal_stop(handle);
		return err;
	}

	mark_inode_dirty(inode);
	simplefs_persist_inode(inode);
	inode_set_mtime_to_ts(dir, inode_set_ctime_current(dir));
	mark_inode_dirty(dir);
	simplefs_persist_inode(dir);
	d_instantiate(dentry, inode);

	return simplefs_journal_stop(handle);
}

static int simplefs_tmpfile(struct mnt_idmap *idmap, struct inode *dir,
			    struct file *file, umode_t mode)
{
	struct simplefs_handle *handle;
	struct inode *inode;
	int err, stop_err;

	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	inode = simplefs_new_inode(idmap, dir, mode | S_IFREG);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_stop;
	}

	/* d_tmpfile() drops the initial positive link count to detach the
	 * inode from the namespace. Make the starting state explicit so tmpfile
	 * opens do not inherit a stale zero-link inode state.
	 */
	set_nlink(inode, 1);
	mark_inode_dirty(inode);
	d_tmpfile(file, inode);
	err = finish_open_simple(file, 0);
	if (err)
		discard_new_inode(inode);

out_stop:
	stop_err = simplefs_journal_stop(handle);
	if (!err)
		err = stop_err;
	return err;
}

static int simplefs_mknod(struct mnt_idmap *id, struct inode *dir,
			  struct dentry *dentry, umode_t mode, dev_t rdev)
{
	struct simplefs_handle *handle;
	struct inode *inode;
	struct simplefs_inode_info *ci;
	int err, stop_err;

	handle = simplefs_journal_start(dir, 32);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	inode = simplefs_new_inode(id, dir, mode);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_stop;
	}

	init_special_inode(inode, inode->i_mode, rdev);

	/* Store dev_t in i_data for persistence */
	ci = SIMPLEFS_INODE(inode);
	if (S_ISBLK(mode) || S_ISCHR(mode))
		*((__le32 *)ci->i_data) = cpu_to_le32(old_encode_dev(rdev));

	err = simplefs_init_security(inode, dir, &dentry->d_name);
	if (err) {
		simplefs_release_new_inode(inode);
		goto out_stop;
	}

	err = simplefs_add_to_dir(dir, dentry, inode);
	if (err) {
		simplefs_release_new_inode(inode);
		goto out_stop;
	}

	mark_inode_dirty(inode);
	simplefs_persist_inode(inode);
	inode_set_mtime_to_ts(dir, inode_set_ctime_current(dir));
	mark_inode_dirty(dir);
	simplefs_persist_inode(dir);
	d_instantiate(dentry, inode);

out_stop:
	stop_err = simplefs_journal_stop(handle);
	if (!err)
		err = stop_err;
	return err;
}

static int simplefs_getattr(struct mnt_idmap *idmap,
			    const struct path *path, struct kstat *stat,
			    u32 request_mask, unsigned int query_flags)
{
	struct inode *inode = d_inode(path->dentry);
	unsigned int flags = SIMPLEFS_INODE(inode)->i_flags &
		SIMPLEFS_FL_USER_VISIBLE;

	/* 与 ext2 相同，从磁盘标志位补 STATX 属性（此内核的
	 * generic_fillattr 不从 i_flags 推导 nodump 等属性）。 */
	if (flags & FS_APPEND_FL)
		stat->attributes |= STATX_ATTR_APPEND;
	if (flags & FS_IMMUTABLE_FL)
		stat->attributes |= STATX_ATTR_IMMUTABLE;
	if (flags & FS_NODUMP_FL)
		stat->attributes |= STATX_ATTR_NODUMP;
	stat->attributes_mask |= (STATX_ATTR_APPEND |
			STATX_ATTR_IMMUTABLE | STATX_ATTR_NODUMP);

	if (request_mask & STATX_BTIME) {
		stat->result_mask |= STATX_BTIME;
		stat->btime = SIMPLEFS_INODE(inode)->i_crtime;
	}

	generic_fillattr(idmap, request_mask, inode, stat);
	if ((request_mask & STATX_DIOALIGN) && S_ISREG(inode->i_mode)) {
		stat->result_mask |= STATX_DIOALIGN;
		stat->dio_mem_align = inode->i_sb->s_blocksize;
		stat->dio_offset_align = inode->i_sb->s_blocksize;
	}
	return 0;
}

/* 把 ci->i_flags（FS_*_FL）同步到 inode->i_flags（S_*），
 * VFS 依据后者做 immutable/append/sync/noatime 等通用检查。 */
static void simplefs_set_inode_flags(struct inode *inode)
{
	unsigned int flags = SIMPLEFS_INODE(inode)->i_flags;

	inode->i_flags &= ~(S_IMMUTABLE | S_APPEND | S_SYNC | S_DIRSYNC |
			    S_NOATIME);
	if (flags & FS_IMMUTABLE_FL)
		inode->i_flags |= S_IMMUTABLE;
	if (flags & FS_APPEND_FL)
		inode->i_flags |= S_APPEND;
	if (flags & FS_SYNC_FL)
		inode->i_flags |= S_SYNC;
	if (flags & FS_DIRSYNC_FL)
		inode->i_flags |= S_DIRSYNC;
	if (flags & FS_NOATIME_FL)
		inode->i_flags |= S_NOATIME;
}

static int simplefs_fileattr_get(struct dentry *dentry, struct file_kattr *fa)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(d_inode(dentry));

	fileattr_fill_flags(fa, ci->i_flags & SIMPLEFS_FL_USER_VISIBLE);
	return 0;
}

static int simplefs_fileattr_set(struct mnt_idmap *idmap,
				 struct dentry *dentry, struct file_kattr *fa)
{
	struct inode *inode = d_inode(dentry);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);

	/* 不支持 xfs 风格的扩展属性（extsize/projid 等） */
	if (fileattr_has_fsx(fa))
		return -EOPNOTSUPP;

	/* 未实现的标志位（DAX/compress/verity/projinherit 等）必须明确
	 * 拒绝，不能静默丢弃——否则探测类 require（generic/607 的 DAX、
	 * generic/424 的 +c）会误以为支持而在后续断言中失败。
	 */
	if (fa->flags & ~SIMPLEFS_FL_USER_MODIFIABLE)
		return -EOPNOTSUPP;

	ci->i_flags = (ci->i_flags & ~SIMPLEFS_FL_USER_MODIFIABLE) |
		(fa->flags & SIMPLEFS_FL_USER_MODIFIABLE);
	simplefs_set_inode_flags(inode);
	inode_set_ctime_current(inode);
	mark_inode_dirty(inode);
	return 0;
}

static const struct inode_operations simplefs_inode_ops = {
	.lookup = simplefs_lookup,
	.create = simplefs_create,
	.mkdir = simplefs_mkdir,
	.unlink = simplefs_unlink,
	.rmdir = simplefs_rmdir,
	.rename = simplefs_rename,
	.link = simplefs_link,
	.symlink = simplefs_symlink,
	.mknod = simplefs_mknod,
	.setattr = simplefs_setattr,
	.getattr = simplefs_getattr,
	.tmpfile = simplefs_tmpfile,
	.fiemap = simplefs_fiemap,
	.listxattr = simplefs_listxattr,
	.fileattr_get = simplefs_fileattr_get,
	.fileattr_set = simplefs_fileattr_set,
#ifdef CONFIG_FS_POSIX_ACL
	.get_inode_acl = simplefs_get_acl,
	.set_acl = simplefs_set_acl,
#endif
};
