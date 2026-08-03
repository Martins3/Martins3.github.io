# 文档细读

## 核心资料
- docs/devel/migration/
	- https://www.qemu.org/docs/master/devel/migration/index.html
	- https://www.qemu.org/docs/master/devel/migration/main.html

```txt
  migration
     best-practices.rst : 两个小技巧
     compatibility.rst
     CPR.rst

     features.rst
     index.rst

     main.rst : 核心架构和流程

     dirty-limit.rst
     mapped-ram.rst
     postcopy.rst

     qatzip-compression.rst
     xbzrle.rst
     qpl-compression.rst
     uadk-compression.rst

     vfio.rst
     virtio.rst
```

其他资料
https://wiki.qemu.org/Features/Migration/Troubleshooting
https://www.qemu.org/docs/master/interop/vhost-user.html#migrating-backend-state



## 重新设计这个模式

```txt
   类别          模式                                  成功条件
  ━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   普通热迁移    precopy、multifd、multifd-zstd        源端 postmigrate，目标端 running
  ────────────  ────────────────────────────────────  ─────────────────────────────────────────────────────────
   后拷贝        postcopy、multifd-postcopy            进入 active 后切 postcopy，最终两端 migration completed
  ────────────  ────────────────────────────────────  ─────────────────────────────────────────────────────────
   文件状态      plain、mapped-ram                     源端停止，文件可恢复且 guest 状态连续
  ────────────  ────────────────────────────────────  ─────────────────────────────────────────────────────────
   在线快照      background-snapshot                   migration completed，但源端必须仍是 running
  ────────────  ────────────────────────────────────  ─────────────────────────────────────────────────────────
   CPR           cpr-reboot、cpr-transfer、cpr-exec    按各自协议验证 QEMU 重建和资源继承
```

## 问题
- 那些没有 touch 的页面，都是如何跳过的?

- [ ] 迁移的时候，guest 没有使用的页不用发送的?
  - 似乎比到 proc/pid/map 下去检查更加好的
    - 怀疑，qemu 中是否实现过这个功能

> 严格说，热迁移里“跳过 balloon 页”主要不是靠传统 balloon inflate 本身完成的，而是靠 virtio-balloon 的 free-page-hint 迁移优化。
>
> 传统 balloon inflate 这条路里，guest 把 PFN 交给 balloon 后，QEMU 在源端只是把对应宿主页做 discard，回收宿主内存
> balloon_inflate_page() 里直接调用 ram_block_discard_range()；底层通常走 madvise(DONTNEED) 或 fallocate(PUNCH_HOLE)，
> 是“把 host backing 扔掉”，不是直接改迁移位图。

既然如此，那么

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
