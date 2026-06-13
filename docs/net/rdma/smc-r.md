# smc
<!-- 01accbdf-dc3e-47a3-80c9-339c3892062b -->

终于知道网络中 smc 的含义了:

ali 的这几个文档写的蛮好的:
- https://help.aliyun.com/zh/ecs/user-guide/smc-instructions
- https://openanolis.cn/sig/high-perf-network/doc/734387077585070781
- https://help.aliyun.com/document_detail/327118.html


SMC = sockets over RDMA / shared memory。
也就是在 socket 接口语义基本不变的前提下，把一部分原本走 TCP 的 SOCK_STREAM 通信升级成更低延迟、更低 CPU 开销的数据通路。

更具体一点：

- 它实现了一个独立的 socket 协议族 AF_SMC，以及 IPPROTO_SMC 的收发、监听、连接、关闭等逻辑。
- SMC-R 这支通过 RDMA，主要是 RoCE，把 TCP 连接“透明升级”为 RDMA 数据面。
- SMC-D 这支走 ISM 设备，是共享内存直连路径。

你可以把这个目录粗略理解为：
- net/smc/af_smc.c:1: SMC socket 主入口
- net/smc/smc_inet.c:21: 向 IPv4/IPv6 注册 IPPROTO_SMC
- net/smc/smc_ib.c:5: RDMA/RoCE 侧基础设施
- net/smc/smc_clc.c:1: 连接协商
- net/smc/smc_tx.c:1 / net/smc/smc_rx.c:1: 数据收发
- net/smc/smc_ism.c:1: SMC-D/ISM 支持


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
