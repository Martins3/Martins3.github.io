#define pr_fmt(fmt) "simplefs: " fmt

#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/iomap.h>
#include <linux/pagemap.h>
#include <linux/uio.h>
#include <linux/bvec.h>
#include <linux/falloc.h>
#include <linux/fiemap.h>
#include <linux/writeback.h>
#include <linux/workqueue.h>
#include <linux/splice.h>

#include "simplefs_bitmap.h"
#include "simplefs.h"
#include "simplefs_trace.h"
#include "simplefs_debug.h"

static int simplefs_get_block(struct inode *inode, sector_t iblock,
			      unsigned long maxblocks, u32 *bno_res, bool *new,
			      bool *unwritten, int create, bool exact_alloc,
			      bool create_unwritten,
			      bool convert_unwritten);
static int simplefs_convert_unwritten_extent(
	struct simplefs_extent_buffer *buf, uint32_t extent_idx,
	uint32_t iblock, uint32_t map_len);
static int simplefs_mark_unwritten_range(struct inode *inode,
					 uint32_t start_block,
					 uint32_t nr_blocks);
static int simplefs_discard_prealloc(struct file *file, struct inode *inode);

struct simplefs_block_range {
	uint32_t start;
	uint32_t len;
};

static inline int simplefs_iomap_zero_range_compat(struct inode *inode,
						   loff_t pos, loff_t len)
{
#ifdef IOMAP_IOEND_UNWRITTEN
	return iomap_zero_range(inode, pos, len, NULL,
				&simplefs_write_iomap_ops, NULL, NULL);
#else
	return iomap_zero_range(inode, pos, len, NULL,
				&simplefs_write_iomap_ops);
#endif
}

static inline int simplefs_iomap_truncate_page_compat(struct inode *inode,
						      loff_t pos)
{
#ifdef IOMAP_IOEND_UNWRITTEN
	return iomap_truncate_page(inode, pos, NULL,
				   &simplefs_write_iomap_ops, NULL, NULL);
#else
	return iomap_truncate_page(inode, pos, NULL,
				   &simplefs_write_iomap_ops);
#endif
}

static inline ssize_t simplefs_iomap_buffered_write_compat(struct kiocb *iocb,
							   struct iov_iter *from)
{
#ifdef IOMAP_IOEND_UNWRITTEN
	return iomap_file_buffered_write(iocb, from, &simplefs_write_iomap_ops,
					 NULL, NULL);
#else
	return iomap_file_buffered_write(iocb, from, &simplefs_write_iomap_ops,
					 NULL);
#endif
}

static inline vm_fault_t simplefs_iomap_page_mkwrite_compat(struct vm_fault *vmf)
{
#ifdef IOMAP_IOEND_UNWRITTEN
	return iomap_page_mkwrite(vmf, &simplefs_write_iomap_ops, NULL);
#else
	return iomap_page_mkwrite(vmf, &simplefs_write_iomap_ops);
#endif
}

static void simplefs_touch_mctime(struct inode *inode)
{
	struct timespec64 now = current_time(inode);
	struct timespec64 ctime = inode_get_ctime(inode);
	time64_t floor = inode->i_mtime_sec;

	if (ctime.tv_sec > floor)
		floor = ctime.tv_sec;
	if (now.tv_sec <= floor) {
		now.tv_sec = floor + 1;
		now.tv_nsec = 0;
	}

	inode_set_ctime_to_ts(inode, now);
	inode_set_mtime_to_ts(inode, now);
}

static int __maybe_unused
simplefs_convert_unwritten_range(struct inode *inode, loff_t offset, loff_t len)
{
	struct super_block *sb = inode->i_sb;
	struct simplefs_extent_buffer buf;
	unsigned int blkbits = inode->i_blkbits;
	uint32_t start_block;
	uint32_t end_block;
	int ret;
	uint32_t i;

	if (len <= 0)
		return 0;

	start_block = offset >> blkbits;
	end_block = (offset + len + (1 << blkbits) - 1) >> blkbits;
	if (start_block >= end_block)
		return 0;

	mutex_lock(&SIMPLEFS_INODE(inode)->extent_lock);
	ret = simplefs_file_load_extents(sb, SIMPLEFS_INODE(inode)->ei_block, &buf);
	if (ret) {
		pr_warn("convert-unwritten-range load-extents fail ino=%llu root=%u off=%llu len=%llu ret=%d i_size=%llu i_blocks=%llu\n",
			(unsigned long long)inode->i_ino,
			SIMPLEFS_INODE(inode)->ei_block,
			(unsigned long long)offset, (unsigned long long)len,
			ret,
			(unsigned long long)i_size_read(inode),
			(unsigned long long)inode->i_blocks);
		mutex_unlock(&SIMPLEFS_INODE(inode)->extent_lock);
		return ret;
	}

	for (i = 0; i < buf.nr_extents; i++) {
		struct simplefs_extent *ext = &buf.extents[i];
		uint32_t ext_start;
		uint32_t ext_end;
		uint32_t map_start;
		uint32_t map_len;

		if (!simplefs_ext_unwritten(ext))
			continue;

		ext_start = ext->ee_block;
		ext_end = simplefs_ext_end(ext);
		if (ext_end <= start_block || ext_start >= end_block)
			continue;

		map_start = max(ext_start, start_block);
		map_len = min(ext_end, end_block) - map_start;
		ret = simplefs_convert_unwritten_extent(&buf, i, map_start,
							map_len);
		if (ret)
			goto out;

		/*
		 * 转换可能会把当前 extent 切成 head/data/tail 三段，重新从头
		 * 扫描最稳妥，范围本身很小，优先保证正确性。
		 */
		i = 0;
	}

	ret = simplefs_file_sync_extents(inode, &buf);
out:
	simplefs_file_destroy_extents(&buf);
	mutex_unlock(&SIMPLEFS_INODE(inode)->extent_lock);
	return ret;
}

static int simplefs_prealloc_written(struct inode *inode, loff_t offset,
				     loff_t len, unsigned int blkbits)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	uint32_t write_start, write_end, pre_end;

	if (!ci->prealloc_len)
		return 0;

	write_start = offset >> blkbits;
	write_end = (offset + len + (1 << blkbits) - 1) >> blkbits;
	pre_end = ci->prealloc_block + ci->prealloc_len;

	if (write_end <= ci->prealloc_block || write_start >= pre_end)
		return 0;

	if (write_start <= ci->prealloc_block) {
		if (write_end >= pre_end) {
			ci->prealloc_block = 0;
			ci->prealloc_len = 0;
		} else {
			ci->prealloc_len = pre_end - write_end;
			ci->prealloc_block = write_end;
		}
		return 0;
	}

	if (write_end >= pre_end) {
		ci->prealloc_len = write_start - ci->prealloc_block;
		return 0;
	}

	/* 中间写入会把跟踪区间分裂成两段。额外预分配本身已经在
	 * extent 中持久化为 unwritten，所以丢掉单区间记录的头段不会
	 * 改变 SEEK_DATA/SEEK_HOLE 语义。仅跟踪尾段供 close/fsync 回收。
	 */
	ci->prealloc_len = pre_end - write_end;
	ci->prealloc_block = write_end;
	return 0;
}

static void simplefs_prealloc_collapse(struct simplefs_inode_info *ci,
				       uint32_t start_block,
				       uint32_t collapse_blocks)
{
	uint32_t pre_start;
	uint32_t pre_end;
	uint32_t cut_end;

	if (!ci->prealloc_len)
		return;

	pre_start = ci->prealloc_block;
	pre_end = pre_start + ci->prealloc_len;
	cut_end = start_block + collapse_blocks;

	if (pre_end <= start_block)
		return;

	if (pre_start >= cut_end) {
		ci->prealloc_block -= collapse_blocks;
		return;
	}

	if (pre_start < start_block && pre_end > cut_end) {
		/* 单区间跟踪无法表示 collapse 后的两段结果。保守起见
		 * 直接丢掉记录，避免把真实数据误判成 unwritten。
		 */
		ci->prealloc_block = 0;
		ci->prealloc_len = 0;
		return;
	}

	if (pre_start < start_block) {
		ci->prealloc_len = start_block - pre_start;
		return;
	}

	if (pre_end > cut_end) {
		ci->prealloc_block = start_block;
		ci->prealloc_len = pre_end - cut_end;
		return;
	}

	ci->prealloc_block = 0;
	ci->prealloc_len = 0;
}

static void simplefs_prealloc_insert(struct simplefs_inode_info *ci,
				     uint32_t start_block, uint32_t insert_blocks)
{
	uint32_t pre_start, pre_end;

	if (!ci->prealloc_len)
		return;

	pre_start = ci->prealloc_block;
	pre_end = pre_start + ci->prealloc_len;

	if (pre_end <= start_block)
		return;

	if (pre_start >= start_block) {
		ci->prealloc_block += insert_blocks;
		return;
	}

	/* 跟踪区间跨越插入点：单区间无法表示插入后断开的两段，
	 * 保守截断到插入点之前（与 collapse 的保守处理一致）。
	 */
	ci->prealloc_len = start_block - pre_start;
}

static int simplefs_file_insert_extent(struct simplefs_extent_buffer *buf,
				       uint32_t insert_idx,
				       const struct simplefs_extent *ext)
{
	int ret;

	ret = simplefs_file_append_extent(buf, ext);
	if (ret)
		return ret;

	if (insert_idx < buf->nr_extents - 1)
		memmove(&buf->extents[insert_idx + 1], &buf->extents[insert_idx],
			(buf->nr_extents - insert_idx - 1) *
				sizeof(buf->extents[0]));
	buf->extents[insert_idx] = *ext;
	return 0;
}

static int simplefs_convert_unwritten_extent(
	struct simplefs_extent_buffer *buf, uint32_t extent_idx,
	uint32_t iblock, uint32_t map_len)
{
	struct simplefs_extent orig = buf->extents[extent_idx];
	uint32_t orig_len = simplefs_ext_len(&orig);
	uint32_t head_len = iblock - orig.ee_block;
	uint32_t tail_len;
	struct simplefs_extent ext;
	int ret;

	if (!simplefs_ext_unwritten(&orig))
		return 0;

	map_len = min_t(uint32_t, map_len, orig_len - head_len);
	tail_len = orig_len - head_len - map_len;

	buf->extents[extent_idx].ee_block = iblock;
	buf->extents[extent_idx].ee_start = orig.ee_start + head_len;
	buf->extents[extent_idx].ee_len = map_len;

	if (head_len) {
		simplefs_ext_init(&ext, orig.ee_block, head_len, orig.ee_start,
				  true);
		ret = simplefs_file_insert_extent(buf, extent_idx, &ext);
		if (ret)
			return ret;
		extent_idx++;
	}

	if (tail_len) {
		simplefs_ext_init(&ext, iblock + map_len, tail_len,
				  orig.ee_start + head_len + map_len, true);
		ret = simplefs_file_insert_extent(buf, extent_idx + 1, &ext);
		if (ret)
			return ret;
	}

	simplefs_ext_set_unwritten(&buf->extents[extent_idx], false);
	/* 不在这里 normalize：拆分保序，统一由 sync_extents 归一化。
	 * 逐段 normalize 的冒泡排序是 O(n^2)，对上万段 extent 的
	 * 文件会叠加成 O(n^3)（generic/610）。 */
	return 0;
}

