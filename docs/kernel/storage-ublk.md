# ublk

```txt
#0  ublk_ch_mmap (filp=0xffff888105420700, vma=0xffff8880067dbd10) at drivers/block/ublk_drv.c:923
#1  0xffffffff812e6b58 in call_mmap (vma=0xffff8880067dbd10, file=0xffff888105420700) at ./include/linux/fs.h:2204
#2  mmap_region (file=file@entry=0xffff888105420700, addr=addr@entry=140585578082304, len=len@entry=4096, vm_flags=vm_flags@entry=249, pgoff=<optimized out>, uf=uf@entry=0xffffc90002727eb0) at mm/mmap.c:2625
#3  0xffffffff812e754f in do_mmap (file=file@entry=0xffff888105420700, addr=140585578082304, addr@entry=0, len=len@entry=4096, prot=<optimized out>, prot@entry=1, flags=flags@entry=32769, pgoff=<optimized out>, pgoff@entry=0, populate=0xffffc90002727ea8, uf=0xffffc90002727eb0) at mm/mmap.c:1412
#4  0xffffffff812b9f35 in vm_mmap_pgoff (file=file@entry=0xffff888105420700, addr=addr@entry=0, len=len@entry=4096, prot=prot@entry=1, flag=flag@entry=32769, pgoff=pgoff@entry=0) at mm/util.c:520
#5  0xffffffff812e4442 in ksys_mmap_pgoff (addr=0, len=4096, prot=1, flags=32769, fd=<optimized out>, pgoff=0) at mm/mmap.c:1458
#6  0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90002727f58) at arch/x86/entry/common.c:50
#7  do_syscall_64 (regs=0xffffc90002727f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#8  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

```txt
#0  ublk_ch_uring_cmd (cmd=0xffff8881015d0700, issue_flags=2147483653) at drivers/block/ublk_drv.c:1198
#1  0xffffffff8165dcc6 in io_uring_cmd (req=0xffff8881015d0700, issue_flags=2147483653) at io_uring/uring_cmd.c:131
#2  0xffffffff8165a4b6 in io_issue_sqe (req=req@entry=0xffff8881015d0700, issue_flags=issue_flags@entry=2147483649) at io_uring/io_uring.c:1743
#3  0xffffffff8165ae62 in io_queue_sqe (req=0xffff8881015d0700) at io_uring/io_uring.c:1916
#4  io_submit_sqe (sqe=0xffff888105e70000, req=0xffff8881015d0700, ctx=0xffff888101525000) at io_uring/io_uring.c:2174
#5  io_submit_sqes (ctx=ctx@entry=0xffff888101525000, nr=nr@entry=128) at io_uring/io_uring.c:2285
#6  0xffffffff8165b755 in __do_sys_io_uring_enter (fd=<optimized out>, to_submit=128, min_complete=1, flags=25, argp=0x7fdca11ffcf0, argsz=24) at io_uring/io_uring.c:3220
#7  0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90002727f58) at arch/x86/entry/common.c:50
#8  do_syscall_64 (regs=0xffffc90002727f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#9  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

