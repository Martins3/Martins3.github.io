### 4
- 注入给 swap 的错误最后返回给谁？
  - 不会返回给任何人的。

- 如果采样的都是 read 的，为什么 write 不成功会被阻塞的哇?

```txt
- asm_exc_page_fault
  - exc_page_fault
    - handle_page_fault
      - do_user_addr_fault
        - handle_mm_fault
          - __handle_mm_fault
            - handle_pte_fault
              - do_anonymous_page
                - mem_cgroup_charge
                  - __mem_cgroup_charge
                    - charge_memcg
                      - try_charge
                        - try_charge_memcg
                          - try_to_free_mem_cgroup_pages
                            - do_try_to_free_pages
                              - shrink_zones
                                - shrink_node
                                  - shrink_node_memcgs
                                    - shrink_lruvec
                                      - shrink_list
```
- 各种 shrink_list 无法立刻保证，那么需要循环多少次?

- do_anonymous_page 中如果 mem_cgroup_charge 失败
  - try_charge_memcg
    - try_to_free_mem_cgroup_pages : 尝试释放内存


可以观测的一个的统计事件:
```c
memcg_memory_event(mem_over_limit, MEMCG_MAX);
```

从 shrink_folio_list 中看，只有 free 页面才会是算作 nr_reclaimed 的。

### 5
如果使用 fault injection 机制，就不会堵塞在哪里，将会是 100% CPU 消耗，但是阻塞的位置不是在 try_charge_memcg

shrink_folio_list 中，发现所有的 page 都没有办法写回，
```c
			switch (pageout(folio, mapping, &plug)) {
			case PAGE_KEEP:
				goto keep_locked;
			case PAGE_ACTIVATE:
				goto activate_locked;
			case PAGE_SUCCESS:
				stat->nr_pageout += nr_pages;

				if (folio_test_writeback(folio)){
					goto keep;
				}
				if (folio_test_dirty(folio)){
                    // 从这里直接跳转到 while 循环的尾部
					goto keep;
				}
```

shrink_inactive_list 中调用了 mem_cgroup_uncharge_list，
实际上，这些 page 还是在被使用的，folio_list 中都是没有释放的，
然后就被释放了，也许对于 mem_cgroup_uncharge_list

- shrink_lruvec 中是无法离开的!

```c
	while (nr[LRU_INACTIVE_ANON] || nr[LRU_ACTIVE_FILE] ||
					nr[LRU_INACTIVE_FILE]) {
		unsigned long nr_anon, nr_file, percentage;
		unsigned long nr_scanned;

		for_each_evictable_lru(lru) {
			if (nr[lru]) {
				nr_to_scan = min(nr[lru], SWAP_CLUSTER_MAX);
				nr[lru] -= nr_to_scan;

				nr_reclaimed += shrink_list(lru, nr_to_scan,
							    lruvec, sc);
			}
		}

		cond_resched();
		if (nr_reclaimed < nr_to_reclaim || proportional_reclaim)
			continue;
```

而且是无法杀掉的。

```c
		pr_info("[->:%s:%d] %ld %ld\n", __FUNCTION__, __LINE__, nr_reclaimed, nr_to_reclaim);
```
```txt
Dec 28 15:00:09 localhost.localdomain kernel: vmscan: [->:shrink_lruvec:5958] 1 32
```
按道理来说，是一个都不行的，但是 nr_reclaimed 居然还可以回收一个。

这里并不是死循环才对的，而且跳出还很快，细节之后分析吧。

### scsi debug 注入错误
如果是使用 scsi debug 的方式，会在 wbt 上阻塞。

其实是类似的方式，但是这个 write 循环太过于频繁，甚至导致 wbt 都其作用了。

在 end_swap_bio_write 的地方释放内存:
```txt
#0  end_swap_bio_write (bio=0xffff888101892300) at mm/page_io.c:32
#1  0xffffffff816ccfcc in req_bio_endio (error=10 '\n', nbytes=524288, bio=0xffff888101892300, rq=0xffff88812793c800) at block/blk-mq.c:794
#2  blk_update_request (req=req@entry=0xffff88812793c800, error=error@entry=10 '\n', nr_bytes=524288) at block/blk-mq.c:926
#3  0xffffffff81ad5b42 in scsi_end_request (req=req@entry=0xffff88812793c800, error=error@entry=10 '\n', bytes=<optimized out>) at drivers/scsi/scsi_lib.c:539
#4  0xffffffff81ad6a9b in scsi_io_completion_action (result=<optimized out>, cmd=0xffff88812793c910) at drivers/scsi/scsi_lib.c:841
#5  scsi_io_completion (cmd=0xffff88812793c910, good_bytes=<optimized out>) at drivers/scsi/scsi_lib.c:996
#6  0xffffffff816c9c78 in blk_complete_reqs (list=<optimized out>) at block/blk-mq.c:1131
#7  0xffffffff8219fe14 in __do_softirq () at kernel/softirq.c:571
#8  0xffffffff811318ca in invoke_softirq () at kernel/softirq.c:445
#9  __irq_exit_rcu () at kernel/softirq.c:650
#10 0xffffffff8218cfa6 in sysvec_apic_timer_interrupt (regs=0xffffc90042adfae8) at arch/x86/kernel/apic/apic.c:1107
```