static int simplefs_mark_unwritten_extent(
	struct simplefs_extent_buffer *buf, uint32_t extent_idx,
	uint32_t iblock, uint32_t map_len)
{
	struct simplefs_extent orig = buf->extents[extent_idx];
	uint32_t orig_len = simplefs_ext_len(&orig);
	uint32_t head_len = iblock - orig.ee_block;
	uint32_t tail_len;
	struct simplefs_extent ext;
	int ret;

	if (simplefs_ext_unwritten(&orig))
		return 0;

	map_len = min_t(uint32_t, map_len, orig_len - head_len);
	tail_len = orig_len - head_len - map_len;

	buf->extents[extent_idx].ee_block = iblock;
	buf->extents[extent_idx].ee_start = orig.ee_start + head_len;
	buf->extents[extent_idx].ee_len = map_len;

	if (head_len) {
		simplefs_ext_init(&ext, orig.ee_block, head_len, orig.ee_start,
				  false);
		ret = simplefs_file_insert_extent(buf, extent_idx, &ext);
		if (ret)
			return ret;
		extent_idx++;
	}

	if (tail_len) {
		simplefs_ext_init(&ext, iblock + map_len, tail_len,
				  orig.ee_start + head_len + map_len, false);
		ret = simplefs_file_insert_extent(buf, extent_idx + 1, &ext);
		if (ret)
			return ret;
	}

	simplefs_ext_set_unwritten(&buf->extents[extent_idx], true);
	/* 不在这里 normalize：拆分保序，统一由 sync_extents 归一化。
	 * 逐段 normalize 的冒泡排序是 O(n^2)，对上万段 extent 的
	 * 文件会叠加成 O(n^3)（generic/610）。 */
	return 0;
}

static int simplefs_mark_unwritten_range(struct inode *inode,
					 uint32_t start_block,
					 uint32_t nr_blocks)
{
	struct simplefs_extent_buffer buf;
	uint32_t extent_idx;
	int found;
	int ret;

	if (!nr_blocks)
		return 0;

	mutex_lock(&SIMPLEFS_INODE(inode)->extent_lock);
	ret = simplefs_file_load_extents(inode->i_sb,
				 SIMPLEFS_INODE(inode)->ei_block, &buf);
	if (ret)
		goto out_unlock;

	found = simplefs_file_find_extent(&buf, start_block, &extent_idx, NULL);
	if (!found) {
		ret = -EFSCORRUPTED;
		goto out_destroy;
	}

	ret = simplefs_mark_unwritten_extent(&buf, extent_idx, start_block,
					     nr_blocks);
	if (!ret)
		ret = simplefs_file_sync_extents(inode, &buf);

out_destroy:
	simplefs_file_destroy_extents(&buf);
out_unlock:
	mutex_unlock(&SIMPLEFS_INODE(inode)->extent_lock);
	return ret;
}

/*
 * 把 [first, last) 内的完整块全部转换为 unwritten：hole 分配 unwritten
 * extent，written extent 原地转 unwritten。整个操作只加载/回写一次
 * extent 树。
 *
 * 不能逐段调用 get_block + mark_unwritten_range：那种写法每段 extent
 * 都要全量加载并回写 extent 树，对 punch-alternating 产生的上万段
 * extent 是 O(n^2)，generic/610 的 fzero 100M 因此跑不完。
 */
static int simplefs_zero_range_full_blocks(struct inode *inode,
					   uint32_t first, uint32_t last)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(inode->i_sb);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct simplefs_extent_buffer buf;
	struct simplefs_block_range *alloced;
	uint32_t nr_alloced = 0;
	uint32_t block = first;
	int ret = 0;

	alloced = kvmalloc_array(last - first, sizeof(*alloced), GFP_NOFS);
	if (!alloced)
		return -ENOMEM;

	mutex_lock(&ci->extent_lock);
	ret = simplefs_file_load_extents(inode->i_sb, ci->ei_block, &buf);
	if (ret)
		goto out_free;

	while (block < last) {
		uint32_t extent_idx = 0, insert_idx = 0;
		int found;

		found = simplefs_file_find_extent(&buf, block, &extent_idx,
						  &insert_idx);
		if (found) {
			struct simplefs_extent *ext = &buf.extents[extent_idx];
			uint32_t conv_end = min(simplefs_ext_end(ext), last);

			if (!simplefs_ext_unwritten(ext)) {
				ret = simplefs_mark_unwritten_extent(&buf,
						extent_idx, block,
						conv_end - block);
				if (ret)
					goto out_destroy;
			}
			block = conv_end;
			continue;
		}

		{
			uint32_t alloc_len = last - block;
			uint32_t bno;
			struct simplefs_extent ext;

			if (insert_idx < buf.nr_extents &&
			    buf.extents[insert_idx].ee_block < block + alloc_len)
				alloc_len = buf.extents[insert_idx].ee_block -
					    block;
			alloc_len = min_t(uint32_t, alloc_len,
					  SIMPLEFS_MAX_BLOCKS_PER_EXTENT);
			bno = get_free_blocks(sbi, alloc_len);
			while (!bno && alloc_len > 1) {
				alloc_len /= 2;
				bno = get_free_blocks(sbi, alloc_len);
			}
			if (!bno) {
				ret = -ENOSPC;
				goto out_destroy;
			}

			simplefs_ext_init(&ext, block, alloc_len, bno, true);
			ret = simplefs_file_insert_extent(&buf, insert_idx, &ext);
			if (ret) {
				put_blocks(sbi, bno, alloc_len);
				goto out_destroy;
			}
			alloced[nr_alloced].start = bno;
			alloced[nr_alloced].len = alloc_len;
			nr_alloced++;
			inode->i_blocks += simplefs_blocks_to_sectors(alloc_len);
			block += alloc_len;
		}
	}

	ret = simplefs_file_sync_extents(inode, &buf);
	if (!ret)
		goto out_destroy;

	/* sync 失败时磁盘上的 extent 树未变，把位图里的分配退回 */
	while (nr_alloced) {
		nr_alloced--;
		put_blocks(sbi, alloced[nr_alloced].start,
			   alloced[nr_alloced].len);
		inode->i_blocks -=
			simplefs_blocks_to_sectors(alloced[nr_alloced].len);
	}

out_destroy:
	simplefs_file_destroy_extents(&buf);
out_free:
	kvfree(alloced);
	mutex_unlock(&ci->extent_lock);
	return ret;
}

static int simplefs_append_extent_slice(struct simplefs_extent_buffer *buf,
					const struct simplefs_extent *src,
					uint32_t logical_start,
					uint32_t logical_len,
					uint32_t physical_start)
{
	struct simplefs_extent ext;

	if (!logical_len)
		return 0;

	simplefs_ext_init(&ext, logical_start, logical_len, physical_start,
			  simplefs_ext_unwritten(src));
	return simplefs_file_append_extent(buf, &ext);
}

static void simplefs_dump_extents(const char *tag,
				  struct simplefs_extent_buffer *buf)
{
	uint32_t i;

	pr_debug("dump extents: %s\n", tag);
	for (i = 0; i < buf->nr_extents; i++) {
		struct simplefs_extent *ext = &buf->extents[i];

		if (simplefs_extent_is_empty(ext))
			continue;
		pr_debug("  [%u] l=%u len=%u p=%u %s\n", i, ext->ee_block,
			 simplefs_ext_len(ext), ext->ee_start,
			 simplefs_ext_unwritten(ext) ? "unwritten" : "written");
	}
}

int simplefs_zero_partial_gap(struct inode *inode, loff_t start, loff_t len)
{
	unsigned int blkbits = inode->i_blkbits;
	unsigned int block_size = 1 << blkbits;
	loff_t end;
	unsigned int start_off;
	unsigned int end_off;
	int ret;

	if (len <= 0)
		return 0;

	end = start + len - 1;
	start_off = start & (block_size - 1);
	end_off = end & (block_size - 1);

	if ((start >> blkbits) == (end >> blkbits)) {
		if (!start_off && end_off == block_size - 1)
			return 0;
		return simplefs_iomap_zero_range_compat(inode, start, len);
	}

	if (start_off) {
		ret = simplefs_iomap_zero_range_compat(inode, start,
						       block_size - start_off);
		if (ret)
			return ret;
	}

	if (end_off != block_size - 1)
		return simplefs_iomap_zero_range_compat(inode, end - end_off,
							end_off + 1);

	return 0;
}

/* Core block mapping function used by iomap operations
 * Maps file logical block (iblock) to physical block number.
 * If create is true and block is not allocated, allocates new blocks.
 * Returns: number of blocks mapped, 0 for hole (unmapped), or negative error
 */
