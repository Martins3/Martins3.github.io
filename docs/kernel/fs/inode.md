## inode
感觉 inode.c 其实和 dcache.c 是对称的，inode 本身作为 cache 的，与需要加载，删除，初始化等操作

inode.c 中间存在的函数:
1. iget_locked / iput : 被其他的子系统调用(ext4), 用于创建一个新的 general inode
2. inode_init_owner : 初始化 inode 对应的 uid, gid, mode
3. super_block::s_inode_lru : 用于 inode 的回收工作

而 dcache.c 中存在的函数:
1. d_lookup
2. dput
3. d_alloc
4. d_find_alias : alias 应该是 hash 导致的 ?
5. super_block::s_dentry_lru : 用于回收

1. inode.c 中间存在好几个结尾数字为 5 的函数，表示什么含义啊 ?

其中的集大成者是 iget5_locked ?
```c
struct inode *iget5_locked(struct super_block *sb, unsigned long hashval,
    int (*test)(struct inode *, void *),
    int (*set)(struct inode *, void *), void *data)
{
  struct inode *inode = ilookup5(sb, hashval, test, data);

  if (!inode) {
    struct inode *new = alloc_inode(sb);

    if (new) {
      new->i_state = 0;
      inode = inode_insert5(new, hashval, test, set, data);
      if (unlikely(inode != new))
        destroy_inode(new);
    }
  }
  return inode;
}
```
真正的有意义的调用:

```c
struct block_device *bdget(dev_t dev)
{
  struct block_device *bdev;
  struct inode *inode;

  inode = iget5_locked(blockdev_superblock, hash(dev),
      bdev_test, bdev_set, &dev);
```

有意思
2. 注释，根本不能理解，inode 数量不够是啥意思啊
 * This is a generalized version of ilookup() for file systems where the
 * inode number is not sufficient for unique identification of an inode.

3. 两个版本的函数都是存在的 ilookup 和 ilookup5


* ***hash***
1. 也是存在 inode_hashtable 的，就像是 dentry_hashtable 一样
2. 之所以使用 hash 而不是 inode number，是为了防止多个文件系统，inode numebr 互相重复吧!

```c
static unsigned long hash(struct super_block *sb, unsigned long hashval)
{
  unsigned long tmp;

  tmp = (hashval * (unsigned long)sb) ^ (GOLDEN_RATIO_PRIME + hashval) /
      L1_CACHE_BYTES;
  tmp = tmp ^ ((tmp ^ GOLDEN_RATIO_PRIME) >> i_hash_shift);
  return tmp & i_hash_mask;
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
