# fs/super.c
1. 所以到底为什么 ? 同时存在 sget 和 sget_fc ?

## mount_bdev
1. blkdev_get_by_path : => lookup_bdev　=> kern_path @todo 所以，设备是如何被探测，然后加入到文件系统中间，之后只是需要使用通用的 kern_path 就可以实现查询的。
2. sget : 对于 file_system_type 中间挂载的 super_block 进行搜索

```c
struct dentry *mount_bdev(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data,
	int (*fill_super)(struct super_block *, void *, int))
{
	struct block_device *bdev;
	struct super_block *s;
	fmode_t mode = FMODE_READ | FMODE_EXCL;
	int error = 0;

	if (!(flags & SB_RDONLY))
		mode |= FMODE_WRITE;

	bdev = blkdev_get_by_path(dev_name, mode, fs_type); // todo 参数为什么含有 fs_type ? 获取的 block_device 其实其持有何种信息 ?
	if (IS_ERR(bdev))
		return ERR_CAST(bdev);

	/*
	 * once the super is inserted into the list by sget, s_umount
	 * will protect the lockfs code from trying to start a snapshot
	 * while we are mounting
	 */
	mutex_lock(&bdev->bd_fsfreeze_mutex);
	if (bdev->bd_fsfreeze_count > 0) {
		mutex_unlock(&bdev->bd_fsfreeze_mutex);
		error = -EBUSY;
		goto error_bdev;
	}
	s = sget(fs_type, test_bdev_super, set_bdev_super, flags | SB_NOSEC,
		 bdev);
	mutex_unlock(&bdev->bd_fsfreeze_mutex);
	if (IS_ERR(s))
		goto error_s;

	if (s->s_root) { // 当该 super_block 被初始化过，那么进入到下面的位置
		if ((flags ^ s->s_flags) & SB_RDONLY) {
			deactivate_locked_super(s);
			error = -EBUSY;
			goto error_bdev;
		}

		/*
		 * s_umount nests inside bd_mutex during
		 * __invalidate_device().  blkdev_put() acquires
		 * bd_mutex and can't be called under s_umount.  Drop
		 * s_umount temporarily.  This is safe as we're
		 * holding an active reference.
		 */
		up_write(&s->s_umount);
		blkdev_put(bdev, mode);
		down_write(&s->s_umount);
	} else {
		s->s_mode = mode;
		snprintf(s->s_id, sizeof(s->s_id), "%pg", bdev);
		sb_set_blocksize(s, block_size(bdev));
		error = fill_super(s, data, flags & SB_SILENT ? 1 : 0); // 对于 super_block 初始化，每一个文件系统都含有其初始化的方法
		if (error) {
			deactivate_locked_super(s);
			goto error;
		}

		s->s_flags |= SB_ACTIVE;
		bdev->bd_super = s;
	}

	return dget(s->s_root); // s->s_root 在 fill_super 中间初始化的，ext2_fill_super 中利用 ext2_iget 以及 d_make_root 来实现

error_s:
	error = PTR_ERR(s);
error_bdev:
	blkdev_put(bdev, mode);
error:
	return ERR_PTR(error);
}
EXPORT_SYMBOL(mount_bdev);
```

```c
/**
 *	sget	-	find or create a superblock
 *	@type:	  filesystem type superblock should belong to
 *	@test:	  comparison callback
 *	@set:	  setup callback
 *	@flags:	  mount flags
 *	@data:	  argument to each of them
 */
struct super_block *sget(struct file_system_type *type,
			int (*test)(struct super_block *,void *),
			int (*set)(struct super_block *,void *),
			int flags,
			void *data)
{
	struct user_namespace *user_ns = current_user_ns();
	struct super_block *s = NULL;
	struct super_block *old;
	int err;

	/* We don't yet pass the user namespace of the parent
	 * mount through to here so always use &init_user_ns
	 * until that changes.
	 */
	if (flags & SB_SUBMOUNT)
		user_ns = &init_user_ns;

retry:
	spin_lock(&sb_lock);
	if (test) {
		hlist_for_each_entry(old, &type->fs_supers, s_instances) { // 对于 file_system_type 中间的 instance 比对，应该是由于可以支持同一个 block device 可以 mount 到多个位置上。
			if (!test(old, data))
				continue;
			if (user_ns != old->s_user_ns) {
				spin_unlock(&sb_lock);
				destroy_unused_super(s);
				return ERR_PTR(-EBUSY);
			}
			if (!grab_super(old))
				goto retry;
			destroy_unused_super(s);
			return old;
		}
	}
	if (!s) {
		spin_unlock(&sb_lock);
		s = alloc_super(type, (flags & ~SB_SUBMOUNT), user_ns); // 找不到只好创建
		if (!s)
			return ERR_PTR(-ENOMEM);
		goto retry;
	}

	err = set(s, data);
	if (err) {
		spin_unlock(&sb_lock);
		destroy_unused_super(s);
		return ERR_PTR(err);
	}
	s->s_type = type;
	strlcpy(s->s_id, type->name, sizeof(s->s_id));
	list_add_tail(&s->s_list, &super_blocks); // 将当前的 super blocks 放到 super_blocks 全局变量
	hlist_add_head(&s->s_instances, &type->fs_supers); // 一个 file_system_type 中可以挂入多个的 super_blocks
	spin_unlock(&sb_lock);
	get_filesystem(type);
	register_shrinker_prepared(&s->s_shrink);
	return s;
}
EXPORT_SYMBOL(sget);
```


