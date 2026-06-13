# fs 的 lock 设计

## Documentation/filesystems/locking.rst

靠这个解析 page_lock 了

一共有那些 lock ，表格的含义是什么?

address_space_operations

inode::i_rwsem

## Documentation/filesystems/directory-locking.rst


## 如果存在进行在一个进程在 mount 中的目录中打开文件，那么就无法 umount 掉，是如何实现的

的确是无处不在的调用哦
```c
/**
 * sb_start_write - get write access to a superblock
 * @sb: the super we write to
 *
 * When a process wants to write data or metadata to a file system (i.e. dirty
 * a page or an inode), it should embed the operation in a sb_start_write() -
 * sb_end_write() pair to get exclusion against file system freezing. This
 * function increments number of writers preventing freezing. If the file
 * system is already frozen, the function waits until the file system is
 * thawed.
 *
 * Since freeze protection behaves as a lock, users have to preserve
 * ordering of freeze protection and other filesystem locks. Generally,
 * freeze protection should be the outermost lock. In particular, we have:
 *
 * sb_start_write
 *   -> i_mutex			(write path, truncate, directory ops, ...)
 *   -> s_umount		(freeze_super, thaw_super)
 */
static inline void sb_start_write(struct super_block *sb)
{
	__sb_start_write(sb, SB_FREEZE_WRITE);
}
```

## 看见这里的注释
fs/dcache.c

## 定义这么多
```c
/*
 * inode->i_mutex nesting subclasses for the lock validator:
 *
 * 0: the object of the current VFS operation
 * 1: parent
 * 2: child/target
 * 3: xattr
 * 4: second non-directory
 * 5: second parent (when locking independent directories in rename)
 *
 * I_MUTEX_NONDIR2 is for certain operations (such as rename) which lock two
 * non-directories at once.
 *
 * The locking order between these classes is
 * parent[2] -> child -> grandchild -> normal -> xattr -> second non-directory
 */
enum inode_i_mutex_lock_class
{
	I_MUTEX_NORMAL,
	I_MUTEX_PARENT,
	I_MUTEX_CHILD,
	I_MUTEX_XATTR,
	I_MUTEX_NONDIR2,
	I_MUTEX_PARENT2,
};

static inline void inode_lock(struct inode *inode)
{
	down_write(&inode->i_rwsem);
}
```

## vfs 是允许一个 thread 在使用的时候，然后另外一个 thread 来删除
<!-- 62378106-6a6a-456a-a00a-7493214c7770 -->

场景分析

目录结构: A/B/C/
进程 P1: 在 C 中执行 readdir() 遍历
进程 P2: 想删除 A 和 B（rmdir A/B, rmdir A）
  VFS 核心锁机制

  1. 目录遍历的锁 (iterate_dir)

