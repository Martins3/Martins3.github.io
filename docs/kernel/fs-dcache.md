## dcache

- [ ] 从哪里看 dcache / icache 的容量的
- [ ] 执行一次 Disk usage analyze 真的就会产生好几个 G 的 dcache / icache 吗

问题
1. alias 是做什么的 ?
    1. d_move 的作用
2. d_ops 的作用是什么
3. negtive 怎么产生的 ?

分析一个 cache 基本方法:
1. 加入
2. 删除
3. 查询

There are a number of functions defined which permit a filesystem to manipulate dentries:
dget : open a new handle for an existing dentry (this just increments the usage count)
dput : close a handle for a dentry (decrements the usage count). If the usage count drops to 0, and the dentry is still in its parent’s hash, the “d_delete” method is called to check whether it should be cached. If it should not be cached, or if the dentry is not hashed, it is deleted. Otherwise cached dentries are put into an LRU list to be reclaimed on memory shortage.
d_drop : **this unhashes a dentry from its parents hash list.** A subsequent call to dput() will deallocate the dentry if its usage count drops to 0
d_delete : delete a dentry. If there are no other open references to the dentry then the dentry is turned into a negative dentry (the d_iput() method is called). **If there are other references, then d_drop() is called instead**
d_add : add a dentry to its parents hash list and then calls d_instantiate()
d_instantiate : add a dentry to the alias hash list for the inode and updates the “d_inode” member. The “i_count” member in the inode structure should be set/incremented. If the inode pointer is NULL, the dentry is called a “negative dentry”. This function is commonly called when an inode is created for an existing negative dentry
d_lookup : look up a dentry given its parent and path name component It looks up the child of that given name from the dcache hash table. If it is found, the reference count is incremented and the dentry is returned. The caller must use dput() to free the dentry when it finishes using it.

* ***分析: d_lookup***

d_lookup 相比 `__d_lookup` 多出了 rename 的 lock 问题，查询是在 parent 是否存在指定的 children
```c
static struct hlist_bl_head *dentry_hashtable __read_mostly;

static inline struct hlist_bl_head *d_hash(unsigned int hash)
{
  return dentry_hashtable + (hash >> d_hash_shift);
}
```

按照字符串的名字，找到位置，然后对比。问题是:
1. 似乎 dentry_hashtable 是一个全局的，如果文件系统中间存在几千个 readme.md 岂不是 GG
2. hlist 是怎么维护的呀，大小如何初始化呀

* ***d_add***

1. d_add 调用位置太少了，比较通用的调用位置在 libfs 中间，
2. 其核心执行的内容如下，也就是存在两个 hash，全局的，每个

    hlist_add_head(&dentry->d_u.d_alias, &inode->i_dentry);
3. 实际上，依托 hlist_bl_add_head_rcu d_hash 加入 dentry_hashtable


The “dcache” caches information about names in each filesystem to make them quickly available for lookup.
Each entry (known as a “dentry”) contains three significant fields:
1. a component name,
2. *a pointer to a parent dentry*,
3. and a pointer to the “inode” which contains further information about the object in that parent with the given name.
> 为啥需要包含 parent dentry ?

#### dcache shrink
1. alloc_super 中间创建了一个完成 shrinker 的初始化 TODO 这个 shrinker 机制还有人用吗 ?
    1. super_cache_scan
    2. super_cache_count

2. `struct superpage` 中间存在两个函数 TODO 应该用于特殊内容的缓存的
```c
  long (*nr_cached_objects)(struct super_block *,
          struct shrink_control *);
  long (*free_cached_objects)(struct super_block *,
            struct shrink_control *);
```
3.  super_cache_scan 调用两个函数
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

prune_icache_sb : TODO 这个是用于释放 inode 还是 inode 持有的文件 ? 还是当 inode 被打开之后就不释放 ?
```


list_lru_shrink_walk 似乎就是遍历一下列表，将可以清理的页面放出来
```c
static enum lru_status dentry_lru_isolate(struct list_head *item,
    struct list_lru_one *lru, spinlock_t *lru_lock, void *arg)
```
然后使用 prune_dcache_sb 紧接着调用 shrink_dentry_list ，将刚刚清理出来的内容真正的释放:

5. shrink 的源头还有 unmount 的时候

那么 dcache / icache 的 shrink 机制在整个 shrink 机制中间是怎么处理的 ?
shrink_node_memcgs ==> shrink_slab ==> 对于所有的 `struct shrinker` 调用 do_shrink_slab

对于 inode 和 icache 的回收是放在 alloc_super 的初始化中间的。
而 x86 kvm 中间也是存在对于 shrinker 的回收工作的。
```c
static struct shrinker mmu_shrinker = {
  .count_objects = mmu_shrink_count,
  .scan_objects = mmu_shrink_scan,
  .seeks = DEFAULT_SEEKS * 10,
};
```
