# ext4

## 有趣
- Visualizing Ext4
  - https://news.ycombinator.com/item?id=38907821


> 主要扩展
1. Delayed allocation
2. extents
3. multiblock

## Ext4 Disk Layout
https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout
https://ext4.wiki.kernel.org/index.php/Frequently_Asked_Questions

> 难道真的使用的是 bitmap 吗?

> boot sector
> group ? inode table ?



```c
// inode.c

/*
 * The ext4_map_blocks() function tries to look up the requested blocks,
 * and returns if the blocks are already mapped.
 *
 * Otherwise it takes the write lock of the i_data_sem and allocate blocks
 * and store the allocated blocks in the result buffer head and mark it
 * mapped.
 *
 * If file type is extents based, it will call ext4_ext_map_blocks(),
 * Otherwise, call with ext4_ind_map_blocks() to handle indirect mapping
 * based files
 *
 * On success, it returns the number of blocks being mapped or allocated.  if
 * create==0 and the blocks are pre-allocated and unwritten, the resulting @map
 * is marked as unwritten. If the create == 1, it will mark @map as mapped.
 *
 * It returns 0 if plain look up failed (blocks have not been allocated), in
 * that case, @map is returned as unmapped but we still do fill map->m_len to
 * indicate the length of a hole starting at map->m_lblk.
 *
 * It returns the error in case of allocation failure.
 */
int ext4_map_blocks(handle_t *handle, struct inode *inode,
		    struct ext4_map_blocks *map, int flags)
```
One caller of `ext4_map_blocks` is ext4_get_block


Although aops::write_begin will write page to disk, but it will write journal to disk, analyzing `__block_write_begin` will verify this.
`__generic_file_write_iter` ==> generic_perform_write ==> address_space_operations::write_begin == ext4_write_begin ==> `__block_write_begin`

## overview
| File             | blank | comment | code | explanation                                                                                                                             |
|------------------|-------|---------|------|-----------------------------------------------------------------------------------------------------------------------------------------|
| super.c          | 632   | 692     | 4695 |                                                                                                                                         |
| inode.c          | 671   | 1525    | 4067 | @todo 看似熟悉的文件名，其实完全不知道在说什么!                                                                                         |
| extents.c        | 648   | 1319    | 3993 | 使用首尾顺序 direct indirect 逐个分析描述                                                                                                |
| mballoc.c        | 639   | 1139    | 3577 | http://oenhan.com/ext4-mballoc @todo 还是非常难以理解!                                                                                  |
| namei.c          | 369   | 432     | 3059 | ext4_dir_inode_operations 和　ext4_special_inode_operations  定义，还是处理 link path 之类的问题                                         |
| xattr.c          | 362   | 395     | 2380 |                                                                                                                                         |
| ext4.h           | 340   | 611     | 2294 |                                                                                                                                         |
| inline.c         | 294   | 189     | 1546 | 似乎 inline 含义?                                                                                                                        |
| resize.c         | 260   | 315     | 1497 | Support for resizing an ext4 filesystem while it is mounted. This could probably be made into a module, because it is not often in use. |
| ialloc.c         | 151   | 243     | 1051 |                                                                                                                                         |
| ioctl.c          | 152   | 88      | 872  |                                                                                                                                         |
| extents_status.c | 152   | 251     | 851  | extents_status 为 extent 服务，不是很懂为什么会出现 extent 内容                                                                          |
| indirect.c       | 116   | 534     | 806  | 处理 indirect 的内容                                                                                                                      |
| balloc.c         | 90    | 207     | 605  | 处理 bitmap 的相关的内容，@todo 但是具体作用不知道                                                                                        |
| fsmap.c          | 96    | 122     | 495  |                                                                                                                                         |
| dir.c            | 63    | 116     | 485  |                                                                                                                                         |
| move_extent.c    | 59    | 157     | 479  |                                                                                                                                         |
| migrate.c        | 68    | 143     | 462  |                                                                                                                                         |
| page-io.c        | 54    | 68      | 415  |                                                                                                                                         |
| sysfs.c          | 62    | 10      | 378  |                                                                                                                                         |
| file.c           | 66    | 87      | 371  | ext4_file_operations 和 ext4_file_inode_operations 两个结构体的定义                                                                     |
| mmp.c            | 57    | 64      | 275  |                                                                                                                                         |
| ext4_jbd2.h      | 68    | 123     | 271  |                                                                                                                                         |
| ext4_jbd2.c      | 53    | 35      | 244  |                                                                                                                                         |
| acl.c            | 30    | 28      | 237  |                                                                                                                                         |
| readpage.c       | 23    | 55      | 217  |                                                                                                                                         |
| hash.c           | 29    | 35      | 204  |                                                                                                                                         |
| block_validity.c | 26    | 23      | 194  |                                                                                                                                         |
| xattr.h          | 34    | 32      | 146  |                                                                                                                                         |
| mballoc.h        | 32    | 53      | 133  |                                                                                                                                         |
| ext4_extents.h   | 36    | 95      | 132  |                                                                                                                                         |
| extents_status.h | 30    | 20      | 128  |                                                                                                                                         |
| fsync.c          | 14    | 65      | 85   |                                                                                                                                         |
| bitmap.c         | 17    | 9       | 72   |                                                                                                                                         |
| acl.h            | 13    | 6       | 54   |                                                                                                                                         |
| xattr_security.c | 7     | 5       | 53   |                                                                                                                                         |
| symlink.c        | 8     | 19      | 47   |                                                                                                                                         |
| xattr_user.c     | 5     | 7       | 37   |                                                                                                                                         |
| fsmap.h          | 10    | 9       | 37   |                                                                                                                                         |
| xattr_trusted.c  | 5     | 7       | 34   |                                                                                                                                         |
| truncate.h       | 7     | 26      | 17   |                                                                                                                                         |
| Makefile         | 3     | 4       | 8    |                                                                                                                                         |

