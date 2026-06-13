## postcopy 真的这有效吗?

为了支持 postcopy 真的付出了好大的代价

block 的热迁移配置一下吧

## 测试一下 live-snapshot 的功能

snapshot_blkdev
snapshot_blkdev_internal
snapshot_delete_blkdev_internal


https://wiki.qemu.org/Features/SnapshottingImprovements#snapshot-create

## 确认一下热迁移的线程模型
<!-- 9ee517c5-92a8-4b2e-88a7-dd74c8bc99da -->

应该是只有一个 thread 的，
也就是

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run

现在记录 migration_iteration_run 的地方真的太多了



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
