# page-io.c 分析


> 看上去就是底层进行io的
```c
swap_writepage
swap_writepage
generic_swapfile_activate <= setup_swap_extents
swap_set_page_dirty
```


```c
// 曾经以为所有的swap 的 address_space 都是相同(swap_state.c 上定义的，现在怀疑，filebased 可能结果是依赖于具体的文件系统)
// 如何实现将 partion 和 file 的操作区分开的 ? 
// if (sis->flags & SWP_FILE)  通过 swap_info_struct 的 flags 来区分，是文件，那么从 file 的 address_space 中找到对应的read函数。

int swap_set_page_dirty(struct page *page)
{
	struct swap_info_struct *sis = page_swap_info(page);

	if (sis->flags & SWP_FILE) {
		struct address_space *mapping = sis->swap_file->f_mapping;

		VM_BUG_ON_PAGE(!PageSwapCache(page), page);
		return mapping->a_ops->set_page_dirty(page);
	} else {
		return __set_page_dirty_no_writeback(page);
	}
}
```


```c
// page-writeback.c 
/*
 * For address_spaces which do not use buffers nor write back.
 */
int __set_page_dirty_no_writeback(struct page *page)
{
	if (!PageDirty(page))
		return !TestSetPageDirty(page);
	return 0;
}
```
