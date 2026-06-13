# block layer integrity
<!-- ea64e951-c9b9-4370-9014-72f318b3a12f -->

- https://www.kernel.org/doc/html/latest/translations/zh_CN/block/data-integrity.html
- https://www.kernel.org/doc/html/latest/block/data-integrity.html

```txt
2
bio_integrity_alloc
bio_integrity_prep
blk_mq_submit_bio
__submit_bio_noacct_mq
write_sb_page
write_page
md_update_sb.part.0
md_check_recovery
raid1d
md_thread
kthread
ret_from_fork
  4

bio_integrity_alloc
bio_integrity_prep
blk_mq_submit_bio
__submit_bio_noacct_mq
md_update_sb.part.0
md_check_recovery
raid1d
md_thread
kthread
ret_from_fork
  4

bio_integrity_alloc
bio_integrity_prep
blk_mq_submit_bio
__submit_bio_noacct_mq
flush_bio_list
raid1_unplug
flush_plug_callbacks
blk_finish_plug
wb_writeback
wb_check_old_data_flush
wb_do_writeback
wb_workfn
process_one_work
worker_thread
kthread
ret_from_fork
  18

Detaching...
```

文档，实际上，我们发现只有当前只有 raid1 会使用这个机制，而且前提是
nvme 盘支持才可以。


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
