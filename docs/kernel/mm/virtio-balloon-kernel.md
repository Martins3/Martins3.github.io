# Virtio Balloon

调查下，这些函数的 backtrace:
- [ ] balloon_ack


## stat: 如何实现周期性的发送统计信息
- https://qemu.readthedocs.io/en/latest/interop/virtio-balloon-stats.html


- qemu::balloon_stats_poll_cb : QEMU 定时触发，通知 guest 需要统计信息
- kernel::stats_request : guest 接受到中断
- kernel::stats_handle_request : 统计所有信息，并且将
- qemu::virtio_balloon_receive_stats : 从 virtuequeue 中接受消息
- qemu::balloon_stats_get_all : qom 的 hook，virsh 从 VirtIOBalloon::stat 中阅读信息


- clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_rw
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - memory_region_dispatch_write
                    - access_with_adjusted_size
                      - memory_region_write_accessor
                        - virtio_queue_notify
                          - virtio_balloon_receive_stats

kernel code:
```c
/*
 * While most virtqueues communicate guest-initiated requests to the hypervisor,
 * the stats queue operates in reverse.  The driver initializes the virtqueue
 * with a single buffer.  From that point forward, all conversations consist of
 * a hypervisor request (a call to this function) which directs us to refill
 * the virtqueue with a fresh stats buffer.  Since stats collection can sleep,
 * we delegate the job to a freezable workqueue that will do the actual work via
 * stats_handle_request().
 */
static void stats_request(struct virtqueue *vq)
{
	struct virtio_balloon *vb = vq->vdev->priv;

	spin_lock(&vb->stop_update_lock);
	if (!vb->stop_update)
		queue_work(system_freezable_wq, &vb->update_balloon_stats_work);
	spin_unlock(&vb->stop_update_lock);
}
```
- 每次当 host 发过来请求，将任务放到 workqueu 中。
  - [ ] 什么统计会导致 sleep
  - [x] 为什么不可以 sleep : 因为在中断的上下文中。


- common_interrupt
  - __common_interrupt
    - handle_irq
      - generic_handle_irq_desc
        - handle_fasteoi_irq
          - handle_irq_event
            - handle_irq_event_percpu
              - __handle_irq_event_percpu
                - vp_vring_interrupt
                  - vring_interrupt
                    - vring_interrupt
                      - stats_request

## size

### balloon N 的实现
qemu 中，经过 QMP 到达 virtio_balloon_to_target
- virtio_balloon_to_target
  - dev->num_pages = (vm_ram_size - target) >> VIRTIO_BALLOON_PFN_SHIFT;
  - virtio_notify_config


当 guest 接受到消息之后，在 `virtio_balloon_queue_free_page_work` 中发起 workqueue

- common_interrupt
  - __common_interrupt
    - handle_irq
      - generic_handle_irq_desc
        - handle_fasteoi_irq
          - handle_irq_event
            - handle_irq_event_percpu
              - __handle_irq_event_percpu
                - vp_interrupt
                  - vp_config_changed
                    - virtio_config_changed
                      - __virtio_config_changed
                        - virtballoon_changed
                          - virtio_balloon_queue_free_page_work

然后在 workqueue 中间:
- ret_from_fork
  - kthread
    - worker_thread
      - process_one_work
        - update_balloon_size_func

- update_balloon_size_func
  - towards_target : 这个会导致进入 QEMU::virtio_balloon_get_config
  - leak_balloon
  - fill_balloon : 增大 balloon
    - balloon_page_alloc
    - balloon_page_enqueue
      - balloon_page_insert : 设置 page 的 `PAGE_MAPPING_MOVABLE`
    - tell_host : 将页面发送给 host，然后等待
  - update_balloon_size : 这将会进入到 QEMU::virtio_balloon_set_config，在其中更新 actual 数值。

如果接受 pages，那么最后在 QEMU 中间:
- virtio_balloon_handle_output
  - balloon_inflate_page
    - ram_block_discard_range
  - balloon_deflate_page
    - qemu_madvise(host_addr, rb_page_size, QEMU_MADV_WILLNEED)

### info balloon 的实现
不会进入到 guest 中，利用上次 balloon N 更新的 actual 直接返回。

