# nvme

## 内核模块的参数
/sys/module/nvme_core/parameters
```txt
admin_timeout
apst_primary_latency_tol_us
apst_primary_timeout_ms
apst_secondary_latency_tol_us
apst_secondary_timeout_ms
default_ps_max_latency_us
disable_pi_offsets
force_apst
io_timeout
iopolicy
max_retries
multipath
shutdown_timeout
```

/sys/module/nvme/parameters
```txt
 io_queue_depth
 max_host_mem_size_mb
 noacpi
 poll_queues
 sgl_threshold
 use_cmb_sqes
 use_threaded_interrupts
 write_queues
```

为什么 nvme.poll_queues=4 对于 iouring 的 poll 是必须的。

## kernel nvme 的代码分析

- `nvme_create_io_queues` => `nvme_alloc_queue` => `dma_alloc_coherent`

简单分析下 host 文件夹中的内容:

host 下的文件，主要是
1. core.c : 重点
1. 四种传输方法: pci fc tcp rdma
3. 几个高级话题
  1. multipath.c
  2. zns.c : zone block device
  3. pr.c : https://www.kernel.org/doc/Documentation/block/pr.rst

target 下的文件:
需要打开 CONFIG_NVME_TARGET，猜测是当使用 fc tcp rdma 的时候，
在 target 端需要的驱动。


所以，总结下，掌握如下两个文件的差不多可以了
- ./host/core.c  4770
- ./host/pci.c   3529


## 关键数据结构
- `nvme_queue` : 描述 nvme 硬件队列
- `struct nvme_ctrl` : 描述一个 hardware controller 的

### host/pci.c

关键路径:
```txt
#0  nvme_setup_rw (op=nvme_cmd_write, cmnd=0xffff88810ebc6320, req=0xffff88810ebc6200, ns=0xffff88810dd6e800) at drivers/nvme/host/core.c:898
#1  nvme_setup_cmd (ns=0xffff88810dd6e800, req=req@entry=0xffff88810ebc6200) at drivers/nvme/host/core.c:1003
#2  0xffffffff81b0cfc8 in nvme_prep_rq (req=0xffff88810ebc6200, dev=0xffff888106400000) at drivers/nvme/host/pci.c:846
#3  nvme_queue_rq (hctx=<optimized out>, bd=0xffffc9000284baf8) at drivers/nvme/host/pci.c:893
#4  0xffffffff817e7cc8 in __blk_mq_issue_directly (hctx=0xffff88810dd6e800, hctx@entry=0xffff88810df45800, rq=rq@entry=0xffff88810ebc6200, last=last@entry=true) at block/blk-mq.c:2590
#5  0xffffffff817ebc27 in blk_mq_try_issue_directly (hctx=hctx@entry=0xffff88810df45800, rq=rq@entry=0xffff88810ebc6200) at block/blk-mq.c:2649
#6  0xffffffff817eca2a in blk_mq_submit_bio (bio=<optimized out>) at block/blk-mq.c:3022
#7  0xffffffff817dc68d in __submit_bio_noacct_mq (bio=0xffff88810a284900) at block/blk-core.c:678
#8  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:707
#9  0xffffffff817dc8a2 in submit_bio_noacct (bio=<optimized out>) at block/blk-core.c:801
#10 0xffffffff817d3a6f in __blkdev_direct_IO_async (nr_pages=<optimized out>, iter=0xffffc9000284bd08, iocb=0xffff888107f32480) at block/fops.c:361
#11 blkdev_direct_IO (iocb=iocb@entry=0xffff888107f32480, iter=iter@entry=0xffffc9000284bd08) at block/fops.c:377
#12 0xffffffff817d3fbb in blkdev_direct_IO (iter=0xffffc9000284bd08, iocb=0xffff888107f32480) at ./include/linux/uio.h:298
#13 blkdev_direct_write (from=0xffffc9000284bd08, iocb=0xffff888107f32480) at block/fops.c:621
#14 blkdev_write_iter (iocb=0xffff888107f32480, from=0xffffc9000284bd08) at block/fops.c:679
#15 0xffffffff814c1513 in call_write_iter (iter=0xffff88810ebc6200, kio=0xffff888107f32480, file=0xffff88811182d800) at ./include/linux/fs.h:1956
#16 aio_write (req=req@entry=0xffff888107f32480, iocb=iocb@entry=0xffffc9000284be58, vectored=vectored@entry=false, compat=<optimized out>) at fs/aio.c:1582
#17 0xffffffff814c47c7 in __io_submit_one (ctx=0xffff8881080c0000, compat=false, req=0xffff888107f32480, user_iocb=0x5581493602c8, iocb=0xffffc9000284be58) at fs/aio.c:1954
#18 io_submit_one (ctx=ctx@entry=0xffff8881080c0000, user_iocb=0x5581493602c8, compat=compat@entry=false) at fs/aio.c:2001
#19 0xffffffff814c4e1d in __do_sys_io_submit (iocbpp=0x55814935ac90, nr=1, ctx_id=<optimized out>) at fs/aio.c:2060
```

