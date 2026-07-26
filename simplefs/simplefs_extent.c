#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/slab.h>

#include "simplefs.h"
#include "simplefs_bitmap.h"

static void simplefs_extent_log_leaf(struct super_block *sb, uint32_t leaf_block,
				     struct simplefs_extent_leaf *leaf,
				     const char *tag)
{
	pr_warn("%s sb=%s leaf=%u magic=0x%x entries=%u max=%u first=[%u,%u,%u] raw_len=0x%x\n",
		tag, sb->s_id, leaf_block, leaf->header.eh_magic,
		leaf->header.eh_entries, leaf->header.eh_max,
		leaf->extents[0].ee_block, leaf->extents[0].ee_start,
		simplefs_ext_len(&leaf->extents[0]), leaf->extents[0].ee_len);
}

void simplefs_extent_clear(struct simplefs_extent *ext)
{
	ext->ee_start = 0;
	ext->ee_len = 0;
	ext->ee_block = 0;
}

static int simplefs_file_grow_extent_buffer(struct simplefs_extent_buffer *buf,
					    uint32_t min_capacity)
{
	struct simplefs_extent *new_extents;
	uint32_t new_capacity;

	if (buf->capacity >= min_capacity)
		return 0;

	new_capacity = buf->capacity ? buf->capacity : SIMPLEFS_FILE_EXTENTS_PER_LEAF;
	while (new_capacity < min_capacity)
		new_capacity *= 2;

	new_extents = kvmalloc_array(new_capacity, sizeof(*new_extents),
				     GFP_KERNEL);
	if (!new_extents)
		return -ENOMEM;

	if (buf->nr_extents)
		memcpy(new_extents, buf->extents,
		       buf->nr_extents * sizeof(*new_extents));
	if (buf->extents)
		kvfree(buf->extents);

	buf->extents = new_extents;
	buf->capacity = new_capacity;
	return 0;
}

int simplefs_file_append_extent(struct simplefs_extent_buffer *buf,
				const struct simplefs_extent *ext)
{
	int ret;

	ret = simplefs_file_grow_extent_buffer(buf, buf->nr_extents + 1);
	if (ret)
		return ret;

	buf->extents[buf->nr_extents++] = *ext;
	return 0;
}

void simplefs_file_destroy_extents(struct simplefs_extent_buffer *buf)
{
	kvfree(buf->extents);
	kvfree(buf->leaf_blocks);
	memset(buf, 0, sizeof(*buf));
}

void simplefs_file_init_extent_root(void *block)
{
	struct simplefs_extent_root *root = block;

	memset(root, 0, sizeof(*root));
	root->header.eh_magic = SIMPLEFS_EXTENT_ROOT_MAGIC;
	root->header.eh_max = SIMPLEFS_FILE_INDEX_SLOTS;
}

static int simplefs_file_load_leaf(struct super_block *sb, uint32_t leaf_block,
				   struct simplefs_extent_buffer *buf)
{
	struct folio *folio;
	struct simplefs_extent_leaf *leaf;
	uint32_t i;
	int ret;

	leaf = simplefs_get_folio(sb, leaf_block, &folio);
	if (IS_ERR(leaf))
		return PTR_ERR(leaf);

	if (leaf->header.eh_magic != SIMPLEFS_EXTENT_LEAF_MAGIC) {
		simplefs_extent_log_leaf(sb, leaf_block, leaf,
					 "load-leaf bad magic");
		/* 与 extent root 同理：全零的 leaf 只可能来自失效的底层
		 * 设备，按 I/O 错误上抛，不能静默当成空 leaf。
		 */
		ret = leaf->header.eh_magic == 0 ? -EIO : -EINVAL;
		goto out;
	}

	if (leaf->header.eh_entries > SIMPLEFS_FILE_EXTENTS_PER_LEAF) {
		simplefs_extent_log_leaf(sb, leaf_block, leaf,
					 "load-leaf bad entries");
		ret = -EINVAL;
		goto out;
	}

	ret = simplefs_file_grow_extent_buffer(buf,
					       buf->nr_extents +
						       leaf->header.eh_entries);
	if (ret)
		goto out;

