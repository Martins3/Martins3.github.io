# Professional Linux Kernel Architecture : Appendix C

## C.2 Standard Data Structures and Techniques of the Kernel

#### C.2.10 Radix Trees
Radix trees differ from other trees because it is **not necessary to compare the entire
key at every branch**, but only part of the key with the stored value of the node when performing search
operations. This results in slightly different worst-case and average-case behavior than in other implementations, 
which are described in detail in the corresponding textbooks on algorithms. Also, radix trees
are not particularly difficult to implement, which adds to their attraction.


```c
struct radix_tree_node {
	unsigned int	path;	/* Offset in parent & height from the bottom */
	unsigned int	count;
	union {
		struct {
			/* Used when ascending tree */
			struct radix_tree_node *parent;
			/* For tree user */
			void *private_data;
		};
		/* Used when freeing node */
		struct rcu_head	rcu_head;
	};
	/* For tree user */
	struct list_head private_list;
	void __rcu	*slots[RADIX_TREE_MAP_SIZE];
	unsigned long	tags[RADIX_TREE_MAX_TAGS][RADIX_TREE_TAG_LONGS];
};
```
`slots` is an array of pointers that, according to their position in the tree (i.e., the level on which the node
is located), **point either to other nodes or to data elements**

`count` indicates the number of occupied array positions

Every tree node can be associated with `tags` that correspond to a set or an unset bit
> 1. tag 到底是做什么用的
> 2. 为什么非要做成二维数组的形式
> 3. page cache 为什么就是可以使用

```c
/* root tags are stored in gfp_mask, shifted by __GFP_BITS_SHIFT */
struct radix_tree_root {
	unsigned int		height;
	gfp_t			gfp_mask;
	struct radix_tree_node	__rcu *rnode;
};
```
1. `height` specifies the current height of the tree,
2. and `rnode` points to the first node.
3. `gfp_mask` specifies the memory area from which the required data structure instances of the tree are to be taken
> 看来gfp_mask 说明了必定是相同类型的memory area 会放到一个radix_tree_node, 是不是说明一个inode　被分配的内存一定是相同类型的内存

The maximum number of elements that can be stored in a tree can be derived directly from the tree
height — that is, from the number of node levels. The kernel provides the following function to compute
the height:

@todo 分析一下，其使用位置已有一个，而且对于所有的radix tree 都是可以使用的 height_to_maxnodes 的作用是什么
@question 而且其初始化过程　 radix_tree_init_maxnodes　中间定义的莫名其妙的变量的含义尚且不明确
但是可以清楚的是，这一个功能是用于preload 确定范围的东西




使用height_to_maxnodes 唯一的函数，`error = radix_tree_maybe_preload_order(gfp_mask, compound_order(page));`，进而仅仅被两个函数使用:

```c
/*
 * shmem_getpage_gfp - find page in cache, or get from swap, or allocate
 *
 * If we allocate a new one we do not mark it dirty. That's up to the
 * vm. If we swap it in we mark it dirty since we also free the swap
 * entry since a page cannot live in both the swap and page cache.
 *
 * fault_mm and fault_type are only supplied by shmem_fault:
 * otherwise they are NULL.
 */
static int shmem_getpage_gfp(struct inode *inode, pgoff_t index,
```
```c
int add_to_swap_cache(struct page *page, swp_entry_t entry, gfp_t gfp_mask)
{
	int error;

	error = radix_tree_maybe_preload_order(gfp_mask, compound_order(page));
	if (!error) {
		error = __add_to_swap_cache(page, entry);
		radix_tree_preload_end();
	}
	return error;
}
```


