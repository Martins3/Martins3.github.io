# Professional Linux Kernel Architecture : The Virtual Filesystem
To support various native filesystems and, at the same time, to allow access to files of other operating systems, the Linux kernel includes a layer between user processes (or the standard library)
and the filesystem implementation. This layer is known as the Virtual File System, or VFS for short

> ucore 中间有没有办法添加sfs之外的文件系统的。
> 可以的，可以尝试添加一个支持多级目录的

## 8.1 Filesystem Types
1. Disk-based filesystems
2. Virtual filesystems 
3. Network filesystems

## 8.2 The Common File Model
Not every filesystem supports all abstraction types in VFS. Device files cannot be stored in filesystems
originating from other systems (i.e., FAT) because the latter do not cater to objects of this kind.


Naturally, the structure of the virtual filesystem is not a product of fantasy but is based on structures used
to describe classical filesystems. The VFS layer was also organized to clearly resemble the Ext2 filesystem.
This makes life more difficult for filesystems based on totally different concepts (e.g., the Reiser filesystem
or XFS) but delivers speed gains when working with Ext2fs because practically no time is lost converting
between Ext2 and VFS structures
> VFS 和 Ext2 相似在什么地方?

the inode does not contain one important item of information — the filename.

#### 8.2.1 Inodes
The directory is represented by an inode whose data segment does not contain normal data but
only the root directory entries. These entries may stand for files or other directories. Each entry consists
of two elements.
1. The number of the inode in which the data of the next entry are located
2. The name of the file or directory

> inode 中间不保存文件名的原因和软硬链接有关系的。 

#### 8.2.2 Links
A separate inode is used for each symbolic link.
The data segment of the inode contains a character string that gives the name of the link target.

When a hard link is created, a directory entry is generated whose associated inode uses an existing inode number.

#### 8.2.3 Programming Interface
The interface between user processes and the kernel implementation of the VFS is formed, as usual, by
**system calls**, most of which involve the manipulation of files, directories, and filesystems in general.

The assigned file descriptor
numbers start at 3. Recall that numbering does not start at 0 because the first three file descriptors are
reserved for all processes, although no explicit instructions need be given. 0 represents standard input, 1
standard output, and 2 standard error

Since the introduction of multiple namespaces and containers, multiple file descriptors with
the same numerical value can coexist in the kernel. A unique representation is provided by a special data
structure (struct file), discussed below.

> so, how kernel access the file, in ucore, how kernel thread access the file
> what is the difference ?

#### 8.2.4 Files as a Universal Interface

## 8.3 Structure of the VFS
#### 8.3.1 Structural Overview
The VFS consists of two components — **files** and **filesystems** — that need to be managed and abstracted

No fixed functions are used to abstract access to the underlying filesystems. Instead, function pointers
are required. These are held in two structures that group together related functions.
1. `Inode Operations` — Create links, rename files, generate new file entries in a directory, and delete files.
2. `File Operations` — Act on the data contents of a file. They include obvious operations such as read and write, but also operations such as setting file pointers and creating memory mappings.

As discussed briefly in Chapter 2, the task
structure includes an element in which all opened files are held (via a roundabout route). This element is
an array that is accessed using the file descriptor as an index. The objects it contains are not only linked
with the inode of the corresponding file, but also have a pointer to an element of the dentry cache used to
speed lookup operations

`struct task`中间包含 `struct files_struct *files;`, 

```c
/*
 * Open file table structure
 */
struct files_struct {
  /*
   * read mostly part
   */
	atomic_t count;
	bool resize_in_progress;
	wait_queue_head_t resize_wait;

	struct fdtable __rcu *fdt;
	struct fdtable fdtab;
  /*
   * written part on a separate cache line in SMP
   */
	spinlock_t file_lock ____cacheline_aligned_in_smp;
	int next_fd;
	unsigned long close_on_exec_init[1];
	unsigned long open_fds_init[1];
	unsigned long full_fds_bits_init[1];
	struct file __rcu * fd_array[NR_OPEN_DEFAULT];
};
```

