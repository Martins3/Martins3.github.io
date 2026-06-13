## inode

1. new_inode(): creates a new inode, sets the i_nlink field to 1 and initializes i_blkbits, i_sb and i_dev;
```c
/**
 *	new_inode 	- obtain an inode
 *	@sb: superblock
 *
 *	Allocates a new inode for given superblock. The default gfp_mask
 *	for allocations related to inode->i_mapping is GFP_HIGHUSER_MOVABLE.
 *	If HIGHMEM pages are unsuitable or it is known that pages allocated
 *	for the page cache are not reclaimable or migratable,
 *	mapping_set_gfp_mask() must be called with suitable flags on the
 *	newly created inode's mapping
 *
 */
struct inode *new_inode(struct super_block *sb)
{
	struct inode *inode;

	spin_lock_prefetch(&sb->s_inode_list_lock);

	inode = new_inode_pseudo(sb);
	if (inode)
		inode_sb_list_add(inode);
	return inode;
}
```
和 super_operations::alloc_inode 的关系是什么 ? new_inode 会调用 alloc_inode ，进而调用其。


2. insert_inode_hash(): adds the inode to the hash table of inodes; an interesting effect of this call is that the inode will be written to the disk if it is marked as dirty;
An inode created with new_inode() is not in the hash table, and unless you have serious reasons not to, you must enter it in the hash table;

3. mark_inode_dirty(): marks the inode as dirty; at a later moment, it will be written on the disc;
其实

4. iget_locked(): **loads the inode with the given number from the disk, if it is not already loaded;**(总结到位)

5. unlock_new_inode(): used in conjunction with iget_locked(), releases the lock on the inode;

6. iput(): tells the kernel that the work on the inode is finished; if no one else uses it, it will be destroyed (after being written on the disk if it is maked as dirty);

7. make_bad_inode(): tells the kernel that the inode can not be used; It is generally used from the function that reads the inode when the inode could not be read from the disk, being invalid.


inode 可以分析的方面:
1. disk inode 如何加载到 memory inode : iget_locked 使用 inode cache 查询， super_operations::alloc_inode 分配空间， ext2_iget 中间进行读取和装配。
2. inode 如何关联上 struct file 的 ?
3. inode 如何关联上 下层设备的 ?
4. inode 在 fs-writeback 的作用 ?
5. inode 在 page reclaim 的时候，是如何选择进行回收的 ?
6. inode 的整个生命流程描述: iget_locked => iput 加载到内存到写回磁盘的过程，从 create 到 delete 的


或者内容的整理为:
1. fs/inode.c (inode 的各种管理，evict 等操作) fs/namei.c (查询)
2. ext2/inode.c (这里居然放置的内容是 address_space_operation 的系统) ext2/namei.c (查询的支持，和查询到之后的操作)

inode 在查询的过程，inode 在 inode_operation 的工作体现，inode

# inode.md

| Name              | desc                                      |
|-------------------|-------------------------------------------|
| inode_init_always | 默认初始化操作 inode 及其 `inode->i_data` |

## TODO
1. ilookup
2. time 函数
3. 各种初始化 和 clear 函数

## 需要回答一下关键问题
1. 一个 disk inode 被加载到 内存中间的过程是什么 ?
    1. 找到
    2. 读取
2. inode 之间是如何勾连起来的
3. inode 的各个关键部分的初始化:
   1. backing dev
   2. inode 对应的 file struct ? (会有吗 ?)

4. 回忆 ext2 的结构: bitmap(管理 free blocks) + inode table(存储 inode 结构体) + data block 的内容
    1. 也就是说 inode number 是用于定位 inode table 的
    2. 如果存在多个文件系统，在内存中间的 inode number 此时其含义是什么 ?
        1. 猜测是 : inode number 在内存中间也是 disk inode，所以不同的内存的 inode 结构体可以持有相同的 inode number
        2. 所以，没有，也没有必要存在利用 inode number 找到 inode struct 的操作
    3. 所以 get_next_ino 的意义是什么 ?

## inode_dio_wait

```c
/**
 * inode_dio_wait - wait for outstanding DIO requests to finish
 * @inode: inode to wait for
 *
 * Waits for all pending direct I/O requests to finish so that we can
 * proceed with a truncate or equivalent operation.
 *
 * Must be called under a lock that serializes taking new references
 * to i_dio_count, usually by inode->i_mutex.
 */
void inode_dio_wait(struct inode *inode)
{
	if (atomic_read(&inode->i_dio_count))
		__inode_dio_wait(inode);
}
EXPORT_SYMBOL(inode_dio_wait);
```

## inode lru
```c
static void inode_lru_list_add(struct inode *inode)
{
	if (list_lru_add(&inode->i_sb->s_inode_lru, &inode->i_lru))
		this_cpu_inc(nr_unused);
	else
		inode->i_state |= I_REFERENCED;
}
```


## inode_hashtable

```c
/**
 *	__insert_inode_hash - hash an inode
 *	@inode: unhashed inode
 *	@hashval: unsigned long value used to locate this object in the
 *		inode_hashtable.
 *
 *	Add an inode to the inode hash for this superblock.
 */
void __insert_inode_hash(struct inode *inode, unsigned long hashval)
{
	struct hlist_head *b = inode_hashtable + hash(inode->i_sb, hashval);

	spin_lock(&inode_hash_lock);
	spin_lock(&inode->i_lock);
	hlist_add_head(&inode->i_hash, b);
	spin_unlock(&inode->i_lock);
	spin_unlock(&inode_hash_lock);
}
EXPORT_SYMBOL(__insert_inode_hash);
```


## evict : 关键位置

```c
/*
 * Free the inode passed in, removing it from the lists it is still connected
 * to. We remove any pages still attached to the inode and wait for any IO that
 * is still in progress before finally destroying the inode.
 *
 * An inode must already be marked I_FREEING so that we avoid the inode being
 * moved back onto lists if we race with other code that manipulates the lists
 * (e.g. writeback_single_inode). The caller is responsible for setting this.
 *
 * An inode must already be removed from the LRU list before being evicted from
 * the cache. This should occur atomically with setting the I_FREEING state
 * flag, so no inodes here should ever be on the LRU when being evicted.
 */
static void evict(struct inode *inode)
```


## ino : 为什么需要这个东西

```c
unsigned int get_next_ino(void)
{
	unsigned int *p = &get_cpu_var(last_ino);
	unsigned int res = *p;

#ifdef CONFIG_SM
	if (unlikely((res & (LAST_INO_BATCH-1)) == 0)) {
		static atomic_t shared_last_ino;
		int next = atomic_add_return(LAST_INO_BATCH, &shared_last_ino);

		res = next - LAST_INO_BATCH;
	}
#endif

	res++;
	/* get_next_ino should not provide a 0 inode number */
	if (unlikely(!res))
		res++;
	*p = res;
	put_cpu_var(last_ino);
	return res;
}
EXPORT_SYMBOL(get_next_ino);
```

## address_space 是嵌入到 inode 中的

```c
struct inode {
  // ...
	struct address_space	i_data;
```

编译内核的时候：
```txt
    inode_init_always+5
    alloc_inode+49
    iget_locked+231
    __ext4_iget+305
    ext4_lookup+267
    __lookup_slow+131
    walk_component+219
    path_lookupat+103
    filename_lookup+232
    vfs_statx+158
    vfs_fstatat+85
    __do_sys_newfstatat+63
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 14039
```


之后传递给映射文件的 page 上:
```c
struct address_space *page_mapping(struct page *page)
{
	return folio_mapping(page_folio(page));
}
```
