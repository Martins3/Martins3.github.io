# ext4

## overview

以下统计和职责说明基于 `/home/martins3/data/kernel/linux` 的 `v7.1-dirty` 工作树。行数由 `tokei` 统计；
不包含 KUnit 测试文件和 `.kunitconfig`。

| File             | blank | comment | code | explanation |
|------------------|------:|--------:|-----:|-------------|
| super.c          | 886 | 835 | 5885 | ext4 的挂载与卸载中心：解析挂载参数、读取和校验超级块及块组描述符、初始化日志/配额/工作队列，定义 `super_operations`，并统一处理错误、冻结和恢复。 |
| mballoc.c        | 869 | 1587 | 4827 | 多块分配器（mballoc）：以块组 buddy 和位图寻找连续空闲区，执行按 inode/局部性分组的预分配、释放、discard 与 `fstrim`。实际的数据块分配主路径在这里。 |
| inode.c          | 769 | 1622 | 4435 | 普通文件数据与 inode 的核心实现：逻辑块映射 `ext4_map_blocks()`、buffered write/delalloc/writeback、iomap/DIO 映射、截断打洞，以及磁盘 inode 的读写和 VFS 属性操作。 |
| extents.c        | 742 | 1295 | 4271 | extent B+ 树实现：查找、插入、分裂、合并和删除 extent，处理 unwritten extent 转换、`fallocate`/collapse/insert range、FIEMAP 与 fast-commit replay。 |
| namei.c          | 410 | 463 | 3376 | 目录项查找和命名空间修改：lookup/create/link/unlink/mkdir/rmdir/rename，维护线性目录或 HTree 索引，并定义目录及特殊 inode 的 `inode_operations`。 |
| ext4.h           | 426 | 790 | 2738 | ext4 的主内部头文件：磁盘格式、挂载状态、`ext4_inode_info`/`ext4_sb_info`、块映射标志、特性判断、锁规则和跨文件接口声明。 |
| xattr.c          | 358 | 410 | 2462 | 扩展属性核心：在 inode body、外部 xattr block 或 EA inode 中查找/增删值，负责共享块、引用计数、校验和、配额和缓存。 |
| resize.c         | 271 | 333 | 1588 | 在线扩容：新增块组/flex_bg、分配并初始化位图和 inode table、扩展块组描述符，更新超级块及其备份。 |
| fast_commit.c    | 262 | 479 | 1572 | JBD2 fast commit 实现：跟踪 inode、数据范围和目录项变更，编码增量日志，提交、扫描并回放 fast-commit 区域；不适用时退化为完整 journal commit。 |
| ioctl.c          | 283 | 171 | 1565 | ext4 专有 ioctl 与 fileattr 入口：flags/version、在线扩容、迁移、移动 extent、加密、verity、shutdown、fsmap、预缓存和保留空间等管理操作。 |
| extents_status.c | 257 | 577 | 1558 | 每 inode 的内存 extent-status 红黑树缓存，记录 written/unwritten/delayed/hole 状态，加速映射查询；还管理 delayed-allocation 待分配 cluster 和 shrinker 回收。 |
| inline.c         | 280 | 207 | 1513 | inline data 实现：把小文件数据、目录项或符号链接保存在 inode 的 `i_block` 和 inode-body xattr 空间中，并处理读写、目录操作及转为普通 extent。 |
| ialloc.c         | 173 | 264 | 1189 | inode 分配与释放：读取/校验 inode bitmap，使用 Orlov 等策略选择块组和 inode，更新块组/超级块计数，并初始化 inode table。 |
| indirect.c       | 117 | 532 | 825 | 兼容非 extent inode 的传统直接、一级、二级和三级间接块映射，负责查找、分配、截断和移除间接块树。 |
| file.c           | 129 | 203 | 675 | 普通文件的 VFS 前端，定义 `file_operations` 和 `inode_operations`；分派 buffered I/O、iomap direct I/O、DAX、mmap、open/release 和写入检查。 |
| balloc.c         | 107 | 231 | 665 | 块组级块位图基础设施：定位块组、读取/初始化/校验 block bitmap、维护空闲 cluster 计数并判断是否可重试分配；不负责 mballoc 的空间搜索策略。 |
| sysfs.c          | 84 | 11 | 603 | 注册 `/sys/fs/ext4/<dev>`、`/sys/fs/ext4/features` 和 proc 项，暴露调优参数、统计、错误信息及测试触发器。 |
| fsmap.c          | 105 | 134 | 553 | 实现 `FS_IOC_GETFSMAP`：枚举数据设备和 journal 设备上的已分配区、空洞及固定元数据区域，返回文件系统物理空间所有权映射。 |
| dir.c            | 65 | 124 | 505 | 目录文件读取路径：实现线性目录和 HTree 目录的 `iterate_shared/readdir`，校验目录项，并管理按 hash 排序的遍历缓存。目录增删在 `namei.c`。 |
| orphan.c         | 66 | 103 | 490 | orphan 管理和崩溃恢复：在截断/删除未完成时把 inode 加入传统 orphan 链或 orphan file，事务完成后移除，并在挂载时清理遗留 inode。 |
| migrate.c        | 64 | 141 | 479 | inode 块映射格式迁移：把传统 indirect 映射转换为 extent，也支持在约束条件下从 extent 退回 indirect 格式。 |
| move_extent.c    | 77 | 103 | 478 | 在线碎片整理所需的 extent 交换：在两个普通文件间按逻辑范围交换物理块映射，同时处理锁、page cache、校验和 journal 更新。 |
| page-io.c        | 68 | 94 | 451 | buffered writeback 的 BIO 提交与完成层：聚合 buffer 到 BIO，跟踪 `ext4_io_end`，在 I/O 完成后转换 unwritten extent 并释放预留空间。 |
| readpage.c       | 52 | 74 | 329 | buffered read/readahead 路径：把连续文件块组成 BIO；读完成后按需执行 fscrypt 解密和 fs-verity 校验，复杂映射则回退到 buffer-head 读路径。 |
| ext4_jbd2.c      | 55 | 43 | 310 | ext4 与 JBD2 的非内联接口：选择 data=journal/ordered/writeback 模式，启动/停止或扩展事务，传播 journal/块设备写错误。 |
| mmp.c            | 54 | 68 | 282 | Multiple Mount Protection：挂载时检查并周期性更新 MMP block，检测同一文件系统被另一节点并发读写挂载。 |
| ext4_jbd2.h      | 69 | 118 | 274 | ext4/JBD2 适配层声明和大量内联包装：journal credits、buffer write access/dirty/revoke、事务类型及无 journal 情况的统一处理。 |
| block_validity.c | 38 | 51 | 281 | 建立文件系统固定元数据占用的 system-zone 红黑树，并验证 inode 给出的物理块范围，阻止损坏映射覆盖超级块、位图或 inode table。 |
| hash.c           | 36 | 35 | 252 | 计算 HTree 目录索引使用的 legacy、half-MD4、TEA 和 SipHash 文件名 hash，并处理大小端和 signed-hash 兼容性。 |
| verity.c         | 64 | 81 | 246 | ext4 对 fs-verity 的适配：创建/读写 Merkle tree 和 descriptor，管理启用过程，并向 fs-verity 提供数据页访问操作。 |
| acl.c            | 31 | 28 | 245 | POSIX ACL 的读取、缓存、继承和设置，把 ACL 编解码后存入 `system.posix_acl_*` 扩展属性并协调 inode mode。 |
| crypto.c         | 38 | 29 | 181 | ext4 对 fscrypt 的适配：准备加密文件名、读写加密上下文和 nonce、提供加密 salt ioctl，并定义 `fscrypt_operations`。 |
| mballoc.h        | 40 | 76 | 187 | mballoc 私有类型和接口，核心是 allocation context、buddy、预分配空间及 group-info 结构。 |
| xattr.h          | 35 | 40 | 161 | ext4 磁盘 xattr entry/header、查找上下文、namespace handler 和 xattr 核心接口定义。 |
| extents_status.h | 37 | 59 | 158 | extent-status 树节点、状态位编码、pending cluster 树、统计和增删查接口定义。 |
| ext4_extents.h   | 34 | 98 | 148 | extent 磁盘结构、树路径 `ext4_ext_path`、深度/长度编码辅助函数及 extent 树接口。 |
| fast_commit.h    | 23 | 38 | 128 | fast-commit 磁盘 TLV 标签、各类变更记录格式、replay 状态和统计结构定义。 |
| fsync.c          | 26 | 60 | 104 | 实现 ext4 `fsync/fdatasync`：先刷数据和 inode 元数据，再等待 fast/full journal commit；无 journal 时同步父目录并按需发 flush/barrier。 |
| symlink.c        | 16 | 23 | 97 | 普通、fast、inline 和加密符号链接的 `get_link` 及对应 `inode_operations`。 |
| bitmap.c         | 16 | 9 | 74 | 只负责 inode bitmap 和 block bitmap 的 metadata checksum 计算、设置与校验。 |
| acl.h            | 12 | 7 | 55 | POSIX ACL 的 ext4 接口声明；未启用 `CONFIG_EXT4_FS_POSIX_ACL` 时提供空实现。 |
| xattr_security.c | 7 | 5 | 54 | `security.*` xattr namespace handler，并在新 inode 上初始化 LSM 安全扩展属性。 |
| xattr_user.c     | 5 | 7 | 38 | `user.*` xattr namespace handler，受 `user_xattr` 挂载选项控制。 |
| xattr_hurd.c     | 7 | 8 | 37 | `gnu.*` xattr namespace handler，仅在文件系统 creator OS 标记为 Hurd 时允许访问。 |
| fsmap.h          | 10 | 9 | 37 | ext4 内部 fsmap key/head 结构、记录标志和 `ext4_getfsmap()` 接口。 |
| xattr_trusted.c  | 5 | 7 | 35 | `trusted.*` xattr namespace handler，仅特权调用者可见。 |
| truncate.h       | 8 | 26 | 18 | 传统 indirect truncate 使用的事务重启辅助函数，确保 journal credits 不足时安全地续开事务。 |
| Makefile         | 3 | 4 | 14 | 定义 ext4 主模块对象、按 ACL/security/fscrypt/fs-verity 配置加入的可选对象，以及 KUnit 测试模块对象。 |


