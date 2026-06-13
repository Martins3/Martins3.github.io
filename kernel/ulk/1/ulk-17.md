# Understand Linux Kernel : Page Frame Reclaiming

## Questions

In previous chapters, *we explained how the kernel handles dynamic memory by keeping track of free and busy page frames.*
We have also discussed how every process in User Mode has its own address space and has its requests for memory satisfied by the kernel one page at a time, so that page frames can be assigned to the process at the very last possible moment.
Last but not least, we have shown how the kernel makes use of dynamic memory to implement both *memory and disk caches.*
> @todo 前面的章节哪里 track free and busy page frames 过 ?
> disk caches 应该指的是inode cache 之类的，但是 memory cache 应该指的是 page cache 之类的东西

The last main section, “Swapping,” is almost a chapter by itself: it covers the
swap subsystem, a kernel component used to save anonymous (not mapping data of files) pages on disk.


## 1 The Page Frame Reclaiming Algorithm

One of the fascinating aspects of Linux is that the checks performed before allocating
dynamic memory to User Mode processes or to the kernel are somewhat perfunctory.
> 由于存在 memcontrol 的存在，所以，@todo 是不是可以现在一个 group 持有 cache 的数量，或者说，其 cache 数量和正常的memory 的数量是分开管理的


#### 1.1 Selecting a Target Page

Table 17-1. The types of pages considered by the PFRA

| Type of pages | Description                                                                                                                                                                                                                                                                                                              | Reclaim action                                            |
|---------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------|
| Unreclaimable | Free pages (included in buddy system lists) <br>Reserved pages (with PG_reserved flag set) <br>Pages dynamically allocated by the kernel <br>Pages in the Kernel Mode stacks of the processes <br>Temporarily locked pages (with PG_locked flag set) <br>Memory locked pages (in memory regions with VM_LOCKED flag set) | (No reclaiming allowed or neded)                          |
| Swappable     | Anonymous pages in User Mode address spaces <br> Mapped pages of tmpfs filesystem (e.g., pages of IPC shared memory)                                                                                                                                                                                                     | Save the page contents in a swap area                     |
| Syncable      | Mapped pages in User Mode address spaces <br>Pages included in the page cache and containing data of disk files <br>Block device buffer pages <br>Pages of some disk caches (e.g., the inode cache)                                                                                                                  | Synchronize the page with its image on disk, if necessary |
| Discardable   | Unused pages included in memory caches (e.g., slab allocator caches) <br>Unused pages of the dentry cache                                                                                                                                                                                                                | Nothing to be done                                        |
> 1. swappable : 也是需要处理 tmpfs 的
> 2. Syncable 清晰的区分了两个 mapped 和 io page cache
> 3. 对于 Discardable 的需要一定的理解

As we’ll see in Chapter 19, the `tmpfs` special filesystem is used by the IPC shared memory mechanism.

A shared page frame belongs to multiple User Mode address spaces, while a
non-shared page frame belongs to just one. Notice that a non-shared page frame might
belong to several lightweight processes referring to the same memory descriptor.

Shared page frames are typically created when a process spawns a child; as explained
in the section “Copy On Write” in Chapter 9, the page tables of the child are copied
from those of the parent, thus parent and child share the same page frames. Another
common case occurs when two or more processes access the same file by means of a
shared memory mapping (see the section “Memory Mapping” in Chapter 16).

It should be noted, however, that when a single process accesses a file through a shared memory mapping,
the corresponding pages are non-shared as far as the PFRA is concerned. Similarly, a page belonging to a private memory mapping may be treated as shared by the PFRA (for instance, because two processes read the
same file portion and none of them modified the data in the page).
> @todo 这个 footnote 看不懂啊

#### 1.2 Design of the PFRA

Looking too close to the trees’ leaves might lead us to miss the whole forest.
Therefore, let us present a few general rules adopted by the PFRA. These rules are embedded in the functions that will be described later in this chapter.

- *Free the “harmless” pages first*

  Pages included in disk and memory caches not referenced by any process should
  be reclaimed before pages belonging to the User Mode address spaces of the processes;
  in the former case, in fact, the page frame reclaiming can be done without modifying any Page Table entry. As we will see in the section “The Least
  Recently Used (LRU) Lists” later in this chapter, this rule is somewhat mitigated
  by introducing a “swap tendency factor.”

