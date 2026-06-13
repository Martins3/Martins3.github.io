# raid1

## 基本的数据结构
```c
	struct r1conf *conf = mddev->private;

	conf->mirrors = newmirrors;
	kfree(conf->poolinfo);
	conf->poolinfo = newpoolinfo;
```
mddev 是 generic 的，而 r1conf 并不是

### mddev

1. mddev::degraded 是做什么的?
  - 应该就是记录现在差几块盘的

### struct md_rdev
```c
struct md_rdev {

	struct page	*sb_page, *bb_page;

```
其中 sb_page 是 superblock 所在的 page ，而 bb_page 指的是 badblocks

> [!NOTE]
> 参考 Deepseeek ，有待验证

核心作用：这个结构体在内核中代表一个单一的、物理的成员盘。
当您将一个分区（比如 /dev/sdb1）添加到一个MD阵列（比如 /dev/md0）中时，内核就会创建一个 md_rdev 结构体来管理 /dev/sdb1 的所有相关信息。
您可以把它看作是物理磁盘在RAID阵列中的“代理”或“档案”。

包含的关键信息：

- 指向底层块设备（struct block_device）的指针，代表了真正的物理分区。
- 这块盘在阵列中的状态，例如 In_sync（同步完成）、Faulty（故障）、Spare（备用盘）。
- 从磁盘上读取的超级块（superblock）信息，如事件计数器（sb_events）、UUID等。
- 它在阵列中的角色编号（raid_disk），比如在RAID1中是第0个镜像还是第1个镜像。
- 用于将自己链接到所属 mddev 的设备链表中的指针。

### struct r1conf

> [!NOTE]
> 参考 Deepseeek ，有待验证

这是一个RAID1“个性”模块私有的配置和运行时状态结构体。
当一个MD阵列被配置为RAID1模式并启动时，raid1 内核模块会分配一个 r1conf 结构体，并将其挂载到通用 mddev 结构体的 private 指针上。
它存储了运行一个RAID1阵列所需的所有特定信息和状态，这些信息对于通用的MD层是不可见的。

包含的关键信息：

一个指针数组（mirrors），直接指向构成镜像的各个 md_rdev 结构体，以便快速访问。
镜像盘的数量（raid_disks）。
关于写时意图位图（write-intent bitmap）的所有信息和状态（如果启用了bitmap策略）。
用于处理I/O请求、管理后台同步（resync）和恢复（recovery）的状态变量。
一个指回其所属 mddev 的反向指针。

<!-- Deepseeek -->

的确
```c
	struct r1conf *conf = mddev->private;
```

```c
struct r1conf {
	spinlock_t		device_lock;
  wait_queue_head_t	wait_barrier;
	spinlock_t		resync_lock;
```

r1conf::pending_bio_list ?
raid1_write_request 将 bio 放到该链表中

### struct r1bio

### mdp_superblock_1
mdp_superblock_s 是旧版 (v0.90) 格式，用两个 32 位整数 events_lo 和 events_hi 拼接成一个 64 位计数器。
mdp_superblock_1 是新版 (v1.x) 格式，直接使用一个 64 位整数 events。



### raid1_personality

```c
static struct md_personality raid1_personality =
{
	.name		= "raid1",
	.level		= 1,
	.owner		= THIS_MODULE,
	.make_request	= raid1_make_request,
	.run		= raid1_run, // 激活整列 assemble
	.free		= raid1_free,
	.status		= raid1_status, // /proc/mdstat
	.error_handler	= raid1_error,
	.hot_add_disk	= raid1_add_disk,
	.hot_remove_disk= raid1_remove_disk,
	.spare_active	= raid1_spare_active, // 自动将备用盘换上来
	.sync_request	= raid1_sync_request, // 用于 resync 一个 block
  // 发起 resync 的时机
  // 1. 创建后：自动开始 resync
  // 2. 故障恢复后：自动开始 nixos-rebuild
  // 3. 手动检查：可以手动触发一次数据一致性检查和修复。
	.resize		= raid1_resize, // sudo mdadm --grow /dev/md0 --size=20G
	.size		= raid1_size,
	.check_reshape	= raid1_reshape,  // 这个命令会先调用 check_reshape 验证，然后才开始操作 sudo mdadm --grow /dev/md0 --raid-devices=3 --add /dev/sde1
	.quiesce	= raid1_quiesce,
	.takeover	= raid1_takeover, // raid 的级别转换
};
```

