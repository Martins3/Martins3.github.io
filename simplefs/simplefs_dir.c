#define pr_fmt(fmt) "simplefs: " fmt

#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/writeback.h>

#include "simplefs.h"
#include "simplefs_journal.h"

/* Iterate over the files contained in dir and commit them to @ctx.
 * This function is called by the VFS as ctx->pos changes.
 * Returns 0 on success.
 */
static int simplefs_iterate(struct file *dir, struct dir_context *ctx)
{
	struct inode *inode = file_inode(dir);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct folio *efolio;
	struct folio *dfolio;
	struct simplefs_file_ei_block *eblock = NULL;
	struct simplefs_dir_block *dblock = NULL;
	struct simplefs_file *f = NULL;
	int ei = 0, bi = 0, fi = 0;
	int ret = 0;

	/* Check that dir is a directory */
	if (!S_ISDIR(inode->i_mode))
		return -ENOTDIR;

	/* Check that ctx->pos is not bigger than what we can handle (including
         * . and ..)
         */
	if (ctx->pos > SIMPLEFS_MAX_SUBFILES + 2)
		return 0;

	pr_debug("[martins3:%s:%d]\n", __func__, __LINE__);
	/* Commit . and .. to ctx */
	if (!dir_emit_dots(dir, ctx))
		return 0;

	/* Read the directory index block on disk */
	eblock = (struct simplefs_file_ei_block *)simplefs_get_folio(
		sb, ci->ei_block, &efolio);
	if (IS_ERR(eblock))
		return -EIO;

	/*
	 * Calculate (ei, bi, fi) from ctx->pos by walking actual extents.
	 * ctx->pos - 2 is the slot index (excluding . and ..).
	 */
	{
		int remaining = ctx->pos - 2;

		ei = 0;
		bi = 0;
		fi = 0;
		for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS; ei++) {
			int ext_files;

			if (!eblock->extents[ei].ee_start)
				break;
			ext_files = simplefs_ext_len(&eblock->extents[ei]) *
				    SIMPLEFS_FILES_PER_BLOCK;
			if (remaining < ext_files) {
				bi = remaining / SIMPLEFS_FILES_PER_BLOCK;
				fi = remaining % SIMPLEFS_FILES_PER_BLOCK;
				break;
			}
			remaining -= ext_files;
		}
	}

	/* Iterate over the index block and commit subfiles */
	for (; ei < SIMPLEFS_MAX_EXTENTS; ei++) {
		if (eblock->extents[ei].ee_start == 0)
			break;

		/* Iterate over blocks in one extent */
		for (; bi < simplefs_ext_len(&eblock->extents[ei]); bi++) {
			dblock = (struct simplefs_dir_block *)simplefs_get_folio(
				sb, eblock->extents[ei].ee_start + bi, &dfolio);
			if (IS_ERR(dblock)) {
				ret = PTR_ERR(dblock);
				goto release_bh;
			}

			/* Iterate every file in one block */
			for (; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
				f = &dblock->files[fi];
				if (f->inode && !dir_emit(ctx, f->filename,
							  strnlen(f->filename, SIMPLEFS_FILENAME_LEN),
							  f->inode, DT_UNKNOWN)) {
					folio_release_kmap(dfolio, dblock);
					goto release_bh;
				}
				ctx->pos++;
			}
			folio_release_kmap(dfolio, dblock);
			fi = 0;
			cond_resched();
		}
		bi = 0;
	}

release_bh:
	folio_release_kmap(efolio, eblock);
	return ret;
}

/* Directory entries and their inode snapshot share JBD2 transactions. */
static int simplefs_dir_fsync(struct file *file, loff_t start, loff_t end,
			      int datasync)
{
	struct inode *inode = file_inode(file);
	struct super_block *sb = inode->i_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	int err, err2;

	err = sync_inode_metadata(inode, 1);
	if (sbi->s_journal && sbi->s_journal->j_jbd2) {
		err2 = simplefs_journal_force_commit(sbi->s_journal);
		if (!err)
			err = err2;
		return err;
	}

	err2 = filemap_write_and_wait_range(sb->s_bdev->bd_mapping, 0, LLONG_MAX);
	if (err)
		return err;

	return err2 ? err2 : blkdev_issue_flush(sb->s_bdev);
}

static loff_t simplefs_dir_llseek(struct file *file, loff_t offset, int whence)
{
	return generic_file_llseek(file, offset, whence);
}

const struct file_operations simplefs_dir_ops = {
	.owner = THIS_MODULE,
	.llseek = simplefs_dir_llseek,
	.iterate_shared = simplefs_iterate,
	.fsync = simplefs_dir_fsync,
	.unlocked_ioctl = simplefs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = simplefs_compat_ioctl,
#endif
};
