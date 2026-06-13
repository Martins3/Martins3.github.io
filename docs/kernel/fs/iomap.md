# fs/iomap

先看看代码分布:

| file          | blank | comment | code | desc                                                                 |
|---------------|-------|---------|------|----------------------------------------------------------------------|
| buffered-io.c | 228   | 281     | 1146 |                                                                      |
| direct-io.c   | 76    | 107     | 392  | 唯二的 non static 函数 iomap_dio_rw 的唯一调用者 ext4_file_read_iter |
| seek.c        | 29    | 30      | 153  |                                                                      |
| fiemap.c      | 20    | 7       | 121  | 函数无人调用                                                         |
| swapfile.c    | 18    | 43      | 118  |                                                                      |
| apply.c       | 8     | 48      | 38   |                                                                      |


然后看看 tracepoint
```txt
39:     trace_iomap_iter_dstmap(iter->inode, &iter->iomap);
41:             trace_iomap_iter_srcmap(iter->inode, &iter->srcmap);
66:     trace_iomap_iter(iter, ops, _RET_IP_);

ioend.c
161:    trace_iomap_add_to_ioend(wpc->inode, pos, dirty_len, &wpc->iomap);

direct-io.c
137:    trace_iomap_dio_complete(iocb, dio->error, ret);
618:    trace_iomap_dio_rw_begin(iocb, iter, dio_flags, done_before);
704:                            trace_iomap_dio_invalidate_fail(inode, iomi.pos,
790:                    trace_iomap_dio_rw_queued(inode, iomi.pos, iomi.len);

buffered-io.c
469:    trace_iomap_readpage(iter.inode, 1);
541:    trace_iomap_readahead(rac->mapping->host, readahead_count(rac));
632:    trace_iomap_release_folio(folio->mapping->host, folio_pos(folio),
649:    trace_iomap_invalidate_folio(folio->mapping->host,
1682:   trace_iomap_writeback_folio(inode, pos, folio_size(folio));
```

只是编辑一下文件，看来的确大多数的 io 都是在 iomap 上的:
```txt
                25      iomap:iomap_dio_complete
                25      iomap:iomap_dio_rw_begin
                23      iomap:iomap_dio_rw_queued
                 0      iomap:iomap_dio_invalidate_fail
             5,008      iomap:iomap_iter
             1,388      iomap:iomap_add_to_ioend
             1,388      iomap:iomap_writeback_folio
             2,509      iomap:iomap_iter_dstmap
                 0      iomap:iomap_iter_srcmap
                 2      iomap:iomap_invalidate_folio
                 0      iomap:iomap_release_folio
                 0      iomap:iomap_readahead
                 0      iomap:iomap_readpage
```

```txt
@[
        iomap_iter+5
        iomap_file_buffered_write+174
        xfs_file_buffered_write+144
        vfs_write+602
        ksys_write+107
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 4
```
似乎基本的 workflow 就是，fs 注册下层接口，然后 fs 来调用上层接口

## iomap 来同时控制通过文件的三个 io 路径 direct io buffer io 和 mmap

所以提供了:
上层 I/O 操作专用 ops 结构体
- `struct iomap_dio_ops`（用于 Direct I/O）
- buffer io
	- `struct iomap_write_ops`（buffered write）
	- `struct iomap_writeback_ops`（用于 writeback）
	- `simplefs_read_iomap_ops` (buffered read)
- mmap io 操作和 buffer io 基本是复用的，只不过一个是拷贝，一个直接用的内核的 page cache (share mmap ，private mmap 都没意义)

例如 mmap 写之后，然后手动执行 sync ，就可以得到这个效果:
```txt
@[
        simplefs_writeback_range+5
        iomap_writeback_folio+751
        iomap_writepages+96
        simplefs_writepages+74
        do_writepages+187
        __writeback_single_inode+90
        writeback_sb_inodes+642
        __writeback_inodes_wb+84
        wb_writeback+538
        wb_workfn+864
        process_one_work+516
        worker_thread+465
        kthread+291
        ret_from_fork+648
        ret_from_fork_asm+26
]: 3
```

## Documentation
https://www.kernel.org/doc/html/latest/filesystems/iomap/index.html
https://www.kernel.org/doc/html/latest/filesystems/iomap/porting.html

