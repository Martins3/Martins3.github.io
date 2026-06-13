# FUSE 机制梳理

本文基于原始笔记 `/home/martins3/data/vn/docs/kernel/fs/fuse/fuse.md` 重新整理。原始文件只作为问题来源读取，没有修改。

主要源码和材料依据：

- `/home/martins3/data/kernel/linux`，当前本地源码标识：`v7.0.11-1-g1d36469c943a`
- `Documentation/filesystems/fuse/fuse.rst`
- `Documentation/filesystems/fuse/fuse-io.rst`
- `Documentation/filesystems/fuse/fuse-io-uring.rst`
- `Documentation/filesystems/fuse/fuse-passthrough.rst`
- `include/uapi/linux/fuse.h`
- `fs/fuse/fuse_i.h`
- `fs/fuse/dev.c`
- `fs/fuse/dev_uring.c`
- `fs/fuse/dir.c`
- `fs/fuse/inode.c`
- `fs/fuse/file.c`
- `fs/fuse/iomode.c`
- `fs/fuse/passthrough.c`
- `fs/fuse/backing.c`
- `fs/fuse/virtio_fs.c`
- `fs/fuse/dax.c`
- libfuse `fuse_loop_mt.c` 文档源码：https://libfuse.github.io/doxygen/fuse__loop__mt_8c_source.html
- SSHFS release 说明：https://github.com/libfuse/sshfs/releases
- e2fsprogs / fuse2fs 仓库镜像：https://kernel.googlesource.com/pub/scm/fs/ext2/e2fsprogs/

## 1. FUSE 是什么

FUSE 是一个把 VFS 文件系统操作转发给用户态进程处理的内核/用户态协议框架。它不是单纯的库，也不是一个具体文件系统。完整系统通常包含：

- 内核模块和内核文件系统实现：`fuse.ko`，源码在 `fs/fuse/`。
- 通信设备：通常是 `/dev/fuse`。
- 用户态 daemon：真正提供元数据和数据，例如 sshfs、s3fs、rclone mount、mergerfs、ntfs-3g、lxcfs 等。
- 用户态库和挂载工具：常见为 libfuse 和 fusermount。

一个普通进程访问挂载点时仍然走标准 VFS：

```text
应用程序
  -> read/write/open/lookup/stat 等 syscall
  -> VFS
  -> FUSE 内核文件系统 fs/fuse/*
  -> FUSE 请求队列
  -> /dev/fuse read 或 io_uring CQE
  -> 用户态 daemon 处理请求
  -> /dev/fuse write 或 io_uring commit
  -> FUSE 内核唤醒原 syscall
```

所以 FUSE 的关键价值是：应用程序看到的是普通 POSIX 文件系统接口，具体文件系统逻辑却可以在用户态实现。它降低了文件系统开发、调试和部署成本，也允许非特权用户挂载自己的文件系统。代价是多一次内核/用户态往返、数据拷贝或页映射管理、调度开销，以及更复杂的缓存一致性问题。

## 2. FUSE 在内核中的定位

FUSE 内核代码一边接 VFS，一边接 daemon 通信通道：

- `inode.c`：superblock、mount、connection 初始化、`FUSE_INIT` 能力协商、inode 分配和属性更新。
- `dir.c`：lookup、create、mkdir、unlink、rename、getattr/statx、permission、dentry revalidate、目录缓存和反向 invalidation。
- `file.c`：open/release、read/write、mmap、writeback、direct I/O、folio/page cache 读写、splice、copy_file_range 等。
- `dev.c`：传统 `/dev/fuse` 通信通道，请求入队、daemon read 请求、daemon write reply、通知、FORGET、中断、后台请求限流。
- `dev_uring.c`：FUSE-over-io_uring 通信通道。
- `iomode.c`：同一个 inode 上 cached、uncached、passthrough、direct write 的互斥和切换。
- `passthrough.c` / `backing.c`：FUSE passthrough 到底层 backing file。
- `virtio_fs.c` / `dax.c`：virtiofs 和 FUSE DAX。

FUSE 的“文件系统逻辑”不是完全在用户态，也不是完全在内核态。更准确的拆分是：

- VFS 对象、dcache、icache、page cache、writeback 框架、权限检查入口、mmap 入口、请求队列和同步机制在内核。
- 文件名到 nodeid 的解析结果、inode 属性、目录项有效期、文件数据内容、具体后端访问逻辑主要由用户态 daemon 提供。
- 内核缓存 daemon 返回的结果，但缓存有效期、失效通知和一致性策略依赖 daemon 与协商能力。

## 3. FUSE 协议和核心对象

### 3.1 UAPI 协议

`include/uapi/linux/fuse.h` 定义了 FUSE 协议对象。所有普通请求都有 header：

- `struct fuse_in_header`：内核发给 daemon，包含 `len`、`opcode`、`unique`、`nodeid`、调用者 uid/gid/pid 等。
- `struct fuse_out_header`：daemon 回给内核，包含 `len`、`error`、`unique`。

`unique` 是单个连接里的请求 ID。daemon 写回 reply 时，内核用它在 processing 队列里找到原来的 `fuse_req`。

核心 opcode 包括：

- 元数据：`FUSE_LOOKUP`、`FUSE_FORGET`、`FUSE_GETATTR`、`FUSE_SETATTR`、`FUSE_STATX`
- 文件打开和关闭：`FUSE_OPEN`、`FUSE_RELEASE`、`FUSE_CREATE`
- 数据：`FUSE_READ`、`FUSE_WRITE`
- 目录：`FUSE_OPENDIR`、`FUSE_READDIR`、`FUSE_READDIRPLUS`、`FUSE_RELEASEDIR`
- 修改目录项：`FUSE_MKDIR`、`FUSE_MKNOD`、`FUSE_UNLINK`、`FUSE_RMDIR`、`FUSE_RENAME`、`FUSE_RENAME2`、`FUSE_LINK`
- 能力协商：`FUSE_INIT`
- 中断和通知：`FUSE_INTERRUPT`、`FUSE_NOTIFY_REPLY`
- DAX/virtiofs 相关：`FUSE_SETUPMAPPING`、`FUSE_REMOVEMAPPING`

### 3.2 `struct fuse_conn`

`struct fuse_conn` 是一个 daemon 连接。它不是一个单独文件，而是一条“内核 FUSE 实例”和“daemon 通道”的连接。

关键字段：

