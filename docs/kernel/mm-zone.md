# zone

## node / zone 初始化
- paging_init
  - zone_sizes_init
    - free_area_init
      - free_area_init_node : 初始化 pg_data_t
        - calculate_node_totalpages : 在这里给每一个 zone 的大小进行划定，顺着分配，先分配的占据架构允许的所有的内存
          - zone_spanned_pages_in_node
        - free_area_init_core
          - zone_init_internals : 初始化一个 zone

如果自习观察 zone_spanned_pages_in_node，发现 zone 的范围是受 `arch_zone_highest_possible_pfn` 控制的，如果是 NUMA 机器，可以发现只有 Node 0 上是有 DMA 和 DMA32，其他的 Node 上是没有该节点的。

## 问题
- 因为 swap 每一个 node 的，分配的过程中，会导致 node 中较少的开始触发水位线吗？
  - 应该存在机制来，让这边的至少是处于良好运行的状态吧
- gfp_zone 是做啥的

- [ ] mmzone.c 里面的几个函数是做什么的?

## physical memory initialization
1. 探测
2. memblock

[LoyenWang](https://www.cnblogs.com/LoyenWang/p/11568481.html)
[LoyenWang](https://www.cnblogs.com/LoyenWang/p/11523678.html)

整体完成的工作也比较简单，将所有 Node 中可用的 zone 全部添加到各个 Node 中的 zonelist 中，也就是对应的 struct pglist_data 结构体中的 struct zonelist node_zonelists 字段。 这一步之后，准备工作基本就绪，进行页面申请的工作就可以开始了。
![loading](https://img2018.cnblogs.com/blog/1771657/201910/1771657-20191006001313609-398829452.png)

## memory zone

TODO 等待分析: zone 功能不同，甚至 swap cache 到 page cache 需要靠拷贝的方法维持生活 ?
```c
/*
 * When a page is moved from swapcache to shmem filecache (either by the
 * usual swapin of shmem_getpage_gfp(), or by the less common swapoff of
 * shmem_unuse_inode()), it may have been read in earlier from swap, in
 * ignorance of the mapping it belongs to.  If that mapping has special
 * constraints (like the gma500 GEM driver, which requires RAM below 4GB),
 * we may need to copy to a suitable page before moving to filecache.
 *
 * In a future release, this may well be extended to respect cpuset and
 * NUMA mempolicy, and applied also to anonymous pages in do_swap_page();
 * but for now it is a simple matter of zone.
 */
static bool shmem_should_replace_page(struct page *page, gfp_t gfp)
{
  return page_zonenum(page) > gfp_zone(gfp);
}
```
