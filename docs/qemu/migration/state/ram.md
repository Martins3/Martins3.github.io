# ram

## iterable 的设备: SaveVMHandlers

主要关联
- migration/ram.c
- migration/postcopy-ram.c
- migration/block-dirty-bitmap.c

```c
static SaveVMHandlers savevm_slirp_state = {
    .save_state = net_slirp_state_save,
    .load_state = net_slirp_state_load,
};
```

```c
static SaveVMHandlers savevm_dirty_bitmap_handlers = {
    .save_setup = dirty_bitmap_save_setup,
    .save_live_complete_postcopy = dirty_bitmap_save_complete,
    .save_live_complete_precopy = dirty_bitmap_save_complete,
    .has_postcopy = dirty_bitmap_has_postcopy,
    .state_pending_exact = dirty_bitmap_state_pending,
    .state_pending_estimate = dirty_bitmap_state_pending,
    .save_live_iterate = dirty_bitmap_save_iterate,
    .is_active_iterate = dirty_bitmap_is_active_iterate,
    .load_state = dirty_bitmap_load,
    .save_cleanup = dirty_bitmap_save_cleanup,
    .is_active = dirty_bitmap_is_active,
};
```

普通的内存热迁移:
```c
static SaveVMHandlers savevm_ram_handlers = {
    .save_setup = ram_save_setup,
    .save_live_iterate = ram_save_iterate,
    .save_complete = ram_save_complete,
    .has_postcopy = ram_has_postcopy,
    .state_pending_exact = ram_state_pending_exact,
    .state_pending_estimate = ram_state_pending_estimate,
    .load_state = ram_load,
    .save_cleanup = ram_save_cleanup,
    .load_setup = ram_load_setup,
    .load_cleanup = ram_load_cleanup,
    .resume_prepare = ram_resume_prepare,
    .save_postcopy_prepare = ram_save_postcopy_prepare,
};
```

vfio 热迁移:
```c
static const SaveVMHandlers savevm_vfio_handlers = {
    .save_prepare = vfio_save_prepare,
    .save_setup = vfio_save_setup,
    .save_cleanup = vfio_save_cleanup,
    .state_pending_estimate = vfio_state_pending_estimate,
    .state_pending_exact = vfio_state_pending_exact,
    .is_active_iterate = vfio_is_active_iterate,
    .save_live_iterate = vfio_save_iterate,
    .save_complete = vfio_save_complete_precopy,
    .save_state = vfio_save_state,
    .load_setup = vfio_load_setup,
    .load_cleanup = vfio_load_cleanup,
    .load_state = vfio_load_state,
    .switchover_ack_needed = vfio_switchover_ack_needed,
    /*
     * Multifd support
     */
    .load_state_buffer = vfio_multifd_load_state_buffer,
    .switchover_start = vfio_switchover_start,
    .save_complete_precopy_thread = vfio_multifd_save_complete_precopy_thread,
};
```

- coroutine_trampoline
  - process_incoming_migration_co
    - qemu_loadvm_state
      - qemu_loadvm_state_main
        - qemu_loadvm_section_start_full
          - net_slirp_state_load

## RAMState : ram_state
<!-- f59bf708-4be9-4985-acc9-dc068ed17494 -->

### RAMState 的作用是什么?

配合 source 的 migration thread 使用， 基本上所有的资源都是放到这里的

### 观察诡异的中间状态

在 migration/ram.c 中，虽然 ram_state 是全局静态变量（static RAMState *ram_state），但有不少函数把 RAMState *rs 作为参数传入，内部调用处再传
ram_state。这样的函数有：

迁移主流程相关：
- save_xbzrle_page() — ram.c:627
- migration_bitmap_clear_dirty() — ram.c:829
- ramblock_sync_dirty_bitmap() — ram.c:1006
- migration_update_rates() — ram.c:1043
- migration_trigger_throttle() — ram.c:1106
- migration_bitmap_sync() — ram.c:1134
- save_zero_page() — ram.c:1213
- ram_save_page() — ram.c:1303
- find_dirty_block() — ram.c:1364
- get_queued_page() — ram.c:1843
- migration_page_queue_free() — ram.c:1907
- ram_save_target_page() — ram.c:2045
- ram_page_hint_update() — ram.c:2130
- ram_save_host_page() — ram.c:2216
- ram_page_hint_valid() — ram.c:2278
- ram_page_hint_collect() — ram.c:2288
- ram_find_and_save_block() — ram.c:2315

postcopy / 故障页相关：
- postcopy_has_request() — ram.c:445
- unqueue_page() — ram.c:1418
- poll_fault_page() — ram.c:1463（以及 ram.c:1794 的 stub 版本）
- ram_save_release_protection() — ram.c:1495（以及 ram.c:1802 的 stub 版本）

状态管理相关：
- colo_bitmap_find_dirty() — ram.c:806
- ram_state_reset() — ram.c:2488
- migration_bitmap_clear_discarded_pages() — ram.c:2854
- ram_init_bitmaps() — ram.c:2867
- ram_state_resume_prepare() — ram.c:2918
- ram_dirty_bitmap_sync_all() — ram.c:4518

这些函数签名里都接收 RAMState *rs，调用处形如 RAMState *rs = ram_state;（如 ram.c:585、1707、1937、4561 等）再传下去。也就是说目前 ram_state
全局变量和参数传递是混用的——函数内部用 rs，但在回调入口（如 ram_save_setup、ram_save_iterate）里从全局 ram_state 取值。

#### 为什么没人推动这个变量彻底移除
目前看几乎所有的位置实际上都是在引用 ram.c 中定义的这个变量:
```c
static RAMState *ram_state;
```

