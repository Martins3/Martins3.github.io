# pipe

## 先把基本理念搞清楚了
- https://unix.stackexchange.com/questions/436864/how-does-a-fifo-named-pipe-differs-from-a-regular-pipe-unnamed-pipe

- un-named pipe : 使用 pipe2 可以获取两个
- named pipe : mkfifo 创建一个文件出来，两个程序分别打开这个文件

## 系统调用

### pipe
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

### vmsplice splice tee pipe
fs/splice.c 提供了三个系统调用
- tee
- splice
- vmsplice

## 有趣的东西
https://mazzo.li/posts/fast-pipes.html

https://news.ycombinator.com/item?id=41348844
不知道为什么一天到晚都在分析 pipe

## splice

-  [connect buffer and pipe buffer with splice](https://gist.github.com/karthick18/1234187)

> splice() moves data between two file descriptors without copying between kernel address space and user address space.
> It transfers up to len bytes of data from the file descriptor fd_in to the file descriptor `fd_out`, where one of the file descriptors must refer to a pipe.

without splice, we can copy file by coping file to user space and coping data in userspace to kernel space.
splice can copy file directly in kernel with pipe.

Because splice connect two fd (one of is pipe fd), so splice also works with tee.
Tee can copy files without consuming it, in the second stage, we can splice the data to files.

- [ ] ./tlpi-dist/pipes/pipe_sync.c
    - if parent contains a pipe and fork several children, then they share a public pipe, in another word, everyone write to same buffer and everyone read from same buffer.
    - Only the buffer is close by parent and all children, will it be closed
    - [ ] so, check how kernel work for this

## pipe
- https://questions.wizardzines.com/bash-redirects.html

- [x] get_pipe_inode()
  - init inode
  - only called by create_pipe_files
    - create_pipe_files ==> get_pipe_inode + alloc_file_pseudo + alloc_file_clone
    - create_pipe_files alloc one `struct inode` and two `struct file`.
    - both of `file` points to the `inode`
  - alloc_pipe_info : alloc pipe related code


If we get fd, then find inode, then standard path to `pipefifo_fops::pipe_read`


## 为什么 pipe 还需要一个 fs 啊

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


为什么需要文件系统，因为 pipe2 会产生两个 fd ，这两个 fd 是可以 close / read / write ，所以必须重载
对应的各种行为，所以需要有 pipefifo_fops 。同时他们的 fd 也是需要关联 inode ，有 inode 就需要 fs 来管理这些 inode .

在 inode 中，
struct pipe_inode_info::pipe_buffer points an array of pipe_buffer, every pipe_buffer manages one page frame.

### 有趣的 dentry_operations::d_dname

使用 fs/pipe2.c 下:

```txt
@[
    pipefs_dname+5
    proc_pid_readlink+218
    vfs_readlink+255
    do_readlinkat+270
    __x64_sys_readlink+30
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 8
```

```txt
🧀  ls -la fd
lrwx------ - martins3 24 Jan 22:48  0 -> /dev/pts/12
lrwx------ - martins3 24 Jan 22:48  1 -> /dev/pts/12
lrwx------ - martins3 24 Jan 22:48  2 -> /dev/pts/12
lr-x------ - martins3 24 Jan 22:48  3 -> pipe:[1443562]
l-wx------ - martins3 24 Jan 22:48  4 -> pipe:[1443562]
```

```c
/*
 * pipefs_dname() is called from d_path().
 */
static char *pipefs_dname(struct dentry *dentry, char *buffer, int buflen)
{
	return dynamic_dname(buffer, buflen, "pipe:[%lu]",
				d_inode(dentry)->i_ino);
}

static const struct dentry_operations pipefs_dentry_operations = {
	.d_dname	= pipefs_dname,
};
```

除了展示，应该还有其他的作用吧
### [ ] pipe_buf_operations
buffer ops: because some optimization, e.g., steal page to avoid coping,
we can do some magic about buffer page.

## 如何理解 file_operations 中的
-  splice_write 和 splice_read 和 pipe 的关系是什么 ?

## 有趣的阅读
https://www.moritz.systems/blog/mastering-unix-pipes-part-1/

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