```txt
#0  __wbt_done (wb_acct=WBT_TRACKED, rqos=0xffff888354881558) at block/blk-wbt.c:176
#1  wbt_done (rqos=0xffff888354881558, rq=0xffff88812793c800) at block/blk-wbt.c:201
#2  0xffffffff816dc6f0 in __rq_qos_done (rqos=0xffff888354881558, rq=rq@entry=0xffff88812793c800) at block/blk-rq-qos.c:39
#3  0xffffffff816cbdf1 in rq_qos_done (rq=0xffff88812793c800, q=0xffff8883413b3390) at block/blk-rq-qos.h:178
#4  blk_mq_free_request (rq=rq@entry=0xffff88812793c800) at block/blk-mq.c:742
#5  0xffffffff816cbf03 in __blk_mq_end_request (rq=rq@entry=0xffff88812793c800, error=error@entry=10 '\n') at block/blk-mq.c:1044
#6  0xffffffff81ad5bf9 in scsi_end_request (req=req@entry=0xffff88812793c800, error=error@entry=10 '\n', bytes=<optimized out>) at drivers/scsi/scsi_lib.c:574
#7  0xffffffff81ad6a9b in scsi_io_completion_action (result=<optimized out>, cmd=0xffff88812793c910) at drivers/scsi/scsi_lib.c:841
#8  scsi_io_completion (cmd=0xffff88812793c910, good_bytes=<optimized out>) at drivers/scsi/scsi_lib.c:996
#9  0xffffffff816c9c78 in blk_complete_reqs (list=<optimized out>) at block/blk-mq.c:1131
#10 0xffffffff8219fe14 in __do_softirq () at kernel/softirq.c:571
#11 0xffffffff811318ca in invoke_softirq () at kernel/softirq.c:445
#12 __irq_exit_rcu () at kernel/softirq.c:650
#13 0xffffffff8218cfa6 in sysvec_apic_timer_interrupt (regs=0xffffc90042adfae8) at arch/x86/kernel/apic/apic.c:1107
```
虽然是迅速的返回失败，但是在 wbt 的眼中，这个还是相当于 write 提交太快了。


### scsi debug 没有 wbt，可以立刻 OOM 掉
去掉 wbt 机制，对于 fault injection 是没有影响的。

- 但是 为什么去掉 wbt ，那么就会立刻 OOM 掉的
  - 在 mem cgroup 的限制下，通过 charge 收缩内存，如果不成功，会如何?

```txt
./fault/inject.sh: line 16:  1597 Bus error               (core dumped) cgexec -g memory:mem ./a.out
```

内核日志显示，死掉了，然后进行 read 的时候，进一步触发死亡的:
```txt
[   31.408189] vmscan: [->:shrink_lruvec:5958] 0 32
[   31.408378] vmscan: [->:shrink_lruvec:5958] 0 32
[   31.408563] vmscan: [->:shrink_lruvec:5958] 0 32
[   31.408748] vmscan: [->:shrink_lruvec:5958] 0 32
[   31.408935] vmscan: [->:shrink_lruvec:5958] 0 32
[   31.409120] vmscan: [->:shrink_lruvec:5958] 0 32
[   31.410510] systemd-coredum (1552) used greatest stack depth: 12880 bytes left
[   31.588199] Read-error on swap-device (8:64:131160)
[   31.590922] a.out (1549) used greatest stack depth: 11224 bytes left
➜  ~ dmesg | grep try_charge_memcg
[   31.221360] [->:try_charge_memcg:2743]
```