```c
void ram_mig_init(void)
{
    qemu_mutex_init(&XBZRLE.lock);
    register_savevm_live("ram", 0, 4, &savevm_ram_handlers, &ram_state);
    ram_block_notifier_add(&ram_mig_ram_notifier);
}
```
这个代码极其逆天，ram.c 中一会直接使用参数，一会直接引用 ram_state 。

- QEMU 的惯例是 cleanup 搭功能改动的便车，像这个 commit 一样"改到哪儿顺手清到哪儿"，review 成本低、理由充分；独立的大规模 janitorial patch 反 而难推进。

```diff
History:        #0
Commit:         6a39ba7cab67da05b91e215142ce5781e77e5d9f
Committer:      Peter Xu <peterx@redhat.com>
Author Date:    Thu 17 Oct 2024 02:42:53 PM CST
Committer Date: Fri 01 Nov 2024 03:48:18 AM CST

migration: Remove "rs" parameter in migration_bitmap_sync_precopy

The global static variable ram_state in fact is referred to by the
"rs" parameter in migration_bitmap_sync_precopy. For ease of calling
by the callees, use the global variable directly in
migration_bitmap_sync_precopy and remove "rs" parameter.

The migration_bitmap_sync_precopy will be exported in the next commit.
```

## pss

find_dirty_block 中这段代码的作用:
```c
    if (pss->complete_round && pss->block == rs->last_seen_block &&
        pss->page >= rs->last_page) {
        /*
         * We've been once around the RAM and haven't found anything.
         * Give up.
         */
        return PAGE_ALL_CLEAN;
    }
```
就是为了看回绕，然后理解问题。


```txt
History:        #0
Commit:         d9e474ea564bc109bc6fc81323ae90a7c9e7f04f
Author:         Peter Xu <peterx@redhat.com>
Committer:      Juan Quintela <quintela@trasno.org>
Author Date:    Wed 12 Oct 2022 05:55:52 AM CST
Committer Date: Thu 15 Dec 2022 05:30:37 PM CST

migration: Teach PSS about host page

Migration code has a lot to do with host pages.  Teaching PSS core about
the idea of host page helps a lot and makes the code clean.  Meanwhile,
this prepares for the future changes that can leverage the new PSS helpers
that this patch introduces to send host page in another thread.

Three more fields are introduced for this:

  (1) host_page_sending: this is set to true when QEMU is sending a host
      page, false otherwise.

  (2) host_page_{start|end}: these point to the start/end of host page
      we're sending, and it's only valid when host_page_sending==true.

For example, when we look up the next dirty page on the ramblock, with
host_page_sending==true, we'll not try to look for anything beyond the
current host page boundary.  This can be slightly efficient than current
code because currently we'll set pss->page to next dirty bit (which can be
over current host page boundary) and reset it to host page boundary if we
found it goes beyond that.

With above, we can easily make migration_bitmap_find_dirty() self contained
by updating pss->page properly.  rs* parameter is removed because it's not
even used in old code.

When sending a host page, we should use the pss helpers like this:

  - pss_host_page_prepare(pss): called before sending host page
  - pss_within_range(pss): whether we're still working on the cur host page?
  - pss_host_page_finish(pss): called after sending a host page

Then we can use ram_save_target_page() to save one small page.

Currently ram_save_host_page() is still the only user. If there'll be
another function to send host page (e.g. in return path thread) in the
future, it should follow the same style.

Reviewed-by: Dr. David Alan Gilbert <dgilbert@redhat.com>
Signed-off-by: Peter Xu <peterx@redhat.com>
Reviewed-by: Juan Quintela <quintela@redhat.com>
Signed-off-by: Juan Quintela <quintela@redhat.com>
```

这么想，就是这个东西引入了复杂度的
```c
/* used by the search for pages to send */
struct PageSearchStatus {
    /* The migration channel used for a specific host page */
    QEMUFile    *pss_channel;
    /* Last block from where we have sent data */
    RAMBlock *last_sent_block;
    /* Current block being searched */
    RAMBlock    *block;
    /* Current page to search from */
    unsigned long page;
    /* Set once we wrap around */
    bool         complete_round;

    /* Whether we're sending a host page */
    bool          host_page_sending;
    /* The start/end of current host page.  Invalid if host_page_sending==false */
    unsigned long host_page_start;
    unsigned long host_page_end;
};
```


- thread_start
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate
            - ram_save_iterate
              - ram_find_and_save_block
                - pss_init
                - find_dirty_block (首先找一次 page)
                  - pss_find_next_dirty
                - ram_save_host_page (这里有一个大循环 ，不断的 find and search ，但是 boundary 只是一个 hostpage)
                  - pss_find_next_dirty

(我感觉这里有可以优化的可能，第一次 find_dirty_block 就是查找了，但是没有 clear bit ，
然后第二次又去查找，发送了 page 之后才去 clear bit )


似乎需要先 clear ，然后:
```txt
test_and_clear_bit(page, rb->bmap)
```

find_next_bit 本身很简单，复杂的是这个结构体其实是一个可重入的、跨多次调用存活的迭代游标，它同时承载了五六个不同功能的状态。逐个字段看就清楚了：

1. 迁移是迭代式的，搜索不能一次扫完（block、page）

ram_find_and_save_block()（migration/ram.c:2315）每被调用一次只找一个 host page 发出去就返回，外层 ram_save_iterate 循环要在每次调用之间检查带宽、pending 大小、是
否该进入 stop-and-copy。所以"扫到哪了"这个游标必须跨调用持久化，不能是局部变量。

2. 收敛判定需要知道"扫完一整轮"（complete_round，配合 rs->last_seen_block/last_page）

