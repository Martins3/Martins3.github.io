// SimpleFS mkfs 核心实现
// 分离纯计算逻辑和 I/O 操作，便于测试

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>  // for htonl/htole32

#include "mkfs_common.h"

/* ========== 纯计算函数 ========== */

int mkfs_calculate_layout(size_t image_size, struct mkfs_layout *layout)
{
    if (!layout)
        return -EINVAL;
    
    if (image_size <= SIMPLEFS_MIN_SIZE)
        return -E2BIG;  // 镜像太小
    
    memset(layout, 0, sizeof(*layout));
    
    // 总块数
    layout->nr_blocks = image_size / SIMPLEFS_BLOCK_SIZE;
    
    // inode 数（与块数相同，但需对齐）
    layout->nr_inodes = layout->nr_blocks;
    uint32_t mod = layout->nr_inodes % SIMPLEFS_INODES_PER_BLOCK;
    if (mod)
        layout->nr_inodes += SIMPLEFS_INODES_PER_BLOCK - mod;
    
    // 各区域块数
    layout->nr_istore_blocks = idiv_ceil(layout->nr_inodes, SIMPLEFS_INODES_PER_BLOCK);
    layout->nr_ifree_blocks = idiv_ceil(layout->nr_inodes, SIMPLEFS_BLOCK_SIZE * 8);
    layout->nr_bfree_blocks = idiv_ceil(layout->nr_blocks, SIMPLEFS_BLOCK_SIZE * 8);
    
    /* JBD2 requires at least 1024 blocks.  Do not disable the journal merely
     * because that minimum exceeds an arbitrary percentage of a small
     * filesystem: generic/042 deliberately creates a 25 MiB inner filesystem
     * and still requires LOGFLUSH recovery semantics.  Fall back to no journal
     * only when the fixed metadata, one root data block and the JBD2 minimum
     * genuinely cannot fit. */
    uint32_t fixed_blocks = 1 + layout->nr_istore_blocks +
                            layout->nr_ifree_blocks +
                            layout->nr_bfree_blocks;

    layout->nr_journal_blocks = 0;
    if (layout->nr_blocks > fixed_blocks + SIMPLEFS_JOURNAL_BLOCKS)
        layout->nr_journal_blocks = SIMPLEFS_JOURNAL_BLOCKS;
    
    // Journal 起始位置（放在数据区末尾，预留空间）
    // 注意：这是 journal 的预留位置，实际通过 inode 访问
    layout->journal_start_block = layout->nr_blocks - layout->nr_journal_blocks;
    
    // 数据块（不包括 journal 区域）
    layout->nr_data_blocks = layout->nr_blocks - 1 -  // superblock
                             layout->nr_istore_blocks -
                             layout->nr_ifree_blocks -
                             layout->nr_bfree_blocks -
                             layout->nr_journal_blocks;
    
    // 第一个数据块号
    layout->first_data_block = 1 + layout->nr_istore_blocks +
                               layout->nr_ifree_blocks +
                               layout->nr_bfree_blocks;
    
    return 0;
}

bool mkfs_validate_layout(const struct mkfs_layout *layout)
{
    if (!layout)
        return false;
    
    // 基本检查
    if (layout->nr_blocks == 0)
        return false;
    
    if (layout->nr_inodes == 0)
        return false;
    
    if (layout->nr_data_blocks == 0)
        return false;  // 没有数据块
    
    // 布局检查：元数据块 + 数据块 = 总块数
    uint32_t used_blocks = 1 + layout->nr_istore_blocks +
                           layout->nr_ifree_blocks +
                           layout->nr_bfree_blocks;
    
    if (used_blocks >= layout->nr_blocks)
        return false;  // 元数据占满整个文件系统
    
    // 验证：istore 块足够存储所有 inode
    if (layout->nr_istore_blocks * SIMPLEFS_INODES_PER_BLOCK < layout->nr_inodes)
        return false;
    
    return true;
}