1. 为什么要切换？
  - Pagecache operations lock a single base page at a time and then call into the filesystem to return a mapping for only that page. Direct I/O operations build I/O requests a single file block at a time. This worked well enough for direct/indirect-mapped filesystems such as ext2, but is very inefficient for extent-based filesystems such as XFS.
  - Large folios are only supported via iomap; there are no plans to convert the old buffer_head path to use them.
  - Direct access to storage on memory-like devices (fsdax) is only supported via iomap.
  - Lower maintenance overhead for individual filesystem maintainers. iomap handles common pagecache related operations itself, such as allocating, instantiating, locking, and unlocking of folios. No ->write_begin(), ->write_end() or direct_IO address_space_operations are required to be implemented by filesystem using iomap.

我们
1. 如何理解什么叫作 direct/indirect-mapped fs ，什么是 extent-based fs
  - 但是有这个文件 fs/ext4/extents.c
2. iomap 如何支持的 large folios 的
3. fsdax 和 dax 是什么关系?

> [!NOTE]
> 参考 Deepseeek ，有待验证

一个文件系统若满足以下条件，才算“完全转换”：
- 使用 iomap_file_buffered_write 替代 ->write_begin/->write_end
- 使用 iomap_read_folio / iomap_readahead
- 使用 iomap_writepages 替代自定义 writeback
- 使用 iomap_page_mkwrite 替代 ->page_mkwrite
- 不设置 IOMAP_F_BUFFER_HEAD（即摆脱 buffer_head）
- 支持 large folio（mapping_set_large_folios）

## 实验

ext4 dio
```txt

             2,492      iomap:iomap_dio_complete
             2,502      iomap:iomap_dio_rw_begin
             5,040      iomap:iomap_iter
                16      iomap:iomap_add_to_ioend
                 0      iomap:iomap_iter_srcmap
             2,521      iomap:iomap_iter_dstmap
             2,501      iomap:iomap_dio_rw_queued
                 0      iomap:iomap_dio_invalidate_fail
                 0      iomap:iomap_invalidate_folio
                 0      iomap:iomap_release_folio
                16      iomap:iomap_writeback_folio
                 0      iomap:iomap_readahead
                 0      iomap:iomap_readpage
```

buffer io ，基本上完全无法观测到，所以的确 ext4 是没有支持的。


direct io 的
```txt

                 2      iomap:iomap_dio_complete
                 2      iomap:iomap_dio_rw_begin
         3,337,648      iomap:iomap_iter
                39      iomap:iomap_add_to_ioend
                 0      iomap:iomap_iter_srcmap
         1,668,824      iomap:iomap_iter_dstmap
                 2      iomap:iomap_dio_rw_queued
                 0      iomap:iomap_dio_invalidate_fail
                 0      iomap:iomap_invalidate_folio
                 0      iomap:iomap_release_folio
                39      iomap:iomap_writeback_folio
         1,668,811      iomap:iomap_readahead
                 0      iomap:iomap_readpage
```

## 为什么 iomap 需要分开考虑 buffered io 和 dio
<!-- 6f419ed6-2fbb-46ee-b07e-e22dd8e2731d -->

`iomap` 之所以需要**分别考虑 buffered io 和 Direct I/O（DIO）路径**，根本原因在于这两种 I/O 模式在**数据路径、缓存行为、锁语义和错误处理**等方面存在本质差异。`iomap` 的设计目标是在统一映射抽象（`struct iomap`）的基础上，为不同 I/O 类型提供高效、正确的实现，而不是强行用同一套逻辑处理所有场景。

### 1. **页缓存 vs 绕过页缓存**
- **Buffered I/O**：
  - 数据必须经过 **页缓存（pagecache/folio）**。
  - 需要 **分配、填充、管理 folio 的状态**（如 uptodate、dirty）。
  - 可能涉及 **readahead、partial uptodate、block-level 状态跟踪**（通过 `iomap_folio_state`）。
- **Direct I/O**：
  - **绕过页缓存**，直接在用户缓冲区和存储设备之间传输数据。
  - 不需要管理 folio，也不关心 uptodate/dirty 状态。
  - 必须先 **flush/invalidate 页缓存中对应区域**（避免数据不一致）。

`iomap` 必须为两者提供不同的数据路径：`iomap_read_folio` / `iomap_readahead` vs `iomap_dio_rw`。

### 2. **映射重验证（Revalidation）需求不同**
- **Buffered I/O**：
  - 在 `->iomap_begin` 获取映射后，可能因 **页分配、用户页 fault、内存回收** 等长时间操作，导致映射**过期（stale）**。
  - 因此需要 `->iomap_valid` 回调和 `validity_cookie` 机制，在 folio 锁定后**重新验证映射是否有效**（见文档中 `iomap_valid` 章节）。
