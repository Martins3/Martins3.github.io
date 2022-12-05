- [ ] 一个 QEMU 可以混合使用不同大小的大页吗?
- [ ] QEMU 的启动参数
  - -m 24G -mem-prealloc -mem-path /dev/hugepages/test_vm
- [ ] 一个 mmap 可以混合使用各种大页吗?
- [ ] for_each_zone_zonelist_nodemask
- highatomic 是做什么意思的
- [ ] tools/vm directory
- [ ] 检查一下 zero page 和 swap 的代码，应该是 zero page 不会被换出的。
- https://www.kernel.org/doc/Documentation/vm/pagemap.txt
  - 从这里介绍内核的 flags，是极好的
- [ ] 如果是 private 映射一个文件，其修改应该最后也是写入到 swap 中的吧
  - 应该是的，但是需要验证
- [What Every Programmer Should Know About Memory](https://people.freebsd.org/~lstewart/articles/cpumemory.pdf)
  - 总体结论，还是正确的
  - https://stackoverflow.com/questions/8126311/how-much-of-what-every-programmer-should-know-about-memory-is-still-valid
- numa remote access 是如何确定的
- vmpressure.c 是做什么的
- mmu notifier

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
- fixmap 是做啥的

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

- 整理一下 GFP 之类的
  - 因为有时候注意到，有时候让 page cache 都要清理一些，有时候不会
  - 有时候，可以导致 cache 都要出去
  - 例如 : `__GFP_NORETRY`
  - 其实也就是这里的内容: linux/include/linux/gfp_types.h

- memory 调试方法:
  - https://stackoverflow.com/questions/22717661/linux-page-poisoning


- /sys/devices/system/node/node2 : 这个是如何创建出来的

- [ ] page_evictable() and PageMovable()
  - [ ] I think, if a page can be evicted to swap, so it can movable too.

- put_page : 理解一下 reference counting 机制