```txt
  // fs/readdir.c:85-116
  int iterate_dir(struct file *file, struct dir_context *ctx)
  {
      struct inode *inode = file_inode(file);

      res = down_read_killable(&inode->i_rwsem);  // 【读锁】
      if (res)
          goto out;

      res = -ENOENT;
      if (!IS_DEADDIR(inode)) {  // 检查目录是否已被删除
          ctx->pos = file->f_pos;
          res = file->f_op->iterate_shared(file, ctx);
          file->f_pos = ctx->pos;
      }
      inode_unlock_shared(inode);  // 释放读锁
      return res;
  }
  ```

  关键点：遍历目录 C 时，只持有 C 的 i_rwsem 读锁，不会持有父目录 B 或祖父目录 A 的锁。

  2. 目录删除的锁 (vfs_rmdir)

  ```
  // fs/namei.c:5218-5260
  int vfs_rmdir(struct mnt_idmap *idmap, struct inode *dir,
                struct dentry *dentry, ...)
  {
      int error = may_delete(idmap, dir, dentry, 1);  // 权限检查

      dget(dentry);
      inode_lock(dentry->d_inode);  // 【写锁 - 排他】

      error = -EBUSY;
      if (is_local_mountpoint(dentry) ||
          (dentry->d_inode->i_flags & S_KERNEL_FILE))
          goto out;

      error = dir->i_op->rmdir(dir, dentry);  // 调用具体文件系统的 rmdir
      if (error)
          goto out;

      shrink_dcache_parent(dentry);
      dentry->d_inode->i_flags |= S_DEAD;  // 标记为"死目录"
      ...
  }

  ```

  3. 目录是否为空的检查

  ```
  // fs/libfs.c:765-784
  int simple_empty(struct dentry *dentry)
  {
      struct dentry *child;

      spin_lock(&dentry->d_lock);
      hlist_for_each_entry(child, &dentry->d_children, d_sib) {
          if (simple_positive(child)) {  // 有真实子项
              spin_unlock(&dentry->d_lock);
              return 0;  // 不为空
          }
      }
      return 1;  // 为空
  }
  ```

  回答你的问题

  能否删除 B？

  不能，如果 C 还在 B 里面：

  1. rmdir B 需要检查 B 是否为空
  2. simple_empty(B) 会发现 C 仍是 B 的子项
  3. 返回 -ENOTEMPTY，删除失败

  这与 P1 是否在遍历 C 无关，纯粹是因为 C 作为 dentry 还挂在 B 的 d_children 链表上。

  能否删除 A？

  取决于具体情况：

   条件                      结果       原因
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   B 不为空（C 还在）        失败       rmdir A 时检查 simple_empty(A)，发现 B 还在
   B 为空（C 已从 B 移除）   可能成功   但需要检查 IS_DEADDIR

  关键：什么时候能删除父目录？

  场景 1：P1 只打开 C，C 还在 B 中

  P1: fd = open("A/B/C"); getdents(fd, ...)  // 持有 C 的读锁
  P2: rmdir A/B/C      // 可以！只要 C 是空的
  P2: rmdir A/B        // 可以！C 已从 B 移除
  P2: rmdir A          // 可以！B 已从 A 移除

  场景 2：删除后的影响

  // P2 删除 C 后
  P2: rmdir("A/B/C");  // 成功，C->i_flags |= S_DEAD

  // P1 继续遍历
  // iterate_dir 中的 IS_DEADDIR 检查会失败！
  if (!IS_DEADDIR(inode))  // 现在 C 已被标记 S_DEAD
      res = iterate_shared(...);
  else
      res = -ENOENT;  // P1 后续操作返回 -ENOENT

  总结

```txt
          A (可以被删除，如果 B 为空)
          │
          B (不能被删除，因为 C 还在里面)
          │
          C (P1 在这里遍历，持有 C 的读锁)
```

1. 删除 B 失败不是因为 P1 在遍历 C，而是因为 C 还作为子项存在于 B 中（simple_empty(B) == false）
2. 锁的粒度：VFS 的 i_rwsem 是 per-inode 的，遍历子目录不会锁住父目录
3. "死目录"机制：S_DEAD 标志确保已被删除但仍有 fd 打开的目录不能进行新操作
4. 引用计数：dget/dput 确保即使目录被删除，已打开的 fd 仍能正常工作（直到 close）

## vfs 允许文件被删除
<!-- 1398a9d1-a0de-4cc8-b0ea-116793cb4dbb -->

当你 unlink 一个文件后（i_nlink 变为 0），但仍有进程持有 fd 打开该文件时，inode 仍然存在于内存中，直到最后一个 fd 被关闭。这期间文件系统会：
1. 将 inode 加入 orphan list（ext4 的做法）
2. 允许继续读写，而且**数据会落盘**（不检查 i_nlink）
3. 在 fd 关闭时真正回收空间（ext4_evict_inode）

```c
  // fs/ext4/inode.c:165-267
  void ext4_evict_inode(struct inode *inode)
  {
      if (inode->i_nlink) {
          goto no_delete;  // 还有链接，不删除
      }

      // i_nlink == 0，真正删除文件
      inode->i_size = 0;
      ext4_truncate(inode);  // 回收所有块
      ext4_orphan_del(handle, inode);
      ...
  }
```

在最后一个 fd 关闭时（iput 导致 i_count 变为 0）：

