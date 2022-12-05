# linux 文件系统

<!-- vim-markdown-toc GitLab -->

* [打通fs的方法](#打通fs的方法)
    * [TODO](#todo)
* [VFS](#vfs)
    * [lookup](#lookup)
    * [path](#path)
* [path walking](#path-walking)
* [VFS standard file operation](#vfs-standard-file-operation)
    * [inode_operations::fiemap](#inode_operationsfiemap)
    * [file_operations::mmap](#file_operationsmmap)
    * [file_operations::write_iter](#file_operationswrite_iter)
    * [file_operations::iopoll](#file_operationsiopoll)
* [kiocb](#kiocb)
* [file io](#file-io)
    * [aio(dated)](#aiodated)
* [file writeback](#file-writeback)
* [fd](#fd)
* [fcntl](#fcntl)
* [eventfd](#eventfd)
* [epoll](#epoll)
* [anon_inodes](#anon_inodes)
* [flock](#flock)
* [ext2](#ext2)
* [nobdv fs](#nobdv-fs)
* [virtual fs library summary](#virtual-fs-library-summary)
    * [libfs](#libfs)
* [virtual fs for debug](#virtual-fs-for-debug)
* [file descriptor](#file-descriptor)
* [virtual fs interface](#virtual-fs-interface)
* [mount](#mount)
    * [pivot_root](#pivot_root)
* [superblock](#superblock)
    * [super operations](#super-operations)
* [dentry](#dentry)
    * [d_path](#d_path)
* [helper](#helper)
* [attr](#attr)
* [open.c(make this part more clear, split it)](#opencmake-this-part-more-clear-split-it)
* [open](#open)
* [iomap](#iomap)
* [block layer](#block-layer)
* [initfs](#initfs)
* [overlay fs](#overlay-fs)
* [inode](#inode)
* [dcache](#dcache)
    * [dcache shrink](#dcache-shrink)
* [splice](#splice)
* [pipe](#pipe)
* [devpts](#devpts)
* [dup(merge)](#dupmerge)
* [IO buffer(merge)](#io-buffermerge)
* [notify](#notify)
* [attr](#attr-1)
* [dax](#dax)
* [block device](#block-device)
* [char device](#char-device)
* [tmpfs](#tmpfs)
* [exciting](#exciting)
* [benchmark](#benchmark)
    * [fio](#fio)
    * [filebench](#filebench)
* [nvme](#nvme)
* [fallocate](#fallocate)
* [union fs](#union-fs)
* [TODO](#todo-1)
* [multiqueue](#multiqueue)
* [null blk](#null-blk)
* [proc](#proc)
    * [sysctl](#sysctl)
* [mnt](#mnt)
* [fifo](#fifo)
* [configfs](#configfs)
* [binder](#binder)
* [zone](#zone)
* [nfs](#nfs)
* [compression fs](#compression-fs)
* [timerfd](#timerfd)
* [ceph](#ceph)
* [close](#close)
* [[ ] hugetlbfs](#-hugetlbfs)
* [cache](#cache)
* [用户态](#用户态)
* [https://www.zhihu.com/question/21536660](#httpswwwzhihucomquestion21536660)
* [ext4_fiemap && do_vfs_ioctl 是做啥的](#ext4_fiemap-do_vfs_ioctl-是做啥的)
* [erofs](#erofs)
* [引用计数](#引用计数)

<!-- vim-markdown-toc -->



用户层的角度:
1. fs 可以用于访问文件: ext2, ext3
2. fs 用于导出信息: (audit 为什么使用网络)
    1. 系统的状态 : proc
    2. 设备状态 : sysfs
    3. 调试 : debugfs
3. 设备可以通过文件系统进行访问 : fs/char_dev.c fs/block_dev.c

内核的角度：
1. 路径 hardlink softlink 创建文件 等

还是认为，只要将 linux kernel lab 中间搞定了，那么就是掌握了核心内容，其余的问题在逐个分析就不难了:

1. 似乎从来遇到过的 dentry 的 operation 的操作是做什么的 ? 还是不知道，甚至不知道 ext2 是否注册过

感觉需要快速浏览一下书才可以心中有个大致的概念。


## 打通fs的方法
1. journal 的实现了解一下
2. lfs 的实现 (了解)
3. 文件系统的启动过程
  1. 想一下，/dev/ mount 时间是在 initramfs，也就是在根文件系统被 mount 之后，但是根文件系统的 mount 似乎需要 /dev/something
  2. mount 的接口是什么 ? 可以 mount 的是 路径 fd inode 设备还是 ?
  3. 根文件系统的 mount 位置是靠什么指定的 ? 内核参数 root=/ 在 initramfs 中间，这是什么探测的
  4. mount 参数 ： 设备，路径。根文件系统的设备，内核参数或者 initramfs，其他的各种设备，探测，然后放到 /dev/sda 之内的位置
    1. 所以，/dev/sda 被探测是确定的
4. 文件系统启动除了 mount 还有什么 ? inode cache, dcache 以及其他的数据结构需要初始化吗 ?
5. uring io
6. io scheduler
7. 磁盘驱动，不要基于内存的虚假驱动
8. fuse
9. nfs

#### TODO
- [ ] 内部的 lock 的设计
- [ ] 了解一下 squashfs 的设计

## VFS
基本元素:

superblock : 持有整个文件系统的基本信息。 superblock operation 描述创建 fs specific 的 inode

inode : 描述文件的静态属性，比如大小，创建的时间等等。

file : 描述文件的动态属性，也就是打开一个文件之后，读写指针的位置在何处

dir entry : 目录也是一个文件，这是目录保存的基本内容。(那么目录文件中间会保存 . 和 .. 吗 ? 应该不会吧)

VFS 提供的功能:
1. 提供一些统一的解决方案 : file lock，路径解析
2. 提供一些 generic 的实现 :
3. 一些需要具体文件系统实现的接口 :

file_operations::mkdir 作为 file 的inode 和 dir 有区别吗 ?

| x    | inode_operations                                | file_operations     |
|------|-------------------------------------------------|---------------------|
| dir  | inode operation 应该是主要提供给 dir 的操作的， | 主要是 readdir 操作 |
| file | attr acl 相关                                   | 各种IO              |

所以，不相关的功能被放到同一个 operation 结构体中间了。


#### lookup

1. 流程
2. 如何处理锁机制
3. namei.c 的作用是什么: 各种文件名的处理吧，尤其处理 hardlink, symbol link 之类的操作
3. 基于 pwd 的是怎么回事


4. follow_dotdot



* ***核心数据结构***
1. 都是在什么时候装配的 ?
2.

```c
struct nameidata {
  struct path path;
  struct qstr last;
  struct path root;
  struct inode  *inode; /* path.dentry.d_inode */
  unsigned int  flags;
  unsigned  seq, m_seq, r_seq;
  int   last_type;
  unsigned  depth;
  int   total_link_count;
  struct saved {
    struct path link;
    struct delayed_call done;
    const char *name;
    unsigned seq;
  } *stack, internal[EMBEDDED_LEVELS];
  struct filename *name;
  struct nameidata *saved;
  unsigned  root_seq;
  int   dfd;
  kuid_t    dir_uid;
  umode_t   dir_mode;
} __randomize_layout;
```

* ***流程***
walk_component

```c
static const char *walk_component(struct nameidata *nd, int flags) // 调用 lookup_fast 和 lookup_slow
```

lookup_fast : 调用 `__d_lookup`
`__lookup_slow` :
1. d_alloc_parallel : ?
2. `old = inode->i_op->lookup(inode, dentry, flags);` : vfs 提供 looup 接口

使用ext2作为例子:
```c
static struct dentry *ext2_lookup(struct inode * dir, struct dentry *dentry, unsigned int flags)
{
  struct inode * inode;
  ino_t ino;

  if (dentry->d_name.len > EXT2_NAME_LEN)
    return ERR_PTR(-ENAMETOOLONG);

  ino = ext2_inode_by_name(dir, &dentry->d_name);
  inode = NULL;
  if (ino) {
    inode = ext2_iget(dir->i_sb, ino);
    if (inode == ERR_PTR(-ESTALE)) {
      ext2_error(dir->i_sb, __func__,
          "deleted inode referenced: %lu",
          (unsigned long) ino);
      return ERR_PTR(-EIO);
    }
  }
  return d_splice_alias(inode, dentry);
}
```

1. 第一个:
```c
ino_t ext2_inode_by_name(struct inode *dir, const struct qstr *child)
{
  ino_t res = 0;
  struct ext2_dir_entry_2 *de;
  struct page *page;

  de = ext2_find_entry (dir, child, &page); // 将 direcory 的内容加载到 page cache 中间，其实 dentry 和 这个没有什么关系，此处加载的 page 其实就是当做普通文件内容即可
  if (de) {
    res = le32_to_cpu(de->inode);
    ext2_put_page(page);
  }
  return res;
}
```

2. 第二个: d_splice_alias
  1. 是不是 inode 和 dentry 不是一一对应的，是由于硬链接的原因吗 ? TODO
  2. 如果不考虑 alias 问题，等价于 `__d_add`


#### path
1. kern_path 的实现原理，感觉反反复复一直看到
2. path 的定义，其实还是不懂，为什么路径需要 mount 和 dentry 两个项

```c
struct path {
  struct vfsmount *mnt;
  struct dentry *dentry;
} __randomize_layout;
```

```c
int kern_path(const char *name, unsigned int flags, struct path *path)
{
  return filename_lookup(AT_FDCWD, getname_kernel(name),
             flags, path, NULL);
}

filename_lookup : 装配 nameidata，call path_lookupat
path_lookupat : 就是查询的核心位置了
```

## path walking



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

```c
static int blkdev_iopoll(struct kiocb *kiocb, bool wait)
{
  struct block_device *bdev = I_BDEV(kiocb->ki_filp->f_mapping->host);
  struct request_queue *q = bdev_get_queue(bdev);

  return blk_poll(q, READ_ONCE(kiocb->ki_cookie), wait);
}
```



## kiocb

```c
struct kiocb {
  struct file   *ki_filp;

  /* The 'ki_filp' pointer is shared in a union for aio */
  randomized_struct_fields_start

  loff_t      ki_pos;
  void (*ki_complete)(struct kiocb *iocb, long ret, long ret2);
  void      *private;
  int     ki_flags;
  u16     ki_hint;
  u16     ki_ioprio; /* See linux/ioprio.h */
  union {
    unsigned int    ki_cookie; /* for ->iopoll */
    struct wait_page_queue  *ki_waitq; /* for async buffered IO */
  };

  randomized_struct_fields_end
};
```

## file io
epoll
poll
select

aio 和 uring :

> file_operations : 每一个项目的内容都应该清楚吧 !

特别关注的内容 : 才发现几乎没有一个看的懂
2. owner 的作用是什么，什么时候注册的 ?
3. iopoll 的 epoll 有没有关系 ?
4. iterate 和 iterate_shared 是用于遍历什么的 ?
5. fsycn 和 fasync 的区别
6. flock
7. splice_write 和 splice_read 和 pipe 的关系是什么 ?
8. setfl 是做什么的 ?

```c
struct file_operations {
  struct module *owner;
  loff_t (*llseek) (struct file *, loff_t, int);
  ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
  ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
  ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
  ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
  int (*iopoll)(struct kiocb *kiocb, bool spin);
  int (*iterate) (struct file *, struct dir_context *);
  int (*iterate_shared) (struct file *, struct dir_context *);
  __poll_t (*poll) (struct file *, struct poll_table_struct *);
  long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
  long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
  int (*mmap) (struct file *, struct vm_area_struct *);
  unsigned long mmap_supported_flags;
  int (*open) (struct inode *, struct file *);
  int (*flush) (struct file *, fl_owner_t id);
  int (*release) (struct inode *, struct file *);
  int (*fsync) (struct file *, loff_t, loff_t, int datasync);
  int (*fasync) (int, struct file *, int);
  int (*lock) (struct file *, int, struct file_lock *);
  ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
  unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
  int (*check_flags)(int);
  int (*setfl)(struct file *, unsigned long);
  int (*flock) (struct file *, int, struct file_lock *);
  ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
  ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
  int (*setlease)(struct file *, long, struct file_lock **, void **);
  long (*fallocate)(struct file *file, int mode, loff_t offset,
        loff_t len);
  void (*show_fdinfo)(struct seq_file *m, struct file *f);
#ifndef CONFIG_MMU
  unsigned (*mmap_capabilities)(struct file *);
#endif
  ssize_t (*copy_file_range)(struct file *, loff_t, struct file *,
      loff_t, size_t, unsigned int);
  loff_t (*remap_file_range)(struct file *file_in, loff_t pos_in,
           struct file *file_out, loff_t pos_out,
           loff_t len, unsigned int remap_flags);
  int (*fadvise)(struct file *, loff_t, loff_t, int);
} __randomize_layout;
```

2.

#### aio(dated)
1. 但是似乎是可以替代 read 和 write 的功能，比如 ext2 中间就没有为 ext2_file_operation 注册 read 和 write
2. read_iter 和 write_iter 是用来实现 异步IO 的。 为什么可以实现 aio ? aio 在用户层的体现是什么 ?

aio 在工程上的具体使用 libaio 的库， https://oxnz.github.io/2016/10/13/linux-aio/


第一个问题: 如何实现 write(2)
在 fs/read_write.c 中间描述
```c
SYSCALL_DEFINE3(write, unsigned int, fd, const char __user *, buf, size_t, count) { return ksys_write(fd, buf, count); }
```
=> ksys_write => vfs_write

vfs_write
1. file_start_write 和 file_end_write 处理锁相关的(应该吧)
2. `__vfs_write` : 首先尝试使用file_operation::write，然后尝试使用 new_sync_write
3. new_sync_write : 首先初始化 aio 相关的结构体，然后调用 write_iter

```c
static ssize_t new_sync_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
  struct iovec iov = { .iov_base = (void __user *)buf, .iov_len = len };
  struct kiocb kiocb;
  struct iov_iter iter;
  ssize_t ret;

  init_sync_kiocb(&kiocb, filp);
  kiocb.ki_pos = (ppos ? *ppos : 0);
  iov_iter_init(&iter, WRITE, &iov, 1, len);

  ret = call_write_iter(filp, &kiocb, &iter);
  BUG_ON(ret == -EIOCBQUEUED);
  if (ret > 0 && ppos)
    *ppos = kiocb.ki_pos;
  return ret;
}
```

第二个问题: aio 如何使用 [^2]
The Asynchronous Input/Output (AIO) interface allows many I/O requests to be submitted in parallel without the overhead of a thread per request.
The purpose of this document is to explain how to use the Linux AIO interface,
namely the function family `io_setup`, `io_submit`, `io_getevents`, `io_destroy`. Currently, the AIO interface is best for O_DIRECT access to a raw block device like a disk, flash drive or storage array.
> 这些系统调用都是在 aio.c 中间

```c
int io_setup(int maxevents, io_context_t *ctxp);
int io_destroy(io_context_t ctx);
```
io_context_t 是指向 io_context 的指针类型，其中 maxevents 是参数，ctxp 是返回值。

io_context 是公共资源:
An `io_context_t` object can be shared between threads, both for submission and completion.
No guarantees are provided about ordering of submission and completion with respect to interaction from multiple threads.
There may be performance implications from sharing `io_context_t` objects between threads.

```c
struct kiocb {
  struct file   *ki_filp;

  loff_t      ki_pos;
  void (*ki_complete)(struct kiocb *iocb, long ret, long ret2);
  void      *private;
  int     ki_flags;
  u16     ki_hint;
  u16     ki_ioprio; /* See linux/ioprio.h */
  unsigned int    ki_cookie; /* for ->iopoll */
};

struct iocb {
  PADDEDptr(void *data, __pad1);  /* Return in the io completion event */
  /* key: For use in identifying io requests */
  /* aio_rw_flags: RWF_* flags (such as RWF_NOWAIT) */
  PADDED(unsigned key, aio_rw_flags);

  short   aio_lio_opcode;
  short   aio_reqprio;
  int   aio_fildes;

  union {
    struct io_iocb_common   c;
    struct io_iocb_vector   v;
    struct io_iocb_poll   poll;
    struct io_iocb_sockaddr saddr;
  } u;
};

struct io_iocb_common {
  PADDEDptr(void  *buf, __pad1);
  PADDEDul(nbytes, __pad2);
  long long offset;
  long long __pad3;
  unsigned  flags;
  unsigned  resfd;
};  /* result code is the amount read or -'ve errno */
```
The meaning of the fields is as follows: data is a pointer to a user-defined object used to represent the operation

- `aio_lio_opcode` is a flag indicate whether the operation is a read (IO_CMD_PREAD) or a write (IO_CMD_PWRITE) or one of the other supported operations
- `aio_fildes` is the `fd` of the file that the iocb reads or writes
- `buf` is the pointer to memory that is read or written
- `nbytes` is the length of the request
- `offset` is the initial offset of the read or write within the file

> 为什么 kiocb 中间没有 nbytes 之类内容

`io_prep_pread` and `io_prep_pwrite` 初始化 iocb 之后，然后使用 io_submit 提交，使用 `io_getevents` 获取。

作者还分析了 io_getevents 的参数不同取值的含义。

第三个问题: aio 内核如何实现的
1. 分析 syscall io_submit ，可以非常容易的跟踪到 aio_write，并且在其中调用 call_write_iter
2. 如何实现系统调用 : io_getevents

io_getevents => read_events => aio_read_events => aio_read_events_ring

aio_read_events_ring 会访问 kioctx::ring_pages 来提供给用户就绪的 io，使用 aio_complete 向其中添加。
aio_complete 会调用 eventfd_signal，这是实现 epoll 机制的核心。

> 1. io_submit 提交请求之后，然后返回到用户空间，用于完成IO的内核线程是如何管理的 ?　暂时没有找到，io_submit 开始返回的位置。
> 2. aio 不能使用 page cache, 那么对于 metadata 的读取， aio 可以实现异步吗 ? (我猜测，应该所有的文件系统都是不支持的吧!)

## file writeback
fs/file-writeback.c 中间到底完成什么工作
// 具体内容有点迷惑，但是file-writeback.c 绝对不是 page-writeback.c 更加底层的东西
// 其利用 flusher thread ，然后调用 do_writepages 实现将整个文件，甚至整个文件系统写回，

- [ ] so file-writeback and page-writeback.c are one of two base modules of vmscan, another base moduler is swap.

## fd
Related code resides in fs/file.c, it works just what we expected.

- fd_install
- dup
- fget

## fcntl
abbreviation for file control

1. lease ?
2. dnotify
3. seal

来源自 man fcntl(2)

主要的功能:
1. Duplicating a file descriptor
2. File descriptor flags

The principal difference between the two lock types is that whereas traditional record locks are associated with a process,
open file description locks are associated with the open file description on which they are acquired, much like locks acquired with flock(2).
Consequently (and unlike traditional advisory record locks), open file description locks are inherited across fork(2) (and clone(2) with CLONE_FILES),
and are only automatically released on the last close of the open file description, instead of being released on any close of the file.

> Warning: the Linux implementation of mandatory locking is unreliable.  See BUGS below.  Because of these bugs, and the fact that the feature is believed to be little used, since Linux 4.5, mandatory locking has been made an optional feature, governed by a configuration option (CONFIG_MANDATORY_FILE_LOCKING).  This is an initial step toward removing this feature completely.

## eventfd
> - https://stackoverflow.com/questions/13607730/writing-to-eventfd-from-kernel-module : 在内核模块中间可以直接让等待的等待 eventfd 的 select 返回
> - https://unixism.net/loti/tutorial/register_eventfd.html : io_uring 可以注册 eventfd ，从而每次 io_uring 的操作完成之后，eventfd 都可以收到消息，而另一个 thread 调用 eventfd_read 的线程可以进入到下一步

- 所以，kvm 是如何使用 eventfd 的 ?
http://blog.allenx.org/2015/07/05/kvm-irqfd-and-ioeventfd

## epoll
- [ ] https://zhou-yuxin.github.io/articles/2017/%E7%AC%AC%E4%B8%80%E4%B8%AALinux%E9%A9%B1%E5%8A%A8%E7%A8%8B%E5%BA%8F%EF%BC%88%E4%B8%89%EF%BC%89%E2%80%94%E2%80%94aMsg%E7%9A%84%E9%9D%9E%E9%98%BB%E5%A1%9E%E5%BC%8FIO%E4%B9%8Bselect-poll/index.html
在内核中间实现一个支持的 poll 的模块

fs/eventpoll.md 和 fs/eventfd.md

epoll_create
```c
       int epoll_create(int size); // 过时了，内核不需要动态分配，不需要 size 变量
       int epoll_create1(int flags);

```
```c
/*
 * Open an eventpoll file descriptor.
 */
static int do_epoll_create(int flags)

   * Creates all the items needed to setup an eventpoll file. That is,
   * a file structure and a free file descriptor.


/* File callbacks that implement the eventpoll file behaviour */
static const struct file_operations eventpoll_fops = {
#ifdef CONFIG_PROC_FS
  .show_fdinfo  = ep_show_fdinfo,
#endif
  .release  = ep_eventpoll_release,
  .poll   = ep_eventpoll_poll,
  .llseek   = noop_llseek,
};
```

- do_epoll_create
  - get_unused_fd_flags
  - anon_inode_getfile
  - fd_install

- do_epoll_ctl
  - ep_insert
    - ep_rbtree_insert : 插入到 eventpoll:rbr 这个结构体上
    - init_poll_funcptr(&epq.pt, ep_ptable_queue_proc);
  - ep_remove :
  - ep_modify


- do_epoll_wait
  - ep_poll
    - ep_events_available
      - `ep->rdllist`

> Because different file systems have different implementations, it is impossible to get the waiting queue directly through the struct file object, so we use the poll operation of struct file to return the waiting queue of the object in the way of callback.
The callback function set here is `ep_ptable_queue_proc`

1. 感觉 ep_ptable_queue_proc 是用于加入队列的时候初始化
2. ep_item_poll
  - `__ep_eventpoll_poll`
    - poll_wait
      - poll_table_struct:poll_queue_proc : 调用 callback 也就是 ep_ptable_queue_proc
3. ep_poll_callback : 是 wakeup 的时候，调用的 callback 函数


> 总结，从描述上看，似乎并没有什么神奇的不得了的事情，只是 epoll 和 eventfd , aio , io_uring 在异步机制上的区别是什么

还需要阅读 poll, ppoll, select, pselect 的代码吗 ? 没必要, 可以看看 [Evans 的 blog](https://jvns.ca/blog/2017/06/03/async-io-on-linux--select--poll--and-epoll/)

## anon_inodes
- [ ] 很短的一个代码 : /home/maritns3/core/linux/fs/anon_inodes.c
    - [ ]  kmv 用于创建 kvm-vm 的方法

## flock
首先，区分一件事情 :
https://www.kernel.org/doc/html/latest/filesystems/locking.html : 说明几乎VFS api 调用的时候需要持有的锁

主要内容在 : fs/locks.c

下面仅仅阅读一下 fcntl:
- `__do_sys_fcntl`
  - do_fcntl
    - fcntl_setlk
      - mandatory_lock : 对于 fs 和 inode 进行检查, 看是不是 mandatory lock，和 tlpi 上描述完全一致
      - do_lock_file_wait
        - vfs_lock_file
          - filp->f_op->lock(filp, cmd, fl); : 一些 nfs 使用自定义的
          - posix_lock_file
            - posix_lock_inode : fcntl 上锁是对于 inode 实施的, 在这里对于 file_lock_context 的链表进行遍历，决定是否上锁了之类的
              -

```c
struct file_lock_context {
  spinlock_t    flc_lock;
  struct list_head  flc_flock;
  struct list_head  flc_posix;
  struct list_head  flc_lease;
};
```

还有关于 lease 相关的实现，close file 和 exec 对于锁的释放，flock 的实现，都在 fs/locks.c 中间占据了不少篇幅。

## ext2
内部结构:
https://www.nongnu.org/ext2-doc/ext2.html#s-creator-os

## nobdv fs
tmp proc sysfs

ramfs 和 ext2 fs 应该可以作为两个典型

> 怀疑这个分类有点问题，正确的分类应该参考几个mount 函数


sysfs : device model 的形象显示，更加重要的是，让驱动有一个和用户沟通的快捷方式。
使用 ioctl 似乎不能解决问题：不是所有的驱动都是有 /dev/ 下存在设备的啊 !
这种根本没有在文件系统中间出现的东西，应该不可以使用 ioctl 吧 !

## virtual fs library summary
kernfs seq libfs

kernfs 也是利用 seq 中间的内容，那么 libfs 和 kernfs 的区别在于什么地方呀 ?

#### libfs
通过 ramfs 理解 libfs 吧 !


## virtual fs for debug
1. kprobes
2. debugfs
3. tracefs

wowotech 有一个嵌入式工程师都应该知道的 debug

## file descriptor
为什么需要 file descriptor，而不是直接返回 inode ?
首先，为什么需要 file struct，因为 file struct 描述了读写的过程，
多以，可以存在多个 file struct 对应同一个 inode。
那么，当一个进程需要打开一个文件，最简单显然是返回一个数值，让用户来找到该文件。


在同一个进程中间不同 fd 指向相同的 file struct : 为什么存在这种需求 ?
This situation may arise as a result of a call to dup(), dup2(), or fcntl() (see Section 5.5).

## virtual fs interface
1. fd
5. dentry :
    1. 实在是想不懂为什么会出现 dops 这种东西
```c
// 其中的想法是什么 :
struct dentry_operations {
        int (*d_revalidate)(struct dentry *, unsigned int);
        int (*d_weak_revalidate)(struct dentry *, unsigned int);
        int (*d_hash)(const struct dentry *, struct qstr *);
        int (*d_compare)(const struct dentry *,
                         unsigned int, const char *, const struct qstr *);
        int (*d_delete)(const struct dentry *);
        int (*d_init)(struct dentry *);
        void (*d_release)(struct dentry *);
        void (*d_iput)(struct dentry *, struct inode *);
        char *(*d_dname)(struct dentry *, char *, int);
        struct vfsmount *(*d_automount)(struct path *);
        int (*d_manage)(const struct path *, bool);
        struct dentry *(*d_real)(struct dentry *, const struct inode *);
};
```
1. 存在 system wide 的 inode table 吗 ?
    2. 如果不同的文件系统 inode 被放在一起吗 ?
3. 什么时候不同的 file struct 指向同一个 inode 上 ?
    1. This occurs because each process independently called open() for the same file. A similar situation could occur if a single process opened the same file twice.
    2. 是不是只要打开一次文件就会出现一次 file struct ?
4. 所以标准输入，标准输出的 fd 为 0 1 2 设置于支持都是如何形成的 ?


[^1] 提供了所有的API的解释说明，主要是几个 operation，需要解释一下:

```c
int generic_file_mmap(struct file * file, struct vm_area_struct * vma)
{
  struct address_space *mapping = file->f_mapping;

  if (!mapping->a_ops->readpage)
    return -ENOEXEC;
  file_accessed(file);
  vma->vm_ops = &generic_file_vm_ops;
  return 0;
}

struct vm_operations_struct generic_file_vm_ops = {
  .fault    = filemap_fault, // 标准 page fault 函数
  .map_pages  = filemap_map_pages, // TODO 从 do_fault_around 调用 filemap_map_pages https://lwn.net/Articles/588802/ 但是具体实现有点不懂，什么叫做 easy access page
  .page_mkwrite = filemap_page_mkwrite, // 用于通知当 page 从 read 变成可以 write 的过程
};
```

ext2 的内容:
```
#define ext2_file_mmap  generic_file_mmap
```

ext4 的内容:
```c
static const struct vm_operations_struct ext4_file_vm_ops = {
  .fault    = ext4_filemap_fault,
  .map_pages  = filemap_map_pages,
  .page_mkwrite   = ext4_page_mkwrite,
};

static int ext4_file_mmap(struct file *file, struct vm_area_struct *vma)
{
  struct inode *inode = file->f_mapping->host;
  struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
  struct dax_device *dax_dev = sbi->s_daxdev;

  if (unlikely(ext4_forced_shutdown(sbi)))
    return -EIO;

  /*
   * We don't support synchronous mappings for non-DAX files and
   * for DAX files if underneath dax_device is not synchronous.
   */
  if (!daxdev_mapping_supported(vma, dax_dev))
    return -EOPNOTSUPP;

  file_accessed(file);
  if (IS_DAX(file_inode(file))) {
    vma->vm_ops = &ext4_dax_vm_ops;
    vma->vm_flags |= VM_HUGEPAGE;
  } else {
    vma->vm_ops = &ext4_file_vm_ops;
  }
  return 0;
}

vm_fault_t ext4_filemap_fault(struct vm_fault *vmf)
{
  struct inode *inode = file_inode(vmf->vma->vm_file);
  vm_fault_t ret;

  down_read(&EXT4_I(inode)->i_mmap_sem);
  ret = filemap_fault(vmf);
  up_read(&EXT4_I(inode)->i_mmap_sem);

  return ret;
}
```
由此看来，file_operations::mmap 的作用:
1. file_accessed
2. 注册 vm_ops

## mount

- [ ] https://www.kernel.org/doc/html/latest/filesystems/mount_api.html read the documenation

问题:
1. mount 在路径查询到时候的作用是什么 ?
2. master 和 slave 在这里是个什么概念
3. kern_mount 的时候 : 那些根本没有路径的，应该不调用 graft_tree 吧
4. 根文件系统的 mount 是怎么回事

6. 忽然想到，在本来的文件系统上，一个文件夹，比如 /home/shen/core 下面本来就是存在文件的，
在这种情况下，该文件系统被 mount ， 描述一下这种情况

TODO
1. https://zhuanlan.zhihu.com/p/93592262 新版的 mount 看这里就可以了吧，现在大致理解了 mount 的作用，先看下一个吧!


| function    | explaination                                                                                                                                                                                             |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| lock_mount  | 后挂载的文件系统会在挂载到前一次挂载的文件系统的根dentry上。lock_mount这个函数的一部分逻辑就保证了在多文件系统挂载同路径的时候，让每个新挂载的文件系统都顺序的挂载（覆盖）上一次挂载实例的根dentry。[^5] |
| lokup_mount | 通过 parent fs 的 vfsmount 和 dentry 找到在该位置上的 mount 实例，lock_mount 需要调用从而找到多次在同一个位置 mount 的最终 mount 点                                                                      |



两种特殊的mount 情况:
1. 同一个设备可以 mount 到不同的位置
    1. 可以理解为多个进入到该文件系统可以多个入口
2. 多个设备可以 mount 到同一个目录中间
    1. 后面的隐藏前面的
    2. 多次 mount 需要多次 unmount 对应


loop device: The loop device driver transforms operations on the associated block device into file(system)operations, that's how the data/partitions end up in a file. [^3]
> 1. 问题是，loop device 的实现方法在哪里，不会是 deriver/block/loop 吧
> 2. dev/loop 的文件是做什么的 ?  `mkfs.xfs -f /dev/loop0` 的作用是啥 ?

理解一下mount系统调用:
实际上flags + data对应mount命令的所有-o选项。那怎么区分哪些属于flags哪些属于data呢？
在深入内核代码一探究竟之前（本篇不深入了）我们可以通俗的认为flags就是所有文件系统通用的挂载选项，由VFS层解析。data是每个文件系统特有的挂载选项，由文件系统自己解析。[^4]
> 其中还总结了，/mnt/mi/linux/tools/include/uapi/linux/mount.h MS 之类的宏和 mount -o 的对应关系


file_system_type + super block + mount的关系 : file_system_type 一个文件系统中间只有一个，每一个 partitions 被 mount 之后，都可以创建一个 superblock，当其中的
1. sda1同时挂在了/mnt/a和/mnt/x上，所以它有两个挂载实例对应同一个super_block. : 当一个设备被mount的了，就会存在一个 superblock，但是之后无论 mount 多少次，都是只有一个
2. 每mount一次就会存在一个 mount 实例



相关的数据结构:
```c
// 这个结构体强调的是自己的信息
struct vfsmount {
  struct dentry *mnt_root;  /* root of the mounted tree */
  struct super_block *mnt_sb; /* pointer to superblock */
  int mnt_flags;
} __randomize_layout;

// 和其他人关系
// 1. namespace
// 2. mount
struct mount {
  struct hlist_node mnt_hash; // 全局表格
  struct mount *mnt_parent; // 爸爸是谁
  struct dentry *mnt_mountpoint; // 放在爸爸的什么位置
  struct vfsmount mnt;
  union {
    struct rcu_head mnt_rcu;
    struct llist_node mnt_llist;
  };
#ifdef CONFIG_SMP
  struct mnt_pcp __percpu *mnt_pcp;
#else
  int mnt_count;
  int mnt_writers;
#endif
  struct list_head mnt_mounts;  /* list of children, anchored here */
  struct list_head mnt_child; /* and going through their mnt_child */
  struct list_head mnt_instance;  /* mount instance on sb->s_mounts */
  const char *mnt_devname;  /* Name of device e.g. /dev/dsk/hda1 */
  struct list_head mnt_list; /* 链接到进程namespace中已挂载文件系统中，表头为mnt_namespace的list域 */
  struct list_head mnt_expire;  /* link in fs-specific expiry list */
  struct list_head mnt_share; /* circular list of shared mounts */
  struct list_head mnt_slave_list;/* list of slave mounts */
  struct list_head mnt_slave; /* slave list entry */
  struct mount *mnt_master; /* slave is on master->mnt_slave_list */
  struct mnt_namespace *mnt_ns; /* containing namespace */
  struct mountpoint *mnt_mp;  /* where is it mounted */
  union {
    struct hlist_node mnt_mp_list;  /* list mounts with the same mountpoint */
    struct hlist_node mnt_umount;
  };
  struct list_head mnt_umounting; /* list entry for umount propagation */
#ifdef CONFIG_FSNOTIFY
  struct fsnotify_mark_connector __rcu *mnt_fsnotify_marks;
  __u32 mnt_fsnotify_mask;
#endif
  int mnt_id;     /* mount identifier */
  int mnt_group_id;   /* peer group identifier */
  int mnt_expiry_mark;    /* true if marked for expiry */
  struct hlist_head mnt_pins;
  struct hlist_head mnt_stuck_children;
} __randomize_layout;
```


老版本的记录:
1. register_filesystem => file_system_type::mount => 调用文件系统自定义mount函数，该函数只是 mount_nodev 的封装，作用是提供自己的 fill_super
2. fill_super 完成的基本任务 :


按照全新的版本 : fs_context_operations 即可
1. 参数解析
2. 上线

```c
static const struct fs_context_operations hugetlbfs_fs_context_ops = {
  .free   = hugetlbfs_fs_context_free,
  .parse_param  = hugetlbfs_parse_param,
  .get_tree = hugetlbfs_get_tree,
};

static int hugetlbfs_get_tree(struct fs_context *fc)
{
  int err = hugetlbfs_validate(fc);
  if (err)
    return err;
  return get_tree_nodev(fc, hugetlbfs_fill_super);
  // @todo 总结一下 get_tree_nodev 对于含有 dev 对称函数
}
```

新版本:
1. some important calling graph of mount :
```c
kern_mount
  vfs_kern_mount : 调用特定文件系统的回调函数，构建一个vfsmount
      fs_context_for_mount
        alloc_fs_context : fc->fs_type->init_fs_context or legacy_init_fs_context
  fc_mount
      vfs_get_tree
          fc->ops->get_tree : get_tree is init in the alloc_fs_context, legacy_init_fs_context is init get_tree with legacy_get_tree
              A : legacy_get_tree : fc->fs_type->mount
              B : get_tree_nodev : it seems that kernel always call get_tree_nodev eg. get_tree_nodev(fc, shmem_fill_super) :
                    vfs_get_super
                        sget_fc
                        fill_super
      vfs_create_mount : Note that this does not attach the mount to anything.
```

```c
do_mount
  do_new_mount
    fs_context_for_mount : init the context
    vfs_get_tree
    do_new_mount_fc
      vfs_create_mount
      do_add_mount : 将 vfsmount 结构放到全局目录树中间
        graft_tree
```


2. some important function :
    1. fs_context_for_mount : init context
    2. vfs_get_tree :  sget and fill_super
    3. vfs_create_mount : init vfsmount

从do_mount的代码可以它主要就是：
- 将dir_name解析为path格式到内核
- 一路解析flags位表，将flags拆分位mnt_flags和sb_flags
- 根据flags中的标记，决定下面做哪一个mount操作。

do_add_mount函数主要做两件事：
1. lock_mount确定本次挂载要挂载到哪个父挂载实例parent的哪个挂载点mp上。
2. 把newmnt挂载到parent的mp下，完成newmnt到全局的安装。安装后的样子就像我们前文讲述的那样。

#### pivot_root
1. 为什么更改 root 的mount 位置存在这种诡异的需求啊
    1. 找到使用这个的软件


## superblock
super 作为一个文件系统实例的信息管理中心，很多动态信息依赖于 superlock.
    1. sync，fs-writeback
    2. remove/create inode / file

mount 的时候，首先需要加载 superblock

问题1 : kill super 到底是在做什么 ?
```c
void kill_anon_super(struct super_block *sb)
{
  dev_t dev = sb->s_dev;
  generic_shutdown_super(sb);
  free_anon_bdev(dev);
}

void kill_litter_super(struct super_block *sb)
{
  if (sb->s_root)
    d_genocide(sb->s_root);
  kill_anon_super(sb);
}

void kill_block_super(struct super_block *sb) // 提供基于block 的 fs, ext2 fat 之类的
{
  struct block_device *bdev = sb->s_bdev;
  fmode_t mode = sb->s_mode;

  bdev->bd_super = NULL;
  generic_shutdown_super(sb); // TODO
  sync_blockdev(bdev);
  WARN_ON_ONCE(!(mode & FMODE_EXCL));
  blkdev_put(bdev, mode | FMODE_EXCL);
}
```
- kill_block_super(), which unmounts a file system on a block device
- kill_anon_super(), which unmounts a virtual file system (information is generated when requested)
- kill_litter_super(), which unmounts a file system that is not on a physical device (the information is kept in memory)
An example for a file system without disk support is the ramfs_mount() function in the ramfs file system:
> kill_anon_super 和 kill_litter_super 使用标准是什么，基于内存的和 generated when needed 的区别还是很清晰的，
> 但是检查了一下 reference ，感觉存在疑惑 !
> 其次 TODO 这些 kill_super 函数 和 super_operation::put_super 的关系是什么 ?

问题2 : 各种 mount 辅助函数，搞不清楚到底谁在使用新版本的接口
- mount_bdev(), which mounts a file system stored on a block device
- mount_single(), which mounts a file system that shares an instance between all mount operations
- mount_nodev(), which mounts a file system that is not on a physical device
- mount_pseudo(), a helper function for pseudo-file systems (sockfs, pipefs, generally file systems that can not be mounted)

#### super operations
> TODO 这些都是需要检查一下，总结一下的。
- alloc_inode: allocates an inode. Usually, this funcion allocates a struct <fsname>_inode_info structure and performs basic VFS inode initialization (using inode_init_once()); minix uses for allocation the kmem_cache_alloc() function that interacts with the SLAB subsystem. For each allocation, the cache construction is called, which in the case of minix is the init_once() function. Alternatively, kmalloc() can be used, in which case the inode_init_once() function should be called. The alloc_inode() function will be called by the new_inode() and iget_locked() functions.
- destroy_inode releases the memory occupied by inode
- write_inode : 对于一个 inode 的信息进行更新
- - evict_inode: removes any information about the inode with the number received in the i_ino field **from the disk and memory** (both the inode on the disk and the associated data blocks). This involves performing the following operations:

```c
static const struct super_operations myfs_sops = {
  .statfs = simple_statfs,
  .drop_inode = generic_delete_inode,
};
```




## dentry
存在如下的问题:
1. vfs inode 对应磁盘中间的一个 disk inode，dentry 是不是也是磁盘中间 dentry 的对应 ?
    1. 加载过程和写回过程 ?
2. 其中 d_op 的作用是什么 ?
3. dcache 和 dentry 的关系
    1. 有没有拿掉 dcache 的操作，这种选项
1. Everything is file.
    1. directory is file too ! Find the evidence :
        1. `struct dentry` for ext2 : contains filename + inode number
            1. ino -> inode table index -> fetch inode content
            2. inode content -> file
        2. how ext2 init `struct dentry` ?
        3. file whoes type is directory : a array of dentry !
        4. but dir and file is different : there are there are different file operation and inode operations for directory
            1. (file directory) (inode file)
2. how `struct dentry` are init ? walk_component => lookup_slow
3. if a directory contains thousands of files, so we will create thousands of struct dentry when open the directory ?


1. `d_make_root`: allocates the root dentry. It is generally used in the function that is called to read the superblock (fill_super), which must initialize the root directory. So the root inode is obtained from the superblock and is used as an argument to this function, to fill the s_root field from the struct super_block structure.
2. `d_add`: associates a dentry with an inode; the dentry received as a parameter in the calls discussed above signifies the entry (name, length) that needs to be created. This function will be used when creating/loading a new inode that does not have a dentry associated with it and has not yet been introduced to the hash table of inodes (at lookup); 将 新创建的 inode 和 其 dentry 关联起来。
3. `d_instantiate`: The lighter version of the previous call, in which the dentry was previously added in the hash table.

```c
struct dentry {
  /* RCU lookup touched fields */
  unsigned int d_flags;   /* protected by d_lock */
  seqcount_t d_seq;   /* per dentry seqlock */
  struct hlist_bl_node d_hash;  /* lookup hash list */
  struct dentry *d_parent;  /* parent directory */
  struct qstr d_name;
  struct inode *d_inode;    /* Where the name belongs to - NULL is
           * negative */
  unsigned char d_iname[DNAME_INLINE_LEN];  /* small names */

  /* Ref lookup also touches following */
  struct lockref d_lockref; /* per-dentry lock and refcount */
  const struct dentry_operations *d_op;
  struct super_block *d_sb; /* The root of the dentry tree */
  unsigned long d_time;   /* used by d_revalidate */
  void *d_fsdata;     /* fs-specific data */

  union {
    struct list_head d_lru;   /* LRU list */
    wait_queue_head_t *d_wait;  /* in-lookup ones only */
  };
  struct list_head d_child; /* child of parent list */
  struct list_head d_subdirs; /* our children */
  /*
   * d_alias and d_rcu can share memory
   */
  union {
    struct hlist_node d_alias;  /* inode alias list */
    struct hlist_bl_node d_in_lookup_hash;  /* only for in-lookup ones */
    struct rcu_head d_rcu;
  } d_u;
} __randomize_layout;
```

negative dentry [^6] :
1. dentry is a way of remembering the resolution of a given file or directory name without having to search through the filesystem to find it.
2. A negative dentry is a little different, though: it is a memory of a filesystem lookup that failed.

#### d_path

- [ ] fs/d_path.c

- [ ] d_path
- [ ] getpwd

## helper
1. `inode_init_owner` :

3. `d_make_root` : 通过 root inode 创建出来对应的 dentry，所有的inode 都是存在对应的 dentry 并且存放在 parent direcory file 中间，但是唯独 root 不行。

```c
struct dentry *d_make_root(struct inode *root_inode)
{
  struct dentry *res = NULL;

  if (root_inode) {
    res = d_alloc_anon(root_inode->i_sb);
    if (res) {
      res->d_flags |= DCACHE_RCUACCESS;
      d_instantiate(res, root_inode);
    } else {
      iput(root_inode);
    }
  }
  return res;
}
```

4. `inode_init_always` : 基本的初始化

5. `inode_init_once` : 所以，inode_init_always 和 inode_init_once 存在什么区别吗?
inode_init_once 看来作用更像是更加基本的初始化，在 minfs 中间之所以需要单独调用 init_once，
因为其使用的是通用的 kmalloc 机制。

```c
static int __init init_inodecache(void)
{
  ext4_inode_cachep = kmem_cache_create_usercopy("ext4_inode_cache",
        sizeof(struct ext4_inode_info), 0,
        (SLAB_RECLAIM_ACCOUNT|SLAB_MEM_SPREAD|
          SLAB_ACCOUNT),
        offsetof(struct ext4_inode_info, i_data),
        sizeof_field(struct ext4_inode_info, i_data),
        init_once); //
  if (ext4_inode_cachep == NULL)
    return -ENOMEM;
  return 0;
}
```

## attr
1. 是不是还有一个扩展属性的一个东西 ?

2. 看了一下 ext2_setattr 和 ext2_getattr 的实现，只是提供 inode 的各种基本属性而已。


## open.c(make this part more clear, split it)
vfs 提供的基础设施总结一下
1. lookup
2. open ? 为什么 open 需要单独分析

getname_kernel 和  getname_flags 有什么区别吗 ?


## open
- [ ] relation with openat
  - [ ] AT_FDCWD

Three kinds of open flags accoring to man page:
1. file privilege : O_RDONLY, O_WRONLY, or O_RDWR
2. file create : O_CLOEXEC, O_CREAT, O_DIRECTORY, O_EXCL, O_NOCTTY, O_NOFOLLOW, O_TMPFILE
3. file status : O_APPEND, O_ASYNC

- [ ] what's relation with fmode, e.g. FMODE_WRITE  ?


- [ ]  O_CLOEXEC
> By default, the new file descriptor is set to remain open across an execve(2) (i.e., the FD_CLOEXEC file descriptor flag described in fcntl(2) is initially disabled); the O_CLOEXEC flag, described below, can be used to change this default.  The file offset is set to the beginning of the file (see lseek(2)).

- [ ] O_DIRECT
    - [ ] what if I open file with O_DIRECT and do read write with aio ?
        - There is no contradictory, just skip page cache

> The O_DIRECT flag on its own makes an effort to transfer data synchronously,
but does not give the guarantees of the *O_SYNC* flag that data and necessary metadata are transferred.
To guarantee synchronous I/O, O_SYNC must be used in addition to O_DIRECT.

```c
ssize_t noop_direct_IO(struct kiocb *iocb, struct iov_iter *iter)
{
  /*
   * iomap based filesystems support direct I/O without need for
   * this callback. However, it still needs to be set in
   * inode->a_ops so that open/fcntl know that direct I/O is
   * generally supported.
   */
  return -EINVAL;
}
```
Trace caller of `iomap_dio_rw`, we will find out how to do DIRECT IO on linux.

file_operations::write_iter(ext4_file_write_iter) ==> ext4_dio_write_iter ==> iomap_dio_rw

- [ ] O_ASYNC

**This flags's meaning/function is not what I expected, really interesting**

## iomap
ext4/xfs's direct IO call `iomap_dio_rw`

- [ ] iomap_dio_rw

xfs_file_read_iter
==> xfs_file_dio_aio_read ==> iomap_dio_rw
==> xfs_file_buffered_aio_read ==> generic_file_read_iter

  - [ ] I don't know how iomap_dio_rw write page to disk ?


--> several

- [ ] iomap_write_actor
  - xfs_file_write_iter ==> xfs_file_buffered_aio_write ==> iomap_file_buffered_write, direct io has very similar path
- [ ] iomap_page_mkwrite_actor
  - [ ] iomap_page_mkwrite : use `xfs` as example: xfs_filemap_fault ==> `_xfs_filemap_fault` ==> `__xfs_filemap_fault`
- [ ] iomap_readpage_actor
- [ ] iomap_unshare_actor
- [ ] iomap_zero_range_actor
- [ ] iomap_readahead_actor

IOMAP_F_BUFFER_HEAD : It seems iomap is used for kill page cache, by read function iomap_write_actor, that's just how `generic_file_write_iter` works.

maybe iomap is not mature yet, iomap provide a generic interface for fs to read / write / readahead ..., but vfs also provide generic_file_write_iter / generic_file_read_iter, it just doesn't make sense.
```c
/*
 * Flags reported by the file system from iomap_begin:
 *
 * IOMAP_F_NEW indicates that the blocks have been newly allocated and need
 * zeroing for areas that no data is copied to.
 *
 * IOMAP_F_DIRTY indicates the inode has uncommitted metadata needed to access
 * written data and requires fdatasync to commit them to persistent storage.
 * This needs to take into account metadata changes that *may* be made at IO
 * completion, such as file size updates from direct IO.
 *
 * IOMAP_F_SHARED indicates that the blocks are shared, and will need to be
 * unshared as part a write.
 *
 * IOMAP_F_MERGED indicates that the iomap contains the merge of multiple block
 * mappings.
 *
 * IOMAP_F_BUFFER_HEAD indicates that the file system requires the use of
 * buffer heads for this mapping.
 */
#define IOMAP_F_NEW   0x01
#define IOMAP_F_DIRTY   0x02
#define IOMAP_F_SHARED    0x04
#define IOMAP_F_MERGED    0x08
#define IOMAP_F_BUFFER_HEAD 0x10
```



## block layer
似乎 block layer 被取消了，但是似乎 mq 又是存在的 ?
一个 block 被发送到被使用:
```python
b.attach_kprobe(event="blk_mq_start_request", fn_name="trace_start")
b.attach_kprobe(event="blk_account_io_completion", fn_name="trace_completion")
```


## initfs
[Documentation for ramfs, rootfs, initramfs.](https://lwn.net/Articles/156098/)

https://landley.net/writing/rootfs-intro.html
1. 因为需要 mount rootfs，在曾经的世界中间，给内核指定一个参数就可以了。
2. 如果是靠指定参数的方法，为什么还需要 initrd ?
3. 既然 initrd 还是可以使用的，为什么 qemu 不能运行 ?

Robert love 谈到 initramfs 的作用:
https://qr.ae/pNs0eS
但是还是让人非常迷惑:
1. grub 如何让 ssd 中间的 initramfs 被加载到内存
2. nvme 模块如何被加载内存
3. kernel image 如何被加载到内存的

## overlay fs
- [ ] an intorduction : https://www.terriblecode.com/blog/how-docker-images-work-union-file-systems-for-dummies/

## inode
感觉 inode.c 其实和 dcache.c 是对称的，inode 本身作为 cache 的，与需要加载，删除，初始化等操作

inode.c 中间存在的函数:
1. iget_locked / iput : 被其他的子系统调用(ext4), 用于创建一个新的 general inode
2. inode_init_owner : 初始化 inode 对应的 uid, gid, mode
3. super_block::s_inode_lru : 用于 inode 的回收工作

而 dcache.c 中存在的函数:
1. d_lookup
2. dput
3. d_alloc
4. d_find_alias : alias 应该是 hash 导致的 ?
5. super_block::s_dentry_lru : 用于回收



1. inode.c 中间存在好几个结尾数字为5的函数，表示什么含义啊 ?

其中的集大成者是 iget5_locked ?
```c
struct inode *iget5_locked(struct super_block *sb, unsigned long hashval,
    int (*test)(struct inode *, void *),
    int (*set)(struct inode *, void *), void *data)
{
  struct inode *inode = ilookup5(sb, hashval, test, data);

  if (!inode) {
    struct inode *new = alloc_inode(sb);

    if (new) {
      new->i_state = 0;
      inode = inode_insert5(new, hashval, test, set, data);
      if (unlikely(inode != new))
        destroy_inode(new);
    }
  }
  return inode;
}
```
真正的有意义的调用:

```c
struct block_device *bdget(dev_t dev)
{
  struct block_device *bdev;
  struct inode *inode;

  inode = iget5_locked(blockdev_superblock, hash(dev),
      bdev_test, bdev_set, &dev);
```

有意思
2. 注释，根本不能理解，inode 数量不够是啥意思啊
 * This is a generalized version of ilookup() for file systems where the
 * inode number is not sufficient for unique identification of an inode.

3. 两个版本的函数都是存在的 ilookup 和 ilookup5


* ***hash***
1. 也是存在 inode_hashtable 的，就像是 dentry_hashtable 一样
2. 之所以使用 hash 而不是 inode number，是为了防止多个文件系统，inode numebr 互相重复吧!

```c
static unsigned long hash(struct super_block *sb, unsigned long hashval)
{
  unsigned long tmp;

  tmp = (hashval * (unsigned long)sb) ^ (GOLDEN_RATIO_PRIME + hashval) /
      L1_CACHE_BYTES;
  tmp = tmp ^ ((tmp ^ GOLDEN_RATIO_PRIME) >> i_hash_shift);
  return tmp & i_hash_mask;
}
```

- [ ] get_next_ino()
  - [ ] what's relation with struct inode::i_ino and ext4 disk inode

## dcache
问题
1. alias 是做什么的 ?
    1. d_move 的作用
2. d_ops 的作用是什么
3. negtive 怎么产生的 ?

分析一个 cache 基本方法:
1. 加入
2. 删除
3. 查询

There are a number of functions defined which permit a filesystem to manipulate dentries:
dget : open a new handle for an existing dentry (this just increments the usage count)
dput : close a handle for a dentry (decrements the usage count). If the usage count drops to 0, and the dentry is still in its parent’s hash, the “d_delete” method is called to check whether it should be cached. If it should not be cached, or if the dentry is not hashed, it is deleted. Otherwise cached dentries are put into an LRU list to be reclaimed on memory shortage.
d_drop : **this unhashes a dentry from its parents hash list.** A subsequent call to dput() will deallocate the dentry if its usage count drops to 0
d_delete : delete a dentry. If there are no other open references to the dentry then the dentry is turned into a negative dentry (the d_iput() method is called). **If there are other references, then d_drop() is called instead**
d_add : add a dentry to its parents hash list and then calls d_instantiate()
d_instantiate : add a dentry to the alias hash list for the inode and updates the “d_inode” member. The “i_count” member in the inode structure should be set/incremented. If the inode pointer is NULL, the dentry is called a “negative dentry”. This function is commonly called when an inode is created for an existing negative dentry
d_lookup : look up a dentry given its parent and path name component It looks up the child of that given name from the dcache hash table. If it is found, the reference count is incremented and the dentry is returned. The caller must use dput() to free the dentry when it finishes using it.

* ***分析: d_lookup***

d_lookup 相比 `__d_lookup` 多出了 rename 的 lock 问题，查询是在 parent 是否存在指定的 children
```c
static struct hlist_bl_head *dentry_hashtable __read_mostly;

static inline struct hlist_bl_head *d_hash(unsigned int hash)
{
  return dentry_hashtable + (hash >> d_hash_shift);
}
```

按照字符串的名字，找到位置，然后对比。问题是:
1. 似乎 dentry_hashtable 是一个全局的，如果文件系统中间存在几千个 readme.md 岂不是 GG
2. hlist 是怎么维护的呀，大小如何初始化呀

* ***d_add***

1. d_add 调用位置太少了，比较通用的调用位置在 libfs 中间，
2. 其核心执行的内容如下，也就是存在两个 hash，全局的，每个

    hlist_add_head(&dentry->d_u.d_alias, &inode->i_dentry);
3. 实际上，依托 hlist_bl_add_head_rcu d_hash 加入 dentry_hashtable


The “dcache” caches information about names in each filesystem to make them quickly available for lookup.
Each entry (known as a “dentry”) contains three significant fields:
1. a component name,
2. *a pointer to a parent dentry*,
3. and a pointer to the “inode” which contains further information about the object in that parent with the given name.
> 为啥需要包含 parent dentry ?

#### dcache shrink
1. alloc_super 中间创建了一个完成 shrinker 的初始化 TODO 这个 shrinker 机制还有人用吗 ?
    1. super_cache_scan
    2. super_cache_count

2. `struct superpage` 中间存在两个函数 TODO 应该用于特殊内容的缓存的
```c
  long (*nr_cached_objects)(struct super_block *,
          struct shrink_control *);
  long (*free_cached_objects)(struct super_block *,
            struct shrink_control *);
```
3.  super_cache_scan 调用两个函数
    1. prune_dcache_sb
    2. prune_icache_sb
```c
long prune_dcache_sb(struct super_block *sb, struct shrink_control *sc)
{
  LIST_HEAD(dispose);
  long freed;

  freed = list_lru_shrink_walk(&sb->s_dentry_lru, sc,
             dentry_lru_isolate, &dispose);
  shrink_dentry_list(&dispose);
  return freed;
}

prune_icache_sb : TODO 这个是用于释放 inode 还是 inode 持有的文件 ? 还是当 inode 被打开之后就不释放 ?
```

4.
list_lru_shrink_walk 似乎就是遍历一下列表，将可以清理的页面放出来
```c
static enum lru_status dentry_lru_isolate(struct list_head *item,
    struct list_lru_one *lru, spinlock_t *lru_lock, void *arg)
```
然后使用 prune_dcache_sb 紧接着调用 shrink_dentry_list ，将刚刚清理出来的内容真正的释放:

5. shrink 的源头还有 unmount 的时候

那么 dcache / icache 的 shrink 机制在整个 shrink 机制中间是怎么处理的 ?
shrink_node_memcgs ==> shrink_slab ==> 对于所有的 `struct shrinker` 调用 do_shrink_slab

对于 inode 和 icache 的回收是放在 alloc_super 的初始化中间的。
而 x86 kvm 中间也是存在对于 shrinker 的回收工作的。
```c
static struct shrinker mmu_shrinker = {
  .count_objects = mmu_shrink_count,
  .scan_objects = mmu_shrink_scan,
  .seeks = DEFAULT_SEEKS * 10,
};
```

## splice
Four syscall related : vmsplice splice tee pipe

-  [connect buffer and pipe buffer with splice](https://gist.github.com/karthick18/1234187)

> splice() moves data between two file descriptors without copying between kernel address space and user address space.
> It transfers up to len bytes of data from the file descriptor fd_in to the file descriptor `fd_out`, where one of the file descriptors must refer to a pipe.

without splice, we can copy file by coping file to user space and coping data in userspace to kernel space.
splice can copy file directly in kernel with pipe.

Because splice connect two fd (one of is pipe fd), so splice also works with tee.
Tee can copy files without consuming it, in the second stage, we can splice the data to files.

- [ ] ./tlpi-dist/pipes/pipe_sync.c
    - if parent contains a pipe and fork several children, then they share a public pipe, in another word, everyone write to same buffer and everyone read from same buffer.
    - Only the buffer is close by parent and all children, will it be closed
    - [ ] so, check how kernel work for this

## pipe
- https://questions.wizardzines.com/bash-redirects.html
  - > will sudo echo x > /file allow you to redirect to /file if it's owned by root?
  - > some_cmd 2>&1 > file
  - > some_cmd 2>&1
  - > some_cmd 2> file.txt


- [ ] pipefs
  - [ ] why we need pipefs ?

- [ ] linux-kernel-labs 中间似乎创建 pipe 文件
```c
|rw-rw-r-- maritns3 maritns3   0 B  Thu Nov  5 17:59:49 2020  pipe1.in
|rw-rw-r-- maritns3 maritns3   0 B  Thu Nov  5 17:59:49 2020  pipe1.out
|rw-rw-r-- maritns3 maritns3   0 B  Thu Nov  5 17:59:49 2020  pipe2.in
|rw-rw-r-- maritns3 maritns3   0 B  Thu Nov  5 17:59:49 2020  pipe2.out
```


- [x] get_pipe_inode()
  - init inode
  - only called by create_pipe_files
    - create_pipe_files ==> get_pipe_inode + alloc_file_pseudo + alloc_file_clone
    - create_pipe_files alloc one `struct inode` and two `struct file`.
    - both of `file` points to the `inode`
  - alloc_pipe_info : alloc pipe related code


If we get fd, then find inode, then standard path to `pipefifo_fops::pipe_read`


struct pipe_inode_info::pipe_buffer points an array of pipe_buffer, every pipe_buffer manages one page frame.
```c
const struct file_operations pipefifo_fops = {
  .open   = fifo_open,
  .llseek   = no_llseek,
  .read_iter  = pipe_read,
  .write_iter = pipe_write,
  .poll   = pipe_poll,
  .unlocked_ioctl = pipe_ioctl,
  .release  = pipe_release,
  .fasync   = pipe_fasync,
};
```


buffer ops: because some optimization, e.g., steal page to avoid coping, we can do some magic about buffer page.
```c
/*
 * Note on the nesting of these functions:
 *
 * ->confirm()
 *  ->try_steal()
 *
 * That is, ->try_steal() must be called on a confirmed buffer.  See below for
 * the meaning of each operation.  Also see the kerneldoc in fs/pipe.c for the
 * pipe and generic variants of these hooks.
 */
struct pipe_buf_operations {
  /*
   * ->confirm() verifies that the data in the pipe buffer is there
   * and that the contents are good. If the pages in the pipe belong
   * to a file system, we may need to wait for IO completion in this
   * hook. Returns 0 for good, or a negative error value in case of
   * error.  If not present all pages are considered good.
   */
  int (*confirm)(struct pipe_inode_info *, struct pipe_buffer *);

  /*
   * When the contents of this pipe buffer has been completely
   * consumed by a reader, ->release() is called.
   */
  void (*release)(struct pipe_inode_info *, struct pipe_buffer *);

  /*
   * Attempt to take ownership of the pipe buffer and its contents.
   * ->try_steal() returns %true for success, in which case the contents
   * of the pipe (the buf->page) is locked and now completely owned by the
   * caller. The page may then be transferred to a different mapping, the
   * most often used case is insertion into different file address space
   * cache.
   */
  bool (*try_steal)(struct pipe_inode_info *, struct pipe_buffer *);

  /*
   * Get a reference to the pipe buffer.
   */
  bool (*get)(struct pipe_inode_info *, struct pipe_buffer *);
};
```

## devpts
https://www.kernel.org/doc/html/latest/filesystems/devpts.html


## dup(merge)
1. 为什么会存在 dup 一个 fd 的需求啊
2. 多个 file 会指向 inode，因为对于文件的读写状态不同的
    1. 但是为什么会存在多个 file descriptor 指向同一个 fd 啊 ?

Duplicated file descriptors created by dup2() or fork() point to the same file object.
## IO buffer(merge)
1. tlpi 的 chapter 13 可以阅读一下

## notify
1. io_uring 或者 aio 或者 之类的机制可以替代 inotify 吗 ?
2. 可以搞一下的细节:
    1. fsnotify_group 是怎么回事
    2. 如何通知支持 inotify 和 dnotify 两个模块的


* ***如何实现告知 ?***
```c
fsnotify_modify : 在 open.c 中间可以查看到
/*
 * fsnotify_modify - file was modified
 */
static inline void fsnotify_modify(struct file *file) { fsnotify_file(file, FS_MODIFY); }

/**
 * This is the main call to fsnotify.  The VFS calls into hook specific functions
 * in linux/fsnotify.h.  Those functions then in turn call here.  Here will call
 * out to all of the registered fsnotify_group.  Those groups can then use the
 * notification event in whatever means they feel necessary.
 */
int fsnotify(struct inode *to_tell, __u32 mask, const void *data, int data_is,
       const struct qstr *file_name, u32 cookie)
```

* ***如何让从 notifyFd 中间读出 event***

```c
/* inotify syscalls */
static int do_inotify_init(int flags)
{
  struct fsnotify_group *group;
  int ret;

  /* Check the IN_* constants for consistency.  */
  BUILD_BUG_ON(IN_CLOEXEC != O_CLOEXEC);
  BUILD_BUG_ON(IN_NONBLOCK != O_NONBLOCK);

  if (flags & ~(IN_CLOEXEC | IN_NONBLOCK))
    return -EINVAL;

  /* fsnotify_obtain_group took a reference to group, we put this when we kill the file in the end */
  group = inotify_new_group(inotify_max_queued_events);
  if (IS_ERR(group))
    return PTR_ERR(group);

  ret = anon_inode_getfd("inotify", &inotify_fops, group, // 这就是关键吧
          O_RDONLY | flags);
  if (ret < 0)
    fsnotify_destroy_group(group);

  return ret;
}
```

notify 的工作，和 aio epoll 机制其实都是类似的，相比于 blocking io，每一个 IO 需要一个 thread 阻塞，而切换为 epoll，多个 IO 被阻塞到同一个 thread 而已，还是说其实 aio 是存在各种设计模式的 TODO

## attr

## dax
2. dax 机制是什么 ?

## block device
1. 也许读一下 ldd3 比较有用 ?
2. 现在的问题是，根本搞不清楚，为什么，为什么 char dev 是有效的

## char device

## tmpfs
cat /proc/mounts : we find the filesystem type of /dev is tmpfs, so what's the difference between fs between

## exciting
http://www.betrfs.org/

## benchmark
https://lwn.net/Articles/385081/
https://research.cs.wic.edu/wind/Publications/impressions-fast09.pdfs

#### fio
> TODO 将 fio 的类容整理到此处, 然后将那个文件夹删除

#### filebench
下面记录 [^12]:
I/O Microbenchmarking
Trace Capture/Reply
Model Based

FileBench is an application simulator

"Benchpoint" Run Generation Wrapper ??????
prof : 真的存在吗 ?

下面记录 [^11]:

There are four main entities in WML: fileset, process, thread, and flowp

A Filebenh run proceeds in two stages: fileset preallocation and
an actual workload executionc

By default, Filebench does not create any files in the file system and only allocates enough internal
file entries to accommodate all defined files.
> internal file entries 是 filebench 自己管理的吧 !

Every thread executes a loop of flowops.
> 能不能 randomly choose a file

Filebench uses Virtual Fie Descriptors (VFDs) to refer to files in flowops.

When opening a file, Filebench first needs to pick afile from a
fileset. *By default this is done by iterating over all file entries in
a fileset.* To change this behavior one can use the index attribute that allows one to refer to a specific file in a fileset using
a unique index. In most real cases, instead of using a constant
number for the index, one should use custom variables described
in the following section
> fd 自动顺序选择，是不是 fd 默认为 1

The abilty to quickly define multiple processes and synchronization between them was one of the main requirements during
Filebench framework conception.
> Filebench 设计本来的目的是对于实际 workload 的模拟

https://github.com/filebench/filebench/issues/130
似乎有的需要添加上 `run time`

https://github.com/filebench/filebench/issues/112
```
echo 0 > /proc/sys/kernel/ranomize_va_spaced
```
解决 wait pid 的问题


**还存在的一些问题:**
1. 虽然 配置 中间显示只是使用一个 process 和 一个 thread，但是 htop 显示存在两个
      1. 使用 4 process 1 thread 的结构，结果
2. 感觉文件都是没有被创建过的样子
3. prealloc 的含义是什么 ?
4. 是不是只有循环这个操作，那么调整 read / write 之类的比例，岂不是直接 GG
When openinga file, Filebench first needs to pick a file from a
fileset. By default this is done by iterating over all file entries in
a fileset. To change this behavior one can use the index attribute that allows one to refer to a specific file in a fileset using
a unique *index*. In most real cases, instead of using a constant
number for the index, one should use custom variables described
in the following section.
> index 从来没有体现过作用!


使用的记录:
1. read write 必须配对，但是可以都去掉.
2. 通过 debug 10 得到的日志，显示默认 fd = 1 的方法，如果

运行一下各个结果:
1. finishoncount : 似乎没有作用
2. fd=1 是用于一个 loop 中，而不是用于选择文件的

计划:
1. fileset 的构建
2. 这个支持太少了，有必要吗增加吗 ? open 是测试 vfs 的性能，还是测试

## nvme
multi-stream :
https://m.weixin.qq.com/s/iXIhqqP-j1zxMIjA31G4fwp
https:/www.samsung.com/us/labs/pdfs/2016-08-fms-multi-stream-v4.pdf/


## fallocate
本以为只是快速实现创建大文件，实际上可以进行很多有意思的操作，可以简单调查一下.

## union fs
docker

## TODO
- [ ] https://news.ycombinator.com/item?id=24758024 : O_SYNC O_DIRECT and many other semantic of posix interface

## multiqueue
[lwn : The multiqueue block layer](https://lwn.net/Articles/552904/)
> While requests are in the submission queue, they can be operated on by the block layer in the usual manner. Reordering of requests for locality offers **little** or no benefit on solid-state devices;
> indeed, spreading requests out across the device might help with the parallel processing of requests.
> So reordering will not be done, but coalescing requests will reduce the total number of I/O operations, improving performance somewhat.
> Since the submission queues are **per-CPU**, there is no way to coalesce requests submitted to different queues.
> With no empirical evidence whatsoever, your editor would guess that adjacent requests are most likely to come from the same process and,
> thus, will automatically find their way into the same submission queue, so the lack of cross-CPU coalescing is probably not a big problem.



[Linux Block IO: Introducing Multi-queue SSD Access on Multi-core Systems](https://kernel.dk/systor13-final18.pdf)
Why we need block layer:
1. It is a convenience library to hide the complexity and diversity of storage devices from the application while providing common services that are valuable to applications.
2. In addition, the block layer implements IO-fairness, IO-error handling, IO-statistics, and IO-scheduling that improve performance and help protect end-users from poor or malicious implementations of other applications or device drivers.

Specifically, we identified three main problems:
1. Request Queue Locking
2. Hardware Interrupts
3. Remote Memory Accesses

reducing lock contention and remote memory accesses are key challenges when redesigning the block layer to scale on high NUMA-factor architectures.
Dealing efficiently with the high number of hardware interrupts is beyond the control of the block layer (more on this below) as the block layer cannot dictate how a
device driver interacts with its hardware.

Based on our analysis of the Linux block layer, we identify three major requirements for a block layer:
1. **Single Device Fairness** :  Without a centralized arbiter of device access, applications must either coordinate among themselves for fairness or rely on the fairness policies implemented in device drivers (which rarely exist).
2. **Single and Multiple Device Accounting** : Having a uniform interface for system performance monitoring and accounting enables applications and other operating system components to make intelligent decisions about application scheduling, load balancing, and performance.
3. **Single Device IO Staging Area** :
    - To do this, the block layer requires a staging area, where IOs may be buffered before they are sent down into the device driver.
    - Using a staging area, the block layer can reorder IOs, typically to promote sequential accesses over random ones, or it can group IOs, to submit larger IOs to the underlying device.
    - In addition, the staging area allows the block layer to adjust its submission rate for quality of service or due to device back-pressure indicating the OS should not send down additional IO or risk overflowing the device’s buffering capability.


**Our two level queuing strategy relies on the fact that modern SSD’s have random read and write latency that is as fast
as their sequential access. Thus interleaving IOs from multiple software dispatch queues into a single hardware dispatch
queue does not hurt device performance. Also, by inserting
requests into the local software request queue, our design
respects thread locality for IOs and their completion.**

In our design we have moved IO-scheduling functionality into the software queues only, thus even legacy devices that implement just a single
dispatch queue see improved scaling from the new multiqueue block layer.

**In addition** to introducing a **two-level queue based model**,
our design incoporates several other implementation improvements.
1. First, we introduce tag-based completions within the block layer. Device command tagging was first introduced
with hardware supporting native command queuing. A tag is an integer value that uniquely identifies the position of the
block IO in the driver submission queue, so when completed the tag is passed back from the device indicating which IO has been completed.
2. Second, to support fine grained IO accounting we have
modified the internal Linux accounting library to provide
statistics for the states of both the software queues and dis-
patch queues. We have also modified the existing tracing
and profiling mechanisms in blktrace, to support IO tracing
for future devices that are multi-queue aware.

While the basic mechanisms for driver registration and IO submission/completion remain
unchanged, our design introduces these following requirements:
- HW dispatch queue registration: The device driver must export the number of submission queues that it supports as well as the size of these queues, so that the
block layer can allocate the matching hardware dispatch queues.
- HW submission queue mapping function: The device driver must export a function that returns a mapping
between a given software level queue (associated to core i or NUMA node i), and the appropriate hardware dispatch queue.
- IO tag handling: The device driver tag management mechanism must be revised so that it accepts tags generated by the block layer. While not strictly required,
using a single data tag will result in optimal CPU usage between the device driver and block layer.

## null blk
https://lwn.net/Articles/552911/

https://www.kernel.org/doc/html/latest/block/null_blk.html

**TO BE CONTINUE**

*Lwn's example is a simplified version of drivers/block/null_blk_main.c, this , of course , is a great start to understand block layer.*

## proc

#### sysctl
`linux/kernel/sysctl.c` is a three thousand lines file, first part is about `proc_put_long`, second part is hierarchy of tables describing /proc/sys/

- [ ] so It works and different from part of proc ?

## mnt
在 fs/namespace.c 下面:

```c
__do_sys_mount [Function] 3431,1                                                                                                                                                                                                                          │
__do_sys_umount [Function] 1781,1                                                                                                                                                                                                                         │
__do_sys_fsmount [Function] 3496,1                                                                                                                                                                                                                        │
__do_sys_oldumount [Function] 1791,1                                                                                                                                                                                                                      │
__do_sys_open_tree [Function] 2441,1                                                                                                                                                                                                                      │
__do_sys_move_mount [Function] 3626,1                                                                                                                                                                                                                     │
__do_sys_pivot_root [Function] 3726,1                                                                                                                                                                                                                     │
__do_sys_mount_setattr [Function] 4132,1                                                                                                                                                                                                                  │
```
open_tree 和 move_mount 连 man 都没有

[这里](https://unix.stackexchange.com/questions/464033/understanding-how-mount-namespaces-work-in-linux)
提供了一个很好的例子，首先 unshare mount namespace，创建一个新的 mount point, 使用 findmnt 来检查，可以看到新的 mnt 点, 但是在另一个 shell 中间就是没有该 mnt 的。
在 container 的情况下，为了修改一个程序内部观测到 root 是从我们设置好的位置，需要 pivot_root 那嫁接一下。

猜测 : 由于 mount 的空间不同，实际上，在同一个路径，一个可能进入到 mount，一个可能不会，这导致路径查找变得更加复杂了。

## fifo
https://unix.stackexchange.com/questions/433488/what-is-the-purpose-of-using-a-fifo-vs-a-temporary-file-or-a-pipe


## configfs
https://www.kernel.org/doc/html/latest/filesystems/configfs.html

- [ ] after a quick browsing, I think is a userspace driven interface for driver

## binder
- [ ] https://www.jianshu.com/p/82b691cbdde4

## zone
https://zonedstorage.io/introduction/zoned-storage/

## nfs
- https://about.gitlab.com/blog/2018/11/14/how-we-spent-two-weeks-hunting-an-nfs-bug/
- https://github.com/nfs-ganesha/nfs-ganesha
- [ ] https://zhuanlan.zhihu.com/p/295230549 : 简介

- https://lwn.net/Articles/897917/
- 如何搭建 nfs 的调试环境？

## compression fs
- [ ] https://github.com/mhx/dwarfs
  - it's written with cpp

## timerfd
based on linux/kernel/time/alarmtimer.c

## ceph
https://www.youtube.com/watch?v=c4sgV_FEb4I

https://github.com/gluster/glusterfs

Somebody says ceph has one millions of code.

## close
- `__do_sys_close`
  - close_fd
    - pick_file : 通过 fd 获取 file
    - filp_close
      - fput
        - fput_many : 使用 task_work 来封装延迟
          - `____fput`
            - __fput : 真正完成工作的位置
              - file->f_op->release


```c
static struct file_operations kvm_vcpu_fops = {
  .release        = kvm_vcpu_release,
  .unlocked_ioctl = kvm_vcpu_ioctl,
  .mmap           = kvm_vcpu_mmap,
  .llseek   = noop_llseek,
  KVM_COMPAT(kvm_vcpu_compat_ioctl),
};
```

## [ ] hugetlbfs
- [ ] 虽然这是 memory 的功能，为什么使用 hugetlbfs 来作为接口
  - transparent huge tlb 没有使用这个东西吧

## cache
- 忽然理解了为什么需要将
https://www.kernel.org/doc/html/latest/filesystems/caching/index.html

## 用户态
- https://github.com/juicedata/juicefs
  - Using the JuiceFS to store data, the data itself will be persisted in object storage (e.g. Amazon S3), and the metadata corresponding to the data can be persisted in various database engines such as Redis, MySQL, and SQLite according to the needs of the scene.

## https://www.zhihu.com/question/21536660

## ext4_fiemap && do_vfs_ioctl 是做啥的

## erofs
fs/erofs : 什么场景下需要只读文件系统。

## 引用计数
- https://zhuanlan.zhihu.com/p/93228807

[^1]: [kernel doc : Overview of the Linux Virtual File System](https://www.kernel.org/doc/html/latest/filesystems/vfs.html)
[^2]: [github : aio](https://github.com/littledan/linux-aio)
[^3]: [stackexchange : loop device](https://unix.stackexchange.com/questions/66075/when-mounting-when-should-i-use-a-loop-device)
[^4]: [zhihu : mount syscall](https://zhuanlan.zhihu.com/p/36268333)
[^5]: [zhihu : mount](https://zhuanlan.zhihu.com/p/76419740)
[^6]: [lwn : Dentry negativity](https://lwn.net/Articles/814535/)
[^11]: [usenix : Filebench a flexible framework for fs benchmark](https://www.usenix.org/system/files/login/articles/login_spring16_02_tarasov.pdf)
[^12]: [sun : Filebench tutorial](http://www.nfsv4bat.org/Documents/nasconf/2005/mcdougall.pdf)