## vfs_get_super
1. get_tree_nodev => vfs_get_super => sget_fc
2. graft_tree 在此处应该是不存在的 @todo 但是从哪里可以访问这个，以及其中 inode 在哪里，是不是这些不需要 graft_tree 的都是没有 tree 结构的，所有都是放在 root inode 下

```c
int get_tree_nodev(struct fs_context *fc,
		  int (*fill_super)(struct super_block *sb,
				    struct fs_context *fc))
{
	return vfs_get_super(fc, vfs_get_independent_super, fill_super);
}
EXPORT_SYMBOL(get_tree_nodev);

/**
 * vfs_get_super - Get a superblock with a search key set in s_fs_info.
 * @fc: The filesystem context holding the parameters
 * @keying: How to distinguish superblocks
 * @fill_super: Helper to initialise a new superblock
 *
 * Search for a superblock and create a new one if not found.  The search
 * criterion is controlled by @keying.  If the search fails, a new superblock
 * is created and @fill_super() is called to initialise it.
 *
 * @keying can take one of a number of values:
 *
 * (1) vfs_get_single_super - Only one superblock of this type may exist on the
 *     system.  This is typically used for special system filesystems.
 *
 * (2) vfs_get_keyed_super - Multiple superblocks may exist, but they must have
 *     distinct keys (where the key is in s_fs_info).  Searching for the same
 *     key again will turn up the superblock for that key.
 *
 * (3) vfs_get_independent_super - Multiple superblocks may exist and are
 *     unkeyed.  Each call will get a new superblock.
 *
 * A permissions check is made by sget_fc() unless we're getting a superblock
 * for a kernel-internal mount or a submount.
 */
int vfs_get_super(struct fs_context *fc,
		  enum vfs_get_super_keying keying,
		  int (*fill_super)(struct super_block *sb,
				    struct fs_context *fc))
{
	int (*test)(struct super_block *, struct fs_context *);
	struct super_block *sb;
	int err;

	switch (keying) { // 关于 test 的含义的完美解释，同一个 file_system_type 是否可以对应多个 super_block 的
	case vfs_get_single_super:
	case vfs_get_single_reconf_super:
		test = test_single_super;
		break;
	case vfs_get_keyed_super:
		test = test_keyed_super;
		break;
	case vfs_get_independent_super:
		test = NULL;
		break;
	default:
		BUG();
	}

	sb = sget_fc(fc, test, set_anon_super_fc); // 和 sget 类似，搜索或者创建一个 super_block
	if (IS_ERR(sb))
		return PTR_ERR(sb);

	if (!sb->s_root) {
		err = fill_super(sb, fc); // 首次初始化使用这一个
		if (err)
			goto error;

		sb->s_flags |= SB_ACTIVE;
		fc->root = dget(sb->s_root);
	} else {
		fc->root = dget(sb->s_root);
		if (keying == vfs_get_single_reconf_super) {
			err = reconfigure_super(fc);
			if (err < 0) {
				dput(fc->root);
				fc->root = NULL;
				goto error;
			}
		}
	}

	return 0;

error:
	deactivate_locked_super(sb);
	return err;
}
EXPORT_SYMBOL(vfs_get_super);

/**
 * sget_fc - Find or create a superblock
 * @fc:	Filesystem context.
 * @test: Comparison callback
 * @set: Setup callback
 *
 * Find or create a superblock using the parameters stored in the filesystem
 * context and the two callback functions.
 *
 * If an extant superblock is matched, then that will be returned with an
 * elevated reference count that the caller must transfer or discard.
 *
 * If no match is made, a new superblock will be allocated and basic
 * initialisation will be performed (s_type, s_fs_info and s_id will be set and
 * the set() callback will be invoked), the superblock will be published and it
 * will be returned in a partially constructed state with SB_BORN and SB_ACTIVE
 * as yet unset.
 */
struct super_block *sget_fc(struct fs_context *fc,
			    int (*test)(struct super_block *, struct fs_context *),
			    int (*set)(struct super_block *, struct fs_context *))
{
	struct super_block *s = NULL;
	struct super_block *old;
	struct user_namespace *user_ns = fc->global ? &init_user_ns : fc->user_ns;
	int err;

retry:
	spin_lock(&sb_lock);
	if (test) {
		hlist_for_each_entry(old, &fc->fs_type->fs_supers, s_instances) {
			if (test(old, fc))
				goto share_extant_sb;
		}
	}
	if (!s) {
		spin_unlock(&sb_lock);
		s = alloc_super(fc->fs_type, fc->sb_flags, user_ns);
		if (!s)
			return ERR_PTR(-ENOMEM);
		goto retry;
	}

	s->s_fs_info = fc->s_fs_info;
	err = set(s, fc);
	if (err) {
		s->s_fs_info = NULL;
		spin_unlock(&sb_lock);
		destroy_unused_super(s);
		return ERR_PTR(err);
	}
	fc->s_fs_info = NULL;
	s->s_type = fc->fs_type;
	s->s_iflags |= fc->s_iflags;
	strlcpy(s->s_id, s->s_type->name, sizeof(s->s_id));
	list_add_tail(&s->s_list, &super_blocks);
	hlist_add_head(&s->s_instances, &s->s_type->fs_supers);
	spin_unlock(&sb_lock);
	get_filesystem(s->s_type);
	register_shrinker_prepared(&s->s_shrink);
	return s;

share_extant_sb:
	if (user_ns != old->s_user_ns) {
		spin_unlock(&sb_lock);
		destroy_unused_super(s);
		return ERR_PTR(-EBUSY);
	}
	if (!grab_super(old))
		goto retry;
	destroy_unused_super(s);
	return old;
}
EXPORT_SYMBOL(sget_fc);

```

