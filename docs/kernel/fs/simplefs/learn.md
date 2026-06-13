## fiemap
<!-- cfd413fd-d4da-4210-be97-37e2b835a305 -->

居然的确是可以分析文件系统的:
```txt
filefrag /mnt/simplefs/file    # 分析文件碎片，内部使用 FIEMAP/FIBMAP
```

## reflink
<!-- 4c1a871a-bafc-4f33-8dda-340fbf8f48d9 -->

Reflink（有时称为 "File Cloning" 或 "Range Cloning"）允许两个或多个文件共享相同的数据块（extents），而不需要实际复制数据
。它通过 remap_file_range 函数实现，对应用户空间的 ioctl 命令：

```c
#define FICLONE		_IOW(0x94, 9, int)
#define FICLONERANGE	_IOW(0x94, 13, struct file_clone_range)
#define FIDEDUPERANGE	_IOWR(0x94, 54, struct file_dedupe_range)
```
- FICLONE / FICLONERANGE：将一个文件的数据块映射到另一个文件
- FIDEDUPERANGE：数据去重（只在内容相同时映射）

只是在 xfs 中简单的拷贝 cp 命令，就可以发现有:
```txt
@[
        generic_remap_file_range_prep+5
        xfs_reflink_remap_prep+548
        xfs_file_remap_range+132
        vfs_clone_file_range+235
        ioctl_file_clone+76
        __x64_sys_ioctl+118
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 2
```

1. 快速文件复制
2. 数据去重
3. 原子文件更新: 根据 XFS 文档描述的一种模式：
	1. 通过 reflink 克隆原文件到临时文件
	2. 在临时文件上进行修改
	3. 使用原子交换操作提交更改


## iomap


## file_operations::compat_ioctl
<!-- 25cbbf60-db44-443c-b88d-435e0b9df8ca -->
```txt
  • 指针大小差异：32 位指针是 4 字节，64 位指针是 8 字节
  • 地址空间问题：32 位应用的指针在 64 位内核中需要特殊转换
  • compat_ptr(arg) 函数负责将 32 位用户空间指针转换为内核可用的 64 位指针

简单总结
┌─────────────────────────────────────────────────────────┐
│  32位应用程序 ──ioctl──→ 64位内核                       │
│                              │                          │
│                              ▼                          │
│                    发现是32位兼容调用                   │
│                              │                          │
│                              ▼                          │
│                    调用 .compat_ioctl                   │
│                              │                          │
│                              ▼                          │
│                    compat_ptr() 转换指针                │
│                              │                          │
│                              ▼                          │
│                    转发到 simplefs_ioctl()              │
└─────────────────────────────────────────────────────────┘
```

```txt
#ifdef CONFIG_COMPAT
	.compat_ioctl = simplefs_compat_ioctl,
#endif
```

## simplefs 的文件 io
<!-- 1a9ba876-c1d3-41e8-a040-3d446dbd6d19 -->

1. 第一部分就是注册和磁盘 io 相关的，这部分交给 iomap 完成就可以了:
```c
static const struct iomap_ops simplefs_read_iomap_ops = {
	.iomap_begin = simplefs_read_iomap_begin,
};

static const struct iomap_writeback_ops simplefs_writeback_ops = {
	.writeback_range	= simplefs_writeback_range,
	.writeback_submit	= iomap_ioend_writeback_submit,
};

const struct address_space_operations simplefs_iomap_aops = {
	.read_folio		= simplefs_read_folio,
	.readahead		= simplefs_readahead,
	.writepages		= simplefs_writepages,
	.dirty_folio		= iomap_dirty_folio,
	.release_folio		= iomap_release_folio,
	.invalidate_folio	= iomap_invalidate_folio,
	.migrate_folio		= filemap_migrate_folio,
	.is_partially_uptodate	= iomap_is_partially_uptodate,
	.error_remove_folio	= generic_error_remove_folio,
	.bmap			= simplefs_bmap,
};
```

