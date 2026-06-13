# iopoll

## 两种 poll 的抽象
<!-- 6b1a3f7c-569a-4dca-a7c5-4f3caeaeed50 -->
```c
static int io_uring_classic_poll(struct io_kiocb *req, struct io_comp_batch *iob,
				unsigned int poll_flags)
{
	struct file *file = req->file;

	if (req->opcode == IORING_OP_URING_CMD) {
		struct io_uring_cmd *ioucmd;

		ioucmd = io_kiocb_to_cmd(req, struct io_uring_cmd);
		return file->f_op->uring_cmd_iopoll(ioucmd, iob, poll_flags);
	} else {
		struct io_rw *rw = io_kiocb_to_cmd(req, struct io_rw);

		return file->f_op->iopoll(&rw->kiocb, iob, poll_flags);
	}
}
```

- file_operations::iopoll 就是文件系统在注册，例如 ext4 , xfs 以及 block 层
- file->f_op->uring_cmd_iopoll 一半来说，用于驱动
  - fuse_uring_cmd
  - blkdev_uring_cmd : https://lore.kernel.org/all/cover.1726072086.git.asml.silence@gmail.com/
    - 这个不是用于 io 的，而是用于 implement async block discards and other ops via io_uring

(2026-03-11 不知道为什么，使用 xfs 显示不支持，但是也不知道错误是从哪里返回的。)

## uring_cmd

### 如果 iouring 可以支持 ioctl

https://lwn.net/Articles/844875/ (2021)

It is natural to want to support ioctl() in io_uring;
it is not uncommon to mix ioctl() calls with regular I/O,
and it would be useful to be able to do everything asynchronously.
But every ioctl() call is different,
and none of them were designed for asynchronous execution,
so an ioctl() implementation within io_uring would have no choice but to execute every call in a separate thread.
That might be better than nothing, but it is not anywhere near as efficient as it could be,
especially for calls that can be executed right away.
Doing ioctl() right for io_uring essentially calls for reinventing the ioctl() interface.

由于 ioctl 天然不是用于异步的，所以 ioctl 似乎只是异步的提交一个 op 而已。

实际上，这个功能演化为:

```txt
struct io_uring_cmd {
	struct file	*file;
	const struct io_uring_sqe *sqe;
	/* callback to defer completions to task context */
	void (*task_work_cb)(struct io_uring_cmd *cmd, unsigned);
	u32		cmd_op;
	u32		flags;
	u8		pdu[32]; /* available inline for free use */
};
```

### iopoll
使用用户: io_uring::io_do_iopoll

```c
const struct file_operations ext4_file_operations = {
   // ...
	.iopoll		= iocb_bio_iopoll,
  // ...
```

- iocb_bio_iopoll
  - bio_poll
    - blk_mq_poll : 如果是 multiqueue
      - 循环调用: request_queue::mq_ops::poll，主要分析 virtblk_poll nvme_tcp_poll nvme_poll
    - gendisk::fops::poll_bio

- nvme_poll
  - nvme_poll_cq
    - while nvme_cqe_pending; do nvme_handle_cqe; nvme_update_cq_head; done

## io uring
https://lore.gnuweeb.org/io-uring/20250422030153.1166445-1-qq282012236@gmail.com/T/#m7364930cb96d611685e53ebbc20980e69846893d
似乎不是一个专业的回复


https://lpc.events/event/11/contributions/989/attachments/747/1723/lpc-2021-building-a-fast-passthru.pdf

用户态的驱动，可以使用 ioctl 来操作 nvme  ，但是 ioctl 都是同步的:

这里的 issue flags 怎么获取到的:
```c
int io_uring_cmd(struct io_kiocb *req, unsigned int issue_flags)
```


fio -name=test -filename=/dev/nvme1n1 \
  -size=1G -rw=randread -bs=4k -iodepth=256 \
  -ioengine=io_uring_cmd -direct=1 -hipri

不过测试有问题:
```txt
sudo fio -name=test -filename=/dev/nvme1n1 \
  -size=1G -rw=randread -bs=4k -iodepth=256 \
  -ioengine=io_uring_cmd -direct=1 -hipri

[sudo] password for martins3:
test: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring_cmd, iodepth=256
fio-3.39
Starting 1 process
ioengine io_uring_cmd only works with nvme ns generic char devices (/dev/ngXnY)
```