```txt
#0  ublk_queue_rq (hctx=0xffff8880055f1600, bd=0xffffc90001797d60) at drivers/block/ublk_drv.c:843
#1  0xffffffff81635c1c in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff8880055f1600, list=list@entry=0xffffc90001797de0, nr_budgets=0, nr_budgets@entry=1) at block/blk-mq.c:1992
#2  0xffffffff8163ba34 in __blk_mq_do_dispatch_sched (hctx=0xffff8880055f1600) at block/blk-mq-sched.c:173
#3  blk_mq_do_dispatch_sched (hctx=hctx@entry=0xffff8880055f1600) at block/blk-mq-sched.c:187
#4  0xffffffff8163bd48 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff8880055f1600) at block/blk-mq-sched.c:313
#5  0xffffffff8163bde0 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff8880055f1600) at block/blk-mq-sched.c:339
#6  0xffffffff81632860 in __blk_mq_run_hw_queue (hctx=0xffff8880055f1600) at block/blk-mq.c:2110
#7  0xffffffff8112bd54 in process_one_work (worker=worker@entry=0xffff8881007b10c0, work=0xffff8880055f1640) at kernel/workqueue.c:2289
#8  0xffffffff8112bf68 in worker_thread (__worker=0xffff8881007b10c0) at kernel/workqueue.c:2436
#9  0xffffffff81133870 in kthread (_create=0xffff8881007af340) at kernel/kthread.c:376
#10 0xffffffff81001a6f in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

```txt
#0  ublk_rq_task_work_fn (work=0xffff888025caa110) at drivers/block/ublk_drv.c:801
#1  0xffffffff81130cd1 in task_work_run () at kernel/task_work.c:179
#2  0xffffffff8165b338 in io_run_task_work () at io_uring/io_uring.h:250
#3  io_run_task_work () at io_uring/io_uring.h:239
#4  io_run_task_work_ctx (ctx=<optimized out>) at io_uring/io_uring.h:272
#5  io_run_task_work_sig (ctx=<optimized out>) at io_uring/io_uring.c:2351
#6  0xffffffff8165bb0d in io_cqring_wait_schedule (timeout=<optimized out>, iowq=0xffffc90002727ec8, ctx=0xffff888101525000) at io_uring/io_uring.c:2367
#7  io_cqring_wait (uts=0x7fdca11ffd80, sigsz=<optimized out>, sig=0x0 <fixed_percpu_data>, min_events=1, ctx=0xffff888101525000) at io_uring/io_uring.c:2449
#8  __do_sys_io_uring_enter (fd=<optimized out>, to_submit=<optimized out>, min_complete=1, flags=<optimized out>, argp=<optimized out>, argsz=<optimized out>) at io_uring/io_uring.c:3265
#9  0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90002727f58) at arch/x86/entry/common.c:50
#10 do_syscall_64 (regs=0xffffc90002727f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#11 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

