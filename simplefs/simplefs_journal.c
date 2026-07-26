/* SPDX-License-Identifier: GPL-2.0 */

/*
 * SimpleFS metadata journal adapter.
 *
 * Regular file data remains iomap/folio based.  JBD2's client API is built
 * around buffer_head, so this file is the single compatibility boundary that
 * maps a SimpleFS metadata block number and its folio snapshot to JBD2.
 */

#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/slab.h>

#include "simplefs.h"
#include "simplefs_journal.h"
#include "simplefs_trace.h"

static handle_t *simplefs_current_jbd2_handle(
	struct simplefs_journal *journal);

static void simplefs_journal_mark_shutdown(struct simplefs_journal *journal)
{
	if (journal && journal->j_sb)
		((struct simplefs_sb_info *)SIMPLEFS_SB(journal->j_sb))->s_shutdown = 1;
}

static int simplefs_journal_bmap(journal_t *journal, sector_t *block)
{
	struct super_block *sb = journal->j_private;
	struct simplefs_sb_info *sbi;

	if (!sb)
		return -EIO;
	sbi = SIMPLEFS_SB(sb);
	if (*block >= journal->j_total_len)
		return -EFSCORRUPTED;

	*block += sbi->s_journal_start;
	return 0;
}

static void simplefs_journal_free(struct simplefs_journal *journal)
{
	if (!journal)
		return;
	if (journal->j_jbd2)
		jbd2_journal_destroy(journal->j_jbd2);
	mutex_destroy(&journal->j_handle_mutex);
	kfree(journal);
}

int simplefs_journal_set_needs_recovery(struct super_block *sb, bool dirty)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_sb_info *disk_sbi;
	struct buffer_head *bh;
	int ret = 0;

	if (sbi->s_needs_recovery == dirty)
		return 0;
	if (bdev_read_only(sb->s_bdev))
		return -EROFS;

	/* This is the JBD2 lifecycle marker, not ordinary SimpleFS metadata.
	 * Write it outside the running log so a forced shutdown cannot make a
	 * fully checkpointed journal look like a clean unmount.  buffer_head is
	 * deliberately confined to this adapter. */
	bh = sb_bread(sb, SIMPLEFS_SB_BLOCK_NR);
	if (!bh)
		return -EIO;

	lock_buffer(bh);
	disk_sbi = (struct simplefs_sb_info *)bh->b_data;
	disk_sbi->s_needs_recovery = dirty;
	set_buffer_uptodate(bh);
	mark_buffer_dirty(bh);
	unlock_buffer(bh);

	sync_dirty_buffer(bh);
	if (!buffer_uptodate(bh))
		ret = -EIO;
	if (!ret)
		ret = blkdev_issue_flush(sb->s_bdev);
	brelse(bh);
	if (!ret)
		sbi->s_needs_recovery = dirty;
	return ret;
}

