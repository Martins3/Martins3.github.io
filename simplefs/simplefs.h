#ifndef SIMPLEFS_H
#define SIMPLEFS_H

/*
 * SimpleFS keeps regular file I/O on iomap/folios.  The only buffer_head
 * user is simplefs_journal.c because the in-kernel JBD2 client API is
 * buffer_head based.  Filesystem code outside that adapter continues to pass
 * metadata block numbers and snapshots through simplefs_journal.h.
 */

/* source: https://en.wikipedia.org/wiki/Hexspeak */
#define SIMPLEFS_MAGIC 0xDEADCELL

#define SIMPLEFS_SB_BLOCK_NR 0

#define SIMPLEFS_BLOCK_SIZE (1 << 12) /* 4 KiB */
#define SIMPLEFS_MAX_EXTENTS                        \
	((SIMPLEFS_BLOCK_SIZE - sizeof(uint32_t)) / \
	 sizeof(struct simplefs_extent))
#define SIMPLEFS_FILE_EXTENTS_PER_LEAF                                \
	((SIMPLEFS_BLOCK_SIZE - sizeof(struct simplefs_extent_leaf_header)) / \
	 sizeof(struct simplefs_extent))
#define SIMPLEFS_FILE_INDEX_SLOTS                                  \
	((SIMPLEFS_BLOCK_SIZE - sizeof(struct simplefs_extent_root_header)) / \
	 sizeof(struct simplefs_extent_index))
/*
 * Keep large I/O from being split into a stream of tiny extent metadata
 * updates.  ee_len reserves only its high bit for unwritten state, so an
 * 8192-block (32 MiB) extent remains well within the on-disk field.
 */
#define SIMPLEFS_MAX_BLOCKS_PER_EXTENT 8192
#define SIMPLEFS_MAX_FILE_EXTENTS \
	(SIMPLEFS_FILE_EXTENTS_PER_LEAF * SIMPLEFS_FILE_INDEX_SLOTS)
#define SIMPLEFS_MAX_FILESIZE                                               \
	((uint64_t)SIMPLEFS_MAX_BLOCKS_PER_EXTENT * SIMPLEFS_BLOCK_SIZE *   \
	 SIMPLEFS_MAX_FILE_EXTENTS)

#define SIMPLEFS_FILENAME_LEN 255

#define SIMPLEFS_FILES_PER_BLOCK \
	(SIMPLEFS_BLOCK_SIZE / sizeof(struct simplefs_file))
#define SIMPLEFS_DIR_BLOCKS_PER_EXTENT 8
#define SIMPLEFS_FILES_PER_EXT \
	(SIMPLEFS_FILES_PER_BLOCK * SIMPLEFS_DIR_BLOCKS_PER_EXTENT)

#define SIMPLEFS_MAX_SUBFILES (SIMPLEFS_FILES_PER_EXT * SIMPLEFS_MAX_EXTENTS)

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/xattr.h>

/* inode->i_blocks and stat.st_blocks are always measured in 512-byte
 * sectors, independent of the filesystem block size.  The on-disk inode
 * keeps a count in SimpleFS blocks, so conversion is required at every VFS
 * accounting boundary.
 */
static inline blkcnt_t simplefs_blocks_to_sectors(uint64_t blocks)
{
	return blocks * (SIMPLEFS_BLOCK_SIZE / 512);
}

static inline uint64_t simplefs_sectors_to_blocks(blkcnt_t sectors)
{
	return sectors / (SIMPLEFS_BLOCK_SIZE / 512);
}

/* simplefs partition layout
 * +---------------+
 * |  superblock   |  1 block
 * +---------------+
 * |  inode store  |  sb->nr_istore_blocks blocks
 * +---------------+
 * | ifree bitmap  |  sb->nr_ifree_blocks blocks
 * +---------------+
 * | bfree bitmap  |  sb->nr_bfree_blocks blocks
 * +---------------+
 * |    data       |
 * |      blocks   |  rest of the blocks
 * +---------------+
 */

struct simplefs_inode {
	uint32_t i_mode; /* File mode */
	uint32_t i_uid; /* Owner id */
	uint32_t i_gid; /* Group id */
	uint32_t i_size; /* Size in bytes */
	uint32_t i_ctime; /* Inode change time */
	uint32_t i_atime; /* Access time */
	uint32_t i_mtime; /* Modification time */
	uint32_t i_ctime_nsec; /* Inode change time nanoseconds */
	uint32_t i_atime_nsec; /* Access time nanoseconds */
	uint32_t i_mtime_nsec; /* Modification time nanoseconds */
	uint32_t i_blocks; /* Allocated SimpleFS block count (not 512B sectors) */
	uint32_t i_nlink; /* Hard links count */
	uint32_t ei_block; /* Block with list of extents for this file */
	uint32_t i_xattr_block; /* Block number for xattr data (0 = none) */
	uint32_t i_generation; /* File-handle generation for inode reuse */
	uint32_t i_flags; /* chattr 标志（FS_*_FL：immutable/append/noatime/sync 等） */
	uint32_t i_crtime; /* Inode creation time (btime) */
	uint32_t i_crtime_nsec; /* Inode creation time nanoseconds */
	char i_data[32]; /* store symlink content */
};

