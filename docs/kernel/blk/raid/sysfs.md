# syfs md
## /proc/mdstat

公共的入口 : md_seq_show
```c
		if (mddev->pers) {
			mddev->pers->status(seq, mddev);
			seq_printf(seq, "\n      ");
			if (mddev->pers->sync_request) {
				if (status_resync(seq, mddev))
					seq_printf(seq, "\n      ");
			}
		} else
			seq_printf(seq, "\n       ");
```

```c
static void raid1_status(struct seq_file *seq, struct mddev *mddev)
{
	struct r1conf *conf = mddev->private;
	int i;

	seq_printf(seq, " [%d/%d] [", conf->raid_disks,
		   conf->raid_disks - mddev->degraded);
	rcu_read_lock();
	for (i = 0; i < conf->raid_disks; i++) {
		struct md_rdev *rdev = rcu_dereference(conf->mirrors[i].rdev);
		seq_printf(seq, "%s",
			   rdev && test_bit(In_sync, &rdev->flags) ? "U" : "_");
	}
	rcu_read_unlock();
	seq_printf(seq, "]");
}
```
简单清晰

bitmap 相关的:
md_bitmap_status

## sys 的接口
/sys/devices/virtual/block/md10/md

他们的作用罗列在: https://www.kernel.org/doc/html/latest/admin-guide/md.html 中

### ./md
```txt
➜  md10 tree
├── array_size
├── array_state
├── bitmap
│   ├── backlog
│   ├── can_clear
│   ├── chunksize
│   ├── location
│   ├── max_backlog_used
│   ├── metadata
│   ├── space
│   └── time_base
├── bitmap_set_bits
├── chunk_size
├── component_size
├── consistency_policy
├── degraded
├── dev-sda1
│   ├── bad_blocks
│   ├── block -> ../../../../../pci0000:00/0000:00:0a.0/virtio5/host0/target0:0:0/0:0:0:0/block/sda/sda1
│   ├── errors
│   ├── new_offset
│   ├── offset
│   ├── ppl_sector
│   ├── ppl_size
│   ├── recovery_start
│   ├── size
│   ├── slot
│   ├── state
│   └── unacknowledged_bad_blocks
├── fail_last_dev
├── last_sync_action
├── layout
├── level
├── max_read_errors
├── metadata_version
├── mismatch_cnt
├── new_dev
├── raid_disks
├── rd0 -> dev-sda1
├── reshape_direction
├── reshape_position
├── resync_start
├── safe_mode_delay
├── serialize_policy
├── suspend_hi
├── suspend_lo
├── sync_action
├── sync_completed
├── sync_force_parallel
├── sync_max
├── sync_min
├── sync_speed
├── sync_speed_max
├── sync_speed_min
└── uuid
```

通过
```c
static struct attribute *md_default_attrs[] = {
	&md_level.attr,
	&md_layout.attr,
	&md_raid_disks.attr,
	&md_uuid.attr,
	&md_chunk_size.attr,
	&md_size.attr,
	&md_resync_start.attr,
	&md_metadata.attr,
	&md_new_device.attr,
	&md_safe_delay.attr,
	&md_array_state.attr,
	&md_reshape_position.attr,
	&md_reshape_direction.attr,
	&md_array_size.attr,
	&max_corr_read_errors.attr,
	&md_consistency_policy.attr,
	&md_fail_last_dev.attr,
	&md_serialize_policy.attr,
	NULL,
};
```

```c
static struct attribute *md_redundancy_attrs[] = {
	&md_scan_mode.attr,
	&md_last_scan_mode.attr,
	&md_mismatches.attr,
	&md_sync_min.attr,
	&md_sync_max.attr,
	&md_sync_speed.attr,
	&md_sync_force_parallel.attr,
	&md_sync_completed.attr,
	&md_min_sync.attr,
	&md_max_sync.attr,
	&md_suspend_lo.attr,
	&md_suspend_hi.attr,
	&md_bitmap.attr,
	&md_degraded.attr,
	NULL,
};
```


#### raid_disks

可以用来修改阵列的槽的数量
```c
static struct md_sysfs_entry md_raid_disks =
__ATTR(raid_disks, S_IRUGO|S_IWUSR, raid_disks_show, raid_disks_store);
```

#### array_state

想不到有这么多属性:
```c
static char *array_states[] = {
	"clear", "inactive", "suspended", "readonly", "read-auto", "clean", "active",
	"write-pending", "active-idle", "broken", NULL };
```

```c
enum md_ro_state {
	MD_RDWR,
	MD_RDONLY,
	MD_AUTO_READ,
	MD_MAX_STATE
};
```

