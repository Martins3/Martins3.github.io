// SPDX-License-Identifier: GPL-2.0
/*
 * SimpleFS Debug Infrastructure
 * Provides debugfs interface for monitoring filesystem metrics
 */

#define pr_fmt(fmt) "simplefs: " fmt

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/module.h>

#include "simplefs_debug.h"
#include "simplefs.h"

/* Global statistics */
struct simplefs_stats sfs_stats;
EXPORT_SYMBOL(sfs_stats);

/* Debugfs root directory */
static struct dentry *sfs_debugfs_root;

/*
 * Show all statistics in a single file
 * Format: key value (one per line)
 */
static int stats_show(struct seq_file *m, void *v)
{
	seq_printf(m, "read_count %lld\n",
		   atomic64_read(&sfs_stats.read_count));
	seq_printf(m, "write_count %lld\n",
		   atomic64_read(&sfs_stats.write_count));
	seq_printf(m, "read_bytes %lld\n",
		   atomic64_read(&sfs_stats.read_bytes));
	seq_printf(m, "write_bytes %lld\n",
		   atomic64_read(&sfs_stats.write_bytes));
	seq_printf(m, "block_alloc_count %lld\n",
		   atomic64_read(&sfs_stats.block_alloc_count));
	seq_printf(m, "block_free_count %lld\n",
		   atomic64_read(&sfs_stats.block_free_count));
	seq_printf(m, "inode_alloc_count %lld\n",
		   atomic64_read(&sfs_stats.inode_alloc_count));
	seq_printf(m, "inode_free_count %lld\n",
		   atomic64_read(&sfs_stats.inode_free_count));
	seq_printf(m, "error_count %lld\n",
		   atomic64_read(&sfs_stats.error_count));

	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, stats_show, NULL);
}

static const struct file_operations stats_fops = {
	.owner = THIS_MODULE,
	.open = stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
 * Reset all statistics
 * Write any value to the reset file to clear counters
 */
static ssize_t reset_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	atomic64_set(&sfs_stats.read_count, 0);
	atomic64_set(&sfs_stats.write_count, 0);
	atomic64_set(&sfs_stats.read_bytes, 0);
	atomic64_set(&sfs_stats.write_bytes, 0);
	atomic64_set(&sfs_stats.block_alloc_count, 0);
	atomic64_set(&sfs_stats.block_free_count, 0);
	atomic64_set(&sfs_stats.inode_alloc_count, 0);
	atomic64_set(&sfs_stats.inode_free_count, 0);
	atomic64_set(&sfs_stats.error_count, 0);

	pr_debug("statistics reset\n");
	return count;
}

static const struct file_operations reset_fops = {
	.owner = THIS_MODULE,
	.write = reset_write,
};

/*
 * Initialize debugfs interface
 * Creates /sys/kernel/debug/simplefs/ directory with:
 *   - stats: read-only file showing all statistics
 *   - reset: write-only file to reset statistics
 */
int simplefs_debugfs_init(void)
{
	/* Initialize all stats to zero */
	memset(&sfs_stats, 0, sizeof(sfs_stats));

	/* Create debugfs directory */
	sfs_debugfs_root = debugfs_create_dir("simplefs", NULL);
	if (IS_ERR(sfs_debugfs_root)) {
		pr_err("failed to create debugfs directory\n");
		return PTR_ERR(sfs_debugfs_root);
	}

	/* Create stats file */
	debugfs_create_file("stats", 0444, sfs_debugfs_root, NULL, &stats_fops);

	/* Create reset file */
	debugfs_create_file("reset", 0200, sfs_debugfs_root, NULL, &reset_fops);

	pr_debug("debugfs initialized at /sys/kernel/debug/simplefs/\n");
	return 0;
}

/*
 * Cleanup debugfs interface
 */
void simplefs_debugfs_exit(void)
{
	debugfs_remove_recursive(sfs_debugfs_root);
	sfs_debugfs_root = NULL;
	pr_debug("debugfs cleanup complete\n");
}
