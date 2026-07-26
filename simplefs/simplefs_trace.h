// SimpleFS Tracepoint 定义头文件
// 通过脚本自动生成，不要手动修改

#undef TRACE_SYSTEM
#define TRACE_SYSTEM simplefs

#if !defined(_SIMPLEFS_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _SIMPLEFS_TRACE_H

#include <linux/tracepoint.h>
#include <linux/fs.h>

/* ========== 文件 I/O 事件 ========== */

TRACE_EVENT(simplefs_read_page,
    TP_PROTO(struct inode *inode, pgoff_t index, size_t size, bool async),
    TP_ARGS(inode, index, size, async),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(unsigned long, ino)
        __field(pgoff_t, index)
        __field(size_t, size)
        __field(bool, async)
    ),
    
    TP_fast_assign(
        __entry->dev = inode->i_sb->s_dev;
        __entry->ino = inode->i_ino;
        __entry->index = index;
        __entry->size = size;
        __entry->async = async;
    ),
    
    TP_printk("dev=%d,%d ino=%lu index=%lu size=%zu async=%d",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->ino, __entry->index, __entry->size, __entry->async)
);

TRACE_EVENT(simplefs_write_page,
    TP_PROTO(struct inode *inode, pgoff_t index, size_t size, bool async),
    TP_ARGS(inode, index, size, async),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(unsigned long, ino)
        __field(pgoff_t, index)
        __field(size_t, size)
        __field(bool, async)
    ),
    
    TP_fast_assign(
        __entry->dev = inode->i_sb->s_dev;
        __entry->ino = inode->i_ino;
        __entry->index = index;
        __entry->size = size;
        __entry->async = async;
    ),
    
    TP_printk("dev=%d,%d ino=%lu index=%lu size=%zu async=%d",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->ino, __entry->index, __entry->size, __entry->async)
);

/* ========== 块分配事件 ========== */

TRACE_EVENT(simplefs_alloc_blocks,
    TP_PROTO(struct super_block *sb, uint32_t start, uint32_t count,
             const char *caller),
    TP_ARGS(sb, start, count, caller),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(uint32_t, start)
        __field(uint32_t, count)
        __string(caller, caller)
    ),
    
    TP_fast_assign(
        __entry->dev = sb->s_dev;
        __entry->start = start;
        __entry->count = count;
        __assign_str(caller);
    ),
    
    TP_printk("dev=%d,%d start=%u count=%u caller=%s",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->start, __entry->count, __get_str(caller))
);

TRACE_EVENT(simplefs_free_blocks,
    TP_PROTO(struct super_block *sb, uint32_t start, uint32_t count),
    TP_ARGS(sb, start, count),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(uint32_t, start)
        __field(uint32_t, count)
    ),
    
    TP_fast_assign(
        __entry->dev = sb->s_dev;
        __entry->start = start;
        __entry->count = count;
    ),
    
    TP_printk("dev=%d,%d start=%u count=%u",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->start, __entry->count)
);

/* ========== Inode 生命周期事件 ========== */

TRACE_EVENT(simplefs_create_inode,
    TP_PROTO(struct inode *inode, umode_t mode),
    TP_ARGS(inode, mode),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(unsigned long, ino)
        __field(umode_t, mode)
        __field(size_t, size)
    ),
    
    TP_fast_assign(
        __entry->dev = inode->i_sb->s_dev;
        __entry->ino = inode->i_ino;
        __entry->mode = mode;
        __entry->size = inode->i_size;
    ),
    
    TP_printk("dev=%d,%d ino=%lu mode=0%o size=%zu",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->ino, __entry->mode, __entry->size)
);

TRACE_EVENT(simplefs_evict_inode,
    TP_PROTO(struct inode *inode, unsigned long ino),
    TP_ARGS(inode, ino),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(unsigned long, ino)
        __field(size_t, size)
        __field(int, nlink)
    ),
    
    TP_fast_assign(
        __entry->dev = inode->i_sb->s_dev;
        __entry->ino = ino;
        __entry->size = inode->i_size;
        __entry->nlink = inode->i_nlink;
    ),
    
    TP_printk("dev=%d,%d ino=%lu size=%zu nlink=%d",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->ino, __entry->size, __entry->nlink)
);

/* ========== 目录操作事件 ========== */

