# sysfs fs

## /proc/fs

```txt
🧀  tree
.
├── ext4
│   └── nvme0n1p1
│       ├── es_shrinker_info
│       ├── fc_info
│       ├── mb_groups
│       ├── mb_stats
│       ├── mb_structs_summary
│       └── options
├── jbd2
│   └── nvme0n1p1-8
│       └── info
├── lockd
│   └── nlm_end_grace
├── nfs
│   └── exports
├── nfsd
│   ├── clients  [error opening dir]
│   ├── export_features
│   ├── exports
│   ├── export_stats
│   ├── filecache
│   ├── filehandle
│   ├── max_block_size
│   ├── max_connections
│   ├── nfsv4gracetime
│   ├── nfsv4leasetime
│   ├── nfsv4recoverydir
│   ├── pool_stats
│   ├── pool_threads
│   ├── portlist
│   ├── reply_cache_stats
│   ├── supported_krb5_enctypes -> /proc/net/rpc/gss_krb5_enctypes
│   ├── threads
│   ├── unlock_filesystem
│   ├── unlock_ip
│   ├── v4_end_grace
│   └── versions
└── xfs
    ├── stat -> /sys/fs/xfs/stats/stats
    ├── xqm
    └── xqmstat

9 directories, 32 files
```

/sys/fs 中信息更多，当然在 /sys/fs 最重要的就是 cgroup

当然 fs options 还是可以从 fsstat 中获取的，fsstat 应该走的是 ioctl 的。

## /proc/sys/fs
- https://docs.kernel.org/admin-guide/sysctl/fs.html

## /sys/fs/

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