1. 如果 i_nlink == 0，调用 ext4_evict_inode
2. 设置 i_size = 0
3. 调用 ext4_truncate 回收所有块（包括你刚刚写入的新数据！）
4. 从 orphan list 移除



## vfs : directory-locking.rst — 目录操作锁定机制
<!-- d1cfdcb9-cdd9-48c2-827a-d8ff519ccece -->

核心锁类型

 锁类型                 说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 ->i_rwsem              每个 inode 的读写信号量
 ->s_vfs_rename_mutex   每个文件系统的重命名互斥锁

6 类目录操作的加锁规则

 操作类型                  加锁规则
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 读访问 (lookup)           目标目录共享锁 (shared)
 创建对象 (create/mkdir)   目标目录独占锁 (exclusive)
 删除对象 (unlink/rmdir)   父目录独占 → 找到目标 → 目标独占
 创建链接 (link)           父目录独占 → 检查源非目录 → 源独占
 同目录重命名              父目录独占 → 按 inode 地址顺序锁定源/目标
 跨目录重命名              文件系统锁 → 按祖先优先顺序锁定父目录 → 验证无循环依赖 → 子目录独占 → 非目录按 inode 地址锁定

 操作类型                  具体使用的锁
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 读访问 (lookup)           inode->i_rwsem (shared)
 创建对象 (create/mkdir)   父目录 inode->i_rwsem (exclusive)
 删除对象 (unlink/rmdir)   父目录 inode->i_rwsem (exclusive) → 目标 inode->i_rwsem (exclusive)
 创建链接 (link)           父目录 inode->i_rwsem (exclusive) → 源文件 inode->i_rwsem (exclusive)
 同目录重命名              父目录 inode->i_rwsem (exclusive) → 源/目标 inode->i_rwsem (exclusive，按 inode 地址排序)
 跨目录重命名              sb->s_vfs_rename_mutex → 源/目标父目录 inode->i_rwsem (exclusive，祖先优先) → 子目录 inode->i_rwsem → 非目录按 inode 地址锁 i_rwsem


关键设计要点
• Splicing（拼接）：当 dcacache 中发现已有别名的目录需要合并时的处理，涉及 trylock 避免死锁
• 多文件系统：要求文件系统间操作遵循非对称关系（如 overlayfs < 底层文件系统）
• 死锁避免：通过严格的锁排序（非目录按 inode 地址，目录同等级，文件系统锁最低级）确保无死锁
• 循环避免：跨目录重命名前验证源和目标互不为祖先/后代

### dentry alias
<!-- 7ce255d1-3fda-4a9d-b9ff-887f9bbcf9f1 -->

(很容易理解的概念，但是 nfs 导致的目录 alias 如何理解?)

1. 数据结构关系

在 Linux 内核中，inode 和 dentry 是多对多的关系：

┌─────────────┐      i_dentry (hlist_head)      ┌──────────────────┐
│   inode     │◄────────────────────────────────│  dentry (alias 1) │──┐
│  (文件A)    │◄────────────────────────────────│  /home/file      │  │
└─────────────┘                                 └──────────────────┘  │
        ▲                                      ┌──────────────────┐  │
        │                                      │  dentry (alias 2) │──┤ d_alias
        └──────────────────────────────────────│  /tmp/link       │  │
                                               └──────────────────┘  │
                                                                     │
                                               ┌──────────────────┐  │
                                               │  dentry (alias 3) │──┘
                                               │  /mnt/foo        │
                                               └──────────────────┘

关键字段：
- inode->i_dentry：该 inode 的所有 dentry 组成的链表头
- dentry->d_u.d_alias：将 dentry 链接到 inode 链表的节点
- dentry->d_parent：指向父目录的 dentry

2. 什么是 Dentry Alias？

Dentry Alias 是指指向同一个 inode 的多个 dentry。由于文件名（路径）和 inode 是分离的，一个文件可以有多个名字（硬链接），
每个名字对应一个 dentry，但它们都指向同一个 inode。

普通文件（非目录）的 Alias

```txt
$ echo "hello" > /tmp/original
$ ln /tmp/original /tmp/hardlink1
$ ln /tmp/original /tmp/hardlink2

# 现在有 3 个 dentry alias 指向同一个 inode
# /tmp/original, /tmp/hardlink1, /tmp/hardlink2
```

