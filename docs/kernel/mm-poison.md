# poison

When we enable CONFIG_PAGE_POISONING, the pages are filled with poison byte pattern after free_pages() and verifying the poison patterns before alloc_pages(). [^1]

```txt
#0  __kernel_poison_pages (page=page@entry=0xffffea0006b7a580, n=n@entry=1) at mm/page_poison.c:38
#1  0xffffffff812fad96 in kernel_poison_pages (numpages=1, page=0xffffea0006b7a580) at include/linux/mm.h:3116
#2  free_pages_prepare (fpi_flags=0, check_free=false, order=0, page=0xffffea0006b7a580, page@entry=0x1 <fixed_percpu_data+1>) at mm/page_alloc.c:1469
#3  free_pcp_prepare (page=page@entry=0xffffea0006b7a580, order=order@entry=0) at mm/page_alloc.c:1532
#4  0xffffffff812fd130 in free_unref_page_prepare (order=0, pfn=1760918, page=0xffffea0006b7a580) at mm/page_alloc.c:3387
#5  free_unref_page (page=0xffffea0006b7a580, order=0) at mm/page_alloc.c:3483
#6  0xffffffff8132c356 in __unfreeze_partials (s=0xffff88810012d500, partial_slab=0x0 <fixed_percpu_data>) at mm/slub.c:2586
#7  0xffffffff81195847 in rcu_do_batch (rdp=0xffff888238a2c3c0) at kernel/rcu/tree.c:2250
#8  rcu_core () at kernel/rcu/tree.c:2510
#9  0xffffffff822000f3 in __do_softirq () at kernel/softirq.c:571
#10 0xffffffff81113eca in invoke_softirq () at kernel/softirq.c:445
#11 __irq_exit_rcu () at kernel/softirq.c:650
#12 0xffffffff811140a5 in irq_exit_rcu () at kernel/softirq.c:662
#13 0xffffffff81faea74 in sysvec_apic_timer_interrupt (regs=0xffffc90001c2bf58) at arch/x86/kernel/apic/apic.c:1107
#14 0xffffffff82000d06 in asm_sysvec_apic_timer_interrupt () at ./arch/x86/include/asm/idtentry.h:649
```

```txt
#0  0xffffffff81328e25 in check_poison_mem (bytes=<optimized out>, mem=<optimized out>, page=<optimized out>) at mm/page_poison.c:55
#1  unpoison_page (page=0xffffea000489f080) at mm/page_poison.c:88
#2  __kernel_unpoison_pages (page=page@entry=0xffffea000489f080, n=n@entry=1) at mm/page_poison.c:98
#3  0xffffffff812ff11d in kernel_unpoison_pages (numpages=1, page=0xffffea000489f080) at include/linux/mm.h:3121
#4  post_alloc_hook (gfp_flags=2592, order=0, page=0xffffea000489f080) at mm/page_alloc.c:2493
#5  prep_new_page (alloc_flags=2305, gfp_flags=2592, order=0, page=0xffffea000489f080) at mm/page_alloc.c:2539
#6  get_page_from_freelist (gfp_mask=2592, order=order@entry=0, alloc_flags=2305, ac=ac@entry=0xffffc900001c0da8) at mm/page_alloc.c:4288
#7  0xffffffff8130059d in __alloc_pages (gfp=gfp@entry=2592, order=order@entry=0, preferred_nid=<optimized out>, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/page_alloc.c:5555
#8  0xffffffff81c1abaa in __alloc_pages_node (order=0, gfp_mask=2592, nid=<optimized out>) at include/linux/gfp.h:223
#9  alloc_pages_node (order=0, gfp_mask=2592, nid=<optimized out>) at include/linux/gfp.h:246
#10 page_frag_alloc_1k (gfp=2592, nc=0xffff888238a2a5b8) at net/core/skbuff.c:163
#11 __napi_alloc_skb (napi=napi@entry=0xffff8880249e8c10, len=1024, len@entry=122, gfp_mask=gfp_mask@entry=2592) at net/core/skbuff.c:678
#12 0xffffffff81aa90ed in napi_alloc_skb (length=122, napi=0xffff8880249e8c10) at include/linux/skbuff.h:3212
#13 e1000_clean_rx_irq (rx_ring=0xffff888121c8f400, work_done=0xffffc900001c0ecc, work_to_do=64) at drivers/net/ethernet/intel/e1000e/netdev.c:1008
#14 0xffffffff81ab2bd1 in e1000e_poll (napi=0xffff8880249e8c10, budget=64) at drivers/net/ethernet/intel/e1000e/netdev.c:2682
#15 0xffffffff81c3cb77 in __napi_poll (n=n@entry=0xffff8880249e8c10, repoll=repoll@entry=0xffffc900001c0f37) at net/core/dev.c:6498
#16 0xffffffff81c3d090 in napi_poll (repoll=0xffffc900001c0f48, n=0xffff8880249e8c10) at net/core/dev.c:6565
#17 net_rx_action (h=<optimized out>) at net/core/dev.c:6676
#18 0xffffffff822000f3 in __do_softirq () at kernel/softirq.c:571
#19 0xffffffff81113eca in invoke_softirq () at kernel/softirq.c:445
#20 __irq_exit_rcu () at kernel/softirq.c:650
#21 0xffffffff81fac668 in common_interrupt (regs=0xffffc900000ffe38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc900001c1018
```

## MADV_HWPOISON

## init_on_free 不是和 poison 是冲突的吗

[^1]: https://stackoverflow.com/questions/22717661/linux-page-poisoning