- `iq`：input queue，内核准备给 daemon 的 pending 请求。
- `devices`：连接上的 `struct fuse_dev` 列表。传统 `/dev/fuse` 模式下，daemon 通过这些 device 实例读写请求。
- `mounts`：一个连接可以被多个 mount/superblock 共享，尤其是 submount。
- `max_read`、`max_write`、`max_pages`：单请求读写尺寸和页数上限。
- `max_background`、`congestion_threshold`、`num_background`、`active_background`、`bg_queue`：后台请求限流。
- `initialized`、`blocked`、`connected`、`aborted`：连接状态。
- 能力位：`writeback_cache`、`async_read`、`async_dio`、`parallel_dirops`、`auto_inval_data`、`explicit_inval_data`、`do_readdirplus`、`direct_io_allow_mmap`、`passthrough`、`io_uring` 等。
- `ring`：启用 FUSE-over-io_uring 时的 ring 状态。
- `backing_files_map`：passthrough backing file 的 IDR。

### 3.3 `struct fuse_mount`

`struct fuse_mount` 绑定 `fuse_conn` 和 superblock。一个 connection 可以有多个 `fuse_mount`，这允许 submount 共享 daemon 连接，但有各自的 superblock 和设备号。

### 3.4 `struct fuse_inode`

`struct fuse_inode` 包含内核 `struct inode`，并附带 FUSE 私有状态：

- `nodeid`：daemon 和内核共同使用的 inode ID。FUSE_ROOT_ID 是根。
- `nlookup`：lookup 引用计数，回收时内核通过 `FORGET` 告诉 daemon 减引用。
- `i_time`：属性缓存有效期。
- `inval_mask`：哪些属性无效，需要重新 getattr/statx。
- `attr_version`：属性版本，用于避免旧 reply 覆盖新状态。
- `write_files`、`queued_writes`、`writectr`：writeback 相关。
- `iocachectr`：同一 inode 上 cached/uncached IO 模式的互斥计数。
- `rdc`：目录 readdir cache。
- `dax`、`fb`：DAX 和 passthrough backing 状态。

`fuse_iget()` 用 `iget5_locked()` 以 `nodeid` 查找或创建内核 inode，然后用 daemon 返回的 `fuse_attr` 填充属性。也就是说，内核维护 icache，但 inode 的权威元数据来自 daemon。

### 3.5 `struct fuse_file`

`struct fuse_file` 是内核里每个打开文件的 FUSE 私有状态：

- `fh`：daemon 返回的 file handle，后续 READ/WRITE/GETATTR 可携带它。
- `kh`：内核分配的唯一 file handle。
- `nodeid`：对应 inode。
- `open_flags`：daemon 在 `FUSE_OPEN` reply 中返回的 `FOPEN_*`。
- `iomode`：cached 或 uncached 模式引用。
- `passthrough`、`cred`：passthrough 打开 backing file 后保存的对象。

关键点：`FOPEN_DIRECT_IO`、`FOPEN_KEEP_CACHE`、`FOPEN_PASSTHROUGH` 是按 open file 生效，不是全局 inode 属性。但 `iomode.c` 会限制同一 inode 上 cached 和 uncached/passthrough 混用，避免 page cache 和直接后端访问并发破坏一致性。

### 3.6 `struct fuse_args`、`struct fuse_args_pages`、`struct fuse_req`

`struct fuse_args` 描述一次 FUSE 请求的 opcode、nodeid、输入参数、输出参数和页参数方向。`struct fuse_args_pages` 在 `fuse_args` 外增加 folio 数组和每个 folio 的 offset/length。

`struct fuse_req` 是真正排队的请求对象：

- `in.h` / `out.h`：请求和回复 header。
- `args`：请求 payload 描述。
- `waitq`：同步请求等待 reply。
- `flags`：`FR_WAITING`、`FR_PENDING`、`FR_SENT`、`FR_BACKGROUND`、`FR_INTERRUPTED`、`FR_ASYNC`、`FR_URING` 等。
- `ring_entry` / `ring_queue`：io_uring 路径使用。

## 4. 请求生命周期

### 4.1 传统 `/dev/fuse` 同步请求

以 `LOOKUP`、`GETATTR`、同步 `READ` 等为例，调用路径是：

```text
VFS 操作
  -> fs/fuse/dir.c 或 file.c 构造 fuse_args
  -> fuse_simple_request()
  -> __fuse_simple_request()
  -> fuse_get_req()
  -> fuse_args_to_req()
  -> __fuse_request_send()
  -> fuse_send_one()
  -> fiq->ops->send_req()，传统路径为 fuse_dev_queue_req()
  -> request_wait_answer() 睡眠等待
```

`fuse_dev_queue_req()` 做三件事：

- 分配 `unique`。
- 把请求挂到 `fc->iq.pending`。
- 唤醒等待 `/dev/fuse` 可读的 daemon 线程。

daemon 调用 `read(/dev/fuse)` 时进入：

```text
fuse_dev_read()
  -> fuse_dev_do_read()
  -> 从 fiq->pending 取请求
  -> 拷贝 fuse_in_header 和输入参数到 daemon buffer
  -> 如果需要 reply，把请求移到 fuse_pqueue.processing[hash(unique)]
  -> 设置 FR_SENT
```

daemon 处理后调用 `write(/dev/fuse)`：

```text
fuse_dev_write()
  -> fuse_dev_do_write()
  -> 读取 fuse_out_header
  -> 按 unique 在 processing hash 表找到 fuse_req
  -> 拷贝输出参数到 req->args
  -> fuse_request_end()
  -> 唤醒原 syscall 线程
```

所以，FUSE 请求不是“系统调用直接调用用户态函数”。中间有明确的队列、request ID、copy state、processing hash、等待队列和中断处理。

### 4.2 后台请求和限流

readahead、writeback、release 等可能走 background 路径：

```text
fuse_simple_background()
  -> fuse_get_req(..., for_background=true)
  -> fuse_request_queue_background()
  -> bg_queue / active_background / num_background
```

`max_background` 限制后台请求总量。达到上限后 `fc->blocked = 1`，后续 background 请求会等待。`congestion_threshold` 达到后，内核会跳过一些异步 readahead，或者延迟非同步 writeback，避免 daemon 堵塞时继续放大积压。

`/sys/fs/fuse/connections/<id>/waiting`、`max_background`、`congestion_threshold`、`abort` 来自 fusectl，是生产上判断 FUSE 是否 hang 住和强制 abort 的重要入口。

### 4.3 信号中断

`request_wait_answer()` 对中断的处理和文档一致：

