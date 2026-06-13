## Blockdev 源码分析

- drivers/scsi/sd.c
- block/genhd.c : 各种 sysfs 相关
- block/fops.c : 如何实现对于 /dev/sda 进行 io
- block/bdev.c : 提供 bdev fs ，提供 bdev 相关的插叙之类的
- block/events
- block/ioctl.c

## bdev fs

```c
static const struct super_operations bdev_sops;
```

是创建一个 disk 就会存在一个在 bdev fs 中形成一个文件

  - ret_from_fork_asm
    - ret_from_fork
      - kernel_init
        - kernel_init_freeable
          - do_initcalls
            - do_initcall_level
              - do_one_initcall
                - loop_init
                  - loop_add
                    - __blk_mq_alloc_disk
                      - __alloc_disk_node
                        - bdev_alloc
                          - new_inode
                            - new_inode_pseudo
                              - alloc_inode
                                - bdev_alloc_inode


- ret_from_fork_asm
  - ret_from_fork
    - kthread
      - worker_thread
        - process_scheduled_works
          - process_one_work
            - async_run_entry_fn
              - __device_attach_async_helper
                - bus_for_each_drv
                  - __device_attach_driver
                    - driver_probe_device
                      - __driver_probe_device
                        - really_probe
                          - call_driver_probe
                            - sd_probe
                              - blk_mq_alloc_disk_for_queue
                                - __alloc_disk_node
                                  - bdev_alloc
                                    - new_inode
                                      - alloc_inode
                                        - bdev_alloc_inode

```c
static struct file_system_type bd_type = {
	.name		= "bdev",
	.init_fs_context = bd_init_fs_context,
	.kill_sb	= kill_anon_super,
};
```

不是为了管理 /dev/nvme0n1 的增删改查，而是为了方便的实现对于 inode 的操作
1. page writeback 机制，writeback 机制都是和


## blockdev 的 核心 io 路径

三个公共的
```c
static const struct super_operations bdev_sops;
static const struct address_space_operations def_blk_aops;
const struct file_operations def_blk_fops;
```
`address_space_operations` 是文件系统注册的，用于向下传导的

分析 1 : 如何实现
```c
const struct file_operations def_blk_fops = {
```
走到

```c
const struct address_space_operations def_blk_aops = {
```
观察 generic_perform_write 函数，在其中，通过 ops:write_begin 将 file 的 offset
装换为了可以写入的 page ，然后将该 page 中的内容拷贝出来


需要区分的是，block_device_operations 也是注册了一大组参数，这是控制面上的内容。


- `blkdev_readpage` 和 `ext4_mpage_readpages` 都会调用 `block_read_full_page` 的，但是 ext4 中会去调用 `ext4_get_block` 的实现。
  - `get_block_t`，给定一个 inode 以及偏移量，找到对应的 block number

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - ksys_read
        - vfs_read
          - new_sync_read
            - call_read_iter
              - blkdev_read_iter
                - filemap_read
                  - filemap_get_pages
                    - page_cache_sync_readahead
                      - page_cache_sync_ra
                        - page_cache_ra_unbounded
                          - read_pages
                            - mpage_readahead
                              - do_mpage_readpage
                                - bdev_read_page

从上面的过程中，可以看到直接读写裸盘的时候，也是存在 page cahce 的，而 page cache 总是关联 inode 作为来构建映射的，
所以，需要一个 super_operations 来创建 inode 的。

### blkdev_direct_IO

- generic_file_read_iter : 被引用的位置并不多
  - address_space_operations::direct_IO
  - filemap_read
- generic_file_direct_write

- __do_sys_io_submit
  - io_submit_one
    - __io_submit_one
      - aio_write
        - call_write_iter
          - blkdev_write_iter
            - __generic_file_write_iter
              - generic_perform_write
                - blkdev_write_begin

- __do_sys_io_submit
  - io_submit_one
    - __io_submit_one
      - aio_read
        - call_read_iter
          - blkdev_read_iter
            - blkdev_direct_IO
              - blkdev_direct_IO
                - __blkdev_direct_IO_async