- nvme_queue_rq
  - nvme_submit_cmds
    - nvme_prep_rq : 将从 block layer 中到来的命令组装为
    - nvme_sq_copy_cmd
    - nvme_write_sq_db : 直接写到 bar 空间中，发起命令


定义了和 blk layer 沟通的核心: `nvme_mq_ops`

中断的管理: nvme_irq nvme_setup_irqs

实现了 io queue 的管理 : nvme_create_queue nvme_free_queue

设备的探测和管理 : nvme_probe nvme_shutdown nvme_resume

### host/core.c
关键结构体:


host/core.c 很多都是和 nvme_ctrl 的操作有关，例如当 pci 的结果如下:
```txt
#0  nvme_start_ctrl (ctrl=ctrl@entry=0xffff888104ef81f0) at drivers/nvme/host/core.c:4345
#1  0xffffffff81b0e35a in nvme_probe (pdev=0xffff8881040a5000, id=<optimized out>) at drivers/nvme/host/pci.c:3055
#2  0xffffffff818ab5ff in local_pci_probe (_ddi=_ddi@entry=0xffffc90000853d98) at drivers/pci/pci-driver.c:324
#3  0xffffffff818ad051 in pci_call_probe (id=<optimized out>, dev=0xffff8881040a5000, drv=<optimized out>) at drivers/pci/pci-driver.c:392
#4  __pci_device_probe (pci_dev=0xffff8881040a5000, drv=<optimized out>) at drivers/pci/pci-driver.c:417
#5  pci_device_probe (dev=0xffff8881040a50b8) at drivers/pci/pci-driver.c:460
```

各种传输协议都是定义自己的 controller :
```c
static const struct nvme_ctrl_ops nvme_pci_ctrl_ops = {
	.name			= "pcie",
	.module			= THIS_MODULE,
	.flags			= NVME_F_METADATA_SUPPORTED,
	.dev_attr_groups	= nvme_pci_dev_attr_groups,
	.reg_read32		= nvme_pci_reg_read32,
	.reg_write32		= nvme_pci_reg_write32,
	.reg_read64		= nvme_pci_reg_read64,
	.free_ctrl		= nvme_pci_free_ctrl,
	.submit_async_event	= nvme_pci_submit_async_event,
	.get_address		= nvme_pci_get_address,
	.print_device_info	= nvme_pci_print_device_info,
	.supports_pci_p2pdma	= nvme_pci_supports_pci_p2pdma,
};
```


更多都是一些公共函数，提供给 tcp fc 之类的使用


## nvme 的错误处理机制

理解这四个基本的参数:
- admin_timeout : admin cmd 的超时，看 nvme_init_request
- nvme_io_timeout
- shutdown_timeout : 关机时间 nvme_disable_ctrl
- nvme_max_retries

nvme_complete_rq
  - nvme_decide_disposition : 在这里重试 5 次
  - nvme_end_req : 如果当时超时，这里会向上抛出错误的


## 看看 sysfs 有什么好玩的

| 内核数据结构            | 对应的文件夹                                                              |
| ----------------------- | ------------------------------------------------------------------------- |
| nvme_subsys_attrs_group | /sys/devices/virtual/nvme-subsystem/nvme-subsys0/firmware_rev             |
| nvme_dev_attrs_group    | /sys/devices/pci0000:00/0000:00:01.2/0000:02:00.0/nvme/nvme0/cntrltype    |
| nvme_ns_id_attr_group   | /sys/devices/pci0000:00/0000:00:02.1/0000:03:00.0/nvme/nvme1/nvme1n1/wwid |