目录的 Alias（特殊情况）

目录通常不允许有多个 alias（防止目录循环），但存在特殊情况：

 场景                  说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Disconnected dentry   NFS 等网络文件系统中，通过文件句柄查找到的目录 dentry，暂时未连接到目录树
 挂载点                挂载后根目录的 dentry 可能被特殊处理
 splicing 过程中       临时存在多个 alias，随后合并

3. 核心函数解析

```txt
  __d_find_any_alias() (fs/dcache.c:1007)

  static struct dentry * __d_find_any_alias(struct inode *inode)
  {
      if (hlist_empty(&inode->i_dentry))
          return NULL;
      // 返回 inode->i_dentry 链表中的第一个 dentry
      alias = hlist_entry(inode->i_dentry.first, struct dentry, d_u.d_alias);
      lockref_get(&alias->d_lockref);
      return alias;
  }

  作用：找到指向该 inode 的任意一个现有 dentry（用于 splicing）。

  d_find_alias() (fs/dcache.c:1036-1053)

  static struct dentry *__d_find_alias(struct inode *inode)
  {
      if (S_ISDIR(inode->i_mode))
          return __d_find_any_alias(inode);  // 目录：返回任意 alias

      // 非目录：遍历所有 alias，找一个已 hash 的（在 dcache 中的）
      hlist_for_each_entry(alias, &inode->i_dentry, d_u.d_alias) {
          if (!d_unhashed(alias)) {
              dget_dlock(alias);
              return alias;
          }
      }
      return NULL;
  }
```

  区别：

  • 目录：直接返回第一个 alias（因为理论上只有一个）
  • 普通文件：需要遍历找已加入 hash 表的 alias

  4. Disconnected Dentry

  Disconnected dentry 是一种特殊的 alias，它没有父目录（IS_ROOT 标志），通常出现在：

```txt
  // fs/dcache.c: d_obtain_alias() 创建 disconnected dentry
  struct dentry *d_obtain_alias(struct inode *inode)
  {
      res = d_find_any_alias(inode);  // 查找现有 alias
      if (res)
          goto out;  // 找到了，直接返回

      // 没有找到，创建一个 disconnected dentry
      new = d_alloc_anon(sb);  // 匿名分配（无父目录）
      // ...
      add_flags |= DCACHE_DISCONNECTED;  // 标记为 disconnected
      hlist_add_head(&new->d_u.d_alias, &inode->i_dentry);
  }
```

  典型场景：

- NFS：通过文件句柄（filehandle）查找 inode，但还没有路径信息
- open-by-handle：用户通过文件句柄打开文件，内核需要创建临时 dentry
- 挂载时：新的文件系统被挂载，根目录 dentry 初始时是 disconnected

5. 为什么目录不能有多 Alias？

// 内核代码中多处检查
if (S_ISDIR(inode->i_mode)) {
    // 目录只允许一个 alias，否则可能是文件系统损坏
}

原因：

1. 防止循环：如果目录 A 是目录 B 的子目录，同时目录 B 又是目录 A 的子目录，文件系统就坏了
2. 简化路径查找：保证从根目录到任意目录只有一条路径
3. .  和 .. 的语义：目录的硬链接会破坏这些特殊文件的语义

例外：NFS 等网络文件系统中，服务器上的目录移动后，客户端可能通过文件句柄找到旧位置和新位置的 alias，此时需要通过 splicing 合并。

  6. 总结

   概念           说明
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   Dentry Alias   指向同一 inode 的多个 dentry（多个文件名）
   普通文件       允许任意数量的 alias（硬链接限制除外）
   目录           原则上只允许一个 alias（防止循环）
   Disconnected   无父目录的临时 dentry（如 NFS filehandle 查找结果）
   Splicing       将 disconnected dentry 合并到目录树中的过程

理解 dentry alias 是理解 Linux 文件系统路径查找、硬链接、挂载机制的基础。

### Splicing 机制
1. 场景背景

