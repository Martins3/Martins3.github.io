## iostat 的每一项的含义是什么
<!-- 74d47ce1-5163-4a7a-97d9-8898d487a08b -->

```txt
Linux 6.17.7-00001-gfd23f075a322 (localhost.localdomain)        11/21/2025      _x86_64_        (32 CPU)

avg-cpu:  %user   %nice %system %iowait  %steal   %idle
           0.23    0.00    0.16    0.01    0.00   99.60

Device            r/s     rMB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wMB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dMB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
dm-0            36.59      0.19     0.00   0.00    0.13     5.19    6.54      0.23     0.00   0.00    4.29    36.36    0.06      4.18     0.00   0.00  109.43 66869.63    0.00    0.00    0.04   0.65
nvme0n1       1634.97      6.43     0.03   0.00    0.19     4.03    6.29      0.23     0.41   6.18    4.50    37.82    0.00      4.18     0.06  96.29  117.12 1803659.39    0.48    1.37    0.34   0.81
nvme2n1        222.23      0.87     0.00   0.00    0.72     4.00    0.00      0.00     0.00   0.00    0.00     0.00    0.00      0.00     0.00   0.00    0.00     0.00    0.00    0.00    0.16   0.13
sda              0.18      0.00     0.00   0.12   73.25     4.02    0.00      0.00     0.00  36.56   30.17     5.83    0.02      2.34     0.00   0.00    1.88 128910.82    0.00   30.84    0.01   0.04
sdb              1.96      0.01     0.00   0.00    0.68     4.00    0.00      0.00     0.00   0.00    0.00     0.00    0.00      0.00     0.00   0.00    0.00     0.00    0.00    0.00    0.00   0.00
zram0            0.00      0.00     0.00   0.00    0.00    21.02    0.00      0.00     0.00   0.00    0.00     4.00    0.00      0.00     0.00   0.00    0.00     0.00    0.00    0.00    0.00   0.00
```

- w_await r_await 应该都是 C2D 的延迟，单位是 ms ，和 biolatency 中观测的东西基本一致
- rareq-sz 可能平均 inflight 的 io
  - 是用 fio 测试 HDD 盘，可以发现 iodepth 是 12 ，那么 rareq-sz 就是 12


## 这都写的什么东西啊，都整理一下吧
在 2112 上测试，无论是读写都无法复现这个场景:
```txt
-   93.60%     0.00%  fio      [kernel.kallsyms]   [k] entry_SYSCALL_64_after_hwframe
     entry_SYSCALL_64_after_hwframe
   - do_syscall_64
      - 87.57% __x64_sys_io_submit
         - 80.22% blk_finish_plug
            - blk_flush_plug_list
               - blk_mq_flush_plug_list
                  - 79.90% blk_mq_sched_insert_requests
                     - blk_mq_try_issue_list_directly
                        - 79.67% blk_mq_request_issue_directly
                           - __blk_mq_try_issue_directly
                              - 79.38% nvme_queue_rq
                                 - 78.07% nvme_submit_cmd
                                    - 0.62% apic_timer_interrupt
                                         smp_apic_timer_interrupt
                                         irq_exit
                                         __softirqentry_text_start
         - 6.64% io_submit_one
            - 5.40% aio_write
               - 5.16% blkdev_write_iter
                  - 5.07% __generic_file_write_iter
                     - 4.93% generic_file_direct_write
                        - 4.81% blkdev_direct_IO
                           - 3.43% submit_bio
                              - generic_make_request
                                 - 2.98% blk_mq_make_request
                                    - 1.27% blk_mq_get_request
                                       - 0.90% blk_mq_get_tag
                                            __sbitmap_queue_get
                                      0.63% blk_mq_bio_to_request
                           - 0.63% bio_iov_iter_get_pages
                              - 0.59% iov_iter_get_pages
                                   get_user_pages_fast
```

