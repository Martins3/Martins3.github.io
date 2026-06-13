## [ ] 如何改造 iomap_get_block

基本的调用路径：
```txt
simple_fs_iomap_begin+469
iomap_iter+299
iomap_file_buffered_write+155
simple_fs_file_write_iter+89
vfs_write+673
ksys_write+110
do_syscall_64+188
```

page cache 都是文件系统中的一个 page 的 cache ，但是如果是在加载元数据的时候，例如 inode 的时候，
也是需要获取到一个 cache 来缓存这个数据的


为什么不去直接用这种方法？
获取元数据都是利用 block 的 pagecache 的
```c
void erofs_init_metabuf(struct erofs_buf *buf, struct super_block *sb)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);

	buf->file = NULL;
	if (erofs_is_fileio_mode(sbi)) {
		buf->file = sbi->dif0.file;	/* some fs like FUSE needs it */
		buf->mapping = buf->file->f_mapping;
	} else if (erofs_is_fscache_mode(sb))
		buf->mapping = sbi->dif0.fscache->inode->i_mapping;
	else
		buf->mapping = sb->s_bdev->bd_mapping;
}
```

似乎 ext2 也是这种，采用 read_mapping_folio 的实现方法来实现
读取 dir 文件，dir 文件通过 buffer io 来:
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
这里希望获取到文件系统中的，那么只有两个数值，

## inode 的信息的加载

### 写回到内存中 : simplefs_write_inode

simplefs_inode_info 是内存中的 inode :
```c
struct simplefs_inode_info {
	uint32_t ei_block; /* Block with list of extents for this file */
	char i_data[32];
	struct inode vfs_inode;
};
```

```txt
@[
    simplefs_write_inode+5
    __writeback_single_inode+668
    writeback_single_inode+175
    sync_inode_metadata+71
    __generic_file_fsync+114
    generic_file_fsync+22
    do_fsync+57
    __x64_sys_fsync+19
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 2
```

- i_data 用于存 symbol link 对吧

### 读入到内存中 : simplefs_iget

```txt
@[
    simplefs_iget+5
    simplefs_fill_super+670
    mount_bdev+240
    simplefs_mount+26
    legacy_get_tree+40
    vfs_get_tree+38
    vfs_cmd_create+89
    __do_sys_fsconfig+1248
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```
很奇怪，居然 ls /mnt 之后，居然只有这一个调用

- simplefs_new_inode
  - simplefs_iget

- simplefs_lookup
  - simplefs_iget : 按道理就是这里的了，但是

simplefs_iget -> iget_locked


实际上，更加经典的调用路径在这里，当 ls 的时候，这些文件的 inode 的 dentry 就会全部加载进来:
```txt
@[
    simplefs_lookup+5
    __lookup_slow+131
    walk_component+219
    path_lookupat+106
    filename_lookup+242
    vfs_statx+117
    do_statx+102
    __x64_sys_statx+165
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 6
```

### 创建 : simplefs_create

这个只有文件创建的时候才有，不是从 disk 上加载的时候，来加载一个 inode 的情况。


## 基本 io 路径
```txt
@[
    simple_fs_iomap_begin+5
    iomap_iter+299
    iomap_file_buffered_write+155
    simple_fs_file_write_iter+89
    vfs_write+673
    ksys_write+110
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

# TODO

3. 将 `__i_atime` 之类全部重构掉

使用 inode_set_ctime_current

minix 居然只有 4000 行 ，似乎比 simplefs 没复杂多少啊

### 如何增加 O_DIRECT 的支持

### 文件测试文件， page cache 写回过程中需要分配内存

### 将 iomap 和 bh 变为 mount 的选项

### 研究下 page->private 的使用吧

### bcachefs fs/bcachefs/ 既不使用 iomap ，也不使用 buffer head

### ./simplefs_inode.c 中大量的使用了 bh ，其实是有作为 cache 的嫌疑的

那么会不会，其实没有必要使用吧

### 测试这个 api
- find_get_page
- find_get_folio

## 太多时间相关的调整了

类似这种的赋值太多了
```c
	cur_time = current_time(dir);
