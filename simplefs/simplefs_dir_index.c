// SPDX-License-Identifier: GPL-2.0
/* Directory slots are stable logical offsets in an ordinary extent tree.
 * The name hash and free-slot bitmap are disposable caches, never disk truth.
 * VFS i_rwsem protects their use; dir_cache_lock serializes first construction
 * by concurrent lookups. Mutators enter with the VFS directory write lock.
 */
#include <linux/bitmap.h>
#include <linux/blkdev.h>
#include <linux/pagemap.h>
#include <linux/rhashtable.h>
#include <linux/slab.h>

#include "simplefs.h"
#include "simplefs_bitmap.h"

struct simplefs_dir_name {
	char name[SIMPLEFS_FILENAME_LEN + 1];
	struct rhash_head node;
	u32 ino;
	u32 slot;
};

static const struct rhashtable_params simplefs_name_params = {
	.head_offset = offsetof(struct simplefs_dir_name, node),
	.key_offset = offsetof(struct simplefs_dir_name, name),
	.key_len = SIMPLEFS_FILENAME_LEN + 1,
	.min_size = 4,
	.automatic_shrinking = true,
};

static void simplefs_free_name(void *ptr, void *arg)
{
	kfree(ptr);
}

static void simplefs_free_dir_index(struct simplefs_dir_index *index)
{
	if (!index)
		return;
	rhashtable_free_and_destroy(&index->names, simplefs_free_name, NULL);
	simplefs_file_destroy_extents(&index->mapping);
	kvfree(index->blocks);
	kvfree(index->entries);
	bitmap_free(index->used);
	kfree(index);
}

void simplefs_dir_destroy(struct inode *dir)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(dir);

	simplefs_free_dir_index(ci->dir_index);
	ci->dir_index = NULL;
}

static int simplefs_dir_reserve(struct simplefs_dir_index *index, u32 blocks)
{
	u32 capacity = max_t(u32, 4, index->capacity);
	u32 *map;
	struct simplefs_dir_name **entries;
	unsigned long *used;

	if (blocks > SIMPLEFS_MAX_DIR_BLOCKS)
		return -ENOSPC;
	if (blocks <= index->capacity)
		return 0;
	while (capacity < blocks)
		capacity = min_t(u32, capacity * 2, SIMPLEFS_MAX_DIR_BLOCKS);
	map = kvmalloc_array(capacity, sizeof(*map), GFP_NOFS);
	entries = kvcalloc(capacity * SIMPLEFS_FILES_PER_BLOCK,
			   sizeof(*entries), GFP_NOFS);
	used = bitmap_zalloc(capacity * SIMPLEFS_FILES_PER_BLOCK, GFP_NOFS);
	if (!map || !entries || !used) {
		kvfree(map);
		kvfree(entries);
		bitmap_free(used);
		return -ENOMEM;
	}
	if (index->capacity) {
		memcpy(map, index->blocks, index->nr_blocks * sizeof(*map));
		memcpy(entries, index->entries,
		       index->nr_blocks * SIMPLEFS_FILES_PER_BLOCK * sizeof(*entries));
		bitmap_copy(used, index->used,
			    index->nr_blocks * SIMPLEFS_FILES_PER_BLOCK);
	}
	kvfree(index->blocks);
	kvfree(index->entries);
	bitmap_free(index->used);
	index->blocks = map;
	index->entries = entries;
	index->used = used;
	index->capacity = capacity;
	return 0;
}

