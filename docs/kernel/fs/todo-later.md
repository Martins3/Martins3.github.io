# 有趣，但是不是主线，有时间可以看看

1. 基本可以猜到，使用 timerfd 就可以来 epoll timer 了，那么问题是 fs/timerfd.c 要写的这么长啊?
1. 如何理解 CONFIG_NFS_SWAP 和 CONFIG_SUNRPC_SWAP
2. smb 和 cifs 是什么关系?
3. afs 居然就是一个分布式文件系统啊?
4. 原来 FAT 和 VFAT 的差别这么大啊

```txt
<M> MSDOS fs support                                                                                     │ │
<M> VFAT (Windows-95) fs support                                                                         │ │
(437) Default codepage for FAT                                                                           │ │
(iso8859-1) Default iocharset for FAT                                                                    │ │
[ ] Enable FAT UTF-8 option by default                                                                   │ │
< > exFAT filesystem support                                                                             │ │
< > NTFS Read-Write file system support                                                                  │ │
< > NTFS file system support
```

## 到时候看看这个吧，感觉没有使用用途吧
CONFIG_ISO9660_FS

## EXPORTFS_BLOCK_OPS 只有 xfs 使用
```txt
config EXPORTFS_BLOCK_OPS
	bool "Enable filesystem export operations for block IO"
	help
	  This option enables the export operations for a filesystem to support
	  external block IO.
```

## 这个做什么的？
kernel/fs/exportfs/exportfs.ko

## 这个 commit 当时想要调查什么来着

```diff
History:   #0
Commit:    a2ad63daa88b9d6846976fd2a0b5e4f5cfc58377
Author:    NeilBrown <neilb@suse.de>
Committer: Andrew Morton <akpm@linux-foundation.org>
Date:      Tue 10 May 2022 09:20:49 AM CST

VFS: add FMODE_CAN_ODIRECT file flag

Currently various places test if direct IO is possible on a file by
checking for the existence of the direct_IO address space operation.
This is a poor choice, as the direct_IO operation may not be used - it is
only used if the generic_file_*_iter functions are called for direct IO
and some filesystems - particularly NFS - don't do this.

Instead, introduce a new f_mode flag: FMODE_CAN_ODIRECT and change the
various places to check this (avoiding pointer dereferences).
do_dentry_open() will set this flag if ->direct_IO is present, so
filesystems do not need to be changed.

NFS *is* changed, to set the flag explicitly and discard the direct_IO
entry in the address_space_operations for files.

Other filesystems which currently use noop_direct_IO could usefully be
changed to set this flag instead.

Link: https://lkml.kernel.org/r/164859778128.29473.15189737957277399416.stgit@noble.brown
Reviewed-by: Christoph Hellwig <hch@lst.de>
Signed-off-by: NeilBrown <neilb@suse.de>
Tested-by: David Howells <dhowells@redhat.com>
Tested-by: Geert Uytterhoeven <geert+renesas@glider.be>
Cc: Hugh Dickins <hughd@google.com>
Cc: Mel Gorman <mgorman@techsingularity.net>
Cc: Trond Myklebust <trond.myklebust@hammerspace.com>
Cc: Miaohe Lin <linmiaohe@huawei.com>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
```

