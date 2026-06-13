## sync

似乎在最开始创建的时候，存在如下的内容:
- raid1d
  - sync_request_write

### 这两个函数的关联是什么?
- raid1_sync_request
- sync_request_write -> end_sync_write


### 众所周知 raid1_sync_request 是 resync 的 action

的确，最后是会走到这里，但是这里只是提交了 io 啊
```c
	/* For a user-requested sync, we read all readable devices and do a
	 * compare
	 */
	if (test_bit(MD_RECOVERY_REQUESTED, &mddev->recovery)) {
		atomic_set(&r1_bio->remaining, read_targets);
		for (i = 0; i < conf->raid_disks * 2 && read_targets; i++) {
			bio = r1_bio->bios[i];
			if (bio->bi_end_io == end_sync_read) {
				read_targets--;
				md_sync_acct_bio(bio, nr_sectors);
				if (read_targets == 1)
					bio->bi_opf &= ~MD_FAILFAST;
				submit_bio_noacct(bio);
			}
		}
	} else {
		atomic_set(&r1_bio->remaining, 1);
		bio = r1_bio->bios[r1_bio->read_disk];
		md_sync_acct_bio(bio, nr_sectors);
		if (read_targets == 1)
			bio->bi_opf &= ~MD_FAILFAST;
		submit_bio_noacct(bio);
	}
```

似乎 bio 完成之后，会调用 raid1_sync_request

- raid1_sync_request -> end_sync_read -> reschedule_retry : 如果是最后一个 bio，
那么 r1_bio 加入到 retry_list 中 ，并且启动 raid1d

raid1d -> sync_request_write -> process_checks -> memcmp


当让 raid 下面的盘的 io 暂停之后，观察 kthread 的行为:
```txt
🤒  rk -j raid1_sync_request+0x407/0xb60

vm dir : /home/martins3/hack/vm/oe2
/home/martins3/core/linux-build/scripts/faddr2line /home/martins3/core/linux-build/vmlinux raid1_sync_request+0x407/0xb60
raid1_sync_request+0x407/0xb60
raid1_sync_request+0x407/0xb60:
spin_lock_irq at include/linux/spinlock.h:376
(inlined by) raise_barrier at drivers/md/raid1.c:993
(inlined by) raid1_sync_request at drivers/md/raid1.c:2840
```
居然会卡到 raid1_sync_request 中的这个地方
```c
	if (raise_barrier(conf, sector_nr))
		return 0;
```

```c
	/*
	 * If there is non-resync activity waiting for a turn, then let it
	 * though before starting on this new sync request.
	 */
	if (atomic_read(&conf->nr_waiting[idx]))
		schedule_timeout_uninterruptible(1);
```

### raid1_sync_request 一次性需要接受多少个 sector

使用 ftrace 观察，在 raid1_sync_request 中存在特别多的 alloc_pages

```txt
perf ftrace -G raid1_sync_request -g 'smp_*' -g irq_enter_rcu -g __sysvec_irq_work -g irq_exit_rcu
```

看上去一次性需要提交 64M 的 page 下去，所以，这个 io 速度几乎就是物理盘的上限了
```c
	do {
		struct page *page;
		int len = PAGE_SIZE;
		if (sector_nr + (len>>9) > max_sector)
			len = (max_sector - sector_nr) << 9;
		if (len == 0)
			break;
		if (sync_blocks == 0) {
			if (!md_bitmap_start_sync(mddev->bitmap, sector_nr,
						  &sync_blocks, still_degraded) &&
			    !conf->fullsync &&
			    !test_bit(MD_RECOVERY_REQUESTED, &mddev->recovery))
				break;
			if ((len >> 9) > sync_blocks)
				len = sync_blocks<<9;
		}

		for (i = 0 ; i < conf->raid_disks * 2; i++) {
			struct resync_pages *rp;

			bio = r1_bio->bios[i];
			rp = get_resync_pages(bio);
			if (bio->bi_end_io) {
				page = resync_fetch_page(rp, page_idx);

				/*
				 * won't fail because the vec table is big
				 * enough to hold all these pages
				 */
				__bio_add_page(bio, page, len, 0);
			}
		}
		nr_sectors += len>>9;
		sector_nr += len>>9;
		sync_blocks -= (len>>9);
	} while (++page_idx < RESYNC_PAGES);
```

### process_checks 有啥意义 ?

echo repair > sync_action 和 echo check 对于 raid1 其体现只是在于此处:
```c
		if (j < 0 || (test_bit(MD_RECOVERY_CHECK, &mddev->recovery)
			      && !status)) {
			/* No need to write to this device. */
			sbio->bi_end_io = NULL;
			rdev_dec_pending(conf->mirrors[i].rdev, mddev);
			continue;
		}
```
1. IO 成功并且数据相等
2. IO 成功并且用户态只是要求进行 check

所以，check 的含义在于提早发现 fault device !

### 为什么需要使用这个脚本周期性的 sync 的

- 如果真的遇到问题的时候，也许 resync 是很快的

- 真的是互相阻塞了吗? 看上去是的，那么是所有的位置都是阻塞的，还是说仅仅是互相冲突的 bio 是互相冲突的:

```c
/*
 * perform a "sync" on one "block"
 *
 * We need to make sure that no normal I/O request - particularly write
 * requests - conflict with active sync requests.
 *
 * This is achieved by tracking pending requests and a 'barrier' concept
 * that can be installed to exclude normal IO requests.
 */

static sector_t raid1_sync_request(struct mddev *mddev, sector_t sector_nr,
				   int *skipped)
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
