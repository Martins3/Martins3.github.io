# 热迁移中 share memory 会被自动 touch
## 为什么热迁移后，使用 memfd 的后端会自动的 touch 所有的内存

本来一直都以为，source 没有 touch 的内存到了 target 端，也是会保持

### qemu 的源码分析
使用 ./qemu-migration-target-touch-test.py 来配置:

memfd target 热迁移后 RSS 到 16G，不是因为 migration 传了 16G； 而是因为 target
收到每个 zero page marker 后，ram_handle_zero() 里的 buffer_is_zero() 读了目标
memfd 映射，导致 shmem page 被 fault/instantiate。

对应代码：

```c
case RAM_SAVE_FLAG_ZERO:
    // ...
    ram_handle_zero(host, TARGET_PAGE_SIZE);
```

```c
void ram_handle_zero(void host, uint64_t size)
{
    if (!buffer_is_zero(host, size)) {
        memset(host, 0, size);
    }
}
```

## memfd target 如何做到 zero page 不 touch

用 ./qemu-migration-target-touch-test.py 实测。环境：qemu-system-x86_64
10.2.2，TCG，source 用 memory-backend-file 5 GiB（全零，guest 未启动），
target 默认 memory-backend-memfd share=on。脚本支持环境变量：

- TARGET_BACKEND=ram|memfd：target 的内存后端
- MULTIFD=on：两端打开 multifd（8 channels），target 侧走
  multifd_recv_zero_page_process()
- POSTCOPY=on：两端打开 postcopy-ram，precopy 开始后立即
  migrate-start-postcopy

四组对照结果（target 进程迁移前后 /proc/pid/status）：

| 配置                       | RssShmem         | minflt 增量 | transferred |
|----------------------------|------------------|-------------|-------------|
| memfd + 经典单线程         | 0 -> 5242880 kB  | +1310883    | 12 MB       |
| memfd + multifd            | 0 -> 0           | +154        | 14 MB       |
| ram(匿名) + 经典单线程     | RssAnon 基本不变 | +2723       | 12 MB       |
| memfd + multifd + postcopy | 0 -> 5242880 kB  | +370435     | 3.9 GB      |

结论：

1. memfd + 经典路径：和源码分析一致，ram_handle_zero() 的 buffer_is_zero()
   把 5 GiB shmem 全部 fault 进来。只传了 12 MB 数据，RSS 却涨满 5 GiB。
2. memfd + multifd：首次收到的 zero page 直接跳过不 touch,RssShmem 保持
   0,minflt 几乎不动。这就是生产上 memfd 后端避免 RSS 暴涨的配置方法。
3. 匿名后端即使经典路径也不涨：read fault 直接挂全局 zero page，不分配
   真实页。
4. multifd + postcopy 是例外：代码里明确写了 postcopy 下 zero page 必须
   立即 memset（否则 postcopy 阶段 fault 该页时 receivedmap 已标记、无人
   服务会挂起），实测 RSS 照样涨满。而且 postcopy 会把 zero page 当普通页
   全量传输，transferred 从 12 MB 变成 3.9 GB。

所以 memfd target 要避免 zero page 撑爆 RSS，配置就是：QEMU >= 9.1
（跳过逻辑 5ef7e26bdb7e 在此版本合入）+ 两端开 multifd + 不开 postcopy:

```txt
migrate_set_capability multifd on
migrate_set_parameter multifd-channels 8
```

libvirt 对应 virsh migrate --parallel --parallel-connections 8。
注意 multifd 必须在 incoming 开始之前设置，所以 target 要用
-incoming defer 再 migrate-incoming，不能直接 -incoming tcp:...。

### 那么 Linux kernel 中是处于什么考虑，为什么 shmem 不去自动的配置 zero page ?

mmap read fault 通常会真的分配 shmem/page-cache folio，不会像匿名映射那样直接挂全局 zero page。

原因在代码里很清楚：

- memfd_create() 非 MFD_HUGETLB 走 shmem_file_setup()，见 mm/memfd.c:465。
- shmem 的 mmap fault 走 shmem_fault()，它调用 shmem_get_folio_gfp(...,
  SGP_CACHE, ...)，见 mm/shmem.c:2768
- SGP_CACHE 的语义是 may allocate page，而不是 SGP_READ 的 “don't allocate
  page”，见 include/linux/shmem_fs.h:165
- 找不到 page cache / swap entry 时，代码直接进入 “allocate”，调用
  shmem_alloc_and_add_folio()，见 mm/shmem.c:2539

所以对于：

```txt
fd = memfd_create(...);
ftruncate(fd, 1 << 20);
p = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
x = p[0];
```

这个 p[0] 的 read fault 大概率会为 memfd 的 offset 0 分配一个零填充的 shmem
folio，并插入 page cache。

但要区分 read(2)：普通 read(fd, buf, ...) 从 shmem hole 读取时，shmem 用的是
SGP_READ，hole 返回 folio == NULL，然后直接从 ZERO_PAGE(0) 拷贝给用户，不 分配
page cache folio，见 mm/shmem.c:3388 和 mm/shmem.c:3454。

核心区别是：

- 匿名 mmap read fault：虚拟地址还没有真实后端对象，可以临时映射全局 zero page。
- shmem/memfd mmap fault：这是 file-backed mapping，fault 结果要进入这个 inode
  的 page cache 语义里，后续 MAP_SHARED 写入、truncate、swap、回收、rmap、
  多个进程共享同一 offset 都围绕这个 shmem folio 运转，所以 fault
  路径倾向于分配真实 folio。



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