xfs 的
```diff
 History:   #0
 Commit:    b294349993716ca67c8bd2183c7d43c28df481fc
 Author:    Christoph Hellwig <hch@lst.de>
 Committer: Darrick J. Wong <djwong@kernel.org>
 Date:      Tue 13 Jun 2023 09:09:23 AM CST

 xfs: set FMODE_CAN_ODIRECT instead of a dummy direct_IO method

 Since commit a2ad63daa88b ("VFS: add FMODE_CAN_ODIRECT file flag") file
 systems can just set the FMODE_CAN_ODIRECT flag at open time instead of
 wiring up a dummy direct_IO method to indicate support for direct I/O.
 Do that for xfs so that noop_direct_IO can eventually be removed.

 Signed-off-by: Christoph Hellwig <hch@lst.de>
 Reviewed-by: Dave Chinner <dchinner@redhat.com>
 Reviewed-by: Darrick J. Wong <djwong@kernel.org>
 Signed-off-by: Darrick J. Wong <djwong@kernel.org>

 diff --git a/fs/xfs/xfs_file.c b/fs/xfs/xfs_file.c
 index aede746541f8..605587fefbd3 100644
 --- a/fs/xfs/xfs_file.c
 +++ b/fs/xfs/xfs_file.c
 @@ -1172,7 +1172,7 @@ xfs_file_open(
  	if (xfs_is_shutdown(XFS_M(inode->i_sb)))
  		return -EIO;
  	file->f_mode |= FMODE_NOWAIT | FMODE_BUF_RASYNC | FMODE_BUF_WASYNC |
 -			FMODE_DIO_PARALLEL_WRITE;
 +			FMODE_DIO_PARALLEL_WRITE | FMODE_CAN_ODIRECT;
  	return generic_file_open(inode, file);
  }

```


基本的测试方法:
```diff
History:        #0
Commit:         5ee3d10f84d0a32fc11a55c70c204b6d81fd9ef6
Author:         Dave Wysochanski <dwysocha@redhat.com>
Committer:      Anna Schumaker <Anna.Schumaker@Netapp.com>
Author Date:    Fri 10 Jun 2022 08:46:29 AM CST
Committer Date: Thu 16 Jun 2022 03:03:12 AM CST

NFSv4: Add FMODE_CAN_ODIRECT after successful open of a NFS4.x file

Commit a2ad63daa88b ("VFS: add FMODE_CAN_ODIRECT file flag")
added the FMODE_CAN_ODIRECT flag for NFSv3 but neglected to add
it for NFSv4.x.  This causes direct io on NFSv4.x to fail open
with EINVAL:
  mount -o vers=4.2 127.0.0.1:/export /mnt/nfs4
  dd if=/dev/zero of=/mnt/nfs4/file.bin bs=128k count=1 oflag=direct
  dd: failed to open '/mnt/nfs4/file.bin': Invalid argument
  dd of=/dev/null if=/mnt/nfs4/file.bin bs=128k count=1 iflag=direct
  dd: failed to open '/mnt/dir1/file1.bin': Invalid argument

Fixes: a2ad63daa88b ("VFS: add FMODE_CAN_ODIRECT file flag")
Signed-off-by: Dave Wysochanski <dwysocha@redhat.com>
Signed-off-by: Anna Schumaker <Anna.Schumaker@Netapp.com>

diff --git a/fs/nfs/dir.c b/fs/nfs/dir.c
index a8ecdd527662..0c4e8dd6aa96 100644
--- a/fs/nfs/dir.c
+++ b/fs/nfs/dir.c
@@ -2124,6 +2124,7 @@ int nfs_atomic_open(struct inode *dir, struct dentry *dentry,
 		}
 		goto out;
 	}
+	file->f_mode |= FMODE_CAN_ODIRECT;

 	err = nfs_finish_open(ctx, ctx->dentry, file, open_flags);
 	trace_nfs_atomic_open_exit(dir, ctx, open_flags, err);

```

从这个 patch 看，为什么连 inode_operations 的 open 也是需要注册的?

```c
static const struct inode_operations nfs4_dir_inode_operations;
```

- https://lore.kernel.org/all/20230612053537.585525-1-hch@lst.de/
  - ceph
- https://lore.kernel.org/all/20230613054700.GA14648@lst.de/
  - ext4

## exportfs 是一个公共的库
```txt
🤒  rg "select EXPORTFS"
init/Kconfig
1585:   select EXPORTFS

fs/xfs/Kconfig
5:      select EXPORTFS

fs/bcachefs/Kconfig
5:      select EXPORTFS

fs/overlayfs/Kconfig
5:      select EXPORTFS

fs/notify/fanotify/Kconfig
5:      select EXPORTFS

fs/nfsd/Kconfig
9:      select EXPORTFS
100:    select EXPORTFS_BLOCK_OPS
113:    select EXPORTFS_BLOCK_OPS
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