对于 raid1 ，只是检查输出为 clean 和 active 的盘
具体含义，以后再看了。

#### array_size

默认 default ，也就是如果下面的 device 有大多，
那么这个 raid 就会有多大

#### resync_start

应该是记录上次 resync 所在的位置

#### sync_action
```c
static struct md_sysfs_entry md_scan_mode =
__ATTR_PREALLOC(sync_action, S_IRUGO|S_IWUSR, action_show, action_store);
```

action_store 中指出的动作:
```c
static const char *action_name[NR_SYNC_ACTIONS] = {
	[ACTION_RESYNC]		= "resync", // 通过 sysfs 触发，对于 raid1 没有意义
	[ACTION_RECOVER]	= "recover", //
	[ACTION_CHECK]		= "check", // mdcheck 来触发这个操作
	[ACTION_REPAIR]		= "repair", // check 自动会有这个动作
	[ACTION_RESHAPE]	= "reshape", // 对于 raid1 没有意义，直接返回 Invalid argument
	[ACTION_FROZEN]		= "frozen",
	[ACTION_IDLE]		= "idle",
};
```
基本逻辑就是根据动作，在 mddev::recovery 上设置 flag ，然后
通过唤醒
```txt
			clear_bit(MD_RECOVERY_FROZEN, &mddev->recovery);
      md_wakeup_thread(mddev->thread); // raid1d
```
大致的执行路线为:
raid1d -> md_check_recovery -> md_start_sync -> md_do_sync
动作的含义参考 enum sync_action 中的注释


而 action_show 的逻辑
```c
action_show(struct mddev *mddev, char *page)
{
	enum sync_action action = md_sync_action(mddev);

	return sprintf(page, "%s\n", md_sync_action_name(action));
}
```

会根据 enum recovery_flags 计算到 action ，然后通过 action_name 展示
是的，不是写什么下去，就会得到什么东西。
```c
enum recovery_flags {
	/*
	 * If neither SYNC or RESHAPE are set, then it is a recovery.
	 */
	MD_RECOVERY_RUNNING,	/* a thread is running, or about to be started */
	MD_RECOVERY_SYNC,	/* actually doing a resync, not a recovery */
	MD_RECOVERY_RECOVER,	/* doing recovery, or need to try it. */
	MD_RECOVERY_INTR,	/* resync needs to be aborted for some reason */
	MD_RECOVERY_DONE,	/* thread is done and is waiting to be reaped */
	MD_RECOVERY_NEEDED,	/* we might need to start a resync/recover */
	MD_RECOVERY_REQUESTED,	/* user-space has requested a sync (used with SYNC) */
	MD_RECOVERY_CHECK,	/* user-space request for check-only, no repair */
	MD_RECOVERY_RESHAPE,	/* A reshape is happening */
	MD_RECOVERY_FROZEN,	/* User request to abort, and not restart, any action */
	MD_RECOVERY_ERROR,	/* sync-action interrupted because io-error */
	MD_RECOVERY_WAIT,	/* waiting for pers->start() to finish */
	MD_RESYNCING_REMOTE,	/* remote node is running resync thread */
};
```
- MD_RECOVERY_INTR : 看上去是出现错误的时候，设置上这个 flags ，
让 sync_thread 停止

> [!NOTE]
> 参考 Deepseeek ，有待验证

- resync 是 RAID1 中的一种维护操作，目的是确保所有镜像磁盘上的数据完全一致。它通常在阵列处于正常运行状态时执行。
- recover 是 RAID1 中的一种故障恢复操作，专门用于在磁盘故障或更换后重建阵列的冗余。它在阵列处于降级状态（即一个或多个磁盘故障）时执行。

#### layout
含义取决于具体是什么 raid ，对于 raid1 没啥意义。

在 raid1_reshape 中，需要保证这些内容不可以修改:
```c
	/* Cannot change chunk_size, layout, or level */
	if (mddev->chunk_sectors != mddev->new_chunk_sectors ||
	    mddev->layout != mddev->new_layout ||
	    mddev->level != mddev->new_level) {
		mddev->new_chunk_sectors = mddev->chunk_sectors;
		mddev->new_layout = mddev->layout;
		mddev->new_level = mddev->level;
		return -EINVAL;
	}
```

#### level
```c
static struct md_sysfs_entry md_level =
__ATTR(level, S_IRUGO|S_IWUSR, level_show, level_store);
```
到底是那种 raid ，甚至可以修改这个文件实现

