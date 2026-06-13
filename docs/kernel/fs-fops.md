# VFS standard file operation

## 基本总结
1. fops 是注册到 inode::i_fop 中，毕竟对于文件的 io 是底层属性控制的
  - get_pipe_inode


## 问题
```c
/*
 * Support for read() - Find the page attached to f_mapping and copy out the
 * data. Its *very* similar to do_generic_mapping_read(), we can't use that
 * since it has PAGE_SIZE assumptions.
 */
static ssize_t hugetlbfs_read_iter(struct kiocb *iocb, struct iov_iter *to)
```

2. 很窒息，为什么 disk fs 和 nobev 的 fs 的 address_space_operations 的内容是相同的。
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


## mmap

- 主调调用位置 : mmap_region
  - call_mmap : file_operations::mmap

```c
/* This is used for a general mmap of a disk file */
int generic_file_mmap(struct file * file, struct vm_area_struct * vma)
{
  struct address_space *mapping = file->f_mapping;

  if (!mapping->a_ops->readpage)
    return -ENOEXEC;
  file_accessed(file);
  vma->vm_ops = &generic_file_vm_ops;
  return 0;
}
```

这个注册位置比较多，暂时分析几个有趣的:
- io_uring_mmap : 通过 mmap 这个 fd ，用户态空间获取到 se cq 这些共享队列
- ext4_file_mmap
  - 注册 vm_area_struct::vm_ops
- shmem_mmap : 见 shmem 的分析吧

## write_iter

- io_issue_sqe
  - io_write
    - call_write_iter
      - file_operations::write_iter

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

### write 和 write_iter 的区别

write 几乎没有什么用户，read 面前有点

`__kernel_read`


## iopoll
使用用户: io_uring::io_do_iopoll

```c
const struct file_operations ext4_file_operations = {
   // ...
	.iopoll		= iocb_bio_iopoll,
  // ...
```

- iocb_bio_iopoll
  - bio_poll
    - blk_mq_poll : 如果是 multiqueue
      - 循环调用: request_queue::mq_ops::poll，主要分析 virtblk_poll nvme_tcp_poll nvme_poll
    - gendisk::fops::poll_bio

- nvme_poll
  - nvme_poll_cq
    - while nvme_cqe_pending; do nvme_handle_cqe; nvme_update_cq_head; done


## poll

- vfs_poll 来调用这个 hook，主要给 select 和 aio 使用


- io_uring_poll
  - poll_wait


一种触发的路径:
- io_cqring_ev_posted_iopoll
  - io_commit_cqring_flush
    - `__io_commit_cqring_flush`
      - io_poll_wq_wake


## show_fdinfo

```txt
[root@nixos:/proc/118496/fdinfo]# cat 4
pos:    0
flags:  02000002
mnt_id: 14
ino:    240059
SqMask: 0x7f
SqHead: 3777186
SqTail: 3777186
CachedSqHead:   3777186
CqMask: 0x7f
CqHead: 3777060
CqTail: 3777091
CachedCqTail:   3777091
SQEs:   0
CQEs:   31
   36: user_data:0, res:4096, flag:0
   37: user_data:0, res:4096, flag:0
   38: user_data:0, res:4096, flag:0
   39: user_data:0, res:4096, flag:0
   40: user_data:0, res:4096, flag:0
   41: user_data:0, res:4096, flag:0
   42: user_data:0, res:4096, flag:0
   43: user_data:0, res:4096, flag:0
   44: user_data:0, res:4096, flag:0
   45: user_data:0, res:4096, flag:0
   46: user_data:0, res:4096, flag:0
   47: user_data:0, res:4096, flag:0
   48: user_data:0, res:4096, flag:0
   49: user_data:0, res:4096, flag:0
   50: user_data:0, res:4096, flag:0
   51: user_data:0, res:4096, flag:0
   52: user_data:0, res:4096, flag:0
   53: user_data:0, res:4096, flag:0
   54: user_data:0, res:4096, flag:0
   55: user_data:0, res:4096, flag:0
   56: user_data:0, res:4096, flag:0
   57: user_data:0, res:4096, flag:0
   58: user_data:0, res:4096, flag:0
   59: user_data:0, res:4096, flag:0
   60: user_data:0, res:4096, flag:0
   61: user_data:0, res:4096, flag:0
   62: user_data:0, res:4096, flag:0
   63: user_data:0, res:4096, flag:0
   64: user_data:0, res:4096, flag:0
   65: user_data:0, res:4096, flag:0
   66: user_data:0, res:4096, flag:0
SqThread:       -1
SqThreadCpu:    -1
UserFiles:      1
UserBufs:       128
PollList:
CqOverflowList:
```