哦，才意识到在 /dev 下面不仅仅有用 lsblk 可以看到的 /dev/nvme0n1 ，
其实 /dev/nvme0 才是控制这个
```c
static const struct file_operations nvme_dev_fops = {
	.owner		= THIS_MODULE,
	.open		= nvme_dev_open,
	.release	= nvme_dev_release,
	.unlocked_ioctl	= nvme_dev_ioctl,
	.compat_ioctl	= compat_ptr_ioctl,
	.uring_cmd	= nvme_dev_uring_cmd,
};
```

还有有错误:
```txt
sudo fio -name=test -filename=/dev/nvme1 \
  -size=1G -rw=randread -bs=4k -iodepth=256 \
  -ioengine=io_uring_cmd -direct=1 -hipri

[sudo] password for martins3:
test: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring_cmd, iodepth=256
fio-3.39
Starting 1 process
/dev/nvme1: failed to fetch namespace-id
```

### socket
```c
static const struct file_operations socket_file_ops = {
	.owner =	THIS_MODULE,
	.read_iter =	sock_read_iter,
	.write_iter =	sock_write_iter,
	.poll =		sock_poll,
	.unlocked_ioctl = sock_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_sock_ioctl,
#endif
	.uring_cmd =    io_uring_cmd_sock,
	.mmap =		sock_mmap,
	.release =	sock_close,
	.fasync =	sock_fasync,
	.splice_write = splice_to_socket,
	.splice_read =	sock_splice_read,
	.splice_eof =	sock_splice_eof,
	.show_fdinfo =	sock_show_fdinfo,
};
```
相关测试在 vn/code/src/c/iouring/cmd-socket.c

此外 liburing 的两个测试文件吧:
- test/socket-io-cmd.c
- test/socket-getsetsock-cmd.c

内核中 io_uring_cmd_sock

其实就是普通的 ioctl 的 io uring 化而已。

## file_operations::iopoll


- io_iopoll_check
- `__io_sq_thread`
  - io_do_iopoll : 如果是 io uring cmd
    - file->f_op->uring_cmd_iopoll
      - blk_rq_poll
        - blk_hctx_poll : 一个需要进行文件上变换
    - file->f_op->iopoll
      - iocb_bio_iopoll : xfs ext4 和 blk 都是注册的这个
        - bio_poll
          - blk_mq_poll
            - blk_hctx_poll
              - blk_mq_ops::poll



hipri 做什么的? hipri 在不同的环境中，其意义不同

https://fio.readthedocs.io/en/latest/fio_doc.html
```txt
hipri
[io_uring] [io_uring_cmd] [xnvme]

If this option is set, fio will attempt to use polled IO completions. Normal IO completions generate interrupts to signal the completion of IO, polled completions do not. Hence they are require active reaping by the application. The benefits are more efficient IO for high IOPS scenarios, and lower latencies for low queue depth IO.

[libblkio]

Use poll queues. This is incompatible with libblkio_wait_mode=eventfd and libblkio_force_enable_completion_eventfd.

[pvsync2]

Set RWF_HIPRI on I/O, indicating to the kernel that it’s of higher priority than normal.

[sg]

If this option is set, fio will attempt to use polled IO completions. This will have a similar effect as (io_uring)hipri. Only SCSI READ and WRITE commands will have the SGV4_FLAG_HIPRI set (not UNMAP (trim) nor VERIFY). Older versions of the Linux sg driver that do not support hipri will simply ignore this flag and do normal IO. The Linux SCSI Low Level Driver (LLD) that “owns” the device also needs to support hipri (also known as iopoll and mq_poll). The MegaRAID driver is an example of a SCSI LLD. Default: clear (0) which does normal (interrupted based) IO.
```