```c
struct file {
	union {
		struct llist_node	fu_llist;
		struct rcu_head 	fu_rcuhead;
	} f_u;
	struct path		f_path;
	struct inode		*f_inode;	/* cached value */
	const struct file_operations	*f_op;

	/*
	 * Protects f_ep_links, f_flags.
	 * Must not be taken from IRQ context.
	 */
	spinlock_t		f_lock;
	atomic_long_t		f_count;
	unsigned int 		f_flags;
	fmode_t			f_mode;
	struct mutex		f_pos_lock;
	loff_t			f_pos;
	struct fown_struct	f_owner;
	const struct cred	*f_cred;
	struct file_ra_state	f_ra;

	u64			f_version;
#ifdef CONFIG_SECURITY
	void			*f_security;
#endif
	/* needed for tty driver, and maybe others */
	void			*private_data;

#ifdef CONFIG_EPOLL
	/* Used by fs/eventpoll.c to link all the hooks to this file */
	struct list_head	f_ep_links;
	struct list_head	f_tfile_llink;
#endif /* #ifdef CONFIG_EPOLL */
	struct address_space	*f_mapping;
} __attribute__((aligned(4)));	/* lest something weird decides that 2 is OK */
```
> ext4 中间是如何处理其中的file_operations 的，将inode 中间的file_operations 直接赋值。

The individual filesystem implementations are also able to store their own data (that is not manipulated
by the VFS layer) in the VFS inode
> ucore 中间的处理，使用union 将所有struct 放置到一起
> kernle 在inode 中间添加 `void			*i_private; /* fs or device private pointer */` , 具体使用需要分析。


the superblock contains function pointers to read, write, and manipulate inodes.
> 既然superblock 中间已经含有这些函数，那么inode 中间的 `file_operations`的作用是什么


The kernel also creates a list of the superblock instances of all active filesystems. I use the term **active**
instead of mounted because, *in certain circumstances, it is possible to use a single superblock for several
mount points*

Whereas each filesystem appears just once in file_system_type, there may be
several instances of a superblock for the same filesystem type in the list of
superblock instances because several filesystems of the same type can be stored on
various block devices or partitions
> 所以**partition**到底是什么东西?
> 相同的filesystem 在不同的devices 或者 partitions, 是什么情景?

An important element of the superblock structure is a list with all modified inodes of the relevant filesystem (the kernel refers to these rather disrespectfully as dirty inodes). Files and directories that have been
modified are easily identified by reference to this list so that they can be written back to the storage
medium. 
> `struct super_block` 掌管dirty 节点的控制。不禁让人想到，放到磁盘上的super_block 和 内存上的 super_block 是一个东西吗?

> holly shit 在ucore中间有没有 `struct super_operations` 这一个操作。

#### 8.3.2 Inodes
> 在ucore中间，一共含有三种inode : disk_inde sfs_inode and inode(the vfs inode)
> 那么在kernel 中间的这三个inode 是分别是如何实现的

Before explaining the meanings of the individual structural members, it is worth remembering that the
inode structure examined here was designed for processing in memory and therefore includes a few
elements that are not present in the stored inodes. 

`i_rdev` is needed when the inode represents a device file. 
Note that i_rdev is only a number, not a data structure! The information contained in this number is, however, sufficient to find everything interesting about the device.

> 从来就没有理解设备的major 和 minor number 的作用
> 是如何通过访问它们实现 访问设备的


Since an inode cannot represent more than one type of
device at a time, it is safe to keep `i_pipe`, `i_bdev`, and `i_cdev` in a union
```c
struct inode{
  ...
	union {
		struct pipe_inode_info	*i_pipe;
		struct block_device	*i_bdev;
		struct cdev		*i_cdev;
		char			*i_link;
	};
  ...
}
```

`i_devices` is also connected to
device file handling: It allows a block or character device to keep a list of inodes that represent a device
file over which it can be accessed. 
```c
struct inode{
  ...
	struct list_head	i_devices;
  ...
}
```
The inode structure has two pointers (i_op and i_fop) to arrays that implement the above abstraction.
One relates to the inode-specific operations, and the other provides file operations.

```c
struct inode{
  ...
	const struct inode_operations	*i_op;
	const struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
  ...
}
```
Here, suffice it to say that file operations deal with manipulating the data contained in a file, while inode operations are
responsible for managing the structural operations (e.g., deleting a file) and metadata associated with
files (e.g., attributes)

```c
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
	int (*iterate) (struct file *, struct dir_context *);
	unsigned int (*poll) (struct file *, struct poll_table_struct *);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t id);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, loff_t, loff_t, int datasync);
	int (*aio_fsync) (struct kiocb *, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	int (*flock) (struct file *, int, struct file_lock *);
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
	ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
	int (*setlease)(struct file *, long, struct file_lock **, void **);
	long (*fallocate)(struct file *file, int mode, loff_t offset,
			  loff_t len);
	void (*show_fdinfo)(struct seq_file *m, struct file *f);
#ifndef CONFIG_MMU
	unsigned (*mmap_capabilities)(struct file *);
#endif
};
```
The `xattr` functions create, read, and delete extended attributes not supported in the
classicUnix model. They are used, for example, in the implementation of access control
lists (ACLs)