void mkfs_init_superblock(struct simplefs_superblock *sb,
                          const struct mkfs_layout *layout)
{
    if (!sb || !layout)
        return;
    
    memset(sb, 0, sizeof(*sb));
    
    sb->info.magic = htole32(SIMPLEFS_MAGIC);
    sb->info.nr_blocks = htole32(layout->nr_blocks);
    sb->info.nr_inodes = htole32(layout->nr_inodes);
    sb->info.nr_istore_blocks = htole32(layout->nr_istore_blocks);
    sb->info.nr_ifree_blocks = htole32(layout->nr_ifree_blocks);
    sb->info.nr_bfree_blocks = htole32(layout->nr_bfree_blocks);
    sb->info.nr_free_inodes = htole32(layout->nr_inodes - 1);  // 根inode已用
    sb->info.nr_free_blocks = htole32(layout->nr_data_blocks - 1);  // 第一个数据块已用
    
    // 标记 journal 是否存在
    sb->info.s_journal_present = (layout->nr_journal_blocks > 0) ? 1 : 0;
    sb->info.s_journal_start = htole32(layout->journal_start_block);
    sb->info.s_needs_recovery = htole32(0);
}

void mkfs_init_root_inode(struct simplefs_inode *inode,
                          uint32_t first_data_block)
{
    if (!inode)
        return;
    
    memset(inode, 0, sizeof(*inode));
    
    inode->i_mode = htole32(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | 
                            S_IWUSR | S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH);
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_size = htole32(SIMPLEFS_BLOCK_SIZE);
    inode->i_ctime = htole32(0);
    inode->i_atime = htole32(0);
    inode->i_mtime = htole32(0);
    inode->i_blocks = htole32(1);
    inode->i_nlink = htole32(2);  // . 和 ..
    inode->ei_block = htole32(first_data_block);
}

void mkfs_init_journal_superblock(struct simplefs_jbd2_superblock *jsb,
				  uint32_t journal_blocks)
{
    if (!jsb)
        return;

    memset(jsb, 0, sizeof(*jsb));

    jsb->s_header.h_magic = htobe32(SIMPLEFS_JBD2_MAGIC);
    jsb->s_header.h_blocktype = htobe32(SIMPLEFS_JBD2_SUPERBLOCK_V2);
    jsb->s_header.h_sequence = htobe32(0);
    jsb->s_blocksize = htobe32(SIMPLEFS_BLOCK_SIZE);
    jsb->s_maxlen = htobe32(journal_blocks);
    jsb->s_first = htobe32(1);
    jsb->s_sequence = htobe32(1);
    jsb->s_start = htobe32(0); /* clean journal */
    jsb->s_nr_users = htobe32(1);
}

uint64_t mkfs_calc_ifree_value(uint32_t block_idx)
{
    if (block_idx == 0) {
        // 第一个块：bit 0-1 已用（0），其余空闲（1）
        // 0xfffffffffffffffc = 0b...11111100
        return 0xfffffffffffffffcULL;
    }
    // 其他块：全部空闲
    return 0xffffffffffffffffULL;
}

int mkfs_calc_bfree_values(uint32_t block_idx, uint32_t nr_used,
                           uint64_t *values, int max_values)
{
    if (!values || max_values < 1)
        return -EINVAL;
    
    // 计算需要在当前块标记的已用位数
    uint32_t bits_per_block = SIMPLEFS_BLOCK_SIZE * 8;
    uint32_t block_start_bit = block_idx * bits_per_block;
    
    if (block_start_bit >= nr_used) {
        // 这个块全部空闲
        values[0] = 0xffffffffffffffffULL;
        return 1;
    }
    
    // 填充 64 位值
    int count = 0;
    uint32_t remaining_used = nr_used - block_start_bit;
    
    for (int i = 0; i < max_values && remaining_used > 0; i++) {
        uint64_t val = 0xffffffffffffffffULL;
        
        // 在这一组 64 位中标记已用的位
        for (int bit = 0; bit < 64 && remaining_used > 0; bit++) {
            val &= ~(1ULL << bit);
            remaining_used--;
        }
        
        values[count++] = val;
    }
    
    return count;
}

/* ========== I/O 函数 ========== */