2. 文件 io 相关，的
```c
const struct file_operations simple_fs_iomap_fops = {
	.open = simplefs_file_open,
	.llseek = simplefs_llseek, // iomap
	.owner = THIS_MODULE,
	.read_iter = simplefs_file_read_iter, // iomap ，主要在区分 o_direct
	.write_iter = simplefs_file_write_iter, // iomap ，主要在区分 o_direct
	.mmap = simplefs_file_mmap,
	.get_unmapped_area = thp_get_unmapped_area,
	.splice_read = filemap_splice_read,
	.splice_write = iter_file_splice_write,
	.fsync = simplefs_file_fsync,
	.fallocate = simplefs_fallocate, // 这个其实是和 inode 相关了
	/* NOTE: remap_file_range (reflink) removed - simplefs lacks CoW support,
	 * so sharing physical blocks corrupts data on write. Without this,
	 * copy_file_range correctly falls back to splice.
	 */
	.unlocked_ioctl = simplefs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = simplefs_compat_ioctl,
#endif
};
```

## 创建文件之后，什么时候会落盘？
<!-- a59c6646-48b2-4c4a-8fcd-567faa7dd5c5 -->

一般来说， 当 mkdir 的时候，调用回调，简单参考 ext4_write_inode ，可以发现也就是:
```txt
@[
        simplefs_write_inode+5
        __writeback_single_inode+1085
        writeback_single_inode+208
        write_inode_now+157
        simplefs_create_internal.isra.0+1044
        simplefs_mkdir+76
        vfs_mkdir+489
        do_mkdirat+143
        __x64_sys_mkdir+44
        do_syscall_64+117
        entry_SYSCALL_64_after_hwframe+118
]: 3
```

## [ ] journal 机制如何保证?
<!-- 7804571e-1173-47b5-9814-ee449c9cc9d6 -->

## [ ] 访问磁盘的方法真的合理吗?

这两个配合的方法
simplefs_get_folio

folio_mark_dirty(folio);
folio_release_kmap(folio, disk_inode - inode_shift);

例如，我们发现 simplefs_sync_fs 就非常的诡异!

或者说，xfs 是如何读磁盘的。

## dir 的 file_operations 是用于如何遍历
```c
const struct file_operations simplefs_dir_ops = {
	.owner = THIS_MODULE,
	.iterate_shared = simplefs_iterate,
	.fsync = simplefs_dir_fsync,
	.unlocked_ioctl = simplefs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = simplefs_compat_ioctl,
#endif
};
```

simplefs_write_inode 中持有的锁是什么效果:

```txt
[ 3670.777815] 2 locks held by mkdir/10902:
[ 3670.778031]  #0: ffff88811af2b420 (sb_writers#14){.+.+}-{0:0}, at: filename_create+0x7a/0x160
[ 3670.778571]  #1: ffff888153940178 (&type->i_mutex_dir_key#8/1){+.+.}-{4:4}, at: filename_create+0xb9/0x160
```
似乎和 Documentation/filesystems/locking.rst 中说的不一样的
基本一样吧，细节再分析

## .read_folio 和 .readahead 的关系是什么?
<!-- 8b7c4039-b007-4b32-a872-89836cca019c -->

```c
/* Pure iomap read - using iomap_bio helpers */
static int simplefs_read_folio(struct file *unused, struct folio *folio)
{
	struct inode *inode = folio->mapping->host;

	trace_simplefs_read_page(inode, folio->index, folio_size(folio), false);
	sfs_stat_inc(read_count);
	sfs_stat_add(read_bytes, folio_size(folio));

	debug_show_held_locks(current);
	iomap_bio_read_folio(folio, &simplefs_read_iomap_ops);
	return 0;
}

static void simplefs_readahead(struct readahead_control *rac)
{
	sfs_stat_inc(read_count);
	debug_show_held_locks(current);
	iomap_bio_readahead(rac, &simplefs_read_iomap_ops);
}
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
   特性       read_folio             readahead
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   读取方式   同步读取               异步预读取
   数据量     单个 folio             多个连续 folio
   调用时机   缓存未命中，需要等待   检测到顺序访问模式
   性能目标   正确性                 吞吐量优化
   失败处理   返回错误码             可跳过部分 folio
```

