## null blk
https://www.kernel.org/doc/html/latest/block/null_blk.html

使用 null blk 是理解 kernel 的绝佳的工具

虽然 null_complete_rq 直接注册到内核中，但是 nullb 在 queue 的过程中，就直接返回命令了，
连中断都省掉了。

- null_queue_rq
  - null_handle_cmd
    - nullb_complete_cmd
      - blk_mq_complete_request

```txt
@[
    null_complete_rq+5
    null_queue_rq+262
    __blk_mq_issue_directly+72
    blk_mq_try_issue_directly+137
    blk_mq_submit_bio+1456
    submit_bio_noacct_nocheck+653
    blkdev_direct_IO.part.0+574
    blkdev_read_iter+176
    __io_read+234
    io_read+21
    io_issue_sqe+96
    io_submit_sqes+507
    __do_sys_io_uring_enter+948
    do_syscall_64+67
    entry_SYSCALL_64_after_hwframe+111
]: 3527339
```

## null blk 有时候也是测试 CPU 和 内存的好工具

关闭 smt 之前:
```txt
🤒  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
[sudo] password for martins3:
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=64
fio-3.34
Starting 1 process
^Cbs: 1 (f=1): [r(1)][30.0%][r=6088MiB/s][r=1559k IOPS][eta 01m:10s]
fio: terminating on signal 2

trash: (groupid=0, jobs=1): err= 0: pid=42432: Fri Nov 24 17:36:09 2023
  read: IOPS=1529k, BW=5971MiB/s (6261MB/s)(179GiB/30765msec)
    slat (nsec): min=435, max=165096, avg=496.08, stdev=313.49
    clat (nsec): min=416, max=233980, avg=41245.17, stdev=4113.28
     lat (nsec): min=895, max=234459, avg=41741.25, stdev=4135.15
    clat percentiles (nsec):
     |  1.00th=[37632],  5.00th=[38144], 10.00th=[38144], 20.00th=[38656],
     | 30.00th=[39680], 40.00th=[40192], 50.00th=[40704], 60.00th=[41216],
     | 70.00th=[41728], 80.00th=[42240], 90.00th=[43264], 95.00th=[44800],
     | 99.00th=[64768], 99.50th=[67072], 99.90th=[70144], 99.95th=[72192],
     | 99.99th=[83456]
   bw (  MiB/s): min= 5710, max= 6272, per=100.00%, avg=5972.94, stdev=137.10, samples=61
   iops        : min=1461896, max=1605712, avg=1529072.62, stdev=35097.61, samples=61
  lat (nsec)   : 500=0.01%
  lat (usec)   : 2=0.01%, 4=0.01%, 10=0.01%, 20=0.01%, 50=96.94%
  lat (usec)   : 100=3.06%, 250=0.01%
  cpu          : usr=33.80%, sys=66.20%, ctx=167, majf=0, minf=73
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=0.1%, >=64=100.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.1%, >=64=0.0%
     issued rwts: total=47027928,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=64

Run status group 0 (all jobs):
   READ: bw=5971MiB/s (6261MB/s), 5971MiB/s-5971MiB/s (6261MB/s-6261MB/s), io=179GiB (193GB), run=30765-30765msec

Disk stats (read/write):
  nullb0: ios=46842928/0, merge=0/0, ticks=3465/0, in_queue=3465, util=99.70%
```

```txt
[sudo] password for martins3:
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=64
fio-3.34
Starting 1 process
^Cbs: 1 (f=1): [r(1)][34.3%][r=6476MiB/s][r=1658k IOPS][eta 01m:05s]
fio: terminating on signal 2

trash: (groupid=0, jobs=1): err= 0: pid=5301: Fri Nov 24 17:40:43 2023
  read: IOPS=1662k, BW=6492MiB/s (6807MB/s)(217GiB/34299msec)
    slat (nsec): min=433, max=120907, avg=463.21, stdev=102.99
    clat (nsec): min=974, max=158718, avg=37937.87, stdev=1475.67
     lat (nsec): min=1450, max=175885, avg=38401.08, stdev=1485.41
    clat percentiles (nsec):
     |  1.00th=[37120],  5.00th=[37120], 10.00th=[37120], 20.00th=[37632],
     | 30.00th=[37632], 40.00th=[37632], 50.00th=[37632], 60.00th=[37632],
     | 70.00th=[37632], 80.00th=[38144], 90.00th=[38656], 95.00th=[39680],
     | 99.00th=[41728], 99.50th=[43776], 99.90th=[62208], 99.95th=[63232],
     | 99.99th=[66048]
   bw (  MiB/s): min= 6329, max= 6611, per=100.00%, avg=6497.55, stdev=55.79, samples=68
   iops        : min=1620268, max=1692590, avg=1663372.15, stdev=14281.67, samples=68
  lat (nsec)   : 1000=0.01%
  lat (usec)   : 2=0.01%, 4=0.01%, 10=0.01%, 20=0.01%, 50=99.69%
  lat (usec)   : 100=0.31%, 250=0.01%
  cpu          : usr=31.20%, sys=68.79%, ctx=320, majf=0, minf=73
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=0.1%, >=64=100.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.1%, >=64=0.0%
     issued rwts: total=57002739,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=64

Run status group 0 (all jobs):
   READ: bw=6492MiB/s (6807MB/s), 6492MiB/s-6492MiB/s (6807MB/s-6807MB/s), io=217GiB (233GB), run=34299-34299msec

Disk stats (read/write):
  nullb0: ios=56958136/0, merge=0/0, ticks=4153/0, in_queue=4153, util=100.00%
```

