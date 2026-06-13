# git
https://lucasoshiro.github.io/posts-en/2023-02-13-git-debug

```txt
git log -S # You can also use -G instead of -S. This allows you to use a regular expression instead of string.
git log -- <pathspec>
git log -L 10,20:my_file.c # 有趣
git log -L :<function>:<file> # 有趣
```

此外补充一下
1. nvim 中的集成 git blame
2. nvim 中的 git-messager

问题:
1. git grep 和 rg 有什么差别吗?
2. git ls-files 和 fd 有区别？

## stable branch 有所有的分支
https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux.git
```txt
  master
  remotes/origin/HEAD -> origin/master
  remotes/origin/linux-2.6.11.y
  remotes/origin/linux-2.6.12.y
  remotes/origin/linux-2.6.13.y
  remotes/origin/linux-2.6.14.y
  remotes/origin/linux-2.6.15.y
  remotes/origin/linux-2.6.16.y
  remotes/origin/linux-2.6.17.y
  remotes/origin/linux-2.6.18.y
  remotes/origin/linux-2.6.19.y
  remotes/origin/linux-2.6.20.y
  remotes/origin/linux-2.6.21.y
  remotes/origin/linux-2.6.22.y
  remotes/origin/linux-2.6.23.y
  remotes/origin/linux-2.6.24.y
  remotes/origin/linux-2.6.25.y
  remotes/origin/linux-2.6.26.y
  remotes/origin/linux-2.6.27.y
  remotes/origin/linux-2.6.28.y
  remotes/origin/linux-2.6.29.y
  remotes/origin/linux-2.6.30.y
  remotes/origin/linux-2.6.31.y
  remotes/origin/linux-2.6.32.y
  remotes/origin/linux-2.6.33.y
```

## vim 中的集成

## 内核的发布规则
https://mp.weixin.qq.com/s/Ocnrw6znF6cf_pp9P_firA

## 对于 kernel commit 进行一个模糊搜索
https://github.com/typesense/showcase-linux-commits-search

这个东西我本地尝试了好几次，
首先，手动尝试了下，还特地的搞了一个虚拟机，最后都失败了。

2026-02-06
qwen 和 gemini 都失败了，继续 claude 来支持一下吧
基本是成功的，但是我们还需要更多的东西

2026-06-03
kimi 只是用了几分钟就解决了:
https://github.com/Martins3/showcase-linux-commits-search

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
