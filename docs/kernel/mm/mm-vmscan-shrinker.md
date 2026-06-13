# mm shrinker

mm/shrinker.c
## slab

```c
// TODO 那么fs中的dcache.c的内容，而且buffer.c 中间的内容还是看不懂!

// 调用者，这只是一个调用路径而已
// https://sysctl-explorer.net/vm/drop_caches/
int drop_caches_sysctl_handler(struct ctl_table *table, int write, void __user *buffer, size_t *length, loff_t *ppos)
  -> void drop_slab(void)
    -> void drop_slab_node(int nid)
      -> static unsigned long shrink_slab(gfp_t gfp_mask, int nid, struct mem_cgroup *memcg, int priority)
        -> static unsigned long do_shrink_slab(struct shrink_control *shrinkctl, struct shrinker *shrinker, int priority)
```

#### shrink slab
1. 面试问题 : dcache 的需要 slab，slab 分配需要 page，那么 page cache, slab 和 dcache 的回收之间的关系是什么


或者说，shrink 的 slab 的关系是什么?

这是唯一的使用位置:
```c
static unsigned long shrink_slab(gfp_t gfp_mask, int nid,
                 struct mem_cgroup *memcg,
                 int priority)
         // 对于所有的注册的 shrinker 循环调用 do_shrink_slab
         // 也就是说，其实每个 cache shrinker 之间其实没有什么关系

static unsigned long do_shrink_slab(struct shrink_control *shrinkctl,
                    struct shrinker *shrinker, int priority)
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
