## vfs 的上锁规则是什么

```txt
#0  instance_mkdir (name=0xffff8881613b8520 "g") at kernel/trace/trace.c:9364
#1  0xffffffff815bc0da in tracefs_syscall_mkdir (mnt_userns=<optimized out>, inode=0xffff888161aec750, dentry=<optimized out>, mode=<optimized out>) at fs/tracefs/inode.c:87
#2  0xffffffff81374a0f in vfs_mkdir (mnt_userns=0xffffffff82a627c0 <init_user_ns>, dir=0xffff888161aec750, dentry=dentry@entry=0xffff888127f4f240, mode=<optimized out>, mode@entry=511) at fs/namei.c:4035
#3  0xffffffff813797e1 in do_mkdirat (dfd=dfd@entry=-100, name=0xffff88812165c000, mode=mode@entry=511) at fs/namei.c:4060
#4  0xffffffff813799d3 in __do_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4080
#5  __se_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4078
#6  __x64_sys_mkdir (regs=<optimized out>) at fs/namei.c:4078
#7  0xffffffff81fa4bcb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001cc3f58) at arch/x86/entry/common.c:50
#8  do_syscall_64 (regs=0xffffc90001cc3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#9  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#10 0x0000000000000000 in ?? ()
```

为什么最后上锁是在这里的，如果修改目录的时候，上锁规则是什么?
```c
static int instance_mkdir(const char *name)
{
	struct trace_array *tr;
	int ret;

	mutex_lock(&event_mutex);
	mutex_lock(&trace_types_lock);

	ret = -EEXIST;
	if (trace_array_find(name))
		goto out_unlock;

	tr = trace_array_create(name);

	ret = PTR_ERR_OR_ZERO(tr);

out_unlock:
	mutex_unlock(&trace_types_lock);
	mutex_unlock(&event_mutex);
	return ret;
}
```

https://zhuanlan.zhihu.com/p/261669249



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