- 如果请求还没发给 userspace，fatal signal 可以把 pending 请求移除并返回。
- 如果请求已发给 daemon，内核会排一个 `FUSE_INTERRUPT` 请求。
- daemon 可以忽略 interrupt，也可以给原请求返回 `-EINTR`。
- 如果 interrupt 和原请求完成竞争，daemon 可对 interrupt 返回 `-EAGAIN`，内核可能重排。

这解释了为什么杀掉访问 FUSE 的进程不一定立刻终止底层 daemon 正在处理的操作：请求是否已经进入 userspace 决定了后续行为。

## 5. mount 和初始化

传统 mount 时，用户态一般先打开 `/dev/fuse`，再把 fd 通过 mount option `fd=N` 传给内核。内核建立 `fuse_conn` 后发送 `FUSE_INIT`。

`fuse_new_init()` 构造 `FUSE_INIT`，内核声明自己支持的能力，例如：

- `FUSE_ASYNC_READ`
- `FUSE_BIG_WRITES`
- `FUSE_WRITEBACK_CACHE`
- `FUSE_PARALLEL_DIROPS`
- `FUSE_ASYNC_DIO`
- `FUSE_DIRECT_IO_ALLOW_MMAP`
- `FUSE_PASSTHROUGH`
- `FUSE_OVER_IO_URING`
- `FUSE_REQUEST_TIMEOUT`

daemon 的 `FUSE_INIT` reply 决定实际启用哪些能力。`process_init_reply()` 会设置 `fc->writeback_cache`、`fc->async_dio`、`fc->parallel_dirops`、`fc->passthrough`、`fc->io_uring` 等字段，并确定 `max_write`、`max_pages`、readahead 页数、后台请求限制。

需要注意：

- `FUSE_PASSTHROUGH` 和 `FUSE_WRITEBACK_CACHE` 在当前源码里不能同时启用。`process_init_reply()` 明确拒绝 passthrough + writeback-cache 的组合。
- `FUSE_OVER_IO_URING` 还要求模块参数 `enable_uring` 打开；`fuse_uring_enabled()` 返回 true 后内核才在 INIT 中声明或接受这个能力。

## 6. dcache 和 icache 到底谁维护

答案是：内核和 daemon 都维护，但分工不同。

### 6.1 dcache

内核使用普通 VFS dentry。FUSE 给 dentry 挂了 `struct fuse_dentry`：

- `dentry->d_fsdata` 指向 `struct fuse_dentry`。
- `fuse_dentry.time` 保存目录项缓存过期时间。
- `dentry->d_time` 保存 connection epoch，用于全局 epoch invalidation。

`FUSE_LOOKUP` reply 的 `struct fuse_entry_out` 包含：

- `nodeid`
- `generation`
- `entry_valid` / `entry_valid_nsec`
- `attr_valid` / `attr_valid_nsec`
- `attr`

其中 `entry_valid` 控制 dentry 名字缓存，`attr_valid` 控制 inode 属性缓存。

`fuse_lookup()` 发送 `FUSE_LOOKUP`，拿到 reply 后：

- `fuse_iget()` 创建或查找 inode。
- `d_splice_alias()` 把 inode 和 dentry 关联。
- `fuse_change_entry_timeout()` 设置 dentry 过期时间。

`fuse_dentry_revalidate()` 在 dentry 过期、`LOOKUP_REVAL`、`LOOKUP_EXCL` 等场景重新发 `FUSE_LOOKUP`。如果新 lookup 返回不同 nodeid，内核让 VFS invalid dentry 并重查。

daemon 也可以主动写 notification：

- `FUSE_NOTIFY_INVAL_ENTRY`：让内核 invalid 某个 dentry。
- `FUSE_NOTIFY_DELETE`：删除 dentry 并检查 child nodeid。
- `FUSE_NOTIFY_INC_EPOCH`：增加 connection epoch，使旧 dentry 失效。
- `FUSE_NOTIFY_PRUNE`：按 nodeid prune inode alias。

### 6.2 icache

内核维护 `struct inode` 和 inode hash。FUSE 用 `nodeid` 作为 inode 查找 key：

```text
fuse_lookup_name()
  -> FUSE_LOOKUP
  -> fuse_iget(sb, nodeid, generation, attr, ...)
  -> iget5_locked(sb, nodeid, ...)
```

daemon 维护 nodeid 的生命周期。内核每 lookup 成功一次会增加 `fi->nlookup`。inode 回收或 dentry 释放到一定程度后，内核发送 `FUSE_FORGET` 或 `FUSE_BATCH_FORGET`，告诉 daemon 对应 nodeid 的 lookup 引用减少。

这就是 FUSE 中 `LOOKUP/FORGET` 的核心语义：它不是普通路径查询而已，还承担 daemon 端对象生命周期引用计数。

### 6.3 属性缓存

属性缓存在 `fuse_inode`：

- `fi->i_time`：属性有效期。
- `fi->inval_mask`：哪些 statx 字段已经 invalid。
- `fi->attr_version`：防止旧请求覆盖新状态。

`fuse_update_get_attr()` 会根据 `i_time`、`inval_mask`、statx sync flag 决定是否发送 `FUSE_GETATTR` 或 `FUSE_STATX`。如果属性有效，就用 `generic_fillattr()` 从内核 inode 填 stat。

writeback-cache 开启时，内核更信任本地的 size/mtime/ctime。`fuse_get_cache_mask()` 对 regular file 返回 `STATX_MTIME | STATX_CTIME | STATX_SIZE`，`fuse_change_attributes_i()` 会避免 daemon 返回的旧 size/mtime/ctime 覆盖本地 dirty page cache 状态。

## 7. FUSE I/O 模式

内核文档把 FUSE I/O 分成三类：

- direct-io
- cached + write-through
- cached + writeback-cache

当前源码还增加了 passthrough、DAX、io_uring 通道等优化，但基础分类仍然成立。

### 7.1 cached read

普通 read 路径：

```text
fuse_file_read_iter()
  -> fuse_cache_read_iter()
  -> generic_file_read_iter()
  -> page cache miss
  -> fuse_read_folio() / fuse_readahead()
  -> fuse_do_readfolio() 或 fuse_send_readpages()
  -> FUSE_READ
```

现代源码里 FUSE cached read 使用 iomap 的 folio 辅助函数：

- `iomap_read_folio()`
- `iomap_readahead()`
- `iomap_finish_folio_read()`

但这里的 `fuse_iomap_begin()` 只是返回 `IOMAP_MAPPED`、offset 和 length。它没有把文件 offset 映射到块设备 sector，也没有构造 bio。真正的数据来源仍然是 `FUSE_READ` 请求。

### 7.2 cached write-through

默认 cached write 是 write-through。路径大致为：

