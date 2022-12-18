# page-io.c

从这里通往文件系统和 blkdev:
```txt
#0  swap_readpage (page=page@entry=0xffffea0000cb5a40, synchronous=synchronous@entry=true, plug=plug@entry=0x0 <fixed_percpu_data>) at mm/page_io.c:450
#1  0xffffffff81308eb2 in read_swap_cache_async (plug=0x0 <fixed_percpu_data>, do_poll=<optimized out>, addr=140088455159808, vma=<optimized out>, gfp_mask=1051850, entry=...) at mm/swap_state.c:526
#2  swap_cluster_readahead (entry=..., gfp_mask=gfp_mask@entry=1051850, vmf=vmf@entry=0xffffc900018b7df8) at mm/swap_state.c:661
#3  0xffffffff813092af in swapin_readahead (entry=..., entry@entry=..., gfp_mask=gfp_mask@entry=1051850, vmf=vmf@entry=0xffffc900018b7df8) at mm/swap_state.c:855
#4  0xffffffff812d84fc in do_swap_page (vmf=vmf@entry=0xffffc900018b7df8) at mm/memory.c:3822
#5  0xffffffff812dccdc in handle_pte_fault (vmf=0xffffc900018b7df8) at mm/memory.c:4959
#6  __handle_mm_fault (vma=vma@entry=0xffff8881a866c5f0, address=address@entry=140088455163232, flags=flags@entry=596) at mm/memory.c:5097
#7  0xffffffff812dd7b0 in handle_mm_fault (vma=0xffff8881a866c5f0, address=address@entry=140088455163232, flags=flags@entry=596, regs=regs@entry=0xffffc900018b7f58) at mm/memory.c:5218
#8  0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc900018b7f58, error_code=error_code@entry=4, address=address@entry=140088455163232) at arch/x86/mm/fault.c:1428
#9  0xffffffff81faf042 in handle_page_fault (address=140088455163232, error_code=4, regs=0xffffc900018b7f58) at arch/x86/mm/fault.c:1519
#10 exc_page_fault (regs=0xffffc900018b7f58, error_code=4) at arch/x86/mm/fault.c:1575
#11 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

```txt
#0  swap_writepage (page=0xffffea0000f31380, wbc=0xffffc900012c3a78) at include/linux/page-flags.h:253
#1  0xffffffff812ac6e2 in pageout (folio=folio@entry=0xffffea0000f31380, mapping=mapping@entry=0xffff888100f2c000, plug=plug@entry=0xffffc900012c3b40) at mm/vmscan.c:1276
#2  0xffffffff812ada75 in shrink_folio_list (folio_list=folio_list@entry=0xffffc900012c3c30, pgdat=pgdat@entry=0xffff88803fffc000, sc=sc@entry=0xffffc900012c3dd8, stat=stat@entry=0xffffc900012c3cb8, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1929
#3  0xffffffff812af5d8 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc900012c3dd8, lruvec=0xffff8880040da000, nr_to_scan=<optimized out>) at mm/vmscan.c:2489
#4  shrink_list (sc=0xffffc900012c3dd8, lruvec=0xffff8880040da000, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2716
#5  shrink_lruvec (lruvec=lruvec@entry=0xffff8880040da000, sc=sc@entry=0xffffc900012c3dd8) at mm/vmscan.c:5885
#6  0xffffffff812afe3f in shrink_node_memcgs (sc=0xffffc900012c3dd8, pgdat=0xffff88803fffc000) at mm/vmscan.c:6074
#7  shrink_node (pgdat=pgdat@entry=0xffff88803fffc000, sc=sc@entry=0xffffc900012c3dd8) at mm/vmscan.c:6105
#8  0xffffffff812b0577 in kswapd_shrink_node (sc=0xffffc900012c3dd8, pgdat=0xffff88803fffc000) at mm/vmscan.c:6894
#9  balance_pgdat (pgdat=pgdat@entry=0xffff88803fffc000, order=order@entry=0, highest_zoneidx=highest_zoneidx@entry=3) at mm/vmscan.c:7084
#10 0xffffffff812b0b2b in kswapd (p=0xffff88803fffc000) at mm/vmscan.c:7344
#11 0xffffffff81133853 in kthread (_create=0xffff8880250bb4c0) at kernel/kthread.c:376
#12 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