simplefs_readahead 中的确观察到了:
```txt
[ 5192.309723] 1 lock held by cat/18200:
[ 5192.310649]  #0: ffff888105f69690 (mapping.invalidate_lock#4){.+.+}-{4:4}, at: page_cache_ra_unbounded+0x84/0x250
```
和 Documentation/filesystems/locking.rst 总体来说，基本上可以对上的。

## extent 的方式
<!-- abf589b2-7eb4-412b-893c-fa9fe157e67f -->

1. 核心 extent 操作
 函数                           文件                作用
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 simplefs_ext_search()          simplefs_extent.c   根据逻辑块号查找对应的 extent
 simplefs_get_block()           simplefs_file.c     分配/查找 extent，创建新的 extent
 simplefs_write_iomap_begin()   simplefs_file.c     写操作时映射 extent
 simplefs_read_iomap_begin()    simplefs_file.c     读操作时查找 extent

2. 修改 extent 结构的函数

// 分配/释放块（修改 extent）
simplefs_fallocate_prealloc() // 预分配 extent
simplefs_fallocate_punch_hole() // 打孔：分割/删除 extent
simplefs_fallocate_collapse()   // 折叠：删除中间 extent，移动后续 extent
simplefs_truncate()           // 截断：删除尾部 extent

3. 查询 extent 结构的函数

// 返回 extent 信息给用户空间（fiemap ioctl）
simplefs_fiemap()             // 使用 iomap_fiemap 返回 extent 列表

// 调试/查看
simplefs_get_folio()          // 读取 ei_block 获取 extent 索引

4. 创建/删除时的 extent 操作

// 创建文件
simplefs_create_internal()    // 分配 ei_block，初始化 extent 数组
simplefs_new_inode()          // 分配第一个块作为 ei_block

// 删除文件
simplefs_evict_inode()        // 遍历 extent 释放所有数据块
simplefs_unlink_internal()    // 修改父目录的 extent（删除目录项）

5. 关键数据结构

```c
// simplefs.h:123-127
struct simplefs_extent {
    uint32_t ee_block;  // 逻辑起始块
    uint32_t ee_len;    // 连续块数（最大 256）
    uint32_t ee_start;  // 物理起始块
};

// simplefs.h:129-132
struct simplefs_file_ei_block {
    uint32_t nr_files;                    // 目录用：文件数
    struct simplefs_extent extents[341];  // extent 数组（最大 341 个）
};
```


## simplefs_create_internal / simplefs_unlink_internal : 目录操作
<!-- adcaf0f4-e08b-4e77-8984-3b1922b6d40a -->

inode_operations::create 和 inode_operations::mkdir 都是调用到 simplefs_create_internal 中的
simplefs_rmdir 和 simplefs_rename_internal 都是调用 simplefs_unlink_internal

简单来说:
1. inode = simplefs_new_inode(dir, mode); 在 inode bitmap 中找一个，然后在 inode store 中获取一个
2. 遍历这个 inode 文件的所有位置，尝试找到一个块来做
```c
  // 扫描所有 extent、所有块、所有槽位，找第一个空闲位置
  for (ei = 0; ei < SIMPLEFS_MAX_EXTENTS && !slot_found; ei++) {
      for (bi = 0; bi < extent_len && !slot_found; bi++) {
          for (fi = 0; fi < SIMPLEFS_FILES_PER_BLOCK; fi++) {
              if (dblock->files[fi].inode == 0) {
                  slot_found = 1;
                  break;
              }
          }
      }
  }
```

## simplefs_rename_internal
这似乎就是导致问题失控的关键了