```

## 看看有办法测试 negative dentry 吗?
到时候对比测试下:
cat /proc/sys/fs/dentry-state
cat /proc/slabinfo

## 调查一下 fs/iomap
- https://patchwork.kernel.org/project/linux-fsdevel/patch/1464792297-13185-3-git-send-email-hch@lst.de/

## 这个
https://zhuanlan.zhihu.com/p/545906763

## 之前的笔记整理下
- mount_bdev

## 部署到 /dev/vdb 上有什么问题?

## 在 simplefs/simplefs_inode.c 中有大量的 inode read 的内容
sb_bread

## libfs  也提供了很多
```c
const struct file_operations simple_dir_operations = {
	.open		= dcache_dir_open,
	.release	= dcache_dir_close,
	.llseek		= dcache_dir_lseek,
	.read		= generic_read_dir,
	.iterate_shared	= dcache_readdir,
	.fsync		= noop_fsync,
};

/*
 * Provides ramfs-style behavior: data in the pagecache, but no writeback.
 */
const struct address_space_operations ram_aops = {
	.read_folio	= simple_read_folio,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
	.dirty_folio	= noop_dirty_folio,
};

```

io uring 测试 ext4 的效果，完全想不到的效果，这里先是调用到 iomap ，然后 ext2_get_blocks 中还是调用到 bread 中:

ext2_get_blocks 也是使用的 sb_bread 其他的文件系统如何实现这个?

## 添加自定义的操作
static const struct dentry_operations 的 cache

## 可以把 simplefs 最后写成一个 vfs 的 benchmark 工具
就像是 nullblk 一样

## 在模块中测试一下 iget5_locked 这函数

## 直接集成 jbd2 可以吗?
参考主线的 simplefs 更新即可

## 增加 xttar 的支持


## 从 simple_fs_iomap_begin 开始

对比一下各个
1. ext4_getblk
2. ext2_get_blocks
3. erofs_iomap_begin

## [ ]  这个东西在 simplefs 中对应的实现没有的
```c
int ext2_fiemap(struct inode *inode, struct fiemap_extent_info *fieinfo,
		u64 start, u64 len)
{
	int ret;

	inode_lock(inode);
	len = min_t(u64, len, i_size_read(inode));
	ret = iomap_fiemap(inode, fieinfo, start, len, &ext2_iomap_ops);
	inode_unlock(inode);

	return ret;
}
```

## 目前是使用 buffer head 的
- sb_bread / brelse
- mark_buffer_dirty && sync_dirty_buffer
  - 如果修改了
- map_bh


似乎是这样的，没有问题:
```txt
@[
    blkdev_read_folio+5
    filemap_read_folio+176
    do_read_cache_folio+124
    simplefs_get_folio+33
    simplefs_fill_super+328
    mount_bdev+240
    simplefs_mount+58
    legacy_get_tree+40
    vfs_get_tree+38
    vfs_cmd_create+89
    __do_sys_fsconfig+1248
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

做替换的一个经典例子:
```diff
@@ -325,17 +297,16 @@ int simplefs_fill_super(struct super_block *sb, void *data, int silent)

 	for (i = 0; i < sbi->nr_bfree_blocks; i++) {
 		int idx = sbi->nr_istore_blocks + sbi->nr_ifree_blocks + i + 1;
-
-		bh = sb_bread(sb, idx);
-		if (!bh) {
+		void *data = simplefs_get_folio(sb, idx, &folio);
+		if (IS_ERR(data)) {
 			ret = -EIO;
 			goto free_bfree;
 		}

 		memcpy((void *)sbi->bfree_bitmap + i * SIMPLEFS_BLOCK_SIZE,
-		       bh->b_data, SIMPLEFS_BLOCK_SIZE);
+		       data, SIMPLEFS_BLOCK_SIZE);

-		brelse(bh);
+		folio_release_kmap(folio, data);
 	}

 	/* Create root inode */
@@ -364,7 +335,7 @@ free_ifree:
 free_sbi:
 	kfree(sbi);
 release:
-	brelse(bh);
+	folio_release_kmap(folio, csb);

 	return ret;
 }
```
但是还没完全搞完

### [ ] 我们需要处理 dirty 的问题吗?
需要的

### [x] map_bh 什么的时候使用?
先不去管了，是 simplefs/simplefs_file.c 中使用的

## TODO
- 现在假设了 page size block size 总是相等的
- 继续理解下 simplefs_iterate 中的操作

## 对比一下，simple fs 中定义了那些 ops ，ext4 定义过那些
```txt
960:static const struct inode_operations simplefs_inode_ops = {
971:static const struct inode_operations symlink_inode_ops = {

171:static const struct iomap_ops simple_fs_iomap_ops = {
219:static const struct iomap_writeback_ops simple_fs_writeback_ops = {
259:static const struct iomap_dio_ops ext2_dio_write_ops = {
```

## 按道理 sb_bread 可以使用类似这个就可以了，但是为什么并没有?

```c
int sync_page_io(struct md_rdev *rdev, sector_t sector, int size,
		 struct page *page, blk_opf_t opf, bool metadata_op)
{
	struct bio bio;
	struct bio_vec bvec;

	if (metadata_op && rdev->meta_bdev)
		bio_init(&bio, rdev->meta_bdev, &bvec, 1, opf);
	else
		bio_init(&bio, rdev->bdev, &bvec, 1, opf);

	if (metadata_op)
		bio.bi_iter.bi_sector = sector + rdev->sb_start;
	else if (rdev->mddev->reshape_position != MaxSector &&
		 (rdev->mddev->reshape_backwards ==
		  (sector >= rdev->mddev->reshape_position)))
		bio.bi_iter.bi_sector = sector + rdev->new_data_offset;
	else
		bio.bi_iter.bi_sector = sector + rdev->data_offset;
	__bio_add_page(&bio, page, size, 0);

	submit_bio_wait(&bio);

	return !bio.bi_status;
}
```

## 我需要通过理解一下 negative dentry 是做什么的?

## 这个也上手看看

https://github.com/iomesh/libuzfs-rs/blob/main/README-cn.md

在 slabtop -s c 中，可以看到很多 buffer head ，如果使用 iomap 之后，这些 buffer
head 导致的内存浪费，是不是就可以完全释放掉?

## simplefs_lookup 中发现，即便是一个空目录
其中的 dentry 也会输出
```txt
[ 7125.281102] simplefs: [martins3:simplefs_lookup:120] dentry name: .envrc
[ 7125.334490] simplefs: [martins3:simplefs_iterate:37]
[ 7125.334665] simplefs: [martins3:simplefs_iterate:37]
[ 7125.334852] simplefs: [martins3:simplefs_lookup:120] dentry name: .git
[ 7125.334868] simplefs: [martins3:simplefs_iterate:37]
[ 7125.335049] simplefs: [martins3:simplefs_lookup:120] dentry name: HEAD
[ 7125.335300] simplefs: [martins3:simplefs_iterate:37]
```
也就是说这个 dentry 是从外部传递过来的

## 遇到一个问题

如果在 simplefs_create_internal 中返回 dentry ，那么会触发这个问题:
正确的做法是直接返回 PTR_ERR(0)
```txt
[  283.505033] ------------[ cut here ]------------
[  283.505370] WARNING: CPU: 9 PID: 3539 at fs/dcache.c:828 dput.part.0+0x3f3/0x410
[  283.505743] Modules linked in: simplefs(OE) xfs virtio_net net_failover virtio_input virtio_console virtio_scsi virtio_balloon failover zram zsmalloc zstd_compress fuse e1000e raid1 md_mod kheaders vfat fat nls_iso8859_1 nls_cp437 scsi_debug crc_t10dif null_blk bridge stp llc xt_mark nf_tables nf_conntrack_netlink xt_MASQUERADE xt_conntrack xt_tcpudp xt_addrtype x_tables rpcsec_gss_krb5 af_packet auth_rpcgss dm_mod openvswitch nsh tun 9p 9pnet netfs nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 overlay nfsv4 dns_resolver nfs lockd grace sunrpc configs configfs nfnetlink virtio_pci virtio_pci_legacy_dev virtio_pci_modern_dev autofs4
[  283.508854] CPU: 9 UID: 1000 PID: 3539 Comm: mkdir Tainted: G           OE       6.17.7-martins3-00001-gfd23f075a322 #12 PREEMPT(full)
[  283.509443] Tainted: [O]=OOT_MODULE, [E]=UNSIGNED_MODULE
[  283.509698] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[  283.510178] RIP: 0010:dput.part.0+0x3f3/0x410
[  283.510374] Code: 80 3d 00 3a 39 01 00 75 aa 48 c7 c2 48 e2 12 82 be 65 03 00 00 48 c7 c7 99 71 1a 82 c6 05 e4 39 39 01 01 e8 9f 27 ce ff eb 89 <0f> 0b 48 89 ef e8 c3 d9 63 00 e9 6f fc ff ff 66 66 2e 0f 1f 84 00
[  283.511048] RSP: 0018:ffffc9000bdcfe90 EFLAGS: 00010246
[  283.511190] RAX: 0000000000000000 RBX: ffff888100725860 RCX: 2382736f898cc848
[  283.511387] RDX: 0000000000000001 RSI: 0000000090a33071 RDI: ffff888100725918
[  283.511592] RBP: ffff888100725918 R08: 0000000029b17476 R09: 000000000000007f
[  283.511788] R10: 0000000000000200 R11: 0000000000000000 R12: 00000000000001ff
[  283.511985] R13: ffff888102241728 R14: ffff888105db07a0 R15: ffffc9000bdcfec8
[  283.512182] FS:  00007faf10712e00(0000) GS:ffff8882b47b5000(0000) knlGS:0000000000000000
[  283.512408] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[  283.512572] CR2: 00007faf108862e0 CR3: 0000000116f47000 CR4: 00000000000006f0
[  283.512778] Call Trace:
[  283.512834]  <TASK>
[  283.512881]  do_mkdirat+0xa9/0x1a0
[  283.513000]  __x64_sys_mkdir+0x28/0x30
[  283.513137]  do_syscall_64+0x74/0x3a0
[  283.513275]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[  283.513483] RIP: 0033:0x7faf108a6f2b
[  283.513639] Code: 0f 1e fa 48 89 f2 b9 00 01 00 00 48 89 fe bf 9c ff ff ff e9 37 c8 ff ff 0f 1f 80 00 00 00 00 f3 0f 1e fa b8 53 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 05 c3 0f 1f 40 00 48 8b 15 a1 fe 0f 00 f7 d8
[  283.514502] RSP: 002b:00007fff87ba2a28 EFLAGS: 00000246 ORIG_RAX: 0000000000000053
[  283.514853] RAX: ffffffffffffffda RBX: 00007fff87ba3225 RCX: 00007faf108a6f2b
[  283.515195] RDX: 0000000000000000 RSI: 00000000000001ff RDI: 00007fff87ba3225
[  283.515541] RBP: 00007fff87ba2b40 R08: 0000000000000000 R09: 0000000000000000
[  283.515915] R10: 00007faf10858d80 R11: 0000000000000246 R12: 00007fff87ba3225
[  283.516303] R13: 00000000000001ff R14: 00007fff87ba2e30 R15: 0000000000000000
[  283.516744]  </TASK>
[  283.516841] irq event stamp: 2711
[  283.517001] hardirqs last  enabled at (2721): [<ffffffff813973ee>] __up_console_sem+0x5e/0x80
[  283.517465] hardirqs last disabled at (2730): [<ffffffff813973d3>] __up_console_sem+0x43/0x80
[  283.517931] softirqs last  enabled at (2614): [<ffffffff812dd726>] __irq_exit_rcu+0xa6/0xd0
[  283.518363] softirqs last disabled at (2609): [<ffffffff812dd726>] __irq_exit_rcu+0xa6/0xd0
[  283.518805] ---[ end trace 0000000000000000 ]---
```

## 写一个最简单的 shell 来自动测试一下吧
最好是让 ai 可以自动检查

现在搞的，一通修改，目录都不显示了。

## 这样的替代是没问题的吗?
```diff
-       mark_buffer_dirty(bh2);
-       mark_buffer_dirty(bh);
-       brelse(bh2);
-       brelse(bh);
+       folio_mark_dirty(dfolio2);
+       folio_mark_dirty(dfolio);
+       folio_release_kmap(dfolio2, dblock);
```

这两个有什么区别?
```diff
-       d_instantiate(dentry, inode);
+       d_instantiate_new(dentry, inode);  // Using d_i
```

类似的这种转变:
```diff
-               mark_buffer_dirty(bh);
-               if (wait)
-                       sync_dirty_buffer(bh);
-               brelse(bh);
+               folio_mark_dirty(folio);
+               if (wait) {
+                       folio_start_writeback(folio);
+                       folio_wait_writeback(folio);
+               }
+               folio_release_kmap(folio, data);
```

现在这个
```txt
[<0>] folio_wait_bit_common+0x139/0x310
[<0>] folio_wait_writeback+0x27/0xd0
[<0>] simplefs_sync_fs+0x261/0x270 [simplefs]
[<0>] sync_filesystem+0x7b/0xa0
[<0>] generic_shutdown_super+0x28/0x170
[<0>] kill_block_super+0x1a/0x40
[<0>] simplefs_kill_sb+0xe/0x20 [simplefs]
[<0>] deactivate_locked_super+0x30/0xa0
[<0>] cleanup_mnt+0xba/0x150
[<0>] task_work_run+0x59/0xa0
[<0>] exit_to_user_mode_loop+0xd4/0xe0
[<0>] do_syscall_64+0x2b1/0x3a0
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

## 一些其他的有趣想法
这是 ai 时代了，让 ai 来做吧

- kvfs : 文件系统只能有一个目录，没有 links ，相当于文件名是 key ，文件是 value
- qemu fs : 把 qemu 的 block layer ( snapshot ，dirty miration) qcow2
  之类的移到文件系统中， 然后让 qmeu 的后端是这个文件系统中的一个文件
- brain fuck fs : 可以搞一些特殊的熟悉，每一个 ls ，结果都是随机的?

似乎其实就是 fs/zonefs 了，参考这个实现是最好的了，还是一步步的来，从 super 的加载，到支持目录的创建。

按到写正常代码的方法来写。


## 如果 unix domain socket 可以在 simpelfs 上创建吗?

从 net/unix/af_unix.c 中的 unix_find_bsd

这里的 dentry 和 inode 会有关系吗?
```txt
	inode = d_backing_inode(path.dentry);
```

## 如果内核直接删除一个 umount 一个 partion ，该 partion 上的 page cache 都是如何处理的

到时候看看这个话题。

拔掉一个盘的时候
```txt
  b'invalidate_inodes'
  b'__invalidate_device'
  b'invalidate_partition'
  b'del_gendisk'
  b'virtblk_remove'
  b'virtio_dev_remove'
  b'device_release_driver_internal'
  b'bus_remove_device'
  b'device_del'
  b'device_unregister'
  b'unregister_virtio_device'
  b'virtio_pci_remove'
  b'pci_device_remove'
  b'device_release_driver_internal'
  b'pci_stop_bus_device'
  b'pci_stop_and_remove_bus_device'
  b'disable_slot'
  b'acpiphp_disable_and_eject_slot'
  b'acpiphp_hotplug_notify'
  b'acpi_device_hotplug'
  b'acpi_hotplug_work_fn'
  b'process_one_work'
  b'worker_thread'
  b'kthread'
  b'ret_from_fork'
```

```txt
$ bt
#0  xfs_fs_drop_inode (inode=0xffff8881795f0538) at fs/xfs/xfs_super.c:741
#1  0xffffffff814856ec in iput_final (inode=0xffff8881795f0538) at fs/inode.c:1745
#2  iput (inode=<optimized out>) at fs/inode.c:1801
#3  iput (inode=0xffff8881795f0538) at fs/inode.c:1787
#4  0xffffffff81480603 in __dentry_kill (dentry=0xffff8881004df780) at fs/dcache.c:607
#5  0xffffffff81481ec6 in shrink_dentry_list (list=list@entry=0xffffc900013abe10) at fs/dcache.c:1201
#6  0xffffffff81482190 in shrink_dcache_parent (parent=parent@entry=0xffff888103720b40) at fs/dcache.c:1652
#7  0xffffffff814824ab in do_one_tree (dentry=0xffff888103720b40) at fs/dcache.c:1681
#8  shrink_dcache_for_umount (sb=sb@entry=0xffff88810d042000) at fs/dcache.c:1698
#9  0xffffffff81461730 in generic_shutdown_super (sb=sb@entry=0xffff88810d042000) at fs/super.c:668
#10 0xffffffff81461a2a in kill_block_super (sb=0xffff88810d042000) at fs/super.c:1667
#11 0xffffffff81720272 in xfs_kill_sb (sb=0xffff88810d042000) at fs/xfs/xfs_super.c:2032
#12 0xffffffff81462ca4 in deactivate_locked_super (s=0xffff88810d042000) at fs/super.c:484
#13 0xffffffff81462dbd in deactivate_super (s=<optimized out>) at fs/super.c:517
#14 0xffffffff8148d12d in cleanup_mnt (mnt=0xffff88810e3c8640) at fs/namespace.c:1256
#15 0xffffffff8116e3aa in task_work_run () at kernel/task_work.c:180
#16 0xffffffff8120e280 in resume_user_mode_work (regs=0xffffc900013abf58) at ./include/linux/resume_user_mode.h:49
#17 exit_to_user_mode_loop (ti_work=<optimized out>, regs=<optimized out>) at kernel/entry/common.c:171
#18 exit_to_user_mode_prepare (regs=regs@entry=0xffffc900013abf58) at kernel/entry/common.c:204
#19 0xffffffff82396dce in __syscall_exit_to_user_mode_work (regs=0xffffc900013abf58) at kernel/entry/common.c:285
#20 syscall_exit_to_user_mode (regs=regs@entry=0xffffc900013abf58) at kernel/entry/common.c:296
#21 0xffffffff82391bf3 in do_syscall_64 (regs=0xffffc900013abf58, nr=<optimized out>) at arch/x86/entry/common.c:88
#22 0xffffffff824000eb in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#23 0x0000000000000000 in ?? ()
```

当 mount 掉一个刚刚构建过 mount point
```txt
@[
    evict+1
    dispose_list+72
    evict_inodes+363
    generic_shutdown_super+61
    kill_block_super+34
    deactivate_locked_super+48
    cleanup_mnt+189
    task_work_run+90
    exit_to_user_mode_prepare+507
    syscall_exit_to_user_mode+27
    do_syscall_64+74
    entry_SYSCALL_64_after_hwframe+110
]: 99212
```

更加具体到释放的内存的位置上:
```txt
@[
    __free_pages+5
    __unfreeze_partials+461
    free_buffer_head+37
    try_to_free_buffers+139
    truncate_cleanup_folio+84
    truncate_inode_pages_range+277
    ext4_evict_inode+794
    evict+205
    dispose_list+72
    evict_inodes+363
    generic_shutdown_super+61
    kill_block_super+34
    deactivate_locked_super+48
    cleanup_mnt+189
    task_work_run+90
    exit_to_user_mode_prepare+507
    syscall_exit_to_user_mode+27
    do_syscall_64+74
    entry_SYSCALL_64_after_hwframe+110
]: 2076
```

```txt
@[
    filemap_release_folio+5
    truncate_cleanup_folio+84
    truncate_inode_pages_range+277
    ext4_evict_inode+794
    evict+205
    dispose_list+72
    evict_inodes+363
    generic_shutdown_super+61
    kill_block_super+34
    deactivate_locked_super+48
    cleanup_mnt+189
    task_work_run+90
    exit_to_user_mode_prepare+507
    syscall_exit_to_user_mode+27
    do_syscall_64+74
    entry_SYSCALL_64_after_hwframe+110
]: 1482041
```

使用 ftrace graph 可以看到的:
```txt
 20)               |  truncate_cleanup_folio() {
 20)               |    ext4_invalidate_folio [ext4]() {
 20)               |      block_invalidate_folio() {
 20)   0.116 us    |        __cond_resched();
 20)   0.092 us    |        wake_up_bit();
 20)               |        filemap_release_folio() {
 20)               |          ext4_release_folio [ext4]() {
                                // 此处省掉 jbd 相关的内容
 20)   5.036 us    |          }
 20)   5.204 us    |        }
 20)   5.973 us    |      }
 20)   6.330 us    |    }
 20)   7.850 us    |  }