Splicing 发生在 lookup 过程中。当查找一个目录时，发现目标 inode（特别是目
录）已经在 dcache 中存在一个别名（alias），但这个别名位于不同的父目录下（
或是一个 disconnected 的根节点）。根据文档描述的场景：

 场景                          处理方式
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 别名是单独树的根（IS_ROOT）   直接 d_move() 附加到当前目录
 别名已是当前目录的子项        改名（d_move()）
 别名是其他目录的子项          需要 trylock，失败则返回错误

  2. 核心代码流程

```txt
  入口函数：d_splice_alias() (fs/dcache.c:3137)

  struct dentry *d_splice_alias(struct inode *inode, struct dentry *dentry)
  {
      return d_splice_alias_ops(inode, dentry, NULL);
  }

  核心实现：d_splice_alias_ops() (fs/dcache.c:3063-3112)

  struct dentry *d_splice_alias_ops(struct inode *inode, struct dentry *den
  try,
                                    const struct dentry_operations *ops)
  {
      // ...
      spin_lock(&inode->i_lock);
      if (S_ISDIR(inode->i_mode)) {
          struct dentry *new = __d_find_any_alias(inode);  // 查找现有别名
          if (unlikely(new)) {
              spin_unlock(&inode->i_lock);
              write_seqlock(&rename_lock);

              if (unlikely(d_ancestor(new, dentry))) {  // 检查循环依赖
                  // 返回 -ELOOP 错误
              } else if (!IS_ROOT(new)) {  // 关键场景：别名不是根节点
                  struct dentry *old_parent = dget(new->d_parent);
                  int err = __d_unalias(dentry, new);  // <-- 调用 trylock
                  write_sequnlock(&rename_lock);
                  // ...
              } else {  // IS_ROOT: 直接移动
                  __d_move(new, dentry, false);
                  write_sequnlock(&rename_lock);
              }
              iput(inode);
              return new;
          }
      }
      __d_add(dentry, inode, ops);  // 无别名，正常添加
      return NULL;
  }
```

```txt
  关键函数：__d_unalias() (fs/dcache.c:3030-3061) — 使用 trylock 避免死锁

  static int __d_unalias(struct dentry *dentry, struct dentry *alias)
  {
      struct mutex *m1 = NULL;
      struct rw_semaphore *m2 = NULL;
      int ret = -ESTALE;

      /* 场景1：同父目录，无需额外锁 */
      if (alias->d_parent == dentry->d_parent)
          goto out_unalias;

      /* 场景2：不同父目录，需要 trylock（避免死锁！）*/
      // 尝试获取文件系统级重命名锁
      if (!mutex_trylock(&dentry->d_sb->s_vfs_rename_mutex))
          goto out_err;  // <-- trylock 失败，返回错误
      m1 = &dentry->d_sb->s_vfs_rename_mutex;

      // 尝试获取别名原父目录的共享锁
      if (!inode_trylock_shared(alias->d_parent->d_inode))
          goto out_err;  // <-- trylock 失败，返回错误
      m2 = &alias->d_parent->d_inode->i_rwsem;

  out_unalias:
      // 文件系统自定义的 unalias 锁（如 NFS 使用）
      if (alias->d_op && alias->d_op->d_unalias_trylock &&
          !alias->d_op->d_unalias_trylock(alias))
          goto out_err;

      __d_move(alias, dentry, false);  // 执行实际的 dentry 移动

      if (alias->d_op && alias->d_op->d_unalias_unlock)
          alias->d_op->d_unalias_unlock(alias);
      ret = 0;

  out_err:
      if (m2)
          up_read(m2);
      if (m1)
          mutex_unlock(m1);
      return ret;  // 成功返回 0，失败返回 -ESTALE
  }
```
3. 为什么必须使用 trylock？
	根据文档和代码，使用 trylock 的原因是：
	1. 避免反向加锁顺序导致的死锁：
	  • lookup 已经持有 当前父目录 的 i_rwsem（shared）
	  • splicing 需要获取 别名原父目录 的 i_rwsem
	  • 如果另一个线程以相反顺序加锁，就会死锁
	2. trylock 失败的处理：
	   if (!mutex_trylock(&dentry->d_sb->s_vfs_rename_mutex))
	    goto out_err;  // 返回 -ESTALE，lookup 失败
	   失败时 lookup 返回错误，而不是阻塞等待，从而避免死锁。