static int simplefs_get_block(struct inode *inode, sector_t iblock,
			      unsigned long maxblocks, u32 *bno_res, bool *new,
			      bool *unwritten, int create, bool exact_alloc,
			      bool create_unwritten, bool convert_unwritten)
{
	struct super_block *sb = inode->i_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_extent_buffer buf;
	struct simplefs_extent ext;
	int ret = 0, bno = 0;
	uint32_t extent_idx = 0, insert_idx = 0;
	uint32_t allocated_len = 0;
	uint32_t reserved_leaf = 0;
	bool allocated = false;
	bool mapped_unwritten = false;
	bool reserved_leaf_committed = false;
	int found;

	if (new)
		*new = false;
	if (unwritten)
		*unwritten = false;

	/* If block number exceeds filesize, fail */
	if (iblock >= SIMPLEFS_MAX_BLOCKS_PER_EXTENT * SIMPLEFS_MAX_FILE_EXTENTS)
		return -EFBIG;

	mutex_lock(&SIMPLEFS_INODE(inode)->extent_lock);

	ret = simplefs_file_load_extents(sb, SIMPLEFS_INODE(inode)->ei_block, &buf);
	if (ret) {
		pr_warn("get-block load-extents fail ino=%llu root=%u iblock=%llu max=%lu create=%d exact=%d create_unwritten=%d convert_unwritten=%d ret=%d i_size=%llu i_blocks=%llu\n",
			(unsigned long long)inode->i_ino,
			SIMPLEFS_INODE(inode)->ei_block,
			(unsigned long long)iblock, maxblocks, create,
			exact_alloc, create_unwritten, convert_unwritten, ret,
			(unsigned long long)i_size_read(inode),
			(unsigned long long)inode->i_blocks);
		mutex_unlock(&SIMPLEFS_INODE(inode)->extent_lock);
		return ret;
	}

	found = simplefs_file_find_extent(&buf, iblock, &extent_idx, &insert_idx);
	if (!found) {
		uint32_t alloc_len;
		int prev_extent = -1;

		if (!create) {
			ret = 0;
			goto out;
		}

		alloc_len = min_t(uint32_t, maxblocks,
				  SIMPLEFS_MAX_BLOCKS_PER_EXTENT);
		if (insert_idx < buf.nr_extents &&
		    buf.extents[insert_idx].ee_block < iblock + alloc_len)
			alloc_len = buf.extents[insert_idx].ee_block - iblock;
		if (insert_idx > 0) {
			struct simplefs_extent *prev = &buf.extents[insert_idx - 1];

			if (!simplefs_extent_is_empty(prev) &&
			    simplefs_ext_unwritten(prev) == create_unwritten &&
			    simplefs_ext_end(prev) == iblock &&
			    simplefs_ext_len(prev) < SIMPLEFS_MAX_BLOCKS_PER_EXTENT)
				prev_extent = insert_idx - 1;
		}

		if (prev_extent >= 0) {
			struct simplefs_extent *prev = &buf.extents[prev_extent];
			uint32_t grow_len = min_t(uint32_t, alloc_len,
						  SIMPLEFS_MAX_BLOCKS_PER_EXTENT -
						  simplefs_ext_len(prev));

			while (grow_len > 0) {
				uint32_t ext_len = simplefs_ext_len(prev);
				uint32_t grow_start = prev->ee_start + ext_len;

				if (simplefs_take_exact_blocks(sbi, grow_start,
							       grow_len)) {
					if (exact_alloc) {
						sector_t sector = (sector_t)grow_start <<
							(sb->s_blocksize_bits - 9);
						sector_t nr_sects = (sector_t)grow_len <<
							(sb->s_blocksize_bits - 9);

						ret = blkdev_issue_zeroout(sb->s_bdev,
							sector, nr_sects, GFP_NOFS, 0);
						if (ret) {
							put_blocks(sbi, grow_start,
								   grow_len);
							goto out;
						}
					}
					bno = grow_start;
					simplefs_ext_set_len(prev,
							     ext_len + grow_len);
					inode->i_blocks +=
						simplefs_blocks_to_sectors(grow_len);
					ret = simplefs_file_sync_extents(inode, &buf);
					if (ret)
						goto out;
					/*
					 * IOMAP_F_NEW must describe only the blocks allocated
					 * by this call.  The extent may have merged with older
					 * neighbours, but zeroing semantics must not extend into
					 * those already populated blocks.
					 */
					ret = grow_len;
					if (new)
						*new = true;
					if (unwritten)
						*unwritten = create_unwritten;
					*bno_res = bno;
					goto out;
				}
				grow_len /= 2;
			}
		}

		if (!buf.nr_leaf_blocks) {
			/*
			 * Reserve the first extent leaf immediately before the first
			 * data run.  Allocating data first would place the new leaf
			 * between the first page and the rest of a sequential write,
			 * needlessly fragmenting that write into two physical extents.
			 */
			reserved_leaf = get_free_blocks(sbi, alloc_len + 1);
			while (!reserved_leaf && alloc_len > 1) {
				alloc_len /= 2;
				reserved_leaf = get_free_blocks(sbi, alloc_len + 1);
			}
			if (reserved_leaf) {
				buf.leaf_blocks = kvmalloc_array(1,
							 sizeof(*buf.leaf_blocks),
							 GFP_NOFS);
				if (!buf.leaf_blocks) {
					put_blocks(sbi, reserved_leaf, alloc_len + 1);
					reserved_leaf = 0;
					ret = -ENOMEM;
					goto out;
				}
				buf.leaf_blocks[0] = reserved_leaf;
				buf.nr_leaf_blocks = 1;
				inode->i_blocks += simplefs_blocks_to_sectors(1);
				bno = reserved_leaf + 1;
			}
		} else {
			bno = get_free_blocks(sbi, alloc_len);
			while (!bno && alloc_len > 1) {
				/* Halve allocation size until we find space */
				alloc_len /= 2;
				bno = get_free_blocks(sbi, alloc_len);
			}
		}
		if (!bno) {
			ret = -ENOSPC;
			goto out;
		}

			/* 精确分配路径在发布 mapped extent 前先把磁盘块置零，
			 * 避免新分配但尚未完成数据写回的区域暴露旧数据。buffered/
			 * mmap 使用这条路径；显式 fallocate 和 DIO 则发布 unwritten
			 * extent，并在实际数据 I/O 成功后转换。
			 */
			if (exact_alloc) {
				sector_t sector =
					(sector_t)bno << (sb->s_blocksize_bits - 9);
				sector_t nr_sects =
					(sector_t)alloc_len <<
					(sb->s_blocksize_bits - 9);

				blkdev_issue_zeroout(sb->s_bdev, sector, nr_sects,
						     GFP_NOFS, 0);
			}

		simplefs_ext_init(&ext, iblock, alloc_len, bno, create_unwritten);
		ret = simplefs_file_insert_extent(&buf, insert_idx, &ext);
		if (ret) {
			put_blocks(sbi, bno, alloc_len);
			goto out;
		}
		inode->i_blocks += simplefs_blocks_to_sectors(alloc_len);
		allocated = true;
		allocated_len = alloc_len;
		mapped_unwritten = create_unwritten;
		ret = simplefs_file_sync_extents(inode, &buf);
		if (ret) {
			put_blocks(sbi, bno, alloc_len);
			goto out;
		}
		reserved_leaf_committed = true;
	} else {
		struct simplefs_extent *mapped = &buf.extents[extent_idx];
		uint32_t ext_len = simplefs_ext_len(mapped);

		if (create && convert_unwritten &&
		    simplefs_ext_unwritten(mapped)) {
			ret = simplefs_convert_unwritten_extent(&buf, extent_idx,
								iblock,
								maxblocks);
			if (ret < 0)
				goto out;
			ret = simplefs_file_sync_extents(inode, &buf);
			if (ret)
				goto out;
			simplefs_file_find_extent(&buf, iblock, &extent_idx, NULL);
			mapped = &buf.extents[extent_idx];
			ext_len = simplefs_ext_len(mapped);
		}
		mapped_unwritten = simplefs_ext_unwritten(mapped);
		bno = mapped->ee_start + iblock - mapped->ee_block;
		ret = ext_len - (iblock - mapped->ee_block);
		*bno_res = bno;
		if (unwritten)
			*unwritten = mapped_unwritten;
		goto out;
	}
	*bno_res = bno;
	ret = allocated_len;
	if (new)
		*new = allocated;
	if (unwritten)
		*unwritten = mapped_unwritten;
out:
	if (reserved_leaf && !reserved_leaf_committed) {
		simplefs_retire_metadata_blocks(sb, reserved_leaf, 1);
		inode->i_blocks -= simplefs_blocks_to_sectors(1);
	}
	if (ret < 0)
		simplefs_dump_extents("get-block-fail", &buf);
	simplefs_file_destroy_extents(&buf);
	mutex_unlock(&SIMPLEFS_INODE(inode)->extent_lock);
	return ret;
}

static int simplefs_report_iomap_begin(struct inode *inode, loff_t offset,
				       struct iomap *iomap)
{
	struct simplefs_extent_buffer buf;
	unsigned int blkbits = inode->i_blkbits;
	uint32_t iblock = offset >> blkbits;
	uint32_t extent_idx, insert_idx;
	uint32_t hole_end;
	int found, ret;

	mutex_lock(&SIMPLEFS_INODE(inode)->extent_lock);
	ret = simplefs_file_load_extents(inode->i_sb,
				 SIMPLEFS_INODE(inode)->ei_block, &buf);
	if (ret)
		goto out_unlock;

	found = simplefs_file_find_extent(&buf, iblock, &extent_idx,
					  &insert_idx);
	iomap->bdev = inode->i_sb->s_bdev;
	if (found) {
		struct simplefs_extent *ext = &buf.extents[extent_idx];
		struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
		u32 ext_start = ext->ee_block;
		u32 ext_end = simplefs_ext_end(ext);
		u32 map_start = ext_start;
		u32 map_end = ext_end;
		bool report_unwritten = simplefs_ext_unwritten(ext);

		/*
		 * Buffered allocation publishes a synchronously zeroed mapped run,
		 * then tracks the unused tail as speculative preallocation.  Split
		 * that subrange out as unwritten for SEEK_DATA/SEEK_HOLE reporting;
		 * otherwise a small write makes the whole reservation look like data.
		 */
		if (!report_unwritten && ci->prealloc_len) {
			u32 pre_start = ci->prealloc_block;
			u32 pre_end = pre_start + ci->prealloc_len;

			if (pre_start < ext_end && pre_end > ext_start) {
				if (iblock < pre_start) {
					map_end = min(ext_end, pre_start);
				} else if (iblock < pre_end) {
					map_start = max(ext_start, pre_start);
					map_end = min(ext_end, pre_end);
					report_unwritten = true;
				} else {
					map_start = max(ext_start, pre_end);
				}
			}
		}

		iomap->offset = (u64)map_start << blkbits;
		iomap->addr = (u64)(ext->ee_start + map_start - ext_start) <<
				blkbits;
		iomap->length = (u64)(map_end - map_start) << blkbits;
		iomap->type = report_unwritten ? IOMAP_UNWRITTEN : IOMAP_MAPPED;
	} else {
		hole_end = DIV_ROUND_UP(i_size_read(inode), 1U << blkbits);
		if (insert_idx < buf.nr_extents)
			hole_end = min(hole_end, buf.extents[insert_idx].ee_block);
		if (hole_end <= iblock)
			hole_end = iblock + 1;
		iomap->offset = (u64)iblock << blkbits;
		iomap->addr = IOMAP_NULL_ADDR;
		iomap->length = (u64)(hole_end - iblock) << blkbits;
		iomap->type = IOMAP_HOLE;
	}

	simplefs_file_destroy_extents(&buf);
out_unlock:
	mutex_unlock(&SIMPLEFS_INODE(inode)->extent_lock);
	return ret;
}

/* iomap begin operation for reads */
static int simplefs_read_iomap_begin(struct inode *inode, loff_t offset,
				     loff_t length, unsigned int flags,
				     struct iomap *iomap, struct iomap *srcmap)
{
	unsigned int blkbits = inode->i_blkbits;
	unsigned long first_block = offset >> blkbits;
	unsigned long max_blocks = DIV_ROUND_UP((offset & ((1 << blkbits) - 1)) +
						length, 1 << blkbits);
	loff_t isize;
	int ret;
	u32 bno;
	bool unwritten = false;

	/* FIEMAP reports the complete on-disk extent intersecting the query. */
	if (flags & IOMAP_REPORT)
		return simplefs_report_iomap_begin(inode, offset, iomap);

	/* All reads are always mapped below i_size. If reading past i_size,
	 * act as if there is a hole up to the file maximum size.
	 */
	iomap->bdev = inode->i_sb->s_bdev;
	iomap->offset = ALIGN_DOWN(offset, 1 << blkbits);
	isize = i_size_read(inode);
	if (iomap->offset >= isize) {
		iomap->type = IOMAP_HOLE;
		iomap->addr = IOMAP_NULL_ADDR;
		iomap->length = offset + length - iomap->offset;
	} else {
		/* Determine the physical block number for this file offset */
		ret = simplefs_get_block(inode, first_block, max_blocks, &bno,
					 NULL, &unwritten, 0, false, false,
					 false);
		if (ret < 0)
			return ret;
		if (ret == 0) {
			/* Block not allocated - hole */
			iomap->type = IOMAP_HOLE;
			iomap->addr = IOMAP_NULL_ADDR;
			iomap->length = 1 << blkbits;
		} else {
			iomap->addr = (u64)bno << blkbits;
			iomap->type = unwritten ? IOMAP_UNWRITTEN :
						  IOMAP_MAPPED;
			iomap->length = (u64)ret << blkbits;
		}
	}

	return 0;
}

static const struct iomap_ops simplefs_read_iomap_ops = {
	.iomap_begin = simplefs_read_iomap_begin,
};

#define SIMPLEFS_WRITE_PREALLOC_BLOCKS 64

/* iomap begin operation for writes */
static int simplefs_write_iomap_begin(struct inode *inode, loff_t offset,
				      loff_t length, unsigned int flags,
				      struct iomap *iomap, struct iomap *srcmap)
{
	unsigned int blkbits = inode->i_blkbits;
	unsigned long first_block = offset >> blkbits;
	unsigned long max_blocks = DIV_ROUND_UP((offset & ((1 << blkbits) - 1)) +
						length, 1 << blkbits);
	loff_t isize;
	int ret;
	u32 bno;
	bool new = false;
	bool unwritten = false;
	bool create = flags & IOMAP_WRITE;
	bool buffered_create = create && !(flags & IOMAP_DIRECT);
	unsigned long max_file_blocks = SIMPLEFS_MAX_BLOCKS_PER_EXTENT *
					SIMPLEFS_MAX_FILE_EXTENTS;
	unsigned long requested_blocks = max_blocks;

	/* Clear iomap to ensure no garbage values */
	memset(iomap, 0, sizeof(*iomap));

	/* iomap-based DIO properly handles new block allocation without
	 * stale data exposure, so we allow block creation for all writes.
	 */

	/*
	 * Writes that span EOF might trigger an IO size update on completion,
	 * so consider them to be dirty for the purposes of O_DSYNC even if
	 * there is no other metadata changes pending or have been made here.
	 */
	if ((flags & IOMAP_WRITE) && offset + length > i_size_read(inode))
		iomap->flags |= IOMAP_F_DIRTY;

	iomap->bdev = inode->i_sb->s_bdev;
	iomap->offset = ALIGN_DOWN(offset, 1 << blkbits);
	isize = i_size_read(inode);
	if (create && offset > isize && !(flags & IOMAP_DIRECT))
		iomap->private = (void *)(uintptr_t)(isize + 1);

