# ksm
kernel doc[^15] 中间说明了如何使用，首先阅读的内容。
- [ ] 这个东西对于性能有不好的影响，同时

## TODO
- [ ] KSM 是不是只能合成匿名页面, 什么是 LRU 页面 ?


- [ ] ksm_slab_init : slab 是什么鬼东西 ?

scan_get_next_rmap_item 获取一个合适的匿名页面。

cmp_and_merge_page 让页面在 KSM 中稳定和不稳定的两颗红黑树中间查找是否存在合并的对象。

rmap_item 代表一个页面

- [ ] `rmap_item::anon_vma;`

- [ ] migrate_pages 移动物理页面，导致所有共享这个物理页面的人都不能使用该页面了。

- [ ] 始终无法理解为什么和 rmap 相关啊 ?

## notes
- 为什么需要使用 stable 和 unstable 两棵树。
  - unstable 的含义是 : 如果这个页面在两次扫描的时候，保持 checksum 不变
  - stable 的含义 : 存在有人的页面和相同的。

```c
/*
 * cmp_and_merge_page - first see if page can be merged into the stable tree;
 * if not, compare checksum to previous and if it's the same, see if page can
 * be inserted into the unstable tree, or merged with a page already there and
 * both transferred to the stable tree.
 *
 * @page: the page that we are searching identical page to.
 * @rmap_item: the reverse mapping into the virtual address of this page
 */
static void cmp_and_merge_page(struct page *page, struct rmap_item *rmap_item)
```


## 读读代码
- ksm_scan_thread
  - ksm_do_scan
    - scan_get_next_rmap_item
      - follow_page
      - get_next_rmap_item
    - cmp_and_merge_page
      - page_stable_node : 通过 page 获取其所在的 stable_node, @todo 但是既然都是挂载到了 stable_node 上了，为什么还是需要 cmp_and_merge_page ? 不应该在更加早的位置就可以检查出来吗 ?
      - stable_tree_search
      - remove_rmap_item_from_tree
      - try_to_merge_with_ksm_page
      - calc_checksum : 为什么等到现在才进行 calc_checksum, 之前的操作依据是什么 ? 为什么是放到最后进行两个 tree 的 insert ?
      - unstable_tree_search_insert
      - stable_tree_insert

## 从 rmap 中看到的
```c
/*
 * On an anonymous page mapped into a user virtual memory area,
 * page->mapping points to its anon_vma, not to a struct address_space;
 * with the PAGE_MAPPING_ANON bit set to distinguish it.  See rmap.h.
 *
 * On an anonymous page in a VM_MERGEABLE area, if CONFIG_KSM is enabled,
 * the PAGE_MAPPING_MOVABLE bit may be set along with the PAGE_MAPPING_ANON
 * bit; and then page->mapping points, not to an anon_vma, but to a private
 * structure which KSM associates with that merged page.  See ksm.h.
 *
 * PAGE_MAPPING_KSM without PAGE_MAPPING_ANON is used for non-lru movable
 * page and then page->mapping points a struct address_space.
 *
 * Please note that, confusingly, "page_mapping" refers to the inode
 * address_space which maps the page from disk; whereas "page_mapped"
 * refers to user virtual address space into which the page is mapped.
 */
#define PAGE_MAPPING_ANON	0x1
#define PAGE_MAPPING_MOVABLE	0x2
#define PAGE_MAPPING_KSM	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
#define PAGE_MAPPING_FLAGS	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
```

6. ksm 和 ksm 页反向映射
  - 相关数据结构体介绍
  - ksm 机制剖析（上）
  - ksm 机制剖析（下）
  - 反向映射查找 ksm 页 pte
  - ksm 实践

[^15]: [kernel doc : Kernel Samepage Merging](https://www.kernel.org/doc/html/latest/vm/ksm.html)
