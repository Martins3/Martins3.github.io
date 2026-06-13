# Professional Linux Kernel Architecture : Filesystems without Persistent Storage
In addition to the proc filesystem, the kernel provides many other virtual filesystems for
various purposes, for example, for the management of all devices and system resources
cataloged in the form of files in hierarchically structured directories.

**Sysfs** is,
per convention, always mounted at /sys, but there is nothing that would prevent including it in other places
It was designed to export information from the kernel into userland
at a highly structured level. In contrast to **procfs**, it was not designed for direct human
use because the information is deeply and hierarchically nested.
The filesystem is, however, very useful for tools that want to gather detailed information about the **hardware present** in a system and the **topological connection** between the
devices.

## 10.1 The proc Filesystem

#### 10.1.1 Contents of /proc
> 介绍了/proc 中间包含的比较关键的项目

Information can be grouped into a few larger categories:
- Memory management
- Characteristic data of system processes
- Filesystems
- Device drivers
- System buses
- Power management
- Terminals
- System control parameters

The trend in kernel development is away from the provision of information by the proc filesystem and
toward the exporting of data by a problem-specific but likewise virtual filesystem. A good example of this
is the USB filesystem which is used to export many types of status information on the **USB** subsystem into
userspace without ‘‘overburdening‘‘ /proc with new entries. 

* ***Process-Specific Data***
1. cmdline
2. maps
3. status


* ***General System Information***

> 描述在 /proc 目录下文件夹下部分关键文件的描述
> 1. /proc/kallsyms
> 1. /proc/kcore
> 1. /proc/interrupts (@question)
> 1. /proc/ioports
> 在其他的章节中间基本见过

The **kallsyms** and **kcore** entries support kernel code debugging. The former holds a table with the
addresses of all global kernel variables and procedures including their addresses in memory
kcore is a dynamic core file that ‘‘contains‘‘ all data of the running kernel — that is, the entire contents of
main memory

* ***Network Information***

The /proc/net subdirectory supplies data on the various network options of the kernel. The information
held there is a colorful mix of protocol and device data and includes several interesting entries as follows:
> tcp udp arp 以及一些网卡驱动的信息

* ***System Control Parameters***

The `sysctl` system call is not really needed because the /proc interface is a kernel data manipulation
option of unrivaled simplicity.

> (Man sysctl)
> sysctl is used to modify kernel parameters at runtime. 
> The parameters available are those listed under /proc/sys/.
> Procfs is required for sysctl support in Linux.
> You can use sysctl to both read and write sysctl data.

> `sysctl` 是实现 /proc/sys　的基础，通过读写 /proc/sys 操控内核运行参数

#### 10.1.2 Data Structures
proc makes generous use of structures of the virtual filesystem , simply because, as a filesystem itself, it must be integrated
into the VFS layer of the kernel.

* ***Representation of proc Entries***
> this section explain proc_dir_entry

**Each entry** in the proc filesystem is described by an instance of `proc_dir_entry` whose (abbreviated)
definition is as follows:

The following three elements are available to control the exchange of information between the virtual
filesystem (and ultimately userspace) and the various proc entries or individual kernel subsystems.

`proc_iops` and `proc_fops` are pointers to instances of types inode_operations and file_operations
discussed in Chapter 8. They hold operations that can be performed on an inode or file and act as an interface to the virtual filesystem that relies on their presence. The operations used depend on the particular
file type and are discussed in more detail below.
> there are a little modification, to following : 
```c
	union {
		const struct proc_ops *proc_ops;
		const struct file_operations *proc_dir_ops;
	};
	const struct dentry_operations *proc_dops;
	union {
		const struct seq_operations *seq_ops;
		int (*single_show)(struct seq_file *, void *);
	};
```

1. Because each entry is given a filename, the kernel uses two elements of the structure to store the corresponding information: name is a pointer to the string in which the `name` is held, and `namelen` specifies
2. Also adopted from the classic filesystem concept is the numbering of all inodes using `low_ino`.
> @question `low_ino` 到底是什么作用 ?
3. The meaning of `mode` is the same as in normal filesystems because the element reflects the
type of the entry (file, directory, etc.), and the assignment of access rights in accordance with the classic
"owner, group, others" scheme by means of the appropriate constants in <stat.h>.
4. `uid` and `gid` specify
the user ID and group ID to which the file belongs. Both are usually set to 0, which means that the root
user is the owner of almost all proc files.
5. `nlink` `parent` `subdir` `subdir_node` : are used by the kernel to represent the hierarchical structure of the filesystem by means of the following elements.

* ***proc inodes***

```c
struct proc_inode {
	struct pid *pid;
	int fd;
	union proc_op op;
	struct proc_dir_entry *pde; // 和 proc_dir_entry 沟通起来
	struct ctl_table_header *sysctl;
	struct ctl_table *sysctl_entry;
	const struct proc_ns_operations *ns_ops;
	struct inode vfs_inode;
};

union proc_op {
	int (*proc_get_link)(struct dentry *, struct path *);
	int (*proc_show)(struct seq_file *m,
		struct pid_namespace *ns, struct pid *pid,
		struct task_struct *task);
	const char *lsm;
};
```
**The purpose of the structure is to link the proc-specific data with the inode data of the VFS layer.**