## swap 的 io 是一个异步的行为
### read swap

swap 的 read 是一个异步的行为:
```txt
#0  folio_wait_bit_common (folio=0xffffea000100c700, bit_nr=0, state=258, behavior=SHARED) at mm/filemap.c:1220
#1  0xffffffff812e37b1 in folio_wait_bit_killable (bit_nr=0, folio=0xffffea000100c700) at mm/filemap.c:1447
#2  folio_wait_locked_killable (folio=0xffffea000100c700) at ./include/linux/pagemap.h:1028
#3  folio_wait_locked_killable (folio=0xffffea000100c700) at ./include/linux/pagemap.h:1024
#4  __folio_lock_or_retry (folio=folio@entry=0xffffea000100c700, mm=mm@entry=0xffff8881401cc740, flags=flags@entry=2644) at mm/filemap.c:1722
#5  0xffffffff81325984 in folio_lock_or_retry (flags=2644, mm=<optimized out>, folio=0xffffea000100c700) at ./include/linux/pagemap.h:1001
#6  do_swap_page (vmf=vmf@entry=0xffffc900404f3df8) at mm/memory.c:3815
#7  0xffffffff8132a27e in handle_pte_fault (vmf=0xffffc900404f3df8) at mm/memory.c:4935
#8  __handle_mm_fault (vma=vma@entry=0xffff888118dc2428, address=address@entry=139787038240768, flags=flags@entry=596) at mm/memory.c:5073
#9  0xffffffff8132ae24 in handle_mm_fault (vma=0xffff888118dc2428, address=address@entry=139787038240768, flags=<optimized out>, flags@entry=596, regs=regs@entry=0xffffc900404f3f58) at mm/memory.c:5219
#10 0xffffffff8110d947 in do_user_addr_fault (regs=regs@entry=0xffffc900404f3f58, error_code=error_code@entry=4, address=address@entry=139787038240768) at arch/x86/mm/fault.c:1428
#11 0xffffffff82184606 in handle_page_fault (address=139787038240768, error_code=4, regs=0xffffc900404f3f58) at arch/x86/mm/fault.c:1519
#12 exc_page_fault (regs=0xffffc900404f3f58, error_code=4) at arch/x86/mm/fault.c:1575
#13 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

```txt
#0  folio_wake_bit (folio=0xffffea00010051c0, bit_nr=bit_nr@entry=0) at mm/filemap.c:1130
#1  0xffffffff812ddbf6 in folio_unlock (folio=<optimized out>) at mm/filemap.c:1527
#2  0xffffffff81357945 in end_swap_bio_read (bio=0xffff88803a2ffe00) at mm/page_io.c:71
#3  0xffffffff816ccd4c in req_bio_endio (error=0 '\000', nbytes=4096, bio=0xffff88803a2ffe00, rq=0xffff88801b73e180) at block/blk-mq.c:794
#4  blk_update_request (req=req@entry=0xffff88801b73e180, error=error@entry=0 '\000', nr_bytes=4096, nr_bytes@entry=8192) at block/blk-mq.c:926
#5  0xffffffff81acd5a2 in scsi_end_request (req=req@entry=0xffff88801b73e180, error=error@entry=0 '\000', bytes=bytes@entry=8192) at drivers/scsi/scsi_lib.c:539
#6  0xffffffff81ace089 in scsi_io_completion (cmd=0xffff88801b73e290, good_bytes=8192) at drivers/scsi/scsi_lib.c:977
#7  0xffffffff816c99f8 in blk_complete_reqs (list=<optimized out>) at block/blk-mq.c:1131
#8  0xffffffff82196e34 in __do_softirq () at kernel/softirq.c:571
#9  0xffffffff811318aa in invoke_softirq () at kernel/softirq.c:445
#10 __irq_exit_rcu () at kernel/softirq.c:650
#11 0xffffffff82183fa6 in sysvec_apic_timer_interrupt (regs=0xffffffff82c03df8) at arch/x86/kernel/apic/apic.c:1107
```

### write swap
submit_bio_noacct 的位置本身就是一个异步的操作。