## ext4_map_blocks 的含义
正如 ext4_map_blocks 注释上所说的

> The ext4_map_blocks() function tries to look up the requested blocks,
> and returns if the blocks are already mapped.

其功能就是获取

```txt
@[
    ext4_map_blocks+5
    ext4_getblk+160
    ext4_bread+15
    __ext4_read_dirblock+82
    ext4_dx_find_entry+271
    __ext4_find_entry+957
    ext4_lookup+152
    __lookup_slow+131
    walk_component+219
    path_lookupat+106
    filename_lookup+242
    vfs_statx+117
    vfs_fstatat+107
    __do_sys_newfstatat+63
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 59797
@[
    ext4_map_blocks+5
    ext4_getblk+160
    ext4_bread+15
    __ext4_read_dirblock+82
    dx_probe+103
    ext4_dx_find_entry+86
    __ext4_find_entry+957
    ext4_lookup+152
    __lookup_slow+131
    walk_component+219
    path_lookupat+106
    filename_lookup+242
    vfs_statx+117
    vfs_fstatat+107
    __do_sys_newfstatat+63
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 59797
```


ext4_map_blocks() 可以理解为 ext4 的“逻辑块映射总入口”：

> 给定文件内的逻辑块范围，查询或创建它对应的磁盘物理块范围。

