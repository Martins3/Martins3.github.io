# fs/iomap

| file          | blank | comment | code | desc                                                                 |
|---------------|-------|---------|------|----------------------------------------------------------------------|
| buffered-io.c | 228   | 281     | 1146 |                                                                      |
| direct-io.c   | 76    | 107     | 392  | 唯二的 non static 函数 iomap_dio_rw 的唯一调用者 ext4_file_read_iter |
| trace.h       | 15    | 7       | 169  |                                                                      |
| seek.c        | 29    | 30      | 153  |                                                                      |
| fiemap.c      | 20    | 7       | 121  | 函数无人调用                                                         |
| swapfile.c    | 18    | 43      | 118  |                                                                      |
| apply.c       | 8     | 48      | 38   |                                                                      |
| Makefile      | 3     | 5       | 9    |                                                                      |
| trace.c       | 1     | 8       | 3    |                                                                      |


1. 对于设备 io mmap 找到具体的位置 ？
2. 这似乎根本不是我们想要的东西 ?

```txt
#0  bio_add_folio (bio=0xffff888005663000, folio=folio@entry=0xffffea0004583140, len=len@entry=4096, off=off@entry=0) at block/bio.c:1165
#1  0xffffffff81418c3a in do_mpage_readpage (args=args@entry=0xffffc9004065bae0) at fs/mpage.c:285
#2  0xffffffff81418fdc in mpage_readahead (rac=0xffffc9004065bcc0, get_block=<optimized out>) at fs/mpage.c:361
#3  0xffffffff812ed6b4 in read_pages (rac=rac@entry=0xffffc9004065bcc0) at mm/readahead.c:161
#4  0xffffffff812ed986 in page_cache_ra_unbounded (ractl=ractl@entry=0xffffc9004065bcc0, nr_to_read=1, lookahead_size=lookahead_size@entry=0) at mm/readahead.c:270
#5  0xffffffff812edd24 in do_page_cache_ra (lookahead_size=0, nr_to_read=<optimized out>, ractl=0xffffc9004065bcc0) at mm/readahead.c:300
#6  force_page_cache_ra (ractl=0xffffc9004065bcc0, nr_to_read=<optimized out>) at mm/readahead.c:331
#7  0xffffffff812ee39f in page_cache_sync_ra (ractl=ractl@entry=0xffffc9004065bcc0, req_count=<optimized out>, req_count@entry=1) at mm/readahead.c:705
#8  0xffffffff812e03d8 in page_cache_sync_readahead (req_count=1, index=26214384, file=0xffff8881160c3100, ra=0xffff8881160c3198, mapping=0xffff8880053f1708) at ./include/linux/pagemap.h:1210
#9  filemap_get_pages (iocb=iocb@entry=0xffffc9004065be98, iter=iter@entry=0xffffc9004065be70, fbatch=fbatch@entry=0xffffc9004065bd78) at mm/filemap.c:2600
#10 0xffffffff812e09d8 in filemap_read (iocb=iocb@entry=0xffffc9004065be98, iter=iter@entry=0xffffc9004065be70, already_read=already_read@entry=0) at mm/filemap.c:2694
#11 0xffffffff816b8cef in blkdev_read_iter (iocb=0xffffc9004065be98, to=0xffffc9004065be70) at block/fops.c:591
#12 0xffffffff813c5690 in call_read_iter (iter=0xffffea0004583140, kio=0xffff888005663000, file=0xffff8881160c3100) at ./include/linux/fs.h:2180
#13 new_sync_read (ppos=0xffffc9004065bf08, len=64, buf=0x5575c6977fb8 "", filp=0xffff8881160c3100) at fs/read_write.c:389
#14 vfs_read (file=file@entry=0xffff8881160c3100, buf=buf@entry=0x5575c6977fb8 "", count=count@entry=64, pos=pos@entry=0xffffc9004065bf08) at fs/read_write.c:470
#15 0xffffffff813c632e in ksys_read (fd=<optimized out>, buf=0x5575c6977fb8 "", count=64) at fs/read_write.c:613
#16 0xffffffff82176b8c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9004065bf58) at arch/x86/entry/common.c:50
#17 do_syscall_64 (regs=0xffffc9004065bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#18 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

- bio 描述的是连续的磁盘空间?
  - 首先，获取到读取，对应的磁盘的位置
  - 然后提交 io


```c
struct iomap_ops {
	/*
	 * Return the existing mapping at pos, or reserve space starting at
	 * pos for up to length, as long as we can do it as a single mapping.
	 * The actual length is returned in iomap->length.
	 */
	int (*iomap_begin)(struct inode *inode, loff_t pos, loff_t length,
			unsigned flags, struct iomap *iomap,
			struct iomap *srcmap);