	/*
	 * Small buffered writes otherwise allocate and persist one block at a
	 * time.  Besides fragmenting the file, a 512-byte workload such as
	 * generic/074 then performs millions of extent-tree updates.  Reserve a
	 * bounded run so later writes can reuse the same iomap.  Buffered/mmap
	 * allocation is synchronously zeroed before it is published.  The blocks
	 * touched by this request stay mapped, while the unused tail is persisted
	 * as unwritten.  Later buffered writes can dirty that reservation without
	 * changing the extent tree; writeback converts only completed ioend ranges.
	 * This also gives SEEK_DATA/SEEK_HOLE an extent-level representation of
	 * every speculative hole instead of relying on the single in-memory
	 * preallocation record.  DIO keeps request-sized unwritten allocation
	 * because its completion and fallback rules are different.
	 */
	if (create && !(flags & IOMAP_DIRECT) &&
	    first_block < max_file_blocks)
		max_blocks = min_t(unsigned long,
				   max_t(unsigned long, max_blocks,
					 SIMPLEFS_WRITE_PREALLOC_BLOCKS),
				   max_file_blocks - first_block);

	/* Determine the physical block number for this file offset */
	ret = simplefs_get_block(inode, first_block, max_blocks, &bno, &new,
				 &unwritten, create, buffered_create,
				 create && !buffered_create,
				 false);
	if (ret < 0)
		return ret;
	pr_debug("write_iomap_begin ino=%llu ei=%u iblocks=%llu off=%llu len=%llu flags=0x%x create=%d iblock=%lu max=%lu -> ret=%d bno=%u new=%d\n",
		(unsigned long long)inode->i_ino,
		SIMPLEFS_INODE(inode)->ei_block,
		(unsigned long long)inode->i_blocks,
		(unsigned long long)offset,
		(unsigned long long)length, flags, create, first_block,
		max_blocks, ret, bno, new);

	if (ret == 0) {
		/* Block not allocated - hole */
		iomap->type = IOMAP_HOLE;
		iomap->addr = IOMAP_NULL_ADDR;
		iomap->length = 1 << blkbits;
	} else {
		iomap->type = unwritten ? IOMAP_UNWRITTEN : IOMAP_MAPPED;
		iomap->addr = (u64)bno << blkbits;
		iomap->length = (u64)ret << blkbits;
	}

	if (new)
		iomap->flags |= IOMAP_F_NEW;
	if (new && !(flags & IOMAP_DIRECT) && ret > requested_blocks) {
		struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
		unsigned long prealloc_blocks = ret - requested_blocks;

		ret = simplefs_mark_unwritten_range(inode,
						     first_block + requested_blocks,
						     prealloc_blocks);
		if (ret)
			return ret;

		ci->prealloc_block = first_block + requested_blocks;
		ci->prealloc_len = prealloc_blocks;
		iomap->length = (u64)requested_blocks << blkbits;
	}

	return 0;
}

static int simplefs_write_iomap_end(struct inode *inode, loff_t offset,
				    loff_t length, ssize_t written,
				    unsigned int flags, struct iomap *iomap)
{
	if (written > 0 && iomap->private) {
		loff_t old_size = (loff_t)((uintptr_t)iomap->private - 1);
		loff_t end_pos = offset + written;

		if (end_pos > i_size_read(inode)) {
			i_size_write(inode, end_pos);
			iomap->flags |= IOMAP_F_SIZE_CHANGED;
		}
		pagecache_isize_extended(inode, old_size, end_pos);
		simplefs_zero_partial_gap(inode, old_size, offset - old_size);
	}
	if (written > 0) {
		int ret;

		ret = simplefs_prealloc_written(inode, offset, written,
						inode->i_blkbits);
		if (ret < 0)
			return ret;
	}
	if (iomap->flags & IOMAP_F_SIZE_CHANGED)
		mark_inode_dirty(inode);
	return 0;
}

const struct iomap_ops simplefs_write_iomap_ops = {
	.iomap_begin = simplefs_write_iomap_begin,
	.iomap_end = simplefs_write_iomap_end,
};

/* Pure iomap read */
static int simplefs_read_folio(struct file *unused, struct folio *folio)
{
	struct inode *inode = folio->mapping->host;
	
	trace_simplefs_read_page(inode, folio->index, folio_size(folio), false);
	sfs_stat_inc(read_count);
	
	// debug_show_held_locks(current);
#ifdef IOMAP_IOEND_UNWRITTEN
	iomap_bio_read_folio(folio, &simplefs_read_iomap_ops);
	return 0;
#else
	return iomap_read_folio(folio, &simplefs_read_iomap_ops);
#endif
}

static void simplefs_readahead(struct readahead_control *rac)
{
	sfs_stat_inc(read_count);
	// debug_show_held_locks(current);
#ifdef IOMAP_IOEND_UNWRITTEN
	iomap_bio_readahead(rac, &simplefs_read_iomap_ops);
#else
	iomap_readahead(rac, &simplefs_read_iomap_ops);
#endif
}

#ifdef IOMAP_IOEND_UNWRITTEN
struct simplefs_ioend_work {
	struct work_struct work;
	struct iomap_ioend *ioend;
};

void simplefs_wait_ioend_conversions(struct inode *inode)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);

	wait_event(ci->writeback_wait,
		   atomic_read(&ci->writeback_ioends) == 0);
}

static void simplefs_finish_ioend_work(struct work_struct *work)
{
	struct simplefs_ioend_work *completion =
		container_of(work, struct simplefs_ioend_work, work);
	struct iomap_ioend *ioend = completion->ioend;
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(ioend->io_inode);
	int error = blk_status_to_errno(ioend->io_bio.bi_status);

	if (!error && (ioend->io_flags & IOMAP_IOEND_UNWRITTEN))
		error = simplefs_convert_unwritten_range(ioend->io_inode,
							 ioend->io_offset,
							 ioend->io_size);
	if (error && error != -ENOSPC) {
		/* 写回失败（例如底层设备被移除）之后，继续提供页缓存里的
		 * 数据会掩盖数据丢失。将文件系统置为 shutdown，让后续读写
		 * 直接返回 -EIO（generic/730）。ENOSPC 不算：盘满时 unwritten
		 * 转换可能申请不到新的 leaf 块，这不会导致已有数据丢失。
		 */
		struct simplefs_sb_info *sbi = SIMPLEFS_SB(ioend->io_inode->i_sb);

		sbi->s_shutdown = 1;
	}
	if (atomic_dec_and_test(&ci->writeback_ioends))
		wake_up_all(&ci->writeback_wait);
	iomap_finish_ioends(ioend, error);
	kfree(completion);
}

static void simplefs_end_bio(struct bio *bio)
{
	struct iomap_ioend *ioend = iomap_ioend_from_bio(bio);
	struct simplefs_ioend_work *completion = ioend->io_private;

	schedule_work(&completion->work);
}

static ssize_t simplefs_writeback_range(struct iomap_writepage_ctx *wpc,
		struct folio *folio, u64 offset, unsigned len, u64 end_pos)
{
	struct inode *inode = wpc->inode;
	int ret;

	ret = simplefs_write_iomap_begin(inode, offset, len, IOMAP_WRITE,
					 &wpc->iomap, NULL);
	if (ret < 0) {
		pr_err("writeback map failed ino=%llu offset=%llu len=%u ret=%d\n",
		       (unsigned long long)inode->i_ino,
		       (unsigned long long)offset, len, ret);
		return ret;
	}

	if (WARN_ON_ONCE(offset < wpc->iomap.offset ||
			 offset >= wpc->iomap.offset + wpc->iomap.length))
		return -EIO;

	trace_simplefs_write_page(inode, folio->index, len, false);
	sfs_stat_inc(write_count);
	sfs_stat_add(write_bytes, len);
	return iomap_add_to_ioend(wpc, folio, offset, end_pos, len);
}

static int simplefs_writeback_submit(struct iomap_writepage_ctx *wpc, int error)
{
	struct iomap_ioend *ioend = wpc->wb_ctx;
	struct simplefs_ioend_work *completion;

	if (error) {
		struct simplefs_sb_info *sbi = SIMPLEFS_SB(wpc->inode->i_sb);

		pr_err("writeback submit failed ino=%llu offset=%llu size=%llu ret=%d\n",
		       (unsigned long long)wpc->inode->i_ino,
		       ioend ? (unsigned long long)ioend->io_offset : 0,
		       ioend ? (unsigned long long)ioend->io_size : 0, error);
		if (error != -ENOSPC)
			sbi->s_shutdown = 1;
	}
	if (error || !(ioend->io_flags & IOMAP_IOEND_UNWRITTEN))
		return iomap_ioend_writeback_submit(wpc, error);

	completion = kmalloc_obj(*completion, GFP_NOFS);
	if (!completion)
		return iomap_ioend_writeback_submit(wpc, -ENOMEM);

	INIT_WORK(&completion->work, simplefs_finish_ioend_work);
	completion->ioend = ioend;
	ioend->io_private = completion;
	ioend->io_bio.bi_end_io = simplefs_end_bio;
	atomic_inc(&SIMPLEFS_INODE(wpc->inode)->writeback_ioends);

	return iomap_ioend_writeback_submit(wpc, 0);
}

static const struct iomap_writeback_ops simplefs_writeback_ops = {
	.writeback_range	= simplefs_writeback_range,
	.writeback_submit	= simplefs_writeback_submit,
};

static int simplefs_writepages(struct address_space *mapping,
			     struct writeback_control *wbc)
{
	struct iomap_writepage_ctx wpc = {
		.inode		= mapping->host,
		.wbc		= wbc,
		.ops		= &simplefs_writeback_ops,
	};

	return iomap_writepages(&wpc);
}
#else
void simplefs_wait_ioend_conversions(struct inode *inode)
{
}

static int simplefs_write_map_blocks(struct iomap_writepage_ctx *wpc,
				     struct inode *inode, loff_t offset,
				     unsigned int len)
{
	if (offset >= wpc->iomap.offset &&
	    offset < wpc->iomap.offset + wpc->iomap.length)
		return 0;

	return simplefs_write_iomap_begin(inode, offset, len, IOMAP_WRITE,
					  &wpc->iomap, NULL);
}

static int simplefs_prepare_ioend(struct iomap_ioend *ioend, int status)
{
	if (!status && ioend->io_type == IOMAP_UNWRITTEN)
		status = simplefs_convert_unwritten_range(ioend->io_inode,
							 ioend->io_offset,
							 ioend->io_size);

	return status;
}

static const struct iomap_writeback_ops simplefs_writeback_ops = {
	.map_blocks	= simplefs_write_map_blocks,
	.prepare_ioend	= simplefs_prepare_ioend,
};

static int simplefs_writepages(struct address_space *mapping,
			     struct writeback_control *wbc)
{
	struct iomap_writepage_ctx wpc = { };

	return iomap_writepages(mapping, wbc, &wpc, &simplefs_writeback_ops);
}
#endif

/* File read - buffered and direct I/O */
static ssize_t simplefs_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	struct inode *inode = file_inode(iocb->ki_filp);

	if (!iov_iter_count(to))
		return 0;

	if (simplefs_is_shutdown(inode->i_sb))
		return -EIO;

	if (iocb->ki_flags & IOCB_DIRECT) {
		/* iomap 自身按设备逻辑扇区粒度构造 bio（IOMAP_F_NEW 和
		 * unwritten 都会补齐头尾清零），因此 DIO 对齐放宽到逻辑块
		 * 大小即可支持 generic/704 的 512 字节扇区设备。
		 */
		unsigned int blksize_mask =
			bdev_logical_block_size(inode->i_sb->s_bdev) - 1;
		ssize_t ret;

		if ((iocb->ki_pos | iov_iter_count(to) |
		     iov_iter_alignment(to)) &
		    blksize_mask)
			return -EINVAL;

		/* Flush dirty page cache before DIO read for coherency */
		ret = filemap_write_and_wait_range(inode->i_mapping,
				iocb->ki_pos,
				iocb->ki_pos + iov_iter_count(to) - 1);
		if (ret)
			return ret;

		return iomap_dio_rw(iocb, to, &simplefs_read_iomap_ops, NULL, 0,
				    NULL, 0);
	}
	return filemap_read(iocb, to, 0);
}

