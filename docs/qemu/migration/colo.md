# colo
- [ ] 所以和迁移又什么关系？
    - 是不是，当 PVM 失败之后
- [ ] 在 PVM 和 SVM 中各自存在一个 client 吧
- [ ] colo 说让 PVM 和 SVM 需要接受相同的数据，如何实现的，能否在代码中找到这些
- [ ] 这个东西在分布式存储出现之后，还有什么意义啊
- [ ] colo 中的 fail over 是如何进行的

## 可以先测试验证一下吗?

## https://wiki.qemu.org/Features/COLO

同时接受 Guest 请求，如果 PVM SVM 的 respone 相同，那么继续，否则进行 machine backup

It consists of a pair of networked physical nodes

## [ ] 似乎是有对应的论文的

## http://events17.linuxfoundation.org/sites/events/files/slides/COLO-status-update.pdf
Existing VM Replication Approaches
- Lock-stepping: Replicating per instruction
    - Execute in parallel for deterministic instructions
    - Lock and step for nondeterministic instructions

- Checkpoint: Replicating per epoch
    - Output is buffered within an epoch
        - Exact machine state matching from external observers

## [ ] 所以 failover 功能仅仅是这里有用吧

## [ ] block replication
https://wiki.qemu.org/Features/BlockReplication

龟龟： https://lists.nongnu.org/archive/html/qemu-devel/2013-10/msg00276.html

奇怪的: qemu/block/replication.c , replication 自己本身成为一个 back 的 backend 。

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
