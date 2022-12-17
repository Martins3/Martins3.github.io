# cgroup

| cgroup interface | source     code |
|------------------|-----------------|
| io.latency       | blk-iolatency.c |
| io.max           | blk-throttle.c  |
| io.prio.class    | blk-ioprio.c    |
| io.stat          | blk-stat.c      |
| io.weight        | blk-iocost.c    |

## 基本使用
为什么这样执行之后:
- duck echo "8:16  wiops=100000" > io.max
- cgexec -g io:duck dd if=/dev/zero of=/dev/sdb bs=1M count=1000

结果 kernel 卡到了这里啊:
```txt
➜  cat /proc/1624/stack
[<0>] blkdev_get_by_dev.part.0+0xf7/0x2f0
[<0>] blkdev_open+0x4b/0x90
[<0>] do_dentry_open+0x157/0x430
[<0>] path_openat+0xb6b/0x1030
[<0>] do_filp_open+0xb1/0x160
[<0>] do_sys_openat2+0x95/0x160
[<0>] __x64_sys_openat+0x52/0xa0
[<0>] do_syscall_64+0x3c/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x72/0xdc
```

具体的分析应该是很有趣的，但是没时间了。

## iolatency

- iolatency_set_min_lat_nsec

## throttle

```txt
#0  blk_cgroup_bio_start (bio=bio@entry=0xffff8881374a7800) at block/blk-cgroup.c:1992
#1  0xffffffff816c0aad in submit_bio_noacct (bio=0xffff8881374a7800) at block/blk-core.c:794
#2  0xffffffff81412289 in submit_bh_wbc (opf=<optimized out>, bh=<optimized out>, wbc=<optimized out>) at fs/buffer.c:2702
#3  0xffffffff81412b8a in submit_bh (bh=0xffff88809e886000, opf=0) at fs/buffer.c:2707
#4  __bh_read (bh=bh@entry=0xffff88809e886000, op_flags=op_flags@entry=0, wait=wait@entry=false) at fs/buffer.c:2972
#5  0xffffffff81415c68 in bh_read_nowait (op_flags=0, bh=0xffff88809e886000) at ./include/linux/buffer_head.h:427
#6  __block_write_begin_int (folio=<optimized out>, pos=pos@entry=624168960, len=len@entry=3, get_block=get_block@entry=0xffffffff816b7f10 <blkdev_get_block>, iomap=iomap@entry=0x0 <fixed_percpu_data>) at fs/buffer.c:2021
#7  0xffffffff81415d41 in __block_write_begin (get_block=0xffffffff816b7f10 <blkdev_get_block>, len=3, pos=624168960, page=0xffffea0002715f80) at fs/buffer.c:2041
#8  block_write_begin (mapping=<optimized out>, pos=624168960, len=3, pagep=0xffffc900402efd88, get_block=0xffffffff816b7f10 <blkdev_get_block>) at fs/buffer.c:2102
#9  0xffffffff812de041 in generic_perform_write (iocb=<optimized out>, i=0xffffc900402efe78) at mm/filemap.c:3772
#10 0xffffffff812e317f in __generic_file_write_iter (iocb=iocb@entry=0xffffc900402efea0, from=from@entry=0xffffc900402efe78) at mm/filemap.c:3900
#11 0xffffffff816b8483 in blkdev_write_iter (iocb=0xffffc900402efea0, from=0xffffc900402efe78) at block/fops.c:534
#12 0xffffffff813c60bd in call_write_iter (iter=0x1000 <cpu_debug_store>, kio=0xffff8881374a7800, file=0xffff888023f85f00) at ./include/linux/fs.h:2186
#13 new_sync_write (ppos=0xffffc900402eff08, len=3, buf=0x40200d "str", filp=0xffff888023f85f00) at fs/read_write.c:491
#14 vfs_write (file=file@entry=0xffff888023f85f00, buf=buf@entry=0x40200d "str", count=count@entry=3, pos=pos@entry=0xffffc900402eff08) at fs/read_write.c:584
#15 0xffffffff813c650e in ksys_write (fd=<optimized out>, buf=0x40200d "str", count=3) at fs/read_write.c:637
#16 0xffffffff8217cbbc in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900402eff58) at arch/x86/entry/common.c:50
#17 do_syscall_64 (regs=0xffffc900402eff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#18 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## io.stat

```txt
#0  blkcg_iostat_update (blkg=blkg@entry=0xffff8880201d3400, cur=cur@entry=0xffffc900402efaa0, last=0xffffe8ff01c60cb8) at block/blk-cgroup.c:847
#1  0xffffffff816dff61 in blkcg_rstat_flush (css=<optimized out>, cpu=<optimized out>) at block/blk-cgroup.c:894
#2  0xffffffff81205f2f in cgroup_rstat_flush_locked (cgrp=cgrp@entry=0xffffffff82d536f0 <cgrp_dfl_root+16>, may_sleep=may_sleep@entry=false) at kernel/cgroup/rstat.c:205
#3  0xffffffff81206073 in cgroup_rstat_flush_irqsafe (cgrp=0xffffffff82d536f0 <cgrp_dfl_root+16>) at kernel/cgroup/rstat.c:254
#4  0xffffffff81399696 in __mem_cgroup_flush_stats () at mm/memcontrol.c:636
#5  0xffffffff813a2da8 in mem_cgroup_flush_stats () at mm/memcontrol.c:644
#6  mem_cgroup_wb_stats (wb=wb@entry=0xffff888138329000, pfilepages=pfilepages@entry=0xffffc900402efc10, pheadroom=pheadroom@entry=0xffffc900402efc18, pdirty=pdirty@entry=0xffffc900402efcb0, pwriteback=pwriteback@entry=0xffffc900402efc20) at mm/memcontrol.c:4583
#7  0xffffffff812ea500 in balance_dirty_pages (wb=wb@entry=0xffff888138329000, pages_dirtied=32, flags=flags@entry=0) at mm/page-writeback.c:1724
#8  0xffffffff812eb2ed in balance_dirty_pages_ratelimited_flags (mapping=mapping@entry=0xffff888140806508, flags=flags@entry=0) at ./arch/x86/include/asm/current.h:41
#9  0xffffffff812eb40b in balance_dirty_pages_ratelimited (mapping=mapping@entry=0xffff888140806508) at mm/page-writeback.c:2064
#10 0xffffffff812de0c9 in generic_perform_write (iocb=<optimized out>, i=0xffffc900402efe78) at mm/filemap.c:3806
#11 0xffffffff812e317f in __generic_file_write_iter (iocb=iocb@entry=0xffffc900402efea0, from=from@entry=0xffffc900402efe78) at mm/filemap.c:3900
#12 0xffffffff816b8483 in blkdev_write_iter (iocb=0xffffc900402efea0, from=0xffffc900402efe78) at block/fops.c:534
#13 0xffffffff813c60bd in call_write_iter (iter=0xffffc900402efaa0, kio=0xffff8880201d3400, file=0xffff888023f85f00) at ./include/linux/fs.h:2186
#14 new_sync_write (ppos=0xffffc900402eff08, len=3, buf=0x40200d "str", filp=0xffff888023f85f00) at fs/read_write.c:491
#15 vfs_write (file=file@entry=0xffff888023f85f00, buf=buf@entry=0x40200d "str", count=count@entry=3, pos=pos@entry=0xffffc900402eff08) at fs/read_write.c:584
#16 0xffffffff813c650e in ksys_write (fd=<optimized out>, buf=0x40200d "str", count=3) at fs/read_write.c:637
#17 0xffffffff8217cbbc in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900402eff58) at arch/x86/entry/common.c:50
#18 do_syscall_64 (regs=0xffffc900402eff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#19 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
