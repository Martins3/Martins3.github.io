# 为什么设计一个文件系统是很难的

## 基本考虑
锁机制
缓存
多进程

## 基本功能
1. O_DIRECT
2. fs freeze 一致性快照
3. journal 错误恢复
4. 容忍磁盘错误
5. 软硬链接
6. overlay

## 高级功能
1. bcache
2. raid : zfs 的 raid 相较于 linux 中 ext4 + raid 有什么好处吗?
3. cow
5. 压缩
6. 加密
8. snapshot
6. 动态扩容，缩容

## 基本使用
- https://bcachefs.org/bcachefs-principles-of-operation.pdf
- Documentation/admin-guide/ext4.rst

## 想象
zfs 和 bcachefs 的分析中，才发现很多 block layer 的功能都是集成到文件系统中。

## 看看 https://bcachefs.org/ 的 feature 和 plans 就很牛逼了

1. reflink : 俗称 cow ，但是似乎也不是 cow
https://unix.stackexchange.com/questions/393305/does-any-file-system-implement-copy-on-write-mechanism-for-cp

- https://bcachefs.org/Wishlist/
- https://bcachefs.org/Roadmap/

man ext4(5) 也是不错的

## 有趣的 fs
- nilfs : 连续 https://news.ycombinator.com/item?id=33162259
  - https://dataswamp.org/~solene/2022-10-05-linux-nilfs-filesystem.html

## bcachefs 这也太慢了吧!

```txt
🧀  fio /home/martins3/core/vn/docs/kernel/code/aio/bcachefs.fio
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=128
fio-3.36
Starting 1 process
trash: Laying out IO file (1 file / 10240MiB)
Jobs: 1 (f=1): [r(1)][1.6%][r=1172KiB/s][r=293 IOPS][eta 16m:24s]
```

真实运行的时候也太慢了，哈哈!

```txt
🧀  fio /home/martins3/core/vn/docs/kernel/code/aio/bcachefs.fio
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=128
fio-3.36
Starting 1 process
kJobs: 1 (f=1): [r(1)][5.9%][r=504KiB/s][r=126 IOPS][eta 15m:41s]s]
```

但是，实际上 grep 之类的，速度还是很快的!


## 几个虚拟机文件系统让事情变的有趣

initramfs nofs 之类的

## 工业环境中几个文件系统让事情真的难受起来了

- fuse
	- virtiofs
- nfs
- overlayfs


## read-only on io error

当发生 io 错误后，将盘变为只读的:

```txt
[ 1949.245253] Aborting journal on device vda2-8.
[ 1949.245487] I/O error, dev vda, sector 89368576 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245487] EXT4-fs error (device vda2): ext4_journal_check_start:84: comm zsh: Detected aborted journal
[ 1949.245891] I/O error, dev vda, sector 7917976 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245894] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989747)
[ 1949.245903] Buffer I/O error on device vda2, logical block 861491
[ 1949.245909] I/O error, dev vda, sector 7916488 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245910] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989561)
[ 1949.245912] Buffer I/O error on device vda2, logical block 861305
[ 1949.245914] I/O error, dev vda, sector 7915656 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245915] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989457)
[ 1949.245916] Buffer I/O error on device vda2, logical block 861201
[ 1949.245917] I/O error, dev vda, sector 7914000 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245919] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989250)
[ 1949.245920] Buffer I/O error on device vda2, logical block 860994
[ 1949.245921] I/O error, dev vda, sector 7913520 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245922] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989190)
[ 1949.245923] Buffer I/O error on device vda2, logical block 860934
[ 1949.245925] I/O error, dev vda, sector 7913224 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245926] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989153)
[ 1949.245927] Buffer I/O error on device vda2, logical block 860897
[ 1949.245928] I/O error, dev vda, sector 7912928 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245929] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989116)
[ 1949.245930] Buffer I/O error on device vda2, logical block 860860
[ 1949.245932] I/O error, dev vda, sector 7909216 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245933] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 988652)
[ 1949.245934] Buffer I/O error on device vda2, logical block 860396
[ 1949.245935] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 988640)
[ 1949.245936] Buffer I/O error on device vda2, logical block 860384
[ 1949.245938] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 986359)
[ 1949.245939] Buffer I/O error on device vda2, logical block 858103
[ 1949.245974] EXT4-fs error (device vda2): ext4_journal_check_start:84: comm journal-offline: Detected aborted journal
[ 1949.246061] Buffer I/O error on dev vda2, logical block 11042816, lost sync page write
[ 1949.246085] JBD2: I/O error when updating journal superblock for vda2-8.
[ 1949.256645] Buffer I/O error on dev vda2, logical block 0, lost sync page write
[ 1949.256971] EXT4-fs (vda2): I/O error while writing superblock
[ 1949.256976] EXT4-fs (vda2): previous I/O error to superblock detected
[ 1949.257188] EXT4-fs (vda2): Remounting filesystem read-only
[ 1949.257972] Buffer I/O error on dev vda2, logical block 0, lost sync page write
[ 1949.258361] EXT4-fs (vda2): I/O error while writing superblock
```

