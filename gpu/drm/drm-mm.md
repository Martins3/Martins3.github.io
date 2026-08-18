# GEM vs TTM: 显存管理的两个层次

让 codex 看了下 /home/martins3/data/kernel/linux-build/Documentation/gpu/drm-mm.rst 这个解释我接受:

  GEM 不是用来“替代” TTM 的，而是另一层抽象，今天内核里两者经常是叠着用的。

  从你给的文档看，历史上确实是这样的：

  - TTM 更早，目标是做“通用图形内存管理器”，覆盖 UMA 和独显 VRAM 这类复杂场景，但代码很大、很重。Documentation/gpu/drm-mm.rst:12
  - GEM 是后来为了回应 TTM 的复杂性而出现的，设计目标更收敛，提供通用的对象模型、句柄、mmap、PRIME 等基础设施。Documentation/gpu/
    drm-mm.rst:20
  - 文档里还明确说了：GEM 本身“没有 VRAM 管理能力”。Documentation/gpu/drm-mm.rst:24

  所以如果你问“概念上”，答案是：

  - GEM 更像 DRM 的 buffer object API / 对象框架
  - TTM 更像底层的 placement / eviction / migration / VRAM-GTT-系统内存管理器

  这也是为什么当前内核里会有专门的 GEM TTM Helper 章节。Documentation/gpu/drm-mm.rst:411

  更直接的代码证据是：

  - struct ttm_buffer_object 里面直接嵌了一个 struct drm_gem_object base; include/drm/ttm/ttm_bo.h:101
  - drm_gem_ttm_helper.h 甚至直接提供了 drm_gem_ttm_of_gem()，把 drm_gem_object 转回 ttm_buffer_object。include/drm/
    drm_gem_ttm_helper.h:14

  这说明在现代 DRM 驱动里，一个很常见的模式是：

  - 对用户态暴露 GEM handle、PRIME、mmap 这套 GEM 接口
  - 底层实际内存对象用 TTM BO 来做放置、迁移、回收

  像你这棵树里就能直接看到很多驱动这么干：

  - amdgpu 用了 drm_gem_ttm_helper drivers/gpu/drm/amd/amdgpu/amdgpu_gem.c:37
  - nouveau 用了 drm_gem_ttm_helper drivers/gpu/drm/nouveau/nouveau_gem.c:27
  - xe 也用了 drm_gem_ttm_helper drivers/gpu/drm/xe/xe_bo.c:13

  所以一句话总结：

  GEM 不是 TTM 的替代品；GEM 是上层对象/用户接口框架，TTM 是下层内存管理后端。历史上它们像是两条路线，但在当前内核里，很多驱动实际
  上是 “GEM front-end + TTM back-end”。

  如果你愿意，我可以继续顺着这个问题，把：

  1. drm_gem_object
  2. ttm_buffer_object
  3. amdgpu 里一个 BO 从创建到 mmap / PRIME 的路径

## 一句话解释

- **GEM**: "这是什么显存对象？" (高层抽象 - 用户接口)
- **TTM**: "这块内存在哪？怎么迁移？" (底层实现 - 物理管理)

## Trace 数据验证

实际 Trace 结果 (TRACE_RESULTS.md):

```
分配 64MB 显存时的内核调用:

GEM 层:
  drm_gem_mmap: 13 次
  → 负责用户空间映射

TTM 层:
  ttm_resource_alloc: 592 次
  → 负责物理内存分配 (分页管理)
```

可以通过这两个 trace 文件大致理解了
```txt
@[
        ttm_resource_alloc+5
        ttm_bo_alloc_resource+266
        ttm_bo_validate+158
        ttm_bo_init_reserved+146
        xe_bo_init_locked+469
        __xe_bo_create_locked+555
        xe_bo_create_pin_map_at_aligned+82
        xe_bo_create_pin_map+32
        xe_pt_create+168
        xe_pt_stage_bind_entry+950
        xe_pt_walk_range+252
        xe_pt_walk_range+564
        xe_pt_walk_range+564
        xe_pt_stage_bind+445
        bind_op_prepare+150
        op_prepare+323
        xe_pt_update_ops_prepare+179
        ops_execute+338
        vm_bind_ioctl_ops_execute+303
        xe_vm_bind_ioctl+3031
        drm_ioctl_kernel+174
        drm_ioctl+680
        xe_drm_ioctl+79
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 254
```

```txt
@[
        drm_gem_mmap+5
        __mmap_new_vma+260
        __mmap_region+2502
        mmap_region+130
        do_mmap+1159
        vm_mmap_pgoff+291
        ksys_mmap_pgoff+354
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 13
```