## qemu 部分

- qemu 下存在两个 nvme.c

  - hw/block/nvme.c 是用于模拟 nvme 设备的
  - block/nvme.c 模拟 nvme 设备直通的场景的

- nvme_init
  - qemu_vfio_open_pci
    - qemu_vfio_init_pci

https://github.com/manishrma/nvme-qemu : 这种 qemu 中使用 qemu 的一些高级技术

## 观察到 fanxiang 的盘忽然卡住了

```txt
[ 6965.905789] nvme nvme0: I/O 911 (I/O Cmd) QID 4 timeout, aborting
[ 6980.241782] nvme nvme0: I/O 213 (I/O Cmd) QID 7 timeout, aborting
[ 6987.192766] nvme nvme0: Abort status: 0x0
[ 6987.193437] nvme nvme0: Abort status: 0x0
```

## NVMe-oF

- NVMe-over-Fabrics Performance Characterization and the Path to Low-Overhead Flash Disaggregation
  - https://dl.acm.org/doi/pdf/10.1145/3078468.3078483

## multipath

- https://spdk.io/doc/nvme_multipath.html : 这个讲的很深入
- https://www.ibm.com/docs/en/flashsystem-7x00/8.4.x?topic=nhtrlos-multipath-configuration-fc-nvme-hosts-1


```diff
History:        #0
Commit:         32acab3181c7053c775ca128c3a5c6ce50197d7f
Author:         Christoph Hellwig <hch@lst.de>
Committer:      Jens Axboe <axboe@kernel.dk>
Author Date:    Thu 02 Nov 2017 07:59:30 PM CST
Committer Date: Sat 11 Nov 2017 10:53:25 AM CST

nvme: implement multipath access to nvme subsystems

This patch adds native multipath support to the nvme driver.  For each
namespace we create only single block device node, which can be used
to access that namespace through any of the controllers that refer to it.
The gendisk for each controllers path to the name space still exists
inside the kernel, but is hidden from userspace.  The character device
nodes are still available on a per-controller basis.  A new link from
the sysfs directory for the subsystem allows to find all controllers
for a given subsystem.

Currently we will always send I/O to the first available path, this will
be changed once the NVMe Asynchronous Namespace Access (ANA) TP is
ratified and implemented, at which point we will look at the ANA state
for each namespace.  Another possibility that was prototyped is to
use the path that is closes to the submitting NUMA code, which will be
mostly interesting for PCI, but might also be useful for RDMA or FC
transports in the future.  There is not plan to implement round robin
or I/O service time path selectors, as those are not scalable with
the performance rates provided by NVMe.

The multipath device will go away once all paths to it disappear,
any delay to keep it alive needs to be implemented at the controller
level.

Signed-off-by: Christoph Hellwig <hch@lst.de>
Reviewed-by: Keith Busch <keith.busch@intel.com>
Reviewed-by: Martin K. Petersen <martin.petersen@oracle.com>
Reviewed-by: Hannes Reinecke <hare@suse.com>
Signed-off-by: Jens Axboe <axboe@kernel.dk>
```

## nvme 的名称是如何确定


```txt
#0  nvme_alloc_ns (info=0xffffc9000038fd48, ctrl=0xffff888101ef81f0) at drivers/nvme/host/core.c:3578
#1  nvme_scan_ns (ctrl=ctrl@entry=0xffff888101ef81f0, nsid=nsid@entry=1) at drivers/nvme/host/core.c:3785
#2  0xffffffff81b074bb in nvme_scan_ns_list (ctrl=<optimized out>) at drivers/nvme/host/core.c:3838
#3  nvme_scan_work (work=<optimized out>) at drivers/nvme/host/core.c:3929
#4  0xffffffff81168618 in process_one_work (worker=worker@entry=0xffff888101daa000, work=0xffff888101ef8b00) at kernel/workqueue.c:2630
#5  0xffffffff81168ad5 in process_scheduled_works (worker=<optimized out>) at kernel/workqueue.c:2703
#6  worker_thread (__worker=0xffff888101daa000) at kernel/workqueue.c:2784
#7  0xffffffff81172ac3 in kthread (_create=0xffff888101dab000) at kernel/kthread.c:388
#8  0xffffffff810e5991 in ret_from_fork (prev=<optimized out>, regs=0xffffc9000038ff58, fn=0xffffffff811729e0 <kthread>, fn_arg=0xffff888101dab000)
    at arch/x86/kernel/process.c:147
#9  0xffffffff8100259b in ret_from_fork_asm () at arch/x86/entry/entry_64.S:242
```
nvme_alloc_ns 中最后展示到 gendisk::disk_name


