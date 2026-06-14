## tencentos 的分支
```txt
  tag 6.6.119-48.3 (c72ccb5d)
      │
      ├─108 commits──┬─ released/6.6.119-49  (0117b514, 6.6.119-49.23)
                     │
                     │    (released 有 43 个提交不在 release 中)
                     │
                     ├─ release  (4eae60d)
                          │
                          ├─695 commits──┬─ devel  (ff48856)
                                         │
                                         │   (devel 和 release 并行发展)
```

具体关系：

1. linux-6.6/devel — 主开发分支，当前最新。在提交 24762d11 之后与 release 分道扬镳，各自并行发展。
2. linux-6.6/release — 发布分支。从 3055f87e 之后与 released/6.6.119-49 分开，继续集成新内容；但后期又与 devel 在 24762d11
   处分歧。
3. linux-6.6/released/6.6.119-49 — 6.6.119-49.x 版本的已发布维护分支。它基于 3055f87e 发展，但走了自己的路径（有 43 个提交是
   release 没有的），当前 HEAD 是 6.6.119-49.23 的发布提交。它不是 release 或 devel 的祖先，而是一个并行的发布快照分支。

总结：
• devel 和 release 都包含了 tag 6.6.119-48.3。
• released/6.6.119-49 是一个独立的已发布版本维护线，专门用于 49.x 系列的后续小版本发布。
• devel 和 release 是两条并行活跃的分支，各自发展。


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
