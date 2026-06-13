# ipc/mqueue.c

1. ipcmq_sysctl.c 提供了关于 proc 的接口

## todo
1. 体会 sysv msg queue 的设计上的差别
2. mq_notify 实现的异步机制
3. proc 的 interface 如何实现 ?
4. 那么如何实现 namespace 的隔离 ?
    1. vfsmount
6. msg.c 和 mqueue.c 都存在 COMPAT 是做什么的 ?

5. 也是采用队列的方法吗 ? 当容量不足的时候会出现 阻塞的吗 ?
## question

## ref && doc

Man mq_overview(7)

> POSIX message queues allow processes to exchange data in the form of messages.  This API is distinct from that provided by System V message queues (msgget(2), msgsnd(2), msgrcv(2), etc.), but provides similar functionality.

一个是 Posix , 一个是 System V message queue

> On Linux, message queues are created in a virtual filesystem.

> On Linux, a message queue descriptor is actually a file descriptor.

利用了文件系统实现

## mq_unlink
1. mq_link 在哪里 ?

```c
SYSCALL_DEFINE1(mq_unlink, const char __user *, u_name)
{
	int err;
	struct filename *name;
	struct dentry *dentry;
	struct inode *inode = NULL;
	struct ipc_namespace *ipc_ns = current->nsproxy->ipc_ns;
	struct vfsmount *mnt = ipc_ns->mq_mnt;

	name = getname(u_name);
	if (IS_ERR(name))
		return PTR_ERR(name);

	audit_inode_parent_hidden(name, mnt->mnt_root);
	err = mnt_want_write(mnt);
	if (err)
		goto out_name;
	inode_lock_nested(d_inode(mnt->mnt_root), I_MUTEX_PARENT);
	dentry = lookup_one_len(name->name, mnt->mnt_root,
				strlen(name->name));
	if (IS_ERR(dentry)) {
		err = PTR_ERR(dentry);
		goto out_unlock;
	}

	inode = d_inode(dentry);
	if (!inode) {
		err = -ENOENT;
	} else {
		ihold(inode);
		err = vfs_unlink(d_inode(dentry->d_parent), dentry, NULL);
	}
	dput(dentry);

out_unlock:
	inode_unlock(d_inode(mnt->mnt_root));
	if (inode)
		iput(inode);
	mnt_drop_write(mnt);
out_name:
	putname(name);

	return err;
}
```
1. lookup_one_len 找到 inode
2. vfs_unlink 实现 unlink

## mq_send && mq_timedsend
1. 根本没有 mq_send 这个 syscall , 应该是 glibc 封装出来的


> Man mq_timedsend(2)
> mq_timedsend() behaves just like mq_send(), except that if the queue is full and the O_NONBLOCK flag is not enabled for the message queue description, then abs_timeout points to a structure which specifies how long the call will block.  This value is an absolute timeout in seconds and nanoseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC), specified in the following structure:

```c
SYSCALL_DEFINE5(mq_timedsend, mqd_t, mqdes, const char __user *, u_msg_ptr,
		size_t, msg_len, unsigned int, msg_prio,
		const struct __kernel_timespec __user *, u_abs_timeout)
{
	struct timespec64 ts, *p = NULL;
	if (u_abs_timeout) {
		int res = prepare_timeout(u_abs_timeout, &ts);
		if (res)
			return res;
		p = &ts;
	}
	return do_mq_timedsend(mqdes, u_msg_ptr, msg_len, msg_prio, p);
}
```

```c
static int do_mq_timedsend(mqd_t mqdes, const char __user *u_msg_ptr,
		size_t msg_len, unsigned int msg_prio,
		struct timespec64 *ts)
  // ...
	if (info->attr.mq_curmsgs == info->attr.mq_maxmsg) {
		if (f.file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
		} else {
			wait.task = current;
			wait.msg = (void *) msg_ptr;
			wait.state = STATE_NONE;
			ret = wq_sleep(info, SEND, timeout, &wait); // todo
			/*
			 * wq_sleep must be called with info->lock held, and
			 * returns with the lock released
			 */
			goto out_free;
		}
	} else {
		receiver = wq_get_first_waiter(info, RECV); // todo
		if (receiver) {
			pipelined_send(&wake_q, info, msg_ptr, receiver); // todo
		} else {
			/* adds message to the queue */
			ret = msg_insert(msg_ptr, info); // todo
			if (ret)
				goto out_unlock;
			__do_notify(info);
		}
		inode->i_atime = inode->i_mtime = inode->i_ctime =
				current_time(inode);
	}
```

## mq_open

```c
SYSCALL_DEFINE4(mq_open, const char __user *, u_name, int, oflag, umode_t, mode,
		struct mq_attr __user *, u_attr)
{
	struct mq_attr attr;
	if (u_attr && copy_from_user(&attr, u_attr, sizeof(struct mq_attr)))
		return -EFAULT;

	return do_mq_open(u_name, oflag, mode, u_attr ? &attr : NULL);
  // 并没有什么神奇的地方
  // 利用了 vfs 的各种标准函数而已
}
```


## mq_getsetattr
```c
SYSCALL_DEFINE3(mq_getsetattr, mqd_t, mqdes,
		const struct mq_attr __user *, u_mqstat,
		struct mq_attr __user *, u_omqstat)
{
	int ret;
	struct mq_attr mqstat, omqstat;
	struct mq_attr *new = NULL, *old = NULL;

	if (u_mqstat) {
		new = &mqstat;
		if (copy_from_user(new, u_mqstat, sizeof(struct mq_attr)))
			return -EFAULT;
	}
	if (u_omqstat)
		old = &omqstat;

	ret = do_mq_getsetattr(mqdes, new, old);
	if (ret || !old)
		return ret;

	if (copy_to_user(u_omqstat, old, sizeof(struct mq_attr)))
		return -EFAULT;
	return 0;
}

static int do_mq_getsetattr(int mqdes, struct mq_attr *new, struct mq_attr *old) // 依旧是利用 vfs
{
	struct fd f;
	struct inode *inode;
	struct mqueue_inode_info *info;

	if (new && (new->mq_flags & (~O_NONBLOCK)))
		return -EINVAL;

	f = fdget(mqdes);
	if (!f.file)
		return -EBADF;

	if (unlikely(f.file->f_op != &mqueue_file_operations)) {
		fdput(f);
		return -EBADF;
	}

	inode = file_inode(f.file);
	info = MQUEUE_I(inode);

	spin_lock(&info->lock);

	if (old) {
		*old = info->attr;
		old->mq_flags = f.file->f_flags & O_NONBLOCK;
	}
	if (new) {
		audit_mq_getsetattr(mqdes, new);
		spin_lock(&f.file->f_lock);
		if (new->mq_flags & O_NONBLOCK)
			f.file->f_flags |= O_NONBLOCK;
		else
			f.file->f_flags &= ~O_NONBLOCK;
		spin_unlock(&f.file->f_lock);

		inode->i_atime = inode->i_ctime = current_time(inode);
	}

	spin_unlock(&info->lock);
	fdput(f);
	return 0;
}
```