/* Direct I/O write completion callback */
static int simplefs_file_dio_write_end_io(struct kiocb *iocb, ssize_t size,
					  int error, unsigned int flags)
{
	off_t pos = iocb->ki_pos;
	struct inode *inode = file_inode(iocb->ki_filp);

	if (error)
		goto out;
	if (flags & IOMAP_DIO_UNWRITTEN) {
		error = simplefs_convert_unwritten_range(inode, pos, size);
		if (error)
			goto out;
	}

	/*
	 * If we are extending the file, we have to update i_size here before
	 * page cache gets invalidated in iomap_dio_rw().
	 */
	pos += size;
	if (pos > i_size_read(inode)) {
		i_size_write(inode, pos);
		mark_inode_dirty(inode);
	}
out:
	return error;
}

static const struct iomap_dio_ops simplefs_write_dio_ops = {
	.end_io = simplefs_file_dio_write_end_io,
};

#define SIMPLEFS_DIO_MAX_BYTES SZ_64M
#define SIMPLEFS_DIO_MAX_BVECS (SIMPLEFS_DIO_MAX_BYTES / PAGE_SIZE + 1)

static void simplefs_unpin_bvecs(struct bio_vec *bvecs,
				 unsigned short nr_bvecs)
{
	unsigned short i;

	for (i = 0; i < nr_bvecs; i++) {
		unsigned int nr_pages;
		unsigned int j;

		nr_pages = DIV_ROUND_UP(bvecs[i].bv_offset + bvecs[i].bv_len,
					PAGE_SIZE);
		for (j = 0; j < nr_pages; j++)
			unpin_user_page(bvecs[i].bv_page + j);
	}
}

/* File write - buffered and direct I/O using pure iomap */
static ssize_t simplefs_file_write_iter(struct kiocb *iocb,
					 struct iov_iter *from)
{
	struct file *file = iocb->ki_filp;
	unsigned int flags = 0;
	ssize_t ret;

	if (!iov_iter_count(from))
		return 0;

	if (simplefs_is_shutdown(file_inode(iocb->ki_filp)->i_sb))
		return -EIO;

	if ((iocb->ki_flags & (IOCB_NOWAIT | IOCB_DIRECT)) == IOCB_NOWAIT)
		return -EOPNOTSUPP;

	if (iocb->ki_flags & IOCB_DIRECT) {
		struct inode *inode = file_inode(file);
		unsigned long blocksize =
			bdev_logical_block_size(inode->i_sb->s_bdev);
		struct bio_vec *bvecs = NULL;
		bool guarded_dio;
		bool invalidate_locked = false;
		loff_t old_size;
		ssize_t written = 0;

		inode_lock(inode);
		if ((iocb->ki_pos | iov_iter_count(from) |
		     iov_iter_alignment(from)) & (blocksize - 1)) {
			inode_unlock(inode);
			return -EINVAL;
		}

		ret = generic_write_checks(iocb, from);
		if (ret <= 0) {
			inode_unlock(inode);
			return ret;
		}

		ret = file_modified(file);
		if (ret) {
			inode_unlock(inode);
			return ret;
		}
		/*
		 * iomap completes O_[D]SYNC direct writes by calling ->fsync before
		 * iomap_dio_rw() returns.  Drop any buffered-I/O reservation while we
		 * already own the inode lock, so that completion never has reservation
		 * cleanup work that would require recursively taking this lock.
		 */
		if (SIMPLEFS_INODE(inode)->prealloc_len) {
			ret = simplefs_discard_prealloc(file, inode);
			if (ret)
				goto dio_unlock;
		}

		old_size = i_size_read(inode);
		if (iocb->ki_pos > old_size) {
			ret = simplefs_zero_partial_gap(inode, old_size,
						 iocb->ki_pos - old_size);
			if (ret)
				goto dio_unlock;
		}

		/*
		 * If the target is mapped, keep DIO synchronous while holding
		 * invalidate_lock.  Otherwise an mmap fault can install and dirty a
		 * page after the pre-DIO invalidation but before async completion.
		 * iomap's post-DIO invalidation then sets mapping->wb_err to -EIO,
		 * which leaks into an unrelated msync (generic/095).  Unmapped targets
		 * use iomap's normal asynchronous path and page lifetime management.
		 *
		 * GUP must not run while invalidate_lock is held: its mmap_lock
		 * fallback has the opposite order from filemap fault/readahead.  Pin a
		 * bounded batch of user pages first, then give iomap a BVEC iterator.
		 * Keeping an aliased page pinned also makes iomap's pre-DIO invalidation
		 * request buffered fallback without fault recursion (generic/729).  The
		 * batch must remain large enough for AIO workloads: splitting every
		 * request at 1 MiB serializes thousands of forced-wait iomap calls.
		 */
		guarded_dio = mapping_mapped(inode->i_mapping);
		if (guarded_dio)
			flags |= IOMAP_DIO_FORCE_WAIT;
		if (guarded_dio && user_backed_iter(from)) {
			bvecs = kvcalloc(SIMPLEFS_DIO_MAX_BVECS, sizeof(*bvecs),
					 GFP_KERNEL);
			if (!bvecs) {
				ret = -ENOMEM;
				goto dio_unlock;
			}
		}

dio_retry:
		if (bvecs) {
			struct iov_iter probe = *from;
			struct iov_iter dio_iter;
			unsigned short nr_bvecs = 0;
			size_t consumed;
			ssize_t extracted;

			extracted = iov_iter_extract_bvecs(
					&probe, bvecs,
					min_t(size_t, iov_iter_count(from),
					      SIMPLEFS_DIO_MAX_BYTES),
					&nr_bvecs, SIMPLEFS_DIO_MAX_BVECS, 0);
			if (extracted <= 0) {
				ret = extracted ? extracted : -EFAULT;
				goto dio_unlock;
			}

			iov_iter_bvec(&dio_iter, ITER_SOURCE, bvecs, nr_bvecs,
					extracted);
			filemap_invalidate_lock(inode->i_mapping);
			invalidate_locked = true;
			ret = iomap_dio_rw(iocb, &dio_iter,
					   &simplefs_write_iomap_ops,
					   &simplefs_write_dio_ops, flags, NULL,
					   written);
			filemap_invalidate_unlock(inode->i_mapping);
			invalidate_locked = false;

			consumed = extracted - iov_iter_count(&dio_iter);
			simplefs_unpin_bvecs(bvecs, nr_bvecs);
			iov_iter_advance(from, consumed);
		} else {
			if (guarded_dio) {
				filemap_invalidate_lock(inode->i_mapping);
				invalidate_locked = true;
			}
			ret = iomap_dio_rw(iocb, from,
					   &simplefs_write_iomap_ops,
					   &simplefs_write_dio_ops, flags, NULL,
					   written);
			if (guarded_dio) {
				filemap_invalidate_unlock(inode->i_mapping);
				invalidate_locked = false;
			}
		}
		/*
		 * iomap uses -ENOTBLK to request buffered-I/O fallback when
		 * pre-DIO page-cache invalidation loses a race with buffered or
		 * mmap I/O.  No bytes have been consumed in that case.
		 */
		if (ret == -ENOTBLK)
			ret = 0;
		if (ret > 0)
			written = ret;
		if (ret > 0 && iov_iter_count(from))
			goto dio_retry;
		if (ret < 0)
			goto dio_unlock;
		ret = written;

		/* Handle partial DIO: fall back to buffered for remainder */
		if (ret >= 0 && iov_iter_count(from)) {
			loff_t pos, endbyte;
			ssize_t status, writeback_status;

			if (invalidate_locked) {
				filemap_invalidate_unlock(inode->i_mapping);
				invalidate_locked = false;
			}
			iocb->ki_flags &= ~IOCB_DIRECT;
			pos = iocb->ki_pos;
			status = simplefs_iomap_buffered_write_compat(iocb, from);
			if (status < 0) {
				ret = status;
				goto dio_unlock;
			}
			if (status > 0) {
				ret += status;
				endbyte = pos + status - 1;
				writeback_status = filemap_write_and_wait_range(
						inode->i_mapping, pos, endbyte);
				if (!writeback_status)
					invalidate_mapping_pages(inode->i_mapping,
							 pos >> PAGE_SHIFT,
							 endbyte >> PAGE_SHIFT);
			}
		}

dio_unlock:
		if (invalidate_locked)
			filemap_invalidate_unlock(inode->i_mapping);
		kvfree(bvecs);
		inode_unlock(inode);

		if (ret > 0)
			ret = generic_write_sync(iocb, ret);
		return ret;
	}

	{
		struct inode *inode = file_inode(file);

		inode_lock(inode);

		ret = generic_write_checks(iocb, from);
		if (ret <= 0) {
			inode_unlock(inode);
			return ret;
		}

		ret = file_modified(file);
		if (ret) {
			inode_unlock(inode);
			return ret;
		}

		ret = simplefs_iomap_buffered_write_compat(iocb, from);
		inode_unlock(inode);
	}

	if (ret > 0)
		ret = generic_write_sync(iocb, ret);
	return ret;
}

/* Pure iomap mmap support - page_mkwrite for handling mmap writes */
static vm_fault_t simplefs_page_mkwrite(struct vm_fault *vmf)
{
	struct inode *inode = file_inode(vmf->vma->vm_file);
	struct folio *folio = page_folio(vmf->page);
	vm_fault_t ret;

	if (simplefs_is_shutdown(inode->i_sb))
		return VM_FAULT_SIGBUS;

	sb_start_pagefault(inode->i_sb);
	file_update_time(vmf->vma->vm_file);
	pr_debug("page_mkwrite ino=%llu folio index=%lu size=%zu pos=%llu uptodate=%d dirty=%d\n",
		(unsigned long long)inode->i_ino, folio->index,
		folio_size(folio), folio_pos(folio),
		folio_test_uptodate(folio), folio_test_dirty(folio));

	filemap_invalidate_lock_shared(inode->i_mapping);
	ret = simplefs_iomap_page_mkwrite_compat(vmf);
	filemap_invalidate_unlock_shared(inode->i_mapping);

	sb_end_pagefault(inode->i_sb);
	return ret;
}

static const struct vm_operations_struct simplefs_file_vm_ops = {
	.fault		= filemap_fault,
	.map_pages	= filemap_map_pages,
	.page_mkwrite	= simplefs_page_mkwrite,
};

static int simplefs_file_mmap_prepare(struct vm_area_desc *desc)
{
	int ret;

	ret = generic_file_mmap_prepare(desc);
	if (!ret)
		desc->vm_ops = &simplefs_file_vm_ops;

	return ret;
}

static sector_t simplefs_bmap(struct address_space *mapping, sector_t block)
{
	return iomap_bmap(mapping, block, &simplefs_read_iomap_ops);
}

/* Address space operations - pure iomap, no buffer_head */
const struct address_space_operations simplefs_iomap_aops = {
	.read_folio		= simplefs_read_folio,
	.readahead		= simplefs_readahead,
	.writepages		= simplefs_writepages,
	.dirty_folio		= iomap_dirty_folio,
	.release_folio		= iomap_release_folio,
	.invalidate_folio	= iomap_invalidate_folio,
	.migrate_folio		= filemap_migrate_folio,
	.is_partially_uptodate	= iomap_is_partially_uptodate,
	.error_remove_folio	= generic_error_remove_folio,
	.bmap			= simplefs_bmap,
};