find_dirty_block() 在所有 RAMBlock 上环形扫描，绕回链表头时置 complete_round（migration/ram.c:1392-1394），然后在 ram.c:1369 比较 pss->page >= rs->last_page 判断
是否已回到本轮起点——这决定"内存全扫干净了、可以停机收尾"。ram.c:2327-2332 的注释还警告：last_seen_block 为 NULL 会让循环永转。这是迭代式迁移收敛逻辑的核心，省不掉
。

3. host 大页 vs guest 4K 页的粒度错配（host_page_sending/start/end）

脏页位图按 TARGET_PAGE_SIZE（4K）记录，但 RAM 后端可能是 2M/1G 大页。只要一个 host 页内任一 4K 脏了，就整个 host 页一起发（减少 header 开销、保证目标端也能落到大
页上）。发一个 host 页要跨多次 bitmap 查找，所以 pss_find_next_dirty() 里要把 find_next_bit 的上限钳到 host_page_end（ram.c:741-743）。这三个字段就是"正在批量发送
一个 host 页"的中途状态。

4. 线上协议压缩（last_sent_block）

save_page_header()（ram.c:522-539）里，连续发送同一 RAMBlock 的页时用 RAM_SAVE_FLAG_CONTINUE 省略块名——纯粹的带宽优化。

5. 多通道复用（pss_channel）

RAMState 里有 pss[RAM_CHANNEL_MAX] 数组（ram.c:373）：precopy 主通道、postcopy 紧急页通道（ram.c:1980-1992，绑定 postcopy_qemufile_src）各自持有一份游标。同一条
ram_save_host_page 代码路径靠 pss->pss_channel 写到正确的流上。另外 postcopy 的优先队列（get_queued_page，ram.c:1843）会直接改写 pss 游标跳到被请求的页。

## ram_save_host_page 是关键的调度点

整体调用链

```
  migration_thread                    migration/migration.c:3664
    └─ migration_iteration_run        migration/migration.c:3367
        └─ qemu_savevm_state_iterate
            └─ ram_save_iterate       migration/ram.c:3255
                └─ ram_find_and_save_block        ram.c:2315
                    ├─ get_queued_page            ram.c:1843  (postcopy 优先队列)
                    ├─ find_dirty_block           ram.c:1364  (正常扫位图)
                    └─ ram_save_host_page         ram.c:2216
                        └─ ram_save_target_page   ram.c:2045
                            ├─ rdma_control_save_page
                            ├─ save_zero_page     ram.c:1213
                            ├─ ram_save_multifd_page
                            └─ ram_save_page → save_xbzrle_page / save_normal_page
```

1. 脏页是怎么被"看见"的

迁移期间 guest 的写操作通过脏页跟踪机制记录：

• KVM 侧用 dirty ring / dirty bitmap，TCG 侧由 softmmu 标记，最终都汇总到 MemoryRegion 的 dirty bitmap。
• migration_bitmap_sync()（ram.c:1134）周期性调用 memory_global_dirty_log_sync() 把内核/listener 的脏位图拉回 QEMU，再由
  ramblock_sync_dirty_bitmap() 合并进每个 RAMBlock 自己的 rb->bmap——这是发送侧真正扫描的位图。
• 这个函数同时每 1 秒做一次统计：更新脏页速率（migration_update_rates），供 auto-converge 判断是否降 CPU 频率（migration_trigger_throttle）。

关键点：发送一页时 migration_bitmap_clear_dirty()（ram.c:2234）清的是 rb->bmap 里那一 bit，但内存的 dirty log 仍在继续记录；下一轮 sync 时如果
guest 又写过，bit 会重新置位——这就是"迭代"收敛的原理。

2. 迭代主循环：ram_save_iterate

migration_thread 不断跑 migration_iteration_run：先用 qemu_savevm_query_pending_iter 估算剩余脏页，如果 ≤ 阈值（由期望停机时间算出）且目的端确
认可切换，就走 migration_completion 收尾；否则调 qemu_savevm_state_iterate → ram_save_iterate 再送一批。

ram_save_iterate 的循环（ram.c:3285）有三个退出条件：

• migration_rate_exceeded(f)：本轮发送量超过带宽配额，限速退出；
• 每 64 页检查一次耗时，超过 MAX_WAIT（50ms）就主动让出 bitmap_mutex，避免长时间阻塞 free-page-hint 等其他使用者；
• ram_find_and_save_block 返回 0：这一圈没找到任何脏页（done = 1）。

每轮结束写 RAM_SAVE_FLAG_EOS 并 flush。

3. 找下一页：ram_find_and_save_block

每次调用只发送一个 host page，流程：

• get_queued_page：postcopy 阶段目的端缺页会发回请求，源端把这些页放进 src_page_requests 队列，优先于后台扫描发送；开了 background-snapshot 时
  还会 poll_fault_page 取 userfaultfd 写保护故障页。
• 队列空则 find_dirty_block：用 pss_find_next_dirty（find_next_bit 在 rb->bmap 上找下一个置位）在当前 RAMBlock 里找脏页；扫完一个 block 换下一
  个，扫完整个链表绕回头部并置 complete_round。如果绕完一整圈回到 last_seen_block/last_page 仍一无所获，返回 PAGE_ALL_CLEAN，本轮迭代结束。
• 找到脏页后调 ram_save_host_page。

4. 发送一个 host page：ram_save_host_page

这是核心，一次处理一个 host page 粒度的范围（在 hugetlbfs 大页场景下就是 2M/1G 内包含的所有 4K guest 页）：

1. pss_host_page_prepare 算出 [host_page_start, host_page_end) 边界（ram.c:2076）。guest 页大于 host 页（如 Alpha on x86）时退化为每次一页。
2. do-while 循环内对每个 guest 页：
    • migration_bitmap_clear_dirty 清位并返回该页是否脏——只有脏页才发送；
    • postcopy preempt 模式下发送前临时释放 bitmap_mutex（ram.c:2243），因为 rp-return 线程也在操作位图，避免发送 I/O 期间互相阻塞；
    • 调 ram_save_target_page 发送；大页中间每送一页做一次 migration_rate_limit()，保证大页内部也能被限速；
    • pss_find_next_dirty 跳到下一个脏 bit，pss_within_range 判断是否越出 host page 边界或 block 末尾。
