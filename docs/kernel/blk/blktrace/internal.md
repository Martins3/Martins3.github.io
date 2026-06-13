## 关键原料

CONFIG_BLK_DEV_IO_TRACE
kernel/trace/blktrace.c
git clone https://git.kernel.dk/blktrace.git

## 和 ftrace 的关系

感觉只是为了借助这个机器来记录而已:
```c
static struct tracer blk_tracer __read_mostly = {
	.name		= "blk",
	.init		= blk_tracer_init,
	.reset		= blk_tracer_reset,
	.start		= blk_tracer_start,
	.stop		= blk_tracer_stop,
	.print_header	= blk_tracer_print_header,
	.print_line	= blk_tracer_print_line,
	.flags		= &blk_tracer_flags,
	.set_flag	= blk_tracer_set_flag,
};
```

tracepoint 的 hook 基本通过这个两个函数来记录:
- blk_add_trace_rq
- blk_add_trace_bio


- submit_bio_noacct_nocheck
  - trace_block_bio_queue
    - blk_add_trace_bio_queue
      - blk_add_trace_bio
        - trace_note_tsk
          - trace_note

blk_tracer_enabled

## /sys/block/sda/trace/

```txt
 act_mask   enable   end_lba   pid   start_lba
```

```c
struct blk_trace {
	int trace_state;
	struct rchan *rchan;
	unsigned long __percpu *sequence;
	unsigned char __percpu *msg_data;
	u16 act_mask;
	u64 start_lba;
	u64 end_lba;
	u32 pid;
	u32 dev;
	struct dentry *dir;
	struct list_head running_list;
	atomic_t dropped;
};
```


其作用体现在 : __blk_add_trace 中来对于任务进行过滤
```c
static int act_log_check(struct blk_trace *bt, u32 what, sector_t sector,
			 pid_t pid)
{
	if (((bt->act_mask << BLK_TC_SHIFT) & what) == 0)
		return 1;
	if (sector && (sector < bt->start_lba || sector > bt->end_lba))
		return 1;
	if (bt->pid && pid != bt->pid)
		return 1;

	return 0;
}
```

### ioctl

当然，也提供了 ioctl 来操作

关注一下 blk_trace_ioctl 和 sg_ioctl_common 即可

```c
	case BLKTRACESETUP:
		return blk_trace_setup(sdp->device->request_queue, sdp->name,
				       MKDEV(SCSI_GENERIC_MAJOR, sdp->index),
				       NULL, p);
	case BLKTRACESTART:
		return blk_trace_startstop(sdp->device->request_queue, 1);
	case BLKTRACESTOP:
		return blk_trace_startstop(sdp->device->request_queue, 0);
	case BLKTRACETEARDOWN:
		return blk_trace_remove(sdp->device->request_queue);
```

当开始执行 `blktrace -d /dev/vdc -o - | blkparse -i -` 的时候，结果如下:

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_ioctl
        - __se_sys_ioctl
          - __do_sys_ioctl
            - vfs_ioctl
              - blkdev_ioctl
                - blk_trace_ioctl
                  - __blk_trace_setup
                    - do_blk_trace_setup
                      - get_probe_ref
                        - blk_register_tracepoints

## blk_register_tracepoints

echo 1 | sudo tee /sys/block/"$disk"/trace/enable 的时候会
触发

- do_syscall_64
  - do_syscall_x64
    - ksys_write
      - vfs_write
        - new_sync_write
          - kernfs_fop_write_iter
            - sysfs_blk_trace_attr_store
              - blk_trace_setup_queue
                - get_probe_ref
                  - blk_register_tracepoints


- do_syscall_64
  - do_syscall_x64
    - ksys_write
      - vfs_write
        - new_sync_write
          - kernfs_fop_write_iter
            - sysfs_blk_trace_attr_store
              - blk_trace_remove_queue
                - put_probe_ref
                  - blk_unregister_tracepoints

## blktrace 各个阶段的含义是什么
<!-- dc2cd2d2-cfdb-4fba-bd1d-c3c2a259a061 -->

简而言之，记住 QGPUIDC

```c
/*
 * Basic trace actions
 */
enum blktrace_act {

};
/*
 * Trace categories
 */
enum blktrace_cat {

}
```

