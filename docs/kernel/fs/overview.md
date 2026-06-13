## 暂时存放一些无法分类的内容

- https://github.com/seaweedfs/seaweedfs

### juicefs : https://github.com/juicedata/juicefs

类似的:
https://github.com/seaweedfs/seaweedfs

Using the JuiceFS to store data, the data itself will be persisted in object storage (e.g. Amazon S3), and the metadata corresponding to the data can be persisted in various database engines such as Redis, MySQL, and SQLite according to the needs of the scene.

### cephfs
- https://docs.ceph.com/en/nautilus/cephfs

### libnfs
https://github.com/sahlberg/libnfs

### 根据内核的 menuconfig 其实可以搞一个主流文件系统分析

### cubefs
https://mp.weixin.qq.com/s/PyOwFzOSZK0pe6Xd0MnHJQ

### nfs-ganesha
- https://github.com/nfs-ganesha/nfs-ganesha
  - https://docs.ceph.com/en/latest/cephfs/nfs/ : cephfs

### glusterfs

qemu : commit 3822df47c240 ("Remove the deprecated glusterfs block driver")
之前作为 block 的后端，但是现在已经移除了。

https://github.com/gluster/glusterfs
看上去，这个 fs 已经不咋更新了。
- glusterfs : Ceph is basically an object-oriented memory for unstructured data, whereas GlusterFS uses hierarchies of file system trees in block storage.
  - https://docs.gluster.org/en/v3/Administrator%20Guide/NFS-Ganesha%20GlusterFS%20Integration/


## Rook
## Longhorn
## OpenEBS

## 类似 nfs 这种文件系统的接入层还有什么?
nbd ?

cifs
smb

### 尝试下 s3 吧


## SquirrelFS: Using the Rust compiler to check file-system crash consistency
https://news.ycombinator.com/item?id=40767529

How does it compare to Nova-Fortis, PMFS, Strata, Ziggurat (Rust), SplitFS, and Aerie?

居然还是 OSDI 2024

## posix 语义
先看看这个，其实具体的内容都是分散到这个系统调用的定义中了:
https://juicefs.com/zh-cn/blog/engineering/cloud-file-system-posix-compliant

- [ ] https://github.com/mhx/dwarfs : c++ 写的
  - 不过他是如何跨 os 的啊，同时支持 windows / macos


## scholar
- http://www.betrfs.org/
  - https://news.ycombinator.com/item?id=29403320

- benchmark
  - https://lwn.net/Articles/385081/

## dup
1. 为什么会存在 dup 一个 fd 的需求啊
  - https://stackoverflow.com/a/28482432/10449460
2. 多个 file 会指向 inode，因为对于文件的读写状态不同的
    1. 但是为什么会存在多个 file descriptor 指向同一个 fd 啊 ?

Duplicated file descriptors created by dup2() or fork() point to the same file object.

## 细节问题
1. 为什么 podman 需要 fuse ?

似乎是为了 fuse-overlayfs ，那么为什么普通的 overlay 需要使用 root 权限


## 常见的分布式文件系统

- ceph
- lustre
- minio : MinIO works as an object storage system while GlusterFS works as a file storage system.
	- https://github.com/rustfs/rustfs 只有 3万行啊
- longhorn : https://github.com/longhorn/longhorn : Cloud-Native distributed storage built on and for Kubernetes
- brdb

- HDFS, Lustre, GlusterFS, ZFS, Ceph, Beegfs, Moosefs


## 可以仅仅看看其中的目录而已
https://book.douban.com/subject/35731316/

内容质量有待确认。

