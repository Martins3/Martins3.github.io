# pipe

系统调用 pipe2 和 pipe2 的区别

- do_pipe2
  - `__do_pipe_flags`
    - create_pipe_files
      - create_pipe_files : 创建一个 inode
        - get_pipe_inode
          - new_inode_pseudo : 通过 super_block 创建出来 inode
          - alloc_pipe_info : 真正分配和 pipe 相关的内容
        - alloc_file_pseudo : 创建文件，参数是 inode vfs_mount 和 pipefifo_fops
        - alloc_file_clone
    - get_unused_fd_flags : 获取到一个 fd
    - get_unused_fd_flags
  - fd_install

1. 为什么 pipe 需要 fs 的支持，mount super_block 这种东西真的用的到吗 ?

## fops

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

## 定义一个简单的文件系统类型来标准化各种操作

```c
/*
 * pipefs should _never_ be mounted by userland - too much of security hassle,
 * no real gain from having the whole whorehouse mounted. So we don't need
 * any operations on the root directory. However, we need a non-trivial
 * d_name - pipe: will go nicely and kill the special-casing in procfs.
 */

static int pipefs_init_fs_context(struct fs_context *fc)
{
	struct pseudo_fs_context *ctx = init_pseudo(fc, PIPEFS_MAGIC);
	if (!ctx)
		return -ENOMEM;
	ctx->ops = &pipefs_ops;
	ctx->dops = &pipefs_dentry_operations;
	return 0;
}
```
