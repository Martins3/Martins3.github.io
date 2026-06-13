## 3fs
<!-- 8363c49b-5da4-4488-ab7d-c61b5dc71212 -->

pi + dpsk v4 pro 分析 3fs 的 USRBIO 路径是什么样子的:
```txt
用户进程                            Daemon
─────────────────────────────────────────────────────────────

[启动时]                            初始化 RDMA 连接、存储/元数据客户端
                                    启动 IoRing Worker 协程池
                                    启动 Watch 线程

open() ───FUSE──►                  hf3fs_open()
                                     ├ 从 Meta Service 加载文件布局
                                     ├ 注册写 session（如果是写打开）
                                     └ 缓存 RcInode 到本地哈希表

hf3fs_iorcreate4()                  （创建共享内存，通过 FUSE 虚拟目录
                                     symlink 机制登记到 daemon 的
                                     IovTable / IoRingTable）

reg_fd(fd)                          （用户侧通过 statx 拿 inode ID，
                                     daemon 不直接参与）

prep_io()                           写入 ring buffer:
                                      fileIid, bufId, offset, len

submit_ios() ───sem_post──►        Watch 线程被唤醒
                                     ├ jobsToProc() → 收集 SQE
                                     └ enqueue → job queue

                                   IoRing Worker 取出 job
                                     ├ lookupFiles(fileIid) → RcInode
                                     ├ lookupBufs(bufId) → 共享内存 memh
                                     ├ beginWrite() → Meta Service
                                     ├ PioV::addRead/addWrite
                                     │    └ chunkIo: inodeID→chunkId, chainId
                                     ├ executeRead/executeWrite
                                     │    └ StorageClient::batchRead/Write
                                     │         └ RDMA → 存储节点 SSD
                                     ├ finishWrite() → 更新写入状态
                                     └ addCqe() → sem_post(cqeSem)

wait_for_ios() ◄───sem_wait──┘     （数据已在 Iov 共享内存中）
```

基本上是这个意思了，一下子让我想起来一些事情:
1. 这个优化很简单啊， client 要去写的时候，直接将其地址空间的内容共享后后端，然后后端
来直接使用这个 buffer ，显然通过 USRBIO ，那么 client 不可以继续使用  read / write
而是必须使用 prep_io 的接口。 ublk fuse 是为了不去改造 client ，所以没法做这种优化。
2. 将 meta 用 fuse 管理，然后 io 共享内存，这的确是一个好想法。

## TODO
docs/design_notes.md

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
