## 基本 io 流程

读的过程，首先确定所在的 mount 的位置，然后找到文件的映射关系

```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - ksys_read
        - vfs_read
          - new_sync_read
            - call_read_iter
              - ovl_read_iter
                - vfs_iter_read
                  - do_iter_read
                    - do_iter_readv_writev
                      - call_read_iter
                        - ext4_file_read_iter
```

## 基本文档
- https://news.ycombinator.com/item?id=21567481
- https://github.com/wagoodman/dive 还存在一个概念，叫作 union fs
- https://news.ycombinator.com/item?id=21569168
- https://medium.com/@knoldus/unionfs-a-file-system-of-a-container-2136cd11a779
- Documentation/filesystems/relay.txt

## TODO
1. 可以对比一下 overlayfs 和 ceph 实现 snapshot 的差异

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
