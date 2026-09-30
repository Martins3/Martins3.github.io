# mini-vim：三个终端绘制后端

一个用 C 实现的 Vim 风格教学编辑器。同一份编辑逻辑可以切换三种绘制后端：

- `ansi`：保留原来的实现，手写 xterm 风格控制序列，整帧输出。
- `terminfo`：用终端能力数据库生成控制序列，仍然整帧输出。
- `curses`：使用 ncursesw 的窗口和屏幕更新 API，由库计算差量输出。

三者仍然通过 PTY 把字节交给终端模拟器，没有 GUI 库或 Vim 插件。
共同使用 `termios` 控制输入方式、`read()` 接收字节、
`ioctl(TIOCGWINSZ)` 查询尺寸、`SIGWINCH` 通知重绘。

已有的 `vim.md` 保留为背景说明，程序实现在 `mini-vim.c`。

## 编译和运行

```bash
cd /home/martins3/data/vn/docs/kernel/tty/vim
make
./mini-vim.out example.txt
./mini-vim.out --backend terminfo example.txt
./mini-vim.out --backend curses example.txt
```

默认后端是 `ansi`，也可以显式传 `--backend ansi`。状态栏显示当前后端。
构建需要 C 编译器、`pkg-config`、ncursesw 和 tinfo 开发库。
本机默认 `cc` 来自 Nix，但 `pkg-config` 找到的是系统库；在这里使用：

```bash
make CC=/usr/bin/cc
```

若只想构建不依赖这两个库的原始实验，可以运行：

```bash
make mini-vim-ansi.out
./mini-vim-ansi.out example.txt
```

在 Kitty、Ghostty、WezTerm 等支持 xterm 风格控制序列的终端中运行。
不需要 root，不创建额外 PTY，直接使用启动它的终端。
terminfo 和 curses 依赖正确的 `$TERM` 及对应数据库条目。
数据库缺失或 `TERM=dumb` 时会报错并退出，不会悄悄回退到 ANSI。

可以先在示例里编辑，再用 `:w /tmp/my-tty-demo.txt` 保存副本，避免覆盖示例。
不传文件名则打开空缓冲区，之后也可以用 `:w filename` 保存。

## 实验零：这两个库分别优化了什么？

terminfo 是终端能力数据库及其访问接口，这里由 `libtinfo` 提供；
curses 是更高层的终端界面 API，这里选择支持宽字符的 `ncursesw` 实现。
它们不是两个互相排斥的技术：ncurses 本身也会使用 terminfo。

| 责任 | ansi | terminfo | curses / ncursesw |
| --- | --- | --- | --- |
| 文档、光标位置、模式、滚动与布局 | 共用 `mini-vim.c` | 相同 | 相同 |
| 控制序列从哪里来 | 代码中的字符串 | `setupterm()` / `tigetstr()` 查 `$TERM` | ncurses 内部查终端能力 |
| 移动光标 | 拼 `ESC[行;列H` | `tparm(cup, ...)` / `tputs()` | `wmove()` |
| 清行、样式、文字 | 手写序列并追加字节 | 查 `el` / `sgr0` / `setaf` 等能力 | `wclrtoeol()` / `wattrset()` / `waddnstr()` |
| 哪些内容真的要发送 | 应用发送完整可见区域 | 应用发送完整可见区域 | `wnoutrefresh()` + `doupdate()` 比较屏幕状态 |
| 应用是否维护输入解析与 raw 模式 | 是 | 是 | 是，本实验仅替换绘制后端 |

运行三个版本，分别按一次 `l`，再按没有绑定操作的 `?`。画面应该基本一样，
但查看输出可以发现差别。下面是本机 `TERM=xterm-256color`、`24×80`、
打开 `example.txt` 时从 PTY 捕获的字节数，**不是函数调用次数或 CPU 耗时**：

| 后端 | 启动及首帧 | 按一次 `l` | 随后按一次 `?` |
| --- | ---: | ---: | ---: |
| ansi | 1080 | 1064 | 1064 |
| terminfo | 1173 | 1148 | 1148 |
| curses | 961 | 30 | 0 |

