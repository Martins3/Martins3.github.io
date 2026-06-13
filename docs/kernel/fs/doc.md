## VFS 文档
An individual dentry usually has a pointer to an inode.

Opening a file requires another operation: allocation of a file structure (this is the kernel-side implementation of file descriptors).
The freshly allocated file structure is initialized with a pointer to the dentry and a set of file operation member functions.
> 也就是说 : file struct 指向 dentry ?

New vfsmount referring to the tree returned by `->mount()` will be attached to the mountpoint, so that when pathname resolution reaches the mountpoint it will jump into the root of that vfsmount.
> 为什么感觉 mount 就像是 symbol link, 比如 mount /dev/sda1 ~/WanShe


Usually, a filesystem uses one of the generic `mount()` implementations and provides a `fill_super()` callback instead. The generic variants are:

- mount_bdev

  mount a filesystem residing on a block device

- mount_nodev

  mount a filesystem that is not backed by a device

- mount_single

  mount a filesystem which shares the instance between all mounts

```c
static struct dentry *ext2_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, ext2_fill_super);
}
```
> @todo file_system_type::mount 这个指针的调用位置，非常的令人窒息，fs_context 什么鬼东西啊 ?

Well before we get to symlinks we have another major division based on the VFS’s approach to locking which will allow us to review “REF-walk” and “RCU-walk” separately. But we are getting ahead of ourselves. There are some important low level distinctions we need to clarify first.

## 核心
- Documentation/filesystems/ 其中包括 fscache 的，主要是
  - Documentation/filesystems/vfs.rst
  - Documentation/filesystems/dax.rst
  - Documentation/filesystems/locking.rst.
  - Documentation/filesystems/devpts.rst
  - Documentation/filesystems/fiemap.rst
  - Documentation/filesystems/files.rst
  - Documentation/filesystems/fuse.rst
  - Documentation/filesystems/idmappings.rst
  - Documentation/filesystems/inotify.rst 和 dnotify 居然是两个
  - Documentation/filesystems/journalling.rst
  - Documentation/filesystems/directory-locking.rst
  - Documentation/filesystems/locking.rst
  - Documentation/filesystems/locks.rst
	- 这个是 flock ，不是我想象的东西
  - Documentation/filesystems/netfs_library.rst
  - Documentation/filesystems/path-lookup.rst
  - Documentation/filesystems/ramfs-rootfs-initramfs.rst

### Documentation/filesystems/porting.rst

> New helpers: sb_bread(), sb_getblk(), sb_find_get_block(), set_bh(),
> sb_set_blocksize() and sb_min_blocksize().

上来就推荐 buffer head 我是蚌埠住了，这都是

然后这里有在哪里说:
Documentation/filesystems/buffer.rst

> Linux uses buffer heads to maintain state about individual filesystem blocks.
> Buffer heads are deprecated and new filesystems should use iomap instead.

## 具体问题
### file_operations::write 和 file_operations::write_iter 什么关系
<!-- b68401ca-8ebc-4b75-ba75-718ef7e43df1 -->

Documentation/filesystems/vfs.rst
```rst
``write``
	called by write(2) and related system calls

``write_iter``
	possibly asynchronous write with iov_iter as source
```
write_iter 是现代推荐接口，支持异步 I/O 和向量操作，而 write 是传统的同步接口。内核通过 new_sy nc_write() 自动适配，使只实现 write_iter 的文件系统也能支持传统 write(2) 系统调用。

- new_sync_write : 同步 io 中调用 file_operations::write_iter
- aio_write : 在 aio 中调用
- io_write : 在 io uring 中调用

基本调用路线就是:
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


## struct iov_iter
<!-- c09481f0-5ed2-4fb4-b987-b7bf375b5008 -->

```c
struct iov_iter {
    u8 iter_type;           /* 类型: ITER_UBUF/ITER_IOVEC/ITER_KVEC/ITER_BVEC 等 */
    u8 data_source;         /* 数据来源标志 */
    bool nofault;           /* 是否禁止缺页异常 */
    bool user_backed;       /* 是否用户空间 backed */
    size_t iov_offset;      /* 当前 buffer 内的偏移 */
    size_t count;           /* 剩余总字节数 */
    union {
        const struct iovec *__iov;   /* 用户空间 iovec 数组 */
        const struct kvec *kvec;     /* 内核空间 kvec 数组 */
        const struct bio_vec *bvec;  /* bio vector (块层) */
        struct folio *folio;         /* folio 迭代器 */
        struct pipe_inode_info *pipe;/* pipe 迭代器 */
        ...
    } __ubuf;
};
```
## struct inode_operations
<!-- eb9ac7ee-3091-48c8-a6a0-cd915de6c6c9 -->

