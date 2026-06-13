## 资料
- https://raid.wiki.kernel.org/index.php/Linux_Raid
- https://en.wikipedia.org/wiki/RAID
- linux/Documentation/driver-api/md/
  - 关于 raid5 raid6 之类的，用途不大
- https://www.kernel.org/doc/html/latest/admin-guide/md.html
- https://www.ducea.com/2009/03/08/mdadm-cheat-sheet/ : 简单看看
- https://superuser.com/questions/942886/fail-device-in-md-raid-when-ata-stops-responding

- drivers/md/md-bitmap.c 是一个公共机制

## ioctl 接口

- SyS_ioctl
  - SYSC_ioctl
    - do_vfs_ioctl
      - vfs_ioctl
        - block_ioctl
          - blkdev_ioctl
            - __blkdev_driver_ioctl
              - md_ioctl

```c
/* status */
#define RAID_VERSION		_IOR (MD_MAJOR, 0x10, mdu_version_t)
#define GET_ARRAY_INFO		_IOR (MD_MAJOR, 0x11, mdu_array_info_t)
#define GET_DISK_INFO		_IOR (MD_MAJOR, 0x12, mdu_disk_info_t)
#define RAID_AUTORUN		_IO (MD_MAJOR, 0x14)
#define GET_BITMAP_FILE		_IOR (MD_MAJOR, 0x15, mdu_bitmap_file_t)

/* configuration */
#define CLEAR_ARRAY		_IO (MD_MAJOR, 0x20)
#define ADD_NEW_DISK		_IOW (MD_MAJOR, 0x21, mdu_disk_info_t)
#define HOT_REMOVE_DISK		_IO (MD_MAJOR, 0x22)
#define SET_ARRAY_INFO		_IOW (MD_MAJOR, 0x23, mdu_array_info_t)
#define SET_DISK_INFO		_IO (MD_MAJOR, 0x24)
#define WRITE_RAID_INFO		_IO (MD_MAJOR, 0x25)
#define UNPROTECT_ARRAY		_IO (MD_MAJOR, 0x26)
#define PROTECT_ARRAY		_IO (MD_MAJOR, 0x27)
#define HOT_ADD_DISK		_IO (MD_MAJOR, 0x28)
#define SET_DISK_FAULTY		_IO (MD_MAJOR, 0x29)
#define HOT_GENERATE_ERROR	_IO (MD_MAJOR, 0x2a)
#define SET_BITMAP_FILE		_IOW (MD_MAJOR, 0x2b, int)

/* usage */
#define RUN_ARRAY		_IOW (MD_MAJOR, 0x30, mdu_param_t)
/*  0x31 was START_ARRAY  */
#define STOP_ARRAY		_IO (MD_MAJOR, 0x32)
#define STOP_ARRAY_RO		_IO (MD_MAJOR, 0x33)
#define RESTART_ARRAY_RW	_IO (MD_MAJOR, 0x34)
#define CLUSTERED_DISK_NACK	_IO (MD_MAJOR, 0x35)
```

1. 这些功能和 sysfs 基本是重复的
  - 例如 new_dev_store 和 ADD_NEW_DISK 都是可以增加新的盘到阵列中

## 如何理解函数 md_check_recovery

1. 当发生了 failed io 之后， 执行了 raid1d 之后， 在 raid1d 中就会反复调用 md_check_recovery 来进行恢复
2. 观察 raid1 ，发现 md_check_recovery 在被不断的调用 (几秒一次)
进而调用到 md_super_write 上，也就是 raid 的 superblock 在被周期的刷新。

```c
/*
 * This routine is regularly called by all per-raid-array threads to
 * deal with generic issues like resync and super-block update.
 * Raid personalities that don't have a thread (linear/raid0) do not
 * need this as they never do any recovery or update the superblock.
 *
 * It does not do any resync itself, but rather "forks" off other threads
 * to do that as needed.
 * When it is determined that resync is needed, we set MD_RECOVERY_RUNNING in
 * "->recovery" and create a thread at ->sync_thread.
 * When the thread finishes it sets MD_RECOVERY_DONE
 * and wakeups up this thread which will reap the thread and finish up.
 * This thread also removes any faulty devices (with nr_pending == 0).
 *
 * The overall approach is:
 *  1/ if the superblock needs updating, update it.
 *  2/ If a recovery thread is running, don't do anything else.
 *  3/ If recovery has finished, clean up, possibly marking spares active.
 *  4/ If there are any faulty devices, remove them.
 *  5/ If array is degraded, try to add spares devices
 *  6/ If array has spares or is not in-sync, start a resync thread.
 */
```