### ./md/rdev
```c
static struct attribute *rdev_default_attrs[] = {
	&rdev_state.attr,
	&rdev_errors.attr,
	&rdev_slot.attr,
	&rdev_offset.attr,
	&rdev_new_offset.attr,
	&rdev_size.attr,
	&rdev_recovery_start.attr,
	&rdev_bad_blocks.attr,
	&rdev_unack_bad_blocks.attr,
	&rdev_ppl_sector.attr,
	&rdev_ppl_size.attr,
	NULL,
};
ATTRIBUTE_GROUPS(rdev_default);
```

#### rdev::state
```c
static struct rdev_sysfs_entry rdev_state =
__ATTR_PREALLOC(state, S_IRUGO|S_IWUSR, state_show, state_store);


static const struct kobj_type rdev_ktype = {
	.release	= rdev_free,
	.sysfs_ops	= &rdev_sysfs_ops,
	.default_groups	= rdev_default_groups,
};
```

```sh
cat /sys/devices/virtual/block/md10/md/dev-sda1/state
```

### ./md/bitmap
```c
static struct attribute *md_bitmap_attrs[] = {
	&bitmap_location.attr,
	&bitmap_space.attr,
	&bitmap_timeout.attr,
	&bitmap_backlog.attr,
	&bitmap_chunksize.attr,
	&bitmap_metadata.attr,
	&bitmap_can_clear.attr,
	&max_backlog_used.attr,
	NULL
};
```



### 其他说明
1. 对于 raid1 ，echo reshape 进去没有意义，只有注册了 start_reshape 的才有意义，
例如 raid5 和 raid10

2. serialize_policy

> [!NOTE]
> 参考 Deepseeek ，有待验证

serialize_policy 是 Linux 软件 RAID (mdadm) 中的一个参数，它控制着 RAID 阵列如何处理并发的 I/O 请求。
这个策略主要影响 RAID 5 和 RAID 6 这类需要奇偶校验计算的阵列。

主要作用
- 控制 I/O 请求的序列化：决定是否以及如何对到达的 I/O 请求进行排序处理
- 影响性能和数据一致性：不同的策略会在性能和数据安全之间做出不同权衡


3. 如果加了很多盘到 raid ，而阵列的大小为 2 的时候:
```txt
drwxr-xr-x     - root 27 Jul 15:42   dev-nvme0n1p1
drwxr-xr-x     - root 27 Jul 15:42   dev-nvme0n1p2
drwxr-xr-x     - root 27 Jul 15:42   dev-nvme0n1p3
drwxr-xr-x     - root 27 Jul 15:42   dev-nvme0n1p4
drwxr-xr-x     - root 27 Jul 15:42   dev-nvme1n1p1
drwxr-xr-x     - root 27 Jul 15:42   dev-nvme1n1p2
drwxr-xr-x     - root 27 Jul 15:42   dev-nvme1n1p3
drwxr-xr-x     - root 27 Jul 15:42   dev-nvme1n1p4
lrwxrwxrwx     - root 27 Jul 15:42   rd0 -> dev-nvme0n1p1
lrwxrwxrwx     - root 27 Jul 15:42   rd1 -> dev-nvme1n1p1
```

### mdcheck 做的工作

> [!NOTE]
> 参考 Deepseeek ，有待验证

/sys/block/md127/md/mismatch_cnt：记录检查过程中发现的不一致扇区数。每次检查开始时清零，检查过程中如果发现不一致，计数会增加。
/sys/block/md127/md/sync_completed：显示检查或同步的进度（已完成扇区数/总扇区数），支持 select 或 poll 系统调用以监控进度。
/sys/block/md127/md/sync_min 和 sync_max ：定义检查或修复操作的扇区范围，必须是块大小（chunk_size）的倍数。


strace 一下，可以看到这些东西，ds 说的应该是很正确的了:
```txt
8110  openat(AT_FDCWD, "/sys/block/md1/md/raid_disks", O_RDONLY) = 4
8110  openat(AT_FDCWD, "/sys/block/md1/md/array_state", O_RDONLY) = 4
8110  openat(AT_FDCWD, "/sys/block/md1/md/dev-nvme1n1p1/slot", O_RDONLY) = 5
8110  openat(AT_FDCWD, "/sys/block/md1/md/dev-nvme1n1p1/block/dev", O_RDONLY) = 5
8110  openat(AT_FDCWD, "/sys/block/md1/md/dev-nvme1n1p1/state", O_RDONLY) = 5
8110  openat(AT_FDCWD, "/sys/block/md1/md/metadata_version", O_RDONLY) = 4
8103  openat(AT_FDCWD, "/sys/devices/virtual/block/md1/md/sync_min", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3
8103  openat(AT_FDCWD, "/sys/devices/virtual/block/md1/md/sync_action", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3
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