```text
fuse_file_write_iter()
  -> fuse_cache_write_iter()
  -> fuse_perform_write()
  -> fuse_fill_write_pages()
      把用户 iov_iter 数据 copy 到 page cache folio
  -> fuse_send_write_pages()
      构造 FUSE_WRITE，携带 folio 作为 in_pages
  -> daemon reply 写入长度
```

这说明 write-through 不是直接把用户 buffer 给 daemon。内核先把数据复制到 page cache folio，再把这些 folio 作为 FUSE 请求 payload 发给 daemon。

部分页写入时，如果 folio 还不是 uptodate，FUSE 可能保持 folio locked 并等待 daemon 写入结果；如果短写，则清掉 uptodate 防止缓存错误数据。

### 7.3 writeback-cache

如果 INIT reply 启用 `FUSE_WRITEBACK_CACHE`：

- `write(2)` 通常只写入内核 page cache 并标脏，很快返回。
- 后续 background writeback、memory reclaim、`fsync(2)`、`close(2)`、`munmap(2)` 等触发 `FUSE_WRITE`。
- partial page write 可能需要先从 daemon `READ` 原页面，因为内核要保留未覆盖字节。

源码中 writeback 路径：

```text
fuse_cache_write_iter()
  -> iomap_file_buffered_write()
  -> iomap_dirty_folio()
  -> fuse_writepages()
  -> iomap_writepages()
  -> fuse_iomap_writeback_range()
  -> fuse_writepages_send()
  -> fuse_send_writepage()
  -> fuse_simple_background()
```

writeback-cache 假设所有文件修改都经过本 FUSE 内核客户端，因此一般不适合没有强一致控制的网络文件系统。如果后端也会被其他客户端修改，daemon 必须非常谨慎地使用 invalidation 或放弃 writeback-cache。

### 7.4 direct-io

daemon 在 `FUSE_OPEN` reply 中设置 `FOPEN_DIRECT_IO` 后，该 open file 的 read/write 绕过 page cache：

```text
fuse_file_read_iter()
  -> ff->open_flags & FOPEN_DIRECT_IO
  -> fuse_direct_read_iter()
  -> fuse_direct_io()
  -> fuse_get_user_pages()
  -> FUSE_READ

fuse_file_write_iter()
  -> ff->open_flags & FOPEN_DIRECT_IO
  -> fuse_direct_write_iter()
  -> fuse_direct_io(..., FUSE_DIO_WRITE)
  -> fuse_get_user_pages()
  -> FUSE_WRITE
```

direct-io 的重点是绕过内核 page cache，不是保证零拷贝。

`fuse_get_user_pages()` 对用户 `iov_iter` 调用 `iov_iter_extract_pages()`，把用户页提取为 folio/page 描述，设置：

- read：`args.out_pages = true`
- write：`args.in_pages = true`
- `args.user_pages = true`
- `args.is_pinned = iov_iter_extract_will_pin(ii)`

之后 `/dev/fuse` 路径里的 `fuse_copy_folio()` 仍然会在内核和 daemon buffer 之间拷贝或通过 splice/pipe 做有限优化。daemon 不能拿到应用进程的 page 指针后直接写内核 page。daemon 是普通用户态进程，它看到的是 `/dev/fuse` read/write 或 io_uring buffer。

direct-io 还要处理 page cache 冲突：

- direct read/write 前后会等待或 invalidate 对应 page cache range。
- direct write 默认需要 inode exclusive lock。
- 如果 daemon 返回 `FOPEN_PARALLEL_DIRECT_WRITES`，并且不是 append、不是越 EOF、没有 cached IO 并发，内核允许并行 direct write。

### 7.5 passthrough

FUSE passthrough 是新的性能优化：daemon 告诉内核某个 FUSE 文件其实可以直接映射到一个 backing file，读写绕过 daemon。

启用条件：

- 内核打开 `CONFIG_FUSE_PASSTHROUGH`。
- INIT 协商 `FUSE_PASSTHROUGH`。
- daemon 有 `CAP_SYS_ADMIN`。
- daemon 用 `FUSE_DEV_IOC_BACKING_OPEN` 注册 backing fd，得到 `backing_id`。
- daemon 在 `OPEN/CREATE` reply 中设置 `FOPEN_PASSTHROUGH` 并填 `backing_id`。

读写路径：

```text
fuse_file_read_iter()
  -> FOPEN_DIRECT_IO 优先
  -> fuse_file_passthrough(ff)
  -> fuse_passthrough_read_iter()
  -> backing_file_read_iter()

fuse_file_write_iter()
  -> FOPEN_DIRECT_IO 优先
  -> fuse_file_passthrough(ff)
  -> fuse_passthrough_write_iter()
  -> backing_file_write_iter()
```

passthrough 适合 overlay、加密、元数据由 daemon 管理但大文件数据实际在本地 backing filesystem 的场景。它不适合任意后端，例如 S3、SSH、远端 KV store，因为这些后端没有可注册给内核直接读写的本地 `struct file`。

当前实现限制：

- passthrough 需要 `CAP_SYS_ADMIN`，因为 backing fd 被内核持有后可能绕过普通进程 fd 可见性和 `RLIMIT_NOFILE` 统计。
- backing file 必须是 regular file。
- 检查 filesystem stack depth，避免 stacking loop。
- 不允许和 writeback-cache 同时启用。
- `FOPEN_DIRECT_IO` 优先级高于 `FOPEN_PASSTHROUGH`。如果二者同时存在，read/write 仍走 FUSE direct IO；但源码注释说明 mmap 可以使用 backing file。

### 7.6 DAX 和 virtiofs

virtiofs 放在 `fs/fuse` 下不是偶然。virtiofs 复用 FUSE 协议、inode/file/dir 逻辑和很多请求结构，只是通信通道从 `/dev/fuse` 换成 virtqueue。

virtiofs 的 daemon 通常在 host 侧，例如 `virtiofsd`。guest 内核里的 `virtio_fs.c` 把 FUSE 请求打包进 virtqueue。对 guest 来说，server 不在 guest 本机进程里；对整体系统来说，它仍是一个处理 FUSE 协议的用户态/host 侧服务。

DAX 路径由 `fs/fuse/dax.c` 实现。核心是：

- guest 有一段 DAX window。
- 内核需要读写某个文件 offset 时，请求 daemon 建立 file offset 到 DAX window offset 的映射。
- 协议 opcode 是 `FUSE_SETUPMAPPING` 和 `FUSE_REMOVEMAPPING`。
- `struct fuse_dax_mapping` 记录 inode、file offset range、window offset、长度、可写性。