## superblock 的基本维护

```c
enum mddev_sb_flags {
	MD_SB_CHANGE_DEVS,		/* Some device status has changed */
	MD_SB_CHANGE_CLEAN,	/* transition to or from 'clean' */
	MD_SB_CHANGE_PENDING,	/* switch from 'clean' to 'active' in progress */
	MD_SB_NEED_REWRITE,	/* metadata write needs to be repeated */
};
```
读 :

- vfs_ioctl
  - blkdev_ioctl
    - md_ioctl
      - md_add_new_disk
        - md_import_device
          - super_1_load

写:

- md_update_sb
  - sync_sbs
    - sync_super
      - super_1_sync
  - md_super_write
    - super_written

### 问题
4. md_ioctl 中，为什么 hot remove disk 需要让 recovery thread 运动起来了

```c
static int md_ioctl(struct block_device *bdev, blk_mode_t mode,
			unsigned int cmd, unsigned long arg)
{
  // ... 如何理解这段代码
	if (cmd == HOT_REMOVE_DISK)
		/* need to ensure recovery thread has run */
		wait_event_interruptible_timeout(mddev->sb_wait,
						 !test_bit(MD_RECOVERY_NEEDED,
							   &mddev->recovery),
						 msecs_to_jiffies(5000));
```

## raid reshape 的两个意义
raid1 中是 : raid1_reshape ，其实应该叫 raid1_check_reshape

不要将 raid5 raid10 的 reshape 指的是
1. mddev::reshape_position
2. sysfs 接口 reshape_direction 和 reshape_position 看上去也不对的

## bitmap io
```txt
@[
        md_super_write+0
        filemap_write_page+108
        __bitmap_unplug+120
        bitmap_unplug+56
        flush_bio_list+64
        raid1_unplug+60
        __blk_flush_plug+208
        blk_finish_plug+64
        xfs_buf_delwri_submit_nowait+288
        xfsaild+692
        kthread+316
        ret_from_fork+16
]: 2
@[
        md_super_write+0
        filemap_write_page+108
        __bitmap_unplug+120
        md_bitmap_unplug_fn+36
        process_one_work+532
        worker_thread+452
        kthread+316
        ret_from_fork+16
]: 2
@[
        md_super_write+0
        filemap_write_page+108
        __bitmap_unplug+120
        bitmap_unplug+56
        flush_bio_list+64
        raid1_unplug+60
        __blk_flush_plug+208
        blk_finish_plug+64
        __iomap_dio_rw+544
        iomap_dio_rw+24
        xfs_file_dio_write_aligned.constprop.0+268
        xfs_file_write_iter+296
        io_write+484
        io_issue_sqe+104
        io_submit_sqes+532
        __arm64_sys_io_uring_enter+516
        invoke_syscall.constprop.0+88
        do_el0_svc+72
        el0_svc+88
        el0t_64_sync_handler+268
        el0t_64_sync+408
]: 2
@[
        md_super_write+0
        filemap_write_page+108
        bitmap_daemon_work+756
        md_check_recovery+48
        raid1d+108
        md_thread+196
        kthread+316
        ret_from_fork+16
]: 6
@[
        md_super_write+0
        filemap_write_page+108
        __bitmap_unplug+120
        bitmap_unplug+56
        flush_bio_list+64
        raid1_unplug+60
        __blk_flush_plug+208
        blk_finish_plug+64
        __iomap_dio_rw+544
        iomap_dio_rw+24
        xfs_file_dio_write_aligned.constprop.0+268
        xfs_file_write_iter+296
        io_write+484
        io_issue_sqe+1444
        io_wq_submit_work+176
        io_worker_handle_work+468
        io_wq_worker+240
        ret_from_fork+16
]: 8
@[
        md_super_write+0
        bitmap_update_sb+272
        md_update_sb+1684
        md_check_recovery+1404
        raid1d+108
        md_thread+196
        kthread+316
        ret_from_fork+16
]: 8
@[
        md_super_write+0
        md_check_recovery+1404
        raid1d+108
        md_thread+196
        kthread+316
        ret_from_fork+16
]: 8
```


## 其他问题
1. 当一个盘作为了 raid ，不知道为什么还是可以继续 echo a > /dev/sda1 的
  - 而且，最神奇的地方在于，echo a > /dev/sda1 之后，raid1 没有任何反应