The remaining elements of the structure are only used if the inode represents a process-specific entry
(which is therefore located in the proc/pid directory). Their meanings are as follows:
1. `fd` holds the filedescriptor for which a file in `/proc/<pid>/fd/` presents information. With the
help of fd, all files in this directory can use the same `file_operations`.
2. `pid` is a pointer to the pid instance of a process. Because it is possible to access a large amount of
process-specific information this way, it is clear why a process-specific inode should be directly
associated with this data.
3. `proc_get_link` and `proc_read` (which are collected in a union because only one at a time makes
sense) are used to get process-specific information or to generate links to process-specific data in
the Virtual Filesystem

#### 10.1.3 Initialization
> explain root.c::proc_root_init
> 1. register the proc_fs_type
> 2. create some entry, but not all entry 
>     1. `fs_initcall(proc_cmdline_init);` some entry are initialized by this way !

#### 10.1.4 Mounting the Filesystem

In the view of the system administrator in userspace, mounting /proc is almost the same as mounting a
non-virtual filesystem. The only difference is that an arbitrary keyword (usually proc or none) is specified
as the source instead of a device file:
```c
root@meitner # mount -t proc proc /proc
```

The root inode differs from all other inodes of the proc file system in that it not only contains ‘‘normal‘‘
files and directories (even though they are generated dynamically), but also manages the process-specific
PID directories that contain detailed information on the individual system processes, as mentioned above.
The root inode therefore has its own inode and file operations, which are defined as follows:
> in `proc_fill_super`, the `proc_root` is initialized as root inode of proc

#### 10.1.5 Managing /proc Entries
Before the proc filesystem can be put to meaningful use, it must be filled with entries containing data.
Several auxiliary routines are provided to add files, create directories, and so on, in order to make this
job as easy as possible for the remaining kernel sections
> 原来/proc 中间的项目都是初始化的创建的，但是又是和各种具体的模块如何逐渐沟通起来的 ?


* ***Creating and Registering Entries***

Creating and Registering Entries
1. First, a new instance of `proc_dir_entry` is created together with all information needed to describe the entry.
2. Once the entry has been created, it is registered with the proc filesystem using `proc_register` in `fs/proc/generic.c`.

```diff
-	entry = create_proc_entry("sequence", 0, NULL);
-	if (entry)
-		entry->proc_fops = &ct_file_ops;
+	entry = proc_create("sequence", 0, NULL, &ct_file_ops);
```
> 3.10的变化, 似乎把原来的create 和 register 的功能放到一起了

```c
struct proc_dir_entry *proc_create_data(const char *name, umode_t mode,
		struct proc_dir_entry *parent,
		const struct file_operations *proc_fops, void *data)
{
	struct proc_dir_entry *p;

	BUG_ON(proc_fops == NULL);

	p = proc_create_reg(name, mode, &parent, data);
	if (!p)
		return NULL;
	p->proc_fops = proc_fops;
	return proc_register(parent, p);
}
EXPORT_SYMBOL(proc_create_data);

struct proc_dir_entry *proc_create(const char *name, umode_t mode,
				   struct proc_dir_entry *parent,
				   const struct file_operations *proc_fops)
{
	return proc_create_data(name, mode, parent, proc_fops, NULL);
}
EXPORT_SYMBOL(proc_create);
```

Once the entry has been created, it is registered with the proc filesystem using proc_register in
`fs/proc/generic.c`. The task is divided into three steps:
1. A unique proc-internal number is generated to give the entry its own identity.
`get_inode_number` is used to return an unused number for dynamically generated
entries.
2. The next and parent elements of the `proc_dir_entry` instance must be set appropriately to
incorporate the new entry into the hierarchy.
3. Depending on the file type, the pointers must be set appropriately to file and inode
operations if the corresponding elements of `proc_dir_entry`, `proc_iops` and `proc_fops`
previously contained a null pointer. Otherwise, the value held there is retained
> 如果分析proc_register 中间的内容，上面的描述的关于 get_inode_number , proc_iops 等操作完全不同，实际上proc_register的操作大概就是做了树的节点的悬挂操作


> proc_dir_operations
> proc_dir_inode_operations
> proc_file_operations
> proc_file_inode_operations
> 找到这些东西，然后确定ext4 中间是不是也含有这些蛇皮，确定这些蛇皮和vfs 是如何沟通的


Although we are not interested in their implementation, I include below a list of other auxiliary functions
used to manage proc entries:
1. `proc_mkdir` creates a new directory.
2. `proc_mkdir_mode` creates a new directory whose access mode can be explicitely specified.
3. `proc_symlink` generates a symbolic link.
4. `remove_proc_entry` deletes a dynamically generated entry from the proc directory