```c
static inline int radix_tree_insert(struct radix_tree_root *root,
			unsigned long index, void *entry)
{
	return __radix_tree_insert(root, index, 0, entry);
}


/**
 *	__radix_tree_insert    -    insert into a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *	@order:		key covers the 2^order indices around index
 *	@item:		item to insert
 *
 *	Insert an item into the radix tree at position @index.
 */
int __radix_tree_insert(struct radix_tree_root *root, unsigned long index,
			unsigned order, void *item)
{
	struct radix_tree_node *node;
	void __rcu **slot;
	int error;

	BUG_ON(radix_tree_is_internal_node(item));

	error = __radix_tree_create(root, index, order, &node, &slot);
	if (error)
		return error;

	error = insert_entries(node, slot, item, order, false);
	if (error < 0)
		return error;

	if (node) {
		unsigned offset = get_slot_offset(node, slot);
		BUG_ON(tag_get(node, 0, offset));
		BUG_ON(tag_get(node, 1, offset));
		BUG_ON(tag_get(node, 2, offset));
	} else {
		BUG_ON(root_tags_get(root));
	}

	return 0;
}
EXPORT_SYMBOL(__radix_tree_insert);


/**
 *	__radix_tree_create	-	create a slot in a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *	@order:		index occupies 2^order aligned slots
 *	@nodep:		returns node
 *	@slotp:		returns slot
 *
 *	Create, if necessary, and return the node and slot for an item
 *	at position @index in the radix tree @root.
 *
 *	Until there is more than one item in the tree, no nodes are
 *	allocated and @root->rnode is used as a direct slot instead of
 *	pointing to a node, in which case *@nodep will be NULL.
 *
 *	Returns -ENOMEM, or 0 for success.
 */
int __radix_tree_create(struct radix_tree_root *root, unsigned long index, // 最终实现插入的关键函数，@todo 
			unsigned order, struct radix_tree_node **nodep,
			void __rcu ***slotp)
{
```


```c
/*
 * Return the pagecache index of the passed page.  Regular pagecache pages
 * use ->index whereas swapcache pages use swp_offset(->private)
 */
static inline pgoff_t page_index(struct page *page)
{
	if (unlikely(PageSwapCache(page)))
		return __page_file_index(page);
	return page->index;
}
```
> 龟龟，可算是知道作为key index是什么了。

@todo 需要进一步知道，`page->index`的作用被使用上来:


```c
/**
 * __page_set_anon_rmap - set up new anonymous rmap
 * @page:	Page to add to rmap	
 * @vma:	VM area to add page to.
 * @address:	User virtual address of the mapping	
 * @exclusive:	the page is exclusively owned by the current process
 */
static void __page_set_anon_rmap(struct page *page,
	struct vm_area_struct *vma, unsigned long address, int exclusive)
{
	struct anon_vma *anon_vma = vma->anon_vma;

	BUG_ON(!anon_vma);

	if (PageAnon(page))
		return;

	/*
	 * If the page isn't exclusively mapped into this vma,
	 * we must use the _oldest_ possible anon_vma for the
	 * page mapping!
	 */
	if (!exclusive)
		anon_vma = anon_vma->root;

	anon_vma = (void *) anon_vma + PAGE_MAPPING_ANON;
	page->mapping = (struct address_space *) anon_vma;
	page->index = linear_page_index(vma, address); // 偏移量是address 在当前page 所映射文件中间偏移
}
```
> 感觉为了实现所谓的rmap, address_space被特定的inode 持有
> @todo 所以为什么不直接阅读rmap 哪一个章节啊!


```c
static inline pgoff_t linear_page_index(struct vm_area_struct *vma,
					unsigned long address)
{
	pgoff_t pgoff;
	if (unlikely(is_vm_hugetlb_page(vma)))
		return linear_hugepage_index(vma, address);
	pgoff = (address - vma->vm_start) >> PAGE_SHIFT;
	pgoff += vma->vm_pgoff;
	return pgoff;
}
```


vm_area_struct　中间 其实定义了:
```c
	/* Information about our backing store: */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
					   units */
	struct file * vm_file;		/* File we map to (can be NULL). */
```
1. `vm_pgoffset` specifies an offset for a file mapping when not all file contents are to be mapped
(the offset is 0 if the whole file is mapped).
2. `vm_file` points to the file instance that describes a mapped file (it holds a null pointer if the
object mapped is not a file). Chapter 8 discusses the contents of the file structure at length


> 还有更加关键的问题没有被解释:
> 1. 为什么单单page cache 需要使用radix tree
> 2. page cache 为什么需要快速查询工作,为什么感觉radix tree 的功能更加像是给字符串使用的
> 3. 我有点怀疑[wiki](https://en.wikipedia.org/wiki/Radix_tree#Comparison_to_other_data_structures)中间的描述和内核中间的radix tree 是不是同一种数据结构。
