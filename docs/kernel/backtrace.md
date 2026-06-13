# 收集经典 backtrace
```txt
- asm_exc_page_fault
  - exc_page_fault
    - handle_page_fault
      - do_user_addr_fault
        - handle_mm_fault
          - __handle_mm_fault
            - handle_pte_fault
              - do_fault
                - do_cow_fault
                  - __do_fault
                    - filemap_fault
                      - do_sync_mmap_readahead
                        - do_page_cache_ra
                          - page_cache_ra_unbounded
                            - read_pages
                              - iomap_readahead
                                - submit_bio
```

```txt
#0  blk_account_io_completion (req=0xffff888100f00300, bytes=12288) at block/blk-mq.c:799
#1  0xffffffff816ccae5 in blk_update_request (req=req@entry=0xffff888100f00300, error=error@entry=0 '\000', nr_bytes=12288) at block/blk-mq.c:914
#2  0xffffffff816cceb9 in blk_mq_end_request (rq=0xffff888100f00300, error=0 '\000') at block/blk-mq.c:1053
#3  0xffffffff81a99ca9 in virtblk_done (vq=0xffff888140cedb00) at drivers/block/virtio_blk.c:291
#4  0xffffffff818035c6 in vring_interrupt (irq=<optimized out>, _vq=0xffff888100f00300) at drivers/virtio/virtio_ring.c:2470
```

```txt
@[
    blk_update_request+5
    scsi_end_request+39
    scsi_io_completion+90
    blk_complete_reqs+61
    __do_softirq+199
    __irq_exit_rcu+147
    common_interrupt+134
    asm_common_interrupt+38
    cpuidle_enter_state+204
    cpuidle_enter+45
    do_idle+472
    cpu_startup_entry+42
    start_secondary+286
    secondary_startup_64_no_verify+382
]: 217841
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