- *Make all pages of a User Mode process reclaimable*

  With the exception of locked pages, the PFRA must be able to steal any page of a
  User Mode process, including the anonymous pages. In this way, processes that
  have been sleeping for a long period of time will progressively lose all their page
  frames.

- *Reclaim a shared page frame by unmapping at once all page table entries that reference it*

  When the PFRA wants to free a page frame shared by several processes, it clears
  all page table entries that refer to the shared page frame, and then reclaims the
  page frame.

- *Reclaim “unused” pages only*

  The PFRA uses a simplified Least Recently Used (LRU) replacement algorithm to
  classify pages as in-use and unused. If a page has not been accessed for a long
  time, the probability that it will be accessed in the near future is low and it can
  be considered “unused;” on the other hand, if a page has been accessed recently,
  the probability that it will continue to be accessed is high and it must be considered as “in-use.” The PFRA reclaims only unused pages. This is just another
  application of the locality principle mentioned in the section “Hardware Cache”
  in Chapter 2.

  The main idea behind the LRU algorithm is to associate a counter storing the age
  of the page with each page in RAM—that is, the interval of time elapsed since
  the last access to the page. This counter allows the PFRA to reclaim only the oldest page of any process. Some computer platforms provide sophisticated support for LRU algorithms;
  unfortunately, 80x86 processors do not offer such a hardware feature, thus the Linux kernel cannot rely on a page counter that keeps
  track of the age of every page. To cope with this restriction, **Linux takes advantage of the Accessed bit included in each Page Table entry**, which is automatically set by the hardware when the page is accessed;
  moreover, the age of a page is represented by the position of the page descriptor in one of two different lists
  (see the section “The Least Recently Used (LRU) Lists” later in this chapter).

Therefore, the page frame reclaiming algorithm is a blend of several heuristics:
- Careful selection of the order in which caches are examined.
- Ordering of pages based on aging (least recently used pages should be freed before pages accessed recently).
- Distinction of pages based on the page state (for example, non-dirty pages are better candidates than dirty pages because they don’t have to be written to disk).
> 不仅仅是时间上的问题，而且不同的page 的释放难易程度也是不同的

## 2 Reverse Mapping
A trivial solution for reverse mapping would be to include in each page descriptor
additional fields to link together all the Page Table entries that point to the page
frame associated with the page descriptor.
> 假装的操作 : 所有的 pte 都被编程链表被管理起来
> 实际上 : page 所在的 vma 被放到 `address_space->i_mapping` 上，从链表变成了 tree 而已吗 ?
> 1. 并不是，interval tree 是 inode 上的，page 虽然也持有 address_space，但是只是减少访存次数而已，也就是，所有的用于映射同一个 file 的page frame 都是在同一个指向 address_space
> 2. 所以，一个 page 也不知道其映射了谁，但是可以知道该范围的 vma
> 3. 就算 page 调入到 vma 的范围的时候，还需要 page walk 确定是不是真的含有存在该 page
>
> anon 的操作 :
> 1. 和文件类似，第一个 vma 创建的时候，其相当于创建了一个文件(这一个文件就是av(anon_vma))，所以其持有一个 interval tree 的
> 2. 当一个 page 在 vma1 的位置创建，那么，vma1 fork 出来的任何 vma 都需要放到 vma1 对应的
> 3. avc 的功能 :
>  1. av 持有被管理的vma, 如此，当 page 做 rmap 的时候，可以对于其中 vma 进行分析。
>  2. vma 会持有其被挂入的所有 av，这个形成了一个链表，当新的 vma 从此处被复制的时候，那么需要该链表维持生活。
> 4. page 指向的是 av, 也就是首次 page fault 所在的 vma 对应的 av

First of all,the PFRA must have a way to determine whether the page to be reclaimed is shared or non-shared,
and whether it is mapped or anonymous. In order to do this, the kernel looks at two fields of the page descriptor: `_mapcount` and `mapping`.
The `_mapcount` field stores the number of Page Table entries that refer to the page frame.
The counter starts from -1: this value means that no Page Table entry references the page frame.
Thus, if the counter is zero, the page is non-shared, while if it
is greater than zero the page is shared. The `page_mapcount()` function receives the
address of a page descriptor and returns the value of its `_mapcount` plus one (thus, for
instance, it returns one for a non-shared page included in the User Mode address
space of some process).

