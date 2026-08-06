# migration 中一共存在那些 thread

基本上关心这几个 thread :
1. background-snapshot thread
2. migration thrasd
3. incoming coroutine

## 总结如下结果
Source（发送端

- mig/src/main（migration/migration.c:3998）——outgoing migration 的整个迭代发送流程（dirty page 扫描、发 RAM、发设备状态）跑在独立线程里，不是
  coroutine。因为它是个长时间 CPU + I/O 密集的循环，而且要反复做 dirty logging 同步、stop-and-copy 等，放线程里最自然。
- 另外还有：multifd 发送线程（migration/multifd.c:851）、TLS 线程、return-path 线程（migration/migration.c:2523）、dirtyrate 测量线程。

Destination（接收端）：coroutine + 线程混合

- precopy 主加载：主线程里的 coroutine（process_incoming_migration_co），上一轮说的那些原因——qemu_loadvm_state() 操作大量设备状态和 QEMUFile，
  这些代码默认跑在主线程/BQL 上下文里，用 coroutine 让出比用线程加锁安全得多。
- postcopy：加载切换到独立线程 mig/dst/listen（migration/postcopy-ram.c:2217），就是上一条回答里注释说的 "Postcopy has standalone thread to do
  vm load"。另外还有一个 fault 线程（postcopy_ram_fault_thread，migration/postcopy-ram.c:1553）专门处理 userfaultfd 的 guest 缺页，向 source
  发 page request——这两个是 postcopy 特有的，因为 guest 已经在跑了，缺页处理必须和主加载并发。
- multifd 接收线程（migration/multifd.c:1573）。
- COLO 专用线程（migration/colo.c:952）。

为什么 precopy 接收用 coroutine 而发送用线程？

核心差别在于谁拥有相关状态的上下文：接收端加载 VM 状态几乎全是在操作主线程拥有的对象（QEMUFile、RAMBlock、设备 vmstate），所以留在主线程里、用
coroutine 做异步等待是侵入最小的方案；发送端的主循环主要是按 dirty bitmap 拷贝内存往 socket 写，自己一个线程跑反而简单，只在需要时通过 BQL/锁
与主线程同步。

## 为什么 background-snapshot 是一个单独的 thread

1. background-snapshot 和 outgoing migration 的逻辑很类似，也是在不断的写内存 所以需要一个 thread
2. 但是 background-snapshot 和 outgoing migration 逻辑差别很大，所以就单独构建了一个 thread

## bitmap_mutex 的作用是什么?
- 把 balloon、COLO、postcopy 都排除后，答案基本是：可以安全地去掉，实际访问收敛为单线程，仅剩一个良性的统计读 race。逐条说：

   剩下的访问者盘点

   bmap / clear_bmap / migration_dirty_pages 的全部写入路径（排除 balloon hint、colo_*、postcopy 后）：

   • 迁移线程：ram_save_iterate / ram_save_complete / migration_bitmap_sync / ram_find_and_save_block 全在这一个线程里。
   • 初始化/销毁（ram_list_init_bitmaps 分配 bmap migration/ram.c:2843、teardown :2449）：发生在迁移线程开始迭代之前和结束之后，由迁移生命周期 + BQL + ramlist lock
     串行化，不与迭代并发。
   • multifd 发送线程：我 grep 了 migration/multifd.c，完全没有 pss/bmap/bitmap 的访问——multifd 模式下仍是迁移线程扫 bitmap、填 MultiFDSendParams，发送线程只负责搬运
     。per-section sync（:3337/:3408）只是控制面握手。
   • guest 写内存产生的脏标记：落在 memory-region 的 dirty bitmap（ram_list.dirty_memory / KVM slot bitmap），那套有自己的原子操作和 DirtyMemoryBlocks 锁
     ；block->bmap 只在迁移线程里由 ramblock_sync_dirty_bitmap 合并进来。guest vCPU 不直接碰 bmap。

   唯一残留的跨线程访问：

   • query-migrate → migration/migration.c:1144 → ram_bytes_remaining()（migration/ram.c:475）本来就不持锁读 migration_dirty_pages。主线程读、迁移线程写。这是个
     race，但只是统计值——读到撕裂/过期值最多影响监控展示，不影响正确性（64 位对齐读实际上也是原子的）。而且这个 race 现在持锁状态下就存在，不是去掉锁才引入的。

   结论

   ┌───────────────────────┬───────────────────────────────────────────────┐
   │ 结构                  │ 排除 balloon/COLO/postcopy 后                 │
   ├───────────────────────┼───────────────────────────────────────────────┤
   │ pss[]                 │ 单线程（迁移线程），无 race                   │
   ├───────────────────────┼───────────────────────────────────────────────┤
   │ page_hint             │ 无人访问                                      │
   ├───────────────────────┼───────────────────────────────────────────────┤
   │ bmap / clear_bmap     │ 所有写都在迁移线程，无 race                   │
   ├───────────────────────┼───────────────────────────────────────────────┤
   │ migration_dirty_pages │ 写都在迁移线程；QMP 良性读 race（现状即如此） │
   └───────────────────────┴───────────────────────────────────────────────┘

   所以在这个受限配置下，bitmap_mutex 实际上退化为空转——锁的所有临界区都只剩一个线程进入。

   两个保留意见：

   1. fragile：这把锁的真正价值是防御性的。free-page-hint（virtio-balloon 里很常用）和 COLO 都是运行时特性，不是编译期能排除的。如果要在你的场景里去锁，应该在
      qemu_guest_free_page_hint() 等入口加断言（非迁移线程触发就直接 warn/bug），否则将来某个版本引入新的并发写者时不会有任何告警。
   2. upstream 保留它不是因为 postcopy——从 migration/ram.c:3264 注释看，precopy 下设计时考虑的唯一竞争者就是 qemu_guest_free_page_hint()。也就是说"去掉 balloon 就可
      以去锁"这个判断和上游的锁定意图是一致的。