TRACE_EVENT(simplefs_lookup,
    TP_PROTO(struct inode *dir, const char *name, struct inode *result),
    TP_ARGS(dir, name, result),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(unsigned long, dir_ino)
        __string(name, name)
        __field(unsigned long, result_ino)
        __field(int, err)
    ),
    
    TP_fast_assign(
        __entry->dev = dir->i_sb->s_dev;
        __entry->dir_ino = dir->i_ino;
        __assign_str(name);
        __entry->result_ino = result ? result->i_ino : 0;
        __entry->err = result ? 0 : -ENOENT;
    ),
    
    TP_printk("dev=%d,%d dir=%lu name=%s result=%lu err=%d",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->dir_ino, __get_str(name), __entry->result_ino, __entry->err)
);

/* ========== 事务日志事件 ========== */

TRACE_EVENT(simplefs_journal_start,
    TP_PROTO(struct super_block *sb, uint32_t tid, int credits, const char *caller),
    TP_ARGS(sb, tid, credits, caller),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(uint32_t, tid)
        __field(int, credits)
        __string(caller, caller)
    ),
    
    TP_fast_assign(
        __entry->dev = sb->s_dev;
        __entry->tid = tid;
        __entry->credits = credits;
        __assign_str(caller);
    ),
    
    TP_printk("dev=%d,%d tid=%u credits=%d caller=%s",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->tid, __entry->credits, __get_str(caller))
);

TRACE_EVENT(simplefs_journal_stop,
    TP_PROTO(struct super_block *sb, uint32_t tid, int used_credits),
    TP_ARGS(sb, tid, used_credits),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(uint32_t, tid)
        __field(int, used_credits)
    ),
    
    TP_fast_assign(
        __entry->dev = sb->s_dev;
        __entry->tid = tid;
        __entry->used_credits = used_credits;
    ),
    
    TP_printk("dev=%d,%d tid=%u used_credits=%d",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->tid, __entry->used_credits)
);

TRACE_EVENT(simplefs_journal_abort,
    TP_PROTO(struct super_block *sb, uint32_t tid, int error),
    TP_ARGS(sb, tid, error),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(uint32_t, tid)
        __field(int, error)
    ),
    
    TP_fast_assign(
        __entry->dev = sb->s_dev;
        __entry->tid = tid;
        __entry->error = error;
    ),
    
    TP_printk("dev=%d,%d tid=%u error=%d",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->tid, __entry->error)
);

TRACE_EVENT(simplefs_journal_dirty_metadata,
    TP_PROTO(struct super_block *sb, uint32_t tid, sector_t block),
    TP_ARGS(sb, tid, block),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(uint32_t, tid)
        __field(sector_t, block)
    ),
    
    TP_fast_assign(
        __entry->dev = sb->s_dev;
        __entry->tid = tid;
        __entry->block = block;
    ),
    
    TP_printk("dev=%d,%d tid=%u block=%llu",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->tid, (unsigned long long)__entry->block)
);

TRACE_EVENT(simplefs_transaction_commit,
    TP_PROTO(struct super_block *sb, uint32_t tid, int num_blocks),
    TP_ARGS(sb, tid, num_blocks),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(uint32_t, tid)
        __field(int, num_blocks)
    ),
    
    TP_fast_assign(
        __entry->dev = sb->s_dev;
        __entry->tid = tid;
        __entry->num_blocks = num_blocks;
    ),
    
    TP_printk("dev=%d,%d tid=%u num_blocks=%d",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->tid, __entry->num_blocks)
);

/* ========== 错误事件 ========== */

TRACE_EVENT(simplefs_error,
    TP_PROTO(struct super_block *sb, int errno, const char *func),
    TP_ARGS(sb, errno, func),
    
    TP_STRUCT__entry(
        __field(dev_t, dev)
        __field(int, err)
        __string(func, func)
    ),
    
    TP_fast_assign(
        __entry->dev = sb ? sb->s_dev : 0;
        __entry->err = errno;
        __assign_str(func);
    ),
    
    TP_printk("dev=%d,%d err=%d func=%s",
        MAJOR(__entry->dev), MINOR(__entry->dev),
        __entry->err, __get_str(func))
);

#endif /* _SIMPLEFS_TRACE_H */

// 必须包含这行来生成代码
#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE simplefs_trace
#include <trace/define_trace.h>