这个两个 enum 构成一个完成的 trace action :
```c
/*
 * Trace actions in full. Additionally, read or write is masked
 */
#define BLK_TA_QUEUE		(__BLK_TA_QUEUE | BLK_TC_ACT(BLK_TC_QUEUE))
#define BLK_TA_BACKMERGE	(__BLK_TA_BACKMERGE | BLK_TC_ACT(BLK_TC_QUEUE))
#define BLK_TA_FRONTMERGE	(__BLK_TA_FRONTMERGE | BLK_TC_ACT(BLK_TC_QUEUE))
#define	BLK_TA_GETRQ		(__BLK_TA_GETRQ | BLK_TC_ACT(BLK_TC_QUEUE))
#define	BLK_TA_SLEEPRQ		(__BLK_TA_SLEEPRQ | BLK_TC_ACT(BLK_TC_QUEUE))
#define	BLK_TA_REQUEUE		(__BLK_TA_REQUEUE | BLK_TC_ACT(BLK_TC_REQUEUE))
#define BLK_TA_ISSUE		(__BLK_TA_ISSUE | BLK_TC_ACT(BLK_TC_ISSUE))
#define BLK_TA_COMPLETE		(__BLK_TA_COMPLETE| BLK_TC_ACT(BLK_TC_COMPLETE))
#define BLK_TA_PLUG		(__BLK_TA_PLUG | BLK_TC_ACT(BLK_TC_QUEUE))
#define BLK_TA_UNPLUG_IO	(__BLK_TA_UNPLUG_IO | BLK_TC_ACT(BLK_TC_QUEUE))
#define BLK_TA_UNPLUG_TIMER	(__BLK_TA_UNPLUG_TIMER | BLK_TC_ACT(BLK_TC_QUEUE))
#define BLK_TA_INSERT		(__BLK_TA_INSERT | BLK_TC_ACT(BLK_TC_QUEUE))
#define BLK_TA_SPLIT		(__BLK_TA_SPLIT)
#define BLK_TA_BOUNCE		(__BLK_TA_BOUNCE)
#define BLK_TA_REMAP		(__BLK_TA_REMAP | BLK_TC_ACT(BLK_TC_QUEUE))
#define BLK_TA_ABORT		(__BLK_TA_ABORT | BLK_TC_ACT(BLK_TC_QUEUE))
#define BLK_TA_DRV_DATA	(__BLK_TA_DRV_DATA | BLK_TC_ACT(BLK_TC_DRV_DATA))
```
这些就是 register_trace_block_plug 注册的 hook 执行的时候记录的 action ，或者说，
内核中的 tracepoint 就是和这些定义来对应的

在 what2act 翻译名称:

```c
static const struct {
	const char *act[2];
	void	   (*print)(struct trace_seq *s, const struct trace_entry *ent,
			    bool has_cg);
} what2act[] = {
	[__BLK_TA_QUEUE]	= { {  "Q", "queue" },	   blk_log_generic },
	[__BLK_TA_FRONTMERGE]	= { {  "F", "frontmerge" }, blk_log_generic },
	[__BLK_TA_GETRQ]	= { {  "G", "getrq" },	   blk_log_generic },
	[__BLK_TA_SLEEPRQ]	= { {  "S", "sleeprq" },	   blk_log_generic },
	[__BLK_TA_REQUEUE]	= { {  "R", "requeue" },	   blk_log_with_error },
	[__BLK_TA_ISSUE]	= { {  "D", "issue" },	   blk_log_generic },
	[__BLK_TA_COMPLETE]	= { {  "C", "complete" },   blk_log_with_error },
	[__BLK_TA_PLUG]		= { {  "P", "plug" },	   blk_log_plug },
	[__BLK_TA_UNPLUG_IO]	= { {  "U", "unplug_io" },  blk_log_unplug },
	[__BLK_TA_UNPLUG_TIMER]	= { { "UT", "unplug_timer" }, blk_log_unplug },
	[__BLK_TA_INSERT]	= { {  "I", "insert" },	   blk_log_generic },
	[__BLK_TA_SPLIT]	= { {  "X", "split" },	   blk_log_split },
	[__BLK_TA_REMAP]	= { {  "A", "remap" },	   blk_log_remap },
};
```