* **Finding Entries**

As discussed there, the lookup process (e.g., of the open system call) duly arrives at `real_lookup`, which
invokes the function saved in the lookup function pointer of `inode_operations` to *resolve the filename
by reference to its individual path components*. 

```c
static struct dentry *proc_root_lookup(struct inode * dir, struct dentry * dentry, unsigned int flags)
{
	if (!proc_pid_lookup(dir, dentry, flags))
		return NULL;
	
	return proc_lookup(dir, dentry, flags);
}
```

The search for entries starts at the mount point of the proc filesystem, usually /proc. In Section 10.1.2
you saw that the lookup pointer of the `file_operations` instance for the root directory of the process
filesystem points to the `proc_root_lookup` function. 
> proc_root_lookup 的赋值的真实位置在 proc_fill_super 中间
> @question 只是proc 的查询函数分普通inode 和 root inode, 还是所有的文件系统丢失如此，为什么需要划分为root 的普通inode 的 operations 函数

The kernel uses this routine simply to distinguish between two different types of proc entries before
delegating the real work to specialized routines
> 查询划分为 process-specific 和 driver/subsystem 的两个部分

#### 10.1.6 Reading and Writing Information
> 非常的奇怪，在generic.c中间，含有各种inode_operations，但是就是没有proc_file_operations 这一个结构体的定义。


#### 10.1.7 Task-Related Information
The goal of the routine is to create an inode that acts as the first object for further PID-specific operations;
this is because the inode represents the `/proc/pid` directory containing all files with process-specific
information. Two cases, analyzed below, must be distinguished.

![](../img/10-4.img)

* **The self directory**

`proc_pid_lookup` 中间并没有关于self 的判断。

```c
static const struct inode_operations proc_self_inode_operations = {
	.readlink	= proc_self_readlink,
	.follow_link	= proc_self_follow_link,
	.put_link	= kfree_put_link,
};
```
This is precisely the purpose of the two functions
in `proc_self_inode_operations` whose implementations require just a few lines:

> 内容都在base.c 中间，但是问题是查询的时候，为什么要创建一个inode, 什么时候销毁这些inode
> 所以ID文件和PGID 文件都是搞什么的

> 为什么需要分析pid 相关的文件，因为其中的文件内容全部都是全部都是静态编写好的，所以初始化和查询含有都是特别的注册的
> 本书，只是通过查询函数做了一个线索

> 所以这些动态的信息到底是他妈怎么获取的 比如/proc/self/cmdline
> 而且这些函数的层次依旧是在VFS 上面啊

The remaining work is then delegated to standard virtual filesystem functions that are responsible for
directing the lookup operation to the right places.
> self 其实本身就是指向当前进程的 pid 号的哪一个文件夹，所以只要处理simbol link即可.

* **Selection According to PID**

```c
struct dentry *proc_pid_lookup(struct inode *dir, struct dentry * dentry, unsigned int flags)
```
> 主要是对于 proc_pid_lookup 的描述(源码简单), lookup 的过程中间动态的创建inode 同时将其中的数据进行填充


> 上面的inode 是创建/proc/1234 的inode, proc_tgid_base_lookup 实现 /proc/10/cmdline 之类的inode 的创建和查找


Because the
contents of the PID-specific directory are always the same, a static list of all files together with a few other
bits of information is defined in the kernel sources. The list is called tgid_base_stuff and is used to find
out easily whether a desired directory entry exists or not.
> 书中描述这些静态信息创建的过程

![](../img/10-5.img)

#### 10.1.8 System Control Mechanism
To resolve this situation, Linux resorts to the proc filesystem. It exports to /proc/sys a directory structure
that arranges all sysctls hierarchically and also allows parameters to be read and manipulated using
simple userspace tools; cat and echo are sufficient to modify kernel run-time behavior

* **Using Sysctls**
> 1. 注意，这是sysctl 不是syscall
> 2. systemctl 的实现是不是基于sysctl 机制的

```c
/* Read or write system parameters.  */
extern int sysctl (int *__name, int __nlen, void *__oldval,
		   size_t *__oldlenp, void *__newval, size_t __newlen) __THROW;
```

* **Data Structures**
Because sysctls are arranged hierarchically (each larger kernel
subsystem defines its own sysctl list with its various subsections), the data structure must not only hold
information on the individual sysctls and their read and write operations, it must also provide ways of
mapping the hierarchy between the individual entries.
```c
/* A sysctl table is an array of struct ctl_table: */
struct ctl_table 
{
	const char *procname;		/* Text ID for /proc/sys, or zero */
	void *data;
	int maxlen;
	umode_t mode;
	struct ctl_table *child;	/* Deprecated */
	proc_handler *proc_handler;	/* Callback for text formatting */
	struct ctl_table_poll *poll;
	void *extra1;
	void *extra2;
} __randomize_layout;
```
> 书中对于每一个成员的描述，@todo