static int simplefs_dir_build(struct simplefs_dir_index *index)
{
	struct inode *dir = index->dir;
	struct super_block *sb = dir->i_sb;
	struct simplefs_dir_block *block;
	struct folio *folio;
	u32 i, bi, slot = 0;
	int ret;

	ret = simplefs_file_load_extents(sb, SIMPLEFS_INODE(dir)->ei_block,
					 &index->mapping);
	if (ret)
		return ret;
	for (i = 0; i < index->mapping.nr_extents; i++) {
		struct simplefs_extent *ext = &index->mapping.extents[i];
		u32 len = simplefs_ext_len(ext);

		if (!len || simplefs_ext_unwritten(ext) ||
		    ext->ee_block != index->nr_blocks ||
		    ext->ee_start >= ((struct simplefs_sb_info *)SIMPLEFS_SB(sb))->nr_blocks ||
		    len > ((struct simplefs_sb_info *)SIMPLEFS_SB(sb))->nr_blocks - ext->ee_start)
			return -EFSCORRUPTED;
		ret = simplefs_dir_reserve(index, index->nr_blocks + len);
		if (ret)
			return ret;
		for (bi = 0; bi < len; bi++)
			index->blocks[index->nr_blocks++] = ext->ee_start + bi;
	}
	for (bi = 0; bi < index->nr_blocks; bi++) {
		block = simplefs_read_metadata(sb, index->blocks[bi], &folio);
		if (IS_ERR(block))
			return PTR_ERR(block);
		for (i = 0; i < SIMPLEFS_FILES_PER_BLOCK; i++, slot++) {
			struct simplefs_dir_name *entry;
			struct simplefs_file *file = &block->files[i];

			if (!file->inode)
				continue;
			if (file->inode >= ((struct simplefs_sb_info *)SIMPLEFS_SB(sb))->nr_inodes || !file->filename[0]) {
				ret = -EFSCORRUPTED;
				goto release;
			}
			entry = kzalloc(sizeof(*entry), GFP_NOFS);
			if (!entry) {
				ret = -ENOMEM;
				goto release;
			}
			memcpy(entry->name, file->filename,
			       strnlen(file->filename, SIMPLEFS_FILENAME_LEN));
			entry->ino = file->inode;
			entry->slot = slot;
			ret = rhashtable_lookup_insert_fast(&index->names, &entry->node,
							 simplefs_name_params);
			if (ret) {
				kfree(entry);
				if (ret == -EEXIST)
					ret = -EFSCORRUPTED;
				goto release;
			}
			index->entries[slot] = entry;
			__set_bit(slot, index->used);
			index->nr_files++;
		}
		folio_release_kmap(folio, block);
		cond_resched();
	}
	index->valid = true;
	return 0;
release:
	folio_release_kmap(folio, block);
	return ret;
}

struct simplefs_dir_index *simplefs_dir_get(struct inode *dir)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(dir);
	struct simplefs_dir_index *index;
	int ret;

	if (simplefs_is_shutdown(dir->i_sb))
		return ERR_PTR(-EIO);
	mutex_lock(&ci->dir_cache_lock);
	index = ci->dir_index;
	if (index && index->valid)
		goto out;
	simplefs_dir_destroy(dir);
	index = kzalloc(sizeof(*index), GFP_NOFS);
	if (!index) {
		index = ERR_PTR(-ENOMEM);
		goto out;
	}
	index->dir = dir;
	ret = rhashtable_init(&index->names, &simplefs_name_params);
	if (ret) {
		kfree(index);
		index = ERR_PTR(ret);
		goto out;
	}
	ret = simplefs_dir_build(index);
	if (ret) {
		simplefs_free_dir_index(index);
		index = ERR_PTR(ret);
		goto out;
	}
	ci->dir_index = index;
out:
	mutex_unlock(&ci->dir_cache_lock);
	return index;
}

int simplefs_dir_find(struct simplefs_dir_index *index, const struct qstr *name,
		      u32 ino, u32 *slot, u32 *found_ino)
{
	char key[SIMPLEFS_FILENAME_LEN + 1] = {};
	struct simplefs_dir_name *entry;

	if (name->len > SIMPLEFS_FILENAME_LEN)
		return -ENAMETOOLONG;
	memcpy(key, name->name, name->len);
	entry = rhashtable_lookup_fast(&index->names, key, simplefs_name_params);
	if (!entry || (ino && ino != entry->ino))
		return -ENOENT;
	if (slot)
		*slot = entry->slot;
	if (found_ino)
		*found_ino = entry->ino;
	return 0;
}

/* Only grow when no reusable slot exists. Publish zeroed data before its
 * extent pointer, in the caller's transaction. Empty blocks stay mapped until
 * rmdir so readdir cookies never change beneath directory readers.
 */