`follow_link` follows a symbolic link by finding the inode of the target file. Because symbolic links may go beyond filesystem boundaries, the implementation of the routine is
usually very short, and work is quickly delegated to generic VFS routines that complete
the task.

`fallocate` is used to pre-allocate space for a file, which can in some circumstances lead to performance benefits. However, only very recent filesystems (like Reiserfs or Ext4) support this
operation.

`truncate_range` allows for truncating a range of blocks (i.e., for punching holes into files), but
this operation is currently only supported by the shared memory filesystem

`unlink` is invoked to delete a file. However, as described above, the delete operation is not
carried out if the hard link reference counter indicates that the inode is in use by more than
one file.

```c
struct inode_operations {
	struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
	const char * (*follow_link) (struct dentry *, void **);
	int (*permission) (struct inode *, int);
	struct posix_acl * (*get_acl)(struct inode *, int);

	int (*readlink) (struct dentry *, char __user *,int);
	void (*put_link) (struct inode *, void *);

	int (*create) (struct inode *,struct dentry *, umode_t, bool);
	int (*link) (struct dentry *,struct inode *,struct dentry *);
	int (*unlink) (struct inode *,struct dentry *);
	int (*symlink) (struct inode *,struct dentry *,const char *);
	int (*mkdir) (struct inode *,struct dentry *,umode_t);
	int (*rmdir) (struct inode *,struct dentry *);
	int (*mknod) (struct inode *,struct dentry *,umode_t,dev_t);
	int (*rename) (struct inode *, struct dentry *,
			struct inode *, struct dentry *);
	int (*rename2) (struct inode *, struct dentry *,
			struct inode *, struct dentry *, unsigned int);
	int (*setattr) (struct dentry *, struct iattr *);
	int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
	int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
	ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
	ssize_t (*listxattr) (struct dentry *, char *, size_t);
	int (*removexattr) (struct dentry *, const char *);
	int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start,
		      u64 len);
	int (*update_time)(struct inode *, struct timespec *, int);
	int (*atomic_open)(struct inode *, struct dentry *,
			   struct file *, unsigned open_flag,
			   umode_t create_mode, int *opened);
	int (*tmpfile) (struct inode *, struct dentry *, umode_t);
	int (*set_acl)(struct inode *, struct posix_acl *, int);
} ____cacheline_aligned;
```


Each inode has a `i_list` list head element to store the inode on a list. Depending on the state of the inode, three main cases are possible:
1. The inode exists in memory but is `not linked to any file` and is not in active use.
2. The inode structure is in memory and is being used for one or more tasks, usually to represent a file. The value of both counters (`i_count` and `i_nlink`) must be greater than 0. The
file contents and the inode metadata are identical to the information on the underlying block
device; that is, the inode has not been changed since the last synchronization with the storage medium.
3. The inode is in active use. Its data contents have been changed and therefore differ from the
contents on the storage medium. Inodes in this state are described as `dirt`

> 在什么情况下，一个inode 会被加载到内存中间，但是没有和文件链接起来。

In `fs/inode.c`, the kernel defines two global variables for use as list heads
1. `inode_unused` for valid but no longer active inodes (the first category in the above list), and
2. `inode_in_use` for all used but unchanged inodes (the second category).
3. The dirty inodes (third category) are held in a superblock-specific list

> 并没有找到这几个变量

Each inode appears not only in the state-specific list but also in a hash table to support quick access by
reference to the **inode number and superblock — this combination is unique throughout the system**. The
hash table is an array that can be accessed with the help of the global variable inode_hashtable (also
from fs/inode.c). 
```
static struct hlist_head *inode_hashtable __read_mostly;
```
> 通过hash 加快访问速度，但是很显然，既不知道为什么需要加快访问速度，也不知道为什么可以通过hash 实现
> 如果不使用hash, 那么访问的过程是什么样子的?

It combines the inode number and the address of the superblock object into
a unique number that is guaranteed to reside within the reserved index range of the hash table. Collisions are resolved as usual by means of an overflow list. The inode element `i_hash` is used to manage the
overflow elements.
```c
struct super_block{
...
	struct hlist_node	i_hash;
...
}
```