int simplefs_journal_load(struct super_block *sb)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_journal *journal;
	journal_t *jbd2;
	uint32_t journal_blocks;
	int ret;

	if (sbi->s_journal_mode == 1 || !sbi->s_journal_present) {
		sbi->s_journal = NULL;
		return 0;
	}

	journal = kzalloc(sizeof(*journal), GFP_KERNEL);
	if (!journal)
		return -ENOMEM;
	journal->j_sb = sb;
	mutex_init(&journal->j_handle_mutex);

	journal_blocks = sbi->nr_blocks - sbi->s_journal_start;
	jbd2 = jbd2_journal_init_dev(sb->s_bdev, sb->s_bdev,
				     sbi->s_journal_start, journal_blocks,
				     sb->s_blocksize);
	if (IS_ERR(jbd2)) {
		ret = PTR_ERR(jbd2);
		journal->j_jbd2 = NULL;
		goto fail;
	}
	journal->j_jbd2 = jbd2;
	jbd2->j_private = sb;
	/* jbd2_journal_init_dev() treats post-superblock journal blocks as
	 * identity-mapped external-device blocks.  SimpleFS reserves the journal
	 * at the tail of the filesystem device, so translate every logical log
	 * block into that reserved physical range. */
	jbd2->j_bmap = simplefs_journal_bmap;

	/* norecovery is a read-only forensic view.  Initialising JBD2 validates
	 * its superblock, but loading it would replay and reset the log. */
	if (sbi->s_journal_mode == 2) {
		jbd2_journal_destroy(journal->j_jbd2);
		journal->j_jbd2 = NULL;
		sbi->s_journal = journal;
		return 0;
	}

	/* A clean filesystem can be inspected on a write-protected device.  A
	 * dirty journal cannot be replayed there; generic/050 expects -EROFS. */
	if (bdev_read_only(sb->s_bdev)) {
		if (sbi->s_needs_recovery) {
			ret = -EROFS;
			goto fail;
		}
		jbd2_journal_destroy(journal->j_jbd2);
		journal->j_jbd2 = NULL;
		sbi->s_journal = journal;
		return 0;
	}

	jbd2->j_flags |= JBD2_BARRIER;
	ret = jbd2_journal_load(jbd2);
	if (ret)
		goto fail;

	sbi->s_journal = journal;
	/* Writable mounts remain recovery-required until put_super completes a
	 * clean sync.  A read-only mount on a writable device may replay an old
	 * log, so clear the old marker after successful recovery. */
	if (!sb_rdonly(sb))
		ret = simplefs_journal_set_needs_recovery(sb, true);
	else if (sbi->s_needs_recovery)
		ret = simplefs_journal_set_needs_recovery(sb, false);
	if (ret) {
		sbi->s_journal = NULL;
		goto fail;
	}
	return 0;

fail:
	simplefs_journal_free(journal);
	return ret;
}

void simplefs_journal_destroy(struct super_block *sb)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);

	simplefs_journal_free(sbi->s_journal);
	sbi->s_journal = NULL;
}

static struct simplefs_handle *
simplefs_journal_start_internal(struct super_block *sb,
				struct simplefs_journal *journal, int blocks)
{
	struct simplefs_handle *handle;
	handle_t *jbd2_handle;
	int credits = max(blocks, 64);

	if (!journal || !journal->j_jbd2)
		return NULL;
	if (simplefs_is_shutdown(sb))
		return ERR_PTR(-EIO);
	/* Nested bitmap/metadata helpers join the outer serialized handle. */
	if (READ_ONCE(journal->j_handle_owner) == current)
		return NULL;

	mutex_lock(&journal->j_handle_mutex);
	if (simplefs_is_shutdown(sb)) {
		mutex_unlock(&journal->j_handle_mutex);
		return ERR_PTR(-EIO);
	}
	handle = kzalloc(sizeof(*handle), GFP_NOFS);
	if (!handle) {
		mutex_unlock(&journal->j_handle_mutex);
		return ERR_PTR(-ENOMEM);
	}

	jbd2_handle = jbd2__journal_start(journal->j_jbd2, credits, 0,
					   credits, GFP_NOFS, 0, 0);
	if (IS_ERR(jbd2_handle)) {
		int ret = PTR_ERR(jbd2_handle);

		if (is_journal_aborted(journal->j_jbd2))
			simplefs_journal_mark_shutdown(journal);
		kfree(handle);
		mutex_unlock(&journal->j_handle_mutex);
		return ERR_PTR(ret);
	}

	handle->h_journal = journal;
	handle->h_jbd2 = jbd2_handle;
	WRITE_ONCE(journal->j_active_handle, jbd2_handle);
	WRITE_ONCE(journal->j_handle_owner, current);
	trace_simplefs_journal_start(sb,
				     jbd2_handle->h_transaction->t_tid,
				     credits, __func__);
	return handle;
}

struct simplefs_handle *simplefs_journal_start(struct inode *inode, int blocks)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(inode->i_sb);

	return simplefs_journal_start_internal(inode->i_sb, sbi->s_journal,
					       blocks);
}

struct simplefs_handle *simplefs_journal_start_sb(struct super_block *sb,
						  int blocks)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);

	if (!sbi->s_journal || !sbi->s_journal->j_jbd2 ||
	    simplefs_current_jbd2_handle(sbi->s_journal))
		return NULL;

	return simplefs_journal_start_internal(sb, sbi->s_journal, blocks);
}

