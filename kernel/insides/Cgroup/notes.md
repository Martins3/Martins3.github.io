# Introduction
groups are organized hierarchically and here this mechanism is similar to usual processes as they are hierarchical too and child cgroups inherit set of certain parameters from their parents.

1. The first way is to create subdirectory in any subsystem from /sys/fs/cgroup and add a pid of a task to a tasks file which will be created automatically right after we will create the subdirectory.
2. The second way is to create/destroy/manage cgroups with utils from libcgroup library
> 用户态添加 两种初始化的方法，作者对于第一个讲解一个例子，很有意思的
> docker 的例子并没有分析

Initialization of cgroups divided into two parts in the Linux kernel: early and late. In this part we will consider only early part and late part will be considered in next parts.

```c
asmlinkage __visible void __init start_kernel(void)

/**
 * cgroup_init_early - cgroup initialization at system boot
 *
 * Initialize cgroups at system boot, and initialize any
 * subsystems that request early init.
 */
int __init cgroup_init_early(void)
{
	static struct cgroup_sb_opts __initdata opts; //  represents mount options of cgroupfs
	struct cgroup_subsys *ss;
	int i;

	init_cgroup_root(&cgrp_dfl_root, &opts);
	cgrp_dfl_root.cgrp.self.flags |= CSS_NO_REF;

	RCU_INIT_POINTER(init_task.cgroups, &init_css_set);

	for_each_subsys(ss, i) {
		WARN(!ss->css_alloc || !ss->css_free || ss->name || ss->id,
		     "invalid cgroup_subsys %d:%s css_alloc=%p css_free=%p id:name=%d:%s\n",
		     i, cgroup_subsys_name[i], ss->css_alloc, ss->css_free,
		     ss->id, ss->name);
		WARN(strlen(cgroup_subsys_name[i]) > MAX_CGROUP_TYPE_NAMELEN,
		     "cgroup_subsys_name %s too long\n", cgroup_subsys_name[i]);

		ss->id = i;
		ss->name = cgroup_subsys_name[i];
		if (!ss->legacy_name)
			ss->legacy_name = cgroup_subsys_name[i];

		if (ss->early_init)
			cgroup_init_subsys(ss, true);
	}
	return 0;
}
```
> 后面都是解释 cgroup_init_early 的，并不关心这些东西!