```txt
io_uring_setup(1, {flags=IORING_SETUP_IOPOLL|IORING_SETUP_CQSIZE|IORING_SETUP_COOP_TASKRUN|IORING_SETUP_SINGLE_ISSUER|IORING_SETUP_DEFER_TASKRUN, sq_thread_cpu=0, sq_thread_idle=0, sq_entries=1, cq_entries=1, features=IORING_FEAT_SINGLE_MMAP|IORING_FEAT_NODROP|IORING_FEAT_SUBMIT_STABLE|IORING_FEAT_RW_CUR_POS|IORING_FEAT_CUR_PERSONALITY|IORING_FEAT_FAST_POLL|IORING_FEAT_POLL_32BITS|IORING_FEAT_SQPOLL_NONFIXED|IORING_FEAT_EXT_ARG|IORING_FEAT_NATIVE_WORKERS|IORING_FEAT_RSRC_TAGS|IORING_FEAT_CQE_SKIP|IORING_FEAT_LINKED_FILE|IORING_FEAT_REG_REG_RING|IORING_FEAT_RECVSEND_BUNDLE|IORING_FEAT_MIN_TIMEOUT|IORING_FEAT_RW_ATTR, sq_off={head=0, tail=4, ring_mask=16, ring_entries=24, flags=36, dropped=32, array=128, user_addr=0}, cq_off={head=8, tail=12, ring_mask=20, ring_entries=28, overflow=44, cqes=64, flags=40, user_addr=0}}) = 6
```
也就是 IORING_SETUP_IOPOLL 就可以了

### 对于 block layer 的改造

 sudo modprobe null-blk
```sh
sudo fio -name=test -filename=/dev/nullb0 \
  -rw=randread -bs=4k -iodepth=1 \
  -ioengine=pvsync2 -direct=1 -hipri
```
还是没有效果

```sh
sudo fio -name=test -filename=/dev/nullb0 \
  -rw=randread -bs=4k -iodepth=1 \
  -ioengine=io_uring -direct=1 -hipri
```

如果 io engine 切换为 io_uring，那么为::
```txt
@[
        null_poll+0
        blk_mq_poll+92
        bio_poll+156
        iocb_bio_iopoll+32
        io_uring_classic_poll+48
        io_do_iopoll+380
        io_iopoll_check+228
        __do_sys_io_uring_enter+788
        __arm64_sys_io_uring_enter+48
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 320184
```
同时在 null_queue_rq 中观测到 hctx->type 也是 HCTX_TYPE_POLL 的
```c
static blk_status_t null_queue_rq(struct blk_mq_hw_ctx *hctx,
				  const struct blk_mq_queue_data *bd)
{
	struct request *rq = bd->rq;
	struct nullb_cmd *cmd = blk_mq_rq_to_pdu(rq);
	struct nullb_queue *nq = hctx->driver_data;
	sector_t nr_sectors = blk_rq_sectors(rq);
	sector_t sector = blk_rq_pos(rq);
	const bool is_poll = hctx->type == HCTX_TYPE_POLL; // 这个是 true
```

上述 fio 测试发现:
blkdev_direct_IO 中，走的总是 `__blkdev_direct_IO_simple`
而在 `__blkdev_direct_IO_async` 在会去将这个 flag 传递下去:

```c
	if (iocb->ki_flags & IOCB_HIPRI) {
		bio->bi_opf |= REQ_POLLED;
		submit_bio(bio);
		WRITE_ONCE(iocb->private, bio);
```

```txt
@[
        __blkdev_direct_IO_async+0
        blkdev_read_iter+232
        __io_read+188
        io_read+36
        __io_issue_sqe+80
        io_issue_sqe+76
        io_submit_sqes+296
        __do_sys_io_uring_enter+456
        __arm64_sys_io_uring_enter+48
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 828979
```

```txt
@[
        __blkdev_direct_IO_simple+0
        blkdev_read_iter+232
        do_iter_readv_writev+420
        vfs_readv+340
        do_preadv+152
        __arm64_sys_preadv2+48
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 1068126
```

### 文件测试
```s
fio -name=test -filename=/home/martins3/data/mm \
  -size=1G -rw=randread -bs=4k -iodepth=256 -ioengine=io_uring -direct=1 -hipri
```

可以发现文件系统其实和 nvme 一样，都是注册的 iocb_bio_iopoll 作为
callback

不知道为什么在 kunpeng 服务器上测试就是可以的，
但是在 windows 虚拟机中测试就不可以

liburing 有一个测试项目
liburing/test/iopoll.c 下测试项目
下执行 test/iopoll.t 就可以

windows 直接报错:
```txt
File/device/fs doesn't support polled IO
```
而且很奇怪，都是 xfs 的

### nvme 测试
sudo fio -name=test -filename=/dev/nvme1n1p1 -rw=randread -bs=256k -iodepth=256 -ioengine=io_uring -direct=1 -hipri

```txt
@[
        iocb_bio_iopoll+0
        io_do_iopoll+380
        io_iopoll_check+228
        __do_sys_io_uring_enter+788
        __arm64_sys_io_uring_enter+48
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 695602
```