```c
/* struct ctl_table_header is used to maintain dynamic lists of
   struct ctl_table trees. */
struct ctl_table_header
{
	union {
		struct {
			struct ctl_table *ctl_table;
			int used;
			int count;
			int nreg;
		};
		struct rcu_head rcu;
	};
	struct completion *unregistering;
	struct ctl_table *ctl_table_arg;
	struct ctl_table_root *root;
	struct ctl_table_set *set;
	struct ctl_dir *parent;
	struct ctl_node *node;
	struct hlist_head inodes; /* head for proc_inode->sysctl_inodes */
};
```
> 实际上 ctl_table_header 比书上的描述要复杂的多得多，但是而且对于ctl_table 的数据结构也是有所不同的
> @todo 所以具体的结构是什么, 使用ctl_path 之类的东西管理

* **Static Sysctl Tables**
```c
static struct ctl_table root_table[] = {
	{
		.procname = "",
		.mode = S_IFDIR|S_IRUGO|S_IXUGO,
	},
	{ }
};

static struct ctl_table_root sysctl_table_root = {
	.default_set.dir.header = {
		{{.count = 1,
		  .nreg = 1,
		  .ctl_table = root_table }},
		.ctl_table_arg = root_table,
		.root = &sysctl_table_root,
		.set = &sysctl_table_root.default_set,
	},
};
```
> 真实情况是，在现在的设计中间，并没有预定如此之多的东西，而是依赖于register的动态加载

* **Registering Sysctls**

```c
static struct ctl_table ipc_root_table[] = { // 比如注册一个kernel 的例子
	{
		.procname	= "kernel",
		.mode		= 0555,
		.child		= ipc_kern_table,
	},
	{}
};

/**
 * register_sysctl_table - register a sysctl table hierarchy
 * @table: the top-level table structure
 *
 * Register a sysctl table hierarchy. @table should be a filled in ctl_table
 * array. A completely 0 filled entry terminates the table.
 *
 * See register_sysctl_paths for more details.
 */
struct ctl_table_header *register_sysctl_table(struct ctl_table *table)
{
	static const struct ctl_path null_path[] = { {} };

	return register_sysctl_paths(null_path, table);
}
EXPORT_SYMBOL(register_sysctl_table);
```

Registering a sysctl entry does not automatically create inode instances that connect the sysctl entries
with proc entries. Since most sysctls are never used via proc, this wastes memory. Instead, the connection
with proc files is created dynamically. Only the directory /proc/sys is created when procfs is initialized:
> sys 整个目录中间大多数的内容只有在access 的时候才会进行创建，只有sys 对应的inode 是在 init proc 的时候创建的

> 之所以sys pid 文件夹需要建立自己的inode, 其中最有可能的原因是，其中的内容都是动态创建的，所以查找的时候需要
> 目录自己特定的属性

* **/proc/sys File Operations**

```c
static const struct file_operations proc_sys_file_operations = {
	.open		= proc_sys_open,
	.poll		= proc_sys_poll,
	.read		= proc_sys_read,
	.write		= proc_sys_write,
	.llseek		= default_llseek,
};
```
> 正如书中所说的，read write open 很类似，都是 找到对应的inode(通过flip 参数), 检查权限，使用inode 自己定义的handler
```c
static ssize_t proc_sys_call_handler(struct file *filp, void __user *buf,
		size_t count, loff_t *ppos, int write)
{
	struct inode *inode = file_inode(filp);
	struct ctl_table_header *head = grab_header(inode);
	struct ctl_table *table = PROC_I(inode)->sysctl_entry;
	ssize_t error;
	size_t res;

	if (IS_ERR(head))
		return PTR_ERR(head);

	/*
	 * At this point we know that the sysctl was not unregistered
	 * and won't be until we finish.
	 */
	error = -EPERM;
	if (sysctl_perm(head, table, write ? MAY_WRITE : MAY_READ))
		goto out;

	/* if that can happen at all, it should be -EINVAL, not -EISDIR */
	error = -EINVAL;
	if (!table->proc_handler)
		goto out;

	/* careful: calling conventions are nasty here */
	res = count;
	error = table->proc_handler(table, write, buf, &res, ppos);
	if (!error)
		error = res;
out:
	sysctl_head_finish(head);

	return error;
}
```

## 10.2 Simple Filesystems
Full-featured filesystems are hard to write and require a considerable amount of effort until they reach
a usable, efficient, and correct state. This is reasonable if the filesystem is really supposed to store data
on disk. However, filesystems — especially virtual ones — serve many purposes that differ from storing
proper files on a block device. 
Such filesystems still run in the kernel, and their code is thus subjected
to the rigorous quality requirements imposed by the kernel developers. However, various standard
methods makes this aspect of life much easier. 
> 虽然，backed with disk 的文件系统非常的窒息，但是，virtual 的实现是按照一定的章法的。

