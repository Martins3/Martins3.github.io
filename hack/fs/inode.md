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
1. disk inode 如何加载到 memory inode : iget_locked 使用inode cache 查询， super_operations::alloc_inode 分配空间， ext2_iget 中间进行读取和装配。
2. inode 如何关联上 struct file 的 ?
3. inode 如何关联上 下层设备的 ?
4. inode 在 fs-writeback 的作用 ?
5. inode 在 page reclaim 的时候，是如何选择进行回收的 ?
6. inode 的整个生命流程描述: iget_locked => iput 加载到内存到写回磁盘的过程，从 create 到 delete 的


或者内容的整理为:
1. fs/inode.c (inode 的各种管理，evict 等操作) fs/namei.c (查询)
2. ext2/inode.c (这里居然放置的内容是 address_space_operation 的系统) ext2/namei.c (查询的支持，和查询到之后的操作)

inode 在查询的过程，inode 在 inode_operation 的工作体现，inode 