int simplefs_journal_stop(struct simplefs_handle *handle)
{
	struct simplefs_journal *journal;
	tid_t tid;
	int ret;

	if (!handle)
		return 0;

	journal = handle->h_journal;
	tid = handle->h_jbd2->h_transaction->t_tid;
	trace_simplefs_journal_stop(journal->j_sb, tid, 0);

	ret = jbd2_journal_stop(handle->h_jbd2);
	/*
	 * All writable folios now obtain JBD2 write access in get_folio() before
	 * their first mutation.  jbd2_journal_stop() may therefore return after
	 * submitting the handle; commit/checkpoint waits belong at explicit sync
	 * and unmount boundaries.  Reused metadata blocks remain protected by
	 * revoke records.
	 */
	if (ret) {
		if (is_journal_aborted(journal->j_jbd2))
			simplefs_journal_mark_shutdown(journal);
		pr_err("JBD2 transaction stop failed tid=%u ret=%d errno=%d flags=0x%lx\n",
		       tid, ret, jbd2_journal_errno(journal->j_jbd2),
		       journal->j_jbd2->j_flags);
	}
	WRITE_ONCE(journal->j_handle_owner, NULL);
	WRITE_ONCE(journal->j_active_handle, NULL);
	kfree(handle);
	mutex_unlock(&journal->j_handle_mutex);
	return ret;
}

void simplefs_journal_abort(struct simplefs_handle *handle, int err)
{
	if (!handle)
		return;
	/*
	 * This hook records a filesystem operation failure; it is not a JBD2
	 * fatal-error notification.  Marking h_aborted here does not roll back
	 * metadata already attached to the transaction.  It only makes every
	 * later access through this handle fail with -EROFS and makes stop return
	 * -EIO, which used to turn ordinary fsstress races such as -ENOENT into a
	 * cascade of misleading JBD2 failures.  JBD2 marks the journal/handle
	 * aborted itself for actual log or device failures.
	 */
	trace_simplefs_journal_abort(handle->h_journal->j_sb,
				     handle->h_jbd2->h_transaction->t_tid, err);
}

static int simplefs_jbd2_dirty(handle_t *handle, struct super_block *sb,
			       uint32_t block_nr, void *data,
			       struct folio *source_folio)
{
	struct buffer_head *bh;
	loff_t block_pos = (loff_t)block_nr << sb->s_blocksize_bits;
	int ret;

	if (source_folio &&
	    (block_pos < folio_pos(source_folio) ||
	     block_pos + sb->s_blocksize >
		folio_pos(source_folio) + folio_size(source_folio)))
		return -EFSCORRUPTED;

	bh = sb_getblk(sb, block_nr);
	if (!bh)
		return -ENOMEM;

	/* JBD2 must capture the old home-buffer image before SimpleFS publishes
	 * its folio or in-memory bitmap snapshot into that buffer. */
	ret = jbd2_journal_get_write_access(handle, bh);
	if (ret) {
		if (is_journal_aborted(handle->h_transaction->t_journal))
			simplefs_journal_mark_shutdown(
				((struct simplefs_sb_info *)SIMPLEFS_SB(sb))->s_journal);
		pr_err("JBD2 get-write-access failed dev=%s block=%u ret=%d errno=%d flags=0x%lx\n",
		       sb->s_id, block_nr, ret,
		       jbd2_journal_errno(handle->h_transaction->t_journal),
		       handle->h_transaction->t_journal->j_flags);
		goto out;
	}

	if (source_folio) {
		if (source_folio != bh->b_folio) {
			memcpy_from_folio(bh->b_data, source_folio,
				offset_in_folio(source_folio, block_pos),
				sb->s_blocksize);
		}
	} else {
		memcpy(bh->b_data, data, sb->s_blocksize);
	}
	set_buffer_uptodate(bh);

	ret = jbd2_journal_dirty_metadata(handle, bh);
	if (!ret)
		trace_simplefs_journal_dirty_metadata(sb,
					handle->h_transaction->t_tid, block_nr);
out:
	brelse(bh);
	return ret;
}

