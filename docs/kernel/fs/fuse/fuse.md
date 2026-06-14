# fuse

首先，推荐用用我让 codex 写的最简单的 fuse 项目，看看基本的 io 流程:

其次看看官方文档:
- https://www.kernel.org/doc/html/latest/filesystems/fuse/index.html

我同意这里聊到的，其实 fuse 是一个相当有意思的机制，基本上你可以把任何东西都转换
为 fuse 。
https://news.ycombinator.com/item?id=46580136

> I've been getting into FUSE a bit lately, as I stole an idea that a friend had of how to add CoW features to an existing non-CoW filesystem, so I've been on/off been hacking on a FUSE driver for ext4 to do that.
> To learn FUSE, however, I started just making everything into filesystems that I could mount. I wrote a FUSE driver for Cassandra, I wrote a FUSE driver for CouchDB, I wrote a FUSE driver for a thing that just wrote JSON files with Base64 encoding.
>
> None of these performed very well and I'm sort of embarrassed at how terrible the code is hence why I haven't published them (and they were also just learning projects), but I did find FUSE to be extremely fun and easy to write against. I encourage everyone to play with it.

## 你想发 kernel patch 吗？

观察了一段时间，如果实在是没有什么创新，那么就做:
1. iouring 的转换
2. folio 的适配
3. zero copy / passthrough
4. iomap 转换

## zero copy
https://lwn.net/Articles/756625/ : The ZUFS zero-copy filesystem

如果是 FOPEN_DIRECT_IO 的实话，那么数据会拷贝，
user process A read 的时候，kernel 会分配一个 page 给 A，daemon 可以直接向其中写一个内容吗?

不太行，首先 A 需要和 daemon 共享这段内存才可以，而且还需要修改其他内核的东西。

```c
static ssize_t fuse_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	struct file *file = iocb->ki_filp;
	struct fuse_file *ff = file->private_data;
	struct inode *inode = file_inode(file);

	if (fuse_is_bad(inode))
		return -EIO;

	if (FUSE_IS_DAX(inode))
		return fuse_dax_read_iter(iocb, to);

	/* FOPEN_DIRECT_IO overrides FOPEN_PASSTHROUGH */
	if (ff->open_flags & FOPEN_DIRECT_IO)
		return fuse_direct_read_iter(iocb, to);
	else if (fuse_file_passthrough(ff))
		return fuse_passthrough_read_iter(iocb, to);
	else
		return fuse_cache_read_iter(iocb, to);
}
```

## io uring

```
  传统方式:                               io_uring 方式:
  ┌──────────┐                            ┌──────────┐
  │  daemon  │                            │  daemon  │
  │  read()  │ ◄── 两个系统调用 ──►       │  SQE/CQE │ ◄── 零系统调用
  │ write()  │      每次请求              │  ring    │     每次请求
  └────┬─────┘                            └────┬─────┘
       │                                       │
    /dev/fuse                            io_uring fd
       │                                       │
  ┌────┴─────┐                            ┌────┴─────┐
  │  kernel  │                            │  kernel  │
  │  fuse    │                            │  fuse    │
  └──────────┘                            └──────────┘
```

## folio 转换
https://mp.weixin.qq.com/s/hHKbXnlpDckO2IaOPJxuIw

## iomap

既然 fuse 定义了 opcode ，那么按道理就不需要使用类似 iomap 啊，就像是 nfs 那样
我这里是真的没搞懂。

```c
enum fuse_opcode {
	FUSE_LOOKUP		= 1,
	FUSE_FORGET		= 2,  /* no reply */
	FUSE_GETATTR		= 3,
	FUSE_SETATTR		= 4,
	FUSE_READLINK		= 5,
	FUSE_SYMLINK		= 6,
	FUSE_MKNOD		= 8,
	FUSE_MKDIR		= 9,
	FUSE_UNLINK		= 10,
	FUSE_RMDIR		= 11,
	FUSE_RENAME		= 12,
	FUSE_LINK		= 13,
	FUSE_OPEN		= 14,
	FUSE_READ		= 15,
	FUSE_WRITE		= 16,
	FUSE_STATFS		= 17,
	FUSE_RELEASE		= 18,
	FUSE_FSYNC		= 20,
	FUSE_SETXATTR		= 21,
	FUSE_GETXATTR		= 22,
	FUSE_LISTXATTR		= 23,
	FUSE_REMOVEXATTR	= 24,
	FUSE_FLUSH		= 25,
	FUSE_INIT		= 26,
	FUSE_OPENDIR		= 27,
	FUSE_READDIR		= 28,
	FUSE_RELEASEDIR		= 29,
	FUSE_FSYNCDIR		= 30,
	FUSE_GETLK		= 31,
	FUSE_SETLK		= 32,
	FUSE_SETLKW		= 33,
	FUSE_ACCESS		= 34,
	FUSE_CREATE		= 35,
	FUSE_INTERRUPT		= 36,
	FUSE_BMAP		= 37,
	FUSE_DESTROY		= 38,
	FUSE_IOCTL		= 39,
	FUSE_POLL		= 40,
	FUSE_NOTIFY_REPLY	= 41,
	FUSE_BATCH_FORGET	= 42,
	FUSE_FALLOCATE		= 43,
	FUSE_READDIRPLUS	= 44,
	FUSE_RENAME2		= 45,
	FUSE_LSEEK		= 46,
	FUSE_COPY_FILE_RANGE	= 47,
	FUSE_SETUPMAPPING	= 48,
	FUSE_REMOVEMAPPING	= 49,
	FUSE_SYNCFS		= 50,
	FUSE_TMPFILE		= 51,
	FUSE_STATX		= 52,

	/* CUSE specific operations */
	CUSE_INIT		= 4096,

	/* Reserved opcodes: helpful to detect structure endian-ness */
	CUSE_INIT_BSWAP_RESERVED	= 1048576,	/* CUSE_INIT << 8 */
	FUSE_INIT_BSWAP_RESERVED	= 436207616,	/* FUSE_INIT << 24 */
};
```