```txt
@[
        io_complete_rw_iopoll+0
        bio_endio+372
        blk_mq_end_request_batch+540
        nvme_pci_complete_batch+216
        nvme_irq+128
        __handle_irq_event_percpu+84
        handle_irq_event+76
        handle_fasteoi_irq+180
        handle_irq_desc+60
        generic_handle_domain_irq+36
        __gic_handle_irq_from_irqson.isra.0+340
        gic_handle_irq+40
        call_on_irq_stack+48
        do_interrupt_handler+136
        el1_interrupt+68
        el1h_64_irq_handler+24
        el1h_64_irq+128
        default_idle_call+56
        cpuidle_idle_call+380
        do_idle+244
        cpu_startup_entry+60
        secondary_start_kernel+224
        __secondary_switched+192
]: 2182
@[
        io_complete_rw_iopoll+0
        bio_endio+372
        bio_submit_split+344
        bio_split_rw+144
        blk_mq_submit_bio+1476
        __submit_bio+124
        submit_bio_noacct_nocheck+324
        submit_bio_noacct+316
        submit_bio+172
        __blkdev_direct_IO_async+340
        blkdev_direct_IO+260
        blkdev_read_iter+232
        __io_read+188
        io_read+36
        __io_issue_sqe+80
        io_issue_sqe+76
        io_submit_sqes+296
        __do_sys_io_uring_enter+456
        __arm64_sys_io_uring_enter+48
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 9830
```
也就是，发生 split io 的时候，就会调用到
io_complete_rw_iopoll

```txt
 sudo bpftrace -e 'fentry:vmlinux:bio_submit_split { @[args->split_sectors]=count() }'
Attaching 1 probe...
stdin:1:35-65: WARNING: File exists
Additional Info - helper: map_update_elem, retcode: -17
fentry:vmlinux:bio_submit_split { @[args->split_sectors]=count() }
                                  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
^C

@[256]: 78942
@[-11]: 78943
@[0]: 78946
```
哦，这里的错误的就是 -EGAGIN

## EGAGIN 丢失问题分析
commit bcb0fda3c2da ("io_uring/rw: ensure reissue path is correctly handled for IOPOLL")
commit d803d123948f ("io_uring/rw: handle -EAGAIN retry at IO completion time")

```txt
History: #0
Commit:  bcb0fda3c2da9fe4721d3e73d80e778c038e7d27
Author:  Jens Axboe <axboe@kernel.dk>
Date:    Wed 05 Mar 2025 04:03:34 PM EST

io_uring/rw: ensure reissue path is correctly handled for IOPOLL

The IOPOLL path posts CQEs when the io_kiocb is marked as completed,
so it cannot rely on the usual retry that non-IOPOLL requests do for
read/write requests.

If -EAGAIN is received and the request should be retried, go through
the normal completion path and let the normal flush logic catch it and
reissue it, like what is done for !IOPOLL reads or writes.

Fixes: d803d123948f ("io_uring/rw: handle -EAGAIN retry at IO completion time")
Reported-by: John Garry <john.g.garry@oracle.com>
Link: https://lore.kernel.org/io-uring/2b43ccfa-644d-4a09-8f8f-39ad71810f41@oracle.com/
Signed-off-by: Jens Axboe <axboe@kernel.dk>
```

```c
static void io_complete_rw_iopoll(struct kiocb *kiocb, long res)
{
	struct io_rw *rw = container_of(kiocb, struct io_rw, kiocb);
	struct io_kiocb *req = cmd_to_io_kiocb(rw);

	if (kiocb->ki_flags & IOCB_WRITE)
		io_req_end_write(req);
	if (unlikely(res != req->cqe.res)) {
		if (res == -EAGAIN && io_rw_should_reissue(req)) {
			req->flags |= REQ_F_REISSUE | REQ_F_BL_NO_RECYCLE;
			return;
		}
		req->cqe.res = res;
	}

	/* order with io_iopoll_complete() checking ->iopoll_completed */
	smp_store_release(&req->iopoll_completed, 1);
}
```