int simplefs_dir_free_slot(struct simplefs_dir_index *index, u32 *slot)
{
	struct inode *dir = index->dir;
	struct super_block *sb = dir->i_sb;
	struct simplefs_extent ext;
	struct folio *folio;
	void *data;
	u32 slots = index->nr_blocks * SIMPLEFS_FILES_PER_BLOCK;
	u32 bno;
	int ret;

	*slot = find_next_zero_bit(index->used, slots, index->next_free);
	if (*slot < slots)
		return 0;
	ret = simplefs_dir_reserve(index, index->nr_blocks + 1);
	if (ret)
		return ret;
	bno = get_free_blocks(SIMPLEFS_SB(sb), 1);
	if (!bno)
		return simplefs_is_shutdown(sb) ? -EIO : -ENOSPC;
	data = simplefs_get_zeroed_folio(sb, bno, &folio);
	if (IS_ERR(data)) {
		ret = PTR_ERR(data);
		goto retire;
	}
	ret = simplefs_journal_dirty_folio(sb, bno, data, folio);
	folio_release_kmap(folio, data);
	if (ret)
		goto retire;
	if (!simplefs_journal_has_current_handle(sb)) {
		ret = filemap_write_and_wait_range(sb->s_bdev->bd_mapping,
			(loff_t)bno * SIMPLEFS_BLOCK_SIZE,
			((loff_t)bno + 1) * SIMPLEFS_BLOCK_SIZE - 1);
		if (ret)
			goto retire;
	}
	simplefs_ext_init(&ext, index->nr_blocks, 1, bno, false);
	ret = simplefs_file_append_extent(&index->mapping, &ext);
	if (ret)
		goto retire;
	ret = simplefs_file_sync_extents(dir, &index->mapping);
	if (ret) {
		/* The root may already have changed. Do not recycle its data. */
		index->valid = false;
		simplefs_journal_fail(sb, ret);
		return ret;
	}
	index->blocks[index->nr_blocks++] = bno;
	dir->i_blocks += simplefs_blocks_to_sectors(1);
	i_size_write(dir, (loff_t)index->nr_blocks * SIMPLEFS_BLOCK_SIZE);
	mark_inode_dirty(dir);
	*slot = slots;
	return 0;
retire:
	simplefs_retire_metadata_blocks(sb, bno, 1);
	return ret;
}

int simplefs_dir_set(struct simplefs_dir_index *index, u32 slot, u32 ino,
		     const struct qstr *name)
{
	struct super_block *sb = index->dir->i_sb;
	struct simplefs_dir_name *old, *entry = NULL;
	struct simplefs_dir_block *block;
	struct folio *folio;
	u32 bno;
	int ret;

	if (slot >= index->nr_blocks * SIMPLEFS_FILES_PER_BLOCK)
		return -EFSCORRUPTED;
	if (name && name->len > SIMPLEFS_FILENAME_LEN)
		return -ENAMETOOLONG;
	old = index->entries[slot];
	if (ino && !name && !old)
		return -ENOENT;
	if (ino && name && (!old || strlen(old->name) != name->len ||
			    memcmp(old->name, name->name, name->len))) {
		entry = kzalloc(sizeof(*entry), GFP_NOFS);
		if (!entry)
			return -ENOMEM;
		memcpy(entry->name, name->name, name->len);
		entry->ino = ino;
		entry->slot = slot;
		ret = rhashtable_lookup_insert_fast(&index->names, &entry->node,
						 simplefs_name_params);
		if (ret) {
			kfree(entry);
			return ret;
		}
	}
	bno = index->blocks[slot / SIMPLEFS_FILES_PER_BLOCK];
	block = simplefs_get_folio(sb, bno, &folio);
	if (IS_ERR(block)) {
		ret = PTR_ERR(block);
		goto undo;
	}
	memset(&block->files[slot % SIMPLEFS_FILES_PER_BLOCK], 0,
	       sizeof(struct simplefs_file));
	if (ino) {
		block->files[slot % SIMPLEFS_FILES_PER_BLOCK].inode = ino;
		memcpy(block->files[slot % SIMPLEFS_FILES_PER_BLOCK].filename,
		       entry ? entry->name : old->name, SIMPLEFS_FILENAME_LEN);
	}
	ret = simplefs_journal_dirty_folio(sb, bno, block, folio);
	folio_release_kmap(folio, block);
	if (ret) {
		index->valid = false;
		simplefs_journal_fail(sb, ret);
		goto undo;
	}
	if (old && (!ino || entry)) {
		rhashtable_remove_fast(&index->names, &old->node, simplefs_name_params);
		kfree(old);
		old = NULL;
	}
	index->entries[slot] = entry ? entry : old;
	if (ino) {
		index->entries[slot]->ino = ino;
		__set_bit(slot, index->used);
		if (slot == index->next_free)
			index->next_free++;
	} else {
		__clear_bit(slot, index->used);
		index->next_free = min(index->next_free, slot);
	}
	return 0;
undo:
	if (entry) {
		rhashtable_remove_fast(&index->names, &entry->node, simplefs_name_params);
		kfree(entry);
	}
	return ret;
}

