# blk layer lock
virtio_queue_rq 中上锁情况:
```txt
[    3.626324] 4 locks held by (udev-worker)/307:
[    3.626947]  #0: ffff888102398358 (&p->lock){+.+.}-{4:4}, at: seq_read_iter+0x56/0x450
[    3.627870]  #1: ffff88810cc53e88 (&of->mutex){+.+.}-{4:4}, at: kernfs_seq_start+0x25/0xf0
[    3.628816]  #2: ffff8881116b04d8 (kn->active#36){.+.+}-{0:0}, at: kernfs_seq_start+0x2e/0xf0
[    3.629786]  #3: ffffffff82559f20 (rcu_read_lock){....}-{1:3}, at: blk_mq_run_hw_queue+0x130/0x2d0
```
很坑，只有 udev 才走这个

## 分析 virtio_queue_rqs 中的上锁情况
unsigned long jiffies_at_begin = 0;

```c
	if (time_after(jiffies, jiffies_at_begin + HZ * 2)) {
		debug_show_held_locks(current);
		jiffies_at_begin = jiffies;
	}
```

我想，定义有那种统计的方法的，就是知道一个点到底当时获取了多少 lock 的统计


在 virtio_queue_rqs 中的 lock
```txt
[   53.578143] 2 locks held by systemd-udevd/576:
[   53.578997]  #0: ffff8881129b2358 (&disk->open_mutex){+.+.}-{4:4}, at: bdev_open+0x2c6/0x430
[   53.580188]  #1: ffffffff82559f20 (rcu_read_lock){....}-{1:3}, at: blk_mq_flush_plug_list+0xb1a/0xca0

[   55.582266] 1 lock held by fio/1714:
[   55.582787]  #0: ffffffff82559f20 (rcu_read_lock){....}-{1:3}, at: blk_mq_flush_plug_list+0xb1a/0xca0

[   57.584259] 1 lock held by fio/1714:
[   57.585004]  #0: ffffffff82559f20 (rcu_read_lock){....}-{1:3}, at: blk_mq_flush_plug_list+0xb1a/0xca0

[   59.586255] 1 lock held by fio/1714:
[   59.586761]  #0: ffffffff82559f20 (rcu_read_lock){....}-{1:3}, at: blk_mq_flush_plug_list+0xb1a/0xca0
```

实在是没有想到，这么深的调用，最后持有的锁只有，blk_mq_flush_plug_list ，而且还是 rcu lock 。

vda 一共 8 个队列，当时 8 个 CPU :
```txt
@[
        virtio_queue_rqs+5
        blk_mq_flush_plug_list+2892
        __blk_flush_plug+211
        __submit_bio+696
        submit_bio_noacct_nocheck+790
        blkdev_direct_IO+1432
        blkdev_write_iter+533
        aio_write+346
        io_submit_one.constprop.0+1557
        __x64_sys_io_submit+175
        do_syscall_64+114
        entry_SYSCALL_64_after_hwframe+118
]: 173393
```

很容易可以找到，唯一的 rcu lock 就是 tag_set 中的 srcu:

```c
/* run the code block in @dispatch_ops with rcu/srcu read lock held */
#define __blk_mq_run_dispatch_ops(q, check_sleep, dispatch_ops)	\
do {								\
	if ((q)->tag_set->flags & BLK_MQ_F_BLOCKING) {		\
		struct blk_mq_tag_set *__tag_set = (q)->tag_set; \
		int srcu_idx;					\
								\
		might_sleep_if(check_sleep);			\
		srcu_idx = srcu_read_lock(__tag_set->srcu);	\
		(dispatch_ops);					\
		srcu_read_unlock(__tag_set->srcu, srcu_idx);	\
	} else {						\
		rcu_read_lock();				\
		(dispatch_ops);					\
		rcu_read_unlock();				\
	}							\
} while (0)
```

先不继续深入了，但是有一个问题:

1. 如果拔盘，然后继续向内核中提交，由于 rcu 的存在，所以，tag_set 不会消失，


这个是 rcu 的唯一的使用地方，继续从这个角度看看问题吧
```c
/**
 * blk_mq_wait_quiesce_done() - wait until in-progress quiesce is done
 * @set: tag_set to wait on
 *
 * Note: it is driver's responsibility for making sure that quiesce has
 * been started on or more of the request_queues of the tag_set.  This
 * function only waits for the quiesce on those request_queues that had
 * the quiesce flag set using blk_mq_quiesce_queue_nowait.
 */
void blk_mq_wait_quiesce_done(struct blk_mq_tag_set *set)
{
	if (set->flags & BLK_MQ_F_BLOCKING)
		synchronize_srcu(set->srcu);
	else
		synchronize_rcu();
}
```

### 如果正在提交的时候，结果盘拔掉了(无视任何 lock ，咋办)

### 在 qemu 中热插一个 virtio blk 的时候