## 调查一个有趣的问题，为什么拯救者的物理机的性能如此之差
物理机:
```txt
-   61.61%     0.00%  fio      [unknown]          [k] 0x000000000232a150
   - 0x232a150
      - 49.50% fio_ioring_commit
         - 49.18% entry_SYSCALL_64_after_hwframe
            - do_syscall_64
               - 49.08% __do_sys_io_uring_enter
                  - 44.23% io_submit_sqes
                     - 44.07% io_issue_sqe
                        - 38.64% io_read
                           - __io_read
                              - 37.82% blkdev_read_iter
                                 - 37.62% blkdev_direct_IO.part.0
                                    - 35.99% submit_bio_noacct_nocheck
                                       - 34.98% blk_mq_submit_bio
                                          - 23.45% blk_mq_try_issue_directly
                                             - __blk_mq_issue_directly
                                                - null_queue_rq
                                                   - 11.21% blk_mq_end_request
                                                      - blk_update_request
                                                         - 8.09% blkdev_bio_end_io_async
                                                              7.49% __io_req_task_work_add.part.0
                                                           2.87% bio_check_pages_dirty
                                                   - 11.17% blk_mq_start_request
                                                      - 11.16% ktime_get
                                                           read_hpet
                                                   - 0.75% __blk_mq_end_request
                                                      - 0.74% ktime_get
                                                           read_hpet
                                          - 11.50% __blk_mq_alloc_requests
                                             - 10.41% ktime_get
                                                  read_hpet
                                             - 0.75% blk_mq_get_tag
                                                - 0.73% sbitmap_get
                                                     sbitmap_find_bit
                                       - 1.00% ktime_get
                                            read_hpet
                                    - 0.96% bio_iov_iter_get_pages
                                       - iov_iter_extract_pages
                                          - 0.75% pin_user_pages_fast
                                               0.72% internal_get_user_pages_fast
                  - 4.23% __io_run_local_work
                     - 3.90% io_req_rw_complete
                          3.89% __fsnotify_parent
        10.30% __vdso_clock_gettime
      - 1.77% clock_gettime@@GLIBC_2.17
         - __vdso_clock_gettime
            - 1.00% entry_SYSCALL_64_after_hwframe
               - do_syscall_64
                  - 0.98% __x64_sys_clock_gettime
                     - 0.87% posix_get_monotonic_timespec
                        - ktime_get_ts64
                             read_hpet
```


