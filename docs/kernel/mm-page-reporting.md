# page reporting

```diff
commit 36e66c554b5c6a9d17a229faca7a61693527b0bd
Author: Alexander Duyck <alexander.h.duyck@linux.intel.com>
Date:   Mon Apr 6 20:04:56 2020 -0700

    mm: introduce Reported pages

    In order to pave the way for free page reporting in virtualized
    environments we will need a way to get pages out of the free lists and
    identify those pages after they have been returned.  To accomplish this,
    this patch adds the concept of a Reported Buddy, which is essentially
    meant to just be the Uptodate flag used in conjunction with the Buddy page
    type.

    To prevent the reported pages from leaking outside of the buddy lists I
    added a check to clear the PageReported bit in the del_page_from_free_list
    function.  As a result any reported page that is split, merged, or
    allocated will have the flag cleared prior to the PageBuddy value being
    cleared.

    The process for reporting pages is fairly simple.  Once we free a page
    that meets the minimum order for page reporting we will schedule a worker
    thread to start 2s or more in the future.  That worker thread will begin
    working from the lowest supported page reporting order up to MAX_ORDER - 1
    pulling unreported pages from the free list and storing them in the
    scatterlist.

    When processing each individual free list it is necessary for the worker
    thread to release the zone lock when it needs to stop and report the full
    scatterlist of pages.  To reduce the work of the next iteration the worker
    thread will rotate the free list so that the first unreported page in the
    free list becomes the first entry in the list.

    It will then call a reporting function providing information on how many
    entries are in the scatterlist.  Once the function completes it will
    return the pages to the free area from which they were allocated and start
    over pulling more pages from the free areas until there are no longer
    enough pages to report on to keep the worker busy, or we have processed as
    many pages as were contained in the free area when we started processing
    the list.

    The worker thread will work in a round-robin fashion making its way though
    each zone requesting reporting, and through each reportable free list
    within that zone.  Once all free areas within the zone have been processed
    it will check to see if there have been any requests for reporting while
    it was processing.  If so it will reschedule the worker thread to start up
    again in roughly 2s and exit.

    Signed-off-by: Alexander Duyck <alexander.h.duyck@linux.intel.com>
    Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
    Acked-by: Mel Gorman <mgorman@techsingularity.net>
    Cc: Andrea Arcangeli <aarcange@redhat.com>
    Cc: Dan Williams <dan.j.williams@intel.com>
    Cc: Dave Hansen <dave.hansen@intel.com>
    Cc: David Hildenbrand <david@redhat.com>
    Cc: Konrad Rzeszutek Wilk <konrad.wilk@oracle.com>
    Cc: Luiz Capitulino <lcapitulino@redhat.com>
    Cc: Matthew Wilcox <willy@infradead.org>
    Cc: Michael S. Tsirkin <mst@redhat.com>
    Cc: Michal Hocko <mhocko@kernel.org>
    Cc: Nitesh Narayan Lal <nitesh@redhat.com>
    Cc: Oscar Salvador <osalvador@suse.de>
    Cc: Pankaj Gupta <pagupta@redhat.com>
    Cc: Paolo Bonzini <pbonzini@redhat.com>
    Cc: Rik van Riel <riel@surriel.com>
    Cc: Vlastimil Babka <vbabka@suse.cz>
    Cc: Wei Wang <wei.w.wang@intel.com>
    Cc: Yang Zhang <yang.zhang.wz@gmail.com>
    Cc: wei qi <weiqi4@huawei.com>
    Link: http://lkml.kernel.org/r/20200211224635.29318.19750.stgit@localhost.localdomain
    Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

QEMU 侧:
- virtio_balloon_handle_report

- virtio_balloon_handle_output 和 virtio_balloon_handle_report 存在什么区别吗?
  - report : 中唯一的检测了 VirtIOBalloon::poison_val，这也是唯一的位置的吧
  - 一个使用的 in_sq ，一个是 out_sg

- 这个机制似乎不错，让 guest 周期性的清理，可以将那些彻底不被使用的内存清理掉。

- [ ] 是不是太频繁了? 测试的速度感觉明显快于 2s 的

- page_reporting_process
  - page_reporting_process_zone
    - page_reporting_cycle : 对于每一个 order

- 为什么需要 flags ，因为这些插上 flags 的 page 可以继续被分配

```c
static inline void del_page_from_free_list(struct page *page, struct zone *zone,
					   unsigned int order)
{
	/* clear reported state and update reported page count */
	if (page_reported(page))
		__ClearPageReported(page);

	list_del(&page->buddy_list);
	__ClearPageBuddy(page);
	set_page_private(page, 0);
	zone->free_area[order].nr_free--;
}
```
