# vmscan

## 问题
- lruvec 是基于 Node 的，会出现一个 Node 开始 swap 而另一个 Node 上还是内存很多的情况吗?
- 对于 dirty memory 和 clean mmeory 的 scan 应该是不同的吧
- lruvec 一共有多少个，处理的时候的规则是怎样的
- 如果整个分配都是在 zone 上进行，那么为什么 reclaim 机制又是在 node 上完成的
- [ ] 按道理来说，所有的 page 分配之后，需要立刻加入的 lru list 中

## shrink 的过程中 isolate_lru_folios 是什么作用

## lruvec
- [ ] 全局搜索一下
- [ ] 我记得 lrulist 是 per zone 的，例如每次都是 shrink zone 的
- lruvec 在 pg_data_t 中间的作用是什么?

通过 lru_add_drain_cpu 将 cpu_fbatches 的内容加入到 lruvec 中。

## kswapd
- 描述主动和被动触发的流程
- [ ] 每一个 numa 一个 kswapd，确认下

## [ ] mem_cgroup_lruvec

## pagevec

似乎已经被取消了:
```c
struct pagevec {
	unsigned long nr;
	unsigned long cold;
	struct page *pages[PAGEVEC_SIZE];
};
```

似乎使用这个作为替代的:
```c
/*
 * The following folio batches are grouped together because they are protected
 * by disabling preemption (and interrupts remain enabled).
 */
struct cpu_fbatches {
	local_lock_t lock;
	struct folio_batch lru_add;
	struct folio_batch lru_deactivate_file;
	struct folio_batch lru_deactivate;
	struct folio_batch lru_lazyfree;
#ifdef CONFIG_SMP
	struct folio_batch activate;
#endif
};
```

- lru_cache_add
  - folio_batch_add_and_move 参数为 lru_add_fn
    - folio_batch_move_lru
      - 调用 hook : lru_add_fn
        - lruvec_add_folio
          - 将 lurvec 加入到

- [ ] 通过 folio 找到 lruvec 的方法

- lru_cache_add_inactive_or_unevictable
- add_to_page_cache_lru


Usually a page is first regarded as inactive and has to earn its merits to be considered active. However, a
selected number of procedures have a high opinion of their pages and invoke `lru_cache_add_active` to
place pages directly on the zone’s active list:
1. `read_swap_cache_async` from mm/swap_state.c; this reads pages from the swap cache.
2. The page fault handlers `__do_fault`, `do_anonymous_page`, `do_wp_page`, and `do_no_page`; these
are implemented in mm/memory.c.

## 转换的方法 : mark_page_accessed 和 folio_check_references
两个 bit : PG_active 和 PG_referenced

- mark_page_accessed 是 folio_mark_accessed 的封装
  - [ ] 为什么 kvm 要调用这个，
- folio_check_references
  - 在扫描 inactive list 的时候调用，返回决定该 page 的四个状态

总体来说，mark_page_accessed / folio_mark_accessed 是内核的执行过程中，将页进行标记，说明其一定是活跃的，
但是一个页被读写之后，分配之后，可能被用户程序长时间的访问，这个就要靠 folio_check_references。

