## 为什么内部的 ssh 有时候需要添加上这个

TERM=xterm-256color  ssh

似乎是我的 tmux 的原因?

### 为什么 ssh 会报告 xterm-ghostty 不存在?
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

## osc52

## terminfo 解决了一个

• 没有固定的“全部类型”枚举。 你说的类型，对应 terminfo 数据库中的“终端描述条目”，例如
  xterm-256color。条目可以新增、定制，也可以有别名和不同能力的变体。格式说明 (https://invisible-island.net/ncurses/man/terminfo.5.html)

  我查了这台机器，toe -a 列出的主名称去重后有 131 个，其中也包含供其他条目复用的片段。主要可以这样分类：

   类别              本机条目示例                                   描述什么
  ━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   历史硬件终端      vt52、vt100、vt102、vt220                      DEC 等实体终端的能力
  ────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
   系统控制台        linux、cons25、sun                             Linux、FreeBSD 等系统的控制台
  ────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
   xterm 系列        xterm、xterm-256color、xterm-direct            xterm 及不同能力配置
  ────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
   现代终端模拟器    xterm-kitty、xterm-ghostty、alacritty、foot    对应模拟器的能力描述
  ────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
   其他终端模拟器    putty、konsole、rxvt-unicode、st               各自实现的终端能力
  ────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
   终端复用器        screen、screen-256color、tmux-256color         Screen、tmux 向内部程序提供的虚拟终端
  ────────────────  ─────────────────────────────────────────────  ───────────────────────────────────────
   最简终端          dumb                                           几乎没有光标定位等高级能力

  这些名字不代表完全不同的协议。许多条目的控制序列高度相似，区别可能只是颜色数量、功能键编码、光标样式或部分操作是否支持。

  例如本机同时存在：

  xterm             基础描述
  xterm-256color    256 色变体
  xterm-direct     直接指定 RGB 颜色的变体

  我当前执行命令的环境是 TERM=tmux-256color，意味着程序应按照 tmux 提供的终端能力来输出。

  你可以直接查看完整列表和具体内容：

  toe -a                            # 列出可找到的条目
  infocmp xterm-256color             # 查看一个条目的能力
  infocmp xterm-256color tmux-256color # 比较两个条目的差异


这里可以执行一个 dree 的
/usr/share/terminfo


### termios 标志位

```c
// c_iflag: 输入模式
IGNBRK  // 忽略 break
BRKINT  // break 产生 SIGINT
IGNPAR  // 忽略帧/奇偶错误
INPCK   // 启用输入校验
ISTRIP  // 剥离第 8 位
INLCR   // 将 NL 映射为 CR
IGNCR   // 忽略 CR
ICRNL   // 将 CR 映射为 NL
IXON    // 启用 XON/XOFF 输出流控
IXOFF   // 启用 XON/XOFF 输入流控

// c_oflag: 输出模式
OPOST   // 启用输出处理
ONLCR   // 将 NL 映射为 CR-NL
OCRNL   // 将 CR 映射为 NL
ONOCR   // 第 0 列不输出 CR
ONLRET  // NL 执行 CR 功能

// c_cflag: 控制模式
CSIZE   // 字符大小掩码
    CS5, CS6, CS7, CS8
CSTOPB  // 2 个停止位
CREAD   // 启用接收
PARENB  // 启用校验
PARODD  // 奇校验
HUPCL   // 最后关闭时挂起
CLOCAL  // 忽略调制解调器状态线

// c_lflag: 本地模式
ISIG    // 启用信号 (INTR, QUIT, SUSP)
ICANON  // 启用规范模式
ECHO    // 回显输入字符
ECHOE   // 回显擦除字符为 BS-SP-BS
ECHOK   // 在 KILL 字符后回显 NL
ECHONL  // 即使 ECHO 关闭也回显 NL
NOFLSH  // 信号产生时不刷新输入输出
IEXTEN  // 启用扩展实现定义函数
```

### 特殊控制字符 (c_cc)

| 索引 | 名称 | 默认 | 说明 |
|------|------|------|------|
| VINTR | Ctrl+C | `^C` | 发送 SIGINT |
| VQUIT | Ctrl+\ | `^\` | 发送 SIGQUIT |
| VERASE | Backspace | `^?` | 擦除前一个字符 |
| VKILL | Ctrl+U | `^U` | 擦除整行 |
| VEOF | Ctrl+D | `^D` | 文件结束 |
| VEOL | - | - | 行结束（备用）|
| VSTART | Ctrl+Q | `^Q` | 恢复输出 |
| VSTOP | Ctrl+S | `^S` | 停止输出 |
| VSUSP | Ctrl+Z | `^Z` | 发送 SIGTSTP |
| VWERASE | Ctrl+W | `^W` | 擦除单词 |

## 为什么会有这个效果

应该才这个 demo 分析开始:

```c
printf("\033[H\033[J"); // 清屏
```

## 参考
printf是怎么输出到控制台的呢？ - 闪光吧Linux的回答 - 知乎
https://www.zhihu.com/question/456916638/answer/3099313413

## 为什么有的环境中，必须添加上 xterm-256color
cmd="TERM=xterm-256color ssh $ssh_port $ssh_user@$ssh_ip"

## 有趣
在Linux中，按上下左右键为什么变成^[[A^[[B^[[C^[[D？ - 海念着梦与夜的回答 - 知乎
https://www.zhihu.com/question/31429658/answer/3601508760


## XTERM=xterm 和 XTERM=xterm-ghostty 的区别是什么?
主要区别是：它们让程序读取不同的“终端能力说明
书”（terminfo），从而决定使用什么颜色、控制序列和按键处
理方式。

TERM 是告诉 Vim、less、top 等程序“当前终端是什么类型”。
把 Ghostty 的 TERM 改成 xterm 后，运行的依然是
Ghostty，但程序会按 xterm 的能力来使用它。

我对比了你本机的 terminfo：

 能力声明       调色板颜色数
 xterm          8 色
 xterm-ghostty  256 色
───────────────────────────────────────────────────────
 能力声明       RGB 真彩色
 xterm          没有显式声明
 xterm-ghostty  有 Tc 和 RGB 控制序列
───────────────────────────────────────────────────────
 能力声明       下划线
 xterm          普通下划线
 xterm-ghostty  额外支持样式、颜色
───────────────────────────────────────────────────────
 能力声明       同步刷新
 xterm          没有声明
 xterm-ghostty  有，供程序减少刷新闪烁
───────────────────────────────────────────────────────
 能力声明       按键及屏幕控制序列
 xterm          xterm 的定义
 xterm-ghostty  匹配 Ghostty 的定义

这里描述的是 terminfo 声明的能力；程序也可能通过主动探
测或自己的配置启用更多功能。所以设成 xterm 并不意味着
Ghostty 本身只能显示 8 色。

实际最容易遇到的区别是 SSH 兼容性：远端如果没有
xterm-ghostty 的 terminfo，运行 top、less 等可能报
unknown terminal
type。可以安装对应条目，或回退到更普及的
xterm-256color。Ghostty 官方说明
(https://ghostty.org/docs/help/terminfo)

在 Ghostty 中通常保留默认的 xterm-ghostty；远端不认识它
时，优先用 xterm-256color 兼容。 裸 xterm 声明的颜色能
力比较保守。

另外，xterm-ghostty 名字中的 xterm-
是为了兼容一些直接检查 TERM 是否含有 xterm
的程序。命名原因
(https://ghostty.org/docs/help/terminfo)

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
