# RAMBlock ，从热迁移的角度分析

这里重新温习一下概念，RAMBlock ，ram_list 和 ram address space ，然后分析热迁移中
dirty bitmap 从 kvm ，到 ram_list 到 RamBlock:bmap 的数据流动过程。

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

### [ ] 需要理一下 MemoryRegion 和 RAMBlock 的关系了
或者那几个关键结构体的关系

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


## dirty tracking 的三个 bitmap
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