- **Direct I/O**：
  - 映射获取后立即构造 bio 并提交，**无中间长时间操作**，通常不需要重验证。
  - 即使需要（如 `IOMAP_NOWAIT`），也由文件系统在 `->iomap_begin` 中一次性完成。

buffered 路径需要额外的 **映射生命周期管理**，DIO 则不需要。

### 3. **错误处理与 partial write 语义不同**
- **Buffered write**：
  - partial write 只需更新 folio 的 dirty 范围，剩余未写部分**保持原样**（可能是 hole 或旧数据）。
  - 若为 `IOMAP_F_NEW` 且写失败，需通过 `iomap_write_delalloc_release` **释放未使用的 delalloc 预留空间**。
- **Direct I/O**：
  - 必须保证 **原子性或明确的错误边界**。
  - 若无法完成整个请求（如未对齐、需分配但 `IOMAP_NOWAIT`），应返回 `-EAGAIN` 或 fallback 到 buffered（通过 `-ENOTBLK`）。

→ 两者对“部分成功”的处理逻辑完全不同。

### 4. **对齐与块边界处理不同**
- **Buffered I/O**：
  - 内核可自动处理 **任意偏移和长度**，页缓存负责填充/截断。
  - 支持 **sub-block 写 + zeroing**（如 `iomap_truncate_page`）。
- **Direct I/O**：
  - 通常要求 **对齐到 filesystem block size 和设备 sector size**。
  - 若未对齐，文件系统可能需 fallback 到 buffered，或执行 **read-modify-write**（效率低，需额外锁）。

`iomap_dio_rw` 需显式处理对齐检查，而 buffered 路径无需关心。

### 5. **锁与并发模型不同**
- **Buffered I/O**：
  - 可能持有 `i_rwsem`（共享或独占），但 **不能长时间持有底层映射锁**（因需 fault 用户页）。
  - 依赖 `invalidate_lock` 防止并发 `page_mkwrite` 与 truncate 冲突。
- **Direct I/O**：
  - 通常在 `i_rwsem` 保护下快速完成映射和 I/O 提交。
  - 不涉及页缓存并发，无需 `invalidate_lock`。

锁定策略直接影响 `->iomap_begin` 的实现方式。

### 6. **I/O 完成回调与后处理不同**
- **Buffered writeback**：
  - 使用 `iomap_ioend` 链，支持 **批量处理 unwritten extent 转换**。
  - 需要 `->writeback_range` 和 `->writeback_submit` 回调。
- **Direct I/O**：
  - 使用 `->submit_io` 和 `->end_io` 回调（见 `struct iomap_dio_ops`）。
  - 可能需处理 **unwritten extent 转换、COW、异步完成（`-EIOCBQUEUED`）**。

## iomap 相关的核心结构体
<!-- 26b232df-84ca-4176-8d06-77d81be8b998 -->

`iomap` 引入了一套**分层抽象的 I/O 框架**，其核心在于将 **“文件偏移到存储设备的映射”**
与 **“在此映射上执行的具体 I/O 操作”** 分离。围绕这一思想，它定义了若干关键结构体，彼此协作，形成清晰的层级关系。

以下是 `iomap` 中最核心的结构体及其关系：

### struct iomap_iter

这是 `iomap` 框架**内部使用的运行时上下文**，用于**遍历文件范围，并逐段获取 `iomap`**。

它包含：
- 指向 `inode`、当前 `pos`、剩余 `len`
- 当前有效的 `struct iomap iomap`
- `flags`（如 `IOMAP_WRITE`, `IOMAP_DIRECT`, `IOMAP_DAX`）
- `private`（传递给文件系统的私有数据）

> **作用**：驱动整个 I/O 过程，每次调用 `iomap_iter(&iter, ops)` 就会推进到下一个映射段。

(在 vfs 中创建，一般作为局部变量创建，然后向下传递，也就是维持一个上下文)

### struct iomap —— 映射描述符

这是整个框架的**基础单元**，由文件系统提供，描述一段**连续的文件偏移区间**（`offset`, `length`）如何映射到存储设备。

