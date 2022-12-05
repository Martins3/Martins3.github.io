# address_space

1. dirty page 除了 address_space 的跟踪，还有什么位置？
2. 每一个process 对应一个address_space 还是，一类对应一个?
3. 控制page 是不是dirty 和 page belong to which process 的实现是不是含有重叠的位置？
4. address_space 同时在支持swap file , page cache 和　swap 如此相似，难道代码实现没有公用的部分吗 ?
5. radix_tree 进行管理，整个内核中间其实只有一个
6. 应该并不是每一个进程管理一个 address_space ,但是实际上，每一个inode 对应一个 address_space
7. 通过mmap 的实现，所有的vm_struct 都是和　inod 对应的吗，当为匿名映射的时候，虽然没有inode, 但是含有swap 的管理工作

todo:
1. radix_tree 的工作原理: radix_tree 的键和值是什么 ?


> *mapping* specifies the address space in which a page frame is located. *index is the offset within
> the mapping.* Address spaces are a very general concept used, for example, when reading a file
> into memory. An address space is used to associate the file contents (data) with the areas in
> memory into which the contents are read.
>
> By means of a small trick, mapping is able to hold not
> only a pointer, but also information on whether a page belongs to an anonymous memory area
> that is not associated with an address space. **If the bit with numeric value 1 is set in mapping, the
> pointer does not point to an instance of address_space** but to another data structure (anon_vma)
> that is important in the implementation of reverse mapping for anonymous pages;
> this structure is discussed in Section 4.11.2. Double use of the pointer is possible because address_space
> instances are always aligned with sizeof(long); the least significant bit of a pointer to this
> instance is therefore 0 on all machines supported by Linux.
> The pointer can be used directly if it points normally to an instance of address_space. If the trick
> involving setting the least significant bit to 1 is used, the kernel can restore the pointer by means
> of the following operation:
>     anon_vma = (struct anon_vma *) (mapping - PAGE_MAPPING_ANON)

这样说: page cache 使用address_space 而匿名映射实际上被替换为 anao_vma

mm/rmap.c 中间实现了反向映射，而且它的头注释介绍了锁的层级结构

> page_get_anon_vma 获取上含有锁的tricky 的机制，感觉其实没有什么特殊，那岂不是访问page 的所有变量都是需要锁机制的吗 ?
```c
struct anon_vma *page_get_anon_vma(struct page *page){
...
  anon_vma = (struct anon_vma *) (mapping - PAGE_MAPPING_ANON)
...
}
```
> rmap.c 有待进一步学习:



address_space 仅仅在page　cache 含有作用，实际上反向映射没有什么关系(不对吧，fs.txt 中间已经说明了含有连个，而且rmap 中间也是大量的使用了该文件)

> Of course, I have shown the situation in simplified form because file data are not generally stored contiguously on hard disk but are distributed over several smaller areas (this is discussed in Chapter 9). The
> kernel makes use of the address_space data structure4 to provide a set of methods to read data from
> a backing store — from a filesystem, for example. address_spaces therefore form an auxiliary layer to
> represent the mapped data as a contiguous linear area to memory management.
是不是说: 如何将一个disk 文件mapped 内存中间，由于disk 本身不是连续的，而且
而且此时address_space 的功能变化成为了，读写磁盘中间商（是virtual process　和 ***?***)

其中定义了 readpage 和 writepage 的功能，aops 的赋值在
```c
void ext4_set_aops(struct inode *inode)
```
但是其中的函数进一步被谁调用目前不清楚啊!

@todo 据说ext2_readpage 的内容非常简单。也说明了address_space的内容穿透到了vfs 层次。

> **Filesystem and block layers are linked by the address_space_operations discussed in Chapter 4.** In the
> Ext2 filesystem, these operations are filled with the following entries
```c
static int ext2_readpage(struct file *file, struct page *page) {
  return mpage_readpage(page, ext2_get_block);
}

static int ext2_writepage(struct page *page, struct writeback_control *wbc) {
  return block_write_full_page(page, ext2_get_block, wbc);
}
```
> 这就是一直想知道的fs 通向底层的道路啊，再一次ext_readpage 很麻烦，被单独放到ext4/readpage 中间了。