	for (i = 0; i < leaf->header.eh_entries; i++)
		buf->extents[buf->nr_extents++] = leaf->extents[i];

	ret = 0;
out:
	folio_release_kmap(folio, leaf);
	return ret;
}

int simplefs_file_load_extents(struct super_block *sb, uint32_t root_block,
			       struct simplefs_extent_buffer *buf)
{
	struct folio *folio;
	struct simplefs_extent_root *root;
	uint16_t nr_entries;
	uint32_t i;
	int ret = 0;

	memset(buf, 0, sizeof(*buf));

	root = simplefs_get_folio(sb, root_block, &folio);
	if (IS_ERR(root))
		return PTR_ERR(root);

	if (root->header.eh_magic != SIMPLEFS_EXTENT_ROOT_MAGIC) {
		pr_warn("load-extents bad root sb=%s root=%u magic=0x%x entries=%u max=%u first_index=[%u,%u,%u,%u]\n",
			sb->s_id, root_block, root->header.eh_magic,
			root->header.eh_entries, root->header.eh_max,
			root->indexes[0].ei_block, root->indexes[0].leaf_block,
			root->indexes[0].nr_extents, root->indexes[0].last_block);
		/* 普通文件的 extent root 在创建时必然写入 magic，读回全零
		 * 只可能是底层设备失效（generic/731 的设备移除场景），必须
		 * 当作 I/O 错误上抛。当成空树继续会让后续读取把整棵
		 * extent 树误判为 hole，向用户返回静默的零数据。
		 */
		if (root->header.eh_magic == 0 && root->header.eh_entries == 0) {
			ret = -EIO;
			goto out;
		}
		ret = -EINVAL;
		goto out;
	}

	nr_entries = READ_ONCE(root->header.eh_entries);
	smp_rmb();

	if (nr_entries > SIMPLEFS_FILE_INDEX_SLOTS) {
		pr_warn("load-extents bad root entries sb=%s root=%u entries=%u max=%u first_index=[%u,%u,%u,%u]\n",
			sb->s_id, root_block, nr_entries,
			root->header.eh_max, root->indexes[0].ei_block,
			root->indexes[0].leaf_block, root->indexes[0].nr_extents,
			root->indexes[0].last_block);
		ret = -EINVAL;
		goto out;
	}

	if (nr_entries) {
		buf->leaf_blocks = kvmalloc_array(nr_entries,
						  sizeof(*buf->leaf_blocks),
						  GFP_KERNEL);
		if (!buf->leaf_blocks) {
			ret = -ENOMEM;
			goto out;
		}
	}

	buf->nr_leaf_blocks = nr_entries;
	for (i = 0; i < nr_entries; i++) {
		buf->leaf_blocks[i] = root->indexes[i].leaf_block;
		ret = simplefs_file_load_leaf(sb, root->indexes[i].leaf_block, buf);
		if (ret)
			goto out_err;
	}

	ret = 0;
	goto out;

out_err:
	simplefs_file_destroy_extents(buf);
out:
	folio_release_kmap(folio, root);
	return ret;
}

int simplefs_file_find_extent(struct simplefs_extent_buffer *buf, uint32_t iblock,
			      uint32_t *extent_idx, uint32_t *insert_idx)
{
	uint32_t i;
	uint32_t pos = buf->nr_extents;

	for (i = 0; i < buf->nr_extents; i++) {
		struct simplefs_extent *ext = &buf->extents[i];

		if (iblock < ext->ee_block) {
			pos = i;
			break;
		}
		if (iblock >= ext->ee_block && iblock < simplefs_ext_end(ext)) {
			if (extent_idx)
				*extent_idx = i;
			if (insert_idx)
				*insert_idx = i;
			return 1;
		}
	}

	if (insert_idx)
		*insert_idx = pos;
	return 0;
}

int simplefs_file_remove_extent(struct simplefs_extent_buffer *buf,
				uint32_t extent_idx)
{
	if (extent_idx >= buf->nr_extents)
		return -EINVAL;

	if (extent_idx + 1 < buf->nr_extents)
		memmove(&buf->extents[extent_idx], &buf->extents[extent_idx + 1],
			(buf->nr_extents - extent_idx - 1) *
				sizeof(buf->extents[0]));
	buf->nr_extents--;
	return 0;
}