关键字段：
- `.type`：映射类型（`IOMAP_HOLE`、`IOMAP_DELALLOC`、`IOMAP_MAPPED`、`IOMAP_UNWRITTEN`、`IOMAP_INLINE`）
- `.flags`：状态标志（如 `IOMAP_F_NEW`, `IOMAP_F_DIRTY`, `IOMAP_F_SHARED`）
- `.addr` / `.bdev` / `.dax_dev`：设备地址和设备对象
- `.validity_cookie`：用于 buffered I/O 中重验证映射是否过期

> **作用**：统一表达“文件位置 → 存储位置”的映射，取代了传统的 `get_block()` 返回的 per-page 映射。

(相当于是一段映射有一个)

### struct iomap_ops —— 映射操作接口（文件系统侧）

文件系统通过实现此结构体，向 `iomap` 框架提供**如何获取和释放映射**的能力。

```c
struct iomap_ops {
    int (*iomap_begin)(struct inode *inode, loff_t pos, loff_t length,
                       unsigned flags, struct iomap *iomap, struct iomap *srcmap);
    int (*iomap_end)(struct inode *inode, loff_t pos, loff_t length,
                     ssize_t written, unsigned flags, struct iomap *iomap);
};
```

- `->iomap_begin()`：根据 `pos/length/flags` 填充 `iomap`（可能仅覆盖部分请求范围）
- `->iomap_end()`：在 I/O 完成后清理资源（如释放 delalloc 预留）

> **作用**：定义文件系统与 iomap 框架的“映射契约”，是所有 I/O 路径的起点。

例如:
```c
const struct iomap_ops xfs_read_iomap_ops = {
	.iomap_begin		= xfs_read_iomap_begin,
};
```

### 上层 I/O 操作专用 ops 结构体

针对不同 I/O 类型，`iomap` 提供专用回调结构体，由文件系统可选实现，用于介入 I/O 生命周期：

#### a. `struct iomap_write_ops`（用于 buffered I/O）
```c
struct iomap_write_ops {
    struct folio *(*get_folio)(...);
    void (*put_folio)(...);
    bool (*iomap_valid)(...);        // 重验证映射（buffered I/O 特有）
    int (*read_folio_range)(...);
};
```
用于 buffered write、page_mkwrite、unshare、zero_range 等页缓存操作。

#### b. `struct iomap_dio_ops`（用于 Direct I/O）
```c
struct iomap_dio_ops {
    void (*submit_io)(...);          // 自定义提交 bio
    int (*end_io)(...);              // I/O 完成后处理（如 unwritten → mapped）
    struct bio_set *bio_set;
};
```
用于 `iomap_dio_rw`，允许文件系统介入 DIO 的 bio 构造与完成。

#### c. `struct iomap_writeback_ops`（用于 writeback）
```c
struct iomap_writeback_ops {
    int (*writeback_range)(...);     // 为脏 folio 的某段分配映射
    int (*writeback_submit)(...);    // 提交整个 writeback 上下文
};
```

用于 `iomap_writepages`，处理脏页回写。

writeback 和 buffer write 不相同，writeback 需要考虑这些:
- 绕过 folio 分配
- 绕过映射重验证
- 使用专用映射逻辑（只查已分配的 extent）

### 5. `struct iomap_folio_state`（IFS）—— 页内块状态管理

当 **文件系统块大小 < folio 大小** 时，`iomap` 使用此结构体**跟踪 folio 内每个 fs-block 的 `uptodate` 和 `dirty` 状态**（用位图实现，极轻量）。

> 取代了传统的 `buffer_head` 链表，使 large folio 成为可能。

### 核心关系图（逻辑数据流）

```
用户系统调用（write/read/dio/fiemap等）
        ↓
iomap 上层 API（如 iomap_file_buffered_write, iomap_dio_rw）
        ↓
创建 struct iomap_iter
        ↓
循环调用 iomap_iter(&iter, ops)
        ↓
        → 调用 ops->iomap_begin() ← 文件系统填充 struct iomap
        ↓
根据 I/O 类型，使用对应的 ops（write_ops / dio_ops / writeback_ops）
        ↓
执行具体操作：
   - Buffered I/O：分配 folio，管理 IFS，copy 数据
   - DIO：构造 bio，调用 dio_ops->submit_io
   - Writeback：遍历脏块，调用 writeback_ops
        ↓
I/O 完成后（可能异步），调用 ops->iomap_end()
```

### 总结：分层与职责分离