```txt
ext2_rename 的核心流程

ext2_rename(old_dir, old_dentry, new_dir, new_dentry, flags)
├── 1. 参数检查 (flags & ~RENAME_NOREPLACE)
├── 2. 配额初始化 (dquot_initialize)
├── 3. 查找旧目录项 (ext2_find_entry → old_de)
├── 4. 【关键】如果是目录移动且跨目录，获取 ".." 项 (ext2_dotdot → dir_de)
├── 5. 处理目标已存在的情况 (new_inode):
│   ├── 检查目标目录是否为空 (ext2_empty_dir)
│   ├── 查找目标目录项 (ext2_find_entry → new_de)
│   ├── 【原子操作】直接修改目标目录项指向旧 inode (ext2_set_link)
│   ├── 更新 new_inode 的 ctime
│   ├── 如果是目录覆盖，减少 new_inode 的 nlink (drop_nlink)
│   └── 减少 new_inode 的链接计数 (inode_dec_link_count)
├── 6. 目标不存在:
│   ├── 添加新链接 (ext2_add_link)
│   └── 如果是目录，增加新目录的 nlink (inode_inc_link_count)
├── 7. 更新旧 inode 的 ctime 并标记脏
├── 8. 删除旧目录项 (ext2_delete_entry)
├── 9. 【关键】如果是目录移动:
│   ├── 如果跨目录，更新 ".." 指向新父目录 (ext2_set_link)
│   └── 减少旧目录的 nlink (inode_dec_link_count)
└── 10. 清理资源

ext2 设计的关键特点

 特性           实现方式                                        目的
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 原子性覆盖     ext2_set_link 直接修改目标目录项的 inode 指针   避免先删除后添加的中间状态
 ".." 更新      通过 ext2_dotdot 获取并更新                     目录移动时保证父目录引用正确
 链接计数管理   精确控制 nlink 增减时机                         保证文件系统一致性

───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
simplefs_inode.c rename 实现的问题分析

经过对比分析，发现 simplefs_rename_internal 存在严重缺陷：

问题 1：目录移动时未更新 ".." 指针【严重】

ext2 的正确做法：

if (old_is_dir && old_dir != new_dir) {
    dir_de = ext2_dotdot(old_inode, &dir_folio);  // 获取 ".."
    ...
    err = ext2_set_link(old_inode, dir_de, dir_folio, new_dir, false);  // 更新 ".."
}

simplefs 的缺陷：

// simplefs 根本没有获取和更新 ".." 的逻辑！
// 当目录跨目录移动时，目录内的 ".." 仍然指向旧父目录

后果： 目录移动后，cd moved_dir/.. 会回到旧目录而不是新目录，文件系统层次结构断裂。

问题 2：覆盖场景下非原子性操作【严重】

ext2 的原子覆盖：

// 目标存在时，直接修改目录项，不解不建
err = ext2_set_link(new_dir, new_de, new_folio, old_inode, true);

simplefs 的两步操作：

// 先删除目标（可能失败）
ret = simplefs_unlink_internal(new_dir, new_dentry, handle);
...
// 再添加新条目（可能失败，导致数据丢失）
// 添加新目录项的代码...

后果： 如果在删除目标和添加新条目之间发生崩溃或错误，文件系统处于不一致状态。

问题 3：覆盖时未更新被覆盖 inode 的 ctime

ext2：

inode_set_ctime_current(new_inode);  // 更新被覆盖 inode 的 ctime

simplefs： 缺少这一操作，违反 POSIX 标准。

问题 4：目录移动时 ".." 链接计数未正确处理

虽然 simplefs 处理了父目录的 nlink：

if (S_ISDIR(old_inode->i_mode) && old_dir != new_dir) {
    drop_nlink(old_dir);  // 旧父目录减少 ".."
    inc_nlink(new_dir);   // 新父目录增加 ".."
}

但缺少最关键的步骤：更新被移动目录内部的 ".." 条目指向新父目录！

───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
总结

 问题               严重程度   描述
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 未更新 ".." 指针   致命       目录移动后父目录引用错误
 非原子性覆盖       高         可能导致数据丢失
 未更新 ctime       中         POSIX 不符合
 代码冗余           低         重复查找旧目录项

修复建议： 参考 ext2 的实现，添加 simplefs_dotdot 函数获取 ".." 条目，并在目录跨目录移动时调用类似 simplefs_set_link 的函数更新 "..
" 指针。
```

## [ ] simplefs_mark_folio_dirty 的合理性有待分析

## [ ] 现在指出 DAX (Direct Access) 了吗?
不

## [ ] 我感觉这个东西还是有问题啊

1. 为什么 page_mkwrite 需要单独定义?
2. filemap_invalidate_lock_shared 似乎出现过问题

