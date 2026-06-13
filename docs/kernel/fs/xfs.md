## xfs
- https://lwn.net/Kernel/Index/#Filesystems-XFS
  - https://www.servethehome.com/an-introduction-to-zfs-a-place-to-start/


```txt
  4)               |  xfs_btree_lookup [xfs]() {
  4)   0.066 us    |    xfs_refcountbt_init_ptr_from_cur [xfs]();
  4)               |    xfs_btree_lookup_get_block [xfs]() {
  4)   0.065 us    |      xfs_btree_ptr_to_daddr [xfs]();
  4)               |      xfs_btree_read_buf_block.constprop.0 [xfs]() {
  4)   0.062 us    |        xfs_btree_ptr_to_daddr [xfs]();
  4)               |        xfs_trans_read_buf_map [xfs]() {
  4)               |          xfs_buf_read_map [xfs]() {
  4)               |            xfs_buf_get_map [xfs]() {
  4)               |              xfs_perag_get [xfs]() {
  4)   0.059 us    |                __rcu_read_lock();
  4)   0.058 us    |                __rcu_read_unlock();
  4)   0.277 us    |              }
  4)   0.059 us    |              __rcu_read_lock();
  4)   0.056 us    |              __rcu_read_unlock();
  4)               |              xfs_buf_find_lock [xfs]() {
  4)               |                xfs_buf_lock [xfs]() {
  4)               |                  down() {
  4)   0.057 us    |                    __cond_resched();
  4)   0.074 us    |                    _raw_spin_lock_irqsave();
  4)   0.067 us    |                    _raw_spin_unlock_irqrestore();
  4)   0.460 us    |                  }
  4)   0.566 us    |                }
  4)   0.680 us    |              }
  4)   0.062 us    |              xfs_perag_put [xfs]();
  4)   1.598 us    |            }
  4)   1.713 us    |          }
  4)   1.823 us    |        }
  4)               |        xfs_btree_set_refs [xfs]() {
  4)   0.059 us    |          xfs_buf_set_ref [xfs]();
  4)   0.176 us    |        }
  4)   2.293 us    |      }
  4)   0.155 us    |      xfs_btree_setbuf [xfs]();
  4)   2.833 us    |    }
  4)               |    xfs_lookup_get_search_key [xfs]() {
  4)   0.318 us    |      xfs_btree_key_offset [xfs]();
  4)   0.428 us    |    }
```

看上去还是会访问磁盘的
```txt
@[
    __xfs_buf_submit+233
    xfs_buf_read_map+301
    xfs_buf_readahead_map+60
    xfs_btree_readahead_sblock.isra.0+141
    xfs_btree_increment+68
    xfs_refcount_find_shared+479
    xfs_reflink_find_shared+137
    xfs_reflink_trim_around_shared+217
    xfs_buffered_write_iomap_begin+1102
    iomap_iter+381
    iomap_file_buffered_write+147
    xfs_file_buffered_write+135
    io_write+284
    io_issue_sqe+96
    io_submit_sqes+490
    __do_sys_io_uring_enter+1489
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 2
```

才意识到文件系统也是存在对下屏蔽错误的操作:
```txt
/sys/fs/xfs/nvme1n1/error/metadata/EIO/max_retries
/sys/fs/xfs/nvme1n1/error/metadata/ENODEV/max_retries
/sys/fs/xfs/nvme1n1/error/metadata/default/max_retries
/sys/fs/xfs/nvme1n1/error/metadata/ENOSPC/max_retries
```

## 原来 xfs 的工具

```txt
xfs_admin      xfs_db         xfs_freeze     xfs_info       xfs_mdrestore  xfs_ncheck     xfs_rtcp
xfs_spaceman
xfs_bmap       xfsdist.bt     xfs_fsr        xfs_io         xfs_metadump   xfs_quota      xfs_scrub
xfs_copy       xfs_estimate   xfs_growfs     xfs_logprint   xfs_mkfile     xfs_repair     xfs_scrub_all
```

```txt
xfs_info .
meta-data=/dev/nvme1n1p2         isize=512    agcount=4, agsize=48837846 blks
         =                       sectsz=512   attr=2, projid32bit=1
         =                       crc=1        finobt=1, sparse=1, rmapbt=1
         =                       reflink=1    bigtime=1 inobtcount=1 nrext64=1
data     =                       bsize=4096   blocks=195351382, imaxpct=25
         =                       sunit=0      swidth=0 blks
naming   =version 2              bsize=4096   ascii-ci=0, ftype=1
log      =internal log           bsize=4096   blocks=95386, version=2
         =                       sectsz=512   sunit=0 blks, lazy-count=1
```

无法降级，而且还默认打开，太坑了

https://man7.org/linux/man-pages/man8/xfs_admin.8.html

> nrext64
>    Upgrade a filesystem to support large per-inode extent
>    counters. The maximum data fork extent count will be
>    2^48 - 1, while the maximum attribute fork extent count
>    will be 2^32 - 1. The filesystem cannot be downgraded
>    after this feature is enabled. Once enabled, the
>    filesystem will not be mountable by older kernels.
>    This feature was added to Linux 5.19.

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
