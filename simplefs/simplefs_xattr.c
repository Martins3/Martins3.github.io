#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/xattr.h>
#include <linux/slab.h>
#include <linux/highmem.h>

#include "simplefs_bitmap.h"
#include "simplefs.h"

static int simplefs_xattr_validate(const char *block_data)
{
	const struct simplefs_xattr_header *hdr;
	size_t entries_end = sizeof(*hdr);
	size_t values_start = SIMPLEFS_BLOCK_SIZE;
	u32 i;

	hdr = (const struct simplefs_xattr_header *)block_data;
	if (hdr->h_magic != SIMPLEFS_XATTR_MAGIC)
		return -EUCLEAN;
	if (hdr->h_count >
	    (SIMPLEFS_BLOCK_SIZE - sizeof(*hdr)) /
		    sizeof(struct simplefs_xattr_entry))
		return -EUCLEAN;

	for (i = 0; i < hdr->h_count; i++) {
		const struct simplefs_xattr_entry *entry;
		size_t entry_size;

		if (entries_end >
		    SIMPLEFS_BLOCK_SIZE - sizeof(struct simplefs_xattr_entry))
			return -EUCLEAN;
		entry = (const struct simplefs_xattr_entry *)(block_data +
							      entries_end);
		entry_size = SIMPLEFS_XATTR_ENTRY_SIZE(entry->e_name_len);
		if (entry_size > SIMPLEFS_BLOCK_SIZE - entries_end)
			return -EUCLEAN;
		if (entry->e_value_offs > SIMPLEFS_BLOCK_SIZE ||
		    entry->e_value_size >
			    SIMPLEFS_BLOCK_SIZE - entry->e_value_offs)
			return -EUCLEAN;

		entries_end += entry_size;
		values_start = min_t(size_t, values_start,
				     entry->e_value_offs);
	}

	if (entries_end > values_start)
		return -EUCLEAN;
	return 0;
}

/*
 * Get xattr value from on-disk xattr block.
 * Returns value size on success, -ENODATA if not found, or negative error.
 * If buffer is NULL, returns the size of the value.
 */
int simplefs_xattr_get(struct inode *inode, const char *name,
		       void *buffer, size_t size)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct simplefs_xattr_header *hdr;
	struct simplefs_xattr_entry *entry;
	struct folio *folio;
	char *block_data;
	int i, name_len, ret = -ENODATA;

	if (!ci->i_xattr_block)
		return -ENODATA;

	name_len = strlen(name);
	if (name_len > U8_MAX)
		return -ERANGE;

	block_data = simplefs_get_folio(sb, ci->i_xattr_block, &folio);
	if (IS_ERR(block_data))
		return PTR_ERR(block_data);

	ret = simplefs_xattr_validate(block_data);
	if (ret)
		goto out;
	ret = -ENODATA;
	hdr = (struct simplefs_xattr_header *)block_data;

	entry = SIMPLEFS_XATTR_FIRST(hdr);
	for (i = 0; i < hdr->h_count; i++) {
		if (entry->e_name_len == name_len &&
		    memcmp(entry->e_name, name, name_len) == 0) {
			if (!buffer) {
				ret = entry->e_value_size;
				goto out;
			}
			if (size < entry->e_value_size) {
				ret = -ERANGE;
				goto out;
			}
			memcpy(buffer, block_data + entry->e_value_offs,
			       entry->e_value_size);
			ret = entry->e_value_size;
			goto out;
		}
		entry = (struct simplefs_xattr_entry *)((char *)entry +
			SIMPLEFS_XATTR_ENTRY_SIZE(entry->e_name_len));
	}

out:
	folio_release_kmap(folio, block_data);
	return ret;
}

/*
 * Set or remove xattr in on-disk xattr block.
 * If value is NULL, remove the xattr.
 * Flags: XATTR_CREATE (fail if exists), XATTR_REPLACE (fail if not exists).
 */