mdadm /dev/md0 --fail /dev/sdc1

- do_syscall_64
  - do_syscall_x64
    - __x64_sys_ioctl
      - __se_sys_ioctl
        - __do_sys_ioctl
          - vfs_ioctl
            - blkdev_ioctl
              - md_ioctl
                - set_disk_faulty
                  - md_error
                    - md_error
                      - raid1_error


mdadm /dev/md0 --remove /dev/sdc1

- ret_from_fork
  - kthread
    - md_thread
      - raid1d
        - md_check_recovery
          - remove_and_add_spares
            - raid1_remove_disk


### 综合

#### raid_disks
1. mddev::raid_disks md_rdev::raid_disk 和 r1conf::raid_disks
```c
struct pool_info {
	struct mddev *mddev;
	int	raid_disks;
};
```

```c
struct r1conf {
	struct mddev		*mddev;
	struct raid1_info	*mirrors;	/* twice 'raid_disks' to allow for replacements. */
	int			raid_disks;
```

是什么关系？如何理解这个代码?
```c
static int raid1_remove_disk(struct mddev *mddev, struct md_rdev *rdev)
	int number = rdev->raid_disk;
	struct raid1_info *p = conf->mirrors + number;

	if (unlikely(number >= conf->raid_disks))
  		goto abort;
```
被 remove_and_add_spares 唯一调用，经过调试，可以知道，

1. md_rdev::raid_disks 就是阵列的数量，而 md_rdev::raid_disk 是这个盘在整列的第几个 slot

2. 从 raid1_reshape 中看，r1conf::raid_disks 就是 mddev::raid_disks 的缓存，而
poolinfo::raid_disks 直接把两倍计算放进去了:
```c
	conf->raid_disks = mddev->raid_disks = raid_disks;

  newpoolinfo->mddev = mddev;
	newpoolinfo->raid_disks = raid_disks * 2;
```
由于这些计算都是在 raid1_reshape 中，所以不用担心没有更新的问题。

#### raid_disks * 2

可以看到大量的 raid_disks * 2 的代码:

```c
struct r1conf {
	struct mddev		*mddev;
	struct raid1_info	*mirrors;	/* twice 'raid_disks' to
						 * allow for replacements.
						 */
```

经过分析，发现是 由于 replacement 机制， man mdadm
```txt
       --replace
              Mark listed devices as requiring replacement.  As soon as a spare is available, it will be rebuilt and will replace the marked device.  This is similar to marking a device as faulty, but the device remains in service during  the  re‐
              covery process to increase resilience against multiple failures.  When the replacement process finishes, the replaced device will be marked as faulty.
```
mdadm /dev/md0 --replace /dev/sdb --with /dev/sdc

也就是当时正在使用的 rdev 都是放到 r1conf::mirrors 上的，
具体调用的函数是 **raid1_add_conf**

由于我们并不会使用 replacement 机制 ，其实
r1conf::mirrors 中只有 `[0, raid_disks)` 才有效。

理解了以上的内容之后，我们就终于可以可以窥探 raid1_remove_disk 的逻辑了，真的非常晦涩:

```c
static int raid1_remove_disk(struct mddev *mddev, struct md_rdev *rdev)
{

  // 如果这个槽位上的盘不是的 rdev，那么当前的就是 rdev 就是 replacement
  // 所以需要加 conf->raid_disks
	if (rdev != p->rdev) {
		number += conf->raid_disks;
		p = conf->mirrors + number;
	}
```

#### events

```c
struct mddev {
  // ...
	__u64				events;
	/* If the last 'event' was simply a clean->dirty transition, and
	 * we didn't write it to the spares, then it is safe and simple
	 * to just decrement the event count on a dirty->clean transition.
	 * So we record that possibility here.
	 */
```

```c
struct md_rdev{
	__u64		sb_events; // 只是一个缓存
}
```

```c
struct mdp_superblock_1 {
  // ...
	__le64	events;		/* incremented when superblock updated */
  // ...
```



1. mddev->events : 内存中、整个阵列级别的事件计数器。
2. md_rdev->sb_events : 内存中、单个成员盘级别的事件计数器。

在 md_update_sb 中更新的时候，会刷新这个，相当于是一个缓存了。
```c
			rdev->sb_events = mddev->events;
```

