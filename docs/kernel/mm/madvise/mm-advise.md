# madvise

- madvise 告知内核该范围的内存如何访问
- fadvise 告知内核该范围的文件如何访问，内核从而可以调节 readahead 的参数，或者清理掉该范围的 page cache
fadvise 很简单， mm/fadvise.c 的源代码只有 200 行，具体可以看看 Man fadvise64(2)

## 总体分析

下面覆盖 Linux v7.0.1 通用 UAPI 中的全部 27 个 `MADV_*`。定义位于
`include/uapi/asm-generic/mman-common.h`，主要实现位于 `mm/madvise.c`。

最初的五个传统 advice 随 Linux `madvise()` 在 2.4.0 出现。Torvalds 主线
Git 的完整历史从 2.6.12-rc2 才开始，因此没有可信的原始 Git commit；这里
不能用后来的头文件整理 commit 冒充功能引入 commit。

| 功能                   | 添加理由                                                                                                                                   | 作用                                                                                                              | Commit                                                                                                                                   | 首次版本     |
|------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| `MADV_NORMAL`          | 提供默认访问策略                                                                                                                           | 清除随机/顺序访问提示，恢复默认 readahead 行为                                                                    | 早于 Git 历史                                                                                                                            | Linux 2.4.0  |
| `MADV_RANDOM`          | 随机访问时普通预读会浪费 I/O 和 page cache                                                                                                 | 标记 VMA 为随机访问，尽量减少预读                                                                                 | 早于 Git 历史                                                                                                                            | Linux 2.4.0  |
| `MADV_SEQUENTIAL`      | 优化顺序扫描的吞吐和缓存占用                                                                                                               | 标记 VMA 为顺序访问，增强顺序预读，并更积极处理已经越过的页面                                                     | 早于 Git 历史                                                                                                                            | Linux 2.4.0  |
| `MADV_WILLNEED`        | 应用预告即将访问一段映射                                                                                                                   | 对文件映射执行 best-effort 预读；它不保证完成 prefault                                                            | 早于 Git 历史                                                                                                                            | Linux 2.4.0  |
| `MADV_DONTNEED`        | 应用已经不需要当前页面内容                                                                                                                 | 立即丢弃或解除页面映射；匿名内存再次访问得到零页，私有文件映射重新从文件读取                                      | 早于 Git 历史                                                                                                                            | Linux 2.4.0  |
| `MADV_REMOVE`          | 数据库需要丢弃共享 buffer pool 的一部分；释放 tmpfs 页面和 swap 后备存储；支持 UML 内存热插拔                                              | 对共享可写映射执行 hole punch，释放页面及文件后备空间；以后读取返回零                                             | [`f6b3ec238d12`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=f6b3ec238d12c8cc6cc71490c6e3127988460349) | Linux 2.6.16 |
| `MADV_DONTFORK`        | `fork()` 后 COW 会改变 pinned page 的物理地址，破坏 RDMA/InfiniBand DMA；也可减少 fork 开销                                                | 子进程不继承指定 VMA，访问对应地址会触发 `SIGSEGV`                                                                | [`f822566165dd`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=f822566165dd46ff5de9bf895cfa6c51f53bb0c4) | Linux 2.6.16 |
| `MADV_DOFORK`          | 撤销 `MADV_DONTFORK`                                                                                                                       | 恢复默认的 fork 继承行为                                                                                          | 同上                                                                                                                                     | Linux 2.6.16 |
| `MADV_HWPOISON`        | 测试内核硬件内存错误处理路径                                                                                                               | 人工将页面标记为硬件损坏，后续访问按真实 memory failure 处理，通常产生 `SIGBUS`                                   | [`9893e49d64a4`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=9893e49d64a4874ea67849ee2cfbf3f3d6817573) | Linux 2.6.32 |
| `MADV_MERGEABLE`       | KSM 原先使用 `/dev/ksm` ioctl，社区希望采用按地址范围的接口                                                                                | 将私有匿名 VMA 注册为 KSM 候选，允许合并内容相同的页面                                                            | [`d19f35248446`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=d19f352484467a5e518639ddff0554669c10ffab) | Linux 2.6.32 |
| `MADV_UNMERGEABLE`     | 撤销 KSM 注册                                                                                                                              | 停止合并并拆分已经合并的 KSM 页面                                                                                 | 同上                                                                                                                                     | Linux 2.6.32 |
| `MADV_SOFT_OFFLINE`    | 测试程序需要先构造特定页面状态，再测试软下线逻辑                                                                                           | 迁移页面内容并将原物理页隔离，通常不破坏进程数据                                                                  | [`afcf938ee0aa`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=afcf938ee0aac4ef95b1a23bac704c6fbeb26de6) | Linux 2.6.33 |
| `MADV_HUGEPAGE`        | THP 需要应用按 VMA 指定适合使用大页的关键内存                                                                                              | 持久标记 VMA 为 THP 候选，但不保证立即产生 THP                                                                    | [`a826e422420b`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=a826e422420b461a6247137c292ff83c4800354a) | Linux 2.6.38 |
| `MADV_NOHUGEPAGE`      | 稀疏或延迟敏感映射可能不适合 THP                                                                                                           | 禁止指定 VMA 使用 THP                                                                                             | [`1ddd6db43a08`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=1ddd6db43a08cba56c7ee920800980862086f1c3) | Linux 2.6.38 |
| `MADV_DONTDUMP`        | 避免 QEMU core dump 包含全部 guest RAM；保护密钥等敏感内存                                                                                 | 将 VMA 排除在 core dump 之外，优先级高于 `coredump_filter`                                                        | [`accb61fe7bb0`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=accb61fe7bb0f5c2a4102239e4981650f9048519) | Linux 3.4    |
| `MADV_DODUMP`          | 撤销 `MADV_DONTDUMP`                                                                                                                       | 恢复该 VMA 的默认 core dump 策略                                                                                  | 同上                                                                                                                                     | Linux 3.4    |
| `MADV_FREE`            | jemalloc、tcmalloc 等 allocator 需要低成本释放，避免把无用数据换出，同时允许页面在真正回收前快速复用                                       | 将匿名页面标记为 lazy-free；有内存压力才丢弃，期间若再次写入则取消释放                                            | [`854e9ed09ded`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=854e9ed09dedf0c19ac8640e91bcc74bc3f9e5c9) | Linux 4.5    |
| `MADV_WIPEONFORK`      | PRNG、OpenSSL、PKCS#11 等库需要在 fork 后自动重新初始化状态，同时不能让地址失效                                                            | 子进程保留 VMA，但内容全部变为零；适用于私有匿名映射                                                              | [`d2cd9ede6e19`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=d2cd9ede6e193dd7d88b6d27399e96229a551b19) | Linux 4.14   |
| `MADV_KEEPONFORK`      | 撤销 `MADV_WIPEONFORK`                                                                                                                     | 恢复 fork 时复制或继承原内容                                                                                      | 同上                                                                                                                                     | Linux 4.14   |
| `MADV_COLD`            | Android 比内核 LRU 更清楚哪些后台应用已变冷，希望优先回收它们，同时保留数据                                                                | 将页面从 active LRU 移到 inactive LRU；不立即回收、不丢数据                                                       | [`9c276cc65a58`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=9c276cc65a58faf98be8e56962745ec99ab87636) | Linux 5.4    |
| `MADV_PAGEOUT`         | 用户态希望主动回收已知长期不用的页面，而不是等待内存压力或杀进程                                                                           | 立即尝试回收页面；匿名页换出、脏文件页回写，内容仍被保留                                                          | [`1a4e58cce84e`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=1a4e58cce84ee88129d5d49c064bd2852b481357) | Linux 5.4    |
| `MADV_POPULATE_READ`   | QEMU、数据库和稀疏映射需要可靠 prefault；`MAP_POPULATE` 只能在 mmap 时使用且不能可靠报告错误，逐页 touch 又慢                              | 同步建立可读页表；类似逐页读取，不打破 COW，成功表示整个范围当时可读                                              | [`4ca9b3859dac`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=4ca9b3859dac14bbef0c27d00667bb5b10917adb) | Linux 5.14   |
| `MADV_POPULATE_WRITE`  | 同时需要预分配后备内存并建立可写页表                                                                                                       | 同步建立可写页表，可能分配后备存储并打破 COW；成功表示整个范围当时可写                                            | 同上                                                                                                                                     | Linux 5.14   |
| `MADV_DONTNEED_LOCKED` | 安全内存 allocator 和 `MLOCK_ONFAULT` 用户需要释放 locked VMA 的物理页；`munlock`、`madvise`、`mlock` 会产生额外 syscall、VMA 分裂和锁竞争 | 与 `MADV_DONTNEED` 相同，但显式允许处理 `mlock` 区域                                                              | [`9457056ac426`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=9457056ac426e5ed0671356509c8dcce69f8dee0) | Linux 5.18   |
| `MADV_COLLAPSE`        | khugepaged 的时机不可预测；TCMalloc、VM 迁移等希望主动恢复 THP，并让请求进程承担 CPU 和回收成本                                            | 同步把范围折叠成 THP；这是一次性操作，不给 VMA 添加持久策略，可绕过全局 THP `never`，但不能绕过 `MADV_NOHUGEPAGE` | [`7d8faaf15545`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=7d8faaf155454f8798ec56404faca29a82689c77) | Linux 6.1    |
| `MADV_GUARD_INSTALL`   | 使用大量独立 `PROT_NONE` VMA 作为 guard page 会消耗 VMA、触发 mmap syscall 和 `mmap_lock` 竞争                                             | 使用 PTE marker 安装轻量 guard；丢弃该位置的现有页面，随后访问产生致命 `SIGSEGV`                                  | [`662df3e5c376`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=662df3e5c37666d6ed75c88098699e070a4b35b5) | Linux 6.13   |
| `MADV_GUARD_REMOVE`    | 移除轻量 guard                                                                                                                             | 只清除范围内的 guard marker，其他页面保持不变                                                                     | 同上                                                                                                                                     | Linux 6.13   |