static long simplefs_fallocate_punch_hole(struct file *file,
					  struct inode *inode,
					  loff_t offset, loff_t len,
					  bool user_visible)
{
	struct super_block *sb = inode->i_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	struct simplefs_extent_buffer buf;
	struct simplefs_block_range *freed_ranges = NULL;
	unsigned int blkbits = inode->i_blkbits;
	unsigned int block_size = 1 << blkbits;
	loff_t end;
	uint32_t first_full, last_excl;
	uint32_t i, blocks_freed = 0, nr_freed_ranges = 0;
	int ret;

	/* Clamp range to i_size -- punch beyond EOF is no-op */
	if (offset >= inode->i_size)
		return 0;
	end = min(offset + len, inode->i_size);

	if (user_visible) {
		ret = file_modified(file);
		if (ret)
			return ret;
	}

	/* 释放完整块之前，先把目标范围内仍在页缓存里的脏数据刷完。
	 * 否则 punch 之后迟到的 writeback 仍可能命中已经回收到空闲位图
	 * 的旧块，随后在后续 truncate/fsx 路径上放大成 EIO。
	 */
	ret = filemap_write_and_wait_range(inode->i_mapping, offset, end - 1);
	simplefs_wait_ioend_conversions(inode);
	if (ret)
		return ret;

	filemap_invalidate_lock(inode->i_mapping);

	/* Zero partial boundary blocks and full blocks in page cache */
	ret = simplefs_iomap_zero_range_compat(inode, offset, end - offset);
	if (ret)
		goto out_unlock;

	/* Compute aligned block range for full extent freeing */
	first_full = ALIGN(offset, block_size) >> blkbits;
	last_excl = end >> blkbits;

	if (first_full < last_excl) {
		mutex_lock(&SIMPLEFS_INODE(inode)->extent_lock);
		ret = simplefs_file_load_extents(sb, SIMPLEFS_INODE(inode)->ei_block,
						 &buf);
		if (ret)
			goto out_unlock_extent;
		if (buf.nr_extents) {
			freed_ranges = kvmalloc_array(buf.nr_extents,
						       sizeof(*freed_ranges),
						       GFP_NOFS);
			if (!freed_ranges) {
				ret = -ENOMEM;
				goto out_buf;
			}
		}

		for (i = 0; i < buf.nr_extents; i++) {
			uint32_t ee_block = buf.extents[i].ee_block;
			uint32_t ee_len = simplefs_ext_len(&buf.extents[i]);
			uint32_t ee_end;
			bool ee_unwritten = simplefs_ext_unwritten(&buf.extents[i]);

			ee_end = ee_block + ee_len;

			/* No overlap with punch range */
			if (ee_end <= first_full || ee_block >= last_excl)
				continue;

			if (ee_block >= first_full && ee_end <= last_excl) {
				/* Entirely within punch range: free whole extent */
				freed_ranges[nr_freed_ranges++] =
					(struct simplefs_block_range) {
						.start = buf.extents[i].ee_start,
						.len = ee_len,
					};
				blocks_freed += ee_len;
				simplefs_extent_clear(&buf.extents[i]);
			} else if (ee_block < first_full && ee_end > last_excl) {
				/* Split: punch range is in the middle of extent.
				 * Keep head [ee_block, first_full) and tail
				 * [last_excl, ee_end). Free the middle.
				 */
				uint32_t head_len = first_full - ee_block;
				uint32_t mid_len = last_excl - first_full;
				uint32_t tail_len = ee_end - last_excl;
				struct simplefs_extent tail;

				freed_ranges[nr_freed_ranges++] =
					(struct simplefs_block_range) {
						.start = buf.extents[i].ee_start + head_len,
						.len = mid_len,
					};
				blocks_freed += mid_len;

				/* Shrink current extent to head */
				simplefs_ext_set_len(&buf.extents[i], head_len);
				simplefs_ext_init(
					&tail, last_excl, tail_len,
					buf.extents[i].ee_start + head_len + mid_len,
					ee_unwritten);
				ret = simplefs_file_insert_extent(&buf, i + 1, &tail);
				if (ret)
					goto out_buf;
				i++;
			} else if (ee_block < first_full) {
				/* Head overlap: extent starts before punch,
				 * ends within. Trim tail off.
				 */
				uint32_t keep = first_full - ee_block;
				uint32_t free_len = ee_len - keep;

				freed_ranges[nr_freed_ranges++] =
					(struct simplefs_block_range) {
						.start = buf.extents[i].ee_start + keep,
						.len = free_len,
					};
				blocks_freed += free_len;
				simplefs_ext_set_len(&buf.extents[i], keep);
			} else {
				/* Tail overlap: extent starts within punch,
				 * ends after. Trim head off.
				 */
				uint32_t skip = last_excl - ee_block;
				uint32_t keep = ee_len - skip;

				freed_ranges[nr_freed_ranges++] =
					(struct simplefs_block_range) {
						.start = buf.extents[i].ee_start,
						.len = skip,
					};
				blocks_freed += skip;
				buf.extents[i].ee_block = last_excl;
				buf.extents[i].ee_start += skip;
				simplefs_ext_set_len(&buf.extents[i], keep);
			}
		}

		if (blocks_freed) {
			ret = simplefs_file_sync_extents(inode, &buf);
			if (ret)
				goto out_buf;

			for (i = 0; i < nr_freed_ranges; i++)
				put_blocks(sbi, freed_ranges[i].start,
					   freed_ranges[i].len);
			inode->i_blocks -=
				simplefs_blocks_to_sectors(blocks_freed);
		}
out_buf:
		kvfree(freed_ranges);
		simplefs_file_destroy_extents(&buf);
		mutex_unlock(&SIMPLEFS_INODE(inode)->extent_lock);
		if (ret)
			goto out_unlock;

		/* Invalidate page cache for fully freed blocks to prevent
		 * writeback from re-allocating them. Only invalidate the
		 * aligned block range, not partial boundary blocks whose
		 * zeroed pages must remain dirty for writeback.
		 */
		if (blocks_freed)
			truncate_pagecache_range(inode,
				(loff_t)first_full << blkbits,
				((loff_t)last_excl << blkbits) - 1);
	}

	if (user_visible)
		simplefs_touch_mctime(inode);
	mark_inode_dirty(inode);
	write_inode_now(inode, 1);

out_unlock:
	filemap_invalidate_unlock(inode->i_mapping);
	return ret;

out_unlock_extent:
	mutex_unlock(&SIMPLEFS_INODE(inode)->extent_lock);
	goto out_unlock;
}

static int simplefs_discard_prealloc(struct file *file, struct inode *inode)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	unsigned int blkbits = inode->i_blkbits;
	uint32_t start_block = ci->prealloc_block;
	uint32_t nr_blocks = ci->prealloc_len;
	loff_t size = i_size_read(inode);
	loff_t start, end;
	int ret;

	if (!nr_blocks)
		return 0;

	/* Clear the in-memory reservation before freeing it so writeback caused
	 * by the range operation cannot consume stale reservation state.
	 */
	ci->prealloc_block = 0;
	ci->prealloc_len = 0;
	start = (loff_t)start_block << blkbits;
	end = (loff_t)(start_block + nr_blocks) << blkbits;

	if (start < size) {
		ret = simplefs_fallocate_punch_hole(file, inode, start,
						   min(end, size) - start,
						   false);
		if (ret)
			return ret;
	}

	/* The same speculative run may extend past EOF, where punch-hole is a
	 * no-op.  Truncating to the unchanged size drops that remaining tail.
	 */
	return simplefs_truncate(inode, size);
}

static long simplefs_fallocate_prealloc(struct file *file,
					struct inode *inode,
					int mode, loff_t offset, loff_t len)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	unsigned int blkbits = inode->i_blkbits;
	sector_t first_block = offset >> blkbits;
	sector_t end_block = (offset + len + (1 << blkbits) - 1) >> blkbits;
	sector_t block;
	loff_t old_size = inode->i_size;
	loff_t new_size = offset + len;
	u32 bno;
	bool new_block;
	int ret;

	ret = file_modified(file);
	if (ret)
		return ret;

	ci->prealloc_block = 0;
	ci->prealloc_len = 0;

	/* For mode==0, extend i_size first so iomap_zero_range won't warn
	 * about zeroing beyond EOF. For KEEP_SIZE, only zero up to i_size.
	 */
	if (mode == 0 && new_size > old_size) {
		truncate_setsize(inode, new_size);
	}

	if (mode == 0 && new_size > old_size &&
	    !IS_ALIGNED(old_size, 1 << blkbits)) {
		loff_t tail_end = min_t(loff_t, new_size,
					ALIGN(old_size, 1 << blkbits));
		loff_t tail_len = tail_end - old_size;

		if (tail_len > 0) {
			ret = simplefs_iomap_zero_range_compat(inode, old_size,
							       tail_len);
			if (ret)
				goto out_restore;
		}
	}

	for (block = first_block; block < end_block;) {
		new_block = false;
		ret = simplefs_get_block(inode, block, end_block - block, &bno,
					 &new_block, NULL, 1, true, true,
					 false);
		if (ret < 0)
			goto out_restore;

		block += ret > 0 ? ret : 1;
	}

	if (mode & FALLOC_FL_KEEP_SIZE) {
		/*
		 * KEEP_SIZE preallocation now lives entirely in extent state.
		 * No inode-level prealloc bookkeeping remains to sync here.
		 */
	} else {
		ci->prealloc_block = 0;
		ci->prealloc_len = 0;
	}

	if (mode == 0 && new_size > old_size) {
		ret = simplefs_iomap_truncate_page_compat(inode, new_size);
		if (ret)
			goto out_restore;
	}

	mark_inode_dirty(inode);
	return 0;

out_restore:
	if (mode == 0 && new_size > old_size)
		truncate_setsize(inode, old_size);
	return ret;
}

static long simplefs_fallocate_collapse(struct inode *inode,
					loff_t offset, loff_t len)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(inode->i_sb);
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct simplefs_extent_buffer buf;
	struct simplefs_extent_buffer new_buf;
	struct simplefs_block_range *freed_ranges = NULL;
	unsigned int blkbits = inode->i_blkbits;
	unsigned int block_size = 1 << blkbits;
	uint32_t start_block, end_block, collapse_blocks;
	uint32_t i, blocks_freed = 0, nr_freed_ranges = 0;
	int ret;

	/* offset and len must be block-aligned */
	if (!IS_ALIGNED(offset, block_size) || !IS_ALIGNED(len, block_size))
		return -EINVAL;

	/* Cannot collapse past EOF */
	if (offset + len > inode->i_size)
		return -EINVAL;

	/* Write tail of the last page before removed range and data that
	 * will be shifted since they will get removed from the page cache
	 * below. Round down offset to page boundary for page size > block
	 * size.
	 */
	{
		loff_t start = round_down(offset, PAGE_SIZE);

		filemap_invalidate_lock(inode->i_mapping);

		ret = filemap_write_and_wait(inode->i_mapping);
		simplefs_wait_ioend_conversions(inode);
		if (ret) {
			filemap_invalidate_unlock(inode->i_mapping);
			return ret;
		}

		truncate_pagecache(inode, start);
	}

	start_block = offset >> blkbits;
	end_block = (offset + len) >> blkbits;
	collapse_blocks = end_block - start_block;

	mutex_lock(&ci->extent_lock);
	ret = simplefs_file_load_extents(inode->i_sb, ci->ei_block, &buf);
	if (ret) {
		mutex_unlock(&ci->extent_lock);
		filemap_invalidate_unlock(inode->i_mapping);
		return ret;
	}
	memset(&new_buf, 0, sizeof(new_buf));
	if (buf.nr_extents) {
		freed_ranges = kvmalloc_array(buf.nr_extents,
					       sizeof(*freed_ranges), GFP_NOFS);
		if (!freed_ranges) {
			ret = -ENOMEM;
			goto out_buf;
		}
	}

	simplefs_dump_extents("collapse-before", &buf);

	for (i = 0; i < buf.nr_extents; i++) {
		const struct simplefs_extent *ext = &buf.extents[i];
		uint32_t ee_block = ext->ee_block;
		uint32_t ee_len = simplefs_ext_len(ext);
		uint32_t ee_end = ee_block + ee_len;
		uint32_t overlap_start;
		uint32_t overlap_end;

		if (simplefs_extent_is_empty(ext))
			continue;

		if (ee_end <= start_block) {
			ret = simplefs_file_append_extent(&new_buf, ext);
			if (ret)
				goto out_buf;
			continue;
		}

		if (ee_block >= end_block) {
			ret = simplefs_append_extent_slice(&new_buf, ext,
							   ee_block -
								   collapse_blocks,
							   ee_len,
							   ext->ee_start);
			if (ret)
				goto out_buf;
			continue;
		}

		if (ee_block < start_block) {
			uint32_t head_len = start_block - ee_block;

			ret = simplefs_append_extent_slice(&new_buf, ext,
							   ee_block, head_len,
							   ext->ee_start);
			if (ret)
				goto out_buf;
		}

		overlap_start = max(ee_block, start_block);
		overlap_end = min(ee_end, end_block);
		if (overlap_end > overlap_start) {
			uint32_t freed = overlap_end - overlap_start;
			uint32_t phys = ext->ee_start + (overlap_start - ee_block);

			freed_ranges[nr_freed_ranges++] =
				(struct simplefs_block_range) {
					.start = phys,
					.len = freed,
				};
			blocks_freed += freed;
		}

		if (ee_end > end_block) {
			uint32_t tail_skip = end_block - ee_block;
			uint32_t tail_len = ee_end - end_block;

			ret = simplefs_append_extent_slice(&new_buf, ext,
							   end_block -
								   collapse_blocks,
							   tail_len,
							   ext->ee_start +
								   tail_skip);
			if (ret)
				goto out_buf;
		}
	}

	simplefs_file_normalize_extents(&new_buf);
	simplefs_dump_extents("collapse-after", &new_buf);

	/* new_buf 是重建后的数据 extent 集合，但 extent leaf 的所有权仍
	 * 属于旧 buf。把 leaf 列表转移过去，sync_extents 才能原位复用
	 * 现有 leaf，并在数量减少时释放多余块。否则每次 collapse 都会
	 * 把旧 leaf 永久泄漏在块位图中。
	 */
	new_buf.leaf_blocks = buf.leaf_blocks;
	new_buf.nr_leaf_blocks = buf.nr_leaf_blocks;
	buf.leaf_blocks = NULL;
	buf.nr_leaf_blocks = 0;
	ret = simplefs_file_sync_extents(inode, &new_buf);
	if (!ret) {
		for (i = 0; i < nr_freed_ranges; i++)
			put_blocks(sbi, freed_ranges[i].start,
				   freed_ranges[i].len);
		simplefs_prealloc_collapse(ci, start_block, collapse_blocks);
		inode->i_size -= len;
		inode->i_blocks -= simplefs_blocks_to_sectors(blocks_freed);
	}