3. 最后 ram_save_release_protection：postcopy 阶段对刚发完的页解除写保护。

注意"清位图"和"发送"不是原子的：清了 bit 之后发送途中 guest 再写，下一轮 sync 会重新标脏，保证不丢。

5. 单页发送：ram_save_target_page

分情况考虑:

- RDMA：直接交给 rdma_control_save_page；
- zero page：buffer_is_zero 检查全零页，只发 RAM_SAVE_FLAG_ZERO 头 + 一个字节，带宽收益很大；
- multifd：ram_save_multifd_page → multifd_queue_page 把页丢给多个发送线程并行压缩/传输（迁移线程本身不直接写网络）；
- 默认路径 ram_save_page：若 XBZRLE 已启用（第一轮完整扫描后自动开启，ram.c:1396）先尝试 save_xbzrle_page 发增量编码；否则 save_normal_page 写
  RAM_SAVE_FLAG_PAGE 头 + 整页数据，通常 qemu_put_buffer_async 异步发送。

6. 收敛与收尾

migration_iteration_run 里 pending.total_bytes <= s->threshold_size（阈值 = downtime_limit × 带宽）时进入 migration_completion：暂停 vCPU →
ram_save_complete（ram.c:3365）做最后一轮 bitmap sync 并发送剩余所有脏页（last_stage，此时 guest 已停，不会再产生新脏页）→ 发送设备状态 → 切到
目的端运行。停机时间本质上就等于"最后这批剩余脏页 + 设备状态"的传输时间。

如果脏页速率持续高于带宽（一直不收敛），auto-converge 会逐步限制 vCPU；达到 max_postcopy_bandwidth 或用户触发时可切 postcopy，之后流程换成
ram_save_host_page_urgent 响应目的端的缺页请求。

需要我再展开哪一段（比如 multifd 的页分发、postcopy 请求路径、或 bitmap sync 与 KVM dirty ring 的细节）可以说。

## RAMBlock

### 基本调用路径
- RAMBlock::host : 一般来说 HVA，也就是 mmap 的返回值

创建一个 RAM 的过程大致如此:
1. 创建一个 MemoryRegion / RamBlock，并且关联起来
2. mmap 出来一个 host virtual memory 当做 guest 的内存

- memory_region_init_ram : 创建出来 RAM, 但是 memory_region_set_readonly 不就让这里没有作用了
    - memory_region_init_ram_nomigrate
      - memory_region_init_ram_flags_nomigrate
        - qemu_ram_alloc
          - ram_block_add
            - dirty_memory_extend : 初始化 ram_list.dirty_memory , 使用的位置在 cpu_physical_memory_test_and_clear_dirty 和  cpu_physical_memory_snapshot_and_clear_dirty
            - phys_mem_alloc (qemu_anon_ram_alloc)
              - qemu_ram_mmap
                - mmap : 可见 RAMBlock 在初始化的时候会在 host virtual address space 中 map 出来一个空间

RAMBlock 结构体分析:
1. RAMBlock::host : host 的虚拟地址空间，存储 mmap 的返回值
2. RAMBlock::offset : 将所有的 RAMBlock 连续的放到一起，每一个 RAMBlock 的 offset，第一个加入的 offset 为 0
    - 通过 RAMBlock::offset 可以放一个 RAM 内的 page 知道在 RAMList::dirty_memory 对应的 bit 位

看一个在综合路径中的使用:
- get_page_addr_code : 从 guest 虚拟地址的 pc 获取 guest 物理地址的 pc
  - tlb_hit : 进行虚实转换获取 hva
  - get_page_addr_code_hostp
    - qemu_ram_addr_from_host_nofail : 通过 hva 获取 gpa
      - qemu_ram_addr_from_host
        - qemu_ram_block_from_host

## RAMBlock 中和热迁移相关的 bitmap 的功能
<!-- 5a5134f7-ff0e-4e52-bb1b-dcc20727d011 -->

```c
struct RAMBlock {
    struct rcu_head rcu;
    struct MemoryRegion *mr;
    uint8_t *host;
    uint8_t *colo_cache; /* For colo, VM's ram cache */
    ram_addr_t offset;
    ram_addr_t used_length;
    ram_addr_t max_length;
    void (*resized)(const char*, uint64_t length, void *host);
    uint32_t flags;
    /* Protected by the BQL.  */
    char idstr[256];
    /* RCU-enabled, writes protected by the ramlist lock */
    QLIST_ENTRY(RAMBlock) next;
    QLIST_HEAD(, RAMBlockNotifier) ramblock_notifiers;
    Error *cpr_blocker;
    int fd;
    uint64_t fd_offset;
    int guest_memfd;
    RamBlockAttributes *attributes;
    size_t page_size;
    /* dirty bitmap used during migration */
    unsigned long *bmap;

    /*
     * Below fields are only used by mapped-ram migration
     */
    /* bitmap of pages present in the migration file */
    unsigned long *file_bmap;
    /*
     * offset in the file pages belonging to this ramblock are saved,
     * used only during migration to a file.
     */
    off_t bitmap_offset;
    uint64_t pages_offset;

    /* Bitmap of already received pages.  Only used on destination side. */
    unsigned long *receivedmap;

    /*
     * bitmap to track already cleared dirty bitmap.  When the bit is
     * set, it means the corresponding memory chunk needs a log-clear.
     * Set this up to non-NULL to enable the capability to postpone
     * and split clearing of dirty bitmap on the remote node (e.g.,
     * KVM).  The bitmap will be set only when doing global sync.
     *
     * It is only used during src side of ram migration, and it is
     * protected by the global ram_state.bitmap_mutex.
     *
     * NOTE: this bitmap is different comparing to the other bitmaps
     * in that one bit can represent multiple guest pages (which is
     * decided by the `clear_bmap_shift' variable below).  On
     * destination side, this should always be NULL, and the variable
     * `clear_bmap_shift' is meaningless.
     */
    unsigned long *clear_bmap;
    uint8_t clear_bmap_shift;

    /*
     * RAM block length that corresponds to the used_length on the migration
     * source (after RAM block sizes were synchronized). Especially, after
     * starting to run the guest, used_length and postcopy_length can differ.
     * Used to register/unregister uffd handlers and as the size of the received
     * bitmap. Receiving any page beyond this length will bail out, as it
     * could not have been valid on the source.
     */
    ram_addr_t postcopy_length;
};
```