| 层级               | 结构体                        | 职责                     | 由谁实现                     |
|--------------------|-------------------------------|--------------------------|------------------------------|
| **映射抽象层**     | `struct iomap`                | 描述一段文件到存储的映射 | 文件系统（通过 `iomap_ops`） |
| **映射获取接口**   | `struct iomap_ops`            | 获取/释放映射            | 文件系统                     |
| **迭代驱动层**     | `struct iomap_iter`           | 驱动整个 I/O 过程        | iomap 框架内部               |
| **I/O 类型适配层** | `*ops`（write/dio/writeback） | 定制 I/O 行为            | 文件系统（可选）             |
| **页内状态管理**   | `struct iomap_folio_state`    | 管理 sub-folio 块状态    | iomap 框架自动管理           |


## 基本执行流程
ext4/xfs's direct IO call `iomap_dio_rw`

- iomap_dio_rw
  - `__iomap_dio_rw`
    - filemap_write_and_wait_range
      - iomap_dio_iter
        - iomap_dio_hole_iter
        - iomap_dio_bio_iter
          - iomap_dio_alloc_bio
          - iomap_dio_submit_bio

- xfs_file_read_iter
  - xfs_file_dio_aio_read ==> iomap_dio_rw
  - xfs_file_buffered_aio_read ==> generic_file_read_iter

似乎简单来说，就是通过 iter 将 io 切分为多个，然后通过 bio 提交下去。

- common_interrupt
  - __common_interrupt
    - handle_irq
      - generic_handle_irq_desc
        - handle_edge_irq
          - handle_irq_event
            - handle_irq_event_percpu
              - __handle_irq_event_percpu
                - vring_interrupt
                  - vring_interrupt
                    - virtscsi_req_done
                      - virtscsi_vq_done
                        - scsi_io_completion
                          - scsi_end_request
                            - blk_update_request
                              - req_bio_endio
                                - xfs_buf_bio_end_io

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_io_submit
        - __se_sys_io_submit
          - __do_sys_io_submit
            - io_submit_one
              - __io_submit_one
                - aio_read
                  - call_read_iter
                    - ext4_file_read_iter
                      - ext4_dio_read_iter
                        - iomap_dio_rw
                          - __iomap_dio_rw
                            - iomap_iter
                              - ext4_iomap_begin