```c
struct ext4_map_blocks {
	ext4_fsblk_t m_pblk; /* 输出：起始物理块 */
	ext4_lblk_t  m_lblk; /* 输入：起始逻辑块 */
	unsigned int m_len;  /* 输入：最大长度；输出：实际连续长度 */
	unsigned int m_flags;/* 输出：映射状态 */
	u64 m_seq;
};
```

```txt
extent status tree 内存缓存
        │
        ├── 命中 written/unwritten/delayed/hole → 直接返回
        │
        └── 未命中
              │
              ├── i_data_sem 读锁
              ├── 查询磁盘映射
              │     ├── extent inode → ext4_ext_map_blocks()
              │     └── indirect inode → ext4_ind_map_blocks()
              │
              └── 如果指定 CREATE 且需要分配/转换
                    ├── i_data_sem 写锁
                    └── 创建物理映射
```

## CONFIG_EXT4_USE_FOR_EXT2 : kimi

不编译独立的 ext2 驱动，让 ext4 驱动同时负责挂载 ext2 文件系统。

关键配置关系：

CONFIG_EXT4_FS=y/m
CONFIG_EXT2_FS=n
CONFIG_EXT4_USE_FOR_EXT2=y

启用后，ext4 驱动额外注册一个名为 ext2 的文件系统类型。因此：

