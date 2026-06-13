# martins3 scheduler

- [ ] 为什么在 sched_fork 中配置了 task 的

通过 policy 可以控制 scheduler class ，然后就是:
```c
struct task_struct {
    // ...
	const struct sched_class	*sched_class;
    unsigned int			policy;
    // ...
```

因为首先加入到 sched_class 的队列中，然后加入到 kernel 中:

所以，这里的逻辑还有一个问题，就是需要知道逐个 pick 的入口

这里的说的 switch all ，其实只是让默认在 ext 吧
```c
/*
 * Used by sched_fork() and __setscheduler_prio() to pick the matching
 * sched_class. dl/rt are already handled.
 */
bool task_should_scx(int policy)
{
	if (!scx_enabled() ||
	    unlikely(scx_ops_enable_state() == SCX_OPS_DISABLING))
		return false;
	if (READ_ONCE(scx_switching_all))
		return true;
	return policy == SCHED_EXT;
}
```

所以，现在自然有一个问题，如果在框架之上的东西会不会成为问题?

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