## 文章
- [Converting filesystems to iomap](https://mp.weixin.qq.com/s/M9cK2if7rxG5bH-1nEwqfQ)
- https://kernelnewbies.org/KernelProjects/iomap : 有点老了

- [Sunsetting buffer heads](https://lwn.net/Articles/931809/)

> The core functionality they provide — facilitating sector-size I/O operations to a block device underlying a filesystem — must be provided somehow.

对啊，什么时候变为 buffer cache 了!

Filesystems work hard to pack data on disk, and chances are good that the adjacent sectors will also be accessed soon. With current hardware, he said, I/O size is no longer as important for performance, so filesystem developers should not hesitate to use page-sized I/O operations.

- [A kernel without buffer heads](https://lwn.net/Articles/930173/)

才意识到，page cache 和 block cache 差别，page cache 是的 key 是文件，
而 block cache 的 key 是 disk 的 offset 。

如果 page cache 在一个文件中不命中，然后内容其实因为另外一个文件缓存过的，命中过程是什么样子的?

这里有一些让人迷惑的地方，
[buffer 和 cache 的区别](https://stackoverflow.com/questions/6345020/what-is-the-difference-between-buffer-vs-cache-memory-in-linux)

[IO 子系统全流程介绍](https://zhuanlan.zhihu.com/p/545906763)

看看这里说的 IOMAP_F_BUFFER_HEAD

## fuse 的支持

- adds fuse iomap support for buffered reads and readahead.
  - https://lore.kernel.org/all/20250926002609.1302233-1-joannelkoong@gmail.com/
  (这里解答了，为什么 fuse 需要 iomap 的，那么岂不是 nfs 也可以这样?)

fuse 是没有使用 BUFFER_HEAD 的，但是
fuse 也没有用注册 iomap_dio_ops 的。

```c
static const struct iomap_ops fuse_iomap_ops = {
	.iomap_begin	= fuse_iomap_begin,
};


static const struct iomap_write_ops fuse_iomap_write_ops = {
	.read_folio_range = fuse_iomap_read_folio_range,
};

static const struct iomap_writeback_ops fuse_writeback_ops = {
	.writeback_range	= fuse_iomap_writeback_range,
	.writeback_submit	= fuse_iomap_writeback_submit,
};
```

为什么 direct io 不支持，感觉原因是:
fuse_file_read_iter fuse_file_write_iter 一路看下去，最后
就是走到 fuse_direct_io 了，然后就搞完了。

## block dev

取决于是否打开 CONFIG_BUFFER_HEAD

如果打开了 ，那么走 blkdev_get_block ， 否则定义
```c
static const struct iomap_writeback_ops blkdev_writeback_ops = {
	.writeback_range	= blkdev_writeback_range,
	.writeback_submit	= iomap_ioend_writeback_submit,
};
```
也就是说，block dev 可以实现 buffer io 也走 iomap ，但是现在还没有。

## ext2 只有 direct io
direct 还是 iomap ，但是 buffer io 是通过 buffer head 的

ext2 和 blkdev 的 aops 还是注册了，例如 blkdev_read_folio
```c
const struct address_space_operations ext2_aops = {
	.dirty_folio		= block_dirty_folio,
	.invalidate_folio	= block_invalidate_folio,
	.read_folio		= ext2_read_folio,
	.readahead		= ext2_readahead,
	.write_begin		= ext2_write_begin,
	.write_end		= ext2_write_end,
	.bmap			= ext2_bmap,
	.writepages		= ext2_writepages,
	.migrate_folio		= buffer_migrate_folio,
	.is_partially_uptodate	= block_is_partially_uptodate,
	.error_remove_folio	= generic_error_remove_folio,
};

static int ext2_read_folio(struct file *file, struct folio *folio)
{
	return mpage_read_folio(folio, ext2_get_block);
}

static void ext2_readahead(struct readahead_control *rac)
{
	mpage_readahead(rac, ext2_get_block);
}
```

ext2 的 buffer io 走的 buffer head 的路径:
```txt
     1.88% ext2_get_block
      - ext2_get_blocks
         - 1.55% ext2_get_branch
            - 1.33% __bread_gfp
               - 1.28% __find_get_block
                  - 0.67% __filemap_get_folio
                       0.66% filemap_get_entry
```

```txt
@[
    ext2_get_blocks+1
    ext2_get_block+94
    do_mpage_readpage+638
    mpage_readahead+158
    ext2_readahead+5 <- aops
    read_pages+113
    page_cache_ra_unbounded+370
    force_page_cache_ra+147
    filemap_get_pages+314
    filemap_read+251
    ext2_file_read_iter+5
    vfs_read+665
    ksys_read+110
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 3200
```



```txt
+ sudo bpftrace -e 'kprobe:blkdev_get_block { @[kstack(bpftrace)] = count(); }'
Attaching 1 probe...

@[
    blkdev_get_block+5
    do_mpage_readpage+638
    mpage_readahead+158
    read_pages+113
    page_cache_ra_unbounded+370
    force_page_cache_ra+147
    filemap_get_pages+314
    filemap_read+251
    blkdev_read_iter+104
    vfs_read+665
    ksys_read+110
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 713

```

但是 direct io 是不会走这里的:
```txt
-   66.68%     0.03%  fio      [kernel.kallsyms]  [k] entry_SYSCALL_64_after_hwframe
     66.65% entry_SYSCALL_64_after_hwframe
      - do_syscall_64
         - 42.90% __x64_sys_io_submit
            - 40.80% io_submit_one
               - 30.47% aio_read
                  - 30.39% blkdev_read_iter
                     - 30.10% blkdev_direct_IO
                        - 17.87% submit_bio_noacct_nocheck
                           - 16.30% __submit_bio
                              - 8.87% __blk_flush_plug
                                 - blk_mq_flush_plug_list
                                    - null_queue_rqs
```

```txt
-   81.82%     0.35%  fio      [kernel.kallsyms]  [k] entry_SYSCALL_64_after_hwframe
     81.47% entry_SYSCALL_64_after_hwframe
      - do_syscall_64
         - 61.38% __x64_sys_io_submit
            - 59.82% io_submit_one
               - 55.86% aio_read
                  - 55.31% ext2_file_read_iter
                     - 53.72% iomap_dio_rw
                        - 53.48% __iomap_dio_rw
                           - 30.56% blk_finish_plug
                              - 30.47% __blk_flush_plug
                                 - 30.27% blk_mq_flush_plug_list
                                    - 28.45% blk_mq_run_hw_queue
                                       - 27.74% blk_mq_sched_dispatch_requests
                                          - 27.71% __blk_mq_sched_dispatch_requests
                                             - 23.72% blk_mq_dispatch_rq_list
                                                - 23.01% scsi_queue_rq
```

但是 meta 的 io 路径下会调用的:
```txt
@[
    ext2_read_folio+5
    filemap_read_folio+57
    do_read_cache_folio+124
    ext2_get_folio.constprop.0.isra.0+21
    ext2_find_entry+125
    ext2_inode_by_name+49
    ext2_lookup+100
    path_openat+1703
    do_filp_open+216
    do_sys_openat2+171
    __x64_sys_openat+87
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

## ext4 目前只有 direct io 的支持

ext4 只是对于 iomap 移植了 direct io ，而 buffered 没有移植:
```txt
sudo bpftrace -e 'kprobe:ext4_sb_bread { @[kstack(bpftrace)] = count(); }'
```
这个是需要关联 fs 的结果的:

ext4_read_bh 可以有调用的:
```txt
@[
    ext4_read_bh+5
    ext4_bread+82
    __ext4_read_dirblock+82
    dx_probe+103
    ext4_dx_find_entry+86
    __ext4_find_entry+957
    ext4_lookup+152
    __lookup_slow+131
    walk_component+219
    path_lookupat+106
    filename_lookup+242
    user_path_at+55
    do_faccessat+249
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    ext4_read_bh+5
    ext4_bread+82
    __ext4_read_dirblock+82
    ext4_dx_find_entry+271
    __ext4_find_entry+957
    ext4_lookup+152
    path_openat+1703
    do_filp_open+216
    do_sys_openat2+171
    __x64_sys_openat+87
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

## buffer head vs iomap 的使用情况

```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

cd /home/martins3/data/kernel/linux-build
readarray -t a < <(rg -l "select BUFFER_HEAD")

for i in "${a[@]}"; do
	m=${i#*/}
	fs=${m%/*}
	if ! rg --type c "${fs}_get_block\("; then
		echo "$fs doesn't define get_block_t"
	fi

	if ! rg --type c "${fs}_get_folio\("; then
		echo "$fs doesn't define get_block_t"
	fi
done
```

然后去类似的找对于 get_block 的实现，可以有:
```c
int ext4_get_block(struct inode *inode, sector_t iblock,
		   struct buffer_head *bh, int create)
{
	return _ext4_get_block(inode, iblock, bh,
			       create ? EXT4_GET_BLOCKS_CREATE : 0);
}
```

nfs 和 fuse 都没有定义 BUFFER_HEAD ，也没有 9p
看来没有这个文件系统，是由于其他的原因的。

依赖的
```txt
fs/zonefs/Kconfig
fs/btrfs/Kconfig
fs/f2fs/Kconfig
fs/ext4/Kconfig
fs/xfs/Kconfig
fs/ext2/Kconfig
fs/erofs/Kconfig
fs/hpfs/Kconfig
fs/gfs2/Kconfig
fs/fuse/Kconfig
block/Kconfig
```

## get_block 就是最原始的文件到 disk 映射
```c
struct iomap {
	u64			addr; /* disk offset of mapping, bytes */
	loff_t			offset;	/* file offset of mapping, bytes */
	u64			length;	/* length of mapping, bytes */
	u16			type;	/* type of mapping */
	u16			flags;	/* flags for mapping */
	struct block_device	*bdev;	/* block device for I/O */
	struct dax_device	*dax_dev; /* dax_dev for dax operations */
	void			*inline_data;
	void			*private; /* filesystem private */
	const struct iomap_folio_ops *folio_ops;
	u64			validity_cookie; /* used with .iomap_valid() */
};
```

似乎我们忘记了我的老朋友 get_block 了:
这既避免了传统 `get_block()` 接口的碎片化（每个文件系统重复实现各种 I/O 路径），又保留了对各类 I/O 特性的精确控制。

## 和 bio 的关系

iomap 会构造 bio 的
例如 iomap_dio_zero 中，

bio 中记录映射，而 iomap 来实现映射，所以 bio 的生成在 iomap 中:
所以，这就是很合理了。


## sb_bread
如你正在开发一个新文件系统并考虑迁移，记住：
数据 I/O → 用 iomap
元数据 I/O → 用 bio 或 buffer_head

## 为什么 iomap 不再缓存内存和 block 的映射关系
<!-- ba7ad310-51d0-403e-92b5-ffffde437dea -->

- **buffer_head 路径**：需要缓存 bh，因为每个 fs-block 都要独立状态跟踪。
- **iomap 路径**：用 `iomap_folio_state`（位图）替代 bh，**无需缓存 bh**，因为：
  - 映射由 `->iomap_begin()` 按需提供
  - 状态存储在 folio 的 `private`，无独立 bh 对象

### 一、设计哲学差异：**缓存映射 vs 缓存数据**

| 模型             | `buffer_head`（传统）                     | `iomap`（现代）                                   |
|------------------|-------------------------------------------|---------------------------------------------------|
| **缓存对象**     | 映射元数据（`b_blocknr`, `b_bdev`, 状态） | **不缓存映射**，只缓存**数据**（pagecache/folio） |
| **映射获取频率** | 每次 I/O（甚至 per-page）都可能查缓存     | 仅在 I/O 开始时调用 `->iomap_begin()` 获取        |
| **映射生命周期** | 长期驻留（绑定到 folio，随其释放）        | **瞬时有效**（仅用于当前 I/O 操作）               |
| **适用文件系统** | 间接块表（ext2）、小 extent 系统          | **大 extent / B+树**（XFS, ext4 extent）          |

现代文件系统（XFS/ext4）用 B+树管理 extent，**查一次映射可覆盖几百 KB 到 MB 的数据**，缓存单个 block 映射收益极低。

### 二、为什么 iomap 不缓存映射？

1. **映射粒度大，复用率低**
  - `iomap` 的目标是一次返回**尽可能大的连续映射**（如 1MB extent）。
  - 传统 `get_block()` 是 per-block 调用（4KB 一次），缓存有收益。
  - 但 `iomap_begin()` 一次调用就覆盖整个 write/read 范围，**无需重复查询**。

2. **映射可能失效（stale）**
  - 在 buffered I/O 路径中，从 `->iomap_begin()` 到实际写入 folio 之间可能：
    - 发生 `page fault`
    - 触发内存回收
    - 其他线程 truncate / punch hole
  - 因此 iomap 引入了 `->iomap_valid()` 和 `validity_cookie` 机制来**重验证映射**。
  - **缓存映射反而会加剧 stale 问题**。

3. **内存开销 vs 收益**
  - `buffer_head` 每个 block 104 字节，对 64KB folio（16 blocks）需 ~1.6KB。
  - `iomap_folio_state`（IFS）仅用 **2 bits/block**（状态位图），内存减少 400 倍。
  - **缓存映射地址（u64）本身也要 8 字节/block**，不如直接查 filesystem。

4. **文件系统已内建高效映射缓存**
  - XFS 有 inode 的 extent cache（`if_extents`）
  - ext4 有 extent 空闲缓存和 leaf 缓存
  - **这些缓存位于文件系统内部**，比通用 `bh_lru` 更高效、更语义化。

📌 iomap 的设计原则是：**“把映射查询交给文件系统，你比我知道如何高效缓存”**。

### 三、例外：Direct I/O 的“隐式缓存”

在 `iomap_dio_rw` 中：
- 映射只用一次，提交 bio 后立即丢弃。
- 但若使用 `IOMAP_DIO_PARTIAL` 或 retry（如 page fault），**可能多次调用 `->iomap_begin()`**。
- 然而，DIO 路径通常要求 **aligned、pure overwrite**，映射本身稳定，且 retry 极少发生。
- **即使 retry，重新查一次映射成本远低于维护缓存的复杂性**。

| 场景           | `buffer_head`      | `iomap`                    |
|----------------|--------------------|----------------------------|
| **映射缓存**   | ✅ 有 per-CPU LRU  | ❌ 无                      |
| **映射粒度**   | per-fs-block (4KB) | 大 extent (up to MB)       |
| **映射有效性** | 隐式假设有效       | 显式验证（`iomap_valid`）  |
| **适用时代**   | 2000s 之前（ext2） | 2010s+（XFS, ext4 extent） |

> **是的，iomap 没有提供 fs-block 到设备扇区映射的缓存，这是有意设计**。
> 因为现代文件系统的映射：
> - 粒度大，复用率低；
> - 易失效，缓存不安全；
> - 文件系统自身已有更高效的缓存；
> - 缓存映射的收益远小于其内存和复杂性开销。

iomap 的哲学是：**“按需查询，用完即弃，信任文件系统”** —— 这正是它比 `buffer_head` 路径更简洁高效的原因。


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