out_buf:
	kvfree(freed_ranges);
	simplefs_file_destroy_extents(&new_buf);
	simplefs_file_destroy_extents(&buf);
	mutex_unlock(&ci->extent_lock);
	if (ret) {
		filemap_invalidate_unlock(inode->i_mapping);
		return ret;
	}

	mark_inode_dirty(inode);

	filemap_invalidate_unlock(inode->i_mapping);
	return 0;
}

static long simplefs_fallocate_insert(struct inode *inode,
				      loff_t offset, loff_t len)
{
	struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
	struct simplefs_extent_buffer buf;
	struct simplefs_extent_buffer new_buf;
	unsigned int blkbits = inode->i_blkbits;
	unsigned int block_size = 1 << blkbits;
	uint32_t start_block, insert_blocks;
	uint32_t i;
	int ret;

	/* offset 和 len 必须块对齐（与 collapse 相同） */
	if (!IS_ALIGNED(offset, block_size) || !IS_ALIGNED(len, block_size))
		return -EINVAL;

	/* 只能在文件内或 EOF 处插入 */
	if (offset > inode->i_size)
		return -EINVAL;

	ret = inode_newsize_ok(inode, inode->i_size + len);
	if (ret)
		return ret;

	insert_blocks = len >> blkbits;
	start_block = offset >> blkbits;

	/* 插入点之后的数据在页缓存中的位置会整体右移，先把脏页刷完
	 * 再从插入点开始丢弃页缓存，后续访问按新映射重新读入。
	 */
	{
		loff_t start = round_down(offset, PAGE_SIZE);

		filemap_invalidate_lock(inode->i_mapping);
		ret = filemap_write_and_wait(inode->i_mapping);
		simplefs_wait_ioend_conversions(inode);
		if (ret) {
			filemap_invalidate_unlock(inode->i_mapping);
			return ret;
		}
		truncate_pagecache(inode, start);
	}

	mutex_lock(&ci->extent_lock);
	ret = simplefs_file_load_extents(inode->i_sb, ci->ei_block, &buf);
	if (ret) {
		mutex_unlock(&ci->extent_lock);
		filemap_invalidate_unlock(inode->i_mapping);
		return ret;
	}
	memset(&new_buf, 0, sizeof(new_buf));

	for (i = 0; i < buf.nr_extents; i++) {
		const struct simplefs_extent *ext = &buf.extents[i];
		uint32_t ee_block = ext->ee_block;
		uint32_t ee_len = simplefs_ext_len(ext);
		uint32_t ee_end = ee_block + ee_len;

		if (simplefs_extent_is_empty(ext))
			continue;

		if (ee_end <= start_block) {
			/* 完全在插入点之前：原样保留 */
			ret = simplefs_file_append_extent(&new_buf, ext);
			if (ret)
				goto out_buf;
			continue;
		}

		if (ee_block >= start_block) {
			/* 完全在插入点之后：逻辑块号整体右移，物理块不动 */
			ret = simplefs_append_extent_slice(&new_buf, ext,
							   ee_block + insert_blocks,
							   ee_len,
							   ext->ee_start);
			if (ret)
				goto out_buf;
			continue;
		}

		/* 跨越插入点：前半保留，后半右移，中间形成 hole */
		{
			uint32_t head_len = start_block - ee_block;
			uint32_t tail_len = ee_end - start_block;

			ret = simplefs_append_extent_slice(&new_buf, ext,
							   ee_block, head_len,
							   ext->ee_start);
			if (ret)
				goto out_buf;
			ret = simplefs_append_extent_slice(&new_buf, ext,
							   start_block +
							   insert_blocks,
							   tail_len,
							   ext->ee_start + head_len);
			if (ret)
				goto out_buf;
		}
	}

	simplefs_file_normalize_extents(&new_buf);

	/* 与 collapse 相同：leaf 所有权转移到 new_buf，sync 原位复用。 */
	new_buf.leaf_blocks = buf.leaf_blocks;
	new_buf.nr_leaf_blocks = buf.nr_leaf_blocks;
	buf.leaf_blocks = NULL;
	buf.nr_leaf_blocks = 0;
	ret = simplefs_file_sync_extents(inode, &new_buf);
	if (!ret) {
		simplefs_prealloc_insert(ci, start_block, insert_blocks);
		inode->i_size += len;
	}
out_buf:
	simplefs_file_destroy_extents(&new_buf);
	simplefs_file_destroy_extents(&buf);
	mutex_unlock(&ci->extent_lock);
	if (ret) {
		filemap_invalidate_unlock(inode->i_mapping);
		return ret;
	}

	mark_inode_dirty(inode);
	filemap_invalidate_unlock(inode->i_mapping);
	return 0;
}

static long simplefs_fallocate_zero_range(struct file *file,
					  struct inode *inode,
					  int mode, loff_t offset, loff_t len)
{
	unsigned int blkbits = inode->i_blkbits;
	unsigned int block_size = 1 << blkbits;
	loff_t end = offset + len;
	loff_t old_size = inode->i_size;
	loff_t new_size = 0;
	loff_t align_start, align_end;
	int ret;

	if (!(mode & FALLOC_FL_KEEP_SIZE) && end > old_size) {
		new_size = end;
		ret = inode_newsize_ok(inode, new_size);
		if (ret)
			return ret;
	}

	/* 先把范围内的脏页刷出去，避免标记 unwritten 之后迟到的
	 * 写回又把旧数据写进已经转换的块。
	 */
	ret = filemap_write_and_wait_range(inode->i_mapping, offset, end - 1);
	simplefs_wait_ioend_conversions(inode);
	if (ret)
		return ret;

	/* ZERO_RANGE 不带 KEEP_SIZE 时与 prealloc 一样扩展 i_size，
	 * 并清零旧文件尾部的部分块。
	 */
	if (new_size) {
		truncate_setsize(inode, new_size);
		if (!IS_ALIGNED(old_size, block_size)) {
			loff_t tail_end = min_t(loff_t, new_size,
						ALIGN(old_size, block_size));

			ret = simplefs_iomap_zero_range_compat(inode, old_size,
							       tail_end - old_size);
			if (ret)
				goto out_restore;
		}
	}

	align_start = ALIGN(offset, block_size);
	align_end = round_down(end, block_size);

	filemap_invalidate_lock(inode->i_mapping);

	/* 完整块部分：hole 分配为 unwritten，written 转换为 unwritten，
	 * 与 ext4 的 zero range 语义一致（generic/009 的 fiemap 校验）。
	 * 整个范围单遍处理，只加载/回写一次 extent 树。
	 */
	if (align_end > align_start) {
		ret = simplefs_zero_range_full_blocks(inode,
				align_start >> blkbits,
				align_end >> blkbits);
		if (ret)
			goto out_unlock_invalidate;

		/* 边缘未对齐部分物理清零。iomap_zero_range 会按需分配
		 * 并把已分配块的对应区间写零。
		 */
		if (offset < align_start) {
			ret = simplefs_iomap_zero_range_compat(inode, offset,
							       align_start - offset);
			if (ret)
				goto out_unlock_invalidate;
		}
		if (align_end < end) {
			ret = simplefs_iomap_zero_range_compat(inode, align_end,
							       end - align_end);
			if (ret)
				goto out_unlock_invalidate;
		}

		/* 完整块范围的缓存页失效，后续读才能看到 unwritten 的零。 */
		truncate_pagecache_range(inode, align_start, align_end - 1);
	} else {
		/* 范围不足一个完整块：整块都按边缘处理。 */
		ret = simplefs_iomap_zero_range_compat(inode, offset, len);
		if (ret)
			goto out_unlock_invalidate;
	}

	filemap_invalidate_unlock(inode->i_mapping);

	if (new_size) {
		ret = simplefs_iomap_truncate_page_compat(inode, new_size);
		if (ret)
			goto out_restore;
	}

	mark_inode_dirty(inode);
	return 0;

out_unlock_invalidate:
	filemap_invalidate_unlock(inode->i_mapping);
out_restore:
	if (new_size)
		truncate_setsize(inode, old_size);
	return ret;
}

static long simplefs_fallocate(struct file *file, int mode,
			       loff_t offset, loff_t len)
{
	struct inode *inode = file_inode(file);
	int ret;

	if (mode & ~(FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE |
		     FALLOC_FL_COLLAPSE_RANGE | FALLOC_FL_INSERT_RANGE |
		     FALLOC_FL_ZERO_RANGE))
		return -EOPNOTSUPP;

	inode_lock(inode);
	/*
	 * DIO completion may still reference blocks that punch/collapse is about
	 * to free.  Drain it before changing extent ownership so a late bio cannot
	 * overwrite a block after the allocator gives it to another inode.
	 */
	inode_dio_wait(inode);

	ret = file_modified(file);
	if (ret)
		goto out;
	if (SIMPLEFS_INODE(inode)->prealloc_len) {
		ret = simplefs_discard_prealloc(file, inode);
		if (ret)
			goto out;
	}

	if (mode == (FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE)) {
		ret = simplefs_fallocate_punch_hole(file, inode, offset, len,
						   true);
	} else if (mode == 0 || mode == FALLOC_FL_KEEP_SIZE) {
		if (!(mode & FALLOC_FL_KEEP_SIZE)) {
			ret = inode_newsize_ok(inode, offset + len);
			if (ret)
				goto out;
		}
		ret = simplefs_fallocate_prealloc(file, inode, mode, offset,
						  len);
	} else if (mode == FALLOC_FL_COLLAPSE_RANGE) {
		ret = simplefs_fallocate_collapse(inode, offset, len);
	} else if (mode == FALLOC_FL_INSERT_RANGE) {
		ret = simplefs_fallocate_insert(inode, offset, len);
	} else if (mode == FALLOC_FL_ZERO_RANGE ||
		   mode == (FALLOC_FL_ZERO_RANGE | FALLOC_FL_KEEP_SIZE)) {
		ret = simplefs_fallocate_zero_range(file, inode, mode, offset,
						    len);
	} else {
		ret = -EOPNOTSUPP;
	}

out:
	inode_unlock(inode);
	return ret;
}