In addition to the hash table, inodes are also kept on a per-superblock list headed by `super_block->s_ inodes.i_sb_list` is used as the list element.
```c
struct super_block{
...
	struct list_head	s_inodes;	/* all inodes */
...
}
/**
 * inode_sb_list_add - add inode to the superblock list of inodes
 * @inode: inode to add
 */
void inode_sb_list_add(struct inode *inode)
{
	spin_lock(&inode->i_sb->s_inode_list_lock);
	list_add(&inode->i_sb_list, &inode->i_sb->s_inodes);
	spin_unlock(&inode->i_sb->s_inode_list_lock);
}
EXPORT_SYMBOL_GPL(inode_sb_list_add);
```
> 实际上，显然s_inodes没有i_sb_list 这一个成员，该成员在inode中间，但是inode 持有i_sb_list 的原因是什么?
> 忽然发现list_head的一个小缺点，那就是，对于每一个容器，都是需要创建 struct list_head 出来

#### 8.3.3 Process-Specific Information
`<sched.h>`

```c
struct task_struct {
  ...
  /* file system info */
  int link_count, total_link_count;
  ...
  /* filesystem information */
  struct fs_struct *fs;
  /* open file information */
  struct files_struct *files;
  /* namespaces */
  struct nsproxy *nsproxy;
  ...
}
```

The integer elements `link_count` and `total_link_count` are used to prevent endless loops when looking
up circularly chained links as I will demonstrate in Section 8.4.2
> ?? what lookup and what to see ?
> Section 8.4.2 讲过了吗?

The filesystem-related data of a process are stored in `fs`.

`close_on_exec_init` and `open_fds_init` are bitmaps. `close_on_exec` contains a set bit for all file
descriptors that will be closed on exec. `open_fds_init` is the initial set of file descriptors. `struct embedded_fd_set` is just a simple unsigned long encapsulated in a special structure.
> 完全没有描述清楚这两个东西的内容

> 接下来的内容，只是采用何种机制实现了 超过最初的一个proc 使用的文件的数目。

#### 8.3.4 File Operations
> 再次分析struct file_operations
> 只记录有意思的几个
`aio_read` is used for asynchronous read operations
> asynchronous, the eternal heartrending hurt

`poll` is used with the `poll` and `select` system calls needed to implement synchronous I/O multiplexing. What does this mean? The read function is used when a process is waiting for input
data from a file object. If no data are available (this may be the case when the process is reading from an external interface), the call blocks until data become available. This could result
in unacceptable situations if there are no more data to be read and the read function blocks
forever.
The select system call, which is also based on the poll method, comes to the rescue. It sets a
time-out to abort a read operation after a certain period during which no new data arrive. This
ensures that program flow is resumed if no further data are available

`fsync` is used by the `fsync` and `fdatasync` system calls to initiate synchronization of file data in
memory with that on a storage medium.

`fasync` is needed to enable and disable signal-controlled input and output (processes are notified
of changes in the file object by means of signals).

> and some more function not used by me.

Every `task_struct` instance therefore includes a pointer to a further structure of type `fs_struct`.
```c
struct fs_struct {
	int users;
	spinlock_t lock;
	seqcount_t seq;
	int umask;
	int in_exec;
	struct path root, pwd;
};
```
> 和书上几乎完全不一样了

A VFS namespace is the collection of all mounted filesystems that make up the directory tree of a container.
> namespace ?? container ?? what's the relation and how to implement it.
> https://wvi.cz/diyC/namespaces/ 有可能有作用，但是似乎首先需要阅读他自己的代码才可以



Recall that `struct task_struct` contains a member element, `nsproxy`, which is responsible for namespace handling.

```c
struct nsproxy {
	atomic_t count;
	struct uts_namespace *uts_ns;
	struct ipc_namespace *ipc_ns;
	struct mnt_namespace *mnt_ns;
	struct pid_namespace *pid_ns_for_children;
	struct net 	     *net_ns;
};
```
```c
struct mnt_namespace {
	atomic_t		count;
	struct ns_common	ns;
	struct mount *	root;
	struct list_head	list;
	struct user_namespace	*user_ns;
	u64			seq;	/* Sequence number to prevent loops */
	wait_queue_head_t poll;
	u64 event;
	unsigned int		mounts; /* # of mounts in the namespace */
	unsigned int		pending_mounts;
};
```


#### 8.3.5 Directory Entry Cache
Even if the device data are already in the `page cache` (see Chapter 16), it is nonsensical to repeat the full lookup
operation each time.
> 什么是page cache

