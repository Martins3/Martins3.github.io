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

## nfs server 使用的内存如果需要被 swap out ，那么可能会死锁
<!-- 24395fac-6b8d-4c0a-b4ea-a682049293c1 -->

需要首先写一个 nfs server 看看，让 codex 来搞吧

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
