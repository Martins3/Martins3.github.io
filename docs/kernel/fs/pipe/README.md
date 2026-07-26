# pipe 和 splice

## 基本理念

splice(2) / tee(2) / vmsplice(2) / sendfile(2)
都是用的这个机制:

主要涉及两个文件:
- fs/pipe.c
- fs/splice.c 中提供了三个系统调用
	- tee
	- splice
	- vmsplice

## named 和 unnamed pipe

- unnamed pipe : `pipe2()` 直接返回两个 fd，一个读端，一个写端。它们指向 pipefs 上的匿名 inode，在 `/proc/<pid>/fd` 里通常显示成 `pipe:[ino]`。
- named pipe / FIFO : `mkfifo()` 在真实文件系统里创建一个 `S_IFIFO` 特殊文件。两个无亲缘关系的进程可以分别 `open()` 同一个路径，然后通过这个 FIFO 通信。
  - Linux 没有一个叫 `mkfifo` 的 syscall。`mkfifo(3)` 是 libc 接口，典型实现是调用 `mknod/mknodat`，mode 里带上 `S_IFIFO`。
  - 内核里文件系统创建 FIFO inode 后，`init_special_inode()` 会让 `S_IFIFO` 使用 `pipefifo_fops`；第一次打开 FIFO 时 `fifo_open()` 会给 inode 挂上 `struct pipe_inode_info`。

参考:
https://unix.stackexchange.com/questions/436864/how-does-a-fifo-named-pipe-differs-from-a-regular-pipe-unnamed-pipe

## pipefs

源码在: fs/pipe.c 下

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

为什么需要文件系统，因为 pipe2 会产生两个 fd ，这两个 fd 是可以 close / read / write
，所以必须重载 对应的各种行为，所以需要有 pipefifo_fops 。同时他们的 fd 也是需要关联 inode ，有 inode 就需要 fs 来管理这些 inode .

在 inode 中，
struct pipe_inode_info::pipe_buffer points an array of pipe_buffer, every pipe_buffer manages one page frame.


pipefs_dname
```txt
ls  -la  /proc/self/fd | grep pipe
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

## pipe 三种数据来源
```c
//  anon_pipe_buf_ops —— 匿名页（普通 pipe write 产生的页）
static const struct pipe_buf_operations anon_pipe_buf_ops = {
	.release	= anon_pipe_buf_release,
	.try_steal	= anon_pipe_buf_try_steal,
	.get		= generic_pipe_buf_get,
};

//  page cache 页（splice 从文件读时直接塞进来的页）
const struct pipe_buf_operations page_cache_pipe_buf_ops = {
	.confirm	= page_cache_pipe_buf_confirm,
	.release	= page_cache_pipe_buf_release,
	.try_steal	= page_cache_pipe_buf_try_steal,
	.get		= generic_pipe_buf_get,
};