```c
struct dentry {
	/* RCU lookup touched fields */
	unsigned int d_flags;		/* protected by d_lock */
	seqcount_t d_seq;		/* per dentry seqlock */
	struct hlist_bl_node d_hash;	/* lookup hash list */
	struct dentry *d_parent;	/* parent directory */
	struct qstr d_name;
	struct inode *d_inode;		/* Where the name belongs to - NULL is
					 * negative */
	unsigned char d_iname[DNAME_INLINE_LEN];	/* small names */

	/* Ref lookup also touches following */
	struct lockref d_lockref;	/* per-dentry lock and refcount */
	const struct dentry_operations *d_op;
	struct super_block *d_sb;	/* The root of the dentry tree */
	unsigned long d_time;		/* used by d_revalidate */
	void *d_fsdata;			/* fs-specific data */

	struct list_head d_lru;		/* LRU list */
	struct list_head d_child;	/* child of parent list */
	struct list_head d_subdirs;	/* our children */
	/*
	 * d_alias and d_rcu can share memory
	 */
	union {
		struct hlist_node d_alias;	/* inode alias list */
	 	struct rcu_head d_rcu;
	} d_u;
};
```
The dentry tree can be split into several subtrees because each superblock
structure contains a pointer to the dentry element of the directory on which the filesystem is
mounted.

`d_alias` links the dentry objects of identical files. This situation arises when links are used to
make the file available under two different names. This list is linked from the corresponding
inode by using its `i_dentry` element as a list head. The individual dentry objects are linked by
`d_alias`
> link should has nothing to do with directory, so what's the role `d_alias` play in link

All active instances of dentry in memory are held in a hash table implemented using the global variable
dentry_hashtable from fs/dcache.c

How is the cache organized ? It comprises the following two components to organize dentry objects in memory:
1. A hash table (`dentry_hashtable`) containing all dentry objects.
2. An LRU (least recently used) list in which objects no longer used are granted a last period of grace before they are removed from memory

> all dentry objects, all the directory in filesystem， or all the directory accessed, now that the hash table contains all the dentry objects, what's the meaing of LRU which write the dentry to medium
> all dentry objects : All active instances of dentry in memory
> still very stupid and confusing 

The following auxiliary functions require a pointer to struct dentry as parameter.

```c
static inline struct dentry *dget(struct dentry *dentry)
{
	if (dentry)
		lockref_get(&dentry->d_lockref);
	return dentry;
}

void d_drop(struct dentry *dentry){
	spin_lock(&dentry->d_lock);
	__d_drop(dentry);
	spin_unlock(&dentry->d_lock);
}
EXPORT_SYMBOL(d_drop);

d_delete // 没有分析
dput // 没有分析
```


Some helper functions are more complicated, so it’s best to inspect their prototypes
`<dcache.h>`
```c
extern void d_instantiate(struct dentry *, struct inode *);
struct dentry * d_alloc(struct dentry *, const struct qstr *);
struct dentry * d_alloc_anon(struct inode *);
struct dentry * d_splice_alias(struct inode *, struct dentry *);
static inline void d_add(struct dentry *entry, struct inode *inode);
struct dentry * d_lookup(struct dentry *, struct qstr *);
```


## 8.4 Working with VFS Objects
## 8.4.1 Filesystem Operations
A further important aspect must also be taken into consideration. Filesystems are implemented in
modular form in the kernel; this means that they can be compiled into the kernel as modules
(see Chapter 7) or can be totally ignored by compiling the kernel without support for a particular
filesystem version — given the fact that there are about 50 filesystems, it would make little sense to keep
the code for all of them in the kernel.

When a filesystem is registered with the kernel, it makes no difference whether the code is compiled as
a module or is permanently compiled into the kernel.

`register_filesystem` from fs/super.c is used to register a filesystem with the kernel.

The structure used to describe a filesystem is defined as follows
```c
struct file_system_type {
	const char *name;
	int fs_flags;
#define FS_REQUIRES_DEV		1 
#define FS_BINARY_MOUNTDATA	2
#define FS_HAS_SUBTYPE		4
#define FS_USERNS_MOUNT		8	/* Can be mounted by userns root */
#define FS_USERNS_DEV_MOUNT	16 /* A userns mount does not imply MNT_NODEV */
#define FS_USERNS_VISIBLE	32	/* FS must already be visible */
#define FS_RENAME_DOES_D_MOVE	32768	/* FS will handle d_move() during rename() internally. */
	struct dentry *(*mount) (struct file_system_type *, int,
		       const char *, void *);
	void (*kill_sb) (struct super_block *);
	struct module *owner;
	struct file_system_type * next;
	struct hlist_head fs_supers;

	struct lock_class_key s_lock_key;
	struct lock_class_key s_umount_key;
	struct lock_class_key s_vfs_rename_key;
	struct lock_class_key s_writers_key[SB_FREEZE_LEVELS];

	struct lock_class_key i_lock_key;
	struct lock_class_key i_mutex_key;
	struct lock_class_key i_mutex_dir_key;
};
```
> why should define `FS_XXX` in the struct.
> what's `struct lock_class_key` and the purpose of it.