int mkfs_write_superblock(int fd, const struct simplefs_superblock *sb)
{
    if (fd < 0 || !sb)
        return -EINVAL;
    
    // 写入 block 0
    if (lseek(fd, 0, SEEK_SET) < 0)
        return -errno;
    
    ssize_t written = write(fd, sb, sizeof(*sb));
    if (written != sizeof(*sb))
        return (written < 0) ? -errno : -EIO;
    
    // 同步确保写入磁盘
    if (fsync(fd) < 0)
        return -errno;
    
    return 0;
}

int mkfs_write_journal(int fd, const struct mkfs_layout *layout)
{
    if (fd < 0 || !layout)
        return -EINVAL;
    
    if (layout->nr_journal_blocks == 0)
        return 0;  // No journal
    
    // Allocate and zero out journal area
    char *zero_buf = calloc(1, SIMPLEFS_BLOCK_SIZE);
    if (!zero_buf)
        return -ENOMEM;
    
    // Write a standard JBD2 v2 superblock at the beginning of the journal.
    struct simplefs_jbd2_superblock jsb;
    mkfs_init_journal_superblock(&jsb, layout->nr_journal_blocks);
	memcpy(zero_buf, &jsb, sizeof(jsb));
    
    off_t journal_offset = (off_t)layout->journal_start_block * SIMPLEFS_BLOCK_SIZE;
    if (lseek(fd, journal_offset, SEEK_SET) < 0) {
        free(zero_buf);
        return -errno;
    }
    
    // Always write full blocks; the previous short superblock write shifted
    // every following journal block by sizeof(jsb).
    ssize_t written = write(fd, zero_buf, SIMPLEFS_BLOCK_SIZE);
    if (written != SIMPLEFS_BLOCK_SIZE) {
        free(zero_buf);
        return (written < 0) ? -errno : -EIO;
    }
	memset(zero_buf, 0, SIMPLEFS_BLOCK_SIZE);
    
    // Write zeros for the rest of journal blocks
    for (uint32_t i = 1; i < layout->nr_journal_blocks; i++) {
        written = write(fd, zero_buf, SIMPLEFS_BLOCK_SIZE);
        if (written != SIMPLEFS_BLOCK_SIZE) {
            free(zero_buf);
            return (written < 0) ? -errno : -EIO;
        }
    }
    
    free(zero_buf);
    
    // Sync to ensure journal is written
    if (fsync(fd) < 0)
        return -errno;
    
    return 0;
}

int mkfs_write_inode_store(int fd, const struct mkfs_layout *layout)
{
    if (fd < 0 || !layout)
        return -EINVAL;
    
    char *block = calloc(1, SIMPLEFS_BLOCK_SIZE);
    if (!block)
        return -ENOMEM;
    
    // 第一个块：包含根 inode（inode 1）
    struct simplefs_inode *inodes = (struct simplefs_inode *)block;
    mkfs_init_root_inode(&inodes[1], layout->first_data_block);
    
    // 写入第一个块
    off_t offset = SIMPLEFS_BLOCK_SIZE;  // block 1
    if (lseek(fd, offset, SEEK_SET) < 0) {
        free(block);
        return -errno;
    }
    
    ssize_t written = write(fd, block, SIMPLEFS_BLOCK_SIZE);
    if (written != SIMPLEFS_BLOCK_SIZE) {
        free(block);
        return (written < 0) ? -errno : -EIO;
    }
    
    // 其余块：全部清零
    memset(block, 0, SIMPLEFS_BLOCK_SIZE);
    for (uint32_t i = 1; i < layout->nr_istore_blocks; i++) {
        written = write(fd, block, SIMPLEFS_BLOCK_SIZE);
        if (written != SIMPLEFS_BLOCK_SIZE) {
            free(block);
            return (written < 0) ? -errno : -EIO;
        }
    }
    
    free(block);
    return 0;
}