## [ ] 这些 feature 需要逐个检查一下
```c
/* The feature bitmap for virtio balloon */
#define VIRTIO_BALLOON_F_MUST_TELL_HOST 0 /* Tell before reclaiming pages */
#define VIRTIO_BALLOON_F_STATS_VQ   1 /* Memory Stats virtqueue */
#define VIRTIO_BALLOON_F_DEFLATE_ON_OOM 2 /* Deflate balloon on OOM */
#define VIRTIO_BALLOON_F_FREE_PAGE_HINT 3 /* VQ to report free pages */
#define VIRTIO_BALLOON_F_PAGE_POISON    4 /* Guest is using page poisoning */
#define VIRTIO_BALLOON_F_REPORTING  5 /* Page reporting virtqueue */
```

- [ ] 如何连这个 feature 都没有，将会如何?

Linux 都是支持的。
- VIRTIO_BALLOON_F_FREE_PAGE_HINT : (kernel 86a559787e6f5cf662c081363f64a20cad654195)
- VIRTIO_BALLOON_F_REPORTING : (kernel 2e991629bcf55a43681aec1ee096eeb03cf81709) 这个不是写的很详细，也不懂 poison 的原理

- [ ] 可以用 qmp 来操作这些 property 吗?
- [ ] 使用 cmdline 来操作这个接口的流程是什么?

这个警告?
```txt
qemu-system-x86_64: queue_enable is only suppported in devices of virtio 1.0 or later.
qemu-system-x86_64: queue_enable is only suppported in devices of virtio 1.0 or later.
```

- 在老 QEMU 上，这些功能都可以支持吗?

## qemu 代码分析

- qmp 对外仅仅提供两个功能
  - virtio_balloon_stat
    - [ ] get_current_ram_size
  - virtio_balloon_to_target
    - dev->num_pages = (vm_ram_size - target) >> VIRTIO_BALLOON_PFN_SHIFT;
    - virtio_notify_config
- [ ] 但是存在 5 个 qeueu 啊

```c
struct VirtIOBalloon {
    VirtIODevice parent_obj;
    VirtQueue *ivq, *dvq, *svq, *free_page_vq, *reporting_vq;
    uint32_t free_page_hint_status;
    uint32_t num_pages; // 希望归还的页面
    uint32_t actual; // 实际捕获的页面 ?
    uint32_t free_page_hint_cmd_id;
    uint64_t stats[VIRTIO_BALLOON_S_NR];
    VirtQueueElement *stats_vq_elem;
    size_t stats_vq_offset;
    // 定时查询 ?
    QEMUTimer *stats_timer;
    IOThread *iothread;
    QEMUBH *free_page_bh;
    /*
     * Lock to synchronize threads to access the free page reporting related
     * fields (e.g. free_page_hint_status).
     */
    QemuMutex free_page_lock;
    QemuCond  free_page_cond;
    /*
     * Set to block iothread to continue reading free page hints as the VM is
     * stopped.
     */
    bool block_iothread;
    NotifierWithReturn free_page_hint_notify;
    int64_t stats_last_update;
    int64_t stats_poll_interval;
    uint32_t host_features;

    bool qemu_4_0_config_size;
    uint32_t poison_val;
};
```

## BALLOON_COMPACTION

```txt
config BALLOON_COMPACTION
    bool "Allow for balloon memory compaction/migration"
    def_bool y
    depends on COMPACTION && MEMORY_BALLOON
    help
      Memory fragmentation introduced by ballooning might reduce
      significantly the number of 2MB contiguous memory blocks that can be
      used within a guest, thus imposing performance penalties associated
      with the reduced number of transparent huge pages that could be used
      by the guest workload. Allowing the compaction & migration for memory
      pages enlisted as being part of memory balloon devices avoids the
      scenario aforementioned and helps improving memory defragmentation.
```

- isolate_movable_page : 中的，只有注册的页面才可以 isolate 的；
- move_to_new_folio : 将一个 balloon page 搬运到 newpage，需要通知 host 将 newpage 释放，将要使用 balloon page；
- putback_movable_pages : 如果 migrate 失败，将之前 isolate 的 page 释放掉。

## HYPERV_BALLOON

## 初始化
### kernel

- virtballoon_probe
  - init_vqs ：初始化接口
  - virtio_balloon_register_shrinker ：注册 scanner 的接口
    - virtio_balloon_shrinker_scan
    -  virtio_balloon_shrinker_count

### qemu

