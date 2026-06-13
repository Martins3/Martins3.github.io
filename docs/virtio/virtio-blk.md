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

- read_mapping_folio
  - read_cache_folio
    - do_read_cache_folio
      - filemap_read_folio
        - block_read_full_folio
          - submit_bh
            - submit_bh_wbc
              - submit_bio_noacct_nocheck
                - submit_bio_noacct_nocheck
                  - __submit_bio_noacct_mq
                    - __submit_bio
                      - blk_mq_submit_bio
                        - blk_mq_try_issue_directly
                          - __blk_mq_try_issue_directly
                            - __blk_mq_issue_directly
                              - virtio_queue_rq
                                - virtblk_prep_rq
                                  - virtblk_setup_cmd

但是追查 `request::__sector` 就和 block layer 放到一起了。

kick queue 的来源:
```txt
#0  0xffffffff816cd803 in blk_mq_kick_requeue_list (q=0xffff888005748368) at block/blk-mq.c:1511
#1  blk_mq_add_to_requeue_list (rq=<optimized out>, at_head=at_head@entry=true, kick_requeue_list=kick_requeue_list@entry=true) at block/blk-mq.c:1506
#2  0xffffffff816c2fe7 in blk_flush_queue_rq (add_front=true, rq=<optimized out>) at block/blk-flush.c:143
#3  blk_flush_complete_seq (rq=<optimized out>, fq=fq@entry=0xffff8880055f5240, seq=<optimized out>, error=error@entry=0 '\000') at block/blk-flush.c:198
#4  0xffffffff816c34ae in flush_end_io (flush_rq=<optimized out>, error=0 '\000') at block/blk-flush.c:268
#5  0xffffffff816cbaa6 in __blk_mq_end_request (rq=0xffff8880056b6000, error=<optimized out>) at block/blk-mq.c:1043
#6  0xffffffff81a99ca9 in virtblk_done (vq=0xffff88810067d000) at drivers/block/virtio_blk.c:291
#7  0xffffffff818035c6 in vring_interrupt (irq=<optimized out>, _vq=0xffffffff) at drivers/virtio/virtio_ring.c:2470
#8  vring_interrupt (irq=<optimized out>, _vq=0xffffffff) at drivers/virtio/virtio_ring.c:2445
#9  0xffffffff811a3c72 in __handle_irq_event_percpu (desc=desc@entry=0xffff8880056b4400) at kernel/irq/handle.c:158
#10 0xffffffff811a3e53 in handle_irq_event_percpu (desc=0xffff8880056b4400) at kernel/irq/handle.c:193
#11 handle_irq_event (desc=desc@entry=0xffff8880056b4400) at kernel/irq/handle.c:210
#12 0xffffffff811a8b2e in handle_edge_irq (desc=0xffff8880056b4400) at kernel/irq/chip.c:819
#13 0xffffffff810ce1d5 in generic_handle_irq_desc (desc=0xffff8880056b4400) at ./include/linux/irqdesc.h:158
#14 handle_irq (regs=<optimized out>, desc=0xffff8880056b4400) at arch/x86/kernel/irq.c:231
#15 __common_interrupt (regs=<optimized out>, vector=35) at arch/x86/kernel/irq.c:250
#16 0xffffffff82178827 in common_interrupt (regs=0xffffffff82c03df8, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
## 看看这个
https://mp.weixin.qq.com/s/tSgKUQ2FLvoEZ1kERO5umw

## qemu 的 fstrim 

参考这个命令:
```sh
systemctl status fstrim.service
```

```txt
➜  ~ /usr/sbin/fstrim --listed-in /etc/fstab:/proc/self/mountinfo --verbose --quiet-unsupported

/home: 3.7 TiB (4084023906304 bytes) trimmed on /dev/mapper/openeuler-home
/boot: 799.1 MiB (837894144 bytes) trimmed on /dev/sdd2
/: 31.7 GiB (34079129600 bytes) trimmed on /dev/mapper/openeuler-root
```
似乎没有正确的 QEMU 配置吗？
显然，是有很大的空间差别的。

记得
man fstrim(8)

https://askubuntu.com/questions/1492995/how-to-know-if-my-nvme-ssd-needs-trim

fstrm 真的对于 qcow2 有效吗?

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
