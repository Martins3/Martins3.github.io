## VFS standard file operation
1.
```c
/*
 * Support for read() - Find the page attached to f_mapping and copy out the
 * data. Its *very* similar to do_generic_mapping_read(), we can't use that
 * since it has PAGE_SIZE assumptions.
 */
static ssize_t hugetlbfs_read_iter(struct kiocb *iocb, struct iov_iter *to)
```

2. 很窒息，为什么 disk fs 和 nobev 的fs 的 address_space_operations 的内容是相同的。
    1. simple_readpage : 将对应的 page 的内容清空
    2. simple_write_begin : 将需要加载的页面排列好
    3. 所以说不过去
```c
static const struct address_space_operations myfs_aops = {
  /* TODO 6: Fill address space operations structure. */
  .readpage = simple_readpage,
  .write_begin  = simple_write_begin,
  .write_end  = simple_write_end,
};

static const struct address_space_operations minfs_aops = {
  .readpage = simple_readpage,
  .write_begin = simple_write_begin,
  .write_end = simple_write_end,
};
```


#### inode_operations::fiemap
// TODO
// 啥功能呀 ?

```c
const struct inode_operations ext2_file_inode_operations = {
#ifdef CONFIG_EXT2_FS_XATTR
  .listxattr  = ext2_listxattr,
#endif
  .getattr  = ext2_getattr,
  .setattr  = ext2_setattr,
  .get_acl  = ext2_get_acl,
  .set_acl  = ext2_set_acl,
  .fiemap   = ext2_fiemap,
};

/**
 * generic_block_fiemap - FIEMAP for block based inodes
 * @inode: The inode to map
 * @fieinfo: The mapping information
 * @start: The initial block to map
 * @len: The length of the extect to attempt to map
 * @get_block: The block mapping function for the fs
 *
 * Calls __generic_block_fiemap to map the inode, after taking
 * the inode's mutex lock.
 */

int generic_block_fiemap(struct inode *inode,
       struct fiemap_extent_info *fieinfo, u64 start,
       u64 len, get_block_t *get_block)
{
  int ret;
  inode_lock(inode);
  ret = __generic_block_fiemap(inode, fieinfo, start, len, get_block);
  inode_unlock(inode);
  return ret;
}
```

#### file_operations::mmap

唯一的调用位置: mmap_region
```c
static inline int call_mmap(struct file *file, struct vm_area_struct *vma) {
  return file->f_op->mmap(file, vma);
}

/* This is used for a general mmap of a disk file */
int generic_file_mmap(struct file * file, struct vm_area_struct * vma)
{
  struct address_space *mapping = file->f_mapping;

  if (!mapping->a_ops->readpage)
    return -ENOEXEC;
  file_accessed(file);
  vma->vm_ops = &generic_file_vm_ops; // TODO 追踪一下
  return 0;
}
```
#### file_operations::write_iter
- [x] trace function from `io_uring_enter` to `file_operations::write_iter`

io_issue_sqe ==>
io_write ==> call_write_iter ==> file_operations::write_iter

```c
static int io_write(struct io_kiocb *req, bool force_nonblock,
        struct io_comp_state *cs)
{
  // ...
  if (req->file->f_op->write_iter)
    ret2 = call_write_iter(req->file, kiocb, iter);
  else if (req->file->f_op->write)
    ret2 = loop_rw_iter(WRITE, req->file, kiocb, iter);
  // ...
```

```c
static inline ssize_t call_write_iter(struct file *file, struct kiocb *kio,
              struct iov_iter *iter)
{
  return file->f_op->write_iter(kio, iter);
}
```

There are many similar calling chain in read_write.c which summaries io models except aio and io_uring


#### file_operations::iopoll
example user: io_uring::io_do_iopoll

类似的行为:
```c
struct block_device_operations {
	int (*poll_bio)(struct bio *bio, struct io_comp_batch *iob,
			unsigned int flags);
  // ...
}
```
这个也值得仔细调查。
