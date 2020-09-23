# fs/pipe.c

1. 为什么 pipe 需要 fs 的支持，mount super_block 这种东西真的用的到吗 ?

## ops : 操作的支持

```c
const struct file_operations pipefifo_fops = {
	.open		= fifo_open,
	.llseek		= no_llseek,
	.read_iter	= pipe_read,
	.write_iter	= pipe_write,
	.poll		= pipe_poll,
	.unlocked_ioctl	= pipe_ioctl,
	.release	= pipe_release,
	.fasync		= pipe_fasync,
};
```

## init pipe : part 1
1. 外部接口
```c
/*
 * sys_pipe() is the normal C calling standard for creating
 * a pipe. It's not the way Unix traditionally does this, though.
 */
static int do_pipe2(int __user *fildes, int flags)
{
	struct file *files[2];
	int fd[2];
	int error;

	error = __do_pipe_flags(fd, files, flags); // 分配 file 和 fd
	if (!error) {
		if (unlikely(copy_to_user(fildes, fd, sizeof(fd)))) { // fildes 是返回值，而不是结果
			fput(files[0]);
			fput(files[1]);
			put_unused_fd(fd[0]);
			put_unused_fd(fd[1]);
			error = -EFAULT;
		} else {
			fd_install(fd[0], files[0]); // struct file 和 fd 挂钩
			fd_install(fd[1], files[1]);
		}
	}
	return error;
}

SYSCALL_DEFINE2(pipe2, int __user *, fildes, int, flags)
{
	return do_pipe2(fildes, flags);
}

SYSCALL_DEFINE1(pipe, int __user *, fildes)
{
	return do_pipe2(fildes, 0);
}
```

2. create_pipe_files && get_unused_fd_flags : 分别创建 struct file 和 fd

## init pipe : part 2
1. get_pipe_inode : 获取 inode 及其 关联的 pipe_inode_info
2. alloc_file_pseudo 和 alloc_file_clone : 创建 file struct

```c
int create_pipe_files(struct file **res, int flags)
{
	struct inode *inode = get_pipe_inode(); // XXX 关键
	struct file *f;

	if (!inode)
		return -ENFILE;

	f = alloc_file_pseudo(inode, pipe_mnt, "",
				O_WRONLY | (flags & (O_NONBLOCK | O_DIRECT)), // xxx 
				&pipefifo_fops);
	if (IS_ERR(f)) {
		free_pipe_info(inode->i_pipe);
		iput(inode);
		return PTR_ERR(f);
	}

	f->private_data = inode->i_pipe;

	res[0] = alloc_file_clone(f, O_RDONLY | (flags & O_NONBLOCK), // xxx
				  &pipefifo_fops);
	if (IS_ERR(res[0])) {
		put_pipe_info(inode, inode->i_pipe);
		fput(f);
		return PTR_ERR(res[0]);
	}
	res[0]->private_data = inode->i_pipe;
	res[1] = f;
	stream_open(inode, res[0]);
	stream_open(inode, res[1]);
	return 0;
}
```

```c
static struct inode * get_pipe_inode(void)
{
	struct inode *inode = new_inode_pseudo(pipe_mnt->mnt_sb); // 创建 struct inode
	struct pipe_inode_info *pipe;

	if (!inode)
		goto fail_inode;

	inode->i_ino = get_next_ino(); // todo ino 的分配 和 fd 的分配似乎很类似

	pipe = alloc_pipe_info(); // 分配 pipe_inode_info 并初始化
	if (!pipe)
		goto fail_iput;

	inode->i_pipe = pipe;
	pipe->files = 2;
	pipe->readers = pipe->writers = 1;
	inode->i_fop = &pipefifo_fops;

	/*
	 * Mark the inode dirty from the very beginning,
	 * that way it will never be moved to the dirty
	 * list because "mark_inode_dirty()" will think
	 * that it already _is_ on the dirty list.
	 */
	inode->i_state = I_DIRTY;
	inode->i_mode = S_IFIFO | S_IRUSR | S_IWUSR;
	inode->i_uid = current_fsuid();
	inode->i_gid = current_fsgid();
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);

	return inode;

fail_iput:
	iput(inode);

fail_inode:
	return NULL;
}
```

## init_fs : 初始化 pipefs

```c
struct pseudo_fs_context {
	const struct super_operations *ops;
	const struct xattr_handler **xattr;
	const struct dentry_operations *dops;
	unsigned long magic;
};
```

```c
/*
 * pipefs should _never_ be mounted by userland - too much of security hassle,
 * no real gain from having the whole whorehouse mounted. So we don't need
 * any operations on the root directory. However, we need a non-trivial
 * d_name - pipe: will go nicely and kill the special-casing in procfs.
 */

static int pipefs_init_fs_context(struct fs_context *fc)
{
	struct pseudo_fs_context *ctx = init_pseudo(fc, PIPEFS_MAGIC); // 进入到 fs/libfs.c 中间
	if (!ctx)
		return -ENOMEM;
	ctx->ops = &pipefs_ops; // 这两个内容填充的都非常简单
	ctx->dops = &pipefs_dentry_operations;
	return 0;
}
```

## pipe fs
1. 为什么需要 fs 的支持 ?
2. 在用户层可以观测到这一个 fs 吗 ?