```txt
#0  ublk_copy_user_pages (data=data@entry=0xffffc90002727d88, to_vm=to_vm@entry=false) at drivers/block/ublk_drv.c:432
#1  0xffffffff819aec5c in ublk_unmap_io (io=0xffff888105df9640, req=0xffff888025caa000, ubq=0xffff888105df9000) at drivers/block/ublk_drv.c:512
#2  ublk_complete_rq (req=0xffff888025caa000) at drivers/block/ublk_drv.c:621
#3  ublk_commit_completion (ub_cmd=<optimized out>, ub=<optimized out>) at drivers/block/ublk_drv.c:974
#4  ublk_ch_uring_cmd (cmd=<optimized out>, issue_flags=<optimized out>) at drivers/block/ublk_drv.c:1274
#5  0xffffffff8165dcc6 in io_uring_cmd (req=0xffff888103289e00, issue_flags=2147483653) at io_uring/uring_cmd.c:131
#6  0xffffffff8165a4b6 in io_issue_sqe (req=req@entry=0xffff888103289e00, issue_flags=issue_flags@entry=2147483649) at io_uring/io_uring.c:1743
#7  0xffffffff8165ae62 in io_queue_sqe (req=0xffff888103289e00) at io_uring/io_uring.c:1916
#8  io_submit_sqe (sqe=0xffff888105e74080, req=0xffff888103289e00, ctx=0xffff888101525000) at io_uring/io_uring.c:2174
#9  io_submit_sqes (ctx=ctx@entry=0xffff888101525000, nr=nr@entry=1) at io_uring/io_uring.c:2285
#10 0xffffffff8165b755 in __do_sys_io_uring_enter (fd=<optimized out>, to_submit=1, min_complete=1, flags=25, argp=0x7fdca11ffcf0, argsz=24) at io_uring/io_uring.c:3220
#11 0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90002727f58) at arch/x86/entry/common.c:50
#12 do_syscall_64 (regs=0xffffc90002727f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#13 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

```txt
#0  ublk_rq_task_work_fn (work=0xffff888025d75110) at drivers/block/ublk_drv.c:801
#1  0xffffffff81130cd1 in task_work_run () at kernel/task_work.c:179
#2  0xffffffff8165b338 in io_run_task_work () at io_uring/io_uring.h:250
#3  io_run_task_work () at io_uring/io_uring.h:239
#4  io_run_task_work_ctx (ctx=<optimized out>) at io_uring/io_uring.h:272
#5  io_run_task_work_sig (ctx=<optimized out>) at io_uring/io_uring.c:2351
#6  0xffffffff8165bb0d in io_cqring_wait_schedule (timeout=<optimized out>, iowq=0xffffc90001d3fec8, ctx=0xffff888107e89800) at io_uring/io_uring.c:2367
#7  io_cqring_wait (uts=0x0 <fixed_percpu_data>, sigsz=<optimized out>, sig=0x0 <fixed_percpu_data>, min_events=1, ctx=0xffff888107e89800) at io_uring/io_uring.c:2449
#8  __do_sys_io_uring_enter (fd=<optimized out>, to_submit=<optimized out>, min_complete=1, flags=<optimized out>, argp=<optimized out>, argsz=<optimized out>) at io_uring/io_uring.c:3265
#9  0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001d3ff58) at arch/x86/entry/common.c:50
#10 do_syscall_64 (regs=0xffffc90001d3ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#11 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## 运行时间
```txt
#0  arch_atomic_try_cmpxchg (new=1, old=<synthetic pointer>, v=0xffff888005e81530) at ./arch/x86/include/asm/atomic.h:202
#1  atomic_try_cmpxchg_acquire (new=1, old=<synthetic pointer>, v=0xffff888005e81530) at ./include/linux/atomic/atomic-instrumented.h:543
#2  queued_spin_lock (lock=0xffff888005e81530) at ./include/asm-generic/qspinlock.h:111
#3  do_raw_spin_lock (lock=0xffff888005e81530) at ./include/linux/spinlock.h:186
#4  __raw_spin_lock_irqsave (lock=0xffff888005e81530) at ./include/linux/spinlock_api_smp.h:111
#5  _raw_spin_lock_irqsave (lock=lock@entry=0xffff888005e81530) at kernel/locking/spinlock.c:162
#6  0xffffffff816ac6d6 in percpu_counter_add_batch (fbc=0xffff888005e81530, amount=amount@entry=1, batch=<optimized out>) at lib/percpu_counter.c:90
#7  0xffffffff8141ba2a in percpu_counter_add (amount=1, fbc=<optimized out>) at ./include/linux/percpu_counter.h:59
#8  percpu_counter_inc (fbc=<optimized out>) at ./include/linux/percpu_counter.h:209
#9  ext4_es_lookup_extent (inode=inode@entry=0xffff888005bf61a0, lblk=185472, next_lblk=next_lblk@entry=0x0 <fixed_percpu_data>, es=es@entry=0xffffc90001c4fa50) at fs/ext4/extents_status.c:968
#10 0xffffffff8142b1da in ext4_map_blocks (handle=handle@entry=0x0 <fixed_percpu_data>, inode=inode@entry=0xffff888005bf61a0, map=map@entry=0xffffc90001c4fae0, flags=flags@entry=0) at fs/ext4/inode.c:528
#11 0xffffffff8142bec0 in ext4_iomap_begin (inode=0xffff888005bf61a0, offset=759693312, length=4096, flags=48, iomap=0xffffc90001c4fbe0, srcmap=<optimized out>) at fs/ext4/inode.c:3466
#12 0xffffffff813dcb2f in iomap_iter (iter=iter@entry=0xffffc90001c4fbb8, ops=ops@entry=0xffffffff8244dd20 <ext4_iomap_ops>) at fs/iomap/iter.c:74
#13 0xffffffff813e0947 in __iomap_dio_rw (iocb=iocb@entry=0xffff88800c262900, iter=iter@entry=0xffffc90001c4fd08, ops=0xffffffff8244dd20 <ext4_iomap_ops>, dops=dops@entry=0x0 <fixed_percpu_data>, dio_flags=dio_flags@entry=0, private=private@entry=0x0 <fixed_percpu_data>, done_before=0) at fs/iomap/direct-io.c:601
#14 0xffffffff813e1099 in iomap_dio_rw (iocb=iocb@entry=0xffff88800c262900, iter=iter@entry=0xffffc90001c4fd08, ops=<optimized out>, dops=dops@entry=0x0 <fixed_percpu_data>, dio_flags=dio_flags@entry=0, private=private@entry=0x0 <fixed_percpu_data>, done_before=0) at fs/iomap/direct-io.c:690
#15 0xffffffff8141cba1 in ext4_dio_read_iter (to=0xffffc90001c4fd08, iocb=0xffff88800c262900) at fs/ext4/file.c:94
#16 ext4_file_read_iter (iocb=0xffff88800c262900, to=0xffffc90001c4fd08) at fs/ext4/file.c:145
#17 0xffffffff8166811e in call_read_iter (iter=0xffffc90001c4fd08, kio=<optimized out>, file=0xffff8881034be900) at ./include/linux/fs.h:2193
#18 io_iter_do_read (iter=0xffffc90001c4fd08, rw=<optimized out>) at io_uring/rw.c:637
#19 io_read (req=0xffff88800c262900, issue_flags=2147483649) at io_uring/rw.c:759
#20 0xffffffff8165a2fc in io_issue_sqe (req=req@entry=0xffff88800c262900, issue_flags=issue_flags@entry=2147483649) at io_uring/io_uring.c:1743
#21 0xffffffff8165ae62 in io_queue_sqe (req=0xffff88800c262900) at io_uring/io_uring.c:1916
#22 io_submit_sqe (sqe=0xffff888014319580, req=0xffff88800c262900, ctx=0xffff88800b741800) at io_uring/io_uring.c:2174
#23 io_submit_sqes (ctx=ctx@entry=0xffff88800b741800, nr=nr@entry=89) at io_uring/io_uring.c:2285
#24 0xffffffff8165b755 in __do_sys_io_uring_enter (fd=<optimized out>, to_submit=89, min_complete=1, flags=25, argp=0x7f48ea5ffcf0, argsz=24) at io_uring/io_uring.c:3220
#25 0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001c4ff58) at arch/x86/entry/common.c:50
#26 do_syscall_64 (regs=0xffffc90001c4ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#27 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
```txt
#1  0xffffffff8132d654 in new_slab (node=-1, flags=<optimized out>, s=0xffff888004468f00) at mm/slub.c:1992
#2  ___slab_alloc (s=s@entry=0xffff888004468f00, gfpflags=gfpflags@entry=600192, node=node@entry=-1, addr=addr@entry=18446744071581582474, c=c@entry=0xffff888083e33090, orig_size=orig_size@entry=248) at mm/slub.c:3180
#3  0xffffffff8132db2d in __slab_alloc (s=s@entry=0xffff888004468f00, gfpflags=gfpflags@entry=600192, node=node@entry=-1, addr=addr@entry=18446744071581582474, orig_size=orig_size@entry=248, c=<optimized out>) at mm/slub.c:3279
#4  0xffffffff8132e411 in slab_alloc_node (orig_size=248, addr=18446744071581582474, node=-1, gfpflags=600192, lru=0x0 <fixed_percpu_data>, s=0xffff888004468f00) at mm/slub.c:3364
#5  slab_alloc (orig_size=248, addr=18446744071581582474, gfpflags=600192, lru=0x0 <fixed_percpu_data>, s=0xffff888004468f00) at mm/slub.c:3406
#6  __kmem_cache_alloc_lru (gfpflags=600192, lru=0x0 <fixed_percpu_data>, s=0xffff888004468f00) at mm/slub.c:3413
#7  kmem_cache_alloc (s=0xffff888004468f00, gfpflags=600192) at mm/slub.c:3422
#8  0xffffffff8129c48a in mempool_alloc (pool=pool@entry=0xffffffff8353ab38 <blkdev_dio_pool+24>, gfp_mask=601280, gfp_mask@entry=3264) at mm/mempool.c:392
#9  0xffffffff81624653 in bio_alloc_bioset (bdev=bdev@entry=0xffff888100c68000, nr_vecs=<optimized out>, nr_vecs@entry=1, opf=opf@entry=34817, gfp_mask=gfp_mask@entry=3264, bs=bs@entry=0xffffffff8353ab20 <blkdev_dio_pool>) at block/bio.c:525
#10 0xffffffff8162251e in __blkdev_direct_IO_async (iocb=0xffff8882391ffa80, iter=0xffffc9000227fd08, nr_pages=1) at block/fops.c:311
#11 0xffffffff81299464 in generic_file_direct_write (iocb=iocb@entry=0xffff8882391ffa80, from=from@entry=0xffffc9000227fd08) at mm/filemap.c:3677
#12 0xffffffff81299640 in __generic_file_write_iter (iocb=iocb@entry=0xffff8882391ffa80, from=from@entry=0xffffc9000227fd08) at mm/filemap.c:3837
#13 0xffffffff816223ef in blkdev_write_iter (iocb=0xffff8882391ffa80, from=0xffffc9000227fd08) at block/fops.c:541
#14 0xffffffff813c740f in call_write_iter (iter=0xffffc9000227fd08, kio=0xffff8882391ffa80, file=0xffff888006763000) at ./include/linux/fs.h:2199
#15 aio_write (req=req@entry=0xffff8882391ffa80, iocb=iocb@entry=0xffffc9000227fe58, vectored=vectored@entry=false, compat=compat@entry=false) at fs/aio.c:1600
#16 0xffffffff813c969a in __io_submit_one (ctx=0xffff888011a98480, compat=<optimized out>, req=0xffff8882391ffa80, user_iocb=0x55c6b8178780, iocb=<optimized out>) at fs/aio.c:1972
#17 io_submit_one (ctx=ctx@entry=0xffff888011a98480, user_iocb=0x55c6b8178780, compat=compat@entry=false) at fs/aio.c:2019
#18 0xffffffff813c9eeb in __do_sys_io_submit (iocbpp=0x55c6b8188f50, nr=1, ctx_id=<optimized out>) at fs/aio.c:2078
#19 __se_sys_io_submit (iocbpp=94311980502864, nr=<optimized out>, ctx_id=<optimized out>) at fs/aio.c:2048
#20 __x64_sys_io_submit (regs=<optimized out>) at fs/aio.c:2048
#21 0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9000227ff58) at arch/x86/entry/common.c:50
#22 do_syscall_64 (regs=0xffffc9000227ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#23 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
- [ ] 为什么这里存在一个 mempool_alloc 的?