这里的 terminfo 序列含有额外的属性复位等操作，输出反而稍多。
**terminfo 改善的是终端适配，不会自动减少重绘量。**
curses 在移动光标时只更新必要的光标和状态栏内容；没有视觉变化时可以完全不输出。
这不意味着 curses 的 CPU 成本为零：公共布局仍然遍历可见区域，库也需要比较屏幕。
具体字节数会随窗口大小、终端条目和 ncurses 版本变化。

可重复运行同一组操作，同时检查三个后端的终端文本画面和保存结果：

```bash
make CC=/usr/bin/cc check
```

测试额外需要支持 `jobstart(..., {term=true})` 的 Neovim（本机为 0.12）。
`test-backends.lua` 使用 Neovim 的终端解析器和真实 PTY，覆盖中文分段输入、
控制字节可视化、粘贴、长行和长命令滚动、保存、极小窗口、Ctrl-C 和异常 `$TERM`。

### 对代码复杂度的影响

`backend.h` 中 `struct backend` 只抽出绘制需要的操作，`draw_screen()`
仍负责“这一行应该显示什么”。编辑器不再直接拼光标、颜色和清行序列。

- 看 `backend-ansi.c` 中 `position()`：原来的手写协议完整保留，可逐字节学习。
- 看 `backend-terminfo.c` 中 `start()`、`position()`、`put_cap()`：
  数据库查询代替协议字符串；应用仍负责帧缓冲和刷新策略。
  示例要求 `cup`、`el`、`clear`、`sgr0`，缺少它们时明确失败；
  颜色、反色、备用屏幕和光标形状等可选能力缺失时则降级。
- 看 `backend-curses.c` 中 `begin()`、`text()`、`present()`：
  应用调用屏幕 API，库负责输出缓冲和差量刷新。
  每帧的 `werase()` 只清虚拟窗口；没有每帧调用强制清物理屏幕的 `wclear()`。

新增两个后端和测试后，总代码当然变多了。优化体现在职责的转移：
terminfo 接管终端能力适配，curses 进一步接管屏幕缓存和差量输出，
不是简单地“引入库就能让这个小程序总行数更少”。

本次刻意保留公共 `read_key()` 和 raw 模式教学路径。
完整 curses 应用还可以使用 `get_wch()`、`keypad()` 等接管输入；这里没有做，
因此不能把本实验当成任意历史终端的完整兼容层。
方向键解析和 bracketed paste 仍采用原来的 xterm 风格协议。
curses 光标使用 `curs_set(2/1)` 切换高可见/普通可见，不能保证 INSERT 模式一定是竖线；
terminfo 只有条目提供扩展能力 `Ss` 时才切换形状，ANSI 后端保留原来的显式形状。

## 操作

| 按键 | 操作 |
| --- | --- |
| `i` / `a` | 在光标处 / 当前字符后进入 INSERT |
| `o` | 在当前行下方插入新行，进入 INSERT |
| `Esc` | 回到 NORMAL |
| `h j k l` / 方向键 | 左、下、上、右；方向键也可在 INSERT 使用 |
| `0` / `$` | 行首 / 行末；Home、End 也可以使用 |
| `gg` / `G` | 文件开头 / 最后一行 |
| `x` / Delete | 删除光标处字符，不合并行 |
| `dd` | 删除当前行 |
| INSERT 中 Backspace | 删除前一个字符；在行首时合并行 |
| INSERT 中 Enter / Tab | 插入换行 / 制表符 |
| `:w` / `:w filename` | 保存 / 保存到指定路径 |
| `:wq` | 保存成功后退出 |
| `:q` / `:q!` | 退出 / 放弃未保存修改并退出 |
| `:help` | 在消息栏显示按键提示 |
| `Ctrl-C` | 收到 `0x03` 字节，取消当前模式并显示说明 |

支持基本 UTF-8 输入、中文移动和删除；终端的 bracketed paste 会被作为文本插入，
不会把粘贴内容中的 `:q!` 当成编辑器命令。

## 实验一：关掉 canonical 和 echo，谁来编辑？

启动后看状态栏（终端足够宽时）：

```text
NORMAL [-] | ansi | 1:1 | 24x80 | winch=0 | ICANON=0 ECHO=0 ISIG=0
```

1. 按 `i`，逐个输入 `abc`。每个按键立即触发重绘，不需要等回车。
2. 按 Backspace，再输入 `d`，得到 `abd`。
3. 按 Enter，编辑器自己往文档中插入 `\n`。
4. 按 `Ctrl-C`，程序继续运行，并说明自己收到了 `0x03`。

