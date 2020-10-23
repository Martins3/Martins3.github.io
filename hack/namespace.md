## TODO
- [ ] fs/proc/namespace handles /proc/$pid/ns

## 待分析的资料
**[namespace with c code](https://windsock.io/uts-namespace/)**

The UTS namespace is used to isolate two specific elements of the system that relate to the uname system call

In short, the UTS namespace is about isolating hostnames.

So, how do you place processes into namespaces?
This is achieved with three different system calls: *clone*, *unshare* and *setns*.

* Man clone

       clone() creates a new process, in a manner similar to fork(2).

       This page describes both the glibc clone() wrapper function and the underlying system call on which it is based.  The main text describes the wrapper function; the differences for the raw system call are described toward the end of this page.

       Unlike fork(2), clone() allows the child process to share parts of its execution context with the calling process, such as the virtual address space, the table of file descriptors, and the table of signal handlers.  (Note that on this manual page, "calling process" normally corresponds to "parent process".  But see the description of CLONE_PARENT below.)

       One use of clone() is to implement threads: multiple flows of control in a program that run concurrently in a shared address space.

* Man unshare

       unshare() allows a process (or thread) to disassociate parts of its execution context that are currently being shared with other processes (or threads).  Part of the execution context, such as the mount namespace, is shared implicitly when a new process is created using fork(2) or vfork(2), while other parts, such as virtual memory, may be shared by explicit request when creating a process or thread using clone(2).

       The main use of unshare() is to allow a process to control its shared execution context without creating a new process.


* Man setns

        So, how do you place processes into namespaces? This is achieved with three different system calls: clone, unshare and setns.

The files are special symlinks bearing the namespace type (e.g. mnt), which point to inodes which represent unique namespaces, which the kernel reports as a conjugation of the namespace type and the inode number. Hence, if two different processes exist in the same namespace, their symlinks will point to the same inodes.

* https://en.wikipedia.org/wiki/Cgroups

* https://github.com/p8952/bocker 一百行实现 docker

## 问题
2. copy_namespace, 那么销毁一个 namespace 的时间点


## general
The namespace API consists of three system calls—clone(), unshare(), and setns()—and a number of /proc files. [^2]
```
ls -l /proc/$$/ns
```
两种 让当前 fs 不被销毁的方法 : 
- The /proc/PID/ns symbolic links also serve other purposes. If we open one of these files, then the namespace will continue to exist as long as the file descriptor remains open, even if all processes in the namespace terminate.
- The same effect can also be obtained by bind mounting one of the symbolic links to another location in the file system:

proc_ns_operation

```c
struct proc_ns_operations {
	const char *name;
	const char *real_ns_name;
	int type;
	struct ns_common *(*get)(struct task_struct *task);
	void (*put)(struct ns_common *ns);
	int (*install)(struct nsset *nsset, struct ns_common *ns);
	struct user_namespace *(*owner)(struct ns_common *ns);
	struct ns_common *(*get_parent)(struct ns_common *ns);
} __randomize_layout;
```



## mount


## UTS : hostname &&  domainname

https://unix.stackexchange.com/questions/183717/whats-a-uts-namespace
https://superuser.com/questions/59093/difference-between-host-name-and-domain-name


## pid
1. namespace 里面的 pid = 1 的那个如果挂掉了，那么剩下的 process 需要挂到哪里 ?
    - 全部 terminate [^3]
2. pid 的父子关系 在不同的 namespace 之间如何维持 ?
    - 在外层的 parent 挂掉之后，会将 children 放到内层的 init 中间

作用[^1]:
pid namespace isolate the process ID number space. 
In other words, processes in different PID namespaces can have the same PID. 
1. One of the main benefits of PID namespaces is that containers can be migrated between hosts while keeping the same process IDs for the processes inside the container. 
2. PID namespaces also allow each container to have its own init (PID 1), the "ancestor of all processes" that manages various system initialization tasks and reaps orphaned child processes when they terminate.

结构[^1]:
1. From the point of view of a particular PID namespace instance, a process has two PIDs: the PID inside the namespace, and the PID outside the namespace on the host system.
2. PID namespaces can be nested: a process will have one PID for each of the layers of the hierarchy starting from the PID namespace in which it resides through to the root PID namespace. 
3. A process can see (e.g., view via /proc/PID and send signals with kill()) only processes contained in its own PID namespace and the namespaces nested below that PID namespace.

unshare 和 setns 在处理 pid 不会把 caller 放到该 namespace 中间，原因是
这样就改变了 caller 的 pid，而很多程序是依赖于 pid 是 constant 的假设. [^3]

**TODO**:
1. task_struct 的 pid tgid thread_id 的。。。。。

https://stackoverflow.com/questions/26779416/what-is-the-relation-between-task-struct-and-pid-namespace

If you trace the steps needed to create a new process, you can find the relationship(s) between the `task_struct`, `struct nsproxyand`, `struct pid`.


## ipc


## net
能力:
provide isolation of the system resources associated with networking. Thus, each network namespace has its own network devices, IP addresses, IP routing tables, /proc/net directory, port numbers, and so on.[^1]

## user

## time

[^1]: https://lwn.net/Articles/531114/
[^2]: https://lwn.net/Articles/531381/
[^3]: https://lwn.net/Articles/532748/

# fs/namespace.c


## Question
1. 将 block device /dev/fb0 挂载到特定的目录，利用 /run/media/shen/Will 一样可以访问，这种是什么原因，是因为自动 mount 过吗 ?
    1. 应该是吧! 不然为什么制作启动盘的时候会出现 : 首先需要 unmount 的操作。
    2. 如果没有 mount 虽然可以访问到设备，但是是没有文件系统的，所以其实并不能做什么操作。

## Doc
#### Man mount(8)

All files accessible in a Unix system are arranged in one big tree, the file hierarchy, rooted at /.  These files can be spread out over several devices.  The mount command serves to attach the filesystem found on some device to the big file tree.  Conversely, the umount(8) command will detach it again.  The filesystem is used to control how data is stored on the device or provided in a virtual way by network or another services.

```
➜  c sudo mount a b
mount: /home/shen/c/b: /home/shen/c/a is not a block device.
```
> @todo 似乎最大的问题是 mknod 是什么时候调用的了

## core struct
1. fs_context
2. mount
3. mnt_namespace

```c
/*
 * Filesystem context for holding the parameters used in the creation or
 * reconfiguration of a superblock.
 *
 * Superblock creation fills in ->root whereas reconfiguration begins with this
 * already set.
 *
 * See Documentation/filesystems/mount_api.txt
 */
struct fs_context {
	const struct fs_context_operations *ops;
	struct mutex		uapi_mutex;	/* Userspace access mutex */
	struct file_system_type	*fs_type;
	void			*fs_private;	/* The filesystem's context */
	void			*sget_key;
	struct dentry		*root;		/* The root and superblock */ // 利用 get_tree 获取，即将放入的位置，也就是其实该文件系统的 root
	struct user_namespace	*user_ns;	/* The user namespace for this mount */
	struct net		*net_ns;	/* The network namespace for this mount */
	const struct cred	*cred;		/* The mounter's credentials */
	struct p_log		log;		/* Logging buffer */
	const char		*source;	/* The source name (eg. dev path) */ // todo 使用的节点
	void			*security;	/* Linux S&M options */
	void			*s_fs_info;	/* Proposed s_fs_info */
	unsigned int		sb_flags;	/* Proposed superblock flags (SB_*) */
	unsigned int		sb_flags_mask;	/* Superblock flags that were changed */
	unsigned int		s_iflags;	/* OR'd with sb->s_iflags */
	unsigned int		lsm_flags;	/* Information flags from the fs to the LSM */
	enum fs_context_purpose	purpose:8;
	enum fs_context_phase	phase:8;	/* The phase the context is in */
	bool			need_free:1;	/* Need to call ops->free() */
	bool			global:1;	/* Goes into &init_user_ns */
};

struct fs_context_operations {
	void (*free)(struct fs_context *fc);
	int (*dup)(struct fs_context *fc, struct fs_context *src_fc);
	int (*parse_param)(struct fs_context *fc, struct fs_parameter *param);
	int (*parse_monolithic)(struct fs_context *fc, void *data);
	int (*get_tree)(struct fs_context *fc);
	int (*reconfigure)(struct fs_context *fc);
};
```


```c
struct mnt_namespace {
	atomic_t		count;
	struct ns_common	ns;
	struct mount *	root;
	struct list_head	list; // 用于存在 vfsmount
	struct user_namespace	*user_ns;
	struct ucounts		*ucounts;
	u64			seq;	/* Sequence number to prevent loops */
	wait_queue_head_t poll;
	u64 event;
	unsigned int		mounts; /* # of mounts in the namespace */
	unsigned int		pending_mounts;
} __randomize_layout;
```

```c
struct vfsmount {
	struct dentry *mnt_root;	/* root of the mounted tree */
	struct super_block *mnt_sb;	/* pointer to superblock */
	int mnt_flags;
} __randomize_layout;
```

```c
struct mount {
	struct hlist_node mnt_hash;
	struct mount *mnt_parent;
	struct dentry *mnt_mountpoint;
	struct vfsmount mnt;
	union {
		struct rcu_head mnt_rcu;
		struct llist_node mnt_llist;
	};
#ifdef CONFIG_SMP
	struct mnt_pcp __percpu *mnt_pcp;
#else
	int mnt_count;
	int mnt_writers;
#endif
	struct list_head mnt_mounts;	/* list of children, anchored here */
	struct list_head mnt_child;	/* and going through their mnt_child */
	struct list_head mnt_instance;	/* mount instance on sb->s_mounts */
	const char *mnt_devname;	/* Name of device e.g. /dev/dsk/hda1 */
	struct list_head mnt_list; // 利用这一个变量将 vfsmount 挂载到 mount 上
	struct list_head mnt_expire;	/* link in fs-specific expiry list */
	struct list_head mnt_share;	/* circular list of shared mounts */
	struct list_head mnt_slave_list;/* list of slave mounts */
	struct list_head mnt_slave;	/* slave list entry */
	struct mount *mnt_master;	/* slave is on master->mnt_slave_list */
	struct mnt_namespace *mnt_ns;	/* containing namespace */
	struct mountpoint *mnt_mp;	/* where is it mounted */
	union {
		struct hlist_node mnt_mp_list;	/* list mounts with the same mountpoint */
		struct hlist_node mnt_umount;
	};
	struct list_head mnt_umounting; /* list entry for umount propagation */
#ifdef CONFIG_FSNOTIFY
	struct fsnotify_mark_connector __rcu *mnt_fsnotify_marks;
	__u32 mnt_fsnotify_mask;
#endif
	int mnt_id;			/* mount identifier */
	int mnt_group_id;		/* peer group identifier */
	int mnt_expiry_mark;		/* true if marked for expiry */
	struct hlist_head mnt_pins;
	struct hlist_head mnt_stuck_children;
} __randomize_layout;
```

## kern_mount
1. 路线上和 do_new_mount 非常相似

```c
static int __init init_pipe_fs(void)
{
	int err = register_filesystem(&pipe_fs_type); // 创建

	if (!err) {
		pipe_mnt = kern_mount(&pipe_fs_type);　// mount，todo 但是 mount 到哪里，似乎不会 mount 到任何位置 !
		if (IS_ERR(pipe_mnt)) {
			err = PTR_ERR(pipe_mnt);
			unregister_filesystem(&pipe_fs_type);
		}
	}
	return err;
}
```

2. 使用 pipe 机制作为例子

```c
struct vfsmount *kern_mount(struct file_system_type *type)
{
	struct vfsmount *mnt;
	mnt = vfs_kern_mount(type, SB_KERNMOUNT, type->name, NULL);
	if (!IS_ERR(mnt)) {
		/*
		 * it is a longterm mount, don't release mnt until
		 * we unmount before file sys is unregistered
		*/
		real_mount(mnt)->mnt_ns = MNT_NS_INTERNAL;
	}
	return mnt;
}

static struct file_system_type pipe_fs_type = {
	.name		= "pipefs",
	.init_fs_context = pipefs_init_fs_context, // fs_context_for_mount => alloc_fs_context 中间分配并且初始化 kern，此函数将被调用，xxx 进入到 pipe.c
	.kill_sb	= kill_anon_super,
};
```

## vfsmount API
1. alloc_vfsmnt : 创建 mount 并且对于其中的成员进行基本的初始化。
2. free_vfsmnt : 
3. lookup_mnt
```c
static void free_vfsmnt(struct mount *mnt) // 为什么和 alloc_vfsmnt 不对称啊 !
{
	kfree_const(mnt->mnt_devname);
#ifdef CONFIG_SMP
	free_percpu(mnt->mnt_pcp);
#endif
	kmem_cache_free(mnt_cache, mnt);
}
```

```c
/*
 * find the first mount at @dentry on vfsmount @mnt.
 * call under rcu_read_lock()
 */
struct mount *__lookup_mnt(struct vfsmount *mnt, struct dentry *dentry) // todo 分析实现 todo 谁要使用这个程序 ?
{
	struct hlist_head *head = m_hash(mnt, dentry);
	struct mount *p;

	hlist_for_each_entry_rcu(p, head, mnt_hash)
		if (&p->mnt_parent->mnt == mnt && p->mnt_mountpoint == dentry)
			return p;
	return NULL;
}
```

## do_mount
1. do_mount : 所有的情况
2. do_new_mount : 最常用的情况


## do_new_mount
1. fs_context_for_mount : 初始化 context 
2. parse_monolithic_mount_data && vfs_parse_fs_string : @todo 应该完成 fs 特殊内容相关的参数解析吧 ! Man mount(2/8) 应该有帮助吧 !
3. vfs_get_tree : 获取 mount root，链接两个系统的
4. do_new_mount_fc : 处理一些检查工作(和 ns 相关的)，最后利用 graft_tree
    1. graft_tree 是 attach_recursive_mnt 的封装

```c
struct fs_context *fs_context_for_mount(struct file_system_type *fs_type,
					unsigned int sb_flags)
{
	return alloc_fs_context(fs_type, NULL, sb_flags, 0, // 还可能调用 fc->fs_type->init_fs_context 进一步进行初始化，如果没有注册，那么调用 legacy_init_fs_context，其中初始化了 各种 operation
					FS_CONTEXT_FOR_MOUNT);
}
```

```c
/**
 * vfs_get_tree - Get the mountable root
 * @fc: The superblock configuration context.
 *
 * The filesystem is invoked to get or create a superblock which can then later
 * be used for mounting.  The filesystem places a pointer to the root to be
 * used for mounting in @fc->root.
 */
int vfs_get_tree(struct fs_context *fc)
{
	struct super_block *sb;
	int error;

	if (fc->root)
		return -EBUSY;

	/* Get the mountable root in fc->root, with a ref on the root and a ref
	 * on the superblock.
	 */
	error = fc->ops->get_tree(fc); // 最终的任务还是交给 file_system_type::mount 中间，其中搜索或者创建了 super_block 以及初始化 fc 中间 root 
	if (error < 0)
		return error;

	if (!fc->root) {
		pr_err("Filesystem %s get_tree() didn't set fc->root\n",
		       fc->fs_type->name);
		/* We don't know what the locking state of the superblock is -
		 * if there is a superblock.
		 */
		BUG();
	}

	sb = fc->root->d_sb; // root dentry 对应的 super_block
	WARN_ON(!sb->s_bdi);

	/*
	 * Write barrier is for super_cache_count(). We place it before setting
	 * SB_BORN as the data dependency between the two functions is the
	 * superblock structure contents that we just set up, not the SB_BORN
	 * flag.
	 */
	smp_wmb();
	sb->s_flags |= SB_BORN;

	error = security_sb_set_mnt_opts(sb, fc->security, 0, NULL);
	if (unlikely(error)) {
		fc_drop_locked(fc);
		return error;
	}

	/*
	 * filesystems should never set s_maxbytes larger than MAX_LFS_FILESIZE
	 * but s_maxbytes was an unsigned long long for many releases. Throw
	 * this warning for a little while to try and catch filesystems that
	 * violate this rule.
	 */
	WARN((sb->s_maxbytes < 0), "%s set sb->s_maxbytes to "
		"negative value (%lld)\n", fc->fs_type->name, sb->s_maxbytes);

	return 0;
}
EXPORT_SYMBOL(vfs_get_tree);


/*
 * Get a mountable root with the legacy mount command.
 */
static int legacy_get_tree(struct fs_context *fc)
{
	struct legacy_fs_context *ctx = fc->fs_private;
	struct super_block *sb;
	struct dentry *root;

	root = fc->fs_type->mount(fc->fs_type, fc->sb_flags,
				      fc->source, ctx->legacy_data);
	if (IS_ERR(root))
		return PTR_ERR(root);

	sb = root->d_sb;
	BUG_ON(!sb);

	fc->root = root;
	return 0;
}

static struct dentry *ext2_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
return mount_bdev(fs_type, flags, dev_name, data, ext2_fill_super); // 返回的应该是 被挂载的，进入 super.c 中间分
}
```

```c
/*
 * Create a new mount using a superblock configuration and request it
 * be added to the namespace tree.
 */
static int do_new_mount_fc(struct fs_context *fc, struct path *mountpoint,
			   unsigned int mnt_flags)
{
	struct vfsmount *mnt;
	struct super_block *sb = fc->root->d_sb;
	int error;

	error = security_sb_kern_mount(sb); // 安全机制插入的 hook
	if (!error && mount_too_revealing(sb, &mnt_flags))
		error = -EPERM;

	if (unlikely(error)) {
		fc_drop_locked(fc);
		return error;
	}

	up_write(&sb->s_umount);

	mnt = vfs_create_mount(fc); // 创建 mount 及其对应的 vfsmount
	if (IS_ERR(mnt))
		return PTR_ERR(mnt);

	mnt_warn_timestamp_expiry(mountpoint, mnt);

	error = do_add_mount(real_mount(mnt), mountpoint, mnt_flags);
	if (error < 0)
		mntput(mnt);
	return error;
}
```

```c
static int graft_tree(struct mount *mnt, struct mount *p, struct mountpoint *mp)
{
	if (mnt->mnt.mnt_sb->s_flags & SB_NOUSER)
		return -EINVAL;

	if (d_is_dir(mp->m_dentry) !=
	      d_is_dir(mnt->mnt.mnt_root))
		return -ENOTDIR;

	return attach_recursive_mnt(mnt, p, mp, false);
}
```

## attach_recursive_mnt : todo u1s1 有点复杂


# ipc/namespace.c

## todo
1. kernel/utsname.c 同样的只有简单的 200 多行的样子，感觉完全就是对称的
    1. @todo  hostname domain , 为什么不和网络的放在一起呀 ?

2. 

## 瞎几把复制

```c
const struct proc_ns_operations ipcns_operations = {
	.name		= "ipc",
	.type		= CLONE_NEWIPC,
	.get		= ipcns_get,
	.put		= ipcns_put,
	.install	= ipcns_install,
	.owner		= ipcns_owner,
};
```

> 这是要注册到 proc 中间吗 ?




```c
struct ipc_namespace {
	refcount_t	count;
	struct ipc_ids	ids[3];

	int		sem_ctls[4];
	int		used_sems;

	unsigned int	msg_ctlmax;
	unsigned int	msg_ctlmnb;
	unsigned int	msg_ctlmni;
	atomic_t	msg_bytes;
	atomic_t	msg_hdrs;

	size_t		shm_ctlmax;
	size_t		shm_ctlall;
	unsigned long	shm_tot;
	int		shm_ctlmni;
	/*
	 * Defines whether IPC_RMID is forced for _all_ shm segments regardless
	 * of shmctl()
	 */
	int		shm_rmid_forced;

	struct notifier_block ipcns_nb;

	/* The kern_mount of the mqueuefs sb.  We take a ref on it */
	struct vfsmount	*mq_mnt;

	/* # queues in this ns, protected by mq_lock */
	unsigned int    mq_queues_count;

	/* next fields are set through sysctl */
	unsigned int    mq_queues_max;   /* initialized to DFLT_QUEUESMAX */
	unsigned int    mq_msg_max;      /* initialized to DFLT_MSGMAX */
	unsigned int    mq_msgsize_max;  /* initialized to DFLT_MSGSIZEMAX */
	unsigned int    mq_msg_default;
	unsigned int    mq_msgsize_default;

	/* user_ns which owns the ipc ns */
	struct user_namespace *user_ns;
	struct ucounts *ucounts;

	struct ns_common ns;
} __randomize_layout;
```


## CLONE_NEWIPC 的跟踪

```c
struct ipc_namespace *copy_ipcs(unsigned long flags,
	struct user_namespace *user_ns, struct ipc_namespace *ns)
{
	if (!(flags & CLONE_NEWIPC))
		return get_ipc_ns(ns);
	return create_ipc_ns(user_ns, ns); // todo 应该不是拷贝，而是创建出来层级关系吧 ?
}
```
> nsproxy.c:create_new_namespaces 唯一调用 !

```c

/*
 * Create new nsproxy and all of its the associated namespaces.
 * Return the newly created nsproxy.  Do not attach this to the task,
 * leave it to the caller to do proper locking and attach it to task.
 */
static struct nsproxy *create_new_namespaces(unsigned long flags,
	struct task_struct *tsk, struct user_namespace *user_ns,
	struct fs_struct *new_fs)
  ...
	new_nsp->ipc_ns = copy_ipcs(flags, user_ns, tsk->nsproxy->ipc_ns); // 将 tsk->nsproxy 的 ipc_ns 拿过来拷贝，根据 flags 自适应的处理
	if (IS_ERR(new_nsp->ipc_ns)) {
		err = PTR_ERR(new_nsp->ipc_ns);
		goto out_ipc;
	}
  ...
```
1. ref : copy_namespaces setns unshare (如 Man 中间所说)
2. `_do_fork` -> copy_process -> copy_namespaces