// vmsplice() 把用户态内存页直接挂到 pipe 上时用的
static const struct pipe_buf_operations user_page_pipe_buf_ops = {
	.release	= page_cache_pipe_buf_release,
	.try_steal	= user_page_pipe_buf_try_steal,
	.get		= generic_pipe_buf_get,
};
```

## 数据流

```sh
sudo perf record  -g ./splice-copy.out -- sleep 10
```

大致可以观察到这样的结果，这个结果已经相当清晰了，splice 不仅 "不是 zero copy "，
而是大多数的时间都是在 copy ，这是合理的，pipe 只是引用了 source 的 page cache ，
需要拷贝到 target 的 page cache 中去，避免的是内核和用户态之间的拷贝。
```txt
- __do_sys_splice (inlined)
   - 98.65% class_fd_destructor (inlined)
      - fdput (inlined)
         - __do_splice
            - 98.56% do_splice
               - 79.26% do_splice_from (inlined)
                  - iter_file_splice_write
                     - 77.20% xfs_file_buffered_write
                        - 77.14% iomap_file_buffered_write
                           - 76.51% iomap_write_iter
                              - 63.49% copy_folio_from_iter_atomic
                                 - 63.40% __copy_from_iter (inlined)
                                      iterate_and_advance (inlined)
                                    - iterate_and_advance2 (inlined)
                                       - 63.39% iterate_bvec (inlined)
                                            62.46% __memcpy (inlined)
                              - 8.29% iomap_write_begin
                                 - 8.13% __iomap_get_folio (inlined)
                                    - 8.12% __filemap_get_folio_mpol
                                       + 6.45% folio_alloc_noprof
                                       + 1.29% filemap_get_entry
                              + 4.31% iomap_write_end
                             0.58% iomap_iter
                       0.55% page_cache_pipe_buf_release
               - 11.40% splice_file_to_pipe
                  - 11.24% xfs_refcount_split_extent (inlined)
                     - 11.21% trace_xfs_refcount_split_extent_error (inlined)
                        - __do_trace_xfs_refcount_split_extent_error (inlined)
                           - 11.12% filemap_splice_read
                              - 10.13% filemap_get_pages
                                 - 9.76% filemap_readahead (inlined)
                                    - page_cache_ra_order
				      + 7.17% ra_alloc_folio (inlined)
				      + 2.59% filemap_invalidate_unlock_shared (inli
				0.65% splice_folio_into_pipe
                + 7.54% fsnotify_modify (inlined)
```

从上面也可以很容易理解为什么文件系统一般都需要注册:
- file_operations::splice_read
- file_operations::splice_write

Documentation/filesystems/vfs.rst
```txt
``splice_write``
	called by the VFS to splice data from a pipe to a file.  This
	method is used by the splice(2) system call

``splice_read``
	called by the VFS to splice data from file to a pipe.  This
	method is used by the splice(2) system call
```


```c
ssize_t filemap_splice_read(struct file *in, loff_t *ppos,
                            struct pipe_inode_info *pipe,
                            size_t len, unsigned int flags);
```

- 直接从文件的 page cache 中拿 folio/page；
- 把这些 page 挂到 pipe buffer 上，**不经过用户态拷贝**；
- 如果 pipe 满了，返回 `-EAGAIN`（`SPLICE_F_NONBLOCK`）或睡眠等待。

```c
ssize_t iter_file_splice_write(struct pipe_inode_info *pipe, struct file *out,
                               loff_t *ppos, size_t len, unsigned int flags);
```

- 把 pipe 里的 page 组织成 `bio_vec`；
- 调用 `out->f_op->write_iter()` 写出去；
- 写完后释放/调整对应的 pipe buffer。

普通文件系统通常直接复用它们：

```c
static const struct file_operations ext4_file_operations = {
    ...
    .splice_read  = ext4_file_splice_read,  // 有些 fs 会包一层做特殊处理
    .splice_write = iter_file_splice_write,
};
```

### 为什么 pipe 自己也有 `splice_write`？

在 `fs/pipe.c` 中：

```c
static const struct file_operations pipefifo_fops = {
    ...
    .splice_write = iter_file_splice_write,
};
```

一般来说，数据流是 xfs page cache -> pipe -> xfs page cache
这个时候，调用的都是 xfs 注册的 splice_write 和 splice_read
也就是 ./code/splice-copy.c 中的:
```c
splice(file_fd, NULL, pipe_fd[0], NULL, len, SPLICE_F_MORE);
splice(pipe_fd[1], NULL, file_fd, NULL, len, SPLICE_F_MORE);
```

因为 pipe/FIFO 也可以作为 splice 的目标端。例如：
```c
splice(in_pipe, NULL, pipe_fd[0], NULL, len, 0);
splice(pipe_fd[1], NULL, out_pipe, NULL, len, 0);
```

第一步: splice 走 `splice_pipe_to_pipe()`，不需要 `splice_read`，这一步没有数据拷贝
第二步: pipe 提供了 `splice_write`，让别人把它当作“数据来源” ，iter_file_splice_write 这里是有一次数据拷贝



## splice 和 O_DIRECT

将两个文件都修改为 O_DIRECT ，那么可以观察到:
```txt
- __se_sys_splice (inlined)
- __do_sys_splice (inlined)
   - 98.12% class_fd_destructor (inlined)
      - fdput (inlined)
         - __do_splice
            - 98.05% do_splice
               - 49.90% splice_file_to_pipe
                  - 49.77% copy_splice_read
                     + 41.31% alloc_pages_bulk_noprof
                     - 5.88% xfs_file_read_iter
                        - 5.84% xfs_refcountbt_calc_reserves (inlined)
                           - 5.04% iomap_dio_rw
                              - IS_ERR_OR_NULL (inlined)
                                 + __iomap_dio_rw
                     - 1.28% init_sync_kiocb (inlined)
                        - get_current_ioprio (inlined)
                             1.10% get_current (inlined)
               - 41.94% do_splice_from (inlined)
                  - iter_file_splice_write
                     - 39.58% xfs_rtrefcountbt_broot_realloc (inlined)
                        - 39.14% xfs_refcountbt_verify (inlined)
                           - 23.73% iomap_dio_rw
                              + IS_ERR_OR_NULL (inlined)
                           - 15.36% iomap_dio_complete
                              - 15.25% should_report_dio_fserror (inlined)
                                 + xfs_refcount_adjust_extents (inlined)
                       1.40% __free_frozen_pages
               + 5.86% fsnotify_modify (inlined)
```

前面可以看到，splice 就是基于 page cache 制作的
所以 do_splice_read 中，这种情况，
copy_splice_read 中分配 pages ，然后做 io ，等数据就绪后，
将这些 pages 放到 pipe 中去:

```c
static ssize_t do_splice_read(struct file *in, loff_t *ppos,
			      struct pipe_inode_info *pipe, size_t len,
			      unsigned int flags)
{
	// ...
	/*
	 * O_DIRECT and DAX don't deal with the pagecache, so we allocate a
	 * buffer, copy into it and splice that into the pipe.
	 */
	if ((in->f_flags & O_DIRECT) || IS_DAX(in->f_mapping->host))
		return copy_splice_read(in, ppos, pipe, len, flags);
	return in->f_op->splice_read(in, ppos, pipe, len, flags);
}
```

作为输出端，没有什么变化:
pipe buffer
  - iter_file_splice_write()
  - iov_iter_bvec()
  - out->f_op->write_iter()
  - 如果 out 是 O_DIRECT，则走文件系统 direct write 路径

这里只有一个问题， 这没有什么很特殊的，但是如果这个时候，kernel 存在 page cache
而且是 dirty ，那么怎么办?

### O_DIRECT 和 page cache 记得不要混用

1. 用户态手册页：man 2 open 的 NOTES 小节

在 “O_DIRECT” 这一段里，明确写道：

> Applications should avoid mixing O_DIRECT and normal I/O to the same file, and especially to overlapping byte regions in the same file. Even when
> the filesystem correctly handles the coherency issues in this situation, overall I/O throughput is likely to be slower than using either mode
> alone. Likewise, applications should avoid mixing mmap(2) of files with direct I/O to the same files.

也就是说，官方文档的定性是：不建议混用，混用会产生 coherency issues（一致性问题），即便文件系统努力去处理，也可能读到不一致的数据，而且性能还会下降。

2. 内核源码：mm/filemap.c

generic_file_direct_write() 函数（mm/filemap.c 约 4242 行）在发起 O_DIRECT 写之前和之后，都会尝试失效（invalidate）对应的 page cache：

```c
  written = kiocb_invalidate_pages(iocb, write_len);   // 写前失效
  ...
  kiocb_invalidate_post_direct_write(iocb, written);   // 写后再失效一次
```

如果失效失败，内核会打一条 pr_crit 警告：

```c
  pr_crit("Page cache invalidation failure on direct I/O.  "
          "Possible data corruption due to collision with buffered I/O!\n");
```

位置在 mm/filemap.c 的 dio_warn_stale_pagecache() 函数里（约第 4213–4228 行）。

### O_DIRECT 产生的 page 如何释放的

copy_splice_read 中会分配 pages 来存放数据，这些数据放到 pipe 中，
其实和 page cache 类似，当 pipe 释放数据，也就是在 generic_pipe_buf_release 中，这些 page 如果 refcount 变为 0
那么就释放了
```txt
@[
        generic_pipe_buf_release+5
        iter_file_splice_write+1128
        do_splice+456
        __do_splice+527
        __x64_sys_splice+156
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 1324480
```


## 有趣的阅读
pipe 的基本使用:
- https://www.moritz.systems/blog/mastering-unix-pipes-part-1/
- https://questions.wizardzines.com/bash-redirects.html

扩展:
- https://mazzo.li/posts/fast-pipes.html
- https://news.ycombinator.com/item?id=41348844


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