/* simplefs 支持查看/修改的 inode 标志集合（FS_*_FL 取自 uapi/linux/fs.h） */
#define SIMPLEFS_FL_USER_VISIBLE \
	(FS_IMMUTABLE_FL | FS_APPEND_FL | FS_NOATIME_FL | FS_SYNC_FL | \
	 FS_DIRSYNC_FL | FS_NODUMP_FL)
#define SIMPLEFS_FL_USER_MODIFIABLE SIMPLEFS_FL_USER_VISIBLE

#define SIMPLEFS_INODES_PER_BLOCK \
	(SIMPLEFS_BLOCK_SIZE / sizeof(struct simplefs_inode))

#define SIMPLEFS_LABEL_MAX 63

struct simplefs_sb_info {
	uint32_t magic; /* Magic number */

	uint32_t nr_blocks; /* Total number of blocks (incl sb & inodes) */
	uint32_t nr_inodes; /* Total number of inodes */

	uint32_t nr_istore_blocks; /* Number of inode store blocks */
	uint32_t nr_ifree_blocks; /* Number of inode free bitmap blocks */
	uint32_t nr_bfree_blocks; /* Number of block free bitmap blocks */

	uint32_t nr_free_inodes; /* Number of free inodes */
	uint32_t nr_free_blocks; /* Number of free blocks */
	
	/* Journal support */
	uint32_t s_journal_present; /* Journal exists on filesystem */
	uint32_t s_journal_start;   /* Journal start block number */
	
	/* Mount options */
	uint32_t s_journal_mode;    /* 0=default, 1=disable, 2=no recovery */

	/* 卷标（FS_IOC_GETFSLABEL/SETFSLABEL，generic/492） */
	char s_volume_name[64];

	/* Set for the lifetime of a writable mount and cleared only after a
	 * successful journal recovery or clean unmount.  Appending it here keeps
	 * old images compatible because their superblock padding is zeroed. */
	uint32_t s_needs_recovery;

#ifdef __KERNEL__
	struct super_block *sb; /* owning VFS superblock */
	unsigned long *ifree_bitmap; /* In-memory free inodes bitmap */
	unsigned long *bfree_bitmap; /* In-memory free blocks bitmap */
	unsigned long *discard_pending; /* Free blocks awaiting post-commit discard */
	struct mutex bitmap_lock; /* 保护空闲位图和 free 计数 */
	int s_discard; /* Issue online discard after the freeing transaction commits */

	/* In-memory journal state */
	struct simplefs_journal *s_journal;

	/* Shutdown state */
	int s_shutdown;
#endif
};

#ifdef __KERNEL__

#include <linux/wait.h>

/* Forward declaration for journal */
struct simplefs_journal;

struct simplefs_inode_info {
	uint32_t ei_block; /* Block with list of extents for this file */
	uint32_t i_xattr_block; /* Block number for xattr data (0 = none) */
	uint32_t i_flags; /* chattr 标志（FS_*_FL） */
	struct timespec64 i_crtime; /* Inode creation time (btime) */
	char i_data[32];
	uint32_t prealloc_block; /* KEEP_SIZE 预分配起始逻辑块 */
	uint32_t prealloc_len; /* KEEP_SIZE 预分配块数 */
	struct mutex extent_lock; /* 串行化 extent tree 读写，避免读到半更新 root */
	atomic_t writeback_ioends; /* 等待异步 unwritten extent 转换 */
	wait_queue_head_t writeback_wait;
	struct inode vfs_inode;
};

void simplefs_wait_ioend_conversions(struct inode *inode);

struct simplefs_extent {
	uint32_t ee_block; /* first logical block extent covers */
	uint32_t ee_len; /* low bits = len, high bit = unwritten state */
	uint32_t ee_start; /* first physical block extent covers */
};

struct simplefs_extent_root_header {
	uint32_t eh_magic;
	uint16_t eh_entries;
	uint16_t eh_max;
};

struct simplefs_extent_index {
	uint32_t ei_block; /* first logical block covered by this leaf */
	uint32_t leaf_block; /* physical block storing the leaf */
	uint32_t nr_extents; /* valid extents in the leaf */
	uint32_t last_block; /* exclusive end of the last extent */
};

struct simplefs_extent_leaf_header {
	uint32_t eh_magic;
	uint16_t eh_entries;
	uint16_t eh_max;
};