3. mdp_superblock_1::events 才是写入到磁盘中的


`mdp_superblock_s` 和 `mdp_superblock_1` 中的 events
角色: 磁盘上、持久化的事件计数器。
说明: 这是真正被写入到每个成员盘物理扇区上的元数据。它记录了该磁盘最后一次成功同步阵列元数据时的事件计数值。
mdp_superblock_s 是旧版 (v0.90) 格式，用两个 32 位整数 events_lo 和 events_hi 拼接成一个 64 位计数器。
mdp_superblock_1 是新版 (v1.x) 格式，直接使用一个 64 位整数 events。

所以，这两个东西可以不用管:
```c
typedef struct mdp_superblock_s {
  // ...
	__u32 events_lo;	/*  7 low-order of superblock update count    */
	__u32 events_hi;	/*  8 high-order of superblock update count   */
	__u32 cp_events_lo;	/*  9 low-order of checkpoint update count    */
	__u32 cp_events_hi;	/* 10 high-order of checkpoint update count   */
  // ...
```

```c
static inline __u64 md_event(mdp_super_t *sb) {
	__u64 ev = sb->events_hi;
	return (ev<<32)| sb->events_lo;
}
```

测试办法:
```txt
sudo mdadm --examine /dev/sdc2
/dev/sdc2:
          Magic : a92b4efc
        Version : 1.2
    Feature Map : 0x0
     Array UUID : 252bfa22:f6914695:df71fa22:e614660e
           Name : xxxxxxxxxxxxxxxxxxxxxxxxxx
  Creation Time : Mon Nov  4 17:58:43 2024
     Raid Level : raid1
   Raid Devices : 2

 Avail Dev Size : 209584128 (99.94 GiB 107.31 GB)
     Array Size : 104792064 (99.94 GiB 107.31 GB)
    Data Offset : 131072 sectors
   Super Offset : 8 sectors
   Unused Space : before=130920 sectors, after=0 sectors
          State : clean
    Device UUID : b2f2b6f4:217c4a54:fdc08bc5:fd8ace20

          Flags : failfast
    Update Time : Tue Jul 22 14:00:31 2025
  Bad Block Log : 512 entries available at offset 136 sectors
       Checksum : 3d83a1d1 - correct
         Events : 10024


   Device Role : Active device 1
   Array State : AA ('A' == active, '.' == missing, 'R' == replacing)
```


## mddev 的线程模型

### 基本数据结构和操作
mddev 一共定义四个 work_struct 来处理异步的任务:
```c
struct mddev {

	/* used for delayed sysfs removal */
	struct work_struct del_work;
	/* used for register new sync thread */
	struct work_struct sync_work; // 因为启动 sync work 还有一系列的检查动作


	struct work_struct flush_work;
	struct work_struct event_work;	/* used by dm to report failure event */
```

一共定义两个任务来处理:
```c
struct mddev {

	struct md_thread __rcu		*thread;	/* management thread */
	struct md_thread __rcu		*sync_thread;	/* doing resync or reconstruct */
```


echo check > /sys/block/md2/md/sync_action 之后，可以观察到两个两个 kthread :

```txt
➜  ~ ps -elf | grep md2
1 S root         615       2  0  80   0 -     0 md_thr 16:29 ?        00:00:02 [md2_raid1]
1 D root        2023       2  3  80   0 -     0 msleep 17:31 ?        00:00:01 [md2_resync]
```


### mddev::sync_thread = md_do_sync

mddev::sync_work 执行 md_start_sync 中创建 mddev::sync_thread 最后来执行 md_do_sync

最终调用到 md_personality::sync_request()，md_do_sync 是 md_personality::sync_request 的唯一调用者。

md_do_sync 会对于每一个 sector 调用 md_personality::sync_request

md_reap_sync_thread 可以在 action_store 中停止 resync_thread() 。


### mddev::thread = raid1d

每创建一个 raid 的时候，需要增加一个

raid1.c 中创建
```c
	rcu_assign_pointer(conf->thread,
			   md_register_thread(raid1d, mddev, "raid1"));
```
md_register_thread 注册一个 handler 是 md_thread 的 kthread ，
该 kthread 来执行 `raid1d`