xfs 中直接根本就不注册 `.direct_IO` ，所以不需要在 do_dentry_open 中进行判断，这是因为其在 xfs_file_open 中存在额外的设置:

```c
STATIC int
xfs_file_open(
  struct inode  *inode,
  struct file *file)
{
  if (xfs_is_shutdown(XFS_M(inode->i_sb)))
    return -EIO;
  file->f_mode |= FMODE_NOWAIT | FMODE_BUF_RASYNC | FMODE_BUF_WASYNC |
      FMODE_DIO_PARALLEL_WRITE | FMODE_CAN_ODIRECT;
  return generic_file_open(inode, file);
}
```

本来以为 def_blk_aops::direct_IO 不用注册为 blkdev_direct_IO, 但是看来是我想多了

generic_file_direct_write

```c
  written = mapping->a_ops->direct_IO(iocb, from);
```

此外，才发现，经过了 iomap 改造的就是不需要这个 hook 的。




## bio 和 gendisk 的关系
```c
		struct gendisk *disk = bio->bi_bdev->bd_disk;
```

显然是 bio 注册到时候
```txt
#0  bio_init (opf=<optimized out>, max_vecs=<optimized out>, table=<optimized out>, bdev=<optimized out>, bio=<optimized out>) at block/bio.c:284
#1  bio_alloc_bioset (bdev=bdev@entry=0xffff888105153c00, nr_vecs=<optimized out>, nr_vecs@entry=32, opf=opf@entry=0, gfp_mask=<optimized out>, gfp_mask@entry=3264, bs=0xffffffff83a1bba0 <fs_bio_set>) at block/bio.c:567
#2  0xffffffff81530a6e in bio_alloc (gfp_mask=3264, opf=0, nr_vecs=32, bdev=0xffff888105153c00) at ./include/linux/bio.h:427
#3  ext4_mpage_readpages (inode=0xffff888106af3c78, rac=<optimized out>, folio=<optimized out>) at fs/ext4/readpage.c:362
```

在 ext4_mpage_readpages 中:
```c
	    struct block_device *bdev = inode->i_sb->s_bdev;

			bio = bio_alloc(bdev, bio_max_segs(nr_pages),
					REQ_OP_READ, GFP_KERNEL);
```
通过 inode 找到 superblock，

## 一个 gendisk 对应一个物理设备，一个 block device 对应分区
- `gendisk` : 侧重和硬件交互，一个硬件一个
  - `add_disk` : 删除
  - `alloc_disk` : 创建
- `block_device` : 侧重和文件系统交互，一个分区一个，因为每个分区的文件系统都不同

所有的 block device 指向同一个 request_queue :
```c
static inline struct request_queue *bdev_get_queue(struct block_device *bdev)
{
	return bdev->bd_queue;	/* this is never NULL */
}
```

```c
struct block_device *bdev_alloc(struct gendisk *disk, u8 partno)
{
  // ...
	bdev->bd_queue = disk->queue;
```
bdev_alloc() 的参数是 gendisk 和 partno ，然后创建出来一个 partion ，返回的是一个 block_device
此外，由于一个分区一个 block_device ，整个盘一个 block_device ，所以 bdev_alloc 调用就是地方就是
在 block/partitions/core.c 和 block/genhd.c ( disk->part0 = bdev_alloc(disk, 0); )

当然这个物理设备也可以是一个 raid 这种的，这个时候，一个 /dev/md1 就是一个 gendisk 。



## 如何理解 /proc/meminfo 中的 Buffers
```c
long nr_blockdev_pages(void)
{
  struct inode *inode;
  long ret = 0;

  spin_lock(&blockdev_superblock->s_inode_list_lock);
  list_for_each_entry(inode, &blockdev_superblock->s_inodes, i_sb_list)
    ret += inode->i_mapping->nrpages;
  spin_unlock(&blockdev_superblock->s_inode_list_lock);

  return ret;
}
```
- 创建一个文件系统，其中每一个 inode 都是关联一个 block 设备的。