DAX 不是普通 FUSE 的默认行为。它主要服务 virtiofs 这类本地虚拟化共享目录性能场景。

## 8. `/dev/fuse` 数据拷贝细节

`dev.c` 的 `fuse_copy_state` 负责在 daemon buffer 和 request 参数之间搬数据：

- `fuse_copy_one()`：拷贝普通参数。
- `fuse_copy_folios()`：拷贝 page/folio 参数。
- `fuse_copy_folio()`：处理单个 folio。
- `fuse_try_move_folio()`：在 splice 场景尝试把 pipe page 移入 page cache，避免一次拷贝。
- `fuse_ref_folio()`：splice read 时把 folio 引用放入 pipe。

传统 read/write `/dev/fuse` 的语义决定了 daemon 最终读写的是自己的用户态 buffer。FUSE 可以通过 folio、pipe、splice、iov_iter 等减少中间开销，但不能把“用户进程 read 的目标 page”直接交给 daemon 任意写。内核必须控制页生命周期、权限、dirty 标记、cache 一致性和异常路径。

## 9. FUSE-over-io_uring

本地源码包含 `fs/fuse/dev_uring.c` 和文档 `Documentation/filesystems/fuse/fuse-io-uring.rst`。当前设计目标是优化 kernel/userspace 通信通道，而不是改变 FUSE 协议本质。

传统通道：

```text
daemon read(/dev/fuse) 获取请求
daemon write(/dev/fuse) 提交 reply
```

io_uring 通道：

```text
daemon 提交 IORING_OP_URING_CMD / FUSE_IO_URING_CMD_REGISTER
  -> 注册一个 header buffer + payload buffer 的 ring entry
  -> 每个 CPU queue 至少有 entry 后 ring ready

内核有 FUSE 请求
  -> fuse_uring_queue_fuse_req()
  -> 找可用 ring entry
  -> fuse_uring_copy_to_ring()
  -> io_uring_cmd_done() 让 daemon 收到 CQE

daemon 处理后
  -> IORING_OP_URING_CMD / FUSE_IO_URING_CMD_COMMIT_AND_FETCH
  -> fuse_uring_commit()
  -> fuse_uring_copy_from_ring()
  -> fuse_request_end()
  -> 同一个 entry 立刻 fetch 下一个请求
```

关键对象：

- `struct fuse_ring`：一个 connection 的 uring 总状态。
- `struct fuse_ring_queue`：每 CPU 一个队列。
- `struct fuse_ring_ent`：daemon 注册的 header/payload buffer 和对应 io_uring cmd。

它的收益主要来自：

- 减少 read/write syscall 往返。
- daemon 可以通过 CQE 收请求，通过 SQE 提交 reply 并立刻 fetch 下一个。
- 每 CPU queue 降低共享队列竞争。
- 更适合高并发 daemon event loop。

但它没有自动解决所有零拷贝问题。当前源码仍用 `copy_to_user()` 写 header，用 `import_ubuf()` + `fuse_copy_args()` 在 ring payload 和请求参数之间搬运 payload。文档还明确说当前并非所有请求类型都支持 io_uring，notifications 和 interrupts 仍需要 `/dev/fuse` 路径处理。

原笔记提到 “support io-uring registered buffers” 的 patch。从当前本地源码看，FUSE-over-io_uring 已经有 daemon 注册 header/payload buffer 的模型，但没有看到独立的、类似 io_uring registered buffer 固定缓冲池的 FUSE UAPI opcode；需要按对应 patch 版本单独确认。

## 10. FUSE 和 iomap

当前源码中 FUSE 已经使用 iomap 框架，但用途要分清。

`file.c` 中：

- `fuse_iomap_begin()` 只填 `IOMAP_MAPPED`、offset、length。
- cached read 使用 `iomap_read_folio()` / `iomap_readahead()`。
- writeback-cache 使用 `iomap_file_buffered_write()`、`iomap_dirty_folio()`、`iomap_writepages()`。
- `address_space_operations` 中的 `dirty_folio`、`release_folio`、`invalidate_folio`、`is_partially_uptodate` 也使用 iomap helper。

这说明 iomap 在这里主要是 folio/page-cache 管理框架，帮助 FUSE 适配大 folio、部分 uptodate、dirty tracking、writeback 聚合。它不意味着 FUSE 现在有 block mapping，也不意味着 FUSE 请求变成 bio。

原笔记提到 Wong 系列中的 `FUSE_IOMAP_BEGIN` / `FUSE_IOMAP_END`。在当前本地 `include/uapi/linux/fuse.h` 的 `enum fuse_opcode` 中没有这两个 opcode。因此它至少不在这份源码的 FUSE UAPI 中。这个方向如果合入，价值主要在 daemon 能把文件 offset 映射到底层本地文件或块设备 extent 的场景；如果后端是 S3、SSH、Ceph 用户态协议、数据库、日历 API 等没有本地 extent 语义的服务，收益会小很多。

## 11. 是否需要构造 bio

普通 FUSE I/O 不构造 bio。

bio 是块层 I/O 描述，用于文件系统把逻辑块映射到底层块设备后提交给 block layer。FUSE 的普通读写是协议请求：

```text
FUSE_READ / FUSE_WRITE
  -> fuse_args / fuse_args_pages
  -> /dev/fuse 或 io_uring
  -> daemon 自己决定从哪里取数据或写到哪里
```

daemon 的后端如果是一个本地文件，它自己调用 `pread/pwrite`，最终那个本地文件系统可能构造 bio。daemon 的后端如果是网络对象存储，就可能是 TCP/HTTP 请求。FUSE 内核层不关心这些后端细节。

例外和相关场景：

- `fuseblk` 是 FUSE 的 block-device-based mount 类型，但 FUSE 仍通过 daemon 协议交互。
- virtiofs 通过 virtqueue，不是 block bio。
- DAX/passthrough 会绕过部分普通 FUSE 数据路径，但也不是普通 FUSE 自己构造 bio。
- passthrough 到底层文件时，底层文件系统可能在自己的实现里走 iomap/bio。

## 12. 多线程 daemon 如何工作

内核侧天然支持多个 daemon 线程处理同一个连接：

- 多个线程可以同时阻塞在 `/dev/fuse` read。
- 请求在 `fc->iq.pending` 上排队。
- 一个 daemon 线程读走请求后，请求移到该 `fuse_dev` 的 `processing` hash 表。
- 另一个线程可以继续读下一个请求。
- reply 用 `unique` 匹配原请求，所以完成顺序可以不同于发出顺序。