```c
/* Pure iomap mmap support - page_mkwrite for handling mmap writes */
static vm_fault_t simplefs_page_mkwrite(struct vm_fault *vmf)
{
	struct inode *inode = file_inode(vmf->vma->vm_file);
	vm_fault_t ret;

	if (simplefs_is_shutdown(inode->i_sb))
		return VM_FAULT_SIGBUS;

	sb_start_pagefault(inode->i_sb);
	file_update_time(vmf->vma->vm_file);

	filemap_invalidate_lock_shared(inode->i_mapping);
	ret = iomap_page_mkwrite(vmf, &simplefs_write_iomap_ops, NULL);
	filemap_invalidate_unlock_shared(inode->i_mapping);

	sb_end_pagefault(inode->i_sb);
	return ret;
}

static const struct vm_operations_struct simplefs_file_vm_ops = {
	.fault		= filemap_fault,
	.map_pages	= filemap_map_pages,
	.page_mkwrite	= simplefs_page_mkwrite,
};
```

## 测试一下磁盘的容量是什么回事?
<!-- 1f8bcc5d-6eb5-4b45-b552-3b43c72f33b4 -->

连 vn 仓库都没法拷贝到目录下，这是不是有大问题?

## [ ] 还是先看下，那些测试是可以通过的，然后就继续看那些通过的测试
<!-- 66c3ca48-bb55-49ef-9dbf-43e60a7296f1 -->

## 理解一下 fallocate 的基本语义
<!-- be78a10d-5341-4621-b614-7ea99d54295f -->

fallocate/collapse/punch/fiemap

`KEEP_SIZE` 指的是 `fallocate(2)` 的一个标志：`FALLOC_FL_KEEP_SIZE`。

它的语义很简单：

- 给文件“预分配磁盘空间”
- 但不改变文件当前的 `i_size`
- 所以用户看到的文件长度不变
- 只是 EOF 之后那一段已经先占好块了

例子：

```bash
fallocate -k -o 0 -l 10M file
```

这里的 `-k` 就是 keep size。

如果 `file` 原来大小是 0，那么执行后：

- `stat file` 看到的大小还是 0
- 但底层已经为 10M 范围分配了块
- 这段在 `fiemap` 里通常应该显示成 `unwritten`
- 等后面真正 `write` 到其中一部分，那部分才变成 `data`

这也是 `generic/092` 在测的东西。它在验证两件事：

1. 先 `falloc -k 0 10M`，再写前 5M，再 `truncate 5M`
   期望：
   - 前 5M 是 `data`
   - 5M 之后那段预分配空间应该被回收，不该还留着

2. 再 `falloc -k 5M 5M`，然后 `truncate 7M`
   期望：
   - 前 5M 是 `data`
   - 5M..10M 里已经预分配但没写的部分，仍然以 `unwritten` 形式保留

这里同时混了 3 个概念：

- `i_size`
  用户看到的文件长度
- 已分配空间
  文件系统是否真的占了块
- 已写入数据
  这些块里哪些已经是有效数据，哪些只是预留

这次修复的核心是把这三件事分开，不再混在一起：

- 在 `simplefs_file.c` 里单独记录 `KEEP_SIZE` 预分配区间
- 真正写入后，把那部分从“预分配未写”变成“data”
- 在 `simplefs_inode.c` 里，`truncate` 即使大小没变，也要清理 EOF 后面的预分配尾巴
- 在 `simplefs_file.c` 的 `fiemap` 输出里，把还没写过的预分配部分标成 `unwritten`

所以结果上“错误变少了”，本质原因是：
之前 SimpleFS 把“预分配但未写”和“真正数据”混成一类了；现在这层语义被拆开了，xfstests 就不再把它判错。

## 的确，那么当调用到需要获取 free inode 的时候，当时持有了什么锁没有
```txt
static inline uint32_t get_free_inode(struct simplefs_sb_info *sbi)
```

## [ ] filemap_write_and_wait_range
什么时候应该调用这个函数？

一般来说，不是配置 dirty ，然后让 page 慢慢的 write back 就可以了吗?

## [ ] 所以，按道理，可以用这个文件系统来测试 folio 问题

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
