# Guest 的 virtio-blk 如何将数据发送到 vring 中的

和 scsi 以及 nvme 相同的，驱动注册 multiqueue 的 hook:
```c
static const struct blk_mq_ops virtio_mq_ops = {
  .queue_rq = virtio_queue_rq,
  .complete = virtblk_request_done,
  .init_request = virtblk_init_request,
  .map_queues = virtblk_map_queues,
};
```

```c
/*
 * This comes first in the read scatter-gather list.
 * For legacy virtio, if VIRTIO_F_ANY_LAYOUT is not negotiated,
 * this is the first element of the read scatter-gather list.
 */
struct virtio_blk_outhdr {
  /* VIRTIO_BLK_T* */
  __virtio32 type;
  /* io priority. */
  __virtio32 ioprio;
  /* Sector (ie. 512 byte offset) */
  __virtio64 sector;
};
```

- virtblk_add_req 中，将 virtblk_req::out_hdr 放到 scatterlist 中，之后 scatterlist 的元素会被逐个转换为 vring_desc

分析一下 virtio_blk_outhdr 是如何生成的，在 virtblk_setup_cmd 中是 virtio_blk_outhdr::sector 中唯一的访问位置:

```txt
#0  virtblk_setup_cmd (vdev=0xffff888100705000, req=req@entry=0xffff8881411d0000, vbr=vbr@entry=0xffff8881411d0108) at drivers/block/virtio_blk.c:220
#1  0xffffffff81977e31 in virtblk_prep_rq (vblk=0xffff888101910000, vblk=0xffff888101910000, vbr=0xffff8881411d0108, req=0xffff8881411d0000, hctx=0xffff888141087600) at drivers/block/virtio_blk.c:321
#2  virtio_queue_rq (hctx=0xffff888141087600, bd=0xffffc9000003b868) at drivers/block/virtio_blk.c:348
#3  0xffffffff81611edc in __blk_mq_issue_directly (last=true, rq=0xffff8881411d0000, hctx=0xffff888141087600) at block/blk-mq.c:2440
#4  __blk_mq_try_issue_directly (hctx=0xffff888141087600, rq=rq@entry=0xffff8881411d0000, bypass_insert=bypass_insert@entry=false, last=last@entry=true) at block/blk-mq.c:2493
#5  0xffffffff816120d2 in blk_mq_try_issue_directly (hctx=<optimized out>, rq=0xffff8881411d0000) at block/blk-mq.c:2517
#6  0xffffffff8161370c in blk_mq_submit_bio (bio=<optimized out>) at block/blk-mq.c:2843
#7  0xffffffff81606212 in __submit_bio (bio=<optimized out>) at block/blk-core.c:595
#8  0xffffffff81606806 in __submit_bio_noacct_mq (bio=<optimized out>) at block/blk-core.c:672
#9  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:689
#10 submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:678
#11 0xffffffff81390395 in submit_bh_wbc (opf=<optimized out>, opf@entry=0, bh=0xffff8881017ab000, wbc=wbc@entry=0x0 <fixed_percpu_data>) at fs/buffer.c:2719
#12 0xffffffff81391e88 in submit_bh (bh=<optimized out>, opf=0) at fs/buffer.c:2725
#13 block_read_full_folio (folio=0xffffea0004066980, folio@entry=<error reading variable: value has been optimized out>, get_block=0xffffffff815fedb0 <blkdev_get_block>, get_block@entry=<error reading variable: value has been optimized out>) at fs/buffer.c:2340
#14 0xffffffff8127d0da in filemap_read_folio (file=0x0 <fixed_percpu_data>, filler=<optimized out>, folio=0xffffea0004066980) at mm/filemap.c:2394
#15 0xffffffff8127f9de in do_read_cache_folio (mapping=0xffff88810176b500, index=index@entry=0, filler=0xffffffff815fee00 <blkdev_read_folio>, filler@entry=0x0 <fixed_percpu_data>, file=file@entry=0x0 <fixed_percpu_data>, gfp=1051840) at mm/filemap.c:3519
#16 0xffffffff8127faa9 in read_cache_folio (mapping=<optimized out>, index=index@entry=0, filler=filler@entry=0x0 <fixed_percpu_data>, file=file@entry=0x0 <fixed_percpu_data>) at include/linux/pagemap.h:274
#17 0xffffffff8161deed in read_mapping_folio (file=0x0 <fixed_percpu_data>, index=0, mapping=<optimized out>) at include/linux/pagemap.h:762
```

但是追查 `request::__sector` 就和 block layer 放到一起了。
