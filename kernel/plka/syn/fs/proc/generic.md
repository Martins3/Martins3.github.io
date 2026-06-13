# fs/proc/generic.c
1. this file contains:
    1. create entry
    2. remove entry
    3. pde_subdir

## TODO
1.  pde_subdir_insert : why pde need rb tree ?



##  create entry(2) : proc_mkdir => proc_mkdir_data

```c
struct proc_dir_entry *proc_mkdir(const char *name,
		struct proc_dir_entry *parent)
{
	return proc_mkdir_data(name, 0, parent, NULL);
}

struct proc_dir_entry *proc_mkdir_data(const char *name, umode_t mode,
		struct proc_dir_entry *parent, void *data)
{
	struct proc_dir_entry *ent;

	if (mode == 0)
		mode = S_IRUGO | S_IXUGO;

	ent = __proc_create(&parent, name, S_IFDIR | mode, 2);
	if (ent) {
		ent->data = data;
		ent->proc_dir_ops = &proc_dir_operations;
		ent->proc_iops = &proc_dir_inode_operations;
		ent = proc_register(parent, ent); // proc_register again
	}
	return ent;
}
```


## create entry(1) :  proc_create_single_data && proc_create_single_data
1. usage : fs/proc/cmdline.c
2. how vfs open becomes proc_single_ops ?
    1. read => seq_read
3. @todo compare `proc_create_data` and `proc_create_single_data`

```c
struct proc_dir_entry *proc_create_single_data(const char *name, umode_t mode,
		struct proc_dir_entry *parent,
		int (*show)(struct seq_file *, void *), void *data)
{
	struct proc_dir_entry *p;

	p = proc_create_reg(name, mode, &parent, data); // create one entry !
	if (!p)
		return NULL;
	p->proc_ops = &proc_single_ops; // XXX difference between proc_create_data
	p->single_show = show;
	return proc_register(parent, p); // subdir insert
}

struct proc_dir_entry *proc_create_data(const char *name, umode_t mode,
		struct proc_dir_entry *parent,
		const struct proc_ops *proc_ops, void *data)
{
	struct proc_dir_entry *p;

	p = proc_create_reg(name, mode, &parent, data);
	if (!p)
		return NULL;
	p->proc_ops = proc_ops; // XXX difference between proc_create_single_data
	return proc_register(parent, p);
}

static const struct proc_ops proc_single_ops = {
	.proc_open	= proc_single_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int proc_single_open(struct inode *inode, struct file *file)
{
	struct proc_dir_entry *de = PDE(inode);

	return single_open(file, de->single_show, de->data);
}

int single_open(struct file *file, int (*show)(struct seq_file *, void *),
		void *data)
{
	struct seq_operations *op = kmalloc(sizeof(*op), GFP_KERNEL_ACCOUNT);
	int res = -ENOMEM;

	if (op) {
		op->start = single_start;
		op->next = single_next;
		op->stop = single_stop;
		op->show = show;
		res = seq_open(file, op);
		if (!res)
			((struct seq_file *)file->private_data)->private = data;
		else
			kfree(op);
	}
	return res;
}
EXPORT_SYMBOL(single_open);
```


## TODO : /home/martins3/core/vn/kernel/plka/syn/fs/proc/ 中的内容可以一并整理过来
