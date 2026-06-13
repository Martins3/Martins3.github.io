# sysfs module
## /proc/modules
cat /proc/modules 可以查看所有的 module 的链接位置

## /proc/sys/kernel/modprobe
- https://docs.kernel.org/next/admin-guide/sysctl/kernel.html#modprobe

```c
#ifdef CONFIG_MODULES
	{
		.procname	= "modprobe",
		.data		= &modprobe_path,
		.maxlen		= KMOD_PATH_LEN,
		.mode		= 0644,
		.proc_handler	= proc_dostring,
	},
	{
		.procname	= "modules_disabled",
		.data		= &modules_disabled,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		/* only handle a transition from default "0" to "1" */
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= SYSCTL_ONE,
		.extra2		= SYSCTL_ONE,
	},
#endif
```

不知道是谁初始化的!
```txt
🧀  cat /proc/sys/kernel/modprobe
/nix/store/1z6hk4iky1wv6gaa8s0isn35489x0fa2-kmod-30/bin/modprobe
```
其使用位置是:
- `__request_module` : 调用位置非常多，我猜测是，这个的作用是，内核想要调用 modprobe 的时候，就需要知道 modprobe 的位置。
  - call_modprobe

### cat /sys/module/kvm_intel/parameters/nested
分析下这个目录是如何形成的

### sysfs 中的 module 链接都是如何形成的
/sys/bus/pci/devices/0000:00:0b.0 中
```txt
lrwxrwxrwx     - root 13 Dec 20:30   driver -> ../../../bus/pci/drivers/nvme
```

## /proc/kallsyms

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
