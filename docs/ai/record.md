## 记录一下使用 ai 的感觉

似乎 ai 不会切分任务，一个超级大的，复杂的任务:
ai 会偷懒，尝试在当前上下文中完成

现在每一个任务都很复杂，结果每一个任务都是简单的搞搞而已
```txt
• Used Task (Add verification to sys worktree)
  • Used ReadFile (/home/martins3/vn-worktre...s/docs/kernel/sysv-ipc.md)
  • Used ReadFile
  • Used WriteFile (/home/martins3/vn-worktre...s/docs/kernel/sysv-ipc.md)
  • Used WriteFile
  Failed to run subagent
• Used Task (Add verification to tools worktree)
  • 2 more tool calls ...
  • Used ReadFile
  • Used StrReplaceFile (/home/martins3/vn-worktre...rent/kernel/kernel-doc.md)
  • Used StrReplaceFile
  • Used StrReplaceFile
• Used Task (Add verification to debug worktree)
  • 4 more tool calls ...
  • Used StrReplaceFile
  • Used Shell (grep -n "### 验证方法" /home/...docs/kernel/irq/vector.md)
  • Used ReadFile
  • Used ReadFile
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