2. raid1_run 中将 r1conf->thread 赋值到 mddev->htread
```c
	rcu_assign_pointer(mddev->thread, conf->thread);
	rcu_assign_pointer(conf->thread, NULL);
```

raid1d 的工作是什么?

1. 更新 sb
  - raid1d
    - md_check_recovery
      - bitmap_daemon_work
        - bitmap_wait_writes
          - bitmap_daemon_work
      - md_check_recovery
        - md_reap_sync_thread : 检查 sync 工作完成了没有
        - md_update_sb
          - md_update_sb
            - md_super_write

2. reschedule_retry 中将无法发送下去的 r1bio 放到 r1conf::retry_list 中
如上所说，`reschedule_retry` 的调用位置就是 `end_bio` 的位置了:

- raid1d
  - r1conf::retry_list 中取出一个 r1bio 出来
  - atomic_dec(&conf->nr_queued[idx]);
  - 针对情况分别提交下去:
    - handle_sync_write_finished
    - sync_request_write
    - handle_write_finished
    - handle_read_error

### 唤醒
raid1d 很容易被唤醒
```txt
@[
        md_wakeup_thread+0
        call_timer_fn+192
        __run_timer_base+708
        timer_expire_remote+48
        tmigr_handle_remote_up+820
        __walk_groups.isra.0+68
        tmigr_handle_remote+220
        run_timer_softirq+108
        handle_softirqs+304
        __do_softirq+28
        ____do_softirq+24
        call_on_irq_stack+36
        do_softirq_own_stack+36
        __irq_exit_rcu+328
        irq_exit_rcu+24
        el1_interrupt+56
        el1h_64_irq_handler+24
        el1h_64_irq+108
        default_idle_call+280
        do_idle+540
        cpu_startup_entry+60
        secondary_start_kernel+312
        __secondary_switched+192
]: 2

```
```txt
@[
        md_wakeup_thread+0
        end_sync_read+152
        bio_endio+388
        blk_mq_end_request_batch+608
        nvme_pci_complete_batch+100
        nvme_irq+128
        __handle_irq_event_percpu+148
        handle_irq_event+84
        handle_fasteoi_irq+168
        handle_irq_desc+60
        generic_handle_domain_irq+36
        gic_handle_irq+84
        call_on_irq_stack+36
        do_interrupt_handler+136
        el1_interrupt+52
        el1h_64_irq_handler+24
        el1h_64_irq+108
        __pi_memcmp+116
        md_thread+196
        kthread+316
        ret_from_fork+16
]: 184
```

而 sync_thread 不是常驻的，只有有 sync 任务的时候才会启动。

## 总结锁的使用是什么东西

### 基本思路
基本思路很简单:
1. 不可以对于同一个盘进行 io 同时进行不同类型的 io (也就是 sync 和 normal io 需要互斥)
2. 当修改 raid 整列大小的时候，需要 freeze array
3. 相同类型的 io ，即使在相同的位置，也是不会互相阻碍

所以:

1. raid1_read_request 中开始的时候调用 wait_read_barrier
2. raid1_write_request 中开始需要 wait_barrier wait_blocked_rdev
3. 当 normal io 结束的时候，调用 allow_barrier
  - raid1_end_read_request -> raid_end_bio_io -> allow_barrier
  - raid1_end_write_request ->  r1_bio_write_done -> raid_end_bio_io -> allow_barrier
4. 而 raise_barrier 和 lower_barrier 是 sync io 使用的

( sync io 和 normal io 为什么不是对称的)


barrier 的最小单位为 64M
```c
static inline int sector_to_idx(sector_t sector)
{
	return hash_long(sector >> BARRIER_UNIT_SECTOR_BITS,
			 BARRIER_BUCKETS_NR_BITS);
}
```

- 机制加入的时间 commit fd76863e37fe ("RAID1: a new I/O barrier implementation to remove resync window")

