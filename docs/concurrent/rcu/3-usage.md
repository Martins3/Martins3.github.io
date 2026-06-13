## 总结下其中的 workqueue 中间的 rcu
- flush_rcu_work

## [ ] xarray

```c
/* Private */
static inline void *xa_head(const struct xarray *xa)
{
	return rcu_dereference_check(xa->xa_head,
						lockdep_is_held(&xa->xa_lock));
}

/* Private */
static inline void *xa_head_locked(const struct xarray *xa)
{
	return rcu_dereference_protected(xa->xa_head,
						lockdep_is_held(&xa->xa_lock));
}
```

## [ ] md raid
https://lore.kernel.org/all/20230523021017.3048783-6-yukuai1@huaweicloud.com/


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