int simplefs_journal_dirty_metadata(struct simplefs_handle *handle,
				     struct super_block *sb,
				     uint32_t block_nr, void *data,
				     struct folio *folio)
{
	if (!handle)
		return 0;
	return simplefs_jbd2_dirty(handle->h_jbd2, sb, block_nr, data, folio);
}

static handle_t *simplefs_current_jbd2_handle(struct simplefs_journal *journal)
{
	handle_t *handle;

	if (!journal || !journal->j_jbd2)
		return NULL;
	handle = journal_current_handle();
	if (handle && handle->h_transaction &&
	    handle->h_transaction->t_journal == journal->j_jbd2)
		return handle;

	/* j_handle_mutex makes the active handle single-owner.  A nested helper
	 * in that same task must join it instead of recursively locking the
	 * adapter or modifying metadata outside JBD2 write access. */
	if (READ_ONCE(journal->j_handle_owner) == current)
		return READ_ONCE(journal->j_active_handle);
	return NULL;
}

int simplefs_journal_prepare_current(struct super_block *sb,
				     uint32_t block_nr)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	handle_t *handle;
	struct buffer_head *bh;
	int ret;

	/* fill_super() reads block zero before it allocates s_fs_info. */
	if (!sbi)
		return 0;
	handle = simplefs_current_jbd2_handle(sbi->s_journal);
	if (!handle)
		return 0;

	bh = sb_getblk(sb, block_nr);
	if (!bh)
		return -ENOMEM;

	ret = jbd2_journal_get_write_access(handle, bh);
	brelse(bh);
	return ret;
}

int simplefs_journal_prepare_new_blocks(struct super_block *sb,
					 uint32_t block_nr, uint32_t len)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	handle_t *handle;
	uint32_t i;

	if (!sbi || !sbi->s_journal)
		return 0;
	handle = simplefs_current_jbd2_handle(sbi->s_journal);
	if (!handle)
		return 0;

	for (i = 0; i < len; i++) {
		struct buffer_head *bh;
		struct journal_head *jh;
		bool transaction_owned = false;

		bh = sb_getblk(sb, block_nr + i);
		if (!bh)
			return -ENOMEM;

		/*
		 * Do not reuse a block whose previous journal lifetime is still
		 * active.  Checking only buffer_revoked() is insufficient: JBD2
		 * clears revoke flags when it opens the next transaction, while the
		 * old journal_head can still belong to a committing transaction's
		 * BJ_Forget list.  get_write_access() has a fast path for an already
		 * associated buffer and dirty_metadata() then correctly rejects that
		 * BJ_Forget -> BJ_Metadata transition.
		 *
		 * The buffer reference plus RCU protect b_private while the commit
		 * thread detaches/refiles the journal_head.  A stale busy result only
		 * skips this allocation candidate; the final b_bh check prevents a
		 * recycled journal_head from producing a false free result.
		 */
		rcu_read_lock();
		if (buffer_jbd(bh)) {
			jh = READ_ONCE(bh->b_private);
			if (jh) {
				transaction_owned =
					READ_ONCE(jh->b_transaction) ||
					READ_ONCE(jh->b_next_transaction);
				smp_mb();
				if (unlikely(READ_ONCE(jh->b_bh) != bh))
					transaction_owned = true;
			}
		}
		rcu_read_unlock();

		if (buffer_revoked(bh) || transaction_owned) {
			brelse(bh);
			return -EAGAIN;
		}
		brelse(bh);
	}

	return 0;
}

bool simplefs_journal_has_current_handle(struct super_block *sb)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);

	return simplefs_current_jbd2_handle(sbi->s_journal) != NULL;
}

int simplefs_journal_dirty_current(struct super_block *sb,
				   uint32_t block_nr, void *data)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_journal *journal = sbi->s_journal;
	handle_t *handle = simplefs_current_jbd2_handle(journal);

	if (!handle)
		return 0;
	return simplefs_jbd2_dirty(handle, sb, block_nr, data, NULL);
}