### 细节分析
```c
struct r1conf {
	/* for use when syncing mirrors:
	 * We don't allow both normal IO and resync/recovery IO at
	 * the same time - resync/recovery can only happen when there
	 * is no other IO.  So when either is active, the other has to wait.
	 * See more details description in raid1.c near raise_barrier().
	 */
	wait_queue_head_t	wait_barrier;
	spinlock_t		resync_lock;
	atomic_t		nr_sync_pending;  // 有多少个 inflight 的 sync io
	atomic_t		*nr_pending;      // 有多少个 inflight 的 normal io
	atomic_t		*nr_waiting;      // 有多少个，由于 barrier 而需要等待的 等待者
	atomic_t		*nr_queued;       // 用于追踪有多少个普通I/O请求因为屏障升起而被临时排队，尚未提交。
                                // 对于已经提交的 io ，如果失败了，那么不能继续提交，需要先放起来
                                // 所以，这个数值总是和
	atomic_t		*barrier;         // raise_barrier 来增加 1
	int			array_frozen;         // 当前 array 是否在 frozen
```

```c
/* Barriers....
 * Sometimes we need to suspend IO while we do something else,
 * either some resync/recovery, or reconfigure the array.
 * To do this we raise a 'barrier'.
 * The 'barrier' is a counter that can be raised multiple times
 * to count how many activities are happening which preclude
 * normal IO.
 * We can only raise the barrier if there is no pending IO.
 * i.e. if nr_pending == 0.
 * We choose only to raise the barrier if no-one is waiting for the
 * barrier to go down.  This means that as soon as an IO request
 * is ready, no other operations which require a barrier will start
 * until the IO request has had a chance.
 *
 * So: regular IO calls 'wait_barrier'.  When that returns there
 *    is no backgroup IO happening,  It must arrange to call
 *    allow_barrier when it has finished its IO.
 *    backgroup IO calls must call raise_barrier.  Once that returns
 *    there is no normal IO happeing.  It must arrange to call
 *    lower_barrier when the particular background IO completes.
 *
 * If resync/recovery is interrupted, returns -EINTR;
 * Otherwise, returns 0.
 */
static int raise_barrier(struct r1conf *conf, sector_t sector_nr)
```
所以:

r1conf:nr_pending 增加的位置：
- 在 `raid1_write_request` -> `wait_barrier` -> `_wait_barrier` 中，增加 r1conf::nr_pending
- wait_read_barrier

r1conf:nr_pending 减少的位置：
- `raid_end_bio_io` -> `allow_barrier`

r1conf:nr_sync_pending 增加的位置:
- raid1_sync_request
  - raise_barrier
    - atomic_inc(&conf->nr_sync_pending);

r1conf:nr_sync_pending 减少的位置:
- put_sync_write_buf
  - put_buf
    - lower_barrier

r1conf::nr_queued 增加的位置，也就是当其中的机制为
- reschedule_retry : 其调用位置
  * raid1_end_read_request
  * r1_bio_write_done
  * put_sync_write_buf
  * handle_write_finished

当 `freeze_array` 是需要阻碍住所有的 io ，其使用位置为:
- raid1_remove_disk
- handle_write_finished 被 raid1d 调用
- raid1_reshape
- raid1_quiesce

所以可以知道很多东西:
1. 使用 `get_unqueued_pending` 来判断的: `nr_pending` - `nr_queued` 等于 0 的时候。

### 其他的
1. mddev_suspend 和 mddev_resume : 一个通用操作，来阻塞所有的 io

2. md_wait_for_blocked_rdev 来等待异常盘，这个是不太想干的机制了:
raid1_write_request
- wait_barrier
- wait_blocked_rdev
  - rdev_blocked : 检查是不是需要等待
  - md_wait_for_blocked_rdev


## io 路径
### 基本

```c
const struct block_device_operations md_fops =
{
	.owner		= THIS_MODULE,
	.submit_bio	= md_submit_bio,
```
通过文件系统对于 disk 写的过程:
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_fadvise64
        - ksys_fadvise64_64
          - vfs_fadvise
            - generic_fadvise
              - __filemap_fdatawrite_range
                  - filemap_fdatawrite_wbc
                    - do_writepages
                      - ext4_writepages
                        - ext4_do_writepages
                          - ext4_io_submit
                            - submit_bio_noacct
                              - submit_bio_noacct_nocheck
                                - __submit_bio_noacct
                                  - __submit_bio
                                    - md_handle_request
                                      - raid1_make_request : 注册为 mddev::md_personality::make_request
                                        - md_write_start

如果是普通的设备进行 io ，是通过 `def_blk_fops` 进行的，两者所在的层次不同，使用 block_device_operations::submit_bio 是在文件系统下面的
```c
const struct file_operations def_blk_fops;
```

而 bio 返回的时候为:
- raid1_end_write_request
  - r1_bio_write_done
