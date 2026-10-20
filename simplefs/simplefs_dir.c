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
	return simplefs_dir_emit(dir, ctx);
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