| 字段                 | 端     | 迁移阶段        | 粒度  | 作用             |
| -------------------- | ------ | --------------- | ----- | -------------    |
| `bmap`               | source | pre-copy / sync | 页    | 当前 dirty 页    |
| `clear_bmap`         | source | global sync     | chunk | 延迟清 dirty log |
| `clear_bmap_shift`   | source | global sync     | N/A   | chunk 大小       |
| `dirty_restore_bmap` | source | 多轮迁移        | 页    |                  |
| `receivedmap`        | dest   | postcopy        | 页    | 已接收页         |
| `postcopy_length`    | dest   | postcopy        | 区间  | 合法 RAM 上限    |

- bmap 这是最核心的概念, 表示 当前 RAM block 中哪些 guest 页是 dirty 的
- dirty_restore_bmap : 之前迁移轮次中被“跳过”的 dirty 页在 热度感知迁移（hot/cold page）和 内存换出（memory swap / ballooning）
也就是 “这页之前 dirty 过，但我们故意没传” 后续轮次可参考该信息进行策略决策(参考 ds ，这是真的吗?)

只有开启热迁移的时候，才会有 bmap 的创建:
- thread_start
  - start_thread
    - qemu_thread_start
      - migration_thread
        - qemu_savevm_state_setup
          - ram_save_setup
            - ram_init_all
              - ram_init_bitmaps
                - ram_list_init_bitmaps

### RamBlock::clear_bmap 的作用
<!-- c4cad885-7a21-432a-80fa-131990c98f1e -->

(这里没完全看懂，physmem 中的函数都有点难懂哦)

virtio-balloon 不是借用的 clear_bmap 的，clear_bmap 的意义
对应的位置需要告诉 kvm 等，dirty bitmap 位置需要清理掉。
一些优化就是，拆分成多次来清理，clear_bmap 的一个 bit 记录一个 chunk 也不是一个 page 。

clear_bmap 就是 QEMU 用来记录**“哪些内存块已经获取了脏页，但还没在内核中执行清除操作”**的账本。

访问 clear_bmap 的经典的两个位置大致如此

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate
            - ram_save_iterate
              - ram_find_and_save_block
                - ram_save_host_page
                  - migration_bitmap_clear_dirty
                    - migration_clear_memory_region_dirty_bitmap
                      - clear_bmap_test_and_clear
		      - memory_region_clear_dirty_bitmap
			- kvm_log_clear : 告诉 kvm 来清理

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_pending_exact
            - ram_state_pending_exact
              - migration_bitmap_sync_precopy
                - migration_bitmap_sync
                  - ramblock_sync_dirty_bitmap
                    - physical_memory_sync_dirty_bitmap
		      - memory_region_clear_dirty_bitmap (低速)
                      - clear_bmap_set (默认操作，记录在 clear_bmap 中)

## RAMList 和 ram_addr_t
<!-- 05b8d166-0c6c-4a28-8ff1-c546e86f6fef -->

简而言之，将所有的 RamBlock 连接到一起，构建 ram address space

所有的 page 的 dirty 都是记录在 `RAMList::DirtyMemoryBlocks::blocks` 中
给出一个 ram 中的一个 page，需要找到在 blocks 数组中的下标，于是发明了 ram addr
```c
typedef struct {
    struct rcu_head rcu;
    unsigned long *blocks[];
} DirtyMemoryBlocks;

typedef struct RAMList {
    QemuMutex mutex;
    RAMBlock *mru_block;
    /* RCU-enabled, writes protected by the ramlist lock. */
    QLIST_HEAD(, RAMBlock) blocks;
    DirtyMemoryBlocks *dirty_memory[DIRTY_MEMORY_NUM];
    uint32_t version;
    QLIST_HEAD(, RAMBlockNotifier) ramblock_notifiers;
} RAMList;
```
QEMU 使用 RAMBlock 来描述 ram，MemoryRegion 的类型是 ram，那么就会关联一个 RAMBlock

将所有的 RAMBlock 连续的连到一起，形成 RAMList ，一个 RAMBlock 在其中偏移量记录在 `RAMBlock::offset`, 显然，第一个 offset 为 0

find_ram_offset 中 RAM 的对齐至少为 0x40000
```c
        candidate = ROUND_UP(candidate, BITS_PER_LONG << TARGET_PAGE_BITS);
```

在 ram_list 中，RAMBlock 按照大小排序的。
```txt
pc.ram: offset=0 size=180000000
pc.bios: offset=180000000 size=40000
pc.rom: offset=180040000 size=20000
vga.vram: offset=180080000 size=800000
/rom@etc/acpi/tables: offset=180900000 size=200000
virtio-vga.rom: offset=180880000 size=10000
e1000.rom: offset=1808c0000 size=40000
/rom@etc/table-loader: offset=180b00000 size=10000
/rom@etc/acpi/rsdp: offset=180b40000 size=1000
```
任何一个 page 的 ram_addr = offset in RAM + `RAMBlock::offset`