The mapping field of the page descriptor determines whether the page is mapped or
anonymous, as follows:
• If the mapping field is NULL, the page belongs to the **swap cache** (see the section “The Swap Cache” later in this chapter).
• If the mapping field is not NULL and its least significant bit is 1, it means the page
is anonymous and the mapping field encodes the pointer to an `anon_vma` descriptor (see the next section, “Reverse Mapping for Anonymous Pages”).
• If the mapping field is non-NULL and its least significant bit is 0, the page is
mapped; the mapping field points to the address_space object of the corresponding file (see the section “The address_space Object” in Chapter 15).

The `try_to_unmap()` function, which receives as its parameter a pointer to a page
descriptor, tries to clear all the Page Table entries that point to the page frame
associated with that page descriptor.


#### 2.1 Reverse Mapping for Anonymous Pages
Anonymous pages are often shared among several processes. The most common case
occurs when forking a new process: as explained in the section “Copy On Write” in
Chapter 9, all page frames owned by the parent—including the anonymous pages—
are assigned also to the child. Another (quite unusual) case occurs when a process
creates a memory region specifying both the MAP_ANONYMOUS and `MAP_SHARED` flag: the
pages of such a region will be shared among the future descendants of the process.
> MAP_SHARED 居然也可以用于 anon 的情景

> @todo 当时的设计，并没有兴趣了解

* ***The try_to_unmap_anon() function***

* ***The try_to_unmap_one() function***
> 总体来说，其中描述的内容区别很大，@todo 有时间可以办逐条检查其中内容被替换到哪里

#### 2.2 Reverse Mapping for Mapped Pages
> skip 没有兴趣

* ***The priority search tree***
> skip

* ***The try_to_unmap_file( ) function***
> skip

## 3 Implementing the PFRA

#### 3.1 The Least Recently Used (LRU) Lists
If a page belongs to an LRU list, its PG_lru flag in the page descriptor is set.
Moreover, if the page belongs to the active list, the `PG_active` flag is set, while if it belongs to the inactive list, the `PG_active` flag is cleared.

Several auxiliary functions are available to handle the LRU lists:

* ***Moving pages across the LRU lists***

The `PG_referenced` flag in the page descriptor is used to double the number of
accesses required to move a page from the inactive list to the active list; it is also used
to double the number of “missing accesses” required to move a page from the active
list to the inactive list (see below).

* ***The mark_page_accessed( ) function***

> 列举了各种 mark_page_accessed 各种被调用的情况，其实就是当一个 page 被大费周章的调入进来，所以其
> @todo 我相信，不是说调用 page_referenced() 会让 page 降级，而是因为调用 page_referenced() 发现这个 page 在连续两次调用的过程中间都没有被访问过。不然，为什么要做 rmap 统计次数，直接降级不就完成了。

* ***The page_referenced( ) function***

* ***The refill_inactive_zone( ) function***
> 分析如今 shrink_active_list 的内容，建议全文背诵。



#### 3.2 Low On Memory Reclaiming
> 下面的内容对应了 direct reclaim 的标准轨迹，虽然变化非常大，但是就是那个意思，我认为此处的内容之所以难以理解，是因为各种调用的外部函数无法理解。
> 所有的内容也只是几页而已，建议全文背诵。

* ***The free_more_memory() function***

* ***The try_to_free_pages( ) function***

* ***The shrink_caches() function***

* ***The shrink_zone() function***

* ***The shrink_cache() function***

* ***The shrink_list() function***
> shrink_page_list 的功能，真正的处理各种操作

We have now reached the heart of page frame reclaiming. While the purpose of the
functions illustrated so far, from `try_to_free_pages()` to `shrink_cache()`, was to
select the proper set of pages candidates for reclaiming, the `shrink_list()` function
effectively tries to reclaim the pages passed as a parameter in the `page_list` list.

> @todo 看代码的时候，可以特地的划分一下两者

* ***The pageout() function***


shrink_lruvec 调用 shrink_list，会根据当前到底是 active 还是 inactive 分别处理，
当 shrink_active_list 现在造成障碍的的内容:
1. page_has_private
2. try_to_release_page
> @todo 认为两者都是和


shrink_inactive_list 以及其调用的 shrink_page_list 的麻烦非常多:
1. pageout() 几乎不知道其说明的内容是什么
2. page_check_dirty_writeback
3. page write backed 之类的

#### 3.3 Reclaiming Pages of Shrinkable Disk Caches
> skip 包括后面的整个章节