void simplefs_file_normalize_extents(struct simplefs_extent_buffer *buf)
{
	uint32_t i, j;

	for (i = 0; i < buf->nr_extents; i++) {
		for (j = i + 1; j < buf->nr_extents; j++) {
			if (buf->extents[i].ee_block > buf->extents[j].ee_block)
				swap(buf->extents[i], buf->extents[j]);
		}
	}

	for (i = 0; i < buf->nr_extents;) {
		struct simplefs_extent *cur = &buf->extents[i];

		if (simplefs_extent_is_empty(cur)) {
			simplefs_file_remove_extent(buf, i);
			continue;
		}

		if (i + 1 < buf->nr_extents) {
			struct simplefs_extent *next = &buf->extents[i + 1];

			if (simplefs_ext_unwritten(cur) ==
				    simplefs_ext_unwritten(next) &&
			    simplefs_ext_end(cur) == next->ee_block &&
			    cur->ee_start + simplefs_ext_len(cur) ==
				    next->ee_start &&
			    simplefs_ext_len(cur) + simplefs_ext_len(next) <=
				    SIMPLEFS_MAX_BLOCKS_PER_EXTENT) {
				simplefs_ext_set_len(cur,
						     simplefs_ext_len(cur) +
							     simplefs_ext_len(next));
				simplefs_file_remove_extent(buf, i + 1);
				continue;
			}
		}

		i++;
	}
}

static void simplefs_file_init_extent_leaf(struct simplefs_extent_leaf *leaf)
{
	memset(leaf, 0, sizeof(*leaf));
	leaf->header.eh_magic = SIMPLEFS_EXTENT_LEAF_MAGIC;
	leaf->header.eh_max = SIMPLEFS_FILE_EXTENTS_PER_LEAF;
}

static int simplefs_writeback_metadata_block(struct super_block *sb,
					     uint32_t block)
{
	loff_t start = (loff_t)block << sb->s_blocksize_bits;

	return filemap_write_and_wait_range(sb->s_bdev->bd_mapping, start,
					    start + sb->s_blocksize - 1);
}

int simplefs_file_sync_extents(struct inode *inode,
			       struct simplefs_extent_buffer *buf)
{
	struct super_block *sb = inode->i_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct simplefs_handle *handle;
	struct folio *root_folio;
	struct simplefs_extent_root *root;
	uint32_t old_leaf_count = buf->nr_leaf_blocks;
	uint32_t new_leaf_count;
	uint32_t *leaf_blocks = NULL;
	bool root_updated = false;
	uint32_t i;
	int ret = 0, stop_ret;

	simplefs_file_normalize_extents(buf);
	if (buf->nr_extents > SIMPLEFS_MAX_FILE_EXTENTS)
		return -EFBIG;

	new_leaf_count = DIV_ROUND_UP(buf->nr_extents, SIMPLEFS_FILE_EXTENTS_PER_LEAF);
	if (new_leaf_count) {
		leaf_blocks = kvmalloc_array(new_leaf_count, sizeof(*leaf_blocks),
					     GFP_KERNEL);
		if (!leaf_blocks)
			return -ENOMEM;
	}

	/* Extent writeback is also called from standalone writeback/truncate
	 * paths.  Join or start a transaction before allocating or mapping any
	 * metadata block so get_folio() obtains write access before mutation. */
	handle = simplefs_journal_start_sb(
		sb, new_leaf_count + old_leaf_count + 8);
	if (IS_ERR(handle)) {
		kvfree(leaf_blocks);
		return PTR_ERR(handle);
	}

	for (i = 0; i < min(old_leaf_count, new_leaf_count); i++)
		leaf_blocks[i] = buf->leaf_blocks[i];

	for (i = old_leaf_count; i < new_leaf_count; i++) {
		leaf_blocks[i] = get_free_blocks(sbi, 1);
		if (!leaf_blocks[i]) {
			ret = -ENOSPC;
			goto out;
		}
	}