## 为什么 vmware 的 balloon 写的这么长

- linux git:(master) ✗ /home/martins3/core/linux/drivers/misc/vmw_balloon.c
- 内核中页存在 HyperV 的 balloon :
  - https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/hh831766(v=ws.11)?redirectedfrom=MSDN


## virtio

```diff
History:        #0
Commit:         997e120843e82609c8d99a9d5714e6cf91e14cbe
Author:         Denis V. Lunev <den@openvz.org>
Committer:      Michael S. Tsirkin <mst@redhat.com>
Author Date:    Thu 20 Aug 2015 05:49:49 AM CST
Committer Date: Tue 08 Sep 2015 06:32:11 PM CST

virtio_balloon: do not change memory amount visible via /proc/meminfo

Balloon device is frequently used as a mean of cooperative memory control
in between guest and host to manage memory overcommitment. This is the
typical case for any hosting workload when KVM guest is provided for
end-user.

Though there is a problem in this setup. The end-user and hosting provider
have signed SLA agreement in which some amount of memory is guaranted for
the guest. The good thing is that this memory will be given to the guest
when the guest will really need it (f.e. with OOM in guest and with
VIRTIO_BALLOON_F_DEFLATE_ON_OOM configuration flag set). The bad thing
is that end-user does not know this.

Balloon by default reduce the amount of memory exposed to the end-user
each time when the page is stolen from guest or returned back by using
adjust_managed_page_count and thus /proc/meminfo shows reduced amount
of memory.

Fortunately the solution is simple, we should just avoid to call
adjust_managed_page_count with VIRTIO_BALLOON_F_DEFLATE_ON_OOM set.

Signed-off-by: Denis V. Lunev <den@openvz.org>
CC: Michael S. Tsirkin <mst@redhat.com>
Signed-off-by: Michael S. Tsirkin <mst@redhat.com>

diff --git a/drivers/virtio/virtio_balloon.c b/drivers/virtio/virtio_balloon.c
index 8543c9a97307..7efc32945810 100644
--- a/drivers/virtio/virtio_balloon.c
+++ b/drivers/virtio/virtio_balloon.c
@@ -157,7 +157,9 @@ static void fill_balloon(struct virtio_balloon *vb, size_t num)
 		}
 		set_page_pfns(vb->pfns + vb->num_pfns, page);
 		vb->num_pages += VIRTIO_BALLOON_PAGES_PER_PAGE;
-		adjust_managed_page_count(page, -1);
+		if (!virtio_has_feature(vb->vdev,
+					VIRTIO_BALLOON_F_DEFLATE_ON_OOM))
+			adjust_managed_page_count(page, -1);
 	}

 	/* Did we get any? */
@@ -173,7 +175,9 @@ static void release_pages_balloon(struct virtio_balloon *vb)
 	/* Find pfns pointing at start of each page, get pages and free them. */
 	for (i = 0; i < vb->num_pfns; i += VIRTIO_BALLOON_PAGES_PER_PAGE) {
 		struct page *page = balloon_pfn_to_page(vb->pfns[i]);
-		adjust_managed_page_count(page, 1);
+		if (!virtio_has_feature(vb->vdev,
+					VIRTIO_BALLOON_F_DEFLATE_ON_OOM))
+			adjust_managed_page_count(page, 1);
 		put_page(page); /* balloon reference */
 	}
 }
```

## [ ] Out of puff! Can't get 1 pages
直接访问

似乎有时候会触发连续的这个报错

- 不科学，除非一直有

## [ ] 似乎有时候，设置 balloon 不会立刻得到响应

## 基本理解
QEMU 中:
- virtio_balloon_device_realize 中创建两个 virt queue 的

从 QEMU 这端会接受 page ，最后分别调用到:
```c
    if (vq == s->ivq) {
        balloon_inflate_page(s, section.mr,
                             section.offset_within_region, &pbp);
    } else if (vq == s->dvq) {
        balloon_deflate_page(s, section.mr, section.offset_within_region);
```

- QEMU_MADV_REMOVE
- QEMU_MADV_DONTNEED
```c
madvise(host_startaddr, length, QEMU_MADV_REMOVE); // share anonymous 采用此方法
madvise(host_startaddr, length, QEMU_MADV_DONTNEED);
```

```c
qemu_madvise(host_addr, rb_page_size, QEMU_MADV_WILLNEED);
```

