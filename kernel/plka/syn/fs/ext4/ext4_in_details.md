## Ext4
https://ext4.wiki.kernel.org/index.php/Main_Page

> 主要扩展
1. Delayed allocation
2. extents
3. multiblock 

## Ext4 Disk Layout
https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout

> 难道真的使用的是bitmap 吗?

> boot sector
> group ? inode table ?

## ext4_map_blocks()
https://ext4.wiki.kernel.org/index.php/Life_of_an_ext4_write_request


```c
// inode.c

/*
 * The ext4_map_blocks() function tries to look up the requested blocks,
 * and returns if the blocks are already mapped.
 *
 * Otherwise it takes the write lock of the i_data_sem and allocate blocks
 * and store the allocated blocks in the result buffer head and mark it
 * mapped.
 *
 * If file type is extents based, it will call ext4_ext_map_blocks(),
 * Otherwise, call with ext4_ind_map_blocks() to handle indirect mapping
 * based files
 *
 * On success, it returns the number of blocks being mapped or allocated.  if
 * create==0 and the blocks are pre-allocated and unwritten, the resulting @map
 * is marked as unwritten. If the create == 1, it will mark @map as mapped.
 *
 * It returns 0 if plain look up failed (blocks have not been allocated), in
 * that case, @map is returned as unmapped but we still do fill map->m_len to
 * indicate the length of a hole starting at map->m_lblk.
 *
 * It returns the error in case of allocation failure.
 */
int ext4_map_blocks(handle_t *handle, struct inode *inode,
		    struct ext4_map_blocks *map, int flags)
```

Description of how a write request happens from userspace (i.e., the codepath from `generic_file_buffered_write()` calling
`ext4_write_begin()` and `ext4_{writeback,ordered,journalled}_write_end()` and/or the codepath from `page_mkwrite()` calling `ext4_page_mkwrite()`).

The allocator used in ext2 and ext3 would scan the free blocks bitmap for every new block written to a file. This was inefficient, and the block allocator in ext4 (mballoc) replaced the bitmap allocator and is one of the reasons ext4 is much faster than ext3.
> 不再使用bitmap allocator