```c
    /*
     * For areas with an address space and backing store,
     * linkage into the address_space->i_mmap interval tree.
     */
    struct {
        struct rb_node rb;
        unsigned long rb_subtree_last;
    } shared;

    /*
     * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
     * list, after a COW of one of the file pages.  A MAP_SHARED vma
     * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
     * or brk vma (with NULL file) can only be in an anon_vma list.
   * @todo 这J8 英语说的是什么东西 ? 到底含有多少种类FLAGS, 分别表示什么含义 ?
     */
    struct list_head anon_vma_chain; /* Serialized by mmap_sem &
                      * page_table_lock */
    struct anon_vma *anon_vma;  /* Serialized by page_table_lock */
```
> mm_area_struct 中间的内容，注释说: 对于含有backing store 的 area, 那么放到i_mmap

i_mmap 在 address_space 中间定义
```c
    struct rb_root_cached   i_mmap;     /* tree of private and shared mappings */
```
@todo i_mmap 的具体使用规则也是不清楚的

doc:
1. 观察page 的定义，page 中间的定义支持各种类型，
其中，包括:  *page cache and anonymous pages*
> 1. 从buddy 系统中间分配的page 含有确定的类型吗, 除了上面的两个类型还有什么类型
> 2. ucore 如何管理page cache
> 3. 其他类型的page 都没有含有address_space, 是不是意味着这些page 永远不会被刷新出去，只是被内核使用的

在16章才是对于address_space的终极描述:
1. host  page_tree 的作用:
The link with the areas managed by an address space is established by means of a pointer to
an inode instance (of type struct inode) to specify the backing store and a root radix tree
(page_tree) with a list of all physical memory pages in the address space.(@question 这是说明一个backstore (disk ssd partition)对应一个文档，还是说仅仅对应一个file)
@question 所以什么时候创建的address_space

#### mmap and sys_read

一个进程为读写文件:
1. 使用mmap 的机制和read 的机制是不是有区别
2. 其中mmap 是不是特指将二进制镜像读入,一般的文件采用其他read vfs ext4 之类的
3. 或者说mmap 和 read 的关系到底是什么

* **filep**
1. filep 出现的位置在什么层次 ?
2. filep 包含的内容是什么 ?
3. filep 的出现就是为了支持process可以访问同一个文件，谁持有文件，也就是持有inode, 在inode 中间包含进程需要的信息，比如引用计数不就可以了吗,
为什么需要单独独立出来信息 ?
4. 其中的andress_space 的作用是什么 ？
5. file_operations 什么时候赋值 ?
6. 问题是inode 中间也包含file_operations 结构体，所以和struct file　的 file_operations 有什么区别 ?


* **`anon_vma`**

```c
/*
 * The anon_vma heads a list of private "related" vmas, to scan if
 * an anonymous page pointing to this anon_vma needs to be unmapped:
 * the vmas on the list will be related by forking, or by splitting.
 *
 * Since vmas come and go as they are split and merged (particularly
 * in mprotect), the mapping field of an anonymous page cannot point
 * directly to a vma: instead it points to an anon_vma, on whose list
 * the related vmas can be easily linked or unlinked.
 *
 * After unlinking the last vma on the list, we must garbage collect
 * the anon_vma object itself: we're guaranteed no page can be
 * pointing to this anon_vma once its vma list is empty.
 */
struct anon_vma {
```
> 为了管理 anonymous page 单独建立的数据结构，具体原因没有太看懂.


# fread 和 mmap 有什么区别
反正最后数据都是交给 page cache 中间了

1. fread 会一开始就将 整个文件缓存到内存中间吗 ? 应该不会，fopen 只是打开文件，而fread 的参数会指定大小，用户程序负责这些细节。
2. fread 打开的文件显然不在地址空间中间
3. 当fread 打开的文件会被共享吗 ? 应该不会 !
4. fread file 还会继续使用 radix tree 实现反向映射吗 ?
