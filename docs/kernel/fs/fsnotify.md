## fsnotify

使用 fs/fsnotify.c 作为测试
两个系统调用
```txt
inotify_init()                          = 3
inotify_add_watch(3, ".", IN_MODIFY|IN_CREATE|IN_DELETE) = 1
```



```txt
systemd-udevd: inotify_add_watch(7, /dev/sdv, 10) failed: No such file or directory
```

https://mp.weixin.qq.com/s/VrkPPCs1p5OqNL7PI1eBfQ

## notify
1. fsnotify_group 是怎么回事
2. 如何通知支持 inotify 和 dnotify 两个模块的


* ***如何实现告知 ?***
```c
fsnotify_modify : 在 open.c 中间可以查看到
/*
 * fsnotify_modify - file was modified
 */
static inline void fsnotify_modify(struct file *file) { fsnotify_file(file, FS_MODIFY); }

/**
 * This is the main call to fsnotify.  The VFS calls into hook specific functions
 * in linux/fsnotify.h.  Those functions then in turn call here.  Here will call
 * out to all of the registered fsnotify_group.  Those groups can then use the
 * notification event in whatever means they feel necessary.
 */
int fsnotify(struct inode *to_tell, __u32 mask, const void *data, int data_is,
       const struct qstr *file_name, u32 cookie)
```

* ***如何让从 notifyFd 中间读出 event***

```c
/* inotify syscalls */
static int do_inotify_init(int flags)
{
  struct fsnotify_group *group;
  int ret;

  /* Check the IN_* constants for consistency.  */
  BUILD_BUG_ON(IN_CLOEXEC != O_CLOEXEC);
  BUILD_BUG_ON(IN_NONBLOCK != O_NONBLOCK);

  if (flags & ~(IN_CLOEXEC | IN_NONBLOCK))
    return -EINVAL;

  /* fsnotify_obtain_group took a reference to group, we put this when we kill the file in the end */
  group = inotify_new_group(inotify_max_queued_events);
  if (IS_ERR(group))
    return PTR_ERR(group);

  ret = anon_inode_getfd("inotify", &inotify_fops, group, // 这就是关键吧
          O_RDONLY | flags);
  if (ret < 0)
    fsnotify_destroy_group(group);

  return ret;
}
```

notify 的工作，和 aio epoll 机制其实都是类似的，相比于 blocking io，每一个 IO 需要一个 thread 阻塞，而切换为 epoll，多个 IO 被阻塞到同一个 thread 而已，还是说其实 aio 是存在各种设计模式的 TODO

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
