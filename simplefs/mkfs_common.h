// SimpleFS mkfs 公共头文件
// 定义可测试的接口和数据结构

#ifndef MKFS_COMMON_H
#define MKFS_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "simplefs.h"
#include "simplefs_journal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#define SIMPLEFS_MIN_SIZE (100 * SIMPLEFS_BLOCK_SIZE)  // 400KB 最小大小

/* ========== 数据结构 ========== */

// Superblock 内存结构（包含填充）
struct simplefs_superblock {
    struct simplefs_sb_info info;
    char padding[SIMPLEFS_BLOCK_SIZE - sizeof(struct simplefs_sb_info)];
};

// mkfs 操作结果
struct mkfs_result {
    int error_code;           // 0 = 成功，负数 = 错误码
    char error_msg[256];      // 错误描述
    struct simplefs_sb_info sb_info;  // 创建的 superblock 信息（副本）
};

// 布局计算结果
struct mkfs_layout {
    uint32_t nr_blocks;           // 总块数
    uint32_t nr_inodes;           // inode 总数（对齐后）
    uint32_t nr_istore_blocks;    // inode store 块数
    uint32_t nr_ifree_blocks;     // inode bitmap 块数
    uint32_t nr_bfree_blocks;     // block bitmap 块数
    uint32_t nr_journal_blocks;   // journal 块数（0 = 无日志）
    uint32_t nr_data_blocks;      // 数据块数
    uint32_t first_data_block;    // 第一个数据块号
    uint32_t journal_start_block; // journal 起始块号
};

/* ========== 纯计算函数（可单元测试） ========== */

/**
 * 计算 ceil(a/b)
 */
static inline uint32_t idiv_ceil(uint32_t a, uint32_t b)
{
    return (a + b - 1) / b;
}

/**
 * 计算文件系统布局
 * @param image_size: 镜像大小（字节）
 * @param layout: 输出布局结构
 * @return: 0 成功，-1 失败（镜像太小）
 */
int mkfs_calculate_layout(size_t image_size, struct mkfs_layout *layout);

/**
 * 验证布局是否合理
 */
bool mkfs_validate_layout(const struct mkfs_layout *layout);

/**
 * 初始化 superblock 结构（内存操作，无 I/O）
 */
void mkfs_init_superblock(struct simplefs_superblock *sb, 
                          const struct mkfs_layout *layout);

/**
 * 初始化根 inode（内存操作）
 */
void mkfs_init_root_inode(struct simplefs_inode *inode, 
                          uint32_t first_data_block);

/**
 * 初始化 journal superblock（内存操作）
 * @param jsb: JBD2 journal superblock 指针
 * @param journal_blocks: journal 区域总块数
 */
void mkfs_init_journal_superblock(struct simplefs_jbd2_superblock *jsb,
				  uint32_t journal_blocks);

/**
 * 写入 journal 区域
 * @param fd: 文件描述符
 * @param layout: 布局结构
 * @return: 0 成功，负数错误码
 */
int mkfs_write_journal(int fd, const struct mkfs_layout *layout);

/**
 * 计算 ifree bitmap 初始值
 * @param block_idx: bitmap 块索引
 * @return: 64位初始值
 */
uint64_t mkfs_calc_ifree_value(uint32_t block_idx);

/**
 * 计算 bfree bitmap 初始值
 * @param block_idx: bitmap 块索引
 * @param nr_used: 已用块数
 * @param values: 输出数组（每个元素是一个 uint64_t）
 * @param max_values: 数组最大长度
 * @return: 实际使用的数组元素数
 */
int mkfs_calc_bfree_values(uint32_t block_idx, uint32_t nr_used,
                           uint64_t *values, int max_values);

/* ========== I/O 函数（需要文件描述符） ========== */

/**
 * 写入 superblock
 */
int mkfs_write_superblock(int fd, const struct simplefs_superblock *sb);

/**
 * 写入 inode store（包含根 inode）
 */
int mkfs_write_inode_store(int fd, const struct mkfs_layout *layout);

/**
 * 写入 inode free bitmap
 */
int mkfs_write_ifree_bitmap(int fd, const struct mkfs_layout *layout);

/**
 * 写入 block free bitmap
 */
int mkfs_write_bfree_bitmap(int fd, const struct mkfs_layout *layout);

/**
 * 完整创建文件系统
 * 这是主要的入口函数
 */
int mkfs_create_fs(int fd, size_t image_size, struct mkfs_result *result);

/* ========== 验证函数（用于测试） ========== */

/**
 * 读取并验证 superblock
 */
int mkfs_verify_superblock(int fd, const struct mkfs_layout *expected);

/**
 * 读取并验证根 inode
 */
int mkfs_verify_root_inode(int fd, const struct mkfs_layout *layout);

/**
 * 读取并验证 bitmap
 */
int mkfs_verify_bitmap(int fd, uint32_t block_start, uint32_t block_count,
                       const char *type);  // "ifree" 或 "bfree"

#ifdef __cplusplus
}
#endif

#endif /* MKFS_COMMON_H */