1. A small filesystem library — libfs — contains nearly all
ingredients required to implement a filesystem. Developers only need to provide an interface to their
data, and they are done.
2. `seq_file` mechanism — are available to handle sequential files with little effort.
3. developers might want to just export a value or two into
userspace without messing with the existing filesystems like procfs. The kernel also provides a cure for
this need: The debugfs filesystem allows for implementing a bidirectional debugging interface with only
a few function calls.

#### 10.2.1 Sequential Files
The kprobe mechanism contains an interface to the aforementioned debug filesystem. A sequential file
presents all registered probes to userland. I consider the implementation to illustrate the idea of sequential files.

* **Writing Sequential File Handlers**
> @todo really clear, but not interesting sofar.

`seq_open` sets up the data structures required by the sequential file mechanism.
> 其实主要做的事情: filp 中间privat 指向 seq_file 的类型， 同时该 seq_file 的 seq_operations 指向 probe自定义的 seq_operations \


The most important element from a filesystem implementor’s point of view is the pointer op to an
instance of seq_operations. This connects the generic sequential file implementation with routines
providing file-specific contents. Four methods are required by the kernel and need to be implemented by
the file provider:


* **Connection with the Virtual Filesystem**
> @todo 

First of all, the function needs to obtain the `seq_file` instance from the VFS layer’s struct file. Recall
that `seq_opened` has established a connection via `private_data`.

#### 10.2.2 Writing Filesystems with Libfs
Libfs is a library that provides several very generic standard routines that can be used to create small
filesystems that serve one specific purpose. The routines are well suited for in-memory files without a
backing store.

Routines provided by
libfs are generally prefixed by `simple_`. Recall from Chapter 8 that the kernel also
provides several generic filesystem routines that are prefixed by `generic_`.
In contrast to libfs routines, these can also be used for full-blown filesystems.


If a virtual filesystem sets up a proper dentry tree, it suffices to install `simple_dir_operations` and
`simple_dir_inode_operations` as file or inode operations, respectively, for directories. The libfs functions then ensure that the information contained on the tree is exported to userland via the standard
system calls like getdents.

A filesystem also requires a superblock. Thankfully for lazy programmers, libfs provides the method
`simple_fill_super`, that can be used to fill in a given superblock:

Debugfs (discussed below) is one filesystem that employs libfs.

```c
struct dentry *debugfs_create_dir(const char *name, struct dentry *parent)
```
> 1. 在 debugfs_create_dir 中间，将上面定义的simple_dir_operations 对其inode 的i_op 和 i_iop 进行了赋值。
> 2. 因为仅仅对于目录采用libfs 的机制，其文件的ops 这里是各自采用各自的设计，所以没有讨论


#### 10.2.3 The Debug Filesystem
One particular filesystem using functions from libfs is the debug filesystem debugfs. It presents kernel
developers with a possibility of providing information to userland. The information is not supposed to
be compiled into production kernels.

* ***Example***

```c
static int __init debugfs_kprobe_init(void)
{
	struct dentry *dir, *file;
	unsigned int value = 1;

	dir = debugfs_create_dir("kprobes", NULL);
	if (!dir)
		return -ENOMEM;

	file = debugfs_create_file("list", 0400, dir, NULL,
				&debugfs_kprobes_operations);
	if (!file)
		goto error;

	file = debugfs_create_file("enabled", 0600, dir,
					&value, &fops_kp);
	if (!file)
		goto error;

	file = debugfs_create_file("blacklist", 0400, dir, NULL,
				&debugfs_kprobe_blacklist_ops);
	if (!file)
		goto error;

	return 0;

error:
	debugfs_remove(dir);
	return -ENOMEM;
}
```

> 得益于debugfs 的存在， kprobes 的实现简单的一逼
> kprobe create files under /sys/kernel/debug/

* ***Programming Interface***
Since the debugfs code is very clean, simple, and well documented, it is not necessary to add remarks
about the implementation. It suffices to discuss the programming interface. 

When kernel code is being debugged, the need to export and manipulate a single elementary value like
an int or a long often arises. Debugfs also provides several functions that create a new file that allows
for reading the value from userspace and passing a new value into the kernel. They all share a common
prototype.

#### 10.2.4 Pseudo Filesystems
Recall from Section 8.4.1 that the kernel supports **pseudo-filesystems that collect related inodes, but
cannot be mounted and are thus not visible in userland.** Libfs also provides an auxiliary function to
implement this specialized type of filesystem.


The kernel employs a pseudo-filesystem to keep track of all inodes that represent block devices.

```c
static struct dentry *bd_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	struct dentry *dent;
	dent = mount_pseudo(fs_type, "bdev:", &bdev_sops, NULL, BDEVFS_MAGIC);
	if (!IS_ERR(dent))
		dent->d_sb->s_iflags |= SB_I_CGROUPWB;
	return dent;
}

static struct file_system_type bd_type = {
	.name		= "bdev",
	.mount		= bd_mount,
	.kill_sb	= kill_anon_super,
};
```

For `bdev`, all
inodes that represent block devices are grouped together. The collection, however, will only be visible to
the kernel and not to userspace.