#define SIMPLEFS_EXTENT_ROOT_MAGIC 0xF5007E01
#define SIMPLEFS_EXTENT_LEAF_MAGIC 0xF5007E02

struct simplefs_extent_root {
	struct simplefs_extent_root_header header;
	struct simplefs_extent_index indexes[SIMPLEFS_FILE_INDEX_SLOTS];
};

struct simplefs_extent_leaf {
	struct simplefs_extent_leaf_header header;
	struct simplefs_extent extents[SIMPLEFS_FILE_EXTENTS_PER_LEAF];
};

struct simplefs_file_ei_block {
	uint32_t nr_files; /* Number of files in directory */
	struct simplefs_extent extents[SIMPLEFS_MAX_EXTENTS];
};

/* extent state is encoded in ee_len to preserve on-disk layout */
#define SIMPLEFS_EXTENT_UNWRITTEN (1U << 31)
#define SIMPLEFS_EXTENT_LEN_MASK  (~SIMPLEFS_EXTENT_UNWRITTEN)

/*
 * Safely get extent length, capped at SIMPLEFS_MAX_BLOCKS_PER_EXTENT
 * to prevent hangs from corrupted on-disk data.
 */
static inline uint32_t simplefs_ext_len(const struct simplefs_extent *ext)
{
	uint32_t len = ext->ee_len & SIMPLEFS_EXTENT_LEN_MASK;

	if (len > SIMPLEFS_MAX_BLOCKS_PER_EXTENT)
		return SIMPLEFS_MAX_BLOCKS_PER_EXTENT;
	return len;
}

static inline bool simplefs_ext_unwritten(const struct simplefs_extent *ext)
{
	return ext->ee_len & SIMPLEFS_EXTENT_UNWRITTEN;
}

static inline void simplefs_ext_set_len(struct simplefs_extent *ext,
					uint32_t len)
{
	ext->ee_len = (ext->ee_len & SIMPLEFS_EXTENT_UNWRITTEN) |
		      (len & SIMPLEFS_EXTENT_LEN_MASK);
}

static inline void simplefs_ext_set_unwritten(struct simplefs_extent *ext,
					      bool unwritten)
{
	ext->ee_len &= SIMPLEFS_EXTENT_LEN_MASK;
	if (unwritten)
		ext->ee_len |= SIMPLEFS_EXTENT_UNWRITTEN;
}

static inline void simplefs_ext_init(struct simplefs_extent *ext,
				     uint32_t ee_block, uint32_t ee_len,
				     uint32_t ee_start, bool unwritten)
{
	ext->ee_block = ee_block;
	ext->ee_start = ee_start;
	ext->ee_len = ee_len & SIMPLEFS_EXTENT_LEN_MASK;
	if (unwritten)
		ext->ee_len |= SIMPLEFS_EXTENT_UNWRITTEN;
}

static inline uint32_t simplefs_ext_end(const struct simplefs_extent *ext)
{
	return ext->ee_block + simplefs_ext_len(ext);
}

static inline bool simplefs_extent_is_empty(const struct simplefs_extent *ext)
{
	return !ext->ee_start || !simplefs_ext_len(ext);
}

struct simplefs_file {
	uint32_t inode;
	char filename[SIMPLEFS_FILENAME_LEN];
};

struct simplefs_dir_block {
	struct simplefs_file files[SIMPLEFS_FILES_PER_BLOCK];
};

/* superblock functions */
int simplefs_fill_super(struct super_block *sb, struct fs_context *fc);
int simplefs_rebuild_block_bitmap(struct super_block *sb);

/* inode functions */
int simplefs_init_inode_cache(void);
void simplefs_destroy_inode_cache(void);
struct inode *simplefs_iget(struct super_block *sb, unsigned long ino);
int simplefs_persist_inode(struct inode *inode);
int simplefs_clear_disk_inode(struct inode *inode);
int simplefs_truncate(struct inode *inode, loff_t size);

/* file functions */
extern const struct file_operations simplefs_dir_ops;
extern const struct address_space_operations simplefs_iomap_aops;
extern const struct file_operations simple_fs_iomap_fops;
extern const struct iomap_ops simplefs_write_iomap_ops;
int simplefs_fiemap(struct inode *inode, struct fiemap_extent_info *fieinfo,
		    u64 start, u64 len);
int simplefs_zero_partial_gap(struct inode *inode, loff_t start, loff_t len);

void *simplefs_get_folio(struct super_block *sb, unsigned long n,
			 struct folio **foliop);
int simplefs_retire_metadata_blocks(struct super_block *sb, uint32_t bno,
				    uint32_t len);

/* extent functions */
extern uint32_t simplefs_ext_search(struct simplefs_file_ei_block *index,
				    uint32_t iblock);

#ifdef __KERNEL__
struct simplefs_extent_buffer {
	struct simplefs_extent *extents;
	uint32_t nr_extents;
	uint32_t capacity;
	uint32_t *leaf_blocks;
	uint32_t nr_leaf_blocks;
};