- [ ] 关于 share anonymous 需要使用 QEMU_MADV_REMOVE 而不是 QEMU_MADV_DONTNEED
  - 大致原因，应该是使用 QEMU_MADV_DONTNEED, 如果是 shared anonymous 的，其实只能将 page table 删除，而不可以页面删除。
  - QEMU_MADV_REMOVE 和 QEMU_MADV_DONTNEED 的实现原理，暂时没有时间分析。
    - QEMU_MADV_REMOVE 在调用的时候，明显

- 我看了下内核的实现，感觉明显不科学啊?
```diff
History:        #0
Commit:         cdfa56c551bb48f286cfe1f2daa1083d333ee45d
Author:         David Hildenbrand <david@redhat.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Tue 06 Apr 2021 04:01:25 PM CST
Committer Date: Wed 16 Jun 2021 02:27:37 AM CST

softmmu/physmem: Fix ram_block_discard_range() to handle shared anonymous memory

We can create shared anonymous memory via
    "-object memory-backend-ram,share=on,..."
which is, for example, required by PVRDMA for mremap() to work.

Shared anonymous memory is weird, though. Instead of MADV_DONTNEED, we
have to use MADV_REMOVE: MADV_DONTNEED will only remove / zap all
relevant page table entries of the current process, the backend storage
will not get removed, resulting in no reduced memory consumption and
a repopulation of previous content on next access.

Shared anonymous memory is internally really just shmem, but without a
fd exposed. As we cannot use fallocate() without the fd to discard the
backing storage, MADV_REMOVE gets the same job done without a fd as
documented in "man 2 madvise". Removing backing storage implicitly
invalidates all page table entries with relevant mappings - an additional
MADV_DONTNEED is not required.

Fixes: 06329ccecfa0 ("mem: add share parameter to memory-backend-ram")
Reviewed-by: Peter Xu <peterx@redhat.com>
Reviewed-by: Dr. David Alan Gilbert <dgilbert@redhat.com>
Signed-off-by: David Hildenbrand <david@redhat.com>
Message-Id: <20210406080126.24010-3-david@redhat.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>

diff --git a/softmmu/physmem.c b/softmmu/physmem.c
index b78b30e7ba..c0a3c47167 100644
--- a/softmmu/physmem.c
+++ b/softmmu/physmem.c
@@ -3527,6 +3527,7 @@ int ram_block_discard_range(RAMBlock *rb, uint64_t start, size_t length)
         /* The logic here is messy;
          *    madvise DONTNEED fails for hugepages
          *    fallocate works on hugepages and shmem
+         *    shared anonymous memory requires madvise REMOVE
          */
         need_madvise = (rb->page_size == qemu_host_page_size);
         need_fallocate = rb->fd != -1;
@@ -3560,7 +3561,11 @@ int ram_block_discard_range(RAMBlock *rb, uint64_t start, size_t length)
              * fallocate'd away).
              */
 #if defined(CONFIG_MADVISE)
-            ret =  madvise(host_startaddr, length, MADV_DONTNEED);
+            if (qemu_ram_is_shared(rb) && rb->fd < 0) {
+                ret = madvise(host_startaddr, length, QEMU_MADV_REMOVE);
+            } else {
+                ret = madvise(host_startaddr, length, QEMU_MADV_DONTNEED);
+            }
             if (ret) {
                 ret = -errno;
                 error_report("ram_block_discard_range: Failed to discard range "
```

## 统计信息是如何导出的

## 如何理解 qapi_event_send_balloon_change
```c
    if (dev->actual != oldactual) {
        qapi_event_send_balloon_change(vm_ram_size -
                        ((ram_addr_t) dev->actual << VIRTIO_BALLOON_PFN_SHIFT));
    }
```

## TODO
```c
struct VirtIOBalloon {
    VirtIODevice parent_obj;
    VirtQueue *ivq, *dvq, *svq, *free_page_vq, *reporting_vq;
```
- 这几个 vqs 都是做什么的?
  - svq : 统计信息

- balloon 并不会将 Linux kernel 彻底压缩掉，但是其停手的规则是什么?

- qemu/softmmu/balloon.c 似乎没有任何作用了，似乎是因为以前 QEMU 中存在多个 balloon dirver，现在只是剩下 virtio balloon 了
  - 是因为考虑 HyperV 吗?