int simplefs_journal_dirty_folio(struct super_block *sb, uint32_t block_nr,
				 void *data, struct folio *folio)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_journal *journal = sbi->s_journal;
	handle_t *handle = simplefs_current_jbd2_handle(journal);

	if (handle) {
		return simplefs_jbd2_dirty(handle, sb, block_nr, data, folio);
	}

	/* Starting a transaction here would be too late: the caller has already
	 * changed the mapped folio.  Every writable metadata/data path must start
	 * or join a transaction before simplefs_get_folio(), which lets the JBD2
	 * adapter capture the old home image first. */
	if (!journal || !journal->j_jbd2) {
		folio_mark_dirty(folio);
		return 0;
	}

	pr_err("metadata block %u dirtied without a current journal handle\n",
	       block_nr);
	return -EUCLEAN;
}

int simplefs_journal_forget_current(struct super_block *sb,
				    uint32_t block_nr)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	handle_t *handle = simplefs_current_jbd2_handle(sbi->s_journal);
	struct buffer_head *bh;

	if (!handle)
		return -EAGAIN;
	if (handle->h_revoke_credits <= 0)
		return -ENOSPC;

	bh = sb_getblk(sb, block_nr);
	if (!bh)
		return -ENOMEM;
	if (buffer_revoked(bh)) {
		pr_err("duplicate revoke dev=%s block=%u tid=%u credits=%d\n",
		       sb->s_id, block_nr, handle->h_transaction->t_tid,
		       handle->h_revoke_credits);
	}

	/* jbd2_journal_revoke() consumes the caller's buffer reference. */
	return jbd2_journal_revoke(handle, block_nr, bh);
}

int simplefs_journal_checkpoint(struct simplefs_journal *journal)
{
	int ret;

	if (!journal || !journal->j_jbd2)
		return 0;
	mutex_lock(&journal->j_handle_mutex);
	ret = jbd2_journal_flush(journal->j_jbd2, 0);
	mutex_unlock(&journal->j_handle_mutex);
	return ret;
}

int simplefs_journal_force_commit(struct simplefs_journal *journal)
{
	struct simplefs_sb_info *sbi;
	int ret;

	if (!journal || !journal->j_jbd2)
		return 0;
	sbi = SIMPLEFS_SB(journal->j_sb);
	mutex_lock(&journal->j_handle_mutex);
	ret = jbd2_journal_force_commit(journal->j_jbd2);
	/* Free-space discard is delayed to this durability boundary.  Issuing it
	 * from every handle stop splits a shared JBD2 transaction between bitmap,
	 * extent and data-write phases.  Allocators cancel pending bits when a
	 * block is reused, so only blocks that are still free after the commit are
	 * discarded here while the handle mutex excludes new metadata users. */
	if (!ret && sbi->s_discard &&
	    !bitmap_empty(sbi->discard_pending, sbi->nr_blocks))
		ret = simplefs_issue_pending_discards(journal->j_sb);
	/* A concurrent fsync can commit the shared running transaction before
	 * this caller reaches its own force-commit.  JBD2 then has no new commit
	 * record to write and is not required to emit another cache flush, even
	 * though this caller has just completed regular-file data writeback.
	 * Always close the durability boundary explicitly. */
	if (!ret)
		ret = blkdev_issue_flush(journal->j_sb->s_bdev);
	mutex_unlock(&journal->j_handle_mutex);
	return ret;
}

void simplefs_journal_shutdown(struct simplefs_journal *journal)
{
	if (!journal || !journal->j_jbd2 ||
	    is_journal_aborted(journal->j_jbd2))
		return;

	/* Match JBD2 filesystem forced-shutdown semantics: LOGFLUSH commits the
	 * log before this call, while NOLOGFLUSH deliberately does not.  In both
	 * cases aborting prevents jbd2_journal_destroy() from treating the later
	 * unmount as a clean lifecycle boundary and marking the journal empty.
	 * -ESHUTDOWN tells recovery that this was requested, not corruption. */
	jbd2_journal_abort(journal->j_jbd2, -ESHUTDOWN);
}

void simplefs_journal_dump(struct simplefs_journal *journal)
{
	struct simplefs_sb_info *sbi;

	if (!journal || !journal->j_jbd2) {
		pr_info("journal inactive\n");
		return;
	}
	sbi = SIMPLEFS_SB(journal->j_sb);
	pr_info("JBD2 journal start=%u tail=%lu\n", sbi->s_journal_start,
		journal->j_jbd2->j_tail);
}
