## nvim 为什么必须借助 tmux 才可以拷贝，

ssh 过去， 打开 nvim 就无法拷贝了?

对于 zellij 没有什么配置，发现默认也是不能拷贝的


## 你是否感觉到恐惧了

codex resume 01a105ba-2dd0-7011-8870-64f46edbd279


已经修复并热加载，不用重启 tmux 或注销 GNOME。修改在 /home/martins3/.dotfiles/config/tmux.conf:91，清理了 38 个异常旧进程；隔离测试中连续三次复制都输出
了正确的 OSC 52 数据。

这个问题有趣的地方在于：一个命令行复制工具，为了操作剪贴板，实际创建了 GUI 窗口。

1. 为什么 tmux 会启动它

你的 tmux-yank 插件会自动选择复制工具。检测代码 (/home/martins3/.tmux/plugins/tmux-yank/scripts/helpers.sh:138) 发现系统存在 wl-copy，就使用它，但没有
检查当前桌面下它能否可靠工作。

之前实际生效的绑定相当于：

按 y 或结束鼠标拖选
    → tmux copy-pipe
    → 启动新的 wl-copy
    → 将选中文字通过标准输入交给它

每次复制都会启动独立进程。现场的 38 个旧进程，父进程全部是 tmux，环境变量也指向对应 tmux 会话，来源很明确。

2. 命令行工具为什么需要窗口

普通 Wayland 剪贴板操作受到输入焦点和输入事件序号等约束。终端窗口拥有焦点，并不意味着终端启动的任意子进程也拥有这份权限：wl-copy 连接 Wayland 后，是另
一个独立客户端。

在无法使用 data-control 接口直接操作剪贴板时，wl-copy 会走一条兼容路径：

1. 创建临时窗口。
2. 等窗口获得键盘焦点。
3. 利用焦点事件的 serial 设置剪贴板。
4. 设置完成后销毁窗口。

源码中，copy_action_init() 注册 on_focus 回调；只有执行该回调后，才会进入 do_set_selection()，随后销毁临时窗口。复制流程源码
(https://github.com/bugaevc/wl-clipboard/blob/e808203/src/types/copy-action.c)

3. 为什么窗口看不见，Dock 却有图标

这个窗口只有 1×1 像素，完全透明，但在窗口系统里仍然是真实窗口。源码通过 xdg_surface_get_toplevel() 创建顶层窗口，并设置：

窗口标题：wl-clipboard
应用 ID：io.github.bugaevc.wl-clipboard

因此，“透明”仅意味着像素不可见，并不意味着 GNOME 不把它当作一个应用窗口。透明窗口源码
(https://github.com/bugaevc/wl-clipboard/blob/e808203/src/types/popup-surface.c)、窗口身份源码
(https://github.com/bugaevc/wl-clipboard/blob/e808203/src/types/shell-surface.c)

如果 GNOME 没有把焦点交给它，后面的设置剪贴板、销毁窗口流程就无法推进。旧窗口留下来，下一次复制又创建新窗口，Dock
中的条目便不断累积。上游手册明确记录了这种“临时窗口拿不到焦点导致挂起”的问题。上游说明
(https://github.com/bugaevc/wl-clipboard/blob/master/data/wl-clipboard.1#L173-L179)

现场特征与这条路径吻合；不过，没有采集旧进程的完整 Wayland 事件日志，因此还不能进一步确定 GNOME 当时为什么没有授予焦点。

4. 为什么不能把所有常驻 wl-copy 都当作异常

剪贴板通常采用“声明数据来源，需要粘贴时再传输”的方式。成功复制后，wl-copy 可以转到后台，继续为其他应用提供数据；它不一定应该立即退出。

源码中，成功设置剪贴板后才会执行以下后台化操作：

• 标准输入、输出切换到 /dev/null。
• 工作目录切换到 /。
• fork()，原进程退出，后台子进程继续服务。后台化源码 (https://github.com/bugaevc/wl-clipboard/blob/e808203/src/wl-copy.c)

这恰好解释了现场的区别：

 特征              已清理的旧进程    保留的一个新进程
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━
 父进程            tmux              用户 systemd
────────────────  ────────────────  ──────────────────────
 工作目录          /home/martins3    /
────────────────  ────────────────  ──────────────────────
 标准输入、输出    tmux 的 socket    /dev/null
────────────────  ────────────────  ──────────────────────
 所处阶段          尚未完成后台化    已完成设置后的后台化

所以清理的是 38 个异常旧进程。今天新启动的那个符合正常剪贴板服务进程特征，我保留了它，以免影响当前剪贴板内容。

5. OSC 52 为什么能解决

现在配置增加了：

set -g @override_copy_command 'tmux load-buffer -w -'

插件快捷键仍然保留，复制路径变为：

┌─────────────────┐  ┌───────────────────────┐  ┌─────────────────┐  ┌─────────────────────┐  ┌────────────┐
│ tmux 中选择文字 │  │ tmux load-buffer -w - │  │ OSC 52 控制序列 │  │ 已有的 Ghostty 窗口 │  │ 系统剪贴板 │
└┬────────────────┘  └┬─┬────────────────────┘  └┬─┬──────────────┘  └┬─┬──────────────────┘  └┬───────────┘
 │                    ▲ │                        ▲ │                  ▲ │                      ▲
 │                    │ │                        │ │                  │ │                      │
 │                    │ │                        │ │                  │ │                      │
 │                    │ │                        │ │                  │ │                      │
 └────────────────────┘ │                        │ │                  │ │                      │
                        │                        │ │                  │ │                      │
                        └────────────────────────┘ │                  │ │                      │
                                                   │                  │ │                      │
                                                   └──────────────────┘ │                      │
                                                                        │                      │
                                                                        └──────────────────────┘

load-buffer 从标准输入读取文字，-w 要求 tmux 同时通过终端控制序列写入剪贴板。OSC 52 大致是：

ESC ] 52 ; c ; Base64编码的文字 BEL

Ghostty 识别它后，由已有的终端 GUI 客户端完成复制。这样无需再启动独立 Wayland 客户端、创建透明窗口并等待它获得焦点。tmux 官方说明
(https://github.com/tmux/tmux/wiki/Clipboard)

你原本已经启用了 set-clipboard on，Ghostty 的 Ms 能力也已正确识别；这次主要是让插件使用已有的这条通道。y、鼠标复制，以及复制后停留在复制模式的行为都保
留了。

至于 /home/martins3/data/vn/docs/linux/fedora/doc.md:162，它记录的旧 Flameshot 方案也使用过 wl-copy，但相关脚本和服务已经移除。此次直接触发点是 tmux 插
件；没有证据表明这些进程仍由旧截图方案产生。


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