mount -t ext2 /dev/sdb1 /mnt

实际执行挂载的是 ext4 驱动，内核会输出：

EXT4-fs: mounting ext2 file system using the ext4 subsystem


## SSD 考虑 : kimi

不能笼统地说“已经没有意义”。HDD → SSD 后，ext4 中以减少机械寻道为目的的设计收益明显下降，但大部分核心机制仍然必要。

- 明显弱化：inode 与文件数据放在附近、目录局部性、线性扫描块组、传统 readahead 等。这些主要用于降低磁头寻道。
- 仍然重要：extent、延迟分配、multiblock allocator、预分配和碎片控制。SSD 没有磁头，但连续 I/O 仍能减少请求数量、合并 bio、降低 CPU/FTL 开销和写放大。
  内核文档也明确说明了这一点：Documentation/filesystems/ext4/allocators.rst:6。

- 与介质无关：JBD2 日志、崩溃一致性、元数据校验、orphan recovery、fsync 语义、并发控制。这些解决掉电和系统崩溃，不是解决磁盘寻道：Documentation/
  filesystems/ext4/journal.rst:3。

- SSD 特有需求：discard/TRIM、减少写放大、对 flush/FUA 的正确处理。ext4 已经支持这些机制。

ext4 也不是完全沿用 HDD 参数。它会检查块设备是否为 non-rotational，并针对 SSD 取消为了减少寻道而进行的线性块组搜索：fs/ext4/mballoc.c:213、fs/ext4/
mballoc.c:3843。

真正的问题是：ext4 面向通用块设备和稳定的磁盘格式，并不理解 SSD 内部的 FTL、erase block、GC 和磨损均衡。因此它不是“SSD 原生文件系统”，但也绝不是在 SSD
上失去意义。更准确的说法是：

> SSD 消除了 ext4 局部性设计中的机械寻道收益，但没有消除连续分配、碎片控制、崩溃一致性和可靠持久化的价值。

## ext4_file_vm_ops 中的 filemap_map_pages 和 ext4_page_mkwrite 的作用 : kimi

```c
static const struct vm_operations_struct ext4_file_vm_ops = {
	.fault		= filemap_fault,
	.map_pages	= filemap_map_pages,
	.page_mkwrite   = ext4_page_mkwrite,
};
```


这组 vm_operations_struct 用于普通非 DAX ext4 文件的 mmap()：

.fault        = filemap_fault,
.map_pages    = filemap_map_pages,
.page_mkwrite = ext4_page_mkwrite,

三者可以理解为：

- fault：处理当前缺页。
- map_pages：顺便映射附近已经存在于 page cache 的页面。
- page_mkwrite：MAP_SHARED 页面第一次变为可写前，让 ext4 准备好文件系统状态。

### filemap_map_pages

核心作用是实现 file-backed mmap 的 fault-around 优化。

普通读缺页路径首先尝试：

page fault
  -> do_read_fault()
     -> do_fault_around()
        -> vm_ops->map_pages()
           -> filemap_map_pages()

参见 mm/memory.c:5713 和 mm/filemap.c:3872。