```txt
[128608.885507] pci 0000:00:0c.0: [1af4:1001] type 00 class 0x010000 conventional PCI endpoint
[128608.887080] pci 0000:00:0c.0: BAR 0 [io  0x0000-0x007f]
[128608.887704] pci 0000:00:0c.0: BAR 1 [mem 0x00000000-0x00000fff]
[128608.888592] pci 0000:00:0c.0: BAR 4 [mem 0x00000000-0x00003fff 64bit pref]
[128608.892871] pci 0000:00:0c.0: BAR 4 [mem 0xe0000008000-0xe000000bfff 64bit pref]: assigned
[128608.894056] pci 0000:00:0c.0: BAR 1 [mem 0xc0000000-0xc0000fff]: assigned
[128608.894900] pci 0000:00:0c.0: BAR 0 [io  0x1000-0x107f]: assigned
[128608.895982] virtio-pci 0000:00:0c.0: enabling device (0000 -> 0003)
[128608.904339] virtio_blk virtio9: 8/0/0 default/read/poll queues
[128608.911838] virtio_blk virtio9: [vdb] 8388608000 512-byte logical blocks (4.29 TB/3.91 TiB)
[128608.924164] 9 locks held by kworker/u512:4/22922:
[128608.924851]  #0: ffff888101050d48 ((wq_completion)kacpi_hotplug){+.+.}-{0:0}, at: process_one_work+0x4f1/0x570
[128608.926367]  #1: ffffc90003e2be30 ((work_completion)(&hpw->work)){+.+.}-{0:0}, at: process_one_work+0x1b7/0x570
[128608.927899]  #2: ffffffff825e5508 (device_hotplug_lock){+.+.}-{4:4}, at: acpi_device_hotplug+0x2a/0x440
[128608.929286]  #3: ffffffff825d7488 (acpi_scan_lock){+.+.}-{4:4}, at: acpi_device_hotplug+0x38/0x440
[128608.930630]  #4: ffffffff825d4288 (pci_rescan_remove_lock){+.+.}-{4:4}, at: acpiphp_hotplug_notify+0x95/0x270
[128608.932080]  #5: ffff888105ba7198 (&dev->mutex){....}-{4:4}, at: __device_attach+0x33/0x1b0
[128608.933246]  #6: ffff88810a047170 (&dev->mutex){....}-{4:4}, at: __device_attach+0x33/0x1b0
[128608.934457]  #7: ffff88814dc24b58 (&disk->open_mutex){+.+.}-{4:4}, at: bdev_open+0x2c6/0x430
[128608.935725]  #8: ffffffff82559f20 (rcu_read_lock){....}-{1:3}, at: blk_mq_flush_plug_list+0xb1a/0xca0
```
每一个锁的位置都可以看看。

为什么这个 dev->mutex 上来两次锁啊

## scsi 的 delete 必须等待所有的 io 返回才可以成功

如果 virtscsi_eh_timed_out 设置为重试
```c
static enum scsi_timeout_action virtscsi_eh_timed_out(struct scsi_cmnd *scmd)
{
	return SCSI_EH_RESET_TIMER;
}
```
实际上，内核会卡在此处:
```txt
[<0>] blk_mq_freeze_queue_wait+0x96/0xd0
[<0>] del_gendisk+0x257/0x390
[<0>] sd_remove+0x2f/0x60
[<0>] device_release_driver_internal+0x19f/0x200
[<0>] bus_remove_device+0xc4/0x100
[<0>] device_del+0x15c/0x490
[<0>] __scsi_remove_device+0x12a/0x180
[<0>] sdev_store_delete+0x6a/0xd0
[<0>] kernfs_fop_write_iter+0x10c/0x1f0
[<0>] vfs_write+0x24a/0x440
[<0>] ksys_write+0x6f/0xf0
[<0>] do_syscall_64+0x3b/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x6e/0xd8
```
io 总是不返回，但是如果修改为:

```c
static enum scsi_timeout_action virtscsi_eh_timed_out(struct scsi_cmnd *scmd)
{
	scsi_print_command(scmd);

	if (time_after(jiffies, scmd->jiffies_at_alloc +
				(30 * 2) * HZ)) {
		return SCSI_EH_NOT_HANDLED;
	}
	return SCSI_EH_RESET_TIMER;
}
```

delete 也是会卡在各种位置，例如:
```txt
[<0>] blk_execute_rq+0x147/0x220
[<0>] scsi_execute_cmd+0x15c/0x2d0
[<0>] sd_sync_cache+0xf1/0x1e0
[<0>] sd_shutdown+0x6a/0x100
[<0>] sd_remove+0x55/0x60
[<0>] device_release_driver_internal+0x19f/0x200
[<0>] bus_remove_device+0xc4/0x100
[<0>] device_del+0x15c/0x490
[<0>] __scsi_remove_device+0x12a/0x180
[<0>] sdev_store_delete+0x6a/0xd0
[<0>] kernfs_fop_write_iter+0x10c/0x1f0
[<0>] vfs_write+0x24a/0x440
[<0>] ksys_write+0x6f/0xf0
[<0>] do_syscall_64+0x3b/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x6e/0xd8
```

- sdev_store_delete
  - scsi_remove_device
    - `__scsi_remove_device`
      - transport_remove_device
      - blk_mq_destroy_queue(sdev->request_queue);
      - sdev->host->hostt->slave_destroy

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - ksys_write
        - vfs_write
          - new_sync_write
            - call_write_iter
              - kernfs_fop_write_iter
                - sdev_store_delete
                  - scsi_remove_device
                    - __scsi_remove_device
                      - transport_remove_device
                        - attribute_container_device_trigger
                          - transport_remove_classdev

## blk_mq_quiesce_queue

## blk_mq_freeze_queue

- [ ] 为什么 `blk_freeze_queue_start` 调用 blk_mq_run_hw_queues 来刷新，而不是刷新软件的 queue

应该等待在 struct request_queue::mq_freeze_wq 应该在位置被阻塞，但是测试了一下，都无法调用到:
- blk_queue_enter
- `__bio_queue_enter`
- blk_mq_freeze_queue_wait

当 ref = 0，调用:
```c
static void blk_queue_usage_counter_release(struct percpu_ref *ref)
{
	struct request_queue *q =
		container_of(ref, struct request_queue, q_usage_counter);

	wake_up_all(&q->mq_freeze_wq);
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