## 10.3 Sysfs
Sysfs is a filesystem for exporting kernel objects to userspace, providing the ability to not only observe
properties of kernel-internal data structures, but also to modify them. Especially important is the highly
hierarchical organization of the filesystem layout: The entries of sysfs originate from kernel objects
(kobjects) as introduced in Chapter 1, and the hierarchical order of these is directly reflected in the
directory layout of sysfs.5 Since all devices and buses of the system are organized via kobjects, sysfs
provides a representation of the system’s hardware topology.
> 所以 /sys 和 /proc/sys/ 的功能有什么区别 ? 不都是ioctl机制很麻烦，使用之后，cat echo 就可以修改kernel 参数吗 ?

As for many virtual filesystems, sysfs was initially based on ramfs; thus, the implementation uses many
techniques known from other places in the kernel. Note that sysfs is always compiled into the kernel
if it is configured to be active; generating it as a module is not possible. The canonical mount point for
sysfs is /sys.
> 忽然冒出来一个kobject

Finally, note that the connection between kobjects and sysfs is not automatically set up. Standalone
kobject instances are by default not integrated into sysfs. You need to call kobject_add to make an
object visible in the filesystem. If the kobject is a member of a kernel subsystem, the registration is
performed automatically, though.
#### 10.3.1 Overview
here
our discussion is restricted to a recap of the most essential points. In particular, it is important to
remember that
1. kobjects are included in a hierarchic organization; most important, they can have a parent and
can be included in a kset. This determines where the kobject appears in the sysfs hierarchy: If
a parent exists, a new entry in the directory of the parent is created. Otherwise, it is placed in the
directory of the kobject that belongs to the kset the object is contained in (if both of these possibilities fail, the entry for the kobject is located directly in the top level of the system hierarchy,
but this is obviously a rare case).
2. Every kobject is represented as a directory within sysfs. The files that appear in this directory
are the attributes of the object. The operations used to export and set attribute values are provided by the subsystem (class, driver, etc.) to which the kobject belongs.
3. Buses, devices, drivers, and classes are the main kernel objects using the kobject mechanism;
they thus account for nearly all entries of sysfs

#### 10.3.2 Data Structures
* **Directory Entries**

Directory entries are represented by struct sysfs_dirent as defined in <sysfs.h>. It is the main data
structure of sysfs; the whole implementation is centered around it. Each sysfs node is represented by a
single instance of `sysfs_dirent`. 
> 整个，sysfs_dirent 都消失，而且暂时不知道对应的替代者是谁

* **Attributes**

```c
struct attribute {
	const char		*name;
	umode_t			mode;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	bool			ignore_lockdep:1;
	struct lock_class_key	*key;
	struct lock_class_key	skey;
#endif
};
```
> OMG! 关键的module 成员没有了

```c
/**
 * struct attribute_group - data structure used to declare an attribute group.
 * @name:	Optional: Attribute group name
 *		If specified, the attribute group will be created in
 *		a new subdirectory with this name.
 * @is_visible:	Optional: Function to return permissions associated with an
 *		attribute of the group. Will be called repeatedly for each
 *		non-binary attribute in the group. Only read/write
 *		permissions as well as SYSFS_PREALLOC are accepted. Must
 *		return 0 if an attribute is not visible. The returned value
 *		will replace static permissions defined in struct attribute.
 * @is_bin_visible:
 *		Optional: Function to return permissions associated with a
 *		binary attribute of the group. Will be called repeatedly
 *		for each binary attribute in the group. Only read/write
 *		permissions as well as SYSFS_PREALLOC are accepted. Must
 *		return 0 if a binary attribute is not visible. The returned
 *		value will replace static permissions defined in
 *		struct bin_attribute.
 * @attrs:	Pointer to NULL terminated list of attributes.
 * @bin_attrs:	Pointer to NULL terminated list of binary attributes.
 *		Either attrs or bin_attrs or both must be provided.
 */
struct attribute_group {
	const char		*name;
	umode_t			(*is_visible)(struct kobject *,
					      struct attribute *, int);
	umode_t			(*is_bin_visible)(struct kobject *,
						  struct bin_attribute *, int);
	struct attribute	**attrs;
	struct bin_attribute	**bin_attrs;
};
```

Note, though, that it is customary that the `show` and `store` operations of the subsystem rely
on attribute-specific show and store methods that are internally connected with the attribute and that
differ on a per-attribute basis. The implementation details are left to the respective subsystem; sysfs is
unconcerned about this.
```c
struct sysfs_ops {
	ssize_t	(*show)(struct kobject *, struct attribute *, char *);
	ssize_t	(*store)(struct kobject *, struct attribute *, const char *, size_t);
};
```

```c
struct bin_attribute {
	struct attribute	attr;
	size_t			size;
	void			*private;
	ssize_t (*read)(struct file *, struct kobject *, struct bin_attribute *,
			char *, loff_t, size_t);
	ssize_t (*write)(struct file *, struct kobject *, struct bin_attribute *,
			 char *, loff_t, size_t);
	int (*mmap)(struct file *, struct kobject *, struct bin_attribute *attr,
		    struct vm_area_struct *vma);
};
```
> 一上来就讲解一堆架构体，而且还有一个关键的sysfs_dientry 不存在，现在感觉到很迷