## migration 中 dirty tracking 的三个 bitmap
<!-- 7af2190d-6c72-4a60-a3a3-b21b69273d01 -->

由于层次划分问题，dirty bitmap 出现在三个地方，在热迁移的过程中会进行搬移

一共有三个种类:
1. KVMSlot::dirty_bmap : 显然这个是暂存的做用，也就是从 kvm 中获取到了 dirty bitmap
之后存储在这里，之后
```c
typedef struct KVMSlot
{
    /* Dirty bitmap cache for the slot */
    unsigned long *dirty_bmap;
    unsigned long dirty_bmap_size;

    /* Cache of the offset in ram address space */
    ram_addr_t ram_start_offset;
```

2. `ram_list.dirty_memory[DIRTY_MEMORY_MIGRATION]`
KVMSlot 会存放其 bitmap 在 ram address space 的偏移（ram_start_offset）；
RAMBlock 存放的 bitmap，clear_bmap 都是基于 ram address space 的地址。

```c
typedef struct {
    struct rcu_head rcu;
    unsigned long *blocks[];
} DirtyMemoryBlocks;

typedef struct RAMList {
    QemuMutex mutex;
    RAMBlock *mru_block;
    /* RCU-enabled, writes protected by the ramlist lock. */
    QLIST_HEAD(, RAMBlock) blocks;
    DirtyMemoryBlocks *dirty_memory[DIRTY_MEMORY_NUM];
    unsigned int num_dirty_blocks;
    uint32_t version;
    QLIST_HEAD(, RAMBlockNotifier) ramblock_notifiers;
} RAMList;
```

3. RAMBlock::bitmap 和 RAMBlock::clear_bmap :  RAMBlock->host + RAMBlock->offset 计算得到内存脏页的 HVA 的 bitmap 设置为 0。

迁移过程:
1. 从 KVMSlot 到 ram_list 的同步:
- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_pending_exact
            - ram_state_pending_exact
              - migration_bitmap_sync_precopy
                - migration_bitmap_sync
                  - memory_global_dirty_log_sync
                    - memory_region_sync_dirty_bitmap
                      - kvm_log_sync
                        - kvm_physical_sync_dirty_bitmap
                          - kvm_slot_sync_dirty_pages
                            - physical_memory_set_dirty_lebitmap

- coroutine_trampoline
  - blk_aio_read_entry
    - blk_aio_complete
      - blk_aio_complete
        - virtio_blk_rw_complete
          - virtio_blk_req_complete
            - virtqueue_push
              - virtqueue_fill
                - virtqueue_unmap_sg
                  - dma_memory_unmap
                    - address_space_unmap
                      - invalidate_and_set_dirty
                        - physical_memory_set_dirty_range

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_dispatch_ready_handlers
                      - aio_dispatch_handler
                        - virtio_queue_notify_vq
                          - virtio_blk_handle_vq
                            - virtio_queue_set_notification
                              - virtio_queue_set_notification
                                - virtio_queue_split_set_notification
                                  - vring_set_avail_event
                                    - vring_set_avail_event
                                      - address_space_cache_invalidate
                                        - invalidate_and_set_dirty
                                          - physical_memory_set_dirty_range

2. 从 ram_list 到 RamBlock:bmap 的同步

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_pending_exact
            - ram_state_pending_exact
              - migration_bitmap_sync_precopy
                - migration_bitmap_sync
                  - ramblock_sync_dirty_bitmap
                    - physical_memory_sync_dirty_bitmap

其实合并起来调用，也就是这个结果:

```c
void migration_bitmap_sync_precopy(bool last_stage)
{
    Error *local_err = NULL;
    assert(ram_state);

    /*
     * The current notifier usage is just an optimization to migration, so we
     * don't stop the normal migration process in the error case.
     */
    if (precopy_notify(PRECOPY_NOTIFY_BEFORE_BITMAP_SYNC, &local_err)) { // 这里会有 virtio-balloon 的 hook
        error_report_err(local_err);
        local_err = NULL;
    }

    migration_bitmap_sync(ram_state, last_stage);

    if (precopy_notify(PRECOPY_NOTIFY_AFTER_BITMAP_SYNC, &local_err)) {
        error_report_err(local_err);
    }
}
```

- migration_bitmap_sync
  - memory_global_dirty_log_sync
    - memory_region_sync_dirty_bitmap (从各个后端中获取，kvm 只是后端之一，而且不要忘记了来自设备的 dirty)
      - kvm_log_sync
        - kvm_physical_sync_dirty_bitmap
          - kvm_slot_sync_dirty_pages
            - physical_memory_set_dirty_lebitmap
  - ramblock_sync_dirty_bitmap (同步到 bmap 中)
    - physical_memory_sync_dirty_bitmap

这里可以看到一共存在两次同步，
1. 从各个设备来源同步到 ram_list 中
2. ram_list 同步到 RamBlock::bmap 中，为什么会使用 ram_list 作为过渡

DirtyMemoryBlocks::blocks
生命周期短
语义是：
“自上次同步以来，新产生的 dirty 页”
同步后：
通常会被清空 / 复位

RAMBlock::bmap
生命周期贯穿迁移
语义是：
“仍待处理的工作集合”
只有在：
页面成功发送 / 确认
或策略决定丢弃
才会清 bit

(再问一次 codex ，结果如下:)

ram_list.dirty_memory[DIRTY_MEMORY_MIGRATION] 这套 DirtyMemoryBlocks 是全局的、按 guest physical address 编址的“实时脏页日志”。CPU/
KVM 把某页弄脏时，先记到这里；它按固定大小分块，主要是为了支持 RCU 下扩容和并发访问

