# 未来的计划

## 拷贝 linux tree 到 simplefs 中大部分实际都是在查询
```txt
- 61.82% lookup_open.isra.0                                                                                                   ▒
   - 39.89% simplefs_create                                                                                                   ▒
      - 39.76% simplefs_create_internal.isra.0                                                                                ▒
         - 34.80% simplefs_get_folio                                                                                          ▒
            - 24.79% simplefs_journal_prepare_current                                                                         ▒
               - 20.00% bdev_getblk                                                                                           ▒
                  - 15.47% find_get_block_common                                                                              ▒
                     - 9.25% __filemap_get_folio_mpol                                                                         ▒
                        - 9.07% filemap_get_entry                                                                             ▒
                           + 4.59% xas_load                                                                                   ▒
                             1.91% lock_acquire                                                                               ▒
                             1.22% lock_release                                                                               ▒
                             0.77% lock_is_held_type                                                                          ▒
                     - 1.02% __might_resched                                                                                  ▒
                          0.69% lock_is_held_type                                                                             ▒
                  - 2.08% fs_reclaim_acquire                                                                                  ▒
                       lock_acquire                                                                                           ▒
                    1.31% lock_release                                                                                        ▒
                  - 0.93% __might_resched                                                                                     ▒
                       0.65% lock_is_held_type                                                                                ▒
               + 4.49% jbd2_journal_get_write_access                                                                          ▒
            - 9.93% do_read_cache_folio                                                                                       ▒
               + 9.72% __filemap_get_folio_mpol                                                                               ▒
         + 2.66% simplefs_new_inode                                                                                           ▒
   - 21.61% simplefs_lookup                                                                                                   ▒
      - 9.84% simplefs_get_folio                                                                                              ▒
         - 9.70% do_read_cache_folio                                                                                          ▒
            - 9.55% __filemap_get_folio_mpol                                                                                  ▒
               - filemap_get_entry                                                                                            ▒
                  - 4.58% xas_load                                                                                            ▒
                       2.20% lock_is_held_type                                                                                ▒
                     - 0.82% xas_start                                                                                        ▒
                          0.54% lock_is_held_type                                                                             ▒
                       0.77% rcu_read_lock_held                                                                               ▒
                    1.92% lock_acquire                                                                                        ▒
                    1.25% lock_release                                                                                        ▒
                    0.73% lock_is_held_type
```

## 拷贝完成之后，为什么还是有大量的 buffers
利用 htop 观察到的

## 容量限制是什么?

1. inode 数量
2. 文件大小
3. 支持的磁盘的大小

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
