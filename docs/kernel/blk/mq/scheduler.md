# scheduler
原来切换 scheduler 的时候，需要配置
这些东西，实在是难顶啊:


```txt
sudo cat /proc/43507/stack
[<0>] blk_mq_freeze_queue_wait+0x61/0x90
[<0>] elevator_switch+0x16/0x170
[<0>] elv_iosched_store+0x27/0x40
[<0>] queue_attr_store+0x3f/0x70
[<0>] kernfs_fop_write_iter+0x125/0x1c0
[<0>] new_sync_write+0x110/0x1b0
[<0>] vfs_write+0x1b7/0x250
[<0>] ksys_write+0x5f/0xe0
[<0>] do_syscall_64+0x3d/0x80
[<0>] entry_SYSCALL_64_after_hwframe+0x67/0xcc
```

## 什么情况下， block_rq_insert 但是无法 block_rq_issue

## scheduler 到底影响了哪些 sysfs 接口
1. /sys/block/*/queue/nr_requests

## 两个流程的对比

```text
A. 不带 scheduler
=================

bio
 |
 v
blk_mq_submit_bio()
 |
 v
分配 / 获取 request
 |
 v
当前 CPU 的 blk_mq_ctx
(ctx->rq_lists[] 可能短暂停留，但不经过电梯调度器)
 |
 | 通过 ctx->hctxs[type]
 v
blk_mq_hw_ctx
 |
 | 直接分配 driver tag
 | hctx->tags -> blk_mq_tags -> rqs[tag] = rq
 v
blk_mq_ops.queue_rq(hctx, bd)
 |
 v
驱动 / 硬件
 |
 v
complete
 |
 v
通过 tag 找回 rq，完成 I/O
```

```text
B. 带 scheduler
===============

bio
 |
 v
blk_mq_submit_bio()
 |
 v
分配 / 获取 request
 |
 | 先分配 scheduler tag
 | hctx->sched_tags / q->sched_shared_tags
 v
当前 CPU 的 blk_mq_ctx
 |
 v
I/O scheduler
(elevator / mq-deadline / kyber / bfq 等)
 |
 | request 先进入调度器内部队列
 | 合并、排序、限流、选择派发时机
 v
blk_mq_hw_ctx
 |
 | 真正下发前，再分配 driver tag
 | hctx->tags -> blk_mq_tags -> rqs[tag] = rq
 v
blk_mq_ops.queue_rq(hctx, bd)
 |
 v
驱动 / 硬件
 |
 v
complete
 |
 v
释放 driver tag
 |
 v
rq 从 scheduler 生命周期结束，完成 I/O
```

## 一个 hba 下，每一个盘的 scheduler 都可以是独立的

```txt
 grep . */queue/scheduler
nvme0n1/queue/scheduler:[none] mq-deadline kyber bfq
sda/queue/scheduler:[none] mq-deadline kyber bfq
sdb/queue/scheduler:none mq-deadline kyber [bfq]
sdc/queue/scheduler:none mq-deadline kyber [bfq]
sdd/queue/scheduler:none mq-deadline kyber [bfq]
sde/queue/scheduler:none mq-deadline kyber [bfq]
sdf/queue/scheduler:none mq-deadline kyber [bfq]
/sys/block🔒 🦇
🧀  lsscsi
[2:0:0:0]    disk    QEMU     QEMU HARDDISK    2.5+  /dev/sdf
[2:0:0:10]   disk    QEMU     QEMU HARDDISK    2.5+  /dev/sda
[3:0:0:0]    disk    QEMU     QEMU HARDDISK    2.5+  /dev/sdb
[4:0:0:0]    disk    Linux    scsi_debug       0191  /dev/sdc
[5:0:0:0]    disk    Linux    scsi_debug       0191  /dev/sdd
[5:0:0:1]    disk    Linux    scsi_debug       0191  /dev/sde
[N:0:0:1]    disk    ZHITAI TiPro7000 1TB__1                    /dev/nvme0n1
```
这是非常合理的，因为 scheduler 是在上层的。

(那么 software queue 大致哪一个层次，有没有一个 tracepoint 可以展示当前
什么时候进入到 software queue ，什么时候离开 software queue 的)


## scheduler 重新引入了一个 tag 的
```txt
rq 生命周期:
[ 创建 ]----[ 在 scheduler 中等待 ]----[ 已 dispatch 到驱动 ]----[ 完成 ]

sched_tag:
           acquire ================================ release
           |<------ 从进调度器到请求结束 ------->|

driver tag:
                                           acquire ======= release
                                           |< 真正在硬件飞行 >|
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