1. /etc/mdadm.conf 这个文件是做什么的

2. 内核启动参数这个是如何解析的
rd.md.uuid=48a42099:d9b0ecb3:04845acd:3f724548

echo "file drivers/md/md.c line 2670 +p" > /sys/kernel/debug/dynamic_debug/control


### 写的很好，常看常新
> [!NOTE]
> 参考 Deepseeek ，有待验证

简单来说，mddev->pers == NULL 这句判断的意思是：这个 MD 阵列（mddev）当前处于“非活动”状态，还没有加载或关联任何具体的 RAID “个性”模块（personality module）。

mddev->pers 是什么？
mddev 代表一个“多设备”阵列，也就是我们常说的 /dev/mdX。

- pers 是 personality 的缩写。在 MD 驱动的语境中，“个性”就是一个实现了特定 RAID 级别逻辑的内核模块。
- 比如，raid1 模块实现了镜像逻辑。 - raid5 模块实现了带奇偶校验的条带化逻辑。 linear 模块实现了线性追加逻辑。
- 所以，mddev->pers 是一个指向具体 RAID 级别处理函数的结构体指针。如果这个指针是 NULL，就意味着内核虽然知道存在一个名为 mdX 的阵列，但还不知道该如何以 RAID1、RAID5 或是其他任何方式来组织和读写它的成员盘。

您可以把它想象成：mddev 是一个空的电脑机箱，而 mddev->pers 则是要装进去的主板和CPU。只有装入了主板和CPU（设置了 pers），这个机箱才能作为一个真正的电脑（RAID阵列）运行起来。

为什么在您的代码中要检查 mddev->pers == NULL？
您的代码片段处于一个非常关键的阶段：阵列组装 (Assembling)。

在这个阶段，内核正在检查一块物理磁盘（rdev）上的元数据（sb，即超级块），判断它是否能成为 mddev 这个阵列的一员。

此时，mddev->pers 必然是 NULL，因为阵列还没有被正式“运行”起来。内核需要先收集足够数量的、元数据一致的成员盘，才能最终确定“哦，这是一个由3块盘组成的RAID5阵列”，然后才会去加载 raid5 这个“个性”模块，并把 mddev->pers 指向它。

所以，这个 if 块内的逻辑，就是在阵列正式激活之前，对候选成员盘进行的严格审查。

代码 if (ev1 < mddev->events) 的含义是：
ev1 是从当前候选磁盘（rdev）元数据中读出的事件计数器（event_count）。
mddev->events 是这个阵列已知的、最新的事件计数器（可能从第一个被扫描的磁盘上获得）。
如果一个候选盘的事件计数器比阵列已知的还要旧，说明这个盘可能已经“掉队”了（离线过，错过了某些写操作），数据是不新鲜的。在这种严格的检查模式下，内核会拒绝这个“过时”的磁盘加入阵列，返回 -EINVAL 错误。

什么时候会导致 mddev->pers 为 NULL？
mddev->pers 为 NULL 主要发生在以下几个关键的生命周期节点：

1. 阵列创建和组装期间 (Array Creation and Assembly)

当您在用户空间执行 mdadm --create /dev/md0 ... 或 mdadm --assemble /dev/md0 ... 时。
内核会创建一个 mddev 结构体，此时 pers 初始化为 NULL。
然后内核开始扫描和添加成员盘（rdev），您的代码片段就是在这个过程中执行的。
只有当所有检查通过，内核收集到足够的有效成员盘后，才会调用 md_run() 函数。md_run() 会根据元数据找到并加载正确的个性模块（如 raid5），并将 mddev->pers 指向它。从这一刻起，阵列才算真正“激活”。

2. 阵列停止期间 (Array Stopping)
当您执行 mdadm --stop /dev/md0 时。
内核会调用 md_stop() 函数。

这个函数会执行与 md_run() 相反的操作：它会卸载个性模块，并将 mddev->pers 重新设置为 NULL，然后释放所有成员盘。

3. 阵列配置变更期间
在一些复杂的场景下，比如改变 RAID 级别（mdadm --grow ... --level=...），也可能涉及到先停止阵列（pers 变 NULL），修改配置，然后再重新运行阵列（pers 被赋予新值）的过程。
总结：mddev->pers == NULL 是一个明确的标志，表明 MD 阵列正处于一个“非活动”的管理状态，要么是正在组装，要么是已经被停止。在您的代码上下文中，它特指正在组装这个阶段。

<!-- ds 结束 -->

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
