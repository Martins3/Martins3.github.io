- [ ] for_each_zone_zonelist_nodemask
- highatomic 是做什么意思的
- [ ] tools/mm 文件夹的内容
- [ ] 检查一下 zero page 和 swap 的代码，应该是 zero page 不会被换出的。
- [ ] 如果是 private 映射一个文件，其修改应该最后也是写入到 swap 中的吧
  - 应该是的，但是需要验证
- [What Every Programmer Should Know About Memory](https://people.freebsd.org/~lstewart/articles/cpumemory.pdf)
  - 总体结论，还是正确的
  - https://stackoverflow.com/questions/8126311/how-much-of-what-every-programmer-should-know-about-memory-is-still-valid

- numa 的支持
  - numa remote access 是如何确定的
  - numa_balancing 是如何实现的

- [ ] slab
  - [ ] negative dentry 如何处理了的
- [ ] numa
- [ ] 几个压缩技术 zfold 之类的
- [ ] gup

## mseal
https://lwn.net/Articles/958438/

## 似乎 numastat -p 的结果是错误的

```txt
stress-ng --vm-bytes 2000M --vm-keep -m 1

➜  ~ numastat  1988

Per-node process memory usage (in MBs) for PID 1988 (stress-ng)
                           Node 0          Node 1           Total
                  --------------- --------------- ---------------
Huge                         0.00            0.00            0.00
Heap                         0.00            0.04            0.04
Stack                        0.00            0.02            0.02
Private                      2.32            3.72            6.05
----------------  --------------- --------------- ---------------
Total                        2.32            3.78            6.10
➜  ~
```
- 而且，这个同时导致了一个问题，migrate 1988 0 1 几乎是瞬间完成，无论正反过来。
- 而且，numastat -m 显示占用的内存的位置没有发生变化。

是对于 stress-ng 理解有什么问题吗?
- cgroup v2 中存在 writeback 吗?

```c
enum migratetype {
	MIGRATE_UNMOVABLE,
	MIGRATE_MOVABLE,
	MIGRATE_RECLAIMABLE,
	MIGRATE_PCPTYPES,	/* the number of types on the pcp lists */
	MIGRATE_HIGHATOMIC = MIGRATE_PCPTYPES,
#ifdef CONFIG_CMA
	/*
	 * MIGRATE_CMA migration type is designed to mimic the way
	 * ZONE_MOVABLE works.  Only movable pages can be allocated
	 * from MIGRATE_CMA pageblocks and page allocator never
	 * implicitly change migration type of MIGRATE_CMA pageblock.
	 *
	 * The way to use it is to change migratetype of a range of
	 * pageblocks to MIGRATE_CMA which can be done by
	 * __free_pageblock_cma() function.
	 */
	MIGRATE_CMA,
#endif
#ifdef CONFIG_MEMORY_ISOLATION
	MIGRATE_ISOLATE,	/* can't allocate from here */
#endif
	MIGRATE_TYPES
};
```
- MIGRATE_RECLAIMABLE : 几乎没有什么引用的位置，而从 fgp flags 看，似乎只有 slub 才是 reclaimable 的
  - 而 page cache 是 unremovable 的
  - 但是这怎么可能?

## memory 中的代办
- vmmem hugeapge : page struct
- vmscan
- migration
- ksm
- compaction
- hw poison 机制
- shmem
- damon
- memory hotplug
- memfd
- folio
- transparent hugepage

- 感觉正是 compaction 导致了问题的复杂
  - compaction 导致了 migraion 的出现
  - compaction 的守护进程和 fskj


## 读了一部分，但是没有读完，大受震撼
- Documentation/vm/unevictable-lru.rst

## 使用 qemu 测试一下，memory 的 hotplug 结果

## 所有 zone 的 struct page 都是放到一起的吗?

如果不是，是出于什么考虑?

如果是，是因为存在什么条件?


## dirty pipe 的漏洞
https://github.com/r1is/CVE-2022-0847

- vmstat.c 是做什么的? vmstat_shepherd

- 问一个问题: pg_data_t 中间 pg 是什么？ ask the stackoverflow


- zap_page_range 为什么会去调用 lru_add_drain，我的一生之敌啊
- online_pages 和内核启动过程中，应该是存在非常多相似之处

## 难道文件删除之后，其 page cache 不会被删除吗?

观察一个 512G 的机器上开启了虚拟机之后，这个虚拟机上最后创建的 qcow2 也就是大约 240G 左右,
但是可以观察到大约存在 230G 的缓存，但是这个虚拟机关机之后，这些缓存也是存在的。

```txt
/dev/nvme0n1    1.5T  270G  1.2T  19% /home/martins3/hack
.dotfiles on  feat [!] aarch64
🧀  free -h
               total        used        free      shared  buff/cache   available
Mem:           502Gi        19Gi       230Gi        58Mi       252Gi       480Gi
Swap:          251Gi          0B       251Gi
```
当然不会


## 是这个样子的吗？
https://stackoverflow.com/questions/18845857/what-does-anon-rss-and-total-vm-mean

rss 是真的使用的内存，而 total-vm 只是映射的


## DRAM
https://arxiv.org/pdf/1902.07609.pdf
Understanding the Interactions of Workloads and DRAM Types: A Comprehensive Experimental Study

## 什么时候会进行 vma 的拆分

vma 的合并是自动进行的吗?

## (mem) map 的类型划分到底如何定义？
private shared
file-based anonosy

是的，通过 cat /proc/vmstat 重新理解下吧


## 实际上，echo 3 并不可以让所有的 KReclaimable 清理掉
```txt
KReclaimable:      38744 kB
```
是因为这也是存在 dirty 之分吗?


## min_free_kbytes 和 /proc/zone 中的 min 是出入的

```txt
cat /proc/sys/vm/min_free_kbytes
67584

cat /proc/zoneinfo | grep min
      min      3
      min      447
      min      22076
      min      0
      min      0
```


## pat
- https://www.kernel.org/doc/Documentation/x86/pat.txt

pat 一直没搞清楚，导致我们现在始终无法理解为什么?


## filemap_fault 中
这里为什么判断 fpin 之前还是首先需要执行一下 filemap_read_folio
```c
page_not_uptodate:
	/*
	 * Umm, take care of errors if the page isn't up-to-date.
	 * Try to re-read it _once_. We do this synchronously,
	 * because there really aren't any performance issues here
	 * and we need to check for errors.
	 */
	fpin = maybe_unlock_mmap_for_io(vmf, fpin);
	error = filemap_read_folio(file, mapping->a_ops->read_folio, folio);
	if (fpin)
		goto out_retry;
	folio_put(folio);
```
也许是疏忽，也许是存在非常深入的考虑，这个量类似的地方不少。

## 这个选项是做什么的?
CONFIG_KVM_MMU_AUDIT

## (mem) lru cache 到底是什么 cache

#### (mem) node_data 初始化细节是什么

#### (mem) 物理映射方法
1. 找到 page frame 和 struct page 通过地址差互相访问的代码
2. 既然不同的用户进程之间的虚拟地址空间都是可以相互重叠的，那么为什么用户和内核的虚拟地址空间就是不能互相重叠的
3. 线性地址空间
    1. 如何初始化的线性地址空间 : 将预定义的 pg table 进行填充，之后所有的进程全部从其中 fork
    2. linear space 带来的结果 : 在内核空间中，当持有了其物理地址，那么可以知道其虚拟地址。物理地址的差值等于虚拟地址的差值。
    3. 什么机制的实现必须依赖于 linear space ? 找到哪一个实现地址宏的使用的位置即可!
4. fork 拷贝 pg table，到底是拷贝整个树，还是仅仅拷贝 dir 的 2k 地址。如果是拷贝了整个树，那么如何确定谁的内容是谁的
5. fork 拷贝 dir, 内核地址空间的映射有意义吗? 反正用户无法访问!
6. 地址空间的保护靠什么实现的, pte 上面的 flag 吗，寄存器中间的标志位和预设的虚拟地址空间阈值?
8. 通过 linear mapping 的确可以知道虚拟地址对应的物理地址，所获取的物理地址具体数值其实没有任何意义的吧 ? 就算获取了之后，没有什么意义啊!
    1. 当没有线性映射的时候，那么，通过

9. 物理地址到虚拟地址之间的切换操作是什么?

11. 内核的虚拟地址空间必须映射整个物理内存，否则，buddy System 无法管理内存，那么它是如何将整个物理内存映射的(仅仅考虑 64 位)
12. 从目前的分析来说，线性映射显然是需要的，内核需要连续的物理内存来提升性能，但是他应该是透明的才对啊!
    1. 如果这一种想法正确的话，那么内核中间就不该有 虚实地址转换函数 存在啊!
13. 一种说法是 x86 的线性地址只是为了兼容而已，在 arm 中间这些蛇皮限制是不存在的。
14. 如果没有线性地址空间，是不是意味着没有办法进行 context switch 中间修改 pgdir 的操作 ?
    1. cr3 中间存放的是物理地址还是虚拟地址, 显然是物理地址

15. 需要 phys_to_virt and it's reverse 的唯一原因是: 我们需要物理地址。几乎没有需要物理地址的时间，但是在整个 pgtable 填充的过程中间，
填充的内容都是物理地址，同时，page_alloc 返回空间是虚拟地址，所以需要 virt_to_phys 的数值

#### (mem) lru cache 是什么东西？

#### (mem) 内核是如何写 pde 和 pte 的, 在 page_fault 中间，当每一级都不存在，将整个 page walk 逐级填充的过程 ?
> 当不采用 linear address space 的时候

内核读写操作数都是虚拟地址，持有物理地址无法进行修改内存。获取了 page walk 的第一级，进而获取 page walk 第二级是没有办法的，

由于 page_fault 的整个过程都是软件处理的，如果不是 linear address space ,　那么显然 page walk 的填充过程无法完成。


## 这里有 memory controller 的介绍，这不冲了
https://safari.ethz.ch/architecture/fall2023/doku.php?id=schedule


## 这个是做什么的?
/sys/module/page_reporting

居然是一个模块

但是 lsmod | grep page 中找不到这个。

## 加急看看
https://huonw.github.io/blog/2024/08/async-hazard-mmap/



其实际含义是什么?

## 测试一下 CONFIG_TMPFS_QUOTA 的实现

## SMAP 和 sgx 关系是什么?

```c
/*
 * 参考 io_uring/io_uring.c 中的 io_mem_alloc
 */
static void *mem_alloc(size_t size)
{
	gfp_t gfp = GFP_KERNEL_ACCOUNT | __GFP_ZERO | __GFP_NOWARN | __GFP_COMP;
	void *ret;

	ret = (void *)__get_free_pages(gfp, get_order(size));
	if (ret)
		return ret;
	return ERR_PTR(-ENOMEM);
}
```

## 如果想要实现 virtiofsd ，必须使用

mem-path=/dev/shm/$guest_id 参数的形式吗?

如果想要 share memory 给其他 process，真的没有什么好办法吗?
只能通过 /dev/shm 吗?

## 为什么 mem-path=/dev/shm/$guest_id

cgroup move 之后

## 如何理解这个问题
```c
static int shmem_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct inode *inode = file_inode(file);
	struct shmem_inode_info *info = SHMEM_I(inode);
	int ret;

	ret = seal_check_write(info->seals, vma);
	if (ret)
		return ret;

	/* arm64 - allow memory tagging on RAM-based files */
	vm_flags_set(vma, VM_MTE_ALLOWED);

	file_accessed(file);
	/* This is anonymous shared memory if it is unlinked at the time of mmap */
	if (inode->i_nlink)
		vma->vm_ops = &shmem_vm_ops;
	else
		vma->vm_ops = &shmem_anon_vm_ops;
	return 0;
}
```

## pcplist 是做什么的 ?


## 如何理解这个 flags
PageUnevictable

## 从 cgroup 的角度整理一下
例如:
vmscan 和 cgroup 和配合使用，page-writeback 和 cgroup 的配合使用

## 这个是 typo 吗?
check_vma_flags 中

```c
			/*
			 * We used to let the write,force case do COW in a
			 * VM_MAYWRITE VM_SHARED !VM_WRITE vma, so ptrace could
			 * set a breakpoint in a read-only mapping of an
			 * executable, without corrupting the file (yet only
			 * when that file had been opened for writing!).
			 * Anon pages in shared mappings are surprising: now
			 * just reject it.
			 */
			if (!is_cow_mapping(vm_flags))
				return -EFAULT;
```
应该是 VM_MAYWRITE !VM_SHARED !VM_WRITE

不过应该不太可能

## mseal
https://news.ycombinator.com/item?id=41945894

## 整理一下
- [ ] mmdrop()
- [ ] mmgrab()
- [ ] vm_normal_page()
  - [ ] why some page can work without `struct page`
  - [ ] check comments above it
  - [ ] do_wp_page's reference
- [ ] https://www.kernel.org/doc/html/latest/core-api/mm-api.html# : check the doc
- [ ] https://www.kernel.org/doc/gorman/html/understand/ : check the book
- [ ] access_ok()
- [ ] update_curr() scheduler_tick()
- [ ] redo hack linux kernel labs, so we can understand kobjs / sys / again
- [ ] https://kernelnewbies.org/FAQ

## 我在说什么来着 ?

> @todo 同样的，此处分析的内容缺少 anon 的分析，除非 shmem.c 中间的是用于 /tmp 的，所以似乎没有用于 anon 的
> 除非，mark_page_accessed 表示这次，这个东西是和 fs 打过交道的
> 所以，对于 anon 处理的地方在于 swap cache 中间吗 ?


> @todo 非常的怀疑，page_referenced 的调用，只有当发现其实在上一次调用 page_referenced 到此次，根本没有任何
> 对于该 page frame 的映射发生过访问，所以，决定下调。
> 而 mark_page_accessed 出现的位置表示 : 该 page 刚刚读入进来，如果此时换出，非常的不应该。

lruvec 中间的类型只有 : anon 以及 file 和 unevictable，都是映射而已。
    1. 如果是靠这个怀疑 io page cache 不在 reclaim 的机制之下，那么 generic_file_buffered_read 中间也是调用过 mark_page_accessed 的
    2. page-writeback 的功能只是表示 dirty 数量不要超过某一个阈值，和到底存在多少个 page 在内存中间没有关系


## shmem 的奇怪现象

将 qemu 的 memory 用 /shm/ ，
结果虚拟机关机了，还是占用内存的，还会被 swap 到
磁盘中。

如果将 /shm 的文件都删除掉，swap 中的内容立刻消失。

## 虚拟机的内存应该使用 transparent hugepage 吗?
1. balloon 的 page out 会导致 transparent hugepage 拆开吗?


## 使用 shmem 给虚拟机非

这样是没问题的:
```txt
			arg_mem_cpu+=" -object memory-backend-ram,id=mem0,size=$ramsize,prealloc=off,share=off "
```

这样是有问题的:
```txt
			arg_mem_cpu+=" -object memory-backend-file,id=mem0,size=$ramsize,mem-path=/dev/shm/qemu-$guest_id,share=on,discard-data=on"
```


为什么无法分配 40G 的空间:
```txt
[   96.380169][   C15] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[   96.380566][   C15] RIP: 0010:clear_page_erms+0x7/0x10
[   96.380834][   C15] Code: 48 89 47 38 48 8d 7f 40 75 d9 90 c3 cc cc cc cc 0f 1f 00 90 90 90 90 90
 90 90 90 90 90 90 90 90 90 90 90 b9 00 10 00 00 31 c0 <f3> aa c3 cc cc cc cc 66 90 90 90 90 90 90 9
0 90 90 90 90 90 90 90
[   96.381811][   C15] RSP: 0000:ffffb1e3c1d5fab0 EFLAGS: 00010246
[   96.382104][   C15] RAX: 0000000000000000 RBX: 0000000000000901 RCX: 0000000000001000
[   96.382529][   C15] RDX: ffffef4b62279500 RSI: 0000000000140dca RDI: ffff9e74c9e54000
[   96.382918][   C15] RBP: ffffef4b62279500 R08: 0000000000000000 R09: ffffef4b62279540
[   96.383311][   C15] R10: 00000000000098cf R11: 0000000000000000 R12: ffff9e7c7fff3c00
[   96.383717][   C15] R13: 0000000000000000 R14: ffff9e7c7fff3c00 R15: 0000000000000000
[   96.384125][   C15] FS:  00007f6b54329740(0000) GS:ffff9e7c3c580000(0000) knlGS:0000000000000000
[   96.384590][   C15] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[   96.384906][   C15] CR2: 00007f6882927000 CR3: 00000001063ba000 CR4: 0000000000750ee0
[   96.385287][   C15] PKRU: 55555554
[   96.385492][   C15] Call Trace:
[   96.385662][   C15]  <IRQ>
[   96.385803][   C15]  ? watchdog_timer_fn+0x1b4/0x220
[   96.386057][   C15]  ? __run_hrtimer+0x59/0x190
[   96.386295][   C15]  ? __hrtimer_run_queues+0x7c/0xe0
[   96.386585][   C15]  ? hrtimer_interrupt+0xf4/0x230
[   96.386832][   C15]  ? __sysvec_apic_timer_interrupt+0x46/0x110
[   96.387130][   C15]  ? sysvec_apic_timer_interrupt+0x69/0x90
[   96.387431][   C15]  </IRQ>
[   96.387575][   C15]  <TASK>
[   96.387717][   C15]  ? asm_sysvec_apic_timer_interrupt+0x16/0x20
[   96.388045][   C15]  ? clear_page_erms+0x7/0x10
[   96.388282][   C15]  prep_new_page+0x6d/0xd0
[   96.388532][   C15]  get_page_from_freelist+0x5c9/0x11c0
[   96.388816][   C15]  ? prep_new_page+0x6d/0xd0
[   96.389040][   C15]  __alloc_pages+0x197/0x1040
[   96.389271][   C15]  __folio_alloc+0x11/0x30
[   96.389514][   C15]  vma_alloc_folio+0xa0/0x370
[   96.389745][   C15]  alloc_anon_folio+0x26a/0x360
[   96.389985][   C15]  do_anonymous_page+0x26d/0x590
[   96.390230][   C15]  __handle_mm_fault+0x592/0x780
[   96.390499][   C15]  ? update_process_times+0x7f/0x90
[   96.390752][   C15]  handle_mm_fault+0x1ae/0x350
[   96.390986][   C15]  exc_page_fault+0x1ec/0x780
[   96.391221][   C15]  asm_exc_page_fault+0x22/0x30
[   96.391487][   C15] RIP: 0033:0x40679a
```

```txt
[   48.530229]  <TASK>
[   48.530403]  ? asm_sysvec_apic_timer_interrupt+0x1a/0x20
[   48.530755]  ? clear_page_erms+0xb/0x20
[   48.531037]  folio_zero_user+0xe7/0x180
[   48.531311]  do_huge_pmd_anonymous_page+0x214/0x7b0
[   48.531641]  handle_mm_fault+0x77e/0x1420
[   48.531932]  do_user_addr_fault+0x1f7/0x770
[   48.532221]  exc_page_fault+0x89/0x1f0
[   48.532489]  asm_exc_page_fault+0x26/0x30
[   48.532784] RIP: 0033:0x427aac
```
一般观察，分配到 30G 就有问题了。

1. 是由于 shmem 创建有限制导致的吗?
2. 可以看看如果直接在物理机中分配有这个问题吗?

## 看看这个
https://docs.kernel.org/admin-guide/mm/soft-dirty.html


## 的确，似乎也不麻烦，同时兼容 cgroup 和 numa
```c
/*
 * per-node information in memory controller.
 */
struct mem_cgroup_per_node {
```

可以在 cgroup 中测试跑一下 memory policy 看看。

## 两个有趣的补充
https://lwn.net/Articles/919143/

## merge this

| File                 | blank | comment | code | desc                                                                                                                                                         |
|----------------------|-------|---------|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| page_alloc.c         | 1119  | 1988    | 5012 | 分配 page frame                                                                                                                                              |
| memcontrol.c         | 943   | 1399    | 4362 | cgroup                                                                                                                                                       |
| slub.c               | 911   | 991     | 3974 |                                                                                                                                                              |
| hugetlb.c            | 640   | 1130    | 3158 | |
| memory.c             | 548   | 1136    | 3100 | pagefault 以及辅助函数，类似于 pagetable 各种操控函数 @todo 分析的清楚 pgfault 那么这个问题就没有什么意义了                                                  |
| shmem.c              | 478   | 558     | 3012 | 用于支持 tmpfs 和 sysv shm 的实现                                                                                                                            |
| slab.c               | 669   | 834     | 2974 | 暂时不用关注的内容，认为已经过时了。                                                                                                                         |
| swapfile.c           | 464   | 632     | 2682 | 应该是处理当 swap 的 base 是 file 而不是 partion 的情况!                                                                                                     |
| mmap.c               | 467   | 910     | 2332 | mmap munmap 实现                                                                                                                                             |
| vmscan.c             | 547   | 1364    | 2304 | 扫描确定需要回收页面，也就是 page reclaim 机制                                                                                                               |
| huge_memory.c        | 354   | 458     | 2118 | transparent hugetlb                                                                                                                                          |
| ksm.c                | 336   | 845     | 1998 | https://en.wikipedia.org/wiki/Kernel_same-page_merging ksm 将内容相同的 page 合并                                                                            |
| mempolicy.c          | 365   | 585     | 1928 | numa 系统分配内存选择 memory node                                                                                          |
| filemap.c            | 364   | 1052    | 1915 | 实现 page cache　也就是无处不在的 address_space                                                                                                              |
| migrate.c            | 395   | 715     | 1844 | 似乎处理的事情是将内存从一个 memory node 到另一个 memory node                                                                                                |
| zsmalloc.c           | 409   | 408     | 1723 | https://lwn.net/Articles/477067/ 新的分配器                                                                                                                  |
| vmalloc.c            | 383   | 678     | 1689 | vmalloc 实现物理页面和虚拟页面的建立映射                                                                                                                     |
| vmstat.c             | 325   | 337     | 1479 | 各种统计                                                                                                                                                     |
| page-writeback.c     | 329   | 1031    | 1472 | Contains functions related to writing back dirty pages at the  address_space level.                                                                          |
| khugepaged.c         | 243   | 289     | 1417 | transparent hugetlb 的守护进程                                                                                                                               |
| kmemleak.c           | 249   | 615     | 1260 | 识别内核中间内存泄露的工具                                                                                                                                   |
| memory_hotplug.c     | 301   | 374     | 1231 | 内核热插拔                                                                                                                                                   |
| nommu.c              | 288   | 431     | 1231 | nommu 处理没有 mmu 的情况                                                                                                                                    |
| compaction.c         | 338   | 605     | 1215 | 依托于 memory migration 实现减少 external fragmentation，但是 page_alloc.c 中间不是已经处理过这一个事情了吗 ?　各自侧重在于何处 ?                            |
| gup.c                | 192   | 511     | 1175 | 访问 user 的内存                                                                                                                |
| memory-failure.c     | 202   | 604     | 1119 | 处理内存控制器之类的底层错误的                                                                                                                               |
| rmap.c               | 243   | 626     | 1093 | 反向映射                                                                                                                                                     |
| memblock.c           | 246   | 593     | 1082 | 启动时候的内存管理                                                                                                                                           |
| slab_common.c        | 247   | 287     | 1027 | 各种 slab slob 分配器的公用函数                                                                                                                              |
| zswap.c              | 211   | 240     | 914  | 对于 swap 的内容进行压缩                                                                                                                                     |
| backing-dev.c        | 174   | 140     | 766  | 应该是用于实现写会到磁盘的内容，@todo 但是具体细节不知道是如何实现的! 如果 backing-dev.c 中间的内容真的是用来实现写回到磁盘，那么 fs/buffer.c 的功能是什么 ? |
| kasan/kasan.c        | 155   | 84      | 664  | https://github.com/google/kasan/wiki @todo 和 kmemleak.c 做的内容有什么不同呢?                                                                               |
| oom_kill.c           | 142   | 342     | 663  | oom killer 新手入门的好文档                                                                                                                                  |
| swap.c               | 125   | 315     | 598  | reclaim 机制中间，用于提供 pagevec 和 page 在 lrulist 之间倒腾的作用。                                                                                       |
| madvise.c            | 99    | 199     | 597  | Man madvice                                                                                                                                                  |
| swap_state.c         | 95    | 184     | 574  | swap cache ? 熟悉的内容.                                                                                                                                     |
| sparse.c             | 109   | 110     | 572  | 处理 sparese memory model                                                                                                                                    |
| bootmem.c            | 150   | 144     | 517  | memblock 的出现已经取消掉东西                                                                                                                                |
| list_lru.c           | 101   | 48      | 511  | 依赖于 MEMCG_KMEM，默认不使用，好像也不是                                                                                                                    |
| truncate.c           | 86    | 324     | 507  | 将 page 从 page cache 中间删除                                                                                                                               |
| mlock.c              | 109   | 267     | 492  | 用于 mlock 系统调用，将有的 vma 需要 lock，到底其上映射的 page 不会被清理掉                                                                                  |
| page_owner.c         | 124   | 59      | 456  | https://www.kernel.org/doc/html/latest/vm/page_owner.html 内核的检查错误机制，默认被 disable 掉                                                              |
| mprotect.c           | 90    | 94      | 452  | 处理 mprotect 系统调用，修改制定 VMA 的权限                                                                                                                  |
| slob.c               | 92    | 124     | 447  | 含有详细的分配器代码，如果开始逐行分析，是一个好入口!                                                                                                        |
| util.c               | 107   | 241     | 442  | 各种辅助函数，@todo 内容有点杂                                                                                                                               |
| mremap.c             | 82    | 115     | 440  | mremap 系统调用支持 !                                                                                                      |
| userfaultfd.c        | 70    | 130     | 387  | 用于支持虚拟机的，似乎 defconfig 模式没有将其编译进去                                                                                                        |
| kasan/kasan_init.c   | 80    | 41      | 368  |                                                                                                                                                              |
| slab.h               | 85    | 86      | 360  |                                                                                                                                                              |
| dmapool.c            | 63    | 112     | 358  | dmapool 用于支持 dma 的，                                                                                                                                    |
| kasan/report.c       | 74    | 34      | 343  |                                                                                                                                                              |
| zbud.c               | 64    | 236     | 336  | zbud is an special purpose allocator for storing compressed pages.                                                                                           |
| mempool.c            | 63    | 158     | 328  | 似乎就是基于 kmalloc 实现的内存池，@todo 所以为什么我们需要内存池啊!0                                                                                        |
| cma.c                | 80    | 124     | 320  | https://lwn.net/Articles/486301/ 有一个 allocator                                                                                                            |
| readahead.c          | 75    | 214     | 319  | 预取文件内容，现在发现只要是处理文件 IO 的函数，参数都有 address_space                                                                                       |
| page_io.c            | 48    | 71      | 315  | swap_readpage 和 swap_writepage 当使用 partion 的时候，调用 bdev_read_page 否则利用 file 对应的 aops 的 readpage/writepage                                   |
| hugetlb_cgroup.c     | 67    | 59      | 314  |                                                                                                                                                              |
| internal.h           | 77    | 159     | 294  | 貌似定义了一些会被 mm/ 下各种文件使用的辅助函数和定义，比如 alloc_context 和 compact_control                                                                 |
| frontswap.c          | 57    | 150     | 291  | https://www.kernel.org/doc/html/latest/vm/frontswap.html WTM 惊了，用各种其他介质替代 swap 机制默认使用的 swap                                               |
| highmem.c            | 68    | 141     | 276  |                                                                                                                                                              |
| page_ext.c           | 64    | 94      | 266  | https://en.wikipedia.org/wiki/Physical_Address_Extension 使用 page ext                                                                                       |
| pagewalk.c           | 44    | 62      | 252  | 各种 page walk                                                                                                                        |
| mmu_notifier.c       | 51    | 135     | 251  | 用于支持虚拟机实现 hardwared 处理                                                                                                                            |
| process_vm_access.c  | 46    | 84      | 246  | 支持系统调用 process_vm_readv 和 process_vm_writev @todo 但是不知道谁在使用这一个东西!                                                                       |
| nobootmem.c          | 72    | 133     | 240  | 感觉只是为了处理当 bootmem 被取消之后，为了保证 interface 的一致而已 ?                                                                                       |
| swap_slots.c         | 38    | 84      | 238  | 为了降低对于 swap_info 的锁的争用而是用的                                                                                                                    |
| vmpressure.c         | 48    | 189     | 235  |                                                                                                                                                              |
| memfd.c              | 51    | 61      | 233  | memfd_create syscall 的支持。                                                                                                                                |
| early_ioremap.c      | 48    | 28      | 227  | 功能应该和其名称类似，但是不知道 ioremap 为什么需要放到 mm/ 下面                                                                                             |
| workingset.c         | 41    | 283     | 215  | 我感觉又是用来支持 swap 的 active list 和 inactive list 的                                                                                                   |
| kasan/quarantine.c   | 54    | 70      | 204  |                                                                                                                                                              |
| sparse-vmemmap.c     | 32    | 34      | 197  | sparse 内存模型支持 pfn_to_page 之类简单实现。                                                                                                               |
| page_isolation.c     | 35    | 86      | 194  | 由于没有 CONFIG_MEMORY_IOSATION，所以并没有人调用该模块，应该还是处理 migration 相关的内容                                                                   |
| percpu-vm.c          | 47    | 141     | 191  | @todo                                                                                                                                                        |
| percpu-stats.c       | 41    | 41      | 154  |                                                                                                                                                              |
| percpu-internal.h    | 39    | 60      | 126  |                                                                                                                                                              |
| percpu-km.c          | 22    | 30      | 67   |                                                                                                                                                              |
| percpu.c             | 353   | 957     | 1478 | percpu allocator 就是实现 percpu 的                                                                                                                          |
| mincore.c            | 27    | 69      | 177  | syscall determine whether pages are resident in memory                                                                                                       |
| page_idle.c          | 29    | 33      | 176  | Idle Page Tracking : https://www.kernel.org/doc/html/latest/admin-guide/mm/idle_page_tracking.html                                                           |
| page_vma_mapped.c    | 23    | 64      | 168  | `page_vma_mapped_walk` Returns true if the page is mapped in the vma 唯一被使用非 static 函数，我感觉没有拆分成为新的 static 的必要                          |
| usercopy.c           | 43    | 98      | 165  | 各种检查函数用于支持 user 和 kernel 之间的拷贝                                                                                                               |
| cleancache.c         | 30    | 125     | 162  | https://www.kernel.org/doc/html/latest/vm/cleancache.html 还是和 page cache 有关的内容                                                                       |
| zpool.c              | 41    | 179     | 161  | This is a common frontend for memory storage pool implementations. Typically, this is used to store compressed memory.                                       |
| swap_cgroup.c        | 32    | 42      | 159  |                                                                                                                                                              |
| pgtable-generic.c    | 24    | 30      | 158  | 对于主流的架构来说，这一个文件是一个空文件，实现在具体的 arch 中间                                                                                           |
| cma_debug.c          | 42    | 7       | 153  |                                                                                                                                                              |
| mm_init.c            | 27    | 13      | 152  | 初始化 mm 子系统，使用@todo postcore_initcall 的                                                                                                             |
| debug.c              | 20    | 17      | 141  | dump_page 和 dump_vma                                                                                                                                        |
| frame_vector.c       | 17    | 86      | 136  | 提供一个函数给驱动使用，具体作用不清楚                                                                                                                       |
| fadvise.c            | 29    | 57      | 134  | Man fadvice 提前通知 kernel 将要访存                                                                                                                         |
| page_counter.c       | 35    | 103     | 128  | 配合 memcontrol 使用                                                                                                                                         |
| kasan/kasan.h        | 24    | 21      | 122  |                                                                                                                                                              |
| hwpoison-inject.c    | 25    | 17      | 98   | https://www.kernel.org/doc/html/latest/vm/hwpoison.html 辅助错误恢复                                                                                         |
| balloon_compaction.c | 20    | 62      | 95   | 和 balloon 机制相关                                                                                                                                          |
| memtest.c            | 16    | 3       | 94   | 测试物理内存是否可用                                                                                                                                         |
| Makefile             | 10    | 7       | 90   |                                                                                                                                                              |
| page_poison.c        | 22    | 13      | 89   | 也是为了处理物理内存错误                                                                                                                                     |
| interval_tree.c      | 16    | 8       | 88   | interval tree for `mapping->i_mmap`                                                                                                                          |
| gup_benchmark.c      | 21    | 0       | 82   |                                                                                                                                                              |
| mmzone.c             | 20    | 14      | 81   | management codes for pgdats, zones and page flagsmanagement codes for pgdats, zones and page flags                                                           |
| vmacache.c           | 18    | 21      | 79   | mm 访问 vma 依赖于此加速                                                                                                                                     |
| msync.c              | 4     | 30      | 74   | msync syscall 同步 The msync() function shall write all modified data to permanent storage locations                                                         |
| kmemleak-test.c      | 14    | 32      | 65   |                                                                                                                                                              |
| quicklist.c          | 18    | 22      | 63   |                                                                                                                                                              |
| failslab.c           | 14    | 2       | 51   |                                                                                                                                                              |
| debug_page_ref.c     | 8     | 1       | 46   |                                                                                                                                                              |
| mmu_context.c        | 7     | 20      | 37   | use_mm 和 unuse_mm 来切换到特定的 mm context,并不是非常清楚会有代码使用这种功能                                                                              |
| rodata_test.c        | 8     | 16      | 33   |                                                                                                                                                              |
| init-mm.c            | 3     | 11      | 26   | 定义用于初始化用途的 mm                                                                                                                                      |
| cma.h                | 4     | 1       | 21   |                                                                                                                                                              |
| kasan/Makefile       | 2     | 3       | 6    |                                                                                                                                                              |


## 当前的路线

1. SetPageSwapBacked 的作用
    1. unuse -> shmem_unuse -> shmem_writepage(shmem.md : 462)

2. PageDirty 和 PageUptodate 关系是什么，感觉 up to date 其实是 valid 的含义 !

3. `__test_set_page_writeback` 是不是相关的内容在 writeback dirty 相关的存在分析了。
    1. end_page_writeback 和 set_page_writeback 的关系是什么 ?

## 找到这些想法的证据
mmap() 的实现，如果多个 process 都是 map 到同一个文件 : 统一一下 mmap 和 pgfault 的对照关系。
1. 开始的时候注册
2. 之后利用注册的信息实现 pgfault，并且利用 page fault 的方法，找到之后，将该 page 注入到 vma 的 page table 上。
    1. 根据的 pgfault 的类型，可能会出现 unshared 写操作，那么就会进行拷贝之类的

5. filemap.c 到处都是使用 PageUptodate 检查一下其中的语义 ? 还有 ClearPageError

1. anon 的 unmap 就是释放内存到 swap 中间，但是当我没有 swap 如何办 ?
    1. 似乎从来都没有找到 swap 没有任何空间，系统的处理办法!

# linux Memory Management

布局: introduction 写一个综述，然后 reference 各个 section 和 subsection 中间的内容。

// TODO 经过讲解 PPT 的内容之后，可以整体框架重做为 物理内存，虚拟内存，page cache 和 swap cache 四个部分来分析

## introduction
大致分析一下和内存相关的 syscall
https://thevivekpandey.github.io/posts/2017-09-25-linux-system-calls.html
1. mmap munmap mremap mprotec brk
2. shmget shmat shmctl
3. membarrier
4. madvise msync mlock munlock mincore
5. mbind set_mempoliyc get_mempolicy


1. 一个干净的地址空间 : virtual memory。
    1. 历史上存在使用段式，现代操作系统使用页式虚实映射，x86 对于段式保持兼容，为了节省物理内存，所以虚实翻译是一个多级的。
    2. 访存需要进行一个 page walk ，原先一次访存，现在需要进行多次，所以存在 TLB 加快速度。为了减少 TLB miss rate，使用 superpage 是一种补救方法。
2. 加载磁盘的内容到内存的时机，linux 使用 page fault 机制，当访问到该页面在加载内存(demand paging)。
2. 哪一个物理页面是空闲，哪一个物理页面正在被使用: buddy allocator
    1. 伙伴系统的分配粒度是 : 2^n * page size 的，但是内核需要更小粒度的分配器，linux 使用 slub slob slab 分配器
    2. 物理内存碎片化会导致即使内存充足，但是 buddy allocator 依据无法分配足够的内存，因此需要 [compaction](#compaction) 机制和 [page reclaim](#page-reclaim) 机制
    3. 当缺乏连续的物理页面，可以通过修改内核 page table 的方法获取虚拟的连续地址，这是通过 vmalloc 实现的。
2. 不同的程序片段的属性不同，代码，数据等不同，linux 使用 vma 来描述。
3. 程序需要访问文件，内存比磁盘快很多，所以需要使用内存作为磁盘的缓存: [page cache](#page-cache)
    1. dirty 缓存什么时候写回磁盘，到底让谁写回到内存。
    4. 如果不加控制，缓存将会占据大量的物理内存，所以需要 page reclaim 机制释放一些内存出来。
4. 当内存不够的时候，利用磁盘进行缓存。
    1. 物理页面可能被多个进程共享，当物理页面被写回磁盘的时候，linux 使用反向映射的机制来告知所有的内存。
    2. 不仅仅可以使用 disk 进行缓存，也可以使用一些异构硬件或者压缩内存的方法
5. 不同进程之间需要进行信息共享，利用内存进行共享是一个高效的方法，linux 支持 Posix 和 sysv 的 shmem。
    1. 父子进程之间由于 fork 也会进行内存共享，使用 cow 机制实现更加高效的拷贝(没有拷贝就是最高效的拷贝)
6. 虚拟机中如何实现内存虚拟化
7. 内存是关键的资源，类似于 docker 之类的容器技术需要利用内核提供的 cgroup 技术来限制一个容器内内存使用。

硬件对于内存的管理提出的挑战：
1. 由于 IO 映射以及 NUMA，内存不是连续的。linux 提供了多个内存模型来解决由于空洞导致的无效 struct page
2. NUMA 系统中间，访问非本地的内存延迟比访问本地的延迟要高，如何让 CPU 尽可能访问本地的内存。
    1. 内存分配器应该确立分配的优先级。
    2. 将经常访问的内存迁移过来。
3. 现在操作系统中间，每一个 core 都存在自己的 local cache，为了让 CPU 尽可能访问自己 local cache 的内容，linux 使用 percpu 机制。
4. 内存是操作系统的运行基础，包括内存的分配，为了解决这个鸡生蛋的问题，linux 使用架构相关的代码探测内存，并且使用 memblock 来实现早期的内存管理。
5. 现代处理器处于性能的考虑，对于访存提出 memory consistency 和 cache coherence 协议，其中 memory consistency 让内核的代码需要特殊注意来避免错误。

克服内核开发人员的疏忽产生的错误:
1. kmemleak @todo
2. kasan @todo
3. vmstat 获取必要的统计数据 https://www.linuxjournal.com/article/8178

克服恶意攻击:
1. stack 随机 ?
2. cow 机制的漏洞是什么 ?
3. 内核的虚拟地址空间 和 用户的虚拟地址空间 互相分离。copy_to_user 和 copy_from_user 如何实现 ?
    1. 内核的物理地址是否也是局限于特定的范围中 ? 否则似乎难以建立 linear 映射。
    2. 猜测，对于 amd64, 内核虚拟地址映射了所有的物理地址，这导致其可以访问任何物理地址，而不会出现 page fault 的情况。
        1. 但是用户的看到的地址空间不仅仅包括内核(线性映射)，也包含自己
        2. 用户进程 syscall 之后，需要切换使用内核的 mm_struct 吗 ?
    3. 对于 x86 32bit 利用 highmem 到底实现了什么内容 ?

那么这些东西具有怎样的联系:(将上面的内容整理成为一个表格)
1. page fault 需要的页面可能是是被 swap 出去的
2. shmem 的内存可能被 swap
3. superpage 需要被纳入到 dirty 和 page claim 中间
4. 进行 page reclaim 可以辅助完成 compaction
5. page reclaim 和 swap 都需要使用反向映射。

现在从一个物理页面的角度将上述的内容串联起来。

> 确立那些是基本要素，然后之间的交互是什么:

| virtual memory | swap | allocator | numa | multicore | hugetlb | page cache | page fault | cgroup | shmem | page reclaim | migrate |
|----------------|------|-----------|------|-----------|---------|------------|------------|--------|-------|--------------|---------|
| virtual memory |
| swap           |
| allocator      |
| numa           |

总结内容主要来自于 lwn [^3], (几本书)，wowotech ，几个试验

## lock
- [ ] mm_take_all_locks

## dmapool
https://lwn.net/Articles/69402/

Some very obscure driver bugs have been traced down to cache coherency problems with structure fields adjacent to small DMA areas. [^17]
> DMA 为什么会导致附近的内存的 cache coherency 的问题 ?

- [ ] dma_pool_create() - Creates a pool of consistent memory blocks, for dma.

- [ ] https://www.kernel.org/doc/html/latest/driver-api/dmaengine/index.html#dmaengine-documentation
- [ ] https://www.kernel.org/doc/html/latest/core-api/index.html#memory-management
- [ ] https://www.kernel.org/doc/Documentation/DMA-API-HOWTO.txt

## zsmalloc
slub 分配器处理 size > page_size / 2 会浪费非常多的内容，zsmalloc 就是为了解决这个问题 [^20]

[^20]: [lwn : The zsmalloc allocator](https://lwn.net/Articles/477067/)

## msync
存在系统调用 msync，实现应该很简单吧!

## mpage
fs/mpage.c : 为毛是需要使用这个机制 ? 猜测其中的机制是为了实现

```c
static int ext2_readpage(struct file *file, struct page *page)
{
  return mpage_readpage(page, ext2_get_block);
}

static int
ext2_readpages(struct file *file, struct address_space *mapping,
    struct list_head *pages, unsigned nr_pages)
{
  return mpage_readpages(mapping, pages, nr_pages, ext2_get_block);
}

/*
 * This is the worker routine which does all the work of mapping the disk
 * blocks and constructs largest possible bios, submits them for IO if the
 * blocks are not contiguous on the disk.
 *
 * We pass a buffer_head back and forth and use its buffer_mapped() flag to
 * represent the validity of its disk mapping and to decide when to do the next
 * get_block() call.
 */
static struct bio *do_mpage_readpage(struct mpage_readpage_args *args)
```
> 无论是 ext2_readpage 还是 ext2_readpages 最后都是走到 do_mpage_readpage

## memblock


## profiler
用户层的:
1. https://github.com/KDE/heaptrack
2. https://github.com/koute/memory-profiler

## mprotect
[changing memory protection](https://perception-point.io/changing-memory-protection-in-an-arbitrary-process/)

> - The `vm_area_struct` contains the field `vm_flags` which represents the protection flags of the memory region in an architecture-independent manner, and `vm_page_prot` which represents it in an architecture-dependent manner.

> After some reading and digging into the kernel code, we detected the most essential work needed to really change the protection of a memory region:
> - Change the field `vm_flags` to the desired protection.
> - Call the function `vma_set_page_prot` to update the field vm_page_prot according to the vm_flags field.
> - Call the function `change_protection` to actually update the protection bits in the page table.

check the code in `mprotect.c:mprotect_fixup`, above claim can be verified

- except what three steps meantions above, mprotect also splitting and joining memory regions by their protection flags
## vmalloc
[TO BE CONTINUE](https://www.cnblogs.com/LoyenWang/p/11965787.html)

## mincore

## pageblock
https://richardweiyang-2.gitbook.io/kernel-exploring/00-memory_a_bottom_up_view/13-physical-layer-partition

- pgdat
- zone
- memory_block : 热插拔
- mem_section
- pageblock
- page

## user address space
/home/maritns3/core/vn/hack/lab/proc-self-maps/main.c
```plain
00400000-00401000 r--p 00000000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
00401000-00402000 r-xp 00001000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
00402000-00403000 r--p 00002000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
00403000-00404000 r--p 00002000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
00404000-00405000 rw-p 00003000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
007fa000-0081b000 rw-p 00000000 00:00 0                                  [heap]
7fd3e0f16000-7fd3e0f19000 rw-p 00000000 00:00 0
7fd3e0f19000-7fd3e0f3e000 r--p 00000000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e0f3e000-7fd3e10b6000 r-xp 00025000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e10b6000-7fd3e1100000 r--p 0019d000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e1100000-7fd3e1101000 ---p 001e7000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e1101000-7fd3e1104000 r--p 001e7000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e1104000-7fd3e1107000 rw-p 001ea000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e1107000-7fd3e110b000 rw-p 00000000 00:00 0
7fd3e110b000-7fd3e111a000 r--p 00000000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e111a000-7fd3e11c1000 r-xp 0000f000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e11c1000-7fd3e1258000 r--p 000b6000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e1258000-7fd3e1259000 r--p 0014c000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e1259000-7fd3e125a000 rw-p 0014d000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e125a000-7fd3e125c000 rw-p 00000000 00:00 0
7fd3e1273000-7fd3e1274000 r--p 00000000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e1274000-7fd3e1297000 r-xp 00001000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e1297000-7fd3e129f000 r--p 00024000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e12a0000-7fd3e12a1000 r--p 0002c000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e12a1000-7fd3e12a2000 rw-p 0002d000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e12a2000-7fd3e12a3000 rw-p 00000000 00:00 0
7ffcb622f000-7ffcb6250000 rw-p 00000000 00:00 0                          [stack]
7ffcb6374000-7ffcb6377000 r--p 00000000 00:00 0                          [vvar]
7ffcb6377000-7ffcb6378000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```

- [ ] why there are more section for `main.out` than expected ? There are five entry whose path is `proc-self-maps/main.out`.
  - [ ] check the main.out with section header
  - [ ] why two entry has same offset ? third and forth

- [x] why text segment start at 0x40000 ?
  - [ ] read this : https://stackoverflow.com/questions/39689516/why-is-address-0x400000-chosen-as-a-start-of-text-segment-in-x86-64-abi

- [ ] why some area with no names ?

- [x] check inode
  - [ ] https://unix.stackexchange.com/questions/35292/quickly-find-which-files-belongs-to-a-specific-inode-number
    - In fact, we can find file name with inode by checking file one by one, but **debufs** impressed my

## kaslr
- [ ] https://unix.stackexchange.com/questions/469016/do-the-virtual-address-spaces-of-all-the-processes-have-the-same-content-in-thei
  - [ ] https://en.wikipedia.org/wiki/Kernel_page-table_isolation
  - [ ] https://lwn.net/Articles/738975/

- [ ] https://bneuburg.github.io/
  - [ ] he has writen three post about it

- [ ] https://lwn.net/Articles/569635/


- [ ] Sometimes /proc/$pid/maps show text address start at 0x400000, sometimes 0x055555555xxx,
maybe because of user space address randomization
    - [  ] https://www.theurbanpenguin.com/aslr-address-space-layout-randomization/


## 基本的测试方法

### stress-ng
```sh
cgexec -g memory:mem stress-ng --vm-bytes 150M --vm-keep --vm 1
```
- --vm N : worker 的数量
- --vm-keep
- --vm-bytes : 文档上写的是 per worker 的样子，但是实际上并不是这样的

```c
stress-ng --memrate 1 --memrate-wr 200 --memrate-rd 100 --memrate-bytes 6000M -v
```
似乎这个内存写完了就结束了，而且看上去 memrate-wr 的限制好像是根本就没有起任何作用。

## code tag
- https://mp.weixin.qq.com/s/WU3ZoF8ljgZyqGZB1bQWSg
- https://lwn.net/Articles/974380/
- https://lwn.net/Articles/906660/
- https://elixir.bootlin.com/linux/v6.9.1/source/include/linux/page_ext.h#L45

有趣的东西: page_ext ，code tagging 和 alloc profile

## 	How DRAM changed the world

https://news.ycombinator.com/item?id=41919262
有趣，随便看看

## 入门经典
https://landley.net/writing/memory-faq.txt

[^1]: [lwn : Huge pages part 1 (Introduction)](https://lwn.net/Articles/374424/)
[^2]: [lwn : An end to high memory?](https://lwn.net/Articles/813201/)
[^3]: [lwn#memory management](https://lwn.net/Kernel/Index/#Memory_management)
[^5]: [Complete virtual memory map of x86_64](https://www.kernel.org/doc/html/latest/x86/x86_64/mm.html)
[^8]: [kernel doc : pin_user_pages() and related calls](https://www.kernel.org/doc/html/latest/core-api/pin_user_pages.html)
[^9]: [lwn : Explicit pinning of user-space pages](https://lwn.net/Articles/807108/)
[^13]: [lwn : Smarter shrinkers](https://lwn.net/Articles/550463/)
[^17]: [stackoverflow : Why do we need DMA pool ?](https://stackoverflow.com/questions/60574054/why-do-we-need-dma-pool)
[^18]: [kernel doc : Kernel Memory Leak Detector](https://www.kernel.org/doc/html/latest/dev-tools/kmemleak.html)
[^21]: [lwn : A reworked contiguous memory allocator](https://lwn.net/Articles/447405/)
[^22]: [lwn : A deep dive into dma](https://lwn.net/Articles/486301/)
[^23]: [kernel doc : z3fold](https://www.kernel.org/doc/html/latest/vm/z3fold.html)
[^29]: https://my.oschina.net/u/3857782/blog/1854548


## mmap_sem 始终没有分析

## page cache 和 buffer (io disk 产生的) 似乎 swap out 的时候都是当做 page cache
似乎很多地方他们都是一模一样处理的，把 /dev/nvme0n1 当做一个普通文件
所以，他们什么时候不一样的

## [linux mm, newbie 的 sub web](https://linux-mm.org/)

## 看看
CONFIG_MEMTEST=y

## mmu_gather
mm/mmu_gather.c

https://mp.weixin.qq.com/s/jS_14xvKftGhdYugPzDdTw

## 看看这个文件中的内容
mm/page_frag_cache.c

## docs/kernel/mm-pgtable.md 中按照 dirty 的思路分析其他的 flags

## page-writeback 如何选择 page ?

## 看看
https://lore.kernel.org/lkml/BYAPR02MB448855960A9656EEA81141FC94D99@BYAPR02MB4488.namprd02.prod.outlook.com/

https://lore.kernel.org/all/20240221114357.13655-2-vbabka@suse.cz/T/#r3c8d9cf1e71b54093a0b1cc6d969e5baaa70304e

https://mp.weixin.qq.com/s/Ln_d0-xY8csYVG6D3M-nhA

## Why we need PageUptodate ?
1. because someone else may access to the file simultaneously ?
    1. so how to inform the page ? (@todo should be easy!)
    2. file_operations::sync @todo
```c
static inline int PageUptodate(struct page *page)
{
	int ret;
	page = compound_head(page);
	ret = test_bit(PG_uptodate, &(page)->flags);
	/*
	 * Must ensure that the data we read out of the page is loaded
	 * _after_ we've loaded page->flags to check for PageUptodate.
	 * We can skip the barrier if the page is not uptodate, because
	 * we wouldn't be reading anything from it.
	 *
	 * See SetPageUptodate() for the other side of the story.
	 */
	if (ret)
		smp_rmb();

	return ret;
}
```

## `__SetPageSwapBacked` 和 `SetPageSwapBacked`
1. 和 SetPageSwapBacked 的关系 : 非上锁的版本
2. @todo 三个调用位置都非常的诡异啊!

```c
rmap.c : page_add_new_anon_rmap : @todo 这个函数的调用者简直就是一个神经病

memory.c : do_swap_page
```

3. `__read_swap_cache_async` 调用 `__SetPageSwapBacked`:

add_to_swap
1. 调用 add_to_swap_cache 用于实现 page 和 entry 关联(利用 radix tree)
    1. add_to_swap_cache 是 SetPageSwapCache 的唯三调用者，其余两个位置是 shmem.c 和 migrate.c 中间的，并没有什么实际的意义。
    2. 所以，可以说，当 page 被挂到 swap cache 中间的时候，那么这一个 flag 就会插上去。

## list 从何时被注入 lru 中间

`lru_cache_add` is required only in `add_to_page_cache_lru` from `mm/filemap.c` and adds a page to
both the page cache and the LRU cache. **This is, however, the standard function to introduce a new
page both into the page cache and the LRU list.** Most importantly, it is used by `mpage_readpages` and
`do_generic_mapping_read`, the standard functions in which the block layer ends up when reading data
from a file or mapping.

## vmscan.c 相关
1. try_to_unmap 唯一的正经的调用地方，vmscan.c : shrink_page_list

## balloon 无法获取到 page 的标准是什么?
```txt
[ 9651.023373] virtio_balloon virtio5: Out of puff! Can't get 1 pages
[ 9651.303980] virtio_balloon virtio5: Out of puff! Can't get 1 pages
[ 9651.617787] virtio_balloon virtio5: Out of puff! Can't get 1 pages
[ 9651.896468] virtio_balloon virtio5: Out of puff! Can't get 1 pages
[ 9652.168875] virtio_balloon virtio5: Out of puff! Can't get 1 pages
[ 9652.425923] virtio_balloon virtio5: Out of puff! Can't get 1 pages
[ 9652.686122] virtio_balloon virtio5: Out of puff! Can't get 1 pages
[ 9653.485481] virtio_balloon virtio5: Out of puff! Can't get 1 pages
```

一波操作完成之后，系统状态如下:
```txt
🧀  cat /proc/meminfo
MemTotal:       32377636 kB
MemFree:        20465332 kB
MemAvailable:   29028784 kB
Buffers:          151840 kB
Cached:          8545788 kB
SwapCached:       189260 kB
Active:          1230856 kB
Inactive:        8538660 kB
Active(anon):    1078052 kB
Inactive(anon):    34800 kB
Active(file):     152804 kB
Inactive(file):  8503860 kB
Unevictable:      125724 kB
Mlocked:          125724 kB
SwapTotal:       8265724 kB
SwapFree:        5754308 kB
Zswap:                 0 kB
Zswapped:              0 kB
Dirty:                52 kB
Writeback:             0 kB
AnonPages:       1146652 kB
Mapped:           260112 kB
Shmem:             24028 kB
KReclaimable:     398940 kB
Slab:             562980 kB
SReclaimable:     398940 kB
SUnreclaim:       164040 kB
KernelStack:       12780 kB
PageTables:        22752 kB
SecPageTables:      1496 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:    24454540 kB
Committed_AS:   10050560 kB
VmallocTotal:   34359738367 kB
VmallocUsed:       92256 kB
VmallocChunk:          0 kB
Percpu:            37120 kB
HardwareCorrupted:     0 kB
AnonHugePages:    385024 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
CmaTotal:              0 kB
CmaFree:               0 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB
DirectMap4k:      194252 kB
DirectMap2M:     6096896 kB
DirectMap1G:    29360128 kB
```
可以发现，系统中有一些 swap ，但是 swap 的空间还有很多。

这就很奇怪，如果 ballon 去分配内存，要么把所有的 swap 全部都消耗掉，
要么不可以将任何内存换出，但是实际上似乎是存在一个阈值的。

## VSwapper
- [ ] 构建一个混合技术，将 virtio-mem, balloon 和 VSwapper 混合使用，探测 guest 中有啥的时候，然后用啥?

## mTHP
https://mp.weixin.qq.com/s/qKhNZdeRGCFNUWKLVt_NNw

## 这个脚本写的很好，不过还是有误差

用这个分析 MemAvailable 是极好的
```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

# 获取基本内存信息
MemFree=$(awk '/^MemFree:/ {print $2}' /proc/meminfo)
PageCache=$(awk '/^Cached:/ {print $2}' /proc/meminfo)
SReclaimable=$(awk '/^SReclaimable:/ {print $2}' /proc/meminfo)
Buffers=$(awk '/^Buffers:/ {print $2}' /proc/meminfo)

# 获取 /proc/meminfo 提供的 MemAvailable
MemAvailable_proc=$(awk '/^MemAvailable:/ {print $2}' /proc/meminfo)

# 计算 watermark[LOW]
watermark_LOW=$(awk '/low/ {sum += $2} END {print sum+0}' /proc/zoneinfo)
watermark_LOW=$((watermark_LOW * 4)) # 转换为 KB（假设 PAGE_SIZE = 4KB）

# 确保所有变量都是整数
if [[ -z $MemFree || -z $PageCache || -z $SReclaimable || -z $Buffers || -z $watermark_LOW ]]; then
	echo "Error: One or more values are empty."
	exit 1
fi

# 计算 min(PageCache / 2, watermark[LOW])
half_PageCache=$((PageCache / 2))
if [ "$half_PageCache" -lt "$watermark_LOW" ]; then
	min_value=$half_PageCache
else
	min_value=$watermark_LOW
fi

# 计算 MemAvailable
SReclaimable_half=$((SReclaimable / 2))
MemAvailable=$((MemFree - watermark_LOW + (PageCache - min_value) + SReclaimable_half + Buffers))

# 输出结果
echo "==================== 计算结果 ===================="
echo "MemFree: ${MemFree} KB"
echo "PageCache: ${PageCache} KB"
echo "SReclaimable: ${SReclaimable} KB"
echo "Buffers: ${Buffers} KB"
echo "watermark[LOW]: ${watermark_LOW} KB"
echo "min(PageCache / 2, watermark[LOW]): ${min_value} KB"
echo "MemAvailable (计算): ${MemAvailable} KB"
echo "MemAvailable (/proc/meminfo): ${MemAvailable_proc} KB"
echo "=================================================="

# 计算误差
diff=$((MemAvailable - MemAvailable_proc))
if [ "$diff" -lt 0 ]; then
	diff=$((-diff)) # 取绝对值
fi
echo "计算误差: ${diff} KB"

# 判断误差是否在合理范围
threshold=$((MemAvailable_proc / 50)) # 2% 误差范围
if [ "$diff" -lt "$threshold" ]; then
	echo "✅ 计算结果与 /proc/meminfo 相近 (误差 < 2%)"
else
	echo "❌ 计算结果偏差较大，请检查计算逻辑"
fi
```

## mm todo
https://mp.weixin.qq.com/s/IjwDyftIPalIthmhB75uNw

https://mp.weixin.qq.com/s/5J_gWmrEsAhXnDZx9DzlLw

## 为什么打开这个这两个 config
```txt
CONFIG_MEMORY_FAILURE=y
CONFIG_HWPOISON_INJECT=m
```
会导致 CONFIG_CONTIG_ALLOC=y 被打开 ?

## KMSAN 是什么东西?
KMSAN 不是 KASAN

## 还没看就被干掉了
commit 6df8bae8e851 ("mm: zbud: remove zbud")
commit 58ba73e521b3 ("mm: z3fold: remove z3fold")

## 收集一个有趣的 log
```txt
     0x742f33736e697472
   - __umount2
      - 99.48% entry_SYSCALL_64_after_hwframe
           do_syscall_64
           syscall_exit_to_user_mode
           task_work_run
           cleanup_mnt
           deactivate_locked_super
           xfs_kill_sb
           kill_block_super
           generic_shutdown_super
           evict_inodes
           evict
         - truncate_inode_pages_range
            - 34.84% delete_from_page_cache_batch
               - 16.03% xas_store
                    2.55% xas_clear_mark
                  - 1.82% xas_load
                       0.72% xas_start
                    1.77% workingset_update_node
                  - 0.71% srso_return_thunk
                       srso_safe_ret
               - 11.49% filemap_unaccount_folio
                  - 9.09% __lruvec_stat_mod_folio
                     - 4.53% __mod_memcg_lruvec_state
                          0.76% cgroup_rstat_updated
                       0.52% __mod_node_page_state
                 0.86% filemap_free_folio
            - 33.19% folios_put_refs
               - 13.44% __page_cache_release.part.0
                  - 4.28% __mod_memcg_lruvec_state
                       1.07% cgroup_rstat_updated
                  - 2.16% __mod_lruvec_state
                       0.68% __mod_node_page_state
                    0.93% __mod_zone_page_state
                    0.73% mem_cgroup_update_lru_size
               - 12.87% free_unref_folios
                  - 9.26% free_frozen_page_commit
                     - 7.63% free_pcppages_bulk
                        - 5.14% __free_one_page
                             0.68% __mod_zone_page_state
               - 4.69% __mem_cgroup_uncharge_folios
                  - 2.65% uncharge_folio
                       0.52% __rcu_read_lock
                    0.51% __rcu_read_unlock
            - 12.96% truncate_folio_batch_exceptionals
               - 10.69% xas_store
                    1.62% xas_clear_mark
                    1.34% workingset_update_node
                  - 1.19% xas_load
                       0.56% xas_start
                  - 0.56% srso_return_thunk
                       srso_safe_ret
            - 11.93% find_lock_entries
                 2.60% xas_find
                 0.85% __rcu_read_lock
                 0.66% xas_get_order
               - 0.56% srso_return_thunk
                    srso_safe_ret
              2.30% truncate_cleanup_folio
              0.90% folio_unlock
```

## 记录一个可以被优化的问题


在 hygon 上，一个 qemu 占用了 500G 的内存，当 qemu 调用 exit ，这个系统调用需大于 20s 才可以结束，
当时， 可以观察到这样的结果:
```txt
  - 59.79% entry_SYSCALL_64_after_hwframe
      - 59.78% do_syscall_64
         - 42.75% syscall_exit_to_user_mode
            - 42.55% arch_do_signal_or_restart
                 get_signal
                 do_group_exit
               - do_exit
                  - 29.23% mmput
                     - exit_mmap
                        - 29.06% unmap_vmas
                           - unmap_page_range
                              - 17.99% tlb_flush_mmu
                                 - __tlb_batch_free_encoded_pages
                                 - free_pages_and_swap_cache
                                    - 16.94% folios_put_refs
                                       - 6.99% free_unref_folios
                                          - 5.20% free_frozen_page_commit
                                             - 4.44% free_pcppages_bulk
                                                  3.23% __free_one_page
                                       - 6.71% __page_cache_release.part.0
                                            2.12% __mod_memcg_lruvec_state
                                            1.03% __mod_lruvec_state
                                       - 2.31% __mem_cgroup_uncharge_folios
                                            1.21% uncharge_folio
                              - 5.85% folio_remove_rmap_ptes
                                 - 4.69% __lruvec_stat_mod_folio
                                      2.12% __mod_memcg_lruvec_state
                  - 13.33% task_work_run
                     - __fput
                        - 13.31% kvm_vm_release
                           - kvm_destroy_vm
                              - 12.99% kvm_free_memslots.part.0
                                 - 10.39% kvm_arch_free_memslot
                                    - vfree.part.0
                                       - 7.69% free_frozen_pages
                                          - 3.99% __memcg_kmem_uncharge_page
                                             - 3.31% obj_cgroup_uncharge_pages
                                                  0.66% __mod_memcg_state
                                          - 1.68% free_frozen_page_commit
                                             - 1.39% free_pcppages_bulk
                                                  0.91% __free_one_page
                                         0.64% __mod_memcg_state
                                 - 2.60% kvm_page_track_free_memslot
                                    - vfree.part.0
                                       - 1.95% free_frozen_pages
                                            - 3.99% __memcg_kmem_uncharge_page
                                             - 3.31% obj_cgroup_uncharge_pages
                                                  0.66% __mod_memcg_state
                                          - 1.68% free_frozen_page_commit
                                             - 1.39% free_pcppages_bulk
                                                  0.91% __free_one_page
                                         0.64% __mod_memcg_state
                                 - 2.60% kvm_page_track_free_memslot
                                    - vfree.part.0
                                       - 1.95% free_frozen_pages
                                          - 1.00% __memcg_kmem_uncharge_page
                                               0.86% obj_cgroup_uncharge_pages
```

## 从来没有搞清楚过这个问题做什么的
1. folio_mark_accessed
2. folio_test_workingset

## 这两个新 feature 也是需要处理一下的
- https://lwn.net/Articles/931812/
- https://lwn.net/Articles/789153/

## 如何理解 folio_test_workingset 的作用

为什么 swap_read_folio 和 __swap_writepage 对于 fs 的处理不是对称的

__swap_writepage 还是需要经过 bdev

## 思考一下 shmem.c 的问题，为什么写的那么复杂?

先考虑一个问题，shmem 和 swap 的关系:


swap_read_folio
swapin_readahead


例如:
```txt
	si = get_swap_device(entry);
```

一般来说，需要维护一个
```txt
struct swap_info_struct
```

swap 的空间中，需要维护


```c
static const struct file_operations shmem_file_operations = {
	.mmap		= shmem_mmap,
	.open		= shmem_file_open,
	.get_unmapped_area = shmem_get_unmapped_area,
#ifdef CONFIG_TMPFS
	.llseek		= shmem_file_llseek,
	.read_iter	= shmem_file_read_iter,
	.write_iter	= shmem_file_write_iter,
	.fsync		= noop_fsync,
	.splice_read	= shmem_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.fallocate	= shmem_fallocate,
#endif
};
```

还是写一个普通的内核模块来 mmap 吗？

## /dev/shm 和 /tmp 有什么区别吗?

用这个测试一下:
```c
static const struct vm_operations_struct shmem_anon_vm_ops = {
	.fault		= shmem_fault,
	.map_pages	= filemap_map_pages,
#ifdef CONFIG_NUMA
	.set_policy     = shmem_set_policy,
	.get_policy     = shmem_get_policy,
#endif
};
```

shmem_writepage 将 page 从 page cache 移动到 swap cache
然后调用 swap_writepage


## 测试一下，kswapd 开始回收的时候，CPU 利用率是多少。

回收的时候，会有 CPU 利用率的限制吗?

## 这个写的不错，作为一个查漏补缺吧
Linux系统启动之后，物理内存的布局是怎么样的？ - 科英的回答 - 知乎
https://www.zhihu.com/question/274054284/answer/3203970743

## 观察 mm_struct 的两个有趣的东西

1. 一个 mm_struct 可以被多个 process 使用，都是谁在调用 mmgrab 和 mmdrop 的，相比绝对
不只 thread
2. aio 为什么需要在 mm_struct 中有结构体?
3. 为什么只有 memcg 的时候才需要 owner ?

```c
struct mm_struct {
	struct {
		/*
		 * Fields which are often written to are placed in a separate
		 * cache line.
		 */
		struct {
			/**
			 * @mm_count: The number of references to &struct
			 * mm_struct (@mm_users count as 1).
			 *
			 * Use mmgrab()/mmdrop() to modify. When this drops to
			 * 0, the &struct mm_struct is freed.
			 */
			atomic_t mm_count;
		} ____cacheline_aligned_in_smp;

#ifdef CONFIG_AIO
		spinlock_t			ioctx_lock;
		struct kioctx_table __rcu	*ioctx_table;
#endif

#ifdef CONFIG_MEMCG
		/*
		 * "owner" points to a task that is regarded as the canonical
		 * user/owner of this mm. All of the following must be true in
		 * order for it to be changed:
		 *
		 * current == mm->owner
		 * current->mm != mm
		 * new_owner->mm == mm
		 * new_owner->alloc_lock is held
		 */
		struct task_struct __rcu *owner;
#endif
```

## 不错
https://zhuanlan.zhihu.com/p/1924460570649800991
https://mp.weixin.qq.com/s/TUe3cghdilqh0dq795ZL8A

在大的框架上修修补补也是不错的

## page_lock 到底什么意思?

## The state of the page in 2024
https://mp.weixin.qq.com/s/MtNsCzkP_UhdQRafeF4HNw

## mmap 的接口清理
https://mp.weixin.qq.com/s?__biz=MzUyNjgzNjI3NQ==&mid=2247493981&idx=1&sn=7724882967920bc055015795a805bc29&chksm=fbda1360d32f9e54aaeff2d85a7abb16cce311bb1de4a872a847e79d1d08abe95668ec468246&mpshare=1&scene=1&srcid=0926WtvDENwFjDdcK2yUDM6g&sharer_shareinfo=fc0549efb847fe57c95dd919aa52c718&sharer_shareinfo_first=fc0549efb847fe57c95dd919aa52c718#rd

## 整理一下这个东西
https://documentation.suse.com/sles/15-SP7/html/SLES-all/cha-tuning-memory.html

这个东西也看下吧:
https://events.linuxfoundation.org/wp-content/uploads/2021/10/LF_Live_memoptimizer.pdf

## 看看这个
https://mp.weixin.qq.com/s/rYmInEe8fgmD-q9kyT8uZA

## 看看
https://mp.weixin.qq.com/s/ccYMakhDN2gGvcz3peYUgQ

https://mp.weixin.qq.com/s/EEiDJ7S3LpdXcRphwlIKIg

https://mp.weixin.qq.com/s/vBEz6w0LjJfFNum0W32GnA

https://news.ycombinator.com/item?id=46300411

## kfence 居然是默认打开的
<!-- 27cfc022-e7e8-46df-8cfe-25c4a3a36805 -->
nixos 的 kernel 为什么默认打开了

```txt
CONFIG_KFENCE=y
```
看看这个导致了多少的性能损失 和 内存损失。

哦，现在 KFENCE 都是默认打开的，例如 arm 的
```txt
 cat config-6.17.12-400.asahi.fc42.aarch64+16k| grep FENCE
# CONFIG_DMA_FENCE_TRACE is not set
CONFIG_HAVE_ARCH_KFENCE=y
CONFIG_KFENCE=y
CONFIG_KFENCE_SAMPLE_INTERVAL=100
CONFIG_KFENCE_NUM_OBJECTS=255
# CONFIG_KFENCE_DEFERRABLE is not set
# CONFIG_KFENCE_STATIC_KEYS is not set
CONFIG_KFENCE_STRESS_TEST_FAULTS=0
CONFIG_KFENCE_KUNIT_TEST=m
```

## 看看
https://mp.weixin.qq.com/s/QjziV8prFdtPH6XniK3EmA

https://mp.weixin.qq.com/s/Wno2ON-UAPfrvOUeLJPxTw

## 总结一下内存中反复进攻，反复失败的问题

1. slub
2. vmscan
3. folio (引用技术，transparet hupage 也会是 folio)
4. rmap
5. swap


## 这个东西好
https://github.com/hoytech/vmtouch

## 荣耀在MGLRU内存回收上的发力或恰到好处
https://mp.weixin.qq.com/s/i65Q9p6W3hRGWi-z1QiRdA


https://mp.weixin.qq.com/s/GnU0eGhBn0RDj_9AegKFxA

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