libfuse 的 `fuse_session_loop_mt_31()` / `fuse_session_loop_mt_312()` 是用户态线程池封装。官方 doxygen 源码显示它维护 worker list、`max_threads`、`max_idle_threads`、`clone_fd` 等配置，并创建 worker 处理请求。`clone_fd` 允许 worker 使用 cloned session fd，减少某些共享 fd 竞争。

daemon 需要自己保证文件系统语义的并发安全。例如同一个目录下 rename/unlink/create 的顺序，后端元数据锁，file handle 生命周期，缓存 invalidation，都不是内核能完全替 daemon 解决的。内核只保证 FUSE 请求匹配和 VFS 层必要的锁；具体后端一致性由 daemon 实现。

`FUSE_PARALLEL_DIROPS` 会影响目录操作并发。未启用时，`fuse_lock_inode()` 用 `fuse_inode.mutex` 串行 lookup 和 readdir，保持老行为。启用后 daemon 表示自己能处理并行目录操作。

## 13. 权限和非特权挂载

FUSE 重要特点之一是非特权挂载。内核文档强调这不是 `/etc/fstab user` 的同一概念，而是普通用户通过 setuid fusermount 建立自己的 FUSE mount。

安全原则：

- 非特权 mount 默认只能 mount owner 访问。
- `allow_other` 才允许其他用户访问，且通常需要配置允许。
- `default_permissions` 决定内核是否按 inode mode 做权限检查。没有它时，daemon 自己负责权限策略。
- fusermount 对非特权 mount 通常加 `nosuid,nodev`，防止用户通过 FUSE 创建设备文件或 suid 程序提权。

`fuse_allow_current_process()`、`default_permissions`、`allow_other`、uid/gid 映射等共同控制访问。请求 header 中也带调用者 uid/gid/pid，daemon 可以据此实现自己的权限。

passthrough 更敏感，因此当前要求 `CAP_SYS_ADMIN`。原因包括 backing fd 被内核长期持有后的可见性、资源计数、stacking loop 和 shutdown deadlock 风险。

## 14. 典型项目和生态定位

FUSE 适合几类场景：

- 协议桥接：sshfs、curlftpfs、webdavfs、rclone mount、s3fs、gcsfuse、blobfuse。
- 本地文件变换：encfs、gocryptfs、mergerfs、unionfs-fuse、fuse-overlayfs、squashfuse、ratarmount、archivemount。
- 非 Linux 原生文件系统：ntfs-3g、exfat-fuse、CephFS FUSE 客户端、fuse2fs。
- 虚拟/伪文件系统：lxcfs、proc/sysfs 虚拟化、数据库或 API 映射成文件树。
- 镜像/容器/分发：nydus、Alluxio POSIX API、libnbd 的 nbdfuse。

FUSE 的商业化空间通常来自两点：

- 以 POSIX 文件系统接口接入非 POSIX 后端，降低应用改造成本。
- 把复杂策略放在用户态，方便快速迭代、跨内核部署、故障隔离和观测。

它不适合所有场景。高吞吐、低延迟、强一致、多客户端共享 page cache、数据库 WAL、块设备语义等场景，需要谨慎评估。NFS、SMB、CephFS kernel client、iSCSI/NVMe-oF、9p、virtiofs、原生内核文件系统可能更合适。

## 15. 原始笔记问题整理

### 15.1 FUSE I/O 需要构造 bio 吗

不需要，普通 FUSE 层不构造 bio。FUSE read/write 构造的是 `FUSE_READ/FUSE_WRITE` 协议请求，payload 可以是普通参数或 folio/page 数组。后端如果最终写本地磁盘，那是 daemon 或 passthrough 的 backing filesystem 走自己的块层路径。

### 15.2 `FOPEN_DIRECT_IO` 时数据会拷贝吗

会有数据搬运或页提取，不等于零拷贝。

`FOPEN_DIRECT_IO` 绕过 FUSE inode 的 page cache。read/write 走 `fuse_direct_io()`，它用 `iov_iter_extract_pages()` 从调用者 iov_iter 提取用户页，构造 `fuse_args_pages`。但传统 `/dev/fuse` daemon 最终仍通过 read/write 设备 fd 读写自己的 buffer，`fuse_copy_folio()` 会在 daemon buffer 和 folio 之间复制，splice 场景有有限 page move/ref 优化。

所以 direct-io 的主要语义是 bypass page cache、避免 readahead/cache 污染和缓存一致性问题，不是 daemon 直接得到应用进程页指针。

### 15.3 read 时用户态 daemon 能直接往内核提供的 page 写数据吗

不能按“拿到 page 指针直接写”的方式理解。

cached read 时，内核 page cache miss 后分配/锁定 folio，然后发 `FUSE_READ`。daemon 写 reply 到 `/dev/fuse` 或 io_uring payload buffer。内核再通过 `fuse_copy_out_args()` / `fuse_copy_folio()` 把 reply payload 填到 folio，最后 `iomap_finish_folio_read()` 标记 uptodate 并解锁。

daemon 是用户态进程，没有内核 page 指针。它只能通过 FUSE 协议通道提交数据。

### 15.4 “似乎所有操作都需要转发一下”是否正确

大体正确，但有缓存和 fast path。

需要转发的典型操作：

- lookup 过期或不存在时转发 `FUSE_LOOKUP`。
- getattr/statx 过期或强同步时转发。
- open 如果 daemon 实现了 open，则转发 `FUSE_OPEN`。
- page cache miss 的 read 转发 `FUSE_READ`。
- write-through 写转发 `FUSE_WRITE`。
- writeback-cache 在 writeback/fsync/close/munmap 时转发 `FUSE_WRITE`。

不一定转发的情况：

- dentry 在 `entry_valid` 内有效，lookup 可命中 dcache。
- attr 在 `attr_valid` 内有效，stat 可用 cached inode 属性。
- cached read 命中 page cache。
- daemon 对某些 op 返回 `-ENOSYS` 后，内核设置 `no_open/no_fsync/no_flush/no_access` 等，后续走简化路径。
- passthrough read/write/mmap 可以直接打到底层 backing file。
- direct-io 虽然绕过 page cache，但仍转发 `FUSE_READ/FUSE_WRITE`，除非 passthrough/DAX 等特殊路径。

### 15.5 FUSE 的 dcache 和 icache 是用户态维护吗

不是。内核维护 dcache 和 icache，用户态维护与之对应的权威命名和元数据状态。

内核缓存对象：

- dentry：`d_fsdata` 保存 FUSE entry timeout。
- inode：`fuse_inode` 保存 nodeid、attr timeout、nlookup、page cache 状态。
- page cache：regular file 数据缓存。