> 感觉 vfs 中间实现的和此处耦合的很重

> simple 从 database 的角度看 linux io stack
https://www.postgresql.eu/events/fosdem2019/sessions/session/2346/slides/159/fosdem_linux_io.pdf

1. 总结一下 ext4 和 blk layer 以及 vfs layer 之间的关系是什么 ?
2. chicken eggs(显然 ucore 已经帮我们处理好了)
    1. 通过文件系统找到对应的设备
    2. 文件系统首先需要挂载到设备上去


## http://oenhan.com/ext4-mballoc
buddy 什么情况 ?

## https://opensource.com/article/17/5/introduction-ext4-filesystem
In EXT4, data allocation was changed from fixed blocks to extents.
An extent is described by its starting and ending place on the hard drive.
This makes it possible to describe very long, physically contiguous files in a single inode pointer entry, which can significantly reduce the number of pointers required to describe the location of all the data in larger files.

EXT4 reduces fragmentation by scattering newly created files across the disk so that they are not bunched up in one location at the beginning of the disk, as many early PC filesystems did.

Aside from the actual location of the data on the disk, EXT4 uses functional strategies, such as delayed allocation, to allow the filesystem to collect all the data being written to the disk before allocating space to it. This can improve the likelihood that the data space will be contiguous.


## 分析 ext4 的错误处理路径

```txt
sudo umount /mnt
sudo rmmod scsi_debug || true
sudo modprobe scsi_debug
sudo mkfs.ext4 /dev/sdc

sudo mount /dev/sdc /mnt
sudo chown -R  martins3 /mnt

echo 30000000 | sudo tee /sys/module/scsi_debug/parameters/delay
echo a > /mnt/a
```

ext4_shutdown 和 ext4_force_shutdown 都没有触发

```txt
@[
    ext4_handle_error+1
    __ext4_error+298
    ext4_journal_check_start+130
    __ext4_journal_start_sb+63
    ext4_dirty_inode+56
    __mark_inode_dirty+87
    touch_atime+415
    iterate_dir+272
    __x64_sys_getdents64+136
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1
@[
    ext4_handle_error+1
    __ext4_error+298
    ext4_read_inode_bitmap+960
    __ext4_new_inode+935
    ext4_create+297
    path_openat+3750
    do_filp_open+179
    do_sys_openat2+171
    __x64_sys_openat+110
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1
@[
    ext4_handle_error+1
    __ext4_error+298
    ext4_wait_block_bitmap+156
    ext4_mb_init_cache+397
    ext4_mb_init_group+238
    ext4_mb_prefetch_fini+125
    ext4_lazyinit_thread+1071
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 1
@[
    ext4_handle_error+1
    __ext4_error+298
    __ext4_journal_get_write_access+316
    ext4_init_inode_table+456
    ext4_lazyinit_thread+952
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 1
```
一次 io 错误，几乎必然可以导致系统挂掉。
只要打开了文件，不进行任何的 io ，就会 dirty io

系统中总是有人在执行各种程序，打开程序之类，必然导致写 inode 的。

看来触发的位置还是很多的。

ext4_handle_error 中

## bh ?
echo a > /mnt/a 形成的这个路径，ext4_read_bh 是在等待谁?
```txt
🤒  sudo cat /proc/450025/stack
[sudo] password for martins3:
[<0>] ext4_read_bh+0x80/0x90 [ext4]
[<0>] ext4_read_inode_bitmap+0x38a/0x530 [ext4]
[<0>] __ext4_new_inode+0x3a7/0x16f0 [ext4]
[<0>] ext4_create+0x129/0x210 [ext4]
[<0>] path_openat+0xea6/0x1160
[<0>] do_filp_open+0xb3/0x160
[<0>] do_sys_openat2+0xab/0xe0
[<0>] __x64_sys_openat+0x6e/0xa0
[<0>] do_syscall_64+0x3b/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x6e/0xd8
```