static int mkfs_init_root_index_block(int fd, const struct mkfs_layout *layout)
{
    char block[SIMPLEFS_BLOCK_SIZE];
    ssize_t written;
    off_t offset;

    if (fd < 0 || !layout)
        return -EINVAL;

    memset(block, 0, sizeof(block));
    offset = (off_t)layout->first_data_block * SIMPLEFS_BLOCK_SIZE;
    if (lseek(fd, offset, SEEK_SET) < 0)
        return -errno;

    written = write(fd, block, sizeof(block));
    if (written != sizeof(block))
        return (written < 0) ? -errno : -EIO;

    return 0;
}

int mkfs_write_ifree_bitmap(int fd, const struct mkfs_layout *layout)
{
    if (fd < 0 || !layout)
        return -EINVAL;
    
    char *block = malloc(SIMPLEFS_BLOCK_SIZE);
    if (!block)
        return -ENOMEM;
    
    off_t offset = SIMPLEFS_BLOCK_SIZE * (1 + layout->nr_istore_blocks);
    if (lseek(fd, offset, SEEK_SET) < 0) {
        free(block);
        return -errno;
    }
    
    for (uint32_t i = 0; i < layout->nr_ifree_blocks; i++) {
        uint64_t val = mkfs_calc_ifree_value(i);
        
        // 填充整个块
        uint64_t *block_vals = (uint64_t *)block;
        for (size_t j = 0; j < SIMPLEFS_BLOCK_SIZE / sizeof(uint64_t); j++) {
            block_vals[j] = val;
        }
        
        ssize_t written = write(fd, block, SIMPLEFS_BLOCK_SIZE);
        if (written != SIMPLEFS_BLOCK_SIZE) {
            free(block);
            return (written < 0) ? -errno : -EIO;
        }
    }
    
    free(block);
    return 0;
}

int mkfs_write_bfree_bitmap(int fd, const struct mkfs_layout *layout)
{
    if (fd < 0 || !layout)
        return -EINVAL;
    
    // 计算已用块数
    uint32_t nr_used = 1 + layout->nr_istore_blocks + 
                       layout->nr_ifree_blocks + 
                       layout->nr_bfree_blocks + 1;  // +1 for first data block
    
    char *block = malloc(SIMPLEFS_BLOCK_SIZE);
    if (!block)
        return -ENOMEM;
    
    off_t offset = SIMPLEFS_BLOCK_SIZE * (1 + layout->nr_istore_blocks + 
                                          layout->nr_ifree_blocks);
    if (lseek(fd, offset, SEEK_SET) < 0) {
        free(block);
        return -errno;
    }
    
    for (uint32_t i = 0; i < layout->nr_bfree_blocks; i++) {
        uint32_t block_start = i * SIMPLEFS_BLOCK_SIZE * 8;

        memset(block, 0xff, SIMPLEFS_BLOCK_SIZE);
        for (uint32_t bit = 0; bit < SIMPLEFS_BLOCK_SIZE * 8; bit++) {
            uint32_t block_no = block_start + bit;

            if (block_no >= layout->nr_blocks)
                break;
            if (block_no < nr_used ||
                (layout->nr_journal_blocks &&
                 block_no >= layout->journal_start_block))
                block[bit / 8] &= (char)~(1U << (bit % 8));
        }
        
        ssize_t written = write(fd, block, SIMPLEFS_BLOCK_SIZE);
        if (written != SIMPLEFS_BLOCK_SIZE) {
            free(block);
            return (written < 0) ? -errno : -EIO;
        }
    }
    
    free(block);
    return 0;
}

/* ========== 完整创建流程 ========== */