```

这里的 umount -l /tmp 也是一种方法。

sync_filesystem 是很早之前搞的。

del_gendisk 中 fsync_bdev 来同步垃圾。

## 到时候在回顾一下
3. 文件系统的启动过程
  1. 想一下，/dev/ mount 时间是在 initramfs，也就是在根文件系统被 mount 之后，但是根文件系统的 mount 似乎需要 /dev/something
  2. mount 的接口是什么 ? 可以 mount 的是 路径 fd inode 设备还是 ?
  3. 根文件系统的 mount 位置是靠什么指定的 ? 内核参数 root=/ 在 initramfs 中间，这是什么探测的
  4. mount 参数 ： 设备，路径。根文件系统的设备，内核参数或者 initramfs，其他的各种设备，探测，然后放到 /dev/sda 之内的位置
    1. 所以，/dev/sda 被探测是确定的
4. 文件系统启动除了 mount 还有什么 ? inode cache, dcache 以及其他的数据结构需要初始化吗 ?
8. fuse

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

[^1] 提供了所有的 API 的解释说明，主要是几个 operation，需要解释一下:

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
```plain
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


## VFS
file_operations::mkdir 作为 file 的 inode 和 dir 有区别吗 ?

| x    | inode_operations                                | file_operations     |
|------|-------------------------------------------------|---------------------|
| dir  | inode operation 应该是主要提供给 dir 的操作的， | 主要是 readdir 操作 |
| file | attr acl 相关                                   | 各种 IO              |

