# namespace
- namespace.c 居然是在 fs 下，这个文件很长，包含了多个文件系统。

namespace 可以让 sysfs 和 proc fs 的样子不一样，但是之前分析 sysfs 的时候居然没有这种感觉

还是老办法，写 c demo 测试 syscall ，然后

## TODO
- [ ] fs/proc/namespace handles /proc/$pid/ns

## 待分析的资料
- [namespace with c code](https://windsock.io/uts-namespace/)
  - https://man7.org/tlpi/code/online/dist/namespaces/simple_init.c.html
  - https://lwn.net/Articles/531114/


The UTS namespace is used to isolate two specific elements of the system that relate to the uname system call

In short, the UTS namespace is about isolating hostnames.

So, how do you place processes into namespaces?
This is achieved with three different system calls: *clone*, *unshare* and *setns*.

* Man clone

```txt
       clone() creates a new process, in a manner similar to fork(2).

       This page describes both the glibc clone() wrapper function and the underlying system call on which it is based.  The main text describes the wrapper function; the differences for the raw system call are described toward the end of this page.

       Unlike fork(2), clone() allows the child process to share parts of its execution context with the calling process, such as the virtual address space, the table of file descriptors, and the table of signal handlers.  (Note that on this manual page, "calling process" normally corresponds to "parent process".  But see the description of CLONE_PARENT below.)

       One use of clone() is to implement threads: multiple flows of control in a program that run concurrently in a shared address space.
```

* Man unshare

```txt
       unshare() allows a process (or thread) to disassociate parts of its execution context that are currently being shared with other processes (or threads).  Part of the execution context, such as the mount namespace, is shared implicitly when a new process is created using fork(2) or vfork(2), while other parts, such as virtual memory, may be shared by explicit request when creating a process or thread using clone(2).

       The main use of unshare() is to allow a process to control its shared execution context without creating a new process.
```


* Man setns

```txt
        So, how do you place processes into namespaces? This is achieved with three different system calls: clone, unshare and setns.
```

The files are special symlinks bearing the namespace type (e.g. mnt), which point to inodes which represent unique namespaces, which the kernel reports as a conjugation of the namespace type and the inode number. Hence, if two different processes exist in the same namespace, their symlinks will point to the same inodes.

* https://en.wikipedia.org/wiki/Cgroups

* https://github.com/p8952/bocker 一百行实现 docker

## 问题
2. copy_namespace, 那么销毁一个 namespace 的时间点


## general
The namespace API consists of three system calls—clone(), unshare(), and setns()—and a number of /proc files. [^2]
```plain
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

## 这个项目看看
https://news.ycombinator.com/item?id=37025841

## /proc /sys 如何被 namespace 影响

## 在物理机上真的遇到了一个 bug

- https://github.com/raspberrypi/linux/issues/6616

关联到这里: https://lkml.org/lkml/2025/1/21/281

在我们的机器上可以观察到的:
```txt
[ 4687.665845] ------------[ cut here ]------------
[ 4687.665848] WARNING: CPU: 18 PID: 3224 at fs/namespace.c:1245 cleanup_mnt+0x130/0x150
[ 4687.665935] CPU: 18 PID: 3224 Comm: dockerd Tainted: P           O       6.6.72 #1-NixOS
[ 4687.665936] Hardware name: LENOVO 82WM/INVALID, BIOS LPCN39WW 04/28/2023
[ 4687.665937] RIP: 0010:cleanup_mnt+0x130/0x150
[ 4687.665938] Code: 2c 01 00 00 85 c0 75 16 e8 9d fb ff ff eb 8a c7 87 2c 01 00 00 00 00 00 00 e9 6a ff ff ff c7 87 2c 01 00 00 00 00 00 00 eb de <0f> 0b 48 83 bd 30 01 00 00 00 0f 84 e9 fe ff ff 48 89 ef e8 08 16
[ 4687.665939] RSP: 0018:ffffc90003a9bec8 EFLAGS: 00010286
[ 4687.665940] RAX: 00000000fffffffc RBX: ffff88810d320bb0 RCX: 0000000000000000
[ 4687.665941] RDX: 0000000000000000 RSI: 0000000000000000 RDI: 0000000000000000
[ 4687.665941] RBP: ffff888106f348c0 R08: 0000000000000000 R09: 0000000000000000
[ 4687.665942] R10: 0000000000000000 R11: 0000000000000000 R12: ffff88810d320924
[ 4687.665942] R13: 0000000000000000 R14: 0000000000000000 R15: 0000000000000000
[ 4687.665942] FS:  00007f6479fda6c0(0000) GS:ffff88881d700000(0000) knlGS:0000000000000000
[ 4687.665943] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[ 4687.665943] CR2: 0000563bb574c0a8 CR3: 000000017c3a0000 CR4: 0000000000f50ee0
[ 4687.665944] PKRU: 55555554
[ 4687.665944] Call Trace:
[ 4687.665945]  <TASK>
[ 4687.665946]  ? cleanup_mnt+0x130/0x150
[ 4687.665947]  ? __warn+0x81/0x130
[ 4687.665951]  ? cleanup_mnt+0x130/0x150
[ 4687.665951]  ? report_bug+0x182/0x1b0
[ 4687.665955]  ? handle_bug+0x60/0xa0
[ 4687.665956]  ? exc_invalid_op+0x17/0x80
[ 4687.665957]  ? asm_exc_invalid_op+0x1a/0x20
[ 4687.665962]  ? cleanup_mnt+0x130/0x150
[ 4687.665963]  ? cleanup_mnt+0x13/0x150
[ 4687.665964]  task_work_run+0x5a/0x90
[ 4687.665966]  exit_to_user_mode_prepare+0x22f/0x240
[ 4687.665968]  syscall_exit_to_user_mode+0x1a/0x50
[ 4687.665970]  do_syscall_64+0x45/0x90
[ 4687.665971]  entry_SYSCALL_64_after_hwframe+0x78/0xe2
[ 4687.665972] RIP: 0033:0x557e60096b2e
[ 4687.665999] Code: 24 28 44 8b 44 24 2c e9 70 ff ff ff cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc 49 89 f2 48 89 fa 48 89 ce 48 89 df 0f 05 <48> 3d 01 f0 ff ff 76 15 48 f7 d8 48 89 c1 48 c7 c0 ff ff ff ff 48
[ 4687.666000] RSP: 002b:000000c00064e000 EFLAGS: 00000212 ORIG_RAX: 00000000000000a6
[ 4687.666001] RAX: 0000000000000000 RBX: 000000c0009d03f0 RCX: 0000557e60096b2e
[ 4687.666001] RDX: 0000000000000000 RSI: 0000000000000002 RDI: 000000c0009d03f0
[ 4687.666002] RBP: 000000c00064e040 R08: 0000000000000000 R09: 0000000000000000
[ 4687.666002] R10: 0000000000000000 R11: 0000000000000212 R12: 000000c0009d03f0
[ 4687.666002] R13: 0000000000000000 R14: 000000c0003088c0 R15: 003fffffffe3ff3f
[ 4687.666003]  </TASK>
[ 4687.666004] ---[ end trace 0000000000000000 ]---
```

# kernel/nsproxy.c
1. 和 process 关联紧密
2. 在 fork clone unshare 以及 exit 中间调用
3. 当 cgroup 的 namespace 的含义是什么 ?

## TODO
1. CLONE_NEWUSER 的 ref

## 问题
1. user_namespace 的实现位置在哪里 ?
2. parent 到底可不可以看到我的存在 ? child 层次创建了 msg queue ，那么 parent 可以感知吗 ?


## 学习目标
1. 感觉 namespace 的作用就是实现一个资源标签的重新映射
2. 为什么又要搞出一堆文件出来 在 /proc/1/ns/ 下面呀 ?
3. 和 global resource 的关系是什么 ?
4. 和 control group 的关系是什么 : namespace 控制的都是数值 pid user id port number 等等东西，这些资源感觉都是无穷分配的，或者说不是资源，只是试图创建出来一个干净的感觉!
但是 control group 的感觉完全不同，就是对于 io cpu memory 等资源的控制。所以，ns 想要实现的效果是隔离，我这里搞的事情，在另一个 ns 看不出区别的


## core struct

```c
struct ns_common {
	atomic_long_t stashed;
	const struct proc_ns_operations *ops;
	unsigned int inum;
};
```

```c
/*
 * A structure to contain pointers to all per-process
 * namespaces - fs (mount), uts, network, sysvipc, etc.
 *
 * The pid namespace is an exception -- it's accessed using
 * task_active_pid_ns.  The pid namespace here is the
 * namespace that children will use.
 *
 * 'count' is the number of tasks holding a reference.
 * The count for each namespace, then, will be the number
 * of nsproxies pointing to it, not the number of tasks.
 *
 * The nsproxy is shared by tasks which share all namespaces.
 * As soon as a single namespace is cloned or unshared, the
 * nsproxy is copied.
 */
struct nsproxy {
	atomic_t count;
	struct uts_namespace *uts_ns;
	struct ipc_namespace *ipc_ns;
	struct mnt_namespace *mnt_ns;
	struct pid_namespace *pid_ns_for_children;
	struct net 	     *net_ns;
	struct cgroup_namespace *cgroup_ns;
};
```


```c
struct proc_ns_operations {
	const char *name;
	const char *real_ns_name;
	int type;
	struct ns_common *(*get)(struct task_struct *task);
	void (*put)(struct ns_common *ns);
	int (*install)(struct nsproxy *nsproxy, struct ns_common *ns);
	struct user_namespace *(*owner)(struct ns_common *ns);
	struct ns_common *(*get_parent)(struct ns_common *ns);
} __randomize_layout;

extern const struct proc_ns_operations netns_operations;
extern const struct proc_ns_operations utsns_operations;
extern const struct proc_ns_operations ipcns_operations;
extern const struct proc_ns_operations pidns_operations;
extern const struct proc_ns_operations pidns_for_children_operations;
extern const struct proc_ns_operations userns_operations;
extern const struct proc_ns_operations mntns_operations;
extern const struct proc_ns_operations cgroupns_operations;
// 这大概就是所有的 proc_ns_operations 了吧!
```

5. 如何创建出来的虚拟空间 ?

## ref && doc

Man setns(2)

- Man ipc_namespace(7)

> IPC namespaces isolate certain IPC resources, namely, System V IPC objects (see sysvipc(7)) and (since Linux 2.6.30) POSIX message queues (see mq_overview(7)).  The common characteristic of these IPC mechanisms is that IPC objects are identified by mechanisms other than filesystem pathnames.

> Each IPC namespace has its own set of System V IPC identifiers and its own POSIX message queue filesystem.  Objects created in an IPC namespace are visible to all other processes that are members of that namespace, but are not visible to processes in other IPC namespaces.

IPC 需要在该空间中间使用 identifiers 来确定，当切换到一个新的空间之后，原来的 identifiers 就消失了。

- Man pid_namespace(7)

> PID namespaces isolate the process ID number space, meaning that processes in different PID namespaces can have the same PID.  PID namespaces allow containers to provide functionality such as suspending/resuming the set of processes in the container and migrating the container to a new host while the processes inside the container maintain the same PIDs.

> If the "init" process of a PID namespace terminates, the kernel terminates all of the processes in the namespace via a SIGKILL signal.

- Man user_namespace(7)

太他妈长了



## include/linux/nsproxy.h
```c
struct mnt_namespace;
struct uts_namespace;
struct ipc_namespace;
struct pid_namespace;
struct cgroup_namespace;
// 为什么没有 net_ns，user


/*
 * A structure to contain pointers to all per-process
 * namespaces - fs (mount), uts, network, sysvipc, etc.
 *
 * The pid namespace is an exception -- it's accessed using
 * task_active_pid_ns.  The pid namespace here is the
 * namespace that children will use.
 *
 * 'count' is the number of tasks holding a reference.
 * The count for each namespace, then, will be the number
 * of nsproxies pointing to it, not the number of tasks.
 *
 * The nsproxy is shared by tasks which share all namespaces.
 * As soon as a single namespace is cloned or unshared, the
 * nsproxy is copied.
 */
struct nsproxy {
	atomic_t count;
	struct uts_namespace *uts_ns;
	struct ipc_namespace *ipc_ns;
	struct mnt_namespace *mnt_ns;
	struct pid_namespace *pid_ns_for_children;
	struct net 	     *net_ns;
	struct cgroup_namespace *cgroup_ns;
  // todo 为什么没有 user 的 : Man namespace(7) 可以找到 7 个
};
extern struct nsproxy init_nsproxy;

/*
 * the namespaces access rules are:
 *
 *  1. only current task is allowed to change tsk->nsproxy pointer or
 *     any pointer on the nsproxy itself.  Current must hold the task_lock
 *     when changing tsk->nsproxy.
 *
 *  2. when accessing (i.e. reading) current task's namespaces - no
 *     precautions should be taken - just dereference the pointers
 *
 *  3. the access to other task namespaces is performed like this
 *     task_lock(task);
 *     nsproxy = task->nsproxy;
 *     if (nsproxy != NULL) {
 *             / *
 *               * work with the namespaces here
 *               * e.g. get the reference on one of them
 *               * /
 *     } / *
 *         * NULL task->nsproxy means that this task is
 *         * almost dead (zombie)
 *         * /
 *     task_unlock(task);
 *
 */

int copy_namespaces(unsigned long flags, struct task_struct *tsk);
void exit_task_namespaces(struct task_struct *tsk);
void switch_task_namespaces(struct task_struct *tsk, struct nsproxy *new);
void free_nsproxy(struct nsproxy *ns);
int unshare_nsproxy_namespaces(unsigned long, struct nsproxy **,
	struct cred *, struct fs_struct *);
int __init nsproxy_cache_init(void);

static inline void put_nsproxy(struct nsproxy *ns)
{
	if (atomic_dec_and_test(&ns->count)) {
		free_nsproxy(ns);
	}
}

static inline void get_nsproxy(struct nsproxy *ns)
{
	atomic_inc(&ns->count);
}
```

## create_new_namespaces

```c
/*
 * Create new nsproxy and all of its the associated namespaces.
 * Return the newly created nsproxy.  Do not attach this to the task,
 * leave it to the caller to do proper locking and attach it to task.
 */
static struct nsproxy *create_new_namespaces(unsigned long flags,
	struct task_struct *tsk, struct user_namespace *user_ns,
	struct fs_struct *new_fs)
```

## copy_namespaces
```c
/*
 * called from clone.  This now handles copy for nsproxy and all
 * namespaces therein.
 */
int copy_namespaces(unsigned long flags, struct task_struct *tsk)
```

```c
void switch_task_namespaces(struct task_struct *p, struct nsproxy *new)
{
	struct nsproxy *ns;

	might_sleep();

	task_lock(p);
	ns = p->nsproxy;
	p->nsproxy = new;
	task_unlock(p);

	if (ns && atomic_dec_and_test(&ns->count))
		free_nsproxy(ns);
}

void exit_task_namespaces(struct task_struct *p)
{
	switch_task_namespaces(p, NULL);
}
```

## 各种 nsproxy 成员 `*_namespace` 来观察一下
1. 以 uts_namespace 为例，存在几个基本要素，其作用是什么 ?
2. parent 为什么不是必须的 ?

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

```c
struct pid_namespace {
	struct kref kref;
	struct idr idr;  // 这就是用于分配 pid 的吧 ?
	struct rcu_head rcu;
	unsigned int pid_allocated;
	struct task_struct *child_reaper;
	struct kmem_cache *pid_cachep;
	unsigned int level;
	struct pid_namespace *parent;
#ifdef CONFIG_PROC_FS
	struct vfsmount *proc_mnt;
	struct dentry *proc_self;
	struct dentry *proc_thread_self;
#endif
#ifdef CONFIG_BSD_PROCESS_ACCT
	struct fs_pin *bacct;
#endif
	struct user_namespace *user_ns;
	struct ucounts *ucounts;
	struct work_struct proc_work;
	kgid_t pid_gid;
	int hide_pid;
	int reboot;	/* group exit code if this pidns was rebooted */
	struct ns_common ns;
} __randomize_layout;
```

```c
struct uts_namespace {
	struct kref kref;
	struct new_utsname name; // 不需要进行分配id 之类的，@todo 谁来设置 ?
	struct user_namespace *user_ns;
	struct ucounts *ucounts;
	struct ns_common ns;
} __randomize_layout;
// 这几乎就是最简单的内容了
```

```c
struct net {
// 1. 复杂，但是必备要素(uts_namespace 中间的)都在
// 2. net 的 namespace 似乎进一步划分为更小的部分了
```


```c
struct mnt_namespace {
	atomic_t		count;
	struct ns_common	ns;
	struct mount *	root; // todo 这就是实现的关键
	struct list_head	list;
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
struct cgroup_namespace {
	refcount_t		count;
	struct ns_common	ns;
	struct user_namespace	*user_ns;
	struct ucounts		*ucounts;
	struct css_set          *root_cset;
};
```

## task_struct::nsproxy
> @todo 跟踪所有相关的 ref
> 现在进行和 ipc 相关的

![](../../img/source/task_struct::nsproxy.png)


## fs/nsfs.c ，这个测试下
https://unix.stackexchange.com/questions/465669/what-is-the-nsfs-filesystem

## user namespace

```txt
config USER_NS
	bool "User namespace"
	default n
	help
	  This allows containers, i.e. vservers, to use user namespaces
	  to provide different user info for different servers.

	  When user namespaces are enabled in the kernel it is
	  recommended that the MEMCG option also be enabled and that
	  user-space use the memory control groups to limit the amount
	  of memory a memory unprivileged users can use.

	  If unsure, say N.
```

哦，用于实现 user 的隔离的

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
