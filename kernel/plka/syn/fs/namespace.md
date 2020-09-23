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
