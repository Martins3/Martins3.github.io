# loop device

测试:
```sh
sudo mount -o loop test.iso /mnt
```

```sh
dd if=/dev/null of=x.ext4 bs=1000M seek=100
mkfs.ext4 -F x.ext4
mkdir -p dir
sudo mount -t ext4 -o loop x.ext4 dir
sudo chown martins3 dir
```

sysfs 中的东西:
```txt
🧀  pwd
/sys/block/loop0/loop

block/loop0/loop🔒 🐶
 grep . *
autoclear:1
backing_file:/home/martins3/x.ext4
dio:0
offset:0
partscan:0
sizelimit:0
```

0xffffffff83003bdc in loop_init () at drivers/block/loop.c:2261

- loop_add

```c
static const struct blk_mq_ops loop_mq_ops = {
	.queue_rq       = loop_queue_rq,
	.complete	= lo_complete_rq,
};
```

这个在 mount 过程中，总是出现的:
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_mount
        - __se_sys_mount
          - __do_sys_mount
            - do_mount
              - path_mount
                - do_new_mount
                  - vfs_get_tree
                    - get_tree_bdev
                      - ext4_fill_super
                        - __ext4_fill_super
                          - ext4_load_super
                            - ext4_sb_bread_unmovable
                              - __ext4_sb_bread_gfp
                                - ext4_read_bh
                                  - __ext4_read_bh
                                    - submit_bio_noacct_nocheck
                                      - submit_bio_noacct_nocheck
                                        - __submit_bio_noacct_mq
                                          - __submit_bio
                                            - blk_mq_submit_bio
                                              - blk_mq_try_issue_directly
                                                - __blk_mq_try_issue_directly
                                                  - __blk_mq_issue_directly
                                                    - loop_queue_rq

## 运行的路线基本

```txt
@[
        loop_queue_rq+0
        blk_mq_request_issue_directly+92
        blk_mq_issue_direct+140
        blk_mq_dispatch_queue_requests+296
        blk_mq_flush_plug_list+152
        __blk_flush_plug+248
        blk_finish_plug+64
        __iomap_dio_rw+552
        iomap_dio_rw+24
        ext4_file_write_iter+804
        vfs_write+548
        ksys_write+120
        __arm64_sys_write+36
        invoke_syscall.constprop.0+88
        do_el0_svc+72
        el0_svc+92
        el0t_64_sync_handler+268
        el0t_64_sync+408
]: 45241
```

- ret_from_fork
  - kthread
    - worker_thread
      - process_one_work
        - loop_process_work
          - loop_handle_cmd
            - do_req_filebacked
              - lo_write_simple
                - lo_write_bvec

## loop 可以让 md 是基于 file 的，真神奇啊

- https://unix.stackexchange.com/questions/302766/persistent-use-of-loop-block-device-in-mdadm
- https://stackoverflow.com/questions/4519761/programming-a-loopback-device-consisting-of-several-files-in-linux

## 简单来说，loop 就是让 mount 一个文件作为盘的，就像是 swap file 一样

- loop_queue_rq
  - loop_queue_work
    - loop_process_work
      - 最后一路达到 lo_write_bvec

## 原来容量是可以设置的
https://serverfault.com/questions/690192/why-is-my-loop-device-size-0

losetup --set-capacity /dev/loop0
blockdev --getsize64 /dev/loop0

可以主动将设备的容量设置为 0 吗?

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