## 似乎两侧通信的格式是按照约定的

### QEMU 的解析
```c
static void balloon_stats_poll_cb(void *opaque)
{
    VirtIOBalloon *s = opaque;
    VirtIODevice *vdev = VIRTIO_DEVICE(s);

    if (s->stats_vq_elem == NULL || !balloon_stats_supported(s)) {
        /* re-schedule */
        balloon_stats_change_timer(s, s->stats_poll_interval);
        return;
    }

    virtqueue_push(s->svq, s->stats_vq_elem, 0);
    virtio_notify(vdev, s->svq);
    g_free(s->stats_vq_elem);
    s->stats_vq_elem = NULL;
}
```

- balloon_stats_poll_cb
  - virtqueue_push
    - virtqueue_fill

- virtio_balloon_receive_stats
  - elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
  - iov_to_buf : 从中解析出来 virtio_balloon_stat

- VirtQueueElement : 是 QEMU 自己创建出来的

```c
static void virtio_balloon_receive_stats(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOBalloon *s = VIRTIO_BALLOON(vdev);
    VirtQueueElement *elem;
    VirtIOBalloonStat stat;
    size_t offset = 0;

    elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
    if (!elem) {
        goto out;
    }

    if (s->stats_vq_elem != NULL) {
        /* This should never happen if the driver follows the spec. */
        virtqueue_push(vq, s->stats_vq_elem, 0);
        virtio_notify(vdev, vq);
        g_free(s->stats_vq_elem);
    }

    s->stats_vq_elem = elem;

    /* Initialize the stats to get rid of any stale values.  This is only
     * needed to handle the case where a guest supports fewer stats than it
     * used to (ie. it has booted into an old kernel).
     */
    reset_stats(s);

    while (iov_to_buf(elem->out_sg, elem->out_num, offset, &stat, sizeof(stat))
           == sizeof(stat)) {
        uint16_t tag = virtio_tswap16(vdev, stat.tag);
        uint64_t val = virtio_tswap64(vdev, stat.val);

        offset += sizeof(stat);
        if (tag < VIRTIO_BALLOON_S_NR)
            s->stats[tag] = val;
    }
    s->stats_vq_offset = offset;
    s->stats_last_update = g_get_real_time() / G_USEC_PER_SEC;

out:
    if (balloon_stats_enabled(s)) {
        balloon_stats_change_timer(s, s->stats_poll_interval);
    }
}
```

### 内核如何传输这两个数值

```c
static void stats_handle_request(struct virtio_balloon *vb)
{
	struct virtqueue *vq;
	struct scatterlist sg;
	unsigned int len, num_stats;

	num_stats = update_balloon_stats(vb);

	vq = vb->stats_vq;
	if (!virtqueue_get_buf(vq, &len))
		return;
	sg_init_one(&sg, vb->stats, sizeof(vb->stats[0]) * num_stats);
	virtqueue_add_outbuf(vq, &sg, 1, vb, GFP_KERNEL);
	virtqueue_kick(vq);
}
```

- sg_init_one : 将 struct virtio_balloon_stat stats[VIRTIO_BALLOON_S_NR]; 生成为一个 sg
- virtqueue_add_outbuf

## 具体代码上的分析
- free_page_hint_status

## 理解一下这个 feature
- VIRTIO_BALLOON_F_FREE_PAGE_HINT
- [ ] get_free_page_and_send

- 如何理解 virtballoon_freeze 和 virtballoon_restore 的功能:
```c
static struct virtio_driver virtio_balloon_driver = {
	.feature_table = features,
	.feature_table_size = ARRAY_SIZE(features),
	.driver.name =	KBUILD_MODNAME,
	.driver.owner =	THIS_MODULE,
	.id_table =	id_table,
	.validate =	virtballoon_validate,
	.probe =	virtballoon_probe,
	.remove =	virtballoon_remove,
	.config_changed = virtballoon_changed,
#ifdef CONFIG_PM_SLEEP
	.freeze	=	virtballoon_freeze,
	.restore =	virtballoon_restore,
#endif
};
```

## struct balloon_dev_info 和 virtio_balloon 的关系是什么
- 内核中同时支持 hyperv,vmware 和 virtio 接口，balloon_dev_info 是三者共通的内容。

## [ ] 如何 Host 或者 Guest 中有的 page 大小不是 4k 如何

