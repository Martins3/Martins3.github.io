从 gregg 的 perf-scripts 说起，
然后分析一下 trace-cmd 和 kernel-shark 的


## perf-scripts 的使用记录


### kprobe 输出效果如下

```sh
sudo /home/martins3/core/perf-tools/kernel/kprobe p:do_sys_openat2
```

```txt
   systemd-oomd-1421    [006] ..... 261304.030641: do_sys_openat2: (do_sys_openat2+0x0/0x170)
           <...>-4452    [019] ..... 261304.242460: do_sys_openat2: (do_sys_openat2+0x0/0x170)
           <...>-4452    [019] ..... 261304.242508: do_sys_openat2: (do_sys_openat2+0x0/0x170)
  grafana-server-4452    [010] ..... 261304.243535: do_sys_openat2: (do_sys_openat2+0x0/0x170)
  grafana-server-4452    [010] ..... 261304.244691: do_sys_openat2: (do_sys_openat2+0x0/0x170)
```

### iosnoop

利用如下三个 tracepoint 来分析

```txt
events/block/block_rq_issue : 在 blk_mq_start_request 中调用，当设备驱动和硬件沟通之后记录
events/block/block_rq_insert
events/block/block_rq_complete
```

分别对应的 backtrace 为:
```txt
@[
    blk_mq_start_request+169
    nvme_prep_rq.part.0+934
    nvme_queue_rq+123
    blk_mq_dispatch_rq_list+755
    __blk_mq_sched_dispatch_requests+516
    blk_mq_sched_dispatch_requests+52
    blk_mq_run_work_fn+100
    process_one_work+453
    worker_thread+81
    kthread+229
    ret_from_fork+41
]
```

```txt
@[
    blk_mq_flush_plug_list+707
    __blk_flush_plug+262
    io_schedule+65
    rq_qos_wait+192
    wbt_wait+166
    __rq_qos_throttle+36
    blk_mq_submit_bio+588
    submit_bio_noacct_nocheck+653
    ext4_bio_write_folio+338
    mpage_submit_folio+100
    mpage_process_page_bufs+299
    mpage_prepare_extent_to_map+939
    ext4_do_writepages+603
    ext4_writepages+173
    do_writepages+207
    filemap_fdatawrite_wbc+99
    __filemap_fdatawrite_range+92
    file_write_and_wait_range+74
    ext4_sync_file+265
    __x64_sys_fsync+59
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]
```
从 blk_mq_insert_request 的实现，此时是放到 software context 中的队列中

```txt
@[
    blk_mq_end_request_batch+768
    nvme_irq+114
    __handle_irq_event_percpu+74
    handle_irq_event+62
    handle_edge_irq+157
    __common_interrupt+67
    common_interrupt+129
    asm_common_interrupt+38
    cpuidle_enter_state+204
    cpuidle_enter+45
    do_idle+472
    cpu_startup_entry+29
    start_secondary+277
    __pfx_verify_cpu+0
]
```