int simplefs_fiemap(struct inode *inode, struct fiemap_extent_info *fieinfo,
		    u64 start, u64 len)
{
	struct simplefs_extent_buffer buf;
	u64 end;
	int ret;
	int i;

	ret = fiemap_prep(inode, fieinfo, start, &len, FIEMAP_FLAG_XATTR);
	if (ret)
		return ret;
	end = start + len;

	/* fiemap -a（FIEMAP_FLAG_XATTR）：simplefs 的全部 xattr 集中存放在
	 * i_xattr_block 单个块中，按一个 extent 上报即可（generic/425）。
	 */
	if (fieinfo->fi_flags & FIEMAP_FLAG_XATTR) {
		struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);

		fieinfo->fi_flags &= ~FIEMAP_FLAG_XATTR;
		if (!ci->i_xattr_block)
			return 0;
		return fiemap_fill_next_extent(fieinfo, 0,
					       (u64)ci->i_xattr_block <<
					       inode->i_blkbits,
					       SIMPLEFS_BLOCK_SIZE,
					       FIEMAP_EXTENT_LAST);
	}

	inode_lock_shared(inode);
	ret = simplefs_file_load_extents(inode->i_sb, SIMPLEFS_INODE(inode)->ei_block,
					 &buf);
	if (ret)
		goto out_unlock;

	for (i = 0; i < buf.nr_extents; i++) {
		struct simplefs_extent *ext = &buf.extents[i];
		u64 logical, logical_end, phys;
		uint32_t ext_len = simplefs_ext_len(ext);
		u32 flags = 0;

		if (!ext->ee_start || !ext_len)
			continue;

		logical = (u64)ext->ee_block << inode->i_blkbits;
		logical_end = logical + ((u64)ext_len << inode->i_blkbits);
		phys = (u64)ext->ee_start << inode->i_blkbits;

		if (logical_end <= start || logical >= end)
			continue;

		if (simplefs_ext_unwritten(ext))
			flags |= FIEMAP_EXTENT_UNWRITTEN;
		if (i == buf.nr_extents - 1)
			flags |= FIEMAP_EXTENT_LAST;
		ret = fiemap_fill_next_extent(fieinfo, logical, phys,
					      logical_end - logical, flags);
		if (ret)
			break;
	}

	simplefs_file_destroy_extents(&buf);
out_unlock:
	inode_unlock_shared(inode);
	return ret;
}

/* NOTE: reflink (remap_file_range) removed - simplefs lacks CoW support,
 * so sharing physical blocks would corrupt data on write.
 */

static int simplefs_shutdown(struct super_block *sb, __u32 flags)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	int ret = 0;

	if (sbi->s_shutdown)
		return 0;

	switch (flags) {
	case SIMPLEFS_GOING_DOWN_FULLSYNC:
		down_read(&sb->s_umount);
		sync_filesystem(sb);
		up_read(&sb->s_umount);
		sbi->s_shutdown = 1;
		break;
	case SIMPLEFS_GOING_DOWN_METASYNC:
		/* LOGFLUSH: write completed VFS inode updates into JBD2 and wait
		 * for the commit record, but do not checkpoint home blocks. */
		down_read(&sb->s_umount);
		sync_inodes_sb(sb);
		up_read(&sb->s_umount);
		sbi->s_shutdown = 1;
		ret = simplefs_journal_force_commit(sbi->s_journal);
		simplefs_journal_shutdown(sbi->s_journal);
		break;
	case SIMPLEFS_GOING_DOWN_NOSYNC:
		sbi->s_shutdown = 1;
		simplefs_journal_shutdown(sbi->s_journal);
		break;
	default:
		return -EINVAL;
	}

	sb->s_flags |= SB_RDONLY;
	return ret;
}

/*
 * FITRIM：对位图中标记为空闲的连续块段下发 discard。空闲块 discard 后
 * 仍保持空闲，供块设备回收（generic/251/260/288/746 验证设备上读出为零）。
 */
static int simplefs_trim_fs(struct super_block *sb, struct fstrim_range *range)
{
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	uint64_t start_block, end_block, minlen, b;
	uint64_t trimmed = 0;
	int ret = 0;

	/* norecovery 明确允许空闲空间元数据处于未重放状态；按这些位图
	 * discard 可能破坏仍被日志引用的数据，因此必须拒绝 FITRIM。 */
	if (sbi->s_journal_mode == 2)
		return -EROFS;

	if (range->len < sb->s_blocksize)
		return -EINVAL;

	start_block = range->start >> sb->s_blocksize_bits;
	/* 先移位再相加：fstrim 不带 -l 时 len 接近 U64_MAX，
	 * 与 start 相加后再移位会溢出，导致 end_block 回绕、整块跳过。
	 */
	end_block = start_block + (range->len >> sb->s_blocksize_bits);
	minlen = range->minlen >> sb->s_blocksize_bits;

	if (start_block >= sbi->nr_blocks)
		return -EINVAL;
	if (end_block > sbi->nr_blocks)
		end_block = sbi->nr_blocks;
	if (minlen == 0)
		minlen = 1;

	mutex_lock(&sbi->bitmap_lock);
	b = start_block;
	while (b < end_block) {
		uint32_t run_end, run;

		/* 位图约定：bit=1 空闲，bit=0 已用 */
		b = find_next_bit(sbi->bfree_bitmap, end_block, b);
		if (b >= end_block)
			break;
		run_end = find_next_zero_bit(sbi->bfree_bitmap, end_block,
					     b + 1);
		run = run_end - b;
		if (run >= minlen) {
			ret = sb_issue_discard(sb, b, run, GFP_NOFS, 0);
			if (ret)
				break;
			trimmed += run;
		}
		b = run_end;
	}
	mutex_unlock(&sbi->bitmap_lock);

	range->len = trimmed << sb->s_blocksize_bits;
	return ret;
}

long simplefs_ioctl(struct file *filp, unsigned int cmd,
		    unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct super_block *sb = inode->i_sb;
	__u32 flags;

	switch (cmd) {
	case SIMPLEFS_IOC_SHUTDOWN:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (get_user(flags, (__u32 __user *)arg))
			return -EFAULT;
		return simplefs_shutdown(sb, flags);
	case FITRIM: {
		struct fstrim_range range;
		int ret;

		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (copy_from_user(&range, (void __user *)arg, sizeof(range)))
			return -EFAULT;
		ret = simplefs_trim_fs(sb, &range);
		if (ret)
			return ret;
		if (copy_to_user((void __user *)arg, &range, sizeof(range)))
			return -EFAULT;
		return 0;
	}
	case FS_IOC_GETFSLABEL: {
		struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
		char label[FSLABEL_MAX];

		memset(label, 0, sizeof(label));
		memcpy(label, sbi->s_volume_name, sizeof(sbi->s_volume_name));
		if (copy_to_user((void __user *)arg, label, sizeof(label)))
			return -EFAULT;
		return 0;
	}
	case FS_IOC_SETFSLABEL: {
		struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
		struct simplefs_handle *handle;
		struct simplefs_sb_info *disk_sb;
		struct folio *folio;
		char new_label[FSLABEL_MAX];
		size_t len;
		int ret, stop_ret;

		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (copy_from_user(new_label, (void __user *)arg,
				   sizeof(new_label)))
			return -EFAULT;
		new_label[sizeof(new_label) - 1] = '\0';
		len = strnlen(new_label, sizeof(new_label));
		if (len > SIMPLEFS_LABEL_MAX)
			return -EINVAL;
		handle = simplefs_journal_start_sb(sb, 1);
		if (IS_ERR(handle))
			return PTR_ERR(handle);

		memset(sbi->s_volume_name, 0, sizeof(sbi->s_volume_name));
		memcpy(sbi->s_volume_name, new_label, len);

		disk_sb = simplefs_get_folio(sb, 0, &folio);
		if (IS_ERR(disk_sb)) {
			ret = PTR_ERR(disk_sb);
			goto label_out_stop;
		}
		memset(disk_sb->s_volume_name, 0,
		       sizeof(disk_sb->s_volume_name));
		memcpy(disk_sb->s_volume_name, sbi->s_volume_name, len);
		ret = simplefs_journal_dirty_folio(sb, 0, disk_sb, folio);
		folio_release_kmap(folio, disk_sb);

label_out_stop:
		stop_ret = simplefs_journal_stop(handle);
		if (!ret)
			ret = stop_ret;
		return ret;
	}
	default:
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
long simplefs_compat_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	return simplefs_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int simplefs_file_open(struct inode *inode, struct file *filp)
{
	if (simplefs_is_shutdown(inode->i_sb))
		return -EIO;
	filp->f_mode |= FMODE_CAN_ODIRECT;
	return generic_file_open(inode, filp);
}

static int simplefs_file_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	inode_lock(inode);
	if (SIMPLEFS_INODE(inode)->prealloc_len)
		ret = simplefs_discard_prealloc(file, inode);
	inode_unlock(inode);
	return ret;
}

/* File fsync: data must reach stable storage before the transaction commit
 * which publishes its extent/inode metadata. */
static int simplefs_file_fsync(struct file *file, loff_t start, loff_t end,
			       int datasync)
{
	struct inode *inode = file_inode(file);
	struct super_block *sb = inode->i_sb;
	struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
	int err, err2;

	if (simplefs_is_shutdown(sb))
		return -EIO;

	err = 0;
	if (READ_ONCE(SIMPLEFS_INODE(inode)->prealloc_len)) {
		inode_lock(inode);
		if (SIMPLEFS_INODE(inode)->prealloc_len)
			err = simplefs_discard_prealloc(file, inode);
		inode_unlock(inode);
	}
	if (err)
		return err;

	err = file_write_and_wait_range(file, start, end);
	simplefs_wait_ioend_conversions(inode);
	if (err)
		return err;

	/* Push the inode snapshot into JBD2, then wait for the commit record.
	 * Writing the bdev mapping directly here would bypass write-ahead order. */
	err = sync_inode_metadata(inode, 1);
	if (sbi->s_journal && sbi->s_journal->j_jbd2) {
		err2 = simplefs_journal_force_commit(sbi->s_journal);
		if (!err)
			err = err2;
		return err;
	}

	err2 = filemap_write_and_wait_range(sb->s_bdev->bd_mapping, 0, LLONG_MAX);
	if (!err)
		err = err2;
	err2 = blkdev_issue_flush(sb->s_bdev);
	if (!err)
		err = err2;
	return err;
}

static loff_t simplefs_llseek(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file_inode(file);
	loff_t ret;

	switch (whence) {
	case SEEK_HOLE:
		inode_lock_shared(inode);
		ret = iomap_seek_hole(inode, offset, &simplefs_read_iomap_ops);
		inode_unlock_shared(inode);
		return ret;
	case SEEK_DATA:
		inode_lock_shared(inode);
		ret = iomap_seek_data(inode, offset, &simplefs_read_iomap_ops);
		inode_unlock_shared(inode);
		return ret;
	default:
		return generic_file_llseek(file, offset, whence);
	}
}

const struct file_operations simple_fs_iomap_fops = {
	.open = simplefs_file_open,
	.release = simplefs_file_release,
	.llseek = simplefs_llseek,
	.owner = THIS_MODULE,
	.read_iter = simplefs_file_read_iter,
	.write_iter = simplefs_file_write_iter,
	.mmap_prepare = simplefs_file_mmap_prepare,
	.get_unmapped_area = thp_get_unmapped_area,
	.splice_read = filemap_splice_read,
	.splice_write = iter_file_splice_write,
	.fsync = simplefs_file_fsync,
	.fallocate = simplefs_fallocate,
	/* NOTE: remap_file_range (reflink) removed - simplefs lacks CoW support,
	 * so sharing physical blocks corrupts data on write. Without this,
	 * copy_file_range correctly falls back to splice. 跨设备 CFR 与
	 * ext2/ext4 一样返回 EXDEV（generic/565 合法跳过）：在持
	 * file_start_write 的 copy_file_range 回调里调 do_splice_direct
	 * 会造成 sb_writers 递归加锁，不能用它来实现跨设备拷贝。
	 */
	.unlocked_ioctl = simplefs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = simplefs_compat_ioctl,
#endif
};