void simplefs_extent_clear(struct simplefs_extent *ext);
void simplefs_file_init_extent_root(void *block);
int simplefs_file_load_extents(struct super_block *sb, uint32_t root_block,
			       struct simplefs_extent_buffer *buf);
int simplefs_file_sync_extents(struct inode *inode,
			       struct simplefs_extent_buffer *buf);
void simplefs_file_destroy_extents(struct simplefs_extent_buffer *buf);
int simplefs_file_append_extent(struct simplefs_extent_buffer *buf,
				const struct simplefs_extent *ext);
void simplefs_file_normalize_extents(struct simplefs_extent_buffer *buf);
int simplefs_file_find_extent(struct simplefs_extent_buffer *buf, uint32_t iblock,
			      uint32_t *extent_idx, uint32_t *insert_idx);
int simplefs_file_remove_extent(struct simplefs_extent_buffer *buf,
				uint32_t extent_idx);
int simplefs_file_free_extent_tree(struct super_block *sb, uint32_t root_block);
#endif


struct simplfs_mount_data {
	char *options;
	const char *dev_name;
};

/* Mount options passed through fs_context */
struct simplefs_fs_context {
	int nojournal;  /* 1 = disable journal */
	int norecovery; /* 1 = skip replay; requires a read-only mount */
	int discard;    /* 1 = issue discard for blocks after durable free */
};

int simplefs_issue_pending_discards(struct super_block *sb);

/* Shutdown ioctl - same as XFS/ext4/f2fs */
#define SIMPLEFS_IOC_SHUTDOWN _IOR('X', 125, __u32)
#define SIMPLEFS_GOING_DOWN_FULLSYNC	0x0
#define SIMPLEFS_GOING_DOWN_METASYNC	0x1
#define SIMPLEFS_GOING_DOWN_NOSYNC	0x2

static inline int simplefs_is_shutdown(struct super_block *sb)
{
	return ((struct simplefs_sb_info *)sb->s_fs_info)->s_shutdown;
}

/* ioctl support */
long simplefs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
long simplefs_compat_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg);
#endif

/*
 * On-disk xattr block format:
 * +----------------------------------------------+
 * | simplefs_xattr_header (8 bytes)               |
 * +----------------------------------------------+
 * | entry1: simplefs_xattr_entry + name (grows ->) |
 * | entry2: ...                                    |
 * | ...                                            |
 * +----------------------------------------------+
 * |              (free space)                      |
 * +----------------------------------------------+
 * | ... value2 data (grows <-)                     |
 * | ... value1 data                                |
 * +----------------------------------------------+
 */
#define SIMPLEFS_XATTR_MAGIC 0x53584154 /* "SXAT" */

struct simplefs_xattr_header {
	uint32_t h_magic;
	uint32_t h_count; /* number of entries */
};

struct simplefs_xattr_entry {
	uint8_t e_name_len;
	uint8_t e_name_index; /* not used, always 0 */
	uint16_t e_value_offs; /* offset from block start */
	uint32_t e_value_size;
	char e_name[]; /* variable length */
};

#define SIMPLEFS_XATTR_ENTRY_SIZE(name_len) \
	ALIGN(sizeof(struct simplefs_xattr_entry) + (name_len), 4)

#define SIMPLEFS_XATTR_FIRST(hdr) \
	((struct simplefs_xattr_entry *)((char *)(hdr) + sizeof(struct simplefs_xattr_header)))

/* xattr functions */
int simplefs_xattr_get(struct inode *inode, const char *name,
		       void *buffer, size_t size);
int simplefs_xattr_set(struct inode *inode, const char *name,
		       const void *value, size_t size, int flags);
void simplefs_xattr_delete_inode(struct inode *inode);

/* xattr support */
extern const struct xattr_handler * const simplefs_xattr_handlers[];
ssize_t simplefs_listxattr(struct dentry *dentry, char *buffer, size_t size);

/* ACL support */
#ifdef CONFIG_FS_POSIX_ACL
struct posix_acl *simplefs_get_acl(struct inode *inode, int type, bool rcu);
int simplefs_set_acl(struct mnt_idmap *idmap, struct dentry *dentry,
		     struct posix_acl *acl, int type);
int simplefs_init_acl(struct inode *inode, struct inode *dir);
#else
static inline int simplefs_init_acl(struct inode *inode, struct inode *dir)
{
	return 0;
}
#endif

/* Getters for superbock and inode */
#define SIMPLEFS_SB(sb) (sb->s_fs_info)
#define SIMPLEFS_INODE(inode) \
	(container_of(inode, struct simplefs_inode_info, vfs_inode))
#endif /* __KERNEL__ */

#endif /* SIMPLEFS_H */