RAMBlock::bmap 则是 migration 层自己的、按单个 RAMBlock 内偏移编址的“待发送页面集合”。它只在 migration 期间用。
迁移开始时它会先被置成全 1，表示首轮需要发送整个 RAMBlock，而不是只发“最近新脏”的页，见 migration/ram.c:2842。

两者关系是“前者喂给后者”：

- migration 做一次同步时，会把 dirty_memory[DIRTY_MEMORY_MIGRATION] 里的脏位搬到 rb->bmap
- 同时把全局 dirty log 清掉
- rb->bmap 保留这些页，直到真正发送后再清

所以核心区别是：
- DirtyMemoryBlocks：底层实时脏页来源，记录“自上次 clear 以来又脏了哪些页”
- RAMBlock::bmap：迁移工作队列，记录“这个 RAMBlock 还有哪些页需要发/重发”

因此 bmap 里可以有很多“并不是刚刚新脏”的页，比如首轮全量迁移时所有页都在 bmap 里；而 dirty_memory 更像增量日志。

## RAMBlock::clear_bmap
<!-- 03c98e59-18f9-4d53-8933-9310edcde8c7 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

- rb->bmap：QEMU 迁移层看到的“这些 guest page 需要发送”。
- rb->clear_bmap：这些页对应的底层 dirty bitmap/KVM dirty log，“还需要在真正发送前清掉一次”。

代码里的定义注释已经写得很直接：当 bit 被置位时，表示对应的内存 chunk 需要做一次 log-clear，而且这个 clear 被推迟执行。include/system/ramblock.h:67

工作流程大致是：
1. 做一次全局 dirty log sync 时，QEMU 把底层脏页同步进 rb->bmap。
2. 这时如果启用了 clear_bmap，并不立刻调用 memory_region_clear_dirty_bitmap()，而是先在 clear_bmap 里做标记。migration/ram.c:982
3. 等到后面真正要发送某个 page/chunk 前，才通过 migration_clear_memory_region_dirty_bitmap() 检查 clear_bmap，并把对应 chunk 的底层 dirty log 清掉。 migration/ram.c:749
4. 清完以后，如果这块内存又被 guest 改写，下一轮 dirty sync 就能重新捕获到。

这样设计的核心目的，是把“同步脏页”和“清底层 dirty log”解耦，避免清得太早或太粗。发送路径里的注释说得很关键：
必须在发送这个 chunk 之前清，因为只有这样，后续对这块内存的新写入才能在下一次 sync 中被重新记录；太晚清就会漏脏页。migration/ram.c:842

还有两个细节：

- clear_bmap 的粒度比 bmap 粗，一个 bit 可以代表多个 guest page，这由 clear_bmap_shift 决定。include/system/ramblock.h:74
- 它本质上是“待清除 chunk”的集合，所以 clear_bmap_test_and_clear() 是 test+clear，一次 chunk 只做一次真正的底层 clear。include/system/ramblock.h:215

## 当发送完成之后，QEMU 会将这些内存释放掉

- thread_start
  - start_thread
    - qemu_thread_start
      - migration_thread
        - qemu_savevm_state_setup
          - ram_save_setup
            - qemu_fflush
              - qemu_fflush
                - qemu_iovec_release_ram
                  - find_next_bit



## RamBlock 和 MemoryRegion 的关系
两个核心结构各自管什么

MemoryRegion — 逻辑视图，描述 guest 物理地址空间里"一段东西"，是 QOM 对象，组织成树（container/subregions/alias/priority)。include/system/memory.h:560:

```c
  struct MemoryRegion {
      bool romd_mode;
      bool ram;
      bool readonly;   /* For RAM regions */
      bool rom_device;
      ...
      RAMBlock *ram_block;
      const MemoryRegionOps *ops;   /* IO 设备用 */
      MemoryRegion *container;
      MemoryRegion *alias;          /* alias 用 */
      Int128 size;
      hwaddr addr;
      QTAILQ_HEAD(, MemoryRegion) subregions;
      ...
  };
```

RAMBlock — host 侧实际分配/映射的那块内存，以及迁移所需的全部元数据。include/system/ramblock.h:25:

```c
  struct RAMBlock {
      struct MemoryRegion *mr;      /* 反向指回唯一的 MemoryRegion */
      uint8_t *host;                /* host 虚拟地址 */
      ram_addr_t offset;            /* 在全局 ram_addr_t 空间中的偏移(dirty bitmap 索引用) */
      ram_addr_t used_length;
      ram_addr_t max_length;
      uint32_t flags;               /* RAM_SHARED/RAM_PMEM/RAM_READONLY/... */
      int fd;                       /* memfd / 文件后端 */
      ...
      unsigned long *bmap;          /* 迁移 dirty bitmap */
  };
```

一句话：MemoryRegion 回答"这段地址是什么、在地址空间哪里"，RAMBlock 回答"这段 RAM 的 host 内存在哪、迁移/dirty log 怎么跟"。

MemoryRegion 与 RAMBlock 是不是一一对应