```c
static void io_complete_rw_iopoll(struct kiocb *kiocb, long res)
{
	struct io_rw *rw = container_of(kiocb, struct io_rw, kiocb);
	struct io_kiocb *req = cmd_to_io_kiocb(rw);

	if (kiocb->ki_flags & IOCB_WRITE)
		io_req_end_write(req);
	if (unlikely(res != req->cqe.res)) {
		if (res == -EAGAIN && io_rw_should_reissue(req))
			req->flags |= REQ_F_REISSUE | REQ_F_BL_NO_RECYCLE;
		else
			req->cqe.res = res;
	}

	/* order with io_iopoll_complete() checking ->iopoll_completed */
	smp_store_release(&req->iopoll_completed, 1);
}
```

io_rw_should_reissue 理解下，也就是:
只有满足特定条件的块设备（如磁盘）或普通文件的异步 I/O，才允许自动重试；
其他情况（比如 socket、pipe、设置了 NOWAIT、上下文正在退出等）一律不重试

对于不允许重试的，直接返回错误到用户态

### [ ] 为什么只有 IOCB_READ 才有影响，
```txt
	if (kiocb->ki_flags & IOCB_WRITE)
		io_req_end_write(req);
```
不是的，是 write 有额外的工作

### 为什么还是可以接受到中断？

不过看，的确是在 __io_prep_rw 这里才对:
```c
	if (req->ctx->flags & IORING_SETUP_IOPOLL)
		rw->kiocb.ki_complete = io_complete_rw_iopoll;
	else
		rw->kiocb.ki_complete = io_complete_rw;
```

只能说，话又说回来，
```txt
- 71.16% invoke_syscall
   - 71.12% __arm64_sys_io_uring_enter
      - 70.28% __do_sys_io_uring_enter
         - 38.90% io_submit_sqes
            + 36.32% io_issue_sqe
            + 1.65% io_init_req
         - 27.65% io_iopoll_check
            - 27.35% io_do_iopoll
               - 23.94% nvme_pci_complete_batch
                  - 19.43% nvme_unmap_data
                     + 19.20% dma_unmap_page_attrs
                  + 4.15% blk_mq_end_request_batch
               + 2.13% __io_submit_flush_completio
               - 0.96% io_uring_classic_poll
                  - iocb_bio_iopoll
                  - bio_poll
                     - 0.96% blk_mq_poll
                        - 0.94% blk_hctx_poll
                           - nvme_poll
                                0.90% nvme_poll_cq
```
哦，io_uring_enter 需要提交多个 io


哦，如果 submit 的 size 修改为 4k ，那么所有的 io_complete_rw_iopoll 提交都是在这里了:
```txt
@[
        io_complete_rw_iopoll+0
        bio_endio+372
        blk_mq_end_request_batch+540
        nvme_pci_complete_batch+216
        io_do_iopoll+508
        io_iopoll_check+228
        __do_sys_io_uring_enter+788
        __arm64_sys_io_uring_enter+48
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 2406039
```
这个时候就无法接受到中断了

### 哦，果然是依赖这个问题的

```txt
commit d803d123948feffbd992213e144df224097f82b0
Author: Jens Axboe <axboe@kernel.dk>
Date:   Tue Jan 7 10:59:35 2025 -0700

    io_uring/rw: handle -EAGAIN retry at IO completion time

    Rather than try and have io_read/io_write turn REQ_F_REISSUE into
    -EAGAIN, catch the REQ_F_REISSUE when the request is otherwise
    considered as done. This is saner as we know this isn't happening
    during an actual submission, and it removes the need to randomly
    check REQ_F_REISSUE after read/write submission.

    If REQ_F_REISSUE is set, __io_submit_flush_completions() will skip over
    this request in terms of posting a CQE, and the regular request
    cleaning will ensure that it gets reissued via io-wq.

    Signed-off-by: Jens Axboe <axboe@kernel.dk>
```

### io_do_iopoll 中访问 io_kiocb::iopoll_completed

如果出现丢失，就很难受

###	if (unlikely(res != req->cqe.res))

req->cqe.res 是当时其他的 io 量
不想等，就是返回有错误

## io uring sq poll
<!-- 12c123ce-3ad3-43c4-9424-50f650a405d0 -->

让内核线程来获取 submit queue ，而不是用系统调用。

分析下 https://unixism.net/loti/tutorial/sq_poll.html

### 基本测试
使用 advance-sqpoll.c 来测试:
```txt
sudo bpftrace -e 'rawtracepoint:io_uring_submit_req { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[iou-sqp-17508]: 4
@[iou-sqp-17427]: 4
```