## kimi : 各个 fs 的功能对比


本文对比 ext4、XFS、Btrfs 和 F2FS。它们都是可作为普通本地数据盘或根文件系统使用的通用文件系统。网络、集群、内存、只读压缩和其他特殊用途文件系统不在本文范围内。

本文基于 Linux v7.0.1 源码。“是”表示内核中有正式实现，但通常仍要求 mkfs 特性、挂载选项、Kconfig、用户态工具或底层设备配合；“有限”表示只覆盖部分对象或存在重要限制；“否”表示文件系统本身没有该能力，不能用 LVM、dm-crypt、块层 RAID 等下层能力代替回答。

- ext4：`fs/ext4/Kconfig`、`fs/ext4/file.c`、`fs/ext4/ioctl.c`、`fs/ext4/super.c`、`Documentation/filesystems/ext4/checksums.rst` 和 `Documentation/filesystems/ext4/atomic_writes.rst`。
- XFS：`fs/xfs/Kconfig`、`fs/xfs/xfs_file.c`、`fs/xfs/xfs_reflink.c`、`fs/xfs/xfs_fsops.c` 和 `Documentation/filesystems/xfs/xfs-online-fsck-design.rst`。
- Btrfs：`fs/btrfs/Kconfig`、`fs/btrfs/reflink.c`、`fs/btrfs/ioctl.c`、`fs/btrfs/scrub.c`、`fs/btrfs/volumes.c` 和 `Documentation/filesystems/btrfs.rst`。
- F2FS：`fs/f2fs/Kconfig`、`fs/f2fs/file.c`、`fs/f2fs/gc.c`、`fs/f2fs/super.c`、`fs/f2fs/verity.c` 和 `Documentation/filesystems/f2fs.rst`。

| 功能 | ext4 | XFS | Btrfs | F2FS |
|---|---|---|---|---|
| 主要写入模型 | 原地更新；JBD2 日志 | 原地更新；元数据日志 | 数据和元数据 CoW | LFS 异地更新、checkpoint |
| 原生 CoW | 否 | 有限：reflink 共享 extent | 是；文件可设置 NOCOW | 否；异地更新不等于可共享 CoW |
| reflink / extent 去重 | 否 | 是；要求 reflink 格式特性 | 是 | 否 |
| 子卷 / 快照 | 否 | 否 | 是；可写或只读快照 | 否 |
| 透明文件压缩 | 否 | 否 | 是；zlib、LZO、Zstd | 是；LZO、LZO-RLE、LZ4、LZ4HC、Zstd |
| 数据校验和 | 否 | 否 | 是；普通 CoW 数据默认校验 | 有限：可校验压缩 cluster，不覆盖普通文件数据 |
| 元数据校验和 | 是；`metadata_csum` | 是；V5/`crc=1` 格式 | 是 | 有限；superblock、checkpoint、inode 等分项特性 |
| 在线 scrub | 否 | 是；依赖 `CONFIG_XFS_ONLINE_SCRUB` | 是；有冗余副本时可修复校验错误 | 否 |
| 在线元数据修复 | 否 | 是；覆盖范围依格式和内核实现 | 有限；scrub 可借助冗余修复，不等同完整 fsck | 否 |
| fscrypt 文件级加密 | 是 | 否 | 否 | 是 |
| fs-verity | 是 | 否 | 是 | 是 |
| Unicode 目录 casefold | 是；按目录 | 否；旧 ASCII-CI 格式已废弃 | 否 | 是；按目录 |
| 用户/组/项目配额 | 是 / 是 / 是 | 是 / 是 / 是 | qgroup/simple quota；不是传统三类配额 | 是 / 是 / 是 |
| 多设备数据存储 | 否；可用独立 journal 设备 | 有限；data/log/realtime 设备分工 | 是 | 是；线性地址空间，不提供文件系统 RAID |
| 内建 RAID | 否 | 否 | 是；含 RAID0/1/10/5/6 等 profile | 否 |
| 在线扩容 | 是；`bigalloc` 有限制 | 是 | 是 | 否；当前 ioctl 拒绝大于原大小的请求 |
| 在线缩容 | 否 | 有限；实验性，只能缩最后一个 AG，不能删除整个 AG | 是；迁移待裁区域中的 chunk | 是；按 section 缩小，过程会短暂 freeze |
| DAX | 是 | 是 | 否 | 否 |
| 原生 zoned block 支持 | 否 | 有限；实验性，使用 realtime 设备 | 是；部分 RAID/profile 组合受限 | 是 |