对应 `terminal_start()`：

- `ICANON=0`：内核不再替应用积攒和编辑完整输入行。
- `ECHO=0`：内核不再回显按键；屏幕上的字符是 `draw_screen()` 输出的。
- `ISIG=0`：这个终端不再把 `Ctrl-C` 转成 `SIGINT`；
  `handle_key()` 把它作为普通字节处理。
- `OPOST=0`：关闭输出后处理，输出字节由程序明确决定。

此时的退格来自 `previous_char()` 和 `delete_range()`，而不是内核
`n_tty.c` 中的 `eraser()`。

## 实验二：亲眼看见 Vim 风格界面的“绘制命令”

下面的序列表格以默认 ANSI 后端为例。切换 terminfo 后，日志中的序列来自数据库。

终端 A（日志路径需要尚不存在；重复实验时换一个名字）：

```bash
./mini-vim.out --trace /tmp/mini-vim-demo.trace example.txt
```

终端 B：

```bash
tail -f /tmp/mini-vim-demo.trace
```

回到 A 输入、移动光标、切换模式。B 中会显示类似：

```text
RAW: ICANON=0 ECHO=0 ISIG=0 OPOST=0 VMIN=1 VTIME=0
TX paste on 8 bytes: <ESC>[?2004h
START -> TIOCGWINSZ: rows=24 cols=80 (drawing 24x80)
RX 1 bytes: i
RX 1 bytes: a
RX 1 bytes: \x7f
```

`TX frame` 后面是程序实际输出的一整帧。日志用 `<ESC>` 表示单个 `0x1b`
字节，用 `\xNN` 表示非 ASCII 字节；这些记号本身没有发送给终端。
UTF-8 字符会出现多个输入字节。

| 输出序列 | 终端执行的操作 | 代码入口 |
| --- | --- | --- |
| `ESC[?1049h` / `ESC[?1049l` | 进入 / 退出备用屏幕，退出后恢复原 shell 画面 | `backend-ansi.c` 中 `start()` / `stop()` |
| `ESC[行;列H` | 移动光标，行列从 1 开始 | `backend-ansi.c` 中 `position()` |
| `ESC[K` | 清除光标到行末的旧内容 | `erase_line()` |
| `ESC[36m` / `ESC[0m` | 设置青色 / 恢复样式 | `style()` |
| `ESC[7m` | 反色，绘制状态栏 | `style()` |
| `ESC[?25l` / `ESC[?25h` | 绘制期间隐藏光标 / 显示光标 | `begin()` / `present()` |
| `ESC[2 q` / `ESC[6 q` | 方块光标 / 竖线光标，留意数字后的空格 | `present()` |
| `ESC[?2004h` / `ESC[?2004l` | 开启 / 关闭 bracketed paste 报告 | 启动 / 恢复终端 |

方向键也是协议。按向上键，通常会收到 `ESC [ A`，程序在 `read_key()`
里把它识别为 `KEY_UP`。单独的 Esc 和这些序列共享前缀，因此这里使用
50 ms 的等待区分；这也是小型演示实现的一个取舍。

日志含编辑内容和输入字节，创建权限为 `0600`。

curses 后端的 `--trace` 记录公共输入、尺寸和 `CURSES frame` 事件，
**不会把逻辑绘制事件冒充实际 TX 字节**，因为输出由 ncurses 内部完成。
要看真实输出，可以换一个未占用的日志路径运行：

```bash
strace -o /tmp/mini-vim-curses-syscalls.log -s 4096 -e trace=write,writev \
    ./mini-vim.out --backend curses example.txt
```

## 实验三：拖动窗口，观察 SIGWINCH

拖动终端 A 的窗口尺寸，观察：

- 状态栏的 `24x80` 等尺寸变化。
- `winch` 计数增加。
- 日志出现 `SIGWINCH -> TIOCGWINSZ: rows=... cols=...`。
- 文本显示区域和状态栏重新布局，光标保持可见。

路径为：