nvme_init_subsystem 中

```c
		subsys->instance = ctrl->instance;
```

nvme_init_ctrl
```c
	ret = ida_alloc(&nvme_instance_ida, GFP_KERNEL);
	if (ret < 0)
		goto out;
	ctrl->instance = ret;
```

```txt
#0  nvme_init_ctrl (ctrl=ctrl@entry=0xffff888102dd81f0, dev=dev@entry=0xffff888101e6e0b8,
    ops=ops@entry=0xffffffff826fa020 <nvme_pci_ctrl_ops>, quirks=quirks@entry=262144) at drivers/nvme/host/core.c:4431
#1  0xffffffff81b1348e in nvme_pci_alloc_dev (id=<optimized out>, pdev=<optimized out>) at drivers/nvme/host/pci.c:2944
#2  nvme_probe (pdev=0xffff888101e6e000, id=<optimized out>) at drivers/nvme/host/pci.c:2984
#3  0xffffffff818b2f7f in local_pci_probe (_ddi=_ddi@entry=0xffffc90000567d98) at drivers/pci/pci-driver.c:324
#4  0xffffffff818b49f1 in pci_call_probe (id=<optimized out>, dev=0xffff888101e6e000, drv=<optimized out>) at drivers/pci/pci-driver.c:392
#5  __pci_device_probe (pci_dev=0xffff888101e6e000, drv=<optimized out>) at drivers/pci/pci-driver.c:417
#6  pci_device_probe (dev=0xffff888101e6e0b8) at drivers/pci/pci-driver.c:460
#7  0xffffffff819f314c in call_driver_probe (drv=0xffffffff8306a958 <nvme_driver+120>, dev=0xffff888101e6e0b8) at drivers/base/dd.c:579
#8  really_probe (dev=dev@entry=0xffff888101e6e0b8, drv=drv@entry=0xffffffff8306a958 <nvme_driver+120>) at drivers/base/dd.c:658
#9  0xffffffff819f33d3 in __driver_probe_device (drv=0xffffffff8306a958 <nvme_driver+120>, dev=dev@entry=0xffff888101e6e0b8)
    at drivers/base/dd.c:800
#10 0xffffffff819f34af in driver_probe_device (drv=<optimized out>, dev=dev@entry=0xffff888101e6e0b8) at drivers/base/dd.c:830
#11 0xffffffff819f3883 in __driver_attach_async_helper (_dev=0xffff888101e6e0b8, cookie=<optimized out>) at drivers/base/dd.c:1148
#12 0xffffffff8117d031 in async_run_entry_fn (work=0xffff888107594c20) at kernel/async.c:127
#13 0xffffffff81168618 in process_one_work (worker=worker@entry=0xffff888101d1c0c0, work=0xffff888107594c20) at kernel/workqueue.c:2630
#14 0xffffffff81168ad5 in process_scheduled_works (worker=<optimized out>) at kernel/workqueue.c:2703
#15 worker_thread (__worker=0xffff888101d1c0c0) at kernel/workqueue.c:2784
#16 0xffffffff81172ac3 in kthread (_create=0xffff888101cb96c0) at kernel/kthread.c:388
#17 0xffffffff810e5991 in ret_from_fork (prev=<optimized out>, regs=0xffffc90000567f58, fn=0xffffffff811729e0 <kthread>,
    fn_arg=0xffff888101cb96c0) at arch/x86/kernel/process.c:147
#18 0xffffffff8100259b in ret_from_fork_asm () at arch/x86/entry/entry_64.S:242
#19 0x0000000000000000 in ?? ()
```

感觉 /dev/nvme0n1 的名称并不可靠，重启之后很有可能类似 /dev/sda 之类的一样，会发生修改，但是没有完全的证据。



## 检查 nvme 的磨损程度

sudo smartctl -t short -a /dev/nvme2n1

