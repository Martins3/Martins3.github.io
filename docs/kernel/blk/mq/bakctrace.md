- ret_from_fork
  - kthread
    - worker_thread
      - process_one_work
        - blk_mq_requeue_work
          - blk_mq_run_hw_queues
            - blk_mq_run_hw_queue
              - blk_mq_sched_dispatch_requests
                - __blk_mq_sched_dispatch_requests
                  - blk_mq_dispatch_rq_list
                    - virtio_queue_rq

- vfs_fsync
  - ext4_sync_file
    - file_write_and_wait_range
      - __filemap_fdatawrite_range
        - filemap_fdatawrite_wbc
          - filemap_fdatawrite_wbc
            - do_writepages
              - ext4_writepages
                - ext4_do_writepages
                  - blk_finish_plug
                    - blk_finish_plug
                      - __blk_flush_plug
                        - blk_mq_flush_plug_list
                          - __blk_mq_flush_plug_list
                            - __blk_mq_flush_plug_list
                              - virtio_queue_rqs
## flush

blk_mq_kick_requeue_list 总是如下的路径:

- journal_submit_commit_record
  - submit_bh
    - submit_bh_wbc
      - submit_bio_noacct
        - submit_bio_noacct_nocheck
          - __submit_bio_noacct_mq
            - blk_mq_submit_bio
              - blk_insert_flush
                - blk_kick_flush
                  - blk_mq_kick_requeue_list

## freeze
例如，如果想要删除一个盘，最后会因为 ref count 不为 0
```txt
echo 1 > /sys/block/sda/device/delete
```

```txt
➜  nvme0n1 cat /proc/4837/stack
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

- ret_from_fork
  - kernel_init
    - kernel_init_freeable
      - do_basic_setup
        - do_initcalls
          - do_initcall_level
            - do_one_initcall
              - loop_init
                - loop_add
                  - __blk_mq_alloc_disk
                    - __alloc_disk_node
                      - blkcg_init_disk
                        - blk_ioprio_init
                          - blkcg_activate_policy
                            - blk_mq_freeze_queue

## init
- nvme_alloc_admin_tag_set
  - blk_mq_init_queue
    - blk_mq_init_queue_data
      - blk_mq_init_allocated_queue 来分配的
        - INIT_WORK(&q->timeout_work, blk_mq_timeout_work);
        - INIT_DELAYED_WORK(&q->requeue_work, blk_mq_requeue_work);

## blk_mq_dispatch_rq_list
<!-- cb19b619-a48a-4506-9727-896fc61bd4ca -->

和 __blk_mq_issue_directly 是唯二调用 blk_mq_ops::queue_rq 的地方

blk_mq_dispatch_rq_list 是来自于 scheduler 的
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

## 有趣的路径，当 echo check > /sys/block/md2/md/sync_action 的时候产生的

```txt
  virtio_queue_rq
  blk_mq_request_issue_directly
  blk_mq_try_issue_list_directly
  blk_mq_flush_plug_list
  blk_add_rq_to_plug
  blk_mq_submit_bio
  submit_bio_noacct_nocheck
  raid1_sync_request
  md_do_sync
  md_thread
  kthread
  ret_from_fork
  ret_from_fork_asm
    406

  virtio_queue_rq
  blk_mq_dispatch_rq_list
  __blk_mq_sched_dispatch_requests
  blk_mq_sched_dispatch_requests
  blk_mq_run_work_fn
  process_scheduled_works
  worker_thread
  kthread
  ret_from_fork
  ret_from_fork_asm
    109222
```

## 获取 tags

- block_read_full_folio
  - submit_bh
    - submit_bio_noacct_nocheck
      - __submit_bio_noacct_mq
        - __submit_bio
          - blk_mq_submit_bio
            - blk_mq_get_new_requests
              - __blk_mq_alloc_requests
                - blk_mq_get_tag
                  - __blk_mq_get_tag
                    - __sbitmap_queue_get
                      - sbitmap_get
                        - __sbitmap_get
                          - sbitmap_find_bit
                            - sbitmap_find_bit_in_word
                              - __sbitmap_get_word

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