当 cat trace_pipe 的时候:

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - ksys_read
        - vfs_read
          - tracing_read_pipe
            - print_one_line 在这里使用 what2act

blk_trace_str2mask 可以看到通过 act_mask 来操作。

```c
static void blk_register_tracepoints(void)
{
	int ret;

	ret = register_trace_block_rq_insert(blk_add_trace_rq_insert, NULL);
	ret = register_trace_block_rq_issue(blk_add_trace_rq_issue, NULL);
	ret = register_trace_block_rq_merge(blk_add_trace_rq_merge, NULL);
	ret = register_trace_block_rq_requeue(blk_add_trace_rq_requeue, NULL);
	ret = register_trace_block_rq_complete(blk_add_trace_rq_complete, NULL);
	ret = register_trace_block_bio_complete(blk_add_trace_bio_complete, NULL);
	ret = register_trace_block_bio_backmerge(blk_add_trace_bio_backmerge, NULL);
	ret = register_trace_block_bio_frontmerge(blk_add_trace_bio_frontmerge, NULL);
	ret = register_trace_block_bio_queue(blk_add_trace_bio_queue, NULL);
	ret = register_trace_block_getrq(blk_add_trace_getrq, NULL);
	ret = register_trace_block_plug(blk_add_trace_plug, NULL);
	ret = register_trace_block_unplug(blk_add_trace_unplug, NULL);
	ret = register_trace_block_split(blk_add_trace_split, NULL);
	ret = register_trace_block_bio_remap(blk_add_trace_bio_remap, NULL);
	ret = register_trace_block_rq_remap(blk_add_trace_rq_remap, NULL);
}
```

|----------------------------|----------------------------------------|-----|----------------------------------------------------|
| trace_block_bio_queue      | BLK_TA_QUEUE                           | 'Q' | submit_bio_noacct_nocheck                          |
| trace_block_getrq          | BLK_TA_GETRQ                           | 'G' | blk_mq_submit_bio                                  |
| trace_block_plug           | BLK_TA_PLUG                            | 'P' | blk_add_rq_to_plug                                 |
| trace_block_unplug         | BLK_TA_UNPLUG_IO / BLK_TA_UNPLUG_TIMER | 'U' | 多个地方                                           |
| trace_block_rq_insert      | BLK_TA_INSERT                          | 'I' | 多个地方                                           |
| trace_block_rq_issue       | BLK_TA_ISSUE                           | 'D' | blk_mq_start_request                               |
| trace_block_rq_complete    | BLK_TA_COMPLETE                        | 'C' | blk_update_request / blk_complete_request          |
| trace_block_bio_complete   | BLK_TA_COMPLETE                        | 'C' | bio_endio / nvme_trace_bio_complete                |
| trace_block_rq_requeue     | BLK_TA_REQUEUE                         |     | __blk_mq_requeue_request                           |
| trace_block_rq_merge       | BLK_TA_BACKMERGE                       | 'M' | attempt_merge
| trace_block_split          | 多个                                   |     | bio_submit_split_bioset / dm_split_and_process_bio |
| trace_block_bio_remap      | BLK_TA_REMAP                           |     | 多个地方，不过主要是 dm_submit_bio_remap
| trace_block_rq_remap       | BLK_TA_REMAP                           |     | drivers/md/dm-rq.c:map_request                     |
| trace_block_bio_backmerge  | BLK_TA_BACKMERGE                       |     | bio_attempt_back_merge                             |
| trace_block_bio_frontmerge | BLK_TA_FRONTMERGE                      |     | bio_attempt_front_merge                            |

终于理解 'Q' 的含义了，也就是需要首先提交 bio ，然后再提交给 request queue 哪里获取到一个 request 。

### block_rq_insert
调用位置比较多，当一个 request 通过 request::queuelist 将自己挂到
链表上的时候: 可以是软件队列，也可以是 scheduler 的队列
```c
/**
 * block_rq_insert - insert block operation request into queue
 * @rq: block IO operation request
 *
 * Called immediately before block operation request @rq is inserted
 * into queue @q.  The fields in the operation request @rq struct can
 * be examined to determine which device and sectors the pending
 * operation would access.
 */
```
当 scheduler 是 none 的时候，这个 tracepoint 是没有办法触发的。

