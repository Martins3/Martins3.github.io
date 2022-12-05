# kernel/systcl.md
> 所以，将这个文件放到 kernel 下面，有点蛇皮，更加应该放到 fs/proc 下的啊 !

1. 所以和fs/sysfs的关系是什么，一个实现/proc 一个实现 /sys/ 的吗 ?
2. 前面2000行都是各种定义蛇皮表格! @todo 通过表格，但是如何形成 fs 的 tree ?
3. handler 最后是如何被调用的呢 ? 或者向 /proc/sys/vm/compact_memory 中间写入1，那么最后如何实现将handler 调用，以及!

```c
int __init sysctl_init(void)
{
	struct ctl_table_header *hdr;

  // TODO sysctl_base_table 中间的子项目不知道对应的内容，至少不是/proc
	hdr = register_sysctl_table(sysctl_base_table);
	kmemleak_not_leak(hdr);
	return 0;
}
// fs/proc/proc_sysctl.c 中间，可以确定这个文件只是proc/sys 的部分内容
// 但是我ls 出来的比 sysctl_base_table 的内容更多啊! 比如缺少关键的net !
// TODO 而且有的tbale 是空的 !
// net 具体的内容被放到网络中间了 !
int __init proc_sys_init(void)
{
	struct proc_dir_entry *proc_sys_root;

	proc_sys_root = proc_mkdir("sys", NULL);
	proc_sys_root->proc_iops = &proc_sys_dir_operations;
	proc_sys_root->proc_fops = &proc_sys_dir_file_operations;
	proc_sys_root->nlink = 0;

	return sysctl_init();
}
```

> 还包含一些字符串，数值，bitmap 等各种io的处理
```c
// 一个proc_handler 可能用于 /proc/sys/kernel/hostname 之类的保存一个全局变量的之类的操作吧 !
typedef int proc_handler (struct ctl_table *ctl, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos);

int proc_dostring(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	if (write)
		proc_first_pos_non_zero_ignore(ppos, table);

	return _proc_do_string((char *)(table->data), table->maxlen, write,
			       (char __user *)buffer, lenp, ppos);
}

int proc_douintvec(struct ctl_table *table, int write,
		     void __user *buffer, size_t *lenp, loff_t *ppos)
```

