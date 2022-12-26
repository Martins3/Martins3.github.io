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

## swap io
### read swap 是一个不同的行为

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

如果出现错误 do_swap_page 可以走到这个位置上去，
```c
	if (unlikely(!folio_test_uptodate(folio))) {
		ret = VM_FAULT_SIGBUS;
		goto out_nomap;
	}
```

如果 end_swap_bio_read 失败了:
```c
static void end_swap_bio_read(struct bio *bio)
{
	struct page *page = bio_first_page_all(bio);
	struct task_struct *waiter = bio->bi_private;

	if (bio->bi_status) {
		SetPageError(page);
		ClearPageUptodate(page);
		pr_alert_ratelimited("Read-error on swap-device (%u:%u:%llu)\n",
				     MAJOR(bio_dev(bio)), MINOR(bio_dev(bio)),
				     (unsigned long long)bio->bi_iter.bi_sector);
		goto out;
	}

	SetPageUptodate(page);
out:
	unlock_page(page);
	WRITE_ONCE(bio->bi_private, NULL);
	bio_put(bio);
	if (waiter) {
		blk_wake_io_task(waiter);
		put_task_struct(waiter);
	}
}
```
如果是采用 fault inject 机制注入错误，submit_bio_noacct 中直接跳转到最后，开始执行 bio_endio ，
进而调用到 end_swap_bio_read

如果是 scsi debug 机制的:
```txt
[   49.130061] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[   49.130373] Call Trace:
[   49.130483]  <IRQ>
[   49.130578]  dump_stack_lvl+0x38/0x4c
[   49.130724]  end_swap_bio_read.cold+0x35/0x3a
[   49.130901]  blk_update_request+0xfc/0x410
[   49.131060]  scsi_end_request+0x22/0x1b0
[   49.131215]  scsi_io_completion+0x4db/0x880
[   49.131378]  blk_complete_reqs+0x3b/0x50
[   49.131532]  __do_softirq+0xf7/0x301
[   49.131672]  __irq_exit_rcu+0xca/0x110
[   49.131817]  sysvec_apic_timer_interrupt+0xa6/0xd0
[   49.132003]  </IRQ>
```
所以，正常来说，只要是注入错误，都是要立刻挂掉的。

### write swap
submit_bio_noacct 的位置本身就是一个异步的操作。