```txt
+ sudo bpftrace -e 'rawtracepoint:io_uring_submit_req { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[
        bpf_prog_6deef7357e7b4530_sd_fw_ingress+327
        bpf_prog_6deef7357e7b4530_sd_fw_ingress+327
        bpf_trace_run1+129
        io_submit_sqes+476
        io_sq_thread+855
        ret_from_fork+49
        ret_from_fork_asm+26
]: 4
```

如果注释掉
```txt
	params.flags |= IORING_SETUP_SQPOLL;
```

提交变化为，也就是说走系统调用了:
```txt
@[
        bpf_prog_6deef7357e7b4530_sd_fw_ingress+495
        bpf_prog_6deef7357e7b4530_sd_fw_ingress+495
        bpf_trace_run1+129
        io_submit_sqes+476
        __do_sys_io_uring_enter+530
        do_syscall_64+127
        entry_SYSCALL_64_after_hwframe+118
]: 8
```

源码分析为:
- io_uring_setup
   - io_sq_offload_create : io_uring_params::sq_thread_idle 传递到 io_ring_ctx::sq_thread_idle
   - create_io_thread(io_sq_thread, sqd, NUMA_NO_NODE)
- io_sq_thread


## [ ] io_uring/sqpoll.c 和 io_uring/poll.c 的差别是什么?

## pvread 也是有 poll 模式的，但是已经被废弃了

正如 fio 文档中介绍
可以不使用 io uring 来实现 block layer 的 polling mode

https://manpages.debian.org/testing/manpages-dev/preadv2.2.en.html

```sh
strace -f -e preadv2 -o log.txt \
fio -name=test -filename=/home/martins3/data/mm.dump \
  -size=1G -rw=randread -bs=4k -iodepth=1 \
  -ioengine=pvsync2 -direct=1 -hipri
```

然后就可以找到这个了:
```txt
33684 preadv2(6, [{iov_base="4\222J\6\341\254\271\37h$\225\f\302Ys?\234\266\337\22\243\6-_\4\333t\37e`\240\236"..., iov_len=4096}], 1, 519000064, RWF_HIPRI) = 4096
33684 preadv2(6, [{iov_base="\340\255@\r\262\20\312\22\300[\201\32d!\224%\240\t\302'\0262^8`eCBzS\362]"..., iov_len=4096}], 1, 377311232, RWF_HIPRI) = 4096
```

似乎这个好几个都是互相有映射关系的:
```c
#define IOCB_HIPRI		(__force int) RWF_HIPRI

#define REQ_POLLED	(__force blk_opf_t)(1ULL << __REQ_POLLED)

enum hctx_type {
	HCTX_TYPE_DEFAULT,
	HCTX_TYPE_READ,
	HCTX_TYPE_POLL,

	HCTX_MAX_TYPES,
};
```

```diff
History:        #0
Commit:         9650b453a3d4b1b8ed4ea8bcb9b40109608d1faf
Author:         Ming Lei <ming.lei@redhat.com>
Committer:      Jens Axboe <axboe@kernel.dk>
Author Date:    Wed 20 Apr 2022 10:31:10 AM EDT
Committer Date: Mon 02 May 2022 12:07:42 PM EDT

block: ignore RWF_HIPRI hint for sync dio

So far bio is marked as REQ_POLLED if RWF_HIPRI/IOCB_HIPRI is passed
from userspace sync io interface, then block layer tries to poll until
the bio is completed. But the current implementation calls
blk_io_schedule() if bio_poll() returns 0, and this way causes io hang or
timeout easily.

But looks no one reports this kind of issue, which should have been
triggered in normal io poll sanity test or blktests block/007 as
observed by Changhui, that means it is very likely that no one uses it
or no one cares it.

Also after io_uring is invented, io poll for sync dio becomes legacy
interface.

So ignore RWF_HIPRI hint for sync dio.

CC: linux-mm@kvack.org
Cc: linux-xfs@vger.kernel.org
Reported-by: Changhui Zhong <czhong@redhat.com>
Suggested-by: Christoph Hellwig <hch@lst.de>
Signed-off-by: Ming Lei <ming.lei@redhat.com>
Tested-by: Changhui Zhong <czhong@redhat.com>
Reviewed-by: Christoph Hellwig <hch@lst.de>
Link: https://lore.kernel.org/r/20220420143110.2679002-1-ming.lei@redhat.com
Signed-off-by: Jens Axboe <axboe@kernel.dk>
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
