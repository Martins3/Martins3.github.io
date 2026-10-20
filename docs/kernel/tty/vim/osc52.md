# osc 52

osc 52 也只是普通的命令:
```sh
printf '\033]52;c;%s\007' \
    "$(printf 'hello world' | base64 -w0)"
```

这条命令：
1. 将 hello world 编码为 Base64。
2. 用 printf 向 Terminal 输出 OSC 52 Sequence。
3. Terminal 解析 Sequence。
4. Terminal 调用本地操作系统的 Clipboard 接口。
5. 你的剪贴板变成 hello world。

所以，这就解决了一个大问题，在远程拷贝的时候，
如何进入本地的剪切板中。

## 没有 osc52 的痛苦
### tmux
https://github.com/tmux-plugins/tmux-yank 就是一个 workaround ，
引入了很多复杂的问题

tmux-yank 主要解决“tmux 内复制了，
但浏览器等桌面应用粘贴不到”的问题。

tmux 的 paste buffer 和系统剪贴板是两套东西。tmux-yank 自动选择 wl-copy、xsel、xclip、pbcopy
等程序，把选区送进系统剪贴板，并配好键盘、鼠标复制绑定。它还提供几个便利快捷键：官方说明
(https://github.com/tmux-plugins/tmux-yank#configuration)

 快捷键            功能
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━
 prefix y          复制当前正在输入的命令行
────────────────  ──────────────────────────
 prefix Y          复制当前 pane 的工作目录
────────────────  ──────────────────────────
 copy mode 的 y    复制选区到系统剪贴板
────────────────  ──────────────────────────
 copy mode 的 Y    复制选区并粘贴回 pane
────────────────  ──────────────────────────
 鼠标拖选后松开    自动复制

在你现在的 Ghostty + tmux 配置中，它的核心作用已经被原生 OSC 52 替代了：

tmux 选区 → OSC 52 → Ghostty → 系统剪贴板

刚才已经把 y、鼠标复制、Alt-y 改为原生实现。保留插件主要是为了延续 prefix y/Y 等辅助功能；这是保留原有行为的选择，并不代表它仍然必
需。

如果你平时只是进入 copy mode、选择、按 y，这个插件可以直接从配置中移除，同时清理它的选项和遗留绑定。

### vim
https://github.com/ojroques/vim-oscyank

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
