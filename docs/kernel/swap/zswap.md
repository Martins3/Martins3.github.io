## zswap
- https://docs.kernel.org/admin-guide/mm/zswap.html

- https://github.com/facebook/zstd
  - 这里有个表格，记录每一种的压缩算法以及压缩的速度

- [x] zswap 压缩的内存还可以进一步的被 swap 出去的?
  - 是的



## 使用
cat /proc/swaps 如何调整一下 priority

存在 debugfs 吗？

如果在 centos 中打开?

`__zswap_pool_release` 中存在 `synchronize_rcu` 开始分析


可以给 /sys/module/zswap/parameters/enabled 动态关闭，但是
用 htop 看 frontswap 还是有

## 观察一下

开机的时候就有:
```txt
🧀  cat /proc/vmstat  | grep zs
nr_zspages 5
zswpin 0
zswpout 0
```

使用 stress-ng 挤压一下，得到如何结果:
```txt
nr_zspages 11286
zswpin 104121
zswpout 1266473
```

- [ ] 为什么系统启动的时候就有 5 个 nr_zspages

## 看看基本的使用吧
https://wiki.archlinux.org/title/zswap


## 关于 zram 和 zswap 的对比
https://www.reddit.com/r/linux/comments/11dkhz7/zswap_vs_zram_in_2023_whats_the_actual_practical/

https://wiki.gentoo.org/wiki/Zswap

## 原来我被 nixos 上的 zswap 骗了
我原来一直使用的 zram 啊


这里是 zswap 的文档:
Documentation/admin-guide/mm/zswap.rst

https://en.wikipedia.org/wiki/Zstd

## zswap 的实现

```c
struct zswap_lruvec_state {
	/*
	 * Number of pages in zswap that should be protected from the shrinker.
	 * This number is an estimate of the following counts:
	 *
	 * a) Recent page faults.
	 * b) Recent insertion to the zswap LRU. This includes new zswap stores,
	 *    as well as recent zswap LRU rotations.
	 *
	 * These pages are likely to be warm, and might incur IO if the are written
	 * to swap.
	 */
	atomic_long_t nr_zswap_protected;
};

unsigned long zswap_total_pages(void);
bool zswap_store(struct folio *folio);
bool zswap_load(struct folio *folio);
void zswap_invalidate(swp_entry_t swp);

int zswap_swapon(int type, unsigned long nr_pages);
void zswap_swapoff(int type);
void zswap_memcg_offline_cleanup(struct mem_cgroup *memcg);
void zswap_lruvec_state_init(struct lruvec *lruvec);
void zswap_folio_swapin(struct folio *folio); // swap in 的 hook 而已

bool zswap_is_enabled(void);
bool zswap_never_enabled(void);
```

其实侵入到其他的模块的代码很少。
只是需要照着写就是没有问题。

- free_swap_slot : 只是被 swapfile 使用的方案而已，有趣的，为什么这样？
  - zswap_invalidate

- zswap_store
- zswap_load

当然，需要支持 swapon 和 swapoff 的东西:

进入到 zswap 之后，就不可以继续映射这个 page 了，


1. 什么时候处理的反向映射的问题的?

- shrink_folio_list
  - pageout : 这里去执行对应的 page
  - __remove_mapping : 是
  - free_unref_folios : 这里来释放的

所以，还是不要共享内存，自己构建相同地址空间的 thread 来填充。


还不如搞一个让 kernel 直接通往后端，写的时候是同步的，

2. 如何和 page fault 的过程想要处理

## 其实 front swap 是一个 generic 的 swap 实现
https://lore.kernel.org/lkml/20230714194610.828210-1-hannes@cmpxchg.org/

1. 一共存在哪几种 backend ?
2. frontend 如何组织它们 ?
3. 涉及到硬件了吗 ?
5. 前端和后端之间的接口是什么 ?

- [ ] 至少存在 swap 下是如何使用文件和磁盘的分流的

## zswap 为什么也是有 cgroup 啊！


## zcache
https://mp.weixin.qq.com/s/XRIkDpMba8hHfJr9ukMw8Q

的确是一个很好的方向，只是为什么到现在没人做

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