	for (i = 0; i < buf->nr_extents; i++) {
		uint32_t start = buf->extents[i].ee_start;
		uint32_t end = start + simplefs_ext_len(&buf->extents[i]);
		uint32_t j;

		if (ci->ei_block >= start && ci->ei_block < end) {
			pr_warn("extent ownership overlap ino=%llu root=%u logical=%u physical=[%u,%u)\n",
				(unsigned long long)inode->i_ino, ci->ei_block,
				buf->extents[i].ee_block, start, end);
			ret = -EUCLEAN;
			goto out;
		}
		for (j = 0; j < new_leaf_count; j++) {
			if (leaf_blocks[j] >= start && leaf_blocks[j] < end) {
				pr_warn("extent ownership overlap ino=%llu leaf=%u logical=%u physical=[%u,%u)\n",
					(unsigned long long)inode->i_ino,
					leaf_blocks[j], buf->extents[i].ee_block,
					start, end);
				ret = -EUCLEAN;
				goto out;
			}
		}
	}

	root = simplefs_get_folio(sb, ci->ei_block, &root_folio);
	if (IS_ERR(root)) {
		ret = PTR_ERR(root);
		goto out;
	}

	pr_debug("sync-extents ino=%llu root=%u nr_extents=%u old_leaf=%u new_leaf=%u i_blocks=%llu\n",
		 (unsigned long long)inode->i_ino, ci->ei_block,
		 buf->nr_extents, old_leaf_count,
		 new_leaf_count, (unsigned long long)inode->i_blocks);

	for (i = 0; i < new_leaf_count; i++) {
		struct folio *leaf_folio;
		struct simplefs_extent_leaf *leaf;
		uint32_t start = i * SIMPLEFS_FILE_EXTENTS_PER_LEAF;
		uint32_t nr = min_t(uint32_t,
				    buf->nr_extents - start,
				    SIMPLEFS_FILE_EXTENTS_PER_LEAF);

		leaf = simplefs_get_folio(sb, leaf_blocks[i], &leaf_folio);
		if (IS_ERR(leaf)) {
			ret = PTR_ERR(leaf);
			folio_release_kmap(root_folio, root);
			goto out;
		}

		simplefs_file_init_extent_leaf(leaf);
		leaf->header.eh_entries = nr;
		if (nr)
			memcpy(leaf->extents, &buf->extents[start],
			       nr * sizeof(buf->extents[0]));
		pr_debug("sync-extents leaf ino=%llu leaf=%u slot=%u nr=%u first=[%u,%u,%u] last=[%u,%u,%u]\n",
			 (unsigned long long)inode->i_ino, leaf_blocks[i], i, nr,
			 nr ? leaf->extents[0].ee_block : 0,
			 nr ? leaf->extents[0].ee_start : 0,
			 nr ? simplefs_ext_len(&leaf->extents[0]) : 0,
			 nr ? leaf->extents[nr - 1].ee_block : 0,
			 nr ? leaf->extents[nr - 1].ee_start : 0,
			 nr ? simplefs_ext_len(&leaf->extents[nr - 1]) : 0);
		ret = simplefs_journal_dirty_folio(sb, leaf_blocks[i], leaf,
						   leaf_folio);
		if (ret) {
			folio_release_kmap(leaf_folio, leaf);
			folio_release_kmap(root_folio, root);
			goto out;
		}
		folio_release_kmap(leaf_folio, leaf);
	}

	simplefs_file_init_extent_root(root);
	for (i = 0; i < new_leaf_count; i++) {
		uint32_t start = i * SIMPLEFS_FILE_EXTENTS_PER_LEAF;
		uint32_t nr = min_t(uint32_t,
				    buf->nr_extents - start,
				    SIMPLEFS_FILE_EXTENTS_PER_LEAF);
		struct simplefs_extent *first = &buf->extents[start];
		struct simplefs_extent *last = &buf->extents[start + nr - 1];

		root->indexes[i].ei_block = first->ee_block;
		root->indexes[i].leaf_block = leaf_blocks[i];
		root->indexes[i].nr_extents = nr;
		root->indexes[i].last_block = simplefs_ext_end(last);
		pr_debug("sync-extents root-index ino=%llu slot=%u logical=[%u,%u) leaf=%u nr=%u\n",
			 (unsigned long long)inode->i_ino, i,
			 root->indexes[i].ei_block,
			 root->indexes[i].last_block, root->indexes[i].leaf_block,
			 root->indexes[i].nr_extents);
	}
	smp_wmb();
	WRITE_ONCE(root->header.eh_entries, new_leaf_count);
	ret = simplefs_journal_dirty_folio(sb, ci->ei_block, root,
					   root_folio);
	folio_release_kmap(root_folio, root);
	if (ret)
		goto out;
	root_updated = true;

