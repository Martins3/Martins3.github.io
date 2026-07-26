# 为什么 ssh 会报告 xterm-ghostty 不存在?
```txt
Web console: https://yyds-fs:9090/ or https://10.0.2.15:9090/

Last login: Fri Jul  3 14:34:00 2026 from 10.0.0.2
~ via C v14.3.0-gcc 📁
    tm
missing or unsuitable terminal: xterm-ghostty
missing or unsuitable terminal: xterm-ghostty
~ via C v14.3.0-gcc 📁
    clearear
'xterm-ghostty': unknown terminal type.
```

显然，我当时是从 ghostty 登录过去的，所以会存在这个报错:

## 原理是什么?
Ghostty
  设置 TERM=xterm-ghostty
        ↓
SSH 把 TERM 传入虚拟机
        ↓
clear/tmux 根据 TERM 查找 terminfo
        ↓
虚拟机没有 xterm-ghostty 的 terminfo
        ↓
报 unknown terminal type

TERM 是什么？

它不是普通的终端名称，而是“终端能力配置名称”。程序通过它判断：

- 如何清屏
- 如何移动光标
- 支持多少颜色
- 功能键会发送什么序列
- 是否支持粗体、下划线等

这些具体规则保存在 terminfo 数据库里。

例如执行 clear 时，它不是固定输出一串字符，而是：

读取 TERM=xterm-ghostty
查找 xterm-ghostty 的 terminfo
获取清屏控制序列
发送给终端

guest 查不到该记录，因此无法清屏。tmux 对终端能力要求更高，会直接拒绝启动。

为什么以前可能正常？

以前常见终端设置的是：

TERM=xterm-256color

几乎所有 Linux 都自带它。Ghostty 使用更精确的 xterm-ghostty，本机有对应 terminfo，但远端虚拟机未必有。

现在的处理只是让 ge 启动 SSH 时声明：

TERM=xterm-256color

它是兼容性降级，不影响本机 Ghostty，只让远端使用一个普遍存在的能力描述。另一种方案是在每台 guest 安装 Ghostty terminfo，但维护成本
更高。


## xterm-ghostty 安装在那里，虚拟机中可以安装 xterm-ghostty 吗?

可以安装。

本机存在这些 terminfo：

/usr/share/terminfo/x/xterm-ghostty
~/.terminfo/x/xterm-ghostty
~/.terminfo/g/ghostty

推荐不要直接复制编译后的文件，而是通过 infocmp 导出、在 guest 用 tic 重新编译：

infocmp -x xterm-ghostty |
    ssh -p 51404 martins3@localhost \
    'mkdir -p ~/.terminfo && tic -x -o ~/.terminfo -'

安装后在 guest 验证：

TERM=xterm-ghostty infocmp xterm-ghostty

生成的位置通常是：

~/.terminfo/x/xterm-ghostty
~/.terminfo/g/ghostty

系统级安装则使用：

sudo tic -x -o /usr/share/terminfo xterm-ghostty.terminfo

注意：目前 ge 强制使用 TERM=xterm-256color。即使安装了 terminfo，ge 仍不会使用 xterm-ghostty；如果决定给 VM 安装 terminfo，就可以移
除刚添加的兼容覆盖。

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