### “ out-of-place write”
ext4 : 普通文件数据最终覆盖自己的 extent，崩溃一致性由 JBD2、ordered writeback 等机制保证。它没有 `remap_file_range` 文件操作，因此不支持 `FICLONE` reflink。数据也不会因为写入而自动保留旧版本。
XFS : 默认仍是原地更新文件系统。启用 on-disk `reflink` 特性后，clone 出来的文件共享 extent；任一方写共享范围时进入 CoW staging fork，再把新 extent remap 回文件。这个 CoW 是维持共享 extent 语义的机制，不是全文件系统历史版本或快照机制。
Btrfs : extent、tree block 和 root 都围绕 CoW 设计，所以 reflink、子卷快照、校验和和多设备映射是同一套引用/回引用基础设施上的能力。`NOCOW` 文件会放弃普通数据 CoW，并通常同时失去数据校验和；快照中已共享的 extent 仍必须 CoW。
F2FS : F2FS 为适应 flash，采用 log-structured allocation，更新后的 block 通常写到新位置，再由 NAT/SIT/checkpoint 和 roll-forward 恢复状态。但它没有 VFS `remap_file_range` 实现、共享 extent 引用计数或快照 root，因此不能把“out-of-place update”记成用户可见的 CoW/reflink 功能。

### 完整性与恢复边界

- ext4 和 XFS 的日志首先解决元数据事务一致性，不等于端到端文件数据校验。ext4 可以选择 `data=journal`，但这仍不是持久化的数据 checksum。
- Btrfs 对元数据和普通 CoW 文件数据保存 checksum；scrub 会读取并核验数据，有 RAID1/10 等正确副本时可以回写修复。`NODATASUM`/`NOCOW` 是例外。
- F2FS 的 `inode_checksum`、`sb_checksum` 以及 checkpoint 校验不构成全文件数据校验。压缩 cluster 的 checksum 也只能保护启用该选项的压缩数据。
- XFS online scrub/repair 由独立 Kconfig 控制。online repair 依赖 rmapbt、parent pointer 等冗余元数据，不能简单视为在所有格式上完全替代 `xfs_repair`。

### 原子写语义

“atomic write”至少有两种不同接口，不应合并成一个是/否项：

- ext4 和 XFS 支持 `pwritev2(RWF_ATOMIC)`。ext4 依赖块设备 atomic-write 能力；当前 XFS 还可以在启用 reflink 的格式上使用软件 CoW 路径。实际范围应通过 `statx` 的 atomic-write 字段查询。
- F2FS 提供 `F2FS_IOC_START_ATOMIC_WRITE`、`COMMIT` 和 `ABORT`，把一个文件的一组更新作为事务提交。这不是同一个 VFS `RWF_ATOMIC` ABI。
- 当前 Btrfs 没有上述两类 atomic-write 文件接口。

### 总结
- ext4 适合功能要求常规、重视兼容性和运维简单性的根盘与数据盘。需要快照、压缩、reflink 或数据校验时，应由其他层补齐或改选文件系统。
- XFS 适合大容量、高并发、持续扩容和在线检查场景。reflink、项目配额和 online scrub/repair 是相对 ext4 的主要增量；它不提供内建快照、压缩和 fscrypt。
- Btrfs 适合明确需要快照、reflink、压缩、端到端校验或文件系统级多设备管理的场景。代价是 CoW、空间回收、qgroup 和 RAID profile 带来更多策略与运维复杂度。
- F2FS 适合 flash/UFS/NVMe 和写入模式可从 LFS 清理机制获益的场景，尤其需要 fscrypt、压缩或 F2FS 文件事务接口时。它不是带快照和 reflink 的 CoW 文件系统，多设备能力也不是 Btrfs 式 RAID。

## 经典
https://www.reddit.com/r/bcachefs/comments/1ux3mmv/benchmarking_what_modern_filesystems_promise/

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
