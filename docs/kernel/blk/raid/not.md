## 多个 raid 共享相同的物理盘会带来什么问题？
md: delaying resync of md126 until md127 has finished (they share one or more physical units)

似乎没什么问题

## raid1 和 raid10 如此代码如此相似

## unraid 内核模块
https://github.com/qvr/nonraid

## raid5

据说最常用的 raid 配置是 raid5

https://en.wikipedia.org/wiki/Standard_RAID_levels#RAID_5

### 分析下为什么 check_reshape start_reshape finish_reshape

- check_sb_changes 中 start_reshape 没有配对的首先 check_reshape
- action_store 调用 start_reshape
	- 因为 check_sb_changes 都是 cluster 在调用，所以只有实际上，start_reshape 只有这个接口会可以用的


- md_reap_sync_thread 中唯一调用 finish_reshape

- raid5_check_reshape
	- check_reshape
		- resize_chunks
		- resize_stripes : 重新构建盘

- raid5_start_reshape : 修改配置
- raid5_finish_reshape : 修改配置

## 专业的讨论
https://lore.kernel.org/linux-raid/20240308093726.1047420-1-yukuai1@huaweicloud.com/T/#meac4ec0c88182929830f7f8903871f615fce4585

## 有时候存在这个报错

```txt
[ 2862.632964] md: could not open unknown-block(8,1).
[ 2862.633525] md: md_import_device returned -16
[ 2862.634200] md: could not open unknown-block(8,1).
[ 2862.635155] md: md_import_device returned -16
```

## 为什么 raid1_reshape 最后需要这三行

```c
	set_bit(MD_RECOVERY_RECOVER, &mddev->recovery);
	set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
	md_wakeup_thread(mddev->thread);
```
让 mddev->thread 运行起来，并且持有 `MD_RECOVERY_NEEDED` thread

- [ ] 这两个 flag 有区别吗?
	- MD_RECOVERY_NEEDED : 调用位置特别多
	- md_check_recovery 中 clear 掉这个

分析 MD_RECOVERY_NEEDED 的行为，说实话，不是特别理解。


### 原来是存在什么道理啊?
- reshape 是增减了盘的，希望启动 md_check_recovery 来将修复信息

修复什么信息?


### check_reshape 调用位置总结
- layout_store
- action_store
- chunk_size_store
- md_ioctl
	__md_set_array_info
		update_array_info
- md_check_recovery


- md_ioctl
	__md_set_array_info
		update_array_info
			update_raid_disks

- process_metadata_update
	md_reload_sb
		check_sb_changes
			update_raid_disks
- raid_disks_store
	update_raid_disks

### 分析下 mddev::delta_disks

正经的的位置: update_raid_disks


mdadm --grow --force --raid-devices=3 /dev/md10

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_ioctl
        - __se_sys_ioctl
          - __do_sys_ioctl
            - vfs_ioctl
              - blkdev_ioctl
                - md_ioctl
                  - __md_set_array_info
                    - update_array_info
                      - update_raid_disks


## raid1_add_disk 有必要这么复杂吗?

这个 err 判断该如何理解?
```c
	if (err && repl_slot >= 0) {
		/* Add this device as a replacement */
		clear_bit(In_sync, &rdev->flags);
		set_bit(Replacement, &rdev->flags);
		raid1_add_conf(conf, rdev, repl_slot, true);
		err = 0;
		conf->fullsync = 1;
	}
```
这个 err 没必要判断吧，可以走到这里，这里的 err 必然是空的。

## 为什么需要考虑 max_sectors ，然后需要将 bio 分裂开

raid1_read_request
```c
	if (max_sectors < bio_sectors(bio)) {
		struct bio *split = bio_split(bio, max_sectors,
					      gfp, &conf->bio_split);
		bio_chain(split, bio);
		submit_bio_noacct(bio);
		bio = split;
		r1_bio->master_bio = bio;
		r1_bio->sectors = max_sectors;
	}
```

raid1_write_request

```c
	if (max_sectors < bio_sectors(bio)) {
		struct bio *split = bio_split(bio, max_sectors,
					      GFP_NOIO, &conf->bio_split);
		bio_chain(split, bio);
		submit_bio_noacct(bio);
		bio = split;
		r1_bio->master_bio = bio;
		r1_bio->sectors = max_sectors;
	}
```
- 为什么发生 split
	- 应该是 md 的 bio 包含的 sector 超过了 rdev

## md cluster

> The Distributed Lock Manager (DLM) in the kernel is the base component used by OCFS2, GFS2, Cluster MD, and Cluster LVM (lvmlockd) to provide active-active storage at each respective layer.

[Cluster support for MD/RAID 1](https://lwn.net/Articles/674085/)

基于 DLM 自动使用远程的磁盘作为来制作 raid1

到处都是 mddev_is_clustered 的判断，真的很烦的

- process_metadata_update : 唯一的来自于 drivers/md/md-cluster.c
	- md_reload_sb
		- check_sb_changes

## 如何理解 : is_mddev_idle

## [ ] 如何理解下 commit
```diff
History:        #0
Commit:         985ca973b68cac0adfa83497db231da7f99c6ed9
Author:         NeilBrown <neilb@suse.com>
Author Date:    Mon 06 Jul 2015 10:26:57 AM CST
Committer Date: Tue 01 Sep 2015 01:30:40 AM CST

md: close some races between setting and checking sync_action.

When checking sync_action in a script, we want to be sure it is
as accurate as possible.
As resync/reshape etc doesn't always start immediately (a separate
thread is scheduled to do it), it is best if 'action_show'
checks if MD_RECOVER_NEEDED is set (which it does) and in that
case reports what is likely to start soon (which it only sometimes
does).

So:
 - report 'reshape' if reshape_position suggests one might start.
 - set MD_RECOVERY_RECOVER in raid1_reshape(), because that is very
   likely to happen next.

Signed-off-by: NeilBrown <neilb@suse.com>

diff --git a/drivers/md/raid1.c b/drivers/md/raid1.c
index 967a4ed73929..742b50794dfd 100644
--- a/drivers/md/raid1.c
+++ b/drivers/md/raid1.c
@@ -3113,6 +3113,7 @@ static int raid1_reshape(struct mddev *mddev)

 	unfreeze_array(conf);

+	set_bit(MD_RECOVERY_RECOVER, &mddev->recovery);
 	set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
 	md_wakeup_thread(mddev->thread);

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