> 紧接着，书上使用disk作为例子，介绍的 disk_attribute disk_sysfs_ops 和 disk_attr_show　似乎都是不存在的

```c
/* interface for exporting device attributes */
struct device_attribute {
	struct attribute	attr;
	ssize_t (*show)(struct device *dev, struct device_attribute *attr,
			char *buf);
	ssize_t (*store)(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
};
```
> 这几乎就是disk_attribute

```c
static DEVICE_ATTR(range, 0444, disk_range_show, NULL);
static DEVICE_ATTR(ext_range, 0444, disk_ext_range_show, NULL);
static DEVICE_ATTR(removable, 0444, disk_removable_show, NULL);
static DEVICE_ATTR(hidden, 0444, disk_hidden_show, NULL);
static DEVICE_ATTR(ro, 0444, disk_ro_show, NULL);
static DEVICE_ATTR(size, 0444, part_size_show, NULL);
static DEVICE_ATTR(alignment_offset, 0444, disk_alignment_offset_show, NULL);
static DEVICE_ATTR(discard_alignment, 0444, disk_discard_alignment_show, NULL);
static DEVICE_ATTR(capability, 0444, disk_capability_show, NULL);
static DEVICE_ATTR(stat, 0444, part_stat_show, NULL);
static DEVICE_ATTR(inflight, 0444, part_inflight_show, NULL);
static DEVICE_ATTR(badblocks, 0644, disk_badblocks_show, disk_badblocks_store);
#ifdef CONFIG_FAIL_MAKE_REQUEST
static struct device_attribute dev_attr_fail =
	__ATTR(make-it-fail, 0644, part_fail_show, part_fail_store);
#endif
#ifdef CONFIG_FAIL_IO_TIMEOUT
static struct device_attribute dev_attr_fail_timeout =
	__ATTR(io-timeout-fail, 0644, part_timeout_show, part_timeout_store);
#endif

static struct attribute *disk_attrs[] = {
	&dev_attr_range.attr,
	&dev_attr_ext_range.attr,
	&dev_attr_removable.attr,
	&dev_attr_hidden.attr,
	&dev_attr_ro.attr,
	&dev_attr_size.attr,
	&dev_attr_alignment_offset.attr,
	&dev_attr_discard_alignment.attr,
	&dev_attr_capability.attr,
	&dev_attr_stat.attr,
	&dev_attr_inflight.attr,
	&dev_attr_badblocks.attr,
#ifdef CONFIG_FAIL_MAKE_REQUEST
	&dev_attr_fail.attr,
#endif
#ifdef CONFIG_FAIL_IO_TIMEOUT
	&dev_attr_fail_timeout.attr,
#endif
	NULL
};
```
> 这是可能相关的内容

#### 10.3.3 Mounting the Filesystem

```c
static struct file_system_type sysfs_fs_type = {
	.name		= "sysfs",
	.mount		= sysfs_mount,
	.kill_sb	= sysfs_kill_sb,
	.fs_flags	= FS_USERNS_VISIBLE | FS_USERNS_MOUNT,
};


static struct dentry *sysfs_mount(struct file_system_type *fs_type,

struct dentry *kernfs_mount_ns(struct file_system_type *fs_type, int flags,
				struct kernfs_root *root, unsigned long magic,
				bool *new_sb_created, const void *ns)


static int kernfs_fill_super(struct super_block *sb, unsigned long magic)
    struct inode *kernfs_get_inode(struct super_block *sb, struct kernfs_node *kn)
        static void kernfs_init_inode(struct kernfs_node *kn, struct inode *inode) // 重点分析，不同情况，区分对待
```




```c
int __init sysfs_init(void)
{
	int err;

	sysfs_root = kernfs_create_root(NULL, KERNFS_ROOT_EXTRA_OPEN_PERM_CHECK,
					NULL);
	if (IS_ERR(sysfs_root))
		return PTR_ERR(sysfs_root);

	sysfs_root_kn = sysfs_root->kn;

	err = register_filesystem(&sysfs_fs_type);
	if (err) {
		kernfs_destroy_root(sysfs_root);
		return err;
	}

	return 0;
}
```
> 所以 sysfs_init 的作用是什么，和mount , register_filesystem 都是什么时间关系

#### 10.3.4 File and Directory Operations
> 真的，似乎关键的功能全部都转移到了kernfs 中间去了
> 这里常规的一套描述，两种operations 和其中的各种实现，实际上根本没有出现

#### 10.3.5 Populating Sysfs
Since sysfs is an interface to export data from the kernel, only the kernel itself can populate sysfs with file
and directory entries. This can be triggered from places all over the kernel, and, indeed, such operations
are ubiquitous within the whole tree, which renders it impossible to cover all appearances in detail. Thus
only the general methods used to connect sysfs with the internal data structures of the diverse subsystems
are demonstrated; the methods used for this purpose are quite similar everywhere.

