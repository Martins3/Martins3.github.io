// SimpleFS Tracepoint 实现
// 参考 Documentation/trace/tracepoints.rst

#include <linux/module.h>
#include <linux/fs.h>
#include "simplefs_trace.h"

// 定义 tracepoint
#define CREATE_TRACE_POINTS
#include "simplefs_trace.h"

// 导出符号，允许模块使用
EXPORT_TRACEPOINT_SYMBOL(simplefs_read_page);
EXPORT_TRACEPOINT_SYMBOL(simplefs_write_page);
EXPORT_TRACEPOINT_SYMBOL(simplefs_alloc_blocks);
EXPORT_TRACEPOINT_SYMBOL(simplefs_free_blocks);
EXPORT_TRACEPOINT_SYMBOL(simplefs_create_inode);
EXPORT_TRACEPOINT_SYMBOL(simplefs_evict_inode);
EXPORT_TRACEPOINT_SYMBOL(simplefs_lookup);
EXPORT_TRACEPOINT_SYMBOL(simplefs_error);
