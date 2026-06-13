# fuse
## 背景知识

原理参考
- [知乎专栏: 5 分钟搞懂用户空间文件系统 FUSE 工作原理](https://zhuanlan.zhihu.com/p/106719192)  差不多真的就 5 分钟就可以看懂
- [兰新宇: 用户态文件系统 - FUSE](https://zhuanlan.zhihu.com/p/143256077) 可以更加深入的理解

用户态的 daemon 的实现:
- [libfuse](https://github.com/libfuse/libfuse)
- [github : Rust FUSE library for server, virtio-fs and vhost-user-fs](https://github.com/cloud-hypervisor/fuse-backend-rs) https://github.com/cberner/fuser
- https://github.com/hanwen/go-fuse

基于 fuse 开发的有意思的项目:
- [sshfs](https://github.com/libfuse/sshfs)
- [etcdfs : 文件浏览器查看 etcd 的内容](https://github.com/polyrabbit/etcdfs)

## fuse io 需要构造 bio 吗?
按道理不需要那么底层的接口吧

### virtio fs

居然需要嵌入到 fs/fuse 下

## doc
https://www.kernel.org/doc/Documentation/filesystems/fuse-io.txt

## 难道 zfs 还是可以放到用户态上面啊 !


## 有趣的项目
- https://news.ycombinator.com/item?id=41154616 : 使用 rust + 后端是 google calendar

## lxc 这个项目中为了虚拟化 procfs 中的内容
- https://github.com/lxc/lxcfs

## fuse 的两个进展
<!-- 880cd423-c2ae-4f5c-8805-9d92e214e57a -->

### 使用 io uring
- https://lore.kernel.org/linux-mm/20240530204736.GH2210558@perftesting/T/
  - https://lwn.net/Articles/756625/

反而无法解决 zero copy 的问题?

这里论述的好处，只能说，没有完全理解。

### 使用 iomap

https://mp.weixin.qq.com/s/kS9KBUydZZbgLZUwDtYp4g

看到 Wang 的这个项目，想到了很多:
1. 如果 libfuse 使用 Sub-Part 4 of Wong's series adds two new operations, FUSE_IOMAP_BEGIN and FUSE_IOMAP_END
那么是不是只有后端本来就是 linux 管理的一个磁盘
才有意义，如果后端是 ceph 之类的用户态分布式存储 就没有意义了
2. 才意识到有 ext4 的用户态实现
  - 用户态的 ext4 实现和内核实现主要区别是什么?
    - 需要考虑 journal 吗? 应该也是需要的
    - 我感觉区别应该不大才对，检查一下这些项目吧 https://github.com/tytso/e2fsprogs

## 为什么大家放弃了 sshfs ?
https://news.ycombinator.com/item?id=37390184
这个号称要维护的项目也没有维护了。

- sshfs 为什么需要过用户态? 整个 io 流程是什么样子的
  - 猜测是，通过 ssh 操作 server 的文件系统，然后在本地构建一个文件系统出来
  - nfs 在 server 和 client 端都是有一个 kernel module 的
    - 如果这样，那么 nfs client 岂不是可以发送恶意的信息到 server 中?
      - 都可以操作文件系统了，还有啥不可以的，只是这个用户被限制了

## 熟悉之后再看看
https://zido.site/blog/2021-11-27-filesystem-in-user-space/

## 细节问题

如果是 FOPEN_DIRECT_IO 的实话，那么数据会拷贝吗?

read 的提供一个 page ，fuse 的实现者(一个用户态程序) 可以直接向其中写一个内容吗?

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

## 似乎所有的操作都是需要转发一下

## 想不到啊，可以这么商业化
https://docs.alluxio.io/os/user/stable/en/api/POSIX-API.html
https://github.com/dragonflyoss/nydus/blob/master/docs/nydus-design.md

## 如何多线程
检查一下 libfuse 中的 fuse_loop_mt_31() 实现，但是没看懂

先看看那些 header 信息吧

## TODO
https://mp.weixin.qq.com/s/LYHQccyhDE5DMzjhMukirg

## fuse 中，dcache 和 icache 是用户态维护吧

还是两个都在维护。

https://mp.weixin.qq.com/s/8H2J4eL39XN1TPUaNhh1oA


https://mp.weixin.qq.com/s/2oRLzVTlQpyQeAvI0ZwDbA

https://mp.weixin.qq.com/s/9GSwOAmkG0mCbcr-PFp6RA

https://mp.weixin.qq.com/s/hHKbXnlpDckO2IaOPJxuIw

## 看看
https://news.ycombinator.com/item?id=39417503

> WebDAV is a much better choice than FUSE

都不知道在说什么

https://unix.stackexchange.com/questions/24032/faster-alternative-to-archivemount

https://github.com/mxmlnkn/ratarmount
https://github.com/cybernoid/archivemount : 已经没有开发了

## 看看这个东西
squashfuse

https://github.com/vasi/squashfuse

完全可以从这里入手的

## 所以，现在基本上有一个规律
如果实在是没有什么创新，那么就做
1. iouring 的转换
2. 做 folio 的适配

https://mp.weixin.qq.com/s/hHKbXnlpDckO2IaOPJxuIw

## 这个文档记得看看
Documentation/filesystems/fuse-io-uring.rst

## fuse 的 server 必须是在本机吗?

估计是的，就像 nfs 也是需要一个前端跑在本机一样

## 看看这个 patch
fuse: support io-uring registered buffers
https://lore.kernel.org/linux-fsdevel/20251022202021.3649586-1-joannelkoong@gmail.com/

## 类似类似的项目很多
https://github.com/trapexit/mergerfs

可以让 deepseek 之类帮忙好好找找


## 既然 fuse 定义了 opcode ，那么按道理就不需要使用类似 iomap 啊，就像是 nfs


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

## 典型项目：encfs（加密）、mergerfs（合并）、rclone mount（云存储）。

unionfs-fuse / mergerfs

ntfs-3g

s3fs / goofys

gcsfuse / blobfuse
CephFS FUSE
ProcFS / SysFS for Userspace

fuse-overlayfs

curlftpfs

httpfs / webdavfs

## 这里的讨论有意思的
https://news.ycombinator.com/item?id=46580136


> I've been getting into FUSE a bit lately, as I stole an idea that a friend had of how to add CoW features to an existing non-CoW filesystem, so I've been on/off been hacking on a FUSE driver for ext4 to do that.
> To learn FUSE, however, I started just making everything into filesystems that I could mount. I wrote a FUSE driver for Cassandra, I wrote a FUSE driver for CouchDB, I wrote a FUSE driver for a thing that just wrote JSON files with Base64 encoding.
>
> None of these performed very well and I'm sort of embarrassed at how terrible the code is hence why I haven't published them (and they were also just learning projects), but I did find FUSE to be extremely fun and easy to write against. I encourage everyone to play with it.

## fuse 的 iouring 得到了支持了
fs/fuse/dev_uring.c

把每一个周期的 kernel 中的 patch 都整理一下:
- https://lore.kernel.org/linux-mm/20240529-fuse-uring-for-6-9-rfc2-out-v1-0-d149476b1d65@ddn.com/T/#m819517c7198781043e435387342306c30d473208
- https://www.phoronix.com/forums/forum/software/general-linux-open-source/1522820-fuse-hooks-up-with-io_uring-for-greater-performance-potential-in-linux-6-14

```
  传统方式:                               io_uring 方式:
  ┌──────────┐                            ┌──────────┐
  │  daemon  │                            │  daemon  │
  │  read()  │ ◄── 两个系统调用 ──►        │  SQE/CQE │ ◄── 零系统调用
  │ write()  │      每次请求               │  ring    │     每次请求
  └────┬─────┘                            └────┬─────┘
       │                                       │
    /dev/fuse                            io_uring fd
       │                                       │
  ┌────┴─────┐                            ┌────┴─────┐
  │  kernel  │                            │  kernel  │
  │  fuse    │                            │  fuse    │
  └──────────┘                            └──────────┘
```
所以，这个改造，现在可以给 userfaultfd 也搞一个。

## libnbd 也是支持 fuse 的

https://gitlab.com/nbdkit/libnbd
https://libguestfs.org/nbdfuse.1.html


## s3 fuse
https://github.com/s3fs-fuse/s3fs-fuse
https://aws.amazon.com/cn/blogs/aws/launching-s3-files-making-s3-buckets-accessible-as-file-systems/

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