2112 内核上，iops 打到 80k iops 的，仅仅可以观察的数量，可见很多时候
```txt
[root@localhost 11:02:35 tools]$ ./funccount blk_mq_queue_tag_busy_iter
Tracing 1 functions for "b'blk_mq_queue_tag_busy_iter'"... Hit Ctrl-C to end.
^C
FUNC                                    COUNT
b'blk_mq_queue_tag_busy_iter'             569
Detaching...
```

当时的 backtrace 是:
```txt
  b'blk_mq_queue_tag_busy_iter'
  b'blk_mq_in_flight'
  b'part_round_stats'
  b'blk_account_io_start'
  b'blk_mq_make_request'
  b'generic_make_request'
  b'submit_bio'
  b'blkdev_direct_IO'
  b'generic_file_direct_write'
  b'__generic_file_write_iter'
  b'blkdev_write_iter'
  b'aio_write'
  b'io_submit_one'
  b'__x64_sys_io_submit'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
    473
```

如果使用 6.12.1 中来测试，发现:
```txt
@[
    blk_mq_queue_tag_busy_iter+5
    blk_mq_timeout_work+128
    process_one_work+349
    worker_thread+677
    kthread+220
    ret_from_fork+49
    ret_from_fork_asm+26
]: 2
```

使用 2307 测试，不会出现在 fio 提交的时候触发，而是在系统的时候触发:
```txt
  b'blk_mq_queue_tag_busy_iter'
  b'blk_mq_in_flight'
  b'part_stat_show'
  b'dev_attr_show'
  b'sysfs_kf_seq_show'
  b'seq_read'
  b'__vfs_read'
  b'vfs_read'
  b'ksys_read'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
  b'[unknown]'
  b'[unknown]'
    27
```
这个也是 fio 提供的

2112 上 part_round_stats 上的调用次数极多:
```txt
[root@localhost 11:50:29 tools]$ ./funccount part_round_stats
Tracing 1 functions for "b'part_round_stats'"... Hit Ctrl-C to end.
^C
FUNC                                    COUNT
b'part_round_stats'                    288034
Detaching...
```

```txt
[root@localhost 12:24:21 tools]$ ./funccount part_in_flight
Tracing 1 functions for "b'part_in_flight'"... Hit Ctrl-C to end.
^C
FUNC                                    COUNT
b'part_in_flight'                        1703
Detaching...
```

## 的确有这个问题，但是耗时没有那么长

```txt
[root@localhost 12:30:18 tools]$ ./funclatency part_in_flight
Tracing 1 functions for "part_in_flight"... Hit Ctrl-C to end.
^C
     nsecs               : count     distribution
         0 -> 1          : 0        |                                        |
         2 -> 3          : 0        |                                        |
         4 -> 7          : 0        |                                        |
         8 -> 15         : 0        |                                        |
        16 -> 31         : 0        |                                        |
        32 -> 63         : 0        |                                        |
        64 -> 127        : 0        |                                        |
       128 -> 255        : 0        |                                        |
       256 -> 511        : 0        |                                        |
       512 -> 1023       : 0        |                                        |
      1024 -> 2047       : 10999    |****************************************|
      2048 -> 4095       : 2258     |********                                |
      4096 -> 8191       : 419      |*                                       |
      8192 -> 16383      : 9        |                                        |
Detaching...
```

这个函数调用次数还好:
```txt
root@localhost 12:30:49 tools]$ ./funccount -i 1 blk_mq_check_inflight
Tracing 1 functions for "b'blk_mq_check_inflight'"... Hit Ctrl-C to end.

FUNC                                    COUNT
b'blk_mq_check_inflight'                 1985

FUNC                                    COUNT
b'blk_mq_check_inflight'                 2224

FUNC                                    COUNT
b'blk_mq_check_inflight'                 2001
^C
FUNC                                    COUNT
b'blk_mq_check_inflight'                 1258
Detaching...
```

