# RDMA
内核主要的代码: drivers/infiniband/

## fio 有一个 plugin 叫 fio-engine-rdma

## 看看内核中直接使用 rdma 的编程框架

Documentation/admin-guide/cgroup-v1/rdma.rst

### nfs
Documentation/admin-guide/nfs/nfs-rdma.rst
net/sunrpc/xprtrdma/

### nvme
drivers/nvme/host/rdma.c
drivers/nvme/target/rdma.c

### smb
fs/smb/server/transport_rdma.c
fs/smb/server/transport_rdma.h

### drivers/infiniband/ulp/
都是基于 rdma 的，主要是
- scsi
- Documentation/infiniband/ipoib.rst : 原来通过这个来实现继续使用之前的网络

存储数据的时候，就使用 rdma ，或者说数据面使用 rdma，为什么要搞这么多 nvme over rdma ，
nfs over rdma ， smb over rdma 的。

### misc
net/9p/trans_rdma.c 似乎通过这个可以看内核中，如果使用 rdma ，最开可以如何实现？
net/rds/ib_rdma.c

### rds

net/rds 下，这个比较特殊，像是为数据中心设计的网络协议


## 用 kunpeng 的这个机器来测试 vf 的功能
和 rdma 的功能
enp130s0f0np0 -> ../../devices/pci0000:80/0000:80:04.0/0000:82:00.0/net/enp130s0f0np0

如果是 vf 直通的话，还需要 vf 驱动吗?

虚拟机中使用的驱动会是 pf 还是 vf ?


https://docs.nvidia.com/networking/display/mlnxofedv53100143/single+root+io+virtualization+(sr-iov)


## drivers/infiniband/hw/

基本上每一个巨头都有一个自己的目录了

ionic -> amd
irdma -> intel
https://www.amd.com/en/blogs/2024/transforming-ai-networks-with-amd-pensando-pollar.html

## rdma over mac ?
https://github.com/exo-explore/exo

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