```txt
#0  swap_writepage (page=0xffffea00054bddc0, wbc=0xffffc9003f1ab8e0) at mm/page_io.c:184
#1  0xffffffff812f67d2 in pageout (folio=folio@entry=0xffffea00054bddc0, mapping=mapping@entry=0xffff88814743b600, plug=plug@entry=0xffffc9003f1ab9a8) at mm/vmscan.c:1298
#2  0xffffffff812f7c22 in shrink_folio_list (folio_list=folio_list@entry=0xffffc9003f1aba90, pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc9003f1abc68, stat=stat@entry=0xffffc9003f1abb18, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1947
#3  0xffffffff812f9944 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc9003f1abc68, lruvec=0xffff8881022f2800, nr_to_scan=<optimized out>) at mm/vmscan.c:2526
#4  shrink_list (sc=0xffffc9003f1abc68, lruvec=0xffff8881022f2800, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2767
#5  shrink_lruvec (lruvec=lruvec@entry=0xffff8881022f2800, sc=sc@entry=0xffffc9003f1abc68) at mm/vmscan.c:5951
#6  0xffffffff812fa20e in shrink_node_memcgs (sc=0xffffc9003f1abc68, pgdat=0xffff88823fff9000) at mm/vmscan.c:6138
#7  shrink_node (pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc9003f1abc68) at mm/vmscan.c:6169
#8  0xffffffff812fb450 in shrink_zones (sc=0xffffc9003f1abc68, zonelist=<optimized out>) at mm/vmscan.c:6407
#9  do_try_to_free_pages (zonelist=zonelist@entry=0xffff88823fffab00, sc=sc@entry=0xffffc9003f1abc68) at mm/vmscan.c:6469
#10 0xffffffff812fc367 in try_to_free_mem_cgroup_pages (memcg=memcg@entry=0xffff8881022f8000, nr_pages=nr_pages@entry=1, gfp_mask=gfp_mask@entry=3264, reclaim_options=reclaim_options@entry=2, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/vmscan.c:6786
#11 0xffffffff813a0bfa in try_charge_memcg (memcg=memcg@entry=0xffff8881022f8000, gfp_mask=gfp_mask@entry=3264, nr_pages=1) at mm/memcontrol.c:2687
#12 0xffffffff813a1b6d in try_charge (nr_pages=1, gfp_mask=3264, memcg=0xffff8881022f8000) at mm/memcontrol.c:2830
#13 charge_memcg (folio=folio@entry=0xffffea00052ceb00, memcg=memcg@entry=0xffff8881022f8000, gfp=gfp@entry=3264) at mm/memcontrol.c:6947
#14 0xffffffff813a3468 in __mem_cgroup_charge (folio=0xffffea00052ceb00, mm=<optimized out>, gfp=gfp@entry=3264) at mm/memcontrol.c:6968
#15 0xffffffff8132a69b in mem_cgroup_charge (gfp=3264, mm=<optimized out>, folio=<optimized out>) at ./include/linux/memcontrol.h:671
#16 do_anonymous_page (vmf=0xffffc9003f1abdf8) at mm/memory.c:4078
#17 handle_pte_fault (vmf=0xffffc9003f1abdf8) at mm/memory.c:4929
#18 __handle_mm_fault (vma=vma@entry=0xffff88812842de40, address=address@entry=139790768721920, flags=flags@entry=597) at mm/memory.c:5073
#19 0xffffffff8132b104 in handle_mm_fault (vma=0xffff88812842de40, address=address@entry=139790768721920, flags=<optimized out>, flags@entry=597, regs=regs@entry=0xffffc9003f1abf58) at mm/memory.c:5219
#20 0xffffffff8110d937 in do_user_addr_fault (regs=regs@entry=0xffffc9003f1abf58, error_code=error_code@entry=6, address=address@entry=139790768721920) at arch/x86/mm/fault.c:1428
#21 0xffffffff8218d606 in handle_page_fault (address=139790768721920, error_code=6, regs=0xffffc9003f1abf58) at arch/x86/mm/fault.c:1519
#22 exc_page_fault (regs=0xffffc9003f1abf58, error_code=6) at arch/x86/mm/fault.c:1575
#23 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
最后，应该无论如何都不会卡到 blk wbt 上啊发，中间都在干什么哇？

- 似乎如果是普通的 readwrite 是立刻死掉的。

然后马上进入到 end_swap_bio_write 中，

- 最后经过超级长的时间，程序才会被 kill 掉。

是同步的吗？

- swap_writepage
  - `__swap_writepage`
    - bdev_write_page
    - set_page_writeback
    - submit_bio

- end_swap_bio_write
  - end_page_writeback
  - folio_wake(folio, PG_writeback); // 这个到底是在唤醒谁？

- folio_wait_writeback : 等待者
- folio_wait_writeback_killable

 换一个角度思考:
 page-io.md 中，直接看看，有没有什么等待之类的：
 ```txt