int simplefs_xattr_set(struct inode *inode, const char *name,
		       const void *value, size_t size, int flags)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_handle *handle;
	struct simplefs_xattr_header *hdr;
	struct simplefs_xattr_entry *entry;
	struct folio *folio;
	char *block_data;
	int i, name_len, found = -1, ret = 0, stop_ret;
	size_t entries_end, values_start;
	uint32_t bno;

	name_len = strlen(name);
	if (name_len > U8_MAX)
		return -ERANGE;
	if (!ci->i_xattr_block && !value)
		return (flags & XATTR_REPLACE) ? -ENODATA : 0;

	/* VFS xattr callbacks are not necessarily nested in a namespace
	 * transaction.  Establish one before allocation/get_folio so JBD2 owns
	 * the old block image before any in-place rebuild. */
	handle = simplefs_journal_start_sb(sb, 3);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	/* If no xattr block yet, allocate one */
	if (!ci->i_xattr_block) {
		bno = get_free_blocks(sbi, 1);
		if (!bno) {
			ret = -ENOSPC;
			goto out_stop;
		}

		block_data = simplefs_get_folio(sb, bno, &folio);
		if (IS_ERR(block_data)) {
			put_blocks(sbi, bno, 1);
			ret = PTR_ERR(block_data);
			goto out_stop;
		}

		memset(block_data, 0, SIMPLEFS_BLOCK_SIZE);
		hdr = (struct simplefs_xattr_header *)block_data;
		hdr->h_magic = SIMPLEFS_XATTR_MAGIC;
		hdr->h_count = 0;

		ci->i_xattr_block = bno;
		inode->i_blocks += simplefs_blocks_to_sectors(1);
		pr_debug("new xattr ino=%llu root=%u xattr=%u\n",
			 (unsigned long long)inode->i_ino, ci->ei_block,
			 ci->i_xattr_block);
		mark_inode_dirty(inode);
	} else {
		block_data = simplefs_get_folio(sb, ci->i_xattr_block, &folio);
		if (IS_ERR(block_data)) {
			ret = PTR_ERR(block_data);
			goto out_stop;
		}

		if (simplefs_xattr_validate(block_data)) {
			folio_release_kmap(folio, block_data);
			ret = -EUCLEAN;
			goto out_stop;
		}
		hdr = (struct simplefs_xattr_header *)block_data;
	}

	/* Find existing entry and compute layout boundaries */
	entry = SIMPLEFS_XATTR_FIRST(hdr);
	entries_end = sizeof(struct simplefs_xattr_header);
	values_start = SIMPLEFS_BLOCK_SIZE;

	for (i = 0; i < hdr->h_count; i++) {
		if (entry->e_value_offs < values_start)
			values_start = entry->e_value_offs;
		if (entry->e_name_len == name_len &&
		    memcmp(entry->e_name, name, name_len) == 0)
			found = i;
		entries_end += SIMPLEFS_XATTR_ENTRY_SIZE(entry->e_name_len);
		entry = (struct simplefs_xattr_entry *)((char *)SIMPLEFS_XATTR_FIRST(hdr) + entries_end - sizeof(struct simplefs_xattr_header));
	}

	/* Check flags */
	if (found >= 0 && (flags & XATTR_CREATE)) {
		folio_release_kmap(folio, block_data);
		ret = -EEXIST;
		goto out_stop;
	}
	if (found < 0 && (flags & XATTR_REPLACE)) {
		folio_release_kmap(folio, block_data);
		ret = -ENODATA;
		goto out_stop;
	}

	/*
	 * Rebuild the xattr block: copy all existing entries except the one
	 * being replaced, then add the new entry if value != NULL.
	 * This is simple but sufficient for a teaching filesystem.
	 */
	{
		char *tmp;
		struct simplefs_xattr_header *new_hdr;
		struct simplefs_xattr_entry *src, *dst;
		size_t new_entries_end, new_values_start;
		int j;

		tmp = kzalloc(SIMPLEFS_BLOCK_SIZE, GFP_KERNEL);
		if (!tmp) {
			folio_release_kmap(folio, block_data);
			ret = -ENOMEM;
			goto out_stop;
		}

		new_hdr = (struct simplefs_xattr_header *)tmp;
		new_hdr->h_magic = SIMPLEFS_XATTR_MAGIC;
		new_hdr->h_count = 0;

		new_entries_end = sizeof(struct simplefs_xattr_header);
		new_values_start = SIMPLEFS_BLOCK_SIZE;

		/* Copy existing entries, skipping the one being replaced */
		src = SIMPLEFS_XATTR_FIRST(hdr);
		for (j = 0; j < hdr->h_count; j++) {
			if (j == found) {
				src = (struct simplefs_xattr_entry *)((char *)src +
					SIMPLEFS_XATTR_ENTRY_SIZE(src->e_name_len));
				continue;
			}

			size_t entry_size = SIMPLEFS_XATTR_ENTRY_SIZE(src->e_name_len);

			/* Check space for entry + value (avoid unsigned underflow) */
			if (src->e_value_size > new_values_start ||
			    new_entries_end + entry_size > new_values_start - src->e_value_size) {
				kfree(tmp);
				folio_release_kmap(folio, block_data);
				ret = -ENOSPC;
				goto out_stop;
			}
			new_values_start -= src->e_value_size;

			dst = (struct simplefs_xattr_entry *)(tmp + new_entries_end);
			dst->e_name_len = src->e_name_len;
			dst->e_name_index = 0;
			dst->e_value_size = src->e_value_size;
			dst->e_value_offs = new_values_start;
			memcpy(dst->e_name, src->e_name, src->e_name_len);
			memcpy(tmp + new_values_start,
			       block_data + src->e_value_offs,
			       src->e_value_size);

			new_entries_end += entry_size;
			new_hdr->h_count++;

			src = (struct simplefs_xattr_entry *)((char *)src + entry_size);
		}

		/* Add new entry if value is provided (including zero-length) */
		if (value) {
			size_t entry_size = SIMPLEFS_XATTR_ENTRY_SIZE(name_len);

			if (size > new_values_start ||
			    new_entries_end + entry_size > new_values_start - size) {
				kfree(tmp);
				folio_release_kmap(folio, block_data);
				ret = -ENOSPC;
				goto out_stop;
			}
			new_values_start -= size;

			dst = (struct simplefs_xattr_entry *)(tmp + new_entries_end);
			dst->e_name_len = name_len;
			dst->e_name_index = 0;
			dst->e_value_size = size;
			dst->e_value_offs = new_values_start;
			memcpy(dst->e_name, name, name_len);
			memcpy(tmp + new_values_start, value, size);

			new_entries_end += entry_size;
			new_hdr->h_count++;
		}

		memcpy(block_data, tmp, SIMPLEFS_BLOCK_SIZE);
		kfree(tmp);
	}

	i = simplefs_journal_dirty_folio(sb, ci->i_xattr_block, block_data,
					 folio);
	if (i) {
		folio_release_kmap(folio, block_data);
		ret = i;
		goto out_stop;
	}
	folio_release_kmap(folio, block_data);

	/* Update ctime when xattr is modified */
	inode_set_ctime_current(inode);
	mark_inode_dirty(inode);