## 看看 biolatency
是不是系统太多了，所以导致系统的服务比较多
虚拟机测试的
```txt
[root@localhost 12:43:34 tools]$ ./biolatency
Tracing block device I/O... Hit Ctrl-C to end.
^C
     usecs               : count     distribution
         0 -> 1          : 0        |                                        |
         2 -> 3          : 0        |                                        |
         4 -> 7          : 0        |                                        |
         8 -> 15         : 0        |                                        |
        16 -> 31         : 760961   |**                                      |
        32 -> 63         : 13247762 |****************************************|
        64 -> 127        : 529502   |*                                       |
       128 -> 255        : 44605    |                                        |
       256 -> 511        : 19482    |                                        |
       512 -> 1023       : 9042     |                                        |
      1024 -> 2047       : 5775     |                                        |
      2048 -> 4095       : 1112     |                                        |
      4096 -> 8191       : 78       |                                        |
      8192 -> 16383      : 6        |                                        |
     16384 -> 32767      : 0        |                                        |
     32768 -> 65535      : 0        |                                        |
     65536 -> 131071     : 0        |                                        |
    131072 -> 262143     : 0        |                                        |
    262144 -> 524287     : 2        |                                        |
```

## 直接看看 patch

### 问题 1
```diff
commit 5b18b5a737600fd20ba2045f320d5926ebbf341a
Author: Mikulas Patocka <om>cka <mpatocka@redhat.com>

Date:   Thu Dec 6 11:41:19 2018 -0500

    block: delete part_round_stats and switch to less precise counting

    We want to convert to per-cpu in_flight counters.

    The function part_round_stats needs the in_flight counter every jiffy, it
    would be too costly to sum all the percpu variables every jiffy, so it
    must be deleted. part_round_stats is used to calculate two counters -
    time_in_queue and io_ticks.

    time_in_queue can be calculated without part_round_stats, by adding the
    duration of the I/O when the I/O ends (the value is almost as exact as the
    previously calculated value, except that time for in-progress I/Os is not
    counted).

    io_ticks can be approximated by increasing the value when I/O is started
    or ended and the jiffies value has changed. If the I/Os take less than a
    jiffy, the value is as exact as the previously calculated value. If the
    I/Os take more than a jiffy, io_ticks can drift behind the previously
    calculated value.

    Signed-off-by: Mikulas Patompatocka@redhat.c
```

### 修复 2 : 导致问题的 ticket

```diff
commit 74efba2393525a2baa6ffa92a45c916e29b6964b
Author: Yufen Yu <yuyufen@huawei.com>
Date:   Wed Mar 18 15:44:01 2020 +0800

    block: fix inaccurate io_ticks

    hulk inclusion
    category: bugfix
    bugzilla: 31388
    CVE: NA
    ---------------------------

    After introducing commit 5b18b5a73760 ("block: delete part_round_stats
    and switch to less precise counting"), '%util' accounted by iostat
    will be over reality data. In fact, the device is quite idle, but
    iostat may show '%util' as a big number (e.g. 50%). It can produce by fio:

    fio --name=1 --direct=1 --bs=4k --rw=read --filename=/dev/sda \
                       --thinktime=4ms --runtime=180

    We fix this by reserving part_round_stats() in io start path.

    fixes: 5b18b5a73760 ("block: delete part_round_stats and switch to less precise counting")
    Signed-off-by: Yufen Yu <yuyufen@huawei.com>
    Reviewed-by: Hou Tao <houtao1@huawei.com>
    Signed-off-by: Yang Yingliang <yangyingliang@huawei.com>

diff --git a/block/bio.c b/block/bio.c
index 48a8cf55fb7a..94d0f4798b5b 100644
--- a/block/bio.c
+++ b/block/bio.c
@@ -1675,7 +1675,7 @@ void update_io_ticks(int cpu, struct hd_struct *part, unsigned long now)
 	stamp = READ_ONCE(part->stamp);
 	if (unlikely(stamp != now)) {
 		if (likely(cmpxchg(&part->stamp, stamp, now) == stamp))
-			__part_stat_add(cpu, part, io_ticks, 1);
+			__part_stat_add(cpu, part, io_ticks, now - stamp);
 	}
 	if (part->partno) {
 		part = &part_to_disk(part)->part0;
@@ -1689,7 +1689,7 @@ void generic_start_io_acct(struct request_queue *q, int op,
 	const int sgrp = op_stat_group(op);
 	int cpu = part_stat_lock();

-	update_io_ticks(cpu, part, jiffies);
+	part_round_stats(q, cpu, part);
 	part_stat_inc(cpu, part, ios[sgrp]);
 	part_stat_add(cpu, part, sectors[sgrp], sectors);
 	part_inc_in_flight(q, part, op_is_write(op));
diff --git a/block/blk-core.c b/block/blk-core.c
index d9e3ee68f377..7afe44d09741 100644
--- a/block/blk-core.c
+++ b/block/blk-core.c
@@ -1674,11 +1674,8 @@ static void part_round_stats_single(struct request_queue *q, int cpu,
 				    struct hd_struct *part, unsigned long now,
 				    unsigned int inflight)
 {
-	if (inflight) {
-		__part_stat_add(cpu, part, time_in_queue,
-				inflight * (now - part->stamp));
+	if (inflight)
 		__part_stat_add(cpu, part, io_ticks, (now - part->stamp));
-	}
 	part->stamp = now;
 }

@@ -2791,12 +2788,11 @@ void blk_account_io_start(struct request *rq, bool new_io)
 		part_stat_inc(cpu, part, merges[rw]);
 	} else {
 		part = disk_map_sector_rcu(rq->rq_disk, blk_rq_pos(rq));
+		part_round_stats(rq->q, cpu, part);
 		part_inc_in_flight(rq->q, part, rw);
 		rq->part = part;
 	}

-	update_io_ticks(cpu, part, jiffies);
-
 	part_stat_unlock();
 }
```