第 5 章 基于网络共享的网络文件系统 . 172
5.1 什么是网络文件系统 . 172
5.2 网络文件系统与本地文件系统的异同 . 174
5.3 常见的网络文件系统简析 . 174
5.3.1 NFS 文件系统 . 174
5.3.2 SMB 协议与 CIFS 协议 . 175
5.4 网络文件系统关键技术 . 175
5.4.1 远程过程调用（RPC 协议） . 176
5.4.2 客户端与服务端的语言——文件系统协议 . 177
5.4.3 文件锁的网络实现 . 178
5.5 准备学习环境与工具 . 179
5.5.1 搭建一个 NFS 服务 . 179
5.5.2 学习网络文件系统的利器 . 180
5.6 网络文件系统实例 . 181
5.6.1 NFS 文件系统架构及流程简析 . 181
5.6.2 RPC 协议简析 . 185
5.6.3 NFS 协议简析 . 186
5.6.4 NFS 协议的具体实现 . 191
5.7 NFS 服务端及实例解析 . 203
5.7.1 NFSD . 203
5.7.2 NFS-Ganesha . 210
第 6 章 提供横向扩展的分布式文件系统 . 216
6.1 什么是分布式文件系统 . 216
6.2 分布式文件系统与网络文件系统的异同 . 217
6.3 常见分布式文件系统 . 217
6.3.1 GFS . 218
6.3.2 CephFS . 219
6.3.3 GlusterFS . 219
6.4 分布式文件系统的横向扩展架构 . 220
6.4.1 中心架构 . 220
6.4.2 对等架构 . 221
6.5 分布式文件系统的关键技术 . 222
6.5.1 分布式数据布局 . 222
6.5.2 分布式数据可靠性（Reliability）. 224
6.5.3 分布式数据一致性（Consistency） . 228
6.5.4 设备故障与容错（Fault Tolerance） . 229
6.6 分布式文件系统实例之 CephFS . 230
6.6.1 搭建一个 CephFS 分布式文件系统 . 230
6.6.2 CephFS 分布式文件系统架构简析 . 231
6.6.3 CephFS 客户端架构 . 234
6.6.4 CephFS 集群端架构 . 236
6.6.5 CephFS 数据组织简析 . 239
6.6.6 CephFS 文件创建流程解析 . 244
6.6.7 CephFS 写数据流程解析 . 251
第 7 章 百花争艳——文件系统的其他形态 . 272
7.1 用户态文件系统框架 . 272
7.1.1 Linux 中的用户态文件系统框架 Fuse . 272
7.1.2 Windows 中的用户态文件系统框架 Dokany . 279
7.2 对象存储与常见实现简析 . 282
7.2.1 从文件系统到对象存储 . 282
7.2.2 S3 对象存储简析 . 287
7.2.3 Haystack 对象存储简析 . 288

## fd , struct file 和 inode 的三级关系
<!-- 66e613c1-af70-4bf2-bb5b-0415c7a5ade4 -->

1. dup2() 可以产生多个 fd 指向同一个 file
  - This situation may arise as a result of a call to dup(), dup2(), or fcntl()
  - 测试代码 code/src/c/fs/dup-fd.c
2. 对于第一个 path open 多次，可以让多个 file 指向一个同一个 inode
  - file struct 描述了这个文件当前写到哪里去了

那么，当多个 fd 指向一个 file struct 的时候，那么一个 fd 通过 fseek 修改了 offset ，其他的 fd 也可以观察到.

为什么存在这种需求，在同一个进程中间不同 fd 指向相同的 file struct ，主要是重定向之类的吧

## dup dup2 和 fcntl 的差别
<!-- 3b129d3e-d0f9-45ca-a864-114d6a1a1788 -->

`dup()`、`dup2()` 和 `fcntl()`（配合 `F_DUPFD` / `F_DUPFD_CLOEXEC`）都可以做

clone 的 fd 会:
- 共享文件偏移量（offset
- 共享 O_NONBLOCK / O_APPEND 等状态
- 共享锁（flock / POSIX locks
- **不共享** fd flags（如 FD_CLOEXEC)
- 引用计数 +1

| 接口                                   | 目标 fd 是否可控 |
| -------------------------------------- | ---------------- |
| `dup(oldfd)`                           | 返回最小可用 fd  |
| `dup2(oldfd, newfd)`                   | 精确指定         |
| `fcntl(oldfd, F_DUPFD, start)`         | 从 start 开始找  |

- `fcntl(oldfd, F_DUPFD_CLOEXEC, start)` 相比就是原子的配置上 CLOEXEC
- dup2 会自动 close newfd ，如果 newfd 之前已经打开


## 常识
1. 硬链接无法跨越 device ，但是软链接可以

```txt
 ln /home/martins3/core/zsh/docker-build.sh run.sh
ln: failed to create hard link 'run.sh' => '/home/martins3/core/zsh/docker-build.sh': Invalid cross-device link
```
2. virtiofs 和 9p 可以共享 host 的文件系统给 windows guest 吗?
可以的

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
