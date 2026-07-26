#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_DATA(__tracepoint_simplefs_read_page, "");
SYMBOL_FLAGS(__tracepoint_simplefs_read_page, 0x00);
KSYMTAB_FUNC(__traceiter_simplefs_read_page, "");
SYMBOL_FLAGS(__traceiter_simplefs_read_page, 0x00);
KSYMTAB_DATA(__SCK__tp_func_simplefs_read_page, "");
SYMBOL_FLAGS(__SCK__tp_func_simplefs_read_page, 0x00);
KSYMTAB_FUNC(__SCT__tp_func_simplefs_read_page, "");
SYMBOL_FLAGS(__SCT__tp_func_simplefs_read_page, 0x00);
KSYMTAB_DATA(__tracepoint_simplefs_write_page, "");
SYMBOL_FLAGS(__tracepoint_simplefs_write_page, 0x00);
KSYMTAB_FUNC(__traceiter_simplefs_write_page, "");
SYMBOL_FLAGS(__traceiter_simplefs_write_page, 0x00);
KSYMTAB_DATA(__SCK__tp_func_simplefs_write_page, "");
SYMBOL_FLAGS(__SCK__tp_func_simplefs_write_page, 0x00);
KSYMTAB_FUNC(__SCT__tp_func_simplefs_write_page, "");
SYMBOL_FLAGS(__SCT__tp_func_simplefs_write_page, 0x00);
KSYMTAB_DATA(__tracepoint_simplefs_alloc_blocks, "");
SYMBOL_FLAGS(__tracepoint_simplefs_alloc_blocks, 0x00);
KSYMTAB_FUNC(__traceiter_simplefs_alloc_blocks, "");
SYMBOL_FLAGS(__traceiter_simplefs_alloc_blocks, 0x00);
KSYMTAB_DATA(__SCK__tp_func_simplefs_alloc_blocks, "");
SYMBOL_FLAGS(__SCK__tp_func_simplefs_alloc_blocks, 0x00);
KSYMTAB_FUNC(__SCT__tp_func_simplefs_alloc_blocks, "");
SYMBOL_FLAGS(__SCT__tp_func_simplefs_alloc_blocks, 0x00);
KSYMTAB_DATA(__tracepoint_simplefs_free_blocks, "");
SYMBOL_FLAGS(__tracepoint_simplefs_free_blocks, 0x00);
KSYMTAB_FUNC(__traceiter_simplefs_free_blocks, "");
SYMBOL_FLAGS(__traceiter_simplefs_free_blocks, 0x00);
KSYMTAB_DATA(__SCK__tp_func_simplefs_free_blocks, "");
SYMBOL_FLAGS(__SCK__tp_func_simplefs_free_blocks, 0x00);
KSYMTAB_FUNC(__SCT__tp_func_simplefs_free_blocks, "");
SYMBOL_FLAGS(__SCT__tp_func_simplefs_free_blocks, 0x00);
KSYMTAB_DATA(__tracepoint_simplefs_create_inode, "");
SYMBOL_FLAGS(__tracepoint_simplefs_create_inode, 0x00);
KSYMTAB_FUNC(__traceiter_simplefs_create_inode, "");
SYMBOL_FLAGS(__traceiter_simplefs_create_inode, 0x00);
KSYMTAB_DATA(__SCK__tp_func_simplefs_create_inode, "");
SYMBOL_FLAGS(__SCK__tp_func_simplefs_create_inode, 0x00);
KSYMTAB_FUNC(__SCT__tp_func_simplefs_create_inode, "");
SYMBOL_FLAGS(__SCT__tp_func_simplefs_create_inode, 0x00);
KSYMTAB_DATA(__tracepoint_simplefs_evict_inode, "");
SYMBOL_FLAGS(__tracepoint_simplefs_evict_inode, 0x00);
KSYMTAB_FUNC(__traceiter_simplefs_evict_inode, "");
SYMBOL_FLAGS(__traceiter_simplefs_evict_inode, 0x00);
KSYMTAB_DATA(__SCK__tp_func_simplefs_evict_inode, "");
SYMBOL_FLAGS(__SCK__tp_func_simplefs_evict_inode, 0x00);
KSYMTAB_FUNC(__SCT__tp_func_simplefs_evict_inode, "");
SYMBOL_FLAGS(__SCT__tp_func_simplefs_evict_inode, 0x00);
KSYMTAB_DATA(__tracepoint_simplefs_lookup, "");
SYMBOL_FLAGS(__tracepoint_simplefs_lookup, 0x00);
KSYMTAB_FUNC(__traceiter_simplefs_lookup, "");
SYMBOL_FLAGS(__traceiter_simplefs_lookup, 0x00);
KSYMTAB_DATA(__SCK__tp_func_simplefs_lookup, "");
SYMBOL_FLAGS(__SCK__tp_func_simplefs_lookup, 0x00);
KSYMTAB_FUNC(__SCT__tp_func_simplefs_lookup, "");
SYMBOL_FLAGS(__SCT__tp_func_simplefs_lookup, 0x00);
KSYMTAB_DATA(__tracepoint_simplefs_error, "");
SYMBOL_FLAGS(__tracepoint_simplefs_error, 0x00);
KSYMTAB_FUNC(__traceiter_simplefs_error, "");
SYMBOL_FLAGS(__traceiter_simplefs_error, 0x00);
KSYMTAB_DATA(__SCK__tp_func_simplefs_error, "");
SYMBOL_FLAGS(__SCK__tp_func_simplefs_error, 0x00);
KSYMTAB_FUNC(__SCT__tp_func_simplefs_error, "");
SYMBOL_FLAGS(__SCT__tp_func_simplefs_error, 0x00);
KSYMTAB_DATA(sfs_stats, "");
SYMBOL_FLAGS(sfs_stats, 0x00);

MODULE_INFO(depends, "");

