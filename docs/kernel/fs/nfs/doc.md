# nfs rfc
https://www.rfc-editor.org/rfc/rfc8881.html

详细状态恢复，分布式锁，缓存策略，非常的复杂。

## v3 vs v4 的差别

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
  NFSv3 更适合“最小用户态 server”

  - v3 是无状态协议，核心就是一组独立 RPC：LOOKUP/GETATTR/READ/WRITE/CREATE/REMOVE。
  - 只要再补一个 mountd 的 MNT，Linux client 就能拿到 root file handle，然后开始发 NFS 请求。
  - 不需要实现 open/close 状态机、lease、clientid、sequence、compound op 这些东西。
  - 对“研究 NFS 基本 IO 路径”来说，v3 足够清楚。

  NFSv4 不适合这个最小目标

  - v4 把 mount 协议合并进 NFS 本身，但代价是协议复杂很多。
  - Linux client 挂载 v4 时会走 COMPOUND 请求，需要支持 PUTROOTFH/LOOKUP/GETFH/GETATTR/OPEN/READ/WRITE/CLOSE/REMOVE/CREATE 等一串操
    作。

  - v4 有 stateful open、stateid、clientid、lease、reclaim、sequence/session 相关语义。哪怕“假实现”，也要骗过 Linux client 的状态检
    查。

  - 如果只想支持基本 io/remove/create，v4 的前置机制会占掉大部分实现量。
```

## udp
nfs 曾经可以基于 tcp ，也可以基于 udp 啊 ，不过 UDP 会有问题，

https://datatracker.ietf.org/doc/draft-ietf-nfsv4-rfc8881bis/01/

~/linux/fs/nfs/Kconfig

```kconfig
  config NFS_DISABLE_UDP_SUPPORT
         bool "NFS: Disable NFS UDP protocol support"
         depends on NFS_FS
         default y
         help
      Choose Y here to disable the use of NFS over UDP. NFS over UDP
      on modern networks (1Gb+) can lead to data corruption caused by
      fragmentation during high loads.
```

rfc8881 中:
https://datatracker.ietf.org/doc/draft-ietf-nfsv4-rfc8881bis/01/
```txt
5.7.  Transport Layers

5.7.1.  REQUIRED and RECOMMENDED Properties of Transports

   NFSv4.1 works over Remote Direct Memory Access (RDMA) and non-RDMA-
   based transports with the following attributes:

   *  The transport supports reliable delivery of data, which NFSv4.1
      requires.  However the possibility of connections breaking is
      addressed in NFSv4.1 by a session-based replay cache to prevent
      the spurious re-execution of non-idempotent requests or modifying
      idempotent requests.

   *  The transport delivers data in the order it was sent.  Ordered
      delivery simplifies detection of transmit errors, and simplifies
      the sending of arbitrary sized requests and responses via the
      record marking protocol [RFC5531].

   Because efficient handling is required when sending large amounts of
   data, congestion control facilities are a significant concern.

   *  When NFSv4.1 is used over an IP-based network protocol, it is
      REQUIRED that the transport provide congestion control.

   *  When NFSv4.1 is used over a non-IP network protocol, it is
      RECOMMENDED that the transport provide congestion control.

   To enhance the possibilities for interoperability, it is strongly
   recommended that NFSv4.1 client and server implementations support
   operation over the TCP transport protocol.

   It is permissible for a connectionless transport to be used under
   NFSv4.1; however, reliable and in-order delivery of data combined
   with congestion control by the connectionless transport is REQUIRED.
   As a consequence, UDP by itself MUST NOT be used as an NFSv4.1
   transport, although transports to be used for NFSv4.1 may be layered
   on UDP.  NFSv4.1 assumes that a client transport address and server
   transport address used to send data over a transport together
   constitute a connection, even if the underlying transport eschews the
   concept of a connection.
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