- raid1_end_read_request
#### read 失败会切换盘

#### 从 bio 的视角


**构建的流程**:
raid1_read_request
raid1_write_request

将上层传入的 bio 放到 r1bio::master_bio
无论是 read 还是 write ，都是使用 bio_alloc_clone 重新 clone 新的 bio 结构体

然后放到 r1bio::bios 中:
```c
struct r1bio {
  // ...
  struct bio		*bios[];
```


**释放的流程**:

- raid1_end_write_request
  - r1_bio_write_done
    - raid_end_bio_io
      - call_bio_endio
      - allow_barrier
      - free_r1bio

最后的位置:
```c
static void free_r1bio(struct r1bio *r1_bio)
{
	struct r1conf *conf = r1_bio->mddev->private;

	put_all_bios(conf, r1_bio);
	mempool_free(r1_bio, &conf->r1bio_pool);
}

static void put_all_bios(struct r1conf *conf, struct r1bio *r1_bio)
{
	int i;

	for (i = 0; i < conf->raid_disks * 2; i++) {
		struct bio **bio = r1_bio->bios + i;
		if (!BIO_SPECIAL(*bio))
			bio_put(*bio);
		*bio = NULL;
	}
}
```

为什么 put_all_bios 中会出现所有的 bio 都是 NULL

常见模式
```txt
[  491.093996] x . . .
[  491.094225] x . . .
[  491.094445] x . . .
[  491.094666] x . . .
[  491.094888] x . . .
```
按道理，写不该是两个盘吗?

**bio 如何定向到其他的盘**

无论是 read 还是 write 都是需要考虑的，实现的地方也很简单，看 reschedule_retry 的调用地方就可以了:
- `raid1_end_write_request` -> r1_bio_write_done
- raid1_end_read_request

### 特殊
- end_sync_write
- end_sync_read

此外，在 drivers/md/ 下 rg "bi_end_io ="
```txt
md.c:		new->bi_end_io = md_end_flush;
md.c:	bio->bi_end_io = super_written;
md.c:	clone->bi_end_io = md_end_clone_io;
```

## superblock 的相关问题

主要是这个问题:
```txt
[   27.770183] md: kicking non-fresh sdc1 from array!
[   28.530056] md/raid1:md127: active with 1 out of 2 mirrors
[   28.538970] md127: detected capacity change from 0 to 91199897600
```

https://realtechtalk.com/md_kicking_nonfresh_sda1_from_array_fixsolution-1014-articles

md_update_sb 的逻辑
```c
sync_sbs(mddev, nospares); // 首先同步内存
md_super_write(mddev,rdev,
				       rdev->sb_start, rdev->sb_size,
				       rdev->sb_page); // 然后在这里真的落盘
```

md_update_sb 中描述了 events 增大和减小，测试发现，两个都是可能的
```c
	/* If this is just a dirty<->clean transition, and the array is clean
	 * and 'events' is odd, we can roll back to the previous clean state */
	if (nospares
	    && (mddev->in_sync && mddev->recovery_cp == MaxSector)
	    && mddev->can_decrease_events
	    && mddev->events != 1) {
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		mddev->events--;
		mddev->can_decrease_events = 0;
	} else {
		/* otherwise we have to go forward and ... */
		mddev->events ++;
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		mddev->can_decrease_events = nospares;
	}
```

## 似乎是 bug 的
`handle_write_finished` 中的注释:

```c
		list_add(&r1_bio->retry_list, &conf->bio_end_io_list);
		/*
		 * In case freeze_array() is waiting for condition
		 * get_unqueued_pending() == extra to be true.
		 */
```
如果将 r1bio 挂到列表中，是不能将 `freeze_array` 直接释放的，此事 reshape，
导致所有的 r1bio 全部都释放。


## 文档的记录

1. 活动 : resync, recovery, or reshape activity
  - resync ?

```txt
       --backup-file=
              This  is  needed  when  --grow  is  used  to increase the number of raid-devices in a RAID5
              or RAID6 if there are no spare devices available, or to shrink, change RAID level or layout.
              See the GROW MODE section below on RAID-DEVICES CHANGES.
              The file must be stored on a separate device, not on the RAID array being reshaped.
```


## raid1 的错误处理是什么样子的
super_written -> super_written

raid1_end_write_request -> md_error