这个逻辑好奇怪啊，为什么在 exit 的时候还需要来 mark_page_accessed 一下
```txt
#0  mark_page_accessed (page=page@entry=0xffffea0005a014c0) at mm/folio-compat.c:50
#1  0xffffffff812d59a3 in zap_pte_range (details=0xffffc90001a93d00, end=<optimized out>, addr=94723981115392, pmd=<optimized out>, vma=<optimized out>, tlb=0xffffc90001a93df0) at mm/memory.c:1453
#2  zap_pmd_range (pud=<optimized out>, pud=<optimized out>, details=<optimized out>, end=<optimized out>, addr=94723981115392, vma=<optimized out>, tlb=<optimized out>) at mm/memory.c:1577
#3  zap_pud_range (p4d=<optimized out>, p4d=<optimized out>, details=0xffffc90001a93d00, end=<optimized out>, addr=94723981115392, vma=<optimized out>, tlb=0xffffc90001a93df0) at mm/memory.c:1606
#4  zap_p4d_range (details=0xffffc90001a93d00, end=<optimized out>, addr=94723981115392, pgd=<optimized out>, vma=<optimized out>, tlb=0xffffc90001a93df0) at mm/memory.c:1627
#5  unmap_page_range (tlb=tlb@entry=0xffffc90001a93df0, vma=<optimized out>, addr=94723981115392, end=<optimized out>, details=details@entry=0xffffc90001a93d00) at mm/memory.c:1648
#6  0xffffffff812d6478 in unmap_single_vma (tlb=tlb@entry=0xffffc90001a93df0, vma=<optimized out>, start_addr=start_addr@entry=0, end_addr=end_addr@entry=18446744073709551615, details=details@entry=0xffffc90001a93d00) at mm/memory.c:1694
#7  0xffffffff812d698c in unmap_vmas (tlb=tlb@entry=0xffffc90001a93df0, mt=mt@entry=0xffff888162ce5d80, vma=<optimized out>, vma@entry=0xffff888166099130, start_addr=start_addr@entry=0, end_addr=end_addr@entry=18446744073709551615) at mm/memory.c:1733
#8  0xffffffff812e5125 in exit_mmap (mm=mm@entry=0xffff888162ce5d80) at mm/mmap.c:3087
#9  0xffffffff81109891 in __mmput (mm=0xffff888162ce5d80) at kernel/fork.c:1185
#10 0xffffffff8110997e in mmput (mm=<optimized out>) at kernel/fork.c:1207
#11 0xffffffff811126a9 in exit_mm () at kernel/exit.c:516
#12 do_exit (code=code@entry=32512) at kernel/exit.c:807
#13 0xffffffff81112f28 in do_group_exit (exit_code=32512) at kernel/exit.c:950
#14 0xffffffff81112f8f in __do_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:961
#15 __se_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:959
#16 __x64_sys_exit_group (regs=<optimized out>) at kernel/exit.c:959
#17 0xffffffff81fa3bdb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001a93f58) at arch/x86/entry/common.c:50
#18 do_syscall_64 (regs=0xffffc90001a93f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#19 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#20 0x0000000000000000 in ?? ()
```

