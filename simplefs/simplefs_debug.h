// SPDX-License-Identifier: GPL-2.0
/*
 * SimpleFS Debug Infrastructure
 * Provides debugfs interface and statistics tracking
 */

#ifndef SIMPLEFS_DEBUG_H
#define SIMPLEFS_DEBUG_H

#include <linux/atomic.h>

/* ========== Statistics Counters ========== */

struct simplefs_stats {
	/* I/O statistics */
	atomic64_t read_count;
	atomic64_t write_count;
	atomic64_t read_bytes;
	atomic64_t write_bytes;

	/* Allocation statistics */
	atomic64_t block_alloc_count;
	atomic64_t block_free_count;
	atomic64_t inode_alloc_count;
	atomic64_t inode_free_count;

	/* Error statistics */
	atomic64_t error_count;
};

extern struct simplefs_stats sfs_stats;

/* Statistics macros */
#define sfs_stat_inc(field) atomic64_inc(&sfs_stats.field)
#define sfs_stat_add(field, n) atomic64_add((n), &sfs_stats.field)
#define sfs_stat_read(field) atomic64_read(&sfs_stats.field)

/* ========== Debugfs Interface ========== */

int simplefs_debugfs_init(void);
void simplefs_debugfs_exit(void);

#endif /* SIMPLEFS_DEBUG_H */