当 md_error 最后调用 raid1_error 来实现盘的隔离

- blk_mq_dispatch_rq_list 中调用完成 blk_mq_ops::queue_rq 之后，直接失败。


难道是因为 failfast 导致的这个 backtrace 不同，现在 nvme 的一个 io 返回之后，就可以立刻出现错误？
，因为 failfast 的话，为什么返回错误的时候可以正确处理。




raid1 的 superblock bio 的也是需要分裂为多个盘吗？

会出现一个盘出现错误，那么整个盘异常吗?
```c
			md_super_write(mddev,rdev,
				       rdev->sb_start, rdev->sb_size,
				       rdev->sb_page);
```


```txt
➜  ~ cat /sys/devices/virtual/block/md0/inflight

       0       97
```

一个经典例子，遇到错误之后，切换到另外一个 io 上:
```txt
[root@bogon 15:33:38 ~]$ dmesg | grep md12
[    1.642677] md/raid1:md127: active with 1 out of 2 mirrors
[    1.649104] md127: detected capacity change from 0 to 91199897600
[    1.693678] EXT4-fs (md127): mounted filesystem with ordered data mode. Opts: (null)
[    2.083742] EXT4-fs (md127): re-mounted. Opts: (null)
[   32.050519] md: recovery of RAID array md127
[   44.504678] md: md127: recovery done.
[  132.241968] md/raid1:md127: Disk failure on sda1, disabling device.
md/raid1:md127: Operation continuing on 1 devices.
[  240.717878] INFO: task md127_raid1:644 blocked for more than 120 seconds.
[  240.717956] md127_raid1     D ffffa03edf21acc0     0   644      2 0x00000000
[  240.717984] INFO: task jbd2/md127-8:666 blocked for more than 120 seconds.
[  240.718062] jbd2/md127-8    D ffffa03edf59acc0     0   666      2 0x00000000
[  335.314148] md/raid1:md127: sda1: rescheduling sector 26185304
[  335.314192] md/raid1:md127: sda1: rescheduling sector 24574632
[  335.323293] md/raid1:md127: redirecting sector 26185304 to other mirror: sdb1
[  335.323299] md/raid1:md127: redirecting sector 24574632 to other mirror: sdb1
[  356.253347] md/raid1:md127: sda1: rescheduling sector 10066760
[  356.253386] md/raid1:md127: redirecting sector 10066760 to other mirror: sdb1
```

- handle_write_finished
  - narrow_write_error
    - submit_bio_wait

## 真的还是没有搞清楚为什么

`_wait_barrier` 中的同步技术

```txt
	/*
	 * Don't worry about checking two atomic_t variables at same time
	 * here. If during we check conf->barrier[idx], the array is
	 * frozen (conf->array_frozen is 1), and chonf->barrier[idx] is
	 * 0, it is safe to return and make the I/O continue. Because the
	 * array is frozen, all I/O returned here will eventually complete
	 * or be queued, no race will happen. See code comment in
	 * frozen_array().
	 */
	if (!READ_ONCE(conf->array_frozen) &&
	    !atomic_read(&conf->barrier[idx]))
		return ret;
```
这里的 array_frozen 为什么需要添加 READ_ONCE ，
像不到任何情况，这个东西会被优化掉?

是为了防止乱序编译吗?

两个 read 在一起，确实非常奇怪
```c
	if (!READ_ONCE(conf->array_frozen) &&
	    !atomic_read(&conf->barrier[idx]))
		return ret;
```

## 其他问题
为什么会 一次 raid1_write_request 21 put_all_bios

使用 echo 1 | sudo tee /dev/md127 测试

因为是使用的内核 thread 来进行 sync ， 如果 io 提交了，但是没有返回，然后直接关机，kthread 需要等待这些 io 返回才可以正常关机吗?


- `RESYNC_DEPTH`


看上去，mddev 都是 bio 的，但是 plug 机制中都是处理 request 的

才发现，原来是存在三种 bio 的:
1. 从文件系统中来的原始的 bio
2. mbio
3. rdev bio


## raid1 基本 io 过程
<!-- 377a5af6-196e-4621-9c5f-f9cc33a92b8a -->

如果从 raid1_make_request 分析代码，发现没有进一步调用到下层路径，
实际上是由于 plug 和 unplug 机制，这些路径变得有点隐晦了。

