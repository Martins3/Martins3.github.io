/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SIMPLEFS_JOURNAL_H
#define _SIMPLEFS_JOURNAL_H

#ifdef __KERNEL__
#include <linux/fs.h>
#include <linux/jbd2.h>
#include <linux/mutex.h>
#include <linux/types.h>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

/* JBD2 requires at least 1024 blocks; SimpleFS reserves exactly 4 MiB. */
#define SIMPLEFS_JOURNAL_BLOCKS 1024

/* On-disk JBD2 constants.  They are repeated here so mkfs.simplefs does not
 * depend on private kernel headers.  All fields in a JBD2 superblock are big
 * endian, unlike the surrounding SimpleFS on-disk format.
 */
#define SIMPLEFS_JBD2_MAGIC 0xc03b3998U
#define SIMPLEFS_JBD2_SUPERBLOCK_V2 4U

#ifdef __KERNEL__
typedef __be32 simplefs_jbd2_be32;
#else
typedef uint32_t simplefs_jbd2_be32;
#endif

struct simplefs_jbd2_header {
	simplefs_jbd2_be32 h_magic;
	simplefs_jbd2_be32 h_blocktype;
	simplefs_jbd2_be32 h_sequence;
};

/* Byte-for-byte prefix/layout of journal_superblock_t in linux/jbd2.h. */
struct simplefs_jbd2_superblock {
	struct simplefs_jbd2_header s_header;
	simplefs_jbd2_be32 s_blocksize;
	simplefs_jbd2_be32 s_maxlen;
	simplefs_jbd2_be32 s_first;
	simplefs_jbd2_be32 s_sequence;
	simplefs_jbd2_be32 s_start;
	simplefs_jbd2_be32 s_errno;
	simplefs_jbd2_be32 s_feature_compat;
	simplefs_jbd2_be32 s_feature_incompat;
	simplefs_jbd2_be32 s_feature_ro_compat;
	uint8_t s_uuid[16];
	simplefs_jbd2_be32 s_nr_users;
	simplefs_jbd2_be32 s_dynsuper;
	simplefs_jbd2_be32 s_max_transaction;
	simplefs_jbd2_be32 s_max_trans_data;
	uint8_t s_checksum_type;
	uint8_t s_padding2[3];
	simplefs_jbd2_be32 s_num_fc_blks;
	simplefs_jbd2_be32 s_head;
	uint32_t s_padding[40];
	simplefs_jbd2_be32 s_checksum;
	uint8_t s_users[16 * 48];
};

#ifdef __KERNEL__

struct simplefs_journal {
	struct super_block *j_sb;
	journal_t *j_jbd2;
	/* Serialize filesystem handle lifetimes.  Folio mappings obtain JBD2
	 * write access before mutation, so completed handles need not wait for
	 * transaction commit here.
	 */
	struct mutex j_handle_mutex;
	/* The adapter serializes handles, so remember the owning task and its
	 * JBD2 handle explicitly.  This also provides a reliable nesting check
	 * when journal_current_handle() is temporarily unavailable to a nested
	 * filesystem helper. */
	struct task_struct *j_handle_owner;
	handle_t *j_active_handle;
};

struct simplefs_handle {
	struct simplefs_journal *h_journal;
	handle_t *h_jbd2;
};

int simplefs_journal_load(struct super_block *sb);
void simplefs_journal_destroy(struct super_block *sb);
int simplefs_journal_set_needs_recovery(struct super_block *sb, bool dirty);

struct simplefs_handle *simplefs_journal_start(struct inode *inode, int blocks);
struct simplefs_handle *simplefs_journal_start_sb(struct super_block *sb,
						  int blocks);
int simplefs_journal_stop(struct simplefs_handle *handle);
void simplefs_journal_abort(struct simplefs_handle *handle, int err);

int simplefs_journal_dirty_metadata(struct simplefs_handle *handle,
				     struct super_block *sb,
				     uint32_t block_nr, void *data,
				     struct folio *folio);
int simplefs_journal_dirty_current(struct super_block *sb,
				   uint32_t block_nr, void *data);
int simplefs_journal_prepare_current(struct super_block *sb,
				     uint32_t block_nr);
int simplefs_journal_prepare_new_blocks(struct super_block *sb,
					 uint32_t block_nr, uint32_t len);
int simplefs_journal_dirty_folio(struct super_block *sb, uint32_t block_nr,
				 void *data, struct folio *folio);
int simplefs_journal_forget_current(struct super_block *sb,
				    uint32_t block_nr);
bool simplefs_journal_has_current_handle(struct super_block *sb);

int simplefs_journal_checkpoint(struct simplefs_journal *journal);
int simplefs_journal_force_commit(struct simplefs_journal *journal);
void simplefs_journal_shutdown(struct simplefs_journal *journal);
void simplefs_journal_dump(struct simplefs_journal *journal);

static inline int simplefs_journal_block(struct super_block *sb,
					 uint32_t block_nr, void *data)
{
	return simplefs_journal_dirty_current(sb, block_nr, data);
}

#endif /* __KERNEL__ */

#endif /* _SIMPLEFS_JOURNAL_H */