本来以为这个命令同样可以查询 ssd，但是发现并不可以:
sudo smartctl -t short -a /dev/sda

## 记录

sudo smartctl -t short -a /dev/nvme2n1 | grep "Data Units Written"

```txt
Thu Jan 11 02:54:11 PM CST 2024
Data Units Written:                 220,749,924 [113 TB]
```


## multipath 基本

```txt
lrwxrwxrwx.  1 root root 0 Jan 11 16:34 nvme1c1n1 -> ../../devices/pci0000:00/0000:00:09.0/nvme/nvme1/nvme1c1n1
lrwxrwxrwx.  1 root root 0 Jan 11 16:34 nvme1c2n1 -> ../../devices/pci0000:00/0000:00:0a.0/nvme/nvme2/nvme1c2n1
lrwxrwxrwx.  1 root root 0 Jan 11 16:34 nvme1n1 -> ../../devices/virtual/nvme-subsystem/nvme-subsys1/nvme1n1
```

```txt
lrwxrwxrwx  1 root root 0 Jan 11 16:07 nvme0c0n1 -> ../devices/pci0000:30/0000:30:02.0/0000:31:00.0/nvme/nvme0/nvme0c0n1
lrwxrwxrwx  1 root root 0 Jan 11 16:07 nvme0n1 -> ../devices/virtual/nvme-subsystem/nvme-subsys0/nvme0n1
lrwxrwxrwx  1 root root 0 Jan 11 16:13 nvme1c4n2 -> ../devices/pci0000:30/0000:30:03.0/0000:32:00.0/nvme/nvme4/nvme1c4n2
lrwxrwxrwx  1 root root 0 Jan 11 16:07 nvme1n1 -> ../devices/virtual/nvme-subsystem/nvme-subsys1/nvme1n1
lrwxrwxrwx  1 root root 0 Jan 11 16:13 nvme1n2 -> ../devices/virtual/nvme-subsystem/nvme-subsys1/nvme1n2
lrwxrwxrwx  1 root root 0 Jan 11 16:07 nvme2c2n1 -> ../devices/pci0000:64/0000:64:02.0/0000:65:00.0/nvme/nvme2/nvme2c2n1
lrwxrwxrwx  1 root root 0 Jan 11 16:07 nvme2n1 -> ../devices/virtual/nvme-subsystem/nvme-subsys2/nvme2n1
lrwxrwxrwx  1 root root 0 Jan 11 16:07 nvme3c3n1 -> ../devices/pci0000:64/0000:64:03.0/0000:66:00.0/nvme/nvme3/nvme3c3n1
lrwxrwxrwx  1 root root 0 Jan 11 16:07 nvme3n1 -> ../devices/virtual/nvme-subsystem/nvme-subsys3/nvme3n1
```

可以修改 io 指向哪一个盘:
```txt
# cat /sys/class/nvme-subsystem/nvme-subsys0/iopolicy

round-robin
```

```txt
@[
    nvme_ns_head_submit_bio+5
    __submit_bio+132
    submit_bio_noacct_nocheck+345
    blkdev_direct_IO.part.0+575
    blkdev_write_iter+427
    io_write+290
    io_issue_sqe+96
    io_submit_sqes+507
    __do_sys_io_uring_enter+1471
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1365299
```

- nvme_ns_head_submit_bio
  - nvme_find_path
  - submit_bio_noacct

- [ ] multipath 下，为什么两个 15G 的盘，组成的盘还是 15G 啊

分析下这个怎么使用吧 : /home/martins3/core/linux/drivers/md/md-multipath.c

## 文档就算了
- https://news.ycombinator.com/item?id=40505167
  - https://github.com/bootreer/vroom : 代码可以看看 nvme 到底有多简单
  - 其实 redox-rs 的也很简单: https://gitlab.redox-os.org/redox-os/drivers/-/tree/master/storage/nvmed/src


## nvme 仿真器

qemu 可以模拟任意的 nvme 吗? 例如三星的某款
恐怕需要用 nvme-cli 配合理解一下

https://jianyue.tech/posts/femu/

https://github.com/MoatLab/FEMU

没理解错的，这个是 nvme simulator 吧

## 最基本的东西了
https://mp.weixin.qq.com/s/s8YeobhEWKPG75WoA2Dk5w

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