从 RAMBlock 方向看：是。 每个 RAMBlock 分配时绑定唯一的 mr，见 qemu_ram_alloc_from_fd(system/physmem.c:2355）和 qemu_ram_alloc_internal(system/physmem.c:2528）里
的 new_block->mr = mr;。

从 MemoryRegion 方向看：只有带后备存储的 region 才有 ram_block。 绑定动作统一在 memory_region_set_ram_block(system/memory.c:1583):

```c
  mr->ram_block = rb;
```

各类 init 函数的行为（system/memory.c):

┌─────────────────────────────┬─────┬─────────────────────────┬──────────────────┬────────────────────────────┐
│ init 函数                   │ ram │ ops                     │ ram_block        │ 说明                       │
├─────────────────────────────┼─────┼─────────────────────────┼──────────────────┼────────────────────────────┤
│ init_io (:1574)             │ 否  │ 有                      │ 无               │ 纯 MMIO，没有 RAMBlock     │
├─────────────────────────────┼─────┼─────────────────────────┼──────────────────┼────────────────────────────┤
│ `init_ram*(:1596 等）       │ 是  │ 无                      │ 有               │ 普通 RAM                   │
├─────────────────────────────┼─────┼─────────────────────────┼──────────────────┼────────────────────────────┤
│ init_rom (:3670)            │ 是  │ 无                      │ 有               │ = init_ram + readonly=true │
├─────────────────────────────┼─────┼─────────────────────────┼──────────────────┼────────────────────────────┤
│ init_rom_device (:3683)     │ 是  │ 有                      │ 有               │ RAM + ops 混合（flash 用） │
├─────────────────────────────┼─────┼─────────────────────────┼──────────────────┼────────────────────────────┤
│ init_ram_device_ptr (:1691) │ 是  │ 有（ram_device_mem_ops) │ 有               │ 设备内存直接映射           │
├─────────────────────────────┼─────┼─────────────────────────┼──────────────────┼────────────────────────────┤
│ init_alias (:1701)          │ 否  │ 无                      │ 共享原 region 的 │ 只设 alias/alias_offset    │
├─────────────────────────────┼─────┼─────────────────────────┼──────────────────┼────────────────────────────┤
│ container（默认）           │ 否  │ 无                      │ 无               │ 只挂子 region              │
└─────────────────────────────┴─────┴─────────────────────────┴──────────────────┴────────────────────────────┘

所以准确说法是：RAMBlock → MemoryRegion 是多对一（严格说每个 RAMBlock 属于且只属于一个 region)；有后备存储的 MemoryRegion → RAMBlock 是一对一；而
IO/alias/container 这类 region 根本没有自己的 RAMBlock。 alias 经由 mr->alias 链最终解析到原 region 的 ram_block（例如 memory_region_get_fd, memory.c:2332 里的
while (mr->alias) mr = mr->alias;)。

"ROM" 在 QEMU 里其实是三个东西

1. 只读 RAM:memory_region_init_rom(memory.c:3670）就是普通 RAM region 置 readonly=true，底层照样有 RAMBlock，只是 guest 写入被忽略。
   memory_region_set_readonly(:2294）还可以动态切换。
2. ROM device(romd 模式）：用于 flash。memory_region_init_rom_device(:3683) = init_io + 额外分配一个 RAMBlock + rom_device=true。运行时通过
   memory_region_rom_device_set_romd(:2314）切换：
    • romd_mode=true：读直接走 RAMBlock（快路径，可当 RAM 用），写走 ops（擦写 flash);
    • romd_mode=false：读写都走 ops。
      判断函数是 memory_region_is_romd(memory.h:1475)= rom_device && romd_mode,memory access 热路径（如 physmem.c:3668）会把它当 RAM 处理。
3. loader 的 struct Rom(hw/core/loader.c:968)：完全另一层——固件镜像（bios、option rom、-kernel/-initrd）的加载列表，负责把文件内容读进 guest 内存、reset 时重新拷
   贝。和 MemoryRegion 没有直接对应关系，只是它加载的目标通常是上面第 1 类 rom region。

FlatView / FlatRange / MemoryRegionSection：拍平之后的视角

MemoryRegion 是树，但 KVM、TCG、DMA 需要的是"线性地址 → 哪段内存"，于是有拍平层：

• render_memory_region(system/memory.c:596）递归遍历树，按 priority/遮挡关系展开成 FlatView——一个按地址排序的 FlatRange 数组（memory.c:222，注释原文："Range of
  memory in the global map. Addresses are absolute."),FlatRange 里有 mr、offset_in_region、绝对地址 addr、romd_mode、readonly、dirty_log_mask。
• MemoryRegionSection(include/system/memory.h:97）是 FlatRange 对外暴露的只读形式，字段是 mr + offset_within_region + offset_within_address_space + size。它是两个
  关键路径的基本单位：
    • MemoryListener 回调（region_add/region_del/log_start...)——KVM 的 KVMMemoryListener 就是按 section 建/删 memslot、注册 dirty log;
    • address_space_translate / dispatch 热路径——返回的正是 section。

注意：一个 MemoryRegion 可以对应多个 FlatRange/Section（被高优先级 region 挖洞、alias 截取、子 region 覆盖都会切分），所以 section 和 RAMBlock 也不是一一对应；只
有整段可见的 RAM region 才会恰好产生一个覆盖整个 RAMBlock 的 section。

整体关系图

```
  AddressSpace ──root──> MemoryRegion 树 (container/subregion/alias/priority)
                              │ ram=true 的叶子
                              │ mr->ram_block ══════> RAMBlock ──> host 内存 (+迁移元数据)
                              │                       rb->mr ══════> 指回唯一 MR
                              ▼ render_memory_region()
                         FlatView = FlatRange[] (拍平、排序、按 priority 裁剪)
                              │
                              ▼ region_add/del (MemoryListener, 如 KVM)
                         MemoryRegionSection → KVM memslot / TCG dispatch / DMA translate
```

• rom 三类含义：readonly RAM region / rom_device 的 romd_mode(flash) / loader.c 的 Rom（固件加载列表）。
• 迁移只认 RAMBlock(dirty bitmap、idstr 注册都在 RAMBlock 层）;MMIO region 不参与迁移。
• 想看运行时实际布局，QMP/HMP 的 info mtree -f（对应 mtree_info, memory.c:3623）打印的就是 FlatView，可以直接对照这些结构。



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