#0  swap_writepage (page=0xffffea00054bddc0, wbc=0xffffc9003f1ab8e0) at mm/page_io.c:184
#1  0xffffffff812f67d2 in pageout (folio=folio@entry=0xffffea00054bddc0, mapping=mapping@entry=0xffff88814743b600, plug=plug@entry=0xffffc9003f1ab9a8) at mm/vmscan.c:1298
#2  0xffffffff812f7c22 in shrink_folio_list (folio_list=folio_list@entry=0xffffc9003f1aba90, pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc9003f1abc68, stat=stat@entry=0xffffc9003f1abb18, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1947
#3  0xffffffff812f9944 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc9003f1abc68, lruvec=0xffff8881022f2800, nr_to_scan=<optimized out>) at mm/vmscan.c:2526
#4  shrink_list (sc=0xffffc9003f1abc68, lruvec=0xffff8881022f2800, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2767
#5  shrink_lruvec (lruvec=lruvec@entry=0xffff8881022f2800, sc=sc@entry=0xffffc9003f1abc68) at mm/vmscan.c:5951
#6  0xffffffff812fa20e in shrink_node_memcgs (sc=0xffffc9003f1abc68, pgdat=0xffff88823fff9000) at mm/vmscan.c:6138
#7  shrink_node (pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc9003f1abc68) at mm/vmscan.c:6169
#8  0xffffffff812fb450 in shrink_zones (sc=0xffffc9003f1abc68, zonelist=<optimized out>) at mm/vmscan.c:6407
#9  do_try_to_free_pages (zonelist=zonelist@entry=0xffff88823fffab00, sc=sc@entry=0xffffc9003f1abc68) at mm/vmscan.c:6469
#10 0xffffffff812fc367 in try_to_free_mem_cgroup_pages (memcg=memcg@entry=0xffff8881022f8000, nr_pages=nr_pages@entry=1, gfp_mask=gfp_mask@entry=3264, reclaim_options=reclaim_options@entry=2, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/vmscan.c:6786
#11 0xffffffff813a0bfa in try_charge_memcg (memcg=memcg@entry=0xffff8881022f8000, gfp_mask=gfp_mask@entry=3264, nr_pages=1) at mm/memcontrol.c:2687
#12 0xffffffff813a1b6d in try_charge (nr_pages=1, gfp_mask=3264, memcg=0xffff8881022f8000) at mm/memcontrol.c:2830
#13 charge_memcg (folio=folio@entry=0xffffea00052ceb00, memcg=memcg@entry=0xffff8881022f8000, gfp=gfp@entry=3264) at mm/memcontrol.c:6947
#14 0xffffffff813a3468 in __mem_cgroup_charge (folio=0xffffea00052ceb00, mm=<optimized out>, gfp=gfp@entry=3264) at mm/memcontrol.c:6968
#15 0xffffffff8132a69b in mem_cgroup_charge (gfp=3264, mm=<optimized out>, folio=<optimized out>) at ./include/linux/memcontrol.h:671
#16 do_anonymous_page (vmf=0xffffc9003f1abdf8) at mm/memory.c:4078
#17 handle_pte_fault (vmf=0xffffc9003f1abdf8) at mm/memory.c:4929
#18 __handle_mm_fault (vma=vma@entry=0xffff88812842de40, address=address@entry=139790768721920, flags=flags@entry=597) at mm/memory.c:5073
#19 0xffffffff8132b104 in handle_mm_fault (vma=0xffff88812842de40, address=address@entry=139790768721920, flags=<optimized out>, flags@entry=597, regs=regs@entry=0xffffc9003f1abf58) at mm/memory.c:5219
#20 0xffffffff8110d937 in do_user_addr_fault (regs=regs@entry=0xffffc9003f1abf58, error_code=error_code@entry=6, address=address@entry=139790768721920) at arch/x86/mm/fault.c:1428
#21 0xffffffff8218d606 in handle_page_fault (address=139790768721920, error_code=6, regs=0xffffc9003f1abf58) at arch/x86/mm/fault.c:1519
#22 exc_page_fault (regs=0xffffc9003f1abf58, error_code=6) at arch/x86/mm/fault.c:1575
#23 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
 ```

#### 分析普通的 io 的如何处理错误

完成 write 之后，是如何将 page 释放掉的?

因为一直在推迟 write 的，一直都是正常的，只有等到 sync 之后才会开始 write 的。

取决于是否为同步 IO，如果是，那么问题反映非常及时。

如果是 buffer io 的话 ，注入错误之后，需要 umount 一次，否则错误一直出现。

肯定是有问题的，如果同步 IO 出现错误是立刻的，那么在

#### [ ]  wbt 延长了错误的出现时间而已，应该，总是会出现错误的

#### pageout 调用 address_space_operations::writepage，但是文件系统都是不注册这个 hook 的
- [ ] 等价于，dirty file 的 flush 是谁在做？

#### 如何释放被 swap 的页
- end_swap_bio_write
  - end_page_writeback : 这里的时候 folio 还是存在一个 reference count 的!
    - 忽然意识到，这个 jb 玩意儿实际上是在 swap cache 中存在一个 referenced


怎么有 hugepage 啊？
```txt
#0  free_swap_cache (page=page@entry=0xffffea00087ab740) at mm/swap_state.c:281
#1  0xffffffff8135961d in free_page_and_swap_cache (page=0xffffea00087ab740) at mm/swap_state.c:297
#2  0xffffffff8138f56b in __split_huge_page (end=18446744073709551615, list=0xffffea00087a8000, page=0x0 <fixed_percpu_data>) at mm/huge_memory.c:2613
#3  split_huge_page_to_list (page=page@entry=0xffffea00087a8000, list=list@entry=0xffffc90042bf7b90) at mm/huge_memory.c:2778
#4  0xffffffff812f801a in split_folio_to_list (list=0xffffc90042bf7b90, folio=0xffffea00087a8000) at ./include/linux/huge_mm.h:444
#5  shrink_folio_list (folio_list=folio_list@entry=0xffffc90042bf7b90, pgdat=pgdat@entry=0xffff88833fffc000, sc=sc@entry=0xffffc90042bf7d68, stat=stat@entry=0xffffc90042bf7c18, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1856
#6  0xffffffff812f9944 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc90042bf7d68, lruvec=0xffff8881036bd000, nr_to_scan=<optimized out>) at mm/vmscan.c:2526
#7  shrink_list (sc=0xffffc90042bf7d68, lruvec=0xffff8881036bd000, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2767
#8  shrink_lruvec (lruvec=lruvec@entry=0xffff8881036bd000, sc=sc@entry=0xffffc90042bf7d68) at mm/vmscan.c:5951
#9  0xffffffff812fa20e in shrink_node_memcgs (sc=0xffffc90042bf7d68, pgdat=0xffff88833fffc000) at mm/vmscan.c:6138
#10 shrink_node (pgdat=pgdat@entry=0xffff88833fffc000, sc=sc@entry=0xffffc90042bf7d68) at mm/vmscan.c:6169
#11 0xffffffff812fb450 in shrink_zones (sc=0xffffc90042bf7d68, zonelist=<optimized out>) at mm/vmscan.c:6407
#12 do_try_to_free_pages (zonelist=zonelist@entry=0xffff8884bfff3700, sc=sc@entry=0xffffc90042bf7d68) at mm/vmscan.c:6469
#13 0xffffffff812fc367 in try_to_free_mem_cgroup_pages (memcg=memcg@entry=0xffff888358a97000, nr_pages=<optimized out>, gfp_mask=gfp_mask@entry=3264, reclaim_options=reclaim_options@entry=2, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/vmscan.c:6786
#14 0xffffffff8139ecb2 in memory_max_write (of=<optimized out>, buf=<optimized out>, nbytes=5, off=<optimized out>) at mm/memcontrol.c:6481
#15 0xffffffff8146bbf4 in kernfs_fop_write_iter (iocb=0xffffc90042bf7ea0, iter=<optimized out>) at fs/kernfs/file.c:334
#16 0xffffffff813c634d in call_write_iter (iter=0x7f898e5ff, kio=0xffffea00087ab740, file=0xffff88810438f800) at ./include/linux/fs.h:2186
#17 new_sync_write (ppos=0xffffc90042bf7f08, len=5, buf=0x18f49f0 "1000m", filp=0xffff88810438f800) at fs/read_write.c:491
#18 vfs_write (file=file@entry=0xffff88810438f800, buf=buf@entry=0x18f49f0 "1000m", count=count@entry=5, pos=pos@entry=0xffffc90042bf7f08) at fs/read_write.c:584
#19 0xffffffff813c679e in ksys_write (fd=<optimized out>, buf=0x18f49f0 "1000m", count=5) at fs/read_write.c:637
#20 0xffffffff82188c1c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90042bf7f58) at arch/x86/entry/common.c:50
#21 do_syscall_64 (regs=0xffffc90042bf7f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#22 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
- add_to_swap_cache ?

