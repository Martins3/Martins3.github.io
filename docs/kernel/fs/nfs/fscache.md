## fscache

## 基本代码结构

```txt
config FSCACHE
	bool "General filesystem local caching manager"
	depends on NETFS_SUPPORT
	help
	  This option enables a generic filesystem caching manager that can be
	  used by various network and other filesystems to cache data locally.
	  Different sorts of caches can be plugged in, depending on the
	  resources available.

	  See Documentation/filesystems/caching/fscache.rst for more information.
```

依赖 fscache 的文件系统
- 9P
- AFS
- CEPH
- CIFS
- NFS

他们都是有 CONFIG_NFS_FSCACHE 的代码

但是 coda fs 也是一个文件系统，但是不依赖于 fscache

## /proc
```txt
🧀  l /proc/fs/
Permissions Size User Date Modified Name
dr-xr-xr-x     - root 22 Jan 17:05   ext4
lrwxrwxrwx     - root 22 Jan 17:05   fscache -> netfs
dr-xr-xr-x     - root 22 Jan 17:05   jbd2
dr-xr-xr-x     - root 22 Jan 17:05   lockd
dr-xr-xr-x     - root 22 Jan 17:05   netfs
dr-xr-xr-x     - root 22 Jan 17:05   nfsd
dr-xr-xr-x     - root 22 Jan 17:05   nfsfs
```

为什么建立这个软链接啊，这么说，其实 /proc/ 是有其他的实现的吗?

fs/netfs/fscache_proc.c:fscache_proc_init 中的
```c
	if (!proc_symlink("fs/fscache", NULL, "netfs"))
		goto error_sym;
```

看来在这一次重构中: commit 7eb5b3e3a0a5 ("netfs, fscache: Move /proc/fs/fscache to /proc/fs/netfs and put in a symlink")

## 关键重构内容
- [netfs, afs, 9p: Delegate high-level I/O to netfslib](https://lwn.net/Articles/955944/)

## netfs
fs/netfs/

```txt
🧀  tree /proc/fs/netfs
/proc/fs/netfs
├── caches
├── cookies
├── requests
├── stats
└── volumes
```

## 基本结构
例如 fs/9p/vfs_file.c 会调用到 v9fs_file_write_iter

- 如何才可以调用到 netfs_file_write_iter 上 ?

## 这个就是 netfs cache 吗?
```txt
[   48.833775] netfs: FS-Cache loaded
```
- fs/cachefiles

## 看看文档再说
- https://www.kernel.org/doc/html/latest/filesystems/caching/index.html

## 关键问题
1. 先实际上触发一些，然后测试一下效果吧
```sh
sudo bpftrace -e 'kprobe:nfs_fscache_open_file { @[kstack(bpftrace)] = count(); }'
```
2. 可以写一个基于 netfs 的文件系统吗?


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