```c
struct vfsmount {
	struct dentry *mnt_root;	/* root of the mounted tree */
	struct super_block *mnt_sb;	/* pointer to superblock */
	int mnt_flags;
};
```
> it seems that `struct vfsmount` changed greatly since 2.6
> what fucks more is that we don't know who substitute the orignal function.

Various filesystem-independent flags can be set in nmt_flags. The following constants list all possible
flags:
```c
<mount.h>
#define MNT_NOSUID 0x01
#define MNT_NODEV 0x02
#define MNT_NOEXEC 0x04
#define MNT_NOATIME 0x08
#define MNT_NODIRATIME 0x10
#define MNT_RELATIME 0x20
#define MNT_SHRINKABLE 0x100
#define MNT_SHARED 0x1000 /* if the vfsmount is a shared mount */
#define MNT_UNBINDABLE 0x2000 /* if the vfsmount is a unbindable mount */
#define MNT_PNODE_MASK 0x3000 /* propagation flag mask *
```
The first block is concerned with classic properties like disallowing setuid execution or the existence of
device files on the mount, or how access time handling is managed. MNT_NODEV is set if the mounted
filesystem is virtual, that is, does not have a physical backing device. MNT_SHRINKABLE is a specialty of
NFS and AFS that is used to mark submounts. Mounts with this mark are allowed to be automatically
removed.


Let's analyze `struct super_block`'s member now.
> 众多list_head 类型变量都消失了
>
> 包括: s_dirty s_files
> 
> s_inodes到底控制的inode 是做什么的，s_dirty 取消之后，在哪里控制dirty


1. `s_dev` and `s_bdev` specify the block device on which the data of the underlying filesystem reside.
The former uses the internal kernel number, whereas the latter is a pointer to the `block_device`
structure in memory that is used to define device operations and capabilities in more detail
(Chapter 6 takes a closer look at both types).
The `s_dev` entry is always supplied with a number (even for virtual filesystems that
do not require block devices). In contrast, the `s_bdev` pointer may also contain a null
pointer.
> 在ucore 中间，dev是由sfs_dev负责。并且，fs 包含sfs_fs，而fs 又包含super_block 

2. `s_type` points to the `file_system_type` instance (discussed in Section 8.4.1), which holds general type information on the filesystem.
> ucore里面的superblock 的作用和s_type 的作用更加类似，用于描述文件系统的静态信息。

3. `s_root` associates the superblock with the dentry entry of the global root directory as seen by the filesystem
> ucore 中间没有任何位置和这一个功能对应。而且dentry 这一个功能被仅仅在 sfs_disk_entry在文件系统层次实现了


```c
struct super_operations {
   	struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *);

   	void (*dirty_inode) (struct inode *, int flags);
	int (*write_inode) (struct inode *, struct writeback_control *wbc);
	int (*drop_inode) (struct inode *);
	void (*evict_inode) (struct inode *);
	void (*put_super) (struct super_block *);
	int (*sync_fs)(struct super_block *sb, int wait);
	int (*freeze_super) (struct super_block *);
	int (*freeze_fs) (struct super_block *);
	int (*thaw_super) (struct super_block *);
	int (*unfreeze_fs) (struct super_block *);
	int (*statfs) (struct dentry *, struct kstatfs *);
	int (*remount_fs) (struct super_block *, int *, char *);
	void (*umount_begin) (struct super_block *);

	int (*show_options)(struct seq_file *, struct dentry *);
	int (*show_devname)(struct seq_file *, struct dentry *);
	int (*show_path)(struct seq_file *, struct dentry *);
	int (*show_stats)(struct seq_file *, struct dentry *);
#ifdef CONFIG_QUOTA
	ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
	ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
	struct dquot **(*get_dquots)(struct inode *);
#endif
	int (*bdev_try_to_free_page)(struct super_block*, struct page*, gfp_t);
	long (*nr_cached_objects)(struct super_block *,
				  struct shrink_control *);
	long (*free_cached_objects)(struct super_block *,
				    struct shrink_control *);
};
```
The operations in the structure do not change the contents of inodes but control the way in which inode
data are obtained and returned to the underlying implementations
The structure also includes methods for carrying out relatively extensive tasks such as **remounting filesystems**.