```c
/* Size of a PFN in the balloon interface. */
#define VIRTIO_BALLOON_PFN_SHIFT 12
```

- [ ] 为什么 free page hint 功能需要使用 iothread，而 iothread 需要命令行的制定
  - 也就是一个 iothread 可以被多个功能使用哇

## tell_host
从 virtballoon_migratepage 中观察到的:

- [ ] 感觉不是增量的增加的?

```c
	/* The array of pfns we tell the Host about. */
	unsigned int num_pfns;
	__virtio32 pfns[VIRTIO_BALLOON_ARRAY_PFNS_MAX];
```

## VIRTIO_BALLOON_F_PAGE_POISON

- [ ] 看懂这个函数: virtballoon_validate

- 如果 !want_init_on_free() && !page_poisoning_enabled_static 的时候，说明，没有打开 poison 选项

feature 似乎是双向协商的

## VIRTIO_BALLOON_F_FREE_PAGE_HINT
<!-- a7568510-efb0-46c5-bd7b-ade9a407d9fa -->

migration 中，会尽量增大 balloon 的大小，然后告诉 host 那些页面自己是不需要的。
之后热迁移的时候，这些页面可以直接跳过。

```diff
History:        #0
Commit:         86a559787e6f5cf662c081363f64a20cad654195
Author:         Wei Wang <wei.w.wang@intel.com>
Committer:      Michael S. Tsirkin <mst@redhat.com>
Author Date:    Mon 27 Aug 2018 09:32:17 AM CST
Committer Date: Thu 25 Oct 2018 08:57:55 AM CST

virtio-balloon: VIRTIO_BALLOON_F_FREE_PAGE_HINT

Negotiation of the VIRTIO_BALLOON_F_FREE_PAGE_HINT feature indicates the
support of reporting hints of guest free pages to host via virtio-balloon.
Currenlty, only free page blocks of MAX_ORDER - 1 are reported. They are
obtained one by one from the mm free list via the regular allocation
function.

Host requests the guest to report free page hints by sending a new cmd id
to the guest via the free_page_report_cmd_id configuration register. When
the guest starts to report, it first sends a start cmd to host via the
free page vq, which acks to host the cmd id received. When the guest
finishes reporting free pages, a stop cmd is sent to host via the vq.
Host may also send a stop cmd id to the guest to stop the reporting.

VIRTIO_BALLOON_CMD_ID_STOP: Host sends this cmd to stop the guest reporting.
VIRTIO_BALLOON_CMD_ID_DONE: Host sends this cmd to tell the guest that the reported pages are ready to be freed.

Why does the guest free the reported pages when host tells it is ready to free?
This is because freeing pages appears to be expensive for live migration.
free_pages() dirties memory very quickly and makes the live migraion not
converge in some cases. So it is good to delay the free_page operation
when the migration is done, and host sends a command to guest about that.

Why do we need the new VIRTIO_BALLOON_CMD_ID_DONE, instead of reusing VIRTIO_BALLOON_CMD_ID_STOP?
This is because live migration is usually done in several rounds. At the
end of each round, host needs to send a VIRTIO_BALLOON_CMD_ID_STOP cmd to
the guest to stop (or say pause) the reporting. The guest resumes the
reporting when it receives a new command id at the beginning of the next
round. So we need a new cmd id to distinguish between "stop reporting" and
"ready to free the reported pages".
```


### 内存的使用

在虚拟机中可以观察到内存被扩展的很大，似乎是不会导致 oom 出现的。

因为普通的页是挂载到 balloon_dev_info::pages 上的，这些页面不可以随意收缩的，
因为这些页面已经
```c
/*
 * Balloon device information descriptor.
 * This struct is used to allow the common balloon compaction interface
 * procedures to find the proper balloon device holding memory pages they'll
 * have to cope for page compaction / migration, as well as it serves the
 * balloon driver as a page book-keeper for its registered balloon devices.
 */
struct balloon_dev_info {
	unsigned long isolated_pages;	/* # of isolated pages for migration */
	spinlock_t pages_lock;		/* Protection to pages list */
	struct list_head pages;		/* Pages enqueued & handled to Host */
	int (*migratepage)(struct balloon_dev_info *, struct page *newpage,
			struct page *page, enum migrate_mode mode);
};
```
而 hint 的页是挂载到 virtio_balloon::free_page_list 这里的:

