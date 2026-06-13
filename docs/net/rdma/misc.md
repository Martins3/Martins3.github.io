## qemu

- `rdma_start_outgoing_migration` : 也是 socket fd 等方法中的一种

- 用于传递什么?
    - 物理内存

- 参考 qemu/docs/rdma.txt 来理解 rdma.c 中的内容。

### 为什么需要在其中需要重新增加一个 QIOChannel
- [ ] QIOChannel 是在给 QEMU 使用的吗?

- `rdma_start_incoming_migration`
    - `qemu_set_fd_handler` 设置 handler 为 `rdma_accept_incoming_migration`
        - `rdma_accept_incoming_migration`
            - `qemu_rdma_accept` ：TODO 全部都是 RDMA 的相关领域的内容
            - `qemu_fopen_rdma` ：返回的 QEMUFile 将会持有这个 QIOChannelRDMA
                - 创建 `QIOChannelRDMA`
                - `qemu_file_new_output`
                - 设置 `QEMUFileHooks`
            - `migration_fd_process_incoming` : 这是常规流程


## 探索与解决：Intel E810网卡的4ms单并发IO延迟问题
https://zhuanlan.zhihu.com/p/735757082


## 一些很老的代码
https://github.com/zrlio/softiwarp
https://github.com/animeshtrivedi/rdma-example

## 1.5 TB of VRAM on Mac Studio - RDMA over Thunderbolt 5
https://news.ycombinator.com/item?id=46319657

RDMA 和 VRAM ？

什么东西是 VRAM ?

## 字节的 rdma
https://www.kad8.com/network/bytedance-veroce-makes-rdma-work-on-lossy-networks
https://mp.weixin.qq.com/s/AHWtHnIQWUhIU1NRN5TsPw

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