```txt
#0  0xffffffff812d10f8 in gup_pmd_range (pudp=0xffff8881034b1720, nr=0xffffc9000227fb24, pages=0xffff88822bcd99d8, flags=5, end=140433301229568, addr=140433301225472, pud=...) at mm/gup.c:2813
#1  gup_pud_range (p4dp=0xffffc9000227fb28, nr=0xffffc9000227fb24, pages=<optimized out>, flags=5, end=140433301229568, addr=<optimized out>, p4d=...) at mm/gup.c:2863
#2  gup_p4d_range (pgdp=0xffff8881072ba7f8, nr=0xffffc9000227fb24, pages=<optimized out>, flags=5, end=140433301229568, addr=<optimized out>, pgd=...) at mm/gup.c:2888
#3  gup_pgd_range (nr=0xffffc9000227fb24, pages=<optimized out>, flags=5, end=140433301229568, addr=<optimized out>) at mm/gup.c:2916
#4  lockless_pages_from_mm (pages=<optimized out>, gup_flags=5, end=140433301229568, start=140433301225472) at mm/gup.c:2992
#5  internal_get_user_pages_fast (start=start@entry=140433301225472, nr_pages=nr_pages@entry=1, gup_flags=5, pages=<optimized out>) at mm/gup.c:3037
#6  0xffffffff812d1c5b in get_user_pages_fast (start=start@entry=140433301225472, nr_pages=nr_pages@entry=1, gup_flags=<optimized out>, pages=<optimized out>) at mm/gup.c:3136
#7  0xffffffff81673b5e in __iov_iter_get_pages_alloc (i=i@entry=0xffffc9000227fd08, pages=pages@entry=0xffffc9000227fc08, maxsize=4096, maxpages=4, start=start@entry=0xffffc9000227fc38) at lib/iov_iter.c:1460
#8  0xffffffff81674249 in iov_iter_get_pages2 (i=i@entry=0xffffc9000227fd08, pages=<optimized out>, pages@entry=0xffff88822bcd99d8, maxsize=<optimized out>, maxpages=<optimized out>, start=start@entry=0xffffc9000227fc38) at lib/iov_iter.c:1503
#9  0xffffffff81625253 in __bio_iov_iter_get_pages (iter=0xffffc9000227fd08, bio=0xffff88822bcd9940) at block/bio.c:1218
#10 bio_iov_iter_get_pages (bio=bio@entry=0xffff88822bcd9940, iter=iter@entry=0xffffc9000227fd08) at block/bio.c:1288
#11 0xffffffff81622556 in __blkdev_direct_IO_async (iocb=iocb@entry=0xffff88822f72d3c0, iter=iter@entry=0xffffc9000227fd08, nr_pages=1) at block/fops.c:329
#12 0xffffffff81622ccf in blkdev_direct_IO (iter=0xffffc9000227fd08, iocb=0xffff88822f72d3c0) at block/fops.c:369
#13 blkdev_direct_IO (iter=0xffffc9000227fd08, iocb=0xffff88822f72d3c0) at block/fops.c:358
#14 blkdev_read_iter (iocb=0xffff88822f72d3c0, to=0xffffc9000227fd08) at block/fops.c:588
#15 0xffffffff813c76d7 in call_read_iter (iter=0xffffc9000227fd08, kio=0xffff88822f72d3c0, file=0xffff888006763000) at ./include/linux/fs.h:2193
#16 aio_read (req=req@entry=0xffff88822f72d3c0, iocb=iocb@entry=0xffffc9000227fe58, vectored=vectored@entry=false, compat=compat@entry=false) at fs/aio.c:1560
#17 0xffffffff813c946d in __io_submit_one (ctx=0xffff888011a98480, compat=<optimized out>, req=0xffff88822f72d3c0, user_iocb=0x55c6b81a78c0, iocb=<optimized out>) at fs/aio.c:1970
#18 io_submit_one (ctx=ctx@entry=0xffff888011a98480, user_iocb=0x55c6b81a78c0, compat=compat@entry=false) at fs/aio.c:2019
#19 0xffffffff813c9eeb in __do_sys_io_submit (iocbpp=0x55c6b8188ec0, nr=1, ctx_id=<optimized out>) at fs/aio.c:2078
#20 __se_sys_io_submit (iocbpp=94311980502720, nr=<optimized out>, ctx_id=<optimized out>) at fs/aio.c:2048
#21 __x64_sys_io_submit (regs=<optimized out>) at fs/aio.c:2048
#22 0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9000227ff58) at arch/x86/entry/common.c:50
#23 do_syscall_64 (regs=0xffffc9000227ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
```
- 这里是调用到 gup，似乎是所有的读写都会发送给 userspace 的位置，然后 userspace 通过 iouring 将数据发送到内核中。


好吧，大致是理解了，但是需要更加深入的理解一下 io uring
