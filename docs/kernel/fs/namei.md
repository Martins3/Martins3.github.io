## 关键源码位置

fs/namei.c : 文件系统的路径解析，提供 hardlink, symbol link 之类的操作
fs/d_path.c
  - 提供辅助函数 : d_path
  - 提供了 getpwd 的系统调用
- fs/dcache.c
- fs/inode.c (inode 的各种管理，evict 等操作)
- ext2/inode.c (这里居然放置的内容是 address_space_operation 的系统)
   ext2/namei.c (查询的支持，和查询到之后的操作)

## Documentation

- [Pathname lookup in Linux](https://lwn.net/Articles/649115)
- [A walk among the symlinks](https://lwn.net/Articles/650786)

## 基本流程

```c
int kern_path(const char *name, unsigned int flags, struct path *path)
{
  return filename_lookup(AT_FDCWD, getname_kernel(name),
             flags, path, NULL);
}
```

注意，这里是通过 name ，获取到 struct path

- kern_path
  - filename_lookup : 装配 nameidata
    - path_lookupat
      - link_path_walk
        - walk_component
          - handle_dots
          - lookup_fast
          - lookup_slow : 相对于 __lookup_slow ，持有
            - __lookup_slow
              - d_alloc_parallel
                - d_alloc : 最基本的分配和初始化而已，复杂的东西在
                  d_alloc_parallel
              - simplefs_lookup : 读取磁盘文件，比对，然后获取到 inode number
                - simplefs_iget
                  - iget_locked : 首先尝试从 inode cache 中获取
          - step_into : 处理 symbolic

path_lookupat 中:

```c
while (!(err = link_path_walk(s, nd)) &&
       (s = lookup_last(nd)) != NULL)
	;
```

原来 openat 也是一个路径:

```txt
@[
        vfs_open+5
        path_openat+2820
        do_filp_open+215
        do_sys_openat2+138
        __x64_sys_openat+84
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 28
```

在 path_openat 中:

```c
while (!(error = link_path_walk(s, nd)) &&
       (s = open_last_lookups(nd, file, op)) != NULL)
	;
if (!error)
	error = do_open(nd, file, op); // 调用 vfs_open
```

## 关键结构体

### struct path

```c
struct path {
	struct vfsmount *mnt;
	struct dentry *dentry;
} __randomize_layout;
```

非常合理，如果想要知道路径，那么需要知道路径了

### struct nameidata

```c
struct nameidata {
  struct path path;
  struct qstr last;
  struct path root;
  struct inode  *inode; /* path.dentry.d_inode */
  unsigned int  flags;
  unsigned  seq, m_seq, r_seq;
  int   last_type;
  unsigned  depth;
  int   total_link_count;
  struct saved {
    struct path link;
    struct delayed_call done;
    const char *name;
    unsigned seq;
  } *stack, internal[EMBEDDED_LEVELS];
  struct filename *name;
  struct nameidata *saved;
  unsigned  root_seq;
  int   dfd;
  kuid_t    dir_uid;
  umode_t   dir_mode;
} __randomize_layout;
```

### struct dentry

三个关键内容:
1. a component name,
2. a pointer to a parent dentry,
3. and a pointer to the “inode” which contains further information about the
   object in that parent with the given name.

测试 /sys/kernel/debug/block/vdb/state 这个文件 然后 blk_mq_debugfs_show
中打点，利用 gdb 可以看到:

```txt
$ p *m->file->f_path.dentry
$4 = {
  d_flags = 4194312,
  d_seq = {
    seqcount = {
      sequence = 2
    }
  },
  d_hash = {
    next = 0x0 <fixed_percpu_data>,
    pprev = 0xffffc90000154970
  },
  d_parent = 0xffff88800581d780,
  d_name = {
    {
      {
        hash = 2664144965,
        len = 5
      },
      hash_len = 24138981445
    },
    name = 0xffff888004bdecf8 "state"
  },
  d_inode = 0xffff88800b78b288,
  d_iname = "state\000-switch-root.service", '\000' <repeats 13 times>,
  d_op = 0xffffffff8246a100 <debugfs_dops>,
  d_sb = 0xffff8880045d6800,
  d_time = 0,
```

然后继续展示他的 parent ，可以看到:

```txt
$ p *(struct dentry *)0xffff88800581d780
$6 = {
  d_flags = 2097160,
  d_seq = {
    seqcount = {
      sequence = 2
    }
  },
  d_hash = {
    next = 0x0 <fixed_percpu_data>,
    pprev = 0xffffc9000013ae18
  },
  d_parent = 0xffff88804081c000,
  d_name = {
    {
      {
        hash = 2448476182,
        len = 3
      },
      hash_len = 15333378070
    },
    name = 0xffff88800581d7b8 "vdb"
  },
  d_inode = 0xffff88800b78a8e8,
  d_iname = "vdb\000MSIX-0000:00:01.0\000p.gz", '\000' <repeats 13 times>,
  d_op = 0xffffffff8246a100 <debugfs_dops>,
  d_sb = 0xffff8880045d6800,
  d_time = 0,
```

可以看到，dentry 就是通过描述自己的 parent 是什么来构建整个路线的.

## 路径查询

### [ ] 核心 : link_path_walk

link_path_walk 调用 walk_component walk_component 负责处理单个组件

### 核心 : walk_component

```c
static int walk_component(struct nameidata *nd, int flags)
{
	err = lookup_fast(nd, &path, &inode, &seq); // todo this dcache, what
	path.dentry = lookup_slow(&nd->last, nd->path.dentry, // path.dentry is parent !
	err = follow_managed(&path, nd); // todo ?
	return step_into(nd, &path, flags, inode, seq); // todo
}

/* Fast lookup failed, do it the slow way */
static struct dentry *__lookup_slow(const struct qstr *name,
				    struct dentry *dir,
				    unsigned int flags)
{
	dentry = d_alloc_parallel(dir, name, &wq); // create and link : struct dentry *new = d_alloc(parent, name); and then insert into the file name
  // we will not create dentry for every item in the directory ! only one
	old = inode->i_op->lookup(inode, dentry, flags);
}
```

## link

一共这些 ops 和 link 有关

|----------|-----------------------|
| get_link | 查询 |
| readlink | 似乎专门给 /proc 用的 |
| link | 创建硬链接 |
| unlink | 删除 |
| symlink | 创建软链接 |

```txt
@[
        ext4_get_link+5
        step_into+1392
        path_openat+348
        do_filp_open+215
        do_open_execat+91
        alloc_bprm+36
        do_execveat_common+147
        __x64_sys_execve+52
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 90
```

在 struct dentry::d_alias

`d_alias` links the dentry objects of identical files. This situation arises
when links are used to make the file available under two different names. This
list is linked from the corresponding inode by using its `i_dentry` element as a
list head. The individual dentry objects are linked by `d_alias`

- d_splice_alias
  1. 如果不考虑 alias 问题，等价于 `__d_add`
  2. 对于目录 VFS 原则上不允许存在多个别名（即硬链接）。

> [!NOTE]
> 参考 Deepseeek ，有待验证

> 但有一个重要的例外：
> 当一个目录作为另一个文件系统的挂载点根目录时（例如，将一个U盘挂载到 /mnt/usb），
> 此时代表 U盘根目录的 inode 就会同时拥有它自己文件系统内的根 dentry (/) 和宿主系统中的挂载点 dentry (/mnt/usb) 两个别名
>
> NFS 场景举例:
> 服务器导出了一个目录，比如 /export/data。
>
> 对于 NFS 服务器来说，/export/data 这个目录的 inode 存在一个 dentry，并且这个 dentry 被标记为 IS_ROOT（因为它是导出文件系统的根）。
>
> 当一个 NFS 客户端请求访问 /export/data/file.txt 时，服务器的 VFS 会尝试在 dcache 中构建这个路径。
>
> 当查找到 data 这个名字时，VFS 准备创建一个新的 dentry，并发现它指向的 inode 已经有了一个 IS_ROOT 的别名 dentry (new)

对于 nfs 的场景，我勉强可以接受，另外的场景，我不太可以接受


## inode 基本维护

1. new_inode(): creates a new inode, sets the i_nlink field to 1 and initializes
   i_blkbits, i_sb and i_dev; 和 super_operations::alloc_inode 的关系是什么 ?
   new_inode 会调用 alloc_inode ，进而调用其。

```txt
@[
        new_inode+5
        __ext4_new_inode+234
        ext4_create+227
        path_openat+2357
        do_filp_open+215
        do_sys_openat2+138
        __x64_sys_openat+84
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 5
```

```txt
@[
        iget_locked+5
        __ext4_iget+310
        ext4_lookup+258
        __lookup_slow+133
        walk_component+219
        path_lookupat+103
        filename_lookup+241
        user_path_at+55
        do_faccessat+255
        __x64_sys_access+28
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 2
```

5. unlock_new_inode(): used in conjunction with iget_locked(), releases the lock
   on the inode;

6. iput(): tells the kernel that the work on the inode is finished; if no one
   else uses it, it will be destroyed (after being written on the disk if it is
   maked as dirty);

```txt
@[
        iput+5
        __dentry_kill+113
        dput+235
        __fput+302
        __x64_sys_close+61
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 1377
```

### inode 如何关联上 struct file 的

简而言之，当然，首先需要进行路径解析，获取到 dentry ，
然后在 do_dentry_open 中打开:
```txt
@[
        do_dentry_open+1
        vfs_open+46
        path_openat+2820
        do_filp_open+215
        do_sys_openat2+138
        __x64_sys_openat+84
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 7370
```

### inode 如何关联上 下层设备的 ?

是通过 superblock 关联的，应该是在这里初始化的
set_bdev_super

### inode 落盘

inode 从磁盘上读取一般通过
```c
static const struct inode_operations simplefs_inode_ops = {
	.lookup = simplefs_lookup,
	.create = simplefs_create,
	.mkdir = simplefs_mkdir,
};
```

那么 dirty 的 inode 是通过 struct super_operations 来落盘的:
```txt
@[
        __mark_inode_dirty+5
        block_commit_write+77
        block_write_end+59
        ext4_da_write_end+137
        generic_perform_write+276
        ext4_buffered_write_iter+104
        vfs_write+663
        __x64_sys_pwrite64+157
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 202
```

```txt
@[
        inode_dio_wait+5
        ext4_setattr+1276
        notify_change+881
        do_truncate+148
        path_openat+2903
        do_filp_open+215
        do_sys_openat2+138
        __x64_sys_openat+84
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 178
```

```txt
@[
        evict+1
        __dentry_kill+113
        shrink_dentry_list+162
        shrink_dcache_parent+215
        d_invalidate+104
        proc_invalidate_siblings_dcache+317
        release_task+847
        wait_consider_task+1276
        __do_wait+162
        do_wait+106
        kernel_wait4+182
        __do_sys_wait4+71
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 25
```

```txt
@[
        ext4_write_inode+5
        __writeback_single_inode+655
        writeback_sb_inodes+539
        __writeback_inodes_wb+76
        wb_writeback+427
        wb_workfn+822
        process_one_work+346
        worker_thread+826
        kthread+251
        ret_from_fork+49
        ret_from_fork_asm+26
]: 4942
```


## dcache sysfs

### slabtop

🤒 sudo slabtop --once | grep dentry 239316 239223 99% 0.19K 5698 42 45584K
dentry

### /proc/sys/fs/dentry-state

- proc_nr_dentry 中实现的

```c
struct dentry_stat_t {
	long nr_dentry;
	long nr_unused;
	long age_limit;		/* age in seconds */
	long want_pages;	/* pages requested by system */
	long nr_negative;	/* # of unused negative dentries */
	long dummy;		/* Reserved for future use */
};
```

其中每一个项目的结果是：d_lru_add

- retain_dentry
  - d_lru_add 的时候才去判断

```txt
🧀  cat /proc/sys/fs/dentry-state
766073  706656  45      0       271345  0
```

执行 `echo 3 | sudo tee /proc/sys/vm/drop_caches` 之后

```txt
🧀  cat /proc/sys/fs/dentry-state
40562   3267    45      0       778     0
```

类似的有

```txt
cat /proc/sys/fs/inode-state
41713   437     0       0       0       0       0
```

- 执行一次 Disk usage analyze 真的就会产生好几个 G 的 dcache / icache 吗
  - 在 /home/martins3/ 中执行一次 ncdu ，dentry 增加 1 百万左右，考虑一个 dentry
    100 多 byte ，所以差不多增加几百兆吧

## dentry 的基本维护

分析一个 cache 基本方法:

1. 加入
2. 删除
3. 查询

There are a number of functions defined which permit a filesystem to manipulate
dentries:

- dget : open a new handle for an existing dentry (this just increments the
  usage count)
- dput (引用计数) : close a handle for a dentry (decrements the usage count). If
  the usage count drops to 0, and the dentry is still in its parent’s hash, the
  “d_delete” method is called to check whether it should be cached. If it should
  not be cached, or if the dentry is not hashed, it is deleted. Otherwise cached
  dentries are put into an LRU list to be reclaimed on memory shortage.
- d_drop : **this unhashes a dentry from its parents hash list.** A subsequent
  call to dput() will deallocate the dentry if its usage count drops to 0
- d_delete (文件删除) : delete a dentry. If there are no other open references
  to the dentry then the dentry is turned into a negative dentry (the d_iput()
  method is called). **If there are other references, then d_drop() is called
  instead**
- d_add (构建新的 dentry ，不是新建文件) : add a dentry to its parents hash list
  and then calls d_instantiate()
- d_instantiate : add a dentry to the alias hash list for the inode and updates
  the “d_inode” member. The “i_count” member in the inode structure should be
  set/incremented. If the inode pointer is NULL, the dentry is called a
  “negative dentry”. This function is commonly called when an inode is created
  for an existing negative dentry
- d_lookup : look up a dentry given its parent and path name component It looks
  up the child of that given name from the dcache hash table. If it is found,
  the reference count is incremented and the dentry is returned. The caller must
  use dput() to free the dentry when it finishes using it.
- d_move : 实现 rename

```txt
@[
    d_delete+5
    vfs_unlink+539
    do_unlinkat+657
    __x64_sys_unlinkat+53
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 100000
```

```txt
@[
        d_lru_add+1
        dput+404
        path_put+22
        vfs_statx+218
        vfs_fstatat+107
        __do_sys_newfstatat+59
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 5661
```

```txt
@[
    dput+5
    __fput+299
    task_work_run+89
    do_exit+753
    do_group_exit+48
    __x64_sys_exit_group+24
    x64_sys_call+6131
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 17437
```

```c
static inline struct dentry *dget(struct dentry *dentry)
{
	if (dentry)
		lockref_get(&dentry->d_lockref);
	return dentry;
}
```

```txt
@[
    dput+5
    terminate_walk+88
    path_lookupat+150
    filename_lookup+220
    vfs_statx+143
    vfs_fstatat+123
    __do_sys_newfstatat+63
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 11015
```

```c
extern void d_instantiate(struct dentry *, struct inode *);
struct dentry * d_alloc_anon(struct inode *);
struct dentry * d_splice_alias(struct inode *, struct dentry *);
static inline void d_add(struct dentry *entry, struct inode *inode);
void dput(struct dentry *dentry);
static inline struct dentry *dget(struct dentry *dentry)
struct dentry * d_lookup(struct dentry *, struct qstr *);
static struct dentry *__dentry_kill(struct dentry *dentry);
void d_drop(struct dentry *dentry){
void d_delete(struct dentry * dentry)
```

1. `d_make_root`: allocates the root dentry. It is generally used in the
   function that is called to read the superblock (fill_super), which must
   initialize the root directory. So the root inode is obtained from the
   superblock and is used as an argument to this function, to fill the s_root
   field from the struct super_block structure.
2. `d_add`: associates a dentry with an inode; the dentry received as a
   parameter in the calls discussed above signifies the entry (name, length)
   that needs to be created. This function will be used when creating/loading a
   new inode that does not have a dentry associated with it and has not yet been
   introduced to the hash table of inodes (at lookup); 将 新创建的 inode 和 其
   dentry 关联起来。
3. `d_instantiate`: The lighter version of the previous call, in which the
   dentry was previously added in the hash table.

dentry_kill 是做啥的

1. d_delete : 删除文件
2. d_put -> __dentry_kill

d_instantiate 和 d_add 都是才是将 dentry 和 inode 关联的函数， 一个
d_instantiate 只有关联了 inode 才会有

### d_add
1. d_add 调用位置太少了，比较通用的调用位置在 libfs 中间，
2. 其核心执行的内容如下，也就是存在两个 hash，全局的，每个

   hlist_add_head(&dentry->d_u.d_alias, &inode->i_dentry);
3. 实际上，依托 hlist_bl_add_head_rcu d_hash 加入 dentry_hashtable

## dcache

将 parent 指针和 name 来共同实现 hash :

```c
struct dentry *d_alloc_name(struct dentry *parent, const char *name)
{
	struct qstr q;

	q.name = name;
	q.hash_len = hashlen_string(parent, name);
	return d_alloc(parent, &q);
}
```

为什么不是一个 mount point 一个 hash table ?
我猜测是，dentry 的量没有那么大，会刷掉很多

```c
static struct hlist_bl_head *dentry_hashtable __read_mostly;

static inline struct hlist_bl_head *d_hash(unsigned int hash)
{
  return dentry_hashtable + (hash >> d_hash_shift);
}
```

```txt
@[
    simplefs_lookup+5
    __lookup_slow+131
    walk_component+219
    path_lookupat+106
    filename_lookup+220
    vfs_statx+143
    do_statx+102
    __x64_sys_statx+154
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 2
```



## lru

1. alloc_super
   1. super_cache_scan
   2. super_cache_count

2. `struct superpage` 中间存在两个函数 TODO 应该用于特殊内容的缓存的

```c
long (*nr_cached_objects)(struct super_block *,
        struct shrink_control *);
long (*free_cached_objects)(struct super_block *,
          struct shrink_control *);
```

3. super_cache_scan 调用两个函数
   1. prune_dcache_sb
   2. prune_icache_sb

```c
long prune_dcache_sb(struct super_block *sb, struct shrink_control *sc)
{
  LIST_HEAD(dispose);
  long freed;

  freed = list_lru_shrink_walk(&sb->s_dentry_lru, sc,
             dentry_lru_isolate, &dispose);
  shrink_dentry_list(&dispose);
  return freed;
}
```

prune_icache_sb : TODO 这个是用于释放 inode 还是 inode 持有的文件 ? 还是当 inode
被打开之后就不释放 ?

list_lru_shrink_walk 似乎就是遍历一下列表，将可以清理的页面放出来

```c
static enum lru_status dentry_lru_isolate(struct list_head *item,
    struct list_lru_one *lru, spinlock_t *lru_lock, void *arg)
```

然后使用 prune_dcache_sb 紧接着调用 shrink_dentry_list
，将刚刚清理出来的内容真正的释放:

5. shrink 的源头还有 unmount 的时候

那么 dcache / icache 的 shrink 机制在整个 shrink 机制中间是怎么处理的 ?
shrink_node_memcgs ==> shrink_slab ==> 对于所有的 `struct shrinker` 调用
do_shrink_slab

对于 inode 和 icache 的回收是放在 alloc_super 的初始化中间的。 而 x86 kvm
中间也是存在对于 shrinker 的回收工作的。

```c
static struct shrinker mmu_shrinker = {
  .count_objects = mmu_shrink_count,
  .scan_objects = mmu_shrink_scan,
  .seeks = DEFAULT_SEEKS * 10,
};
```

```txt
@[
        inode_add_lru+5
        delete_from_page_cache_batch+794
        truncate_inode_pages_range+298
        ext4_evict_inode+296
        evict+259
        __dentry_kill+113
        dput+235
        __fput+302
        task_work_run+89
        do_exit+717
        do_group_exit+48
        get_signal+2075
        arch_do_signal_or_restart+58
        syscall_exit_to_user_mode+173
        do_syscall_64+107
        entry_SYSCALL_64_after_hwframe+118
]: 5
```

## inode cache
从 superblock 下，从 inode number 到 inode 的查询:

和 dcache 一样，也是定义 hashtable ，但是
```c
static struct hlist_head *inode_hashtable __ro_after_init;
static __cacheline_aligned_in_smp DEFINE_SPINLOCK(inode_hash_lock);
```

inode_insert5 中
```c
struct hlist_head *head = inode_hashtable + hash(inode->i_sb, hashval);
	spin_lock(&inode_hash_lock);
  	old = find_inode(inode->i_sb, head, test, data, true);
```

find_inode 中为什么还是需要 rcu_read_lock 的保护? 具体看

commit 7180f8d91fcb ("vfs: add rcu-based find_inode variants for iget ops")

iget_locked -> find_inode_fast

测试方法，不用用 tree ，要用 yazi ，需要打开文件才可以:
```txt
@[
        iget_locked+5
        __ext4_iget+310
        ext4_lookup+258
        __lookup_slow+133
        walk_component+219
        path_lookupat+103
        filename_lookup+241
        vfs_statx+128
        do_statx+98
        __x64_sys_statx+165
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 855
```


## negative dentry cache

- [Negative dentries, 20 years later](https://lwn.net/Articles/890025/) : 2022
  - https://news.ycombinator.com/item?id=30993527
- [Dentry negativity](https://lwn.net/Articles/814535/) : 2020
- [Dealing with negative dentries](https://lwn.net/Articles/894098) : 2022

- 为什么 nagative dentry 会加速 lookup 的速度？
- 每一次查询都会导致产生一个新的 negative cache 吗?

- 回答这个问题
  - https://unix.stackexchange.com/questions/236914/negative-dentry


## [ ] 分析 fs/readdir.c

1. this file is aim at Man getdents(2)
   1. the example code in Man behave counter intuition : `linux_dirent::d_off`

- [Why does Linux use getdents() on directories instead of read()?](https://stackoverflow.com/questions/36144807/why-does-linux-use-getdents-on-directories-instead-of-read)

- [Two paths to a better readdir()](https://lwn.net/Articles/606995/)

ccls 索引的时候，但是没人用的 `__x64_sys_getdents`

```txt
@[
    __x64_sys_getdents64+5
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 3390
```

注册这个给具体的文件系统用

```c
struct getdents_callback64 buf = {
	.ctx.actor = filldir64,
	.count = count,
	.current_dir = dirent
};
```

- `__x64_sys_getdents64`
  - iterate_dir : 携带参数 getdents_callback64
    - file->f_op->iterate_shared(file, ctx);
      - xfs_dir_file_operations
    - file->f_op->iterate(file, ctx); # 目前的配置中，从来没有被调用过

## 经典问题
2. 基于 pwd 的 openat 实现细节看看


## open 系统调用的基本 flags
<!-- 8537d6bc-e80b-4508-9aef-b98cad1514d0 -->
Three kinds of open flags accoring to man page:
1. file privilege : O_RDONLY, O_WRONLY, or O_RDWR
2. file create : O_CLOEXEC, O_CREAT, O_DIRECTORY, O_EXCL, O_NOCTTY, O_NOFOLLOW, O_TMPFILE
3. file status : O_APPEND, O_ASYNC

- [ ] 这些是如何转换为 e.g. FMODE_WRITE  ?
- [ ] 为什么总是使用的是 openat 而非 open

- [ ]  O_CLOEXEC
> By default, the new file descriptor is set to remain open across an execve(2) (i.e., the FD_CLOEXEC file descriptor flag described in fcntl(2) is initially disabled); the O_CLOEXEC flag, described below, can be used to change this default.  The file offset is set to the beginning of the file (see lseek(2)).

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
