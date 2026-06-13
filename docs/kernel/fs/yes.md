# fs 基础

## 当有 user 在 fs 上的时候不可以 umount
```c
struct fs_struct {
	int users;
	spinlock_t lock;
	seqcount_t seq;
	int umask;
	int in_exec;
	struct path root, pwd;
} __randomize_layout;
```

```txt
       CLONE_FS (since Linux 2.0)
              If CLONE_FS is set, the caller and the child process share  the  same  filesystem
              information.   This  includes the root of the filesystem, the current working di‐
              rectory, and the umask.  Any call to chroot(2), chdir(2), or  umask(2)  performed
              by the calling process or the child process also affects the other process.

              If  CLONE_FS  is not set, the child process works on a copy of the filesystem in‐
              formation of the calling process at the time of the clone  call.   Calls  to  ch‐
              root(2), chdir(2), or umask(2) performed later by one of the processes do not af‐
              fect the other process.
```

增加引用计数的地方:
```c
static int copy_fs(unsigned long clone_flags, struct task_struct *tsk)
{
	struct fs_struct *fs = current->fs;
	if (clone_flags & CLONE_FS) {
		/* tsk->fs is already what we want */
		spin_lock(&fs->lock);
		/* "users" and "in_exec" locked for check_unsafe_exec() */
		if (fs->in_exec) {
			spin_unlock(&fs->lock);
			return -EAGAIN;
		}
		fs->users++;
		spin_unlock(&fs->lock);
		return 0;
	}
	tsk->fs = copy_fs_struct(fs);
	if (!tsk->fs)
		return -ENOMEM;
	return 0;
}
```

1. 一个 prcess 同时打开了很多 fs 的文件，


## 当有文件打开的时候，不可以 rmmod

通过
```c
struct file_operations {
  struct module *owner; // 为什么?
  }
```


## [ ] 当有文件打开的时候，可以 rmmod nvme 吗?

## inode 中存储的 struct address_space
<!-- b6f69e65-16c1-4ab0-980a-6f0ae53e4e56 -->

```c
struct inode {
  // ...
	struct address_space	i_data;
```

之后传递给映射文件的 page 上:
```c
struct address_space *page_mapping(struct page *page)
{
	return folio_mapping(page_folio(page));
}
```



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