4. 实际例子（NFS）

NFS 在 nfs_lookup() 中使用 d_splice_alias()：

```txt
  // fs/nfs/dir.c
  struct dentry *nfs_lookup(struct inode *dir, struct dentry *dentry, unsig
  ned int flags)
  {
      // ...
      inode = nfs_fhget(dentry->d_sb, fhandle, fattr);
      res = d_splice_alias(inode, dentry);  // 可能触发 splicing
      if (res != NULL) {
          if (IS_ERR(res))
              goto out;  // 处理 -ELOOP 或 -ESTALE 错误
          dentry = res;
      }
  }
```

5. 总结图

```txt
  lookup(dir, "subdir")
      │
      ▼
  找到 inode，发现已有别名 alias（位于其他父目录）
      │
      ▼
  d_splice_alias(inode, dentry)
      │
      ├── alias 是 IS_ROOT ──► __d_move() 直接移动
      │
      └── alias 非 IS_ROOT ──► __d_unalias()
                                  │
                                  ├── 同父目录 ──► 直接移动
                                  │
                                  └── 不同父目录
                                          │
                                          ├── trylock(s_vfs_rename_mutex) ─ ─► 失败返回 -ESTALE
                                          ├── trylock(alias->parent->i_rwsem) ──► 失败返回 -ESTALE
                                          └── 成功 ──► __d_move() 移动别名
```

Splicing 的本质是在不修改底层文件系统的情况下，调整 dcache 中的目录树视图
，使得同一个目录 inode 在 dcache 中只有一个入口。使用 trylock 是为了在并发环境下保证死锁安全。

## dentry_operations
<!-- bb778f77-7a0b-4c62-b6ac-e3290560e3d8 -->

```c
static const struct dentry_operations efivarfs_d_ops = {
	.d_compare = efivarfs_d_compare,
	.d_hash = efivarfs_d_hash,
	.d_delete = always_delete_dentry,
};
```
包括看一个 fs/fat/namei_msdos.c 的实现，如果 overload 这里的函数那么相当于，
自己重新定义文件系统各种名称的对比了。

## vfs : locking.rst — VFS 方法锁定规则总览
<!-- 876f2dff-6b4f-4ee5-9635-eb8ecfd9f517 -->

主要操作结构

 结构体                     关键方法                                        锁要求
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 super_operations           put_super, sync_fs, freeze_fs 等                s_umount 读/写锁
 inode_operations           lookup, create, unlink, rename, link 等         i_rwsem 共享/独占，详见下表
 file_operations            read, write, mmap, fallocate 等                 都可阻塞，i_rwsem 保护
 address_space_operations   read_folio, write_begin/end, invalidate_folio   folio 锁 + invalidate_lock
 dentry_operations          d_revalidate, d_hash, d_compare 等              详见表格，部分支持 RCU-walk

inode_operations 锁规则

 方法                                      i_rwsem(inode)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 lookup                                    shared
 create, mknod, symlink, mkdir             exclusive
 link, unlink, rmdir                       exclusive (双方)
 rename                                    exclusive (双方父目录 + 部分子目录)
 setattr, fileattr_set                     exclusive
 readlink, get_link, permission, getattr   no

重要补充说明

• ->write_begin/end：需要 folio 锁 + i_rwsem 独占
• ->invalidate_folio：需要 invalidate_lock 独占，用于截断/打洞时与 page cache 填充操作互斥
• ->fallocate (打洞)：需获取 invalidate_lock 防止 stale page 重新加载
• ->copy_file_range/remap_file_range：需 i_rwsem + invalidate_lock 串行化
• ->page_mkwrite：需 invalidate_lock 防止与截断/remap 竞态
• quota 操作：通过 dqonoff_sem / dqptr_sem 保护

VM 操作锁 (vm_operations_struct)

 方法           mmap_lock   说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 open           write
 fault          read        可返回 page locked
 page_mkwrite   read        需处理截断竞态，通常用 invalidate_lock

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