具体参考 Documentation/filesystems/vfs.rst 中 struct inode_operations 章节

### fiemap
```c
int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start, u64 len);
```

```txt
fiemap 是 struct inode_operations 中的一个回调函数，用于获取
文件在存储设备上的物理映射信息（file extent mapping）。

功能概述

它实现了 FIEMAP ioctl 的内核侧支持，允许用户空间查询：

• 文件数据在磁盘上的物理位置
• 文件是否存在空洞（sparse file holes）
• 数据块是否连续
• 文件的 extent 布局详情

典型用途

 场景            说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 文件碎片分析    filefrag 命令通过 fiemap 统计文件碎片数
 稀疏文件检测    cp --sparse=auto 利用 fiemap 保留空洞
 直接 I/O 优化   数据库/备份工具根据物理布局优化读取策略
 去重工具        通过 extent 信息识别重复数据块

用户空间接口

用户空间通过 ioctl(fd, FS_IOC_FIEMAP, &fiemap) 调用，内核会
遍历文件的 extent 并填充 struct fiemap_extent 数组返回。

如果文件系统未实现 fiemap 回调，则返回 -EOPNOTSUPP。主流文件
系统（ext4/xfs/btrfs）均实现了此接口。
```

### folio 状态
```c
const struct address_space_operations simplefs_iomap_aops = {
	.read_folio		= simplefs_read_folio,
	.readahead		= simplefs_readahead,
	.writepages		= simplefs_writepages,
	.dirty_folio		= iomap_dirty_folio,
	.release_folio		= iomap_release_folio,
	.invalidate_folio	= iomap_invalidate_folio,
	.migrate_folio		= filemap_migrate_folio,
	.is_partially_uptodate	= iomap_is_partially_uptodate,
	.error_remove_folio	= generic_error_remove_folio,
	.bmap			= simplefs_bmap,
};
```

才注意到 address_space_operations 中携带的了很多处理 folio 状态的内容

```rst
``dirty_folio``
	called by the VM to mark a folio as dirty.  This is particularly
	needed if an address space attaches private data to a folio, and
	that data needs to be updated when a folio is dirtied.  This is
	called, for example, when a memory mapped page gets modified.
	If defined, it should set the folio dirty flag, and the
	PAGECACHE_TAG_DIRTY search mark in i_pages.

``release_folio``
	release_folio is called on folios with private data to tell the
	filesystem that the folio is about to be freed.  ->release_folio
	should remove any private data from the folio and clear the
	private flag.  If release_folio() fails, it should return false.
	release_folio() is used in two distinct though related cases.
	The first is when the VM wants to free a clean folio with no
	active users.  If ->release_folio succeeds, the folio will be
	removed from the address_space and be freed.

	The second case is when a request has been made to invalidate
	some or all folios in an address_space.  This can happen
	through the fadvise(POSIX_FADV_DONTNEED) system call or by the
	filesystem explicitly requesting it as nfs and 9p do (when they
	believe the cache may be out of date with storage) by calling
	invalidate_inode_pages2().  If the filesystem makes such a call,
	and needs to be certain that all folios are invalidated, then
	its release_folio will need to ensure this.  Possibly it can
	clear the uptodate flag if it cannot free private data yet.

``free_folio``
	free_folio is called once the folio is no longer visible in the
	page cache in order to allow the cleanup of any private data.
	Since it may be called by the memory reclaimer, it should not
	assume that the original address_space mapping still exists, and
	it should not block.
```

简单来说，是为了注册:
是的，这三个回调的存在主要是为了处理 folio 的私有数据。 如果文件系统
不给 folio 附加私有数据（如 buffer_head 等），通常不需要注册这些回调，内核的通用代码可以处理标准的页缓存操作。

例如，当一个文件系统是 iomap 的时候，大致的效果是:
注册 iomap_dirty_folio 的根本原因就是：iomap 框架给 folio 附加了私 有数据 struct iomap_folio_state。

1. iomap 的私有数据结构

```txt
  struct iomap_folio_state {
      spinlock_t      state_lock;
      unsigned int    read_bytes_pending;
      atomic_t        write_bytes_pending;

      /*
       * 每个块有两个位：
       * - [0..blocks_per_folio): uptodate 状态
       * - [b_p_f...2*b_p_f):     dirty 状态
       */
      unsigned long   state[];  // 位图
  };
```
作用：当文件系统块大小小于 folio 大小时，需要细粒度跟踪每个块的状态。