### 再次修复
1. 09614b9693ac2104c3a

这里似乎有好几个 fix 的
https://gitee.com/src-openeuler/kernel/pulls/555

最重要的问题，precise iostat 的基础设施: : 2022 3
```txt
History:        #0
Commit:         69e55430e9eb2f6df76f2709a744b388d69d567b
Author:         Zhang Wensheng <zhangwensheng5@huawei.com>
Committer:      Yang Yingliang <yangyingliang@huawei.com>
Author Date:    Wed 09 Mar 2022 04:43:21 PM CST
Committer Date: Wed 09 Mar 2022 04:35:32 PM CST

block: add a switch for precise iostat accounting

hulk inclusion
category: bugfix
bugzilla: 39265, https://gitee.com/openeuler/kernel/issues/I4WC06
CVE: NA

-----------------------------------------------
```

```txt

 History:        #0
 Commit:         9dcfca65eb0f18796426b6f85bd9603da21427b2
 Author:         Zhang Wensheng <zhangwensheng5@huawei.com>
 Committer:      Yongqiang Liu <liuyongqiang13@huawei.com>
 Author Date:    Wed 06 Jul 2022 03:12:37 PM CST
 Committer Date: Wed 06 Jul 2022 03:17:26 PM CST

 block: use "precise_iostat" to switch accurate iostat account

 hulk inclusion
 category: bugfix
 bugzilla: 187044, https://gitee.com/openeuler/kernel/issues/I5F2BY
 CVE: NA
```
再次强化。

参数微调:
```txt
 History:        #0
 Commit:         09614b9693ac2104c3a1518d102aff935bb1c4bd
 Author:         Konstantin Khlebnikov <khlebnikov@yandex-team.ru>
 Committer:      Yongqiang Liu <iang13@huawei.com>
 Author Date:    Wed 06 Jul 2022 03:12:36 PM CST
 Committer Date: Wed 06 Jul 2022 03:17:26 PM CST

 block/diskstats: more accurate approximation of io_ticks for slow disks

 mainline inclusion
 from mainline-v5.7-rc1
 commit 2b8bd423614c595540eaadcfbc702afe8e155e50
 category: bugfix
 bugzilla: 187044, https://gitee.com/openeuler/kernel/issues/I5F2BY
 CVE: NA
liuyongq
```

## 主线真正的修复