一般来说，是这个调用路径
```txt
#0  touch_buffer (bh=<optimized out>) at fs/buffer.c:62
#1  __find_get_block (bdev=0xffff888161ba9200, block=11534947, size=<optimized out>) at fs/buffer.c:1311
#2  0xffffffff813adb3f in __getblk_gfp (bdev=0xffff888161ba9200, block=block@entry=11534947, size=4096, gfp=gfp@entry=8) at fs/buffer.c:1329
#3  0xffffffff8142408c in sb_getblk (block=11534947, sb=0xffff8881215e4800) at include/linux/buffer_head.h:356
#4  __ext4_get_inode_loc (sb=0xffff8881215e4800, ino=2892864, inode=inode@entry=0xffff888166f24610, iloc=iloc@entry=0xffffc9000175bcb0, ret_block=ret_block@entry=0xffffc9000175bc58) at fs/ext4/inode.c:4479
#5  0xffffffff81426389 in ext4_get_inode_loc (inode=inode@entry=0xffff888166f24610, iloc=iloc@entry=0xffffc9000175bcb0) at fs/ext4/inode.c:4607
#6  0xffffffff81427d96 in ext4_reserve_inode_write (handle=handle@entry=0xffff888166c415e8, inode=inode@entry=0xffff888166f24610, iloc=iloc@entry=0xffffc9000175bcb0) at fs/ext4/inode.c:5804
#7  0xffffffff81428012 in __ext4_mark_inode_dirty (handle=handle@entry=0xffff888166c415e8, inode=inode@entry=0xffff888166f24610, func=func@entry=0xffffffff8244ceb0 <__func__.36> "ext4_ext_tree_init", line=line@entry=879) at fs/ext4/inode.c:5973
#8  0xffffffff8140afd7 in ext4_ext_tree_init (handle=handle@entry=0xffff888166c415e8, inode=inode@entry=0xffff888166f24610) at fs/ext4/extents.c:879
#9  0xffffffff8141b797 in __ext4_new_inode (mnt_userns=mnt_userns@entry=0xffffffff82a627c0 <init_user_ns>, handle=0xffff888166c415e8, handle@entry=0x0 <fixed_percpu_data>, dir=dir@entry=0xffff8881240ee1a0, mode=mode@entry=41471, qstr=qstr@entry=0xffff888166debb60, goal=<optimized out>, goal@entry=0, owner=<optimized out>, i_flags=<optimized out>, handle_type=<optimized out>, line_no=<optimized out>, nblocks=<optimized out>) at fs/ext4/ialloc.c:1333
#10 0xffffffff81443977 in ext4_symlink (mnt_userns=0xffffffff82a627c0 <init_user_ns>, dir=0xffff8881240ee1a0, dentry=<optimized out>, symname=<optimized out>) at fs/ext4/namei.c:3361
#11 0xffffffff813745ac in vfs_symlink (oldname=0xffff888162cd8020 "/pid-1092/host-localhost.localdomain", dentry=0xffff888166debb40, dir=0xffff8881240ee1a0, mnt_userns=0xffffffff82a627c0 <init_user_ns>) at fs/namei.c:4400
#12 vfs_symlink (mnt_userns=0xffffffff82a627c0 <init_user_ns>, dir=0xffff8881240ee1a0, dentry=0xffff888166debb40, oldname=0xffff888162cd8020 "/pid-1092/host-localhost.localdomain") at fs/namei.c:4385
#13 0xffffffff8137a0f5 in do_symlinkat (from=0xffff888162cd8000, newdfd=newdfd@entry=-100, to=to@entry=0xffff888162cde000) at fs/namei.c:4429
#14 0xffffffff8137a293 in __do_sys_symlink (newname=<optimized out>, oldname=0x5626a6d08830 "/pid-1092/host-localhost.localdomain") at fs/namei.c:4451
#15 __se_sys_symlink (newname=<optimized out>, oldname=<optimized out>) at fs/namei.c:4449
#16 __x64_sys_symlink (regs=<optimized out>) at fs/namei.c:4449
#17 0xffffffff81fa3bdb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9000175bf58) at arch/x86/entry/common.c:50
#18 do_syscall_64 (regs=0xffffc9000175bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#19 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## 确认所有的页面都是加入到 lru 中的

是的，例如这个例子
```txt
#0  folio_add_lru (folio=0xffffea00059a5040) at arch/x86/include/asm/atomic.h:95
#1  0xffffffff812a7926 in folio_add_lru_vma (folio=<optimized out>, vma=<optimized out>) at mm/swap.c:554
#2  0xffffffff812dd086 in do_anonymous_page (vmf=0xffffc9000175bdf8) at mm/memory.c:4154
#3  handle_pte_fault (vmf=0xffffc9000175bdf8) at mm/memory.c:4953
#4  __handle_mm_fault (vma=vma@entry=0xffff888125ae3000, address=address@entry=139945414307856, flags=flags@entry=597) at mm/memory.c:5097
#5  0xffffffff812dd600 in handle_mm_fault (vma=0xffff888125ae3000, address=address@entry=139945414307856, flags=flags@entry=597, regs=regs@entry=0xffffc9000175bf58) at mm/memory.c:5218
#6  0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc9000175bf58, error_code=error_code@entry=6, address=address@entry=139945414307856) at arch/x86/mm/fault.c:1428
#7  0xffffffff81fa7e12 in handle_page_fault (address=139945414307856, error_code=6, regs=0xffffc9000175bf58) at arch/x86/mm/fault.c:1519
#8  exc_page_fault (regs=0xffffc9000175bf58, error_code=6) at arch/x86/mm/fault.c:1575
#9  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#10 0x00005626a6dea0e0 in ?? ()
#11 0x0000000000000000 in ?? ()
```

## [ ] swappiness 的控制

## 多个 lruvec 是如何排序的

各自排序各自的情况，如果一个 node 上很忙，你自己最好不要过来分配，这个 mempolicy 的事情