daemon 控制：

- lookup 返回哪个 nodeid。
- nodeid/generation/attr 是否有效。
- `entry_valid` 和 `attr_valid` 多久。
- 何时通过 notify invalidation 主动清缓存。
- FORGET 后释放自己的 nodeid 资源。

### 15.6 FUSE server 必须在本机吗

传统 FUSE mount 的 daemon 必须是挂载命名空间中打开 `/dev/fuse` 并参与 mount 的本机进程。它可以再连接远端服务，例如 sshfs 连 SSH/SFTP，s3fs 连 S3，rclone 连云存储。

从应用和内核视角看：

```text
应用 -> 本机 FUSE 内核 -> 本机 daemon -> 远端服务
```

virtiofs 是特殊情况。guest 内核使用 virtio 设备发送 FUSE 请求，host 侧 virtiofsd 处理请求。server 不在 guest 本机进程里，但仍是 FUSE 协议服务端。

### 15.7 virtiofs 为什么放在 `fs/fuse` 下

因为 virtiofs 是 FUSE 协议的传输和共享内存扩展，而不是一个完全独立的文件系统实现。它复用：

- FUSE opcode 和 UAPI 结构。
- FUSE inode/file/dir 逻辑。
- FUSE request 生命周期。
- DAX mapping opcode。

差异在通道：

- 传统 FUSE：`/dev/fuse` read/write。
- FUSE-over-io_uring：`IORING_OP_URING_CMD`。
- virtiofs：virtqueue。

### 15.8 io_uring 对 FUSE 的收益是什么，能解决 zero-copy 吗

收益主要是通信通道效率，不是彻底 zero-copy。

io_uring 路径避免 daemon 反复 read/write `/dev/fuse`，改成注册 ring entry，内核通过 CQE 通知 daemon 有请求，daemon 用 commit-and-fetch 提交 reply 并取下一个请求。这样能减少 syscall、减少唤醒开销、改善多队列并发。

但当前源码仍有：

- header 的 `copy_to_user()` / `copy_from_user()`。
- payload buffer 通过 `import_ubuf()` 和 `fuse_copy_args()` 搬运。
- notifications 和 interrupts 仍未完全走 io_uring，需要传统 `/dev/fuse`。

所以它不能被简单理解为“解决 FUSE zero-copy”。

### 15.9 FUSE 使用 iomap 是否多余，既然已经有 opcode

不多余，但当前用途不是替代 FUSE opcode。

FUSE opcode 描述的是内核和 daemon 的协议操作，例如 `READ/WRITE/LOOKUP`。iomap 是内核内部 page-cache/folio/writeback helper 框架。当前 FUSE 用 iomap 简化：

- folio read 完成处理。
- readahead。
- buffered write。
- dirty/writeback。
- large folio 的部分 uptodate 和 dirty tracking。

如果未来出现真正的 `FUSE_IOMAP_BEGIN/END`，那是另一层优化：让 daemon 把文件 offset 映射到底层 extent，然后内核可能绕过一部分 daemon 数据路径。这个方向只对本地 extent/backing-file 场景特别有意义。

### 15.10 用户态 ext4 和内核 ext4 的主要区别

用户态实现 ext4 是可行的，e2fsprogs 仓库里就有 `fuse2fs` 相关代码和近期提交。区别主要不是“能不能解析 ext4 格式”，而是运行环境和职责不同：

- 内核 ext4 深度集成 VFS、page cache、writeback、block layer、iomap、buffer head、jbd2 journal、quota、DAX、fscrypt、fsverity、错误处理和 mount 生命周期。
- 用户态 ext4/FUSE 实现需要通过普通文件或块设备接口访问底层镜像，再通过 FUSE 把文件系统暴露给 VFS。它必须自己处理 ext4 元数据一致性、分配器、目录索引、extent、权限和崩溃恢复边界。
- journal 不能忽略。只读或维护工具可以简化；可写 mount 若要可靠，必须处理 journal replay、事务一致性和崩溃恢复语义，否则容易损坏文件系统。
- 性能上，用户态 ext4 多一层 FUSE 往返，难以达到内核 ext4 的 page cache/writeback/block layer 协同效率。

所以用户态 ext4 更适合调试、恢复、镜像访问、跨平台工具或受限场景；生产主文件系统通常仍应使用内核 ext4。

### 15.11 ZFS 是否可以放到用户态

可以，但取舍很大。历史上有 zfs-fuse 这类用户态实现；现代 OpenZFS 在 Linux 上主要是内核模块形态。

ZFS 这类复杂文件系统包含 ARC、事务组、校验、压缩、快照、RAID-Z、ZIL/SLOG 等机制。放到用户态可以降低内核集成难度，但会遇到：

- 多一层 VFS/FUSE 往返。
- page cache 与 ZFS 自己缓存的双缓存问题。
- direct I/O、mmap、fsync、崩溃一致性难度。
- 内存回收和 writeback 与内核协调困难。

因此“可以做”和“适合作为高性能生产主路径”是两件事。

### 15.12 sshfs 为什么需要过用户态，I/O 流程是什么

sshfs 的目标是把远端 SFTP 暴露成本地 POSIX-ish 文件系统。Linux 内核没有通用 SFTP 文件系统 client，因此 sshfs 用 FUSE 做桥：

```text
本地应用 read/write/open
  -> 本地 VFS
  -> 本地 FUSE 内核
  -> 本地 sshfs daemon
  -> SSH/SFTP
  -> 远端 sshd/sftp-server
  -> 远端文件系统
```

它必须过本地用户态，因为 SSH/SFTP 协议栈、认证、加密、known_hosts、agent、配置等都天然在用户态生态里。

### 15.13 sshfs 和 NFS 的区别

NFS 是专门的网络文件系统协议：

- Linux 有 kernel NFS client。
- server 端有 NFS server。
- 协议设计围绕文件系统语义、缓存、一致性、文件句柄、锁等。
- 通常要求两端和网络环境更可控。

sshfs 是 SFTP 到 FUSE 的适配：

- server 端只需要 SSH/SFTP，不需要内核文件系统 server。
- 部署方便、安全模型复用 SSH。
- 语义和性能受 SFTP 限制，metadata-heavy workload 和随机小 I/O 容易慢。
- 缓存一致性弱于专门网络文件系统。

“NFS client 能否发送恶意信息到 server”这个问题本质是 server 必须验证 client 请求和权限。任何网络文件系统 server 都不能信任 client。NFS server 按导出配置、认证机制、UID/GID 映射、文件权限等限制访问。client 能发畸形请求，但 server 应该防御；如果 server 有漏洞，那是服务端安全问题。