所以，不相关的功能被放到同一个 operation 结构体中间了。

## 原来文件系统需要打上标签之后，才可以支持 O_DIRECT

```c
static int simplefs_file_open(struct inode *inode, struct file *filp)                                                                                 │
{                                                                                                                                                     │
      filp->f_mode |= FMODE_CAN_ODIRECT;                                                                                                              │
      return generic_file_open(inode, filp);                                                                                                          │
}
```

## 原来 xfs 中提到的 SCRATCH 是个专门的工具啊
```txt
4. (optional) Create SCRATCH device pool.
    - needed for BTRFS testing
    - specifies 3 or more independent SCRATCH devices via the SCRATCH_DEV_POOL
      variable e.g SCRATCH_DEV_POOL="/dev/sda /dev/sdb /dev/sdc"
    - device contents will be destroyed.
    - SCRATCH device should be left unset, it will be overridden
      by the SCRATCH_DEV_POOL implementation.
```

## 不管怎么说，连 linux 源码都拷贝不上去，这是不可以接受的

## 应该还有引用计数的问题的

## 设计目标，我自己可以在这个项目中工作!


## 这个错误真的就是挥之不去的阴影啊
```c
/*
 * Warn about a page cache invalidation failure during a direct I/O write.
 */
static void dio_warn_stale_pagecache(struct file *filp)
{
	static DEFINE_RATELIMIT_STATE(_rs, 86400 * HZ, DEFAULT_RATELIMIT_BURST);
	char pathname[128];
	char *path;

	errseq_set(&filp->f_mapping->wb_err, -EIO);
	if (__ratelimit(&_rs)) {
		path = file_path(filp, pathname, sizeof(pathname));
		if (IS_ERR(path))
			path = "(unknown)";
		pr_crit("Page cache invalidation failure on direct I/O.  Possible data corruption due to collision with buffered I/O!\n");
		pr_crit("File: %s PID: %d Comm: %.20s\n", path, current->pid,
			current->comm);
	}
}
```

## 如果想继续
agent --resume=a1194863-9327-4dcc-af4d-b8312841a57a

但是已经很不错了，后面的继续好好玩玩吧

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