| Operations       | Functions                                                                                                        |
|------------------|------------------------------------------------------------------------------------------------------------------|
| inode_operations | managing the structural operations (e.g., deleting a file) and metadata associated with files (e.g., attributes) |
| super_operations | control the way in which inode data are obtained and returned to the underlying implementations                  |
| file_operations  | file operations deal with manipulating the data contained in a file                                              |

> 终于到了mount的位置
The point of entry for the `mount` system call is the `sys_mount` function defined in fs/namespace.c.


sys_mount 是入口函数:
```c
SYSCALL_DEFINE5(mount, char __user *, dev_name, char __user *, dir_name,
		char __user *, type, unsigned long, flags, void __user *, data)
{
	int ret;
	char *kernel_type;
	char *kernel_dev;
	unsigned long data_page;

  // 三个复制装载选项
	kernel_type = copy_mount_string(type);
	ret = PTR_ERR(kernel_type);
	if (IS_ERR(kernel_type))
		goto out_type;

	kernel_dev = copy_mount_string(dev_name);
	ret = PTR_ERR(kernel_dev);
	if (IS_ERR(kernel_dev))
		goto out_dev;

	ret = copy_mount_options(data, &data_page);
	if (ret < 0)
		goto out_data;

  // 复制结束，开始调用
	ret = do_mount(kernel_dev, dir_name, kernel_type, flags,
		(void *) data_page);

	free_page(data_page);
out_data:
	kfree(kernel_dev);
out_dev:
	kfree(kernel_type);
out_type:
	return ret;
}
```

`do_mount`的主要的作用,调用解析flags,然后调用一个正确的函数。
```c
long do_mount(const char *dev_name, const char __user *dir_name,
		const char *type_page, unsigned long flags, void *data_page){
```

`do_new_mount` 是当flags没有特殊选项的默认选择，用于处理普通装载操作。
```c
static int do_new_mount(struct path *path, const char *fstype, int flags,
			int mnt_flags, const char *name, void *data){
  // 首先似乎做了 file_system_type 的检查
	mnt = vfs_kern_mount(type, flags, name, data);
	err = do_add_mount(real_mount(mnt), path, mnt_flags);
```
> **好吧，还是他喵的看不懂啊, namespace 之类的 太变态了**

#### 8.4.2 File Operations
To permit universal access to files regardless of the filesystem used, the VFS provides interface functions for file processing in the form of various system calls as already noted above. 

**Lookup Inodes**
> 查询是一个复杂的东西，ucore 使用lookup.c来实现


The `nameidata` structure is used to pass parameters to the lookup function and to hold the lookup result.
```c
struct nameidata {
	struct path	path;
	struct qstr	last;
	struct path	root;
	struct inode	*inode; /* path.dentry.d_inode */
	unsigned int	flags;
	unsigned	seq, m_seq;
	int		last_type;
	unsigned	depth;
	int		total_link_count;
	struct saved {
		struct path link;
		void *cookie;
		const char *name;
		struct inode *inode;
		unsigned seq;
	} *stack, internal[EMBEDDED_LEVELS];
	struct filename	*name;
	struct nameidata *saved;
	unsigned	root_seq;
	int		dfd;
};
```

`filename_lookup`将会被kern以及user的查询函数分别调用。此函数处理了flags解析，然后调用`path_lookupat`
> 有必要分析一下kern 和 user 分别如何处理kern
```
static int filename_lookup(int dfd, struct filename *name, unsigned flags,
			   struct path *path, struct path *root) {
```

`path_lookupat`调用各种walk函数。
```c
static int path_lookupat(struct nameidata *nd, unsigned flags, struct path *path) {
...
	while (!(error = link_path_walk(s, nd)) &&
		(error = do_last(nd, file, op, &opened)) > 0) {
		nd->flags &= ~(LOOKUP_OPEN|LOOKUP_CREATE|LOOKUP_EXCL);
		s = trailing_symlink(nd);
		if (IS_ERR(s)) {
			error = PTR_ERR(s);
			break;
		}
	}
....
```

