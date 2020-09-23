##  export symbol

```c
static const struct address_space_operations myfs_aops = {
	/* TODO 6: Fill address space operations structure. */
	.readpage	= simple_readpage,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
	.set_page_dirty	= __set_page_dirty_no_writeback,
};
```

```
WARNING: "__set_page_dirty_no_writeback" [/home/shen/Core/hack-linux-kernel/tools/labs/skels/./filesystems/myfs/myfs.ko] undefined!
```
结果发现其他的文章都是 EXPORT_SYMBOL 的
但是编译器是怎么知道的 ?

## kallsymbols 