```txt
   62.77%     0.07%  fio      [kernel.kallsyms]  [k] entry_SYSCALL_64_after_hwframe
     62.71% entry_SYSCALL_64_after_hwframe                                                                                                                                   ▒
      - do_syscall_64                                                                                                                                                        ▒
         - 42.67% __do_sys_io_uring_enter                                                                                                                                    ▒
            - 36.33% io_submit_sqes                                                                                                                                          ▒
               - 34.55% io_issue_sqe                                                                                                                                         ▒
                  - 33.82% io_read                                                                                                                                           ▒
                     - 33.20% blkdev_read_iter                                                                                                                               ▒
                        - 32.62% blkdev_direct_IO.part.0                                                                                                                     ▒
                           - 21.06% submit_bio_noacct_nocheck                                                                                                                ▒
                              - 19.53% blk_mq_submit_bio                                                                                                                     ▒
                                 - 12.90% blk_mq_try_issue_directly                                                                                                          ▒
                                    - 12.85% __blk_mq_issue_directly                                                                                                         ▒
                                       - 9.87% null_handle_cmd                                                                                                               ▒
                                          - 8.57% blk_mq_end_request                                                                                                         ▒
                                             - 3.62% blk_update_request                                                                                                      ▒
                                                - 1.50% bio_check_pages_dirty                                                                                                ▒
                                                     0.51% __bio_release_pages                                                                                               ▒
                                                - 0.76% blkdev_bio_end_io_async                                                                                              ▒
                                                     0.65% __io_req_task_work_add.part.0                                                                                     ▒
                                                  0.61% bio_put                                                                                                              ▒
                                               3.44% blk_account_io_done                                                                                                     ▒
                                             - 1.33% ktime_get                                                                                                               ▒
                                                - kvm_clock_get_cycles                                                                                                       ▒
                                                     pvclock_clocksource_read_nowd                                                                                           ▒
                                          - 0.62% __blk_mq_free_request                                                                                                      ▒
                                               0.58% sbitmap_queue_clear                                                                                                     ▒
                                            0.58% blk_mq_free_request                                                                                                        ▒
                                       - 2.66% null_queue_rq                                                                                                                 ▒
                                          - 1.59% blk_mq_start_request                                                                                                       ▒
                                             - 0.81% ktime_get                                                                                                               ▒
                                                - kvm_clock_get_cycles                                                                                                       ▒
                                                     pvclock_clocksource_read_nowd                                                                                           ▒
                                 - 3.23% __blk_mq_alloc_requests                                                                                                             ▒
                                    - 1.61% ktime_get                                                                                                                        ▒
                                       - kvm_clock_get_cycles                                                                                                                ▒
                                            pvclock_clocksource_read_nowd                                                                                                    ▒
                                      0.64% blk_mq_get_tag                                                                                                                   ▒
                                   0.76% blkcg_set_ioprio                                                                                                                    ▒
                                   0.64% __rcu_read_lock                                                                                                                     ▒
                                0.63% ktime_get                                                                                                                              ▒
                           - 7.32% bio_iov_iter_get_pages                                                                                                                    ▒
                              - 6.34% iov_iter_extract_pages                                                                                                                 ▒
                                 - 5.67% pin_user_pages_fast                                                                                                                 ▒
                                    - 5.52% internal_get_user_pages_fast                                                                                                     ▒
                                         2.95% try_grab_folio                                                                                                                ▒
                           - 2.06% bio_set_pages_dirty                                                                                                                       ▒
                                1.28% folio_unlock                                                                                                                           ▒
                                0.54% folio_mark_dirty                                                                                                                       ▒
                           - 1.24% bio_alloc_bioset                                                                                                                          ▒
                              - 1.08% bio_associate_blkg                                                                                                                     ▒
                                   0.97% bio_associate_blkg_from_css                                                                                                         ▒
                 1.14% io_prep_rw                                                                                                                                            ▒
              2.30% mutex_lock                                                                                                                                               ▒
            - 2.18% __io_run_local_work                                                                                                                                      ▒
               - 0.73% io_req_rw_complete                                                                                                                                    ▒
                    0.58% __fsnotify_parent                                                                                                                                  ▒
              1.49% mutex_unlock
         - 19.02% __x64_sys_clock_gettime                                                                                                                                         ▒
            - 15.79% put_timespec64                                                                                                                                               ▒
                 15.60% _copy_to_user                                                                                                                                             ▒
            - 3.21% posix_get_monotonic_timespec                                                                                                                                  ▒
               - 1.72% ktime_get_ts64                                                                                                                                             ▒
                  - 1.42% kvm_clock_get_cycles                                                                                                                                    ▒
                       pvclock_clocksource_read_nowd                                                                                                                              ▒
         - 0.98% syscall_exit_to_user_mode                                                                                                                                        ▒
              0.51% exit_to_user_mode_prepare
```

使用 sysbench 测试，cpu 性能没有太大的差别。

比较搞笑的是，使用了 dd 之后，两者性能差别很小
```txt
sudo dd if=/dev/null of=/dev/null0 oflag=direct count=10000000 bs=4k
```

在物理机上测试:
```txt
🧀  sudo dd if=/dev/nvme0n1 of=/dev/zero  count=10000000 bs=4k

[sudo] password for martins3:
Sorry, try again.
[sudo] password for martins3:

^C5297630+0 records in
5297630+0 records out
21699092480 bytes (22 GB, 20 GiB) copied, 9.06811 s, 2.4 GB/s
```

所以，这个应该是 fio 的 bug 才对，就像是 microsoft-edge 在 amd 上启动就会挂掉一样。

尝试自己构建一下 fio 吧!

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