```diff
History:        #0
Commit:         99dc422335d8b2bd4d105797241d3e715bae90e9
Author:         Yu Kuai <yukuai3@huawei.com>
Committer:      Jens Axboe <axboe@kernel.dk>
Author Date:    Thu 09 May 2024 08:37:16 PM CST
Committer Date: Thu 09 May 2024 09:59:44 PM CST

block: support to account io_ticks precisely

Currently, io_ticks is accounted based on sampling, specifically
update_io_ticks() will always account io_ticks by 1 jiffies from
bdev_start_io_acct()/blk_account_io_start(), and the result can be
inaccurate, for example(HZ is 250):

Test script:
fio -filename=/dev/sda -bs=4k -rw=write -direct=1 -name=test -thinktime=4ms

Test result: util is about 90%, while the disk is really idle.

This behaviour is introduced by commit 5b18b5a73760 ("block: delete
part_round_stats and switch to less precise counting"), however, there
was a key point that is missed that this patch also improve performance
a lot:

Before the commit:
part_round_stats:
  if (part->stamp != now)
   stats |= 1;

  part_in_flight()
  -> there can be lots of task here in 1 jiffies.
  part_round_stats_single()
   __part_stat_add()
  part->stamp = now;

After the commit:
update_io_ticks:
  stamp = part->bd_stamp;
  if (time_after(now, stamp))
   if (try_cmpxchg())
    __part_stat_add()
    -> only one task can reach here in 1 jiffies.

Hence in order to account io_ticks precisely, we only need to know if
there are IO inflight at most once in one jiffies. Noted that for
rq-based device, iterating tags should not be used here because
'tags->lock' is grabbed in blk_mq_find_and_get_req(), hence
part_stat_lock_inc/dec() and part_in_flight() is used to trace inflight.
The additional overhead is quite little:

 - per cpu add/dec for each IO for rq-based device;
 - per cpu sum for each jiffies;

And it's verified by null-blk that there are no performance degration
under heavy IO pressure.

Fixes: 5b18b5a73760 ("block: delete part_round_stats and switch to less precise counting")
Signed-off-by: Yu Kuai <yukuai3@huawei.com>
Link: https://lore.kernel.org/r/20240509123717.3223892-2-yukuai1@huaweicloud.com
Signed-off-by: Jens Axboe <axboe@kernel.dk>

diff --git a/block/blk-mq.c b/block/blk-mq.c
index 9f677ea85a52..8e01e4b32e10 100644
--- a/block/blk-mq.c
+++ b/block/blk-mq.c
@@ -996,6 +996,8 @@ static inline void blk_account_io_done(struct request *req, u64 now)
 		update_io_ticks(req->part, jiffies, true);
 		part_stat_inc(req->part, ios[sgrp]);
 		part_stat_add(req->part, nsecs[sgrp], now - req->start_time_ns);
+		part_stat_local_dec(req->part,
+				    in_flight[op_is_write(req_op(req))]);
 		part_stat_unlock();
 	}
 }
@@ -1018,6 +1020,8 @@ static inline void blk_account_io_start(struct request *req)

 		part_stat_lock();
 		update_io_ticks(req->part, jiffies, false);
+		part_stat_local_inc(req->part,
+				    in_flight[op_is_write(req_op(req))]);
 		part_stat_unlock();
 	}
 }
```

## 理解一下基本问题
1. 为什么 blk_account_io_done 结束的时候也需要调用 update_io_ticks ，没关系的
2. 为什么这样修改就好了

精确和不精确的差别是:
```c
/**
 * part_round_stats() - Round off the performance stats on a struct disk_stats.
 * @q: target block queue
 * @cpu: cpu number for stats access
 * @part: target partition
 *
 * The average IO queue length and utilisation statistics are maintained
 * by observing the current state of the queue length and the amount of
 * time it has been in this state for.
 *
 * Normally, that accounting is done on IO completion, but that can result
 * in more than a second's worth of IO being accounted for within any one
 * second, leading to >100% utilisation.  To deal with that, we call this
 * function to do a round-off before returning the results when reading
 * /proc/diskstats.  This accounts immediately for all queue usage up to
 * the current jiffies and restarts the counters again.
 */
void part_round_stats(struct request_queue *q, int cpu, struct hd_struct *part)
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
