# ext4 基本使用

## ext4 的参数
- Documentation/admin-guide/ext4.rst
  - https://www.kernel.org/doc/html/latest/admin-guide/ext4.html

## https://github.com/tytso/e2fsprogs
这个项目是 e2fsprogs，Linux 上管理 ext2/ext3/ext4 文件系统的核心工具集。

主要包含这些常用工具：
- mke2fs / mkfs.ext4 — 创建 ext 文件系统
- e2fsck / fsck.ext4 — 检查并修复文件系统错误
- tune2fs — 调整文件系统参数（如 label、UUID、日志模式等）
- resize2fs — 在线或离线调整文件系统大小
- debugfs — 文件系统调试器，可手动查看/修改 ext 元数据
- dumpe2fs — 显示文件系统的超级块、块组等信息
- badblocks — 扫描磁盘坏块
- chattr / lsattr — 修改/查看 ext 文件属性
- e2image — 备份/恢复文件系统元数据

此外还包含底层库 libext2fs、UUID 库 libuuid、块 ID 库 libblkid
等，是很多分区、挂载、文件系统管理工具依赖的基础组件。简单说，Linux
上只要用到 ext4，基本都离不开这个包。

### fuse2fs

### dumpe2fs

```txt
🧀  sudo dumpe2fs -h  /dev/mapper/openeuler-home
dumpe2fs 1.47.0 (5-Feb-2023)
Filesystem volume name:   <none>
Last mounted on:          /home
Filesystem UUID:          8f296976-c0a9-4d9b-9bcf-87adbf31702c
Filesystem magic number:  0xEF53
Filesystem revision #:    1 (dynamic)
Filesystem features:      has_journal ext_attr resize_inode dir_index filetype needs_recovery extent 64bit flex_bg sparse_super large_file huge_file dir_nlink extra_isize metadata_csum
Filesystem flags:         signed_directory_hash
Default mount options:    user_xattr acl
Filesystem state:         clean
Errors behavior:          Continue
Filesystem OS type:       Linux
Inode count:              7905280
Block count:              31596544
Reserved block count:     1579827
Overhead clusters:        642697
Free blocks:              25268284
Free inodes:              7841947
First block:              0
Block size:               4096
Fragment size:            4096
Group descriptor size:    64
Reserved GDT blocks:      1024
Blocks per group:         32768
Fragments per group:      32768
Inodes per group:         8192
Inode blocks per group:   512
Flex block group size:    16
Filesystem created:       Fri Jan 10 12:57:09 2025
Last mount time:          Thu Jan  1 08:00:10 1970
Last write time:          Thu Jan  1 08:00:10 1970
Mount count:              328
Maximum mount count:      -1
Last checked:             Fri Jan 10 12:57:09 2025
Check interval:           0 (<none>)
Lifetime writes:          5662 GB
Reserved blocks uid:      0 (user root)
Reserved blocks gid:      0 (group root)
First inode:              11
Inode size:               256
Required extra isize:     32
Desired extra isize:      32
Journal inode:            8
Default directory hash:   half_md4
Directory Hash Seed:      a29de8e6-8986-4228-bb69-fdf86e8be41e
Journal backup:           inode blocks
Checksum type:            crc32c
Checksum:                 0x904b214f
Journal features:         journal_incompat_revoke journal_64bit journal_checksum_v3
Total journal size:       512M
Total journal blocks:     131072
Max transaction length:   131072
Fast commit length:       0
Journal sequence:         0x00005dd9
Journal start:            119226
Journal checksum type:    crc32c
Journal checksum:         0x02a3fad1
```

类似的:
sudo tune2fs -l /dev/mapper/openeuler-home

## 工具 debugfs

debugfs 是 ext4 的调试工具
https://man7.org/linux/man-pages/man8/debugfs.8.html

## sysfs 和 procfs 接口

1. /proc/fs/ext4/vda1
```txt
es_shrinker_info  fc_info  mb_groups  mb_stats  mb_structs_summary  options
```

2. /sys/fs/ext4

具体说明参考 Documentation/admin-guide/ext4.rst

```txt
[root@debug-ext4 14:26:19 features]$ ls
batched_discard  fast_commit  lazy_itable_init  meta_bg_resize  metadata_csum_seed
```

```txt
delayed_allocation_blocks  inode_goal             max_writeback_mb_bump         msg_count
errors_count               inode_readahead_blks   mb_best_avail_max_trim_order  msg_ratelimit_burst
err_ratelimit_burst        journal_task           mb_group_prealloc             msg_ratelimit_interval_ms
err_ratelimit_interval_ms  last_error_block       mb_max_linear_groups          reserved_clusters
extent_max_zeroout_kb      last_error_errcode     mb_max_to_scan                session_write_kbytes
first_error_block          last_error_func        mb_min_to_scan                sra_exceeded_retry_limit
first_error_errcode        last_error_ino         mb_order2_req                 trigger_fs_error
first_error_func           last_error_line        mb_prefetch                   warning_count
first_error_ino            last_error_time        mb_prefetch_limit             warning_ratelimit_burst
first_error_line           last_trim_minblks      mb_stats                      warning_ratelimit_interval_ms
first_error_time           lifetime_write_kbytes  mb_stream_req
```

这里含义，就没必要一个一个的看了，看名称大致就知道了。

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