## RAMState 中的 bitmap_mutex
<!-- 5e8bf3c3-e187-41ea-85b2-2d5b15c8b83e -->

1. struct RAMState 定义在 migration/ram.c 中，管理热迁移的状态
2. 持有两个 PageSearchStatus 分别管理
```c
/* State of RAM for migration */
struct RAMState {
    /*
     * PageSearchStatus structures for the channels when send pages.
     * Protected by the bitmap_mutex.
     */
    PageSearchStatus pss[RAM_CHANNEL_MAX];

    // ...

    /* number of dirty bits in the bitmap */
    uint64_t migration_dirty_pages;
    /*
     * Protects:
     * - dirty/clear bitmap
     * - migration_dirty_pages
     * - pss structures
     */
    QemuMutex bitmap_mutex;

    // ...
```

bitmap_mutex 一共使用的地方:
- migration_bitmap_sync : 从 kvm 哪里同步，需要持有锁
- ram_save_queue_pages : 又是 postcopy ，好烦
- ram_save_host_page : 只有 postcopy 模式才需要
- qemu_guest_free_page_hint
- ram_save_iterate
- ram_save_complete

- thread_start
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate
            - ram_save_iterate

这个分析结果和我想象的完全不一样，这相当于 dirty bitmap 的获取和使用也是 mutex 互斥的
这里锁让我豁然开朗啊，原来是给
```c
static int ram_save_iterate(QEMUFile *f, void *opaque)
{
    // ...
    /*
     * We'll take this lock a little bit long, but it's okay for two reasons.
     * Firstly, the only possible other thread to take it is who calls
     * qemu_guest_free_page_hint(), which should be rare; secondly, see
     * MAX_WAIT (if curious, further see commit 4508bd9ed8053ce) below, which
     * guarantees that we'll at least released it in a regular basis.
     */
```
qemu_guest_free_page_hint() 是 balloon 来优化掉那些不需要热迁移的 page 的。


第二个问题，为什么需要 pss 来跟踪遍历到哪里了，以及为什么需要 bitmap_mutex 来保护?

## [ ] migration_bitmap_sync 中的 rcu 是做什么的?
```c
    WITH_QEMU_LOCK_GUARD(&rs->bitmap_mutex) {
        WITH_RCU_READ_LOCK_GUARD() {
            RAMBLOCK_FOREACH_NOT_IGNORED(block) {
                ramblock_sync_dirty_bitmap(rs, block);
            }
            stat64_set(&mig_stats.dirty_bytes_last_sync, ram_bytes_remaining());
        }
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