out_stop:
	stop_ret = simplefs_journal_stop(handle);
	if (!ret)
		ret = stop_ret;
	return ret;
}

/* Free the xattr block when an inode is deleted */
void simplefs_xattr_delete_inode(struct inode *inode)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);

	if (ci->i_xattr_block) {
		simplefs_retire_metadata_blocks(inode->i_sb,
						ci->i_xattr_block, 1);
		ci->i_xattr_block = 0;
	}
}

/* xattr handler callbacks */
static int simplefs_xattr_handler_get(const struct xattr_handler *handler,
				      struct dentry *dentry,
				      struct inode *inode, const char *name,
				      void *buffer, size_t size)
{
	return simplefs_xattr_get(inode, xattr_full_name(handler, name),
				  buffer, size);
}

static int simplefs_xattr_handler_set(const struct xattr_handler *handler,
				      struct mnt_idmap *idmap,
				      struct dentry *dentry,
				      struct inode *inode, const char *name,
				      const void *value, size_t size,
				      int flags)
{
	if (!value)
		return simplefs_xattr_set(inode,
					  xattr_full_name(handler, name),
					  NULL, 0, XATTR_REPLACE);
	return simplefs_xattr_set(inode, xattr_full_name(handler, name),
				  value, size, flags);
}

static const struct xattr_handler simplefs_xattr_user_handler = {
	.prefix = XATTR_USER_PREFIX,
	.get = simplefs_xattr_handler_get,
	.set = simplefs_xattr_handler_set,
};

static bool simplefs_xattr_trusted_list(struct dentry *dentry)
{
	return capable(CAP_SYS_ADMIN);
}

static const struct xattr_handler simplefs_xattr_trusted_handler = {
	.prefix = XATTR_TRUSTED_PREFIX,
	.list = simplefs_xattr_trusted_list,
	.get = simplefs_xattr_handler_get,
	.set = simplefs_xattr_handler_set,
};

static const struct xattr_handler simplefs_xattr_security_handler = {
	.prefix = XATTR_SECURITY_PREFIX,
	.get = simplefs_xattr_handler_get,
	.set = simplefs_xattr_handler_set,
};

const struct xattr_handler * const simplefs_xattr_handlers[] = {
	&simplefs_xattr_user_handler,
	&simplefs_xattr_trusted_handler,
	&simplefs_xattr_security_handler,
	NULL,
};

ssize_t simplefs_listxattr(struct dentry *dentry, char *buffer, size_t size)
{
	struct inode *inode = d_inode(dentry);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct simplefs_xattr_header *hdr;
	struct simplefs_xattr_entry *entry;
	struct folio *folio;
	char *block_data;
	ssize_t ret = 0;
	int i;

	if (!ci->i_xattr_block)
		return 0;

	block_data = simplefs_get_folio(sb, ci->i_xattr_block, &folio);
	if (IS_ERR(block_data))
		return PTR_ERR(block_data);

	ret = simplefs_xattr_validate(block_data);
	if (ret) {
		folio_release_kmap(folio, block_data);
		return ret;
	}
	hdr = (struct simplefs_xattr_header *)block_data;

	entry = SIMPLEFS_XATTR_FIRST(hdr);
	for (i = 0; i < hdr->h_count; i++) {
		size_t full_len = entry->e_name_len + 1; /* name + '\0' */

		if (buffer) {
			if (ret + full_len > size) {
				ret = -ERANGE;
				goto out;
			}
			memcpy(buffer + ret, entry->e_name, entry->e_name_len);
			buffer[ret + entry->e_name_len] = '\0';
		}
		ret += full_len;

		entry = (struct simplefs_xattr_entry *)((char *)entry +
			SIMPLEFS_XATTR_ENTRY_SIZE(entry->e_name_len));
	}

out:
	folio_release_kmap(folio, block_data);
	return ret;
}