raid1_make_request 自己会攒一些，然后调用 raid1_unplug ，raid1_unplug 调用 block layer
最后 blk_finish_plug 真的完成提交


```txt
     95.15% syscall
        entry_SYSCALL_64_after_hwframe
      - do_syscall_64
         - 90.70% __x64_sys_io_submit
            - 88.07% io_submit_one.constprop.0
               - 84.55% aio_write
                  - 83.29% ext4_file_write_iter
                     - 80.42% iomap_dio_rw
                        - __iomap_dio_rw
                           - 58.86% blk_finish_plug
                              - __blk_flush_plug
                                 - 51.80% blk_mq_flush_plug_list
                                    - blk_mq_dispatch_queue_requests
                                       - 50.53% nvme_queue_rqs
                                          - 49.40% nvme_submit_cmds.part.0
                                             - 0.60% _raw_spin_lock
                                                  lock_acquire
                                            0.51% nvme_prep_rq
                                 - 6.66% raid1_unplug
                                    - flush_bio_list
                                       - 3.45% submit_bio_noacct_nocheck
                                          - 3.39% __submit_bio
                                             - blk_mq_submit_bio
                                                  0.78% lock_acquire
                                                  0.76% ktime_get
                                       - 1.91% bio_associate_blkg
                                          - 1.25% bio_associate_blkg_from_css
                                               0.73% lock_acquire
                                            0.50% lock_acquire
                                         0.56% submit_bio_noacct
                           - 18.60% iomap_dio_bio_iter
                              - 13.57% submit_bio_noacct_nocheck
                                 - 13.49% __submit_bio
                                    - 12.24% md_handle_request
                                       - 11.53% raid1_make_request
                                          - 10.51% raid1_write_request
                                             - 4.64% bio_alloc_clone
                                                - 3.34% bio_alloc_bioset
                                                   - 2.05% bio_associate_blkg
                                                        0.79% blkcg_css.part.0
                                                        0.72% bio_associate_blkg_from_css
                                                     1.23% mempool_alloc_noprof
                                                - 1.26% bio_associate_blkg_from_css
                                                     0.72% lock_acquire
                                             - 3.96% md_account_bio
                                                - 2.63% bio_alloc_clone
                                                   - 1.86% bio_alloc_bioset
                                                        1.04% bio_associate_blkg
                                                        0.79% mempool_alloc_noprof
                                                     0.70% bio_associate_blkg_from_css
                                                - 0.84% bitmap_start_write
                                                     0.75% _raw_spin_unlock_irq
                                               0.78% mempool_alloc_noprof
                                            0.93% md_write_start
                                      0.60% lock_acquire
                              - 2.45% bio_alloc_bioset
                                 - 1.25% mempool_alloc_noprof
                                    - 0.53% fs_reclaim_acquire
                                         lock_acquire
                                   1.14% bio_associate_blkg
                              - 2.10% bio_iov_iter_get_pages
                                 - 1.97% iov_iter_extract_pages
                                      1.93% gup_fast_fallback
                           - 1.27% iomap_iter
                              - 1.06% ext4_iomap_overwrite_begin
                                 - ext4_iomap_begin
                                      0.95% ext4_map_blocks
                           - 1.16% __kmalloc_cache_noprof
                              - 0.56% fs_reclaim_acquire
                                   lock_acquire
                       0.96% ext4_map_blocks
                     - 0.69% file_modified
                          0.50% inode_needs_update_time.part.0
                       0.58% down_read
               - 1.26% kmem_cache_alloc_noprof
                  - 0.69% fs_reclaim_acquire
                       lock_acquire
                 0.63% _copy_from_user
              1.56% lookup_ioctx
```

如果直接提交，不用 plug ，那么是这样的，制作一个 rb
```c
static inline void raid1_submit_write(struct bio *bio)
{
	struct md_rdev *rdev = (void *)bio->bi_bdev;

	bio->bi_next = NULL;
	bio_set_dev(bio, rdev->bdev);
	if (test_bit(Faulty, &rdev->flags))
		bio_io_error(bio);
	else if (unlikely(bio_op(bio) ==  REQ_OP_DISCARD &&
			  !bdev_max_discard_sectors(bio->bi_bdev)))
		/* Just ignore it */
		bio_endio(bio);
	else
		submit_bio_noacct(bio);
}
```

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