### 如果 swap 正在 writeback 的过程中，对于该 page 进行了一次写，如何？
folio_mark_dirty 中会调用 folio_clear_reclaim，该 page 重新为 dirty 的，并且进入到 lru 的队列尾部。

### 如果 swap 正在 writeback 的过程中，对于该 page 进行了一次读，如何？
感觉这会导致，page 进入到 lru 队列尾。但是这会产生一个 swap cache 吗?

### [ ] SetPageError 为什么是没有结果的?

```c
static __always_inline bool folio_test_error(struct folio *folio) {
  return test_bit(PG_error, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline int PageError(struct page *page) {
  return test_bit(PG_error, &PF_NO_TAIL(page, 0)->flags);
}
static __always_inline void folio_set_error(struct folio *folio) {
  set_bit(PG_error, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void SetPageError(struct page *page) {
  set_bit(PG_error, &PF_NO_TAIL(page, 1)->flags);
}
static __always_inline void folio_clear_error(struct folio *folio) {
  clear_bit(PG_error, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void ClearPageError(struct page *page) {
  clear_bit(PG_error, &PF_NO_TAIL(page, 1)->flags);
}
static __always_inline bool folio_test_clear_error(struct folio *folio) {
  return test_and_clear_bit(PG_error, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline int TestClearPageError(struct page *page) {
  return test_and_clear_bit(PG_error, &PF_NO_TAIL(page, 1)->flags);
}
```
几个 Test 的使用者就是如下的:
感觉根本没有人使用这个东西！
```c
mm/migrate.c
539:    if (folio_test_error(folio))

fs/gfs2/lops.c
477:    if (folio_test_error(folio))
```

```c
fs/f2fs/node.c
2082:           if (TestClearPageError(page))
```

mm/migrate.c 这个代码有意义吗?
```c
	if (folio_test_error(folio))
		folio_set_error(newfolio);
```