```text
终端模拟器设置 PTY 尺寸：ioctl(master, TIOCSWINSZ, ...)
    → 内核向前台进程组发 SIGWINCH
    → signal_handler() 只设置 resized 标志
    → 主循环调用 update_size() 查询新尺寸
    → draw_screen() 输出新的控制序列和文字
    → 终端模拟器解析并绘制
```

`SIGWINCH` 不携带尺寸。多个普通信号可能合并，计数表示应用处理了几次通知，
不是鼠标移动次数。过小的窗口会提示放大；恢复后可以继续编辑。

## 实验四：从系统调用看同一件事

```bash
strace -o /tmp/mini-vim-syscalls.log -s 256 \
    -e trace=read,write,ioctl,poll,rt_sigaction,rt_sigreturn \
    ./mini-vim.out example.txt
```

编辑并退出后查看日志，可找到：

- `ioctl(..., TCGETS, ...)` / `ioctl(..., TCSETSF, ...)`：读取属性、进入 raw 模式。
- `read(0, "i", 1)`：一个按键就能读到，不用回车。
- `write(1, "\33[...", ...)`：绘制序列与普通文本混在同一条字节流中。
- `SIGWINCH` 和随后的 `TIOCGWINSZ`：尺寸通知与查询。
- 退出时的 `TCSETS`：恢复原来的终端属性。

## 阅读代码的顺序

先读 `main()`，再沿下面两条路径看：

```text
输入：terminal_start → read_byte → read_key → handle_key → 修改文档
输出：draw_screen → cursor_to / draw_text → struct backend
      ansi / terminfo → backend_emit → backend_flush → write_all → PTY
      curses → ncurses 虚拟窗口 → wnoutrefresh / doupdate → PTY
      PTY → 终端模拟器
```

`trace_bytes()` 只是旁路记录，不参与协议转换。`draw_text()` 把文件中的
ESC、NUL 等控制字节显示为 `^[`、`^@`，避免文件内容被终端当成绘制命令。

## 范围与取舍

- 这是教学编辑器，没有撤销、搜索、语法高亮、计数命令或完整 Vim 兼容性。
  普通模式允许光标停在行末的插入位置，`$` 也移动到该位置。
- 每次输入事件重新计算可见区域，ANSI / terminfo 后端整帧输出，curses 提供差量刷新。
  长行采用横向滚动，制表符按 4 列展开，留出屏幕最后一列避免自动折行。
- 使用 UTF-8 locale 和 `wcwidth()` 计算宽度，不实现完整 Unicode grapheme
  cluster；组合 emoji 等复杂字符的删除和宽度可能与终端不一致。
  文档可编辑中文，冒号命令输入目前只接受 ASCII；中文文件名可通过启动参数传入。
- 最大文件 8 MiB。保留原始字节和有无末尾换行；已有 CRLF 中的 CR 显示为 `^M`，
  新输入的换行是 LF，不做整份文件的换行格式转换。
- 保存使用同目录临时文件、`fsync()` 和 `rename()`，写失败不截断原文件。
  新文件权限 `0600`，已有文件保留普通权限位；拒绝保存到符号链接和多硬链接文件。
  不保留 ACL、扩展属性、原 inode，也不检测外部并发修改；用于练习文件。
- 正常退出、输出错误、可处理的 INT/TERM/HUP/QUIT 信号会恢复终端属性。
  未实现作业挂起/恢复。SIGKILL 等不可处理的退出无法执行清理；需要时在 shell 运行
  `stty sane`，再用 `printf '\033[0m\033[?25h\033[?2004l\033[?1049l'` 恢复显示。
- ANSI 后端硬编码常用 xterm 风格序列；另两个后端使用 terminfo 数据库。
  三者共用的输入解析仍是小型演示实现，Vim 的真实兼容层比这里复杂。
  经前面的普通 socat 字节转发连接运行时，窗口尺寸也不会自动跨连接传播。

协议背景可查 [Vim terminal 文档](https://vimhelp.org/term.txt.html)、
[XTerm 控制序列](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html) 和
[TIOCGWINSZ/TIOCSWINSZ](https://man7.org/linux/man-pages/man2/TIOCSWINSZ.2const.html)。
库接口可查 [terminfo 低层 API](https://invisible-island.net/ncurses/man/curs_terminfo.3x.html)
与 [curses 屏幕刷新](https://invisible-island.net/ncurses/man/curs_refresh.3x.html)。

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