因为普通的页是真的不能自己用的，而 hint 的页是不可以给其他人用的，所以可以 shrink 的，无需通知 Host 。

### 发送过程

INIT_WORK(&vb->report_free_page_work, report_free_page_func); ，然后当 virtio config 变化的时候，
那么就会自动的

- report_free_page_func
  - virtio_balloon_report_free_page
    - send_cmd_id_start :
    - send_free_pages
      - get_free_page_and_send
    - send_cmd_id_stop

- [ ] 为什么分配的时候，使用那么大的 order 啊?
  - 是不是，如果 order 很小的，就直接走常规路径了。

在 qemu 中存在两个调用路线:

1. 热迁移的同步路线
```c
void migration_bitmap_sync_precopy(bool last_stage)
{
    precopy_notify(PRECOPY_NOTIFY_BEFORE_BITMAP_SYNC, &local_err) // virtio_balloon_free_page_hint_notify 中的
    migration_bitmap_sync(ram_state, last_stage);
    precopy_notify(PRECOPY_NOTIFY_AFTER_BITMAP_SYNC, &local_err)
}
```

2. qemu 中的调用路径:
- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_bh_poll
            - aio_bh_call
              - virtio_ballloon_get_free_page_hints
                - get_free_page_hints
                  - qemu_guest_free_page_hint
                    - migration_clear_memory_region_dirty_bitmap_range
                      - migration_clear_memory_region_dirty_bitmap
                        - migration_clear_memory_region_dirty_bitmap
                          - memory_region_clear_dirty_bitmap

### 如何同步的
- virtio_balloon_free_page_hint_notify : 中看，
        qemu_mutex_lock(&s->free_page_lock); 状态更新和

和 virtio_ballloon_get_free_page_hints 也是需要获取锁的

### 为什么不会出现 dirty 丢失

可以理解这个机制中，guest 告知的 page 相当于说，
这些页面都是 clean 的，不用发送了。

因为 guest 可以随时调用到 virtio_ballloon_get_free_page_hints
所以，随时都可以过来清理掉 dirty ，然后配置上 bmap 。
(那么这里会存在问题，就是)

热迁移不会出现 dirty 丢失?
对于 dirty 的 page ，是先发送，然后清理掉 kvm_log_clear
那么这个中间状态可能有问题吗?

### 其他的问题

1. 为什么这个机制不可以和 postcopy 共存?

## VIRTIO_BALLOON_F_REPORTING

```diff
History:        #0
Commit:         b0c504f154718904ae49349147e3b7e6ae91ffdc
Author:         Alexander Duyck <alexander.h.duyck@linux.intel.com>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Tue 07 Apr 2020 11:05:05 AM CST
Committer Date: Wed 08 Apr 2020 01:43:39 AM CST

virtio-balloon: add support for providing free page reports to host

Add support for the page reporting feature provided by virtio-balloon.
Reporting differs from the regular balloon functionality in that is is
much less durable than a standard memory balloon.  Instead of creating a
list of pages that cannot be accessed the pages are only inaccessible
while they are being indicated to the virtio interface.  Once the
interface has acknowledged them they are placed back into their respective
free lists and are once again accessible by the guest system.

Unlike a standard balloon we don't inflate and deflate the pages.  Instead
we perform the reporting, and once the reporting is completed it is
assumed that the page has been dropped from the guest and will be faulted
back in the next time the page is accessed.

Signed-off-by: Alexander Duyck <alexander.h.duyck@linux.intel.com>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Reviewed-by: David Hildenbrand <david@redhat.com>
Acked-by: Michael S. Tsirkin <mst@redhat.com>
Cc: Andrea Arcangeli <aarcange@redhat.com>
Cc: Dan Williams <dan.j.williams@intel.com>
Cc: Dave Hansen <dave.hansen@intel.com>
Cc: Konrad Rzeszutek Wilk <konrad.wilk@oracle.com>
Cc: Luiz Capitulino <lcapitulino@redhat.com>
Cc: Matthew Wilcox <willy@infradead.org>
Cc: Mel Gorman <mgorman@techsingularity.net>
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
Link: http://lkml.kernel.org/r/20200211224657.29318.68624.stgit@localhost.localdomain
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

QEMU 提交信息:
```diff
History:        #0
Commit:         91b867191ddf73c64f51ca2ddb489b922ed4058e
Author:         Alexander Duyck <alexander.h.duyck@linux.intel.com>
Committer:      Michael S. Tsirkin <mst@redhat.com>
Author Date:    Wed 27 May 2020 12:14:07 PM CST
Committer Date: Wed 10 Jun 2020 02:18:04 AM CST