https://mp.weixin.qq.com/s/kS9KBUydZZbgLZUwDtYp4g

看到 Wang 的这个项目，想到了很多:
1. 如果 libfuse 使用 Sub-Part 4 of Wong's series adds two new operations, FUSE_IOMAP_BEGIN and FUSE_IOMAP_END
那么是不是只有后端本来就是 linux 管理的一个磁盘
才有意义，如果后端是 ceph 之类的用户态分布式存储 就没有意义了
2. 才意识到有 ext4 的用户态实现
  - 用户态的 ext4 实现和内核实现主要区别是什么?
    - 需要考虑 journal 吗? 应该也是需要的
    - 我感觉区别应该不大才对，检查一下这些项目吧 https://github.com/tytso/e2fsprogs


## cuse
- [Character devices in user space](https://lwn.net/Articles/308445/)

用户态的 chardev ，和 fuse 共享一些代码

## FUSE 项目分类

### 库与框架
- [libfuse](https://github.com/libfuse/libfuse)
- [fuse-backend-rs](https://github.com/cloud-hypervisor/fuse-backend-rs)
- [fuser](https://github.com/cberner/fuser)
- [go-fuse](https://github.com/hanwen/go-fuse)
- [HN: 使用 Rust 实现、后端为 Google Calendar 的 FUSE 文件系统](https://news.ycombinator.com/item?id=41154616)

### 容器与系统虚拟化
- [lxcfs](https://github.com/lxc/lxcfs) : 为容器虚拟化 procfs / sysfs 内容
- [fuse-overlayfs](https://github.com/containers/fuse-overlayfs)

### 网络与远程文件系统
- [sshfs](https://github.com/libfuse/sshfs)（[HN: 不再维护](https://news.ycombinator.com/item?id=37390184)）
- [curlftpfs](https://github.com/curlftpfs/curlftpfs)
- [webdavfs](https://github.com/miquels/webdavfs)

### 数据库与键值存储
- [etcdfs](https://github.com/polyrabbit/etcdfs) : 以文件浏览器方式查看 etcd
- [HN: 通过文件系统访问 SQLite](https://news.ycombinator.com/item?id=39417503)

### 归档与压缩
- [archivemount](https://github.com/cybernoid/archivemount) : 已停止开发
- [ratarmount](https://github.com/mxmlnkn/ratarmount) : 更快的 archivemount 替代品
- [squashfuse](https://github.com/vasi/squashfuse)

### 联合与合并文件系统
- [mergerfs](https://github.com/trapexit/mergerfs)
- [unionfs-fuse](https://github.com/rpodgorny/unionfs-fuse)

### 云存储
- [s3fs](https://github.com/s3fs-fuse/s3fs-fuse)
    - https://aws.amazon.com/cn/blogs/aws/launching-s3-files-making-s3-buckets-accessible-as-file-systems/
- [goofys](https://github.com/kahing/goofys)
- [gcsfuse](https://github.com/GoogleCloudPlatform/gcsfuse)
- [blobfuse](https://github.com/Azure/azure-storage-fuse)

### 其他
- [ntfs-3g](https://github.com/tuxera/ntfs-3g)
- [CephFS FUSE](https://docs.ceph.com/en/latest/cephfs/)
- nbd fuse

- https://gitlab.com/nbdkit/libnbd
    - https://libguestfs.org/nbdfuse.1.html
nbdfuse 是 libnbd + FUSE 的一个典型应用：它把远端或本地的 NBD（Network Block Device） 服务器，映射成本地文件系统中的一个普通文件。
对这个文件做 read/write，会经由 FUSE 进入 nbdfuse 进程，再由 libnbd 转成 NBD 协议请求发给服务器。

- https://docs.alluxio.io/os/user/stable/en/api/POSIX-API.html
- https://github.com/dragonflyoss/nydus/blob/master/docs/nydus-design.md

## 参考和阅读

- [知乎专栏: 5 分钟搞懂用户空间文件系统 FUSE 工作原理](https://zhuanlan.zhihu.com/p/106719192)  差不多真的就 5 分钟就可以看懂
- [兰新宇: 用户态文件系统 - FUSE](https://zhuanlan.zhihu.com/p/143256077) 可以更加深入的理解

https://mp.weixin.qq.com/s/8H2J4eL39XN1TPUaNhh1oA : 提高 FUSE writeback 性能！

https://mp.weixin.qq.com/s/LYHQccyhDE5DMzjhMukirg : 字节营销
https://mp.weixin.qq.com/s/2oRLzVTlQpyQeAvI0ZwDbA : ali 营销

https://mp.weixin.qq.com/s/9GSwOAmkG0mCbcr-PFp6RA :
Hierarchical storage management, fanotify, FUSE, and more

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
