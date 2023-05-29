##
blk_mq_freeze_queue

```txt
#0  blk_mq_freeze_queue (q=q@entry=0xffff888108fb8000) at block/blk-mq.c:177
#1  0xffffffff81775ad3 in blkcg_activate_policy (disk=disk@entry=0xffff888108fb1400, pol=pol@entry=0xffffffff8322cfc0 <ioprio_policy>) at block/blk-cgroup.c:1470
#2  0xffffffff8177c685 in blk_ioprio_init (disk=disk@entry=0xffff888108fb1400) at block/blk-ioprio.c:212
#3  0xffffffff817765c9 in blkcg_init_disk (disk=disk@entry=0xffff888108fb1400) at block/blk-cgroup.c:1387
#4  0xffffffff8176d676 in __alloc_disk_node (q=q@entry=0xffff888108fb8000, node_id=<optimized out>, lkclass=lkclass@entry=0xffffffff83f3c3f4 <part_shift>) at block/genhd.c:1400
#5  0xffffffff8176677c in __blk_mq_alloc_disk (set=set@entry=0xffff888105c7b108, queuedata=queuedata@entry=0xffff888105c7b000, lkclass=lkclass@entry=0xffffffff83f3c3f4 <part_shift>) at block/blk-mq.c:4092
#6  0xffffffff81b8ab9c in loop_add (i=i@entry=0) at drivers/block/loop.c:2007
#7  0xffffffff83a8c6cc in loop_init () at drivers/block/loop.c:2248
#8  0xffffffff81001a55 in do_one_initcall (fn=0xffffffff83a8c610 <loop_init>) at init/main.c:1246
#9  0xffffffff83a26700 in do_initcall_level (command_line=0xffff888103b45100 "root", level=6) at init/main.c:1319
#10 do_initcalls () at init/main.c:1335
#11 do_basic_setup () at init/main.c:1354
#12 kernel_init_freeable () at init/main.c:1571
#13 0xffffffff822b8d4a in kernel_init (unused=<optimized out>) at init/main.c:1462
#14 0xffffffff81002939 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

- blk_mq_freeze_queue : io 会堵在什么地方?

```txt
@[
    blk_mq_dispatch_rq_list+5
    __blk_mq_sched_dispatch_requests+301
    blk_mq_sched_dispatch_requests+57
    __blk_mq_run_hw_queue+115
    process_one_work+482
    worker_thread+84
    kthread+218
    ret_from_fork+41
]: 57
@[
    blk_mq_dispatch_rq_list+5
    __blk_mq_sched_dispatch_requests+171
    blk_mq_sched_dispatch_requests+57
    __blk_mq_run_hw_queue+115
    blk_mq_run_hw_queues+105
    blk_mq_requeue_work+340
    process_one_work+482
    worker_thread+84
    kthread+218
    ret_from_fork+41
]: 92
```


```txt
#0  nvme_alloc_admin_tag_set (ctrl=ctrl@entry=0xffff8881062d01f0, set=set@entry=0xffff8881062d00b0, ops=ops@entry=0xffffffff82717fe0 <nvme_mq_admin_ops>, cmd_size=cmd_size@entry=184) at drivers/nvme/host/core.c:4906
#1  0xffffffff81c1a752 in nvme_probe (pdev=0xffff888104089000, id=<optimized out>) at drivers/nvme/host/pci.c:3004
#2  0xffffffff81820ff4 in local_pci_probe (_ddi=_ddi@entry=0xffffc900011efdc0) at drivers/pci/pci-driver.c:324
#3  0xffffffff81822ac3 in pci_call_probe (id=<optimized out>, dev=0xffff888104089000, drv=<optimized out>) at drivers/pci/pci-driver.c:392
```

- nvme_alloc_admin_tag_set
  - blk_mq_init_queue
    - blk_mq_init_queue_data
      - blk_mq_init_allocated_queue 来分配的
        - INIT_WORK(&q->timeout_work, blk_mq_timeout_work);
        - INIT_DELAYED_WORK(&q->requeue_work, blk_mq_requeue_work);