版本依据来自对应的主线 commit、主线 release tag 和
[`madvise(2)`](https://man7.org/linux/man-pages/man2/madvise.2.html)。

## 回收类 advice 的区别

| 接口            |       是否立即行动 |         是否丢失内容 |
|-----------------|-------------------:|---------------------:|
| `MADV_DONTNEED` |                 是 |                   是 |
| `MADV_FREE`     |         内存压力时 | 是，但再次写入可取消 |
| `MADV_COLD`     |         只调整 LRU |                   否 |
| `MADV_PAGEOUT`  | 立即尝试回收或换出 |                   否 |

`MADV_WILLNEED` 与 `MADV_POPULATE_*` 也不同：前者只是 best-effort
预读提示；后者是同步 prefault，并向调用者报告失败。

- MADV_COLD：`deactivate_page()` 将 page 从 active LRU 移到 inactive LRU。
- MADV_PAGEOUT：通过 `reclaim_pages()` 等回收路径主动回收页面，必要时执行换出或写回。

- MADV_FREE : 想要释放内存，但是不想立刻完成，可以推迟到系统内存不足的时候

相关讨论:
https://news.ycombinator.com/item?id=23216590

## MADV_POPULATE_WRITE

QEMU 中的代码，在做 MADV_POPULATE_WRITE 的时候需要持有 mmap_sem 的 read lock
```c
    /*
     * On Linux, the page faults from the loop below can cause mmap_sem
     * contention with allocation of the thread stacks.  Do not start
     * clearing until all threads have been created.
     */
    qemu_mutex_lock(&page_mutex);
    while (!memset_args->context->all_threads_created) {
        qemu_cond_wait(&page_cond, &page_mutex);
    }
    qemu_mutex_unlock(&page_mutex);
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