> 我曹，有使用disk 作为例子，说过了，没有!

## 10.4 Summary

## 补充
**分析一下文件系统的启动过程**
首先在`init/main.c`中间的
https://jin-yang.github.io/post/kernel-bootstrap.html
```
 |-vfs_caches_init_early()
 |-vfs_caches_init()                         ← 根据参数计算可以作为缓存的页面数，并建立一个存放文件名称的slab缓存
 | |-kmem_cache_create()                     ← 创建slab缓存
 | |-dcache_init()                           ← 建立dentry和dentry_hashtable的缓存
 | |-inode_init()                            ← 建立inode和inode_hashtable的缓存
 | |-files_init()                            ← 建立filp的slab缓存，设置内核可打开的最大文件数
 | |-mnt_init()                              ← 完成sysfs和rootfs的注册和挂载
 |   |-kernfs_init()
 |   |-sysfs_init()                          ← 注册挂载sysfs
 |   | |-kmem_cache_create()                 ← 创建缓存
 |   | |-register_filesystem()
 |   |-kobject_create_and_add()              ← 创建fs目录
 |   |-init_rootfs()                         ← 注册rootfs文件系统
 |   |-init_mount_tree()                     ← 建立目录树，将init_task的命名空间与之联系起来
 |     |-vfs_kern_mount()                    ← 挂载已经注册的rootfs文件系统
 |     | |-alloc_vfsmnt()
 |     |-create_mnt_ns()                     ← 创建命名空间
 |     |-set_fs_pwd()                        ← 设置init的当前目录
 |     |-set_fs_root()                       ← 以及根目录
```

```c
void __init mnt_init(void)
{
	unsigned u;
	int err;

	mnt_cache = kmem_cache_create("mnt_cache", sizeof(struct mount),
			0, SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL);

	mount_hashtable = alloc_large_system_hash("Mount-cache",
				sizeof(struct hlist_head),
				mhash_entries, 19,
				0,
				&m_hash_shift, &m_hash_mask, 0, 0);
	mountpoint_hashtable = alloc_large_system_hash("Mountpoint-cache",
				sizeof(struct hlist_head),
				mphash_entries, 19,
				0,
				&mp_hash_shift, &mp_hash_mask, 0, 0);

	if (!mount_hashtable || !mountpoint_hashtable)
		panic("Failed to allocate mount hash table\n");

	for (u = 0; u <= m_hash_mask; u++)
		INIT_HLIST_HEAD(&mount_hashtable[u]);
	for (u = 0; u <= mp_hash_mask; u++)
		INIT_HLIST_HEAD(&mountpoint_hashtable[u]);

	kernfs_init();

	err = sysfs_init();
	if (err)
		printk(KERN_WARNING "%s: sysfs_init error: %d\n",
			__func__, err);
	fs_kobj = kobject_create_and_add("fs", NULL);
	if (!fs_kobj)
		printk(KERN_WARNING "%s: kobj create error\n", __func__);
	init_rootfs();
	init_mount_tree();
}
```

`mnt_init()`还调用过两个函数: `bdev_cache_init();`和`chrdev_init();`
发现block设备 是 依旧注册了文件系统，但是 char 设备没有 注册文件系统，而是简单的初始化了 `cdev_map`

> **TODO** 所以vfs_kern_mount 函数 是搞什么的

> **TODO** 所以什么东西是`rootfs`

```c
static struct file_system_type rootfs_fs_type = {
	.name		= "rootfs",
	.mount		= rootfs_mount,
	.kill_sb	= kill_litter_super,
};

int __init init_rootfs(void)
{
	int err = register_filesystem(&rootfs_fs_type);

	if (err)
		return err;

	if (IS_ENABLED(CONFIG_TMPFS) && !saved_root_name[0] &&
		(!root_fs_names || strstr(root_fs_names, "tmpfs"))) {
		err = shmem_init();
		is_tmpfs = true;
	} else {
		err = init_ramfs_fs();
	}

	if (err)
		unregister_filesystem(&rootfs_fs_type);

	return err;
}
```

mount_nodev

```c
struct dentry *mount_nodev(struct file_system_type *fs_type,
	int flags, void *data,
	int (*fill_super)(struct super_block *, void *, int))
{
	int error;
	struct super_block *s = sget(fs_type, NULL, set_anon_super, flags, NULL);

	if (IS_ERR(s))
		return ERR_CAST(s);

	error = fill_super(s, data, flags & MS_SILENT ? 1 : 0);
	if (error) {
		deactivate_locked_super(s);
		return ERR_PTR(error);
	}
	s->s_flags |= MS_ACTIVE;
	return dget(s->s_root);
}
EXPORT_SYMBOL(mount_nodev);
```

# 补充
1. [difference between /sys and /proc](https://superuser.com/questions/794198/directory-sys-in-linux)