它会在 fault 地址附近的一段文件偏移中遍历 mapping->i_pages，把满足条件的 folio 一次性安装进页表：

- 已经存在于 page cache。
- uptodate。
- 当前没有被锁住。
- 没有超出 i_size。
- 对应 PTE 仍为空。
- 不是 HWPoison 页面。

主要价值是减少顺序访问文件时的缺页次数和页表锁开销。例如访问 page 10 时，可以顺便映射 page 8～15；之后访问这些页面不再产生缺页。

它不会：

- 从磁盘读取缺失页面。
- 等待被锁住的 folio。
- 为 hole 分配磁盘块。
- 处理 ext4 日志事务。

如果目标页面不在 page cache，或者尚未准备好，就跳过它；当前 fault 最终回退到 filemap_fault()，由后者读取数据。

因此两者关系是：

filemap_map_pages = page-cache 命中时的批量快速路径
filemap_fault     = 当前 fault 的完整兜底路径，必要时发起 I/O

### ext4_page_mkwrite

它处理可写 MAP_SHARED 映射中的写 fault：

mmap(..., PROT_READ | PROT_WRITE, MAP_SHARED, ...);
p[0] = 'x';

文件页通常先以只读 PTE 映射。第一次写入触发保护异常：

write fault
  -> wp_page_shared() / do_shared_fault()
     -> do_page_mkwrite()
        -> ext4_page_mkwrite()
     -> finish_mkwrite_fault()
        -> 将 PTE 设置为 writable + dirty

参见 mm/memory.c:3972 和 fs/ext4/inode.c:6643。

ext4_page_mkwrite() 必须在 CPU 真正修改页面之前完成以下工作：

1. 与文件系统冻结协调

   sb_start_pagefault() 防止 page fault 与文件系统 freeze 冲突。

2. 更新时间

   file_update_time() 更新文件的 mtime/ctime。

3. 与 truncate、hole punch 等操作同步

   获取 filemap_invalidate_lock_shared(mapping)，并在 folio lock 下重新检查：
    - folio 是否仍属于该 inode。
    - 页面是否仍位于当前 i_size 内。

4. 处理 inline data

   必要时调用 ext4_convert_inline_data()，将 inode 内联数据转换成普通块映射。

5. 为 mmap 写入准备块映射

   mmap 写操作绕过普通的 write()/write_begin() 路径，因此这里必须保证页面对应的文件块可写：
    - delalloc：通过 ext4_da_get_block_prep() 建立延迟分配状态。
    - 非 delalloc：必要时真正分配磁盘块。
    - dioread_nolock：可能创建 unwritten extent。
    - data journaling：启动日志事务并登记页面。
    - 遇到 ENOSPC 时执行 ext4 的分配重试逻辑。

6. 等待正在进行的 I/O 稳定

   folio_wait_stable() 防止 CPU 在页面仍被写回时修改内容。

成功返回时 folio 保持锁定，并返回 VM_FAULT_LOCKED。随后通用 MM 代码才把 PTE 设置为可写并将 folio 标脏。

### 为什么必须有 page_mkwrite

假设一个 sparse 文件的某页是 hole：

磁盘上没有块
page cache 中读出来是全零页

进程通过 MAP_SHARED 写这个页面时，如果 MM 直接将 PTE 改成可写，CPU 可以修改内存，但 ext4 尚未：

- 分配或预留磁盘块。
- 建立 extent。
- 获取日志 credits。
- 棘入 data-journal 事务。
- 检查 ENOSPC。

之后写回可能无法正确持久化。ext4_page_mkwrite() 正是 mmap 写路径进入 ext4 块分配和日志机制的关口。

它主要针对 MAP_SHARED：

- MAP_SHARED：修改必须写回原文件，因此调用 ext4_page_mkwrite()。
- MAP_PRIVATE：写入走匿名页 COW，不修改 ext4 文件，通常不调用它。

另外，它并非每次内存写入都会调用。通常只在页面从只读 PTE 转换为可写 PTE时调用；转换完成后，后续写入不再 fault，除非 PTE 后来再次被写保护。


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
