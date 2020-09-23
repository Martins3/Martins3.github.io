# kernel/cgroup/cpuset.md
> 以后再说吧! 这他妈是什么啊 ?

## 外部interface cpuset_cgrp_subsys
1. scheduler 中间不是已经存在了管理cpu 的机制吗 ?


```c
struct cgroup_subsys cpuset_cgrp_subsys = {
	.css_alloc	= cpuset_css_alloc,
	.css_online	= cpuset_css_online,
	.css_offline	= cpuset_css_offline,
	.css_free	= cpuset_css_free,
	.can_attach	= cpuset_can_attach,
	.cancel_attach	= cpuset_cancel_attach,
	.attach		= cpuset_attach,
	.post_attach	= cpuset_post_attach,
	.bind		= cpuset_bind,
	.fork		= cpuset_fork,
	.legacy_cftypes	= files,
	.early_init	= true,
};

```

## struct cpuset 
```c
struct cpuset {
	struct cgroup_subsys_state css;
```
怀疑所有的subsys 采用这种机制

