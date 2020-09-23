# fs/proc/proc_sysctl.c 实现
> 这个文件的所有的内容，还是限于/proc/sys/ 的描述

```c
static const struct file_operations proc_sys_file_operations = {
	.open		= proc_sys_open,
	.poll		= proc_sys_poll,
	.read		= proc_sys_read,
	.write		= proc_sys_write,
	.llseek		= default_llseek,
};

static const struct file_operations proc_sys_dir_file_operations = {
	.read		= generic_read_dir,
	.iterate_shared	= proc_sys_readdir,
	.llseek		= generic_file_llseek,
};

static const struct inode_operations proc_sys_inode_operations = {
	.permission	= proc_sys_permission,
	.setattr	= proc_sys_setattr,
	.getattr	= proc_sys_getattr,
};

static const struct inode_operations proc_sys_dir_operations = {
	.lookup		= proc_sys_lookup,
	.permission	= proc_sys_permission,
	.setattr	= proc_sys_setattr,
	.getattr	= proc_sys_getattr,
};
```


> how to build a tree by table
```c
struct ctl_table_header *register_sysctl_table(struct ctl_table *table) {
	static const struct ctl_path null_path[] = { {} };
	return register_sysctl_paths(null_path, table);
}

struct ctl_table_header *register_sysctl_paths(const struct ctl_path *path,
						struct ctl_table *table) {
	return __register_sysctl_paths(&sysctl_table_root.default_set,
					path, table);
}

struct ctl_table_header *__register_sysctl_paths(
	struct ctl_table_set *set,
	const struct ctl_path *path, struct ctl_table *table)

// 调用下面两者 :
struct ctl_table_header *__register_sysctl_table( struct ctl_table_set *set, const char *path, struct ctl_table *table)
static int register_leaf_sysctl_tables(const char *path, char *pos, struct ctl_table_header ***subheader, struct ctl_table_set *set, struct ctl_table *table)
```


