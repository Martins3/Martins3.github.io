# nfs swap 支持
## 为什么 nfs 需要对于 swap 特殊支持
<!-- 965960c6-2343-45d1-bbbd-e697183aebed -->

(基本是这个意思了)

1. Swap I/O 的两条路径

  在 mm/page_io.c 中可以看到 swap 写入的分支逻辑（行 447-468）：

```txt
  void __swap_writepage(struct folio *folio, struct swap_iocb **swap_plug)
  {
      struct swap_info_struct *sis = __swap_entry_to_info(folio->swap);

      if (data_race(sis->flags & SWP_FS_OPS))
          // 文件系统路径（NFS 使用）
          swap_writepage_fs(folio, swap_plug);
      else if (data_race(sis->flags & SWP_SYNCHRONOUS_IO))
          // 块设备同步路径
          swap_writepage_bdev_sync(folio, sis);
      else
          // 块设备异步路径
          swap_writepage_bdev_async(folio, sis);
  }
```

3. NFS 特有的 Swap 处理

fs/nfs/file.c 中的 nfs_swap_activate()（行 570-605）需要：

• 检查文件空洞：swap 文件不能有空洞（blocks*512 < isize）
• 激活 RPC swap 模式：rpc_clnt_swap_activate() 确保 RPC 层进入 swap 模式
• 注册 swap 范围：add_swap_extent() 将整个文件映射为连续的 swap 空间

4. 网络层的特殊需求

通过 net/sunrpc/clnt.c 中的 rpc_clnt_swap_activate()，NFS swap 需要：

• 保持网络连接活跃
• 处理网络故障时的重试/恢复
• 管理传输缓冲区分配（swap-over-nfs 可能因内存不足而暂时失败）

5. Direct I/O 语义

nfs_swap_rw()（fs/nfs/direct.c:155）使用 direct I/O 路径：

int nfs_swap_rw(struct kiocb *iocb, struct iov_iter *iter)
{
    if (iov_iter_rw(iter) == READ)
        ret = nfs_file_direct_read(iocb, iter, true);
    else
        ret = nfs_file_direct_write(iocb, iter, true);
}

这是必要的，因为 swap I/O 必须绕过页缓存，避免双重缓存和保持数据一致性。

总结

 特性       块设备 Swap    NFS Swap
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 I/O 接口   submit_bio()   a_ops->swap_rw()
 激活检查   简单           检查空洞、RPC 激活
 错误处理   硬件错误       网络超时、重连
 内存压力   直接 I/O       需处理传输缓冲区分配

文件系统参与使得 NFS swap 能够正确处理网络文件系统的复杂性，同时保持与内核 swap 子系统的兼容性。

## NFS_SWAP 打开或者关闭意味着什么?

- 为什么会依赖 CONFIG_SUNRPC_SWAP
- 到底预留的是什么?

```c
	.swap_activate = nfs_swap_activate,
	.swap_deactivate = nfs_swap_deactivate,
	.swap_rw = nfs_swap_rw,
```
注册的 nfs_swap_rw 只有 nfs 一个人使用

```txt
🧀  ag "\.swap_rw"
fs/nfs/file.c
573:    .swap_rw = nfs_swap_rw,
```

就这两个位置:
- swap_write_unplug
- `__swap_read_unplug`


这两个 commit 增加了 page-io.c 来实现的，看上去专门是为了支持 nfs 的
```diff
 History:   #0
 Commit:    2282679fb20bf036a714ed49fadd0230c278a203
 Author:    NeilBrown <neilb@suse.de>
 Committer: Andrew Morton <akpm@linux-foundation.org>
 Date:      Tue 10 May 2022 09:20:49 AM CST

 mm: submit multipage write for SWP_FS_OPS swap-space

 swap_writepage() is given one page at a time, but may be called repeatedly
 in succession.

 For block-device swapspace, the blk_plug functionality allows the multiple
 pages to be combined together at lower layers.  That cannot be used for
 SWP_FS_OPS as blk_plug may not exist - it is only active when
 CONFIG_BLOCK=y.  Consequently all swap reads over NFS are single page
 reads.

 With this patch we pass a pointer-to-pointer via the wbc.  swap_writepage
 can store state between calls - much like the pointer passed explicitly to
 swap_readpage.  After calling swap_writepage() some number of times, the
 state will be passed to swap_write_unplug() which can submit the combined
 request.
```

```diff
 History:   #0
 Commit:    e1209d3a7a67c281260ba9989621060ba7328b8c
 Author:    NeilBrown <neilb@suse.de>
 Committer: Andrew Morton <akpm@linux-foundation.org>
 Date:      Tue 10 May 2022 09:20:48 AM CST

 mm: introduce ->swap_rw and use it for reads from SWP_FS_OPS swap-space

 swap currently uses ->readpage to read swap pages.  This can only request
 one page at a time from the filesystem, which is not most efficient.

 swap uses ->direct_IO for writes which while this is adequate is an
 inappropriate over-loading.  ->direct_IO may need to had handle allocate
 space for holes or other details that are not relevant for swap.

 So this patch introduces a new address_space operation: ->swap_rw.  In
 this patch it is used for reads, and a subsequent patch will switch writes
 to use it.

 No filesystem yet supports ->swap_rw, but that is not a problem because
 no filesystem actually works with filesystem-based swap.
 Only two filesystems set SWP_FS_OPS:
 - cifs sets the flag, but ->direct_IO always fails so swap cannot work.
 - nfs sets the flag, but ->direct_IO calls generic_write_checks()
   which has failed on swap files for several releases.

 To ensure that a NULL ->swap_rw isn't called, ->activate_swap() for both
 NFS and cifs are changed to fail if ->swap_rw is not set.  This can be
 removed if/when the function is added.

 Future patches will restore swap-over-NFS functionality.

 To submit an async read with ->swap_rw() we need to allocate a structure
 to hold the kiocb and other details.  swap_readpage() cannot handle
 transient failure, so we create a mempool to provide the structures.
```


## 为什么 NFS_SWAP 依赖 SUNRPC_SWAP 啊

看似这个 NFS_SWAP 完全找不到其依赖，实则不然
```txt
config NFS_SWAP
	bool "Provide swap over NFS support"
	default n
	depends on NFS_FS && SWAP
	select SUNRPC_SWAP
	help
	  This option enables swapon to work on files located on NFS mounts.
```
因为这个 config 打开了 SUNRPC_SWAP


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