例如，我们继续看看:
```c
  bool iomap_dirty_folio(struct address_space *mapping, struct folio *folio)
  {
      struct inode *inode = mapping->host;
      size_t len = folio_size(folio);

      ifs_alloc(inode, folio, 0);        // 1. 分配私有数据（如果需要）
      iomap_set_range_dirty(folio, 0, len);  // 2. 设置块级脏状态
      return filemap_dirty_folio(mapping, folio);  // 3. 通用标记
  }
```

注册 iomap_dirty_folio 的原因
        │
        ├── 1. iomap 框架使用 folio->private 存储 iomap_folio_state
        │
        ├── 2. 标记脏页时需要：
        │      - 分配私有数据（如果多于一页一块）
        │      - 更新块级脏状态位图
        │
        └── 3. 最后调用通用的 filemap_dirty_folio

类比：就像 ext4 需要 ext4_dirty_folio 处理 buffer_head 一样，iomap 需要
iomap_dirty_folio 处理 iomap_folio_state。这是细粒度块状态跟踪的需要。

从 iomap_dirty_folio 的地方，我再强调一次，folio 和 block 的粒度不同的:

 层级                               用途
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 folio 级别 (filemap_dirty_folio)   告诉 VM "这个 folio 需要写回"，管理页缓存生命周期
 块级别 (iomap_folio_state)         告诉文件系统 "folio 内哪些块实际需要写入磁盘"，优化 I/O

的确，iomap 主要都是处理和 mmap 相关的东西:
```txt
@[
        iomap_dirty_folio+5
        iomap_page_mkwrite+307
        __xfs_write_fault.constprop.0+285
        do_page_mkwrite+80
        do_wp_page+657
        __handle_mm_fault+1478
        handle_mm_fault+252
        do_user_addr_fault+537
        exc_page_fault+138
        asm_exc_page_fault+38
]: 343
```

当开始 writeback 的时候，在 iomap_writeback_folio 中来处理:
```txt
@[
        iomap_writeback_folio+5
        iomap_writepages+87
        xfs_vm_writepages+145
        do_writepages+211
        filemap_fdatawrite_wbc+81
        __filemap_fdatawrite_range+95
        file_write_and_wait_range+65
        xfs_file_fsync+80
        do_fsync+59
        __x64_sys_fsync+19
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 323
```

### [ ] bmap
1. FIBMAP ioctl (最常见)

int fd = open("/mnt/simplefs/file", O_RDONLY);
int block = 0;  // 逻辑块号
ioctl(fd, FIBMAP, &block);  // 返回物理块号到 block


## struct file_operations
<!-- 29747b20-ee70-4053-9c15-0c53002eb1dc -->

### 基本问题
4. iterate vs iterate_shared

目录遍历：

• 都用于 readdir() 系统调用，读取目录内容
• iterate_shared：读锁 (i_rwsem)，允许并发多个读取者
• iterate（旧）：写锁，独占访问（已废弃，新代码都用 iterate_shared）

5. fsync vs fasync

完全不同：

• fsync：同步文件数据到磁盘（fsync() 系统调用）
• fasync：异步信号通知（fcntl(fd, F_SETFL, FASYNC)），当文件可读/可写时发送 SIGIO 信号给进程

8. setfl

设置文件状态标志：

• 实现 fcntl(fd, F_SETFL, ...)
• 修改 file->f_flags（如 O_APPEND, O_NONBLOCK, O_ASYNC 等）
• 注意：不能修改 O_RDONLY/O_WRONLY/O_RDWR（这些通过 f_mode 控制）

## unix domain socket 又可以做网络，这个文件又可以拷贝
这算是一个 vfs 失败吧，创建了 unix domain socket ，然后拷贝到其他的路径。
这个需要各个 fs 的支持吗？

例如拷贝 /home/martins3/data/hack/vm/nix/s/vport.sock 到
```txt
🧀  cp vport.sock /tmp
cp: cannot open 'vport.sock' for reading: No such device or address
nix/s on  master [!?] 😋
🤒  mv vport.sock /tmp
```

## fiemap
为了支持 ioctl_fiemap，通过其可以获取到一个文件如何映射到的 disk 上的。

- xfs_vn_fiemap
  - iomap_fiemap
    - iomap_fiemap_iter
    - iomap_to_fiemap
      - fiemap_fill_next_extent

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