### 15.14 为什么很多人放弃 sshfs

主要原因不是 FUSE 单点问题，而是 sshfs/SFTP 作为文件系统的综合取舍：

- 上游维护状态弱。SSHFS 3.7.3 release 页面说明这是当前维护者最后一个 release，SSHFS 不再维护或开发，issue 和 PR 被关闭。
- 性能有限。SFTP 不是为低延迟、高并发 POSIX 文件系统语义设计的协议。
- metadata-heavy 操作慢，例如大量 stat、readdir、small file。
- 缓存一致性和错误语义复杂。
- 断线恢复、挂起进程、unmount 卡住等体验不好。
- 替代方案多：NFS/SMB 用于局域网共享，rclone mount 用于云存储，rsync/sftp 用于同步或临时访问，sshuttle/VPN + 原生协议用于更稳定的网络文件系统。

sshfs 仍然有价值：临时访问远端目录、低频操作、只想依赖 SSH、不想配置 NFS/SMB 的场景。只是它不适合作为高性能、强一致、长期在线的生产文件系统。

### 15.15 WebDAV 为什么可能比 FUSE 更好

“WebDAV 比 FUSE 更好”不是普遍真理。它通常指某些使用场景下 WebDAV 更合适：

- 如果只是远端文档访问，WebDAV server/client 生态足够。
- WebDAV 是网络协议，很多系统原生支持。
- 不需要本地 FUSE daemon 和内核 mount。
- 失败边界更像网络文件访问，不容易把本地 VFS 调用挂死。

但 WebDAV 的 POSIX 语义更弱，性能和锁语义也未必好。如果应用必须通过普通 POSIX path 访问，FUSE 仍然更通用。

### 15.16 lxcfs 为什么适合 FUSE

lxcfs 需要为容器虚拟化 `/proc`、`/sys`、cgroup 视图等内容。这类文件不是传统磁盘文件，内容动态生成，依赖容器 namespace/cgroup 状态。用 FUSE 可以让容器里的应用看到普通文件路径，而具体内容由用户态 daemon 按容器上下文生成。

### 15.17 squashfuse、ratarmount、archivemount 这类项目为什么适合 FUSE

归档和只读镜像类项目非常适合 FUSE：

- 文件系统逻辑主要是解析格式和按 offset 读取。
- 通常只读或少写，缓存一致性简单。
- 不需要写内核模块。
- 用户可以无特权挂载。

squashfuse 用 FUSE 暴露 squashfs，ratarmount 暴露 tar 压缩包，archivemount 暴露归档文件。它们共同点是后端不是普通可写块设备文件系统，而是一个已有容器格式。

### 15.18 mergerfs / unionfs-fuse / fuse-overlayfs 的定位

这类项目用 FUSE 做路径合并或 overlay：

- mergerfs 把多个目录合并成一个视图，并实现自己的放置策略。
- unionfs-fuse 提供 union mount。
- fuse-overlayfs 为无特权容器提供 overlayfs-like 能力。

这类场景的元数据策略复杂但数据通常仍落在本地文件。未来 passthrough、iomap extent 类优化对它们更可能有价值，因为大文件数据可以直接走 backing file。

### 15.19 s3fs / gcsfuse / blobfuse / rclone mount 的限制

对象存储不是 POSIX 文件系统。FUSE 只是把它包装成 POSIX-ish 接口，无法消除底层差异：

- rename 可能不是原子元数据操作，而是 copy + delete。
- append 和 random write 可能很昂贵。
- 目录可能只是 key prefix。
- fsync 语义可能弱。
- listing、stat、小文件大量访问成本高。

因此这些项目适合兼容性接入，不应假定等价于本地 ext4/XFS。

### 15.20 Alluxio、Nydus 这类项目为什么可以商业化

它们不是简单“写一个 FUSE demo”，而是在 FUSE 接口背后提供更复杂的系统能力：

- 缓存层、预取、分层存储。
- 镜像按需加载。
- 数据去重、压缩、校验。
- 容器启动加速。
- 对应用保持 POSIX path 兼容。

FUSE 在这里是兼容接口和接入层，商业价值在后端缓存、分发、调度和一致性系统。

### 15.21 libnbd / nbdfuse 的意义

NBD 是块设备协议。nbdfuse 把 NBD export 暴露成 FUSE 文件，通常是把块设备内容作为一个文件访问，或方便工具链集成。它体现的是 FUSE 的另一个用途：不一定要实现完整目录树，也可以把某种设备、对象或 API 映射成一个文件。

### 15.22 “如果没有创新，就做 io_uring 或 folio 适配”是否成立

这是内核演进里的常见性能和维护方向，但不能简单贬低。

对 FUSE 这类高频路径来说：

- folio 适配能减少 page-cache 路径开销，支持大 folio，降低锁和元数据管理成本。
- iomap 化能复用成熟的 buffered IO/writeback helper。
- io_uring 能减少 daemon 通信往返成本。
- passthrough 能绕开 daemon 数据路径。

这些优化未必改变抽象模型，但能显著改善生产工作负载。

## 16. 读源码后的几个关键结论

1. FUSE 的核心不是“用户态文件系统”这五个字，而是 VFS 对象缓存和用户态 daemon 权威状态之间的一套协议。

2. 普通 FUSE 不走 bio。它走 `fuse_req`、`fuse_args`、`/dev/fuse` 或 io_uring。

3. `FOPEN_DIRECT_IO` 绕过 page cache，但不是 daemon 直接写应用进程 page，也不保证 zero-copy。

4. 当前 FUSE 已经使用 iomap，但主要是 page-cache/folio/writeback helper，不是块映射。

5. dcache/icache 在内核；daemon 通过 `LOOKUP` reply、timeout、FORGET、notify invalidation 参与维护一致性。

6. writeback-cache 性能好，但只适合 daemon 能保证所有修改经过本客户端或能正确失效缓存的场景。

7. passthrough 是本地 backing-file 场景的重要优化，但权限、stacking、缓存互斥都很严格。

8. io_uring 优化的是 daemon 通信调度模型，当前没有完全替代 `/dev/fuse`，也没有自动消除 payload 拷贝。

9. virtiofs 是 FUSE 协议在虚拟化共享文件系统里的扩展，所以放在 `fs/fuse` 下合理。

10. FUSE 的生态价值来自“把非文件系统东西包装成文件系统”和“把复杂策略放在用户态”，但它不是所有存储问题的最佳答案。

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