```c
/*
 * Name resolution.
 * This is the basic name resolution function, turning a pathname into
 * the final dentry. We expect 'base' to be positive and a directory.
 *
 * Returns 0 and nd will have valid dentry and mnt on success.
 * Returns error and drops reference to input namei data on failure.
 */
static int link_path_walk(const char *name, struct nameidata *nd) {
```

> 和ucore 不同的是，查询根本几乎没有`inode->i_op->some_dev_specific`函数。



* **Opening Files**
```c
SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, umode_t, mode)
{
	if (force_o_largefile())
		flags |= O_LARGEFILE;

	return do_sys_open(AT_FDCWD, filename, flags, mode);
}
```

```c
long do_sys_open(int dfd, const char __user *filename, int flags, umode_t mode)
{
	struct open_flags op;
	int fd = build_open_flags(flags, mode, &op);
	struct filename *tmp;

	if (fd)
		return fd;

	tmp = getname(filename);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	fd = get_unused_fd_flags(flags);
	if (fd >= 0) {
		struct file *f = do_filp_open(dfd, tmp, &op);
		if (IS_ERR(f)) {
			put_unused_fd(fd);
			fd = PTR_ERR(f);
		} else {
			fsnotify_open(f);
			fd_install(fd, f);
		}
	}
	putname(tmp);
	return fd;
}
```

```c
struct file *do_filp_open(int dfd, struct filename *pathname,
		const struct open_flags *op)
{
	struct nameidata nd;
	int flags = op->lookup_flags;
	struct file *filp;

	set_nameidata(&nd, dfd, pathname);
	filp = path_openat(&nd, op, flags | LOOKUP_RCU);
	if (unlikely(filp == ERR_PTR(-ECHILD)))
		filp = path_openat(&nd, op, flags);
	if (unlikely(filp == ERR_PTR(-ESTALE)))
		filp = path_openat(&nd, op, flags | LOOKUP_REVAL);
	restore_nameidata();
	return filp;
}
```
> 打开文件函数实现很简单，调用lookup，返回file

**Reading and Writing**
By reference to the file descriptor number, the kernel (using the `fget_light` function from
`fs/file_table.c`) is able to find the file instance associated with the task structure of the process.
> 终于找到了 file 和 `file descriptor number` 的联系了

```c
struct file *fget(unsigned int fd) {
	return __fget(fd, FMODE_PATH);
}
EXPORT_SYMBOL(fget);
```

经过了`fdtable.c`的文件中间，经过多层跳转（估计目的是为了实现其中如何支持更加多的文件和锁),
最后通过fdtable 中间获取file

> **之所以理解不了**super_block相关的内容，其实还是因为没有理解ucore中间的处理


```c
SYSCALL_DEFINE3(read, unsigned int, fd, char __user *, buf, size_t, count)
{
	struct fd f = fdget_pos(fd);
	ssize_t ret = -EBADF;

	if (f.file) {
		loff_t pos = file_pos_read(f.file);
		ret = vfs_read(f.file, buf, count, &pos);
		if (ret >= 0)
			file_pos_write(f.file, pos);
		fdput_pos(f);
	}
	return ret;
}
```

```c
ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	ssize_t ret;

	if (!(file->f_mode & FMODE_READ))
		return -EBADF;
	if (!(file->f_mode & FMODE_CAN_READ))
		return -EINVAL;
	if (unlikely(!access_ok(VERIFY_WRITE, buf, count)))
		return -EFAULT;

	ret = rw_verify_area(READ, file, pos, count);
	if (ret >= 0) {
		count = ret;
		ret = __vfs_read(file, buf, count, pos);
		if (ret > 0) {
			fsnotify_access(file);
			add_rchar(current, ret);
		}
		inc_syscr(current);
	}

	return ret;
}
```

> 终于，在这一个位置进入到虚拟化阶段。
```c
ssize_t __vfs_read(struct file *file, char __user *buf, size_t count,
		   loff_t *pos)
{
	if (file->f_op->read)
		return file->f_op->read(file, buf, count, pos);
	else if (file->f_op->read_iter)
		return new_sync_read(file, buf, count, pos);
	else
		return -EINVAL;
}
EXPORT_SYMBOL(__vfs_read);
```

## 8.5 Standard Functions
> 终于 找到了同步和异步io的位置。

Most filesystems include the `do_sync_read` and `do_sync_write` standard routines in the read and
write pointers of their `file_operations` instance

#### 8.5.1 Generic Read Routine
@todo

* **Asynchronous Reading**

* **Reading from Mappings**


#### 8.5.2 The fault Mechanism

#### 8.5.3 Permission-Checking

## 8.6 Summary