virtio-balloon: Provide an interface for free page reporting

Add support for free page reporting. The idea is to function very similar
to how the balloon works in that we basically end up madvising the page as
not being used. However we don't really need to bother with any deflate
type logic since the page will be faulted back into the guest when it is
read or written to.

This provides a new way of letting the guest proactively report free
pages to the hypervisor, so the hypervisor can reuse them. In contrast to
inflate/deflate that is triggered via the hypervisor explicitly.

Acked-by: David Hildenbrand <david@redhat.com>
Signed-off-by: Alexander Duyck <alexander.h.duyck@linux.intel.com>
Message-Id: <20200527041407.12700.73735.stgit@localhost.localdomain>
```

这个机制是 Guest 主动触发的？


- [ ] 好吧，我傻了

- 原来 page hinting 和 page reporing 和 poison 是有关系的?
```diff
History:        #0
Commit:         d74b78fabe043158e26196291d2e439b487395bd
Author:         Alexander Duyck <alexander.h.duyck@linux.intel.com>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Tue 07 Apr 2020 11:05:01 AM CST
Committer Date: Wed 08 Apr 2020 01:43:38 AM CST

virtio-balloon: pull page poisoning config out of free page hinting

Currently the page poisoning setting wasn't being enabled unless free page
hinting was enabled.  However we will need the page poisoning tracking
logic as well for free page reporting.  As such pull it out and make it a
separate bit of config in the probe function.

In addition we need to add support for the more recent init_on_free
feature which expects a behavior similar to page poisoning in that we
expect the page to be pre-zeroed.

Signed-off-by: Alexander Duyck <alexander.h.duyck@linux.intel.com>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Reviewed-by: David Hildenbrand <david@redhat.com>
Acked-by: Michael S. Tsirkin <mst@redhat.com>
Cc: Andrea Arcangeli <aarcange@redhat.com>
Cc: Dan Williams <dan.j.williams@intel.com>
Cc: Dave Hansen <dave.hansen@intel.com>
Cc: Konrad Rzeszutek Wilk <konrad.wilk@oracle.com>
Cc: Luiz Capitulino <lcapitulino@redhat.com>
Cc: Matthew Wilcox <willy@infradead.org>
Cc: Mel Gorman <mgorman@techsingularity.net>
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
Link: http://lkml.kernel.org/r/20200211224646.29318.695.stgit@localhost.localdomain
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

## 快速回答
- 为什么将 update_balloon_size_func 和 update_balloon_stats_func 设置为 workqueue 的?
  - 因为他们都会导致睡眠？

- 为什么这里的 hook 是空的？
```c
	if (virtio_has_feature(vb->vdev, VIRTIO_BALLOON_F_FREE_PAGE_HINT)) {
		names[VIRTIO_BALLOON_VQ_FREE_PAGE] = "free_page_vq";
		callbacks[VIRTIO_BALLOON_VQ_FREE_PAGE] = NULL;
	}
```
因为只需要 Guest 通知 Host，无需额外的。

## 是否会导致 Guest 中出现 swap 的出现
- 会
- 会触发很多 : Out of puff! Can't get 1 pages

## init_on_free=1 为什么会产生影响

如何理解这一段代码?
```c
	if (virtio_has_feature(vdev, VIRTIO_BALLOON_F_PAGE_POISON)) {
		/* Start with poison val of 0 representing general init */
		__u32 poison_val = 0;

		/*
		 * Let the hypervisor know that we are expecting a
		 * specific value to be written back in balloon pages.
		 *
		 * If the PAGE_POISON value was larger than a byte we would
		 * need to byte swap poison_val here to guarantee it is
		 * little-endian. However for now it is a single byte so we
		 * can pass it as-is.
		 */
		if (!want_init_on_free())
			memset(&poison_val, PAGE_POISON, sizeof(poison_val));

		virtio_cwrite_le(vb->vdev, struct virtio_balloon_config,
				 poison_val, &poison_val);
	}
```


<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