这，是不是有点没啥关联?
```txt
#0  martins3 () at mm/memcontrol.c:2633
#1  0xffffffff82122868 in try_charge_memcg (memcg=memcg@entry=0xffff88810632e000, gfp_mask=gfp_mask@entry=1843402, nr_pages=512) at mm/memcontrol.c:2687
#2  0xffffffff813a1b3d in try_charge (nr_pages=512, gfp_mask=1843402, memcg=0xffff88810632e000) at mm/memcontrol.c:2845
#3  charge_memcg (folio=folio@entry=0xffffea0004c10000, memcg=memcg@entry=0xffff88810632e000, gfp=gfp@entry=1843402) at mm/memcontrol.c:6962
#4  0xffffffff813a3438 in __mem_cgroup_charge (folio=0xffffea0004c10000, mm=<optimized out>, gfp=gfp@entry=1843402) at mm/memcontrol.c:6983
#5  0xffffffff8138a7af in mem_cgroup_charge (gfp=1843402, mm=<optimized out>, folio=<optimized out>) at ./include/linux/memcontrol.h:671
#6  __do_huge_pmd_anonymous_page (gfp=1843402, page=0xffffea0004c10000, vmf=0xffffc90043187df8) at mm/huge_memory.c:662
#7  do_huge_pmd_anonymous_page (vmf=vmf@entry=0xffffc90043187df8) at mm/huge_memory.c:837
#8  0xffffffff8132a8ab in create_huge_pmd (vmf=0xffffc90043187df8) at mm/memory.c:4790
#9  __handle_mm_fault (vma=vma@entry=0xffff8881134f7390, address=address@entry=139697269506048, flags=flags@entry=597) at mm/memory.c:5043
#10 0xffffffff8132b0a4 in handle_mm_fault (vma=0xffff8881134f7390, address=address@entry=139697269506048, flags=<optimized out>, flags@entry=597, regs=regs@entry=0xffffc90043187f58) at mm/memory.c:5219
#11 0xffffffff8110d937 in do_user_addr_fault (regs=regs@entry=0xffffc90043187f58, error_code=error_code@entry=6, address=address@entry=139697269506048) at arch/x86/mm/fault.c:1428
#12 0xffffffff8218b6d6 in handle_page_fault (address=139697269506048, error_code=6, regs=0xffffc90043187f58) at arch/x86/mm/fault.c:1519
#13 exc_page_fault (regs=0xffffc90043187f58, error_code=6) at arch/x86/mm/fault.c:1575
#14 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
是的，但是纯粹就是巧合而已，当超过 100M 之后，恰好使用 transparent hugepage 了，实际上。

如果将 wbt 关掉，那么我们就可以看到 memory 总是可以下写的。

### 3 如果只是单纯的写入过慢，应该存在不一样的效果

1. 难道说，为什么 a.out 的代码段成为读取的内容了 ?
  - 这只是随机现象中的一种而已

### 将几个 printk 去掉之后，几乎立刻是 OOM 的

这个才是正常的，在
- try_charge_memcg
  - try_to_free_mem_cgroup_pages 返回值是 0，因为无法 reclaim 任何 page
  - mem_cgroup_oom : 出现 oom ，然后进程被 kill 掉
```txt
[  180.874870] CPU: 0 PID: 1791 Comm: a.out Not tainted 6.2.0-rc1-dirty #95
[  180.875115] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[  180.875361] Call Trace:
[  180.875455]  <TASK>
[  180.875538]  dump_stack_lvl+0x38/0x4c
[  180.875678]  dump_header+0x45/0x200
[  180.875811]  oom_kill_process.cold+0xb/0x10
[  180.875971]  out_of_memory+0x1a5/0x4f0
[  180.876113]  mem_cgroup_out_of_memory+0x131/0x150
[  180.876291]  try_charge_memcg+0x744/0x820
[  180.876442]  charge_memcg+0x2d/0xa0
[  180.876574]  __mem_cgroup_charge+0x28/0x80
[  180.876727]  __filemap_add_folio+0x355/0x430
[  180.876887]  ? __pfx_workingset_update_node+0x10/0x10
[  180.877075]  filemap_add_folio+0x36/0xa0
[  180.877222]  __filemap_get_folio+0x1fc/0x330
[  180.877385]  filemap_fault+0x150/0xa00
[  180.877527]  ? filemap_map_pages+0x12f/0x620
[  180.877687]  __do_fault+0x2c/0xb0
[  180.877813]  do_fault+0x1e1/0x590
[  180.877940]  __handle_mm_fault+0x5fa/0x12a0
[  180.878096]  handle_mm_fault+0xe4/0x2c0
[  180.878240]  do_user_addr_fault+0x1c7/0x670
[  180.878398]  ? kvm_read_and_reset_apf_flags+0x49/0x60
[  180.878585]  exc_page_fault+0x66/0x150
[  180.878725]  asm_exc_page_fault+0x26/0x30
[  180.878876] RIP: 0033:0x4012cc
[  180.878995] Code: Unable to access opcode bytes at 0x4012a2.
[  180.879203] RSP: 002b:00007ffd8431e4b0 EFLAGS: 00010206
[  180.879398] RAX: 00007fa6273e2000 RBX: 0000000000000000 RCX: 0000000000000000
[  180.879659] RDX: 0000000006422000 RSI: 0000000000b57910 RDI: 00007ffd8431df50
[  180.879919] RBP: 00007ffd8431e510 R08: 0000000000000000 R09: 0000000000000064
[  180.880180] R10: 0000000000000000 R11: 0000000000000246 R12: 00007ffd8431e628
[  180.880442] R13: 0000000000401192 R14: 0000000000403e08 R15: 00007fa633e09000
[  180.880703]  </TASK>
```

## 实际上 wbt 只是让时间推迟了而已，最后，会按照完全相同的剧本死掉

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