int mkfs_create_fs(int fd, size_t image_size, struct mkfs_result *result)
{
    if (!result)
        return -EINVAL;
    
    memset(result, 0, sizeof(*result));
    
    // 步骤 1: 计算布局
    struct mkfs_layout layout;
    int ret = mkfs_calculate_layout(image_size, &layout);
    if (ret < 0) {
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to calculate layout: image size %zu too small (min %d)",
                 image_size, SIMPLEFS_MIN_SIZE);
        return ret;
    }
    
    if (!mkfs_validate_layout(&layout)) {
        result->error_code = -EINVAL;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Invalid layout calculated");
        return -EINVAL;
    }
    
    // 步骤 2: 创建 superblock
    struct simplefs_superblock sb;
    mkfs_init_superblock(&sb, &layout);
    
    ret = mkfs_write_superblock(fd, &sb);
    if (ret < 0) {
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to write superblock: %s", strerror(-ret));
        return ret;
    }
    
    // 步骤 3: 写入 inode store
    ret = mkfs_write_inode_store(fd, &layout);
    if (ret < 0) {
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to write inode store: %s", strerror(-ret));
        return ret;
    }

    ret = mkfs_init_root_index_block(fd, &layout);
    if (ret < 0) {
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to initialize root index block: %s",
                 strerror(-ret));
        return ret;
    }
    
    // 步骤 4: 写入 ifree bitmap
    ret = mkfs_write_ifree_bitmap(fd, &layout);
    if (ret < 0) {
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to write ifree bitmap: %s", strerror(-ret));
        return ret;
    }
    
    // 步骤 5: 写入 bfree bitmap
    ret = mkfs_write_bfree_bitmap(fd, &layout);
    if (ret < 0) {
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to write bfree bitmap: %s", strerror(-ret));
        return ret;
    }
    
    // 步骤 6: 写入 journal
    ret = mkfs_write_journal(fd, &layout);
    if (ret < 0) {
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to write journal: %s", strerror(-ret));
        return ret;
    }
    
    // 成功
    result->error_code = 0;
    memcpy(&result->sb_info, &sb.info, sizeof(result->sb_info));
    
    return 0;
}

/* ========== 验证函数 ========== */

int mkfs_verify_superblock(int fd, const struct mkfs_layout *expected)
{
    if (fd < 0)
        return -EINVAL;
    
    struct simplefs_superblock sb;
    
    if (lseek(fd, 0, SEEK_SET) < 0)
        return -errno;
    
    ssize_t n = read(fd, &sb, sizeof(sb));
    if (n != sizeof(sb))
        return (n < 0) ? -errno : -EIO;
    
    // 验证 magic
    if (le32toh(sb.info.magic) != SIMPLEFS_MAGIC)
        return -EINVAL;
    
    // 如果提供了期望的布局，验证各个字段
    if (expected) {
        if (le32toh(sb.info.nr_blocks) != expected->nr_blocks)
            return -EINVAL;
        if (le32toh(sb.info.nr_inodes) != expected->nr_inodes)
            return -EINVAL;
        if (le32toh(sb.info.nr_istore_blocks) != expected->nr_istore_blocks)
            return -EINVAL;
        if (le32toh(sb.info.nr_ifree_blocks) != expected->nr_ifree_blocks)
            return -EINVAL;
        if (le32toh(sb.info.nr_bfree_blocks) != expected->nr_bfree_blocks)
            return -EINVAL;
    }
    
    return 0;
}

int mkfs_verify_root_inode(int fd, const struct mkfs_layout *layout)
{
    if (fd < 0 || !layout)
        return -EINVAL;
    
    // 读取 inode 1（根目录）
    off_t offset = SIMPLEFS_BLOCK_SIZE + sizeof(struct simplefs_inode);
    
    if (lseek(fd, offset, SEEK_SET) < 0)
        return -errno;
    
    struct simplefs_inode inode;
    ssize_t n = read(fd, &inode, sizeof(inode));
    if (n != sizeof(inode))
        return (n < 0) ? -errno : -EIO;
    
    // 验证模式：目录 + 权限
    uint32_t mode = le32toh(inode.i_mode);
    if (!S_ISDIR(mode))
        return -EINVAL;
    // 原始代码使用 S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IXUSR|S_IXGRP|S_IXOTH = 0775
    if ((mode & 0777) != 0775)
        return -EINVAL;
    
    // 验证大小
    if (le32toh(inode.i_size) != SIMPLEFS_BLOCK_SIZE)
        return -EINVAL;
    
    // 验证 link count
    if (le32toh(inode.i_nlink) != 2)
        return -EINVAL;
    
    // 验证数据块指针
    if (le32toh(inode.ei_block) != layout->first_data_block)
        return -EINVAL;
    
    return 0;
}