- bdev_alloc 创建 inode 和文件系统的关系。

`inode->i_mapping->nrpages` 和其他的文件系统沟通的相同。

至少可以清楚的说明，/proc/meminfo 中的 Buffers 和 inode cache 没有任何关系了，就是 block 设备的空间。

## 看看 blockdev 的插拔的结果

- del_gendisk
  - blk_report_disk_dead
    - bdev_mark_dead
      - sync_blockdev : 最后会考虑是不是 surprise hotplug 的:

一路检查，只有在这里是设置为非
```c
	/*
	 * Tell the file system to write back all dirty data and shut down if
	 * it hasn't been notified earlier.
	 */
	if (!test_bit(GD_DEAD, &disk->state))
		blk_report_disk_dead(disk, false);
	__blk_mark_disk_dead(disk);
```

思考一下可以搞出来  surprise 的 block unplug 的?


什么叫做 bd_holder_ops ，真的存在有的机器没有注册吗?
```c
void bdev_mark_dead(struct block_device *bdev, bool surprise)
{
	mutex_lock(&bdev->bd_holder_lock);
	if (bdev->bd_holder_ops && bdev->bd_holder_ops->mark_dead)
		bdev->bd_holder_ops->mark_dead(bdev, surprise);
	else {
		mutex_unlock(&bdev->bd_holder_lock);
		sync_blockdev(bdev);
	}

	invalidate_bdev(bdev);
}
```

```c
static void fs_bdev_mark_dead(struct block_device *bdev, bool surprise)
{
	struct super_block *sb;

	sb = bdev_super_lock_shared(bdev);
	if (!sb)
		return;

	if (!surprise)
		sync_filesystem(sb); // <----  如果是 unsurprise 的才会 syncfile
	shrink_dcache_sb(sb);
	invalidate_inodes(sb);
	if (sb->s_op->shutdown)
		sb->s_op->shutdown(sb);

	super_unlock_shared(sb);
}
```

## 解释几个问题
- /dev/sda 和 /dev/sdb 之类的名称是如何确定的？
- 如何修改一个 partion 的 UUID
  - [ ] 找到 kernel 处理 partion UUID 的位置

- 为什么需要设置 /dev/disk/by-** 之类的名称，这个是如何组织的
- [ ] 在函数 `__submit_bio` 中， 这样获取的 gendisk 实际上可能是 md 的，
`struct gendisk *disk = bio->bi_bdev->bd_disk;`，有趣。
  - 有的设备管理可能是 md
- [ ] 什么时候，出现

- /dev/sda 之类的名称是怎么决定的
- uuid 之类的存储再那里的？

## YES


## 有趣
1. https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2018/06/14/linux-block-device-driver
2. https://www.binss.me/blog/boot-process-of-linux/

在 mount 的过程中，根据路径得到:

- kernel_init
  - kernel_init_freeable
    - prepare_namespace
      - mount_root
        - mount_block_root
          - do_mount_root
            - init_mount
              - path_mount
                - do_new_mount
                  - vfs_get_tree
                    - get_tree_bdev
                      - get_tree_bdev_flags
                        - lookup_bdev

- 通过 `lookup_bdev` 可以将 path 装换为 `dev_t`

```txt
[    7.224403] [martins3:lookup_bdev:1201] /dev/mapper/openeuler-root
[    9.766409] [martins3:lookup_bdev:1201] /dev/vda2
[    9.859085] [martins3:lookup_bdev:1201] /dev/vda1
[   10.494174] [martins3:lookup_bdev:1201] /dev/mapper/openeuler-home
```

## 看看
1. register_blkdev 和 add_disk 的区别:
    1. register_blkdev : 只要使用一个数值即可，和 register_chrdev_region 对称，cdev_add 之后将 cdev 和 设备号关联起来。


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