	/*
	 * Publish extent ownership in dependency order.  New/updated leaves must
	 * reach disk before the root points at them.  Conversely, a removed leaf
	 * cannot return to the global allocator until the durable root no longer
	 * references it.
	 */
	if (!simplefs_journal_has_current_handle(sb)) {
		for (i = 0; i < new_leaf_count; i++) {
			ret = simplefs_writeback_metadata_block(sb, leaf_blocks[i]);
			if (ret)
				goto out;
		}
		ret = simplefs_writeback_metadata_block(sb, ci->ei_block);
		if (ret)
			goto out;
	}

	for (i = new_leaf_count; i < old_leaf_count; i++) {
		int retire_ret;

		retire_ret = simplefs_retire_metadata_blocks(
			sb, buf->leaf_blocks[i], 1);
		if (retire_ret)
			pr_warn("failed to retire extent leaf ino=%llu block=%u ret=%d\n",
				(unsigned long long)inode->i_ino,
				buf->leaf_blocks[i], retire_ret);
	}

	if (new_leaf_count >= old_leaf_count)
		inode->i_blocks += simplefs_blocks_to_sectors(
			new_leaf_count - old_leaf_count);
	else
		inode->i_blocks -= simplefs_blocks_to_sectors(
			old_leaf_count - new_leaf_count);
	kvfree(buf->leaf_blocks);
	buf->leaf_blocks = leaf_blocks;
	buf->nr_leaf_blocks = new_leaf_count;
	leaf_blocks = NULL;
	mark_inode_dirty(inode);

out:
	if (ret && !root_updated) {
		for (i = old_leaf_count; i < new_leaf_count; i++) {
			if (leaf_blocks && leaf_blocks[i])
				simplefs_retire_metadata_blocks(
					sb, leaf_blocks[i], 1);
		}
	}
	kvfree(leaf_blocks);
	stop_ret = simplefs_journal_stop(handle);
	if (!ret)
		ret = stop_ret;
	return ret;
}

int simplefs_file_free_extent_tree(struct super_block *sb, uint32_t root_block)
{
	struct simplefs_extent_buffer buf;
	uint32_t i;
	int ret, first_error = 0;

	ret = simplefs_file_load_extents(sb, root_block, &buf);
	if (ret)
		return ret;

	for (i = 0; i < buf.nr_leaf_blocks; i++) {
		ret = simplefs_retire_metadata_blocks(sb, buf.leaf_blocks[i], 1);
		if (ret && !first_error)
			first_error = ret;
	}
	ret = simplefs_retire_metadata_blocks(sb, root_block, 1);
	if (ret && !first_error)
		first_error = ret;
	simplefs_file_destroy_extents(&buf);
	return first_error;
}

/* Search for the extent containing the target block in a directory index.
 * Returns the matching extent index if found.
 * Returns the first unused file index if not found (for allocation).
 * Returns -1 if no free slots and no match.
 */
uint32_t simplefs_ext_search(struct simplefs_file_ei_block *index,
			     uint32_t iblock)
{
	uint32_t i;
	uint32_t first_free = (uint32_t)-1;

	for (i = 0; i < SIMPLEFS_MAX_EXTENTS; i++) {
		if (index->extents[i].ee_start == 0) {
			if (first_free == (uint32_t)-1)
				first_free = i;
			continue;
		}

		if (iblock >= index->extents[i].ee_block &&
		    iblock < index->extents[i].ee_block +
				 simplefs_ext_len(&index->extents[i]))
			return i;
	}

	return first_free;
}