### block_rq_issue
发送到 device driver ，几乎等同于发送给硬件

#### D2C 的 latency 的确切含义
blk_mq_start_request 被各种驱动调用，其中解答各种我们疑惑的问题:

```txt
	trace_block_rq_issue(rq); // 'D' 状态
	blk_add_timer(rq);        // 超时
	WRITE_ONCE(rq->state, MQ_RQ_IN_FLIGHT); // inflight 统计
```

典型的调用路径为:

```txt
    blk_mq_start_request+205
    nvme_prep_rq.part.0+934
    nvme_queue_rqs+166
    blk_mq_flush_plug_list.part.0+1146
    blk_add_rq_to_plug+167
    blk_mq_submit_bio+1371
```

- __blk_flush_plug
  - blk_mq_flush_plug_list
    - __blk_mq_flush_plug_list
      - __blk_mq_flush_plug_list
        - virtio_queue_rqs
          - virtblk_prep_rq_batch
            - virtblk_prep_rq
              - blk_mq_start_request
                - trace_block_rq_issue
                  - blk_add_trace_rq_issue

### bio_complete 和 rq_complete

实际上存在 bio_complete 和 rq_complete 的，但是最后只有一个
tracepoint 的展示
```txt
@[
        bio_endio+5
        clone_endio+142
        blk_update_request+257 (trace_block_rq_complete 的地方)
        scsi_end_request+41
        scsi_io_completion+83
        virtscsi_req_done+126
        vring_interrupt+97
        __handle_irq_event_percpu+138
        handle_irq_event+59
        handle_edge_irq+217
        __common_interrupt+77
        common_interrupt+128
        asm_common_interrupt+38
        pv_native_safe_halt+15
        default_idle+19
        default_idle_call+122
        do_idle+480
        cpu_startup_entry+41
        start_secondary+281
        common_startup_64+318
]: 9
```

### block_getrq
由于 bio 合并，未必会申请一个新的 request ，当申请到一个新的 request 之后，
就会触发 trace_block_getrq

### block_rq_remap 和 block_bio_remap 具体是?

trace_block_bio_remap 一共四个调用位置:

- dm_submit_bio_remap **主要** ，也就是在虚拟机中经常观察到的
- linear_make_request
- blk_partition_remap
- nvme_ns_head_submit_bio

trace_block_rq_remap 调用位置: 仅仅用于 drivers/md/dm-rq.c 中，这是
基本上被废弃的东西。

### 例子

sudo cgexec --sticky -g cpu,memory,cpuset:/ blktrace -d /dev/sdh
每一个 CPU 都会生成一个文件，观察其中一个 CPU 的效果
sudo blkparse -i sde.blktrace.26

得到的结果大致为
```txt
sudo blkparse -i sde.blktrace.25
Input file sde.blktrace.25 added
Input file sde.blktrace.26 added
  8,64  26        1     0.000000000  3486  Q   R 0 + 8 [a.out]
  8,64  26        2     0.000003446  3486  G   R 0 + 8 [a.out]
  8,64  26        3     0.000004205  3486  P   N [a.out]
  8,64  26        4     0.000004481  3486  U   N [a.out] 1
  8,64  26        5     0.000005128  3486  I   R 0 + 8 [a.out]
  8,64  26        6     0.000009648  3486  D   R 0 + 8 [a.out]
  8,64  26        7     0.001027448     0  C   R 0 + 8 [0]
CPU26 (sde):
 Reads Queued:           1,        4KiB  Writes Queued:           0,        0KiB
 Read Dispatches:        1,        4KiB  Write Dispatches:        0,        0KiB
 Reads Requeued:         0               Writes Requeued:         0
 Reads Completed:        1,        4KiB  Writes Completed:        0,        0KiB
 Read Merges:            0,        0KiB  Write Merges:            0,        0KiB
 Read depth:             1               Write depth:             0
 IO unplugs:             1               Timer unplugs:           0

Throughput (R/W): 4,000KiB/s / 0KiB/s
Events (sde): 7 entries
Skips: 0 forward (0 -   0.0%)
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