	/*
	 * Commit and/or unreserve space previous allocated using iomap_begin.
	 * Written indicates the length of the successful write operation which
	 * needs to be commited, while the rest needs to be unreserved.
	 * Written might be zero if no data was written.
	 */
	int (*iomap_end)(struct inode *inode, loff_t pos, loff_t length,
			ssize_t written, unsigned flags, struct iomap *iomap);
};
```

好像 bio 中记录映射，而 iomap 来实现映射，所以 bio 的生成在 iomap 中:


看上去，ext4 只是对于 iomap 移植了 direct io ，而 buffered 没有移植:
```txt
#0  ext4_iomap_begin (inode=0xffff8880248bd870, offset=6324224, length=4096, flags=16, iomap=0xffffc90040317bc0, srcmap=0xffffc90040317c10) at fs/ext4/inode.c:3513
#1  0xffffffff8143f628 in iomap_iter (iter=iter@entry=0xffffc90040317b98, ops=ops@entry=0xffffffff8244fa40 <ext4_iomap_ops>) at fs/iomap/iter.c:91
#2  0xffffffff814439db in __iomap_dio_rw (iocb=iocb@entry=0xffff88802169f300, iter=iter@entry=0xffffc90040317d08, ops=0xffffffff8244fa40 <ext4_iomap_ops>, dops=dops@entry=0x0 <fixed_percpu_data>, dio_flags=dio_flags@entry=0, private=private@entry=0x0 <fixed_percpu_data>, done_before=0) at fs/iomap/direct-io.c:600
#3  0xffffffff8144415d in iomap_dio_rw (iocb=iocb@entry=0xffff88802169f300, iter=iter@entry=0xffffc90040317d08, ops=<optimized out>, dops=dops@entry=0x0 <fixed_percpu_data>, dio_flags=dio_flags@entry=0, private=private@entry=0x0 <fixed_percpu_data>, done_before=0) at fs/iomap/direct-io.c:689
#4  0xffffffff81488cd5 in ext4_dio_read_iter (to=0xffffc90040317d08, iocb=0xffff88802169f300) at fs/ext4/file.c:94
#5  ext4_file_read_iter (iocb=0xffff88802169f300, to=0xffffc90040317d08) at fs/ext4/file.c:145
#6  0xffffffff814287f7 in call_read_iter (iter=0xffffc90040317d08, kio=0xffff88802169f300, file=0xffff88801d6ce700) at ./include/linux/fs.h:2180
#7  aio_read (req=req@entry=0xffff88802169f300, iocb=iocb@entry=0xffffc90040317e58, vectored=vectored@entry=false, compat=compat@entry=false) at fs/aio.c:1560
#8  0xffffffff8142b6ed in __io_submit_one (ctx=0xffff888020fe0000, compat=<optimized out>, req=0xffff88802169f300, user_iocb=0x56524e6c2280, iocb=<optimized out>) at fs/aio.c:1970
#9  io_submit_one (ctx=ctx@entry=0xffff888020fe0000, user_iocb=0x56524e6c2280, compat=compat@entry=false) at fs/aio.c:2019
#10 0xffffffff8142bf1f in __do_sys_io_submit (iocbpp=0x56524e69fd80, nr=1, ctx_id=<optimized out>) at fs/aio.c:2078
#11 __se_sys_io_submit (iocbpp=94911502876032, nr=<optimized out>, ctx_id=<optimized out>) at fs/aio.c:2048
#12 __x64_sys_io_submit (regs=<optimized out>) at fs/aio.c:2048
#13 0xffffffff82176b8c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90040317f58) at arch/x86/entry/common.c:50
#14 do_syscall_64 (regs=0xffffc90040317f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#15 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## iomap
ext4/xfs's direct IO call `iomap_dio_rw`

- [ ] iomap_dio_rw

xfs_file_read_iter
==> xfs_file_dio_aio_read ==> iomap_dio_rw
==> xfs_file_buffered_aio_read ==> generic_file_read_iter

- [ ] I don't know how iomap_dio_rw write page to disk ?

IOMAP_F_BUFFER_HEAD

## 参考
[IO 子系统全流程介绍](https://zhuanlan.zhihu.com/p/545906763)

## buffer head 似乎还是存在的

例如
```c
static int blkdev_writepage(struct page *page, struct writeback_control *wbc)
{
	return block_write_full_page(page, blkdev_get_block, wbc);
}
```
中的 blkdev_get_block，其参数为 buffer_head

## xfs 注册
正常的路径
```txt
#0  xfs_buf_bio_end_io (bio=0xffff8881462f1900) at fs/xfs/xfs_buf.c:1421
#1  0xffffffff816ccffc in req_bio_endio (error=0 '\000', nbytes=512, bio=0xffff8881462f1900, rq=0xffff8881044c5780) at block/blk-mq.c:794
#2  blk_update_request (req=req@entry=0xffff8881044c5780, error=error@entry=0 '\000', nr_bytes=nr_bytes@entry=512) at block/blk-mq.c:926
#3  0xffffffff81ad5b82 in scsi_end_request (req=req@entry=0xffff8881044c5780, error=error@entry=0 '\000', bytes=bytes@entry=512) at drivers/scsi/scsi_lib.c:539
#4  0xffffffff81ad6669 in scsi_io_completion (cmd=0xffff8881044c5890, good_bytes=512) at drivers/scsi/scsi_lib.c:977
#5  0xffffffff81af8c7b in virtscsi_vq_done (fn=0xffffffff81af8a40 <virtscsi_complete_cmd>, virtscsi_vq=0xffff8881008e8a40, vscsi=0xffff8881008e8818) at drivers/scsi/virtio_scsi.c:183
#6  virtscsi_req_done (vq=<optimized out>) at drivers/scsi/virtio_scsi.c:198
#7  0xffffffff818134f9 in vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2470
#8  vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2445
#9  0xffffffff811a3d25 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100bba400) at kernel/irq/handle.c:158
#10 0xffffffff811a3f03 in handle_irq_event_percpu (desc=0xffff888100bba400) at kernel/irq/handle.c:193
#11 handle_irq_event (desc=desc@entry=0xffff888100bba400) at kernel/irq/handle.c:210
#12 0xffffffff811a8bde in handle_edge_irq (desc=0xffff888100bba400) at kernel/irq/chip.c:819
#13 0xffffffff810ce1d8 in generic_handle_irq_desc (desc=0xffff888100bba400) at ./include/linux/irqdesc.h:158
#14 handle_irq (regs=<optimized out>, desc=0xffff888100bba400) at arch/x86/kernel/irq.c:231
#15 __common_interrupt (regs=<optimized out>, vector=36) at arch/x86/kernel/irq.c:250
#16 0xffffffff8218a8b7 in common_interrupt (regs=0xffffc9000009be38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