int simplefs_dir_add(struct inode *dir, const struct qstr *name, u32 ino)
{
	struct simplefs_dir_index *index = simplefs_dir_get(dir);
	u32 slot;
	int ret;

	if (IS_ERR(index))
		return PTR_ERR(index);
	ret = simplefs_dir_free_slot(index, &slot);
	if (!ret)
		ret = simplefs_dir_set(index, slot, ino, name);
	if (!ret) {
		index->nr_files++;
		mark_inode_dirty(dir);
		ret = simplefs_persist_inode(dir);
		if (ret) {
			index->valid = false;
			simplefs_journal_fail(dir->i_sb, ret);
		}
	}
	return ret;
}

int simplefs_dir_emit(struct file *file, struct dir_context *ctx)
{
	struct simplefs_dir_index *index = simplefs_dir_get(file_inode(file));
	u32 slots;

	if (IS_ERR(index))
		return PTR_ERR(index);
	if (!dir_emit_dots(file, ctx))
		return 0;
	slots = index->nr_blocks * SIMPLEFS_FILES_PER_BLOCK;
	while (ctx->pos - 2 < slots) {
		struct simplefs_dir_name *entry = index->entries[ctx->pos - 2];

		if (entry && !dir_emit(ctx, entry->name, strlen(entry->name),
				      entry->ino, DT_UNKNOWN))
			break;
		ctx->pos++;
		cond_resched();
	}
	return 0;
}

int simplefs_dir_get_name(struct inode *dir, char *name, u32 ino)
{
	struct simplefs_dir_index *index = simplefs_dir_get(dir);
	u32 i;

	if (IS_ERR(index))
		return PTR_ERR(index);
	for (i = 0; i < index->nr_blocks * SIMPLEFS_FILES_PER_BLOCK; i++) {
		struct simplefs_dir_name *entry = index->entries[i];

		if (entry && entry->ino == ino) {
			strscpy(name, entry->name, SIMPLEFS_FILENAME_LEN + 1);
			return 0;
		}
	}
	return -ENOENT;
}

/* Retire an empty directory in bounded transactions while its parent still
 * names it. A crash between batches leaves a valid (smaller) empty directory,
 * rather than an unreachable tree whose thousands of revokes cannot fit in
 * the final rmdir transaction. VFS holds the victim directory exclusively.
 */
int simplefs_dir_trim(struct inode *dir)
{
	struct simplefs_dir_index *index = simplefs_dir_get(dir);
	struct super_block *sb = dir->i_sb;
	int ret = 0;

	if (IS_ERR(index))
		return PTR_ERR(index);
	if (index->nr_files)
		return -ENOTEMPTY;
	while (index->mapping.nr_extents) {
		struct simplefs_extent *ext;
		struct simplefs_handle *handle;
		u32 len, count, start;
		int stop_ret;

		/* Only the tail leaf/root and a bounded free range change.
		 * sync_extents skips unchanged leaves even in a large directory. */
		handle = simplefs_journal_start_sb(sb, 32);
		if (IS_ERR(handle))
			return PTR_ERR(handle);
		ext = &index->mapping.extents[index->mapping.nr_extents - 1];
		len = simplefs_ext_len(ext);
		count = min_t(u32, len, 16);
		start = ext->ee_start + len - count;
		if (count == len)
			index->mapping.nr_extents--;
		else
			simplefs_ext_set_len(ext, len - count);
		ret = simplefs_file_sync_extents(dir, &index->mapping);
		if (!ret)
			ret = simplefs_retire_metadata_blocks(sb, start, count);
		if (!ret) {
			index->nr_blocks -= count;
			dir->i_blocks -= simplefs_blocks_to_sectors(count);
			i_size_write(dir, (loff_t)index->nr_blocks * SIMPLEFS_BLOCK_SIZE);
			mark_inode_dirty(dir);
			ret = simplefs_persist_inode(dir);
		}
		if (ret) {
			index->valid = false;
			simplefs_journal_fail(sb, ret);
		}
		stop_ret = simplefs_journal_stop(handle);
		if (ret || stop_ret)
			return ret ? ret : stop_ret;
		cond_resched();
	}
	index->next_free = 0;
	return 0;
}
