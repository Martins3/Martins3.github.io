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

4. 回忆 ext2 的结构: bitmap(管理free blocks) + inode table(存储inode 结构体) + data block 的内容
    1. 也就是说 inode number 是用于定位 inode table 的
    2. 如果存在多个文件系统，在内存中间的 inode number 此时其含义是什么 ?
        1. 猜测是 : inode number 在内存中间也是 disk inode，所以不同的内存的inode 结构体可以持有相同的inode number
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