cat /mnt/a 会得到如下的等待:
```txt
🤒  sudo cat /proc/454840/stack
[sudo] password for martins3:
[<0>] folio_wait_bit_common+0x13d/0x350
[<0>] filemap_get_pages+0x5ff/0x630
[<0>] filemap_read+0xd9/0x350
[<0>] vfs_read+0x1fe/0x350
[<0>] ksys_read+0x6f/0xf0
[<0>] do_syscall_64+0x3b/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x6e/0xd8
```

## 对比分析 ext4 中的数据 layout 和 swap

为什么一个简单的 fs 还有这么多

## 搞一个各个 fs 的功能对比

## 基本的如何使用也是需要看看的
https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/8/html/managing_file_systems/getting-started-with-an-ext4-file-system_managing-file-systems#comparison-of-tools-used-with-ext4-and-xfs_getting-started-with-an-ext4-file-system

## debugfs 是 ext4 的调试工具
https://man7.org/linux/man-pages/man8/debugfs.8.html

## 分析  hook 中 filemap_map_pages 和 ext4_page_mkwrite 是如何实现的
```c
static const struct vm_operations_struct ext4_file_vm_ops = {
	.fault		= filemap_fault,
	.map_pages	= filemap_map_pages,
	.page_mkwrite   = ext4_page_mkwrite,
};
```

## 为什么在 cgroup 下 fio 会卡主
```sh
   cgexec -g memory:system.slice/irqbalance.service fio a.fio
```

```txt
[root@node120 17:39:17 tools]$ cat /proc/4644/stack
[<0>] call_rwsem_down_write_failed+0x13/0x20
[<0>] ext4_map_blocks+0xec/0x570 [ext4]
[<0>] ext4_writepages+0x8d7/0xea0 [ext4]
[<0>] do_writepages+0x4b/0xe0
[<0>] __filemap_fdatawrite_range+0xcf/0x110
[<0>] generic_fadvise+0x21f/0x230
[<0>] ksys_fadvise64_64+0x3c/0x70
[<0>] __x64_sys_fadvise64+0x1a/0x20
[<0>] do_syscall_64+0x61/0x250
[<0>] entry_SYSCALL_64_after_hwframe+0x44/0xa9
[<0>] 0xffffffffffffffff
```

 /sys/fs/cgroup/memory/system.slice/irqbalance.service

## 有一部分还是在使用 buffer head

```txt
@[
    loop_queue_rq+5
    __blk_mq_issue_directly+72
    blk_mq_plug_issue_direct+110
    blk_mq_flush_plug_list+1571
    __blk_flush_plug+214
    __submit_bio+327
    submit_bio_noacct_nocheck+742
    ext4_read_bh_lock+62
    ext4_bread_batch+222
    __ext4_find_entry+389
    ext4_lookup+152
    path_openat+1703
    do_filp_open+216
    do_sys_openat2+171
    __x64_sys_openat+87
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    loop_queue_rq+5
    __blk_mq_issue_directly+72
    blk_mq_plug_issue_direct+110
    blk_mq_flush_plug_list+1571
    __blk_flush_plug+214
    __submit_bio+327
    submit_bio_noacct_nocheck+742
    ext4_read_bh+89
    ext4_read_inode_bitmap+842
    __ext4_new_inode+786
    ext4_create+278
    path_openat+4081
    do_filp_open+216
    do_sys_openat2+171
    __x64_sys_openat+87
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

### [ ] 那些部分已经被修改为使用 iomap 的?

猜测是类似这种，当 io 的内容不是 metadata 的时候，那么就走这些结果:
```txt
@[
    submit_bio+5
    iomap_dio_bio_iter+889
    __iomap_dio_rw+1112
    iomap_dio_rw+18
    xfs_file_dio_read+185
    xfs_file_read_iter+188
    aio_read+308
    io_submit_one+392
    __x64_sys_io_submit+149
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 3427617
```

```txt
@[
    submit_bio+5
    iomap_dio_bio_iter+889
    __iomap_dio_rw+1112
    iomap_dio_rw+18
    ext4_file_read_iter+326
    aio_read+308
    io_submit_one+392
    __x64_sys_io_submit+149
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 619060
```

## ext4_map_blocks 的含义
正如 ext4_map_blocks 注释上所说的

> The ext4_map_blocks() function tries to look up the requested blocks,
> and returns if the blocks are already mapped.

这个是如果 map 了，那么就使用去调用 ext4_map_blocks

之前还以为这个是表示用于在使用 iomap 的。

两个经典的 backtrace 如下:
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


## 有趣的啊
CONFIG_EXT4_USE_FOR_EXT2

## ext4 的那些设计是为了磁盘而设计的，现在已经没有意义了

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
